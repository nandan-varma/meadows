#include "CodeGen.h"
#include "StringUtils.h"
#include "SymbolTable.h"
#include <climits>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Verifier.h>
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

  auto *errorMsg =
      builder->CreateGlobalStringPtr("RuntimeError: Division by zero\n");
  auto *format = builder->CreateGlobalString("%s");
  builder->CreateCall(printfFunc, {format, errorMsg});
  builder->CreateRet(
      llvm::ConstantInt::get(llvm::Type::getInt32Ty(*context), -1));

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

  auto *errorMsg = builder->CreateGlobalStringPtr(
      "RuntimeError: Array index out of bounds\n");
  auto *format = builder->CreateGlobalString("%s");
  builder->CreateCall(printfFunc, {format, errorMsg});
  builder->CreateRet(
      llvm::ConstantInt::get(llvm::Type::getInt32Ty(*context), -1));

  builder->SetInsertPoint(continueBB);
}

CodeGen::CodeGen(bool optimize) : optimize_(optimize) {
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

  auto freeType = llvm::FunctionType::get(
      llvm::Type::getVoidTy(*context),
      {llvm::PointerType::get(llvm::Type::getInt8Ty(*context), 0)}, false);
  freeFunc = llvm::cast<llvm::Function>(
      module->getOrInsertFunction("free", freeType).getCallee());

  currentFunction = nullptr;
  exprResult = nullptr;
  currentBlock = nullptr;
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

  for (auto &stmt : statements) {
    stmt->accept(*this);
  }

  exitScope();

  freeAllocatedStrings();

  if (optimize_) {
    module->print(llvm::errs(), nullptr);
  }

  builder->CreateRet(
      llvm::ConstantInt::get(llvm::Type::getInt32Ty(*context), 0));
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
  expr.value->accept(*this);
  auto val = exprResult;

  llvm::Value *var = lookupVariable(expr.name);
  if (!var) {
    error("Undefined variable in assignment: ", expr.name);
  }

  builder->CreateStore(val, var);
  exprResult = val;
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
  } else if (expr.op == "==") {
    exprResult = builder->CreateICmpEQ(left, right);
  } else if (expr.op == "!=") {
    exprResult = builder->CreateICmpNE(left, right);
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

void CodeGen::visitFieldAccessExpr(FieldAccessExpr &expr) {
  expr.object->accept(*this);
  auto objPtr = exprResult;

  auto structType = llvm::StructType::get(*context);
  if (auto objExpr = dynamic_cast<ObjectExpr *>(expr.object.get())) {
    std::vector<llvm::Type *> fieldTypes;
    for (auto &pair : objExpr->pairs) {
      pair.second->accept(*this);
      fieldTypes.push_back(exprResult->getType());
    }
    structType = llvm::StructType::get(*context, fieldTypes);
  }

  size_t fieldIndex = 0;
  if (auto objExpr = dynamic_cast<ObjectExpr *>(expr.object.get())) {
    int i = 0;
    for (auto &pair : objExpr->pairs) {
      if (pair.first == expr.fieldName) {
        fieldIndex = i;
        break;
      }
      i++;
    }
  }

  std::vector<llvm::Value *> indices = {
      llvm::ConstantInt::get(llvm::Type::getInt32Ty(*context), 0),
      llvm::ConstantInt::get(llvm::Type::getInt32Ty(*context),
                             static_cast<int>(fieldIndex))};
  auto fieldPtr =
      builder->CreateGEP(structType, objPtr, indices, "fieldaccess");
  auto fieldType = structType->getElementType(fieldIndex);
  exprResult = builder->CreateLoad(fieldType, fieldPtr, "fieldvalue");
}

void CodeGen::visitCallExpr(CallExpr &expr) {
  auto varExpr = dynamic_cast<VarExpr *>(expr.callee.get());
  if (!varExpr)
    error("Only variable calls supported");

  auto func = module->getFunction(varExpr->name);
  if (!func) {
    error("Undefined function: ", varExpr->name);
  }

  if (func->arg_size() != expr.args.size()) {
    std::ostringstream oss;
    oss << "Function " << varExpr->name << " expects " << func->arg_size()
        << " arguments, got " << expr.args.size();
    error(oss.str());
  }

  std::vector<llvm::Value *> args;
  for (auto &arg : expr.args) {
    arg->accept(*this);
    if (!exprResult) {
      error("Failed to generate argument code for function: ", varExpr->name);
    }
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

llvm::Value *CodeGen::concatenateStrings(llvm::Value *left,
                                         llvm::Value *right) {
  auto i8PtrType = llvm::PointerType::get(llvm::Type::getInt8Ty(*context), 0);

  llvm::Value *leftPtr;
  llvm::Value *rightPtr;

  if (left->getType()->isPointerTy()) {
    leftPtr = builder->CreateBitCast(left, i8PtrType, "leftptr");
  } else {
    auto ci = llvm::dyn_cast<llvm::ConstantInt>(left);
    auto numStr =
        builder->CreateGlobalStringPtr(std::to_string(ci->getZExtValue()));
    leftPtr = numStr;
  }

  if (right->getType()->isPointerTy()) {
    rightPtr = builder->CreateBitCast(right, i8PtrType, "rightptr");
  } else {
    auto ci = llvm::dyn_cast<llvm::ConstantInt>(right);
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

  auto *errorMsg = builder->CreateGlobalStringPtr(
      "RuntimeError: Memory allocation failed\n");
  auto *format = builder->CreateGlobalString("%s");
  builder->CreateCall(printfFunc, {format, errorMsg});
  builder->CreateRet(
      llvm::ConstantInt::get(llvm::Type::getInt32Ty(*context), -1));

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
    return;
  }

  std::vector<llvm::Type *> fieldTypes;
  for (auto &pair : expr.pairs) {
    pair.second->accept(*this);
    fieldTypes.push_back(exprResult->getType());
  }

  auto structType = llvm::StructType::create(*context, fieldTypes, "object");
  auto alloca = builder->CreateAlloca(structType, nullptr, "object");

  size_t i = 0;
  for (auto &pair : expr.pairs) {
    pair.second->accept(*this);
    auto val = exprResult;
    std::vector<llvm::Value *> indices = {
        llvm::ConstantInt::get(llvm::Type::getInt32Ty(*context), 0),
        llvm::ConstantInt::get(llvm::Type::getInt32Ty(*context),
                               static_cast<int>(i))};
    auto fieldPtr = builder->CreateGEP(structType, alloca, indices, "field");
    builder->CreateStore(val, fieldPtr);
    i++;
  }

  exprResult = alloca;
}

void CodeGen::visitExprStmt(ExprStmt &stmt) { stmt.expr->accept(*this); }

void CodeGen::visitLetStmt(LetStmt &stmt) {
  stmt.initializer->accept(*this);
  auto val = exprResult;
  auto alloca = builder->CreateAlloca(val->getType());
  builder->CreateStore(val, alloca);
  declareVariable(stmt.name, alloca);
  variableTypes[stmt.name] = val->getType();
}

void CodeGen::visitFuncStmt(FuncStmt &stmt) {
  auto savedBlock = builder->GetInsertBlock();
  auto savedFunction = currentFunction;
  std::vector<llvm::Type *> paramTypes(stmt.params.size(),
                                       llvm::Type::getInt32Ty(*context));
  auto funcType = llvm::FunctionType::get(llvm::Type::getInt32Ty(*context),
                                          paramTypes, false);
  auto func = llvm::Function::Create(funcType, llvm::Function::ExternalLinkage,
                                     stmt.name, module.get());
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
  auto endBB = llvm::BasicBlock::Create(*context, "endfor", currentFunction);

  auto savedBreakBlock = breakBlock;
  auto savedContinueBlock = continueBlock;
  breakBlock = endBB;
  continueBlock = condBB;

  builder->CreateBr(condBB);
  builder->SetInsertPoint(condBB);
  auto current = builder->CreateLoad(llvm::Type::getInt32Ty(*context), loopVar);
  auto cond = builder->CreateICmpSLT(current, endVal);
  builder->CreateCondBr(cond, bodyBB, endBB);
  builder->SetInsertPoint(bodyBB);
  for (auto &s : stmt.body)
    s->accept(*this);
  auto next = builder->CreateAdd(
      current, llvm::ConstantInt::get(llvm::Type::getInt32Ty(*context), 1));
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
  builder->CreateBr(condBB);
  builder->SetInsertPoint(endBB);

  breakBlock = savedBreakBlock;
  continueBlock = savedContinueBlock;
}

void CodeGen::visitReturnStmt(ReturnStmt &stmt) {
  stmt.value->accept(*this);
  auto val = exprResult;
  builder->CreateRet(val);
}

void CodeGen::visitBlockStmt(BlockStmt &stmt) {
  for (auto &s : stmt.body) {
    s->accept(*this);
  }
}

void CodeGen::visitPrintStmt(PrintStmt &stmt) {
  stmt.expr->accept(*this);
  auto val = exprResult;
  if (val->getType()->isIntegerTy()) {
    auto format = builder->CreateGlobalString("%d\n");
    builder->CreateCall(printfFunc, {format, val});
  } else if (val->getType()->isPointerTy()) {
    auto format = builder->CreateGlobalString("%s\n");
    builder->CreateCall(printfFunc, {format, val});
  } else {
    error("Unsupported type in print statement");
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