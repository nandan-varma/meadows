#ifndef MEADOWS_TOKEN_H
#define MEADOWS_TOKEN_H

#include <string>

#include "../utils/SourceMap.h"

namespace meadows {

enum class TokenType {
  LET,
  FUNC,
  IF,
  ELSE,
  FOR,
  WHILE,
  RETURN,
  IN,
  RANGE,
  TRUE,
  FALSE,
  BREAK,
  CONTINUE,
  IMPORT,
  IDENTIFIER,
  STRING,
  NUMBER,
  PLUS,
  MINUS,
  STAR,
  SLASH,
  PERCENT,
  EQUAL,
  EQUAL_EQUAL,
  BANG,
  BANG_EQUAL,
  GREATER,
  LESS,
  GREATER_EQUAL,
  LESS_EQUAL,
  AND,
  OR,
  LEFT_PAREN,
  RIGHT_PAREN,
  LEFT_BRACE,
  RIGHT_BRACE,
  LEFT_BRACKET,
  RIGHT_BRACKET,
  COMMA,
  COLON,
  SEMICOLON,
  DOT,
  END_OF_FILE
};

struct Token {
  TokenType type;
  std::string text;
  Location location;
  uint32_t length;
};

const char* tokenTypeName(TokenType type);
std::string describeToken(const Token& token);

} // namespace meadows

#endif
