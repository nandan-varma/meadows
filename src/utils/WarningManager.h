#ifndef WARNING_MANAGER_H
#define WARNING_MANAGER_H

#include <string>
#include <unordered_set>

#include "ErrorCodes.h"

namespace meadows {

/**
 * @brief Manages compiler warning levels and per-warning suppression.
 *
 * Modelled after GCC/Clang's -Wall / -Wextra / -Werror / -Wno-<name> flags.
 */
class WarningManager {
public:
  enum class Level {
    OFF,
    DEFAULT,
    ALL,
    EXTRA,
  };

  WarningManager() : level_(Level::DEFAULT), treatAsErrors_(false) {}

  void setLevel(Level level) { level_ = level; }
  Level level() const { return level_; }

  bool treatAsErrors() const { return treatAsErrors_; }
  void setTreatAsErrors(bool value) { treatAsErrors_ = value; }

  void disableWarning(ErrorCode code) {
    disabled_.insert(code);
    enabled_.erase(code);
  }

  void enableWarning(ErrorCode code) {
    enabled_.insert(code);
    disabled_.erase(code);
  }

  bool isEnabled(ErrorCode code) const {
    if (disabled_.count(code)) return false;
    if (enabled_.count(code)) return true;
    if (level_ == Level::OFF) return false;
    return isEnabledByLevel(code);
  }

  static std::string levelToString(Level level);

private:
  Level level_;
  bool treatAsErrors_;
  std::unordered_set<ErrorCode> disabled_;
  std::unordered_set<ErrorCode> enabled_;

  bool isEnabledByLevel(ErrorCode code) const;
};

} // namespace meadows

#endif
