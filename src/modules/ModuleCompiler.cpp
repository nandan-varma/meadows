/**
 * @file ModuleCompiler.cpp
 * @brief Multi-file module compilation implementation.
 */

#include "ModuleCompiler.h"
#include <fstream>
#include <sstream>
#include <sys/stat.h>

namespace meadows {

ModuleCompiler::ModuleCompiler(const ModuleCompileConfig &config)
    : config_(config), resolver_(ModuleResolverConfig{}) {
  ModuleResolverConfig resolverConfig;
  resolverConfig.projectRoot = config.projectRoot;
  resolverConfig.stdlibPath = config.stdlibPath;
  resolverConfig.cachePath = config.projectRoot + "/.meadows/cache";
}

ModuleCompileResult ModuleCompiler::compile(const std::string &entryPoint) {
  doLog("Starting module compilation for: " + entryPoint);

  std::vector<std::string> errors;

  auto result = resolveAndParseModule(entryPoint);

  if (!result.success) {
    errors.insert(errors.end(), result.errors.begin(), result.errors.end());
    return ModuleCompileResult::makeFailure(errors);
  }

  ModuleCompileResult success;
  success.success = true;
  success.compiledModules = compiledModules_;
  return success;
}

ModuleCompileResult
ModuleCompiler::compileFromSource(const std::string &source,
                                  const std::string &moduleName) {
  doLog("Compiling source as module: " + moduleName);

  DiagnosticsCollector diagnostics;
  Lexer lexer(source);
  std::vector<Token> tokens;

  try {
    tokens = lexer.tokenize();
  } catch (const std::exception &e) {
    return ModuleCompileResult::makeFailure(
        {std::string("Lexer error: ") + e.what()});
  }

  Parser parser(tokens, diagnostics);
  std::vector<std::unique_ptr<Stmt>> stmts;
  try {
    stmts = parser.parse();
  } catch (const meadows::MeadowsException &e) {
    return ModuleCompileResult::makeFailure(
        {std::string("Parse error: ") + e.what()});
  } catch (const std::runtime_error &e) {
    return ModuleCompileResult::makeFailure(
        {std::string("Parse error: ") + e.what()});
  }

  if (diagnostics.hasFatals()) {
    std::vector<std::string> errors;
    for (const auto &diag : diagnostics.diagnostics()) {
      errors.push_back(diag.message);
    }
    return ModuleCompileResult::makeFailure(errors);
  }

  CompiledModuleInfo info;
  info.name = ModuleName(moduleName);
  info.sourcePath = "<memory>";
  collectExports(stmts, info);
  compiledModules_.push_back(info);

  ModuleCompileResult result;
  result.success = true;
  result.compiledModules = compiledModules_;
  return result;
}

ModuleCompileResult
ModuleCompiler::resolveAndParseModule(const std::string &moduleName,
                                      const std::string &fromPath) {
  auto it = std::find(currentlyCompiling_.begin(), currentlyCompiling_.end(),
                      moduleName);
  if (it != currentlyCompiling_.end()) {
    std::ostringstream oss;
    for (size_t i = 0; i < currentlyCompiling_.size(); ++i) {
      if (i > 0)
        oss << " -> ";
      oss << currentlyCompiling_[i];
    }
    oss << " -> " << moduleName;
    return ModuleCompileResult::makeFailure(
        {"Circular dependency detected: " + oss.str()});
  }

  currentlyCompiling_.push_back(moduleName);

  ModuleName name(moduleName);
  auto resolution = resolver_.resolve(name, fromPath);

  if (!resolution.resolved) {
    currentlyCompiling_.pop_back();
    return ModuleCompileResult::makeFailure({"Could not resolve module '" +
                                             moduleName +
                                             "': " + resolution.errorMessage});
  }

  std::ifstream file(resolution.filePath);
  if (!file.is_open()) {
    currentlyCompiling_.pop_back();
    return ModuleCompileResult::makeFailure(
        {"Cannot open file: " + resolution.filePath});
  }

  std::string source((std::istreambuf_iterator<char>(file)),
                     std::istreambuf_iterator<char>());

  DiagnosticsCollector diagnostics;
  Lexer lexer(source);
  std::vector<Token> tokens;

  try {
    tokens = lexer.tokenize();
  } catch (const std::exception &e) {
    currentlyCompiling_.pop_back();
    return ModuleCompileResult::makeFailure(
        {"Lexer error in " + resolution.filePath + ": " + e.what()});
  }

  Parser parser(tokens, diagnostics);
  auto stmts = parser.parse();

  if (diagnostics.hasFatals()) {
    currentlyCompiling_.pop_back();
    std::vector<std::string> errors;
    for (const auto &diag : diagnostics.diagnostics()) {
      errors.push_back(diag.message);
    }
    return ModuleCompileResult::makeFailure(errors);
  }

  CompiledModuleInfo info;
  info.name = resolution.moduleName;
  info.sourcePath = resolution.filePath;
  info.outputPath = resolution.filePath + ".o";
  info.irPath = resolution.filePath + ".ll";
  collectExports(stmts, info);

  for (const auto &stmt : stmts) {
    auto importStmtPtr = dynamic_cast<ImportStmt *>(stmt.get());
    if (importStmtPtr) {
      auto importResult =
          resolveAndParseModule(importStmtPtr->modulePath, resolution.filePath);
      if (!importResult.success) {
        currentlyCompiling_.pop_back();
        return importResult;
      }
      info.dependencies.push_back(ModuleName(importStmtPtr->modulePath));
    }
  }

  compiledModules_.push_back(info);
  currentlyCompiling_.pop_back();

  doLog("Compiled module: " + moduleName + " from " + resolution.filePath);

  ModuleCompileResult result;
  result.success = true;
  return result;
}

bool ModuleCompiler::checkCacheValidity(const CompiledModuleInfo &cached) {
  std::ifstream cachedFile(cached.irPath);
  if (!cachedFile.is_open()) {
    return false;
  }

  std::ifstream sourceFile(cached.sourcePath);
  if (!sourceFile.is_open()) {
    return false;
  }

  struct stat cachedStat, sourceStat;
  if (stat(cached.irPath.c_str(), &cachedStat) != 0)
    return false;
  if (stat(cached.sourcePath.c_str(), &sourceStat) != 0)
    return false;

  return cachedStat.st_mtime > sourceStat.st_mtime;
}

void ModuleCompiler::markModuleDirty(const std::string &moduleName) {
  for (auto &info : compiledModules_) {
    if (info.name.toString() == moduleName) {
      info.isDirty = true;
      return;
    }
  }
}

ModuleName ModuleCompiler::determineModuleName(const std::string &filePath) {
  return ModuleResolver::parseModuleNameFromFile(filePath);
}

void ModuleCompiler::collectExports(
    const std::vector<std::unique_ptr<Stmt>> &stmts, CompiledModuleInfo &info) {
  for (const auto &stmt : stmts) {
    auto funcStmt = dynamic_cast<FuncStmt *>(stmt.get());
    if (funcStmt) {
      info.exportedFunctions.push_back(funcStmt->name);
    }

    auto exportStmt = dynamic_cast<ExportStmt *>(stmt.get());
    if (exportStmt) {
      info.exportedFunctions.push_back(exportStmt->name);
    }

    auto typeDef = dynamic_cast<TypeDefStmt *>(stmt.get());
    if (typeDef) {
      info.exportedTypes.push_back(typeDef->name);
    }
  }
}

std::vector<CompiledModuleInfo> ModuleCompiler::getCompiledModules() const {
  return compiledModules_;
}

bool ModuleCompiler::linkAndEmitOutput(
    [[maybe_unused]] const std::string &outputFile) {
  return false;
}

} // namespace meadows
