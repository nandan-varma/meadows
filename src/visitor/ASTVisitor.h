#pragma once

namespace meadows {

// Forward declarations
class IntegerLiteral;
class FloatLiteral;
class StringLiteral;
class BooleanLiteral;
class NoneLiteral;
class ListLiteral;
class Identifier;
class BinaryExpression;
class UnaryExpression;
class FunctionCall;
class AttributeAccess;
class IndexAccess;
class Assignment;
class ExpressionStatement;
class Block;
class IfStatement;
class WhileStatement;
class ForStatement;
class ReturnStatement;
class BreakStatement;
class ContinueStatement;
class PassStatement;
class FunctionDefinition;
class ClassDefinition;
class ImportStatement;
class Program;

class ASTVisitor {
public:
  virtual ~ASTVisitor() = default;

  // Expression visitors
  virtual void visit(IntegerLiteral &node) = 0;
  virtual void visit(FloatLiteral &node) = 0;
  virtual void visit(StringLiteral &node) = 0;
  virtual void visit(BooleanLiteral &node) = 0;
  virtual void visit(NoneLiteral &node) = 0;
  virtual void visit(ListLiteral &node) = 0;
  virtual void visit(Identifier &node) = 0;
  virtual void visit(BinaryExpression &node) = 0;
  virtual void visit(UnaryExpression &node) = 0;
  virtual void visit(FunctionCall &node) = 0;
  virtual void visit(AttributeAccess &node) = 0;
  virtual void visit(IndexAccess &node) = 0;
  virtual void visit(Assignment &node) = 0;

  // Statement visitors
  virtual void visit(ExpressionStatement &node) = 0;
  virtual void visit(Block &node) = 0;
  virtual void visit(IfStatement &node) = 0;
  virtual void visit(WhileStatement &node) = 0;
  virtual void visit(ForStatement &node) = 0;
  virtual void visit(ReturnStatement &node) = 0;
  virtual void visit(BreakStatement &node) = 0;
  virtual void visit(ContinueStatement &node) = 0;
  virtual void visit(PassStatement &node) = 0;
  virtual void visit(FunctionDefinition &node) = 0;
  virtual void visit(ClassDefinition &node) = 0;
  virtual void visit(ImportStatement &node) = 0;
  virtual void visit(Program &node) = 0;
};

} // namespace meadows
