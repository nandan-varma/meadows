#include "CodeGen.h"
#include <climits>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/Casting.h>
#include <stdexcept>

CodeGen::CodeGen() {
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

  currentFunction = nullptr;
  exprResult = nullptr;
  currentBlock = nullptr;
}

void CodeGen::generate(const std::vector<std::unique_ptr<Stmt>> &statements) {
  // Create main function
  auto mainType =
      llvm::FunctionType::get(llvm::Type::getInt32Ty(*context), {}, false);
  auto mainFunc = llvm::Function::Create(
      mainType, llvm::Function::ExternalLinkage, "main", module.get());
  auto entry = llvm::BasicBlock::Create(*context, "entry", mainFunc);
  builder->SetInsertPoint(entry);
  currentFunction = mainFunc;
  currentBlock = entry;
  variables.clear();

  for (auto &stmt : statements) {
    stmt->accept(*this);
  }

  builder->CreateRet(
      llvm::ConstantInt::get(llvm::Type::getInt32Ty(*context), 0));
}

std::unique_ptr<llvm::Module> CodeGen::getModule() { return std::move(module); }

void CodeGen::visitLiteralExpr(LiteralExpr &expr) {
  if (!expr.value.empty() && isdigit(expr.value[0])) {
    try {
      exprResult =
          llvm::ConstantInt::get(llvm::Type::getInt32Ty(*context),
                                 llvm::APInt(32, std::stoi(expr.value)));
    } catch (const std::out_of_range &) {
      exprResult = llvm::ConstantInt::get(llvm::Type::getInt32Ty(*context),
                                          llvm::APInt(32, INT32_MAX));
    }
  } else {
    exprResult = builder->CreateGlobalStringPtr(expr.value);
  }
}

void CodeGen::visitVarExpr(VarExpr &expr) {
  auto it = variables.find(expr.name);
  if (it == variables.end()) {
    throw std::runtime_error("Undefined variable: " + expr.name);
  }

  llvm::Value *var = it->second;
  if (!var) {
    throw std::runtime_error("Invalid variable allocation: " + expr.name);
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

  auto it = variables.find(expr.name);
  if (it == variables.end()) {
    throw std::runtime_error("Undefined variable: " + expr.name);
  }

  llvm::Value *var = it->second;
  if (!var) {
    throw std::runtime_error("Invalid variable allocation: " + expr.name);
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
    bool leftIsString = left->getType()->isPointerTy();
    bool rightIsString = right->getType()->isPointerTy();
    if (leftIsString || rightIsString) {
      exprResult = concatenateStrings(left, right);
    } else {
      exprResult = builder->CreateAdd(left, right);
    }
  } else if (expr.op == "-") {
    if (expr.left) {
      exprResult = builder->CreateSub(left, right);
    } else {
      exprResult = builder->CreateNeg(right);
    }
  } else if (expr.op == "*")
    exprResult = builder->CreateMul(left, right);
  else if (expr.op == "/")
    exprResult = builder->CreateSDiv(left, right);
  else if (expr.op == "==")
    exprResult = builder->CreateICmpEQ(left, right);
  else if (expr.op == "!=")
    exprResult = builder->CreateICmpNE(left, right);
  else if (expr.op == ">")
    exprResult = builder->CreateICmpSGT(left, right);
  else if (expr.op == "<")
    exprResult = builder->CreateICmpSLT(left, right);
  else if (expr.op == ">=")
    exprResult = builder->CreateICmpSGE(left, right);
  else if (expr.op == "<=")
    exprResult = builder->CreateICmpSLE(left, right);
  else
    exprResult = nullptr;
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
    throw std::runtime_error("Unknown unary operator: " + expr.op);
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
    throw std::runtime_error("Only variable calls supported");

  auto func = module->getFunction(varExpr->name);
  if (!func) {
    throw std::runtime_error("Undefined function: " + varExpr->name);
  }

  // Check argument count
  if (func->arg_size() != expr.args.size()) {
    throw std::runtime_error("Function " + varExpr->name + " expects " +
                             std::to_string(func->arg_size()) +
                             " arguments, got " +
                             std::to_string(expr.args.size()));
  }

  std::vector<llvm::Value *> args;
  for (auto &arg : expr.args) {
    arg->accept(*this);
    if (!exprResult) {
      throw std::runtime_error("Failed to generate argument code");
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

  auto resultI8Ptr = llvm::PointerType::get(llvm::Type::getInt8Ty(*context), 0);
  auto resultCast = builder->CreateBitCast(result, resultI8Ptr, "resultcpy");
  builder->CreateCall(module->getFunction("strcpy"), {resultCast, leftPtr});

  auto destPtr = builder->CreateGEP(llvm::Type::getInt8Ty(*context), resultCast,
                                    leftLen, "destptr");
  builder->CreateCall(module->getFunction("strcat"), {destPtr, rightPtr});

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
  variables[stmt.name] = alloca;
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
  variables.clear();
  auto it = func->arg_begin();
  for (auto &param : stmt.params) {
    auto alloca = builder->CreateAlloca(llvm::Type::getInt32Ty(*context));
    builder->CreateStore(&*it, alloca);
    variables[param] = alloca;
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
  auto loopVar = builder->CreateAlloca(llvm::Type::getInt32Ty(*context));
  builder->CreateStore(startVal, loopVar);
  variables[stmt.var] = loopVar;
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