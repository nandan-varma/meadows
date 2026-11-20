#ifndef MEADOWS_FRONTEND_H
#define MEADOWS_FRONTEND_H

#include <string>

#include "../ast/AST.h"
#include "../utils/Diagnostics.h"

namespace meadows {

struct Analysis {
  SourceMap sources;
  DiagnosticSink diagnostics;
  StmtList program;

  explicit Analysis(WarningPolicy policy) : diagnostics(std::move(policy)) {}
  Analysis(Analysis&&) = default;
  bool ok() const { return !diagnostics.hasErrors(); }
};

Analysis analyzeSource(const std::string& path, std::string text,
                       WarningPolicy policy = {});

} // namespace meadows

#endif
