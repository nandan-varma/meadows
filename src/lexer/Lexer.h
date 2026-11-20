#ifndef MEADOWS_LEXER_H
#define MEADOWS_LEXER_H

#include <string_view>
#include <vector>

#include "../utils/Diagnostics.h"
#include "Token.h"

namespace meadows {

class Lexer {
public:
  Lexer(std::string_view source, uint32_t file, DiagnosticSink& diagnostics);

  std::vector<Token> tokenize();

private:
  std::string_view source_;
  uint32_t file_;
  DiagnosticSink& diagnostics_;
  size_t pos_ = 0;
  uint32_t line_ = 1;
  uint32_t column_ = 1;

  bool atEnd() const { return pos_ >= source_.size(); }
  char peek(size_t ahead = 0) const;
  char advance();
  Location here() const { return {file_, line_, column_}; }
  Token make(TokenType type, std::string text, Location start) const;

  void skipTrivia();
  void lexToken(std::vector<Token>& tokens);
  Token lexWord(Location start);
  Token lexNumber(Location start);
  Token lexString(Location start);
};

} // namespace meadows

#endif
