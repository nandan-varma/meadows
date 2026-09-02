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
  // Element-type flexibility is still a superset of the native backend
  // (i32-only) — see docs/LANGUAGE.md limitations. len() on arrays itself
  // is supported by both backends now.
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

TEST_CASE("Interpreter: array element assignment", "[interpreter][assign]") {
  auto r = run(R"(
    let arr = [1, 2, 3, 4, 5];
    arr[0] = 100;
    arr[4] = 500;
    print(arr[0]);
    print(arr[1]);
    print(arr[4]);
  )");
  CHECK(r.output == "100\n2\n500\n");
}

TEST_CASE("Interpreter: array element assignment out of bounds is a runtime error",
         "[interpreter][assign]") {
  auto r = run("let arr = [1, 2, 3]; arr[10] = 5;");
  CHECK(r.exitCode == -1);
  CHECK(r.output.find("RuntimeError: Array index out of bounds") != std::string::npos);
}

TEST_CASE("Interpreter: object field assignment, including through an alias",
         "[interpreter][assign]") {
  // Objects have reference semantics: mutating through one alias is visible
  // through every other alias to the same underlying object.
  auto r = run(R"(
    let o = {a: 1, b: 2};
    o.a = 100;
    print(o.a);
    print(o.b);

    let alias = o;
    alias.b = 200;
    print(o.b);
    print(alias.b);
  )");
  CHECK(r.output == "100\n2\n200\n200\n");
}

TEST_CASE("Interpreter: assigning an unknown field is a runtime error",
         "[interpreter][assign]") {
  auto r = run(R"(let o = {a: 1}; o.b = 5;)");
  CHECK(r.exitCode == -1);
  CHECK(r.output.find("RuntimeError") != std::string::npos);
}

TEST_CASE("Interpreter: assignment expression evaluates to the assigned value",
         "[interpreter][assign]") {
  CHECK(run("let arr = [1, 2, 3]; print(arr[0] = 42);").output == "42\n");
  CHECK(run("let o = {a: 1}; print(o.a = 42);").output == "42\n");
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

TEST_CASE("Interpreter: float literals and arithmetic", "[interpreter][float]") {
  CHECK(run("print(3.14159);").output == "3.14159\n");
  CHECK(run("print(1.5 + 2.5);").output == "4\n");
  CHECK(run("print(5.0 - 1.5);").output == "3.5\n");
  CHECK(run("print(2.0 * 3.5);").output == "7\n");
  CHECK(run("print(7.0 / 2.0);").output == "3.5\n");
  CHECK(run("print(10.5 % 3.0);").output == "1.5\n");
  CHECK(run("let x = 3.5; print(-x);").output == "-3.5\n");
}

TEST_CASE("Interpreter: mixed int/float arithmetic promotes to float",
         "[interpreter][float]") {
  CHECK(run("print(7 / 2);").output == "3\n"); // pure-int stays int division
  CHECK(run("print(7.0 / 2);").output == "3.5\n");
  CHECK(run("let x = 5; print(x + 2.5);").output == "7.5\n");
}

TEST_CASE("Interpreter: Int and Float compare equal by numeric value, "
         "matching CodeGen's promotion",
         "[interpreter][float]") {
  CHECK(run("print(1 == 1.0);").output == "1\n");
  CHECK(run("print(1.5 == 1.5);").output == "1\n");
  CHECK(run("print(2.5 < 3);").output == "1\n");
}

TEST_CASE("Interpreter: float division and modulo by zero are runtime errors",
         "[interpreter][float]") {
  auto div = run("let x = 1.0; let y = 0.0; print(x / y);");
  CHECK(div.exitCode == -1);
  CHECK(div.output.find("RuntimeError: Division by zero") != std::string::npos);

  auto mod = run("let x = 1.0; let y = 0.0; print(x % y);");
  CHECK(mod.exitCode == -1);
}

TEST_CASE("Interpreter: str() and print() output for floats matches "
         "CodeGen's printf(\"%g\") formatting byte-for-byte",
         "[interpreter][float]") {
  CHECK(run("print(str(3.14));").output == "3.14\n");
  CHECK(run(R"(print("pi = " + 3.14159);)").output == "pi = 3.14159\n");
}

TEST_CASE("Interpreter: push() grows an array without mutating the original",
         "[interpreter][push]") {
  auto r = run(R"(
    let a = [1, 2, 3];
    let b = a;
    let c = push(a, 4);
    print(len(a));
    print(len(b));
    print(len(c));
    print(c[0]);
    print(c[3]);
  )");
  CHECK(r.output == "3\n3\n4\n1\n4\n");
}

TEST_CASE("Interpreter: rebinding via `arr = push(arr, x);`", "[interpreter][push]") {
  auto r = run(R"(
    let arr = [1, 2, 3];
    arr = push(arr, 4);
    arr = push(arr, 5);
    print(len(arr));
    print(arr[3]);
    print(arr[4]);
  )");
  CHECK(r.output == "5\n4\n5\n");
}

TEST_CASE("Interpreter: push() on a non-array is a runtime error", "[interpreter][push]") {
  auto r = run("let x = 5; push(x, 1);");
  CHECK(r.exitCode == -1);
  CHECK(r.output.find("RuntimeError") != std::string::npos);
}

TEST_CASE("Interpreter: first-class function references", "[interpreter][functions]") {
  SECTION("A bare function name is a callable value") {
    auto r = run(R"(
      func add(a, b) { return a + b; }
      let f = add;
      print(f(3, 4));
    )");
    CHECK(r.output == "7\n");
  }

  SECTION("Reassigning to a different function") {
    auto r = run(R"(
      func add(a, b) { return a + b; }
      func sub(a, b) { return a - b; }
      let f = add;
      print(f(3, 4));
      f = sub;
      print(f(10, 3));
    )");
    CHECK(r.output == "7\n7\n");
  }

  SECTION("Passing a function as an argument (higher-order functions)") {
    auto r = run(R"(
      func add(a, b) { return a + b; }
      func apply(fn, x, y) { return fn(x, y); }
      print(apply(add, 5, 6));
    )");
    CHECK(r.output == "11\n");
  }

  SECTION("Calling a non-function value is a runtime error") {
    auto r = run("let x = 5; x(1, 2);");
    CHECK(r.exitCode == -1);
    CHECK(r.output.find("RuntimeError") != std::string::npos);
  }

  SECTION("Wrong argument count through an indirect call is a runtime "
         "error (can't be validated statically)") {
    auto r = run(R"(
      func add(a, b) { return a + b; }
      let f = add;
      print(f(1));
    )");
    CHECK(r.exitCode == -1);
    CHECK(r.output.find("RuntimeError") != std::string::npos);
  }
}
