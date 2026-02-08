#include "WarningManager.h"
#include <algorithm>

namespace meadows {

void WarningManager::setupDefaultWarnings() {}

bool WarningManager::isEnabledByLevel(ErrorCode code) const {
  switch (level_) {
  case Level::OFF:
    return false;

  case Level::DEFAULT:
    return code == ErrorCode::WARN_UNUSED_VARIABLE ||
           code == ErrorCode::WARN_UNREACHABLE_CODE ||
           code == ErrorCode::WARN_DIVISION_BY_ZERO;

  case Level::ALL:
    // All standard warnings
    return code == ErrorCode::WARN_UNUSED_VARIABLE ||
           code == ErrorCode::WARN_UNREACHABLE_CODE ||
           code == ErrorCode::WARN_DIVISION_BY_ZERO ||
           code == ErrorCode::WARN_SHADOWING_VARIABLE ||
           code == ErrorCode::WARN_UNUSED_FUNCTION;

  case Level::EXTRA:
    // All warnings including pedantic ones
    return isWarning(code);
  }
  return false;
}

std::string WarningManager::levelToString(Level level) {
  switch (level) {
  case Level::OFF:
    return "off";
  case Level::DEFAULT:
    return "default";
  case Level::ALL:
    return "all";
  case Level::EXTRA:
    return "extra";
  }
  return "unknown";
}

// VariableUsageTracker implementation
std::vector<VariableUsageTracker::VariableInfo>
VariableUsageTracker::exitScope() {
  std::vector<VariableInfo> unusedVars;

  if (scopeStack_.size() > 1) {
    // Get variables from current scope
    auto &currentScope = scopeStack_.back();

    for (size_t idx : currentScope) {
      if (!variables_[idx].isUsed && !variables_[idx].isParameter) {
        unusedVars.push_back(variables_[idx]);
      }
    }

    scopeStack_.pop_back();
  }

  return unusedVars;
}

void VariableUsageTracker::declareVariable(const std::string &name,
                                           const SourceLocation &location,
                                           bool isParameter, bool isFunction) {
  size_t idx = variables_.size();
  variables_.emplace_back(name, location, isParameter, isFunction);
  scopeStack_.back().push_back(idx);
}

void VariableUsageTracker::markUsed(const std::string &name) {
  // Search from innermost scope outward
  for (auto it = scopeStack_.rbegin(); it != scopeStack_.rend(); ++it) {
    for (size_t idx : *it) {
      if (variables_[idx].name == name) {
        variables_[idx].isUsed = true;
        return;
      }
    }
  }
}

bool VariableUsageTracker::isDeclared(const std::string &name) const {
  for (const auto &var : variables_) {
    if (var.name == name) {
      return true;
    }
  }
  return false;
}

std::vector<VariableUsageTracker::VariableInfo>
VariableUsageTracker::getUnusedVariables() const {
  std::vector<VariableInfo> unused;

  for (const auto &var : variables_) {
    if (!var.isUsed && !var.isParameter && !var.isFunction) {
      unused.push_back(var);
    }
  }

  return unused;
}

// ControlFlowAnalyzer implementation
bool ControlFlowAnalyzer::inLoop() const {
  for (auto it = blockStack_.rbegin(); it != blockStack_.rend(); ++it) {
    if (*it == BlockType::LOOP) {
      return true;
    }
  }
  return false;
}

bool ControlFlowAnalyzer::inFunction() const {
  for (auto it = blockStack_.rbegin(); it != blockStack_.rend(); ++it) {
    if (*it == BlockType::FUNCTION) {
      return true;
    }
  }
  return false;
}

} // namespace meadows
