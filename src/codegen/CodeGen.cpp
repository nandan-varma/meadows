#include "CodeGen.h"
#include "StringUtils.h"
#include "SymbolTable.h"
#include <algorithm>
#include <climits>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/PassManager.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Support/Casting.h>
#include <sstream>
#include <stdexcept>

constexpr int INT32_BIT_WIDTH = 32;

void CodeGen::enterScope() { symbolTable.enterScope(); }

void CodeGen::exitScope() { symbolTable.exitScope(); }

void CodeGen::declareVariable(const std::string &name, llvm::Value *value) {
  symbolTable.declare(name, value);
}

llvm::Value *CodeGen::lookupVariable(const std::string &name) {
  return symbolTable.lookup(name);
}

bool CodeGen::variableExists(const std::string &name) {
  return symbolTable.exists(name);
}

void CodeGen::validateDivision(llvm::Value *divisor) {
  auto *zero = llvm::ConstantInt::get(divisor->getType(), 0);
  auto *isZero = builder->CreateICmpEQ(divisor, zero, "div_is_zero");
  auto *divErrorBB =
      llvm::BasicBlock::Create(*context, "div_error", currentFunction);
  auto *continueBB =
      llvm::BasicBlock::Create(*context, "div_continue", currentFunction);

  builder->CreateCondBr(isZero, divErrorBB, continueBB);
  builder->SetInsertPoint(divErrorBB);

  generateRuntimeError("RuntimeError: Division by zero\n");

  builder->SetInsertPoint(continueBB);
}

void CodeGen::validateArrayBounds(llvm::Value *array, llvm::Value *index) {
  auto *zero = llvm::ConstantInt::get(index->getType(), 0);
  auto *isNegative = builder->CreateICmpSLT(index, zero, "idx_negative");

  auto *arrayLen =
      builder->CreateLoad(llvm::Type::getInt32Ty(*context), array, "array_len");
  auto *isOutOfBounds = builder->CreateICmpSGE(index, arrayLen, "idx_oob");

  auto *isInvalid = builder->CreateOr(isNegative, isOutOfBounds, "idx_invalid");

  auto *boundsErrorBB =
      llvm::BasicBlock::Create(*context, "bounds_error", currentFunction);
  auto *continueBB =
      llvm::BasicBlock::Create(*context, "bounds_continue", currentFunction);

  builder->CreateCondBr(isInvalid, boundsErrorBB, continueBB);
  builder->SetInsertPoint(boundsErrorBB);

  generateRuntimeError("RuntimeError: Array index out of bounds\n");

  builder->SetInsertPoint(continueBB);
}

void CodeGen::generateRuntimeError(const std::string &message) {
  auto *errorMsg = builder->CreateGlobalStringPtr(message);
  auto *format = builder->CreateGlobalString("%s");
  builder->CreateCall(printfFunc, {format, errorMsg});
  builder->CreateRet(
      llvm::ConstantInt::get(llvm::Type::getInt32Ty(*context), -1));
}

CodeGen::CodeGen(int optLevel) : optLevel_(optLevel) {
  context = std::make_unique<llvm::LLVMContext>();
  module = std::make_unique<llvm::Module>("meadows", *context);
  builder = std::make_unique<llvm::IRBuilder<>>(*context);

  auto printfType = llvm::FunctionType::get(
      llvm::Type::getInt32Ty(*context),
      {llvm::PointerType::get(llvm::Type::getInt8Ty(*context), 0)}, true);
  printfFunc = llvm::cast<llvm::Function>(
      module->getOrInsertFunction("printf", printfType).getCallee());

  auto mallocType = llvm::FunctionType::get(
      llvm::PointerType::get(llvm::Type::getInt8Ty(*context), 0),
      {llvm::Type::getInt64Ty(*context)}, false);
  mallocFunc = llvm::cast<llvm::Function>(
      module->getOrInsertFunction("malloc", mallocType).getCallee());

  auto strlenType = llvm::FunctionType::get(
      llvm::Type::getInt64Ty(*context),
      {llvm::PointerType::get(llvm::Type::getInt8Ty(*context), 0)}, false);
  strlenFunc = llvm::cast<llvm::Function>(
      module->getOrInsertFunction("strlen", strlenType).getCallee());

  auto strcpyType = llvm::FunctionType::get(
      llvm::PointerType::get(llvm::Type::getInt8Ty(*context), 0),
      {llvm::PointerType::get(llvm::Type::getInt8Ty(*context), 0),
       llvm::PointerType::get(llvm::Type::getInt8Ty(*context), 0)},
      false);
  module->getOrInsertFunction("strcpy", strcpyType);

  auto strcatType = llvm::FunctionType::get(
      llvm::PointerType::get(llvm::Type::getInt8Ty(*context), 0),
      {llvm::PointerType::get(llvm::Type::getInt8Ty(*context), 0),
       llvm::PointerType::get(llvm::Type::getInt8Ty(*context), 0)},
      false);
  module->getOrInsertFunction("strcat", strcatType);

  // strcmp — used for string == / != so equality compares content, not the
  // pointer value (two equal string literals aren't guaranteed the same
  // global constant, and two runtime-built strings never are).
  auto strcmpType = llvm::FunctionType::get(
      llvm::Type::getInt32Ty(*context),
      {llvm::PointerType::get(llvm::Type::getInt8Ty(*context), 0),
       llvm::PointerType::get(llvm::Type::getInt8Ty(*context), 0)},
      false);
  module->getOrInsertFunction("strcmp", strcmpType);

  auto freeType = llvm::FunctionType::get(
      llvm::Type::getVoidTy(*context),
      {llvm::PointerType::get(llvm::Type::getInt8Ty(*context), 0)}, false);
  freeFunc = llvm::cast<llvm::Function>(
      module->getOrInsertFunction("free", freeType).getCallee());

  // snprintf(buf, size, fmt, ...) — used by the str() builtin to format ints.
  auto snprintfType = llvm::FunctionType::get(
      llvm::Type::getInt32Ty(*context),
      {llvm::PointerType::get(llvm::Type::getInt8Ty(*context), 0),
       llvm::Type::getInt64Ty(*context),
       llvm::PointerType::get(llvm::Type::getInt8Ty(*context), 0)},
      true);
  module->getOrInsertFunction("snprintf", snprintfType);

  currentFunction = nullptr;
  exprResult = nullptr;
  currentBlock = nullptr;
}

void CodeGen::declareFunctionSignatures(
    const std::vector<std::unique_ptr<Stmt>> &statements) {
  for (auto &stmt : statements) {
    auto *fn = dynamic_cast<FuncStmt *>(stmt.get());
    if (!fn) continue;

    std::vector<llvm::Type *> paramTypes(fn->params.size(),
                                         llvm::Type::getInt32Ty(*context));
    auto funcType = llvm::FunctionType::get(llvm::Type::getInt32Ty(*context),
                                            paramTypes, false);
    llvm::Function::Create(funcType, llvm::Function::ExternalLinkage, fn->name,
                           module.get());
  }
}

void CodeGen::generate(const std::vector<std::unique_ptr<Stmt>> &statements) {
  variableScopeStack.clear();

  auto mainType =
      llvm::FunctionType::get(llvm::Type::getInt32Ty(*context), {}, false);
  auto mainFunc = llvm::Function::Create(
      mainType, llvm::Function::ExternalLinkage, "main", module.get());
  auto entry = llvm::BasicBlock::Create(*context, "entry", mainFunc);
  builder->SetInsertPoint(entry);
  currentFunction = mainFunc;
  currentBlock = entry;

  enterScope();

  declareFunctionSignatures(statements);

  for (auto &stmt : statements) {
    stmt->accept(*this);
  }

  exitScope();

  freeAllocatedStrings();

  builder->CreateRet(
      llvm::ConstantInt::get(llvm::Type::getInt32Ty(*context), 0));

  if (optLevel_ > 0) {
    runOptimizationPasses();
  }
}

void CodeGen::runOptimizationPasses() {
  llvm::OptimizationLevel level = llvm::OptimizationLevel::O1;
  if (optLevel_ == 2) level = llvm::OptimizationLevel::O2;
  else if (optLevel_ >= 3) level = llvm::OptimizationLevel::O3;

  llvm::PassBuilder pb;
  llvm::LoopAnalysisManager lam;
  llvm::FunctionAnalysisManager fam;
  llvm::CGSCCAnalysisManager cgam;
  llvm::ModuleAnalysisManager mam;
  pb.registerModuleAnalyses(mam);
  pb.registerCGSCCAnalyses(cgam);
  pb.registerFunctionAnalyses(fam);
  pb.registerLoopAnalyses(lam);
  pb.crossRegisterProxies(lam, fam, cgam, mam);
  auto mpm = pb.buildPerModuleDefaultPipeline(level);
  mpm.run(*module, mam);
}

void CodeGen::freeAllocatedStrings() {
  for (auto ptr : allocatedStrings) {
    builder->CreateCall(freeFunc, {ptr});
  }
  allocatedStrings.clear();
}

std::unique_ptr<llvm::Module> CodeGen::getModule() { return std::move(module); }

void CodeGen::visitLiteralExpr(LiteralExpr &expr) {
  if (!expr.value.empty() && isdigit(expr.value[0])) {
    try {
      exprResult = llvm::ConstantInt::get(
          llvm::Type::getInt32Ty(*context),
          llvm::APInt(INT32_BIT_WIDTH, std::stoi(expr.value)));
    } catch (const std::out_of_range &) {
      constexpr int32_t MAX_I32_VALUE = INT32_MAX;
      exprResult =
          llvm::ConstantInt::get(llvm::Type::getInt32Ty(*context),
                                 llvm::APInt(INT32_BIT_WIDTH, MAX_I32_VALUE));
    }
  } else {
    StringUtils::StringPool::getInstance().intern(expr.value);
    exprResult = builder->CreateGlobalStringPtr(expr.value);
  }
}

void CodeGen::visitVarExpr(VarExpr &expr) {
  llvm::Value *var = lookupVariable(expr.name);
  if (!var) {
    error("Undefined variable: ", expr.name);
  }

  auto typeIt = variableTypes.find(expr.name);
  llvm::Type *loadType = (typeIt != variableTypes.end())
                             ? typeIt->second
                             : llvm::Type::getInt32Ty(*context);
  exprResult = builder->CreateLoad(loadType, var);
}

void CodeGen::visitAssignExpr(AssignExpr &expr) {
  if (auto *varTarget = dynamic_cast<VarExpr *>(expr.target.get())) {
    expr.value->accept(*this);
    auto val = exprResult;

    llvm::Value *var = lookupVariable(varTarget->name);
    if (!var) {
      error("Undefined variable in assignment: ", varTarget->name);
    }

    builder->CreateStore(val, var);
    exprResult = val;
    return;
  }

  if (auto *indexTarget = dynamic_cast<IndexExpr *>(expr.target.get())) {
    indexTarget->array->accept(*this);
    auto arrayPtr = exprResult;
    indexTarget->index->accept(*this);
    auto indexVal = exprResult;
    validateArrayBounds(arrayPtr, indexVal);

    expr.value->accept(*this);
    auto val = exprResult;

    auto arrayType = llvm::ArrayType::get(llvm::Type::getInt32Ty(*context), 0);
    auto zero = llvm::ConstantInt::get(llvm::Type::getInt32Ty(*context), 0);
    std::vector<llvm::Value *> indices = {zero, indexVal};
    auto elementPtr =
        builder->CreateGEP(arrayType, arrayPtr, indices, "arrayidxassign");
    builder->CreateStore(val, elementPtr);
    exprResult = val;
    return;
  }

  if (auto *fieldTarget = dynamic_cast<FieldAccessExpr *>(expr.target.get())) {
    fieldTarget->object->accept(*this);
    auto objPtr = exprResult;

    llvm::StructType *structType = nullptr;
    size_t fieldIndex = 0;
    resolveFieldAccess(*fieldTarget, structType, fieldIndex);

    expr.value->accept(*this);
    auto val = exprResult;

    std::vector<llvm::Value *> indices = {
        llvm::ConstantInt::get(llvm::Type::getInt32Ty(*context), 0),
        llvm::ConstantInt::get(llvm::Type::getInt32Ty(*context),
                               static_cast<int>(fieldIndex))};
    auto fieldPtr =
        builder->CreateGEP(structType, objPtr, indices, "fieldassign");
    builder->CreateStore(val, fieldPtr);
    exprResult = val;
    return;
  }

  error("Unsupported assignment target");
}

void CodeGen::visitBinaryExpr(BinaryExpr &expr) {
  expr.left->accept(*this);
  auto left = exprResult;
  expr.right->accept(*this);
  auto right = exprResult;

  if (expr.op == "+") {
    if (TypeUtils::isPointerType(left) || TypeUtils::isPointerType(right)) {
      exprResult = concatenateStrings(left, right);
    } else {
      exprResult = builder->CreateAdd(left, right);
    }
  } else if (expr.op == "-") {
    exprResult = builder->CreateSub(left, right);
  } else if (expr.op == "*") {
    exprResult = builder->CreateMul(left, right);
  } else if (expr.op == "/") {
    validateDivision(right);
    exprResult = builder->CreateSDiv(left, right);
  } else if (expr.op == "%") {
    validateDivision(right);
    exprResult = builder->CreateSRem(left, right);
  } else if (expr.op == "==") {
    if (TypeUtils::isPointerType(left) && TypeUtils::isPointerType(right)) {
      exprResult = compareStrings(left, right, /*equal=*/true);
    } else {
      exprResult = builder->CreateICmpEQ(left, right);
    }
  } else if (expr.op == "!=") {
    if (TypeUtils::isPointerType(left) && TypeUtils::isPointerType(right)) {
      exprResult = compareStrings(left, right, /*equal=*/false);
    } else {
      exprResult = builder->CreateICmpNE(left, right);
    }
  } else if (expr.op == ">") {
    exprResult = builder->CreateICmpSGT(left, right);
  } else if (expr.op == "<") {
    exprResult = builder->CreateICmpSLT(left, right);
  } else if (expr.op == ">=") {
    exprResult = builder->CreateICmpSGE(left, right);
  } else if (expr.op == "<=") {
    exprResult = builder->CreateICmpSLE(left, right);
  } else {
    error("Unknown binary operator: ", expr.op);
  }
}

void CodeGen::visitUnaryExpr(UnaryExpr &expr) {
  expr.operand->accept(*this);
  auto operand = exprResult;
  if (expr.op == "-") {
    exprResult = builder->CreateNeg(operand);
  } else if (expr.op == "!") {
    auto boolCond = builder->CreateICmpEQ(
        operand, llvm::ConstantInt::get(llvm::Type::getInt32Ty(*context), 0));
    exprResult =
        builder->CreateZExt(boolCond, llvm::Type::getInt32Ty(*context));
  } else {
    error("Unknown unary operator: ", expr.op);
  }
}

void CodeGen::visitLogicalExpr(LogicalExpr &expr) {
  auto savedFunction = currentFunction;

  expr.left->accept(*this);
  auto leftVal = exprResult;

  if (leftVal->getType()->isIntegerTy(1)) {
    leftVal = builder->CreateZExt(leftVal, llvm::Type::getInt32Ty(*context));
  }

  if (expr.op == LogicalOperator::AND) {
    auto entryBB = builder->GetInsertBlock();
    auto thenBB =
        llvm::BasicBlock::Create(*context, "land.then", savedFunction);
    auto endBB = llvm::BasicBlock::Create(*context, "land.end", savedFunction);

    auto cond = builder->CreateICmpNE(
        leftVal, llvm::ConstantInt::get(llvm::Type::getInt32Ty(*context), 0),
        "cond");
    builder->CreateCondBr(cond, thenBB, endBB);

    builder->SetInsertPoint(thenBB);
    expr.right->accept(*this);
    auto rightVal = exprResult;
    if (rightVal->getType()->isIntegerTy(1)) {
      rightVal =
          builder->CreateZExt(rightVal, llvm::Type::getInt32Ty(*context));
    }
    builder->CreateBr(endBB);
    auto thenBlock = builder->GetInsertBlock();

    builder->SetInsertPoint(endBB);
    auto phi =
        builder->CreatePHI(llvm::Type::getInt32Ty(*context), 2, "land.result");
    phi->addIncoming(
        llvm::ConstantInt::get(llvm::Type::getInt32Ty(*context), 0), entryBB);
    phi->addIncoming(rightVal, thenBlock);
    exprResult = phi;
  } else {
    auto entryBB = builder->GetInsertBlock();
    auto elseBB = llvm::BasicBlock::Create(*context, "lor.else", savedFunction);
    auto endBB = llvm::BasicBlock::Create(*context, "lor.end", savedFunction);

    auto cond = builder->CreateICmpNE(
        leftVal, llvm::ConstantInt::get(llvm::Type::getInt32Ty(*context), 0),
        "cond");
    builder->CreateCondBr(cond, endBB, elseBB);

    builder->SetInsertPoint(elseBB);
    expr.right->accept(*this);
    auto rightVal = exprResult;
    if (rightVal->getType()->isIntegerTy(1)) {
      rightVal =
          builder->CreateZExt(rightVal, llvm::Type::getInt32Ty(*context));
    }
    builder->CreateBr(endBB);
    auto elseBlock = builder->GetInsertBlock();

    builder->SetInsertPoint(endBB);
    auto phi =
        builder->CreatePHI(llvm::Type::getInt32Ty(*context), 2, "lor.result");
    phi->addIncoming(leftVal, entryBB);
    phi->addIncoming(rightVal, elseBlock);
    exprResult = phi;
  }
}

void CodeGen::visitIndexExpr(IndexExpr &expr) {
  expr.array->accept(*this);
  auto arrayPtr = exprResult;

  expr.index->accept(*this);
  auto indexVal = exprResult;

  validateArrayBounds(arrayPtr, indexVal);

  auto arrayType = llvm::ArrayType::get(llvm::Type::getInt32Ty(*context), 0);
  auto zero = llvm::ConstantInt::get(llvm::Type::getInt32Ty(*context), 0);
  std::vector<llvm::Value *> indices = {zero, indexVal};
  auto elementPtr =
      builder->CreateGEP(arrayType, arrayPtr, indices, "arrayidx");
  exprResult = builder->CreateLoad(llvm::Type::getInt32Ty(*context), elementPtr,
                                   "element");
}

void CodeGen::resolveFieldAccess(FieldAccessExpr &expr,
                                 llvm::StructType *&structType,
                                 size_t &fieldIndex) {
  if (dynamic_cast<ObjectExpr *>(expr.object.get())) {
    // Inline literal: expr.object->accept() (already run by the caller)
    // just ran visitObjectExpr, which left this literal's shape in
    // lastObjectShape_.
    structType = lastObjectShape_.type;
    auto it = lastObjectShape_.fieldIndex.find(expr.fieldName);
    if (it == lastObjectShape_.fieldIndex.end()) {
      error("Object has no field '", expr.fieldName, "'");
    }
    fieldIndex = it->second;
  } else if (auto *varExpr = dynamic_cast<VarExpr *>(expr.object.get())) {
    auto shapeIt = objectShapes_.find(varExpr->name);
    if (shapeIt == objectShapes_.end()) {
      error("Cannot resolve field access on '", varExpr->name,
            "': not declared directly from an object literal");
    }
    auto fieldIt = shapeIt->second.fieldIndex.find(expr.fieldName);
    if (fieldIt == shapeIt->second.fieldIndex.end()) {
      error("Object has no field '", expr.fieldName, "'");
    }
    structType = shapeIt->second.type;
    fieldIndex = fieldIt->second;
  } else {
    // e.g. chained access `a.b.c` — resolving the shape of an intermediate
    // field access result isn't supported yet.
    error("Field access is only supported on an object literal or a "
          "variable declared directly from one");
  }
}

void CodeGen::visitFieldAccessExpr(FieldAccessExpr &expr) {
  expr.object->accept(*this);
  auto objPtr = exprResult;

  llvm::StructType *structType = nullptr;
  size_t fieldIndex = 0;
  resolveFieldAccess(expr, structType, fieldIndex);

  std::vector<llvm::Value *> indices = {
      llvm::ConstantInt::get(llvm::Type::getInt32Ty(*context), 0),
      llvm::ConstantInt::get(llvm::Type::getInt32Ty(*context),
                             static_cast<int>(fieldIndex))};
  auto fieldPtr =
      builder->CreateGEP(structType, objPtr, indices, "fieldaccess");
  auto fieldType = structType->getElementType(fieldIndex);
  exprResult = builder->CreateLoad(fieldType, fieldPtr, "fieldvalue");
}

void CodeGen::emitPrint(llvm::Value *val) {
  if (val->getType()->isIntegerTy()) {
    auto format = builder->CreateGlobalString("%d\n");
    builder->CreateCall(printfFunc, {format, val});
  } else if (val->getType()->isPointerTy()) {
    auto format = builder->CreateGlobalString("%s\n");
    builder->CreateCall(printfFunc, {format, val});
  } else {
    error("Unsupported type in print()");
  }
}

void CodeGen::visitCallExpr(CallExpr &expr) {
  auto varExpr = dynamic_cast<VarExpr *>(expr.callee.get());
  if (!varExpr)
    error("Only direct function calls supported");

  // Built-in: print(value)
  if (varExpr->name == "print") {
    if (expr.args.size() != 1)
      error("print() takes exactly 1 argument");
    expr.args[0]->accept(*this);
    emitPrint(exprResult);
    exprResult = llvm::ConstantInt::get(llvm::Type::getInt32Ty(*context), 0);
    return;
  }

  // Built-in: len(value) — i32 length of a string (runtime strlen) or an
  // array (compile-time constant: arrays are fixed-size, so the element
  // count is always known from the literal or the variable that aliases it).
  if (varExpr->name == "len") {
    if (expr.args.size() != 1)
      error("len() takes exactly 1 argument");

    if (auto *litArr = dynamic_cast<ArrayExpr *>(expr.args[0].get())) {
      expr.args[0]->accept(*this); // still generate it for side effects
      exprResult = llvm::ConstantInt::get(
          llvm::Type::getInt32Ty(*context),
          static_cast<uint64_t>(litArr->elements.size()));
      return;
    }
    if (auto *varArg = dynamic_cast<VarExpr *>(expr.args[0].get())) {
      auto it = arrayLengths_.find(varArg->name);
      if (it != arrayLengths_.end()) {
        expr.args[0]->accept(*this);
        exprResult = llvm::ConstantInt::get(
            llvm::Type::getInt32Ty(*context), static_cast<uint64_t>(it->second));
        return;
      }
    }

    expr.args[0]->accept(*this);
    auto val = exprResult;
    if (!val->getType()->isPointerTy())
      error("len() argument must be a string or array");
    auto i64Len = getStringLength(val);
    exprResult = builder->CreateTrunc(i64Len, llvm::Type::getInt32Ty(*context),
                                      "lenresult");
    return;
  }

  // Built-in: str(int) — formats an integer as a decimal string at runtime.
  // Allocates a 16-byte buffer via malloc, calls snprintf(buf, 16, "%d", n).
  // The pointer is tracked for cleanup like other allocated strings.
  if (varExpr->name == "str") {
    if (expr.args.size() != 1)
      error("str() takes exactly 1 argument");
    expr.args[0]->accept(*this);
    auto intVal = exprResult;
    if (!intVal->getType()->isIntegerTy())
      error("str() argument must be an integer");

    constexpr int64_t BUF_SIZE = 16; // enough for any i32 + sign + null
    auto buf = builder->CreateCall(
        mallocFunc,
        {llvm::ConstantInt::get(llvm::Type::getInt64Ty(*context), BUF_SIZE)},
        "strbuf");
    auto fmt = builder->CreateGlobalStringPtr("%d");
    auto snprintfFunc = module->getFunction("snprintf");
    builder->CreateCall(
        snprintfFunc,
        {buf, llvm::ConstantInt::get(llvm::Type::getInt64Ty(*context), BUF_SIZE),
         fmt, intVal});
    allocatedStrings.push_back(buf);
    exprResult = buf;
    return;
  }

  auto func = module->getFunction(varExpr->name);
  if (!func)
    error("Undefined function: ", varExpr->name);

  if (func->arg_size() != expr.args.size()) {
    std::ostringstream oss;
    oss << "Function '" << varExpr->name << "' expects " << func->arg_size()
        << " arguments, got " << expr.args.size();
    error(oss.str());
  }

  std::vector<llvm::Value *> args;
  for (auto &arg : expr.args) {
    arg->accept(*this);
    if (!exprResult)
      error("Failed to generate argument for call to '", varExpr->name, "'");
    args.push_back(exprResult);
  }
  exprResult = builder->CreateCall(func, args);
}

void CodeGen::visitArrayExpr(ArrayExpr &expr) {
  auto elementType = llvm::Type::getInt32Ty(*context);
  size_t size = expr.elements.size();
  auto arrayType = llvm::ArrayType::get(elementType, size);

  auto alloca = builder->CreateAlloca(arrayType, nullptr, "array");

  for (size_t i = 0; i < expr.elements.size(); i++) {
    expr.elements[i]->accept(*this);
    auto val = exprResult;
    std::vector<llvm::Value *> indices = {
        llvm::ConstantInt::get(llvm::Type::getInt32Ty(*context), 0),
        llvm::ConstantInt::get(llvm::Type::getInt32Ty(*context),
                               static_cast<int64_t>(i))};
    auto elemPtr = builder->CreateGEP(arrayType, alloca, indices, "arrayelem");
    builder->CreateStore(val, elemPtr);
  }

  exprResult = alloca;
}

llvm::Value *CodeGen::getStringLength(llvm::Value *str) {
  auto i8PtrType = llvm::PointerType::get(llvm::Type::getInt8Ty(*context), 0);
  auto strPtr = builder->CreateBitCast(str, i8PtrType, "strptr");
  return builder->CreateCall(strlenFunc, {strPtr}, "strlen");
}

llvm::Value *CodeGen::compareStrings(llvm::Value *left, llvm::Value *right,
                                     bool equal) {
  auto i8PtrType = llvm::PointerType::get(llvm::Type::getInt8Ty(*context), 0);
  auto leftPtr = builder->CreateBitCast(left, i8PtrType, "streqleft");
  auto rightPtr = builder->CreateBitCast(right, i8PtrType, "streqright");

  auto strcmpFunc = module->getFunction("strcmp");
  auto cmp = builder->CreateCall(strcmpFunc, {leftPtr, rightPtr}, "strcmp");
  auto zero = llvm::ConstantInt::get(llvm::Type::getInt32Ty(*context), 0);
  return equal ? builder->CreateICmpEQ(cmp, zero, "streq")
               : builder->CreateICmpNE(cmp, zero, "strneq");
}

llvm::Value *CodeGen::concatenateStrings(llvm::Value *left,
                                         llvm::Value *right) {
  auto i8PtrType = llvm::PointerType::get(llvm::Type::getInt8Ty(*context), 0);

  llvm::Value *leftPtr;
  llvm::Value *rightPtr;

  if (left->getType()->isPointerTy()) {
    leftPtr = builder->CreateBitCast(left, i8PtrType, "leftptr");
  } else {
    auto ci = llvm::dyn_cast<llvm::ConstantInt>(left);
    if (!ci) {
      error("Expected constant integer in string concatenation");
      return nullptr;
    }
    auto numStr =
        builder->CreateGlobalStringPtr(std::to_string(ci->getZExtValue()));
    leftPtr = numStr;
  }

  if (right->getType()->isPointerTy()) {
    rightPtr = builder->CreateBitCast(right, i8PtrType, "rightptr");
  } else {
    auto ci = llvm::dyn_cast<llvm::ConstantInt>(right);
    if (!ci) {
      error("Expected constant integer in string concatenation");
      return nullptr;
    }
    auto numStr =
        builder->CreateGlobalStringPtr(std::to_string(ci->getZExtValue()));
    rightPtr = numStr;
  }

  auto leftLen = getStringLength(leftPtr);
  auto rightLen = getStringLength(rightPtr);
  auto one = llvm::ConstantInt::get(llvm::Type::getInt64Ty(*context), 1);
  auto totalLen = builder->CreateAdd(builder->CreateAdd(leftLen, rightLen), one,
                                     "totallen");

  auto result = builder->CreateCall(mallocFunc, {totalLen}, "concat");

  auto mallocCheck = builder->CreateICmpNE(
      result, llvm::ConstantPointerNull::get(i8PtrType), "malloc_success");
  auto mallocErrorBB =
      llvm::BasicBlock::Create(*context, "malloc_error", currentFunction);
  auto mallocContinueBB =
      llvm::BasicBlock::Create(*context, "malloc_continue", currentFunction);

  builder->CreateCondBr(mallocCheck, mallocContinueBB, mallocErrorBB);
  builder->SetInsertPoint(mallocErrorBB);

  generateRuntimeError("RuntimeError: Memory allocation failed\n");

  builder->SetInsertPoint(mallocContinueBB);

  auto resultI8Ptr = llvm::PointerType::get(llvm::Type::getInt8Ty(*context), 0);
  llvm::Value *resultCast =
      builder->CreateBitCast(result, resultI8Ptr, "resultcpy");

  auto strcpyFunc = module->getFunction("strcpy");
  if (strcpyFunc) {
    builder->CreateCall(strcpyFunc, {resultCast, leftPtr});
  }

  auto destPtr = builder->CreateGEP(llvm::Type::getInt8Ty(*context), resultCast,
                                    leftLen, "destptr");
  auto strcatFunc = module->getFunction("strcat");
  if (strcatFunc) {
    builder->CreateCall(strcatFunc, {destPtr, rightPtr});
  }

  allocatedStrings.push_back(resultCast);
  return resultCast;
}

void CodeGen::visitObjectExpr(ObjectExpr &expr) {
  if (expr.pairs.empty()) {
    auto structType = llvm::StructType::get(*context);
    auto alloca = builder->CreateAlloca(structType, nullptr, "empty_obj");
    exprResult = alloca;
    lastObjectShape_ = ObjectShape{structType, {}};
    return;
  }

  // Single pass: each field initializer is evaluated exactly once (the
  // previous two-pass version — once to collect types, again to store
  // values — ran every field initializer's side effects twice).
  std::vector<llvm::Type *> fieldTypes;
  std::vector<std::pair<std::string, llvm::Value *>> fieldValues;
  fieldTypes.reserve(expr.pairs.size());
  fieldValues.reserve(expr.pairs.size());
  for (auto &pair : expr.pairs) {
    pair.second->accept(*this);
    fieldTypes.push_back(exprResult->getType());
    fieldValues.emplace_back(pair.first, exprResult);
  }

  auto structType = llvm::StructType::create(*context, fieldTypes, "object");
  auto alloca = builder->CreateAlloca(structType, nullptr, "object");

  ObjectShape shape;
  shape.type = structType;
  for (size_t i = 0; i < fieldValues.size(); i++) {
    const auto &[name, val] = fieldValues[i];
    std::vector<llvm::Value *> indices = {
        llvm::ConstantInt::get(llvm::Type::getInt32Ty(*context), 0),
        llvm::ConstantInt::get(llvm::Type::getInt32Ty(*context),
                               static_cast<int>(i))};
    auto fieldPtr = builder->CreateGEP(structType, alloca, indices, "field");
    builder->CreateStore(val, fieldPtr);
    shape.fieldIndex[name] = i;
  }

  exprResult = alloca;
  lastObjectShape_ = shape;
}

void CodeGen::visitExprStmt(ExprStmt &stmt) { stmt.expr->accept(*this); }

void CodeGen::visitLetStmt(LetStmt &stmt) {
  stmt.initializer->accept(*this);
  auto val = exprResult;
  auto alloca = builder->CreateAlloca(val->getType());
  builder->CreateStore(val, alloca);
  declareVariable(stmt.name, alloca);
  variableTypes[stmt.name] = val->getType();

  if (auto *arrExpr = dynamic_cast<ArrayExpr *>(stmt.initializer.get())) {
    arrayLengths_[stmt.name] = arrExpr->elements.size();
  } else if (dynamic_cast<ObjectExpr *>(stmt.initializer.get())) {
    objectShapes_[stmt.name] = lastObjectShape_;
  } else if (auto *varExpr = dynamic_cast<VarExpr *>(stmt.initializer.get())) {
    // `let b = a;` — thread whatever compile-time shape metadata `a` has
    // through the alias, matching the pointer aliasing this produces at
    // runtime (visitVarExpr just loads and re-stores the same pointer).
    auto arrIt = arrayLengths_.find(varExpr->name);
    if (arrIt != arrayLengths_.end()) arrayLengths_[stmt.name] = arrIt->second;
    auto objIt = objectShapes_.find(varExpr->name);
    if (objIt != objectShapes_.end()) objectShapes_[stmt.name] = objIt->second;
  }
}

void CodeGen::visitFuncStmt(FuncStmt &stmt) {
  auto savedBlock = builder->GetInsertBlock();
  auto savedFunction = currentFunction;

  // declareFunctionSignatures() already created top-level functions before
  // any body was generated, so forward/mutual calls resolve — reuse that
  // declaration rather than creating a second llvm::Function with the same
  // name (which LLVM would silently rename to "name.1", leaving calls bound
  // to the pre-pass's declaration undefined at link time).
  auto *func = module->getFunction(stmt.name);
  if (!func) {
    std::vector<llvm::Type *> paramTypes(stmt.params.size(),
                                         llvm::Type::getInt32Ty(*context));
    auto funcType = llvm::FunctionType::get(llvm::Type::getInt32Ty(*context),
                                            paramTypes, false);
    func = llvm::Function::Create(funcType, llvm::Function::ExternalLinkage,
                                  stmt.name, module.get());
  }
  auto entry = llvm::BasicBlock::Create(*context, "entry", func);
  builder->SetInsertPoint(entry);

  enterScope();

  auto it = func->arg_begin();
  for (auto &param : stmt.params) {
    auto alloca = builder->CreateAlloca(llvm::Type::getInt32Ty(*context));
    builder->CreateStore(&*it, alloca);
    declareVariable(param, alloca);
    ++it;
  }
  currentFunction = func;
  for (auto &s : stmt.body) {
    s->accept(*this);
  }
  if (!builder->GetInsertBlock()->getTerminator()) {
    builder->CreateRet(
        llvm::ConstantInt::get(llvm::Type::getInt32Ty(*context), 0));
  }

  exitScope();

  builder->SetInsertPoint(savedBlock);
  currentFunction = savedFunction;
}

void CodeGen::visitIfStmt(IfStmt &stmt) {
  stmt.condition->accept(*this);
  auto cond = exprResult;
  auto thenBB = llvm::BasicBlock::Create(*context, "then", currentFunction);
  auto elseBB = llvm::BasicBlock::Create(*context, "else", currentFunction);
  auto endBB = llvm::BasicBlock::Create(*context, "endif", currentFunction);
  builder->CreateCondBr(cond, thenBB, elseBB);
  builder->SetInsertPoint(thenBB);
  for (auto &s : stmt.thenBranch)
    s->accept(*this);
  if (!builder->GetInsertBlock()->getTerminator())
    builder->CreateBr(endBB);
  builder->SetInsertPoint(elseBB);
  for (auto &s : stmt.elseBranch)
    s->accept(*this);
  if (!builder->GetInsertBlock()->getTerminator())
    builder->CreateBr(endBB);
  builder->SetInsertPoint(endBB);
}

void CodeGen::visitForStmt(ForStmt &stmt) {
  stmt.rangeStart->accept(*this);
  auto startVal = exprResult;
  stmt.rangeEnd->accept(*this);
  auto endVal = exprResult;

  enterScope();

  auto loopVar = builder->CreateAlloca(llvm::Type::getInt32Ty(*context));
  builder->CreateStore(startVal, loopVar);
  declareVariable(stmt.var, loopVar);

  auto condBB = llvm::BasicBlock::Create(*context, "cond", currentFunction);
  auto bodyBB = llvm::BasicBlock::Create(*context, "body", currentFunction);
  auto incrBB = llvm::BasicBlock::Create(*context, "incr", currentFunction);
  auto endBB = llvm::BasicBlock::Create(*context, "endfor", currentFunction);

  auto savedBreakBlock = breakBlock;
  auto savedContinueBlock = continueBlock;
  breakBlock = endBB;
  continueBlock = incrBB;

  builder->CreateBr(condBB);
  builder->SetInsertPoint(condBB);
  auto current = builder->CreateLoad(llvm::Type::getInt32Ty(*context), loopVar);
  auto cond = builder->CreateICmpSLT(current, endVal);
  builder->CreateCondBr(cond, bodyBB, endBB);

  builder->SetInsertPoint(bodyBB);
  for (auto &s : stmt.body)
    s->accept(*this);
  if (!builder->GetInsertBlock()->getTerminator())
    builder->CreateBr(incrBB);

  builder->SetInsertPoint(incrBB);
  auto latest = builder->CreateLoad(llvm::Type::getInt32Ty(*context), loopVar);
  auto next = builder->CreateAdd(
      latest, llvm::ConstantInt::get(llvm::Type::getInt32Ty(*context), 1));
  builder->CreateStore(next, loopVar);
  builder->CreateBr(condBB);

  builder->SetInsertPoint(endBB);

  exitScope();

  breakBlock = savedBreakBlock;
  continueBlock = savedContinueBlock;
}

void CodeGen::visitWhileStmt(WhileStmt &stmt) {
  auto condBB = llvm::BasicBlock::Create(*context, "cond", currentFunction);
  auto bodyBB = llvm::BasicBlock::Create(*context, "body", currentFunction);
  auto endBB = llvm::BasicBlock::Create(*context, "endwhile", currentFunction);

  auto savedBreakBlock = breakBlock;
  auto savedContinueBlock = continueBlock;
  breakBlock = endBB;
  continueBlock = condBB;

  builder->CreateBr(condBB);
  builder->SetInsertPoint(condBB);
  stmt.condition->accept(*this);
  auto cond = exprResult;
  builder->CreateCondBr(cond, bodyBB, endBB);
  builder->SetInsertPoint(bodyBB);
  for (auto &s : stmt.body)
    s->accept(*this);
  if (!builder->GetInsertBlock()->getTerminator())
    builder->CreateBr(condBB);
  builder->SetInsertPoint(endBB);

  breakBlock = savedBreakBlock;
  continueBlock = savedContinueBlock;
}

void CodeGen::visitReturnStmt(ReturnStmt &stmt) {
  llvm::Value *val;
  if (stmt.value) {
    stmt.value->accept(*this);
    val = exprResult;
    // Caller takes ownership of any returned string pointer — remove it from
    // the free-list so we don't free memory we just handed back, then release
    // all other temporaries accumulated in this scope before the early return.
    allocatedStrings.erase(
        std::remove(allocatedStrings.begin(), allocatedStrings.end(), val),
        allocatedStrings.end());
  } else {
    // Bare `return;` — emit `return 0` since all functions currently return i32.
    val = llvm::ConstantInt::get(llvm::Type::getInt32Ty(*context), 0);
  }
  freeAllocatedStrings();
  builder->CreateRet(val);
}

void CodeGen::visitBlockStmt(BlockStmt &stmt) {
  for (auto &s : stmt.body) {
    s->accept(*this);
  }
}

void CodeGen::visitBreakStmt(BreakStmt &stmt) {
  if (breakBlock) {
    builder->CreateBr(breakBlock);
  }
}

void CodeGen::visitContinueStmt(ContinueStmt &stmt) {
  if (continueBlock) {
    builder->CreateBr(continueBlock);
  }
}