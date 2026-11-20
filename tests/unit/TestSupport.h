#ifndef MEADOWS_TEST_SUPPORT_H
#define MEADOWS_TEST_SUPPORT_H

#include <optional>
#include <string>
#include <vector>

#include "driver/Frontend.h"
#include "interpreter/Interpreter.h"

namespace meadows::testing {

struct Execution {
  std::string output;
  std::optional<RuntimeError> error;
};

inline Analysis analyze(const std::string& source, WarningPolicy policy = {}) {
  return analyzeSource("test.ms", source, std::move(policy));
}

inline std::vector<ErrorCode> codes(const Analysis& analysis) {
  std::vector<ErrorCode> result;
  for (const Diagnostic& d : analysis.diagnostics.diagnostics()) {
    result.push_back(d.code);
  }
  return result;
}

inline Execution execute(const std::string& source) {
  Analysis analysis = analyze(source);
  Execution execution;
  if (!analysis.ok()) {
    execution.output = formatDiagnostics(analysis.diagnostics.diagnostics(),
                                         analysis.sources);
    return execution;
  }
  Interpreter interpreter(
      [&](std::string_view text) { execution.output += text; });
  execution.error = interpreter.run(analysis.program);
  return execution;
}

} // namespace meadows::testing

#endif
