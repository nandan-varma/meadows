#include "Parser.h"

constexpr int MAX_PARSE_DEPTH = 100;

std::unique_ptr<Expr> Parser::parseAssignment(int depth) {
  if (depth > MAX_PARSE_DEPTH) {
    // A hard stack-safety limit, not an ordinary recoverable syntax error:
    // throw unconditionally (even in diagnostics/LSP mode) rather than going
    // through error()'s report-and-continue path, which would leave the
    // token stream unconsumed at this position and cascade into a stream of
    // unrelated follow-on diagnostics.
    meadows::SourceLocation loc("", peek().line, peek().column);
    throw meadows::ParseException(meadows::ErrorCode::PARSE_UNEXPECTED_TOKEN,
                                  "Expression nesting too deep", loc);
  }
  auto expr = parseOr(depth + 1);
  if (match(TokenType::EQUAL)) {
    // A valid assignment target is a plain variable (x = ...), an array
    // element (arr[i] = ...), or an object field (obj.field = ...).
    bool validTarget = dynamic_cast<VarExpr *>(expr.get()) != nullptr ||
                       dynamic_cast<IndexExpr *>(expr.get()) != nullptr ||
                       dynamic_cast<FieldAccessExpr *>(expr.get()) != nullptr;
    if (!validTarget) {
      meadows::SourceLocation loc("", peek().line, peek().column);
      throw meadows::ParseException(
          meadows::ErrorCode::PARSE_INVALID_ASSIGNMENT_TARGET,
          "Invalid assignment target", loc);
    }
    auto value = parseAssignment(depth + 1);
    return std::make_unique<AssignExpr>(std::move(expr), std::move(value));
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
  while (match(TokenType::STAR) || match(TokenType::SLASH) || match(TokenType::PERCENT)) {
    std::string op = previous().value;
    auto right = parseUnary(depth);
    expr = std::make_unique<BinaryExpr>(std::move(expr), op, std::move(right));
  }
  return expr;
}

std::unique_ptr<Expr> Parser::parseUnary(int depth) {
  if (depth > MAX_PARSE_DEPTH) {
    meadows::SourceLocation loc("", peek().line, peek().column);
    throw meadows::ParseException(meadows::ErrorCode::PARSE_UNEXPECTED_TOKEN,
                                  "Expression nesting too deep", loc);
  }
  if (match(TokenType::MINUS)) {
    std::string op = previous().value;
    auto operand = parseUnary(depth + 1);
    return std::make_unique<UnaryExpr>(op, std::move(operand));
  }
  if (match(TokenType::BANG)) {
    std::string op = previous().value;
    auto operand = parseUnary(depth + 1);
    return std::make_unique<UnaryExpr>(op, std::move(operand));
  }
  return parseCall(depth);
}

std::unique_ptr<Expr> Parser::parseCall(int depth) {
  auto expr = parseIndex(depth);
  if (match(TokenType::LEFT_PAREN)) {
    auto args = parseArgs(depth + 1);
    consume(TokenType::RIGHT_PAREN, meadows::ErrorCode::PARSE_EXPECTED_RPAREN, "Expect ')' after arguments");
    expr = std::make_unique<CallExpr>(std::move(expr), std::move(args));
  }
  return expr;
}

std::unique_ptr<Expr> Parser::parseIndex(int depth) {
  auto expr = parseFieldAccess(depth);
  while (match(TokenType::LEFT_BRACKET)) {
    auto index = parseExpr(depth + 1);
    consume(TokenType::RIGHT_BRACKET, meadows::ErrorCode::PARSE_UNEXPECTED_TOKEN, "Expect ']' after index");
    expr = std::make_unique<IndexExpr>(std::move(expr), std::move(index));
  }
  return expr;
}

std::unique_ptr<Expr> Parser::parseFieldAccess(int depth) {
  auto expr = parsePrimary(depth);
  while (match(TokenType::DOT)) {
    Token fieldName =
        consume(TokenType::IDENTIFIER, meadows::ErrorCode::PARSE_EXPECTED_IDENTIFIER, "Expect field name after '.'");
    expr = std::make_unique<FieldAccessExpr>(std::move(expr), fieldName.value);
  }
  return expr;
}

std::unique_ptr<Expr> Parser::parsePrimary(int depth) {
  if (match(TokenType::STRING)) {
    return std::make_unique<LiteralExpr>(LiteralKind::Str, previous().value);
  }
  if (match(TokenType::NUMBER)) {
    const std::string &text = previous().value;
    LiteralKind kind = text.find('.') != std::string::npos ? LiteralKind::Float
                                                            : LiteralKind::Int;
    return std::make_unique<LiteralExpr>(kind, text);
  }
  if (match(TokenType::TRUE)) {
    return std::make_unique<LiteralExpr>(LiteralKind::Int, "1");
  }
  if (match(TokenType::FALSE)) {
    return std::make_unique<LiteralExpr>(LiteralKind::Int, "0");
  }
  if (match(TokenType::IDENTIFIER)) {
    return std::make_unique<VarExpr>(previous().value);
  }
  if (match(TokenType::LEFT_BRACKET)) {
    std::vector<std::unique_ptr<Expr>> elements;
    if (!check(TokenType::RIGHT_BRACKET)) {
      do {
        elements.push_back(parseExpr(depth + 1));
      } while (match(TokenType::COMMA));
    }
    consume(TokenType::RIGHT_BRACKET, meadows::ErrorCode::PARSE_UNEXPECTED_TOKEN, "Expect ']' after array elements");
    return std::make_unique<ArrayExpr>(std::move(elements));
  }
  if (match(TokenType::LEFT_BRACE)) {
    std::unordered_map<std::string, std::unique_ptr<Expr>> pairs;
    if (!check(TokenType::RIGHT_BRACE)) {
      do {
        const Token &key = consume(TokenType::IDENTIFIER, meadows::ErrorCode::PARSE_EXPECTED_IDENTIFIER, "Expect key");
        consume(TokenType::COLON, meadows::ErrorCode::PARSE_EXPECTED_COLON, "Expect ':' after key");
        auto value = parseExpr(depth + 1);
        pairs[key.value] = std::move(value);
      } while (match(TokenType::COMMA));
    }
    consume(TokenType::RIGHT_BRACE, meadows::ErrorCode::PARSE_EXPECTED_RBRACE, "Expect '}' after object");
    return std::make_unique<ObjectExpr>(std::move(pairs));
  }
  if (match(TokenType::LEFT_PAREN)) {
    auto expr = parseExpr(depth + 1);
    consume(TokenType::RIGHT_PAREN, meadows::ErrorCode::PARSE_EXPECTED_RPAREN, "Expect ')' after expression");
    return expr;
  }
  meadows::SourceLocation loc("", peek().line, peek().column);
  throw meadows::ParseException(meadows::ErrorCode::PARSE_EXPECTED_EXPRESSION,
                                "Expect expression", loc);
}

std::vector<std::unique_ptr<Expr>> Parser::parseArgs(int depth) {
  std::vector<std::unique_ptr<Expr>> args;
  if (!check(TokenType::RIGHT_PAREN)) {
    do {
      args.push_back(parseExpr(depth + 1));
    } while (match(TokenType::COMMA));
  }
  return args;
}