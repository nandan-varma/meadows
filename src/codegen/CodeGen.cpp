#include "CodeGen.h"

#include <llvm/IR/Constants.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/TargetParser/Host.h>

#include <cstdint>

#include "../runtime/Limits.h"
#include "../sema/Builtins.h"

namespace meadows {

CodeGen::CodeGen(const SourceMap& sources)
    : sources_(sources), module_("meadows", context_), builder_(context_) {
  int32Type_ = llvm::Type::getInt32Ty(context_);
  pointerType_ = llvm::PointerType::get(context_, 0);
  llvm::Type* word = llvm::Type::getInt64Ty(context_);
  valueType_ = llvm::StructType::create(context_, {word, word}, "mrt.value");
  module_.setTargetTriple(llvm::sys::getDefaultTargetTriple());
}

std::string CodeGen::generate(StmtList& program) {
  declareFunctions(program);
  for (StmtPtr& stmt : program) {
    if (auto* function = dynamic_cast<FuncStmt*>(stmt.get())) {
      emitFunction(*function);
    }
  }
  emitMain(program);

  llvm::raw_string_ostream errors(verificationError_);
  if (llvm::verifyModule(module_, &errors)) {
    errors.flush();
    return {};
  }
  verificationError_.clear();

  std::string ir;
  llvm::raw_string_ostream out(ir);
  module_.print(out, nullptr);
  return ir;
}

llvm::Value* CodeGen::int32(int32_t value) {
  return llvm::ConstantInt::getSigned(int32Type_, value);
}

llvm::Value* CodeGen::slot() {
  llvm::IRBuilder<> allocas(entryBlock_, entryBlock_->begin());
  return allocas.CreateAlloca(valueType_);
}

llvm::Value* CodeGen::arrayOfSlots(size_t count) {
  if (count == 0) {
    return llvm::ConstantPointerNull::get(pointerType_);
  }
  llvm::IRBuilder<> allocas(entryBlock_, entryBlock_->begin());
  return allocas.CreateAlloca(llvm::ArrayType::get(valueType_, count));
}

llvm::Value* CodeGen::elementSlot(llvm::Value* array, size_t count,
                                  size_t index) {
  return builder_.CreateConstInBoundsGEP2_32(
      llvm::ArrayType::get(valueType_, count), array, 0,
      static_cast<unsigned>(index));
}

llvm::GlobalVariable* CodeGen::cString(const std::string& text) {
  auto found = cStrings_.find(text);
  if (found != cStrings_.end()) {
    return found->second;
  }
  llvm::Constant* data = llvm::ConstantDataArray::getString(context_, text);
  auto* global = new llvm::GlobalVariable(module_, data->getType(), true,
                                          llvm::GlobalValue::PrivateLinkage,
                                          data);
  global->setUnnamedAddr(llvm::GlobalValue::UnnamedAddr::Global);
  return cStrings_[text] = global;
}

llvm::GlobalVariable* CodeGen::stringLiteral(const std::string& text) {
  auto found = stringLiterals_.find(text);
  if (found != stringLiterals_.end()) {
    return found->second;
  }
  llvm::Constant* bytes = llvm::ConstantDataArray::getString(
      context_, llvm::StringRef(text.data(), text.size()));
  llvm::StructType* type = llvm::StructType::get(
      context_, {int32Type_, int32Type_, bytes->getType()});
  llvm::Constant* initializer = llvm::ConstantStruct::get(
      type, {llvm::ConstantInt::get(int32Type_, UINT32_MAX),
             llvm::ConstantInt::get(int32Type_, text.size()), bytes});
  auto* global = new llvm::GlobalVariable(
      module_, type, true, llvm::GlobalValue::PrivateLinkage, initializer);
  global->setUnnamedAddr(llvm::GlobalValue::UnnamedAddr::Global);
  return stringLiterals_[text] = global;
}

llvm::GlobalVariable* CodeGen::shape(const std::vector<ObjectField>& fields) {
  std::vector<llvm::Constant*> names;
  for (const ObjectField& field : fields) {
    names.push_back(cString(field.name));
  }
  llvm::ArrayType* namesType = llvm::ArrayType::get(pointerType_, names.size());
  llvm::StructType* type = llvm::StructType::get(
      context_, {int32Type_, int32Type_, namesType});
  llvm::Constant* initializer = llvm::ConstantStruct::get(
      type, {llvm::ConstantInt::get(int32Type_, names.size()),
             llvm::ConstantInt::get(int32Type_, 0),
             llvm::ConstantArray::get(namesType, names)});
  auto* global = new llvm::GlobalVariable(
      module_, type, true, llvm::GlobalValue::PrivateLinkage, initializer);
  global->setUnnamedAddr(llvm::GlobalValue::UnnamedAddr::Global);
  return global;
}

llvm::Value* CodeGen::site(const Location& location) {
  auto key = std::make_tuple(location.file, location.line, location.column);
  auto found = sites_.find(key);
  if (found != sites_.end()) {
    return found->second;
  }
  llvm::StructType* type = llvm::StructType::get(
      context_, {pointerType_, int32Type_, int32Type_});
  llvm::Constant* initializer = llvm::ConstantStruct::get(
      type, {cString(sources_.path(location.file)),
             llvm::ConstantInt::get(int32Type_, location.line),
             llvm::ConstantInt::get(int32Type_, location.column)});
  auto* global = new llvm::GlobalVariable(
      module_, type, true, llvm::GlobalValue::PrivateLinkage, initializer);
  global->setUnnamedAddr(llvm::GlobalValue::UnnamedAddr::Global);
  return sites_[key] = global;
}

llvm::CallInst* CodeGen::runtime(const char* name,
                                 std::vector<llvm::Value*> args,
                                 bool returnsInt) {
  std::vector<llvm::Type*> types;
  for (llvm::Value* arg : args) {
    types.push_back(arg->getType());
  }
  llvm::FunctionType* type = llvm::FunctionType::get(
      returnsInt ? static_cast<llvm::Type*>(int32Type_)
                 : llvm::Type::getVoidTy(context_),
      types, false);
  return builder_.CreateCall(module_.getOrInsertFunction(name, type), args);
}

void CodeGen::move(llvm::Value* target, llvm::Value* source) {
  builder_.CreateStore(builder_.CreateLoad(valueType_, source), target);
}

void CodeGen::release(llvm::Value* value) { runtime("mrt_release", {value}); }

llvm::Value* CodeGen::intSlot(int32_t value) {
  llvm::Value* out = slot();
  runtime("mrt_make_int", {out, int32(value)});
  return out;
}

llvm::Value* CodeGen::eval(Expr& expr) {
  expr.accept(*this);
  return result_;
}

llvm::Value* CodeGen::evalIntoArray(std::vector<ExprPtr>& exprs, size_t count) {
  llvm::Value* array = arrayOfSlots(count);
  for (size_t i = 0; i < count; ++i) {
    llvm::Value* value = eval(*exprs[i]);
    move(elementSlot(array, count, i), value);
  }
  return array;
}

bool CodeGen::terminated() const {
  return builder_.GetInsertBlock()->getTerminator() != nullptr;
}

void CodeGen::popScope() {
  if (!terminated()) {
    const std::vector<Local>& locals = scopes_.back();
    for (auto local = locals.rbegin(); local != locals.rend(); ++local) {
      release(local->slot);
    }
  }
  scopes_.pop_back();
}

void CodeGen::declare(const std::string& name, llvm::Value* value) {
  scopes_.back().push_back({name, value});
}

llvm::Value* CodeGen::findLocal(const std::string& name) const {
  for (auto scope = scopes_.rbegin(); scope != scopes_.rend(); ++scope) {
    for (auto local = scope->rbegin(); local != scope->rend(); ++local) {
      if (local->name == name) {
        return local->slot;
      }
    }
  }
  return nullptr;
}

void CodeGen::releaseDownTo(size_t depth) {
  for (size_t i = scopes_.size(); i > depth; --i) {
    const std::vector<Local>& locals = scopes_[i - 1];
    for (auto local = locals.rbegin(); local != locals.rend(); ++local) {
      release(local->slot);
    }
  }
}

void CodeGen::emitBody(StmtList& body) {
  for (StmtPtr& stmt : body) {
    if (terminated()) {
      return;
    }
    stmt->accept(*this);
  }
}

void CodeGen::emitScopedBody(StmtList& body) {
  pushScope();
  emitBody(body);
  popScope();
}

void CodeGen::emitReturn(llvm::Value* valueSlot) {
  move(returnSlot_, valueSlot);
  releaseDownTo(0);
  builder_.CreateRetVoid();
}

void CodeGen::declareFunctions(StmtList& program) {
  llvm::Type* voidType = llvm::Type::getVoidTy(context_);
  llvm::FunctionType* signature = llvm::FunctionType::get(
      voidType, {pointerType_, pointerType_}, false);
  llvm::StructType* descriptorType = llvm::StructType::get(
      context_, {pointerType_, int32Type_, pointerType_});

  for (StmtPtr& stmt : program) {
    auto* function = dynamic_cast<FuncStmt*>(stmt.get());
    if (!function) {
      continue;
    }
    llvm::Function* code = llvm::Function::Create(
        signature, llvm::GlobalValue::InternalLinkage, "ms." + function->name,
        module_);
    llvm::Constant* initializer = llvm::ConstantStruct::get(
        descriptorType,
        {code, llvm::ConstantInt::get(int32Type_, function->params.size()),
         cString(function->name)});
    auto* descriptor = new llvm::GlobalVariable(
        module_, descriptorType, true, llvm::GlobalValue::PrivateLinkage,
        initializer);
    functions_[function->name] = {code, descriptor};
  }
}

void CodeGen::emitFunction(FuncStmt& stmt) {
  llvm::Function* function = functions_.at(stmt.name).function;
  entryBlock_ = llvm::BasicBlock::Create(context_, "entry", function);
  builder_.SetInsertPoint(entryBlock_);
  returnSlot_ = function->getArg(0);
  llvm::Value* args = function->getArg(1);
  scopes_.clear();
  loops_.clear();
  pushScope();
  for (size_t i = 0; i < stmt.params.size(); ++i) {
    declare(stmt.params[i].name, elementSlot(args, stmt.params.size(), i));
  }
  emitBody(stmt.body);
  if (!terminated()) {
    emitReturn(intSlot(0));
  }
  scopes_.clear();
}

void CodeGen::emitMain(StmtList& program) {
  llvm::FunctionType* entryType =
      llvm::FunctionType::get(llvm::Type::getVoidTy(context_), false);
  llvm::Function* entry = llvm::Function::Create(
      entryType, llvm::GlobalValue::InternalLinkage, "meadows.main", module_);
  entryBlock_ = llvm::BasicBlock::Create(context_, "entry", entry);
  builder_.SetInsertPoint(entryBlock_);
  returnSlot_ = nullptr;
  scopes_.clear();
  loops_.clear();
  pushScope();
  emitBody(program);
  popScope();
  builder_.CreateRetVoid();

  llvm::FunctionType* mainType = llvm::FunctionType::get(int32Type_, false);
  llvm::Function* main = llvm::Function::Create(
      mainType, llvm::GlobalValue::ExternalLinkage, "main", module_);
  builder_.SetInsertPoint(llvm::BasicBlock::Create(context_, "entry", main));
  llvm::Value* status = builder_.CreateCall(
      module_.getOrInsertFunction(
          "mrt_run", llvm::FunctionType::get(int32Type_, {pointerType_}, false)),
      {entry});
  builder_.CreateRet(status);
}

llvm::Value* CodeGen::emitCondition(Expr& condition) {
  llvm::Value* value = eval(condition);
  llvm::Value* flag =
      runtime("mrt_condition", {value, site(condition.location)}, true);
  release(value);
  return builder_.CreateICmpNE(flag, int32(0));
}

void CodeGen::visitLiteralExpr(LiteralExpr& expr) {
  llvm::Value* out = slot();
  switch (expr.kind) {
  case LiteralKind::Int:
    runtime("mrt_make_int", {out, int32(expr.intValue)});
    break;
  case LiteralKind::Float:
    runtime("mrt_make_float",
            {out, llvm::ConstantFP::get(context_, llvm::APFloat(expr.floatValue))});
    break;
  case LiteralKind::String:
    runtime("mrt_make_string", {out, stringLiteral(expr.stringValue)});
    break;
  }
  result_ = out;
}

void CodeGen::visitVarExpr(VarExpr& expr) {
  llvm::Value* out = slot();
  if (llvm::Value* local = findLocal(expr.name)) {
    runtime("mrt_copy", {out, local});
  } else {
    runtime("mrt_make_function", {out, functions_.at(expr.name).descriptor});
  }
  result_ = out;
}

void CodeGen::visitAssignExpr(AssignExpr& expr) {
  if (auto* target = dynamic_cast<VarExpr*>(expr.target.get())) {
    llvm::Value* value = eval(*expr.value);
    llvm::Value* variable = findLocal(target->name);
    runtime("mrt_store", {variable, value});
    llvm::Value* out = slot();
    runtime("mrt_copy", {out, variable});
    result_ = out;
    return;
  }

  if (auto* target = dynamic_cast<IndexExpr*>(expr.target.get())) {
    llvm::Value* object = eval(*target->object);
    llvm::Value* index = eval(*target->index);
    llvm::Value* value = eval(*expr.value);
    runtime("mrt_index_set", {object, index, value, site(target->location)});
    release(object);
    release(index);
    result_ = value;
    return;
  }

  auto& target = static_cast<FieldExpr&>(*expr.target);
  llvm::Value* object = eval(*target.object);
  llvm::Value* value = eval(*expr.value);
  runtime("mrt_field_set",
          {object, cString(target.name), value, site(target.location)});
  release(object);
  result_ = value;
}

void CodeGen::visitBinaryExpr(BinaryExpr& expr) {
  llvm::Value* left = eval(*expr.left);
  llvm::Value* right = eval(*expr.right);
  llvm::Value* out = slot();
  llvm::Value* where = site(expr.location);
  switch (expr.op) {
  case BinaryOp::Add: runtime("mrt_add", {out, left, right, where}); break;
  case BinaryOp::Sub: runtime("mrt_sub", {out, left, right, where}); break;
  case BinaryOp::Mul: runtime("mrt_mul", {out, left, right, where}); break;
  case BinaryOp::Div: runtime("mrt_div", {out, left, right, where}); break;
  case BinaryOp::Mod: runtime("mrt_mod", {out, left, right, where}); break;
  case BinaryOp::Eq: runtime("mrt_eq", {out, left, right}); break;
  case BinaryOp::Ne: runtime("mrt_ne", {out, left, right}); break;
  case BinaryOp::Lt: runtime("mrt_lt", {out, left, right, where}); break;
  case BinaryOp::Le: runtime("mrt_le", {out, left, right, where}); break;
  case BinaryOp::Gt: runtime("mrt_gt", {out, left, right, where}); break;
  case BinaryOp::Ge: runtime("mrt_ge", {out, left, right, where}); break;
  }
  release(left);
  release(right);
  result_ = out;
}

void CodeGen::visitUnaryExpr(UnaryExpr& expr) {
  llvm::Value* operand = eval(*expr.operand);
  llvm::Value* out = slot();
  runtime(expr.op == UnaryOp::Negate ? "mrt_neg" : "mrt_not",
          {out, operand, site(expr.location)});
  release(operand);
  result_ = out;
}

void CodeGen::visitLogicalExpr(LogicalExpr& expr) {
  bool isAnd = expr.op == LogicalOp::And;
  llvm::Function* function = builder_.GetInsertBlock()->getParent();
  llvm::Value* out = slot();
  llvm::Value* where = site(expr.location);

  auto test = [&](Expr& operand) {
    llvm::Value* value = eval(operand);
    llvm::Value* flag =
        runtime("mrt_logical", {value, int32(isAnd), where}, true);
    release(value);
    return flag;
  };

  llvm::Value* left = test(*expr.left);
  llvm::BasicBlock* rightBlock = llvm::BasicBlock::Create(context_, "rhs", function);
  llvm::BasicBlock* shortBlock = llvm::BasicBlock::Create(context_, "short", function);
  llvm::BasicBlock* endBlock = llvm::BasicBlock::Create(context_, "end", function);
  llvm::Value* leftTrue = builder_.CreateICmpNE(left, int32(0));
  builder_.CreateCondBr(leftTrue, isAnd ? rightBlock : shortBlock,
                        isAnd ? shortBlock : rightBlock);

  builder_.SetInsertPoint(rightBlock);
  llvm::Value* right = test(*expr.right);
  runtime("mrt_make_int", {out, right});
  builder_.CreateBr(endBlock);

  builder_.SetInsertPoint(shortBlock);
  runtime("mrt_make_int", {out, int32(isAnd ? 0 : 1)});
  builder_.CreateBr(endBlock);

  builder_.SetInsertPoint(endBlock);
  result_ = out;
}

void CodeGen::visitIndexExpr(IndexExpr& expr) {
  llvm::Value* object = eval(*expr.object);
  llvm::Value* index = eval(*expr.index);
  llvm::Value* out = slot();
  runtime("mrt_index", {out, object, index, site(expr.location)});
  release(object);
  release(index);
  result_ = out;
}

void CodeGen::visitFieldExpr(FieldExpr& expr) {
  llvm::Value* object = eval(*expr.object);
  llvm::Value* out = slot();
  runtime("mrt_field", {out, object, cString(expr.name), site(expr.location)});
  release(object);
  result_ = out;
}

void CodeGen::emitBuiltin(const std::string& name, CallExpr& expr) {
  llvm::Value* where = site(expr.location);
  llvm::Value* out = slot();
  std::vector<llvm::Value*> args;
  for (ExprPtr& arg : expr.args) {
    args.push_back(eval(*arg));
  }
  switch (findBuiltin(name)->builtin) {
  case Builtin::Print:
    runtime("mrt_print", {args[0], where});
    runtime("mrt_make_int", {out, int32(0)});
    break;
  case Builtin::Len:
    runtime("mrt_len", {out, args[0], where});
    break;
  case Builtin::Str:
    runtime("mrt_str", {out, args[0], where});
    break;
  case Builtin::Push:
    runtime("mrt_push", {out, args[0], args[1], where});
    break;
  }
  for (llvm::Value* arg : args) {
    release(arg);
  }
  result_ = out;
}

void CodeGen::emitCall(CallExpr& expr) {
  auto* name = dynamic_cast<VarExpr*>(expr.callee.get());
  bool isLocal = name && findLocal(name->name);
  if (name && !isLocal && findBuiltin(name->name)) {
    emitBuiltin(name->name, expr);
    return;
  }

  llvm::Value* where = site(expr.location);
  bool isDirect = name && !isLocal;
  llvm::Value* callee = isDirect ? nullptr : eval(*expr.callee);
  size_t count = expr.args.size();
  llvm::Value* args = evalIntoArray(expr.args, count);
  llvm::Value* out = slot();

  if (isDirect) {
    runtime("mrt_enter", {where});
    builder_.CreateCall(functions_.at(name->name).function, {out, args});
    runtime("mrt_leave", {});
  } else {
    runtime("mrt_call", {out, callee, args, int32(static_cast<int32_t>(count)), where});
    release(callee);
  }
  result_ = out;
}

void CodeGen::visitCallExpr(CallExpr& expr) { emitCall(expr); }

void CodeGen::visitArrayExpr(ArrayExpr& expr) {
  size_t count = expr.elements.size();
  llvm::Value* items = evalIntoArray(expr.elements, count);
  llvm::Value* out = slot();
  runtime("mrt_make_array", {out, items, int32(static_cast<int32_t>(count))});
  result_ = out;
}

void CodeGen::visitObjectExpr(ObjectExpr& expr) {
  size_t count = expr.fields.size();
  llvm::Value* fields = arrayOfSlots(count);
  for (size_t i = 0; i < count; ++i) {
    llvm::Value* value = eval(*expr.fields[i].value);
    move(elementSlot(fields, count, i), value);
  }
  llvm::Value* out = slot();
  runtime("mrt_make_object", {out, shape(expr.fields), fields});
  result_ = out;
}

void CodeGen::visitExprStmt(ExprStmt& stmt) { release(eval(*stmt.expr)); }

void CodeGen::visitLetStmt(LetStmt& stmt) {
  declare(stmt.name, eval(*stmt.initializer));
}

void CodeGen::visitFuncStmt(FuncStmt&) {}

void CodeGen::visitIfStmt(IfStmt& stmt) {
  llvm::Function* function = builder_.GetInsertBlock()->getParent();
  llvm::Value* flag = emitCondition(*stmt.condition);
  llvm::BasicBlock* thenBlock = llvm::BasicBlock::Create(context_, "then", function);
  llvm::BasicBlock* elseBlock = llvm::BasicBlock::Create(context_, "else", function);
  llvm::BasicBlock* endBlock = llvm::BasicBlock::Create(context_, "endif", function);
  builder_.CreateCondBr(flag, thenBlock, elseBlock);

  builder_.SetInsertPoint(thenBlock);
  emitScopedBody(stmt.thenBody);
  if (!terminated()) {
    builder_.CreateBr(endBlock);
  }

  builder_.SetInsertPoint(elseBlock);
  emitScopedBody(stmt.elseBody);
  if (!terminated()) {
    builder_.CreateBr(endBlock);
  }

  builder_.SetInsertPoint(endBlock);
}

void CodeGen::visitWhileStmt(WhileStmt& stmt) {
  llvm::Function* function = builder_.GetInsertBlock()->getParent();
  llvm::BasicBlock* conditionBlock = llvm::BasicBlock::Create(context_, "while", function);
  llvm::BasicBlock* bodyBlock = llvm::BasicBlock::Create(context_, "body", function);
  llvm::BasicBlock* endBlock = llvm::BasicBlock::Create(context_, "endwhile", function);
  builder_.CreateBr(conditionBlock);

  builder_.SetInsertPoint(conditionBlock);
  builder_.CreateCondBr(emitCondition(*stmt.condition), bodyBlock, endBlock);

  builder_.SetInsertPoint(bodyBlock);
  pushScope();
  loops_.push_back({scopes_.size() - 1, endBlock, conditionBlock});
  emitBody(stmt.body);
  loops_.pop_back();
  popScope();
  if (!terminated()) {
    builder_.CreateBr(conditionBlock);
  }

  builder_.SetInsertPoint(endBlock);
}

void CodeGen::visitForStmt(ForStmt& stmt) {
  llvm::Function* function = builder_.GetInsertBlock()->getParent();
  llvm::Value* start = eval(*stmt.start);
  llvm::Value* end = eval(*stmt.end);
  llvm::IRBuilder<> allocas(entryBlock_, entryBlock_->begin());
  llvm::Value* current = allocas.CreateAlloca(int32Type_);
  llvm::Value* limit = allocas.CreateAlloca(int32Type_);
  runtime("mrt_range", {start, end, current, limit, site(stmt.location)});
  release(start);
  release(end);
  llvm::Value* variable = slot();

  llvm::BasicBlock* conditionBlock = llvm::BasicBlock::Create(context_, "for", function);
  llvm::BasicBlock* bodyBlock = llvm::BasicBlock::Create(context_, "body", function);
  llvm::BasicBlock* stepBlock = llvm::BasicBlock::Create(context_, "step", function);
  llvm::BasicBlock* endBlock = llvm::BasicBlock::Create(context_, "endfor", function);
  builder_.CreateBr(conditionBlock);

  builder_.SetInsertPoint(conditionBlock);
  llvm::Value* inRange = builder_.CreateICmpSLT(
      builder_.CreateLoad(int32Type_, current),
      builder_.CreateLoad(int32Type_, limit));
  builder_.CreateCondBr(inRange, bodyBlock, endBlock);

  builder_.SetInsertPoint(bodyBlock);
  pushScope();
  runtime("mrt_make_int", {variable, builder_.CreateLoad(int32Type_, current)});
  declare(stmt.var, variable);
  loops_.push_back({scopes_.size() - 1, endBlock, stepBlock});
  emitBody(stmt.body);
  loops_.pop_back();
  popScope();
  if (!terminated()) {
    builder_.CreateBr(stepBlock);
  }

  builder_.SetInsertPoint(stepBlock);
  builder_.CreateStore(
      builder_.CreateAdd(builder_.CreateLoad(int32Type_, current), int32(1)),
      current);
  builder_.CreateBr(conditionBlock);

  builder_.SetInsertPoint(endBlock);
}

void CodeGen::visitReturnStmt(ReturnStmt& stmt) {
  emitReturn(stmt.value ? eval(*stmt.value) : intSlot(0));
}

void CodeGen::visitBreakStmt(BreakStmt&) {
  releaseDownTo(loops_.back().scopeDepth);
  builder_.CreateBr(loops_.back().breakBlock);
}

void CodeGen::visitContinueStmt(ContinueStmt&) {
  releaseDownTo(loops_.back().scopeDepth);
  builder_.CreateBr(loops_.back().continueBlock);
}

void CodeGen::visitBlockStmt(BlockStmt& stmt) { emitScopedBody(stmt.body); }

} // namespace meadows
