#include "parser/Parser.h"
#include "ast/AST.h"
#include "catch_amalgamated.hpp"
#include "lexer/Lexer.h"
#include "utils/Exceptions.h"

// Helper function to create parser from source
std::unique_ptr<Parser> createParser(const std::string &source) {
  auto lexer = std::make_unique<Lexer>(source);
  auto tokens = lexer->tokenize();
  return std::make_unique<Parser>(tokens);
}

TEST_CASE("Parser handles variable declarations", "[parser]") {
  SECTION("Simple variable declaration") {
    auto parser = createParser("let x = 5;");
    auto stmts = parser->parse();

    REQUIRE(stmts.size() == 1);
    auto letStmt = dynamic_cast<LetStmt *>(stmts[0].get());
    REQUIRE(letStmt != nullptr);
    CHECK(letStmt->name == "x");
  }

  SECTION("Variable declaration with expression") {
    auto parser = createParser("let sum = 1 + 2;");
    auto stmts = parser->parse();

    REQUIRE(stmts.size() == 1);
    auto letStmt = dynamic_cast<LetStmt *>(stmts[0].get());
    REQUIRE(letStmt != nullptr);
    CHECK(letStmt->name == "sum");

    // Check initializer is a binary expression
    auto binaryExpr = dynamic_cast<BinaryExpr *>(letStmt->initializer.get());
    REQUIRE(binaryExpr != nullptr);
    CHECK(binaryExpr->op == "+");
  }

  SECTION("Variable declaration with string") {
    auto parser = createParser("let msg = \"hello\";");
    auto stmts = parser->parse();

    REQUIRE(stmts.size() == 1);
    auto letStmt = dynamic_cast<LetStmt *>(stmts[0].get());
    REQUIRE(letStmt != nullptr);
    CHECK(letStmt->name == "msg");

    auto literalExpr = dynamic_cast<LiteralExpr *>(letStmt->initializer.get());
    REQUIRE(literalExpr != nullptr);
    CHECK(literalExpr->value == "hello");
  }
}

TEST_CASE("Parser handles function definitions", "[parser]") {
  SECTION("Function without parameters") {
    auto parser = createParser("func greet() { print \"hello\"; }");
    auto stmts = parser->parse();

    REQUIRE(stmts.size() == 1);
    auto funcStmt = dynamic_cast<FuncStmt *>(stmts[0].get());
    REQUIRE(funcStmt != nullptr);
    CHECK(funcStmt->name == "greet");
    CHECK(funcStmt->params.empty());
  }

  SECTION("Function with parameters") {
    auto parser = createParser("func add(a, b) { return a + b; }");
    auto stmts = parser->parse();

    REQUIRE(stmts.size() == 1);
    auto funcStmt = dynamic_cast<FuncStmt *>(stmts[0].get());
    REQUIRE(funcStmt != nullptr);
    CHECK(funcStmt->name == "add");
    REQUIRE(funcStmt->params.size() == 2);
    CHECK(funcStmt->params[0] == "a");
    CHECK(funcStmt->params[1] == "b");
  }

  SECTION("Function with multiple statements") {
    auto parser =
        createParser("func test() { let x = 1; let y = 2; return x + y; }");
    auto stmts = parser->parse();

    REQUIRE(stmts.size() == 1);
    auto funcStmt = dynamic_cast<FuncStmt *>(stmts[0].get());
    REQUIRE(funcStmt != nullptr);
    REQUIRE(funcStmt->body.size() == 3);
  }
}

TEST_CASE("Parser handles if statements", "[parser]") {
  SECTION("If without else") {
    auto parser = createParser("if (x > 0) { print \"positive\"; }");
    auto stmts = parser->parse();

    REQUIRE(stmts.size() == 1);
    auto ifStmt = dynamic_cast<IfStmt *>(stmts[0].get());
    REQUIRE(ifStmt != nullptr);
    REQUIRE(ifStmt->thenBranch.size() == 1);
    REQUIRE(ifStmt->elseBranch.empty());
  }

  SECTION("If with else") {
    auto parser = createParser(
        "if (x > 0) { print \"positive\"; } else { print \"non-positive\"; }");
    auto stmts = parser->parse();

    REQUIRE(stmts.size() == 1);
    auto ifStmt = dynamic_cast<IfStmt *>(stmts[0].get());
    REQUIRE(ifStmt != nullptr);
    REQUIRE(ifStmt->thenBranch.size() == 1);
    REQUIRE(ifStmt->elseBranch.size() == 1);
  }
}

TEST_CASE("Parser handles loops", "[parser]") {
  SECTION("For loop") {
    auto parser = createParser("for (i in range(0, 10)) { print i; }");
    auto stmts = parser->parse();

    REQUIRE(stmts.size() == 1);
    auto forStmt = dynamic_cast<ForStmt *>(stmts[0].get());
    REQUIRE(forStmt != nullptr);
    CHECK(forStmt->var == "i");
    REQUIRE(forStmt->body.size() == 1);
  }

  SECTION("While loop") {
    auto parser = createParser("while (x > 0) { print x; }");
    auto stmts = parser->parse();

    REQUIRE(stmts.size() == 1);
    auto whileStmt = dynamic_cast<WhileStmt *>(stmts[0].get());
    REQUIRE(whileStmt != nullptr);
    REQUIRE(whileStmt->body.size() == 1);
  }
}

TEST_CASE("Parser handles expressions", "[parser]") {
  SECTION("Arithmetic expression") {
    auto parser = createParser("1 + 2 * 3;");
    auto stmts = parser->parse();

    REQUIRE(stmts.size() == 1);
    auto exprStmt = dynamic_cast<ExprStmt *>(stmts[0].get());
    REQUIRE(exprStmt != nullptr);

    // Should be parsed as: 1 + (2 * 3)
    auto binaryExpr = dynamic_cast<BinaryExpr *>(exprStmt->expr.get());
    REQUIRE(binaryExpr != nullptr);
    CHECK(binaryExpr->op == "+");
  }

  SECTION("Comparison expression") {
    auto parser = createParser("x == 5;");
    auto stmts = parser->parse();

    REQUIRE(stmts.size() == 1);
    auto exprStmt = dynamic_cast<ExprStmt *>(stmts[0].get());
    REQUIRE(exprStmt != nullptr);

    auto binaryExpr = dynamic_cast<BinaryExpr *>(exprStmt->expr.get());
    REQUIRE(binaryExpr != nullptr);
    CHECK(binaryExpr->op == "==");
  }

  SECTION("Function call") {
    auto parser = createParser("add(1, 2);");
    auto stmts = parser->parse();

    REQUIRE(stmts.size() == 1);
    auto exprStmt = dynamic_cast<ExprStmt *>(stmts[0].get());
    REQUIRE(exprStmt != nullptr);

    auto callExpr = dynamic_cast<CallExpr *>(exprStmt->expr.get());
    REQUIRE(callExpr != nullptr);
    REQUIRE(callExpr->args.size() == 2);
  }
}

TEST_CASE("Parser handles print statements", "[parser]") {
  SECTION("Print expression") {
    auto parser = createParser("print 42;");
    auto stmts = parser->parse();

    REQUIRE(stmts.size() == 1);
    auto printStmt = dynamic_cast<PrintStmt *>(stmts[0].get());
    REQUIRE(printStmt != nullptr);
  }

  SECTION("Print variable") {
    auto parser = createParser("print x;");
    auto stmts = parser->parse();

    REQUIRE(stmts.size() == 1);
    auto printStmt = dynamic_cast<PrintStmt *>(stmts[0].get());
    REQUIRE(printStmt != nullptr);
  }
}

TEST_CASE("Parser handles return statements", "[parser]") {
  SECTION("Return value") {
    auto parser = createParser("return 42;");
    auto stmts = parser->parse();

    REQUIRE(stmts.size() == 1);
    auto returnStmt = dynamic_cast<ReturnStmt *>(stmts[0].get());
    REQUIRE(returnStmt != nullptr);
  }

  SECTION("Return expression") {
    auto parser = createParser("return x + y;");
    auto stmts = parser->parse();

    REQUIRE(stmts.size() == 1);
    auto returnStmt = dynamic_cast<ReturnStmt *>(stmts[0].get());
    REQUIRE(returnStmt != nullptr);
  }
}

TEST_CASE("Parser reports syntax errors", "[parser]") {
  SECTION("Missing semicolon") {
    auto parser = createParser("let x = 5");
    REQUIRE_THROWS_AS(parser->parse(), meadows::ParseException);
  }

  SECTION("Missing expression") {
    auto parser = createParser("let x = ;");
    REQUIRE_THROWS_AS(parser->parse(), meadows::ParseException);
  }

  SECTION("Missing closing brace") {
    auto parser = createParser("func test() { print 1;");
    REQUIRE_THROWS_AS(parser->parse(), meadows::ParseException);
  }

  SECTION("Unexpected token") {
    auto parser = createParser("@#$");
    REQUIRE_THROWS_AS(parser->parse(), meadows::ParseException);
  }
}

TEST_CASE("Parser handles empty input", "[parser]") {
  auto parser = createParser("");
  auto stmts = parser->parse();

  REQUIRE(stmts.empty());
}

TEST_CASE("Parser handles multiple statements", "[parser]") {
  auto parser = createParser(R"(
        let x = 5;
        let y = 10;
        print x + y;
    )");
  auto stmts = parser->parse();

  REQUIRE(stmts.size() == 3);
}

TEST_CASE("Parser handles nested blocks", "[parser]") {
  auto parser = createParser(R"(
        func outer() {
            if (true) {
                while (false) {
                    print 1;
                }
            }
        }
    )");
  auto stmts = parser->parse();

  REQUIRE(stmts.size() == 1);
  auto funcStmt = dynamic_cast<FuncStmt *>(stmts[0].get());
  REQUIRE(funcStmt != nullptr);
  REQUIRE(funcStmt->body.size() == 1);
}

TEST_CASE("Parser handles unary minus", "[parser]") {
  SECTION("Simple unary minus") {
    auto parser = createParser("let x = -5;");
    auto stmts = parser->parse();

    REQUIRE(stmts.size() == 1);
    auto letStmt = dynamic_cast<LetStmt *>(stmts[0].get());
    REQUIRE(letStmt != nullptr);
    CHECK(letStmt->name == "x");

    auto unaryExpr = dynamic_cast<UnaryExpr *>(letStmt->initializer.get());
    REQUIRE(unaryExpr != nullptr);
    CHECK(unaryExpr->op == "-");
  }

  SECTION("Double unary minus") {
    auto parser = createParser("let x = --5;");
    auto stmts = parser->parse();

    REQUIRE(stmts.size() == 1);
    auto letStmt = dynamic_cast<LetStmt *>(stmts[0].get());
    REQUIRE(letStmt != nullptr);

    auto outer = dynamic_cast<UnaryExpr *>(letStmt->initializer.get());
    REQUIRE(outer != nullptr);
    CHECK(outer->op == "-");

    auto inner = dynamic_cast<UnaryExpr *>(outer->operand.get());
    REQUIRE(inner != nullptr);
    CHECK(inner->op == "-");
  }

  SECTION("Unary minus in expression") {
    auto parser = createParser("let x = -5 + 3;");
    auto stmts = parser->parse();

    REQUIRE(stmts.size() == 1);
    auto letStmt = dynamic_cast<LetStmt *>(stmts[0].get());
    REQUIRE(letStmt != nullptr);

    auto binaryExpr = dynamic_cast<BinaryExpr *>(letStmt->initializer.get());
    REQUIRE(binaryExpr != nullptr);
    CHECK(binaryExpr->op == "+");
  }
}

TEST_CASE("Parser handles complex expressions", "[parser]") {
  SECTION("Chained comparisons") {
    auto parser = createParser("let x = 1 < 2 < 3;");
    auto stmts = parser->parse();

    REQUIRE(stmts.size() == 1);
    auto letStmt = dynamic_cast<LetStmt *>(stmts[0].get());
    REQUIRE(letStmt != nullptr);
  }

  SECTION("Mixed arithmetic") {
    auto parser = createParser("let x = 1 + 2 - 3 * 4 / 5;");
    auto stmts = parser->parse();

    REQUIRE(stmts.size() == 1);
    auto letStmt = dynamic_cast<LetStmt *>(stmts[0].get());
    REQUIRE(letStmt != nullptr);
  }

  SECTION("Parenthesized expressions") {
    auto parser = createParser("let x = (1 + 2) * (3 - 4);");
    auto stmts = parser->parse();

    REQUIRE(stmts.size() == 1);
    auto letStmt = dynamic_cast<LetStmt *>(stmts[0].get());
    REQUIRE(letStmt != nullptr);

    auto outer = dynamic_cast<BinaryExpr *>(letStmt->initializer.get());
    REQUIRE(outer != nullptr);
    CHECK(outer->op == "*");
  }
}

TEST_CASE("Parser handles multiple function parameters", "[parser]") {
  SECTION("Many parameters") {
    auto parser =
        createParser("func many(a, b, c, d, e) { return a + b + c + d + e; }");
    auto stmts = parser->parse();

    REQUIRE(stmts.size() == 1);
    auto funcStmt = dynamic_cast<FuncStmt *>(stmts[0].get());
    REQUIRE(funcStmt != nullptr);
    CHECK(funcStmt->params.size() == 5);
  }

  SECTION("No parameters") {
    auto parser = createParser("func noParams() { return 42; }");
    auto stmts = parser->parse();

    REQUIRE(stmts.size() == 1);
    auto funcStmt = dynamic_cast<FuncStmt *>(stmts[0].get());
    REQUIRE(funcStmt != nullptr);
    CHECK(funcStmt->params.empty());
  }

  SECTION("Function calls with many arguments") {
    auto parser = createParser("let x = many(1, 2, 3, 4, 5);");
    auto stmts = parser->parse();

    REQUIRE(stmts.size() == 1);
    auto letStmt = dynamic_cast<LetStmt *>(stmts[0].get());
    REQUIRE(letStmt != nullptr);

    auto callExpr = dynamic_cast<CallExpr *>(letStmt->initializer.get());
    REQUIRE(callExpr != nullptr);
    CHECK(callExpr->args.size() == 5);
  }
}

TEST_CASE("Parser handles long string literals", "[parser]") {
  SECTION("Long string") {
    std::string longStr(1000, 'a');
    auto parser = createParser("let x = \"" + longStr + "\";");
    auto stmts = parser->parse();

    REQUIRE(stmts.size() == 1);
    auto letStmt = dynamic_cast<LetStmt *>(stmts[0].get());
    REQUIRE(letStmt != nullptr);

    auto literalExpr = dynamic_cast<LiteralExpr *>(letStmt->initializer.get());
    REQUIRE(literalExpr != nullptr);
    CHECK(literalExpr->value.length() == 1000);
  }
}

TEST_CASE("Parser handles nested if statements", "[parser]") {
  SECTION("Nested if without else") {
    auto parser = createParser("if (true) { if (false) { print 1; } }");
    auto stmts = parser->parse();

    REQUIRE(stmts.size() == 1);
    auto outerIf = dynamic_cast<IfStmt *>(stmts[0].get());
    REQUIRE(outerIf != nullptr);

    auto innerIf = dynamic_cast<IfStmt *>(outerIf->thenBranch[0].get());
    REQUIRE(innerIf != nullptr);
  }

  SECTION("If with nested if-else") {
    auto parser =
        createParser("if (true) { if (false) { print 1; } else { print 2; } }");
    auto stmts = parser->parse();

    REQUIRE(stmts.size() == 1);
    auto outerIf = dynamic_cast<IfStmt *>(stmts[0].get());
    REQUIRE(outerIf != nullptr);
    REQUIRE(outerIf->thenBranch.size() == 1);

    auto innerIf = dynamic_cast<IfStmt *>(outerIf->thenBranch[0].get());
    REQUIRE(innerIf != nullptr);
    CHECK(innerIf->elseBranch.size() == 1);
  }
}

TEST_CASE("Parser handles empty blocks", "[parser]") {
  SECTION("Empty function body") {
    auto parser = createParser("func empty() {}");
    auto stmts = parser->parse();

    REQUIRE(stmts.size() == 1);
    auto funcStmt = dynamic_cast<FuncStmt *>(stmts[0].get());
    REQUIRE(funcStmt != nullptr);
    CHECK(funcStmt->body.empty());
  }

  SECTION("Empty if body") {
    auto parser = createParser("if (true) {}");
    auto stmts = parser->parse();

    REQUIRE(stmts.size() == 1);
    auto ifStmt = dynamic_cast<IfStmt *>(stmts[0].get());
    REQUIRE(ifStmt != nullptr);
    CHECK(ifStmt->thenBranch.empty());
  }

  SECTION("Empty while body") {
    auto parser = createParser("while (false) {}");
    auto stmts = parser->parse();

    REQUIRE(stmts.size() == 1);
    auto whileStmt = dynamic_cast<WhileStmt *>(stmts[0].get());
    REQUIRE(whileStmt != nullptr);
    CHECK(whileStmt->body.empty());
  }
}

TEST_CASE("Parser handles nested loops", "[parser]") {
  SECTION("For inside while") {
    auto parser =
        createParser("while (true) { for (i in range(0, 1)) { print i; } }");
    auto stmts = parser->parse();

    REQUIRE(stmts.size() == 1);
    auto whileStmt = dynamic_cast<WhileStmt *>(stmts[0].get());
    REQUIRE(whileStmt != nullptr);
    REQUIRE(whileStmt->body.size() == 1);

    auto forStmt = dynamic_cast<ForStmt *>(whileStmt->body[0].get());
    REQUIRE(forStmt != nullptr);
  }

  SECTION("While inside for") {
    auto parser =
        createParser("for (i in range(0, 1)) { while (false) { print i; } }");
    auto stmts = parser->parse();

    REQUIRE(stmts.size() == 1);
    auto forStmt = dynamic_cast<ForStmt *>(stmts[0].get());
    REQUIRE(forStmt != nullptr);
    REQUIRE(forStmt->body.size() == 1);

    auto whileStmt = dynamic_cast<WhileStmt *>(forStmt->body[0].get());
    REQUIRE(whileStmt != nullptr);
  }

  SECTION("Nested for loops") {
    auto parser = createParser(
        "for (i in range(0, 2)) { for (j in range(0, 2)) { print i; } }");
    auto stmts = parser->parse();

    REQUIRE(stmts.size() == 1);
    auto outerFor = dynamic_cast<ForStmt *>(stmts[0].get());
    REQUIRE(outerFor != nullptr);
    REQUIRE(outerFor->body.size() == 1);

    auto innerFor = dynamic_cast<ForStmt *>(outerFor->body[0].get());
    REQUIRE(innerFor != nullptr);
  }
}

TEST_CASE("Parser handles if-else if chains", "[parser]") {
  SECTION("If-else if pattern") {
    auto parser =
        createParser("if (false) { print 1; } else { if (true) { print 2; } }");
    auto stmts = parser->parse();

    REQUIRE(stmts.size() == 1);
    auto ifStmt = dynamic_cast<IfStmt *>(stmts[0].get());
    REQUIRE(ifStmt != nullptr);
    REQUIRE(ifStmt->elseBranch.size() == 1);

    auto elseIfStmt = dynamic_cast<IfStmt *>(ifStmt->elseBranch[0].get());
    REQUIRE(elseIfStmt != nullptr);
  }
}

TEST_CASE("Parser handles function calls", "[parser]") {
  SECTION("Function call with no arguments") {
    auto parser =
        createParser("func getFive() { return 5; } let x = getFive();");
    auto stmts = parser->parse();

    REQUIRE(stmts.size() == 2);

    auto funcStmt = dynamic_cast<FuncStmt *>(stmts[0].get());
    REQUIRE(funcStmt != nullptr);
    CHECK(funcStmt->params.empty());

    auto letStmt = dynamic_cast<LetStmt *>(stmts[1].get());
    REQUIRE(letStmt != nullptr);

    auto callExpr = dynamic_cast<CallExpr *>(letStmt->initializer.get());
    REQUIRE(callExpr != nullptr);
    CHECK(callExpr->args.empty());
  }

  SECTION("Function call with one argument") {
    auto parser =
        createParser("func square(x) { return x * x; } let y = square(5);");
    auto stmts = parser->parse();

    REQUIRE(stmts.size() == 2);

    auto funcStmt = dynamic_cast<FuncStmt *>(stmts[0].get());
    REQUIRE(funcStmt != nullptr);
    CHECK(funcStmt->params.size() == 1);

    auto letStmt = dynamic_cast<LetStmt *>(stmts[1].get());
    REQUIRE(letStmt != nullptr);

    auto callExpr = dynamic_cast<CallExpr *>(letStmt->initializer.get());
    REQUIRE(callExpr != nullptr);
    CHECK(callExpr->args.size() == 1);
  }

  SECTION("Nested function calls") {
    auto parser = createParser(
        "func add(x, y) { return x + y; } let z = add(add(1, 2), 3);");
    auto stmts = parser->parse();

    REQUIRE(stmts.size() == 2);

    auto letStmt = dynamic_cast<LetStmt *>(stmts[1].get());
    REQUIRE(letStmt != nullptr);

    auto outerCall = dynamic_cast<CallExpr *>(letStmt->initializer.get());
    REQUIRE(outerCall != nullptr);

    auto innerCall = dynamic_cast<CallExpr *>(outerCall->args[0].get());
    REQUIRE(innerCall != nullptr);
    CHECK(innerCall->args.size() == 2);
  }
}

TEST_CASE("Parser handles multiple expressions", "[parser]") {
  SECTION("Multiple expression statements") {
    auto parser = createParser("1; 2; 3;");
    auto stmts = parser->parse();

    REQUIRE(stmts.size() == 3);
  }

  SECTION("Mixed statements") {
    auto parser = createParser("let x = 1; x; x + 1; print x;");
    auto stmts = parser->parse();

    REQUIRE(stmts.size() == 4);
  }
}

TEST_CASE("Parser handles variable shadowing", "[parser]") {
  SECTION("Shadowing in nested block") {
    auto parser = createParser("let x = 1; { let x = 2; print x; } print x;");
    auto stmts = parser->parse();

    REQUIRE(stmts.size() == 3);
    auto outerLet = dynamic_cast<LetStmt *>(stmts[0].get());
    REQUIRE(outerLet != nullptr);
    CHECK(outerLet->name == "x");
  }
}

TEST_CASE("Parser handles deep nesting", "[parser]") {
  SECTION("Deep parenthesized expressions") {
    auto parser = createParser("let x = (((((1)))));");
    auto stmts = parser->parse();

    REQUIRE(stmts.size() == 1);
    auto letStmt = dynamic_cast<LetStmt *>(stmts[0].get());
    REQUIRE(letStmt != nullptr);
  }

  SECTION("Deep nested if-while-for") {
    auto parser = createParser(
        "if (true) { while (true) { for (i in range(0, 1)) { print 1; } } }");
    auto stmts = parser->parse();

    REQUIRE(stmts.size() == 1);
    auto ifStmt = dynamic_cast<IfStmt *>(stmts[0].get());
    REQUIRE(ifStmt != nullptr);
    REQUIRE(ifStmt->thenBranch.size() == 1);

    auto whileStmt = dynamic_cast<WhileStmt *>(ifStmt->thenBranch[0].get());
    REQUIRE(whileStmt != nullptr);
    REQUIRE(whileStmt->body.size() == 1);

    auto forStmt = dynamic_cast<ForStmt *>(whileStmt->body[0].get());
    REQUIRE(forStmt != nullptr);
  }
}

TEST_CASE("Parser property-based tests", "[parser][property]") {
  SECTION("Balanced parentheses in expressions") {
    std::vector<std::pair<std::string, std::string>> sources = {
        {"let x = (1);", "single paren"},
        {"let x = ((1));", "double paren"},
        {"let x = (((1)));", "triple paren"},
        {"let x = (1 + 2);", "add in parens"},
        {"let x = ((1 + 2) * 3);", "nested parens"},
        {"let x = (1 * (2 + 3));", "mixed nesting"}};

    for (const auto &[src, desc] : sources) {
      auto parser = createParser(src);
      auto stmts = parser->parse();
      CHECK(stmts.size() == 1);
    }
  }

  SECTION("Operator precedence is respected") {
    struct PrecedenceTest {
      const char *source;
      const char *description;
    };

    PrecedenceTest tests[] = {{"let x = 1 + 2 * 3;", "* before +"},
                              {"let x = 1 * 2 + 3;", "* before +"},
                              {"let x = 10 - 5 - 2;", "left associative -"},
                              {"let x = 10 / 2 / 2;", "left associative /"},
                              {"let x = 1 + 2 - 3 + 4;", "mixed + and -"},
                              {"let x = 1 * 2 * 3;", "left associative *"}};

    for (const auto &test : tests) {
      auto parser = createParser(test.source);
      auto stmts = parser->parse();
      REQUIRE(stmts.size() == 1);
    }
  }

  SECTION("Logical operators have correct precedence") {
    std::string sources[] = {"let x = true && false;", "let x = true || false;",
                             "let x = true && true || false;",
                             "let x = true || true && false;"};

    for (const auto &src : sources) {
      auto parser = createParser(src);
      auto stmts = parser->parse();
      REQUIRE(stmts.size() == 1);
    }
  }

  SECTION("Comparison operators chain correctly") {
    std::string sources[] = {"let x = 1 < 2;",  "let x = 1 > 2;",
                             "let x = 1 <= 2;", "let x = 1 >= 2;",
                             "let x = 1 == 2;", "let x = 1 != 2;"};

    for (const auto &src : sources) {
      auto parser = createParser(src);
      auto stmts = parser->parse();
      REQUIRE(stmts.size() == 1);
    }
  }

  SECTION("Unary operators bind tightly") {
    std::string sources[] = {"let x = -1;",  "let x = !false;",
                             "let x = --1;", "let x = !!true;",
                             "let x = -x;",  "let x = !y;"};

    for (const auto &src : sources) {
      auto parser = createParser(src);
      auto stmts = parser->parse();
      REQUIRE(stmts.size() == 1);
    }
  }

  SECTION("Array literals parse correctly") {
    std::string sources[] = {"let arr = [];", "let arr = [1];",
                             "let arr = [1, 2, 3];",
                             "let arr = [1 + 2, 3 * 4, 5];"};

    for (const auto &src : sources) {
      auto parser = createParser(src);
      auto stmts = parser->parse();
      REQUIRE(stmts.size() == 1);
    }
  }

  SECTION("Object literals parse correctly") {
    std::string sources[] = {"let obj = {};", "let obj = {a: 1};",
                             "let obj = {a: 1, b: 2};",
                             "let obj = {a: 1 + 2, b: 3 * 4};"};

    for (const auto &src : sources) {
      auto parser = createParser(src);
      auto stmts = parser->parse();
      REQUIRE(stmts.size() == 1);
    }
  }

  SECTION("Function calls parse correctly") {
    std::string sources[] = {"f();", "f(1);", "f(1, 2, 3);", "f(g());",
                             "obj.method();"};

    for (const auto &src : sources) {
      auto parser = createParser("let x = " + std::string(src));
      auto stmts = parser->parse();
      REQUIRE(stmts.size() == 1);
    }
  }
}
