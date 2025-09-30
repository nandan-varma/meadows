#include "Parser.h"
#include <stdexcept>

Parser::Parser(const std::vector<Token>& t) : tokens(t), current(0) {}

std::vector<std::unique_ptr<Stmt>> Parser::parse() {
    std::vector<std::unique_ptr<Stmt>> statements;
    while (!isAtEnd()) {
        statements.push_back(parseStmt());
    }
    return statements;
}

bool Parser::isAtEnd() {
    return peek().type == TokenType::EOF_TOKEN;
}

const Token& Parser::peek() {
    return tokens[current];
}

const Token& Parser::previous() {
    return tokens[current - 1];
}

const Token& Parser::advance() {
    if (!isAtEnd()) current++;
    return previous();
}

bool Parser::check(TokenType type) {
    if (isAtEnd()) return false;
    return peek().type == type;
}

bool Parser::match(TokenType type) {
    if (check(type)) {
        advance();
        return true;
    }
    return false;
}

const Token& Parser::consume(TokenType type, const std::string& message) {
    if (check(type)) {
        return advance();
    }
    throw std::runtime_error(message + " at line " + std::to_string(peek().line));
}

std::unique_ptr<Stmt> Parser::parseStmt() {
    if (match(TokenType::LET)) return parseLetStmt();
    if (match(TokenType::FUNC)) return parseFuncStmt();
    if (match(TokenType::IF)) return parseIfStmt();
    if (match(TokenType::FOR)) return parseForStmt();
    if (match(TokenType::WHILE)) return parseWhileStmt();
    if (match(TokenType::PRINT)) return parsePrintStmt();
    if (match(TokenType::RETURN)) return parseReturnStmt();
    return parseExprStmt();
}

std::unique_ptr<Stmt> Parser::parseLetStmt() {
    const Token& name = consume(TokenType::IDENTIFIER, "Expect variable name");
    consume(TokenType::EQUAL, "Expect '=' after variable name");
    auto initializer = parseExpr();
    consume(TokenType::SEMICOLON, "Expect ';' after variable declaration");
    return std::make_unique<LetStmt>(name.value, std::move(initializer));
}

std::unique_ptr<Stmt> Parser::parseFuncStmt() {
    const Token& name = consume(TokenType::IDENTIFIER, "Expect function name");
    consume(TokenType::LEFT_PAREN, "Expect '(' after function name");
    std::vector<std::string> params;
    if (!check(TokenType::RIGHT_PAREN)) {
        do {
            const Token& param = consume(TokenType::IDENTIFIER, "Expect parameter name");
            params.push_back(param.value);
        } while (match(TokenType::COMMA));
    }
    consume(TokenType::RIGHT_PAREN, "Expect ')' after parameters");
    consume(TokenType::LEFT_BRACE, "Expect '{' before function body");
    auto body = parseBlock();
    consume(TokenType::RIGHT_BRACE, "Expect '}' after function body");
    return std::make_unique<FuncStmt>(name.value, params, std::move(body));
}

std::unique_ptr<Stmt> Parser::parseIfStmt() {
    consume(TokenType::LEFT_PAREN, "Expect '(' after 'if'");
    auto condition = parseExpr();
    consume(TokenType::RIGHT_PAREN, "Expect ')' after condition");
    consume(TokenType::LEFT_BRACE, "Expect '{' after condition");
    auto thenBranch = parseBlock();
    consume(TokenType::RIGHT_BRACE, "Expect '}' after then branch");
    std::vector<std::unique_ptr<Stmt>> elseBranch;
    if (match(TokenType::ELSE)) {
        consume(TokenType::LEFT_BRACE, "Expect '{' after 'else'");
        elseBranch = parseBlock();
        consume(TokenType::RIGHT_BRACE, "Expect '}' after else branch");
    }
    return std::make_unique<IfStmt>(std::move(condition), std::move(thenBranch), std::move(elseBranch));
}

std::unique_ptr<Stmt> Parser::parseForStmt() {
    consume(TokenType::LEFT_PAREN, "Expect '(' after 'for'");
    const Token& var = consume(TokenType::IDENTIFIER, "Expect variable name");
    consume(TokenType::IN, "Expect 'in' after variable");
    consume(TokenType::RANGE, "Expect 'range' after 'in'");
    consume(TokenType::LEFT_PAREN, "Expect '(' after 'range'");
    auto start = parseExpr();
    consume(TokenType::COMMA, "Expect ',' after start");
    auto end = parseExpr();
    consume(TokenType::RIGHT_PAREN, "Expect ')' after end");
    consume(TokenType::RIGHT_PAREN, "Expect ')' after range");
    consume(TokenType::LEFT_BRACE, "Expect '{' after range");
    auto body = parseBlock();
    consume(TokenType::RIGHT_BRACE, "Expect '}' after body");
    return std::make_unique<ForStmt>(var.value, std::move(start), std::move(end), std::move(body));
}

std::unique_ptr<Stmt> Parser::parseWhileStmt() {
    consume(TokenType::LEFT_PAREN, "Expect '(' after 'while'");
    auto condition = parseExpr();
    consume(TokenType::RIGHT_PAREN, "Expect ')' after condition");
    consume(TokenType::LEFT_BRACE, "Expect '{' after condition");
    auto body = parseBlock();
    consume(TokenType::RIGHT_BRACE, "Expect '}' after body");
    return std::make_unique<WhileStmt>(std::move(condition), std::move(body));
}

std::unique_ptr<Stmt> Parser::parseReturnStmt() {
    auto value = parseExpr();
    consume(TokenType::SEMICOLON, "Expect ';' after return value");
    return std::make_unique<ReturnStmt>(std::move(value));
}

std::unique_ptr<Stmt> Parser::parsePrintStmt() {
    auto expr = parseExpr();
    consume(TokenType::SEMICOLON, "Expect ';' after print");
    return std::make_unique<PrintStmt>(std::move(expr));
}

std::unique_ptr<Stmt> Parser::parseExprStmt() {
    auto expr = parseExpr();
    consume(TokenType::SEMICOLON, "Expect ';' after expression");
    return std::make_unique<ExprStmt>(std::move(expr));
}

std::vector<std::unique_ptr<Stmt>> Parser::parseBlock() {
    std::vector<std::unique_ptr<Stmt>> statements;
    while (!check(TokenType::RIGHT_BRACE) && !isAtEnd()) {
        statements.push_back(parseStmt());
    }
    return statements;
}

std::unique_ptr<Expr> Parser::parseExpr() {
    return parseEquality();
}

std::unique_ptr<Expr> Parser::parseEquality() {
    auto expr = parseComparison();
    while (match(TokenType::EQUAL_EQUAL)) {
        std::string op = previous().value;
        auto right = parseComparison();
        expr = std::make_unique<BinaryExpr>(std::move(expr), op, std::move(right));
    }
    return expr;
}

std::unique_ptr<Expr> Parser::parseComparison() {
    auto expr = parseTerm();
    while (match(TokenType::GREATER) || match(TokenType::GREATER_EQUAL) || match(TokenType::LESS) || match(TokenType::LESS_EQUAL)) {
        std::string op = previous().value;
        auto right = parseTerm();
        expr = std::make_unique<BinaryExpr>(std::move(expr), op, std::move(right));
    }
    return expr;
}

std::unique_ptr<Expr> Parser::parseTerm() {
    auto expr = parseFactor();
    while (match(TokenType::PLUS) || match(TokenType::MINUS)) {
        std::string op = previous().value;
        auto right = parseFactor();
        expr = std::make_unique<BinaryExpr>(std::move(expr), op, std::move(right));
    }
    return expr;
}

std::unique_ptr<Expr> Parser::parseFactor() {
    auto expr = parseUnary();
    while (match(TokenType::STAR) || match(TokenType::SLASH)) {
        std::string op = previous().value;
        auto right = parseUnary();
        expr = std::make_unique<BinaryExpr>(std::move(expr), op, std::move(right));
    }
    return expr;
}

std::unique_ptr<Expr> Parser::parseUnary() {
    if (match(TokenType::MINUS)) {
        std::string op = previous().value;
        auto right = parseUnary();
        return std::make_unique<BinaryExpr>(nullptr, op, std::move(right)); // For simplicity, treat as binary with null left
    }
    return parseCall();
}

std::unique_ptr<Expr> Parser::parseCall() {
    auto expr = parsePrimary();
    if (match(TokenType::LEFT_PAREN)) {
        auto args = parseArgs();
        consume(TokenType::RIGHT_PAREN, "Expect ')' after arguments");
        expr = std::make_unique<CallExpr>(std::move(expr), std::move(args));
    }
    return expr;
}

std::unique_ptr<Expr> Parser::parsePrimary() {
    if (match(TokenType::STRING)) {
        return std::make_unique<LiteralExpr>(previous().value);
    }
    if (match(TokenType::NUMBER)) {
        return std::make_unique<LiteralExpr>(previous().value);
    }
    if (match(TokenType::IDENTIFIER)) {
        return std::make_unique<VarExpr>(previous().value);
    }
    if (match(TokenType::LEFT_BRACKET)) {
        std::vector<std::unique_ptr<Expr>> elements;
        if (!check(TokenType::RIGHT_BRACKET)) {
            do {
                elements.push_back(parseExpr());
            } while (match(TokenType::COMMA));
        }
        consume(TokenType::RIGHT_BRACKET, "Expect ']' after array elements");
        return std::make_unique<ArrayExpr>(std::move(elements));
    }
    if (match(TokenType::LEFT_BRACE)) {
        std::unordered_map<std::string, std::unique_ptr<Expr>> pairs;
        if (!check(TokenType::RIGHT_BRACE)) {
            do {
                const Token& key = consume(TokenType::IDENTIFIER, "Expect key");
                consume(TokenType::COLON, "Expect ':' after key");
                auto value = parseExpr();
                pairs[key.value] = std::move(value);
            } while (match(TokenType::COMMA));
        }
        consume(TokenType::RIGHT_BRACE, "Expect '}' after object");
        return std::make_unique<ObjectExpr>(std::move(pairs));
    }
    if (match(TokenType::LEFT_PAREN)) {
        auto expr = parseExpr();
        consume(TokenType::RIGHT_PAREN, "Expect ')' after expression");
        return expr;
    }
    throw std::runtime_error("Expect expression at line " + std::to_string(peek().line));
}

std::vector<std::unique_ptr<Expr>> Parser::parseArgs() {
    std::vector<std::unique_ptr<Expr>> args;
    if (!check(TokenType::RIGHT_PAREN)) {
        do {
            args.push_back(parseExpr());
        } while (match(TokenType::COMMA));
    }
    return args;
}