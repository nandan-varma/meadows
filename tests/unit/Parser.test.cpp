#include <catch2/catch_test_macros.hpp>

#include "ast/AstPrinter.h"
#include "lexer/Lexer.h"
#include "parser/Parser.h"
#include "TestSupport.h"

using namespace meadows;

namespace {

struct Parsed {
  DiagnosticSink sink;
  ParsedFile file;
};

Parsed parse(const std::string& source) {
  Parsed parsed;
  Lexer lexer(source, 0, parsed.sink);
  Parser parser(lexer.tokenize(), parsed.sink);
  parsed.file = parser.parse();
  return parsed;
}

std::string tree(const std::string& source) {
  Parsed parsed = parse(source);
  REQUIRE_FALSE(parsed.sink.hasErrors());
  return AstPrinter().print(parsed.file.statements);
}

ErrorCode firstError(const std::string& source) {
  Parsed parsed = parse(source);
  REQUIRE(parsed.sink.hasErrors());
  return parsed.sink.diagnostics().front().code;
}

} // namespace

TEST_CASE("parser honours operator precedence", "[parser]") {
  CHECK(tree("1 + 2 * 3;") ==
        "ExprStmt\n  Binary +\n    Int 1\n    Binary *\n      Int 2\n      Int "
        "3\n");
  CHECK(tree("a || b && c;") ==
        "ExprStmt\n  Logical ||\n    Var a\n    Logical &&\n      Var b\n      "
        "Var c\n");
  CHECK(tree("(1 + 2) * 3;").find("Binary *\n    Binary +") !=
        std::string::npos);
}

TEST_CASE("parser makes binary operators left associative", "[parser]") {
  CHECK(tree("1 - 2 - 3;") ==
        "ExprStmt\n  Binary -\n    Binary -\n      Int 1\n      Int 2\n    Int "
        "3\n");
}

TEST_CASE("parser makes assignment right associative", "[parser]") {
  CHECK(tree("a = b = 1;") ==
        "ExprStmt\n  Assign\n    Var a\n    Assign\n      Var b\n      Int "
        "1\n");
}

TEST_CASE("parser chains calls, indexing and field access freely",
          "[parser]") {
  CHECK(tree("a[0].x[1](2);") ==
        "ExprStmt\n  Call\n    Index\n      Field x\n        Index\n          "
        "Var a\n          Int 0\n      Int 1\n    Int 2\n");
}

TEST_CASE("parser folds negated literals", "[parser]") {
  CHECK(tree("-5;") == "ExprStmt\n  Int -5\n");
  CHECK(tree("-2147483648;") == "ExprStmt\n  Int -2147483648\n");
  CHECK(tree("-1.5;") == "ExprStmt\n  Float -1.5\n");
  CHECK(tree("-x;") == "ExprStmt\n  Unary -\n    Var x\n");
  CHECK(firstError("2147483648;") == ErrorCode::LEX_NUMBER_OUT_OF_RANGE);
}

TEST_CASE("parser desugars else-if into a nested if", "[parser]") {
  CHECK(tree("if (a) { } else if (b) { } else { }") ==
        "If\n  Var a\nThen\nElse\n  If\n    Var b\n  Then\n");
}

TEST_CASE("parser keeps object fields in source order", "[parser]") {
  CHECK(tree("let o = {b: 1, a: 2};") ==
        "Let o\n  Object\n    Field b\n      Int 1\n    Field a\n      Int "
        "2\n");
}

TEST_CASE("parser records imports by statement position", "[parser]") {
  Parsed parsed = parse("let a = 1; import \"m.ms\"; let b = 2;");
  REQUIRE_FALSE(parsed.sink.hasErrors());
  REQUIRE(parsed.file.imports.size() == 1);
  CHECK(parsed.file.imports[0].path == "m.ms");
  CHECK(parsed.file.imports[0].position == 1);
  CHECK(parsed.file.statements.size() == 2);
}

TEST_CASE("parser reports specific syntax errors", "[parser]") {
  CHECK(firstError("let x = 1") == ErrorCode::PARSE_EXPECTED_SEMICOLON);
  CHECK(firstError("let = 1;") == ErrorCode::PARSE_EXPECTED_IDENTIFIER);
  CHECK(firstError("let x 1;") == ErrorCode::PARSE_EXPECTED_EQUALS);
  CHECK(firstError("let x = ;") == ErrorCode::PARSE_EXPECTED_EXPRESSION);
  CHECK(firstError("if x { }") == ErrorCode::PARSE_EXPECTED_LPAREN);
  CHECK(firstError("if (x { }") == ErrorCode::PARSE_EXPECTED_RPAREN);
  CHECK(firstError("while (1) print(1);") == ErrorCode::PARSE_EXPECTED_LBRACE);
  CHECK(firstError("while (1) { ") == ErrorCode::PARSE_EXPECTED_RBRACE);
  CHECK(firstError("1 = 2;") == ErrorCode::PARSE_INVALID_ASSIGNMENT_TARGET);
  CHECK(firstError("f(1 2);") == ErrorCode::PARSE_EXPECTED_RPAREN);
  CHECK(firstError("a[1;") == ErrorCode::PARSE_EXPECTED_RBRACKET);
  CHECK(firstError("let o = {a 1};") == ErrorCode::PARSE_EXPECTED_COLON);
  CHECK(firstError("for (i of range(0, 1)) { }") ==
        ErrorCode::PARSE_EXPECTED_IN);
  CHECK(firstError("import 3;") == ErrorCode::PARSE_EXPECTED_STRING);
  CHECK(firstError("{ func f() { } }") ==
        ErrorCode::PARSE_MISPLACED_DECLARATION);
  CHECK(firstError("{ import \"a.ms\"; }") ==
        ErrorCode::PARSE_MISPLACED_DECLARATION);
}

TEST_CASE("parser recovers and reports several errors at once", "[parser]") {
  Parsed parsed = parse("let a = ;\nlet b = 2;\nlet c = ;\nprint(b);");
  CHECK(parsed.sink.errorCount() == 2);
  CHECK(parsed.file.statements.size() == 2);
}

TEST_CASE("parser recovers inside blocks without losing the outer block",
          "[parser]") {
  Parsed parsed = parse("func f() { let a = ; let b = 1; }\nlet c = 3;");
  CHECK(parsed.sink.errorCount() == 1);
  CHECK(parsed.file.statements.size() == 2);
}

TEST_CASE("parser survives stray closing braces", "[parser]") {
  Parsed parsed = parse("} } let a = 1;");
  CHECK(parsed.sink.hasErrors());
  CHECK(parsed.file.statements.size() == 1);
}

TEST_CASE("parser bounds nesting depth", "[parser]") {
  CHECK(firstError(std::string(5000, '(') + "1" + std::string(5000, ')') +
                   ";") == ErrorCode::PARSE_NESTING_TOO_DEEP);
  CHECK(firstError(std::string(5000, '{') + std::string(5000, '}')) ==
        ErrorCode::PARSE_NESTING_TOO_DEEP);
  CHECK(firstError(std::string(5000, '-') + "1;") ==
        ErrorCode::PARSE_NESTING_TOO_DEEP);

  std::string chain = "1";
  for (int i = 0; i < 5000; ++i) {
    chain += " + 1";
  }
  CHECK(firstError(chain + ";") == ErrorCode::PARSE_NESTING_TOO_DEEP);
}

TEST_CASE("parser accepts moderately deep nesting", "[parser]") {
  Parsed parsed = parse(std::string(60, '(') + "1" + std::string(60, ')') + ";");
  CHECK_FALSE(parsed.sink.hasErrors());
}
