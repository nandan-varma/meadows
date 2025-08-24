#pragma once

#include "Token.h"
#include <stack>
#include <string>
#include <vector>

namespace meadows {

class Lexer {
private:
  std::string source;
  std::string filename;
  size_t current;
  int line;
  int column;
  std::stack<int> indentStack;
  bool atLineStart;
  std::vector<Token> pendingTokens;

  char peek(int offset = 0) const;
  char advance();
  void skipWhitespace();
  void skipComment();

  Token makeNumber();
  Token makeString(char quote);
  Token makeIdentifier();
  Token makeOperator();

  void handleIndentation();
  int countIndentation();

  bool isAlpha(char c) const;
  bool isDigit(char c) const;
  bool isAlphaNumeric(char c) const;
  bool isAtEnd() const;

public:
  Lexer(const std::string &source, const std::string &filename = "");

  Token nextToken();
  std::vector<Token> tokenize();

  // For error reporting
  SourceLocation getCurrentLocation() const;
  std::string getErrorContext(const SourceLocation &location,
                              int contextLines = 2) const;
};

} // namespace meadows
