#include "ModuleResolver.h"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <sys/stat.h>

namespace meadows {

std::string ModuleResolverConfig::getProjectSrcPath() const {
  if (!projectRoot.empty()) {
    return projectRoot + "/src";
  }
  return "src";
}

std::string ModuleResolverConfig::getStdlibPath() const {
  if (!stdlibPath.empty()) {
    return stdlibPath;
  }
  return "/usr/local/lib/meadows/std";
}

std::string ModuleResolverConfig::getCachePath() const {
  if (!cachePath.empty()) {
    return cachePath;
  }
  return ".meadows/cache";
}

ModuleResolver::ModuleResolver(const ModuleResolverConfig &config)
    : config_(config) {
  searchOrder_ = {ModuleSearchPath::RELATIVE, ModuleSearchPath::PROJECT,
                  ModuleSearchPath::STANDARD_LIB, ModuleSearchPath::DEPENDENCY,
                  ModuleSearchPath::SYSTEM};
}

ModuleResolutionResult
ModuleResolver::resolve(const ModuleName &moduleName,
                        [[maybe_unused]] const std::string &fromPath) {
  for (auto path : searchOrder_) {
    auto result = searchInPath(moduleName, path);
    if (result.resolved) {
      return result;
    }
  }

  return ModuleResolutionResult::makeFailure(
      "Module '" + moduleName.toString() + "' not found");
}

static std::string normalizePath(const std::string &path) {
  std::string result = path;
  // Remove "./" at the beginning
  size_t pos = 0;
  while ((pos = result.find("/", pos)) != std::string::npos) {
    if (pos > 0 && result[pos - 1] == '.' &&
        (pos == 1 || result[pos - 2] == '/')) {
      // Remove the "./"
      result.erase(pos - 1, 2);
      pos = (pos > 1) ? pos - 1 : 0;
    } else {
      ++pos;
    }
  }
  // Handle "./" at the start
  if (result.substr(0, 2) == "./") {
    result = result.substr(2);
  }
  return result;
}

ModuleResolutionResult
ModuleResolver::resolveRelative(const std::string &relativePath,
                                const std::string &fromPath) {
  // Handle ./ and ../ prefixes
  std::string path = relativePath;

  // Reject path traversal attempts
  if (path.find("..") != std::string::npos) {
    return ModuleResolutionResult::makeFailure("Path traversal not allowed");
  }

  // Determine base directory
  std::string baseDir;
  if (!fromPath.empty()) {
    size_t lastSlash = fromPath.find_last_of("/\\");
    if (lastSlash != std::string::npos) {
      baseDir = fromPath.substr(0, lastSlash);
    }
  }

  // Build full path
  std::string fullPath;
  if (!baseDir.empty()) {
    fullPath = baseDir + "/" + path;
  } else {
    fullPath = path;
  }

  // Add .ms extension if not present
  if (fullPath.size() < 3 || fullPath.substr(fullPath.size() - 3) != ".ms") {
    fullPath += ".ms";
  }

  // Normalize path (remove ./)
  fullPath = normalizePath(fullPath);

  // Use canonical paths to prevent path traversal (only if baseDir is provided)
  if (!baseDir.empty()) {
    try {
      std::string canonicalBaseCan =
          std::filesystem::canonical(baseDir).string();
      std::string canonicalFull = std::filesystem::canonical(fullPath).string();

      // Check if resolved path is still under base directory
      if (canonicalFull.find(canonicalBaseCan) != 0) {
        return ModuleResolutionResult::makeFailure("Path traversal detected");
      }
    } catch (const std::filesystem::filesystem_error &) {
      // Canonical path failed - file might not exist or other issues
      // Fall through to fileExists check below
    }
  }

  if (fileExists(fullPath)) {
    ModuleName name = parseModuleNameFromFile(fullPath);
    return ModuleResolutionResult::makeSuccess(fullPath, name,
                                               ModuleSearchPath::RELATIVE);
  }

  return ModuleResolutionResult::makeFailure("Cannot find module at path: " +
                                             fullPath);
}

std::vector<std::string> ModuleResolver::findModulesInDirectory(
    [[maybe_unused]] const std::string &dirPath) {
  std::vector<std::string> modules;
  // Simplified - would use filesystem in real implementation
  return modules;
}

std::vector<ModuleSearchPath> ModuleResolver::getSearchOrder() const {
  return searchOrder_;
}

void ModuleResolver::setSearchPaths(
    const std::vector<ModuleSearchPath> &paths) {
  searchOrder_ = paths;
}

bool ModuleResolver::fileExists(const std::string &path) {
  if (path.empty()) {
    return false;
  }
  struct stat st;
  if (stat(path.c_str(), &st) != 0) {
    return false;
  }
  return S_ISREG(st.st_mode);
}

ModuleName
ModuleResolver::parseModuleNameFromFile(const std::string &filePath) {
  std::ifstream file(filePath);
  if (!file.is_open()) {
    return ModuleName();
  }

  std::string firstLine;
  std::getline(file, firstLine);

  // Trim leading whitespace from firstLine
  size_t lineStart = 0;
  while (lineStart < firstLine.size() && isspace(firstLine[lineStart])) {
    ++lineStart;
  }

  // Look for: module module.name;
  size_t modulePos = firstLine.find("module ", lineStart);
  if (modulePos != std::string::npos) {
    size_t start = modulePos + 7; // length of "module "
    size_t end = firstLine.find(';', start);
    if (end == std::string::npos) {
      end = firstLine.size();
    }
    std::string moduleName = firstLine.substr(start, end - start);
    // Trim whitespace from both ends
    while (!moduleName.empty() && isspace(moduleName.back())) {
      moduleName.pop_back();
    }
    while (!moduleName.empty() && isspace(moduleName.front())) {
      moduleName.erase(0, 1);
    }
    return ModuleName(moduleName);
  }

  // Fallback: derive from filename
  size_t lastSlash = filePath.find_last_of("/\\");
  std::string fileName = (lastSlash != std::string::npos)
                             ? filePath.substr(lastSlash + 1)
                             : filePath;

  // Remove extension
  if (fileName.size() > 3 && fileName.substr(fileName.size() - 3) == ".ms") {
    fileName = fileName.substr(0, fileName.size() - 3);
  }

  return ModuleName(fileName);
}

bool ModuleResolver::isValidModuleName(const std::string &name) {
  if (name.empty())
    return false;

  size_t start = 0;
  bool isFirstComponent = true;
  while (start < name.size()) {
    size_t dot = name.find('.', start);
    std::string component = (dot == std::string::npos)
                                ? name.substr(start)
                                : name.substr(start, dot - start);

    // Empty first component is invalid (e.g., ".leading")
    if (isFirstComponent && component.empty()) {
      return false;
    }
    isFirstComponent = false;

    // Allow empty components after first (e.g., "math..utils" is valid)
    if (!component.empty() && !isValidComponent(component)) {
      return false;
    }

    if (dot == std::string::npos)
      break;
    start = dot + 1;
  }

  return true;
}

std::string ModuleResolver::validateModuleName(const std::string &name) {
  std::string result;
  size_t start = 0;
  bool first = true;

  while (start < name.size()) {
    size_t dot = name.find('.', start);
    std::string component = (dot == std::string::npos)
                                ? name.substr(start)
                                : name.substr(start, dot - start);

    if (!first)
      result += ".";
    result += component;
    first = false;

    if (dot == std::string::npos)
      break;
    start = dot + 1;
  }

  return result;
}

ModuleResolutionResult
ModuleResolver::searchInPath(const ModuleName &moduleName,
                             ModuleSearchPath path) {
  switch (path) {
  case ModuleSearchPath::RELATIVE:
    return ModuleResolutionResult::makeFailure(
        "Use resolveRelative for relative imports");
  case ModuleSearchPath::PROJECT:
    return searchInProject(moduleName);
  case ModuleSearchPath::STANDARD_LIB:
    return searchInStdlib(moduleName);
  case ModuleSearchPath::DEPENDENCY:
    return searchInCache(moduleName);
  case ModuleSearchPath::SYSTEM:
    return searchInSystem(moduleName);
  }
  return ModuleResolutionResult::makeFailure("Unknown search path");
}

ModuleResolutionResult
ModuleResolver::searchInProject(const ModuleName &moduleName) {
  std::string srcPath = config_.getProjectSrcPath();
  std::string relativePath = moduleName.toPath() + ".ms";
  std::string fullPath = srcPath + "/" + relativePath;

  if (fileExists(fullPath)) {
    return ModuleResolutionResult::makeSuccess(fullPath, moduleName,
                                               ModuleSearchPath::PROJECT);
  }

  return ModuleResolutionResult::makeFailure("Not found in project: " +
                                             fullPath);
}

ModuleResolutionResult
ModuleResolver::searchInStdlib(const ModuleName &moduleName) {
  std::string stdlibPath = config_.getStdlibPath();
  std::string relativePath = moduleName.toPath() + ".ms";
  std::string fullPath = stdlibPath + "/" + relativePath;

  if (fileExists(fullPath)) {
    return ModuleResolutionResult::makeSuccess(fullPath, moduleName,
                                               ModuleSearchPath::STANDARD_LIB);
  }

  return ModuleResolutionResult::makeFailure("Not found in stdlib: " +
                                             fullPath);
}

ModuleResolutionResult
ModuleResolver::searchInCache(const ModuleName &moduleName) {
  std::string cachePath = config_.getCachePath();
  std::string relativePath = moduleName.toPath() + ".ms";
  std::string fullPath = cachePath + "/" + relativePath;

  if (fileExists(fullPath)) {
    return ModuleResolutionResult::makeSuccess(fullPath, moduleName,
                                               ModuleSearchPath::DEPENDENCY);
  }

  return ModuleResolutionResult::makeFailure("Not found in cache");
}

ModuleResolutionResult
ModuleResolver::searchInSystem(const ModuleName &moduleName) {
  for (const auto &sysPath : config_.systemPaths) {
    std::string fullPath = sysPath + "/" + moduleName.toPath() + ".ms";
    if (fileExists(fullPath)) {
      return ModuleResolutionResult::makeSuccess(fullPath, moduleName,
                                                 ModuleSearchPath::SYSTEM);
    }
  }

  return ModuleResolutionResult::makeFailure("Not found in system paths");
}

std::string ModuleResolver::moduleNameToPath(const ModuleName &name) const {
  return name.toPath() + ".ms";
}

ModuleName ModuleResolver::pathToModuleName(const std::string &path) const {
  return ModuleName::fromPath(path);
}

bool ModuleResolver::isValidComponent(const std::string &component) {
  if (component.empty())
    return false;
  if (isdigit(component[0]))
    return false;

  for (char c : component) {
    if (!isalnum(c) && c != '_' && c != '-') {
      return false;
    }
  }

  return true;
}

} // namespace meadows
