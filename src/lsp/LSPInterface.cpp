#include "LSPInterface.h"
#include "../utils/Exceptions.h"
#include <iostream>
#include <sstream>

LSPInterface::LSPInterface() {}

void LSPInterface::emitDiagnostics(
    const std::string &filePath,
    const std::vector<meadows::Diagnostic> &diagnostics) {
  // Output JSON
  std::cout << "{" << std::endl;
  std::cout << "  \"file\": \"" << escapeJson(filePath) << "\"," << std::endl;
  std::cout << "  \"diagnostics\": [" << std::endl;

  for (size_t i = 0; i < diagnostics.size(); i++) {
    const auto &d = diagnostics[i];
    std::cout << "    {" << std::endl;
    std::cout << "      \"range\": {" << std::endl;
    std::cout << "        \"start\": {\"line\": "
              << (d.location.line - LSP_LINE_OFFSET)
              << ", \"character\": " << (d.location.column - LSP_COLUMN_OFFSET)
              << "}," << std::endl;
    std::cout << "        \"end\": {\"line\": "
              << (d.location.line - LSP_LINE_OFFSET) << ", \"character\": "
              << (d.location.endColumn - LSP_COLUMN_OFFSET) << "}" << std::endl;
    std::cout << "      }," << std::endl;
    std::cout << "      \"severity\": " << severityFromString(d.severity) << ","
              << std::endl;
    std::cout << "      \"code\": \"" << meadows::errorCodeToString(d.code)
              << "\"," << std::endl;
    std::cout << "      \"message\": \"" << escapeJson(d.message) << "\","
              << std::endl;
    std::cout << "      \"source\": \"meadows-compiler\"";

    // Add related information if available
    if (!d.relatedInfo.empty()) {
      std::cout << "," << std::endl;
      std::cout << "      \"relatedInformation\": [" << std::endl;
      for (size_t j = 0; j < d.relatedInfo.size(); j++) {
        const auto &rel = d.relatedInfo[j];
        std::cout << "        {" << std::endl;
        std::cout << "          \"location\": {" << std::endl;
        std::cout << "            \"uri\": \"file://"
                  << escapeJson(rel.first.file) << "\"," << std::endl;
        std::cout << "            \"range\": {" << std::endl;
        std::cout << "              \"start\": {\"line\": "
                  << (rel.first.line - LSP_LINE_OFFSET) << ", \"character\": "
                  << (rel.first.column - LSP_COLUMN_OFFSET) << "},"
                  << std::endl;
        std::cout << "              \"end\": {\"line\": "
                  << (rel.first.line - LSP_LINE_OFFSET) << ", \"character\": "
                  << (rel.first.endColumn - LSP_COLUMN_OFFSET) << "}"
                  << std::endl;
        std::cout << "            }" << std::endl;
        std::cout << "          }," << std::endl;
        std::cout << "          \"message\": \"" << escapeJson(rel.second)
                  << "\"" << std::endl;
        std::cout << "        }";
        if (j < d.relatedInfo.size() - 1)
          std::cout << ",";
        std::cout << std::endl;
      }
      std::cout << "      ]" << std::endl;
    } else {
      std::cout << std::endl;
    }

    std::cout << "    }";
    if (i < diagnostics.size() - 1) {
      std::cout << ",";
    }
    std::cout << std::endl;
  }

  std::cout << "  ]" << std::endl;
  std::cout << "}" << std::endl;
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
      constexpr unsigned char ASCII_PRINTABLE_MIN = 0x20;
      constexpr unsigned char ASCII_PRINTABLE_MAX = 0x7E;
      if (c >= ASCII_PRINTABLE_MIN && c <= ASCII_PRINTABLE_MAX) {
        oss << c;
      } else {
        oss << "\\u" << std::hex << (static_cast<unsigned int>(c) & 0xFF);
      }
    }
  }
  return oss.str();
}

int LSPInterface::severityFromString(const std::string &severity) {
  if (severity == "error")
    return static_cast<int>(LSPSeverity::Error);
  if (severity == "warning")
    return static_cast<int>(LSPSeverity::Warning);
  if (severity == "info")
    return static_cast<int>(LSPSeverity::Information);
  if (severity == "hint")
    return static_cast<int>(LSPSeverity::Hint);
  return static_cast<int>(LSPSeverity::Error);
}
