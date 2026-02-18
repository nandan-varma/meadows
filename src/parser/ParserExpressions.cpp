#include "../utils/MemoryUtils.h"
#include "Parser.h"
#include <stdexcept>

constexpr int MAX_PARSE_DEPTH = 100;

std::unique_ptr<Expr> Parser::parseAssignment(int depth) {
  if (depth > MAX_PARSE_DEPTH) {
    error(meadows::ErrorCode::PARSE_UNEXPECTED_TOKEN,
          "Expression nesting too deep");
    return nullptr;
  }
  auto expr = parseOr(depth + 1);
  if (match(TokenType::EQUAL)) {
    auto varExpr = dynamic_cast<VarExpr *>(expr.get());
    if (!varExpr) {
      meadows::SourceLocation loc("", peek().line, peek().column);
      throw meadows::ParseException(
          meadows::ErrorCode::PARSE_INVALID_ASSIGNMENT_TARGET,
          "Invalid assignment target", loc);
    }
    auto value = parseAssignment(depth + 1);
    return std::make_unique<AssignExpr>(varExpr->name, std::move(value));
  }
  return expr;
}

std::unique_ptr<Expr> Parser::parseOr(int depth) {
  auto expr = parseAnd(depth);
  while (match(TokenType::OR)) {
    Token op = previous();
    auto right = parseAnd(depth);
    expr = std::make_unique<LogicalExpr>(std::move(expr), LogicalOperator::OR,
                                         std::move(right));
  }
  while (match(TokenType::PIPE)) {
    Token op = previous();
    auto right = parseAnd(depth);
    expr = std::make_unique<BinaryExpr>(std::move(expr), "|", std::move(right));
  }
  return expr;
}

std::unique_ptr<Expr> Parser::parseAnd(int depth) {
  auto expr = parseEquality(depth);
  while (match(TokenType::AND)) {
    Token op = previous();
    auto right = parseEquality(depth);
    expr = std::make_unique<LogicalExpr>(std::move(expr), LogicalOperator::AND,
                                         std::move(right));
  }
  return expr;
}

std::unique_ptr<Expr> Parser::parseEquality(int depth) {
  auto expr = parseComparison(depth);
  while (match(TokenType::EQUAL_EQUAL) || match(TokenType::BANG_EQUAL)) {
    std::string op = previous().value;
    auto right = parseComparison(depth);
    expr = std::make_unique<BinaryExpr>(std::move(expr), op, std::move(right));
  }
  return expr;
}

std::unique_ptr<Expr> Parser::parseComparison(int depth) {
  auto expr = parseTerm(depth);
  while (match(TokenType::GREATER) || match(TokenType::GREATER_EQUAL) ||
         match(TokenType::LESS) || match(TokenType::LESS_EQUAL)) {
    std::string op = previous().value;
    auto right = parseTerm(depth);
    expr = std::make_unique<BinaryExpr>(std::move(expr), op, std::move(right));
  }
  return expr;
}

std::unique_ptr<Expr> Parser::parseTerm(int depth) {
  auto expr = parseFactor(depth);
  while (match(TokenType::PLUS) || match(TokenType::MINUS)) {
    std::string op = previous().value;
    auto right = parseFactor(depth);
    expr = std::make_unique<BinaryExpr>(std::move(expr), op, std::move(right));
  }
  return expr;
}

std::unique_ptr<Expr> Parser::parseFactor(int depth) {
  auto expr = parseUnary(depth);
  while (match(TokenType::STAR) || match(TokenType::SLASH)) {
    std::string op = previous().value;
    auto right = parseUnary(depth);
    expr = std::make_unique<BinaryExpr>(std::move(expr), op, std::move(right));
  }
  return expr;
}

std::unique_ptr<Expr> Parser::parseUnary(int depth) {
  if (match(TokenType::MINUS)) {
    std::string op = previous().value;
    auto operand = parseUnary(depth);
    return std::make_unique<UnaryExpr>(op, std::move(operand));
  }
  if (match(TokenType::BANG)) {
    std::string op = previous().value;
    auto operand = parseUnary(depth);
    return std::make_unique<UnaryExpr>(op, std::move(operand));
  }
  return parseCall(depth);
}

std::unique_ptr<Expr> Parser::parseCall(int depth) {
  auto expr = parseIndex(depth);
  if (match(TokenType::LEFT_PAREN)) {
    auto args = parseArgs();
    consume(TokenType::RIGHT_PAREN, "Expect ')' after arguments");
    expr = std::make_unique<CallExpr>(std::move(expr), std::move(args));
  }
  while (match(TokenType::QUESTION)) {
    expr = std::make_unique<TryExpr>(std::move(expr));
  }
  return expr;
}

std::unique_ptr<Expr> Parser::parseIndex(int depth) {
  auto expr = parseFieldAccess(depth);
  while (match(TokenType::LEFT_BRACKET)) {
    auto index = parseExpr();
    consume(TokenType::RIGHT_BRACKET, "Expect ']' after index");
    expr = std::make_unique<IndexExpr>(std::move(expr), std::move(index));
  }
  return expr;
}

std::unique_ptr<Expr> Parser::parseFieldAccess(int depth) {
  auto expr = parsePrimary(depth);
  while (match(TokenType::DOT)) {
    Token fieldName =
        consume(TokenType::IDENTIFIER, "Expect field name after '.'");
    expr = std::make_unique<FieldAccessExpr>(std::move(expr), fieldName.value);
  }
  return expr;
}

std::unique_ptr<Expr> Parser::parsePrimary([[maybe_unused]] int depth) {
  if (match(TokenType::STRING)) {
    return std::make_unique<LiteralExpr>(previous().value);
  }
  if (match(TokenType::NUMBER)) {
    return std::make_unique<LiteralExpr>(previous().value);
  }
  if (match(TokenType::TRUE)) {
    return std::make_unique<LiteralExpr>("1");
  }
  if (match(TokenType::FALSE)) {
    return std::make_unique<LiteralExpr>("0");
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
      return std::make_unique<EnumVariantExpr>(id.value, variantName.value,
                                               std::move(args));
    }
    return std::make_unique<VarExpr>(id.value);
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
        const Token &key = consume(TokenType::IDENTIFIER, "Expect key");
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
  meadows::SourceLocation loc("", peek().line, peek().column);
  throw meadows::ParseException(meadows::ErrorCode::PARSE_EXPECTED_EXPRESSION,
                                "Expect expression", loc);
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