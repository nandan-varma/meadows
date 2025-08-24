#pragma once

#include <string>
#include <unordered_map>

namespace meadows {

enum class TokenType {
  // Literals
  IDENTIFIER,
  INTEGER,
  FLOAT,
  STRING,

  // Keywords
  DEF,
  IF,
  ELIF,
  ELSE,
  WHILE,
  FOR,
  IN,
  RETURN,
  BREAK,
  CONTINUE,
  PASS,
  TRUE,
  FALSE,
  NONE,
  CLASS,
  IMPORT,
  FROM,
  AS,
  AND,
  OR,
  NOT,

  // Operators
  PLUS,
  MINUS,
  MULTIPLY,
  DIVIDE,
  MODULO,
  POWER,
  ASSIGN,
  PLUS_ASSIGN,
  MINUS_ASSIGN,
  MULTIPLY_ASSIGN,
  DIVIDE_ASSIGN,

  // Comparison
  EQUAL,
  NOT_EQUAL,
  LESS_THAN,
  LESS_EQUAL,
  GREATER_THAN,
  GREATER_EQUAL,

  // Delimiters
  LEFT_PAREN,
  RIGHT_PAREN,
  LEFT_BRACKET,
  RIGHT_BRACKET,
  LEFT_BRACE,
  RIGHT_BRACE,
  COMMA,
  DOT,
  COLON,
  SEMICOLON,

  // Special
  NEWLINE,
  INDENT,
  DEDENT,
  EOF_TOKEN,

  // Error
  UNKNOWN
};

struct SourceLocation {
  int line;
  int column;
  std::string filename;

  SourceLocation(int line = 1, int column = 1, const std::string &filename = "")
      : line(line), column(column), filename(filename) {}
};

class Token {
public:
  TokenType type;
  std::string value;
  SourceLocation location;

  Token(TokenType type, const std::string &value,
        const SourceLocation &location)
      : type(type), value(value), location(location) {}

  Token() : type(TokenType::UNKNOWN), value(""), location() {}

  std::string toString() const;
  bool isKeyword() const;
  bool isOperator() const;
  bool isLiteral() const;
};

class TokenTypeUtil {
public:
  static std::unordered_map<std::string, TokenType> keywords;
  static std::unordered_map<TokenType, std::string> tokenNames;

  static TokenType getKeywordType(const std::string &word);
  static std::string getTokenName(TokenType type);
  static void initializeMaps();
};

} // namespace meadows
