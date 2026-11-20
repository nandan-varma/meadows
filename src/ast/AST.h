#ifndef MEADOWS_AST_H
#define MEADOWS_AST_H

#include <algorithm>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "../utils/SourceMap.h"

namespace meadows {

class ExprVisitor;
class StmtVisitor;
class Expr;
class Stmt;

using ExprPtr = std::unique_ptr<Expr>;
using StmtPtr = std::unique_ptr<Stmt>;
using StmtList = std::vector<StmtPtr>;

class Expr {
public:
  Location location;
  uint32_t height = 1;

  virtual ~Expr() = default;
  virtual void accept(ExprVisitor& visitor) = 0;

protected:
  void growTo(const ExprPtr& child) {
    height = std::max(height, child->height + 1);
  }
};

class Stmt {
public:
  Location location;

  virtual ~Stmt() = default;
  virtual void accept(StmtVisitor& visitor) = 0;
};

enum class LiteralKind { Int, Float, String };

class LiteralExpr : public Expr {
public:
  LiteralKind kind;
  int32_t intValue = 0;
  double floatValue = 0;
  std::string stringValue;

  static std::unique_ptr<LiteralExpr> integer(int32_t value);
  static std::unique_ptr<LiteralExpr> real(double value);
  static std::unique_ptr<LiteralExpr> string(std::string value);

  void accept(ExprVisitor& visitor) override;

private:
  explicit LiteralExpr(LiteralKind k) : kind(k) {}
};

class VarExpr : public Expr {
public:
  std::string name;

  explicit VarExpr(std::string n) : name(std::move(n)) {}
  void accept(ExprVisitor& visitor) override;
};

class AssignExpr : public Expr {
public:
  ExprPtr target;
  ExprPtr value;

  AssignExpr(ExprPtr t, ExprPtr v) : target(std::move(t)), value(std::move(v)) {
    growTo(target);
    growTo(value);
  }
  void accept(ExprVisitor& visitor) override;
};

enum class BinaryOp { Add, Sub, Mul, Div, Mod, Eq, Ne, Lt, Le, Gt, Ge };

class BinaryExpr : public Expr {
public:
  BinaryOp op;
  ExprPtr left;
  ExprPtr right;

  BinaryExpr(BinaryOp o, ExprPtr l, ExprPtr r)
      : op(o), left(std::move(l)), right(std::move(r)) {
    growTo(left);
    growTo(right);
  }
  void accept(ExprVisitor& visitor) override;
};

enum class UnaryOp { Negate, Not };

class UnaryExpr : public Expr {
public:
  UnaryOp op;
  ExprPtr operand;

  UnaryExpr(UnaryOp o, ExprPtr e) : op(o), operand(std::move(e)) {
    growTo(operand);
  }
  void accept(ExprVisitor& visitor) override;
};

enum class LogicalOp { And, Or };

class LogicalExpr : public Expr {
public:
  LogicalOp op;
  ExprPtr left;
  ExprPtr right;

  LogicalExpr(LogicalOp o, ExprPtr l, ExprPtr r)
      : op(o), left(std::move(l)), right(std::move(r)) {
    growTo(left);
    growTo(right);
  }
  void accept(ExprVisitor& visitor) override;
};

class IndexExpr : public Expr {
public:
  ExprPtr object;
  ExprPtr index;

  IndexExpr(ExprPtr o, ExprPtr i) : object(std::move(o)), index(std::move(i)) {
    growTo(object);
    growTo(index);
  }
  void accept(ExprVisitor& visitor) override;
};

class FieldExpr : public Expr {
public:
  ExprPtr object;
  std::string name;

  FieldExpr(ExprPtr o, std::string n)
      : object(std::move(o)), name(std::move(n)) {
    growTo(object);
  }
  void accept(ExprVisitor& visitor) override;
};

class CallExpr : public Expr {
public:
  ExprPtr callee;
  std::vector<ExprPtr> args;

  CallExpr(ExprPtr c, std::vector<ExprPtr> a)
      : callee(std::move(c)), args(std::move(a)) {
    growTo(callee);
    for (const ExprPtr& arg : args) {
      growTo(arg);
    }
  }
  void accept(ExprVisitor& visitor) override;
};

class ArrayExpr : public Expr {
public:
  std::vector<ExprPtr> elements;

  explicit ArrayExpr(std::vector<ExprPtr> e) : elements(std::move(e)) {
    for (const ExprPtr& element : elements) {
      growTo(element);
    }
  }
  void accept(ExprVisitor& visitor) override;
};

struct ObjectField {
  std::string name;
  ExprPtr value;
  Location location;
};

class ObjectExpr : public Expr {
public:
  std::vector<ObjectField> fields;

  explicit ObjectExpr(std::vector<ObjectField> f) : fields(std::move(f)) {
    for (const ObjectField& field : fields) {
      growTo(field.value);
    }
  }
  void accept(ExprVisitor& visitor) override;
};

class ExprVisitor {
public:
  virtual ~ExprVisitor() = default;
  virtual void visitLiteralExpr(LiteralExpr& expr) = 0;
  virtual void visitVarExpr(VarExpr& expr) = 0;
  virtual void visitAssignExpr(AssignExpr& expr) = 0;
  virtual void visitBinaryExpr(BinaryExpr& expr) = 0;
  virtual void visitUnaryExpr(UnaryExpr& expr) = 0;
  virtual void visitLogicalExpr(LogicalExpr& expr) = 0;
  virtual void visitIndexExpr(IndexExpr& expr) = 0;
  virtual void visitFieldExpr(FieldExpr& expr) = 0;
  virtual void visitCallExpr(CallExpr& expr) = 0;
  virtual void visitArrayExpr(ArrayExpr& expr) = 0;
  virtual void visitObjectExpr(ObjectExpr& expr) = 0;
};

class ExprStmt : public Stmt {
public:
  ExprPtr expr;

  explicit ExprStmt(ExprPtr e) : expr(std::move(e)) {}
  void accept(StmtVisitor& visitor) override;
};

class LetStmt : public Stmt {
public:
  std::string name;
  Location nameLocation;
  ExprPtr initializer;

  LetStmt(std::string n, Location nl, ExprPtr i)
      : name(std::move(n)), nameLocation(nl), initializer(std::move(i)) {}
  void accept(StmtVisitor& visitor) override;
};

struct Param {
  std::string name;
  Location location;
};

class FuncStmt : public Stmt {
public:
  std::string name;
  Location nameLocation;
  std::vector<Param> params;
  StmtList body;

  FuncStmt(std::string n, Location nl, std::vector<Param> p, StmtList b)
      : name(std::move(n)), nameLocation(nl), params(std::move(p)),
        body(std::move(b)) {}
  void accept(StmtVisitor& visitor) override;
};

class IfStmt : public Stmt {
public:
  ExprPtr condition;
  StmtList thenBody;
  StmtList elseBody;

  IfStmt(ExprPtr c, StmtList t, StmtList e)
      : condition(std::move(c)), thenBody(std::move(t)),
        elseBody(std::move(e)) {}
  void accept(StmtVisitor& visitor) override;
};

class ForStmt : public Stmt {
public:
  std::string var;
  Location varLocation;
  ExprPtr start;
  ExprPtr end;
  StmtList body;

  ForStmt(std::string v, Location vl, ExprPtr s, ExprPtr e, StmtList b)
      : var(std::move(v)), varLocation(vl), start(std::move(s)),
        end(std::move(e)), body(std::move(b)) {}
  void accept(StmtVisitor& visitor) override;
};

class WhileStmt : public Stmt {
public:
  ExprPtr condition;
  StmtList body;

  WhileStmt(ExprPtr c, StmtList b)
      : condition(std::move(c)), body(std::move(b)) {}
  void accept(StmtVisitor& visitor) override;
};

class ReturnStmt : public Stmt {
public:
  ExprPtr value;

  explicit ReturnStmt(ExprPtr v) : value(std::move(v)) {}
  void accept(StmtVisitor& visitor) override;
};

class BreakStmt : public Stmt {
public:
  void accept(StmtVisitor& visitor) override;
};

class ContinueStmt : public Stmt {
public:
  void accept(StmtVisitor& visitor) override;
};

class BlockStmt : public Stmt {
public:
  StmtList body;

  explicit BlockStmt(StmtList b) : body(std::move(b)) {}
  void accept(StmtVisitor& visitor) override;
};

class StmtVisitor {
public:
  virtual ~StmtVisitor() = default;
  virtual void visitExprStmt(ExprStmt& stmt) = 0;
  virtual void visitLetStmt(LetStmt& stmt) = 0;
  virtual void visitFuncStmt(FuncStmt& stmt) = 0;
  virtual void visitIfStmt(IfStmt& stmt) = 0;
  virtual void visitForStmt(ForStmt& stmt) = 0;
  virtual void visitWhileStmt(WhileStmt& stmt) = 0;
  virtual void visitReturnStmt(ReturnStmt& stmt) = 0;
  virtual void visitBreakStmt(BreakStmt& stmt) = 0;
  virtual void visitContinueStmt(ContinueStmt& stmt) = 0;
  virtual void visitBlockStmt(BlockStmt& stmt) = 0;
};

const char* binaryOpSymbol(BinaryOp op);
const char* logicalOpSymbol(LogicalOp op);
const char* unaryOpSymbol(UnaryOp op);

} // namespace meadows

#endif
