#include "WarningManager.h"

namespace meadows {

bool WarningManager::isEnabledByLevel(ErrorCode code) const {
  switch (level_) {
  case Level::OFF:
    return false;

  case Level::DEFAULT:
    return code == ErrorCode::WARN_UNUSED_VARIABLE ||
           code == ErrorCode::WARN_UNREACHABLE_CODE ||
           code == ErrorCode::WARN_DIVISION_BY_ZERO;

  case Level::ALL:
    return code == ErrorCode::WARN_UNUSED_VARIABLE ||
           code == ErrorCode::WARN_UNREACHABLE_CODE ||
           code == ErrorCode::WARN_DIVISION_BY_ZERO ||
           code == ErrorCode::WARN_SHADOWING_VARIABLE ||
           code == ErrorCode::WARN_UNUSED_FUNCTION;

  case Level::EXTRA:
    return isWarning(code);
  }
  return false;
}

std::string WarningManager::levelToString(Level level) {
  switch (level) {
  case Level::OFF:     return "off";
  case Level::DEFAULT: return "default";
  case Level::ALL:     return "all";
  case Level::EXTRA:   return "extra";
  }
  return "unknown";
}

} // namespace meadows
