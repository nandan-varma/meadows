#include "lexer/Lexer.h"
#include <catch2/catch_all.hpp>
#include "utils/Exceptions.h"

TEST_CASE("Lexer tokenizes keywords correctly", "[lexer]") {
  SECTION("All keywords are recognized") {
    Lexer lexer("let func if else for while return in range");
    auto tokens = lexer.tokenize();

    REQUIRE(tokens.size() == 10); // 9 keywords + EOF
    CHECK(tokens[0].type == TokenType::LET);
    CHECK(tokens[1].type == TokenType::FUNC);
    CHECK(tokens[2].type == TokenType::IF);
    CHECK(tokens[3].type == TokenType::ELSE);
    CHECK(tokens[4].type == TokenType::FOR);
    CHECK(tokens[5].type == TokenType::WHILE);
    CHECK(tokens[6].type == TokenType::RETURN);
    CHECK(tokens[7].type == TokenType::IN);
    CHECK(tokens[8].type == TokenType::RANGE);
    CHECK(tokens[9].type == TokenType::EOF_TOKEN);
  }

  SECTION("print is an identifier, not a keyword") {
    Lexer lexer("print");
    auto tokens = lexer.tokenize();
    REQUIRE(tokens.size() == 2);
    CHECK(tokens[0].type == TokenType::IDENTIFIER);
    CHECK(tokens[0].value == "print");
  }
}

TEST_CASE("Lexer handles numbers correctly", "[lexer]") {
  SECTION("Single number") {
    Lexer lexer("42");
    auto tokens = lexer.tokenize();

    REQUIRE(tokens.size() == 2);
    CHECK(tokens[0].type == TokenType::NUMBER);
    CHECK(tokens[0].value == "42");
  }

  SECTION("Multiple numbers") {
    Lexer lexer("123 456 789");
    auto tokens = lexer.tokenize();

    REQUIRE(tokens.size() == 4);
    CHECK(tokens[0].value == "123");
    CHECK(tokens[1].value == "456");
    CHECK(tokens[2].value == "789");
  }

  SECTION("Zero") {
    Lexer lexer("0");
    auto tokens = lexer.tokenize();

    REQUIRE(tokens.size() == 2);
    CHECK(tokens[0].type == TokenType::NUMBER);
    CHECK(tokens[0].value == "0");
  }

  SECTION("Float literal") {
    Lexer lexer("3.14159");
    auto tokens = lexer.tokenize();

    REQUIRE(tokens.size() == 2);
    CHECK(tokens[0].type == TokenType::NUMBER);
    CHECK(tokens[0].value == "3.14159");
  }

  SECTION("Float with trailing zero") {
    Lexer lexer("10.0");
    auto tokens = lexer.tokenize();

    REQUIRE(tokens.size() == 2);
    CHECK(tokens[0].value == "10.0");
  }

  SECTION("A dot not followed by a digit is not part of the number — stays "
         "separate for field access") {
    Lexer lexer("obj.field");
    auto tokens = lexer.tokenize();

    REQUIRE(tokens.size() == 4);
    CHECK(tokens[0].type == TokenType::IDENTIFIER);
    CHECK(tokens[1].type == TokenType::DOT);
    CHECK(tokens[2].type == TokenType::IDENTIFIER);
  }
}

TEST_CASE("Lexer handles string literals", "[lexer]") {
  SECTION("Simple string") {
    Lexer lexer("\"hello world\"");
    auto tokens = lexer.tokenize();

    REQUIRE(tokens.size() == 2);
    CHECK(tokens[0].type == TokenType::STRING);
    CHECK(tokens[0].value == "hello world");
  }

  SECTION("Empty string") {
    Lexer lexer("\"\"");
    auto tokens = lexer.tokenize();

    REQUIRE(tokens.size() == 2);
    CHECK(tokens[0].type == TokenType::STRING);
    CHECK(tokens[0].value == "");
  }

  SECTION("String with spaces") {
    Lexer lexer("\"  multiple   spaces  \"");
    auto tokens = lexer.tokenize();

    REQUIRE(tokens.size() == 2);
    CHECK(tokens[0].value == "  multiple   spaces  ");
  }
}

TEST_CASE("Lexer detects unterminated strings", "[lexer]") {
  Lexer lexer("\"unterminated string");
  REQUIRE_THROWS_AS(lexer.tokenize(), meadows::LexicalException);
}

TEST_CASE("Lexer handles identifiers", "[lexer]") {
  SECTION("Simple identifier") {
    Lexer lexer("myVariable");
    auto tokens = lexer.tokenize();

    REQUIRE(tokens.size() == 2);
    CHECK(tokens[0].type == TokenType::IDENTIFIER);
    CHECK(tokens[0].value == "myVariable");
  }

  SECTION("Identifier with underscore") {
    Lexer lexer("_private_var");
    auto tokens = lexer.tokenize();

    REQUIRE(tokens.size() == 2);
    CHECK(tokens[0].type == TokenType::IDENTIFIER);
    CHECK(tokens[0].value == "_private_var");
  }

  SECTION("Identifier with numbers") {
    Lexer lexer("var123");
    auto tokens = lexer.tokenize();

    REQUIRE(tokens.size() == 2);
    CHECK(tokens[0].type == TokenType::IDENTIFIER);
    CHECK(tokens[0].value == "var123");
  }
}

TEST_CASE("Lexer handles operators", "[lexer]") {
  SECTION("Arithmetic operators") {
    Lexer lexer("+ - * /");
    auto tokens = lexer.tokenize();

    REQUIRE(tokens.size() == 5);
    CHECK(tokens[0].type == TokenType::PLUS);
    CHECK(tokens[1].type == TokenType::MINUS);
    CHECK(tokens[2].type == TokenType::STAR);
    CHECK(tokens[3].type == TokenType::SLASH);
  }

  SECTION("Comparison operators") {
    Lexer lexer("== > < >= <=");
    auto tokens = lexer.tokenize();

    REQUIRE(tokens.size() == 6);
    CHECK(tokens[0].type == TokenType::EQUAL_EQUAL);
    CHECK(tokens[1].type == TokenType::GREATER);
    CHECK(tokens[2].type == TokenType::LESS);
    CHECK(tokens[3].type == TokenType::GREATER_EQUAL);
    CHECK(tokens[4].type == TokenType::LESS_EQUAL);
  }

  SECTION("Assignment operator") {
    Lexer lexer("=");
    auto tokens = lexer.tokenize();

    REQUIRE(tokens.size() == 2);
    CHECK(tokens[0].type == TokenType::EQUAL);
  }
}

TEST_CASE("Lexer handles punctuation", "[lexer]") {
  Lexer lexer("( ) { } [ ] , ; :");
  auto tokens = lexer.tokenize();

  REQUIRE(tokens.size() == 10);
  CHECK(tokens[0].type == TokenType::LEFT_PAREN);
  CHECK(tokens[1].type == TokenType::RIGHT_PAREN);
  CHECK(tokens[2].type == TokenType::LEFT_BRACE);
  CHECK(tokens[3].type == TokenType::RIGHT_BRACE);
  CHECK(tokens[4].type == TokenType::LEFT_BRACKET);
  CHECK(tokens[5].type == TokenType::RIGHT_BRACKET);
  CHECK(tokens[6].type == TokenType::COMMA);
  CHECK(tokens[7].type == TokenType::SEMICOLON);
  CHECK(tokens[8].type == TokenType::COLON);
}

TEST_CASE("Lexer handles comments", "[lexer]") {
  SECTION("Single-line comment with #") {
    Lexer lexer("let x = 5; # this is a comment\nprint(x);");
    auto tokens = lexer.tokenize();

    bool foundPrint = false;
    for (const auto &token : tokens) {
      if (token.type == TokenType::IDENTIFIER && token.value == "print") {
        foundPrint = true;
        break;
      }
    }
    CHECK(foundPrint);
  }

  SECTION("Single-line comment with //") {
    Lexer lexer("let x = 5; // this is a comment\nprint(x);");
    auto tokens = lexer.tokenize();

    bool foundPrint = false;
    for (const auto &token : tokens) {
      if (token.type == TokenType::IDENTIFIER && token.value == "print") {
        foundPrint = true;
        break;
      }
    }
    CHECK(foundPrint);
  }
}

TEST_CASE("Lexer tracks line numbers", "[lexer]") {
  Lexer lexer("let x = 5;\nlet y = 10;");
  auto tokens = lexer.tokenize();

  // First let should be on line 1
  CHECK(tokens[0].line == 1);

  // Second let should be on line 2
  auto it = std::find_if(tokens.begin(), tokens.end(), [](const Token &t) {
    return t.type == TokenType::LET;
  });
  if (it != tokens.end()) {
    it = std::find_if(it + 1, tokens.end(),
                      [](const Token &t) { return t.type == TokenType::LET; });
    if (it != tokens.end()) {
      CHECK(it->line == 2);
    }
  }
}

TEST_CASE("Lexer handles empty input", "[lexer]") {
  Lexer lexer("");
  auto tokens = lexer.tokenize();

  REQUIRE(tokens.size() == 1);
  CHECK(tokens[0].type == TokenType::EOF_TOKEN);
}

TEST_CASE("Lexer handles whitespace", "[lexer]") {
  Lexer lexer("  \t\n  let   x  =  5  ;  ");
  auto tokens = lexer.tokenize();

  // Should still tokenize correctly despite whitespace
  REQUIRE(tokens.size() >= 6); // let, x, =, 5, ;, EOF
  CHECK(tokens[0].type == TokenType::LET);
}

TEST_CASE("Lexer distinguishes keywords from identifiers", "[lexer]") {
  SECTION("Identifier starting with keyword") {
    Lexer lexer("lettuce");
    auto tokens = lexer.tokenize();

    REQUIRE(tokens.size() == 2);
    CHECK(tokens[0].type == TokenType::IDENTIFIER);
    CHECK(tokens[0].value == "lettuce");
  }

  SECTION("Identifier containing keyword") {
    Lexer lexer("myfunction");
    auto tokens = lexer.tokenize();

    REQUIRE(tokens.size() == 2);
    CHECK(tokens[0].type == TokenType::IDENTIFIER);
    CHECK(tokens[0].value == "myfunction");
  }
}

TEST_CASE("Lexer handles edge cases", "[lexer]") {
  SECTION("Maximum integer value") {
    Lexer lexer("2147483647");
    auto tokens = lexer.tokenize();

    REQUIRE(tokens.size() == 2);
    CHECK(tokens[0].type == TokenType::NUMBER);
    CHECK(tokens[0].value == "2147483647");
  }

  SECTION("Zero value") {
    Lexer lexer("0");
    auto tokens = lexer.tokenize();

    REQUIRE(tokens.size() == 2);
    CHECK(tokens[0].type == TokenType::NUMBER);
    CHECK(tokens[0].value == "0");
  }

  SECTION("Leading zeros") {
    Lexer lexer("007");
    auto tokens = lexer.tokenize();

    REQUIRE(tokens.size() == 2);
    CHECK(tokens[0].type == TokenType::NUMBER);
    CHECK(tokens[0].value == "007");
  }

  SECTION("Mixed case keywords are identifiers") {
    Lexer lexer("LET Func IF");
    auto tokens = lexer.tokenize();

    REQUIRE(tokens.size() == 4);
    CHECK(tokens[0].type == TokenType::IDENTIFIER);
    CHECK(tokens[1].type == TokenType::IDENTIFIER);
    CHECK(tokens[2].type == TokenType::IDENTIFIER);
  }

  SECTION("String with escape-like sequence") {
    Lexer lexer("\"hello\\nworld\"");
    auto tokens = lexer.tokenize();

    REQUIRE(tokens.size() == 2);
    CHECK(tokens[0].type == TokenType::STRING);
    CHECK(tokens[0].value == "hello\nworld");
  }
}

TEST_CASE("Lexer handles escape sequences", "[lexer]") {
  SECTION("Newline escape") {
    Lexer lexer("\"line1\\nline2\"");
    auto tokens = lexer.tokenize();

    REQUIRE(tokens.size() == 2);
    CHECK(tokens[0].type == TokenType::STRING);
    CHECK(tokens[0].value == "line1\nline2");
  }

  SECTION("Tab escape") {
    Lexer lexer("\"col1\\tcol2\"");
    auto tokens = lexer.tokenize();

    REQUIRE(tokens.size() == 2);
    CHECK(tokens[0].type == TokenType::STRING);
    CHECK(tokens[0].value == "col1\tcol2");
  }

  SECTION("Backslash escape") {
    Lexer lexer("\"path\\\\to\\\\file\"");
    auto tokens = lexer.tokenize();

    REQUIRE(tokens.size() == 2);
    CHECK(tokens[0].type == TokenType::STRING);
    CHECK(tokens[0].value == "path\\to\\file");
  }

  SECTION("Quote escape") {
    Lexer lexer("\"He said \\\"hello\\\"\"");
    auto tokens = lexer.tokenize();

    REQUIRE(tokens.size() == 2);
    CHECK(tokens[0].type == TokenType::STRING);
    CHECK(tokens[0].value == "He said \"hello\"");
  }

  SECTION("Carriage return escape") {
    Lexer lexer("\"line1\\rline2\"");
    auto tokens = lexer.tokenize();

    REQUIRE(tokens.size() == 2);
    CHECK(tokens[0].type == TokenType::STRING);
    CHECK(tokens[0].value == "line1\rline2");
  }

  SECTION("Null character escape") {
    Lexer lexer("\"before\\0after\"");
    auto tokens = lexer.tokenize();

    REQUIRE(tokens.size() == 2);
    CHECK(tokens[0].type == TokenType::STRING);
    CHECK(tokens[0].value.length() == 12);
    CHECK(tokens[0].value[0] == 'b');
    CHECK(tokens[0].value[6] == '\0');
    CHECK(tokens[0].value[7] == 'a');
  }

  SECTION("Backspace escape") {
    Lexer lexer("\"text\\bbackspace\"");
    auto tokens = lexer.tokenize();

    REQUIRE(tokens.size() == 2);
    CHECK(tokens[0].type == TokenType::STRING);
    CHECK(tokens[0].value == "text\bbackspace");
  }

  SECTION("Form feed escape") {
    Lexer lexer("\"page1\\fpage2\"");
    auto tokens = lexer.tokenize();

    REQUIRE(tokens.size() == 2);
    CHECK(tokens[0].type == TokenType::STRING);
    CHECK(tokens[0].value == "page1\fpage2");
  }

  SECTION("Unknown escape passes through") {
    Lexer lexer("\"unknown\\xescape\"");
    auto tokens = lexer.tokenize();

    REQUIRE(tokens.size() == 2);
    CHECK(tokens[0].type == TokenType::STRING);
    CHECK(tokens[0].value == "unknownxescape");
  }

  SECTION("Multiple escapes") {
    Lexer lexer("\"\\n\\t\\\\\\\"\"");
    auto tokens = lexer.tokenize();

    REQUIRE(tokens.size() == 2);
    CHECK(tokens[0].type == TokenType::STRING);
    CHECK(tokens[0].value == "\n\t\\\"");
  }

  SECTION("Empty string") {
    Lexer lexer("\"\"");
    auto tokens = lexer.tokenize();

    REQUIRE(tokens.size() == 2);
    CHECK(tokens[0].type == TokenType::STRING);
    CHECK(tokens[0].value == "");
  }
}

TEST_CASE("Lexer handles complex tokens", "[lexer]") {
  SECTION("Deeply nested parentheses") {
    Lexer lexer("((((((x))))))");
    auto tokens = lexer.tokenize();

    REQUIRE(tokens.size() == 14);
    CHECK(tokens[0].type == TokenType::LEFT_PAREN);
    CHECK(tokens[1].type == TokenType::LEFT_PAREN);
    CHECK(tokens[2].type == TokenType::LEFT_PAREN);
    CHECK(tokens[3].type == TokenType::LEFT_PAREN);
    CHECK(tokens[4].type == TokenType::LEFT_PAREN);
    CHECK(tokens[5].type == TokenType::LEFT_PAREN);
    CHECK(tokens[6].type == TokenType::IDENTIFIER);
    CHECK(tokens[7].type == TokenType::RIGHT_PAREN);
    CHECK(tokens[8].type == TokenType::RIGHT_PAREN);
    CHECK(tokens[9].type == TokenType::RIGHT_PAREN);
    CHECK(tokens[10].type == TokenType::RIGHT_PAREN);
    CHECK(tokens[11].type == TokenType::RIGHT_PAREN);
    CHECK(tokens[12].type == TokenType::RIGHT_PAREN);
    CHECK(tokens[13].type == TokenType::EOF_TOKEN);
  }

  SECTION("Multiple operators in sequence") {
    Lexer lexer("+-*/");
    auto tokens = lexer.tokenize();

    REQUIRE(tokens.size() == 5);
    CHECK(tokens[0].type == TokenType::PLUS);
    CHECK(tokens[1].type == TokenType::MINUS);
    CHECK(tokens[2].type == TokenType::STAR);
    CHECK(tokens[3].type == TokenType::SLASH);
  }
}

TEST_CASE("Lexer property-based tests", "[lexer][property]") {
  SECTION("Identifiers preserve exact text") {
    std::string identifiers[] = {
        "x",          "var",
        "myVariable", "_private",
        "a1",         "var_123",
        "__double__", "veryLongVariableNameThatShouldStillWorkCorrectly"};

    for (const auto &id : identifiers) {
      Lexer lexer(id);
      auto tokens = lexer.tokenize();

      REQUIRE(tokens.size() == 2);
      CHECK(tokens[0].type == TokenType::IDENTIFIER);
      CHECK(tokens[0].value == id);
    }
  }

  SECTION("Numbers preserve exact digits") {
    std::string numbers[] = {"0", "1", "42", "12345", "007", "1000", "999999"};

    for (const auto &num : numbers) {
      Lexer lexer(num);
      auto tokens = lexer.tokenize();

      REQUIRE(tokens.size() == 2);
      CHECK(tokens[0].type == TokenType::NUMBER);
      CHECK(tokens[0].value == num);
    }
  }

  SECTION("All escape sequences are processed correctly") {
    struct EscapeTest {
      const char *input;
      const char *expected;
    };

    EscapeTest tests[] = {
        {"\"\\n\"", "\n"},  {"\"\\t\"", "\t"}, {"\"\\\\\"", "\\"},
        {"\"\\\"\"", "\""}, {"\"\\r\"", "\r"}, {"\"\\b\"", "\b"},
        {"\"\\f\"", "\f"},  {"\"\\a\"", "a"},  {"\"test\\n\"", "test\n"}};

    for (const auto &test : tests) {
      Lexer lexer(test.input);
      auto tokens = lexer.tokenize();

      REQUIRE(tokens.size() == 2);
      CHECK(tokens[0].type == TokenType::STRING);
      CHECK(tokens[0].value == test.expected);
    }

    SECTION("Null character escape") {
      Lexer lexer("\"\\0\"");
      auto tokens = lexer.tokenize();
      REQUIRE(tokens.size() == 2);
      CHECK(tokens[0].type == TokenType::STRING);
      CHECK(tokens[0].value.length() == 1);
      CHECK(tokens[0].value[0] == '\0');
    }
  }

  SECTION("Line tracking is consistent") {
    std::string source = "let x = 1;\nlet y = 2;\nlet z = 3;";
    Lexer lexer(source);
    auto tokens = lexer.tokenize();

    int letCount = 0;
    for (const auto &token : tokens) {
      if (token.type == TokenType::LET) {
        letCount++;
        if (letCount == 1)
          CHECK(token.line == 1);
        if (letCount == 2)
          CHECK(token.line == 2);
        if (letCount == 3)
          CHECK(token.line == 3);
      }
    }
    CHECK(letCount == 3);
  }

  SECTION("Comments are completely ignored") {
    std::string sources[] = {"# comment\nlet x = 1;", "// comment\nlet x = 1;",
                             "# line1\n# line2\nlet x = 1;",
                             "let x = 1; # inline\nlet y = 2;"};

    for (const auto &src : sources) {
      Lexer lexer(src);
      auto tokens = lexer.tokenize();

      bool foundLet = false;
      bool foundX = false;
      for (const auto &token : tokens) {
        if (token.type == TokenType::LET)
          foundLet = true;
        if (token.type == TokenType::IDENTIFIER && token.value == "x")
          foundX = true;
      }
      CHECK(foundLet);
      CHECK(foundX);
    }
  }
}
