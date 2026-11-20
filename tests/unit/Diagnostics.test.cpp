#include <catch2/catch_test_macros.hpp>

#include "utils/Diagnostics.h"

using namespace meadows;

namespace {

struct Fixture {
  SourceMap sources;
  uint32_t file = sources.add("t.ms", "let x = 1;\nprint(y);\n");
};

} // namespace

TEST_CASE("diagnostic codes render with their category letter", "[diagnostics]") {
  CHECK(codeString(ErrorCode::SEM_UNDEFINED_VARIABLE) == "E3001");
  CHECK(codeString(ErrorCode::WARN_UNUSED_VARIABLE) == "W6001");
  CHECK(isWarning(ErrorCode::WARN_SHADOWING_VARIABLE));
  CHECK_FALSE(isWarning(ErrorCode::LEX_INVALID_CHARACTER));
  CHECK(std::string(warningName(ErrorCode::WARN_UNREACHABLE_CODE)) ==
        "unreachable-code");
}

TEST_CASE("source map exposes individual lines", "[diagnostics]") {
  Fixture f;
  CHECK(f.sources.lineText(f.file, 1) == "let x = 1;");
  CHECK(f.sources.lineText(f.file, 2) == "print(y);");
  CHECK(f.sources.lineText(f.file, 9).empty());
}

TEST_CASE("diagnostics render the offending line with a caret",
          "[diagnostics]") {
  Fixture f;
  Diagnostic d{ErrorCode::SEM_UNDEFINED_VARIABLE, "Undefined variable 'y'",
               "declare it first", Location{f.file, 2, 7}, 1};
  CHECK(formatDiagnostic(d, f.sources) ==
        "error[E3001]: Undefined variable 'y'\n"
        " --> t.ms:2:7\n"
        "  |\n"
        "2 | print(y);\n"
        "  |       ^\n"
        "  = help: declare it first\n");
}

TEST_CASE("summary counts errors and warnings", "[diagnostics]") {
  Fixture f;
  std::vector<Diagnostic> list = {
      {ErrorCode::SEM_UNDEFINED_VARIABLE, "e", "", Location{f.file, 1, 1}, 1},
      {ErrorCode::WARN_UNUSED_VARIABLE, "w", "", Location{f.file, 1, 5}, 1}};
  std::string text = formatDiagnostics(list, f.sources);
  CHECK(text.find("1 error, 1 warning\n") != std::string::npos);
}

TEST_CASE("diagnostics serialise to LSP-shaped JSON", "[diagnostics]") {
  Fixture f;
  std::vector<Diagnostic> list = {{ErrorCode::SEM_UNDEFINED_VARIABLE,
                                   "say \"hi\"\n", "", Location{f.file, 2, 7},
                                   1}};
  CHECK(diagnosticsToJson(list, f.sources) ==
        "[{\"file\":\"t.ms\",\"range\":{\"start\":{\"line\":1,\"character\":6},"
        "\"end\":{\"line\":1,\"character\":7}},\"severity\":1,\"code\":"
        "\"E3001\",\"message\":\"say \\\"hi\\\"\\n\",\"source\":\"meadows\"}]");
}

TEST_CASE("the sink caps how many diagnostics it keeps", "[diagnostics]") {
  DiagnosticSink sink;
  for (int i = 0; i < 500; ++i) {
    sink.report(ErrorCode::LEX_INVALID_CHARACTER, "x", Location{0, 1, 1});
  }
  CHECK(sink.diagnostics().size() == DiagnosticSink::MAX_DIAGNOSTICS);
  CHECK(sink.hasErrors());
  CHECK(sink.full());
}
