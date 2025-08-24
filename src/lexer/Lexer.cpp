#include "Lexer.h"
#include <iostream>
#include <sstream>

namespace meadows {

Lexer::Lexer(const std::string &source, const std::string &filename)
    : source(source), filename(filename), current(0), line(1), column(1),
      atLineStart(true) {
  indentStack.push(0); // Base indentation level
}

char Lexer::peek(int offset) const {
  size_t pos = current + offset;
  if (pos >= source.length()) {
    return '\0';
  }
  return source[pos];
}

char Lexer::advance() {
  if (isAtEnd()) {
    return '\0';
  }

  char c = source[current++];
  if (c == '\n') {
    line++;
    column = 1;
    atLineStart = true;
  } else {
    column++;
    if (c != ' ' && c != '\t') {
      atLineStart = false;
    }
  }
  return c;
}

void Lexer::skipWhitespace() {
  while (!isAtEnd()) {
    char c = peek();
    if (c == ' ' || c == '\t' || c == '\r') {
      advance();
    } else {
      break;
    }
  }
}

void Lexer::skipComment() {
  if (peek() == '#') {
    while (peek() != '\n' && !isAtEnd()) {
      advance();
    }
  }
}

bool Lexer::isAlpha(char c) const {
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

bool Lexer::isDigit(char c) const { return c >= '0' && c <= '9'; }

bool Lexer::isAlphaNumeric(char c) const { return isAlpha(c) || isDigit(c); }

bool Lexer::isAtEnd() const { return current >= source.length(); }

Token Lexer::makeNumber() {
  SourceLocation location(line, column, filename);
  std::string value;
  bool isFloat = false;

  // Read integer part
  while (isDigit(peek())) {
    value += advance();
  }

  // Check for decimal point
  if (peek() == '.' && isDigit(peek(1))) {
    isFloat = true;
    value += advance(); // consume '.'
    while (isDigit(peek())) {
      value += advance();
    }
  }

  TokenType type = isFloat ? TokenType::FLOAT : TokenType::INTEGER;
  return Token(type, value, location);
}

Token Lexer::makeString(char quote) {
  SourceLocation location(line, column, filename);
  std::string value;
  advance(); // consume opening quote

  while (peek() != quote && !isAtEnd()) {
    if (peek() == '\\') {
      advance(); // consume backslash
      char escaped = advance();
      switch (escaped) {
      case 'n':
        value += '\n';
        break;
      case 't':
        value += '\t';
        break;
      case 'r':
        value += '\r';
        break;
      case '\\':
        value += '\\';
        break;
      case '\'':
        value += '\'';
        break;
      case '\"':
        value += '\"';
        break;
      default:
        value += escaped;
        break;
      }
    } else {
      value += advance();
    }
  }

  if (isAtEnd()) {
    // TODO: Better error handling
    return Token(TokenType::UNKNOWN, "Unterminated string", location);
  }

  advance(); // consume closing quote
  return Token(TokenType::STRING, value, location);
}

Token Lexer::makeIdentifier() {
  SourceLocation location(line, column, filename);
  std::string value;

  while (isAlphaNumeric(peek())) {
    value += advance();
  }

  TokenType type = TokenTypeUtil::getKeywordType(value);
  return Token(type, value, location);
}

Token Lexer::makeOperator() {
  SourceLocation location(line, column, filename);
  char c = advance();

  switch (c) {
  case '+':
    if (peek() == '=') {
      advance();
      return Token(TokenType::PLUS_ASSIGN, "+=", location);
    }
    return Token(TokenType::PLUS, "+", location);
  case '-':
    if (peek() == '=') {
      advance();
      return Token(TokenType::MINUS_ASSIGN, "-=", location);
    }
    return Token(TokenType::MINUS, "-", location);
  case '*':
    if (peek() == '*') {
      advance();
      return Token(TokenType::POWER, "**", location);
    } else if (peek() == '=') {
      advance();
      return Token(TokenType::MULTIPLY_ASSIGN, "*=", location);
    }
    return Token(TokenType::MULTIPLY, "*", location);
  case '/':
    if (peek() == '=') {
      advance();
      return Token(TokenType::DIVIDE_ASSIGN, "/=", location);
    }
    return Token(TokenType::DIVIDE, "/", location);
  case '%':
    return Token(TokenType::MODULO, "%", location);
  case '=':
    if (peek() == '=') {
      advance();
      return Token(TokenType::EQUAL, "==", location);
    }
    return Token(TokenType::ASSIGN, "=", location);
  case '!':
    if (peek() == '=') {
      advance();
      return Token(TokenType::NOT_EQUAL, "!=", location);
    }
    return Token(TokenType::UNKNOWN, "!", location);
  case '<':
    if (peek() == '=') {
      advance();
      return Token(TokenType::LESS_EQUAL, "<=", location);
    }
    return Token(TokenType::LESS_THAN, "<", location);
  case '>':
    if (peek() == '=') {
      advance();
      return Token(TokenType::GREATER_EQUAL, ">=", location);
    }
    return Token(TokenType::GREATER_THAN, ">", location);
  case '(':
    return Token(TokenType::LEFT_PAREN, "(", location);
  case ')':
    return Token(TokenType::RIGHT_PAREN, ")", location);
  case '[':
    return Token(TokenType::LEFT_BRACKET, "[", location);
  case ']':
    return Token(TokenType::RIGHT_BRACKET, "]", location);
  case '{':
    return Token(TokenType::LEFT_BRACE, "{", location);
  case '}':
    return Token(TokenType::RIGHT_BRACE, "}", location);
  case ',':
    return Token(TokenType::COMMA, ",", location);
  case '.':
    return Token(TokenType::DOT, ".", location);
  case ':':
    return Token(TokenType::COLON, ":", location);
  case ';':
    return Token(TokenType::SEMICOLON, ";", location);
  default:
    return Token(TokenType::UNKNOWN, std::string(1, c), location);
  }
}

int Lexer::countIndentation() {
  int count = 0;
  size_t pos = current;

  while (pos < source.length() && (source[pos] == ' ' || source[pos] == '\t')) {
    if (source[pos] == ' ') {
      count++;
    } else if (source[pos] == '\t') {
      count += 8; // Assume tab = 8 spaces
    }
    pos++;
  }

  return count;
}

void Lexer::handleIndentation() {
  if (!atLineStart)
    return;

  int currentIndent = countIndentation();
  int previousIndent = indentStack.top();

  if (currentIndent > previousIndent) {
    // Increased indentation
    indentStack.push(currentIndent);
    pendingTokens.push_back(Token(TokenType::INDENT, "", getCurrentLocation()));
  } else if (currentIndent < previousIndent) {
    // Decreased indentation
    while (!indentStack.empty() && indentStack.top() > currentIndent) {
      indentStack.pop();
      pendingTokens.push_back(
          Token(TokenType::DEDENT, "", getCurrentLocation()));
    }

    if (indentStack.empty() || indentStack.top() != currentIndent) {
      // Indentation error
      pendingTokens.push_back(
          Token(TokenType::UNKNOWN, "Indentation error", getCurrentLocation()));
    }
  }
}

Token Lexer::nextToken() {
  // Return pending tokens first
  if (!pendingTokens.empty()) {
    Token token = pendingTokens.front();
    pendingTokens.erase(pendingTokens.begin());
    return token;
  }

  // Handle indentation at line start
  if (atLineStart) {
    handleIndentation();
    if (!pendingTokens.empty()) {
      Token token = pendingTokens.front();
      pendingTokens.erase(pendingTokens.begin());
      return token;
    }
  }

  skipWhitespace();
  skipComment();

  if (isAtEnd()) {
    // Generate DEDENT tokens for remaining indentation levels
    while (indentStack.size() > 1) {
      indentStack.pop();
      return Token(TokenType::DEDENT, "", getCurrentLocation());
    }
    return Token(TokenType::EOF_TOKEN, "", getCurrentLocation());
  }

  char c = peek();

  // Handle newlines
  if (c == '\n') {
    SourceLocation location = getCurrentLocation();
    advance();
    return Token(TokenType::NEWLINE, "\\n", location);
  }

  // Numbers
  if (isDigit(c)) {
    return makeNumber();
  }

  // Strings
  if (c == '"' || c == '\'') {
    return makeString(c);
  }

  // Identifiers and keywords
  if (isAlpha(c)) {
    return makeIdentifier();
  }

  // Operators and punctuation
  return makeOperator();
}

std::vector<Token> Lexer::tokenize() {
  std::vector<Token> tokens;

  while (true) {
    Token token = nextToken();
    tokens.push_back(token);
    if (token.type == TokenType::EOF_TOKEN) {
      break;
    }
  }

  return tokens;
}

SourceLocation Lexer::getCurrentLocation() const {
  return SourceLocation(line, column, filename);
}

std::string Lexer::getErrorContext(const SourceLocation &location,
                                   int contextLines) const {
  std::istringstream stream(source);
  std::string line;
  std::vector<std::string> lines;

  // Read all lines
  while (std::getline(stream, line)) {
    lines.push_back(line);
  }

  if (location.line < 1 || location.line > static_cast<int>(lines.size())) {
    return "Invalid line number";
  }

  std::ostringstream result;
  int startLine = std::max(1, location.line - contextLines);
  int endLine =
      std::min(static_cast<int>(lines.size()), location.line + contextLines);

  for (int i = startLine; i <= endLine; i++) {
    result << i << ": " << lines[i - 1] << "\n";
    if (i == location.line) {
      result << std::string(
                    std::to_string(i).length() + 2 + location.column - 1, ' ')
             << "^\n";
    }
  }

  return result.str();
}

} // namespace meadows
