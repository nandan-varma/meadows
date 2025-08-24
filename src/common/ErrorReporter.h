#pragma once

#include "../lexer/Token.h"
#include <iostream>
#include <string>
#include <vector>

namespace meadows {

enum class ErrorLevel { WARNING, ERROR, FATAL };

struct CompilerError {
  ErrorLevel level;
  std::string message;
  SourceLocation location;
  std::string context;

  CompilerError(ErrorLevel level, const std::string &message,
                const SourceLocation &location, const std::string &context = "")
      : level(level), message(message), location(location), context(context) {}
};

class ErrorReporter {
private:
  std::vector<CompilerError> errors;
  bool hasErrors_;
  bool hasFatalErrors_;

public:
  ErrorReporter() : hasErrors_(false), hasFatalErrors_(false) {}

  void reportError(ErrorLevel level, const std::string &message,
                   const SourceLocation &location,
                   const std::string &context = "");

  void warning(const std::string &message, const SourceLocation &location,
               const std::string &context = "");
  void error(const std::string &message, const SourceLocation &location,
             const std::string &context = "");
  void fatal(const std::string &message, const SourceLocation &location,
             const std::string &context = "");

  bool hasErrors() const { return hasErrors_; }
  bool hasFatalErrors() const { return hasFatalErrors_; }
  size_t getErrorCount() const { return errors.size(); }

  const std::vector<CompilerError> &getErrors() const { return errors; }

  void printErrors(std::ostream &out = std::cerr) const;
  void clear();

  // Static helper methods
  static std::string levelToString(ErrorLevel level);
  static std::string formatError(const CompilerError &error);
};

} // namespace meadows
