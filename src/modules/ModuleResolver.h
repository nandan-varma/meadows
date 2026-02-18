/**
 * @file ModuleResolver.h
 * @brief Module resolution algorithm.
 */

#ifndef MODULE_RESOLVER_H
#define MODULE_RESOLVER_H

#include "Modules.h"
#include <memory>
#include <vector>

namespace meadows {

enum class ModuleSearchPath {
  RELATIVE,
  PROJECT,
  STANDARD_LIB,
  DEPENDENCY,
  SYSTEM
};

struct ModuleResolutionResult {
  bool resolved;
  std::string filePath;
  ModuleName moduleName;
  std::string errorMessage;
  ModuleSearchPath searchPath;

  static ModuleResolutionResult makeSuccess(const std::string &path,
                                            const ModuleName &name,
                                            ModuleSearchPath pathType) {
    ModuleResolutionResult r;
    r.resolved = true;
    r.filePath = path;
    r.moduleName = name;
    r.searchPath = pathType;
    return r;
  }

  static ModuleResolutionResult makeFailure(const std::string &error) {
    ModuleResolutionResult r;
    r.resolved = false;
    r.errorMessage = error;
    return r;
  }
};

struct ModuleResolverConfig {
  std::string projectRoot;
  std::string stdlibPath;
  std::string cachePath;
  std::vector<std::string> systemPaths;

  std::string getProjectSrcPath() const;
  std::string getStdlibPath() const;
  std::string getCachePath() const;
};

class ModuleResolver {
public:
  explicit ModuleResolver(const ModuleResolverConfig &config);

  ModuleResolutionResult resolve(const ModuleName &moduleName,
                                 const std::string &fromPath = "");
  ModuleResolutionResult resolveRelative(const std::string &relativePath,
                                         const std::string &fromPath);

  std::vector<std::string> findModulesInDirectory(const std::string &dirPath);
  std::vector<ModuleSearchPath> getSearchOrder() const;
  void setSearchPaths(const std::vector<ModuleSearchPath> &paths);
  void setConfig(const ModuleResolverConfig &config);

  static bool fileExists(const std::string &path);
  static ModuleName parseModuleNameFromFile(const std::string &filePath);
  static bool isValidModuleName(const std::string &name);
  static std::string validateModuleName(const std::string &name);

private:
  ModuleResolverConfig config_;
  std::vector<ModuleSearchPath> searchOrder_;

  ModuleResolutionResult searchInPath(const ModuleName &moduleName,
                                      ModuleSearchPath path);
  ModuleResolutionResult searchInProject(const ModuleName &moduleName);
  ModuleResolutionResult searchInStdlib(const ModuleName &moduleName);
  ModuleResolutionResult searchInCache(const ModuleName &moduleName);
  ModuleResolutionResult searchInSystem(const ModuleName &moduleName);

  std::string moduleNameToPath(const ModuleName &name) const;
  ModuleName pathToModuleName(const std::string &path) const;
  static bool isValidComponent(const std::string &component);
};

} // namespace meadows

#endif // MODULE_RESOLVER_H
