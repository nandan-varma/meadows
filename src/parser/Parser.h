#ifndef PARSER_H
#define PARSER_H

#include "../ast/AST.h"
#include "../lexer/Token.h"
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
 * @ Responsibility
 * - Parse tokens into AST nodes according to language grammar
 * - Handle all statement types (variable declarations, functions, control flow)
 * - Parse expressions with correct operator precedence
 * - Generate descriptive error messages for syntax errors
 */
class Parser {
public:
  /**
   * @brief Constructs a Parser for the given tokens.
   * @param tokens The token stream to parse.
   */
  Parser(std::vector<Token> tokens);

  /**
   * @brief Parses the entire token stream into an AST.
   * @return A vector of statement AST nodes.
   * @throws std::runtime_error If a syntax error is encountered.
   */
  std::vector<std::unique_ptr<Stmt>> parse();

private:
  std::vector<Token> tokens;
  size_t current;

  bool isAtEnd();
  const Token &peek();
  const Token &previous();
  const Token &advance();
  bool check(TokenType type);
  bool match(TokenType type);
  const Token &consume(TokenType type, const std::string &message);

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