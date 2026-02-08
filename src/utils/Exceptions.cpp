#include "Exceptions.h"
#include <algorithm>
#include <cstring>
#include <execinfo.h> // For backtrace (Unix/Linux/macOS)
#include <iomanip>
#include <sstream>

namespace meadows {

// Platform-specific stack trace capture
void MeadowsException::captureStackTrace() {
#ifdef __unix__
  const int MAX_FRAMES = 64;
  void *buffer[MAX_FRAMES];
  int nptrs = backtrace(buffer, MAX_FRAMES);
  char **strings = backtrace_symbols(buffer, nptrs);

  if (strings) {
    // Skip first frame (this function)
    for (int i = 1; i < nptrs; ++i) {
      StackFrame frame;

      // Parse backtrace_symbols output
      // Format varies by platform, but generally:
      // executable(function+offset) [address] or just address
      std::string symbol(strings[i]);

      // Try to extract function name
      size_t start = symbol.find('(');
      size_t end = symbol.find('+', start);
      if (start != std::string::npos && end != std::string::npos) {
        frame.function = symbol.substr(start + 1, end - start - 1);
      } else {
        frame.function = symbol;
      }

      // Extract address
      size_t addr_start = symbol.find('[');
      size_t addr_end = symbol.find(']', addr_start);
      if (addr_start != std::string::npos && addr_end != std::string::npos) {
        frame.address =
            symbol.substr(addr_start + 1, addr_end - addr_start - 1);
      }

      // We can't easily get file/line from backtrace without extra libraries
      // This would require libbfd or similar
      frame.file = "";
      frame.line = 0;

      stackTrace_.push_back(frame);
    }
    free(strings);
  }
#else
  // Windows or other platforms - stack trace not implemented
  // Could use StackWalk64 on Windows
#endif
}

std::string MeadowsException::formatMessage() const {
  std::ostringstream oss;

  // Format: error[E2001]: message at file.ms:5:10
  oss << category() << "[" << codeString() << "]: " << message_;

  if (location_.line > 0) {
    oss << " at ";
    if (!location_.file.empty()) {
      oss << location_.file << ":";
    }
    oss << location_.line;
    if (location_.column > 0) {
      oss << ":" << location_.column;
    }
  }

  return oss.str();
}

MeadowsException::MeadowsException(ErrorCode code, const std::string &message)
    : code_(code), message_(message) {
  captureStackTrace();
}

MeadowsException::MeadowsException(ErrorCode code, const std::string &message,
                                   const SourceLocation &location)
    : code_(code), message_(message), location_(location) {
  captureStackTrace();
}

MeadowsException::MeadowsException(const Diagnostic &diagnostic)
    : code_(diagnostic.code), message_(diagnostic.message),
      location_(diagnostic.location) {
  captureStackTrace();
}

const char *MeadowsException::what() const noexcept {
  if (what_.empty()) {
    what_ = formatMessage();
  }
  return what_.c_str();
}

Diagnostic MeadowsException::toDiagnostic() const {
  Diagnostic diag(code_, message_, location_);
  diag.help = help_;
  return diag;
}

std::string
MeadowsException::formatWithContext(const std::string &sourceLine) const {
  std::ostringstream oss;

  // Header with error code and category
  oss << "\033[1;31m" << category() << "[" << codeString() << "]:\033[0m "
      << message_ << "\n";

  // Location
  if (location_.line > 0) {
    oss << "  \033[0;34m-->\033[0m ";
    if (!location_.file.empty()) {
      oss << location_.file << ":";
    }
    oss << location_.line;
    if (location_.column > 0) {
      oss << ":" << location_.column;
    }
    oss << "\n";

    // Source line with highlighting
    if (!sourceLine.empty()) {
      oss << "   |\n";
      oss << std::setw(3) << location_.line << " | " << sourceLine << "\n";
      oss << "   | ";

      // Underline the error location
      int underlineStart = location_.column - 1;
      int underlineLen = std::max(1, location_.endColumn - location_.column);

      for (int i = 0; i < underlineStart; ++i) {
        oss << " ";
      }
      oss << "\033[1;31m";
      for (int i = 0; i < underlineLen; ++i) {
        oss << "^";
      }
      oss << "\033[0m\n";

      // Help text
      if (!help_.empty()) {
        oss << "   |\n";
        oss << "   = \033[1;32mhelp:\033[0m " << help_ << "\n";
      }
    }
  }

  return oss.str();
}

} // namespace meadows
