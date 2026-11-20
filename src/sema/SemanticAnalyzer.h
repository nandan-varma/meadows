#ifndef MEADOWS_SEMANTIC_ANALYZER_H
#define MEADOWS_SEMANTIC_ANALYZER_H

#include <map>
#include <set>
#include <string>
#include <vector>

#include "../ast/AST.h"
#include "../utils/Diagnostics.h"

namespace meadows {

class SemanticAnalyzer : public ExprVisitor, public StmtVisitor {
public:
  explicit SemanticAnalyzer(DiagnosticSink& diagnostics)
      : diagnostics_(diagnostics) {}

  void analyze(StmtList& statements);

private:
  struct Variable {
    std::string name;
    Location location;
    bool used = false;
    bool isParam = false;
  };
  using Scope = std::vector<Variable>;

  DiagnosticSink& diagnostics_;
  std::vector<Scope> scopes_;
  std::map<std::string, size_t> functions_;
  std::set<std::string> topLevelVariables_;
  bool inFunction_ = false;
  bool inLoop_ = false;
  bool unreachable_ = false;

  void enterScope() { scopes_.emplace_back(); }
  void exitScope();
  void declare(const std::string& name, Location location, bool isParam);
  Variable* lookup(const std::string& name);
  bool shadowsOuterScope(const std::string& name) const;
  bool conflictsWithCallable(const std::string& name, Location location);
  void analyzeBody(StmtList& body);
  void reportUndefined(ErrorCode code, const std::string& what,
                       const std::string& name, const Expr& expr);

  void visitLiteralExpr(LiteralExpr& expr) override;
  void visitVarExpr(VarExpr& expr) override;
  void visitAssignExpr(AssignExpr& expr) override;
  void visitBinaryExpr(BinaryExpr& expr) override;
  void visitUnaryExpr(UnaryExpr& expr) override;
  void visitLogicalExpr(LogicalExpr& expr) override;
  void visitIndexExpr(IndexExpr& expr) override;
  void visitFieldExpr(FieldExpr& expr) override;
  void visitCallExpr(CallExpr& expr) override;
  void visitArrayExpr(ArrayExpr& expr) override;
  void visitObjectExpr(ObjectExpr& expr) override;

  void visitExprStmt(ExprStmt& stmt) override;
  void visitLetStmt(LetStmt& stmt) override;
  void visitFuncStmt(FuncStmt& stmt) override;
  void visitIfStmt(IfStmt& stmt) override;
  void visitForStmt(ForStmt& stmt) override;
  void visitWhileStmt(WhileStmt& stmt) override;
  void visitReturnStmt(ReturnStmt& stmt) override;
  void visitBreakStmt(BreakStmt& stmt) override;
  void visitContinueStmt(ContinueStmt& stmt) override;
  void visitBlockStmt(BlockStmt& stmt) override;
};

} // namespace meadows

#endif
