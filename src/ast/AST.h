#ifndef AST_H
#define AST_H

/**
 * @file AST.h
 * @brief Abstract Syntax Tree node definitions for the Meadows language.
 *
 * This file defines the AST node classes used to represent the syntactic
 * structure of Meadows programs. The AST is built by the Parser and used
 * by the CodeGen to generate LLVM IR.
 *
 * @ Architecture
 * - Expression nodes (Expr) represent values and computations
 * - Statement nodes (Stmt) represent program statements
 * - Visitor pattern is used for traversing and processing the AST
 */

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

class Expr;
class Stmt;
class ExprVisitor;
class StmtVisitor;

/**
 * @brief Base class for all expression nodes.
 *
 * Expressions produce values and can be evaluated, combined, or used
 * as sub-expressions in larger expressions.
 */
class Expr {
public:
  virtual ~Expr() = default;
  virtual void accept(ExprVisitor &visitor) = 0;
};

/**
 * @brief Base class for all statement nodes.
 *
 * Statements perform actions but do not produce values directly.
 * They include variable declarations, control flow, function definitions, etc.
 */
class Stmt {
public:
  virtual ~Stmt() = default;
  virtual void accept(StmtVisitor &visitor) = 0;
};

/**
 * @brief Represents a literal value (number or string).
 */
class LiteralExpr : public Expr {
public:
  std::string value;
  LiteralExpr(const std::string &v) : value(v) {}
  void accept(ExprVisitor &visitor) override;
};

/**
 * @brief Represents a variable reference.
 */
class VarExpr : public Expr {
public:
  std::string name;
  VarExpr(const std::string &n) : name(n) {}
  void accept(ExprVisitor &visitor) override;
};

class AssignExpr : public Expr {
public:
  std::string name;
  std::unique_ptr<Expr> value;
  AssignExpr(const std::string &n, std::unique_ptr<Expr> v)
      : name(n), value(std::move(v)) {}
  void accept(ExprVisitor &visitor) override;
};

enum class LogicalOperator { AND, OR };

class LogicalExpr : public Expr {
public:
  std::unique_ptr<Expr> left;
  LogicalOperator op;
  std::unique_ptr<Expr> right;
  LogicalExpr(std::unique_ptr<Expr> l, LogicalOperator o,
              std::unique_ptr<Expr> r)
      : left(std::move(l)), op(o), right(std::move(r)) {}
  void accept(ExprVisitor &visitor) override;
};

class IndexExpr : public Expr {
public:
  std::unique_ptr<Expr> array;
  std::unique_ptr<Expr> index;
  IndexExpr(std::unique_ptr<Expr> a, std::unique_ptr<Expr> i)
      : array(std::move(a)), index(std::move(i)) {}
  void accept(ExprVisitor &visitor) override;
};

class FieldAccessExpr : public Expr {
public:
  std::unique_ptr<Expr> object;
  std::string fieldName;
  FieldAccessExpr(std::unique_ptr<Expr> obj, std::string name)
      : object(std::move(obj)), fieldName(std::move(name)) {}
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

class UnaryExpr : public Expr {
public:
  std::string op;
  std::unique_ptr<Expr> operand;
  UnaryExpr(const std::string &o, std::unique_ptr<Expr> op)
      : op(o), operand(std::move(op)) {}
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
  std::string typeAnnotation; // Empty if no annotation
  LetStmt(const std::string &n, std::unique_ptr<Expr> i,
          const std::string &type = "")
      : name(n), initializer(std::move(i)), typeAnnotation(type) {}
  void accept(StmtVisitor &visitor) override;
};

class FuncParam {
public:
  std::string name;
  std::string typeAnnotation;
  FuncParam(const std::string &n, const std::string &type = "")
      : name(n), typeAnnotation(type) {}

  bool operator==(const std::string &other) const { return name == other; }
  bool operator==(const FuncParam &other) const {
    return name == other.name && typeAnnotation == other.typeAnnotation;
  }
};

class FuncStmt : public Stmt {
public:
  std::string name;
  std::vector<FuncParam> params;
  std::string returnTypeAnnotation;
  std::vector<std::unique_ptr<Stmt>> body;
  FuncStmt(const std::string &n, std::vector<FuncParam> p,
           std::vector<std::unique_ptr<Stmt>> b,
           const std::string &retType = "")
      : name(n), params(std::move(p)), returnTypeAnnotation(retType),
        body(std::move(b)) {}
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

class BreakStmt : public Stmt {
public:
  void accept(StmtVisitor &visitor) override;
};

class ContinueStmt : public Stmt {
public:
  void accept(StmtVisitor &visitor) override;
};

class BlockStmt : public Stmt {
public:
  std::vector<std::unique_ptr<Stmt>> body;
  BlockStmt(std::vector<std::unique_ptr<Stmt>> b) : body(std::move(b)) {}
  void accept(StmtVisitor &visitor) override;
};

class PrintStmt : public Stmt {
public:
  std::unique_ptr<Expr> expr;
  PrintStmt(std::unique_ptr<Expr> e) : expr(std::move(e)) {}
  void accept(StmtVisitor &visitor) override;
};

class TypeDefStmt : public Stmt {
public:
  std::string name;
  std::vector<std::string> typeParams; // For generic types
  // For struct types
  std::unordered_map<std::string, std::string>
      fields; // field name -> type annotation
  // For enum types
  std::vector<std::pair<std::string, std::vector<std::string>>> variants;
  bool isEnum;

  TypeDefStmt(const std::string &n, bool enum_) : name(n), isEnum(enum_) {}
  void accept(StmtVisitor &visitor) override;
};

class ModuleStmt : public Stmt {
public:
  std::string moduleName;

  explicit ModuleStmt(const std::string &name) : moduleName(name) {}
  void accept(StmtVisitor &visitor) override;
};

class ImportStmt : public Stmt {
public:
  std::string modulePath;
  std::vector<std::string> specificImports;
  std::string alias;

  ImportStmt(const std::string &path, std::vector<std::string> imports,
             const std::string &alias = "")
      : modulePath(path), specificImports(std::move(imports)), alias(alias) {}
  void accept(StmtVisitor &visitor) override;
};

class ExportStmt : public Stmt {
public:
  std::string name;
  std::string typeInfo;

  explicit ExportStmt(const std::string &n, const std::string &type = "")
      : name(n), typeInfo(type) {}
  void accept(StmtVisitor &visitor) override;
};

class ExprVisitor {
public:
  virtual ~ExprVisitor() = default;
  virtual void visitLiteralExpr(LiteralExpr &expr) = 0;
  virtual void visitVarExpr(VarExpr &expr) = 0;
  virtual void visitAssignExpr(AssignExpr &expr) = 0;
  virtual void visitBinaryExpr(BinaryExpr &expr) = 0;
  virtual void visitUnaryExpr(UnaryExpr &expr) = 0;
  virtual void visitLogicalExpr(LogicalExpr &expr) = 0;
  virtual void visitIndexExpr(IndexExpr &expr) = 0;
  virtual void visitFieldAccessExpr(FieldAccessExpr &expr) = 0;
  virtual void visitCallExpr(CallExpr &expr) = 0;
  virtual void visitArrayExpr(ArrayExpr &expr) = 0;
  virtual void visitObjectExpr(ObjectExpr &expr) = 0;
};

class StmtVisitor {
public:
  virtual ~StmtVisitor() = default;
  virtual void visitExprStmt(ExprStmt &stmt) = 0;
  virtual void visitLetStmt(LetStmt &stmt) = 0;
  virtual void visitFuncStmt(FuncStmt &stmt) = 0;
  virtual void visitIfStmt(IfStmt &stmt) = 0;
  virtual void visitForStmt(ForStmt &stmt) = 0;
  virtual void visitWhileStmt(WhileStmt &stmt) = 0;
  virtual void visitReturnStmt(ReturnStmt &stmt) = 0;
  virtual void visitBreakStmt(BreakStmt &stmt) = 0;
  virtual void visitContinueStmt(ContinueStmt &stmt) = 0;
  virtual void visitBlockStmt(BlockStmt &stmt) = 0;
  virtual void visitPrintStmt(PrintStmt &stmt) = 0;
  virtual void visitTypeDefStmt(TypeDefStmt &stmt) = 0;
  virtual void visitModuleStmt(ModuleStmt &stmt) = 0;
  virtual void visitImportStmt(ImportStmt &stmt) = 0;
  virtual void visitExportStmt(ExportStmt &stmt) = 0;
};

#endif