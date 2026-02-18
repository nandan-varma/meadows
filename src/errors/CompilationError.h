/**
 * @file CompilationError.h
 * @brief Structured error types for the compilation pipeline
 *
 * Implements the Command Pattern for error operations and provides
 * a unified interface for all compilation errors with source locations,
 * error codes, and recovery suggestions.
 */

#ifndef COMPILATION_ERROR_H
#define COMPILATION_ERROR_H

#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "../utils/ErrorCodes.h"

namespace meadows {

/**
 * @brief Represents a location in source code
 */
struct SourceLocation {
  std::string file;
  int line;
  int column;
  int endColumn;

  SourceLocation() : file(""), line(0), column(0), endColumn(0) {}
  SourceLocation(const std::string &f, int l, int c)
      : file(f), line(l), column(c), endColumn(c + 1) {}
  SourceLocation(const std::string &f, int l, int c, int ec)
      : file(f), line(l), column(c), endColumn(ec) {}

  bool isValid() const { return line > 0; }

  std::string toString() const {
    if (!isValid())
      return "unknown location";
    return file + ":" + std::to_string(line) + ":" + std::to_string(column);
  }
};

/**
 * @brief Severity levels for diagnostics
 */
enum class Severity { ERROR, WARNING, INFO, HINT };

/**
 * @brief Related information for an error (e.g., where a variable was defined)
 */
struct RelatedInfo {
  SourceLocation location;
  std::string message;

  RelatedInfo(const SourceLocation &loc, const std::string &msg)
      : location(loc), message(msg) {}
};

/**
 * @brief Base class for all compilation errors
 *
 * Implements Command Pattern - each error knows how to format itself
 * and can be processed by error handlers.
 */
class CompilationError {
protected:
  ErrorCode code_;
  std::string message_;
  SourceLocation location_;
  std::vector<RelatedInfo> relatedInfo_;
  std::string help_;
  Severity severity_;

public:
  CompilationError(ErrorCode code, const std::string &message,
                   const SourceLocation &location,
                   Severity severity = Severity::ERROR)
      : code_(code), message_(message), location_(location),
        severity_(severity) {}

  virtual ~CompilationError() = default;

  // Getters
  ErrorCode getCode() const { return code_; }
  const std::string &getMessage() const { return message_; }
  const SourceLocation &getLocation() const { return location_; }
  Severity getSeverity() const { return severity_; }
  const std::vector<RelatedInfo> &getRelatedInfo() const {
    return relatedInfo_;
  }
  const std::string &getHelp() const { return help_; }

  // Chainable setters (Builder pattern style)
  CompilationError &withRelatedInfo(const SourceLocation &loc,
                                    const std::string &message) {
    relatedInfo_.emplace_back(loc, message);
    return *this;
  }

  CompilationError &withHelp(const std::string &help) {
    help_ = help;
    return *this;
  }

  /**
   * @brief Format error as human-readable string
   */
  virtual std::string format() const;

  /**
   * @brief Get error category name
   */
  std::string getCategory() const;

  /**
   * @brief Check if this is a warning
   */
  bool isWarning() const { return severity_ == Severity::WARNING; }

  /**
   * @brief Check if this is an error
   */
  bool isError() const { return severity_ == Severity::ERROR; }
};

/**
 * @brief Error during lexical analysis
 */
class LexicalError : public CompilationError {
public:
  LexicalError(ErrorCode code, const std::string &message,
               const SourceLocation &location)
      : CompilationError(code, message, location, Severity::ERROR) {}
};

/**
 * @brief Error during parsing
 */
class ParseError : public CompilationError {
public:
  ParseError(ErrorCode code, const std::string &message,
             const SourceLocation &location)
      : CompilationError(code, message, location, Severity::ERROR) {}

  /**
   * @brief Create error for expected token
   */
  static ParseError expected(const std::string &expected,
                             const std::string &found,
                             const SourceLocation &location);
};

/**
 * @brief Error during semantic analysis
 */
class SemanticError : public CompilationError {
public:
  SemanticError(ErrorCode code, const std::string &message,
                const SourceLocation &location)
      : CompilationError(code, message, location, Severity::ERROR) {}

  /**
   * @brief Create undefined variable error
   */
  static SemanticError undefinedVariable(const std::string &name,
                                         const SourceLocation &location);

  /**
   * @brief Create type mismatch error
   */
  static SemanticError typeMismatch(const std::string &expected,
                                    const std::string &found,
                                    const SourceLocation &location);
};

/**
 * @brief Error during code generation
 */
class CodeGenError : public CompilationError {
public:
  CodeGenError(ErrorCode code, const std::string &message,
               const SourceLocation &location)
      : CompilationError(code, message, location, Severity::ERROR) {}
};

/**
 * @brief System-level error (file I/O, etc.)
 */
class SystemError : public CompilationError {
public:
  SystemError(ErrorCode code, const std::string &message)
      : CompilationError(code, message, SourceLocation(), Severity::ERROR) {}

  SystemError(ErrorCode code, const std::string &message,
              const SourceLocation &location)
      : CompilationError(code, message, location, Severity::ERROR) {}
};

/**
 * @brief Warning (non-fatal)
 */
class Warning : public CompilationError {
public:
  Warning(ErrorCode code, const std::string &message,
          const SourceLocation &location)
      : CompilationError(code, message, location, Severity::WARNING) {}
};

} // namespace meadows

#endif // COMPILATION_ERROR_H
