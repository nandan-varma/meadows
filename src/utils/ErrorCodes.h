#ifndef ERROR_CODES_H
#define ERROR_CODES_H

#include <string>

namespace meadows {

/**
 * @brief Error codes for all compiler diagnostics
 *
 * Format: XXYY where XX = category, YY = specific error
 */
enum class ErrorCode {
  // Success
  SUCCESS = 0,

  // Lexical errors (1000-1999)
  LEX_UNTERMINATED_STRING = 1001,
  LEX_INVALID_CHARACTER = 1002,
  LEX_INVALID_ESCAPE_SEQUENCE = 1003,
  LEX_NUMBER_OVERFLOW = 1004,

  // Parse errors (2000-2999)
  PARSE_EXPECTED_SEMICOLON = 2001,
  PARSE_EXPECTED_EXPRESSION = 2002,
  PARSE_EXPECTED_IDENTIFIER = 2003,
  PARSE_EXPECTED_EQUALS = 2004,
  PARSE_EXPECTED_LPAREN = 2005,
  PARSE_EXPECTED_RPAREN = 2006,
  PARSE_EXPECTED_LBRACE = 2007,
  PARSE_EXPECTED_RBRACE = 2008,
  PARSE_UNEXPECTED_TOKEN = 2009,
  PARSE_INVALID_ASSIGNMENT_TARGET = 2010,
  PARSE_EXPECTED_COLON = 2011,
  PARSE_EXPECTED_COMMA = 2012,
  PARSE_EXPECTED_IN = 2013,

  // Semantic errors (3000-3999)
  SEM_UNDEFINED_VARIABLE = 3001,
  SEM_UNDEFINED_FUNCTION = 3002,
  SEM_REDEFINED_VARIABLE = 3003,
  SEM_REDEFINED_FUNCTION = 3004,
  SEM_TYPE_MISMATCH = 3005,
  SEM_INVALID_ARGUMENT_COUNT = 3006,
  SEM_INVALID_OPERATOR = 3007,
  SEM_INVALID_CALL_TARGET = 3008,
  SEM_BREAK_OUTSIDE_LOOP = 3009,
  SEM_CONTINUE_OUTSIDE_LOOP = 3010,
  SEM_RETURN_OUTSIDE_FUNCTION = 3011,

  // Code generation errors (4000-4999)
  CODEGEN_UNSUPPORTED_OPERATION = 4001,
  CODEGEN_TYPE_ERROR = 4002,
  CODEGEN_MODULE_VERIFICATION_FAILED = 4003,

  // System errors (5000-5999)
  SYS_FILE_NOT_FOUND = 5001,
  SYS_FILE_TOO_LARGE = 5002,
  SYS_INVALID_PATH = 5003,
  SYS_PERMISSION_DENIED = 5004,
  SYS_IO_ERROR = 5005,
  SYS_COMPILER_INVOCATION_FAILED = 5006,

  // Warnings (6000-6999)
  WARN_UNUSED_VARIABLE = 6001,
  WARN_UNUSED_FUNCTION = 6002,
  WARN_UNREACHABLE_CODE = 6003,
  WARN_SHADOWING_VARIABLE = 6004,
  WARN_DEPRECATED_FEATURE = 6005,
  WARN_DIVISION_BY_ZERO = 6006,
  WARN_ARRAY_BOUNDS = 6007,
};

/**
 * @brief Get error category from error code
 */
inline std::string getErrorCategory(ErrorCode code) {
  int c = static_cast<int>(code);
  if (c >= 1000 && c < 2000)
    return "lexical";
  if (c >= 2000 && c < 3000)
    return "parse";
  if (c >= 3000 && c < 4000)
    return "semantic";
  if (c >= 4000 && c < 5000)
    return "codegen";
  if (c >= 5000 && c < 6000)
    return "system";
  if (c >= 6000 && c < 7000)
    return "warning";
  return "unknown";
}

/**
 * @brief Convert error code to string representation
 */
std::string errorCodeToString(ErrorCode code);

/**
 * @brief Check if error code represents a warning
 */
inline bool isWarning(ErrorCode code) {
  int c = static_cast<int>(code);
  return c >= 6000 && c < 7000;
}

/**
 * @brief Check if error code represents an error (non-warning)
 */
inline bool isError(ErrorCode code) { return !isWarning(code); }

} // namespace meadows

#endif
