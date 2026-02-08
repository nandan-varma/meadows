#include "Parser.h"
#include <stdexcept>

Parser::Parser(std::vector<Token> t)
    : tokens(std::move(t)), current(0), diagnostics_(nullptr),
      inErrorRecovery_(false), consecutiveErrors_(0) {}

Parser::Parser(std::vector<Token> t, meadows::DiagnosticsCollector &diagnostics)
    : tokens(std::move(t)), current(0), diagnostics_(&diagnostics),
      inErrorRecovery_(false), consecutiveErrors_(0) {}

std::vector<std::unique_ptr<Stmt>> Parser::parse() {
  std::vector<std::unique_ptr<Stmt>> statements;
  while (!isAtEnd()) {
    try {
      auto stmt = parseStmt();
      if (stmt) {
        statements.push_back(std::move(stmt));
        // Successfully parsed a statement, reset consecutive error counter
        consecutiveErrors_ = 0;
        inErrorRecovery_ = false;
      }
    } catch (const meadows::MeadowsException &e) {
      // Fatal error - rethrow
      throw;
    } catch (const std::runtime_error &e) {
      // Legacy error handling - convert to diagnostic if available
      if (diagnostics_) {
        meadows::SourceLocation loc("", peek().line, peek().column);
        diagnostics_->reportError(meadows::ErrorCode::PARSE_UNEXPECTED_TOKEN,
                                  e.what(), loc);
        synchronize();
      } else {
        throw;
      }
    }
  }
  return statements;
}

bool Parser::hasErrors() const {
  return diagnostics_ && diagnostics_->hasErrors();
}

bool Parser::isAtEnd() const { return peek().type == TokenType::EOF_TOKEN; }

const Token &Parser::peek() const { return tokens[current]; }

const Token &Parser::previous() const { return tokens[current - 1]; }

const Token &Parser::advance() {
  if (!isAtEnd())
    current++;
  return previous();
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

const Token &Parser::consume(TokenType type, meadows::ErrorCode code,
                             const std::string &message) {
  if (check(type)) {
    return advance();
  }
  error(code, message);
  // Return current token anyway to allow parsing to continue
  return peek();
}

void Parser::error(meadows::ErrorCode code, const std::string &message) {
  if (diagnostics_) {
    meadows::SourceLocation loc("", peek().line, peek().column);
    loc.endColumn = loc.column + 1;
    diagnostics_->reportError(code, message, loc);
    consecutiveErrors_++;
    inErrorRecovery_ = true;
  } else {
    meadows::SourceLocation loc("", peek().line, peek().column);
    throw meadows::ParseException(code, message, loc);
  }
}

void Parser::synchronize() {
  if (!diagnostics_)
    return;

  inErrorRecovery_ = true;

  advance(); // Skip the token that caused the error

  // Synchronize at statement boundaries
  while (!isAtEnd()) {
    // Semicolon ends most statements
    if (previous().type == TokenType::SEMICOLON)
      return;

    // Keywords that start new statements
    switch (peek().type) {
    case TokenType::LET:
    case TokenType::FUNC:
    case TokenType::IF:
    case TokenType::FOR:
    case TokenType::WHILE:
    case TokenType::PRINT:
    case TokenType::RETURN:
    case TokenType::BREAK:
    case TokenType::CONTINUE:
    case TokenType::LEFT_BRACE: // Block start
      return;
    default:
      break;
    }

    advance();
  }
}

bool Parser::shouldAbortRecovery() const {
  return consecutiveErrors_ >= MAX_CONSECUTIVE_ERRORS;
}

std::unique_ptr<Stmt> Parser::parseStmt() {
  if (match(TokenType::LET))
    return parseLetStmt();
  if (match(TokenType::FUNC))
    return parseFuncStmt();
  if (match(TokenType::IF))
    return parseIfStmt();
  if (match(TokenType::FOR))
    return parseForStmt();
  if (match(TokenType::WHILE))
    return parseWhileStmt();
  if (match(TokenType::PRINT))
    return parsePrintStmt();
  if (match(TokenType::RETURN))
    return parseReturnStmt();
  if (match(TokenType::BREAK))
    return parseBreakStmt();
  if (match(TokenType::CONTINUE))
    return parseContinueStmt();
  if (match(TokenType::LEFT_BRACE))
    return parseBlockStmt();
  return parseExprStmt();
}

std::unique_ptr<Stmt> Parser::parseLetStmt() {
  const Token &name = consume(TokenType::IDENTIFIER, "Expect variable name");
  consume(TokenType::EQUAL, "Expect '=' after variable name");
  auto initializer = parseExpr();
  consume(TokenType::SEMICOLON, "Expect ';' after variable declaration");
  return std::make_unique<LetStmt>(name.value, std::move(initializer));
}

std::unique_ptr<Stmt> Parser::parseFuncStmt() {
  const Token &name = consume(TokenType::IDENTIFIER, "Expect function name");
  consume(TokenType::LEFT_PAREN, "Expect '(' after function name");
  std::vector<std::string> params;
  if (!check(TokenType::RIGHT_PAREN)) {
    do {
      const Token &param =
          consume(TokenType::IDENTIFIER, "Expect parameter name");
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
  return std::make_unique<IfStmt>(std::move(condition), std::move(thenBranch),
                                  std::move(elseBranch));
}

std::unique_ptr<Stmt> Parser::parseForStmt() {
  consume(TokenType::LEFT_PAREN, "Expect '(' after 'for'");
  const Token &var = consume(TokenType::IDENTIFIER, "Expect variable name");
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
  return std::make_unique<ForStmt>(var.value, std::move(start), std::move(end),
                                   std::move(body));
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

std::unique_ptr<Stmt> Parser::parseBlockStmt() {
  auto body = parseBlock();
  consume(TokenType::RIGHT_BRACE, "Expect '}' after block");
  return std::make_unique<BlockStmt>(std::move(body));
}

std::unique_ptr<Expr> Parser::parseExpr() { return parseAssignment(); }

std::unique_ptr<Expr> Parser::parseAssignment() {
  auto expr = parseOr();
  if (match(TokenType::EQUAL)) {
    auto varExpr = dynamic_cast<VarExpr *>(expr.get());
    if (!varExpr) {
      meadows::SourceLocation loc("", peek().line, peek().column);
      throw meadows::ParseException(
          meadows::ErrorCode::PARSE_INVALID_ASSIGNMENT_TARGET,
          "Invalid assignment target", loc);
    }
    auto value = parseAssignment();
    return std::make_unique<AssignExpr>(varExpr->name, std::move(value));
  }
  return expr;
}

std::unique_ptr<Expr> Parser::parseOr() {
  auto expr = parseAnd();
  while (match(TokenType::OR)) {
    Token op = previous();
    auto right = parseAnd();
    expr = std::make_unique<LogicalExpr>(std::move(expr), LogicalOperator::OR,
                                         std::move(right));
  }
  return expr;
}

std::unique_ptr<Expr> Parser::parseAnd() {
  auto expr = parseEquality();
  while (match(TokenType::AND)) {
    Token op = previous();
    auto right = parseEquality();
    expr = std::make_unique<LogicalExpr>(std::move(expr), LogicalOperator::AND,
                                         std::move(right));
  }
  return expr;
}

std::unique_ptr<Expr> Parser::parseEquality() {
  auto expr = parseComparison();
  while (match(TokenType::EQUAL_EQUAL) || match(TokenType::BANG_EQUAL)) {
    std::string op = previous().value;
    auto right = parseComparison();
    expr = std::make_unique<BinaryExpr>(std::move(expr), op, std::move(right));
  }
  return expr;
}

std::unique_ptr<Expr> Parser::parseComparison() {
  auto expr = parseTerm();
  while (match(TokenType::GREATER) || match(TokenType::GREATER_EQUAL) ||
         match(TokenType::LESS) || match(TokenType::LESS_EQUAL)) {
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
    auto operand = parseUnary();
    return std::make_unique<UnaryExpr>(op, std::move(operand));
  }
  if (match(TokenType::BANG)) {
    std::string op = previous().value;
    auto operand = parseUnary();
    return std::make_unique<UnaryExpr>(op, std::move(operand));
  }
  return parseCall();
}

std::unique_ptr<Expr> Parser::parseCall() {
  auto expr = parseIndex();
  if (match(TokenType::LEFT_PAREN)) {
    auto args = parseArgs();
    consume(TokenType::RIGHT_PAREN, "Expect ')' after arguments");
    expr = std::make_unique<CallExpr>(std::move(expr), std::move(args));
  }
  return expr;
}

std::unique_ptr<Expr> Parser::parseIndex() {
  auto expr = parseFieldAccess();
  while (match(TokenType::LEFT_BRACKET)) {
    auto index = parseExpr();
    consume(TokenType::RIGHT_BRACKET, "Expect ']' after index");
    expr = std::make_unique<IndexExpr>(std::move(expr), std::move(index));
  }
  return expr;
}

std::unique_ptr<Expr> Parser::parseFieldAccess() {
  auto expr = parsePrimary();
  while (match(TokenType::DOT)) {
    Token fieldName =
        consume(TokenType::IDENTIFIER, "Expect field name after '.'");
    expr = std::make_unique<FieldAccessExpr>(std::move(expr), fieldName.value);
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
  if (match(TokenType::TRUE)) {
    return std::make_unique<LiteralExpr>("1");
  }
  if (match(TokenType::FALSE)) {
    return std::make_unique<LiteralExpr>("0");
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

std::unique_ptr<Stmt> Parser::parseBreakStmt() {
  consume(TokenType::SEMICOLON, "Expect ';' after break");
  return std::make_unique<BreakStmt>();
}

std::unique_ptr<Stmt> Parser::parseContinueStmt() {
  consume(TokenType::SEMICOLON, "Expect ';' after continue");
  return std::make_unique<ContinueStmt>();
}