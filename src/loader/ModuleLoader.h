#ifndef MEADOWS_MODULE_LOADER_H
#define MEADOWS_MODULE_LOADER_H

#include <set>
#include <string>

#include "../ast/AST.h"
#include "../parser/Parser.h"
#include "../utils/Diagnostics.h"

namespace meadows {

class ModuleLoader {
public:
  static constexpr int MAX_IMPORT_DEPTH = 64;

  ModuleLoader(SourceMap& sources, DiagnosticSink& diagnostics)
      : sources_(sources), diagnostics_(diagnostics) {}

  StmtList load(const std::string& path, std::string text);

private:
  SourceMap& sources_;
  DiagnosticSink& diagnostics_;
  std::set<std::string> loaded_;

  StmtList parseModule(const std::string& path, std::string text, int depth);
  void splice(StmtList& into, const ImportRequest& request,
              const std::string& importer, int depth);
};

} // namespace meadows

#endif
