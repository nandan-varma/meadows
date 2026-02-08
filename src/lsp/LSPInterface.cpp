#include "LSPInterface.h"
#include <iostream>
#include <regex>
#include <sstream>

LSPInterface::LSPInterface() {}

void LSPInterface::emitDiagnostics(const std::string &filePath,
                                   const std::vector<Token> &tokens,
                                   const std::vector<std::string> &errors) {

  std::vector<LSPDiagnostic> diagnostics;

  // Parse each error message
  for (const auto &error : errors) {
    diagnostics.push_back(parseError(error, filePath));
  }

  // Output JSON
  std::cout << "{" << std::endl;
  std::cout << "  \"file\": \"" << escapeJson(filePath) << "\"," << std::endl;
  std::cout << "  \"diagnostics\": [" << std::endl;

  for (size_t i = 0; i < diagnostics.size(); i++) {
    const auto &d = diagnostics[i];
    std::cout << "    {" << std::endl;
    std::cout << "      \"range\": {" << std::endl;
    std::cout << "        \"start\": {\"line\": " << (d.line - 1)
              << ", \"character\": " << (d.startColumn - 1) << "},"
              << std::endl;
    std::cout << "        \"end\": {\"line\": " << (d.line - 1)
              << ", \"character\": " << (d.endColumn - 1) << "}" << std::endl;
    std::cout << "      }," << std::endl;
    std::cout << "      \"severity\": " << d.severity << "," << std::endl;
    std::cout << "      \"message\": \"" << escapeJson(d.message) << "\","
              << std::endl;
    std::cout << "      \"source\": \"meadows-compiler\"" << std::endl;
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
