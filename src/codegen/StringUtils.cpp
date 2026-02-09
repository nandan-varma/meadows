#include "StringUtils.h"
#include <stdexcept>

namespace StringUtils {

std::string EscapeHandler::process(const std::string &input) {
  std::string result;
  result.reserve(input.length() * 2);
  for (size_t i = 0; i < input.length(); ++i) {
    char c = input[i];
    if (c == '\\' && i + 1 < input.length()) {
      char next = input[++i];
      switch (next) {
      case 'n':
        result += '\n';
        break;
      case 't':
        result += '\t';
        break;
      case '\\':
        result += '\\';
        break;
      case '"':
        result += '"';
        break;
      case 'r':
        result += '\r';
        break;
      case '0':
        result += '\0';
        break;
      case 'b':
        result += '\b';
        break;
      case 'f':
        result += '\f';
        break;
      default:
        result += next;
      }
    } else {
      result += c;
    }
  }
  return result;
}

std::string EscapeHandler::escape(const std::string &input) {
  std::string result;
  result.reserve(input.length() * 2);
  for (char c : input) {
    switch (c) {
    case '\n':
      result += "\\n";
      break;
    case '\t':
      result += "\\t";
      break;
    case '\\':
      result += "\\\\";
      break;
    case '"':
      result += "\\\"";
      break;
    case '\r':
      result += "\\r";
      break;
    case '\0':
      result += "\\0";
      break;
    case '\b':
      result += "\\b";
      break;
    case '\f':
      result += "\\f";
      break;
    default:
      result += c;
    }
  }
  return result;
}

StringPool &StringPool::getInstance() {
  static StringPool instance;
  return instance;
}

const std::string *StringPool::intern(const std::string &str) {
  auto it = pool_.find(str);
  if (it != pool_.end()) {
    return &(it->first);
  }
  auto result = pool_.emplace(str, str);
  return &(result.first->first);
}

} // namespace StringUtils
