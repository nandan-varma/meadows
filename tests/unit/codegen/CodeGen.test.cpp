#include "codegen/CodeGen.h"
#include "ast/AST.h"
#include "catch_amalgamated.hpp"
#include "lexer/Lexer.h"
#include "parser/Parser.h"

static std::unique_ptr<Parser> createParser(const std::string &source) {
  auto lexer = std::make_unique<Lexer>(source);
  auto tokens = lexer->tokenize();
  return std::make_unique<Parser>(tokens);
}

TEST_CASE("CodeGen handles function definitions", "[codegen]") {
  SECTION("Function with return statement") {
    auto parser = createParser("func test() { return 42; }");
    auto stmts = parser->parse();

    REQUIRE(stmts.size() == 1);

    CodeGen codegen;
    REQUIRE_NOTHROW(codegen.generate(stmts));

    auto module = codegen.getModule();
    REQUIRE(module != nullptr);
    REQUIRE(module->getFunction("test") != nullptr);
  }

  SECTION("Function with parameters") {
    auto parser = createParser("func add(a, b) { return a + b; }");
    auto stmts = parser->parse();

    REQUIRE(stmts.size() == 1);

    CodeGen codegen;
    REQUIRE_NOTHROW(codegen.generate(stmts));

    auto module = codegen.getModule();
    REQUIRE(module != nullptr);
    auto func = module->getFunction("add");
    REQUIRE(func != nullptr);
    REQUIRE(func->arg_size() == 2);
  }

  SECTION("Function without return") {
    auto parser = createParser("func noReturn() { let x = 5; }");
    auto stmts = parser->parse();

    REQUIRE(stmts.size() == 1);

    CodeGen codegen;
    REQUIRE_NOTHROW(codegen.generate(stmts));

    auto module = codegen.getModule();
    REQUIRE(module != nullptr);
  }
}

TEST_CASE("CodeGen handles variable initialization", "[codegen]") {
  SECTION("Integer variable") {
    auto parser = createParser("let x = 42;");
    auto stmts = parser->parse();

    REQUIRE(stmts.size() == 1);

    CodeGen codegen;
    REQUIRE_NOTHROW(codegen.generate(stmts));

    auto module = codegen.getModule();
    REQUIRE(module != nullptr);
  }

  SECTION("String variable") {
    auto parser = createParser("let msg = \"hello\";");
    auto stmts = parser->parse();

    REQUIRE(stmts.size() == 1);

    CodeGen codegen;
    REQUIRE_NOTHROW(codegen.generate(stmts));

    auto module = codegen.getModule();
    REQUIRE(module != nullptr);
  }

  SECTION("Expression initialization") {
    auto parser = createParser("let x = 1 + 2 * 3;");
    auto stmts = parser->parse();

    REQUIRE(stmts.size() == 1);

    CodeGen codegen;
    REQUIRE_NOTHROW(codegen.generate(stmts));

    auto module = codegen.getModule();
    REQUIRE(module != nullptr);
  }
}

TEST_CASE("CodeGen handles control flow", "[codegen]") {
  SECTION("If statement") {
    auto parser = createParser("if (1 > 0) { print 1; }");
    auto stmts = parser->parse();

    REQUIRE(stmts.size() == 1);

    CodeGen codegen;
    REQUIRE_NOTHROW(codegen.generate(stmts));

    auto module = codegen.getModule();
    REQUIRE(module != nullptr);
  }

  SECTION("If-else statement") {
    auto parser = createParser("if (1 < 0) { print 1; } else { print 2; }");
    auto stmts = parser->parse();

    REQUIRE(stmts.size() == 1);

    CodeGen codegen;
    REQUIRE_NOTHROW(codegen.generate(stmts));

    auto module = codegen.getModule();
    REQUIRE(module != nullptr);
  }

  SECTION("While loop") {
    auto parser = createParser("while (0 > 1) { print 1; }");
    auto stmts = parser->parse();

    REQUIRE(stmts.size() == 1);

    CodeGen codegen;
    REQUIRE_NOTHROW(codegen.generate(stmts));

    auto module = codegen.getModule();
    REQUIRE(module != nullptr);
  }

  SECTION("For loop") {
    auto parser = createParser("for (i in range(0, 5)) { print i; }");
    auto stmts = parser->parse();

    REQUIRE(stmts.size() == 1);

    CodeGen codegen;
    REQUIRE_NOTHROW(codegen.generate(stmts));

    auto module = codegen.getModule();
    REQUIRE(module != nullptr);
  }
}

TEST_CASE("CodeGen handles function calls", "[codegen]") {
  SECTION("Function call in expression") {
    auto parser =
        createParser("func double(x) { return x * 2; } let y = double(5);");
    auto stmts = parser->parse();

    REQUIRE(stmts.size() == 2);

    CodeGen codegen;
    REQUIRE_NOTHROW(codegen.generate(stmts));

    auto module = codegen.getModule();
    REQUIRE(module != nullptr);
  }

  SECTION("Nested function calls") {
    auto parser = createParser(
        "func add(x, y) { return x + y; } let z = add(add(1, 2), 3);");
    auto stmts = parser->parse();

    REQUIRE(stmts.size() == 2);

    CodeGen codegen;
    REQUIRE_NOTHROW(codegen.generate(stmts));

    auto module = codegen.getModule();
    REQUIRE(module != nullptr);
  }
}

TEST_CASE("CodeGen handles unary expressions", "[codegen]") {
  SECTION("Simple unary minus") {
    auto parser = createParser("let x = -5;");
    auto stmts = parser->parse();

    REQUIRE(stmts.size() == 1);

    CodeGen codegen;
    REQUIRE_NOTHROW(codegen.generate(stmts));

    auto module = codegen.getModule();
    REQUIRE(module != nullptr);
  }

  SECTION("Double unary minus") {
    auto parser = createParser("let x = --10;");
    auto stmts = parser->parse();

    REQUIRE(stmts.size() == 1);

    CodeGen codegen;
    REQUIRE_NOTHROW(codegen.generate(stmts));

    auto module = codegen.getModule();
    REQUIRE(module != nullptr);
  }

  SECTION("Unary minus in expression") {
    auto parser = createParser("let x = -5 + 3;");
    auto stmts = parser->parse();

    REQUIRE(stmts.size() == 1);

    CodeGen codegen;
    REQUIRE_NOTHROW(codegen.generate(stmts));

    auto module = codegen.getModule();
    REQUIRE(module != nullptr);
  }
}

TEST_CASE("CodeGen handles comparisons", "[codegen]") {
  SECTION("Equal comparison") {
    auto parser = createParser("let x = 5 == 5;");
    auto stmts = parser->parse();

    REQUIRE(stmts.size() == 1);

    CodeGen codegen;
    REQUIRE_NOTHROW(codegen.generate(stmts));

    auto module = codegen.getModule();
    REQUIRE(module != nullptr);
  }

  SECTION("Greater than") {
    auto parser = createParser("let x = 10 > 5;");
    auto stmts = parser->parse();

    REQUIRE(stmts.size() == 1);

    CodeGen codegen;
    REQUIRE_NOTHROW(codegen.generate(stmts));

    auto module = codegen.getModule();
    REQUIRE(module != nullptr);
  }

  SECTION("Less than or equal") {
    auto parser = createParser("let x = 3 <= 5;");
    auto stmts = parser->parse();

    REQUIRE(stmts.size() == 1);

    CodeGen codegen;
    REQUIRE_NOTHROW(codegen.generate(stmts));

    auto module = codegen.getModule();
    REQUIRE(module != nullptr);
  }
}

TEST_CASE("CodeGen handles print statements", "[codegen]") {
  SECTION("Print number") {
    auto parser = createParser("print 42;");
    auto stmts = parser->parse();

    REQUIRE(stmts.size() == 1);

    CodeGen codegen;
    REQUIRE_NOTHROW(codegen.generate(stmts));

    auto module = codegen.getModule();
    REQUIRE(module != nullptr);
  }

  SECTION("Print variable") {
    auto parser = createParser("let x = 5; print x;");
    auto stmts = parser->parse();

    REQUIRE(stmts.size() == 2);

    CodeGen codegen;
    REQUIRE_NOTHROW(codegen.generate(stmts));

    auto module = codegen.getModule();
    REQUIRE(module != nullptr);
  }
}

TEST_CASE("CodeGen throws on undefined references", "[codegen]") {
  SECTION("Undefined variable") {
    auto parser = createParser("print undefined_var;");
    auto stmts = parser->parse();

    REQUIRE(stmts.size() == 1);

    CodeGen codegen;
    REQUIRE_THROWS_AS(codegen.generate(stmts), std::runtime_error);
  }

  SECTION("Undefined function") {
    auto parser = createParser("let x = undefined_func(5);");
    auto stmts = parser->parse();

    REQUIRE(stmts.size() == 1);

    CodeGen codegen;
    REQUIRE_THROWS_AS(codegen.generate(stmts), std::runtime_error);
  }

  SECTION("Wrong argument count") {
    auto parser =
        createParser("func oneArg(x) { return x; } let y = oneArg(1, 2);");
    auto stmts = parser->parse();

    REQUIRE(stmts.size() == 2);

    CodeGen codegen;
    REQUIRE_THROWS_AS(codegen.generate(stmts), std::runtime_error);
  }
}

TEST_CASE("CodeGen handles array literals", "[codegen]") {
  SECTION("Simple array") {
    auto parser = createParser("let arr = [1, 2, 3];");
    auto stmts = parser->parse();

    REQUIRE(stmts.size() == 1);

    CodeGen codegen;
    REQUIRE_NOTHROW(codegen.generate(stmts));

    auto module = codegen.getModule();
    REQUIRE(module != nullptr);
  }

  SECTION("Empty array") {
    auto parser = createParser("let arr = [];");
    auto stmts = parser->parse();

    REQUIRE(stmts.size() == 1);

    CodeGen codegen;
    REQUIRE_NOTHROW(codegen.generate(stmts));

    auto module = codegen.getModule();
    REQUIRE(module != nullptr);
  }

  SECTION("Array with expressions") {
    auto parser = createParser("let arr = [1 + 2, 3 * 4, 5 - 1];");
    auto stmts = parser->parse();

    REQUIRE(stmts.size() == 1);

    CodeGen codegen;
    REQUIRE_NOTHROW(codegen.generate(stmts));

    auto module = codegen.getModule();
    REQUIRE(module != nullptr);
  }

  SECTION("Single element array") {
    auto parser = createParser("let arr = [42];");
    auto stmts = parser->parse();

    REQUIRE(stmts.size() == 1);

    CodeGen codegen;
    REQUIRE_NOTHROW(codegen.generate(stmts));

    auto module = codegen.getModule();
    REQUIRE(module != nullptr);
  }
}

TEST_CASE("CodeGen handles object literals", "[codegen]") {
  SECTION("Simple object") {
    auto parser = createParser("let obj = {name: \"test\", value: 42};");
    auto stmts = parser->parse();

    REQUIRE(stmts.size() == 1);

    CodeGen codegen;
    REQUIRE_NOTHROW(codegen.generate(stmts));

    auto module = codegen.getModule();
    REQUIRE(module != nullptr);
  }

  SECTION("Empty object") {
    auto parser = createParser("let obj = {};");
    auto stmts = parser->parse();

    REQUIRE(stmts.size() == 1);

    CodeGen codegen;
    REQUIRE_NOTHROW(codegen.generate(stmts));

    auto module = codegen.getModule();
    REQUIRE(module != nullptr);
  }

  SECTION("Object with expressions") {
    auto parser = createParser("let obj = {a: 1 + 1, b: 2 * 3};");
    auto stmts = parser->parse();

    REQUIRE(stmts.size() == 1);

    CodeGen codegen;
    REQUIRE_NOTHROW(codegen.generate(stmts));

    auto module = codegen.getModule();
    REQUIRE(module != nullptr);
  }

  SECTION("Nested object") {
    auto parser = createParser("let obj = {outer: {inner: 42}};");
    auto stmts = parser->parse();

    REQUIRE(stmts.size() == 1);

    CodeGen codegen;
    REQUIRE_NOTHROW(codegen.generate(stmts));

    auto module = codegen.getModule();
    REQUIRE(module != nullptr);
  }
}

TEST_CASE("CodeGen handles nested control flow", "[codegen]") {
  SECTION("Nested if statements") {
    auto parser = createParser("if (1 > 0) { if (2 > 1) { print 1; } }");
    auto stmts = parser->parse();

    REQUIRE(stmts.size() == 1);

    CodeGen codegen;
    REQUIRE_NOTHROW(codegen.generate(stmts));

    auto module = codegen.getModule();
    REQUIRE(module != nullptr);
  }

  SECTION("If-else if chain") {
    auto parser = createParser(
        "if (0 > 1) { print 1; } else { if (1 > 0) { print 2; } }");
    auto stmts = parser->parse();

    REQUIRE(stmts.size() == 1);

    CodeGen codegen;
    REQUIRE_NOTHROW(codegen.generate(stmts));

    auto module = codegen.getModule();
    REQUIRE(module != nullptr);
  }

  SECTION("Nested while in for") {
    auto parser =
        createParser("for (i in range(0, 2)) { while (i < 1) { print i; } }");
    auto stmts = parser->parse();

    REQUIRE(stmts.size() == 1);

    CodeGen codegen;
    REQUIRE_NOTHROW(codegen.generate(stmts));

    auto module = codegen.getModule();
    REQUIRE(module != nullptr);
  }

  SECTION("For in while") {
    auto parser =
        createParser("while (true) { for (i in range(0, 1)) { print i; } }");
    auto stmts = parser->parse();

    REQUIRE(stmts.size() == 1);

    CodeGen codegen;
    REQUIRE_NOTHROW(codegen.generate(stmts));

    auto module = codegen.getModule();
    REQUIRE(module != nullptr);
  }

  SECTION("Deep nesting") {
    auto parser = createParser(
        "if (true) { while (false) { for (i in range(0, 1)) { print 1; } } }");
    auto stmts = parser->parse();

    REQUIRE(stmts.size() == 1);

    CodeGen codegen;
    REQUIRE_NOTHROW(codegen.generate(stmts));

    auto module = codegen.getModule();
    REQUIRE(module != nullptr);
  }
}

TEST_CASE("CodeGen handles empty blocks", "[codegen]") {
  SECTION("Empty function body") {
    auto parser = createParser("func empty() {}");
    auto stmts = parser->parse();

    REQUIRE(stmts.size() == 1);

    CodeGen codegen;
    REQUIRE_NOTHROW(codegen.generate(stmts));

    auto module = codegen.getModule();
    REQUIRE(module != nullptr);
    REQUIRE(module->getFunction("empty") != nullptr);
  }

  SECTION("Empty if body") {
    auto parser = createParser("if (1 > 0) {}");
    auto stmts = parser->parse();

    REQUIRE(stmts.size() == 1);

    CodeGen codegen;
    REQUIRE_NOTHROW(codegen.generate(stmts));

    auto module = codegen.getModule();
    REQUIRE(module != nullptr);
  }

  SECTION("Empty while body") {
    auto parser = createParser("while (false) {}");
    auto stmts = parser->parse();

    REQUIRE(stmts.size() == 1);

    CodeGen codegen;
    REQUIRE_NOTHROW(codegen.generate(stmts));

    auto module = codegen.getModule();
    REQUIRE(module != nullptr);
  }
}

TEST_CASE("CodeGen handles variable shadowing", "[codegen]") {
  SECTION("Shadowed variables") {
    auto parser = createParser("let x = 1; { let x = 2; print x; } print x;");
    auto stmts = parser->parse();

    REQUIRE(stmts.size() == 3);

    CodeGen codegen;
    REQUIRE_NOTHROW(codegen.generate(stmts));

    auto module = codegen.getModule();
    REQUIRE(module != nullptr);
  }
}

TEST_CASE("CodeGen handles large constants", "[codegen]") {
  SECTION("Maximum 32-bit integer") {
    auto parser = createParser("let x = 2147483647;");
    auto stmts = parser->parse();

    REQUIRE(stmts.size() == 1);

    CodeGen codegen;
    REQUIRE_NOTHROW(codegen.generate(stmts));

    auto module = codegen.getModule();
    REQUIRE(module != nullptr);
  }

  SECTION("Minimum 32-bit integer") {
    auto parser = createParser("let x = -2147483648;");
    auto stmts = parser->parse();

    REQUIRE(stmts.size() == 1);

    CodeGen codegen;
    REQUIRE_NOTHROW(codegen.generate(stmts));

    auto module = codegen.getModule();
    REQUIRE(module != nullptr);
  }

  SECTION("Zero") {
    auto parser = createParser("let x = 0;");
    auto stmts = parser->parse();

    REQUIRE(stmts.size() == 1);

    CodeGen codegen;
    REQUIRE_NOTHROW(codegen.generate(stmts));

    auto module = codegen.getModule();
    REQUIRE(module != nullptr);
  }
}

TEST_CASE("CodeGen handles all operators", "[codegen]") {
  SECTION("Addition") {
    auto parser = createParser("let x = 5 + 3;");
    auto stmts = parser->parse();
    CodeGen codegen;
    REQUIRE_NOTHROW(codegen.generate(stmts));
    REQUIRE(codegen.getModule() != nullptr);
  }

  SECTION("Subtraction") {
    auto parser = createParser("let x = 5 - 3;");
    auto stmts = parser->parse();
    CodeGen codegen;
    REQUIRE_NOTHROW(codegen.generate(stmts));
    REQUIRE(codegen.getModule() != nullptr);
  }

  SECTION("Multiplication") {
    auto parser = createParser("let x = 5 * 3;");
    auto stmts = parser->parse();
    CodeGen codegen;
    REQUIRE_NOTHROW(codegen.generate(stmts));
    REQUIRE(codegen.getModule() != nullptr);
  }

  SECTION("Division") {
    auto parser = createParser("let x = 6 / 2;");
    auto stmts = parser->parse();
    CodeGen codegen;
    REQUIRE_NOTHROW(codegen.generate(stmts));
    REQUIRE(codegen.getModule() != nullptr);
  }

  SECTION("Equality") {
    auto parser = createParser("let x = 5 == 5;");
    auto stmts = parser->parse();
    CodeGen codegen;
    REQUIRE_NOTHROW(codegen.generate(stmts));
    REQUIRE(codegen.getModule() != nullptr);
  }

  SECTION("Not equal") {
    auto parser = createParser("let x = 5 != 3;");
    auto stmts = parser->parse();
    CodeGen codegen;
    REQUIRE_NOTHROW(codegen.generate(stmts));
    REQUIRE(codegen.getModule() != nullptr);
  }

  SECTION("Greater than") {
    auto parser = createParser("let x = 5 > 3;");
    auto stmts = parser->parse();
    CodeGen codegen;
    REQUIRE_NOTHROW(codegen.generate(stmts));
    REQUIRE(codegen.getModule() != nullptr);
  }

  SECTION("Less than") {
    auto parser = createParser("let x = 3 < 5;");
    auto stmts = parser->parse();
    CodeGen codegen;
    REQUIRE_NOTHROW(codegen.generate(stmts));
    REQUIRE(codegen.getModule() != nullptr);
  }

  SECTION("Greater or equal") {
    auto parser = createParser("let x = 5 >= 5;");
    auto stmts = parser->parse();
    CodeGen codegen;
    REQUIRE_NOTHROW(codegen.generate(stmts));
    REQUIRE(codegen.getModule() != nullptr);
  }

  SECTION("Less or equal") {
    auto parser = createParser("let x = 3 <= 3;");
    auto stmts = parser->parse();
    CodeGen codegen;
    REQUIRE_NOTHROW(codegen.generate(stmts));
    REQUIRE(codegen.getModule() != nullptr);
  }
}

TEST_CASE("CodeGen handles complex expressions", "[codegen]") {
  SECTION("Chained arithmetic") {
    auto parser = createParser("let x = 1 + 2 + 3 + 4 + 5;");
    auto stmts = parser->parse();
    CodeGen codegen;
    REQUIRE_NOTHROW(codegen.generate(stmts));
    REQUIRE(codegen.getModule() != nullptr);
  }

  SECTION("Mixed operators") {
    auto parser = createParser("let x = 1 + 2 * 3 - 4 / 2;");
    auto stmts = parser->parse();
    CodeGen codegen;
    REQUIRE_NOTHROW(codegen.generate(stmts));
    REQUIRE(codegen.getModule() != nullptr);
  }

  SECTION("Parenthesized expressions") {
    auto parser = createParser("let x = (1 + 2) * (3 - 1);");
    auto stmts = parser->parse();
    CodeGen codegen;
    REQUIRE_NOTHROW(codegen.generate(stmts));
    REQUIRE(codegen.getModule() != nullptr);
  }

  SECTION("Unary minus chain") {
    auto parser = createParser("let x = ----5;");
    auto stmts = parser->parse();
    CodeGen codegen;
    REQUIRE_NOTHROW(codegen.generate(stmts));
    REQUIRE(codegen.getModule() != nullptr);
  }
}

TEST_CASE("CodeGen handles multiple statements", "[codegen]") {
  SECTION("Sequence of lets") {
    auto parser = createParser("let a = 1; let b = 2; let c = 3;");
    auto stmts = parser->parse();
    REQUIRE(stmts.size() == 3);
    CodeGen codegen;
    REQUIRE_NOTHROW(codegen.generate(stmts));
    REQUIRE(codegen.getModule() != nullptr);
  }

  SECTION("Mixed statements") {
    auto parser = createParser("let x = 5; print x; let y = 10;");
    auto stmts = parser->parse();
    REQUIRE(stmts.size() == 3);
    CodeGen codegen;
    REQUIRE_NOTHROW(codegen.generate(stmts));
    REQUIRE(codegen.getModule() != nullptr);
  }

  SECTION("Expression statements") {
    auto parser = createParser("1 + 2; 3 * 4;");
    auto stmts = parser->parse();
    REQUIRE(stmts.size() == 2);
    CodeGen codegen;
    REQUIRE_NOTHROW(codegen.generate(stmts));
    REQUIRE(codegen.getModule() != nullptr);
  }
}

TEST_CASE("CodeGen handles string concatenation", "[codegen]") {
  SECTION("String plus string") {
    auto parser = createParser("let x = \"hello\" + \" \" + \"world\";");
    auto stmts = parser->parse();
    REQUIRE(stmts.size() == 1);
    CodeGen codegen;
    REQUIRE_NOTHROW(codegen.generate(stmts));
    REQUIRE(codegen.getModule() != nullptr);
  }

  SECTION("String plus number") {
    auto parser = createParser("let x = \"value: \" + 42;");
    auto stmts = parser->parse();
    REQUIRE(stmts.size() == 1);
    CodeGen codegen;
    REQUIRE_NOTHROW(codegen.generate(stmts));
    REQUIRE(codegen.getModule() != nullptr);
  }
}
