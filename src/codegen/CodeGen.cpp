#include "CodeGen.h"
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

    auto printfType = llvm::FunctionType::get(llvm::Type::getInt32Ty(*context), {llvm::PointerType::get(llvm::Type::getInt8Ty(*context), 0)}, true);
    printfFunc = llvm::cast<llvm::Function>(module->getOrInsertFunction("printf", printfType).getCallee());
    currentFunction = nullptr;
    exprResult = nullptr;
    currentBlock = nullptr;
}

void CodeGen::generate(const std::vector<std::unique_ptr<Stmt>>& statements) {
    // Create main function
    auto mainType = llvm::FunctionType::get(llvm::Type::getInt32Ty(*context), {}, false);
    auto mainFunc = llvm::Function::Create(mainType, llvm::Function::ExternalLinkage, "main", module.get());
    auto entry = llvm::BasicBlock::Create(*context, "entry", mainFunc);
    builder->SetInsertPoint(entry);
    currentFunction = mainFunc;
    currentBlock = entry;
    variables.clear();

    for (auto& stmt : statements) {
        stmt->accept(*this);
    }

    builder->CreateRet(llvm::ConstantInt::get(llvm::Type::getInt32Ty(*context), 0));
}

std::unique_ptr<llvm::Module> CodeGen::getModule() {
    return std::move(module);
}

void CodeGen::visitLiteralExpr(LiteralExpr& expr) {
    if (!expr.value.empty() && isdigit(expr.value[0])) {
        exprResult = llvm::ConstantInt::get(llvm::Type::getInt32Ty(*context), llvm::APInt(32, std::stoi(expr.value)));
    } else {
        exprResult = builder->CreateGlobalString(expr.value);
    }
}

void CodeGen::visitVarExpr(VarExpr& expr) {
    auto var = variables[expr.name];
    exprResult = builder->CreateLoad(llvm::Type::getInt32Ty(*context), var);
}

void CodeGen::visitBinaryExpr(BinaryExpr& expr) {
    expr.left->accept(*this);
    auto left = exprResult;
    expr.right->accept(*this);
    auto right = exprResult;
    if (expr.op == "+") {
        if (left->getType()->isIntegerTy() && right->getType()->isIntegerTy()) {
            exprResult = builder->CreateAdd(left, right);
        } else {
            // For simplicity, assume string concat not implemented
            throw std::runtime_error("String concat not implemented");
        }
    } else if (expr.op == "-") {
        if (expr.left) {
            exprResult = builder->CreateSub(left, right);
        } else {
            exprResult = builder->CreateNeg(right);
        }
    } else if (expr.op == "*") exprResult = builder->CreateMul(left, right);
    else if (expr.op == "/") exprResult = builder->CreateSDiv(left, right);
    else if (expr.op == "==") exprResult = builder->CreateICmpEQ(left, right);
    else if (expr.op == ">") exprResult = builder->CreateICmpSGT(left, right);
    else if (expr.op == "<") exprResult = builder->CreateICmpSLT(left, right);
    else if (expr.op == ">=") exprResult = builder->CreateICmpSGE(left, right);
    else if (expr.op == "<=") exprResult = builder->CreateICmpSLE(left, right);
    else exprResult = nullptr;
}

void CodeGen::visitCallExpr(CallExpr& expr) {
    auto varExpr = dynamic_cast<VarExpr*>(expr.callee.get());
    if (!varExpr) throw std::runtime_error("Only variable calls supported");
    auto func = module->getFunction(varExpr->name);
    std::vector<llvm::Value*> args;
    for (auto& arg : expr.args) {
        arg->accept(*this);
        args.push_back(exprResult);
    }
    exprResult = builder->CreateCall(func, args);
}

void CodeGen::visitArrayExpr(ArrayExpr& expr) {
    throw std::runtime_error("Arrays not supported yet");
}

void CodeGen::visitObjectExpr(ObjectExpr& expr) {
    throw std::runtime_error("Objects not supported yet");
}

void CodeGen::visitExprStmt(ExprStmt& stmt) {
    stmt.expr->accept(*this);
}

void CodeGen::visitLetStmt(LetStmt& stmt) {
    stmt.initializer->accept(*this);
    auto val = exprResult;
    auto alloca = builder->CreateAlloca(val->getType());
    builder->CreateStore(val, alloca);
    variables[stmt.name] = alloca;
}

void CodeGen::visitFuncStmt(FuncStmt& stmt) {
    auto savedBlock = builder->GetInsertBlock();
    auto savedFunction = currentFunction;
    std::vector<llvm::Type*> paramTypes(stmt.params.size(), llvm::Type::getInt32Ty(*context));
    auto funcType = llvm::FunctionType::get(llvm::Type::getInt32Ty(*context), paramTypes, false);
    auto func = llvm::Function::Create(funcType, llvm::Function::ExternalLinkage, stmt.name, module.get());
    auto entry = llvm::BasicBlock::Create(*context, "entry", func);
    builder->SetInsertPoint(entry);
    variables.clear();
    auto it = func->arg_begin();
    for (auto& param : stmt.params) {
        auto alloca = builder->CreateAlloca(llvm::Type::getInt32Ty(*context));
        builder->CreateStore(&*it, alloca);
        variables[param] = alloca;
        ++it;
    }
    currentFunction = func;
    for (auto& s : stmt.body) {
        s->accept(*this);
    }
    if (!builder->GetInsertBlock()->getTerminator()) {
        builder->CreateRet(llvm::ConstantInt::get(llvm::Type::getInt32Ty(*context), 0));
    }
    builder->SetInsertPoint(savedBlock);
    currentFunction = savedFunction;
}

void CodeGen::visitIfStmt(IfStmt& stmt) {
    stmt.condition->accept(*this);
    auto cond = exprResult;
    auto thenBB = llvm::BasicBlock::Create(*context, "then", currentFunction);
    auto elseBB = llvm::BasicBlock::Create(*context, "else", currentFunction);
    auto endBB = llvm::BasicBlock::Create(*context, "endif", currentFunction);
    builder->CreateCondBr(cond, thenBB, elseBB);
    builder->SetInsertPoint(thenBB);
    for (auto& s : stmt.thenBranch) s->accept(*this);
    if (!builder->GetInsertBlock()->getTerminator()) builder->CreateBr(endBB);
    builder->SetInsertPoint(elseBB);
    for (auto& s : stmt.elseBranch) s->accept(*this);
    if (!builder->GetInsertBlock()->getTerminator()) builder->CreateBr(endBB);
    builder->SetInsertPoint(endBB);
}

void CodeGen::visitForStmt(ForStmt& stmt) {
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
    for (auto& s : stmt.body) s->accept(*this);
    auto next = builder->CreateAdd(current, llvm::ConstantInt::get(llvm::Type::getInt32Ty(*context), 1));
    builder->CreateStore(next, loopVar);
    builder->CreateBr(condBB);
    builder->SetInsertPoint(endBB);
}

void CodeGen::visitWhileStmt(WhileStmt& stmt) {
    auto condBB = llvm::BasicBlock::Create(*context, "cond", currentFunction);
    auto bodyBB = llvm::BasicBlock::Create(*context, "body", currentFunction);
    auto endBB = llvm::BasicBlock::Create(*context, "endwhile", currentFunction);
    builder->CreateBr(condBB);
    builder->SetInsertPoint(condBB);
    stmt.condition->accept(*this);
    auto cond = exprResult;
    builder->CreateCondBr(cond, bodyBB, endBB);
    builder->SetInsertPoint(bodyBB);
    for (auto& s : stmt.body) s->accept(*this);
    builder->CreateBr(condBB);
    builder->SetInsertPoint(endBB);
}

void CodeGen::visitReturnStmt(ReturnStmt& stmt) {
    stmt.value->accept(*this);
    auto val = exprResult;
    builder->CreateRet(val);
}

void CodeGen::visitPrintStmt(PrintStmt& stmt) {
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