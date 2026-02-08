#include "DiagnosticsCollector.h"
#include <algorithm>

namespace meadows {

void DiagnosticsCollector::reportError(ErrorCode code,
                                       const std::string &message,
                                       const SourceLocation &location) {
  if (atLimit())
    return;

  Diagnostic diagnostic(code, message, location);
  diagnostics_.push_back(diagnostic);
  hasErrors_ = true;

  // Some errors are fatal (e.g., system errors, internal compiler errors)
  int codeVal = static_cast<int>(code);
  if (codeVal >= 5000) {
    hasFatals_ = true;
  }

  if (handler_) {
    handler_(diagnostic);
  }
}

void DiagnosticsCollector::reportWarning(ErrorCode code,
                                         const std::string &message,
                                         const SourceLocation &location) {
  if (atLimit())
    return;

  Diagnostic diagnostic(code, message, location);
  diagnostics_.push_back(diagnostic);

  if (handler_) {
    handler_(diagnostic);
  }
}

void DiagnosticsCollector::report(const Diagnostic &diagnostic) {
  if (atLimit())
    return;

  diagnostics_.push_back(diagnostic);
  if (diagnostic.severity == "error") {
    hasErrors_ = true;
  }

  if (handler_) {
    handler_(diagnostic);
  }
}

size_t DiagnosticsCollector::errorCount() const {
  return std::count_if(
      diagnostics_.begin(), diagnostics_.end(),
      [](const Diagnostic &d) { return d.severity == "error"; });
}

size_t DiagnosticsCollector::warningCount() const {
  return std::count_if(
      diagnostics_.begin(), diagnostics_.end(),
      [](const Diagnostic &d) { return d.severity == "warning"; });
}

void DiagnosticsCollector::clear() {
  diagnostics_.clear();
  hasErrors_ = false;
  hasFatals_ = false;
}

void DiagnosticsCollector::throwIfErrors() const {
  if (hasErrors_) {
    if (!diagnostics_.empty()) {
      throw MeadowsException(diagnostics_[0]);
    }
  }
}

} // namespace meadows
