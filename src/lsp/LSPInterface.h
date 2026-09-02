#ifndef LSP_INTERFACE_H
#define LSP_INTERFACE_H

#include "../utils/Exceptions.h"
#include <memory>
#include <string>
#include <vector>

/**
 * @brief LSP severity levels (LSP specification)
 */
enum class LSPSeverity { Error = 1, Warning = 2, Information = 3, Hint = 4 };

/**
 * @brief LSP position constants
 */
constexpr int LSP_LINE_OFFSET = 1;
constexpr int LSP_COLUMN_OFFSET = 1;
constexpr int LSP_DEFAULT_TOKEN_WIDTH = 1;

/**
 * @brief Interface for Language Server Protocol output
 *
 * Provides JSON-formatted output for LSP-compatible editors
 */
class LSPInterface {
public:
  LSPInterface();

  /**
   * @brief Output diagnostics in LSP JSON format (legacy string-based)
   * @param filePath Path to the source file
   * @param errors List of error messages
   */
  void emitDiagnostics(const std::string &filePath,
                       const std::vector<std::string> &errors);

  /**
   * @brief Output diagnostics using rich Diagnostic structures
   * @param filePath Path to the source file
   * @param diagnostics List of diagnostics
   */
  void emitDiagnostics(const std::string &filePath,
                       const std::vector<meadows::Diagnostic> &diagnostics);

  /**
   * @brief Parse error string into rich Diagnostic structure
   * @param error The error message from compiler
   * @param filePath Path to the source file
   * @return Diagnostic structure
   */
  meadows::Diagnostic parseErrorToDiagnostic(const std::string &error,
                                             const std::string &filePath);

private:
  std::string escapeJson(const std::string &str);

  /**
   * @brief Output diagnostics as JSON (internal implementation)
   */
  void emitDiagnosticsJSON(const std::string &filePath,
                           const std::vector<meadows::Diagnostic> &diagnostics);

  /**
   * @brief Convert severity string to LSP severity number
   */
  int severityFromString(const std::string &severity);
};

#endif
