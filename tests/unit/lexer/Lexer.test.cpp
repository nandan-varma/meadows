#include "lexer/Lexer.h"
#include "catch_amalgamated.hpp"

TEST_CASE("Lexer tokenizes keywords correctly", "[lexer]") {
  SECTION("All keywords are recognized") {
    Lexer lexer("let func if else for while return print in range");
    auto tokens = lexer.tokenize();

    REQUIRE(tokens.size() == 11); // 10 keywords + EOF
    CHECK(tokens[0].type == TokenType::LET);
    CHECK(tokens[1].type == TokenType::FUNC);
    CHECK(tokens[2].type == TokenType::IF);
    CHECK(tokens[3].type == TokenType::ELSE);
    CHECK(tokens[4].type == TokenType::FOR);
    CHECK(tokens[5].type == TokenType::WHILE);
    CHECK(tokens[6].type == TokenType::RETURN);
    CHECK(tokens[7].type == TokenType::PRINT);
    CHECK(tokens[8].type == TokenType::IN);
    CHECK(tokens[9].type == TokenType::RANGE);
    CHECK(tokens[10].type == TokenType::EOF_TOKEN);
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
  REQUIRE_THROWS_AS(lexer.tokenize(), std::runtime_error);
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
    Lexer lexer("let x = 5; # this is a comment\nprint x;");
    auto tokens = lexer.tokenize();

    // Should tokenize without including the comment
    bool foundPrint = false;
    for (const auto &token : tokens) {
      if (token.type == TokenType::PRINT) {
        foundPrint = true;
        break;
      }
    }
    CHECK(foundPrint);
  }

  SECTION("Single-line comment with //") {
    Lexer lexer("let x = 5; // this is a comment\nprint x;");
    auto tokens = lexer.tokenize();

    bool foundPrint = false;
    for (const auto &token : tokens) {
      if (token.type == TokenType::PRINT) {
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
