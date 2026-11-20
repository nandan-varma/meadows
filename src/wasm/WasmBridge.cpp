#include <emscripten/bind.h>
#include <emscripten/val.h>

#include <string>

#include "../ast/AstPrinter.h"
#include "../driver/Frontend.h"
#include "../interpreter/Interpreter.h"
#include "../lexer/Lexer.h"
#include "../utils/PathValidation.h"

namespace {

using namespace meadows;

struct AnalysisReport {
  bool ok = false;
  std::string diagnostics;
  std::string diagnosticsJson;
  std::string tokens;
  std::string ast;
};

struct ExecutionReport {
  bool compiled = false;
  int exitCode = 0;
  std::string diagnostics;
  std::string diagnosticsJson;
  std::string runtimeError;
};

bool readEntry(const std::string& path, std::string& text,
               std::string& problem) {
  ReadResult result = readSourceFile(path, text);
  if (result != ReadResult::OK) {
    problem =
        "cannot read '" + path + "': " + describeReadFailure(result) + "\n";
  }
  return result == ReadResult::OK;
}

std::string describeTokens(const std::string& text) {
  DiagnosticSink sink;
  std::string out;
  for (const Token& token : Lexer(text, 0, sink).tokenize()) {
    out += std::to_string(token.location.line) + ":" +
           std::to_string(token.location.column) + "  " +
           tokenTypeName(token.type);
    if (token.type != TokenType::END_OF_FILE) {
      out += "  '" + token.text + "'";
    }
    out += "\n";
  }
  return out;
}

AnalysisReport analyze(const std::string& path) {
  AnalysisReport report;
  std::string text;
  if (!readEntry(path, text, report.diagnostics)) {
    report.diagnosticsJson = "[]";
    return report;
  }
  report.tokens = describeTokens(text);
  Analysis analysis = analyzeSource(path, std::move(text));
  report.ok = analysis.ok();
  report.diagnostics =
      formatDiagnostics(analysis.diagnostics.diagnostics(), analysis.sources);
  report.diagnosticsJson =
      diagnosticsToJson(analysis.diagnostics.diagnostics(), analysis.sources);
  if (report.ok) {
    report.ast = AstPrinter().print(analysis.program);
  }
  return report;
}

ExecutionReport execute(const std::string& path, emscripten::val onOutput) {
  ExecutionReport report;
  std::string text;
  if (!readEntry(path, text, report.diagnostics)) {
    report.diagnosticsJson = "[]";
    return report;
  }
  Analysis analysis = analyzeSource(path, std::move(text));
  report.diagnostics =
      formatDiagnostics(analysis.diagnostics.diagnostics(), analysis.sources);
  report.diagnosticsJson =
      diagnosticsToJson(analysis.diagnostics.diagnostics(), analysis.sources);
  report.compiled = analysis.ok();
  if (!report.compiled) {
    return report;
  }

  Interpreter interpreter([&](std::string_view chunk) {
    onOutput(emscripten::val(emscripten::typed_memory_view(
        chunk.size(), reinterpret_cast<const unsigned char*>(chunk.data()))));
  });
  std::optional<RuntimeError> error = interpreter.run(analysis.program);
  if (error) {
    report.exitCode = 1;
    report.runtimeError = formatRuntimeError(*error, analysis.sources);
  }
  return report;
}

std::string version() {
  return MEADOWS_VERSION;
}

} // namespace

EMSCRIPTEN_BINDINGS(meadows) {
  emscripten::value_object<AnalysisReport>("AnalysisReport")
      .field("ok", &AnalysisReport::ok)
      .field("diagnostics", &AnalysisReport::diagnostics)
      .field("diagnosticsJson", &AnalysisReport::diagnosticsJson)
      .field("tokens", &AnalysisReport::tokens)
      .field("ast", &AnalysisReport::ast);
  emscripten::value_object<ExecutionReport>("ExecutionReport")
      .field("compiled", &ExecutionReport::compiled)
      .field("exitCode", &ExecutionReport::exitCode)
      .field("diagnostics", &ExecutionReport::diagnostics)
      .field("diagnosticsJson", &ExecutionReport::diagnosticsJson)
      .field("runtimeError", &ExecutionReport::runtimeError);
  emscripten::function("analyze", &analyze);
  emscripten::function("execute", &execute);
  emscripten::function("version", &version);
}
