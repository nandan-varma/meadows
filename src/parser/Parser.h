#ifndef PARSER_H
#define PARSER_H

#include <memory>
#include <vector>

#include "../ast/AST.h"
#include "../lexer/Token.h"
#include "../utils/DiagnosticsCollector.h"

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
   * @brief Constructs a Parser with diagnostics collection.
   * @param tokens The token stream to parse.
   * @param diagnostics Collector for errors and warnings.
   */
  Parser(std::vector<Token> tokens, meadows::DiagnosticsCollector &diagnostics);

  /**
   * @brief Parses the entire token stream into an AST.
   * @return A vector of statement AST nodes. Non-fatal errors are reported
   *         to the diagnostics collector and parsing continues; only a
   *         handful of unrecoverable conditions (e.g. pathological nesting
   *         depth) throw meadows::ParseException.
   */
  std::vector<std::unique_ptr<Stmt>> parse();

  /**
   * @brief Check if any errors were reported during parsing.
   */
  bool hasErrors() const;

private:
  std::vector<Token> tokens_;
  size_t current_;
  meadows::DiagnosticsCollector &diagnostics_;
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
  std::unique_ptr<Stmt> parseExprStmt();
  std::unique_ptr<Stmt> parseBreakStmt();
  std::unique_ptr<Stmt> parseContinueStmt();

  template <typename T>
  std::unique_ptr<T> withLocation(std::unique_ptr<T> node) {
    if (node) {
      node->line = peek().line;
      node->column = peek().column;
    }
    return node;
  }

  std::unique_ptr<Expr> parseExpr(int depth = 0);
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

  std::vector<std::unique_ptr<Expr>> parseArgs(int depth);
  std::vector<std::unique_ptr<Stmt>> parseBlock();
};

#endif
