#ifndef PARSER_H
#define PARSER_H

#include "../lexer/Token.h"
#include "../ast/AST.h"
#include <vector>
#include <memory>

class Parser {
public:
    Parser(const std::vector<Token>& tokens);
    std::vector<std::unique_ptr<Stmt>> parse();

private:
    const std::vector<Token>& tokens;
    size_t current;

    bool isAtEnd();
    const Token& peek();
    const Token& previous();
    const Token& advance();
    bool check(TokenType type);
    bool match(TokenType type);
    const Token& consume(TokenType type, const std::string& message);

    std::unique_ptr<Stmt> parseStmt();
    std::unique_ptr<Stmt> parseLetStmt();
    std::unique_ptr<Stmt> parseFuncStmt();
    std::unique_ptr<Stmt> parseIfStmt();
    std::unique_ptr<Stmt> parseForStmt();
    std::unique_ptr<Stmt> parseWhileStmt();
    std::unique_ptr<Stmt> parseReturnStmt();
    std::unique_ptr<Stmt> parsePrintStmt();
    std::unique_ptr<Stmt> parseExprStmt();

    std::unique_ptr<Expr> parseExpr();
    std::unique_ptr<Expr> parseEquality();
    std::unique_ptr<Expr> parseComparison();
    std::unique_ptr<Expr> parseTerm();
    std::unique_ptr<Expr> parseFactor();
    std::unique_ptr<Expr> parseUnary();
    std::unique_ptr<Expr> parseCall();
    std::unique_ptr<Expr> parsePrimary();

    std::vector<std::unique_ptr<Expr>> parseArgs();
    std::vector<std::unique_ptr<Stmt>> parseBlock();
};

#endif