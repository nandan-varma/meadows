#include "ErrorCodes.h"

namespace meadows {

bool isWarning(ErrorCode code) { return static_cast<int>(code) >= 6000; }

std::string codeString(ErrorCode code) {
  return (isWarning(code) ? "W" : "E") + std::to_string(static_cast<int>(code));
}

const char* warningName(ErrorCode code) {
  switch (code) {
#define X(name, number, flag)                                                  \
  case ErrorCode::name:                                                        \
    return flag;
    MEADOWS_ERROR_CODES(X)
#undef X
  }
  return "";
}

} // namespace meadows
