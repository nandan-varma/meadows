#ifndef LEXER_H
#define LEXER_H

#include "Token.h"
#include <string>
#include <vector>

class Lexer {
public:
  Lexer(const std::string &source);
  std::vector<Token> tokenize();

private:
  std::string source;
  size_t pos;
  int line;

  char peek();
  char advance();
  bool isAtEnd();
  Token nextToken();
  Token identifier();
  Token number();
  Token string();
  void skipWhitespace();
  void skipComment();
};

#endif