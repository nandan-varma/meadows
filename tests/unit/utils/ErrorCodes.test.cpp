#include "utils/ErrorCodes.h"
#include "catch_amalgamated.hpp"

using namespace meadows;

TEST_CASE("Error code to string conversion", "[errorcodes]") {
  SECTION("SUCCESS code") {
    REQUIRE(errorCodeToString(ErrorCode::SUCCESS) == "E0000");
  }

  SECTION("Lexical error codes") {
    REQUIRE(errorCodeToString(ErrorCode::LEX_UNTERMINATED_STRING) == "E1001");
    REQUIRE(errorCodeToString(ErrorCode::LEX_INVALID_CHARACTER) == "E1002");
    REQUIRE(errorCodeToString(ErrorCode::LEX_INVALID_ESCAPE_SEQUENCE) ==
            "E1003");
    REQUIRE(errorCodeToString(ErrorCode::LEX_NUMBER_OVERFLOW) == "E1004");
  }

  SECTION("Parse error codes") {
    REQUIRE(errorCodeToString(ErrorCode::PARSE_EXPECTED_SEMICOLON) == "E2001");
    REQUIRE(errorCodeToString(ErrorCode::PARSE_EXPECTED_EXPRESSION) == "E2002");
    REQUIRE(errorCodeToString(ErrorCode::PARSE_EXPECTED_IDENTIFIER) == "E2003");
    REQUIRE(errorCodeToString(ErrorCode::PARSE_EXPECTED_EQUALS) == "E2004");
    REQUIRE(errorCodeToString(ErrorCode::PARSE_EXPECTED_LPAREN) == "E2005");
    REQUIRE(errorCodeToString(ErrorCode::PARSE_EXPECTED_RPAREN) == "E2006");
    REQUIRE(errorCodeToString(ErrorCode::PARSE_EXPECTED_LBRACE) == "E2007");
    REQUIRE(errorCodeToString(ErrorCode::PARSE_EXPECTED_RBRACE) == "E2008");
    REQUIRE(errorCodeToString(ErrorCode::PARSE_UNEXPECTED_TOKEN) == "E2009");
    REQUIRE(errorCodeToString(ErrorCode::PARSE_INVALID_ASSIGNMENT_TARGET) ==
            "E2010");
    REQUIRE(errorCodeToString(ErrorCode::PARSE_EXPECTED_COLON) == "E2011");
    REQUIRE(errorCodeToString(ErrorCode::PARSE_EXPECTED_COMMA) == "E2012");
    REQUIRE(errorCodeToString(ErrorCode::PARSE_EXPECTED_IN) == "E2013");
  }

  SECTION("Semantic error codes") {
    REQUIRE(errorCodeToString(ErrorCode::SEM_UNDEFINED_VARIABLE) == "E3001");
    REQUIRE(errorCodeToString(ErrorCode::SEM_UNDEFINED_FUNCTION) == "E3002");
    REQUIRE(errorCodeToString(ErrorCode::SEM_REDEFINED_VARIABLE) == "E3003");
    REQUIRE(errorCodeToString(ErrorCode::SEM_REDEFINED_FUNCTION) == "E3004");
    REQUIRE(errorCodeToString(ErrorCode::SEM_TYPE_MISMATCH) == "E3005");
    REQUIRE(errorCodeToString(ErrorCode::SEM_INVALID_ARGUMENT_COUNT) ==
            "E3006");
    REQUIRE(errorCodeToString(ErrorCode::SEM_INVALID_OPERATOR) == "E3007");
    REQUIRE(errorCodeToString(ErrorCode::SEM_INVALID_CALL_TARGET) == "E3008");
    REQUIRE(errorCodeToString(ErrorCode::SEM_BREAK_OUTSIDE_LOOP) == "E3009");
    REQUIRE(errorCodeToString(ErrorCode::SEM_CONTINUE_OUTSIDE_LOOP) == "E3010");
    REQUIRE(errorCodeToString(ErrorCode::SEM_RETURN_OUTSIDE_FUNCTION) ==
            "E3011");
  }

  SECTION("Codegen error codes") {
    REQUIRE(errorCodeToString(ErrorCode::CODEGEN_UNSUPPORTED_OPERATION) ==
            "E4001");
    REQUIRE(errorCodeToString(ErrorCode::CODEGEN_TYPE_ERROR) == "E4002");
    REQUIRE(errorCodeToString(ErrorCode::CODEGEN_MODULE_VERIFICATION_FAILED) ==
            "E4003");
  }

  SECTION("System error codes") {
    REQUIRE(errorCodeToString(ErrorCode::SYS_FILE_NOT_FOUND) == "E5001");
    REQUIRE(errorCodeToString(ErrorCode::SYS_FILE_TOO_LARGE) == "E5002");
    REQUIRE(errorCodeToString(ErrorCode::SYS_INVALID_PATH) == "E5003");
    REQUIRE(errorCodeToString(ErrorCode::SYS_PERMISSION_DENIED) == "E5004");
    REQUIRE(errorCodeToString(ErrorCode::SYS_IO_ERROR) == "E5005");
    REQUIRE(errorCodeToString(ErrorCode::SYS_COMPILER_INVOCATION_FAILED) ==
            "E5006");
  }

  SECTION("Warning codes") {
    REQUIRE(errorCodeToString(ErrorCode::WARN_UNUSED_VARIABLE) == "W6001");
    REQUIRE(errorCodeToString(ErrorCode::WARN_UNUSED_FUNCTION) == "W6002");
    REQUIRE(errorCodeToString(ErrorCode::WARN_UNREACHABLE_CODE) == "W6003");
    REQUIRE(errorCodeToString(ErrorCode::WARN_SHADOWING_VARIABLE) == "W6004");
    REQUIRE(errorCodeToString(ErrorCode::WARN_DEPRECATED_FEATURE) == "W6005");
    REQUIRE(errorCodeToString(ErrorCode::WARN_DIVISION_BY_ZERO) == "W6006");
    REQUIRE(errorCodeToString(ErrorCode::WARN_ARRAY_BOUNDS) == "W6007");
  }
}

TEST_CASE("Error category boundaries", "[errorcodes]") {
  SECTION("SUCCESS is unknown") {
    REQUIRE(getErrorCategory(ErrorCode::SUCCESS) == "unknown");
  }

  SECTION("Lexical category (1000-1999)") {
    REQUIRE(getErrorCategory(ErrorCode::LEX_UNTERMINATED_STRING) == "lexical");
    REQUIRE(getErrorCategory(ErrorCode::LEX_NUMBER_OVERFLOW) == "lexical");
  }

  SECTION("Parse category (2000-2999)") {
    REQUIRE(getErrorCategory(ErrorCode::PARSE_EXPECTED_SEMICOLON) == "parse");
    REQUIRE(getErrorCategory(ErrorCode::PARSE_EXPECTED_IN) == "parse");
  }

  SECTION("Semantic category (3000-3999)") {
    REQUIRE(getErrorCategory(ErrorCode::SEM_UNDEFINED_VARIABLE) == "semantic");
    REQUIRE(getErrorCategory(ErrorCode::SEM_RETURN_OUTSIDE_FUNCTION) ==
            "semantic");
  }

  SECTION("Codegen category (4000-4999)") {
    REQUIRE(getErrorCategory(ErrorCode::CODEGEN_UNSUPPORTED_OPERATION) ==
            "codegen");
    REQUIRE(getErrorCategory(ErrorCode::CODEGEN_MODULE_VERIFICATION_FAILED) ==
            "codegen");
  }

  SECTION("System category (5000-5999)") {
    REQUIRE(getErrorCategory(ErrorCode::SYS_FILE_NOT_FOUND) == "system");
    REQUIRE(getErrorCategory(ErrorCode::SYS_COMPILER_INVOCATION_FAILED) ==
            "system");
  }

  SECTION("Warning category (6000-6999)") {
    REQUIRE(getErrorCategory(ErrorCode::WARN_UNUSED_VARIABLE) == "warning");
    REQUIRE(getErrorCategory(ErrorCode::WARN_ARRAY_BOUNDS) == "warning");
  }

  SECTION("Category boundaries") {
    REQUIRE(getErrorCategory(ErrorCode::SUCCESS) == "unknown");
    REQUIRE(getErrorCategory(static_cast<ErrorCode>(999)) == "unknown");
    REQUIRE(getErrorCategory(static_cast<ErrorCode>(1000)) == "lexical");
    REQUIRE(getErrorCategory(static_cast<ErrorCode>(1999)) == "lexical");
    REQUIRE(getErrorCategory(static_cast<ErrorCode>(2000)) == "parse");
    REQUIRE(getErrorCategory(static_cast<ErrorCode>(2999)) == "parse");
    REQUIRE(getErrorCategory(static_cast<ErrorCode>(3000)) == "semantic");
    REQUIRE(getErrorCategory(static_cast<ErrorCode>(3999)) == "semantic");
    REQUIRE(getErrorCategory(static_cast<ErrorCode>(4000)) == "codegen");
    REQUIRE(getErrorCategory(static_cast<ErrorCode>(4999)) == "codegen");
    REQUIRE(getErrorCategory(static_cast<ErrorCode>(5000)) == "system");
    REQUIRE(getErrorCategory(static_cast<ErrorCode>(5999)) == "system");
    REQUIRE(getErrorCategory(static_cast<ErrorCode>(6000)) == "warning");
    REQUIRE(getErrorCategory(static_cast<ErrorCode>(6999)) == "warning");
    REQUIRE(getErrorCategory(static_cast<ErrorCode>(7000)) == "unknown");
  }
}

TEST_CASE("isWarning function", "[errorcodes]") {
  SECTION("SUCCESS is not warning") {
    REQUIRE(isWarning(ErrorCode::SUCCESS) == false);
  }

  SECTION("Lexical errors are not warnings") {
    REQUIRE(isWarning(ErrorCode::LEX_UNTERMINATED_STRING) == false);
    REQUIRE(isWarning(ErrorCode::LEX_NUMBER_OVERFLOW) == false);
  }

  SECTION("Parse errors are not warnings") {
    REQUIRE(isWarning(ErrorCode::PARSE_UNEXPECTED_TOKEN) == false);
    REQUIRE(isWarning(ErrorCode::PARSE_EXPECTED_SEMICOLON) == false);
  }

  SECTION("Semantic errors are not warnings") {
    REQUIRE(isWarning(ErrorCode::SEM_UNDEFINED_VARIABLE) == false);
    REQUIRE(isWarning(ErrorCode::SEM_TYPE_MISMATCH) == false);
  }

  SECTION("Codegen errors are not warnings") {
    REQUIRE(isWarning(ErrorCode::CODEGEN_UNSUPPORTED_OPERATION) == false);
  }

  SECTION("System errors are not warnings") {
    REQUIRE(isWarning(ErrorCode::SYS_FILE_NOT_FOUND) == false);
  }

  SECTION("Warning codes are warnings") {
    REQUIRE(isWarning(ErrorCode::WARN_UNUSED_VARIABLE) == true);
    REQUIRE(isWarning(ErrorCode::WARN_UNREACHABLE_CODE) == true);
    REQUIRE(isWarning(ErrorCode::WARN_DIVISION_BY_ZERO) == true);
    REQUIRE(isWarning(ErrorCode::WARN_ARRAY_BOUNDS) == true);
  }

  SECTION("Boundary values") {
    REQUIRE(isWarning(static_cast<ErrorCode>(5999)) == false);
    REQUIRE(isWarning(static_cast<ErrorCode>(6000)) == true);
    REQUIRE(isWarning(static_cast<ErrorCode>(6999)) == true);
    REQUIRE(isWarning(static_cast<ErrorCode>(7000)) == false);
  }
}

TEST_CASE("isError function", "[errorcodes]") {
  SECTION("isError is inverse of isWarning") {
    REQUIRE(isError(ErrorCode::SUCCESS) == true);
    REQUIRE(isError(ErrorCode::LEX_UNTERMINATED_STRING) == true);
    REQUIRE(isError(ErrorCode::PARSE_UNEXPECTED_TOKEN) == true);
    REQUIRE(isError(ErrorCode::SEM_UNDEFINED_VARIABLE) == true);
    REQUIRE(isError(ErrorCode::CODEGEN_UNSUPPORTED_OPERATION) == true);
    REQUIRE(isError(ErrorCode::SYS_FILE_NOT_FOUND) == true);
    REQUIRE(isError(ErrorCode::WARN_UNUSED_VARIABLE) == false);
    REQUIRE(isError(ErrorCode::WARN_ARRAY_BOUNDS) == false);
  }
}

TEST_CASE("All error codes have valid categories", "[errorcodes]") {
  SECTION("Lexical errors (1000-1999)") {
    REQUIRE(getErrorCategory(ErrorCode::LEX_UNTERMINATED_STRING) == "lexical");
    REQUIRE(isWarning(ErrorCode::LEX_UNTERMINATED_STRING) == false);
    REQUIRE(isError(ErrorCode::LEX_UNTERMINATED_STRING) == true);
  }

  SECTION("Parse errors (2000-2999)") {
    REQUIRE(getErrorCategory(ErrorCode::PARSE_EXPECTED_SEMICOLON) == "parse");
    REQUIRE(isWarning(ErrorCode::PARSE_EXPECTED_SEMICOLON) == false);
    REQUIRE(isError(ErrorCode::PARSE_EXPECTED_SEMICOLON) == true);
  }

  SECTION("Semantic errors (3000-3999)") {
    REQUIRE(getErrorCategory(ErrorCode::SEM_UNDEFINED_VARIABLE) == "semantic");
    REQUIRE(isWarning(ErrorCode::SEM_UNDEFINED_VARIABLE) == false);
    REQUIRE(isError(ErrorCode::SEM_UNDEFINED_VARIABLE) == true);
  }

  SECTION("Codegen errors (4000-4999)") {
    REQUIRE(getErrorCategory(ErrorCode::CODEGEN_UNSUPPORTED_OPERATION) ==
            "codegen");
    REQUIRE(isWarning(ErrorCode::CODEGEN_UNSUPPORTED_OPERATION) == false);
    REQUIRE(isError(ErrorCode::CODEGEN_UNSUPPORTED_OPERATION) == true);
  }

  SECTION("System errors (5000-5999)") {
    REQUIRE(getErrorCategory(ErrorCode::SYS_FILE_NOT_FOUND) == "system");
    REQUIRE(isWarning(ErrorCode::SYS_FILE_NOT_FOUND) == false);
    REQUIRE(isError(ErrorCode::SYS_FILE_NOT_FOUND) == true);
  }

  SECTION("Warnings (6000-6999)") {
    REQUIRE(getErrorCategory(ErrorCode::WARN_UNUSED_VARIABLE) == "warning");
    REQUIRE(isWarning(ErrorCode::WARN_UNUSED_VARIABLE) == true);
    REQUIRE(isError(ErrorCode::WARN_UNUSED_VARIABLE) == false);
  }
}
