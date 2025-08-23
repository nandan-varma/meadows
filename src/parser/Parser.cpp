#include "Parser.h"
#include <iostream>

namespace meadows {

Parser::Parser(std::vector<Token> tokens) : tokens(std::move(tokens)), current(0) {}

Token Parser::peek(int offset) const {
    size_t pos = current + offset;
    if (pos >= tokens.size()) {
        return tokens.back(); // Should be EOF_TOKEN
    }
    return tokens[pos];
}

Token Parser::advance() {
    if (!isAtEnd()) {
        current++;
    }
    return tokens[current - 1];
}

bool Parser::isAtEnd() const {
    return current >= tokens.size() || peek().type == TokenType::EOF_TOKEN;
}

bool Parser::check(TokenType type) const {
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

bool Parser::match(std::initializer_list<TokenType> types) {
    for (TokenType type : types) {
        if (check(type)) {
            advance();
            return true;
        }
    }
    return false;
}

Token Parser::consume(TokenType type, const std::string& message) {
    if (check(type)) {
        return advance();
    }
    error(message);
    return peek(); // This won't be reached due to exception
}

void Parser::skipNewlines() {
    while (match(TokenType::NEWLINE)) {
        // Skip newlines
    }
}

void Parser::error(const std::string& message) {
    throw ParseError(message, peek().location);
}

void Parser::synchronize() {
    advance();
    
    while (!isAtEnd()) {
        if (tokens[current - 1].type == TokenType::NEWLINE) return;
        
        switch (peek().type) {
            case TokenType::CLASS:
            case TokenType::DEF:
            case TokenType::IF:
            case TokenType::WHILE:
            case TokenType::FOR:
            case TokenType::RETURN:
            case TokenType::IMPORT:
                return;
            default:
                break;
        }
        
        advance();
    }
}

std::unique_ptr<Expression> Parser::expression() {
    return assignment();
}

std::unique_ptr<Expression> Parser::assignment() {
    auto expr = logicalOr();
    
    if (match(TokenType::ASSIGN)) {
        Token equals = tokens[current - 1];
        auto value = assignment();
        return std::make_unique<Assignment>(std::move(expr), std::move(value), equals.location);
    }
    
    return expr;
}

std::unique_ptr<Expression> Parser::logicalOr() {
    auto expr = logicalAnd();
    
    while (match(TokenType::OR)) {
        Token operator_ = tokens[current - 1];
        auto right = logicalAnd();
        expr = std::make_unique<BinaryExpression>(std::move(expr), BinaryOp::OR, 
                                                 std::move(right), operator_.location);
    }
    
    return expr;
}

std::unique_ptr<Expression> Parser::logicalAnd() {
    auto expr = equality();
    
    while (match(TokenType::AND)) {
        Token operator_ = tokens[current - 1];
        auto right = equality();
        expr = std::make_unique<BinaryExpression>(std::move(expr), BinaryOp::AND, 
                                                 std::move(right), operator_.location);
    }
    
    return expr;
}

std::unique_ptr<Expression> Parser::equality() {
    auto expr = comparison();
    
    while (match({TokenType::NOT_EQUAL, TokenType::EQUAL})) {
        Token operator_ = tokens[current - 1];
        auto right = comparison();
        BinaryOp op = tokenToBinaryOp(operator_.type);
        expr = std::make_unique<BinaryExpression>(std::move(expr), op, 
                                                 std::move(right), operator_.location);
    }
    
    return expr;
}

std::unique_ptr<Expression> Parser::comparison() {
    auto expr = term();
    
    while (match({TokenType::GREATER_THAN, TokenType::GREATER_EQUAL, 
                  TokenType::LESS_THAN, TokenType::LESS_EQUAL})) {
        Token operator_ = tokens[current - 1];
        auto right = term();
        BinaryOp op = tokenToBinaryOp(operator_.type);
        expr = std::make_unique<BinaryExpression>(std::move(expr), op, 
                                                 std::move(right), operator_.location);
    }
    
    return expr;
}

std::unique_ptr<Expression> Parser::term() {
    auto expr = factor();
    
    while (match({TokenType::MINUS, TokenType::PLUS})) {
        Token operator_ = tokens[current - 1];
        auto right = factor();
        BinaryOp op = tokenToBinaryOp(operator_.type);
        expr = std::make_unique<BinaryExpression>(std::move(expr), op, 
                                                 std::move(right), operator_.location);
    }
    
    return expr;
}

std::unique_ptr<Expression> Parser::factor() {
    auto expr = power();
    
    while (match({TokenType::DIVIDE, TokenType::MULTIPLY, TokenType::MODULO})) {
        Token operator_ = tokens[current - 1];
        auto right = power();
        BinaryOp op = tokenToBinaryOp(operator_.type);
        expr = std::make_unique<BinaryExpression>(std::move(expr), op, 
                                                 std::move(right), operator_.location);
    }
    
    return expr;
}

std::unique_ptr<Expression> Parser::power() {
    auto expr = unary();
    
    if (match(TokenType::POWER)) {
        Token operator_ = tokens[current - 1];
        auto right = power(); // Right associative
        expr = std::make_unique<BinaryExpression>(std::move(expr), BinaryOp::POWER, 
                                                 std::move(right), operator_.location);
    }
    
    return expr;
}

std::unique_ptr<Expression> Parser::unary() {
    if (match({TokenType::NOT, TokenType::MINUS})) {
        Token operator_ = tokens[current - 1];
        auto right = unary();
        UnaryOp op = tokenToUnaryOp(operator_.type);
        return std::make_unique<UnaryExpression>(op, std::move(right), operator_.location);
    }
    
    return postfix();
}

std::unique_ptr<Expression> Parser::postfix() {
    auto expr = primary();
    
    while (true) {
        if (match(TokenType::LEFT_PAREN)) {
            // Function call
            auto arguments = parseArguments();
            consume(TokenType::RIGHT_PAREN, "Expected ')' after arguments");
            expr = std::make_unique<FunctionCall>(std::move(expr), std::move(arguments), 
                                                 tokens[current - 1].location);
        } else if (match(TokenType::DOT)) {
            // Attribute access
            Token name = consume(TokenType::IDENTIFIER, "Expected property name after '.'");
            expr = std::make_unique<AttributeAccess>(std::move(expr), name.value, name.location);
        } else if (match(TokenType::LEFT_BRACKET)) {
            // Index access
            auto index = expression();
            consume(TokenType::RIGHT_BRACKET, "Expected ']' after index");
            expr = std::make_unique<IndexAccess>(std::move(expr), std::move(index), 
                                               tokens[current - 1].location);
        } else {
            break;
        }
    }
    
    return expr;
}

std::unique_ptr<Expression> Parser::primary() {
    if (match(TokenType::TRUE)) {
        return std::make_unique<BooleanLiteral>(true, tokens[current - 1].location);
    }
    
    if (match(TokenType::FALSE)) {
        return std::make_unique<BooleanLiteral>(false, tokens[current - 1].location);
    }
    
    if (match(TokenType::NONE)) {
        return std::make_unique<NoneLiteral>(tokens[current - 1].location);
    }
    
    if (match(TokenType::INTEGER)) {
        Token token = tokens[current - 1];
        long long value = std::stoll(token.value);
        return std::make_unique<IntegerLiteral>(value, token.location);
    }
    
    if (match(TokenType::FLOAT)) {
        Token token = tokens[current - 1];
        double value = std::stod(token.value);
        return std::make_unique<FloatLiteral>(value, token.location);
    }
    
    if (match(TokenType::STRING)) {
        Token token = tokens[current - 1];
        return std::make_unique<StringLiteral>(token.value, token.location);
    }
    
    if (match(TokenType::IDENTIFIER)) {
        Token token = tokens[current - 1];
        return std::make_unique<Identifier>(token.value, token.location);
    }
    
    if (match(TokenType::LEFT_PAREN)) {
        auto expr = expression();
        consume(TokenType::RIGHT_PAREN, "Expected ')' after expression");
        return expr;
    }
    
    error("Expected expression");
    return nullptr; // This won't be reached due to exception
}

std::unique_ptr<Statement> Parser::statement() {
    try {
        skipNewlines();
        
        if (match(TokenType::IF)) return ifStatement();
        if (match(TokenType::WHILE)) return whileStatement();
        if (match(TokenType::FOR)) return forStatement();
        if (match(TokenType::RETURN)) return returnStatement();
        if (match(TokenType::BREAK)) return breakStatement();
        if (match(TokenType::CONTINUE)) return continueStatement();
        if (match(TokenType::PASS)) return passStatement();
        if (match(TokenType::DEF)) return functionDefinition();
        if (match(TokenType::CLASS)) return classDefinition();
        if (match(TokenType::IMPORT)) return importStatement();
        
        return expressionStatement();
    } catch (ParseError& e) {
        synchronize();
        throw;
    }
}

std::unique_ptr<Statement> Parser::expressionStatement() {
    auto expr = expression();
    skipNewlines();
    return std::make_unique<ExpressionStatement>(std::move(expr), expr->location);
}

std::unique_ptr<Statement> Parser::ifStatement() {
    Token ifToken = tokens[current - 1];
    auto condition = expression();
    consume(TokenType::COLON, "Expected ':' after if condition");
    
    auto thenBranch = block();
    std::unique_ptr<Statement> elseBranch = nullptr;
    
    skipNewlines();
    if (match(TokenType::ELSE)) {
        consume(TokenType::COLON, "Expected ':' after else");
        elseBranch = block();
    }
    
    return std::make_unique<IfStatement>(std::move(condition), std::move(thenBranch), 
                                        std::move(elseBranch), ifToken.location);
}

std::unique_ptr<Statement> Parser::whileStatement() {
    Token whileToken = tokens[current - 1];
    auto condition = expression();
    consume(TokenType::COLON, "Expected ':' after while condition");
    
    auto body = block();
    return std::make_unique<WhileStatement>(std::move(condition), std::move(body), 
                                           whileToken.location);
}

std::unique_ptr<Statement> Parser::forStatement() {
    Token forToken = tokens[current - 1];
    Token variable = consume(TokenType::IDENTIFIER, "Expected variable name in for loop");
    consume(TokenType::IN, "Expected 'in' after for variable");
    auto iterable = expression();
    consume(TokenType::COLON, "Expected ':' after for clause");
    
    auto body = block();
    return std::make_unique<ForStatement>(variable.value, std::move(iterable), 
                                         std::move(body), forToken.location);
}

std::unique_ptr<Statement> Parser::returnStatement() {
    Token returnToken = tokens[current - 1];
    std::unique_ptr<Expression> value = nullptr;
    
    if (!check(TokenType::NEWLINE) && !isAtEnd()) {
        value = expression();
    }
    
    skipNewlines();
    return std::make_unique<ReturnStatement>(std::move(value), returnToken.location);
}

std::unique_ptr<Statement> Parser::breakStatement() {
    Token breakToken = tokens[current - 1];
    skipNewlines();
    return std::make_unique<BreakStatement>(breakToken.location);
}

std::unique_ptr<Statement> Parser::continueStatement() {
    Token continueToken = tokens[current - 1];
    skipNewlines();
    return std::make_unique<ContinueStatement>(continueToken.location);
}

std::unique_ptr<Statement> Parser::passStatement() {
    Token passToken = tokens[current - 1];
    skipNewlines();
    return std::make_unique<PassStatement>(passToken.location);
}

std::unique_ptr<Statement> Parser::functionDefinition() {
    Token defToken = tokens[current - 1];
    Token name = consume(TokenType::IDENTIFIER, "Expected function name");
    
    consume(TokenType::LEFT_PAREN, "Expected '(' after function name");
    auto parameters = parseParameters();
    consume(TokenType::RIGHT_PAREN, "Expected ')' after parameters");
    consume(TokenType::COLON, "Expected ':' after function signature");
    
    auto body = block();
    return std::make_unique<FunctionDefinition>(name.value, std::move(parameters), 
                                               std::move(body), defToken.location);
}

std::unique_ptr<Statement> Parser::classDefinition() {
    Token classToken = tokens[current - 1];
    Token name = consume(TokenType::IDENTIFIER, "Expected class name");
    
    std::vector<std::unique_ptr<Expression>> bases;
    if (match(TokenType::LEFT_PAREN)) {
        if (!check(TokenType::RIGHT_PAREN)) {
            do {
                bases.push_back(expression());
            } while (match(TokenType::COMMA));
        }
        consume(TokenType::RIGHT_PAREN, "Expected ')' after base classes");
    }
    
    consume(TokenType::COLON, "Expected ':' after class declaration");
    auto body = block();
    
    return std::make_unique<ClassDefinition>(name.value, std::move(bases), 
                                            std::move(body), classToken.location);
}

std::unique_ptr<Statement> Parser::importStatement() {
    Token importToken = tokens[current - 1];
    std::vector<std::string> modules;
    std::vector<std::string> aliases;
    
    do {
        Token module = consume(TokenType::IDENTIFIER, "Expected module name");
        modules.push_back(module.value);
        
        if (match(TokenType::AS)) {
            Token alias = consume(TokenType::IDENTIFIER, "Expected alias name");
            aliases.push_back(alias.value);
        } else {
            aliases.push_back("");
        }
    } while (match(TokenType::COMMA));
    
    skipNewlines();
    return std::make_unique<ImportStatement>(std::move(modules), std::move(aliases), 
                                            importToken.location);
}

std::unique_ptr<Statement> Parser::block() {
    std::vector<std::unique_ptr<Statement>> statements;
    SourceLocation blockLocation = peek().location;
    
    skipNewlines();
    consume(TokenType::INDENT, "Expected indented block");
    
    while (!check(TokenType::DEDENT) && !isAtEnd()) {
        statements.push_back(statement());
    }
    
    consume(TokenType::DEDENT, "Expected dedent after block");
    return std::make_unique<Block>(std::move(statements), blockLocation);
}

std::vector<Parameter> Parser::parseParameters() {
    std::vector<Parameter> parameters;
    
    if (!check(TokenType::RIGHT_PAREN)) {
        do {
            Token name = consume(TokenType::IDENTIFIER, "Expected parameter name");
            std::unique_ptr<Expression> defaultValue = nullptr;
            
            if (match(TokenType::ASSIGN)) {
                defaultValue = expression();
            }
            
            parameters.emplace_back(name.value, std::move(defaultValue), name.location);
        } while (match(TokenType::COMMA));
    }
    
    return parameters;
}

std::vector<std::unique_ptr<Expression>> Parser::parseArguments() {
    std::vector<std::unique_ptr<Expression>> arguments;
    
    if (!check(TokenType::RIGHT_PAREN)) {
        do {
            arguments.push_back(expression());
        } while (match(TokenType::COMMA));
    }
    
    return arguments;
}

BinaryOp Parser::tokenToBinaryOp(TokenType type) {
    switch (type) {
        case TokenType::PLUS: return BinaryOp::ADD;
        case TokenType::MINUS: return BinaryOp::SUBTRACT;
        case TokenType::MULTIPLY: return BinaryOp::MULTIPLY;
        case TokenType::DIVIDE: return BinaryOp::DIVIDE;
        case TokenType::MODULO: return BinaryOp::MODULO;
        case TokenType::POWER: return BinaryOp::POWER;
        case TokenType::EQUAL: return BinaryOp::EQUAL;
        case TokenType::NOT_EQUAL: return BinaryOp::NOT_EQUAL;
        case TokenType::LESS_THAN: return BinaryOp::LESS_THAN;
        case TokenType::LESS_EQUAL: return BinaryOp::LESS_EQUAL;
        case TokenType::GREATER_THAN: return BinaryOp::GREATER_THAN;
        case TokenType::GREATER_EQUAL: return BinaryOp::GREATER_EQUAL;
        case TokenType::AND: return BinaryOp::AND;
        case TokenType::OR: return BinaryOp::OR;
        default:
            error("Invalid binary operator");
            return BinaryOp::ADD; // This won't be reached due to exception
    }
}

UnaryOp Parser::tokenToUnaryOp(TokenType type) {
    switch (type) {
        case TokenType::MINUS: return UnaryOp::MINUS;
        case TokenType::NOT: return UnaryOp::NOT;
        default:
            error("Invalid unary operator");
            return UnaryOp::MINUS; // This won't be reached due to exception
    }
}

std::unique_ptr<Program> Parser::parse() {
    std::vector<std::unique_ptr<Statement>> statements;
    SourceLocation programLocation(1, 1, "");
    
    skipNewlines();
    
    while (!isAtEnd()) {
        statements.push_back(statement());
        skipNewlines();
    }
    
    return std::make_unique<Program>(std::move(statements), programLocation);
}

std::string Parser::getErrorContext() const {
    if (current >= tokens.size()) {
        return "At end of file";
    }
    
    Token token = tokens[current];
    return "At token: " + token.toString();
}

} // namespace meadows
