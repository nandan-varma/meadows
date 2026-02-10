/**
 * @file Modules.h
 * @brief Module system for the Meadows language.
 *
 * Provides multi-file project support with:
 * - Module declarations (module math.utils;)
 * - Import statements (import math.utils.{factorial, Point};)
 * - Export visibility control (export func foo() {})
 * - Module resolution (relative, project, stdlib, dependencies)
 * - Incremental compilation via cached .mco files
 */

#ifndef MODULES_H
#define MODULES_H

#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace meadows {

/**
 * @brief Represents a fully qualified module name.
 *
 * Examples:
 *   - "math.utils"
 *   - "std.io.file"
 *   - "main"
 */
class ModuleName {
public:
  std::vector<std::string> components;

  explicit ModuleName() = default;
  explicit ModuleName(const std::string &name);
  explicit ModuleName(std::vector<std::string> comps);

  std::string toString() const;
  std::string toPath() const;
  ModuleName parent() const;
  ModuleName append(const std::string &component) const;

  bool operator==(const ModuleName &other) const;
  bool operator<(const ModuleName &other) const;

  bool isSubModuleOf(const ModuleName &other) const;
  bool startsWith(const ModuleName &other) const;

  static ModuleName fromPath(const std::string &path);
};

/**
 * @brief Visibility modifier for declarations.
 */
enum class Visibility {
  PRIVATE, // Not exported (default)
  PUBLIC,  // Exported from module
};

/**
 * @brief Exported symbol from a module.
 */
struct ExportedSymbol {
  std::string name;
  std::string type; // "func", "type", "var"
  Visibility visibility;
  std::string typeInfo; // Full type signature
};

/**
 * @brief Information about a compiled module.
 */
struct CompiledModule {
  ModuleName name;
  std::string filePath;
  std::string outputPath; // Path to .mco file
  std::vector<ModuleName> imports;
  std::vector<ExportedSymbol> exports;
  std::vector<std::string> dependencies; // Module names this depends on
  bool isStandardLibrary;
  bool isDirty; // Needs recompilation

  CompiledModule() : isStandardLibrary(false), isDirty(true) {}
};

/**
 * @brief Symbol resolved from an import.
 */
struct ResolvedSymbol {
  std::string name;
  ModuleName module;
  std::string fullPath; // Path to the module file
  std::vector<std::string> typeInfo;
};

/**
 * @brief Import statement parsed from source.
 */
struct ImportStatement {
  ModuleName modulePath;
  std::vector<std::string> specificImports; // Empty = import all
  std::string alias;                        // Empty = no alias

  bool importsAll() const;
  bool hasAlias() const;
};

/**
 * @brief Export statement parsed from source.
 */
struct ExportStatement {
  std::string name;
  Visibility visibility;
  std::string typeInfo;
};

} // namespace meadows

namespace std {
template <> struct hash<meadows::ModuleName> {
  size_t operator()(const meadows::ModuleName &name) const noexcept {
    size_t hash = 0;
    for (const auto &comp : name.components) {
      hash ^= std::hash<std::string>{}(comp) + 0x9e3779b9 + (hash << 6) +
              (hash >> 2);
    }
    return hash;
  }
};
} // namespace std

#endif // MODULES_H
