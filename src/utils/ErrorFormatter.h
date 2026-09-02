#ifndef ERROR_FORMATTER_H
#define ERROR_FORMATTER_H

#include "../utils/Exceptions.h"
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace meadows {

/**
 * @brief Formats diagnostics with rich source context
 *
 * Provides Rust-style error messages with:
 * - Source line display with underlines
 * - Multi-line error spans
 * - Related information and help text
 * - Colorized output for terminals
 */
class ErrorFormatter {
public:
  struct FormatOptions {
    bool useColors;
    bool showSourceContext;
    int contextLines;
    bool showErrorCode;
    bool showHelp;

    FormatOptions()
        : useColors(true), showSourceContext(true), contextLines(2),
          showErrorCode(true), showHelp(true) {}
  };

private:
  FormatOptions options_;
  std::unordered_map<std::string, std::vector<std::string>> fileCache_;

  // ANSI color codes
  static const char *COLOR_RED;
  static const char *COLOR_GREEN;
  static const char *COLOR_YELLOW;
  static const char *COLOR_BLUE;
  static const char *COLOR_CYAN;
  static const char *COLOR_RESET;
  static const char *COLOR_BOLD;

  /**
   * @brief Load and cache file contents
   */
  const std::vector<std::string> &loadFile(const std::string &filepath);

  /**
   * @brief Get a specific line from a file
   */
  std::string getLine(const std::string &filepath, int lineNum);

  /**
   * @brief Check if terminal supports colors
   */
  static bool supportsColors();

  /**
   * @brief Format severity with color
   */
  std::string formatSeverity(const std::string &severity) const;

  /**
   * @brief Format error code
   */
  std::string formatErrorCode(ErrorCode code) const;

  /**
   * @brief Create underline for error location
   */
  std::string createUnderline(int startCol, int endCol, bool isMultiline) const;

  /**
   * @brief Calculate line number padding
   */
  int calculateLineNumberWidth(int maxLine) const;

  /**
   * @brief Format a line number with padding
   */
  std::string formatLineNumber(int line, int width) const;

public:
  explicit ErrorFormatter(const FormatOptions &options = FormatOptions());

  /**
   * @brief Format a single diagnostic
   */
  std::string format(const Diagnostic &diagnostic) const;

  /**
   * @brief Format a diagnostic with full source context
   */
  std::string formatWithContext(const Diagnostic &diagnostic,
                                const std::string &filepath);

  /**
   * @brief Format multiple diagnostics
   */
  std::string formatMultiple(const std::vector<Diagnostic> &diagnostics,
                             const std::string &filepath);

};

} // namespace meadows

#endif
