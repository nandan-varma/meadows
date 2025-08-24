#include "Parser.h"
#include <iostream>

namespace meadows {

Parser::Parser(std::vector<Token> tokens, ErrorReporter &errorReporter)
    : tokens(std::move(tokens)), current(0), errorReporter(errorReporter) {}

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
  if (isAtEnd())
    return false;
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

Token Parser::consume(TokenType type, const std::string &message) {
  if (check(type)) {
    return advance();
  }
  reportError(message);

  // Escape the current scope on error
  escapeScope();

  // Return current token as fallback
  return peek();
}

void Parser::skipNewlines() {
  while (match(TokenType::NEWLINE)) {
    // Skip newlines
  }
}

void Parser::reportError(const std::string &message) {
  errorReporter.error(message, peek().location);
}

void Parser::reportError(const std::string &message,
                         const SourceLocation &location) {
  errorReporter.error(message, location);
}

void Parser::escapeScope() {
  // Escape to the end of current scope based on context
  int parenDepth = 0;
  int bracketDepth = 0;
  int braceDepth = 0;

  while (!isAtEnd()) {
    Token token = peek();

    switch (token.type) {
    case TokenType::LEFT_PAREN:
      parenDepth++;
      break;
    case TokenType::RIGHT_PAREN:
      parenDepth--;
      if (parenDepth < 0)
        return; // Escaped paren scope
      break;
    case TokenType::LEFT_BRACKET:
      bracketDepth++;
      break;
    case TokenType::RIGHT_BRACKET:
      bracketDepth--;
      if (bracketDepth < 0)
        return; // Escaped bracket scope
      break;
    case TokenType::LEFT_BRACE:
      braceDepth++;
      break;
    case TokenType::RIGHT_BRACE:
      braceDepth--;
      if (braceDepth < 0)
        return; // Escaped brace scope
      break;
    case TokenType::NEWLINE:
      // If we're at top level (no nesting), newline ends scope
      if (parenDepth == 0 && bracketDepth == 0 && braceDepth == 0) {
        advance(); // consume the newline
        return;
      }
      break;
    case TokenType::DEDENT:
      // Dedent always ends current scope
      return;
    case TokenType::DEF:
    case TokenType::CLASS:
    case TokenType::IF:
    case TokenType::WHILE:
    case TokenType::FOR:
      // If we're at top level, these start new scopes
      if (parenDepth == 0 && bracketDepth == 0 && braceDepth == 0) {
        return;
      }
      break;
    default:
      break;
    }
    advance();
  }
}

void Parser::synchronize() {
  advance();

  while (!isAtEnd()) {
    if (tokens[current - 1].type == TokenType::NEWLINE)
      return;

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

std::unique_ptr<Expression> Parser::expression() { return assignment(); }

std::unique_ptr<Expression> Parser::assignment() {
  auto expr = logicalOr();
  if (!expr) {
    return nullptr;
  }

  if (match(TokenType::ASSIGN)) {
    Token equals = tokens[current - 1];
    auto value = assignment();
    if (!value) {
      return nullptr;
    }
    return std::make_unique<Assignment>(std::move(expr), std::move(value),
                                        equals.location);
  }

  return expr;
}

std::unique_ptr<Expression> Parser::logicalOr() {
  auto expr = logicalAnd();

  while (match(TokenType::OR)) {
    Token operator_ = tokens[current - 1];
    auto right = logicalAnd();
    expr = std::make_unique<BinaryExpression>(
        std::move(expr), BinaryOp::OR, std::move(right), operator_.location);
  }

  return expr;
}

std::unique_ptr<Expression> Parser::logicalAnd() {
  auto expr = equality();

  while (match(TokenType::AND)) {
    Token operator_ = tokens[current - 1];
    auto right = equality();
    expr = std::make_unique<BinaryExpression>(
        std::move(expr), BinaryOp::AND, std::move(right), operator_.location);
  }

  return expr;
}

std::unique_ptr<Expression> Parser::equality() {
  auto expr = comparison();

  while (match({TokenType::NOT_EQUAL, TokenType::EQUAL})) {
    Token operator_ = tokens[current - 1];
    auto right = comparison();
    BinaryOp op = tokenToBinaryOp(operator_.type);
    expr = std::make_unique<BinaryExpression>(
        std::move(expr), op, std::move(right), operator_.location);
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
    expr = std::make_unique<BinaryExpression>(
        std::move(expr), op, std::move(right), operator_.location);
  }

  return expr;
}

std::unique_ptr<Expression> Parser::term() {
  auto expr = factor();

  while (match({TokenType::MINUS, TokenType::PLUS})) {
    Token operator_ = tokens[current - 1];
    auto right = factor();
    BinaryOp op = tokenToBinaryOp(operator_.type);
    expr = std::make_unique<BinaryExpression>(
        std::move(expr), op, std::move(right), operator_.location);
  }

  return expr;
}

std::unique_ptr<Expression> Parser::factor() {
  auto expr = power();

  while (match({TokenType::DIVIDE, TokenType::MULTIPLY, TokenType::MODULO})) {
    Token operator_ = tokens[current - 1];
    auto right = power();
    BinaryOp op = tokenToBinaryOp(operator_.type);
    expr = std::make_unique<BinaryExpression>(
        std::move(expr), op, std::move(right), operator_.location);
  }

  return expr;
}

std::unique_ptr<Expression> Parser::power() {
  auto expr = unary();

  if (match(TokenType::POWER)) {
    Token operator_ = tokens[current - 1];
    auto right = power(); // Right associative
    expr = std::make_unique<BinaryExpression>(
        std::move(expr), BinaryOp::POWER, std::move(right), operator_.location);
  }

  return expr;
}

std::unique_ptr<Expression> Parser::unary() {
  if (match({TokenType::NOT, TokenType::MINUS})) {
    Token operator_ = tokens[current - 1];
    auto right = unary();
    UnaryOp op = tokenToUnaryOp(operator_.type);
    return std::make_unique<UnaryExpression>(op, std::move(right),
                                             operator_.location);
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
      expr = std::make_unique<FunctionCall>(
          std::move(expr), std::move(arguments), tokens[current - 1].location);
    } else if (match(TokenType::DOT)) {
      // Attribute access
      Token name =
          consume(TokenType::IDENTIFIER, "Expected property name after '.'");
      expr = std::make_unique<AttributeAccess>(std::move(expr), name.value,
                                               name.location);
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
    return std::make_unique<BooleanLiteral>(false,
                                            tokens[current - 1].location);
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
    if (!match(TokenType::RIGHT_PAREN)) {
      reportError("Expected ')' after expression");
      // Try to recover and continue with the expression we have
    }
    return expr;
  }

  if (match(TokenType::LEFT_BRACKET)) {
    // List literal
    SourceLocation location = tokens[current - 1].location;
    std::vector<std::unique_ptr<Expression>> elements;

    if (!check(TokenType::RIGHT_BRACKET)) {
      do {
        auto element = expression();
        if (element) {
          elements.push_back(std::move(element));
        }
      } while (match(TokenType::COMMA));
    }

    if (!match(TokenType::RIGHT_BRACKET)) {
      reportError("Expected ']' after list elements");
    }
    return std::make_unique<ListLiteral>(std::move(elements), location);
  }

  reportError("Expected expression");
  return nullptr;
}

std::unique_ptr<Statement> Parser::statement() {
  skipNewlines();

  if (match(TokenType::IF))
    return ifStatement();
  if (match(TokenType::WHILE))
    return whileStatement();
  if (match(TokenType::FOR))
    return forStatement();
  if (match(TokenType::RETURN))
    return returnStatement();
  if (match(TokenType::BREAK))
    return breakStatement();
  if (match(TokenType::CONTINUE))
    return continueStatement();
  if (match(TokenType::PASS))
    return passStatement();
  if (match(TokenType::DEF))
    return functionDefinition();
  if (match(TokenType::CLASS))
    return classDefinition();
  if (match(TokenType::IMPORT))
    return importStatement();

  return expressionStatement();
}

std::unique_ptr<Statement> Parser::expressionStatement() {
  auto expr = expression();
  if (!expr) {
    return nullptr;
  }
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

  // Handle elif chains iteratively
  while (match(TokenType::ELIF)) {
    Token elifToken = tokens[current - 1];
    auto elifCondition = expression();
    consume(TokenType::COLON, "Expected ':' after elif condition");

    auto elifBranch = block();

    // Create the elif as a nested IfStatement
    auto elifStatement = std::make_unique<IfStatement>(
        std::move(elifCondition), std::move(elifBranch), nullptr,
        elifToken.location);

    if (elseBranch == nullptr) {
      elseBranch = std::move(elifStatement);
    } else {
      // Find the deepest else branch and attach this elif there
      IfStatement *current_if = static_cast<IfStatement *>(elseBranch.get());
      while (current_if->elseBranch != nullptr &&
             dynamic_cast<IfStatement *>(current_if->elseBranch.get()) !=
                 nullptr) {
        current_if = static_cast<IfStatement *>(current_if->elseBranch.get());
      }
      current_if->elseBranch = std::move(elifStatement);
    }

    skipNewlines();
  }

  // Handle final else
  if (match(TokenType::ELSE)) {
    consume(TokenType::COLON, "Expected ':' after else");
    auto finalElse = block();

    if (elseBranch == nullptr) {
      elseBranch = std::move(finalElse);
    } else {
      // Find the deepest else branch and attach the final else there
      IfStatement *current_if = static_cast<IfStatement *>(elseBranch.get());
      while (current_if->elseBranch != nullptr &&
             dynamic_cast<IfStatement *>(current_if->elseBranch.get()) !=
                 nullptr) {
        current_if = static_cast<IfStatement *>(current_if->elseBranch.get());
      }
      current_if->elseBranch = std::move(finalElse);
    }
  }

  return std::make_unique<IfStatement>(std::move(condition),
                                       std::move(thenBranch),
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
  Token variable =
      consume(TokenType::IDENTIFIER, "Expected variable name in for loop");
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
  return std::make_unique<ReturnStatement>(std::move(value),
                                           returnToken.location);
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

  // Try to parse function name
  if (!check(TokenType::IDENTIFIER)) {
    reportError("Expected function name after 'def'");
    escapeScope();
    return nullptr;
  }
  Token name = advance();

  // Try to parse opening parenthesis
  if (!match(TokenType::LEFT_PAREN)) {
    reportError("Expected '(' after function name");
    escapeScope();
    return nullptr;
  }

  // Parse parameters with error handling
  std::vector<Parameter> parameters;
  try {
    parameters = parseParameters();
  } catch (...) {
    reportError("Error parsing function parameters");
    escapeScope();
    return nullptr;
  }

  // Try to parse closing parenthesis
  if (!match(TokenType::RIGHT_PAREN)) {
    reportError("Expected ')' after parameters");
    escapeScope();
    return nullptr;
  }

  // Try to parse colon
  if (!match(TokenType::COLON)) {
    reportError("Expected ':' after function signature");
    escapeScope();
    return nullptr;
  }

  // Parse function body
  auto body = block();
  if (!body) {
    return nullptr;
  }

  return std::make_unique<FunctionDefinition>(
      name.value, std::move(parameters), std::move(body), defToken.location);
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

  return std::make_unique<ClassDefinition>(
      name.value, std::move(bases), std::move(body), classToken.location);
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
  return std::make_unique<ImportStatement>(
      std::move(modules), std::move(aliases), importToken.location);
}

std::unique_ptr<Statement> Parser::block() {
  std::vector<std::unique_ptr<Statement>> statements;
  SourceLocation blockLocation = peek().location;

  skipNewlines();

  if (!match(TokenType::INDENT)) {
    reportError("Expected indented block");
    // Try to continue parsing statements without proper indentation
    // This allows us to recover from missing indent errors
    while (!isAtEnd() && !check(TokenType::DEDENT) &&
           peek().type != TokenType::DEF && peek().type != TokenType::CLASS &&
           peek().type != TokenType::IF && peek().type != TokenType::WHILE &&
           peek().type != TokenType::FOR) {
      auto stmt = statement();
      if (stmt) {
        statements.push_back(std::move(stmt));
      } else {
        // Skip this statement and try the next one
        advance();
      }
      skipNewlines();
    }
    return std::make_unique<Block>(std::move(statements), blockLocation);
  }

  while (!check(TokenType::DEDENT) && !isAtEnd()) {
    auto stmt = statement();
    if (stmt) {
      statements.push_back(std::move(stmt));
    }
  }

  if (!match(TokenType::DEDENT)) {
    reportError("Expected dedent after block");
  }

  return std::make_unique<Block>(std::move(statements), blockLocation);
}

std::vector<Parameter> Parser::parseParameters() {
  std::vector<Parameter> parameters;

  if (!check(TokenType::RIGHT_PAREN)) {
    do {
      if (!check(TokenType::IDENTIFIER)) {
        reportError("Expected parameter name");
        // Escape the parameter scope and continue
        escapeScope();
        break; // Give up on remaining parameters
      }

      Token name = advance();
      std::unique_ptr<Expression> defaultValue = nullptr;

      if (match(TokenType::ASSIGN)) {
        auto defaultExpr = expression();
        if (defaultExpr) {
          defaultValue = std::move(defaultExpr);
        }
      }

      parameters.emplace_back(name.value, std::move(defaultValue),
                              name.location);
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
  case TokenType::PLUS:
    return BinaryOp::ADD;
  case TokenType::MINUS:
    return BinaryOp::SUBTRACT;
  case TokenType::MULTIPLY:
    return BinaryOp::MULTIPLY;
  case TokenType::DIVIDE:
    return BinaryOp::DIVIDE;
  case TokenType::MODULO:
    return BinaryOp::MODULO;
  case TokenType::POWER:
    return BinaryOp::POWER;
  case TokenType::EQUAL:
    return BinaryOp::EQUAL;
  case TokenType::NOT_EQUAL:
    return BinaryOp::NOT_EQUAL;
  case TokenType::LESS_THAN:
    return BinaryOp::LESS_THAN;
  case TokenType::LESS_EQUAL:
    return BinaryOp::LESS_EQUAL;
  case TokenType::GREATER_THAN:
    return BinaryOp::GREATER_THAN;
  case TokenType::GREATER_EQUAL:
    return BinaryOp::GREATER_EQUAL;
  case TokenType::AND:
    return BinaryOp::AND;
  case TokenType::OR:
    return BinaryOp::OR;
  default:
    reportError("Invalid binary operator");
    return BinaryOp::ADD; // Fallback
  }
}

UnaryOp Parser::tokenToUnaryOp(TokenType type) {
  switch (type) {
  case TokenType::MINUS:
    return UnaryOp::MINUS;
  case TokenType::NOT:
    return UnaryOp::NOT;
  default:
    reportError("Invalid unary operator");
    return UnaryOp::MINUS; // Fallback
  }
}

std::unique_ptr<Program> Parser::parse() {
  std::vector<std::unique_ptr<Statement>> statements;
  SourceLocation programLocation(1, 1, "");

  skipNewlines();

  int maxStatements = 1000; // Prevent infinite loops
  int statementCount = 0;

  while (!isAtEnd() && statementCount < maxStatements) {
    size_t currentPos = current;

    try {
      auto stmt = statement();
      if (stmt) {
        statements.push_back(std::move(stmt));
      }
    } catch (const std::exception &e) {
      // Log the error and attempt to recover
      reportError("Unexpected error: " + std::string(e.what()));
      synchronize();
    }

    // If we didn't advance, force advance to prevent infinite loop
    if (current == currentPos) {
      advance();
    }

    skipNewlines();
    statementCount++;
  }

  if (statementCount >= maxStatements) {
    reportError("Maximum statement limit reached - possible infinite loop");
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
