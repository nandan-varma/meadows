#include "SymbolTable.h"

void SymbolTable::enterScope() { scopeStack_.emplace_back(); }

void SymbolTable::exitScope() {
  if (!scopeStack_.empty()) {
    scopeStack_.pop_back();
  }
}

bool SymbolTable::declare(const std::string &name, llvm::Value *value) {
  if (scopeStack_.empty()) {
    if (globalScope_.find(name) != globalScope_.end()) {
      return false;
    }
    globalScope_[name] = {value, 0};
    return true;
  }

  auto &currentScope = scopeStack_.back();
  if (currentScope.find(name) != currentScope.end()) {
    return false;
  }

  currentScope[name] = {value, currentScopeLevel()};
  return true;
}

llvm::Value *SymbolTable::lookup(const std::string &name) {
  for (auto it = scopeStack_.rbegin(); it != scopeStack_.rend(); ++it) {
    auto varIt = it->find(name);
    if (varIt != it->end()) {
      return varIt->second.value;
    }
  }

  auto globalIt = globalScope_.find(name);
  if (globalIt != globalScope_.end()) {
    return globalIt->second.value;
  }

  return nullptr;
}

llvm::Value *SymbolTable::lookupCurrentScope(const std::string &name) {
  if (scopeStack_.empty()) {
    auto globalIt = globalScope_.find(name);
    return globalIt != globalScope_.end() ? globalIt->second.value : nullptr;
  }

  auto &currentScope = scopeStack_.back();
  auto it = currentScope.find(name);
  return it != currentScope.end() ? it->second.value : nullptr;
}

bool SymbolTable::exists(const std::string &name) {
  for (const auto &scope : scopeStack_) {
    if (scope.find(name) != scope.end()) {
      return true;
    }
  }
  return globalScope_.find(name) != globalScope_.end();
}
