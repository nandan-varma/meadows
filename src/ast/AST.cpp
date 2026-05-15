#include "AST.h"

void LiteralExpr::accept(ExprVisitor &visitor) {
  visitor.visitLiteralExpr(*this);
}
void VarExpr::accept(ExprVisitor &visitor) { visitor.visitVarExpr(*this); }
void AssignExpr::accept(ExprVisitor &visitor) {
  visitor.visitAssignExpr(*this);
}
void BinaryExpr::accept(ExprVisitor &visitor) {
  visitor.visitBinaryExpr(*this);
}
void UnaryExpr::accept(ExprVisitor &visitor) { visitor.visitUnaryExpr(*this); }
void LogicalExpr::accept(ExprVisitor &visitor) {
  visitor.visitLogicalExpr(*this);
}
void IndexExpr::accept(ExprVisitor &visitor) { visitor.visitIndexExpr(*this); }
void FieldAccessExpr::accept(ExprVisitor &visitor) {
  visitor.visitFieldAccessExpr(*this);
}
void CallExpr::accept(ExprVisitor &visitor) { visitor.visitCallExpr(*this); }
void ArrayExpr::accept(ExprVisitor &visitor) { visitor.visitArrayExpr(*this); }
void ObjectExpr::accept(ExprVisitor &visitor) {
  visitor.visitObjectExpr(*this);
}

void ExprStmt::accept(StmtVisitor &visitor) { visitor.visitExprStmt(*this); }
void LetStmt::accept(StmtVisitor &visitor) { visitor.visitLetStmt(*this); }
void FuncStmt::accept(StmtVisitor &visitor) { visitor.visitFuncStmt(*this); }
void IfStmt::accept(StmtVisitor &visitor) { visitor.visitIfStmt(*this); }
void ForStmt::accept(StmtVisitor &visitor) { visitor.visitForStmt(*this); }
void WhileStmt::accept(StmtVisitor &visitor) { visitor.visitWhileStmt(*this); }
void ReturnStmt::accept(StmtVisitor &visitor) {
  visitor.visitReturnStmt(*this);
}
void BreakStmt::accept(StmtVisitor &visitor) { visitor.visitBreakStmt(*this); }
void ContinueStmt::accept(StmtVisitor &visitor) {
  visitor.visitContinueStmt(*this);
}
void BlockStmt::accept(StmtVisitor &visitor) { visitor.visitBlockStmt(*this); }
