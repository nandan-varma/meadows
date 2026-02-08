#include "LSPInterface.h"
#include "../utils/Exceptions.h"
#include <iostream>
#include <regex>
#include <sstream>

LSPInterface::LSPInterface() {}

void LSPInterface::emitDiagnostics(const std::string &filePath,
                                   const std::vector<Token> &tokens,
                                   const std::vector<std::string> &errors) {
  std::vector<meadows::Diagnostic> diagnostics;

  // Parse each error message
  for (const auto &error : errors) {
    diagnostics.push_back(parseErrorToDiagnostic(error, filePath));
  }

  emitDiagnosticsJSON(filePath, diagnostics);
}

void LSPInterface::emitDiagnostics(
    const std::string &filePath,
    const std::vector<meadows::Diagnostic> &diagnostics) {
  emitDiagnosticsJSON(filePath, diagnostics);
}

void LSPInterface::emitDiagnosticsJSON(
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
    std::cout << "        \"start\": {\"line\": " << (d.location.line - 1)
              << ", \"character\": " << (d.location.column - 1) << "},"
              << std::endl;
    std::cout << "        \"end\": {\"line\": " << (d.location.line - 1)
              << ", \"character\": " << (d.location.endColumn - 1) << "}"
              << std::endl;
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
                  << (rel.first.line - 1)
                  << ", \"character\": " << (rel.first.column - 1) << "},"
                  << std::endl;
        std::cout << "              \"end\": {\"line\": "
                  << (rel.first.line - 1)
                  << ", \"character\": " << (rel.first.endColumn - 1) << "}"
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

LSPDiagnostic LSPInterface::parseError(const std::string &error,
                                       const std::string &filePath) {
  LSPDiagnostic diag;
  diag.severity = 1; // Error
  diag.source = "meadows-compiler";

  // Try to parse line number from error message
  // Common patterns: "Error at line X" or "[line X]"
  std::regex lineRegex(R"(line\s+(\d+))", std::regex::icase);
  std::smatch match;

  if (std::regex_search(error, match, lineRegex)) {
    diag.line = std::stoi(match[1].str());
  } else {
    diag.line = 1; // Default to line 1 if not found
  }

  // Try to find column information
  std::regex colRegex(R"(column\s+(\d+))", std::regex::icase);
  if (std::regex_search(error, match, colRegex)) {
    diag.startColumn = std::stoi(match[1].str());
  } else {
    diag.startColumn = 1;
  }

  // Estimate end column based on token length or default to start + 1
  diag.endColumn = diag.startColumn + 1;

  // Extract the message (remove common prefixes)
  std::string msg = error;
  size_t pos = msg.find(":");
  if (pos != std::string::npos && pos < msg.length() - 1) {
    msg = msg.substr(pos + 1);
    // Trim leading whitespace
    pos = msg.find_first_not_of(" \t");
    if (pos != std::string::npos) {
      msg = msg.substr(pos);
    }
  }

  diag.message = msg;
  return diag;
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
        oss << "\\u" << std::hex << (static_cast<unsigned int>(c) & 0xFF);
      }
    }
  }
  return oss.str();
}

int LSPInterface::estimateEndColumn(int startColumn,
                                    const std::string &tokenValue) {
  return startColumn + static_cast<int>(tokenValue.length());
}

meadows::Diagnostic
LSPInterface::parseErrorToDiagnostic(const std::string &error,
                                     const std::string &filePath) {
  meadows::SourceLocation loc;
  loc.file = filePath;

  // Try to parse line number from error message
  std::regex lineRegex(R"(line\s+(\d+))", std::regex::icase);
  std::smatch match;

  if (std::regex_search(error, match, lineRegex)) {
    loc.line = std::stoi(match[1].str());
  } else {
    loc.line = 1;
  }

  // Try to find column information
  std::regex colRegex(R"(column\s+(\d+))", std::regex::icase);
  if (std::regex_search(error, match, colRegex)) {
    loc.column = std::stoi(match[1].str());
  } else {
    loc.column = 1;
  }

  // Estimate end column
  loc.endColumn = loc.column + 1;

  // Extract the message
  std::string msg = error;
  size_t pos = msg.find(":");
  if (pos != std::string::npos && pos < msg.length() - 1) {
    msg = msg.substr(pos + 1);
    pos = msg.find_first_not_of(" \t");
    if (pos != std::string::npos) {
      msg = msg.substr(pos);
    }
  }

  // Determine error code from message content
  meadows::ErrorCode code = meadows::ErrorCode::PARSE_UNEXPECTED_TOKEN;
  if (error.find("unterminated string") != std::string::npos) {
    code = meadows::ErrorCode::LEX_UNTERMINATED_STRING;
  } else if (error.find("Expect") != std::string::npos &&
             error.find("'") != std::string::npos) {
    if (error.find("';'") != std::string::npos) {
      code = meadows::ErrorCode::PARSE_EXPECTED_SEMICOLON;
    } else if (error.find("'('") != std::string::npos) {
      code = meadows::ErrorCode::PARSE_EXPECTED_LPAREN;
    } else if (error.find("')'") != std::string::npos) {
      code = meadows::ErrorCode::PARSE_EXPECTED_RPAREN;
    } else if (error.find("'{'") != std::string::npos) {
      code = meadows::ErrorCode::PARSE_EXPECTED_LBRACE;
    } else if (error.find("'}'") != std::string::npos) {
      code = meadows::ErrorCode::PARSE_EXPECTED_RBRACE;
    }
  }

  meadows::Diagnostic diag(code, msg, loc);
  return diag;
}

int LSPInterface::severityFromString(const std::string &severity) {
  if (severity == "error")
    return 1;
  if (severity == "warning")
    return 2;
  if (severity == "info")
    return 3;
  if (severity == "hint")
    return 4;
  return 1; // Default to error
}
