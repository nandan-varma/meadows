#include <cctype>
#include <stdexcept>
#include <unordered_map>

#include "../utils/ErrorCodes.h"
#include "../utils/Exceptions.h"
#include "../utils/MemoryUtils.h"
#include "Lexer.h"

static std::unordered_map<std::string, TokenType> keywords = {
    {"let", TokenType::LET},    {"func", TokenType::FUNC},
    {"if", TokenType::IF},      {"else", TokenType::ELSE},
    {"for", TokenType::FOR},    {"while", TokenType::WHILE},
    {"return", TokenType::RETURN}, {"in", TokenType::IN},
    {"range", TokenType::RANGE}, {"true", TokenType::TRUE},
    {"false", TokenType::FALSE}, {"break", TokenType::BREAK},
    {"continue", TokenType::CONTINUE}};

Lexer::Lexer(const std::string &source)
    : source_(source), pos_(0), line_(1), column_(1), currentLineStart_(0) {}

std::vector<Token> Lexer::tokenize() {
  std::vector<Token> tokens;
  while (!isAtEnd()) {
    tokens.push_back(nextToken());
  }
  tokens.push_back(Token(TokenType::EOF_TOKEN, "", line_, column_));
  return tokens;
}

char Lexer::peek() const {
  if (isAtEnd())
    return '\0';
  return source_[pos_];
}

char Lexer::advance() {
  char c = source_[pos_++];
  column_++;
  return c;
}

bool Lexer::isAtEnd() const { return pos_ >= source_.size(); }

Token Lexer::nextToken() {
  skipWhitespace();
  if (isAtEnd())
    return Token(TokenType::EOF_TOKEN, "", line_, column_);

  int startColumn = column_;
  char c = peek();
  if (c == '#') {
    skipComment();
    return nextToken();
  }
  if (c == '/' && pos_ + 1 < source_.size() && source_[pos_ + 1] == '/') {
    skipComment();
    return nextToken();
  }

  if (isalpha(c) || c == '_')
    return identifier();
  if (isdigit(c))
    return number();
  if (c == '"')
    return string();

  return handleOperator(c, startColumn);
}

Token Lexer::handleOperator(char c, int startColumn) {
  switch (c) {
  case '+':
    advance();
    return Token(TokenType::PLUS, "+", line_, startColumn);
  case '-':
    advance();
    return Token(TokenType::MINUS, "-", line_, startColumn);
  case '*':
    advance();
    return Token(TokenType::STAR, "*", line_, startColumn);
  case '/':
    advance();
    return Token(TokenType::SLASH, "/", line_, startColumn);
  case '=':
    advance();
    if (peek() == '=') {
      advance();
      return Token(TokenType::EQUAL_EQUAL, "==", line_, startColumn);
    }
    return Token(TokenType::EQUAL, "=", line_, startColumn);
  case '&':
    advance();
    if (peek() == '&') {
      advance();
      return Token(TokenType::AND, "&&", line_, startColumn);
    }
    return Token(TokenType::IDENTIFIER, "&", line_, startColumn);
  case '|':
    advance();
    if (peek() == '|') {
      advance();
      return Token(TokenType::OR, "||", line_, startColumn);
    }
    return Token(TokenType::IDENTIFIER, "|", line_, startColumn);
  case '>':
    advance();
    if (peek() == '=') {
      advance();
      return Token(TokenType::GREATER_EQUAL, ">=", line_, startColumn);
    }
    return Token(TokenType::GREATER, ">", line_, startColumn);
  case '<':
    advance();
    if (peek() == '=') {
      advance();
      return Token(TokenType::LESS_EQUAL, "<=", line_, startColumn);
    }
    return Token(TokenType::LESS, "<", line_, startColumn);
  case '!':
    advance();
    if (peek() == '=') {
      advance();
      return Token(TokenType::BANG_EQUAL, "!=", line_, startColumn);
    }
    return Token(TokenType::BANG, "!", line_, startColumn);
  case '(':
    advance();
    return Token(TokenType::LEFT_PAREN, "(", line_, startColumn);
  case ')':
    advance();
    return Token(TokenType::RIGHT_PAREN, ")", line_, startColumn);
  case '{':
    advance();
    return Token(TokenType::LEFT_BRACE, "{", line_, startColumn);
  case '}':
    advance();
    return Token(TokenType::RIGHT_BRACE, "}", line_, startColumn);
  case '[':
    advance();
    return Token(TokenType::LEFT_BRACKET, "[", line_, startColumn);
  case ']':
    advance();
    return Token(TokenType::RIGHT_BRACKET, "]", line_, startColumn);
  case ',':
    advance();
    return Token(TokenType::COMMA, ",", line_, startColumn);
  case ':':
    advance();
    return Token(TokenType::COLON, ":", line_, startColumn);
  case ';':
    advance();
    return Token(TokenType::SEMICOLON, ";", line_, startColumn);
  case '.':
    advance();
    return Token(TokenType::DOT, ".", line_, startColumn);
  }

  // Error
  advance();
  return Token(TokenType::IDENTIFIER, std::string(1, c), line_,
               startColumn); // For now, treat as identifier
}

Token Lexer::identifier() {
  int startColumn = column_;
  size_t start = pos_;
  while (isalnum(peek()) || peek() == '_') {
    advance();
  }
  std::string value = source_.substr(start, pos_ - start);
  auto it = keywords.find(value);
  if (it != keywords.end()) {
    return Token(it->second, value, line_, startColumn);
  }
  return Token(TokenType::IDENTIFIER, value, line_, startColumn);
}

Token Lexer::number() {
  int startColumn = column_;
  size_t start = pos_;
  while (isdigit(peek())) {
    advance();
  }
  std::string value = source_.substr(start, pos_ - start);
  return Token(TokenType::NUMBER, value, line_, startColumn);
}

static char unescapeChar(char c) {
  switch (c) {
  case 'n':
    return '\n';
  case 't':
    return '\t';
  case '\\':
    return '\\';
  case '"':
    return '"';
  case 'r':
    return '\r';
  case '0':
    return '\0';
  case 'b':
    return '\b';
  case 'f':
    return '\f';
  default:
    return c;
  }
}

Token Lexer::string() {
  int startColumn = column_;
  advance();
  size_t start = pos_;
  int startLine = line_;
  size_t nonEscapeCount = 0;
  while (!isAtEnd() && peek() != '"') {
    if (peek() == '\\' && pos_ + 1 < source_.size()) {
      advance();
    }
    if (peek() == '\n') {
      line_++;
      column_ = 1;
      currentLineStart_ = pos_ + 1;
    }
    advance();
    nonEscapeCount++;
  }
  if (isAtEnd()) {
    meadows::SourceLocation loc("", startLine, startColumn);
    throw meadows::LexicalException(meadows::ErrorCode::LEX_UNTERMINATED_STRING,
                                    "Unterminated string", loc);
  }
  std::string value;
  value.reserve(nonEscapeCount);
  for (size_t i = start; i < pos_; ++i) {
    if (source_[i] == '\\' && i + 1 < pos_) {
      value += unescapeChar(source_[i + 1]);
      ++i;
    } else {
      value += source_[i];
    }
  }
  if (value.size() > meadows::MAX_STRING_LENGTH) {
    meadows::SourceLocation loc("", startLine, startColumn);
    throw meadows::LexicalException(meadows::ErrorCode::LEX_INVALID_CHARACTER,
                                    "String literal exceeds maximum length",
                                    loc);
  }
  advance();
  return Token(TokenType::STRING, value, line_, startColumn);
}

void Lexer::skipWhitespace() {
  while (!isAtEnd()) {
    char c = peek();
    if (c == ' ' || c == '\t' || c == '\r') {
      advance();
    } else if (c == '\n') {
      advance();
      line_++;
      column_ = 1;
      currentLineStart_ = pos_;
    } else {
      break;
    }
  }
}

void Lexer::skipComment() {
  while (!isAtEnd() && peek() != '\n') {
    advance();
  }
}