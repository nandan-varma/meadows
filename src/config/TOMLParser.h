/**
 * @file TOMLParser.h
 * @brief Simple TOML parser for Meadows configuration files.
 *
 * Supports a subset of TOML 1.0:
 * - String, integer, and boolean values
 * - Table sections [section] and [[array]]
 * - Nested tables via dot notation
 * - Inline tables {key = value}
 * - Basic strings (double-quoted)
 * - Comments with #
 */

#ifndef TOML_PARSER_H
#define TOML_PARSER_H

#include <string>
#include <unordered_map>
#include <vector>

namespace meadows {
namespace config {

/**
 * @brief Represents a TOML value which can be:
 * - String
 * - Integer
 * - Boolean
 * - Table (nested key-value pairs)
 * - Array of values
 */
class TOMLValue {
public:
  enum Type { STRING, INTEGER, BOOLEAN, TABLE, ARRAY };

  Type type;
  std::string stringValue;
  long long intValue;
  bool boolValue;
  std::unordered_map<std::string, TOMLValue> tableValue;
  std::vector<TOMLValue> arrayValue;

  TOMLValue() : type(STRING), intValue(0), boolValue(false) {}

  static TOMLValue makeString(const std::string &s);
  static TOMLValue makeInteger(long long i);
  static TOMLValue makeBoolean(bool b);
  static TOMLValue makeTable();
  static TOMLValue makeArray();

  bool isString() const { return type == STRING; }
  bool isInteger() const { return type == INTEGER; }
  bool isBoolean() const { return type == BOOLEAN; }
  bool isTable() const { return type == TABLE; }
  bool isArray() const { return type == ARRAY; }

  std::string asString() const;
  long long asInteger() const;
  bool asBoolean() const;
};

/**
 * @brief Simple TOML parser.
 */
class TOMLParser {
public:
  TOMLParser();
  ~TOMLParser() = default;

  /**
   * @brief Parse TOML content.
   * @param content TOML formatted string.
   * @return Root table containing all parsed values.
   */
  TOMLValue parse(const std::string &content);

  /**
   * @brief Get error message if parsing failed.
   * @return Error message, or empty string if successful.
   */
  std::string getError() const { return error_; }

  /**
   * @brief Check if last parse was successful.
   * @return true if successful.
   */
  bool success() const { return error_.empty(); }

private:
  std::string content_;
  size_t pos_;
  std::string error_;
  int line_;
  int column_;

  void skipWhitespace();
  void skipComments();
  void advance();
  char peek() const;
  char peekNext() const;
  bool match(char expected);
  bool isAtEnd() const;

  void consumeNewline();

  std::string parseKey();
  TOMLValue parseValue();
  std::string parseString();
  TOMLValue parseNumber();
  TOMLValue parseBoolean();
  TOMLValue parseArray();
  TOMLValue parseInlineTable();

  TOMLValue parseTable();
  void parseTableHeader(std::vector<std::string> &path, bool &isArray);

  void setError(const std::string &msg);
};

} // namespace config
} // namespace meadows

#endif // TOML_PARSER_H
