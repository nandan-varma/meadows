#include "TOMLParser.h"
#include <cctype>
#include <stdexcept>

namespace meadows {
namespace config {

TOMLValue TOMLValue::makeString(const std::string &s) {
  TOMLValue v;
  v.type = STRING;
  v.stringValue = s;
  return v;
}

TOMLValue TOMLValue::makeInteger(long long i) {
  TOMLValue v;
  v.type = INTEGER;
  v.intValue = i;
  return v;
}

TOMLValue TOMLValue::makeBoolean(bool b) {
  TOMLValue v;
  v.type = BOOLEAN;
  v.boolValue = b;
  return v;
}

TOMLValue TOMLValue::makeTable() {
  TOMLValue v;
  v.type = TABLE;
  return v;
}

TOMLValue TOMLValue::makeArray() {
  TOMLValue v;
  v.type = ARRAY;
  return v;
}

std::string TOMLValue::asString() const {
  if (type == STRING)
    return stringValue;
  if (type == INTEGER)
    return std::to_string(intValue);
  if (type == BOOLEAN)
    return boolValue ? "true" : "false";
  return "";
}

long long TOMLValue::asInteger() const {
  if (type == INTEGER)
    return intValue;
  if (type == STRING) {
    try {
      return std::stoll(stringValue);
    } catch (...) {
      return 0;
    }
  }
  return 0;
}

bool TOMLValue::asBoolean() const {
  if (type == BOOLEAN)
    return boolValue;
  if (type == INTEGER)
    return intValue != 0;
  return false;
}

TOMLParser::TOMLParser() : pos_(0), line_(1), column_(1) {}

TOMLValue TOMLParser::parse(const std::string &content) {
  content_ = content;
  pos_ = 0;
  line_ = 1;
  column_ = 1;
  error_.clear();

  TOMLValue root = TOMLValue::makeTable();
  TOMLValue *currentTable = &root;

  while (!isAtEnd()) {
    skipWhitespace();
    skipComments();

    if (isAtEnd())
      break;

    // Skip newlines
    if (peek() == '\n') {
      advance();
      continue;
    }

    // Skip blank lines (whitespace-only lines)
    if (isAtEnd())
      break;

    // Check for table header
    if (peek() == '[') {
      bool isArray = false;
      std::vector<std::string> path;
      parseTableHeader(path, isArray);

      if (!success())
        return root;

      // Navigate to or create the table
      TOMLValue *table = &root;
      for (size_t i = 0; i < path.size(); ++i) {
        if (table->tableValue.find(path[i]) == table->tableValue.end()) {
          if (i == path.size() - 1 && isArray) {
            table->tableValue[path[i]] = TOMLValue::makeArray();
          } else {
            table->tableValue[path[i]] = TOMLValue::makeTable();
          }
        }

        if (i == path.size() - 1 && isArray) {
          // Add new table to array
          table->tableValue[path[i]].arrayValue.push_back(
              TOMLValue::makeTable());
          table = &table->tableValue[path[i]].arrayValue.back();
        } else {
          table = &table->tableValue[path[i]];
        }
      }
      currentTable = table;
    } else {
      // Parse key-value pair
      std::string key = parseKey();
      if (!success())
        return root;

      skipWhitespace();
      if (!match('=')) {
        setError("Expected '=' after key");
        return root;
      }
      skipWhitespace();

      TOMLValue value = parseValue();
      if (!success())
        return root;

      currentTable->tableValue[key] = std::move(value);
    }

    skipWhitespace();
    skipComments();
    consumeNewline();
  }

  return root;
}

void TOMLParser::skipWhitespace() {
  while (!isAtEnd()) {
    char c = peek();
    if (c == ' ' || c == '\t' || c == '\r') {
      advance();
    } else {
      break;
    }
  }
}

void TOMLParser::skipComments() {
  if (peek() == '#') {
    while (!isAtEnd() && peek() != '\n') {
      advance();
    }
  }
}

void TOMLParser::advance() {
  if (!isAtEnd()) {
    if (content_[pos_] == '\n') {
      line_++;
      column_ = 1;
    } else {
      column_++;
    }
    pos_++;
  }
}

char TOMLParser::peek() const {
  if (isAtEnd())
    return '\0';
  return content_[pos_];
}

char TOMLParser::peekNext() const {
  if (pos_ + 1 >= content_.size())
    return '\0';
  return content_[pos_ + 1];
}

bool TOMLParser::match(char expected) {
  if (peek() == expected) {
    advance();
    return true;
  }
  return false;
}

bool TOMLParser::isAtEnd() const { return pos_ >= content_.size(); }

void TOMLParser::consumeNewline() {
  if (peek() == '\n') {
    advance();
  }
}

std::string TOMLParser::parseKey() {
  skipWhitespace();

  std::string key;

  // Check for quoted key
  if (peek() == '"') {
    return parseString();
  }

  // Unquoted key
  while (!isAtEnd()) {
    char c = peek();
    if (std::isalnum(c) || c == '_' || c == '-') {
      key += c;
      advance();
    } else {
      break;
    }
  }

  if (key.empty()) {
    setError("Expected key");
  }

  return key;
}

TOMLValue TOMLParser::parseValue() {
  skipWhitespace();

  if (isAtEnd()) {
    setError("Unexpected end of input, expected value");
    return TOMLValue();
  }

  char c = peek();

  // String
  if (c == '"') {
    return TOMLValue::makeString(parseString());
  }

  // Array
  if (c == '[') {
    return parseArray();
  }

  // Inline table
  if (c == '{') {
    return parseInlineTable();
  }

  // Boolean
  if (content_.substr(pos_, 4) == "true") {
    pos_ += 4;
    column_ += 4;
    return TOMLValue::makeBoolean(true);
  }
  if (content_.substr(pos_, 5) == "false") {
    pos_ += 5;
    column_ += 5;
    return TOMLValue::makeBoolean(false);
  }

  // Number
  if (std::isdigit(c) || (c == '-' && std::isdigit(peekNext()))) {
    return parseNumber();
  }

  setError("Unexpected character: " + std::string(1, c));
  return TOMLValue();
}

std::string TOMLParser::parseString() {
  if (!match('"')) {
    setError("Expected opening quote for string");
    return "";
  }

  std::string value;
  while (!isAtEnd() && peek() != '"') {
    char c = peek();
    if (c == '\\') {
      advance();
      if (isAtEnd()) {
        setError("Unexpected end of string escape");
        return "";
      }
      c = peek();
      switch (c) {
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
      case '"':
        value += '"';
        break;
      case 'b':
        value += '\b';
        break;
      case 'f':
        value += '\f';
        break;
      default:
        value += c;
        break;
      }
    } else {
      value += c;
    }
    advance();
  }

  if (!match('"')) {
    setError("Expected closing quote for string");
    return "";
  }

  return value;
}

TOMLValue TOMLParser::parseNumber() {
  size_t start = pos_;

  // Handle negative sign
  if (peek() == '-') {
    advance();
  }

  // Parse digits
  while (!isAtEnd() && std::isdigit(peek())) {
    advance();
  }

  std::string numStr = content_.substr(start, pos_ - start);
  try {
    return TOMLValue::makeInteger(std::stoll(numStr));
  } catch (...) {
    setError("Invalid number: " + numStr);
    return TOMLValue();
  }
}

TOMLValue TOMLParser::parseArray() {
  if (!match('[')) {
    setError("Expected '[' for array");
    return TOMLValue();
  }

  TOMLValue array = TOMLValue::makeArray();

  skipWhitespace();

  if (peek() == ']') {
    advance();
    return array;
  }

  while (!isAtEnd()) {
    skipWhitespace();

    TOMLValue element = parseValue();
    if (!success())
      return array;

    array.arrayValue.push_back(std::move(element));

    skipWhitespace();

    if (peek() == ']') {
      advance();
      break;
    }

    if (!match(',')) {
      setError("Expected ',' or ']' in array");
      return array;
    }

    // Allow trailing comma
    skipWhitespace();
    if (peek() == ']') {
      advance();
      break;
    }
  }

  return array;
}

TOMLValue TOMLParser::parseInlineTable() {
  if (!match('{')) {
    setError("Expected '{' for inline table");
    return TOMLValue();
  }

  TOMLValue table = TOMLValue::makeTable();

  skipWhitespace();

  if (peek() == '}') {
    advance();
    return table;
  }

  while (!isAtEnd()) {
    skipWhitespace();

    std::string key = parseKey();
    if (!success())
      return table;

    skipWhitespace();
    if (!match('=')) {
      setError("Expected '=' in inline table");
      return table;
    }
    skipWhitespace();

    TOMLValue value = parseValue();
    if (!success())
      return table;

    table.tableValue[key] = std::move(value);

    skipWhitespace();

    if (peek() == '}') {
      advance();
      break;
    }

    if (!match(',')) {
      setError("Expected ',' or '}' in inline table");
      return table;
    }
  }

  return table;
}

void TOMLParser::parseTableHeader(std::vector<std::string> &path,
                                  bool &isArray) {
  isArray = false;

  if (!match('[')) {
    setError("Expected '[' for table header");
    return;
  }

  // Check for array of tables
  if (peek() == '[') {
    isArray = true;
    advance();
  }

  // Parse path components
  while (!isAtEnd()) {
    skipWhitespace();
    std::string key = parseKey();
    if (!success())
      return;

    path.push_back(key);
    skipWhitespace();

    if (peek() == ']') {
      advance();
      if (isArray) {
        if (!match(']')) {
          setError("Expected ']]' for array of tables");
          return;
        }
      }
      break;
    }

    if (!match('.')) {
      setError("Expected '.' or ']' in table header");
      return;
    }
  }

  if (path.empty()) {
    setError("Empty table header");
  }
}

void TOMLParser::setError(const std::string &msg) {
  if (error_.empty()) {
    error_ = "Line " + std::to_string(line_) + ", Column " +
             std::to_string(column_) + ": " + msg;
  }
}

} // namespace config
} // namespace meadows
