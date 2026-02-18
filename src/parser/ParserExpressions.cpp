#include "Parser.h"

#include "../utils/MemoryUtils.h"

constexpr int MAX_PARSE_DEPTH = 100;

std::unique_ptr<Expr> Parser::parseAssignment(int depth) {
  if (depth > MAX_PARSE_DEPTH) {
    error("Expression nesting too deep");
    return nullptr;
  }
  auto expr = parseOr(depth + 1);
  if (match(TokenType::EQUAL)) {
    auto varExpr = dynamic_cast<VarExpr *>(expr.get());
    if (!varExpr) {
      error("Invalid assignment target");
      return nullptr;
    }
    auto value = parseAssignment(depth + 1);
    return factory_.createAssignExpr(varExpr->name, std::move(value),
                                     currentLocation());
  }
  return expr;
}

std::unique_ptr<Expr> Parser::parseOr(int depth) {
  auto expr = parseAnd(depth);
  while (match(TokenType::OR)) {
    Token op = previous();
    auto right = parseAnd(depth);
    expr = factory_.createLogicalExpr(std::move(expr), LogicalOperator::OR,
                                      std::move(right), currentLocation());
  }
  while (match(TokenType::PIPE)) {
    Token op = previous();
    auto right = parseAnd(depth);
    expr = factory_.createBinaryExpr(std::move(expr), "|", std::move(right),
                                     currentLocation());
  }
  return expr;
}

std::unique_ptr<Expr> Parser::parseAnd(int depth) {
  auto expr = parseEquality(depth);
  while (match(TokenType::AND)) {
    Token op = previous();
    auto right = parseEquality(depth);
    expr = factory_.createLogicalExpr(std::move(expr), LogicalOperator::AND,
                                      std::move(right), currentLocation());
  }
  return expr;
}

std::unique_ptr<Expr> Parser::parseEquality(int depth) {
  auto expr = parseComparison(depth);
  while (match(TokenType::EQUAL_EQUAL) || match(TokenType::BANG_EQUAL)) {
    std::string op = previous().value;
    auto right = parseComparison(depth);
    expr = factory_.createBinaryExpr(std::move(expr), op, std::move(right),
                                     currentLocation());
  }
  return expr;
}

std::unique_ptr<Expr> Parser::parseComparison(int depth) {
  auto expr = parseTerm(depth);
  while (match(TokenType::GREATER) || match(TokenType::GREATER_EQUAL) ||
         match(TokenType::LESS) || match(TokenType::LESS_EQUAL)) {
    std::string op = previous().value;
    auto right = parseTerm(depth);
    expr = factory_.createBinaryExpr(std::move(expr), op, std::move(right),
                                     currentLocation());
  }
  return expr;
}

std::unique_ptr<Expr> Parser::parseTerm(int depth) {
  auto expr = parseFactor(depth);
  while (match(TokenType::PLUS) || match(TokenType::MINUS)) {
    std::string op = previous().value;
    auto right = parseFactor(depth);
    expr = factory_.createBinaryExpr(std::move(expr), op, std::move(right),
                                     currentLocation());
  }
  return expr;
}

std::unique_ptr<Expr> Parser::parseFactor(int depth) {
  auto expr = parseUnary(depth);
  while (match(TokenType::STAR) || match(TokenType::SLASH)) {
    std::string op = previous().value;
    auto right = parseUnary(depth);
    expr = factory_.createBinaryExpr(std::move(expr), op, std::move(right),
                                     currentLocation());
  }
  return expr;
}

std::unique_ptr<Expr> Parser::parseUnary(int depth) {
  if (match(TokenType::MINUS)) {
    std::string op = previous().value;
    auto operand = parseUnary(depth);
    return factory_.createUnaryExpr(op, std::move(operand), currentLocation());
  }
  if (match(TokenType::BANG)) {
    std::string op = previous().value;
    auto operand = parseUnary(depth);
    return factory_.createUnaryExpr(op, std::move(operand), currentLocation());
  }
  return parseCall(depth);
}

std::unique_ptr<Expr> Parser::parseCall(int depth) {
  auto expr = parseIndex(depth);
  if (match(TokenType::LEFT_PAREN)) {
    auto args = parseArgs();
    consume(TokenType::RIGHT_PAREN, "Expect ')' after arguments");
    expr = factory_.createCallExpr(std::move(expr), std::move(args),
                                   currentLocation());
  }
  while (match(TokenType::QUESTION)) {
    expr = factory_.createTryExpr(std::move(expr), currentLocation());
  }
  return expr;
}

std::unique_ptr<Expr> Parser::parseIndex(int depth) {
  auto expr = parseFieldAccess(depth);
  while (match(TokenType::LEFT_BRACKET)) {
    auto index = parseExpr();
    consume(TokenType::RIGHT_BRACKET, "Expect ']' after index");
    expr = factory_.createIndexExpr(std::move(expr), std::move(index),
                                    currentLocation());
  }
  return expr;
}

std::unique_ptr<Expr> Parser::parseFieldAccess(int depth) {
  auto expr = parsePrimary(depth);
  while (match(TokenType::DOT)) {
    Token fieldName =
        consume(TokenType::IDENTIFIER, "Expect field name after '.'");
    expr = factory_.createFieldAccessExpr(std::move(expr), fieldName.value,
                                          currentLocation());
  }
  return expr;
}

std::unique_ptr<Expr> Parser::parsePrimary([[maybe_unused]] int depth) {
  if (match(TokenType::STRING)) {
    return factory_.createLiteralExpr(previous().value, currentLocation());
  }
  if (match(TokenType::NUMBER)) {
    return factory_.createLiteralExpr(previous().value, currentLocation());
  }
  if (match(TokenType::TRUE)) {
    return factory_.createLiteralExpr("1", currentLocation());
  }
  if (match(TokenType::FALSE)) {
    return factory_.createLiteralExpr("0", currentLocation());
  }
  if (match(TokenType::MATCH)) {
    return parseMatchExpr();
  }
  if (match(TokenType::IDENTIFIER)) {
    Token id = previous();
    if (match(TokenType::DOT)) {
      const Token &variantName =
          consume(TokenType::IDENTIFIER, "Expect variant name");
      std::vector<std::unique_ptr<Expr>> args;
      if (match(TokenType::LEFT_PAREN)) {
        while (!check(TokenType::RIGHT_PAREN) && !isAtEnd()) {
          args.push_back(parseExpr());
          if (!check(TokenType::RIGHT_PAREN)) {
            consume(TokenType::COMMA, "Expect ',' between arguments");
          }
        }
        consume(TokenType::RIGHT_PAREN, "Expect ')' after variant arguments");
      }
      return factory_.createEnumVariantExpr(id.value, variantName.value,
                                            std::move(args), currentLocation());
    }
    return factory_.createVarExpr(id.value, currentLocation());
  }
  if (match(TokenType::LEFT_BRACKET)) {
    std::vector<std::unique_ptr<Expr>> elements;
    if (!check(TokenType::RIGHT_BRACKET)) {
      do {
        elements.push_back(parseExpr());
      } while (match(TokenType::COMMA));
    }
    consume(TokenType::RIGHT_BRACKET, "Expect ']' after array elements");
    return factory_.createArrayExpr(std::move(elements), currentLocation());
  }
  if (match(TokenType::LEFT_BRACE)) {
    std::unordered_map<std::string, std::unique_ptr<Expr>> pairs;
    if (!check(TokenType::RIGHT_BRACE)) {
      do {
        const Token &key = consume(TokenType::IDENTIFIER, "Expect key");
        consume(TokenType::COLON, "Expect ':' after key");
        auto value = parseExpr();
        pairs[key.value] = std::move(value);
      } while (match(TokenType::COMMA));
    }
    consume(TokenType::RIGHT_BRACE, "Expect '}' after object");
    return factory_.createObjectExpr(std::move(pairs), currentLocation());
  }
  if (match(TokenType::LEFT_PAREN)) {
    auto expr = parseExpr();
    consume(TokenType::RIGHT_PAREN, "Expect ')' after expression");
    return expr;
  }

  error("Expect expression");
  return nullptr;
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
