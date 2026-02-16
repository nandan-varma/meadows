/**
 * @file TypeChecker.h
 * @brief Hindley-Milner type inference and checking.
 */

#ifndef TYPE_CHECKER_H
#define TYPE_CHECKER_H

#include "../ast/AST.h"
#include "../utils/DiagnosticsCollector.h"
#include "Types.h"
#include <iostream>
#include <unordered_map>
#include <vector>

namespace meadows {
namespace types {

struct Constraint {
  std::shared_ptr<Type> t1;
  std::shared_ptr<Type> t2;
  std::string context;
  int line;
  int column;
};

class TypeChecker : public ExprVisitor, public StmtVisitor {
public:
  TypeChecker();

  bool check(const std::vector<std::unique_ptr<Stmt>> &statements);

  const std::vector<std::string> &getErrors() const { return errors_; }
  bool hasErrors() const { return !errors_.empty(); }

  std::shared_ptr<Type> getInferredType(const Expr *expr) const;

  void reset();

  void visitLiteralExpr(LiteralExpr &expr) override;
  void visitVarExpr(VarExpr &expr) override;
  void visitAssignExpr(AssignExpr &expr) override;
  void visitBinaryExpr(BinaryExpr &expr) override;
  void visitUnaryExpr(UnaryExpr &expr) override;
  void visitTryExpr(TryExpr &expr) override;
  void visitLogicalExpr(LogicalExpr &expr) override;
  void visitIndexExpr(IndexExpr &expr) override;
  void visitFieldAccessExpr(FieldAccessExpr &expr) override;
  void visitCallExpr(CallExpr &expr) override;
  void visitArrayExpr(ArrayExpr &expr) override;
  void visitObjectExpr(ObjectExpr &expr) override;
  void visitMatchExpr(MatchExpr &expr) override;
  void visitEnumVariantExpr(EnumVariantExpr &expr) override;

  void visitExprStmt(ExprStmt &stmt) override;
  void visitLetStmt(LetStmt &stmt) override;
  void visitFuncStmt(FuncStmt &stmt) override;
  void visitIfStmt(IfStmt &stmt) override;
  void visitForStmt(ForStmt &stmt) override;
  void visitWhileStmt(WhileStmt &stmt) override;
  void visitReturnStmt(ReturnStmt &stmt) override;
  void visitBreakStmt(BreakStmt &stmt) override;
  void visitContinueStmt(ContinueStmt &stmt) override;
  void visitBlockStmt(BlockStmt &stmt) override;
  void visitPrintStmt(PrintStmt &stmt) override;
  void visitTypeDefStmt(TypeDefStmt &stmt) override;
  void visitModuleStmt(ModuleStmt &stmt) override;
  void visitImportStmt(ImportStmt &stmt) override;
  void visitExportStmt(ExportStmt &stmt) override;
  void visitExternStmt(ExternStmt &stmt) override;

private:
  struct TypeScheme {
    std::vector<std::string> typeParams;
    std::shared_ptr<Type> type;
  };

  struct ScopedEnv {
    std::unordered_map<std::string, TypeScheme> bindings;
    std::shared_ptr<ScopedEnv> parent;
  };

  std::shared_ptr<ScopedEnv> env_;

  std::vector<Constraint> constraints_;
  Substitution subst_;

  std::unordered_map<const Expr *, std::shared_ptr<Type>> exprTypes_;
  std::unordered_map<const Stmt *, std::shared_ptr<Type>> stmtTypes_;

  std::vector<std::string> errors_;

  int currentLine_;
  int currentColumn_;
  std::vector<std::shared_ptr<Type>> functionReturnTypes_;
  int typeVarCounter_;

  std::shared_ptr<Type> i32_, i64_, f32_, f64_, bool_, string_, unit_;

  std::unordered_map<std::string, std::vector<std::string>> definedEnums_;

  void initBuiltins();
  std::shared_ptr<Type> freshTypeVar();
  std::shared_ptr<Type> inferExpr(Expr *expr);
  void inferStmt(Stmt *stmt);
  void checkBlock(const std::vector<std::unique_ptr<Stmt>> &statements);

  void addConstraint(std::shared_ptr<Type> t1, std::shared_ptr<Type> t2,
                     const std::string &context);

  bool unify();
  bool unifyPair(std::shared_ptr<Type> t1, std::shared_ptr<Type> t2);
  bool occursIn(const std::string &varName, const Type *type);

  void reportError(const std::string &message);
  void reportTypeMismatch(std::shared_ptr<Type> expected,
                          std::shared_ptr<Type> actual,
                          const std::string &context);

  std::shared_ptr<Type> getPrimitiveType(const std::string &name);

  void bind(const std::string &name, std::shared_ptr<Type> type);
  std::shared_ptr<Type> lookup(const std::string &name) const;
  std::shared_ptr<ScopedEnv> extendEnv() const;
};

} // namespace types
} // namespace meadows

#endif // TYPE_CHECKER_H
