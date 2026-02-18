#ifndef PARSER_H
#define PARSER_H

#include <memory>
#include <string>
#include <vector>

#include "../ast/AST.h"
#include "../ast/ASTFactory.h"
#include "../errors/ErrorHandler.h"
#include "../lexer/Token.h"
#include "../modules/ModuleResolver.h"

/**
 * @class Parser
 * @brief Performs syntax analysis on tokenized Meadows source code.
 *
 * The Parser uses recursive descent parsing to build an Abstract Syntax Tree
 * (AST) from the token stream produced by the Lexer. It enforces language
 * grammar rules and provides meaningful error messages with line numbers.
 *
 * Supports error recovery - continues parsing after non-fatal errors to
 * report multiple issues in a single pass.
 */
class Parser {
public:
  /**
   * @brief Constructs a Parser for the given tokens.
   * @param tokens The token stream to parse.
   * @param sourceFile The source file path for error reporting.
   */
  Parser(std::vector<Token> tokens, const std::string &sourceFile = "");

  /**
   * @brief Parses the entire token stream into an AST.
   * @return A vector of statement AST nodes.
   */
  std::vector<std::unique_ptr<Stmt>> parse();

  /**
   * @brief Check if any errors were reported during parsing.
   */
  bool hasErrors() const;

  /**
   * @brief Get the error handler.
   */
  meadows::ErrorHandler &errorHandler() { return *errorHandler_; }

  /**
   * @brief Get the error context.
   */
  const meadows::ErrorContext &getErrorContext() const {
    return errorHandler_->getContext();
  }

  /**
   * @brief Set the source file path for module resolution.
   */
  void setSourcePath(const std::string &path) { sourcePath_ = path; }

  /**
   * @brief Set the stdlib path for module resolution.
   */
  void setStdlibPath(const std::string &path) {
    resolverConfig_.stdlibPath = path;
    resolver_ = std::make_unique<meadows::ModuleResolver>(resolverConfig_);
  }

  /**
   * @brief Set the project root for module resolution.
   */
  void setProjectRoot(const std::string &path) {
    resolverConfig_.projectRoot = path;
    resolver_ = std::make_unique<meadows::ModuleResolver>(resolverConfig_);
  }

private:
  std::vector<Token> tokens_;
  size_t current_;
  std::unique_ptr<meadows::ErrorHandler> errorHandler_;
  bool inErrorRecovery_;
  int consecutiveErrors_;
  std::string sourcePath_;
  std::string sourceFile_;
  meadows::ModuleResolverConfig resolverConfig_;
  std::unique_ptr<meadows::ModuleResolver> resolver_;

  static constexpr int MAX_CONSECUTIVE_ERRORS = 3;

  // AST Factory for node creation
  meadows::ASTFactory &factory_;

  bool isAtEnd() const;
  const Token &peek() const;
  const Token &previous() const;
  const Token &advance();
  bool check(TokenType type) const;
  bool match(TokenType type);
  const Token &consume(TokenType type, const std::string &message);

  /**
   * @brief Create a source location from current token position.
   */
  meadows::SourceLocation currentLocation() const;

  /**
   * @brief Report a parse error.
   */
  void error(const std::string &message);

  /**
   * @brief Synchronize parser state after error (panic mode recovery).
   * Skips tokens until a synchronization point is found.
   */
  void synchronize();

  /**
   * @brief Check if we should stop recovery attempts.
   */
  bool shouldAbortRecovery() const;

  std::unique_ptr<Stmt> parseStmt();
  std::unique_ptr<Stmt> parseLetStmt();
  std::unique_ptr<Stmt> parseFuncStmt();
  std::unique_ptr<Stmt> parseIfStmt();
  std::unique_ptr<Stmt> parseForStmt();
  std::unique_ptr<Stmt> parseWhileStmt();
  std::unique_ptr<Stmt> parseReturnStmt();
  std::unique_ptr<Stmt> parseBlockStmt();
  std::unique_ptr<Stmt> parsePrintStmt();
  std::unique_ptr<Stmt> parseExprStmt();
  std::unique_ptr<Stmt> parseBreakStmt();
  std::unique_ptr<Stmt> parseContinueStmt();
  std::unique_ptr<Stmt> parseModuleStmt();
  std::unique_ptr<Stmt> parseImportStmt();
  std::unique_ptr<Stmt> parseExportStmt();
  std::unique_ptr<Stmt> parseExternStmt();
  std::unique_ptr<Stmt> parseTypeDefStmt();

  std::unique_ptr<Expr> parseExpr();
  std::unique_ptr<Expr> parseAssignment(int depth = 0);
  std::unique_ptr<Expr> parseOr(int depth = 0);
  std::unique_ptr<Expr> parseAnd(int depth = 0);
  std::unique_ptr<Expr> parseEquality(int depth = 0);
  std::unique_ptr<Expr> parseComparison(int depth = 0);
  std::unique_ptr<Expr> parseTerm(int depth = 0);
  std::unique_ptr<Expr> parseFactor(int depth = 0);
  std::unique_ptr<Expr> parseUnary(int depth = 0);
  std::unique_ptr<Expr> parseCall(int depth = 0);
  std::unique_ptr<Expr> parseIndex(int depth = 0);
  std::unique_ptr<Expr> parseFieldAccess(int depth = 0);
  std::unique_ptr<Expr> parsePrimary(int depth = 0);
  std::unique_ptr<Expr> parseMatchExpr();
  std::unique_ptr<Pattern> parsePattern();
  std::unique_ptr<Pattern> parsePatternInternal();
  std::unique_ptr<Expr> parseEnumVariant();

  std::vector<std::unique_ptr<Expr>> parseArgs();
  std::vector<std::unique_ptr<Stmt>> parseBlock();

  std::vector<std::unique_ptr<Stmt>>
  resolveAndParseModule(const std::string &modulePath);
};

#endif
