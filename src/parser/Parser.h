#ifndef PARSER_H
#define PARSER_H

#include "../ast/AST.h"
#include "../lexer/Token.h"
#include "../utils/DiagnosticsCollector.h"
#include <memory>
#include <vector>

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
   */
  Parser(std::vector<Token> tokens);

  /**
   * @brief Constructs a Parser with diagnostics collection.
   * @param tokens The token stream to parse.
   * @param diagnostics Collector for errors and warnings.
   */
  Parser(std::vector<Token> tokens, meadows::DiagnosticsCollector &diagnostics);

  /**
   * @brief Parses the entire token stream into an AST.
   * @return A vector of statement AST nodes.
   * @throws std::runtime_error If a fatal syntax error is encountered.
   */
  std::vector<std::unique_ptr<Stmt>> parse();

  /**
   * @brief Check if any errors were reported during parsing.
   */
  bool hasErrors() const;

  /**
   * @brief Get the diagnostics collector (if used).
   */
  meadows::DiagnosticsCollector *diagnostics() { return diagnostics_; }

private:
  std::vector<Token> tokens;
  size_t current;
  meadows::DiagnosticsCollector *diagnostics_;
  bool inErrorRecovery_;
  int consecutiveErrors_;
  static constexpr int MAX_CONSECUTIVE_ERRORS = 3;

  bool isAtEnd() const;
  const Token &peek() const;
  const Token &previous() const;
  const Token &advance();
  bool check(TokenType type) const;
  bool match(TokenType type);
  const Token &consume(TokenType type, meadows::ErrorCode code,
                       const std::string &message);

  // Backward compatibility - uses generic error code
  const Token &consume(TokenType type, const std::string &message) {
    return consume(type, meadows::ErrorCode::PARSE_UNEXPECTED_TOKEN, message);
  }

  /**
   * @brief Report an error at current position.
   */
  void error(meadows::ErrorCode code, const std::string &message);

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

  std::unique_ptr<Expr> parseExpr();
  std::unique_ptr<Expr> parseAssignment();
  std::unique_ptr<Expr> parseOr();
  std::unique_ptr<Expr> parseAnd();
  std::unique_ptr<Expr> parseEquality();
  std::unique_ptr<Expr> parseComparison();
  std::unique_ptr<Expr> parseTerm();
  std::unique_ptr<Expr> parseFactor();
  std::unique_ptr<Expr> parseUnary();
  std::unique_ptr<Expr> parseCall();
  std::unique_ptr<Expr> parseIndex();
  std::unique_ptr<Expr> parseFieldAccess();
  std::unique_ptr<Expr> parsePrimary();

  std::vector<std::unique_ptr<Expr>> parseArgs();
  std::vector<std::unique_ptr<Stmt>> parseBlock();
};

#endif
