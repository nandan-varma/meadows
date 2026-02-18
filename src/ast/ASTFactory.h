/**
 * @file ASTFactory.h
 * @brief Factory Pattern for creating AST nodes with validation
 *
 * Centralizes AST node construction with:
 * - Source location tracking
 * - Validation during construction
 * - Construction metrics
 * - Future extensibility for node pooling
 */

#ifndef AST_FACTORY_H
#define AST_FACTORY_H

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "../errors/CompilationError.h"
#include "AST.h"

namespace meadows {

/**
 * @brief Metrics for AST construction
 */
struct ASTMetrics {
  size_t totalNodesCreated = 0;
  size_t expressionsCreated = 0;
  size_t statementsCreated = 0;
  size_t validationFailures = 0;

  void reset() {
    totalNodesCreated = 0;
    expressionsCreated = 0;
    statementsCreated = 0;
    validationFailures = 0;
  }
};

/**
 * @brief Factory for creating AST nodes with validation
 *
 * Implements Factory Pattern - centralizes node creation and ensures
 * all nodes are properly constructed with source locations.
 */
class ASTFactory {
public:
  /**
   * @brief Get singleton instance
   */
  static ASTFactory &getInstance();

  /**
   * @brief Enable/disable metrics collection
   */
  void setMetricsEnabled(bool enabled) { metricsEnabled_ = enabled; }

  /**
   * @brief Get current metrics
   */
  ASTMetrics getMetrics() const { return metrics_; }

  /**
   * @brief Reset metrics
   */
  void resetMetrics() { metrics_.reset(); }

  // ========== Expression Factory Methods ==========

  std::unique_ptr<LiteralExpr> createLiteralExpr(const std::string &value,
                                                 const SourceLocation &loc);

  std::unique_ptr<VarExpr> createVarExpr(const std::string &name,
                                         const SourceLocation &loc);

  std::unique_ptr<AssignExpr> createAssignExpr(const std::string &name,
                                               std::unique_ptr<Expr> value,
                                               const SourceLocation &loc);

  std::unique_ptr<BinaryExpr> createBinaryExpr(std::unique_ptr<Expr> left,
                                               const std::string &op,
                                               std::unique_ptr<Expr> right,
                                               const SourceLocation &loc);

  std::unique_ptr<UnaryExpr> createUnaryExpr(const std::string &op,
                                             std::unique_ptr<Expr> operand,
                                             const SourceLocation &loc);

  std::unique_ptr<CallExpr>
  createCallExpr(std::unique_ptr<Expr> callee,
                 std::vector<std::unique_ptr<Expr>> args,
                 const SourceLocation &loc);

  std::unique_ptr<LogicalExpr> createLogicalExpr(std::unique_ptr<Expr> left,
                                                 LogicalOperator op,
                                                 std::unique_ptr<Expr> right,
                                                 const SourceLocation &loc);

  std::unique_ptr<IndexExpr> createIndexExpr(std::unique_ptr<Expr> array,
                                             std::unique_ptr<Expr> index,
                                             const SourceLocation &loc);

  std::unique_ptr<FieldAccessExpr>
  createFieldAccessExpr(std::unique_ptr<Expr> object,
                        const std::string &fieldName,
                        const SourceLocation &loc);

  std::unique_ptr<TryExpr> createTryExpr(std::unique_ptr<Expr> expr,
                                         const SourceLocation &loc);

  std::unique_ptr<ArrayExpr>
  createArrayExpr(std::vector<std::unique_ptr<Expr>> elements,
                  const SourceLocation &loc);

  std::unique_ptr<ObjectExpr>
  createObjectExpr(std::unordered_map<std::string, std::unique_ptr<Expr>> pairs,
                   const SourceLocation &loc);

  std::unique_ptr<MatchExpr> createMatchExpr(std::unique_ptr<Expr> scrutinee,
                                             std::vector<MatchArm> arms,
                                             const SourceLocation &loc);

  std::unique_ptr<EnumVariantExpr> createEnumVariantExpr(
      const std::string &enumName, const std::string &variantName,
      std::vector<std::unique_ptr<Expr>> args, const SourceLocation &loc);

  // ========== Statement Factory Methods ==========

  std::unique_ptr<ExprStmt> createExprStmt(std::unique_ptr<Expr> expr,
                                           const SourceLocation &loc);

  std::unique_ptr<LetStmt> createLetStmt(const std::string &name,
                                         std::unique_ptr<Expr> initializer,
                                         const SourceLocation &loc);

  std::unique_ptr<LetStmt> createLetStmt(const std::string &name,
                                         std::unique_ptr<Expr> initializer,
                                         const std::string &typeAnnotation,
                                         const SourceLocation &loc);

  std::unique_ptr<FuncStmt>
  createFuncStmt(const std::string &name, std::vector<FuncParam> params,
                 std::vector<std::unique_ptr<Stmt>> body,
                 const SourceLocation &loc);

  std::unique_ptr<FuncStmt>
  createFuncStmt(const std::string &name, std::vector<FuncParam> params,
                 std::vector<std::unique_ptr<Stmt>> body,
                 const std::string &returnTypeAnnotation,
                 const SourceLocation &loc);

  std::unique_ptr<IfStmt>
  createIfStmt(std::unique_ptr<Expr> condition,
               std::vector<std::unique_ptr<Stmt>> thenBranch,
               std::vector<std::unique_ptr<Stmt>> elseBranch,
               const SourceLocation &loc);

  std::unique_ptr<ForStmt>
  createForStmt(const std::string &var, std::unique_ptr<Expr> rangeStart,
                std::unique_ptr<Expr> rangeEnd,
                std::vector<std::unique_ptr<Stmt>> body,
                const SourceLocation &loc);

  std::unique_ptr<WhileStmt>
  createWhileStmt(std::unique_ptr<Expr> condition,
                  std::vector<std::unique_ptr<Stmt>> body,
                  const SourceLocation &loc);

  std::unique_ptr<ReturnStmt> createReturnStmt(std::unique_ptr<Expr> value,
                                               const SourceLocation &loc);

  std::unique_ptr<BreakStmt> createBreakStmt(const SourceLocation &loc);

  std::unique_ptr<ContinueStmt> createContinueStmt(const SourceLocation &loc);

  std::unique_ptr<BlockStmt>
  createBlockStmt(std::vector<std::unique_ptr<Stmt>> body,
                  const SourceLocation &loc);

  std::unique_ptr<PrintStmt> createPrintStmt(std::unique_ptr<Expr> expr,
                                             const SourceLocation &loc);

  std::unique_ptr<TypeDefStmt> createTypeDefStmt(const std::string &name,
                                                 bool isEnum,
                                                 const SourceLocation &loc);

  std::unique_ptr<ModuleStmt> createModuleStmt(const std::string &moduleName,
                                               const SourceLocation &loc);

  std::unique_ptr<ImportStmt>
  createImportStmt(const std::string &modulePath,
                   std::vector<std::string> specificImports,
                   const std::string &alias, const SourceLocation &loc);

  std::unique_ptr<ExportStmt> createExportStmt(const std::string &name,
                                               const SourceLocation &loc);

  std::unique_ptr<ExportStmt> createExportStmt(const std::string &name,
                                               const std::string &typeInfo,
                                               const SourceLocation &loc);

  std::unique_ptr<ExternStmt>
  createExternStmt(const std::string &cName, const std::string &meadowsName,
                   const std::string &returnType,
                   std::vector<std::pair<std::string, std::string>> params,
                   const SourceLocation &loc);

private:
  ASTFactory() = default;
  ~ASTFactory() = default;
  ASTFactory(const ASTFactory &) = delete;
  ASTFactory &operator=(const ASTFactory &) = delete;

  bool metricsEnabled_ = false;
  ASTMetrics metrics_;

  void recordExprCreated() {
    if (metricsEnabled_) {
      metrics_.totalNodesCreated++;
      metrics_.expressionsCreated++;
    }
  }

  void recordStmtCreated() {
    if (metricsEnabled_) {
      metrics_.totalNodesCreated++;
      metrics_.statementsCreated++;
    }
  }

  void recordValidationFailure() {
    if (metricsEnabled_) {
      metrics_.validationFailures++;
    }
  }

  // Validation helpers
  bool validateIdentifier(const std::string &name);
  bool validateOperator(const std::string &op);
  bool validateNotNull(const Expr *ptr, const char *context);
  bool validateNotNull(const Stmt *ptr, const char *context);
};

} // namespace meadows

#endif // AST_FACTORY_H
