#ifndef MODULE_RESOLVER_H
#define MODULE_RESOLVER_H

#include "Token.h"
#include <string>
#include <unordered_set>
#include <vector>

namespace meadows {

/**
 * Resolves `import "path/to/file.ms";` statements by textually splicing
 * each imported file's tokens in place of the import statement, before
 * parsing — the parser, semantic analyzer, and CodeGen never need to know
 * imports exist; they just see one bigger token stream. This is the entire
 * module system: no namespacing, no per-file scoping — an imported file's
 * top-level declarations land directly in the importing file's global
 * scope, so name collisions across files are exactly the same
 * SEM_REDEFINED_* errors as within one file.
 *
 * Import paths are relative to the importing file's directory. A file
 * already resolved earlier in the same import graph is skipped on a later
 * `import` of it (first-import-wins, like a C header guard) rather than
 * being spliced in again — this also breaks circular imports.
 *
 * CLI-only: the browser playground has one in-memory source string, not a
 * filesystem of importable files, so WasmBridge doesn't use this.
 */
class ModuleResolver {
public:
  // Throws meadows::MeadowsException on an unresolvable import path or
  // malformed `import` statement.
  static std::vector<Token> resolve(const std::string &entryPath);

private:
  static void resolveInto(const std::string &path, std::vector<Token> &out,
                          std::unordered_set<std::string> &visited);
};

} // namespace meadows

#endif
