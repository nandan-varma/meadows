#include "LSPInterface.h"

#include <iomanip>
#include <iostream>
#include <regex>
#include <sstream>

LSPInterface::LSPInterface() {}

void LSPInterface::emitDiagnostics(
    const std::string &filePath,
    [[maybe_unused]] const std::vector<Token> &tokens,
    const std::vector<std::string> &errors) {
  std::vector<meadows::CompilationError> compilationErrors;

  // Parse each error message
  for (const auto &error : errors) {
    compilationErrors.push_back(parseErrorToCompilationError(error, filePath));
  }

  emitDiagnosticsJSON(filePath, compilationErrors);
}

void LSPInterface::emitDiagnostics(
    const std::string &filePath,
    const std::vector<meadows::CompilationError> &errors) {
  emitDiagnosticsJSON(filePath, errors);
}

void LSPInterface::emitDiagnosticsJSON(
    const std::string &filePath,
    const std::vector<meadows::CompilationError> &errors) {
  // Output JSON
  std::cout << "{" << std::endl;
  std::cout << "  \"file\": \"" << escapeJson(filePath) << "\"," << std::endl;
  std::cout << "  \"diagnostics\": [" << std::endl;

  for (size_t i = 0; i < errors.size(); i++) {
    const auto &e = errors[i];
    std::cout << "    {" << std::endl;
    std::cout << "      \"range\": {" << std::endl;
    std::cout << "        \"start\": {\"line\": "
              << (e.getLocation().line - LSP_LINE_OFFSET) << ", \"character\": "
              << (e.getLocation().column - LSP_COLUMN_OFFSET) << "},"
              << std::endl;
    std::cout << "        \"end\": {\"line\": "
              << (e.getLocation().line - LSP_LINE_OFFSET) << ", \"character\": "
              << (e.getLocation().endColumn - LSP_COLUMN_OFFSET) << "}"
              << std::endl;
    std::cout << "      }," << std::endl;
    std::cout << "      \"severity\": " << static_cast<int>(e.getSeverity())
              << "," << std::endl;
    std::cout << "      \"message\": \"" << escapeJson(e.getMessage()) << "\","
              << std::endl;
    std::cout << "      \"code\": \"" << static_cast<int>(e.getCode()) << "\""
              << std::endl;
    std::cout << "    }";
    if (i < errors.size() - 1) {
      std::cout << ",";
    }
    std::cout << std::endl;
  }

  std::cout << "  ]" << std::endl;
  std::cout << "}" << std::endl;
}

LSPDiagnostic LSPInterface::parseError(const std::string &error,
                                       const std::string &filePath) {
  LSPDiagnostic diagnostic;
  diagnostic.line = 1;
  diagnostic.startColumn = 1;
  diagnostic.endColumn = 1;
  diagnostic.severity = static_cast<int>(LSPSeverity::Error);
  diagnostic.message = error;
  diagnostic.source = "meadows";

  // Parse error format: "Error at line X, column Y: message"
  std::regex lineColRegex(R"(Error at line (\d+), column (\d+):\s*(.+))");
  std::smatch match;

  if (std::regex_search(error, match, lineColRegex)) {
    diagnostic.line = std::stoi(match[1].str());
    diagnostic.startColumn = std::stoi(match[2].str());
    diagnostic.message = match[3].str();
    diagnostic.endColumn =
        estimateEndColumn(diagnostic.startColumn, diagnostic.message);
  } else {
    // Try alternative format: "file:line:column: error: message"
    std::regex fileLineColRegex(
        R"(([^:]+):(\d+):(\d+):\s*(error|warning):\s*(.+))");
    if (std::regex_search(error, match, fileLineColRegex)) {
      diagnostic.line = std::stoi(match[2].str());
      diagnostic.startColumn = std::stoi(match[3].str());
      diagnostic.message = match[5].str();
      diagnostic.severity = (match[4].str() == "warning")
                                ? static_cast<int>(LSPSeverity::Warning)
                                : static_cast<int>(LSPSeverity::Error);
      diagnostic.endColumn =
          estimateEndColumn(diagnostic.startColumn, diagnostic.message);
    }
  }

  return diagnostic;
}

meadows::CompilationError
LSPInterface::parseErrorToCompilationError(const std::string &error,
                                           const std::string &filePath) {
  int line = 1;
  int column = 1;
  std::string message = error;
  meadows::Severity severity = meadows::Severity::ERROR;

  // Parse error format: "Error at line X, column Y: message"
  std::regex lineColRegex(R"(Error at line (\d+), column (\d+):\s*(.+))");
  std::smatch match;

  if (std::regex_search(error, match, lineColRegex)) {
    line = std::stoi(match[1].str());
    column = std::stoi(match[2].str());
    message = match[3].str();
  } else {
    // Try alternative format: "file:line:column: error: message"
    std::regex fileLineColRegex(
        R"(([^:]+):(\d+):(\d+):\s*(error|warning):\s*(.+))");
    if (std::regex_search(error, match, fileLineColRegex)) {
      line = std::stoi(match[2].str());
      column = std::stoi(match[3].str());
      message = match[5].str();
      severity = (match[4].str() == "warning") ? meadows::Severity::WARNING
                                               : meadows::Severity::ERROR;
    }
  }

  return meadows::CompilationError(
      meadows::ErrorCode::PARSE_UNEXPECTED_TOKEN, message,
      meadows::SourceLocation(filePath, line, column));
}

std::string LSPInterface::escapeJson(const std::string &str) {
  std::ostringstream oss;
  for (char c : str) {
    switch (c) {
    case '"':
      oss << "\\\"";
      break;
    case '\\':
      oss << "\\\\";
      break;
    case '\b':
      oss << "\\b";
      break;
    case '\f':
      oss << "\\f";
      break;
    case '\n':
      oss << "\\n";
      break;
    case '\r':
      oss << "\\r";
      break;
    case '\t':
      oss << "\\t";
      break;
    default:
      if (c >= 0x20 && c <= 0x7E) {
        oss << c;
      } else {
        oss << "\\u" << std::hex << std::setw(4) << std::setfill('0')
            << (static_cast<unsigned int>(c) & 0xFF);
      }
    }
  }
  return oss.str();
}

int LSPInterface::estimateEndColumn(int startColumn,
                                    const std::string &tokenValue) {
  // Estimate end column based on token length
  return startColumn + static_cast<int>(tokenValue.length());
}

int LSPInterface::severityFromString(const std::string &severity) {
  if (severity == "error")
    return static_cast<int>(LSPSeverity::Error);
  if (severity == "warning")
    return static_cast<int>(LSPSeverity::Warning);
  if (severity == "information")
    return static_cast<int>(LSPSeverity::Information);
  if (severity == "hint")
    return static_cast<int>(LSPSeverity::Hint);
  return static_cast<int>(LSPSeverity::Error);
}
