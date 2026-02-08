#include "ErrorCodes.h"

namespace meadows {

std::string errorCodeToString(ErrorCode code) {
  switch (code) {
  // Lexical errors
  case ErrorCode::LEX_UNTERMINATED_STRING:
    return "E1001";
  case ErrorCode::LEX_INVALID_CHARACTER:
    return "E1002";
  case ErrorCode::LEX_INVALID_ESCAPE_SEQUENCE:
    return "E1003";
  case ErrorCode::LEX_NUMBER_OVERFLOW:
    return "E1004";

  // Parse errors
  case ErrorCode::PARSE_EXPECTED_SEMICOLON:
    return "E2001";
  case ErrorCode::PARSE_EXPECTED_EXPRESSION:
    return "E2002";
  case ErrorCode::PARSE_EXPECTED_IDENTIFIER:
    return "E2003";
  case ErrorCode::PARSE_EXPECTED_EQUALS:
    return "E2004";
  case ErrorCode::PARSE_EXPECTED_LPAREN:
    return "E2005";
  case ErrorCode::PARSE_EXPECTED_RPAREN:
    return "E2006";
  case ErrorCode::PARSE_EXPECTED_LBRACE:
    return "E2007";
  case ErrorCode::PARSE_EXPECTED_RBRACE:
    return "E2008";
  case ErrorCode::PARSE_UNEXPECTED_TOKEN:
    return "E2009";
  case ErrorCode::PARSE_INVALID_ASSIGNMENT_TARGET:
    return "E2010";
  case ErrorCode::PARSE_EXPECTED_COLON:
    return "E2011";
  case ErrorCode::PARSE_EXPECTED_COMMA:
    return "E2012";
  case ErrorCode::PARSE_EXPECTED_IN:
    return "E2013";

  // Semantic errors
  case ErrorCode::SEM_UNDEFINED_VARIABLE:
    return "E3001";
  case ErrorCode::SEM_UNDEFINED_FUNCTION:
    return "E3002";
  case ErrorCode::SEM_REDEFINED_VARIABLE:
    return "E3003";
  case ErrorCode::SEM_REDEFINED_FUNCTION:
    return "E3004";
  case ErrorCode::SEM_TYPE_MISMATCH:
    return "E3005";
  case ErrorCode::SEM_INVALID_ARGUMENT_COUNT:
    return "E3006";
  case ErrorCode::SEM_INVALID_OPERATOR:
    return "E3007";
  case ErrorCode::SEM_INVALID_CALL_TARGET:
    return "E3008";
  case ErrorCode::SEM_BREAK_OUTSIDE_LOOP:
    return "E3009";
  case ErrorCode::SEM_CONTINUE_OUTSIDE_LOOP:
    return "E3010";
  case ErrorCode::SEM_RETURN_OUTSIDE_FUNCTION:
    return "E3011";

  // Code generation errors
  case ErrorCode::CODEGEN_UNSUPPORTED_OPERATION:
    return "E4001";
  case ErrorCode::CODEGEN_TYPE_ERROR:
    return "E4002";
  case ErrorCode::CODEGEN_MODULE_VERIFICATION_FAILED:
    return "E4003";

  // System errors
  case ErrorCode::SYS_FILE_NOT_FOUND:
    return "E5001";
  case ErrorCode::SYS_FILE_TOO_LARGE:
    return "E5002";
  case ErrorCode::SYS_INVALID_PATH:
    return "E5003";
  case ErrorCode::SYS_PERMISSION_DENIED:
    return "E5004";
  case ErrorCode::SYS_IO_ERROR:
    return "E5005";
  case ErrorCode::SYS_COMPILER_INVOCATION_FAILED:
    return "E5006";

  // Warnings
  case ErrorCode::WARN_UNUSED_VARIABLE:
    return "W6001";
  case ErrorCode::WARN_UNUSED_FUNCTION:
    return "W6002";
  case ErrorCode::WARN_UNREACHABLE_CODE:
    return "W6003";
  case ErrorCode::WARN_SHADOWING_VARIABLE:
    return "W6004";
  case ErrorCode::WARN_DEPRECATED_FEATURE:
    return "W6005";
  case ErrorCode::WARN_DIVISION_BY_ZERO:
    return "W6006";
  case ErrorCode::WARN_ARRAY_BOUNDS:
    return "W6007";

  case ErrorCode::SUCCESS:
  default:
    return "E0000";
  }
}

} // namespace meadows
