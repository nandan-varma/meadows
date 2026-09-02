#include "lexer/Lexer.h"
#include "parser/Parser.h"
#include "sema/SemanticAnalyzer.h"
#include "utils/DiagnosticsCollector.h"
#include "utils/WarningManager.h"
#include <catch2/catch_all.hpp>
#include <memory>
#include <vector>

using namespace meadows;

static std::pair<DiagnosticsCollector, bool>
analyze(const std::string &src,
        WarningManager::Level level = WarningManager::Level::ALL,
        const std::string &filename = "") {
  Lexer lexer(src);
  auto tokens = lexer.tokenize();

  DiagnosticsCollector diag;
  Parser parser(tokens, diag);
  auto stmts = parser.parse();

  WarningManager wm;
  wm.setLevel(level);

  SemanticAnalyzer sema(diag, wm, filename);
  bool ok = sema.analyze(stmts);
  return {std::move(diag), ok};
}

// ── Name resolution ───────────────────────────────────────────────────────────

TEST_CASE("SemanticAnalyzer: defined variable accepted", "[sema]") {
  auto [diag, ok] = analyze("let x = 1; print(x);");
  CHECK(ok);
  CHECK(diag.errorCount() == 0);
}

TEST_CASE("SemanticAnalyzer: undefined variable is an error", "[sema]") {
  auto [diag, ok] = analyze("print(undefined_var);");
  CHECK_FALSE(ok);
  CHECK(diag.errorCount() >= 1);
  CHECK(diag.diagnostics()[0].code == ErrorCode::SEM_UNDEFINED_VARIABLE);
}

TEST_CASE("SemanticAnalyzer: defined function accepted", "[sema]") {
  auto [diag, ok] = analyze("func add(a, b) { return a + b; } let r = add(1, 2);");
  CHECK(ok);
  CHECK(diag.errorCount() == 0);
}

TEST_CASE("SemanticAnalyzer: undefined function is an error", "[sema]") {
  auto [diag, ok] = analyze("let r = missing(1);");
  CHECK_FALSE(ok);
  CHECK(diag.errorCount() >= 1);
  CHECK(diag.diagnostics()[0].code == ErrorCode::SEM_UNDEFINED_FUNCTION);
}

TEST_CASE("SemanticAnalyzer: wrong argument count is an error", "[sema]") {
  auto [diag, ok] = analyze("func f(a, b) { return a + b; } f(1);");
  CHECK_FALSE(ok);
  CHECK(diag.errorCount() >= 1);
  CHECK(diag.diagnostics()[0].code == ErrorCode::SEM_INVALID_ARGUMENT_COUNT);
}

TEST_CASE("SemanticAnalyzer: recursive function accepted", "[sema]") {
  auto [diag, ok] = analyze(
      "func factorial(n) { if (n <= 1) { return 1; } return n * factorial(n - 1); }");
  CHECK(ok);
  CHECK(diag.errorCount() == 0);
}

// ── Control flow ──────────────────────────────────────────────────────────────

TEST_CASE("SemanticAnalyzer: return inside function is valid", "[sema]") {
  auto [diag, ok] = analyze("func f() { return 1; }");
  CHECK(ok);
  CHECK(diag.errorCount() == 0);
}

TEST_CASE("SemanticAnalyzer: return at top level is an error", "[sema]") {
  auto [diag, ok] = analyze("return 1;");
  CHECK_FALSE(ok);
  CHECK(diag.errorCount() >= 1);
  CHECK(diag.diagnostics()[0].code == ErrorCode::SEM_RETURN_OUTSIDE_FUNCTION);
}

TEST_CASE("SemanticAnalyzer: break inside loop is valid", "[sema]") {
  auto [diag, ok] = analyze("while (1 == 1) { break; }");
  CHECK(ok);
  CHECK(diag.errorCount() == 0);
}

TEST_CASE("SemanticAnalyzer: break outside loop is an error", "[sema]") {
  auto [diag, ok] = analyze("break;");
  CHECK_FALSE(ok);
  CHECK(diag.errorCount() >= 1);
  CHECK(diag.diagnostics()[0].code == ErrorCode::SEM_BREAK_OUTSIDE_LOOP);
}

TEST_CASE("SemanticAnalyzer: continue outside loop is an error", "[sema]") {
  auto [diag, ok] = analyze("continue;");
  CHECK_FALSE(ok);
  CHECK(diag.errorCount() >= 1);
  CHECK(diag.diagnostics()[0].code == ErrorCode::SEM_CONTINUE_OUTSIDE_LOOP);
}

// ── Warnings ──────────────────────────────────────────────────────────────────

TEST_CASE("SemanticAnalyzer: unused variable warning", "[sema]") {
  auto [diag, ok] = analyze("let unused = 42;", WarningManager::Level::ALL);
  CHECK(ok); // warnings don't fail analysis
  CHECK(diag.warningCount() >= 1);
  CHECK(diag.diagnostics()[0].code == ErrorCode::WARN_UNUSED_VARIABLE);
}

TEST_CASE("SemanticAnalyzer: used variable no warning", "[sema]") {
  auto [diag, ok] = analyze("let x = 1; print(x);", WarningManager::Level::ALL);
  CHECK(ok);
  CHECK(diag.warningCount() == 0);
}

TEST_CASE("SemanticAnalyzer: shadowing warning", "[sema]") {
  auto [diag, ok] = analyze(
      "let x = 1; { let x = 2; print(x); } print(x);",
      WarningManager::Level::ALL);
  CHECK(ok);
  bool hasShadow = false;
  for (const auto &d : diag.diagnostics()) {
    if (d.code == ErrorCode::WARN_SHADOWING_VARIABLE) { hasShadow = true; break; }
  }
  CHECK(hasShadow);
}

TEST_CASE("SemanticAnalyzer: unreachable code warning", "[sema]") {
  auto [diag, ok] = analyze(
      "func f() { return 1; print(42); }",
      WarningManager::Level::ALL);
  CHECK(ok);
  bool hasUnreachable = false;
  for (const auto &d : diag.diagnostics()) {
    if (d.code == ErrorCode::WARN_UNREACHABLE_CODE) { hasUnreachable = true; break; }
  }
  CHECK(hasUnreachable);
}

TEST_CASE("SemanticAnalyzer: warnings suppressed with Level::OFF", "[sema]") {
  auto [diag, ok] = analyze("let unused = 42;", WarningManager::Level::OFF);
  CHECK(ok);
  CHECK(diag.warningCount() == 0);
}

TEST_CASE("SemanticAnalyzer: print() arg count validated", "[sema]") {
  SECTION("wrong arg count") {
    auto [diag, ok] = analyze("print(1, 2);");
    CHECK_FALSE(ok);
    CHECK(diag.errorCount() >= 1);
  }
  SECTION("correct arg count") {
    auto [diag, ok] = analyze("print(42);");
    CHECK(ok);
  }
}

// ── Redefinition detection ────────────────────────────────────────────────────

TEST_CASE("SemanticAnalyzer: duplicate function definition is an error", "[sema]") {
  auto [diag, ok] = analyze("func add(a) { return a; } func add(b) { return b; }");
  CHECK_FALSE(ok);
  CHECK(diag.errorCount() >= 1);
  bool found = false;
  for (const auto &d : diag.diagnostics()) {
    if (d.code == ErrorCode::SEM_REDEFINED_FUNCTION) { found = true; break; }
  }
  CHECK(found);
}

TEST_CASE("SemanticAnalyzer: duplicate variable in same scope is an error", "[sema]") {
  auto [diag, ok] = analyze("let x = 1; let x = 2;");
  CHECK_FALSE(ok);
  CHECK(diag.errorCount() >= 1);
  bool found = false;
  for (const auto &d : diag.diagnostics()) {
    if (d.code == ErrorCode::SEM_REDEFINED_VARIABLE) { found = true; break; }
  }
  CHECK(found);
}

TEST_CASE("SemanticAnalyzer: filename appears in error location", "[sema]") {
  auto [diag, ok] = analyze("print(unknown_var);",
                             WarningManager::Level::ALL, "myfile.ms");
  CHECK_FALSE(ok);
  REQUIRE(diag.errorCount() >= 1);
  CHECK(diag.diagnostics()[0].location.file == "myfile.ms");
}

// ── Built-ins ────────────────────────────────────────────────────────────────

TEST_CASE("SemanticAnalyzer: len() accepts 1 argument", "[sema]") {
  auto [diag, ok] = analyze("let s = \"hello\"; print(len(s));");
  CHECK(ok);
  CHECK(diag.errorCount() == 0);
}

TEST_CASE("SemanticAnalyzer: len() rejects wrong arg count", "[sema]") {
  auto [diag, ok] = analyze("len();");
  CHECK_FALSE(ok);
  CHECK(diag.errorCount() >= 1);
  CHECK(diag.diagnostics()[0].code == ErrorCode::SEM_INVALID_ARGUMENT_COUNT);
}

TEST_CASE("SemanticAnalyzer: str() accepts 1 argument", "[sema]") {
  auto [diag, ok] = analyze("let n = 42; print(str(n));");
  CHECK(ok);
  CHECK(diag.errorCount() == 0);
}

// ── Empty return ─────────────────────────────────────────────────────────────

TEST_CASE("SemanticAnalyzer: bare return inside function is valid", "[sema]") {
  auto [diag, ok] = analyze("func f() { return; }");
  CHECK(ok);
  CHECK(diag.errorCount() == 0);
}

TEST_CASE("SemanticAnalyzer: bare return at top level is an error", "[sema]") {
  auto [diag, ok] = analyze("return;");
  CHECK_FALSE(ok);
  CHECK(diag.errorCount() >= 1);
  CHECK(diag.diagnostics()[0].code == ErrorCode::SEM_RETURN_OUTSIDE_FUNCTION);
}

// ── Field access validation ──────────────────────────────────────────────────

TEST_CASE("SemanticAnalyzer: unknown field on object literal is an error",
          "[sema]") {
  auto [diag, ok] = analyze("let v = {a: 1, b: 2}.c;");
  CHECK_FALSE(ok);
  REQUIRE(diag.errorCount() >= 1);
  bool found = false;
  for (const auto &d : diag.diagnostics()) {
    if (d.code == ErrorCode::SEM_UNKNOWN_FIELD) { found = true; break; }
  }
  CHECK(found);
}

TEST_CASE("SemanticAnalyzer: known field on object literal accepted",
          "[sema]") {
  auto [diag, ok] = analyze("let v = {a: 1, b: 2}.a; print(v);");
  CHECK(ok);
  CHECK(diag.errorCount() == 0);
}

// ── Assignment targets ────────────────────────────────────────────────────────

TEST_CASE("SemanticAnalyzer: array index assignment accepted", "[sema][assign]") {
  auto [diag, ok] = analyze("let arr = [1, 2, 3]; arr[0] = 5; print(arr[0]);");
  CHECK(ok);
  CHECK(diag.errorCount() == 0);
}

TEST_CASE("SemanticAnalyzer: array index assignment to an undefined array is "
         "an error",
         "[sema][assign]") {
  auto [diag, ok] = analyze("missing[0] = 5;");
  CHECK_FALSE(ok);
  REQUIRE(diag.errorCount() >= 1);
  CHECK(diag.diagnostics()[0].code == ErrorCode::SEM_UNDEFINED_VARIABLE);
}

TEST_CASE("SemanticAnalyzer: object field assignment accepted", "[sema][assign]") {
  auto [diag, ok] = analyze("let o = {a: 1}; o.a = 5; print(o.a);");
  CHECK(ok);
  CHECK(diag.errorCount() == 0);
}

TEST_CASE("SemanticAnalyzer: unknown field assignment on a literal is an error",
         "[sema][assign]") {
  // Parenthesized: a bare leading `{` at statement position parses as a
  // block, not an object literal (see Parser::parseStmt).
  auto [diag, ok] = analyze("({a: 1}.b = 5);");
  CHECK_FALSE(ok);
  REQUIRE(diag.errorCount() >= 1);
  CHECK(diag.diagnostics()[0].code == ErrorCode::SEM_UNKNOWN_FIELD);
}
