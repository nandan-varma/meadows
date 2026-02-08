#ifndef LSP_INTERFACE_H
#define LSP_INTERFACE_H

#include "../ast/AST.h"
#include "../lexer/Token.h"
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
   * @brief Output diagnostics in LSP JSON format
   * @param filePath Path to the source file
   * @param tokens Token stream from lexer
   * @param errors List of error messages
   */
  void emitDiagnostics(const std::string &filePath,
                       const std::vector<Token> &tokens,
                       const std::vector<std::string> &errors);

  /**
   * @brief Parse and convert an error message to LSP diagnostic format
   * @param error The error message from compiler
   * @param filePath Path to the source file
   * @return LSPDiagnostic structure
   */
  LSPDiagnostic parseError(const std::string &error,
                           const std::string &filePath);

private:
  std::string escapeJson(const std::string &str);
  int estimateEndColumn(int startColumn, const std::string &tokenValue);
};

#endif
