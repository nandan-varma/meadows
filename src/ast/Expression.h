#pragma once

#include "../lexer/Token.h"
#include <memory>
#include <string>
#include <vector>

namespace meadows {

// Forward declarations
class ASTVisitor;

// Base AST Node
class ASTNode {
public:
  SourceLocation location;

  ASTNode(const SourceLocation &location) : location(location) {}
  virtual ~ASTNode() = default;
  virtual void accept(ASTVisitor &visitor) = 0;
};

// Expression base class
class Expression : public ASTNode {
public:
  Expression(const SourceLocation &location) : ASTNode(location) {}
  virtual ~Expression() = default;
};

// Statement base class
class Statement : public ASTNode {
public:
  Statement(const SourceLocation &location) : ASTNode(location) {}
  virtual ~Statement() = default;
};

// Literal expressions
class IntegerLiteral : public Expression {
public:
  long long value;

  IntegerLiteral(long long value, const SourceLocation &location)
      : Expression(location), value(value) {}

  void accept(ASTVisitor &visitor) override;
};

class FloatLiteral : public Expression {
public:
  double value;

  FloatLiteral(double value, const SourceLocation &location)
      : Expression(location), value(value) {}

  void accept(ASTVisitor &visitor) override;
};

class StringLiteral : public Expression {
public:
  std::string value;

  StringLiteral(const std::string &value, const SourceLocation &location)
      : Expression(location), value(value) {}

  void accept(ASTVisitor &visitor) override;
};

class BooleanLiteral : public Expression {
public:
  bool value;

  BooleanLiteral(bool value, const SourceLocation &location)
      : Expression(location), value(value) {}

  void accept(ASTVisitor &visitor) override;
};

class NoneLiteral : public Expression {
public:
  NoneLiteral(const SourceLocation &location) : Expression(location) {}

  void accept(ASTVisitor &visitor) override;
};

class ListLiteral : public Expression {
public:
  std::vector<std::unique_ptr<Expression>> elements;

  ListLiteral(std::vector<std::unique_ptr<Expression>> elements,
              const SourceLocation &location)
      : Expression(location), elements(std::move(elements)) {}

  void accept(ASTVisitor &visitor) override;
};

// Identifier
class Identifier : public Expression {
public:
  std::string name;

  Identifier(const std::string &name, const SourceLocation &location)
      : Expression(location), name(name) {}

  void accept(ASTVisitor &visitor) override;
};

// Binary operations
enum class BinaryOp {
  ADD,
  SUBTRACT,
  MULTIPLY,
  DIVIDE,
  MODULO,
  POWER,
  EQUAL,
  NOT_EQUAL,
  LESS_THAN,
  LESS_EQUAL,
  GREATER_THAN,
  GREATER_EQUAL,
  AND,
  OR
};

class BinaryExpression : public Expression {
public:
  std::unique_ptr<Expression> left;
  BinaryOp operator_;
  std::unique_ptr<Expression> right;

  BinaryExpression(std::unique_ptr<Expression> left, BinaryOp op,
                   std::unique_ptr<Expression> right,
                   const SourceLocation &location)
      : Expression(location), left(std::move(left)), operator_(op),
        right(std::move(right)) {}

  void accept(ASTVisitor &visitor) override;
};

// Unary operations
enum class UnaryOp { MINUS, NOT };

class UnaryExpression : public Expression {
public:
  UnaryOp operator_;
  std::unique_ptr<Expression> operand;

  UnaryExpression(UnaryOp op, std::unique_ptr<Expression> operand,
                  const SourceLocation &location)
      : Expression(location), operator_(op), operand(std::move(operand)) {}

  void accept(ASTVisitor &visitor) override;
};

// Function call
class FunctionCall : public Expression {
public:
  std::unique_ptr<Expression> function;
  std::vector<std::unique_ptr<Expression>> arguments;

  FunctionCall(std::unique_ptr<Expression> function,
               std::vector<std::unique_ptr<Expression>> arguments,
               const SourceLocation &location)
      : Expression(location), function(std::move(function)),
        arguments(std::move(arguments)) {}

  void accept(ASTVisitor &visitor) override;
};

// Attribute access (obj.attr)
class AttributeAccess : public Expression {
public:
  std::unique_ptr<Expression> object;
  std::string attribute;

  AttributeAccess(std::unique_ptr<Expression> object,
                  const std::string &attribute, const SourceLocation &location)
      : Expression(location), object(std::move(object)), attribute(attribute) {}

  void accept(ASTVisitor &visitor) override;
};

// Index access (obj[index])
class IndexAccess : public Expression {
public:
  std::unique_ptr<Expression> object;
  std::unique_ptr<Expression> index;

  IndexAccess(std::unique_ptr<Expression> object,
              std::unique_ptr<Expression> index, const SourceLocation &location)
      : Expression(location), object(std::move(object)),
        index(std::move(index)) {}

  void accept(ASTVisitor &visitor) override;
};

// Assignment
class Assignment : public Expression {
public:
  std::unique_ptr<Expression> target;
  std::unique_ptr<Expression> value;

  Assignment(std::unique_ptr<Expression> target,
             std::unique_ptr<Expression> value, const SourceLocation &location)
      : Expression(location), target(std::move(target)),
        value(std::move(value)) {}

  void accept(ASTVisitor &visitor) override;
};

} // namespace meadows
