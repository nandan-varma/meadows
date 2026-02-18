#include "ASTFactory.h"

#include <cctype>
#include <iostream>

namespace meadows {

ASTFactory &ASTFactory::getInstance() {
  static ASTFactory instance;
  return instance;
}

// ========== Validation Helpers ==========

bool ASTFactory::validateIdentifier(const std::string &name) {
  if (name.empty()) {
    return false;
  }
  // First character must be letter or underscore
  if (!std::isalpha(name[0]) && name[0] != '_') {
    return false;
  }
  // Rest can be alphanumeric or underscore
  for (size_t i = 1; i < name.size(); ++i) {
    if (!std::isalnum(name[i]) && name[i] != '_') {
      return false;
    }
  }
  return true;
}

bool ASTFactory::validateOperator(const std::string &op) {
  static const std::vector<std::string> validOps = {"+",  "-",  "*",  "/", "=",
                                                    "==", "!=", "<",  ">", "<=",
                                                    ">=", "!",  "&&", "||"};
  for (const auto &valid : validOps) {
    if (op == valid)
      return true;
  }
  return false;
}

bool ASTFactory::validateNotNull(const Expr *ptr, const char *context) {
  if (ptr == nullptr) {
    std::cerr << "ASTFactory: Null expression in " << context << std::endl;
    recordValidationFailure();
    return false;
  }
  return true;
}

bool ASTFactory::validateNotNull(const Stmt *ptr, const char *context) {
  if (ptr == nullptr) {
    std::cerr << "ASTFactory: Null statement in " << context << std::endl;
    recordValidationFailure();
    return false;
  }
  return true;
}

// ========== Expression Factory Methods ==========

std::unique_ptr<LiteralExpr>
ASTFactory::createLiteralExpr(const std::string &value,
                              const SourceLocation &loc) {
  recordExprCreated();
  return std::make_unique<LiteralExpr>(value);
}

std::unique_ptr<VarExpr> ASTFactory::createVarExpr(const std::string &name,
                                                   const SourceLocation &loc) {
  if (!validateIdentifier(name)) {
    std::cerr << "ASTFactory: Invalid identifier '" << name << "' at "
              << loc.toString() << std::endl;
    recordValidationFailure();
  }
  recordExprCreated();
  return std::make_unique<VarExpr>(name);
}

std::unique_ptr<AssignExpr>
ASTFactory::createAssignExpr(const std::string &name,
                             std::unique_ptr<Expr> value,
                             const SourceLocation &loc) {
  if (!validateIdentifier(name)) {
    std::cerr << "ASTFactory: Invalid identifier in assignment '" << name
              << "' at " << loc.toString() << std::endl;
    recordValidationFailure();
  }
  validateNotNull(value.get(), "assignment value");
  recordExprCreated();
  return std::make_unique<AssignExpr>(name, std::move(value));
}

std::unique_ptr<BinaryExpr>
ASTFactory::createBinaryExpr(std::unique_ptr<Expr> left, const std::string &op,
                             std::unique_ptr<Expr> right,
                             const SourceLocation &loc) {
  validateNotNull(left.get(), "binary expression left operand");
  validateNotNull(right.get(), "binary expression right operand");
  if (!validateOperator(op)) {
    std::cerr << "ASTFactory: Invalid operator '" << op << "' at "
              << loc.toString() << std::endl;
    recordValidationFailure();
  }
  recordExprCreated();
  return std::make_unique<BinaryExpr>(std::move(left), op, std::move(right));
}

std::unique_ptr<UnaryExpr>
ASTFactory::createUnaryExpr(const std::string &op,
                            std::unique_ptr<Expr> operand,
                            const SourceLocation &loc) {
  validateNotNull(operand.get(), "unary expression operand");
  if (op != "!" && op != "-" && op != "+") {
    std::cerr << "ASTFactory: Invalid unary operator '" << op << "' at "
              << loc.toString() << std::endl;
    recordValidationFailure();
  }
  recordExprCreated();
  return std::make_unique<UnaryExpr>(op, std::move(operand));
}

std::unique_ptr<CallExpr>
ASTFactory::createCallExpr(std::unique_ptr<Expr> callee,
                           std::vector<std::unique_ptr<Expr>> args,
                           const SourceLocation &loc) {
  validateNotNull(callee.get(), "call expression callee");
  for (const auto &arg : args) {
    validateNotNull(arg.get(), "call expression argument");
  }
  recordExprCreated();
  return std::make_unique<CallExpr>(std::move(callee), std::move(args));
}

std::unique_ptr<LogicalExpr>
ASTFactory::createLogicalExpr(std::unique_ptr<Expr> left, LogicalOperator op,
                              std::unique_ptr<Expr> right,
                              const SourceLocation &loc) {
  validateNotNull(left.get(), "logical expression left operand");
  validateNotNull(right.get(), "logical expression right operand");
  recordExprCreated();
  return std::make_unique<LogicalExpr>(std::move(left), op, std::move(right));
}

std::unique_ptr<IndexExpr>
ASTFactory::createIndexExpr(std::unique_ptr<Expr> array,
                            std::unique_ptr<Expr> index,
                            const SourceLocation &loc) {
  validateNotNull(array.get(), "index expression array");
  validateNotNull(index.get(), "index expression index");
  recordExprCreated();
  return std::make_unique<IndexExpr>(std::move(array), std::move(index));
}

std::unique_ptr<FieldAccessExpr>
ASTFactory::createFieldAccessExpr(std::unique_ptr<Expr> object,
                                  const std::string &fieldName,
                                  const SourceLocation &loc) {
  validateNotNull(object.get(), "field access expression object");
  if (!validateIdentifier(fieldName)) {
    std::cerr << "ASTFactory: Invalid field name '" << fieldName << "' at "
              << loc.toString() << std::endl;
    recordValidationFailure();
  }
  recordExprCreated();
  return std::make_unique<FieldAccessExpr>(std::move(object), fieldName);
}

std::unique_ptr<TryExpr> ASTFactory::createTryExpr(std::unique_ptr<Expr> expr,
                                                   const SourceLocation &loc) {
  validateNotNull(expr.get(), "try expression");
  recordExprCreated();
  return std::make_unique<TryExpr>(std::move(expr));
}

std::unique_ptr<ArrayExpr>
ASTFactory::createArrayExpr(std::vector<std::unique_ptr<Expr>> elements,
                            const SourceLocation &loc) {
  for (const auto &elem : elements) {
    validateNotNull(elem.get(), "array element");
  }
  recordExprCreated();
  return std::make_unique<ArrayExpr>(std::move(elements));
}

std::unique_ptr<ObjectExpr> ASTFactory::createObjectExpr(
    std::unordered_map<std::string, std::unique_ptr<Expr>> pairs,
    const SourceLocation &loc) {
  for (const auto &pair : pairs) {
    validateNotNull(pair.second.get(), "object field");
  }
  recordExprCreated();
  return std::make_unique<ObjectExpr>(std::move(pairs));
}

std::unique_ptr<MatchExpr>
ASTFactory::createMatchExpr(std::unique_ptr<Expr> scrutinee,
                            std::vector<MatchArm> arms,
                            const SourceLocation &loc) {
  validateNotNull(scrutinee.get(), "match expression scrutinee");
  recordExprCreated();
  return std::make_unique<MatchExpr>(std::move(scrutinee), std::move(arms));
}

std::unique_ptr<EnumVariantExpr> ASTFactory::createEnumVariantExpr(
    const std::string &enumName, const std::string &variantName,
    std::vector<std::unique_ptr<Expr>> args, const SourceLocation &loc) {
  if (!validateIdentifier(enumName)) {
    std::cerr << "ASTFactory: Invalid enum name '" << enumName << "' at "
              << loc.toString() << std::endl;
    recordValidationFailure();
  }
  if (!validateIdentifier(variantName)) {
    std::cerr << "ASTFactory: Invalid variant name '" << variantName << "' at "
              << loc.toString() << std::endl;
    recordValidationFailure();
  }
  for (const auto &arg : args) {
    validateNotNull(arg.get(), "enum variant argument");
  }
  recordExprCreated();
  return std::make_unique<EnumVariantExpr>(enumName, variantName,
                                           std::move(args));
}

// ========== Statement Factory Methods ==========

std::unique_ptr<ExprStmt>
ASTFactory::createExprStmt(std::unique_ptr<Expr> expr,
                           const SourceLocation &loc) {
  validateNotNull(expr.get(), "expression statement");
  recordStmtCreated();
  return std::make_unique<ExprStmt>(std::move(expr));
}

std::unique_ptr<LetStmt>
ASTFactory::createLetStmt(const std::string &name,
                          std::unique_ptr<Expr> initializer,
                          const SourceLocation &loc) {
  return createLetStmt(name, std::move(initializer), "", loc);
}

std::unique_ptr<LetStmt> ASTFactory::createLetStmt(
    const std::string &name, std::unique_ptr<Expr> initializer,
    const std::string &typeAnnotation, const SourceLocation &loc) {
  if (!validateIdentifier(name)) {
    std::cerr << "ASTFactory: Invalid variable name '" << name << "' at "
              << loc.toString() << std::endl;
    recordValidationFailure();
  }
  validateNotNull(initializer.get(), "let statement initializer");
  recordStmtCreated();
  return std::make_unique<LetStmt>(name, std::move(initializer),
                                   typeAnnotation);
}

std::unique_ptr<FuncStmt> ASTFactory::createFuncStmt(
    const std::string &name, std::vector<FuncParam> params,
    std::vector<std::unique_ptr<Stmt>> body, const SourceLocation &loc) {
  return createFuncStmt(name, std::move(params), std::move(body), "", loc);
}

std::unique_ptr<FuncStmt> ASTFactory::createFuncStmt(
    const std::string &name, std::vector<FuncParam> params,
    std::vector<std::unique_ptr<Stmt>> body,
    const std::string &returnTypeAnnotation, const SourceLocation &loc) {
  if (!validateIdentifier(name)) {
    std::cerr << "ASTFactory: Invalid function name '" << name << "' at "
              << loc.toString() << std::endl;
    recordValidationFailure();
  }
  if (body.empty()) {
    std::cerr << "ASTFactory: Empty function body for '" << name << "' at "
              << loc.toString() << std::endl;
    recordValidationFailure();
  }
  for (const auto &stmt : body) {
    validateNotNull(stmt.get(), "function body statement");
  }
  recordStmtCreated();
  return std::make_unique<FuncStmt>(name, std::move(params), std::move(body),
                                    returnTypeAnnotation);
}

std::unique_ptr<IfStmt>
ASTFactory::createIfStmt(std::unique_ptr<Expr> condition,
                         std::vector<std::unique_ptr<Stmt>> thenBranch,
                         std::vector<std::unique_ptr<Stmt>> elseBranch,
                         const SourceLocation &loc) {
  validateNotNull(condition.get(), "if statement condition");
  if (thenBranch.empty()) {
    std::cerr << "ASTFactory: Empty then branch at " << loc.toString()
              << std::endl;
    recordValidationFailure();
  }
  for (const auto &stmt : thenBranch) {
    validateNotNull(stmt.get(), "if then branch statement");
  }
  for (const auto &stmt : elseBranch) {
    validateNotNull(stmt.get(), "if else branch statement");
  }
  recordStmtCreated();
  return std::make_unique<IfStmt>(std::move(condition), std::move(thenBranch),
                                  std::move(elseBranch));
}

std::unique_ptr<ForStmt> ASTFactory::createForStmt(
    const std::string &var, std::unique_ptr<Expr> rangeStart,
    std::unique_ptr<Expr> rangeEnd, std::vector<std::unique_ptr<Stmt>> body,
    const SourceLocation &loc) {
  if (!validateIdentifier(var)) {
    std::cerr << "ASTFactory: Invalid loop variable '" << var << "' at "
              << loc.toString() << std::endl;
    recordValidationFailure();
  }
  validateNotNull(rangeStart.get(), "for loop range start");
  validateNotNull(rangeEnd.get(), "for loop range end");
  if (body.empty()) {
    std::cerr << "ASTFactory: Empty for loop body at " << loc.toString()
              << std::endl;
    recordValidationFailure();
  }
  for (const auto &stmt : body) {
    validateNotNull(stmt.get(), "for loop body statement");
  }
  recordStmtCreated();
  return std::make_unique<ForStmt>(var, std::move(rangeStart),
                                   std::move(rangeEnd), std::move(body));
}

std::unique_ptr<WhileStmt>
ASTFactory::createWhileStmt(std::unique_ptr<Expr> condition,
                            std::vector<std::unique_ptr<Stmt>> body,
                            const SourceLocation &loc) {
  validateNotNull(condition.get(), "while loop condition");
  if (body.empty()) {
    std::cerr << "ASTFactory: Empty while loop body at " << loc.toString()
              << std::endl;
    recordValidationFailure();
  }
  for (const auto &stmt : body) {
    validateNotNull(stmt.get(), "while loop body statement");
  }
  recordStmtCreated();
  return std::make_unique<WhileStmt>(std::move(condition), std::move(body));
}

std::unique_ptr<ReturnStmt>
ASTFactory::createReturnStmt(std::unique_ptr<Expr> value,
                             const SourceLocation &loc) {
  recordStmtCreated();
  return std::make_unique<ReturnStmt>(std::move(value));
}

std::unique_ptr<BreakStmt>
ASTFactory::createBreakStmt(const SourceLocation &loc) {
  recordStmtCreated();
  return std::make_unique<BreakStmt>();
}

std::unique_ptr<ContinueStmt>
ASTFactory::createContinueStmt(const SourceLocation &loc) {
  recordStmtCreated();
  return std::make_unique<ContinueStmt>();
}

std::unique_ptr<BlockStmt>
ASTFactory::createBlockStmt(std::vector<std::unique_ptr<Stmt>> body,
                            const SourceLocation &loc) {
  for (const auto &stmt : body) {
    validateNotNull(stmt.get(), "block statement");
  }
  recordStmtCreated();
  return std::make_unique<BlockStmt>(std::move(body));
}

std::unique_ptr<PrintStmt>
ASTFactory::createPrintStmt(std::unique_ptr<Expr> expr,
                            const SourceLocation &loc) {
  validateNotNull(expr.get(), "print statement expression");
  recordStmtCreated();
  return std::make_unique<PrintStmt>(std::move(expr));
}

std::unique_ptr<TypeDefStmt>
ASTFactory::createTypeDefStmt(const std::string &name, bool isEnum,
                              const SourceLocation &loc) {
  if (!validateIdentifier(name)) {
    std::cerr << "ASTFactory: Invalid type name '" << name << "' at "
              << loc.toString() << std::endl;
    recordValidationFailure();
  }
  recordStmtCreated();
  return std::make_unique<TypeDefStmt>(name, isEnum);
}

std::unique_ptr<ModuleStmt>
ASTFactory::createModuleStmt(const std::string &moduleName,
                             const SourceLocation &loc) {
  if (moduleName.empty()) {
    std::cerr << "ASTFactory: Empty module name at " << loc.toString()
              << std::endl;
    recordValidationFailure();
  }
  recordStmtCreated();
  return std::make_unique<ModuleStmt>(moduleName);
}

std::unique_ptr<ImportStmt> ASTFactory::createImportStmt(
    const std::string &modulePath, std::vector<std::string> specificImports,
    const std::string &alias, const SourceLocation &loc) {
  if (modulePath.empty()) {
    std::cerr << "ASTFactory: Empty module path at " << loc.toString()
              << std::endl;
    recordValidationFailure();
  }
  recordStmtCreated();
  return std::make_unique<ImportStmt>(modulePath, std::move(specificImports),
                                      alias);
}

std::unique_ptr<ExportStmt>
ASTFactory::createExportStmt(const std::string &name,
                             const SourceLocation &loc) {
  return createExportStmt(name, "", loc);
}

std::unique_ptr<ExportStmt>
ASTFactory::createExportStmt(const std::string &name,
                             const std::string &typeInfo,
                             const SourceLocation &loc) {
  if (!validateIdentifier(name)) {
    std::cerr << "ASTFactory: Invalid export name '" << name << "' at "
              << loc.toString() << std::endl;
    recordValidationFailure();
  }
  recordStmtCreated();
  return std::make_unique<ExportStmt>(name, typeInfo);
}

std::unique_ptr<ExternStmt> ASTFactory::createExternStmt(
    const std::string &cName, const std::string &meadowsName,
    const std::string &returnType,
    std::vector<std::pair<std::string, std::string>> params,
    const SourceLocation &loc) {
  if (cName.empty() || meadowsName.empty()) {
    std::cerr << "ASTFactory: Empty extern name at " << loc.toString()
              << std::endl;
    recordValidationFailure();
  }
  recordStmtCreated();
  return std::make_unique<ExternStmt>(cName, meadowsName, returnType,
                                      std::move(params));
}

} // namespace meadows
