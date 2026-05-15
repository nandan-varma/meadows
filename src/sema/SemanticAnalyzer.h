#ifndef SEMANTIC_ANALYZER_H
#define SEMANTIC_ANALYZER_H

#include "../ast/AST.h"
#include "../utils/DiagnosticsCollector.h"
#include "../utils/Exceptions.h"
#include "../utils/WarningManager.h"
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace meadows {

/**
 * @brief Semantic analysis pass: runs after parsing, before code generation.
 *
 * Detects:
 *  - Undefined variables and functions
 *  - break/continue outside a loop
 *  - return outside a function
 *  - Variable shadowing (warning)
 *  - Unused variables (warning)
 *  - Unreachable statements after return/break/continue (warning)
 */
class SemanticAnalyzer : public ExprVisitor, public StmtVisitor {
public:
  SemanticAnalyzer(DiagnosticsCollector &diagnostics, WarningManager &warnings);

  /** Returns true if analysis completed with no errors. */
  bool analyze(const std::vector<std::unique_ptr<Stmt>> &stmts);

private:
  DiagnosticsCollector &diag_;
  WarningManager &warnings_;

  // ── Scope stack ─────────────────────────────────────────────────────────────
  struct VarInfo {
    SourceLocation loc;
    bool used = false;
    bool isParam = false;
  };

  std::vector<std::unordered_map<std::string, VarInfo>> scopes_;
  std::unordered_map<std::string, size_t> functions_; // name → param count

  // ── Context flags ────────────────────────────────────────────────────────────
  bool inFunction_ = false;
  bool inLoop_ = false;
  bool unreachable_ = false; // set after return/break/continue

  // ── Scope helpers ────────────────────────────────────────────────────────────
  void enterScope();
  void exitScope();

  void declareVar(const std::string &name, const SourceLocation &loc,
                  bool isParam = false);
  VarInfo *lookupVar(const std::string &name);
  void markVarUsed(const std::string &name);

  void reportError(ErrorCode code, const std::string &msg,
                   const SourceLocation &loc);
  void reportWarning(ErrorCode code, const std::string &msg,
                     const SourceLocation &loc);

  // ── ExprVisitor ──────────────────────────────────────────────────────────────
  void visitLiteralExpr(LiteralExpr &expr) override;
  void visitVarExpr(VarExpr &expr) override;
  void visitAssignExpr(AssignExpr &expr) override;
  void visitBinaryExpr(BinaryExpr &expr) override;
  void visitUnaryExpr(UnaryExpr &expr) override;
  void visitLogicalExpr(LogicalExpr &expr) override;
  void visitIndexExpr(IndexExpr &expr) override;
  void visitFieldAccessExpr(FieldAccessExpr &expr) override;
  void visitCallExpr(CallExpr &expr) override;
  void visitArrayExpr(ArrayExpr &expr) override;
  void visitObjectExpr(ObjectExpr &expr) override;

  // ── StmtVisitor ──────────────────────────────────────────────────────────────
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

  void analyzeBlock(const std::vector<std::unique_ptr<Stmt>> &stmts);
};

} // namespace meadows

#endif
