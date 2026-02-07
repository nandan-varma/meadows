#ifndef SYMBOLTABLE_H
#define SYMBOLTABLE_H

#include <llvm/IR/Value.h>
#include <string>
#include <unordered_map>
#include <vector>

class SymbolTable {
public:
  struct SymbolInfo {
    llvm::Value *value;
    int scopeLevel;
  };

  void enterScope();
  void exitScope();
  bool declare(const std::string &name, llvm::Value *value);
  llvm::Value *lookup(const std::string &name);
  llvm::Value *lookupCurrentScope(const std::string &name);
  bool exists(const std::string &name);
  int currentScopeLevel() const {
    return static_cast<int>(scopeStack_.size()) - 1;
  }
  int scopeDepth() const { return static_cast<int>(scopeStack_.size()); }

private:
  std::vector<std::unordered_map<std::string, SymbolInfo>> scopeStack_;
  std::unordered_map<std::string, SymbolInfo> globalScope_;
};

#endif
