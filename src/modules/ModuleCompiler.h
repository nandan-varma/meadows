/**
 * @file ModuleCompiler.h
 * @brief Multi-file module compilation with dependency resolution.
 */

#ifndef MODULE_COMPILER_H
#define MODULE_COMPILER_H

#include "../ast/AST.h"
#include "../lexer/Lexer.h"
#include "../parser/Parser.h"
#include "ModuleResolver.h"
#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace meadows {

struct CompiledModuleInfo {
  ModuleName name;
  std::string sourcePath;
  std::string outputPath;
  std::string irPath;
  std::vector<ModuleName> dependencies;
  std::vector<std::string> exportedFunctions;
  std::vector<std::string> exportedTypes;
  bool isDirty;
  bool hasErrors;
  std::string errorMessage;

  CompiledModuleInfo() : isDirty(true), hasErrors(false) {}
};

struct ModuleCompileResult {
  bool success;
  std::string outputFile;
  std::vector<std::string> errors;
  std::vector<std::string> warnings;
  std::vector<CompiledModuleInfo> compiledModules;
  std::vector<CompiledModuleInfo> cachedModules;

  static ModuleCompileResult makeSuccess(const std::string &output) {
    ModuleCompileResult r;
    r.success = true;
    r.outputFile = output;
    return r;
  }

  static ModuleCompileResult
  makeFailure(const std::vector<std::string> &errors) {
    ModuleCompileResult r;
    r.success = false;
    r.errors = errors;
    return r;
  }
};

struct ModuleCompileConfig {
  std::string projectRoot;
  std::string entryPoint;
  std::string outputDir;
  bool verbose;
  bool enableCache;
  bool runAfterCompile;
  std::string stdlibPath;
  std::vector<std::string> searchPaths;

  ModuleCompileConfig()
      : verbose(false), enableCache(true), runAfterCompile(false) {}
};

class ModuleCompiler {
public:
  explicit ModuleCompiler(const ModuleCompileConfig &config);

  ModuleCompileResult compile(const std::string &entryPoint);
  ModuleCompileResult compileFromSource(const std::string &source,
                                        const std::string &moduleName);

  void setVerbose(bool verbose) { config_.verbose = verbose; }
  void setEnableCache(bool enable) { config_.enableCache = enable; }

  std::vector<CompiledModuleInfo> getCompiledModules() const;

  bool linkAndEmitOutput(const std::string &outputFile);

private:
  ModuleCompileConfig config_;
  ModuleResolver resolver_;
  std::vector<CompiledModuleInfo> compiledModules_;
  std::vector<std::string> currentlyCompiling_;

  ModuleCompileResult resolveAndParseModule(const std::string &moduleName,
                                            const std::string &fromPath = "");
  bool checkCacheValidity(const CompiledModuleInfo &cached);
  void markModuleDirty(const std::string &moduleName);
  ModuleName determineModuleName(const std::string &filePath);
  void collectExports(const std::vector<std::unique_ptr<Stmt>> &stmts,
                      CompiledModuleInfo &info);

  void doLog(const std::string &msg) {
    if (config_.verbose) {
      std::cerr << msg << std::endl;
    }
  }
};

} // namespace meadows

#endif // MODULE_COMPILER_H
