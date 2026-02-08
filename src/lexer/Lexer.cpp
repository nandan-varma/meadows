#include "Lexer.h"
#include <cctype>
#include <stdexcept>
#include <unordered_map>

static std::unordered_map<std::string, TokenType> keywords = {
    {"let", TokenType::LET},     {"func", TokenType::FUNC},
    {"if", TokenType::IF},       {"else", TokenType::ELSE},
    {"for", TokenType::FOR},     {"while", TokenType::WHILE},
    {"print", TokenType::PRINT}, {"return", TokenType::RETURN},
    {"in", TokenType::IN},       {"range", TokenType::RANGE},
    {"true", TokenType::TRUE},   {"false", TokenType::FALSE},
    {"break", TokenType::BREAK}, {"continue", TokenType::CONTINUE}};

Lexer::Lexer(const std::string &src)
    : source(src), pos(0), line(1), column(1), currentLineStart(0) {}

std::vector<Token> Lexer::tokenize() {
  std::vector<Token> tokens;
  while (!isAtEnd()) {
    tokens.push_back(nextToken());
  }
  tokens.push_back(Token(TokenType::EOF_TOKEN, "", line, column));
  return tokens;
}

char Lexer::peek() {
  if (isAtEnd())
    return '\0';
  return source[pos];
}

char Lexer::advance() {
  char c = source[pos++];
  column++;
  return c;
}

bool Lexer::isAtEnd() { return pos >= source.size(); }

Token Lexer::nextToken() {
  skipWhitespace();
  if (isAtEnd())
    return Token(TokenType::EOF_TOKEN, "", line, column);

  int startColumn = column;
  char c = peek();
  if (c == '#') {
    skipComment();
    return nextToken();
  }
  if (c == '/' && pos + 1 < source.size() && source[pos + 1] == '/') {
    skipComment();
    return nextToken();
  }

  if (isalpha(c) || c == '_')
    return identifier();
  if (isdigit(c))
    return number();
  if (c == '"')
    return string();

  switch (c) {
  case '+':
    advance();
    return Token(TokenType::PLUS, "+", line, startColumn);
  case '-':
    advance();
    return Token(TokenType::MINUS, "-", line, startColumn);
  case '*':
    advance();
    return Token(TokenType::STAR, "*", line, startColumn);
  case '/':
    advance();
    return Token(TokenType::SLASH, "/", line, startColumn);
  case '=':
    advance();
    if (peek() == '=') {
      advance();
      return Token(TokenType::EQUAL_EQUAL, "==", line, startColumn);
    }
    return Token(TokenType::EQUAL, "=", line, startColumn);
  case '&':
    advance();
    if (peek() == '&') {
      advance();
      return Token(TokenType::AND, "&&", line, startColumn);
    }
    return Token(TokenType::IDENTIFIER, "&", line, startColumn);
  case '|':
    advance();
    if (peek() == '|') {
      advance();
      return Token(TokenType::OR, "||", line, startColumn);
    }
    return Token(TokenType::IDENTIFIER, "|", line, startColumn);
  case '>':
    advance();
    if (peek() == '=') {
      advance();
      return Token(TokenType::GREATER_EQUAL, ">=", line, startColumn);
    }
    return Token(TokenType::GREATER, ">", line, startColumn);
  case '<':
    advance();
    if (peek() == '=') {
      advance();
      return Token(TokenType::LESS_EQUAL, "<=", line, startColumn);
    }
    return Token(TokenType::LESS, "<", line, startColumn);
  case '!':
    advance();
    if (peek() == '=') {
      advance();
      return Token(TokenType::BANG_EQUAL, "!=", line, startColumn);
    }
    return Token(TokenType::BANG, "!", line, startColumn);
  case '(':
    advance();
    return Token(TokenType::LEFT_PAREN, "(", line, startColumn);
  case ')':
    advance();
    return Token(TokenType::RIGHT_PAREN, ")", line, startColumn);
  case '{':
    advance();
    return Token(TokenType::LEFT_BRACE, "{", line, startColumn);
  case '}':
    advance();
    return Token(TokenType::RIGHT_BRACE, "}", line, startColumn);
  case '[':
    advance();
    return Token(TokenType::LEFT_BRACKET, "[", line, startColumn);
  case ']':
    advance();
    return Token(TokenType::RIGHT_BRACKET, "]", line, startColumn);
  case ',':
    advance();
    return Token(TokenType::COMMA, ",", line, startColumn);
  case ':':
    advance();
    return Token(TokenType::COLON, ":", line, startColumn);
  case ';':
    advance();
    return Token(TokenType::SEMICOLON, ";", line, startColumn);
  case '.':
    advance();
    return Token(TokenType::DOT, ".", line, startColumn);
  }

  // Error
  advance();
  return Token(TokenType::IDENTIFIER, std::string(1, c), line,
               startColumn); // For now, treat as identifier
}

Token Lexer::identifier() {
  int startColumn = column;
  size_t start = pos;
  while (isalnum(peek()) || peek() == '_') {
    advance();
  }
  std::string value = source.substr(start, pos - start);
  auto it = keywords.find(value);
  if (it != keywords.end()) {
    return Token(it->second, value, line, startColumn);
  }
  return Token(TokenType::IDENTIFIER, value, line, startColumn);
}

Token Lexer::number() {
  int startColumn = column;
  size_t start = pos;
  while (isdigit(peek())) {
    advance();
  }
  std::string value = source.substr(start, pos - start);
  return Token(TokenType::NUMBER, value, line, startColumn);
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
  int startColumn = column;
  advance();
  size_t start = pos;
  int startLine = line;
  size_t nonEscapeCount = 0;
  while (!isAtEnd() && peek() != '"') {
    if (peek() == '\\' && pos + 1 < source.size()) {
      advance();
    }
    if (peek() == '\n') {
      line++;
      column = 1;
      currentLineStart = pos + 1;
    }
    advance();
    nonEscapeCount++;
  }
  if (isAtEnd()) {
    throw std::runtime_error("Unterminated string starting at line " +
                             std::to_string(startLine));
  }
  std::string value;
  value.reserve(nonEscapeCount);
  for (size_t i = start; i < pos; ++i) {
    if (source[i] == '\\' && i + 1 < pos) {
      value += unescapeChar(source[i + 1]);
      ++i;
    } else {
      value += source[i];
    }
  }
  advance();
  return Token(TokenType::STRING, value, line, startColumn);
}

void Lexer::skipWhitespace() {
  while (!isAtEnd()) {
    char c = peek();
    if (c == ' ' || c == '\t' || c == '\r') {
      advance();
    } else if (c == '\n') {
      advance();
      line++;
      column = 1;
      currentLineStart = pos;
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