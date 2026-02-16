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
#include <vector>

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

  auto memcpyType = llvm::FunctionType::get(
      llvm::PointerType::get(llvm::Type::getInt8Ty(*context), 0),
      {llvm::PointerType::get(llvm::Type::getInt8Ty(*context), 0),
       llvm::PointerType::get(llvm::Type::getInt8Ty(*context), 0),
       llvm::Type::getInt64Ty(*context)},
      false);
  module->getOrInsertFunction("memcpy", memcpyType);

  auto memcmpType = llvm::FunctionType::get(
      llvm::Type::getInt32Ty(*context),
      {llvm::PointerType::get(llvm::Type::getInt8Ty(*context), 0),
       llvm::PointerType::get(llvm::Type::getInt8Ty(*context), 0),
       llvm::Type::getInt64Ty(*context)},
      false);
  module->getOrInsertFunction("memcmp", memcmpType);

  auto strdupType = llvm::FunctionType::get(
      llvm::PointerType::get(llvm::Type::getInt8Ty(*context), 0),
      {llvm::PointerType::get(llvm::Type::getInt8Ty(*context), 0)}, false);
  module->getOrInsertFunction("strdup", strdupType);

  auto strndupType = llvm::FunctionType::get(
      llvm::PointerType::get(llvm::Type::getInt8Ty(*context), 0),
      {llvm::PointerType::get(llvm::Type::getInt8Ty(*context), 0),
       llvm::Type::getInt64Ty(*context)},
      false);
  module->getOrInsertFunction("strndup", strndupType);

  auto strncmpType = llvm::FunctionType::get(
      llvm::Type::getInt32Ty(*context),
      {llvm::PointerType::get(llvm::Type::getInt8Ty(*context), 0),
       llvm::PointerType::get(llvm::Type::getInt8Ty(*context), 0),
       llvm::Type::getInt64Ty(*context)},
      false);
  module->getOrInsertFunction("strncmp", strncmpType);

  auto strnlenType = llvm::FunctionType::get(
      llvm::Type::getInt64Ty(*context),
      {llvm::PointerType::get(llvm::Type::getInt8Ty(*context), 0),
       llvm::Type::getInt64Ty(*context)},
      false);
  module->getOrInsertFunction("strnlen", strnlenType);

  // File I/O functions
  auto fopenType = llvm::FunctionType::get(
      llvm::PointerType::get(llvm::Type::getInt8Ty(*context), 0),
      {llvm::PointerType::get(llvm::Type::getInt8Ty(*context), 0),
       llvm::PointerType::get(llvm::Type::getInt8Ty(*context), 0)},
      false);
  module->getOrInsertFunction("fopen", fopenType);

  auto fcloseType = llvm::FunctionType::get(
      llvm::Type::getInt32Ty(*context),
      {llvm::PointerType::get(llvm::Type::getInt8Ty(*context), 0)}, false);
  module->getOrInsertFunction("fclose", fcloseType);

  auto freadType = llvm::FunctionType::get(
      llvm::Type::getInt64Ty(*context),
      {llvm::PointerType::get(llvm::Type::getInt8Ty(*context), 0),
       llvm::Type::getInt64Ty(*context), llvm::Type::getInt64Ty(*context),
       llvm::PointerType::get(llvm::Type::getInt8Ty(*context), 0)},
      false);
  module->getOrInsertFunction("fread", freadType);

  auto fwriteType = llvm::FunctionType::get(
      llvm::Type::getInt64Ty(*context),
      {llvm::PointerType::get(llvm::Type::getInt8Ty(*context), 0),
       llvm::Type::getInt64Ty(*context), llvm::Type::getInt64Ty(*context),
       llvm::PointerType::get(llvm::Type::getInt8Ty(*context), 0)},
      false);
  module->getOrInsertFunction("fwrite", fwriteType);

  auto fgetsType = llvm::FunctionType::get(
      llvm::PointerType::get(llvm::Type::getInt8Ty(*context), 0),
      {llvm::PointerType::get(llvm::Type::getInt8Ty(*context), 0),
       llvm::Type::getInt32Ty(*context),
       llvm::PointerType::get(llvm::Type::getInt8Ty(*context), 0)},
      false);
  module->getOrInsertFunction("fgets", fgetsType);

  auto feofType = llvm::FunctionType::get(
      llvm::Type::getInt32Ty(*context),
      {llvm::PointerType::get(llvm::Type::getInt8Ty(*context), 0)}, false);
  module->getOrInsertFunction("feof", feofType);

  auto ferrorType = llvm::FunctionType::get(
      llvm::Type::getInt32Ty(*context),
      {llvm::PointerType::get(llvm::Type::getInt8Ty(*context), 0)}, false);
  module->getOrInsertFunction("ferror", ferrorType);

  auto removeType = llvm::FunctionType::get(
      llvm::Type::getInt32Ty(*context),
      {llvm::PointerType::get(llvm::Type::getInt8Ty(*context), 0)}, false);
  module->getOrInsertFunction("remove", removeType);

  auto renameType = llvm::FunctionType::get(
      llvm::Type::getInt32Ty(*context),
      {llvm::PointerType::get(llvm::Type::getInt8Ty(*context), 0),
       llvm::PointerType::get(llvm::Type::getInt8Ty(*context), 0)},
      false);
  module->getOrInsertFunction("rename", renameType);

  // Math functions
  auto sinType =
      llvm::FunctionType::get(llvm::Type::getDoubleTy(*context),
                              {llvm::Type::getDoubleTy(*context)}, false);
  module->getOrInsertFunction("sin", sinType);

  auto cosType =
      llvm::FunctionType::get(llvm::Type::getDoubleTy(*context),
                              {llvm::Type::getDoubleTy(*context)}, false);
  module->getOrInsertFunction("cos", cosType);

  auto tanType =
      llvm::FunctionType::get(llvm::Type::getDoubleTy(*context),
                              {llvm::Type::getDoubleTy(*context)}, false);
  module->getOrInsertFunction("tan", tanType);

  auto asinType =
      llvm::FunctionType::get(llvm::Type::getDoubleTy(*context),
                              {llvm::Type::getDoubleTy(*context)}, false);
  module->getOrInsertFunction("asin", asinType);

  auto acosType =
      llvm::FunctionType::get(llvm::Type::getDoubleTy(*context),
                              {llvm::Type::getDoubleTy(*context)}, false);
  module->getOrInsertFunction("acos", acosType);

  auto atanType =
      llvm::FunctionType::get(llvm::Type::getDoubleTy(*context),
                              {llvm::Type::getDoubleTy(*context)}, false);
  module->getOrInsertFunction("atan", atanType);

  auto atan2Type = llvm::FunctionType::get(
      llvm::Type::getDoubleTy(*context),
      {llvm::Type::getDoubleTy(*context), llvm::Type::getDoubleTy(*context)},
      false);
  module->getOrInsertFunction("atan2", atan2Type);

  auto sinhType =
      llvm::FunctionType::get(llvm::Type::getDoubleTy(*context),
                              {llvm::Type::getDoubleTy(*context)}, false);
  module->getOrInsertFunction("sinh", sinhType);

  auto coshType =
      llvm::FunctionType::get(llvm::Type::getDoubleTy(*context),
                              {llvm::Type::getDoubleTy(*context)}, false);
  module->getOrInsertFunction("cosh", coshType);

  auto tanhType =
      llvm::FunctionType::get(llvm::Type::getDoubleTy(*context),
                              {llvm::Type::getDoubleTy(*context)}, false);
  module->getOrInsertFunction("tanh", tanhType);

  auto expType =
      llvm::FunctionType::get(llvm::Type::getDoubleTy(*context),
                              {llvm::Type::getDoubleTy(*context)}, false);
  module->getOrInsertFunction("exp", expType);

  auto exp2Type =
      llvm::FunctionType::get(llvm::Type::getDoubleTy(*context),
                              {llvm::Type::getDoubleTy(*context)}, false);
  module->getOrInsertFunction("exp2", exp2Type);

  auto logType =
      llvm::FunctionType::get(llvm::Type::getDoubleTy(*context),
                              {llvm::Type::getDoubleTy(*context)}, false);
  module->getOrInsertFunction("log", logType);

  auto log10Type =
      llvm::FunctionType::get(llvm::Type::getDoubleTy(*context),
                              {llvm::Type::getDoubleTy(*context)}, false);
  module->getOrInsertFunction("log10", log10Type);

  auto log2Type =
      llvm::FunctionType::get(llvm::Type::getDoubleTy(*context),
                              {llvm::Type::getDoubleTy(*context)}, false);
  module->getOrInsertFunction("log2", log2Type);

  auto powType = llvm::FunctionType::get(
      llvm::Type::getDoubleTy(*context),
      {llvm::Type::getDoubleTy(*context), llvm::Type::getDoubleTy(*context)},
      false);
  module->getOrInsertFunction("pow", powType);

  auto sqrtType =
      llvm::FunctionType::get(llvm::Type::getDoubleTy(*context),
                              {llvm::Type::getDoubleTy(*context)}, false);
  module->getOrInsertFunction("sqrt", sqrtType);

  auto cbrtType =
      llvm::FunctionType::get(llvm::Type::getDoubleTy(*context),
                              {llvm::Type::getDoubleTy(*context)}, false);
  module->getOrInsertFunction("cbrt", cbrtType);

  auto hypotType = llvm::FunctionType::get(
      llvm::Type::getDoubleTy(*context),
      {llvm::Type::getDoubleTy(*context), llvm::Type::getDoubleTy(*context)},
      false);
  module->getOrInsertFunction("hypot", hypotType);

  auto floorType =
      llvm::FunctionType::get(llvm::Type::getDoubleTy(*context),
                              {llvm::Type::getDoubleTy(*context)}, false);
  module->getOrInsertFunction("floor", floorType);

  auto ceilType =
      llvm::FunctionType::get(llvm::Type::getDoubleTy(*context),
                              {llvm::Type::getDoubleTy(*context)}, false);
  module->getOrInsertFunction("ceil", ceilType);

  auto roundType =
      llvm::FunctionType::get(llvm::Type::getDoubleTy(*context),
                              {llvm::Type::getDoubleTy(*context)}, false);
  module->getOrInsertFunction("round", roundType);

  auto truncType =
      llvm::FunctionType::get(llvm::Type::getDoubleTy(*context),
                              {llvm::Type::getDoubleTy(*context)}, false);
  module->getOrInsertFunction("trunc", truncType);

  auto fabsType =
      llvm::FunctionType::get(llvm::Type::getDoubleTy(*context),
                              {llvm::Type::getDoubleTy(*context)}, false);
  module->getOrInsertFunction("fabs", fabsType);

  auto fabsfType =
      llvm::FunctionType::get(llvm::Type::getFloatTy(*context),
                              {llvm::Type::getFloatTy(*context)}, false);
  module->getOrInsertFunction("fabsf", fabsfType);

  auto lldivType = llvm::FunctionType::get(
      llvm::Type::getInt64Ty(*context),
      {llvm::Type::getInt64Ty(*context), llvm::Type::getInt64Ty(*context)},
      false);
  module->getOrInsertFunction("lldiv", lldivType);

  auto randType =
      llvm::FunctionType::get(llvm::Type::getInt32Ty(*context), {}, false);
  module->getOrInsertFunction("rand", randType);

  auto srandType =
      llvm::FunctionType::get(llvm::Type::getVoidTy(*context),
                              {llvm::Type::getInt32Ty(*context)}, false);
  module->getOrInsertFunction("srand", srandType);

  auto setArgsType = llvm::FunctionType::get(
      llvm::Type::getVoidTy(*context),
      {llvm::Type::getInt32Ty(*context),
       llvm::PointerType::get(llvm::Type::getInt8Ty(*context), 0)},
      false);
  setArgsFunc = llvm::cast<llvm::Function>(
      module->getOrInsertFunction("meadows_set_args", setArgsType).getCallee());

  currentFunction = nullptr;
  exprResult = nullptr;
  currentBlock = nullptr;
}

void CodeGen::generate(const std::vector<std::unique_ptr<Stmt>> &statements) {
  variableScopeStack.clear();

  bool hasUserMain = false;
  for (const auto &stmt : statements) {
    if (auto funcStmt = dynamic_cast<FuncStmt *>(stmt.get())) {
      if (funcStmt->name == "main") {
        hasUserMain = true;
        break;
      }
    }
  }

  std::vector<llvm::Type *> mainArgTypes = {
      llvm::Type::getInt32Ty(*context),
      llvm::PointerType::get(llvm::Type::getInt8Ty(*context), 0)};
  auto mainType = llvm::FunctionType::get(llvm::Type::getInt32Ty(*context),
                                          mainArgTypes, false);

  llvm::Function *wrapperMain = llvm::Function::Create(
      mainType, llvm::Function::ExternalLinkage, "main", module.get());
  llvm::BasicBlock *entry =
      llvm::BasicBlock::Create(*context, "entry", wrapperMain);
  builder->SetInsertPoint(entry);
  currentFunction = wrapperMain;
  currentBlock = entry;

  auto argsIter = wrapperMain->arg_begin();
  llvm::Value *argcArg = &*argsIter;
  argcArg->setName("argc");
  llvm::Value *argvArg = &*(++argsIter);
  argvArg->setName("argv");

  builder->CreateCall(setArgsFunc, {argcArg, argvArg});

  enterScope();

  llvm::Function *userMainFunc = nullptr;
  for (auto &stmt : statements) {
    if (auto funcStmt = dynamic_cast<FuncStmt *>(stmt.get())) {
      if (funcStmt->name == "main") {
        funcStmt->name = "_meadows_user_main";
      }
    }
    stmt->accept(*this);
  }

  userMainFunc = module->getFunction("_meadows_user_main");

  if (userMainFunc) {
    std::vector<llvm::Value *> args;
    if (userMainFunc->arg_size() > 0) {
      args.push_back(argcArg);
    }
    if (userMainFunc->arg_size() > 1) {
      args.push_back(argvArg);
    }
    builder->CreateCall(userMainFunc, args);
  }

  exitScope();

  if (optimize_) {
    module->print(llvm::errs(), nullptr);
  }

  // Add return to the current block (which may be entry or a block after if
  // statements)
  if (!builder->GetInsertBlock()->getTerminator()) {
    builder->CreateRet(
        llvm::ConstantInt::get(llvm::Type::getInt32Ty(*context), 0));
  }
}

void CodeGen::freeAllocatedStrings() {
  // Note: We're not freeing strings here because they may be allocated
  // in different basic blocks (e.g., inside loops). The OS will reclaim
  // this memory when the program exits. For a production compiler, we'd
  // need more sophisticated lifetime tracking.
  allocatedStrings.clear();
}

std::unique_ptr<llvm::Module> CodeGen::getModule() { return std::move(module); }

void CodeGen::visitLiteralExpr(LiteralExpr &expr) {
  if (!expr.value.empty() && isdigit(expr.value[0])) {
    if (expr.value.find('.') != std::string::npos ||
        expr.value.find('e') != std::string::npos ||
        expr.value.find('E') != std::string::npos) {
      try {
        exprResult = llvm::ConstantFP::get(llvm::Type::getDoubleTy(*context),
                                           std::stod(expr.value));
      } catch (const std::out_of_range &) {
        exprResult =
            llvm::ConstantFP::get(llvm::Type::getDoubleTy(*context), 0.0);
      }
    } else {
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
    llvm::Value *leftVal = left;
    llvm::Value *rightVal = right;
    if (left->getType() != right->getType()) {
      auto leftTy = left->getType();
      auto rightTy = right->getType();
      if (leftTy->isIntegerTy() && rightTy->isIntegerTy()) {
        auto widerTy =
            (leftTy->getIntegerBitWidth() >= rightTy->getIntegerBitWidth())
                ? leftTy
                : rightTy;
        if (leftTy != widerTy)
          leftVal = builder->CreateZExt(left, widerTy);
        if (rightTy != widerTy)
          rightVal = builder->CreateZExt(right, widerTy);
      }
    }
    exprResult = builder->CreateICmpEQ(leftVal, rightVal);
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
  } else if (expr.op == "|") {
    exprResult = builder->CreateOr(left, right);
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

void CodeGen::visitTryExpr(TryExpr &expr) { expr.expr->accept(*this); }

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
  std::string funcName;
  auto varExpr = dynamic_cast<VarExpr *>(expr.callee.get());
  if (varExpr) {
    funcName = varExpr->name;
  } else {
    auto fieldAccess = dynamic_cast<FieldAccessExpr *>(expr.callee.get());
    if (!fieldAccess) {
      error("Only variable and field access calls supported");
    }
    funcName = fieldAccess->fieldName;
  }

  auto it = externNameMapping.find(funcName);
  if (it != externNameMapping.end()) {
    funcName = it->second;
  }

  auto func = module->getFunction(funcName);
  if (!func) {
    error("Undefined function: ", funcName);
  }

  if (func->arg_size() != expr.args.size()) {
    std::ostringstream oss;
    oss << "Function " << funcName << " expects " << func->arg_size()
        << " arguments, got " << expr.args.size();
    error(oss.str());
  }

  std::vector<llvm::Value *> args;
  for (auto &arg : expr.args) {
    arg->accept(*this);
    if (!exprResult) {
      error("Failed to generate argument code for function: ", funcName);
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

llvm::Type *CodeGen::getTypeFromAnnotation(const std::string &annotation) {
  if (annotation == "i64") {
    return llvm::Type::getInt64Ty(*context);
  } else if (annotation == "string") {
    return llvm::PointerType::get(llvm::Type::getInt8Ty(*context), 0);
  } else if (annotation == "f64" || annotation == "double") {
    return llvm::Type::getDoubleTy(*context);
  } else if (annotation == "f32" || annotation == "float") {
    return llvm::Type::getFloatTy(*context);
  } else if (annotation == "void") {
    return llvm::Type::getVoidTy(*context);
  }
  // Default to i32
  return llvm::Type::getInt32Ty(*context);
}

void CodeGen::visitFuncStmt(FuncStmt &stmt) {
  auto savedBlock = builder->GetInsertBlock();
  auto savedFunction = currentFunction;

  // Build parameter types from annotations
  std::vector<llvm::Type *> paramTypes;
  for (auto &param : stmt.params) {
    paramTypes.push_back(getTypeFromAnnotation(param.typeAnnotation));
  }

  // Get return type from annotation
  llvm::Type *returnType = getTypeFromAnnotation(stmt.returnTypeAnnotation);

  auto funcType = llvm::FunctionType::get(returnType, paramTypes, false);
  auto func = llvm::Function::Create(funcType, llvm::Function::ExternalLinkage,
                                     stmt.name, module.get());
  auto entry = llvm::BasicBlock::Create(*context, "entry", func);
  builder->SetInsertPoint(entry);

  enterScope();

  auto it = func->arg_begin();
  for (auto &param : stmt.params) {
    llvm::Type *paramType = getTypeFromAnnotation(param.typeAnnotation);
    auto alloca = builder->CreateAlloca(paramType);
    builder->CreateStore(&*it, alloca);
    declareVariable(param.name, alloca);
    variableTypes[param.name] = paramType;
    ++it;
  }
  currentFunction = func;
  for (auto &s : stmt.body) {
    s->accept(*this);
  }

  // Free allocated strings before function return
  freeAllocatedStrings();

  if (!builder->GetInsertBlock()->getTerminator()) {
    if (returnType->isVoidTy()) {
      builder->CreateRetVoid();
    } else {
      builder->CreateRet(llvm::ConstantInt::get(returnType, 0));
    }
  }

  exitScope();

  builder->SetInsertPoint(savedBlock);
  currentFunction = savedFunction;
}

void CodeGen::visitIfStmt(IfStmt &stmt) {
  stmt.condition->accept(*this);
  auto cond = exprResult;

  // Convert condition to boolean if needed
  llvm::Value *boolCond = nullptr;
  if (cond->getType()->isIntegerTy()) {
    boolCond = builder->CreateICmpNE(
        cond, llvm::ConstantInt::get(cond->getType(), 0), "boolCond");
  } else {
    boolCond = cond;
  }

  llvm::Function *currentFunc = builder->GetInsertBlock()->getParent();

  // Create all the blocks upfront
  auto thenBB = llvm::BasicBlock::Create(*context, "then", currentFunc);
  auto elseBB = llvm::BasicBlock::Create(*context, "else", currentFunc);
  auto endBB = llvm::BasicBlock::Create(*context, "endif", currentFunc);
  auto afterIf = llvm::BasicBlock::Create(*context, "afterif", currentFunc);

  // Branch from current block to then or else
  builder->CreateCondBr(boolCond, thenBB, elseBB);

  // Generate then branch
  builder->SetInsertPoint(thenBB);
  for (auto &s : stmt.thenBranch)
    s->accept(*this);
  if (!builder->GetInsertBlock()->getTerminator())
    builder->CreateBr(endBB);

  // Generate else branch - recursively handle else-if chains
  builder->SetInsertPoint(elseBB);
  generateElseBranch(stmt.elseBranch, endBB);

  // Add branch from endif to afterif
  builder->SetInsertPoint(endBB);
  builder->CreateBr(afterIf);

  // Continue after the if statement in the new block
  builder->SetInsertPoint(afterIf);
}

void CodeGen::generateElseBranch(
    const std::vector<std::unique_ptr<Stmt>> &elseBranch,
    llvm::BasicBlock *endBB) {
  if (elseBranch.empty()) {
    builder->CreateBr(endBB);
    return;
  }

  // Check if else branch is a single if statement (else-if chain)
  if (elseBranch.size() == 1) {
    auto &firstStmt = elseBranch[0];
    auto *nestedIf = dynamic_cast<IfStmt *>(firstStmt.get());
    if (nestedIf) {
      // This is an else-if: generate nested if condition
      nestedIf->condition->accept(*this);
      auto cond = exprResult;

      llvm::Value *boolCond = nullptr;
      if (cond->getType()->isIntegerTy()) {
        boolCond = builder->CreateICmpNE(
            cond, llvm::ConstantInt::get(cond->getType(), 0), "boolCond");
      } else {
        boolCond = cond;
      }

      llvm::Function *currentFunc = builder->GetInsertBlock()->getParent();

      auto nestedThenBB =
          llvm::BasicBlock::Create(*context, "then", currentFunc);
      auto nestedElseBB =
          llvm::BasicBlock::Create(*context, "else", currentFunc);

      builder->CreateCondBr(boolCond, nestedThenBB, nestedElseBB);

      // Nested then branch
      builder->SetInsertPoint(nestedThenBB);
      for (auto &s : nestedIf->thenBranch)
        s->accept(*this);
      if (!builder->GetInsertBlock()->getTerminator())
        builder->CreateBr(endBB);

      // Nested else branch (recursively handle else-if)
      builder->SetInsertPoint(nestedElseBB);
      generateElseBranch(nestedIf->elseBranch, endBB);

      return;
    }
  }

  // Regular else block (multiple statements or not an if)
  for (auto &s : elseBranch) {
    s->accept(*this);
  }

  // Always branch to end - print statements don't have terminators
  llvm::BasicBlock *current = builder->GetInsertBlock();
  bool hasTerminator = current->getTerminator() != nullptr;
  if (!hasTerminator) {
    builder->CreateBr(endBB);
  }
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

  llvm::Value *boolCond = nullptr;
  if (cond->getType()->isIntegerTy()) {
    boolCond = builder->CreateICmpNE(
        cond, llvm::ConstantInt::get(cond->getType(), 0), "whileCond");
  } else {
    boolCond = cond;
  }

  builder->CreateCondBr(boolCond, bodyBB, endBB);
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
  } else if (val->getType()->isDoubleTy()) {
    auto format = builder->CreateGlobalString("%g\n");
    builder->CreateCall(printfFunc, {format, val});
  } else if (val->getType()->isFloatTy()) {
    auto format = builder->CreateGlobalString("%g\n");
    auto valDouble =
        builder->CreateFPExt(val, llvm::Type::getDoubleTy(*context));
    builder->CreateCall(printfFunc, {format, valDouble});
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

void CodeGen::visitTypeDefStmt(TypeDefStmt &stmt) {
  if (stmt.isEnum) {
    std::vector<llvm::Type *> elementTypes;
    elementTypes.push_back(llvm::Type::getInt32Ty(*context));

    size_t maxPayloadSize = 0;
    for (const auto &variant : stmt.variants) {
      if (variant.second.size() > maxPayloadSize) {
        maxPayloadSize = variant.second.size();
      }
    }

    for (size_t i = 0; i < maxPayloadSize; ++i) {
      elementTypes.push_back(llvm::Type::getInt64Ty(*context));
    }

    auto enumStructType =
        llvm::StructType::create(*context, "enum." + stmt.name);
    enumStructType->setBody(elementTypes, false);
    enumTypes_[stmt.name] = enumStructType;
    definedEnums_[stmt.name] = std::vector<std::string>();
    for (const auto &variant : stmt.variants) {
      definedEnums_[stmt.name].push_back(variant.first);
    }
  } else {
    std::vector<llvm::Type *> fieldTypes;
    for (const auto &field : stmt.fields) {
      llvm::Type *fieldType = llvm::Type::getInt64Ty(*context);
      if (field.second == "i32") {
        fieldType = llvm::Type::getInt32Ty(*context);
      } else if (field.second == "i64") {
        fieldType = llvm::Type::getInt64Ty(*context);
      } else if (field.second == "f32") {
        fieldType = llvm::Type::getFloatTy(*context);
      } else if (field.second == "f64") {
        fieldType = llvm::Type::getDoubleTy(*context);
      } else if (field.second == "bool") {
        fieldType = llvm::Type::getInt1Ty(*context);
      } else if (field.second == "string") {
        fieldType = llvm::PointerType::get(llvm::Type::getInt8Ty(*context), 0);
      }
      fieldTypes.push_back(fieldType);
    }

    auto structType = llvm::StructType::create(*context, "struct." + stmt.name);
    structType->setBody(fieldTypes, false);
    structTypes_[stmt.name] = structType;
  }
}

void CodeGen::visitMatchExpr(MatchExpr &expr) {
  expr.scrutinee->accept(*this);
  auto scrutinee = exprResult;

  auto scrutineeType = scrutinee->getType();
  llvm::Value *scrutineeAlloca = nullptr;

  if (llvm::isa<llvm::LoadInst>(scrutinee)) {
    scrutineeAlloca = llvm::cast<llvm::LoadInst>(scrutinee)->getOperand(0);
  } else if (llvm::isa<llvm::AllocaInst>(scrutinee)) {
    scrutineeAlloca = scrutinee;
  } else {
    auto alloc =
        builder->CreateAlloca(scrutineeType, nullptr, "match_scrutinee");
    builder->CreateStore(scrutinee, alloc);
    scrutineeAlloca = alloc;
    scrutinee =
        builder->CreateLoad(scrutineeType, alloc, "match_scrutinee_val");
  }

  llvm::Value *result = llvm::UndefValue::get(scrutineeType);

  for (size_t i = expr.arms.size(); i > 0; --i) {
    size_t idx = i - 1;
    auto &arm = expr.arms[idx];

    std::unordered_map<std::string, llvm::Value *> matchBindings;
    auto *matchCondition = generateMatchArm(
        *arm.pattern, scrutinee, scrutineeAlloca, *arm.body, matchBindings);

    arm.body->accept(*this);
    llvm::Value *armResult = exprResult;

    if (auto *c = llvm::dyn_cast<llvm::ConstantInt>(matchCondition)) {
      if (c->isOne()) {
        result = armResult;
      } else {
        result =
            builder->CreateSelect(matchCondition, armResult, result, "select");
      }
    } else if (matchCondition) {
      result =
          builder->CreateSelect(matchCondition, armResult, result, "select");
    } else {
      result = armResult;
    }
  }

  exprResult = result;
}

llvm::Value *CodeGen::generateMatchArm(
    Pattern &pattern, llvm::Value *scrutinee, llvm::Value *scrutineeAlloca,
    Expr &body, std::unordered_map<std::string, llvm::Value *> &matchBindings) {
  switch (pattern.kind) {
  case PatternKind::WILDCARD:
    return llvm::ConstantInt::getTrue(*context);

  case PatternKind::LITERAL: {
    int tagValue;
    if (pattern.literalValue == "true") {
      tagValue = 1;
    } else if (pattern.literalValue == "false") {
      tagValue = 0;
    } else {
      tagValue = std::stoi(pattern.literalValue);
    }
    auto caseValue = llvm::ConstantInt::get(scrutinee->getType(), tagValue);
    return builder->CreateICmpEQ(scrutinee, caseValue, "match_literal_cmp");
  }

  case PatternKind::BIND: {
    auto alloca = builder->CreateAlloca(scrutinee->getType(), nullptr,
                                        "bind_" + pattern.name);
    builder->CreateStore(scrutinee, alloca);
    matchBindings[pattern.name] = alloca;
    symbolTable.declare(pattern.name, alloca);
    return llvm::ConstantInt::getTrue(*context);
  }

  case PatternKind::TUPLE: {
    if (pattern.subPatterns.empty()) {
      return llvm::ConstantInt::getTrue(*context);
    }
    llvm::Value *cond = nullptr;
    for (size_t i = 0; i < pattern.subPatterns.size(); ++i) {
      auto *elem = builder->CreateExtractValue(
          scrutinee, {static_cast<unsigned int>(i)}, "tuple_elem");
      std::unordered_map<std::string, llvm::Value *> subBindings;
      auto *elemCond = generateMatchArm(*pattern.subPatterns[i], elem, nullptr,
                                        body, subBindings);
      if (i == 0) {
        cond = elemCond;
      } else {
        cond = builder->CreateAnd(cond, elemCond, "tuple_and");
      }
    }
    return cond ? cond : llvm::ConstantInt::getTrue(*context);
  }

  case PatternKind::STRUCT: {
    if (pattern.subPatterns.empty() || !scrutineeAlloca) {
      return llvm::ConstantInt::getTrue(*context);
    }
    auto structType = llvm::cast<llvm::StructType>(scrutinee->getType());
    llvm::Value *cond = nullptr;
    for (size_t i = 0; i < pattern.subPatterns.size(); ++i) {
      auto *fieldPtr = builder->CreateStructGEP(structType, scrutineeAlloca, i,
                                                "struct_field_ptr");
      auto *fieldVal = builder->CreateLoad(structType->getElementType(i),
                                           fieldPtr, "struct_field");
      std::unordered_map<std::string, llvm::Value *> subBindings;
      auto *fieldCond = generateMatchArm(*pattern.subPatterns[i], fieldVal,
                                         fieldPtr, body, subBindings);
      if (i == 0) {
        cond = fieldCond;
      } else {
        cond = builder->CreateAnd(cond, fieldCond, "struct_and");
      }
    }
    return cond ? cond : llvm::ConstantInt::getTrue(*context);
  }

  case PatternKind::ENUM: {
    auto enumStructType = enumTypes_[pattern.typeName];
    if (!enumStructType || !scrutineeAlloca) {
      auto caseValue = llvm::ConstantInt::get(scrutinee->getType(), 0);
      return builder->CreateICmpEQ(scrutinee, caseValue, "match_enum_cmp");
    }
    auto tagPtr = builder->CreateStructGEP(enumStructType, scrutineeAlloca, 0,
                                           "enum_tag_ptr");
    auto tagVal = builder->CreateLoad(llvm::Type::getInt32Ty(*context), tagPtr,
                                      "enum_tag");
    int variantTag = 0;
    if (definedEnums_.find(pattern.typeName) != definedEnums_.end()) {
      const auto &variants = definedEnums_[pattern.typeName];
      for (size_t i = 0; i < variants.size(); ++i) {
        if (variants[i] == pattern.name) {
          variantTag = static_cast<int>(i);
          break;
        }
      }
    }
    auto expectedTag =
        llvm::ConstantInt::get(llvm::Type::getInt32Ty(*context), variantTag);
    auto tagCmp = builder->CreateICmpEQ(tagVal, expectedTag, "enum_tag_cmp");

    llvm::Value *cond = tagCmp;
    if (!pattern.subPatterns.empty()) {
      llvm::Value *dataCond = nullptr;
      for (size_t i = 0; i < pattern.subPatterns.size(); ++i) {
        auto *dataPtr = builder->CreateStructGEP(
            enumStructType, scrutineeAlloca, i + 1, "enum_data_ptr");
        auto *dataVal = builder->CreateLoad(
            enumStructType->getElementType(i + 1), dataPtr, "enum_data");
        std::unordered_map<std::string, llvm::Value *> subBindings;
        auto *dataCmp = generateMatchArm(*pattern.subPatterns[i], dataVal,
                                         dataPtr, body, subBindings);
        if (i == 0) {
          dataCond = dataCmp;
        } else {
          dataCond = builder->CreateAnd(dataCond, dataCmp, "enum_data_and");
        }
      }
      cond = builder->CreateAnd(cond, dataCond, "enum_cond");
    }
    return cond;
  }

  case PatternKind::OR: {
    if (pattern.subPatterns.empty()) {
      return llvm::ConstantInt::getTrue(*context);
    }
    llvm::Value *cond = nullptr;
    for (size_t i = 0; i < pattern.subPatterns.size(); ++i) {
      std::unordered_map<std::string, llvm::Value *> subBindings;
      auto *subCond = generateMatchArm(*pattern.subPatterns[i], scrutinee,
                                       scrutineeAlloca, body, subBindings);
      if (i == 0) {
        cond = subCond;
      } else {
        cond = builder->CreateOr(cond, subCond, "or_pattern");
      }
    }
    return cond ? cond : llvm::ConstantInt::getTrue(*context);
  }
  }

  return llvm::ConstantInt::getTrue(*context);
}

void CodeGen::visitEnumVariantExpr(EnumVariantExpr &expr) {
  auto enumStructType = enumTypes_[expr.enumName];
  if (!enumStructType) {
    error("Unknown enum type: " + expr.enumName);
    return;
  }

  auto alloc = builder->CreateAlloca(enumStructType);

  int variantTag = 0;
  if (definedEnums_.find(expr.enumName) != definedEnums_.end()) {
    const auto &variants = definedEnums_[expr.enumName];
    for (size_t i = 0; i < variants.size(); ++i) {
      if (variants[i] == expr.variantName) {
        variantTag = i;
        break;
      }
    }
  }

  auto tagValue =
      llvm::ConstantInt::get(llvm::Type::getInt32Ty(*context), variantTag);
  auto tagPtr = builder->CreateStructGEP(enumStructType, alloc, 0);
  builder->CreateStore(tagValue, tagPtr);

  for (size_t i = 0; i < expr.args.size(); ++i) {
    expr.args[i]->accept(*this);
    auto argValue = exprResult;
    auto argPtr = builder->CreateStructGEP(enumStructType, alloc, i + 1);
    builder->CreateStore(argValue, argPtr);
  }

  exprResult = builder->CreateLoad(enumStructType, alloc);
}

void CodeGen::visitModuleStmt(ModuleStmt &stmt) {
  // Module statements are handled at the compilation unit level
  // They don't generate LLVM IR directly
}

void CodeGen::visitImportStmt(ImportStmt &stmt) {
  // Import statements are resolved before code generation
  // They don't generate LLVM IR directly
}

void CodeGen::visitExportStmt(ExportStmt &stmt) {
  // Export statements are handled at the module level
  // They don't generate LLVM IR directly
}

void CodeGen::visitExternStmt(ExternStmt &stmt) {
  llvm::Type *returnType = llvm::Type::getInt32Ty(*context);

  if (stmt.returnType == "i32") {
    returnType = llvm::Type::getInt32Ty(*context);
  } else if (stmt.returnType == "i64") {
    returnType = llvm::Type::getInt64Ty(*context);
  } else if (stmt.returnType == "f32") {
    returnType = llvm::Type::getFloatTy(*context);
  } else if (stmt.returnType == "f64") {
    returnType = llvm::Type::getDoubleTy(*context);
  } else if (stmt.returnType == "bool") {
    returnType = llvm::Type::getInt1Ty(*context);
  } else if (stmt.returnType == "string") {
    returnType = llvm::PointerType::get(llvm::Type::getInt8Ty(*context), 0);
  } else if (stmt.returnType == "void") {
    returnType = llvm::Type::getVoidTy(*context);
  }

  std::vector<llvm::Type *> paramTypes;
  for (const auto &param : stmt.params) {
    llvm::Type *paramType = llvm::Type::getInt32Ty(*context);
    if (param.second == "i32") {
      paramType = llvm::Type::getInt32Ty(*context);
    } else if (param.second == "i64") {
      paramType = llvm::Type::getInt64Ty(*context);
    } else if (param.second == "f32") {
      paramType = llvm::Type::getFloatTy(*context);
    } else if (param.second == "f64") {
      paramType = llvm::Type::getDoubleTy(*context);
    } else if (param.second == "bool") {
      paramType = llvm::Type::getInt1Ty(*context);
    } else if (param.second == "string") {
      paramType = llvm::PointerType::get(llvm::Type::getInt8Ty(*context), 0);
    }
    paramTypes.push_back(paramType);
  }

  auto funcType = llvm::FunctionType::get(returnType, paramTypes, false);
  auto func = llvm::Function::Create(funcType, llvm::Function::ExternalLinkage,
                                     stmt.cName, module.get());
  func->setCallingConv(llvm::CallingConv::C);
  externNameMapping[stmt.meadowsName] = stmt.cName;
}