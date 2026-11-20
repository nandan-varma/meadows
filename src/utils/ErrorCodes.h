#ifndef MEADOWS_ERROR_CODES_H
#define MEADOWS_ERROR_CODES_H

#include <string>

namespace meadows {

#define MEADOWS_ERROR_CODES(X)                                                 \
  X(LEX_UNTERMINATED_STRING, 1001, "unterminated-string")                      \
  X(LEX_INVALID_CHARACTER, 1002, "invalid-character")                          \
  X(LEX_INVALID_ESCAPE, 1003, "invalid-escape")                                \
  X(LEX_NUMBER_OUT_OF_RANGE, 1004, "number-out-of-range")                      \
  X(LEX_STRING_TOO_LONG, 1005, "string-too-long")                              \
  X(PARSE_EXPECTED_SEMICOLON, 2001, "expected-semicolon")                      \
  X(PARSE_EXPECTED_EXPRESSION, 2002, "expected-expression")                    \
  X(PARSE_EXPECTED_IDENTIFIER, 2003, "expected-identifier")                    \
  X(PARSE_EXPECTED_EQUALS, 2004, "expected-equals")                            \
  X(PARSE_EXPECTED_LPAREN, 2005, "expected-lparen")                            \
  X(PARSE_EXPECTED_RPAREN, 2006, "expected-rparen")                            \
  X(PARSE_EXPECTED_LBRACE, 2007, "expected-lbrace")                            \
  X(PARSE_EXPECTED_RBRACE, 2008, "expected-rbrace")                            \
  X(PARSE_UNEXPECTED_TOKEN, 2009, "unexpected-token")                          \
  X(PARSE_INVALID_ASSIGNMENT_TARGET, 2010, "invalid-assignment-target")        \
  X(PARSE_EXPECTED_COLON, 2011, "expected-colon")                              \
  X(PARSE_EXPECTED_COMMA, 2012, "expected-comma")                              \
  X(PARSE_EXPECTED_IN, 2013, "expected-in")                                    \
  X(PARSE_NESTING_TOO_DEEP, 2014, "nesting-too-deep")                          \
  X(PARSE_MISPLACED_DECLARATION, 2015, "misplaced-declaration")                \
  X(PARSE_EXPECTED_RBRACKET, 2016, "expected-rbracket")                        \
  X(PARSE_EXPECTED_STRING, 2017, "expected-string")                            \
  X(SEM_UNDEFINED_VARIABLE, 3001, "undefined-variable")                        \
  X(SEM_UNDEFINED_FUNCTION, 3002, "undefined-function")                        \
  X(SEM_REDEFINED_VARIABLE, 3003, "redefined-variable")                        \
  X(SEM_REDEFINED_FUNCTION, 3004, "redefined-function")                        \
  X(SEM_INVALID_ARGUMENT_COUNT, 3006, "invalid-argument-count")                \
  X(SEM_BREAK_OUTSIDE_LOOP, 3009, "break-outside-loop")                        \
  X(SEM_CONTINUE_OUTSIDE_LOOP, 3010, "continue-outside-loop")                  \
  X(SEM_RETURN_OUTSIDE_FUNCTION, 3011, "return-outside-function")              \
  X(SEM_UNKNOWN_FIELD, 3012, "unknown-field")                                  \
  X(SEM_NAME_CONFLICT, 3013, "name-conflict")                                  \
  X(SEM_DUPLICATE_FIELD, 3014, "duplicate-field")                              \
  X(SEM_ASSIGN_TO_NON_VARIABLE, 3015, "assign-to-non-variable")                \
  X(SEM_BUILTIN_AS_VALUE, 3016, "builtin-as-value")                            \
  X(SYS_FILE_NOT_FOUND, 5001, "file-not-found")                                \
  X(SYS_FILE_TOO_LARGE, 5002, "file-too-large")                                \
  X(SYS_INVALID_PATH, 5003, "invalid-path")                                    \
  X(SYS_IO_ERROR, 5004, "io-error")                                            \
  X(WARN_UNUSED_VARIABLE, 6001, "unused-variable")                             \
  X(WARN_UNREACHABLE_CODE, 6003, "unreachable-code")                           \
  X(WARN_SHADOWING_VARIABLE, 6004, "shadowing")

enum class ErrorCode {
#define X(name, number, flag) name = number,
  MEADOWS_ERROR_CODES(X)
#undef X
};

bool isWarning(ErrorCode code);
std::string codeString(ErrorCode code);
const char* warningName(ErrorCode code);

} // namespace meadows

#endif
