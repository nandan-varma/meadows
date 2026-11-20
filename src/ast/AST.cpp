#include "AST.h"

namespace meadows {

std::unique_ptr<LiteralExpr> LiteralExpr::integer(int32_t value) {
  std::unique_ptr<LiteralExpr> expr(new LiteralExpr(LiteralKind::Int));
  expr->intValue = value;
  return expr;
}

std::unique_ptr<LiteralExpr> LiteralExpr::real(double value) {
  std::unique_ptr<LiteralExpr> expr(new LiteralExpr(LiteralKind::Float));
  expr->floatValue = value;
  return expr;
}

std::unique_ptr<LiteralExpr> LiteralExpr::string(std::string value) {
  std::unique_ptr<LiteralExpr> expr(new LiteralExpr(LiteralKind::String));
  expr->stringValue = std::move(value);
  return expr;
}

void LiteralExpr::accept(ExprVisitor& v) {
  v.visitLiteralExpr(*this);
}
void VarExpr::accept(ExprVisitor& v) {
  v.visitVarExpr(*this);
}
void AssignExpr::accept(ExprVisitor& v) {
  v.visitAssignExpr(*this);
}
void BinaryExpr::accept(ExprVisitor& v) {
  v.visitBinaryExpr(*this);
}
void UnaryExpr::accept(ExprVisitor& v) {
  v.visitUnaryExpr(*this);
}
void LogicalExpr::accept(ExprVisitor& v) {
  v.visitLogicalExpr(*this);
}
void IndexExpr::accept(ExprVisitor& v) {
  v.visitIndexExpr(*this);
}
void FieldExpr::accept(ExprVisitor& v) {
  v.visitFieldExpr(*this);
}
void CallExpr::accept(ExprVisitor& v) {
  v.visitCallExpr(*this);
}
void ArrayExpr::accept(ExprVisitor& v) {
  v.visitArrayExpr(*this);
}
void ObjectExpr::accept(ExprVisitor& v) {
  v.visitObjectExpr(*this);
}

void ExprStmt::accept(StmtVisitor& v) {
  v.visitExprStmt(*this);
}
void LetStmt::accept(StmtVisitor& v) {
  v.visitLetStmt(*this);
}
void FuncStmt::accept(StmtVisitor& v) {
  v.visitFuncStmt(*this);
}
void IfStmt::accept(StmtVisitor& v) {
  v.visitIfStmt(*this);
}
void ForStmt::accept(StmtVisitor& v) {
  v.visitForStmt(*this);
}
void WhileStmt::accept(StmtVisitor& v) {
  v.visitWhileStmt(*this);
}
void ReturnStmt::accept(StmtVisitor& v) {
  v.visitReturnStmt(*this);
}
void BreakStmt::accept(StmtVisitor& v) {
  v.visitBreakStmt(*this);
}
void ContinueStmt::accept(StmtVisitor& v) {
  v.visitContinueStmt(*this);
}
void BlockStmt::accept(StmtVisitor& v) {
  v.visitBlockStmt(*this);
}

const char* binaryOpSymbol(BinaryOp op) {
  switch (op) {
  case BinaryOp::Add:
    return "+";
  case BinaryOp::Sub:
    return "-";
  case BinaryOp::Mul:
    return "*";
  case BinaryOp::Div:
    return "/";
  case BinaryOp::Mod:
    return "%";
  case BinaryOp::Eq:
    return "==";
  case BinaryOp::Ne:
    return "!=";
  case BinaryOp::Lt:
    return "<";
  case BinaryOp::Le:
    return "<=";
  case BinaryOp::Gt:
    return ">";
  case BinaryOp::Ge:
    return ">=";
  }
  return "";
}

const char* logicalOpSymbol(LogicalOp op) {
  return op == LogicalOp::And ? "&&" : "||";
}

const char* unaryOpSymbol(UnaryOp op) {
  return op == UnaryOp::Negate ? "-" : "!";
}

} // namespace meadows
