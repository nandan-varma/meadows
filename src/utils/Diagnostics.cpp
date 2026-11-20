#include "Diagnostics.h"

#include <algorithm>
#include <cstdio>
#include <tuple>

namespace meadows {

namespace {

constexpr const char* RED = "\033[1;31m";
constexpr const char* YELLOW = "\033[1;33m";
constexpr const char* BLUE = "\033[1;34m";
constexpr const char* RESET = "\033[0m";

std::string escapeJson(const std::string& text) {
  std::string out;
  for (char ch : text) {
    unsigned char c = static_cast<unsigned char>(ch);
    switch (c) {
    case '"':
      out += "\\\"";
      break;
    case '\\':
      out += "\\\\";
      break;
    case '\n':
      out += "\\n";
      break;
    case '\r':
      out += "\\r";
      break;
    case '\t':
      out += "\\t";
      break;
    default:
      if (c < 0x20) {
        char buf[8];
        std::snprintf(buf, sizeof(buf), "\\u%04x", c);
        out += buf;
      } else {
        out += static_cast<char>(c);
      }
    }
  }
  return out;
}

std::vector<Diagnostic> inSourceOrder(std::vector<Diagnostic> diagnostics) {
  std::stable_sort(diagnostics.begin(), diagnostics.end(),
                   [](const Diagnostic& a, const Diagnostic& b) {
                     const Location& x = a.location;
                     const Location& y = b.location;
                     return std::tie(x.file, x.line, x.column) <
                            std::tie(y.file, y.line, y.column);
                   });
  return diagnostics;
}

std::string paint(const FormatOptions& options, const char* color,
                  const std::string& text) {
  return options.color ? color + text + RESET : text;
}

} // namespace

void DiagnosticSink::report(ErrorCode code, std::string message,
                            Location location, uint32_t length,
                            std::string help) {
  if (isWarning(code)) {
    if (policy_.disabled.count(code)) {
      return;
    }
    if (policy_.treatAsErrors) {
      ++errorCount_;
    }
  } else {
    ++errorCount_;
  }
  if (full()) {
    return;
  }
  diagnostics_.push_back({code, std::move(message), std::move(help), location,
                          length});
}

std::string formatDiagnostic(const Diagnostic& diagnostic,
                             const SourceMap& sources,
                             const FormatOptions& options) {
  const Location& loc = diagnostic.location;
  std::string severity = diagnostic.isWarning() ? "warning" : "error";
  std::string head = severity + "[" + codeString(diagnostic.code) + "]";
  std::string out =
      paint(options, diagnostic.isWarning() ? YELLOW : RED, head) + ": " +
      diagnostic.message + "\n";

  std::string gutter(std::to_string(loc.line).size(), ' ');
  out += gutter + paint(options, BLUE, "--> ") + formatLocation(sources, loc) +
         "\n";

  std::string_view line = sources.lineText(loc.file, loc.line);
  if (!line.empty()) {
    out += gutter + paint(options, BLUE, " |") + "\n";
    out += paint(options, BLUE, std::to_string(loc.line) + " |") + " " +
           std::string(line) + "\n";
    size_t pad = loc.column > 0 ? loc.column - 1 : 0;
    size_t width = std::min<size_t>(
        diagnostic.length, line.size() > pad ? line.size() - pad : 1);
    out += gutter + paint(options, BLUE, " |") + " " + std::string(pad, ' ') +
           paint(options, diagnostic.isWarning() ? YELLOW : RED,
                 std::string(std::max<size_t>(width, 1), '^')) +
           "\n";
  }
  if (!diagnostic.help.empty()) {
    out += gutter + paint(options, BLUE, " = ") + "help: " + diagnostic.help +
           "\n";
  }
  return out;
}

std::string formatDiagnostics(const std::vector<Diagnostic>& diagnostics,
                              const SourceMap& sources,
                              const FormatOptions& options) {
  std::string out;
  size_t errors = 0;
  size_t warnings = 0;
  for (const Diagnostic& d : inSourceOrder(diagnostics)) {
    out += formatDiagnostic(d, sources, options) + "\n";
    (d.isWarning() ? warnings : errors)++;
  }
  if (errors + warnings > 0) {
    out += std::to_string(errors) + (errors == 1 ? " error, " : " errors, ") +
           std::to_string(warnings) + (warnings == 1 ? " warning" : " warnings") +
           "\n";
  }
  return out;
}

std::string diagnosticsToJson(const std::vector<Diagnostic>& unsorted,
                              const SourceMap& sources) {
  std::vector<Diagnostic> diagnostics = inSourceOrder(unsorted);
  std::string out = "[";
  for (size_t i = 0; i < diagnostics.size(); ++i) {
    const Diagnostic& d = diagnostics[i];
    const Location& loc = d.location;
    uint32_t line = loc.line > 0 ? loc.line - 1 : 0;
    uint32_t start = loc.column > 0 ? loc.column - 1 : 0;
    out += i ? "," : "";
    out += "{\"file\":\"" + escapeJson(sources.path(loc.file)) + "\"";
    out += ",\"range\":{\"start\":{\"line\":" + std::to_string(line) +
           ",\"character\":" + std::to_string(start) + "},\"end\":{\"line\":" +
           std::to_string(line) + ",\"character\":" +
           std::to_string(start + d.length) + "}}";
    out += ",\"severity\":" + std::string(d.isWarning() ? "2" : "1");
    out += ",\"code\":\"" + codeString(d.code) + "\"";
    out += ",\"message\":\"" + escapeJson(d.message) + "\"";
    out += ",\"source\":\"meadows\"}";
  }
  return out + "]";
}

} // namespace meadows
