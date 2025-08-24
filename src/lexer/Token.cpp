#include "Token.h"

namespace meadows {

std::unordered_map<std::string, TokenType> TokenTypeUtil::keywords;
std::unordered_map<TokenType, std::string> TokenTypeUtil::tokenNames;

void TokenTypeUtil::initializeMaps() {
  if (!keywords.empty())
    return; // Already initialized

  // Keywords
  keywords["def"] = TokenType::DEF;
  keywords["if"] = TokenType::IF;
  keywords["elif"] = TokenType::ELIF;
  keywords["else"] = TokenType::ELSE;
  keywords["while"] = TokenType::WHILE;
  keywords["for"] = TokenType::FOR;
  keywords["in"] = TokenType::IN;
  keywords["return"] = TokenType::RETURN;
  keywords["break"] = TokenType::BREAK;
  keywords["continue"] = TokenType::CONTINUE;
  keywords["pass"] = TokenType::PASS;
  keywords["True"] = TokenType::TRUE;
  keywords["False"] = TokenType::FALSE;
  keywords["None"] = TokenType::NONE;
  keywords["class"] = TokenType::CLASS;
  keywords["import"] = TokenType::IMPORT;
  keywords["from"] = TokenType::FROM;
  keywords["as"] = TokenType::AS;
  keywords["and"] = TokenType::AND;
  keywords["or"] = TokenType::OR;
  keywords["not"] = TokenType::NOT;

  // Token names
  tokenNames[TokenType::IDENTIFIER] = "IDENTIFIER";
  tokenNames[TokenType::INTEGER] = "INTEGER";
  tokenNames[TokenType::FLOAT] = "FLOAT";
  tokenNames[TokenType::STRING] = "STRING";
  tokenNames[TokenType::DEF] = "DEF";
  tokenNames[TokenType::IF] = "IF";
  tokenNames[TokenType::ELIF] = "ELIF";
  tokenNames[TokenType::ELSE] = "ELSE";
  tokenNames[TokenType::WHILE] = "WHILE";
  tokenNames[TokenType::FOR] = "FOR";
  tokenNames[TokenType::IN] = "IN";
  tokenNames[TokenType::RETURN] = "RETURN";
  tokenNames[TokenType::BREAK] = "BREAK";
  tokenNames[TokenType::CONTINUE] = "CONTINUE";
  tokenNames[TokenType::PASS] = "PASS";
  tokenNames[TokenType::TRUE] = "TRUE";
  tokenNames[TokenType::FALSE] = "FALSE";
  tokenNames[TokenType::NONE] = "NONE";
  tokenNames[TokenType::CLASS] = "CLASS";
  tokenNames[TokenType::IMPORT] = "IMPORT";
  tokenNames[TokenType::FROM] = "FROM";
  tokenNames[TokenType::AS] = "AS";
  tokenNames[TokenType::AND] = "AND";
  tokenNames[TokenType::OR] = "OR";
  tokenNames[TokenType::NOT] = "NOT";
  tokenNames[TokenType::PLUS] = "PLUS";
  tokenNames[TokenType::MINUS] = "MINUS";
  tokenNames[TokenType::MULTIPLY] = "MULTIPLY";
  tokenNames[TokenType::DIVIDE] = "DIVIDE";
  tokenNames[TokenType::MODULO] = "MODULO";
  tokenNames[TokenType::POWER] = "POWER";
  tokenNames[TokenType::ASSIGN] = "ASSIGN";
  tokenNames[TokenType::PLUS_ASSIGN] = "PLUS_ASSIGN";
  tokenNames[TokenType::MINUS_ASSIGN] = "MINUS_ASSIGN";
  tokenNames[TokenType::MULTIPLY_ASSIGN] = "MULTIPLY_ASSIGN";
  tokenNames[TokenType::DIVIDE_ASSIGN] = "DIVIDE_ASSIGN";
  tokenNames[TokenType::EQUAL] = "EQUAL";
  tokenNames[TokenType::NOT_EQUAL] = "NOT_EQUAL";
  tokenNames[TokenType::LESS_THAN] = "LESS_THAN";
  tokenNames[TokenType::LESS_EQUAL] = "LESS_EQUAL";
  tokenNames[TokenType::GREATER_THAN] = "GREATER_THAN";
  tokenNames[TokenType::GREATER_EQUAL] = "GREATER_EQUAL";
  tokenNames[TokenType::LEFT_PAREN] = "LEFT_PAREN";
  tokenNames[TokenType::RIGHT_PAREN] = "RIGHT_PAREN";
  tokenNames[TokenType::LEFT_BRACKET] = "LEFT_BRACKET";
  tokenNames[TokenType::RIGHT_BRACKET] = "RIGHT_BRACKET";
  tokenNames[TokenType::LEFT_BRACE] = "LEFT_BRACE";
  tokenNames[TokenType::RIGHT_BRACE] = "RIGHT_BRACE";
  tokenNames[TokenType::COMMA] = "COMMA";
  tokenNames[TokenType::DOT] = "DOT";
  tokenNames[TokenType::COLON] = "COLON";
  tokenNames[TokenType::SEMICOLON] = "SEMICOLON";
  tokenNames[TokenType::NEWLINE] = "NEWLINE";
  tokenNames[TokenType::INDENT] = "INDENT";
  tokenNames[TokenType::DEDENT] = "DEDENT";
  tokenNames[TokenType::EOF_TOKEN] = "EOF";
  tokenNames[TokenType::UNKNOWN] = "UNKNOWN";
}

TokenType TokenTypeUtil::getKeywordType(const std::string &word) {
  initializeMaps();
  auto it = keywords.find(word);
  return (it != keywords.end()) ? it->second : TokenType::IDENTIFIER;
}

std::string TokenTypeUtil::getTokenName(TokenType type) {
  initializeMaps();
  auto it = tokenNames.find(type);
  return (it != tokenNames.end()) ? it->second : "UNKNOWN";
}

std::string Token::toString() const {
  return TokenTypeUtil::getTokenName(type) + "(" + value + ") at " +
         std::to_string(location.line) + ":" + std::to_string(location.column);
}

bool Token::isKeyword() const {
  return type >= TokenType::DEF && type <= TokenType::NOT;
}

bool Token::isOperator() const {
  return type >= TokenType::PLUS && type <= TokenType::GREATER_EQUAL;
}

bool Token::isLiteral() const {
  return type >= TokenType::IDENTIFIER && type <= TokenType::STRING;
}

} // namespace meadows
