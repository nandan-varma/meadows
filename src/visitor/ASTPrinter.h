#pragma once

#include "ASTVisitor.h"
#include <sstream>
#include <string>

namespace meadows {

class ASTPrinter : public ASTVisitor {
private:
  std::ostringstream output;
  int indentLevel;

  void indent();
  void print(const std::string &text);
  void println(const std::string &text);

public:
  ASTPrinter() : indentLevel(0) {}

  std::string getOutput() const { return output.str(); }
  void clear() {
    output.str("");
    output.clear();
    indentLevel = 0;
  }

  // Expression visitors
  void visit(IntegerLiteral &node) override;
  void visit(FloatLiteral &node) override;
  void visit(StringLiteral &node) override;
  void visit(BooleanLiteral &node) override;
  void visit(NoneLiteral &node) override;
  void visit(ListLiteral &node) override;
  void visit(Identifier &node) override;
  void visit(BinaryExpression &node) override;
  void visit(UnaryExpression &node) override;
  void visit(FunctionCall &node) override;
  void visit(AttributeAccess &node) override;
  void visit(IndexAccess &node) override;
  void visit(Assignment &node) override;

  // Statement visitors
  void visit(ExpressionStatement &node) override;
  void visit(Block &node) override;
  void visit(IfStatement &node) override;
  void visit(WhileStatement &node) override;
  void visit(ForStatement &node) override;
  void visit(ReturnStatement &node) override;
  void visit(BreakStatement &node) override;
  void visit(ContinueStatement &node) override;
  void visit(PassStatement &node) override;
  void visit(FunctionDefinition &node) override;
  void visit(ClassDefinition &node) override;
  void visit(ImportStatement &node) override;
  void visit(Program &node) override;
};

} // namespace meadows
