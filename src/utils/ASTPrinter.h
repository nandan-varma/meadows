#ifndef AST_PRINTER_H
#define AST_PRINTER_H

#include "../ast/AST.h"
#include <iostream>
#include <sstream>
#include <string>

/**
 * @brief Prints the AST in a tree-like format for debugging
 */
class ASTPrinter : public ExprVisitor, public StmtVisitor {
private:
  std::ostringstream output_;
  int indentLevel_;
  static constexpr int INDENT_SIZE = 2;

  void indent() {
    for (int i = 0; i < indentLevel_ * INDENT_SIZE; i++) {
      output_ << " ";
    }
  }

  void printLine(const std::string &text) {
    indent();
    output_ << text << "\n";
  }

public:
  ASTPrinter() : indentLevel_(0) {}

  std::string print(const std::vector<std::unique_ptr<Stmt>> &statements) {
    output_.str("");
    output_.clear();
    output_ << "AST:\n";

    for (const auto &stmt : statements) {
      stmt->accept(*this);
    }

    return output_.str();
  }

  std::string str() const { return output_.str(); }

  // Expression visitors
  void visitLiteralExpr(LiteralExpr &expr) override {
    indent();
    output_ << "LiteralExpr(" << expr.value << ")\n";
  }

  void visitVarExpr(VarExpr &expr) override {
    indent();
    output_ << "VarExpr(" << expr.name << ")\n";
  }

  void visitAssignExpr(AssignExpr &expr) override {
    indent();
    output_ << "AssignExpr(" << expr.name << ")\n";
    indentLevel_++;
    expr.value->accept(*this);
    indentLevel_--;
  }

  void visitBinaryExpr(BinaryExpr &expr) override {
    indent();
    output_ << "BinaryExpr(" << expr.op << ")\n";
    indentLevel_++;
    expr.left->accept(*this);
    expr.right->accept(*this);
    indentLevel_--;
  }

  void visitUnaryExpr(UnaryExpr &expr) override {
    indent();
    output_ << "UnaryExpr(" << expr.op << ")\n";
    indentLevel_++;
    expr.operand->accept(*this);
    indentLevel_--;
  }

  void visitLogicalExpr(LogicalExpr &expr) override {
    indent();
    output_ << "LogicalExpr(" << (expr.op == LogicalOperator::AND ? "&&" : "||")
            << ")\n";
    indentLevel_++;
    expr.left->accept(*this);
    expr.right->accept(*this);
    indentLevel_--;
  }

  void visitIndexExpr(IndexExpr &expr) override {
    indent();
    output_ << "IndexExpr\n";
    indentLevel_++;
    expr.array->accept(*this);
    expr.index->accept(*this);
    indentLevel_--;
  }

  void visitCallExpr(CallExpr &expr) override {
    indent();
    output_ << "CallExpr\n";
    indentLevel_++;
    expr.callee->accept(*this);
    for (const auto &arg : expr.args) {
      arg->accept(*this);
    }
    indentLevel_--;
  }

  void visitArrayExpr(ArrayExpr &expr) override {
    indent();
    output_ << "ArrayExpr[" << expr.elements.size() << "]\n";
    indentLevel_++;
    for (const auto &elem : expr.elements) {
      elem->accept(*this);
    }
    indentLevel_--;
  }

  void visitObjectExpr(ObjectExpr &expr) override {
    indent();
    output_ << "ObjectExpr{" << expr.pairs.size() << "}\n";
    indentLevel_++;
    for (const auto &[name, value] : expr.pairs) {
      indent();
      output_ << "Field(" << name << "):\n";
      indentLevel_++;
      value->accept(*this);
      indentLevel_--;
    }
    indentLevel_--;
  }

  void visitFieldAccessExpr(FieldAccessExpr &expr) override {
    indent();
    output_ << "FieldAccessExpr(" << expr.fieldName << ")\n";
    indentLevel_++;
    expr.object->accept(*this);
    indentLevel_--;
  }

  // Statement visitors
  void visitLetStmt(LetStmt &stmt) override {
    indent();
    output_ << "LetStmt(" << stmt.name << ")\n";
    indentLevel_++;
    stmt.initializer->accept(*this);
    indentLevel_--;
  }

  void visitFuncStmt(FuncStmt &stmt) override {
    indent();
    output_ << "FuncStmt(" << stmt.name << ")\n";
    indentLevel_++;
    for (const auto &param : stmt.params) {
      indent();
      output_ << "Param(" << param << ")\n";
    }
    for (const auto &bodyStmt : stmt.body) {
      bodyStmt->accept(*this);
    }
    indentLevel_--;
  }

  void visitExprStmt(ExprStmt &stmt) override {
    indent();
    output_ << "ExprStmt\n";
    indentLevel_++;
    stmt.expr->accept(*this);
    indentLevel_--;
  }

  void visitIfStmt(IfStmt &stmt) override {
    indent();
    output_ << "IfStmt\n";
    indentLevel_++;
    indent();
    output_ << "Condition:\n";
    indentLevel_++;
    stmt.condition->accept(*this);
    indentLevel_--;
    indent();
    output_ << "ThenBranch:\n";
    indentLevel_++;
    for (const auto &thenStmt : stmt.thenBranch) {
      thenStmt->accept(*this);
    }
    indentLevel_--;
    if (!stmt.elseBranch.empty()) {
      indent();
      output_ << "ElseBranch:\n";
      indentLevel_++;
      for (const auto &elseStmt : stmt.elseBranch) {
        elseStmt->accept(*this);
      }
      indentLevel_--;
    }
    indentLevel_--;
  }

  void visitWhileStmt(WhileStmt &stmt) override {
    indent();
    output_ << "WhileStmt\n";
    indentLevel_++;
    indent();
    output_ << "Condition:\n";
    indentLevel_++;
    stmt.condition->accept(*this);
    indentLevel_--;
    for (const auto &bodyStmt : stmt.body) {
      bodyStmt->accept(*this);
    }
    indentLevel_--;
  }

  void visitForStmt(ForStmt &stmt) override {
    indent();
    output_ << "ForStmt(" << stmt.var << ")\n";
    indentLevel_++;
    indent();
    output_ << "Range:\n";
    indentLevel_++;
    stmt.rangeStart->accept(*this);
    stmt.rangeEnd->accept(*this);
    indentLevel_--;
    for (const auto &bodyStmt : stmt.body) {
      bodyStmt->accept(*this);
    }
    indentLevel_--;
  }

  void visitReturnStmt(ReturnStmt &stmt) override {
    indent();
    output_ << "ReturnStmt\n";
    indentLevel_++;
    if (stmt.value) {
      stmt.value->accept(*this);
    }
    indentLevel_--;
  }

  void visitBlockStmt(BlockStmt &stmt) override {
    indent();
    output_ << "BlockStmt[" << stmt.body.size() << "]\n";
    indentLevel_++;
    for (const auto &s : stmt.body) {
      s->accept(*this);
    }
    indentLevel_--;
  }

  void visitBreakStmt(BreakStmt &stmt) override {
    indent();
    output_ << "BreakStmt\n";
  }

  void visitContinueStmt(ContinueStmt &stmt) override {
    indent();
    output_ << "ContinueStmt\n";
  }
};

#endif
