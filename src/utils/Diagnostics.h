#ifndef MEADOWS_DIAGNOSTICS_H
#define MEADOWS_DIAGNOSTICS_H

#include <set>
#include <string>
#include <vector>

#include "ErrorCodes.h"
#include "SourceMap.h"

namespace meadows {

struct Diagnostic {
  ErrorCode code;
  std::string message;
  std::string help;
  Location location;
  uint32_t length = 1;

  bool isWarning() const { return meadows::isWarning(code); }
};

struct WarningPolicy {
  std::set<ErrorCode> disabled;
  bool treatAsErrors = false;
};

class DiagnosticSink {
public:
  static constexpr size_t MAX_DIAGNOSTICS = 100;

  explicit DiagnosticSink(WarningPolicy policy = {})
      : policy_(std::move(policy)) {}

  void report(ErrorCode code, std::string message, Location location,
              uint32_t length = 1, std::string help = {});

  bool hasErrors() const { return errorCount_ > 0; }
  bool full() const { return diagnostics_.size() >= MAX_DIAGNOSTICS; }
  size_t errorCount() const { return errorCount_; }
  const std::vector<Diagnostic>& diagnostics() const { return diagnostics_; }

private:
  WarningPolicy policy_;
  std::vector<Diagnostic> diagnostics_;
  size_t errorCount_ = 0;
};

struct FormatOptions {
  bool color = false;
};

std::string formatDiagnostic(const Diagnostic& diagnostic,
                             const SourceMap& sources,
                             const FormatOptions& options = {});
std::string formatDiagnostics(const std::vector<Diagnostic>& diagnostics,
                              const SourceMap& sources,
                              const FormatOptions& options = {});
std::string diagnosticsToJson(const std::vector<Diagnostic>& diagnostics,
                              const SourceMap& sources);

} // namespace meadows

#endif
