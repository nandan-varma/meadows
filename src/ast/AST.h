#ifndef AST_H
#define AST_H

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

class Expr;
class Stmt;
class ExprVisitor;
class StmtVisitor;

class Expr {
public:
  virtual ~Expr() = default;
  virtual void accept(ExprVisitor &visitor) = 0;
};

class Stmt {
public:
  virtual ~Stmt() = default;
  virtual void accept(StmtVisitor &visitor) = 0;
};

class LiteralExpr : public Expr {
public:
  std::string value;
  LiteralExpr(const std::string &v) : value(v) {}
  void accept(ExprVisitor &visitor) override;
};

class VarExpr : public Expr {
public:
  std::string name;
  VarExpr(const std::string &n) : name(n) {}
  void accept(ExprVisitor &visitor) override;
};

class BinaryExpr : public Expr {
public:
  std::unique_ptr<Expr> left;
  std::string op;
  std::unique_ptr<Expr> right;
  BinaryExpr(std::unique_ptr<Expr> l, const std::string &o,
             std::unique_ptr<Expr> r)
      : left(std::move(l)), op(o), right(std::move(r)) {}
  void accept(ExprVisitor &visitor) override;
};

class CallExpr : public Expr {
public:
  std::unique_ptr<Expr> callee;
  std::vector<std::unique_ptr<Expr>> args;
  CallExpr(std::unique_ptr<Expr> c, std::vector<std::unique_ptr<Expr>> a)
      : callee(std::move(c)), args(std::move(a)) {}
  void accept(ExprVisitor &visitor) override;
};

class ArrayExpr : public Expr {
public:
  std::vector<std::unique_ptr<Expr>> elements;
  ArrayExpr(std::vector<std::unique_ptr<Expr>> e) : elements(std::move(e)) {}
  void accept(ExprVisitor &visitor) override;
};

class ObjectExpr : public Expr {
public:
  std::unordered_map<std::string, std::unique_ptr<Expr>> pairs;
  ObjectExpr(std::unordered_map<std::string, std::unique_ptr<Expr>> p)
      : pairs(std::move(p)) {}
  void accept(ExprVisitor &visitor) override;
};

class ExprStmt : public Stmt {
public:
  std::unique_ptr<Expr> expr;
  ExprStmt(std::unique_ptr<Expr> e) : expr(std::move(e)) {}
  void accept(StmtVisitor &visitor) override;
};

class LetStmt : public Stmt {
public:
  std::string name;
  std::unique_ptr<Expr> initializer;
  LetStmt(const std::string &n, std::unique_ptr<Expr> i)
      : name(n), initializer(std::move(i)) {}
  void accept(StmtVisitor &visitor) override;
};

class FuncStmt : public Stmt {
public:
  std::string name;
  std::vector<std::string> params;
  std::vector<std::unique_ptr<Stmt>> body;
  FuncStmt(const std::string &n, std::vector<std::string> p,
           std::vector<std::unique_ptr<Stmt>> b)
      : name(n), params(std::move(p)), body(std::move(b)) {}
  void accept(StmtVisitor &visitor) override;
};

class IfStmt : public Stmt {
public:
  std::unique_ptr<Expr> condition;
  std::vector<std::unique_ptr<Stmt>> thenBranch;
  std::vector<std::unique_ptr<Stmt>> elseBranch;
  IfStmt(std::unique_ptr<Expr> c, std::vector<std::unique_ptr<Stmt>> t,
         std::vector<std::unique_ptr<Stmt>> e)
      : condition(std::move(c)), thenBranch(std::move(t)),
        elseBranch(std::move(e)) {}
  void accept(StmtVisitor &visitor) override;
};

class ForStmt : public Stmt {
public:
  std::string var;
  std::unique_ptr<Expr> rangeStart;
  std::unique_ptr<Expr> rangeEnd;
  std::vector<std::unique_ptr<Stmt>> body;
  ForStmt(const std::string &v, std::unique_ptr<Expr> s,
          std::unique_ptr<Expr> e, std::vector<std::unique_ptr<Stmt>> b)
      : var(v), rangeStart(std::move(s)), rangeEnd(std::move(e)),
        body(std::move(b)) {}
  void accept(StmtVisitor &visitor) override;
};

class WhileStmt : public Stmt {
public:
  std::unique_ptr<Expr> condition;
  std::vector<std::unique_ptr<Stmt>> body;
  WhileStmt(std::unique_ptr<Expr> c, std::vector<std::unique_ptr<Stmt>> b)
      : condition(std::move(c)), body(std::move(b)) {}
  void accept(StmtVisitor &visitor) override;
};

class ReturnStmt : public Stmt {
public:
  std::unique_ptr<Expr> value;
  ReturnStmt(std::unique_ptr<Expr> v) : value(std::move(v)) {}
  void accept(StmtVisitor &visitor) override;
};

class PrintStmt : public Stmt {
public:
  std::unique_ptr<Expr> expr;
  PrintStmt(std::unique_ptr<Expr> e) : expr(std::move(e)) {}
  void accept(StmtVisitor &visitor) override;
};

class ExprVisitor {
public:
  virtual void visitLiteralExpr(LiteralExpr &expr) = 0;
  virtual void visitVarExpr(VarExpr &expr) = 0;
  virtual void visitBinaryExpr(BinaryExpr &expr) = 0;
  virtual void visitCallExpr(CallExpr &expr) = 0;
  virtual void visitArrayExpr(ArrayExpr &expr) = 0;
  virtual void visitObjectExpr(ObjectExpr &expr) = 0;
};

class StmtVisitor {
public:
  virtual void visitExprStmt(ExprStmt &stmt) = 0;
  virtual void visitLetStmt(LetStmt &stmt) = 0;
  virtual void visitFuncStmt(FuncStmt &stmt) = 0;
  virtual void visitIfStmt(IfStmt &stmt) = 0;
  virtual void visitForStmt(ForStmt &stmt) = 0;
  virtual void visitWhileStmt(WhileStmt &stmt) = 0;
  virtual void visitReturnStmt(ReturnStmt &stmt) = 0;
  virtual void visitPrintStmt(PrintStmt &stmt) = 0;
};

#endif