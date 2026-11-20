#include "Parser.h"

#include <cstdint>
#include <cstdlib>

namespace meadows {

namespace {

struct BinaryLevel {
  TokenType token;
  BinaryOp binary;
  LogicalOp logical;
  bool isLogical;
};

constexpr BinaryOp NO_BINARY = BinaryOp::Add;
constexpr LogicalOp NO_LOGICAL = LogicalOp::And;

const std::vector<std::vector<BinaryLevel>>& levels() {
  static const std::vector<std::vector<BinaryLevel>> table = {
      {{TokenType::OR, NO_BINARY, LogicalOp::Or, true}},
      {{TokenType::AND, NO_BINARY, LogicalOp::And, true}},
      {{TokenType::EQUAL_EQUAL, BinaryOp::Eq, NO_LOGICAL, false},
       {TokenType::BANG_EQUAL, BinaryOp::Ne, NO_LOGICAL, false}},
      {{TokenType::LESS, BinaryOp::Lt, NO_LOGICAL, false},
       {TokenType::LESS_EQUAL, BinaryOp::Le, NO_LOGICAL, false},
       {TokenType::GREATER, BinaryOp::Gt, NO_LOGICAL, false},
       {TokenType::GREATER_EQUAL, BinaryOp::Ge, NO_LOGICAL, false}},
      {{TokenType::PLUS, BinaryOp::Add, NO_LOGICAL, false},
       {TokenType::MINUS, BinaryOp::Sub, NO_LOGICAL, false}},
      {{TokenType::STAR, BinaryOp::Mul, NO_LOGICAL, false},
       {TokenType::SLASH, BinaryOp::Div, NO_LOGICAL, false},
       {TokenType::PERCENT, BinaryOp::Mod, NO_LOGICAL, false}}};
  return table;
}

bool isAssignable(const Expr& expr) {
  return dynamic_cast<const VarExpr*>(&expr) ||
         dynamic_cast<const IndexExpr*>(&expr) ||
         dynamic_cast<const FieldExpr*>(&expr);
}

bool startsPostfix(TokenType type) {
  return type == TokenType::LEFT_PAREN || type == TokenType::LEFT_BRACKET ||
         type == TokenType::DOT;
}

bool startsStatement(TokenType type) {
  switch (type) {
  case TokenType::LET:
  case TokenType::FUNC:
  case TokenType::IF:
  case TokenType::FOR:
  case TokenType::WHILE:
  case TokenType::RETURN:
  case TokenType::BREAK:
  case TokenType::CONTINUE:
  case TokenType::IMPORT:
    return true;
  default:
    return false;
  }
}

} // namespace

Parser::DepthScope::DepthScope(Parser& parser) : parser_(parser) {
  if (++parser_.depth_ > MAX_NESTING) {
    parser_.abandoned_ = true;
    parser_.fail(ErrorCode::PARSE_NESTING_TOO_DEEP,
                 "Nesting is too deep (limit " + std::to_string(MAX_NESTING) +
                     ")",
                 parser_.peek().location, parser_.peek().length);
  }
}

Parser::Parser(std::vector<Token> tokens, DiagnosticSink& diagnostics)
    : tokens_(std::move(tokens)), diagnostics_(diagnostics) {}

const Token& Parser::advance() {
  if (!check(TokenType::END_OF_FILE)) {
    ++current_;
  }
  return previous();
}

bool Parser::match(TokenType type) {
  if (!check(type)) {
    return false;
  }
  advance();
  return true;
}

Location Parser::expectedLocation() const {
  if (current_ > 0 && previous().location.line < peek().location.line) {
    Location end = previous().location;
    end.column += previous().length;
    return end;
  }
  return peek().location;
}

void Parser::error(ErrorCode code, const std::string& message,
                   Location location, uint32_t length,
                   const std::string& help) {
  diagnostics_.report(code, message, location, length, help);
  if (diagnostics_.full()) {
    abandoned_ = true;
  }
}

void Parser::fail(ErrorCode code, const std::string& message, Location location,
                  uint32_t length, const std::string& help) {
  error(code, message, location, length, help);
  throw ParseAbort{};
}

const Token& Parser::expect(TokenType type, ErrorCode code,
                            const std::string& what) {
  if (check(type)) {
    return advance();
  }
  fail(code, what + ", found " + describeToken(peek()), expectedLocation(),
       peek().length);
}

void Parser::synchronize(bool insideBlock) {
  while (!atEnd()) {
    if (previous().type == TokenType::SEMICOLON) {
      return;
    }
    TokenType type = peek().type;
    if (startsStatement(type) ||
        (insideBlock && type == TokenType::RIGHT_BRACE)) {
      return;
    }
    advance();
  }
}

ParsedFile Parser::parse() {
  ParsedFile file;
  while (!atEnd()) {
    size_t start = current_;
    try {
      parseTopLevel(file);
    } catch (const ParseAbort&) {
      synchronize(false);
      if (current_ == start && !atEnd()) {
        advance();
      }
    }
  }
  return file;
}

void Parser::parseTopLevel(ParsedFile& file) {
  if (check(TokenType::IMPORT)) {
    file.imports.push_back(parseImport(file.statements.size()));
    return;
  }
  file.statements.push_back(parseStatement(true));
}

ImportRequest Parser::parseImport(size_t position) {
  Location location = advance().location;
  const Token& path =
      expect(TokenType::STRING, ErrorCode::PARSE_EXPECTED_STRING,
             "Expected a file path string after 'import'");
  std::string text = path.text;
  expect(TokenType::SEMICOLON, ErrorCode::PARSE_EXPECTED_SEMICOLON,
         "Expected ';' after import");
  return ImportRequest{position, std::move(text), location};
}

StmtPtr Parser::parseStatement(bool topLevel) {
  DepthScope scope(*this);
  Location start = peek().location;
  StmtPtr stmt;

  if (check(TokenType::FUNC) && !topLevel) {
    error(ErrorCode::PARSE_MISPLACED_DECLARATION,
          "Functions can only be declared at the top level of a file",
          peek().location, peek().length);
  }

  if (match(TokenType::LET)) {
    stmt = parseLet();
  } else if (match(TokenType::FUNC)) {
    stmt = parseFunc();
  } else if (match(TokenType::IF)) {
    stmt = parseIf();
  } else if (match(TokenType::FOR)) {
    stmt = parseFor();
  } else if (match(TokenType::WHILE)) {
    stmt = parseWhile();
  } else if (match(TokenType::RETURN)) {
    stmt = parseReturn();
  } else if (match(TokenType::BREAK)) {
    expect(TokenType::SEMICOLON, ErrorCode::PARSE_EXPECTED_SEMICOLON,
           "Expected ';' after 'break'");
    stmt = std::make_unique<BreakStmt>();
  } else if (match(TokenType::CONTINUE)) {
    expect(TokenType::SEMICOLON, ErrorCode::PARSE_EXPECTED_SEMICOLON,
           "Expected ';' after 'continue'");
    stmt = std::make_unique<ContinueStmt>();
  } else if (check(TokenType::LEFT_BRACE)) {
    stmt = std::make_unique<BlockStmt>(parseBlock());
  } else if (check(TokenType::IMPORT)) {
    parseImport(0);
    fail(ErrorCode::PARSE_MISPLACED_DECLARATION,
         "Imports can only be declared at the top level of a file", start, 6);
  } else {
    stmt = parseExpressionStatement();
  }
  stmt->location = start;
  return stmt;
}

StmtPtr Parser::parseLet() {
  const Token& name = expect(TokenType::IDENTIFIER,
                             ErrorCode::PARSE_EXPECTED_IDENTIFIER,
                             "Expected a variable name after 'let'");
  std::string text = name.text;
  Location nameLocation = name.location;
  expect(TokenType::EQUAL, ErrorCode::PARSE_EXPECTED_EQUALS,
         "Expected '=' after the variable name");
  ExprPtr initializer = parseExpression();
  expect(TokenType::SEMICOLON, ErrorCode::PARSE_EXPECTED_SEMICOLON,
         "Expected ';' after the variable declaration");
  return std::make_unique<LetStmt>(std::move(text), nameLocation,
                                   std::move(initializer));
}

StmtPtr Parser::parseFunc() {
  const Token& name = expect(TokenType::IDENTIFIER,
                             ErrorCode::PARSE_EXPECTED_IDENTIFIER,
                             "Expected a function name after 'func'");
  std::string text = name.text;
  Location nameLocation = name.location;
  expect(TokenType::LEFT_PAREN, ErrorCode::PARSE_EXPECTED_LPAREN,
         "Expected '(' after the function name");
  std::vector<Param> params;
  if (!check(TokenType::RIGHT_PAREN)) {
    do {
      const Token& param = expect(TokenType::IDENTIFIER,
                                  ErrorCode::PARSE_EXPECTED_IDENTIFIER,
                                  "Expected a parameter name");
      params.push_back({param.text, param.location});
    } while (match(TokenType::COMMA));
  }
  expect(TokenType::RIGHT_PAREN, ErrorCode::PARSE_EXPECTED_RPAREN,
         "Expected ')' after the parameters");
  return std::make_unique<FuncStmt>(std::move(text), nameLocation,
                                    std::move(params), parseBlock());
}

StmtPtr Parser::parseIf() {
  expect(TokenType::LEFT_PAREN, ErrorCode::PARSE_EXPECTED_LPAREN,
         "Expected '(' after 'if'");
  ExprPtr condition = parseExpression();
  expect(TokenType::RIGHT_PAREN, ErrorCode::PARSE_EXPECTED_RPAREN,
         "Expected ')' after the condition");
  StmtList thenBody = parseBlock();
  StmtList elseBody;
  if (match(TokenType::ELSE)) {
    if (check(TokenType::IF)) {
      Location location = advance().location;
      elseBody.push_back(parseIf());
      elseBody.back()->location = location;
    } else {
      elseBody = parseBlock();
    }
  }
  return std::make_unique<IfStmt>(std::move(condition), std::move(thenBody),
                                  std::move(elseBody));
}

StmtPtr Parser::parseFor() {
  expect(TokenType::LEFT_PAREN, ErrorCode::PARSE_EXPECTED_LPAREN,
         "Expected '(' after 'for'");
  const Token& var = expect(TokenType::IDENTIFIER,
                            ErrorCode::PARSE_EXPECTED_IDENTIFIER,
                            "Expected a loop variable name");
  std::string name = var.text;
  Location varLocation = var.location;
  expect(TokenType::IN, ErrorCode::PARSE_EXPECTED_IN,
         "Expected 'in' after the loop variable");
  expect(TokenType::RANGE, ErrorCode::PARSE_UNEXPECTED_TOKEN,
         "Expected 'range' after 'in'");
  expect(TokenType::LEFT_PAREN, ErrorCode::PARSE_EXPECTED_LPAREN,
         "Expected '(' after 'range'");
  ExprPtr start = parseExpression();
  expect(TokenType::COMMA, ErrorCode::PARSE_EXPECTED_COMMA,
         "Expected ',' between the range bounds");
  ExprPtr end = parseExpression();
  expect(TokenType::RIGHT_PAREN, ErrorCode::PARSE_EXPECTED_RPAREN,
         "Expected ')' after the range bounds");
  expect(TokenType::RIGHT_PAREN, ErrorCode::PARSE_EXPECTED_RPAREN,
         "Expected ')' after 'range(...)'");
  return std::make_unique<ForStmt>(std::move(name), varLocation,
                                   std::move(start), std::move(end),
                                   parseBlock());
}

StmtPtr Parser::parseWhile() {
  expect(TokenType::LEFT_PAREN, ErrorCode::PARSE_EXPECTED_LPAREN,
         "Expected '(' after 'while'");
  ExprPtr condition = parseExpression();
  expect(TokenType::RIGHT_PAREN, ErrorCode::PARSE_EXPECTED_RPAREN,
         "Expected ')' after the condition");
  return std::make_unique<WhileStmt>(std::move(condition), parseBlock());
}

StmtPtr Parser::parseReturn() {
  ExprPtr value;
  if (!check(TokenType::SEMICOLON)) {
    value = parseExpression();
  }
  expect(TokenType::SEMICOLON, ErrorCode::PARSE_EXPECTED_SEMICOLON,
         "Expected ';' after 'return'");
  return std::make_unique<ReturnStmt>(std::move(value));
}

StmtPtr Parser::parseExpressionStatement() {
  ExprPtr expr = parseExpression();
  expect(TokenType::SEMICOLON, ErrorCode::PARSE_EXPECTED_SEMICOLON,
         "Expected ';' after the expression");
  return std::make_unique<ExprStmt>(std::move(expr));
}

StmtList Parser::parseBlock() {
  expect(TokenType::LEFT_BRACE, ErrorCode::PARSE_EXPECTED_LBRACE,
         "Expected '{'");
  StmtList body;
  while (!check(TokenType::RIGHT_BRACE) && !atEnd()) {
    size_t start = current_;
    try {
      body.push_back(parseStatement(false));
    } catch (const ParseAbort&) {
      synchronize(true);
      if (current_ == start && !check(TokenType::RIGHT_BRACE) && !atEnd()) {
        advance();
      }
    }
  }
  expect(TokenType::RIGHT_BRACE, ErrorCode::PARSE_EXPECTED_RBRACE,
         "Expected '}' to close the block");
  return body;
}

ExprPtr Parser::parseExpression() { return parseAssignment(); }

ExprPtr Parser::bounded(ExprPtr expr, Location location) {
  expr->location = location;
  if (expr->height > MAX_NESTING) {
    abandoned_ = true;
    fail(ErrorCode::PARSE_NESTING_TOO_DEEP,
         "Expression is too deeply nested (limit " +
             std::to_string(MAX_NESTING) + ")",
         location, 1);
  }
  return expr;
}

ExprPtr Parser::parseAssignment() {
  DepthScope scope(*this);
  ExprPtr target = parseBinary(0);
  if (!check(TokenType::EQUAL)) {
    return target;
  }
  const Token& equals = advance();
  Location location = equals.location;
  if (!isAssignable(*target)) {
    fail(ErrorCode::PARSE_INVALID_ASSIGNMENT_TARGET,
         "Invalid assignment target", target->location, 1,
         "Only variables, array elements and object fields can be assigned");
  }
  ExprPtr value = parseAssignment();
  return bounded(
      std::make_unique<AssignExpr>(std::move(target), std::move(value)),
      location);
}

ExprPtr Parser::parseBinary(int level) {
  const auto& table = levels();
  if (static_cast<size_t>(level) == table.size()) {
    return parseUnary();
  }
  ExprPtr left = parseBinary(level + 1);
  while (true) {
    const BinaryLevel* matched = nullptr;
    for (const BinaryLevel& candidate : table[static_cast<size_t>(level)]) {
      if (check(candidate.token)) {
        matched = &candidate;
        break;
      }
    }
    if (!matched) {
      return left;
    }
    Location location = advance().location;
    ExprPtr right = parseBinary(level + 1);
    ExprPtr node;
    if (matched->isLogical) {
      node = std::make_unique<LogicalExpr>(matched->logical, std::move(left),
                                           std::move(right));
    } else {
      node = std::make_unique<BinaryExpr>(matched->binary, std::move(left),
                                          std::move(right));
    }
    left = bounded(std::move(node), location);
  }
}

ExprPtr Parser::parseUnary() {
  DepthScope scope(*this);
  if (check(TokenType::MINUS) || check(TokenType::BANG)) {
    const Token& op = advance();
    Location location = op.location;
    UnaryOp kind = op.type == TokenType::MINUS ? UnaryOp::Negate : UnaryOp::Not;
    if (kind == UnaryOp::Negate && check(TokenType::NUMBER) &&
        !startsPostfix(tokens_[current_ + 1].type)) {
      ExprPtr literal = parseNumber(advance(), true);
      literal->location = location;
      return literal;
    }
    return bounded(std::make_unique<UnaryExpr>(kind, parseUnary()), location);
  }
  return parsePostfix();
}

ExprPtr Parser::parsePostfix() {
  ExprPtr expr = parsePrimary();
  while (true) {
    if (check(TokenType::LEFT_PAREN)) {
      Location location = advance().location;
      std::vector<ExprPtr> args = parseArguments();
      expect(TokenType::RIGHT_PAREN, ErrorCode::PARSE_EXPECTED_RPAREN,
             "Expected ')' after the arguments");
      expr = bounded(std::make_unique<CallExpr>(std::move(expr), std::move(args)),
                     location);
    } else if (check(TokenType::LEFT_BRACKET)) {
      Location location = advance().location;
      ExprPtr index = parseExpression();
      expect(TokenType::RIGHT_BRACKET, ErrorCode::PARSE_EXPECTED_RBRACKET,
             "Expected ']' after the index");
      expr = bounded(
          std::make_unique<IndexExpr>(std::move(expr), std::move(index)),
          location);
    } else if (check(TokenType::DOT)) {
      Location location = advance().location;
      const Token& name = expect(TokenType::IDENTIFIER,
                                 ErrorCode::PARSE_EXPECTED_IDENTIFIER,
                                 "Expected a field name after '.'");
      expr = bounded(std::make_unique<FieldExpr>(std::move(expr), name.text),
                     location);
    } else {
      return expr;
    }
  }
}

std::vector<ExprPtr> Parser::parseArguments() {
  std::vector<ExprPtr> args;
  if (!check(TokenType::RIGHT_PAREN)) {
    do {
      args.push_back(parseExpression());
    } while (match(TokenType::COMMA));
  }
  return args;
}

ExprPtr Parser::parseNumber(const Token& token, bool negated) {
  ExprPtr literal;
  if (token.text.find('.') != std::string::npos) {
    double value = std::strtod(token.text.c_str(), nullptr);
    literal = LiteralExpr::real(negated ? -value : value);
  } else {
    int64_t value = std::strtoll(token.text.c_str(), nullptr, 10);
    if (value > INT32_MAX && !(negated && value == int64_t{INT32_MAX} + 1)) {
      fail(ErrorCode::LEX_NUMBER_OUT_OF_RANGE,
           "Number literal '" + token.text + "' is out of range",
           token.location, token.length);
    }
    literal = LiteralExpr::integer(
        static_cast<int32_t>(negated ? -value : value));
  }
  literal->location = token.location;
  return literal;
}

ExprPtr Parser::parsePrimary() {
  const Token& token = peek();
  Location location = token.location;
  ExprPtr expr;

  switch (token.type) {
  case TokenType::STRING:
    expr = LiteralExpr::string(advance().text);
    break;
  case TokenType::NUMBER:
    return parseNumber(advance(), false);
  case TokenType::TRUE:
    advance();
    expr = LiteralExpr::integer(1);
    break;
  case TokenType::FALSE:
    advance();
    expr = LiteralExpr::integer(0);
    break;
  case TokenType::IDENTIFIER:
    expr = std::make_unique<VarExpr>(advance().text);
    break;
  case TokenType::LEFT_PAREN: {
    advance();
    expr = parseExpression();
    expect(TokenType::RIGHT_PAREN, ErrorCode::PARSE_EXPECTED_RPAREN,
           "Expected ')' after the expression");
    return expr;
  }
  case TokenType::LEFT_BRACKET: {
    advance();
    std::vector<ExprPtr> elements;
    if (!check(TokenType::RIGHT_BRACKET)) {
      do {
        elements.push_back(parseExpression());
      } while (match(TokenType::COMMA));
    }
    expect(TokenType::RIGHT_BRACKET, ErrorCode::PARSE_EXPECTED_RBRACKET,
           "Expected ']' after the array elements");
    return bounded(std::make_unique<ArrayExpr>(std::move(elements)), location);
  }
  case TokenType::LEFT_BRACE:
    return parseObject(advance());
  default:
    fail(ErrorCode::PARSE_EXPECTED_EXPRESSION,
         "Expected an expression, found " + describeToken(token),
         expectedLocation(), token.length);
  }
  expr->location = location;
  return expr;
}

ExprPtr Parser::parseObject(const Token& open) {
  Location location = open.location;
  std::vector<ObjectField> fields;
  if (!check(TokenType::RIGHT_BRACE)) {
    do {
      const Token& key = expect(TokenType::IDENTIFIER,
                                ErrorCode::PARSE_EXPECTED_IDENTIFIER,
                                "Expected a field name");
      std::string name = key.text;
      Location keyLocation = key.location;
      expect(TokenType::COLON, ErrorCode::PARSE_EXPECTED_COLON,
             "Expected ':' after the field name");
      fields.push_back({std::move(name), parseExpression(), keyLocation});
    } while (match(TokenType::COMMA));
  }
  expect(TokenType::RIGHT_BRACE, ErrorCode::PARSE_EXPECTED_RBRACE,
         "Expected '}' after the object fields");
  return bounded(std::make_unique<ObjectExpr>(std::move(fields)), location);
}

} // namespace meadows
