#ifndef LEXER_H
#define LEXER_H

#include <string>
#include <vector>

#include "Token.h"

/**
 * @class Lexer
 * @brief Performs lexical analysis on Meadows source code.
 *
 * The Lexer converts raw source code into a sequence of tokens that can be
 * parsed by the Parser. It identifies keywords, literals, operators, and
 * punctuation while tracking line numbers for error reporting.
 *
 * @ Responsibility
 * - Tokenize source code into lexical tokens
 * - Recognize and categorize language keywords
 * - Parse numeric and string literals
 * - Handle comments (both # and // styles)
 * - Track line numbers for error messages
 */
class Lexer {
public:
  /**
   * @brief Constructs a Lexer for the given source code.
   * @param source The Meadows source code to tokenize.
   */
  Lexer(const std::string &source);

  int getColumn() const { return column_; }

  /**
   * @brief Tokenizes the entire source code.
   * @return A vector of tokens representing the tokenized source.
   * @throws std::runtime_error If an unterminated string is encountered.
   */
  std::vector<Token> tokenize();

private:
  std::string source_;
  size_t pos_;
  int line_;
  int column_;
  int currentLineStart_;

  char peek() const;
  char peekNext() const;
  char advance();
  bool isAtEnd() const;
  Token nextToken();
  Token handleOperator(char c, int startColumn);
  Token identifier();
  Token number();
  Token string();
  void skipWhitespace();
  void skipComment();
};

#endif