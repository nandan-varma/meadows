#include "parser/Parser.h"
#include "ast/AST.h"
#include "catch_amalgamated.hpp"
#include "lexer/Lexer.h"

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
    REQUIRE_THROWS_AS(parser->parse(), std::runtime_error);
  }

  SECTION("Missing expression") {
    auto parser = createParser("let x = ;");
    REQUIRE_THROWS_AS(parser->parse(), std::runtime_error);
  }

  SECTION("Missing closing brace") {
    auto parser = createParser("func test() { print 1;");
    REQUIRE_THROWS_AS(parser->parse(), std::runtime_error);
  }

  SECTION("Unexpected token") {
    auto parser = createParser("@#$");
    REQUIRE_THROWS_AS(parser->parse(), std::runtime_error);
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
