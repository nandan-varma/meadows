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
    exprResult = builder->CreateGlobalString(expr.value);
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

  exprResult = builder->CreateLoad(llvm::Type::getInt32Ty(*context), var);
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
      exprResult = left;
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
  } else {
    throw std::runtime_error("Unknown unary operator: " + expr.op);
  }
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
  if (expr.elements.empty()) {
    auto arrayType = llvm::ArrayType::get(llvm::Type::getInt32Ty(*context), 0);
    exprResult = llvm::ConstantArray::get(arrayType, {});
    return;
  }

  std::vector<llvm::Constant *> values;
  for (auto &elem : expr.elements) {
    elem->accept(*this);
    auto constant = llvm::dyn_cast<llvm::Constant>(exprResult);
    if (!constant) {
      throw std::runtime_error("Array elements must be compile-time constants");
    }
    values.push_back(constant);
  }

  auto arrayType =
      llvm::ArrayType::get(llvm::Type::getInt32Ty(*context), values.size());
  exprResult = llvm::ConstantArray::get(arrayType, values);
}

void CodeGen::visitObjectExpr(ObjectExpr &expr) {
  if (expr.pairs.empty()) {
    std::vector<llvm::Type *> emptyTypes;
    auto structType =
        llvm::StructType::create(*context, emptyTypes, "empty_object");
    exprResult = llvm::ConstantStruct::get(structType);
    return;
  }

  std::vector<llvm::Type *> fieldTypes;
  for (auto &pair : expr.pairs) {
    pair.second->accept(*this);
    fieldTypes.push_back(exprResult->getType());
  }

  auto structType = llvm::StructType::create(*context, fieldTypes, "object");

  std::vector<llvm::Constant *> fieldValues;
  for (auto &pair : expr.pairs) {
    pair.second->accept(*this);
    auto constant = llvm::dyn_cast<llvm::Constant>(exprResult);
    if (!constant) {
      throw std::runtime_error(
          "Object field values must be compile-time constants");
    }
    fieldValues.push_back(constant);
  }

  exprResult = llvm::ConstantStruct::get(structType, fieldValues);
}

void CodeGen::visitExprStmt(ExprStmt &stmt) { stmt.expr->accept(*this); }

void CodeGen::visitLetStmt(LetStmt &stmt) {
  stmt.initializer->accept(*this);
  auto val = exprResult;
  auto alloca = builder->CreateAlloca(val->getType());
  builder->CreateStore(val, alloca);
  variables[stmt.name] = alloca;
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
}

void CodeGen::visitWhileStmt(WhileStmt &stmt) {
  auto condBB = llvm::BasicBlock::Create(*context, "cond", currentFunction);
  auto bodyBB = llvm::BasicBlock::Create(*context, "body", currentFunction);
  auto endBB = llvm::BasicBlock::Create(*context, "endwhile", currentFunction);
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