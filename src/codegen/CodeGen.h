#ifndef CODEGEN_H
#define CODEGEN_H

#include "../ast/AST.h"
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <map>

class CodeGen : public ExprVisitor, public StmtVisitor {
public:
  CodeGen();
  void generate(const std::vector<std::unique_ptr<Stmt>> &statements);
  std::unique_ptr<llvm::Module> getModule();

private:
  std::unique_ptr<llvm::LLVMContext> context;
  std::unique_ptr<llvm::Module> module;
  std::unique_ptr<llvm::IRBuilder<>> builder;
  std::map<std::string, llvm::Value *> variables;
  llvm::Function *printfFunc;
  llvm::Function *currentFunction;
  llvm::Value *exprResult;
  llvm::BasicBlock *currentBlock;

  void visitLiteralExpr(LiteralExpr &expr) override;
  void visitVarExpr(VarExpr &expr) override;
  void visitBinaryExpr(BinaryExpr &expr) override;
  void visitCallExpr(CallExpr &expr) override;
  void visitArrayExpr(ArrayExpr &expr) override;
  void visitObjectExpr(ObjectExpr &expr) override;

  void visitExprStmt(ExprStmt &stmt) override;
  void visitLetStmt(LetStmt &stmt) override;
  void visitFuncStmt(FuncStmt &stmt) override;
  void visitIfStmt(IfStmt &stmt) override;
  void visitForStmt(ForStmt &stmt) override;
  void visitWhileStmt(WhileStmt &stmt) override;
  void visitReturnStmt(ReturnStmt &stmt) override;
  void visitPrintStmt(PrintStmt &stmt) override;
};

#endif