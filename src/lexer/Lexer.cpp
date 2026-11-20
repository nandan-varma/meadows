#include "Lexer.h"

#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <unordered_map>

#include "../runtime/Limits.h"

namespace meadows {

namespace {

const std::unordered_map<std::string_view, TokenType> KEYWORDS = {
    {"let", TokenType::LET},
    {"func", TokenType::FUNC},
    {"if", TokenType::IF},
    {"else", TokenType::ELSE},
    {"for", TokenType::FOR},
    {"while", TokenType::WHILE},
    {"return", TokenType::RETURN},
    {"in", TokenType::IN},
    {"range", TokenType::RANGE},
    {"true", TokenType::TRUE},
    {"false", TokenType::FALSE},
    {"break", TokenType::BREAK},
    {"continue", TokenType::CONTINUE},
    {"import", TokenType::IMPORT}};

bool isDigit(char c) {
  return c >= '0' && c <= '9';
}

bool isWordStart(char c) {
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

bool isWordPart(char c) {
  return isWordStart(c) || isDigit(c);
}

} // namespace

const char* tokenTypeName(TokenType type) {
  switch (type) {
  case TokenType::LET:
    return "LET";
  case TokenType::FUNC:
    return "FUNC";
  case TokenType::IF:
    return "IF";
  case TokenType::ELSE:
    return "ELSE";
  case TokenType::FOR:
    return "FOR";
  case TokenType::WHILE:
    return "WHILE";
  case TokenType::RETURN:
    return "RETURN";
  case TokenType::IN:
    return "IN";
  case TokenType::RANGE:
    return "RANGE";
  case TokenType::TRUE:
    return "TRUE";
  case TokenType::FALSE:
    return "FALSE";
  case TokenType::BREAK:
    return "BREAK";
  case TokenType::CONTINUE:
    return "CONTINUE";
  case TokenType::IMPORT:
    return "IMPORT";
  case TokenType::IDENTIFIER:
    return "IDENTIFIER";
  case TokenType::STRING:
    return "STRING";
  case TokenType::NUMBER:
    return "NUMBER";
  case TokenType::PLUS:
    return "PLUS";
  case TokenType::MINUS:
    return "MINUS";
  case TokenType::STAR:
    return "STAR";
  case TokenType::SLASH:
    return "SLASH";
  case TokenType::PERCENT:
    return "PERCENT";
  case TokenType::EQUAL:
    return "EQUAL";
  case TokenType::EQUAL_EQUAL:
    return "EQUAL_EQUAL";
  case TokenType::BANG:
    return "BANG";
  case TokenType::BANG_EQUAL:
    return "BANG_EQUAL";
  case TokenType::GREATER:
    return "GREATER";
  case TokenType::LESS:
    return "LESS";
  case TokenType::GREATER_EQUAL:
    return "GREATER_EQUAL";
  case TokenType::LESS_EQUAL:
    return "LESS_EQUAL";
  case TokenType::AND:
    return "AND";
  case TokenType::OR:
    return "OR";
  case TokenType::LEFT_PAREN:
    return "LEFT_PAREN";
  case TokenType::RIGHT_PAREN:
    return "RIGHT_PAREN";
  case TokenType::LEFT_BRACE:
    return "LEFT_BRACE";
  case TokenType::RIGHT_BRACE:
    return "RIGHT_BRACE";
  case TokenType::LEFT_BRACKET:
    return "LEFT_BRACKET";
  case TokenType::RIGHT_BRACKET:
    return "RIGHT_BRACKET";
  case TokenType::COMMA:
    return "COMMA";
  case TokenType::COLON:
    return "COLON";
  case TokenType::SEMICOLON:
    return "SEMICOLON";
  case TokenType::DOT:
    return "DOT";
  case TokenType::END_OF_FILE:
    return "END_OF_FILE";
  }
  return "UNKNOWN";
}

std::string describeToken(const Token& token) {
  if (token.type == TokenType::END_OF_FILE) {
    return "end of file";
  }
  if (token.type == TokenType::STRING) {
    return "string literal";
  }
  return "'" + token.text + "'";
}

Lexer::Lexer(std::string_view source, uint32_t file,
             DiagnosticSink& diagnostics)
    : source_(source), file_(file), diagnostics_(diagnostics) {}

char Lexer::peek(size_t ahead) const {
  return pos_ + ahead < source_.size() ? source_[pos_ + ahead] : '\0';
}

char Lexer::advance() {
  char c = source_[pos_++];
  if (c == '\n') {
    ++line_;
    column_ = 1;
  } else {
    ++column_;
  }
  return c;
}

Token Lexer::make(TokenType type, std::string text, Location start) const {
  uint32_t length = line_ == start.line ? column_ - start.column : 1;
  return Token{type, std::move(text), start, length};
}

std::vector<Token> Lexer::tokenize() {
  std::vector<Token> tokens;
  while (true) {
    skipTrivia();
    if (atEnd() || diagnostics_.full()) {
      break;
    }
    lexToken(tokens);
  }
  tokens.push_back(Token{TokenType::END_OF_FILE, "", here(), 0});
  return tokens;
}

void Lexer::skipTrivia() {
  while (!atEnd()) {
    char c = peek();
    if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
      advance();
    } else if (c == '#' || (c == '/' && peek(1) == '/')) {
      while (!atEnd() && peek() != '\n') {
        advance();
      }
    } else {
      return;
    }
  }
}

void Lexer::lexToken(std::vector<Token>& tokens) {
  Location start = here();
  char c = peek();

  if (isWordStart(c)) {
    tokens.push_back(lexWord(start));
    return;
  }
  if (isDigit(c)) {
    tokens.push_back(lexNumber(start));
    return;
  }
  if (c == '"') {
    tokens.push_back(lexString(start));
    return;
  }

  advance();
  auto two = [&](char next, TokenType pair, TokenType single) {
    if (peek() == next) {
      advance();
      tokens.push_back(make(pair, std::string{c, next}, start));
    } else {
      tokens.push_back(make(single, std::string(1, c), start));
    }
  };

  switch (c) {
  case '+':
    tokens.push_back(make(TokenType::PLUS, "+", start));
    break;
  case '-':
    tokens.push_back(make(TokenType::MINUS, "-", start));
    break;
  case '*':
    tokens.push_back(make(TokenType::STAR, "*", start));
    break;
  case '/':
    tokens.push_back(make(TokenType::SLASH, "/", start));
    break;
  case '%':
    tokens.push_back(make(TokenType::PERCENT, "%", start));
    break;
  case '(':
    tokens.push_back(make(TokenType::LEFT_PAREN, "(", start));
    break;
  case ')':
    tokens.push_back(make(TokenType::RIGHT_PAREN, ")", start));
    break;
  case '{':
    tokens.push_back(make(TokenType::LEFT_BRACE, "{", start));
    break;
  case '}':
    tokens.push_back(make(TokenType::RIGHT_BRACE, "}", start));
    break;
  case '[':
    tokens.push_back(make(TokenType::LEFT_BRACKET, "[", start));
    break;
  case ']':
    tokens.push_back(make(TokenType::RIGHT_BRACKET, "]", start));
    break;
  case ',':
    tokens.push_back(make(TokenType::COMMA, ",", start));
    break;
  case ':':
    tokens.push_back(make(TokenType::COLON, ":", start));
    break;
  case ';':
    tokens.push_back(make(TokenType::SEMICOLON, ";", start));
    break;
  case '.':
    tokens.push_back(make(TokenType::DOT, ".", start));
    break;
  case '=':
    two('=', TokenType::EQUAL_EQUAL, TokenType::EQUAL);
    break;
  case '!':
    two('=', TokenType::BANG_EQUAL, TokenType::BANG);
    break;
  case '<':
    two('=', TokenType::LESS_EQUAL, TokenType::LESS);
    break;
  case '>':
    two('=', TokenType::GREATER_EQUAL, TokenType::GREATER);
    break;
  case '&':
  case '|':
    if (peek() == c) {
      advance();
      tokens.push_back(make(c == '&' ? TokenType::AND : TokenType::OR,
                            std::string(2, c), start));
    } else {
      diagnostics_.report(ErrorCode::LEX_INVALID_CHARACTER,
                          std::string("Unexpected character '") + c + "'",
                          start, 1,
                          std::string("Did you mean '") + c + c + "'?");
    }
    break;
  default: {
    char shown[8];
    if (std::isprint(static_cast<unsigned char>(c))) {
      std::snprintf(shown, sizeof(shown), "%c", c);
    } else {
      std::snprintf(shown, sizeof(shown), "\\x%02x",
                    static_cast<unsigned char>(c));
    }
    diagnostics_.report(ErrorCode::LEX_INVALID_CHARACTER,
                        std::string("Unexpected character '") + shown + "'",
                        start);
  }
  }
}

Token Lexer::lexWord(Location start) {
  size_t begin = pos_;
  while (isWordPart(peek())) {
    advance();
  }
  std::string_view word = source_.substr(begin, pos_ - begin);
  auto keyword = KEYWORDS.find(word);
  return make(keyword != KEYWORDS.end() ? keyword->second
                                        : TokenType::IDENTIFIER,
              std::string(word), start);
}

Token Lexer::lexNumber(Location start) {
  size_t begin = pos_;
  while (isDigit(peek())) {
    advance();
  }
  bool isFloat = peek() == '.' && isDigit(peek(1));
  if (isFloat) {
    advance();
    while (isDigit(peek())) {
      advance();
    }
  }
  std::string text(source_.substr(begin, pos_ - begin));
  Token token = make(TokenType::NUMBER, text, start);

  bool inRange =
      isFloat ? std::isfinite(std::strtod(text.c_str(), nullptr))
              : text.size() < 10 || (text.size() == 10 && text <= "2147483648");
  if (!inRange) {
    diagnostics_.report(ErrorCode::LEX_NUMBER_OUT_OF_RANGE,
                        "Number literal '" + text + "' is out of range", start,
                        token.length);
    token.text = isFloat ? "0.0" : "0";
  }
  return token;
}

Token Lexer::lexString(Location start) {
  advance();
  std::string value;
  while (!atEnd() && peek() != '"' && peek() != '\n') {
    Location at = here();
    char c = advance();
    if (c != '\\') {
      value += c;
      continue;
    }
    if (atEnd()) {
      break;
    }
    if (peek() == '\n') {
      break;
    }
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
    case '0':
      value += '\0';
      break;
    case 'b':
      value += '\b';
      break;
    case 'f':
      value += '\f';
      break;
    case '\\':
      value += '\\';
      break;
    case '"':
      value += '"';
      break;
    default:
      diagnostics_.report(
          ErrorCode::LEX_INVALID_ESCAPE,
          std::string("Invalid escape sequence '\\") + escaped + "'", at, 2);
    }
  }
  if (peek() == '"') {
    advance();
  } else {
    diagnostics_.report(ErrorCode::LEX_UNTERMINATED_STRING,
                        "Unterminated string literal", start, 1,
                        "String literals cannot span lines; use \\n");
  }
  if (value.size() > MEADOWS_MAX_STRING_LENGTH) {
    diagnostics_.report(ErrorCode::LEX_STRING_TOO_LONG,
                        "String literal exceeds the maximum length of " +
                            std::to_string(MEADOWS_MAX_STRING_LENGTH) +
                            " bytes",
                        start);
    value.clear();
  }
  return make(TokenType::STRING, std::move(value), start);
}

} // namespace meadows
