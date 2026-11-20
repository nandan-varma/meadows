#include "Frontend.h"

#include "../loader/ModuleLoader.h"
#include "../sema/SemanticAnalyzer.h"

namespace meadows {

Analysis analyzeSource(const std::string& path, std::string text,
                       WarningPolicy policy) {
  Analysis analysis(std::move(policy));
  ModuleLoader loader(analysis.sources, analysis.diagnostics);
  analysis.program = loader.load(path, std::move(text));
  if (analysis.ok()) {
    SemanticAnalyzer(analysis.diagnostics).analyze(analysis.program);
  }
  return analysis;
}

} // namespace meadows
