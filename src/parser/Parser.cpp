#include <stdexcept>

#include "../utils/MemoryUtils.h"
#include "Parser.h"
#include <stdexcept>

Parser::Parser(std::vector<Token> t)
    : tokens_(std::move(t)), current_(0), diagnostics_(nullptr),
      inErrorRecovery_(false), consecutiveErrors_(0) {}

Parser::Parser(std::vector<Token> t, meadows::DiagnosticsCollector &diagnostics)
    : tokens_(std::move(t)), current_(0), diagnostics_(&diagnostics),
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

const Token &Parser::peek() const { return tokens_[current_]; }

const Token &Parser::previous() const { return tokens_[current_ - 1]; }

const Token &Parser::advance() {
  if (!isAtEnd())
    current_++;
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
  // Return current_ token anyway to allow parsing to continue
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
  if (match(TokenType::MODULE))
    return parseModuleStmt();
  if (match(TokenType::IMPORT))
    return parseImportStmt();
  if (match(TokenType::EXPORT))
    return parseExportStmt();
  if (match(TokenType::LEFT_BRACE))
    return parseBlockStmt();
  return parseExprStmt();
}

std::unique_ptr<Stmt> Parser::parseLetStmt() {
  const Token &name = consume(TokenType::IDENTIFIER, "Expect variable name");

  // Parse optional type annotation
  std::string typeAnnotation;
  if (match(TokenType::COLON)) {
    // Type can be a keyword (i32, i64, f32, f64, bool, string) or identifier
    TokenType typeTokens[] = {TokenType::I32,       TokenType::I64,
                              TokenType::F32,       TokenType::F64,
                              TokenType::BOOL,      TokenType::STRING,
                              TokenType::IDENTIFIER};

    bool foundType = false;
    for (auto typeTok : typeTokens) {
      if (match(typeTok)) {
        typeAnnotation = previous().value;
        foundType = true;
        break;
      }
    }

    if (!foundType) {
      error(meadows::ErrorCode::PARSE_UNEXPECTED_TOKEN,
            "Expect type name after ':'");
    }

    // Handle generic types like List<i32>
    if (match(TokenType::LESS)) {
      typeAnnotation += "<";
      const Token &genericToken =
          consume(TokenType::IDENTIFIER, "Expect generic type parameter");
      typeAnnotation += genericToken.value;
      consume(TokenType::GREATER, "Expect '>' after generic type");
      typeAnnotation += ">";
    }
  }

  consume(TokenType::EQUAL, "Expect '=' after variable name");
  auto initializer = parseExpr();
  consume(TokenType::SEMICOLON, "Expect ';' after variable declaration");
  return std::make_unique<LetStmt>(name.value, std::move(initializer),
                                   typeAnnotation);
}

std::unique_ptr<Stmt> Parser::parseFuncStmt() {
  const Token &name = consume(TokenType::IDENTIFIER, "Expect function name");
  consume(TokenType::LEFT_PAREN, "Expect '(' after function name");
  std::vector<FuncParam> params;
  if (!check(TokenType::RIGHT_PAREN)) {
    do {
      const Token &paramName =
          consume(TokenType::IDENTIFIER, "Expect parameter name");
      std::string typeAnnotation;
      if (match(TokenType::COLON)) {
        // Type can be a keyword or identifier
        TokenType typeTokens[] = {TokenType::I32,       TokenType::I64,
                                  TokenType::F32,       TokenType::F64,
                                  TokenType::BOOL,      TokenType::STRING,
                                  TokenType::IDENTIFIER};
        bool foundType = false;
        for (auto typeTok : typeTokens) {
          if (match(typeTok)) {
            typeAnnotation = previous().value;
            foundType = true;
            break;
          }
        }
        if (!foundType) {
          error(meadows::ErrorCode::PARSE_UNEXPECTED_TOKEN,
                "Expect type name after ':'");
        }
      }
      params.emplace_back(paramName.value, typeAnnotation);
    } while (match(TokenType::COMMA));
  }
  consume(TokenType::RIGHT_PAREN, "Expect ')' after parameters");

  // Parse optional return type
  std::string returnType;
  if (match(TokenType::ARROW)) {
    TokenType typeTokens[] = {TokenType::I32,       TokenType::I64,
                              TokenType::F32,       TokenType::F64,
                              TokenType::BOOL,      TokenType::STRING,
                              TokenType::IDENTIFIER};
    bool foundType = false;
    for (auto typeTok : typeTokens) {
      if (match(typeTok)) {
        returnType = previous().value;
        foundType = true;
        break;
      }
    }
    if (!foundType) {
      error(meadows::ErrorCode::PARSE_UNEXPECTED_TOKEN,
            "Expect return type after '->'");
    }
  }

  consume(TokenType::LEFT_BRACE, "Expect '{' before function body");
  auto body = parseBlock();
  consume(TokenType::RIGHT_BRACE, "Expect '}' after function body");
  return std::make_unique<FuncStmt>(name.value, std::move(params),
                                    std::move(body), returnType);
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

std::unique_ptr<Stmt> Parser::parseBreakStmt() {
  consume(TokenType::SEMICOLON, "Expect ';' after break");
  return std::make_unique<BreakStmt>();
}

std::unique_ptr<Stmt> Parser::parseContinueStmt() {
  consume(TokenType::SEMICOLON, "Expect ';' after continue");
  return std::make_unique<ContinueStmt>();
}

std::unique_ptr<Stmt> Parser::parseModuleStmt() {
  const Token &name = consume(TokenType::IDENTIFIER, "Expect module name");
  std::string modulePath = name.value;
  while (match(TokenType::DOT)) {
    const Token &component =
        consume(TokenType::IDENTIFIER, "Expect module path component");
    modulePath += "." + component.value;
  }
  consume(TokenType::SEMICOLON, "Expect ';' after module declaration");
  return std::make_unique<ModuleStmt>(modulePath);
}

std::unique_ptr<Stmt> Parser::parseImportStmt() {
  std::string modulePath;
  std::vector<std::string> specificImports;
  std::string alias;

  const Token &firstComponent =
      consume(TokenType::IDENTIFIER, "Expect module path component");
  modulePath = firstComponent.value;

  while (check(TokenType::DOT)) {
    advance();
    if (check(TokenType::LEFT_BRACE)) {
      break;
    }
    const Token &component =
        consume(TokenType::IDENTIFIER, "Expect module path component");
    modulePath += "." + component.value;
  }

  if (match(TokenType::LEFT_BRACE)) {
    do {
      const Token &importName =
          consume(TokenType::IDENTIFIER, "Expect import name");
      specificImports.push_back(importName.value);
    } while (match(TokenType::COMMA));
    consume(TokenType::RIGHT_BRACE, "Expect '}' after import list");
  }

  if (match(TokenType::AS)) {
    const Token &aliasToken =
        consume(TokenType::IDENTIFIER, "Expect alias name");
    alias = aliasToken.value;
  }

  consume(TokenType::SEMICOLON, "Expect ';' after import statement");
  return std::make_unique<ImportStmt>(modulePath, std::move(specificImports),
                                      alias);
}

std::unique_ptr<Stmt> Parser::parseExportStmt() {
  const Token &name = consume(TokenType::IDENTIFIER, "Expect name to export");
  std::string typeInfo;

  if (match(TokenType::COLON)) {
    TokenType typeTokens[] = {TokenType::I32,       TokenType::I64,
                              TokenType::F32,       TokenType::F64,
                              TokenType::BOOL,      TokenType::STRING,
                              TokenType::IDENTIFIER};
    bool foundType = false;
    for (auto typeTok : typeTokens) {
      if (match(typeTok)) {
        typeInfo = previous().value;
        foundType = true;
        break;
      }
    }
    if (!foundType) {
      error(meadows::ErrorCode::PARSE_UNEXPECTED_TOKEN,
            "Expect type name after ':' in export");
    }

    if (match(TokenType::ARROW)) {
      typeInfo += " -> ";
      bool foundReturnType = false;
      for (auto typeTok : typeTokens) {
        if (match(typeTok)) {
          typeInfo += previous().value;
          foundReturnType = true;
          break;
        }
      }
      if (!foundReturnType) {
        error(meadows::ErrorCode::PARSE_UNEXPECTED_TOKEN,
              "Expect return type after '->' in export");
      }
    }
  }

  consume(TokenType::SEMICOLON, "Expect ';' after export statement");
  return std::make_unique<ExportStmt>(name.value, typeInfo);
}