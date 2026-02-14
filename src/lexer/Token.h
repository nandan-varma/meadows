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
  BREAK,
  CONTINUE,

  // Module system
  MODULE,
  IMPORT,
  EXPORT,
  AS,
  FROM,

  // Type system
  TYPE,
  I32,
  I64,
  F32,
  F64,
  BOOL,
  ARROW,

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
  AND,
  OR,

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
  DOT,
  EXTERN,

  // Comments
  COMMENT,

  // End of file
  EOF_TOKEN
};

struct Token {
  TokenType type;
  std::string value;
  int line;
  int column;

  Token(TokenType t, const std::string &v, int l, int c = 0)
      : type(t), value(v), line(l), column(c) {}
};

#endif