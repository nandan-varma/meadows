#ifndef EXCEPTIONS_H
#define EXCEPTIONS_H

#include "ErrorCodes.h"
#include <exception>
#include <sstream>
#include <string>
#include <vector>

namespace meadows {

/**
 * @brief Represents a single frame in a stack trace
 */
struct StackFrame {
  std::string function;
  std::string file;
  int line;
  std::string address; // Optional: instruction address
};

/**
 * @brief Source location in the Meadows source code
 */
struct SourceLocation {
  std::string file;
  int line;
  int column;
  int endColumn; // For highlighting token span

  SourceLocation() : file(""), line(0), column(0), endColumn(0) {}
  SourceLocation(const std::string &f, int l, int c)
      : file(f), line(l), column(c), endColumn(c + 1) {}
  SourceLocation(const std::string &f, int l, int c, int ec)
      : file(f), line(l), column(c), endColumn(ec) {}
};

/**
 * @brief Rich diagnostic information for errors and warnings
 */
struct Diagnostic {
  ErrorCode code;
  std::string message;
  SourceLocation location;
  std::string severity; // "error", "warning", "info", "hint"
  std::vector<std::pair<SourceLocation, std::string>> relatedInfo;
  std::string help; // Suggested fix or help text

  Diagnostic(ErrorCode c, const std::string &msg, const SourceLocation &loc)
      : code(c), message(msg), location(loc) {
    severity = isWarning(c) ? "warning" : "error";
  }
};

/**
 * @brief Base exception class for all Meadows errors
 *
 * Provides:
 * - Error codes for programmatic handling
 * - Source location (file, line, column)
 * - Stack trace capture
 * - Rich diagnostic information
 */
class MeadowsException : public std::exception {
private:
  ErrorCode code_;
  std::string message_;
  SourceLocation location_;
  mutable std::string what_; // Cached formatted message
  std::vector<StackFrame> stackTrace_;
  std::string help_;

  void captureStackTrace();
  std::string formatMessage() const;

public:
  /**
   * @brief Construct exception with error code and message
   */
  MeadowsException(ErrorCode code, const std::string &message);

  /**
   * @brief Construct exception with full source location
   */
  MeadowsException(ErrorCode code, const std::string &message,
                   const SourceLocation &location);

  /**
   * @brief Construct from a Diagnostic
   */
  explicit MeadowsException(const Diagnostic &diagnostic);

  /**
   * @brief Get error code
   */
  ErrorCode code() const noexcept { return code_; }

  /**
   * @brief Get error code as string (e.g., "E2001")
   */
  std::string codeString() const { return errorCodeToString(code_); }

  /**
   * @brief Get error category ("lexical", "parse", "semantic", etc.)
   */
  std::string category() const { return getErrorCategory(code_); }

  /**
   * @brief Get error message
   */
  const std::string &message() const noexcept { return message_; }

  /**
   * @brief Get source location
   */
  const SourceLocation &location() const noexcept { return location_; }

  /**
   * @brief Get formatted error description
   */
  const char *what() const noexcept override;

  /**
   * @brief Get stack trace
   */
  const std::vector<StackFrame> &stackTrace() const noexcept {
    return stackTrace_;
  }

  /**
   * @brief Check if this is a warning
   */
  bool isWarning() const noexcept { return meadows::isWarning(code_); }

  /**
   * @brief Check if this is an error
   */
  bool isError() const noexcept { return meadows::isError(code_); }

  /**
   * @brief Set help text (suggested fix)
   */
  void setHelp(const std::string &help) { help_ = help; }

  /**
   * @brief Get help text
   */
  const std::string &help() const noexcept { return help_; }

  /**
   * @brief Convert to Diagnostic struct
   */
  Diagnostic toDiagnostic() const;

  /**
   * @brief Format as human-readable string with context
   */
  std::string formatWithContext(const std::string &sourceLine = "") const;
};

// Convenience exception types for different error categories

class LexicalException : public MeadowsException {
public:
  LexicalException(ErrorCode code, const std::string &msg,
                   const SourceLocation &loc)
      : MeadowsException(code, msg, loc) {}
};

class ParseException : public MeadowsException {
public:
  ParseException(ErrorCode code, const std::string &msg,
                 const SourceLocation &loc)
      : MeadowsException(code, msg, loc) {}
};

class SemanticException : public MeadowsException {
public:
  SemanticException(ErrorCode code, const std::string &msg,
                    const SourceLocation &loc)
      : MeadowsException(code, msg, loc) {}
};

class CodeGenException : public MeadowsException {
public:
  CodeGenException(ErrorCode code, const std::string &msg,
                   const SourceLocation &loc)
      : MeadowsException(code, msg, loc) {}
};

class SystemException : public MeadowsException {
public:
  SystemException(ErrorCode code, const std::string &msg)
      : MeadowsException(code, msg) {}
  SystemException(ErrorCode code, const std::string &msg,
                  const SourceLocation &loc)
      : MeadowsException(code, msg, loc) {}
};

/**
 * @brief Macro to throw exception with current file/line (for internal errors)
 */
#define MEADOWS_THROW(code, msg)                                               \
  throw meadows::MeadowsException(                                             \
      code, msg, meadows::SourceLocation(__FILE__, __LINE__, 0))

/**
 * @brief Macro to throw exception with source location from token
 */
#define MEADOWS_THROW_AT(code, msg, token)                                     \
  throw meadows::MeadowsException(                                             \
      code, msg, meadows::SourceLocation("", token.line, token.column))

} // namespace meadows

#endif
