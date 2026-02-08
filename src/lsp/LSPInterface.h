#ifndef LSP_INTERFACE_H
#define LSP_INTERFACE_H

#include "../ast/AST.h"
#include "../lexer/Token.h"
#include "../utils/Exceptions.h"
#include <memory>
#include <string>
#include <vector>

/**
 * @brief Structure representing a diagnostic message for LSP
 */
struct LSPDiagnostic {
  int line;
  int startColumn;
  int endColumn;
  int severity; // 1=Error, 2=Warning, 3=Information, 4=Hint
  std::string message;
  std::string source;
};

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
   * @param tokens Token stream from lexer
   * @param errors List of error messages
   */
  void emitDiagnostics(const std::string &filePath,
                       const std::vector<Token> &tokens,
                       const std::vector<std::string> &errors);

  /**
   * @brief Output diagnostics using rich Diagnostic structures
   * @param filePath Path to the source file
   * @param diagnostics List of diagnostics
   */
  void emitDiagnostics(const std::string &filePath,
                       const std::vector<meadows::Diagnostic> &diagnostics);

  /**
   * @brief Parse and convert an error message to LSP diagnostic format
   * @param error The error message from compiler
   * @param filePath Path to the source file
   * @return LSPDiagnostic structure
   */
  LSPDiagnostic parseError(const std::string &error,
                           const std::string &filePath);

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
  int estimateEndColumn(int startColumn, const std::string &tokenValue);

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
