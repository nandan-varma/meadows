#include "interpreter/Interpreter.h"

#include "lexer/Lexer.h"
#include "parser/Parser.h"
#include "sema/SemanticAnalyzer.h"
#include "utils/DiagnosticsCollector.h"
#include "utils/WarningManager.h"
#include <catch2/catch_all.hpp>

using namespace meadows;

namespace {

struct RunResult {
  std::string output;
  int exitCode;
};

// Runs source through the full pipeline the WASM bridge uses: lex, parse,
// semantic analysis, then interpretation — asserting each stage succeeds,
// since a real program under test should never fail an earlier stage.
RunResult run(const std::string &source) {
  Lexer lexer(source);
  auto tokens = lexer.tokenize();

  DiagnosticsCollector diag;
  Parser parser(tokens, diag);
  auto stmts = parser.parse();
  REQUIRE_FALSE(diag.hasErrors());

  WarningManager warnings;
  SemanticAnalyzer sema(diag, warnings);
  sema.analyze(stmts);
  REQUIRE_FALSE(diag.hasErrors());

  std::string output;
  Interpreter interp([&output](const std::string &chunk) { output += chunk; });
  int exitCode = interp.run(stmts);
  return {output, exitCode};
}

} // namespace

TEST_CASE("Interpreter: literals and print", "[interpreter]") {
  auto r = run(R"(print(42); print("hello");)");
  CHECK(r.output == "42\nhello\n");
  CHECK(r.exitCode == 0);
}

TEST_CASE("Interpreter: arithmetic matches CodeGen's i32 wraparound", "[interpreter]") {
  CHECK(run("print(2 + 3);").output == "5\n");
  CHECK(run("print(2 - 5);").output == "-3\n");
  CHECK(run("print(4 * 6);").output == "24\n");
  CHECK(run("print(7 / 2);").output == "3\n");
  CHECK(run("print(7 % 3);").output == "1\n");
  CHECK(run(R"(print(-7);)").output == "-7\n");
  // i32 add overflow wraps rather than trapping (matches LLVM's CreateAdd).
  CHECK(run("print(2147483647 + 1);").output == "-2147483648\n");
}

TEST_CASE("Interpreter: comparisons yield 0/1 integers", "[interpreter]") {
  CHECK(run("print(3 == 3); print(3 != 3); print(3 < 4); print(4 <= 4);")
            .output == "1\n0\n1\n1\n");
}

TEST_CASE("Interpreter: string concatenation", "[interpreter]") {
  CHECK(run(R"(print("a" + "b");)").output == "ab\n");
  // Superset of the native backend, which only permits compile-time-constant
  // integers here — see docs/LANGUAGE.md.
  CHECK(run(R"(let n = 5; print("n=" + n);)").output == "n=5\n");
}

TEST_CASE("Interpreter: && and || forward operand values, not strict booleans",
         "[interpreter]") {
  // Mirrors the PHI nodes CodeGen::visitLogicalExpr builds.
  auto r = run("print(5 && 10); print(0 && 10); print(5 || 10); print(0 || 10);");
  CHECK(r.output == "10\n0\n5\n10\n");
}

TEST_CASE("Interpreter: unary operators", "[interpreter]") {
  CHECK(run("print(!0); print(!5);").output == "1\n0\n");
}

TEST_CASE("Interpreter: if/else", "[interpreter]") {
  CHECK(run("if (1 == 1) { print(1); } else { print(2); }").output == "1\n");
  CHECK(run("if (1 == 2) { print(1); } else { print(2); }").output == "2\n");
}

TEST_CASE("Interpreter: while with break and continue", "[interpreter]") {
  auto r = run(R"(
    let i = 0;
    let sum = 0;
    while (i < 10) {
      i = i + 1;
      if (i == 3) { continue; }
      if (i == 6) { break; }
      sum = sum + i;
    }
    print(sum);
  )");
  CHECK(r.output == "12\n"); // 1+2+4+5
}

TEST_CASE("Interpreter: for-range loop", "[interpreter]") {
  auto r = run("for (i in range(0, 5)) { print(i); }");
  CHECK(r.output == "0\n1\n2\n3\n4\n");
}

TEST_CASE("Interpreter: function calls and recursion", "[interpreter]") {
  auto r = run(R"(
    func fib(n) {
      if (n <= 1) { return n; }
      return fib(n - 1) + fib(n - 2);
    }
    print(fib(10));
  )");
  CHECK(r.output == "55\n");
}

TEST_CASE("Interpreter: forward and mutual recursion between top-level functions",
         "[interpreter]") {
  // Deliberately more capable than the native backend here: CodeGen has no
  // pre-pass, so a call to a not-yet-generated function fails. The
  // interpreter hoists top-level function names first (matching
  // SemanticAnalyzer's pre-pass), so forward calls resolve.
  auto r = run(R"(
    func isEven(n) {
      if (n == 0) { return 1; }
      return isOdd(n - 1);
    }
    func isOdd(n) {
      if (n == 0) { return 0; }
      return isEven(n - 1);
    }
    print(isEven(10));
  )");
  CHECK(r.output == "1\n");
}

TEST_CASE("Interpreter: bare return yields 0", "[interpreter]") {
  auto r = run(R"(
    func report(n) { print(n); return; }
    report(7);
  )");
  CHECK(r.output == "7\n");
}

TEST_CASE("Interpreter: arrays support arbitrary element types and len()",
         "[interpreter]") {
  // Superset of the native backend, which is i32-elements-only and doesn't
  // track array length at runtime — see docs/LANGUAGE.md limitations.
  auto r = run(R"(
    let a = [1, 2, 3];
    print(a[0]);
    print(a[2]);
    print(len(a));
  )");
  CHECK(r.output == "1\n3\n3\n");
}

TEST_CASE("Interpreter: array index out of bounds is a runtime error", "[interpreter]") {
  auto r = run("let a = [1, 2, 3]; print(a[5]);");
  CHECK(r.exitCode == -1);
  CHECK(r.output.find("RuntimeError: Array index out of bounds") != std::string::npos);
}

TEST_CASE("Interpreter: division and modulo by zero are runtime errors", "[interpreter]") {
  auto div = run("let x = 1; let y = 0; print(x / y);");
  CHECK(div.exitCode == -1);
  CHECK(div.output.find("RuntimeError: Division by zero") != std::string::npos);

  auto mod = run("let x = 1; let y = 0; print(x % y);");
  CHECK(mod.exitCode == -1);
}

TEST_CASE("Interpreter: a runtime error stops execution but keeps prior output",
         "[interpreter]") {
  auto r = run(R"(
    print(1);
    let x = 1; let y = 0;
    print(x / y);
    print(999);
  )");
  CHECK(r.exitCode == -1);
  CHECK(r.output.find("1\n") == 0);
  CHECK(r.output.find("999") == std::string::npos);
}

TEST_CASE("Interpreter: object literals and field access", "[interpreter]") {
  auto r = run(R"(
    let o = {name: "Alice", age: 30};
    print(o.name);
    print(o.age);
  )");
  CHECK(r.output == "Alice\n30\n");
}

TEST_CASE("Interpreter: field access works through a variable", "[interpreter]") {
  // The native backend only resolves field access when the object is an
  // inline literal at the access site — see docs/LANGUAGE.md. The
  // interpreter implements full field access since nothing about the
  // language design requires that restriction.
  auto r = run(R"(
    let o = {x: 10, y: 20};
    let p = o;
    print(p.x + p.y);
  )");
  CHECK(r.output == "30\n");
}

TEST_CASE("Interpreter: unknown field on a variable-bound object is a runtime error",
         "[interpreter]") {
  auto r = run(R"(let o = {a: 1}; print(o.b);)");
  CHECK(r.exitCode == -1);
  CHECK(r.output.find("RuntimeError") != std::string::npos);
}

TEST_CASE("Interpreter: len() and str() builtins", "[interpreter]") {
  CHECK(run(R"(print(len("abcdefgh"));)").output == "8\n");
  CHECK(run("print(str(1234));").output == "1234\n");
  CHECK(run("print(str(-7));").output == "-7\n");
}

TEST_CASE("Interpreter: an unbounded loop trips the step-limit safety guard",
         "[interpreter]") {
  auto r = run("let i = 0; while (1 == 1) { i = i + 1; }");
  CHECK(r.exitCode == -1);
  CHECK(r.output.find("PlaygroundError") != std::string::npos);
}

TEST_CASE("Interpreter: unbounded recursion trips the call-depth safety guard",
         "[interpreter]") {
  auto r = run("func rec(n) { return rec(n + 1); } print(rec(0));");
  CHECK(r.exitCode == -1);
  CHECK(r.output.find("PlaygroundError") != std::string::npos);
}
