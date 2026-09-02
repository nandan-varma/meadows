#include "codegen/CodeGen.h"
#include "ast/AST.h"
#include <catch2/catch_all.hpp>
#include "codegen/StringUtils.h"
#include "codegen/TypeUtils.h"
#include "lexer/Lexer.h"
#include "parser/Parser.h"
#include <llvm/IR/Verifier.h>
#include <sstream>

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
    auto parser = createParser("if (1 > 0) { print(1); }");
    auto stmts = parser->parse();

    REQUIRE(stmts.size() == 1);

    CodeGen codegen;
    REQUIRE_NOTHROW(codegen.generate(stmts));

    auto module = codegen.getModule();
    REQUIRE(module != nullptr);
  }

  SECTION("If-else statement") {
    auto parser = createParser("if (1 < 0) { print(1); } else { print(2); }");
    auto stmts = parser->parse();

    REQUIRE(stmts.size() == 1);

    CodeGen codegen;
    REQUIRE_NOTHROW(codegen.generate(stmts));

    auto module = codegen.getModule();
    REQUIRE(module != nullptr);
  }

  SECTION("While loop") {
    auto parser = createParser("while (0 > 1) { print(1); }");
    auto stmts = parser->parse();

    REQUIRE(stmts.size() == 1);

    CodeGen codegen;
    REQUIRE_NOTHROW(codegen.generate(stmts));

    auto module = codegen.getModule();
    REQUIRE(module != nullptr);
  }

  SECTION("For loop") {
    auto parser = createParser("for (i in range(0, 5)) { print(i); }");
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
    auto parser = createParser("print(42);");
    auto stmts = parser->parse();

    REQUIRE(stmts.size() == 1);

    CodeGen codegen;
    REQUIRE_NOTHROW(codegen.generate(stmts));

    auto module = codegen.getModule();
    REQUIRE(module != nullptr);
  }

  SECTION("Print variable") {
    auto parser = createParser("let x = 5; print(x);");
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
    auto parser = createParser("print(undefined_var);");
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

TEST_CASE("CodeGen handles break and continue", "[codegen][loops]") {
  SECTION("Break in while loop") {
    auto parser = createParser(
        "let x = 0; while (x < 10) { x = x + 1; if (x == 5) { break; } }");
    auto stmts = parser->parse();
    REQUIRE(stmts.size() == 2);
    CodeGen codegen;
    REQUIRE_NOTHROW(codegen.generate(stmts));
    REQUIRE(codegen.getModule() != nullptr);
  }

  SECTION("Continue in while loop") {
    auto parser = createParser(
        "let x = 0; while (x < 5) { x = x + 1; if (x == 3) { continue; } }");
    auto stmts = parser->parse();
    REQUIRE(stmts.size() == 2);
    CodeGen codegen;
    REQUIRE_NOTHROW(codegen.generate(stmts));
    REQUIRE(codegen.getModule() != nullptr);
  }

  SECTION("Break in for loop") {
    auto parser =
        createParser("for (i in range(0, 10)) { if (i == 5) { break; } }");
    auto stmts = parser->parse();
    REQUIRE(stmts.size() == 1);
    CodeGen codegen;
    REQUIRE_NOTHROW(codegen.generate(stmts));
    REQUIRE(codegen.getModule() != nullptr);
  }

  SECTION("Nested break") {
    auto parser =
        createParser("while (true) { while (true) { break; } break; }");
    auto stmts = parser->parse();
    REQUIRE(stmts.size() == 1);
    CodeGen codegen;
    REQUIRE_NOTHROW(codegen.generate(stmts));
    REQUIRE(codegen.getModule() != nullptr);
  }
}

TEST_CASE("CodeGen handles nested control flow", "[codegen]") {
  SECTION("Nested if statements") {
    auto parser = createParser("if (1 > 0) { if (2 > 1) { print(1); } }");
    auto stmts = parser->parse();

    REQUIRE(stmts.size() == 1);

    CodeGen codegen;
    REQUIRE_NOTHROW(codegen.generate(stmts));

    auto module = codegen.getModule();
    REQUIRE(module != nullptr);
  }

  SECTION("If-else if chain") {
    auto parser = createParser(
        "if (0 > 1) { print(1); } else { if (1 > 0) { print(2); } }");
    auto stmts = parser->parse();

    REQUIRE(stmts.size() == 1);

    CodeGen codegen;
    REQUIRE_NOTHROW(codegen.generate(stmts));

    auto module = codegen.getModule();
    REQUIRE(module != nullptr);
  }

  SECTION("Nested while in for") {
    auto parser =
        createParser("for (i in range(0, 2)) { while (i < 1) { print(i); } }");
    auto stmts = parser->parse();

    REQUIRE(stmts.size() == 1);

    CodeGen codegen;
    REQUIRE_NOTHROW(codegen.generate(stmts));

    auto module = codegen.getModule();
    REQUIRE(module != nullptr);
  }

  SECTION("For in while") {
    auto parser =
        createParser("while (true) { for (i in range(0, 1)) { print(i); } }");
    auto stmts = parser->parse();

    REQUIRE(stmts.size() == 1);

    CodeGen codegen;
    REQUIRE_NOTHROW(codegen.generate(stmts));

    auto module = codegen.getModule();
    REQUIRE(module != nullptr);
  }

  SECTION("Deep nesting") {
    auto parser = createParser(
        "if (true) { while (false) { for (i in range(0, 1)) { print(1); } } }");
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
    auto parser = createParser("let x = 1; { let x = 2; print(x); } print(x);");
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
    auto parser = createParser("let x = 5; print(x); let y = 10;");
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

TEST_CASE("CodeGen handles string concatenation", "[codegen][strings]") {
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

  SECTION("Concatenation in print") {
    auto parser = createParser("print(\"hello\" + \" world\");");
    auto stmts = parser->parse();
    REQUIRE(stmts.size() == 1);
    CodeGen codegen;
    REQUIRE_NOTHROW(codegen.generate(stmts));
    REQUIRE(codegen.getModule() != nullptr);
  }

  SECTION("Triple concatenation") {
    auto parser = createParser("let a = \"A\"; let b = a + \"B\" + \"C\";");
    auto stmts = parser->parse();
    REQUIRE(stmts.size() == 2);
    CodeGen codegen;
    REQUIRE_NOTHROW(codegen.generate(stmts));
    REQUIRE(codegen.getModule() != nullptr);
  }
}

TEST_CASE("CodeGen handles string printing", "[codegen][print][strings]") {
  SECTION("Print string literal") {
    auto parser = createParser("print(\"hello\");");
    auto stmts = parser->parse();
    REQUIRE(stmts.size() == 1);
    CodeGen codegen;
    REQUIRE_NOTHROW(codegen.generate(stmts));
    auto module = codegen.getModule();
    REQUIRE(module != nullptr);
    std::string ir;
    llvm::raw_string_ostream os(ir);
    module->print(os, nullptr);
    REQUIRE(ir.find("%s") != std::string::npos);
  }

  SECTION("Print string variable") {
    auto parser = createParser("let msg = \"hello world\"; print(msg);");
    auto stmts = parser->parse();
    REQUIRE(stmts.size() == 2);
    CodeGen codegen;
    REQUIRE_NOTHROW(codegen.generate(stmts));
    auto module = codegen.getModule();
    REQUIRE(module != nullptr);
  }

  SECTION("Print empty string") {
    auto parser = createParser("let empty = \"\"; print(empty);");
    auto stmts = parser->parse();
    REQUIRE(stmts.size() == 2);
    CodeGen codegen;
    REQUIRE_NOTHROW(codegen.generate(stmts));
    auto module = codegen.getModule();
    REQUIRE(module != nullptr);
  }

  SECTION("Print string concatenation") {
    auto parser = createParser("print(\"hello\" + \" world\");");
    auto stmts = parser->parse();
    REQUIRE(stmts.size() == 1);
    CodeGen codegen;
    REQUIRE_NOTHROW(codegen.generate(stmts));
    auto module = codegen.getModule();
    REQUIRE(module != nullptr);
  }
}

TEST_CASE("CodeGen handles variable reassignment", "[codegen][assign]") {
  SECTION("Basic reassignment") {
    auto parser = createParser("let x = 1; x = 2;");
    auto stmts = parser->parse();
    REQUIRE(stmts.size() == 2);
    CodeGen codegen;
    REQUIRE_NOTHROW(codegen.generate(stmts));
    auto module = codegen.getModule();
    REQUIRE(module != nullptr);
  }

  SECTION("Reassignment with expression") {
    auto parser = createParser("let x = 5; x = x + 1;");
    auto stmts = parser->parse();
    REQUIRE(stmts.size() == 2);
    CodeGen codegen;
    REQUIRE_NOTHROW(codegen.generate(stmts));
    auto module = codegen.getModule();
    REQUIRE(module != nullptr);
  }

  SECTION("Multiple reassignments") {
    auto parser = createParser("let x = 0; x = 1; x = 2; x = 3;");
    auto stmts = parser->parse();
    REQUIRE(stmts.size() == 4);
    CodeGen codegen;
    REQUIRE_NOTHROW(codegen.generate(stmts));
    auto module = codegen.getModule();
    REQUIRE(module != nullptr);
  }

  SECTION("Reassignment in expression") {
    auto parser = createParser("let x = 5; let y = x = 10;");
    auto stmts = parser->parse();
    REQUIRE(stmts.size() == 2);
    CodeGen codegen;
    REQUIRE_NOTHROW(codegen.generate(stmts));
    auto module = codegen.getModule();
    REQUIRE(module != nullptr);
  }
}

TEST_CASE("CodeGen handles array index and object field assignment",
         "[codegen][assign]") {
  SECTION("Array element assignment") {
    auto parser =
        createParser("let arr = [1, 2, 3]; arr[0] = 100; print(arr[0]);");
    auto stmts = parser->parse();
    CodeGen codegen;
    REQUIRE_NOTHROW(codegen.generate(stmts));
    auto module = codegen.getModule();
    REQUIRE(module != nullptr);

    std::string ir;
    llvm::raw_string_ostream os(ir);
    REQUIRE(llvm::verifyModule(*module, &os) == false);
  }

  SECTION("Array index assignment still runs the bounds check") {
    auto parser = createParser("let arr = [1, 2, 3]; arr[10] = 5;");
    auto stmts = parser->parse();
    CodeGen codegen;
    REQUIRE_NOTHROW(codegen.generate(stmts));

    std::string dump;
    llvm::raw_string_ostream dumpOs(dump);
    codegen.getModule()->print(dumpOs, nullptr);
    REQUIRE(dump.find("bounds_error") != std::string::npos);
  }

  SECTION("Object field assignment on a direct literal") {
    auto parser = createParser("let o = {a: 1, b: 2}; o.a = 100; print(o.a);");
    auto stmts = parser->parse();
    CodeGen codegen;
    REQUIRE_NOTHROW(codegen.generate(stmts));
    auto module = codegen.getModule();
    REQUIRE(module != nullptr);

    std::string ir;
    llvm::raw_string_ostream os(ir);
    REQUIRE(llvm::verifyModule(*module, &os) == false);
  }

  SECTION("Object field assignment through a variable alias") {
    auto parser = createParser(
        "let o = {a: 1}; let alias = o; alias.a = 100; print(o.a);");
    auto stmts = parser->parse();
    CodeGen codegen;
    REQUIRE_NOTHROW(codegen.generate(stmts));
    auto module = codegen.getModule();
    REQUIRE(module != nullptr);

    std::string ir;
    llvm::raw_string_ostream os(ir);
    REQUIRE(llvm::verifyModule(*module, &os) == false);
  }

  SECTION("Assignment expression evaluates to the assigned value") {
    auto parser =
        createParser("let arr = [1, 2, 3]; print(arr[0] = 42);");
    auto stmts = parser->parse();
    CodeGen codegen;
    REQUIRE_NOTHROW(codegen.generate(stmts));
    REQUIRE(codegen.getModule() != nullptr);
  }
}

TEST_CASE("CodeGen handles logical operators", "[codegen][logical]") {
  SECTION("Logical AND") {
    auto parser = createParser("let x = true && true;");
    auto stmts = parser->parse();
    REQUIRE(stmts.size() == 1);
    CodeGen codegen;
    REQUIRE_NOTHROW(codegen.generate(stmts));
    auto module = codegen.getModule();
    REQUIRE(module != nullptr);
  }

  SECTION("Logical OR") {
    auto parser = createParser("let x = false || true;");
    auto stmts = parser->parse();
    REQUIRE(stmts.size() == 1);
    CodeGen codegen;
    REQUIRE_NOTHROW(codegen.generate(stmts));
    auto module = codegen.getModule();
    REQUIRE(module != nullptr);
  }

  SECTION("Logical NOT") {
    auto parser = createParser("let x = !false;");
    auto stmts = parser->parse();
    REQUIRE(stmts.size() == 1);
    CodeGen codegen;
    REQUIRE_NOTHROW(codegen.generate(stmts));
    auto module = codegen.getModule();
    REQUIRE(module != nullptr);
  }

  SECTION("Compound logical expressions") {
    auto parser = createParser("let x = true && false || true;");
    auto stmts = parser->parse();
    REQUIRE(stmts.size() == 1);
    CodeGen codegen;
    REQUIRE_NOTHROW(codegen.generate(stmts));
    auto module = codegen.getModule();
    REQUIRE(module != nullptr);
  }

  SECTION("Logical operators with comparisons") {
    auto parser = createParser("let x = 5 > 0 && 10 < 20;");
    auto stmts = parser->parse();
    REQUIRE(stmts.size() == 1);
    CodeGen codegen;
    REQUIRE_NOTHROW(codegen.generate(stmts));
    auto module = codegen.getModule();
    REQUIRE(module != nullptr);
  }
}

TEST_CASE("StringUtils escape handler", "[stringutils]") {
  SECTION("Process newline escape") {
    std::string input = "line1\\nline2";
    auto result = StringUtils::EscapeHandler::process(input);
    REQUIRE(result == "line1\nline2");
  }

  SECTION("Process tab escape") {
    std::string input = "col1\\tcol2";
    auto result = StringUtils::EscapeHandler::process(input);
    REQUIRE(result == "col1\tcol2");
  }

  SECTION("Process backslash escape") {
    std::string input = "path\\\\to\\\\file";
    auto result = StringUtils::EscapeHandler::process(input);
    REQUIRE(result == "path\\to\\file");
  }

  SECTION("Process quote escape") {
    std::string input = "He said \\\"hello\\\"";
    auto result = StringUtils::EscapeHandler::process(input);
    REQUIRE(result == "He said \"hello\"");
  }

  SECTION("Escape for output") {
    std::string input = "line1\ncol1\t";
    auto result = StringUtils::EscapeHandler::escape(input);
    REQUIRE(result == "line1\\ncol1\\t");
  }
}

TEST_CASE("StringPool singleton", "[stringpool]") {
  SECTION("Intern returns same pointer for same string") {
    auto &pool = StringUtils::StringPool::getInstance();
    auto *str1 = pool.intern("test");
    auto *str2 = pool.intern("test");
    REQUIRE(str1 == str2);
  }

  SECTION("Intern returns different pointer for different strings") {
    auto &pool = StringUtils::StringPool::getInstance();
    auto *str1 = pool.intern("test1");
    auto *str2 = pool.intern("test2");
    REQUIRE(str1 != str2);
  }

  SECTION("Pool size increases with new strings") {
    auto &pool = StringUtils::StringPool::getInstance();
    size_t initialSize = pool.poolSize();
    pool.intern("unique_test_string_12345");
    REQUIRE(pool.poolSize() > initialSize);
  }
}

TEST_CASE("TypeUtils helpers", "[typeutils]") {
  SECTION("isIntegerType for i32") {
    auto context = std::make_unique<llvm::LLVMContext>();
    auto i32Type = llvm::Type::getInt32Ty(*context);
    llvm::Value *constInt = llvm::ConstantInt::get(i32Type, 42);
    REQUIRE(TypeUtils::isIntegerType(constInt) == true);
    REQUIRE(TypeUtils::isPointerType(constInt) == false);
  }

  SECTION("isPointerType for pointer") {
    auto context = std::make_unique<llvm::LLVMContext>();
    auto i8PtrType = llvm::PointerType::get(llvm::Type::getInt8Ty(*context), 0);
    llvm::Value *ptr = llvm::ConstantPointerNull::get(i8PtrType);
    REQUIRE(TypeUtils::isPointerType(ptr) == true);
    REQUIRE(TypeUtils::isIntegerType(ptr) == false);
  }

  SECTION("isBooleanType for i1") {
    auto context = std::make_unique<llvm::LLVMContext>();
    auto i1Type = llvm::Type::getInt1Ty(*context);
    llvm::Value *boolVal = llvm::ConstantInt::get(i1Type, 0);
    REQUIRE(TypeUtils::isBooleanType(boolVal) == true);
  }
}

TEST_CASE("CodeGen optimization flag", "[codegen][optimization]") {
  SECTION("Create codegen with optimization disabled") {
    CodeGen codegen(0);
    auto parser = createParser("let x = 5;");
    auto stmts = parser->parse();
    REQUIRE_NOTHROW(codegen.generate(stmts));
    REQUIRE(codegen.getModule() != nullptr);
  }

  SECTION("Create codegen with optimization enabled") {
    CodeGen codegen(2);
    codegen.setOptLevel(2);
    auto parser = createParser("let x = 5;");
    auto stmts = parser->parse();
    REQUIRE_NOTHROW(codegen.generate(stmts));
    REQUIRE(codegen.getModule() != nullptr);
  }
}

TEST_CASE("CodeGen module verification", "[codegen][verification]") {
  SECTION("Generated module verifies successfully") {
    auto parser = createParser("let x = 1 + 2; print(x);");
    auto stmts = parser->parse();
    CodeGen codegen;
    codegen.generate(stmts);
    auto module = codegen.getModule();
    REQUIRE(module != nullptr);

    std::string ir;
    llvm::raw_string_ostream os(ir);
    auto error = llvm::verifyModule(*module, &os);
    REQUIRE(error == false);
  }

  SECTION("Complex program verifies successfully") {
    auto parser = createParser("func fib(n) { "
                               "  if (n <= 1) { return n; } "
                               "  return fib(n - 1) + fib(n - 2); "
                               "} "
                               "let result = fib(5); "
                               "print(result);");
    auto stmts = parser->parse();
    CodeGen codegen;
    codegen.generate(stmts);
    auto module = codegen.getModule();
    REQUIRE(module != nullptr);

    std::string ir;
    llvm::raw_string_ostream os(ir);
    auto error = llvm::verifyModule(*module, &os);
    REQUIRE(error == false);
  }
}

TEST_CASE("CodeGen scope management", "[codegen][scope]") {
  SECTION("Nested blocks with variable shadowing") {
    auto parser = createParser("let x = 1; "
                               "{ "
                               "  let x = 2; "
                               "  { "
                               "    let x = 3; "
                               "    print(x); "
                               "  } "
                               "  print(x); "
                               "} "
                               "print(x);");
    auto stmts = parser->parse();
    CodeGen codegen;
    REQUIRE_NOTHROW(codegen.generate(stmts));
    REQUIRE(codegen.getModule() != nullptr);
  }

  SECTION("Function parameter shadows outer variable") {
    auto parser = createParser("let x = 10; "
                               "func outer(x) { "
                               "  let y = x + 1; "
                               "  return y; "
                               "} "
                               "let result = outer(5); "
                               "print(result);");
    auto stmts = parser->parse();
    CodeGen codegen;
    REQUIRE_NOTHROW(codegen.generate(stmts));
    REQUIRE(codegen.getModule() != nullptr);
  }

  SECTION("Loop variable shadows outer variable") {
    auto parser = createParser("let x = 0; "
                               "for (x in range(0, 5)) { "
                               "  print(x); "
                               "}");
    auto stmts = parser->parse();
    CodeGen codegen;
    REQUIRE_NOTHROW(codegen.generate(stmts));
    REQUIRE(codegen.getModule() != nullptr);
  }
}

TEST_CASE("CodeGen resolves forward and mutual recursion between top-level "
         "functions",
         "[codegen][functions]") {
  // Regression test: CodeGen previously only created an llvm::Function when
  // it reached that FuncStmt in source order, so a call to a function
  // defined *later* in the file referenced a not-yet-existing function and
  // failed with "Undefined function" — even though SemanticAnalyzer's
  // pre-pass had already accepted the program. declareFunctionSignatures()
  // fixes this by declaring every top-level function before any body is
  // generated.
  SECTION("A function calling one defined later in the file") {
    auto parser = createParser("func a() { return b(); } "
                               "func b() { return 42; } "
                               "print(a());");
    auto stmts = parser->parse();
    CodeGen codegen;
    REQUIRE_NOTHROW(codegen.generate(stmts));
    auto module = codegen.getModule();
    REQUIRE(module != nullptr);

    std::string ir;
    llvm::raw_string_ostream os(ir);
    REQUIRE(llvm::verifyModule(*module, &os) == false);
  }

  SECTION("Two functions calling each other (true mutual recursion)") {
    auto parser = createParser("func isEven(n) { "
                               "  if (n == 0) { return 1; } "
                               "  return isOdd(n - 1); "
                               "} "
                               "func isOdd(n) { "
                               "  if (n == 0) { return 0; } "
                               "  return isEven(n - 1); "
                               "} "
                               "print(isEven(10));");
    auto stmts = parser->parse();
    CodeGen codegen;
    REQUIRE_NOTHROW(codegen.generate(stmts));
    auto module = codegen.getModule();
    REQUIRE(module != nullptr);

    std::string ir;
    llvm::raw_string_ostream os(ir);
    REQUIRE(llvm::verifyModule(*module, &os) == false);

    // Exactly one llvm::Function per Meadows function — if visitFuncStmt
    // didn't reuse the pre-pass's declaration, LLVM would silently rename
    // the second definition to "isOdd.1", leaving calls bound to the
    // original (bodyless) declaration undefined at link time.
    REQUIRE(module->getFunction("isEven") != nullptr);
    REQUIRE(module->getFunction("isOdd") != nullptr);
    REQUIRE(module->getFunction("isOdd.1") == nullptr);
    REQUIRE_FALSE(module->getFunction("isEven")->empty());
    REQUIRE_FALSE(module->getFunction("isOdd")->empty());
  }
}

TEST_CASE("CodeGen resolves field access through a variable, not just an "
         "inline literal",
         "[codegen][objects]") {
  SECTION("Field access on a variable bound directly to an object literal") {
    auto parser =
        createParser(R"(let o = {name: "Alice", age: 30}; print(o.name); print(o.age);)");
    auto stmts = parser->parse();
    CodeGen codegen;
    REQUIRE_NOTHROW(codegen.generate(stmts));
    auto module = codegen.getModule();
    REQUIRE(module != nullptr);

    std::string ir;
    llvm::raw_string_ostream os(ir);
    REQUIRE(llvm::verifyModule(*module, &os) == false);
  }

  SECTION("Shape threads through a chain of variable aliases") {
    auto parser = createParser(
        "let o = {x: 10, y: 20}; let p = o; let q = p; print(q.x + q.y);");
    auto stmts = parser->parse();
    CodeGen codegen;
    REQUIRE_NOTHROW(codegen.generate(stmts));
    auto module = codegen.getModule();
    REQUIRE(module != nullptr);

    std::string ir;
    llvm::raw_string_ostream os(ir);
    REQUIRE(llvm::verifyModule(*module, &os) == false);
  }

  SECTION("Unknown field through a variable is a compile-time error, not "
         "silently wrong output") {
    auto parser = createParser("let o = {a: 1}; print(o.b);");
    auto stmts = parser->parse();
    CodeGen codegen;
    REQUIRE_THROWS_AS(codegen.generate(stmts), std::runtime_error);
  }

  SECTION("Chained field access is a clear compile-time error, not silently "
         "wrong output") {
    auto parser = createParser("let o = {inner: {x: 1}}; print(o.inner.x);");
    auto stmts = parser->parse();
    CodeGen codegen;
    REQUIRE_THROWS_AS(codegen.generate(stmts), std::runtime_error);
  }
}

TEST_CASE("CodeGen resolves len() on arrays as a compile-time constant",
         "[codegen][arrays]") {
  SECTION("len() on an array literal") {
    auto parser = createParser("print(len([1, 2, 3, 4]));");
    auto stmts = parser->parse();
    CodeGen codegen;
    REQUIRE_NOTHROW(codegen.generate(stmts));
    auto module = codegen.getModule();
    REQUIRE(module != nullptr);

    std::string ir;
    llvm::raw_string_ostream os(ir);
    REQUIRE(llvm::verifyModule(*module, &os) == false);

    // A compile-time-constant length shouldn't need a runtime strlen call.
    std::string dump;
    llvm::raw_string_ostream dumpOs(dump);
    module->print(dumpOs, nullptr);
    REQUIRE(dump.find("call i64 @strlen") == std::string::npos);
  }

  SECTION("len() on a variable bound to an array") {
    auto parser = createParser("let arr = [1, 2, 3, 4, 5]; print(len(arr));");
    auto stmts = parser->parse();
    CodeGen codegen;
    REQUIRE_NOTHROW(codegen.generate(stmts));
    REQUIRE(codegen.getModule() != nullptr);
  }

  SECTION("len() threads through a variable alias") {
    auto parser =
        createParser("let arr = [1, 2, 3]; let alias = arr; print(len(alias));");
    auto stmts = parser->parse();
    CodeGen codegen;
    REQUIRE_NOTHROW(codegen.generate(stmts));
    REQUIRE(codegen.getModule() != nullptr);
  }

  SECTION("len() on a string still uses runtime strlen") {
    auto parser = createParser(R"(print(len("hello"));)");
    auto stmts = parser->parse();
    CodeGen codegen;
    REQUIRE_NOTHROW(codegen.generate(stmts));

    std::string dump;
    llvm::raw_string_ostream dumpOs(dump);
    codegen.getModule()->print(dumpOs, nullptr);
    REQUIRE(dump.find("call i64 @strlen") != std::string::npos);
  }
}

TEST_CASE("CodeGen compares string content, not pointer identity, for == and !=",
         "[codegen][strings]") {
  // Regression test: `==`/`!=` on two pointer-typed operands previously
  // compiled to a raw pointer comparison (ICmpEQ on the pointer values
  // themselves), which only happens to be true when LLVM deduplicates two
  // identical string-literal globals — not a property Meadows programs
  // should depend on, and never true for two runtime-built strings.
  SECTION("Equality on strings lowers to a strcmp call") {
    auto parser = createParser(R"(let a = "hi"; let b = "h" + "i"; print(a == b);)");
    auto stmts = parser->parse();
    CodeGen codegen;
    REQUIRE_NOTHROW(codegen.generate(stmts));
    auto module = codegen.getModule();
    REQUIRE(module != nullptr);

    std::string ir;
    llvm::raw_string_ostream os(ir);
    REQUIRE(llvm::verifyModule(*module, &os) == false);

    std::string dump;
    llvm::raw_string_ostream dumpOs(dump);
    module->print(dumpOs, nullptr);
    REQUIRE(dump.find("call i32 @strcmp") != std::string::npos);
  }

  SECTION("Integer equality still compiles to a direct icmp, not strcmp") {
    auto parser = createParser("let a = 1; let b = 1; print(a == b);");
    auto stmts = parser->parse();
    CodeGen codegen;
    REQUIRE_NOTHROW(codegen.generate(stmts));
    auto module = codegen.getModule();

    std::string dump;
    llvm::raw_string_ostream dumpOs(dump);
    module->print(dumpOs, nullptr);
    REQUIRE(dump.find("call i32 @strcmp") == std::string::npos);
  }
}

TEST_CASE("CodeGen handles float literals and arithmetic", "[codegen][float]") {
  SECTION("Float literal and print use double + %g") {
    auto parser = createParser("print(3.14159);");
    auto stmts = parser->parse();
    CodeGen codegen;
    REQUIRE_NOTHROW(codegen.generate(stmts));
    auto module = codegen.getModule();
    REQUIRE(module != nullptr);

    std::string ir;
    llvm::raw_string_ostream os(ir);
    REQUIRE(llvm::verifyModule(*module, &os) == false);

    std::string dump;
    llvm::raw_string_ostream dumpOs(dump);
    module->print(dumpOs, nullptr);
    REQUIRE(dump.find("double") != std::string::npos);
    REQUIRE(dump.find("%g") != std::string::npos);
  }

  SECTION("Float arithmetic lowers to F-prefixed instructions") {
    // Non-constant operands (variables) throughout — the IRBuilder's
    // constant folder can otherwise fold a pure-constant fadd/fsub/etc. away
    // into a single ConstantFP, leaving no instruction to find in the IR.
    auto parser = createParser(
        "let a = 1.5; let b = 2.5; let c = 5.0; let d = 1.5; let e = 2.0; "
        "let f = 3.5; let g = 7.0; let h = 2.0; let i = 10.5; let j = 3.0; "
        "print(a + b); print(c - d); print(e * f); print(g / h); print(i % j);");
    auto stmts = parser->parse();
    CodeGen codegen;
    REQUIRE_NOTHROW(codegen.generate(stmts));
    auto module = codegen.getModule();
    REQUIRE(module != nullptr);

    std::string ir;
    llvm::raw_string_ostream os(ir);
    REQUIRE(llvm::verifyModule(*module, &os) == false);

    std::string dump;
    llvm::raw_string_ostream dumpOs(dump);
    module->print(dumpOs, nullptr);
    REQUIRE(dump.find("fadd") != std::string::npos);
    REQUIRE(dump.find("fsub") != std::string::npos);
    REQUIRE(dump.find("fmul") != std::string::npos);
    REQUIRE(dump.find("fdiv") != std::string::npos);
    REQUIRE(dump.find("frem") != std::string::npos);
  }

  SECTION("Mixed int/float arithmetic promotes the int operand") {
    auto parser = createParser("let x = 5; print(x + 2.5);");
    auto stmts = parser->parse();
    CodeGen codegen;
    REQUIRE_NOTHROW(codegen.generate(stmts));
    auto module = codegen.getModule();
    REQUIRE(module != nullptr);

    std::string ir;
    llvm::raw_string_ostream os(ir);
    REQUIRE(llvm::verifyModule(*module, &os) == false);

    std::string dump;
    llvm::raw_string_ostream dumpOs(dump);
    module->print(dumpOs, nullptr);
    REQUIRE(dump.find("sitofp") != std::string::npos);
    REQUIRE(dump.find("fadd") != std::string::npos);
  }

  SECTION("Float comparisons lower to ordered fcmp") {
    // Non-constant operands (variables) so the IRBuilder's constant folder
    // doesn't fold the comparison away before it can be observed in the IR.
    auto parser = createParser(
        "let a = 1.5; let b = 1.5; let c = 3.0; let d = 1; let e = 1.0; "
        "print(a == b); print(a < c); print(d == e);");
    auto stmts = parser->parse();
    CodeGen codegen;
    REQUIRE_NOTHROW(codegen.generate(stmts));
    auto module = codegen.getModule();
    REQUIRE(module != nullptr);

    std::string ir;
    llvm::raw_string_ostream os(ir);
    REQUIRE(llvm::verifyModule(*module, &os) == false);

    std::string dump;
    llvm::raw_string_ostream dumpOs(dump);
    module->print(dumpOs, nullptr);
    REQUIRE(dump.find("fcmp oeq") != std::string::npos);
    REQUIRE(dump.find("fcmp olt") != std::string::npos);
  }

  SECTION("Float division by zero still runs a runtime check") {
    auto parser = createParser("let x = 1.0; let y = 0.0; print(x / y);");
    auto stmts = parser->parse();
    CodeGen codegen;
    REQUIRE_NOTHROW(codegen.generate(stmts));

    std::string dump;
    llvm::raw_string_ostream dumpOs(dump);
    codegen.getModule()->print(dumpOs, nullptr);
    REQUIRE(dump.find("div_error") != std::string::npos);
    REQUIRE(dump.find("fcmp oeq") != std::string::npos);
  }

  SECTION("Unary negation on a float uses fneg") {
    // A variable, not a literal — negating a constant folds to a plain
    // ConstantFP at build time, with no fneg instruction to observe.
    auto parser = createParser("let x = 3.5; print(-x);");
    auto stmts = parser->parse();
    CodeGen codegen;
    REQUIRE_NOTHROW(codegen.generate(stmts));

    std::string dump;
    llvm::raw_string_ostream dumpOs(dump);
    codegen.getModule()->print(dumpOs, nullptr);
    REQUIRE(dump.find("fneg") != std::string::npos);
  }

  SECTION("str() accepts a float and formats it with %g") {
    auto parser = createParser("print(str(3.14));");
    auto stmts = parser->parse();
    CodeGen codegen;
    REQUIRE_NOTHROW(codegen.generate(stmts));

    std::string dump;
    llvm::raw_string_ostream dumpOs(dump);
    codegen.getModule()->print(dumpOs, nullptr);
    REQUIRE(dump.find("@snprintf") != std::string::npos);
  }

  SECTION("String concatenation with a constant float literal") {
    auto parser = createParser(R"(print("pi = " + 3.14159);)");
    auto stmts = parser->parse();
    CodeGen codegen;
    REQUIRE_NOTHROW(codegen.generate(stmts));
    auto module = codegen.getModule();
    REQUIRE(module != nullptr);

    std::string ir;
    llvm::raw_string_ostream os(ir);
    REQUIRE(llvm::verifyModule(*module, &os) == false);
  }

  SECTION("String concatenation with a negative constant integer (regression: "
         "getSExtValue, not getZExtValue)") {
    auto parser = createParser(R"(print("n = " + (0 - 5));)");
    auto stmts = parser->parse();
    CodeGen codegen;
    REQUIRE_NOTHROW(codegen.generate(stmts));
    auto module = codegen.getModule();
    REQUIRE(module != nullptr);

    std::string ir;
    llvm::raw_string_ostream os(ir);
    REQUIRE(llvm::verifyModule(*module, &os) == false);
  }
}

TEST_CASE("CodeGen handles push() as a compile-time-sized array grow",
         "[codegen][arrays][push]") {
  SECTION("push() on a literal") {
    auto parser = createParser("let arr = push([1, 2, 3], 4); print(arr[3]);");
    auto stmts = parser->parse();
    CodeGen codegen;
    REQUIRE_NOTHROW(codegen.generate(stmts));
    auto module = codegen.getModule();
    REQUIRE(module != nullptr);

    std::string ir;
    llvm::raw_string_ostream os(ir);
    REQUIRE(llvm::verifyModule(*module, &os) == false);
  }

  SECTION("Rebinding via `arr = push(arr, x);` keeps len() accurate") {
    auto parser = createParser(
        "let arr = [1, 2, 3]; arr = push(arr, 4); print(len(arr));"
        "print(arr[3]);");
    auto stmts = parser->parse();
    CodeGen codegen;
    REQUIRE_NOTHROW(codegen.generate(stmts));
    auto module = codegen.getModule();
    REQUIRE(module != nullptr);

    std::string ir;
    llvm::raw_string_ostream os(ir);
    REQUIRE(llvm::verifyModule(*module, &os) == false);
  }

  SECTION("A pushed-and-reassigned array's last valid index passes bounds "
         "checking — regression for the array_len bug where the bounds "
         "check compared against the array's first element instead of its "
         "true length") {
    auto parser =
        createParser("let arr = [1, 2, 3]; arr = push(arr, 4); print(arr[3]);");
    auto stmts = parser->parse();
    CodeGen codegen;
    REQUIRE_NOTHROW(codegen.generate(stmts));

    // The regression specifically involved a runtime load of the array's
    // first element as a fake "length" — assert that's gone.
    std::string dump;
    llvm::raw_string_ostream dumpOs(dump);
    codegen.getModule()->print(dumpOs, nullptr);
    REQUIRE(dump.find("array_len") == std::string::npos);
  }

  SECTION("push() on a non-array is a compile-time error") {
    auto parser = createParser(R"(let x = 5; push(x, 1);)");
    auto stmts = parser->parse();
    CodeGen codegen;
    REQUIRE_THROWS_AS(codegen.generate(stmts), std::runtime_error);
  }
}
