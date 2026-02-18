#include "CompilationError.h"

#include <sstream>

namespace meadows {

std::string CompilationError::format() const {
  std::ostringstream oss;

  // Severity prefix
  switch (severity_) {
  case Severity::ERROR:
    oss << "error";
    break;
  case Severity::WARNING:
    oss << "warning";
    break;
  case Severity::INFO:
    oss << "info";
    break;
  case Severity::HINT:
    oss << "hint";
    break;
  }

  // Error code
  oss << "[" << static_cast<int>(code_) << "]: ";

  // Location
  if (location_.isValid()) {
    oss << location_.toString() << ": ";
  }

  // Message
  oss << message_;

  // Help
  if (!help_.empty()) {
    oss << "\n  help: " << help_;
  }

  // Related info
  for (const auto &info : relatedInfo_) {
    oss << "\n  note: " << info.message;
    if (info.location.isValid()) {
      oss << " at " << info.location.toString();
    }
  }

  return oss.str();
}

std::string CompilationError::getCategory() const {
  int code = static_cast<int>(code_);
  if (code >= 1000 && code < 2000)
    return "lexical";
  if (code >= 2000 && code < 3000)
    return "parse";
  if (code >= 3000 && code < 4000)
    return "semantic";
  if (code >= 4000 && code < 5000)
    return "codegen";
  if (code >= 5000 && code < 6000)
    return "system";
  if (code >= 6000 && code < 7000)
    return "warning";
  return "unknown";
}

ParseError ParseError::expected(const std::string &expected,
                                const std::string &found,
                                const SourceLocation &location) {
  return ParseError(ErrorCode::PARSE_UNEXPECTED_TOKEN,
                    "expected " + expected + ", found '" + found + "'",
                    location);
}

SemanticError SemanticError::undefinedVariable(const std::string &name,
                                               const SourceLocation &location) {
  SemanticError err(ErrorCode::SEM_UNDEFINED_VARIABLE,
                    "undefined variable '" + name + "'", location);
  err.withHelp("did you forget to declare '" + name + "'?");
  return err;
}

SemanticError SemanticError::typeMismatch(const std::string &expected,
                                          const std::string &found,
                                          const SourceLocation &location) {
  return SemanticError(
      ErrorCode::SEM_TYPE_MISMATCH,
      "type mismatch: expected " + expected + ", found " + found, location);
}

} // namespace meadows
