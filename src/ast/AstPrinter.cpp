#include "AstPrinter.h"

#include <cstdio>

namespace meadows {

std::string AstPrinter::print(const StmtList& statements) {
  out_.clear();
  depth_ = 0;
  for (const StmtPtr& stmt : statements) {
    stmt->accept(*this);
  }
  return out_;
}

void AstPrinter::line(const std::string& text) {
  out_ += std::string(static_cast<size_t>(depth_) * 2, ' ') + text + "\n";
}

void AstPrinter::child(Expr& expr) {
  ++depth_;
  expr.accept(*this);
  --depth_;
}

void AstPrinter::children(StmtList& statements) {
  ++depth_;
  for (StmtPtr& stmt : statements) {
    stmt->accept(*this);
  }
  --depth_;
}

void AstPrinter::visitLiteralExpr(LiteralExpr& expr) {
  switch (expr.kind) {
  case LiteralKind::Int:
    line("Int " + std::to_string(expr.intValue));
    break;
  case LiteralKind::Float: {
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%g", expr.floatValue);
    line(std::string("Float ") + buf);
    break;
  }
  case LiteralKind::String:
    line("String \"" + expr.stringValue + "\"");
    break;
  }
}

void AstPrinter::visitVarExpr(VarExpr& expr) { line("Var " + expr.name); }

void AstPrinter::visitAssignExpr(AssignExpr& expr) {
  line("Assign");
  child(*expr.target);
  child(*expr.value);
}

void AstPrinter::visitBinaryExpr(BinaryExpr& expr) {
  line(std::string("Binary ") + binaryOpSymbol(expr.op));
  child(*expr.left);
  child(*expr.right);
}

void AstPrinter::visitUnaryExpr(UnaryExpr& expr) {
  line(std::string("Unary ") + unaryOpSymbol(expr.op));
  child(*expr.operand);
}

void AstPrinter::visitLogicalExpr(LogicalExpr& expr) {
  line(std::string("Logical ") + logicalOpSymbol(expr.op));
  child(*expr.left);
  child(*expr.right);
}

void AstPrinter::visitIndexExpr(IndexExpr& expr) {
  line("Index");
  child(*expr.object);
  child(*expr.index);
}

void AstPrinter::visitFieldExpr(FieldExpr& expr) {
  line("Field " + expr.name);
  child(*expr.object);
}

void AstPrinter::visitCallExpr(CallExpr& expr) {
  line("Call");
  child(*expr.callee);
  for (ExprPtr& arg : expr.args) {
    child(*arg);
  }
}

void AstPrinter::visitArrayExpr(ArrayExpr& expr) {
  line("Array");
  for (ExprPtr& element : expr.elements) {
    child(*element);
  }
}

void AstPrinter::visitObjectExpr(ObjectExpr& expr) {
  line("Object");
  ++depth_;
  for (ObjectField& field : expr.fields) {
    line("Field " + field.name);
    child(*field.value);
  }
  --depth_;
}

void AstPrinter::visitExprStmt(ExprStmt& stmt) {
  line("ExprStmt");
  child(*stmt.expr);
}

void AstPrinter::visitLetStmt(LetStmt& stmt) {
  line("Let " + stmt.name);
  child(*stmt.initializer);
}

void AstPrinter::visitFuncStmt(FuncStmt& stmt) {
  std::string params;
  for (const Param& param : stmt.params) {
    params += (params.empty() ? "" : ", ") + param.name;
  }
  line("Func " + stmt.name + "(" + params + ")");
  children(stmt.body);
}

void AstPrinter::visitIfStmt(IfStmt& stmt) {
  line("If");
  child(*stmt.condition);
  line("Then");
  children(stmt.thenBody);
  if (!stmt.elseBody.empty()) {
    line("Else");
    children(stmt.elseBody);
  }
}

void AstPrinter::visitForStmt(ForStmt& stmt) {
  line("For " + stmt.var);
  child(*stmt.start);
  child(*stmt.end);
  children(stmt.body);
}

void AstPrinter::visitWhileStmt(WhileStmt& stmt) {
  line("While");
  child(*stmt.condition);
  children(stmt.body);
}

void AstPrinter::visitReturnStmt(ReturnStmt& stmt) {
  line("Return");
  if (stmt.value) {
    child(*stmt.value);
  }
}

void AstPrinter::visitBreakStmt(BreakStmt&) { line("Break"); }

void AstPrinter::visitContinueStmt(ContinueStmt&) { line("Continue"); }

void AstPrinter::visitBlockStmt(BlockStmt& stmt) {
  line("Block");
  children(stmt.body);
}

} // namespace meadows
