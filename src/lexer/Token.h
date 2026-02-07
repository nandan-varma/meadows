#ifndef TOKEN_H
#define TOKEN_H

#include <string>

enum class TokenType {
  // Keywords
  LET,
  FUNC,
  IF,
  ELSE,
  FOR,
  WHILE,
  RETURN,
  PRINT,
  IN,
  RANGE,
  TRUE,
  FALSE,

  // Literals
  IDENTIFIER,
  STRING,
  NUMBER,

  // Operators
  PLUS,
  MINUS,
  STAR,
  SLASH,
  EQUAL,
  EQUAL_EQUAL,
  BANG,
  BANG_EQUAL,
  GREATER,
  LESS,
  GREATER_EQUAL,
  LESS_EQUAL,

  // Punctuation
  LEFT_PAREN,
  RIGHT_PAREN,
  LEFT_BRACE,
  RIGHT_BRACE,
  LEFT_BRACKET,
  RIGHT_BRACKET,
  COMMA,
  COLON,
  SEMICOLON,

  // Comments
  COMMENT,

  // End of file
  EOF_TOKEN
};

struct Token {
  TokenType type;
  std::string value;
  int line;

  Token(TokenType t, const std::string &v, int l)
      : type(t), value(v), line(l) {}
};

#endif