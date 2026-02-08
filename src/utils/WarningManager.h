#ifndef WARNING_MANAGER_H
#define WARNING_MANAGER_H

#include "ErrorCodes.h"
#include "Exceptions.h"
#include <string>
#include <unordered_set>
#include <vector>

namespace meadows {

/**
 * @brief Manages compiler warnings and their configuration
 *
 * Supports warning levels similar to GCC/Clang:
 * - -Wall: Enable all common warnings
 * - -Wextra: Enable extra warnings
 * - -Werror: Treat warnings as errors
 * - -Wno-<warning>: Disable specific warning
 */
class WarningManager {
public:
  enum class Level {
    OFF,     // Warning disabled
    DEFAULT, // Default level (most warnings)
    ALL,     // -Wall
    EXTRA    // -Wextra (all warnings including pedantic)
  };

private:
  Level level_;
  bool treatAsErrors_;
  std::unordered_set<ErrorCode> disabledWarnings_;
  std::unordered_set<ErrorCode> enabledWarnings_;

public:
  WarningManager() : level_(Level::DEFAULT), treatAsErrors_(false) {
    setupDefaultWarnings();
  }

  /**
   * @brief Set warning level
   */
  void setLevel(Level level) { level_ = level; }

  /**
   * @brief Check if warnings should be treated as errors
   */
  bool treatAsErrors() const { return treatAsErrors_; }
  void setTreatAsErrors(bool value) { treatAsErrors_ = value; }

  /**
   * @brief Disable a specific warning
   */
  void disableWarning(ErrorCode code) {
    disabledWarnings_.insert(code);
    enabledWarnings_.erase(code);
  }

  /**
   * @brief Enable a specific warning
   */
  void enableWarning(ErrorCode code) {
    enabledWarnings_.insert(code);
    disabledWarnings_.erase(code);
  }

  /**
   * @brief Check if a warning is enabled
   */
  bool isEnabled(ErrorCode code) const {
    if (disabledWarnings_.count(code))
      return false;

    if (enabledWarnings_.count(code))
      return true;

    if (level_ == Level::OFF)
      return false;

    return isEnabledByLevel(code);
  }

  /**
   * @brief Get warning level as string
   */
  static std::string levelToString(Level level);

private:
  void setupDefaultWarnings();
  bool isEnabledByLevel(ErrorCode code) const;
};

/**
 * @brief Tracks variable usage for unused variable warnings
 */
class VariableUsageTracker {
public:
  struct VariableInfo {
    std::string name;
    SourceLocation location;
    bool isUsed;
    bool isParameter;
    bool isFunction;

    VariableInfo(const std::string &n, const SourceLocation &loc,
                 bool param = false, bool func = false)
        : name(n), location(loc), isUsed(false), isParameter(param),
          isFunction(func) {}
  };

private:
  std::vector<VariableInfo> variables_;
  std::vector<std::vector<size_t>> scopeStack_; // Stack of scope indices

public:
  VariableUsageTracker() {
    // Start with global scope
    scopeStack_.push_back({});
  }

  /**
   * @brief Enter a new scope
   */
  void enterScope() { scopeStack_.push_back({}); }

  /**
   * @brief Exit current scope and return variables that went out of scope
   */
  std::vector<VariableInfo> exitScope();

  /**
   * @brief Declare a variable in current scope
   */
  void declareVariable(const std::string &name, const SourceLocation &location,
                       bool isParameter = false, bool isFunction = false);

  /**
   * @brief Mark a variable as used
   */
  void markUsed(const std::string &name);

  /**
   * @brief Check if variable is declared (in any scope)
   */
  bool isDeclared(const std::string &name) const;

  /**
   * @brief Get unused variables from all scopes
   */
  std::vector<VariableInfo> getUnusedVariables() const;

  /**
   * @brief Clear all tracked variables
   */
  void clear() {
    variables_.clear();
    scopeStack_.clear();
    scopeStack_.push_back({});
  }
};

/**
 * @brief Tracks control flow for unreachable code detection
 */
class ControlFlowAnalyzer {
public:
  enum class BlockType {
    NORMAL,  // Normal code block
    LOOP,    // Inside a loop (break/continue valid)
    FUNCTION // Inside a function (return valid)
  };

private:
  std::vector<BlockType> blockStack_;
  bool hasReturned_;
  bool hasBroken_;
  bool hasContinued_;

public:
  ControlFlowAnalyzer()
      : hasReturned_(false), hasBroken_(false), hasContinued_(false) {
    blockStack_.push_back(BlockType::NORMAL);
  }

  /**
   * @brief Enter a new block
   */
  void enterBlock(BlockType type) {
    blockStack_.push_back(type);
    // Reset terminal states when entering new block
    hasReturned_ = false;
    hasBroken_ = false;
    hasContinued_ = false;
  }

  /**
   * @brief Exit current block
   */
  void exitBlock() {
    if (blockStack_.size() > 1) {
      blockStack_.pop_back();
    }
  }

  /**
   * @brief Mark that current block has a return statement
   */
  void markReturned() { hasReturned_ = true; }

  /**
   * @brief Mark that current block has a break statement
   */
  void markBroken() { hasBroken_ = true; }

  /**
   * @brief Mark that current block has a continue statement
   */
  void markContinued() { hasContinued_ = true; }

  /**
   * @brief Check if code after this point is unreachable
   */
  bool isUnreachable() const {
    return hasReturned_ || hasBroken_ || hasContinued_;
  }

  /**
   * @brief Check if we're inside a loop
   */
  bool inLoop() const;

  /**
   * @brief Check if we're inside a function
   */
  bool inFunction() const;

  /**
   * @brief Reset terminal states (for new branch)
   */
  void resetTerminal() {
    hasReturned_ = false;
    hasBroken_ = false;
    hasContinued_ = false;
  }
};

} // namespace meadows

#endif
