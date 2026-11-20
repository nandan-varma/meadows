#include <catch2/catch_test_macros.hpp>

#include "TestSupport.h"

using namespace meadows;
using testing::execute;

namespace {

std::string output(const std::string& source) {
  testing::Execution run = execute(source);
  REQUIRE_FALSE(run.error.has_value());
  return run.output;
}

std::string failure(const std::string& source) {
  testing::Execution run = execute(source);
  REQUIRE(run.error.has_value());
  return run.error->message;
}

} // namespace

TEST_CASE("integers wrap on overflow and follow C division", "[interpreter]") {
  CHECK(output("print(2147483647 + 1);") == "-2147483648\n");
  CHECK(output("print(-2147483647 - 2);") == "2147483647\n");
  CHECK(output("print(65536 * 65536);") == "0\n");
  CHECK(output("print(-7 / 2); print(-7 % 2);") == "-3\n-1\n");
  CHECK(output("print(-2147483648 / -1);") == "-2147483648\n");
  CHECK(output("print(-2147483648 % -1);") == "0\n");
  CHECK(output("print(-(-2147483648));") == "-2147483648\n");
}

TEST_CASE("floats promote integers and print with %g", "[interpreter]") {
  CHECK(output("print(7.0 / 2);") == "3.5\n");
  CHECK(output("print(1 + 2.5);") == "3.5\n");
  CHECK(output("print(1.0);") == "1\n");
  CHECK(output("print(0.1 + 0.2);") == "0.3\n");
  CHECK(output("print(1000000.0 * 10);") == "1e+07\n");
  CHECK(output("print(7.5 % 2);") == "1.5\n");
  CHECK(output("print(-0.0);") == "-0\n");
}

TEST_CASE("equality compares numbers by value and strings by content",
          "[interpreter]") {
  CHECK(output("print(1 == 1.0); print(\"a\" == \"a\"); print(\"a\" != \"b\");") ==
        "1\n1\n1\n");
  CHECK(output("print(1 == \"1\");") == "0\n");
  CHECK(output("print([1] == [1]);") == "0\n");
  CHECK(output("let a = [1]; let b = a; print(a == b);") == "1\n");
  CHECK(output("let o = {}; print(o == {});") == "0\n");
}

TEST_CASE("strings concatenate with numbers", "[interpreter]") {
  CHECK(output("print(\"n=\" + 5 + \"/\" + 2.5);") == "n=5/2.5\n");
  CHECK(output("print(1 + \"x\");") == "1x\n");
  CHECK(output("print(len(\"héllo\"));") == "6\n");
}

TEST_CASE("logical operators short-circuit and yield 0 or 1", "[interpreter]") {
  CHECK(output("print(5 && 7); print(0 || 9); print(0 && 1 / 0);") ==
        "1\n1\n0\n");
  CHECK(output("print(3 || 1 / 0);") == "1\n");
  CHECK(output("print(!0); print(!5);") == "1\n0\n");
}

TEST_CASE("arrays and objects have reference semantics", "[interpreter]") {
  CHECK(output("let a = [1, 2]; let b = a; b[0] = 9; print(a[0]);") == "9\n");
  CHECK(output("let o = {x: 1}; let p = o; p.x = 5; print(o.x);") == "5\n");
  CHECK(output("let a = [1]; let b = push(a, 2); print(len(a)); print(b == a);") ==
        "2\n1\n");
  CHECK(output("let a = [1]; a = push(push(a, 2), 3); print(len(a));") ==
        "3\n");
}

TEST_CASE("objects keep their declaration order of evaluation",
          "[interpreter]") {
  CHECK(output("func t(n) { print(n); return n; }\n"
               "let o = {b: t(1), a: t(2), c: t(3)};") == "1\n2\n3\n");
}

TEST_CASE("assignment evaluates operands before it checks them",
          "[interpreter]") {
  CHECK(failure("func t() { print(\"rhs\"); return 1; }\n"
                "let a = [1]; a[5] = t();") ==
        "Array index 5 out of bounds for length 1");
}

TEST_CASE("functions are values", "[interpreter]") {
  CHECK(output("func add(a, b) { return a + b; }\n"
               "let f = add; print(f(2, 3));\n"
               "func apply(g, x) { return g(x, x); }\n"
               "print(apply(add, 4));") == "5\n8\n");
  CHECK(output("func one() { return 1; }\nlet fs = [one];\nprint(fs[0]());") ==
        "1\n");
  CHECK(output("func make() { return make; }\nprint(make()()() == make);") ==
        "1\n");
}

TEST_CASE("functions return zero by default", "[interpreter]") {
  CHECK(output("func f() { }\nfunc g() { return; }\nprint(f()); print(g());") ==
        "0\n0\n");
}

TEST_CASE("scopes end with their block", "[interpreter]") {
  CHECK(output("let x = 1; { let y = 2; print(x + y); } let y = 5; print(y);") ==
        "3\n5\n");
  CHECK(output("let i = 10; for (i in range(0, 2)) { print(i); } print(i);") ==
        "0\n1\n10\n");
}

TEST_CASE("loops support break and continue", "[interpreter]") {
  CHECK(output("for (i in range(0, 10)) { if (i == 2) { continue; } "
               "if (i == 4) { break; } print(i); }") == "0\n1\n3\n");
  CHECK(output("let n = 0; while (1) { n = n + 1; if (n > 3) { break; } } "
               "print(n);") == "4\n");
  CHECK(output("for (i in range(5, 5)) { print(i); } print(0);") == "0\n");
  CHECK(output("for (i in range(2147483646, 2147483647)) { print(i); }") ==
        "2147483646\n");
}

TEST_CASE("return unwinds nested loops", "[interpreter]") {
  CHECK(output("func f() { for (i in range(0, 5)) { while (1) { "
               "if (i == 2) { return i; } break; } } return -1; }\n"
               "print(f());") == "2\n");
}

TEST_CASE("else-if chains select one branch", "[interpreter]") {
  CHECK(output("func f(n) { if (n == 1) { return 10; } else if (n == 2) { "
               "return 20; } else { return 30; } }\n"
               "print(f(1)); print(f(2)); print(f(3));") == "10\n20\n30\n");
}

TEST_CASE("runtime errors carry a message and a location", "[interpreter]") {
  testing::Execution run = execute("let a = 1;\nprint(a / 0);");
  REQUIRE(run.error);
  CHECK(run.error->message == "Division by zero");
  CHECK(run.error->location.line == 2);
  CHECK(run.error->location.column == 9);
}

TEST_CASE("runtime errors have stable messages", "[interpreter]") {
  CHECK(failure("print(1 % 0);") == "Division by zero");
  CHECK(failure("print(1.5 / 0.0);") == "Division by zero");
  CHECK(failure("let a = [1]; print(a[1]);") ==
        "Array index 1 out of bounds for length 1");
  CHECK(failure("let a = [1]; print(a[-1]);") ==
        "Array index -1 out of bounds for length 1");
  CHECK(failure("print([1][\"x\"]);") ==
        "Array index must be an integer, got string");
  CHECK(failure("print(5[0]);") == "Cannot index an integer");
  CHECK(failure("let o = {a: 1}; print(o.b);") == "Object has no field 'b'");
  CHECK(failure("print(5.x);") == "Cannot access field 'x' on an integer");
  CHECK(failure("print(\"a\" - 1);") ==
        "Operator '-' cannot be applied to string and integer");
  CHECK(failure("print([1] + 2);") ==
        "Operator '+' cannot be applied to array and integer");
  CHECK(failure("print(\"a\" < \"b\");") ==
        "Operator '<' cannot be applied to string and string");
  CHECK(failure("print(-\"a\");") ==
        "Operator '-' cannot be applied to string");
  CHECK(failure("print(!1.5);") == "Operator '!' cannot be applied to float");
  CHECK(failure("print(1.5 && 1);") ==
        "Operator '&&' cannot be applied to float");
  CHECK(failure("if (\"a\") { }") == "Condition must be an integer, got string");
  CHECK(failure("while (1.0) { }") == "Condition must be an integer, got float");
  CHECK(failure("for (i in range(0, 1.5)) { }") ==
        "Range bounds must be integers, got float");
  CHECK(failure("let x = 3; x();") == "Cannot call an integer");
  CHECK(failure("func f(a) { return a; } let g = f; g(1, 2);") ==
        "Function 'f' takes 1 argument, got 2");
  CHECK(failure("print([1]);") == "print() cannot print an array");
  CHECK(failure("print(len(5));") ==
        "len() requires a string or an array, got integer");
  CHECK(failure("print(str(\"a\"));") ==
        "str() requires a number, got string");
  CHECK(failure("push(1, 2);") ==
        "push() requires an array as its first argument, got integer");
}

TEST_CASE("call depth is bounded", "[interpreter]") {
  CHECK(failure("func f(n) { return f(n + 1); } f(0);") ==
        "Maximum call depth exceeded");
  CHECK(output("func sum(n) { if (n == 0) { return 0; } return n + sum(n - 1); }"
               "\nprint(sum(5000));") == "12502500\n");
}

TEST_CASE("string length is bounded", "[interpreter]") {
  CHECK(failure("let s = \"x\"; while (1) { s = s + s; }") ==
        "String exceeds the maximum length");
}

TEST_CASE("array length is bounded", "[interpreter]") {
  CHECK(failure("let a = [0]; while (1) { a = push(a, 0); }") ==
        "Array exceeds the maximum length");
}

TEST_CASE("deep structures are released without recursion", "[interpreter]") {
  CHECK(output("let list = 0; for (i in range(0, 300000)) { "
               "list = {next: list}; } print(1);") == "1\n");
  CHECK(output("let a = []; for (i in range(0, 300000)) { a = [a]; } "
               "print(1);") == "1\n");
}
