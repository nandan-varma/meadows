#include "ErrorFormatter.h"
#include "../utils/MemoryUtils.h"
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <unistd.h>

namespace meadows {

// ANSI color codes
const char *ErrorFormatter::COLOR_RED = "\033[31m";
const char *ErrorFormatter::COLOR_GREEN = "\033[32m";
const char *ErrorFormatter::COLOR_YELLOW = "\033[33m";
const char *ErrorFormatter::COLOR_BLUE = "\033[34m";
const char *ErrorFormatter::COLOR_CYAN = "\033[36m";
const char *ErrorFormatter::COLOR_RESET = "\033[0m";
const char *ErrorFormatter::COLOR_BOLD = "\033[1m";

ErrorFormatter::ErrorFormatter(const FormatOptions &options)
    : options_(options) {
  // Auto-detect color support if not explicitly set
  if (options_.useColors) {
    options_.useColors = supportsColors();
  }
}

bool ErrorFormatter::supportsColors() {
  // Check if output is a terminal
  if (!isatty(STDERR_FILENO) && !isatty(STDOUT_FILENO)) {
    return false;
  }

  // Check TERM environment variable
  const char *term = getenv("TERM");
  if (!term || strcmp(term, "dumb") == 0) {
    return false;
  }

  // Check NO_COLOR environment variable
  if (getenv("NO_COLOR") != nullptr) {
    return false;
  }

  return true;
}

const std::vector<std::string> &
ErrorFormatter::loadFile(const std::string &filepath) {
  // Check cache first
  auto it = fileCache_.find(filepath);
  if (it != fileCache_.end()) {
    return it->second;
  }

  // Check file size before loading
  try {
    auto size = std::filesystem::file_size(filepath);
    if (size > MAX_ALLOC_SIZE) {
      // Return empty vector for oversized files to avoid DoS
      static const std::vector<std::string> emptyVec;
      return emptyVec;
    }
  } catch (const std::filesystem::filesystem_error &) {
    // File size check failed, proceed (file might not exist)
  }

  // Load file
  std::vector<std::string> lines;
  std::ifstream file(filepath);
  if (file.is_open()) {
    std::string line;
    while (std::getline(file, line)) {
      lines.push_back(line);
    }
  }

  // Cache and return
  fileCache_[filepath] = std::move(lines);
  return fileCache_[filepath];
}

std::string ErrorFormatter::getLine(const std::string &filepath, int lineNum) {
  const auto &lines = loadFile(filepath);
  if (lineNum > 0 && lineNum <= static_cast<int>(lines.size())) {
    return lines[lineNum - 1]; // Convert to 0-indexed
  }
  return "";
}

std::string ErrorFormatter::formatSeverity(const std::string &severity) const {
  if (!options_.useColors) {
    return severity;
  }

  if (severity == "error") {
    return std::string(COLOR_BOLD) + COLOR_RED + "error" + COLOR_RESET;
  } else if (severity == "warning") {
    return std::string(COLOR_BOLD) + COLOR_YELLOW + "warning" + COLOR_RESET;
  } else if (severity == "info") {
    return std::string(COLOR_BOLD) + COLOR_BLUE + "info" + COLOR_RESET;
  } else if (severity == "hint") {
    return std::string(COLOR_BOLD) + COLOR_CYAN + "hint" + COLOR_RESET;
  }
  return severity;
}

std::string ErrorFormatter::formatErrorCode(ErrorCode code) const {
  return errorCodeToString(code);
}

std::string ErrorFormatter::createUnderline(int startCol, int endCol,
                                            bool isMultiline) const {
  if (isMultiline || startCol < 0) {
    return "";
  }

  std::string underline;

  // Reserve capacity to avoid reallocations
  int length = std::max(1, endCol - startCol);
  underline.reserve(startCol + length + 20);

  // Add spaces up to start column
  for (int i = 0; i < startCol - 1; i++) {
    underline += " ";
  }
  if (options_.useColors) {
    underline += COLOR_RED;
  }
  for (int i = 0; i < length; i++) {
    underline += "^";
  }
  if (options_.useColors) {
    underline += COLOR_RESET;
  }

  return underline;
}

int ErrorFormatter::calculateLineNumberWidth(int maxLine) const {
  if (maxLine <= 0)
    return 1;
  int width = 0;
  while (maxLine > 0) {
    width++;
    maxLine /= 10;
  }
  return std::max(width, 1);
}

std::string ErrorFormatter::formatLineNumber(int line, int width) const {
  std::ostringstream oss;
  if (options_.useColors) {
    oss << COLOR_BLUE;
  }
  oss << std::setw(width) << line;
  if (options_.useColors) {
    oss << COLOR_RESET;
  }
  return oss.str();
}

std::string ErrorFormatter::format(const Diagnostic &diagnostic) const {
  std::ostringstream oss;

  // Error header: error[E2001]: message
  oss << formatSeverity(diagnostic.severity);
  if (options_.showErrorCode) {
    oss << "[" << formatErrorCode(diagnostic.code) << "]";
  }
  oss << ": " << diagnostic.message << "\n";

  // Location
  if (diagnostic.location.line > 0) {
    oss << "  ";
    if (options_.useColors) {
      oss << COLOR_BLUE << "-->" << COLOR_RESET;
    } else {
      oss << "-->";
    }
    oss << " ";
    if (!diagnostic.location.file.empty()) {
      oss << diagnostic.location.file << ":";
    }
    oss << diagnostic.location.line;
    if (diagnostic.location.column > 0) {
      oss << ":" << diagnostic.location.column;
    }
    oss << "\n";
  }

  return oss.str();
}

std::string ErrorFormatter::formatWithContext(const Diagnostic &diagnostic,
                                              const std::string &filepath) {
  if (!options_.showSourceContext) {
    return format(diagnostic);
  }

  std::ostringstream oss;

  // Error header
  oss << formatSeverity(diagnostic.severity);
  if (options_.showErrorCode) {
    oss << "[" << formatErrorCode(diagnostic.code) << "]";
  }
  oss << ": " << diagnostic.message << "\n";

  // Location line
  int line = diagnostic.location.line;
  if (line > 0) {
    const auto &lines = loadFile(filepath);
    int maxLine = static_cast<int>(lines.size());
    int lineNumWidth = calculateLineNumberWidth(maxLine);

    // Location header
    oss << " ";
    for (int i = 0; i < lineNumWidth; i++) {
      oss << " ";
    }
    oss << " ";
    if (options_.useColors) {
      oss << COLOR_BLUE << "-->" << COLOR_RESET;
    } else {
      oss << "-->";
    }
    oss << " " << filepath << ":" << line << ":" << diagnostic.location.column
        << "\n";

    // Context lines before
    int startLine = std::max(1, line - options_.contextLines);
    for (int i = startLine; i < line; i++) {
      oss << formatLineNumber(i, lineNumWidth) << " | " << getLine(filepath, i)
          << "\n";
    }

    // Error line
    oss << formatLineNumber(line, lineNumWidth) << " | "
        << getLine(filepath, line) << "\n";

    // Underline
    oss << " ";
    for (int i = 0; i < lineNumWidth; i++) {
      oss << " ";
    }
    oss << " | ";
    if (diagnostic.location.column > 0) {
      oss << createUnderline(diagnostic.location.column,
                             diagnostic.location.endColumn, false);
    }
    oss << "\n";

    // Context lines after
    int endLine = std::min(maxLine, line + options_.contextLines);
    for (int i = line + 1; i <= endLine; i++) {
      oss << formatLineNumber(i, lineNumWidth) << " | " << getLine(filepath, i)
          << "\n";
    }

    // Help text
    if (options_.showHelp && !diagnostic.help.empty()) {
      oss << " ";
      for (int i = 0; i < lineNumWidth; i++) {
        oss << " ";
      }
      oss << " |\n";
      oss << " ";
      for (int i = 0; i < lineNumWidth; i++) {
        oss << " ";
      }
      oss << " = ";
      if (options_.useColors) {
        oss << COLOR_GREEN << "help:" << COLOR_RESET;
      } else {
        oss << "help:";
      }
      oss << " " << diagnostic.help << "\n";
    }
  }

  return oss.str();
}

std::string
ErrorFormatter::formatMultiple(const std::vector<Diagnostic> &diagnostics,
                               const std::string &filepath) {
  std::ostringstream oss;

  for (size_t i = 0; i < diagnostics.size(); i++) {
    oss << formatWithContext(diagnostics[i], filepath);
    if (i < diagnostics.size() - 1) {
      oss << "\n";
    }
  }

  return oss.str();
}

} // namespace meadows
