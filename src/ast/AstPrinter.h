#ifndef MEADOWS_AST_PRINTER_H
#define MEADOWS_AST_PRINTER_H

#include <string>

#include "AST.h"

namespace meadows {

class AstPrinter : public ExprVisitor, public StmtVisitor {
public:
  std::string print(const StmtList& statements);

private:
  std::string out_;
  int depth_ = 0;

  void line(const std::string& text);
  void child(Expr& expr);
  void children(StmtList& statements);

  void visitLiteralExpr(LiteralExpr& expr) override;
  void visitVarExpr(VarExpr& expr) override;
  void visitAssignExpr(AssignExpr& expr) override;
  void visitBinaryExpr(BinaryExpr& expr) override;
  void visitUnaryExpr(UnaryExpr& expr) override;
  void visitLogicalExpr(LogicalExpr& expr) override;
  void visitIndexExpr(IndexExpr& expr) override;
  void visitFieldExpr(FieldExpr& expr) override;
  void visitCallExpr(CallExpr& expr) override;
  void visitArrayExpr(ArrayExpr& expr) override;
  void visitObjectExpr(ObjectExpr& expr) override;

  void visitExprStmt(ExprStmt& stmt) override;
  void visitLetStmt(LetStmt& stmt) override;
  void visitFuncStmt(FuncStmt& stmt) override;
  void visitIfStmt(IfStmt& stmt) override;
  void visitForStmt(ForStmt& stmt) override;
  void visitWhileStmt(WhileStmt& stmt) override;
  void visitReturnStmt(ReturnStmt& stmt) override;
  void visitBreakStmt(BreakStmt& stmt) override;
  void visitContinueStmt(ContinueStmt& stmt) override;
  void visitBlockStmt(BlockStmt& stmt) override;
};

} // namespace meadows

#endif
