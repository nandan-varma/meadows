#pragma once

#include "../lexer/Lexer.h"
#include "../ast/Expression.h"
#include "../ast/Statement.h"
#include <memory>
#include <vector>
#include <stdexcept>

namespace meadows {

class ParseError : public std::runtime_error {
public:
    SourceLocation location;
    
    ParseError(const std::string& message, const SourceLocation& location)
        : std::runtime_error(message), location(location) {}
};

class Parser {
private:
    std::vector<Token> tokens;
    size_t current;
    
    // Token management
    Token peek(int offset = 0) const;
    Token advance();
    bool isAtEnd() const;
    bool check(TokenType type) const;
    bool match(TokenType type);
    bool match(std::initializer_list<TokenType> types);
    Token consume(TokenType type, const std::string& message);
    void skipNewlines();
    
    // Error handling
    void error(const std::string& message);
    void synchronize();
    
    // Expression parsing (precedence climbing)
    std::unique_ptr<Expression> expression();
    std::unique_ptr<Expression> assignment();
    std::unique_ptr<Expression> logicalOr();
    std::unique_ptr<Expression> logicalAnd();
    std::unique_ptr<Expression> equality();
    std::unique_ptr<Expression> comparison();
    std::unique_ptr<Expression> term();
    std::unique_ptr<Expression> factor();
    std::unique_ptr<Expression> power();
    std::unique_ptr<Expression> unary();
    std::unique_ptr<Expression> postfix();
    std::unique_ptr<Expression> primary();
    
    // Statement parsing
    std::unique_ptr<Statement> statement();
    std::unique_ptr<Statement> expressionStatement();
    std::unique_ptr<Statement> ifStatement();
    std::unique_ptr<Statement> whileStatement();
    std::unique_ptr<Statement> forStatement();
    std::unique_ptr<Statement> returnStatement();
    std::unique_ptr<Statement> breakStatement();
    std::unique_ptr<Statement> continueStatement();
    std::unique_ptr<Statement> passStatement();
    std::unique_ptr<Statement> functionDefinition();
    std::unique_ptr<Statement> classDefinition();
    std::unique_ptr<Statement> importStatement();
    std::unique_ptr<Statement> block();
    
    // Helper methods
    std::vector<Parameter> parseParameters();
    std::vector<std::unique_ptr<Expression>> parseArguments();
    BinaryOp tokenToBinaryOp(TokenType type);
    UnaryOp tokenToUnaryOp(TokenType type);
    
public:
    Parser(std::vector<Token> tokens);
    
    std::unique_ptr<Program> parse();
    
    // For error reporting
    std::string getErrorContext() const;
};

} // namespace meadows
