#include <catch2/catch_test_macros.hpp>

#include "lexer/Lexer.h"

using namespace meadows;

namespace {

std::vector<Token> lex(const std::string& source, DiagnosticSink& sink) {
  return Lexer(source, 0, sink).tokenize();
}

std::vector<TokenType> types(const std::string& source) {
  DiagnosticSink sink;
  std::vector<TokenType> result;
  for (const Token& token : lex(source, sink)) {
    result.push_back(token.type);
  }
  return result;
}

} // namespace

TEST_CASE("lexer classifies keywords, identifiers and literals", "[lexer]") {
  using T = TokenType;
  CHECK(types("let x = 1;") ==
        std::vector<T>{T::LET, T::IDENTIFIER, T::EQUAL, T::NUMBER,
                       T::SEMICOLON, T::END_OF_FILE});
  CHECK(types("func if else for while return in range true false break "
              "continue import") ==
        std::vector<T>{T::FUNC, T::IF, T::ELSE, T::FOR, T::WHILE, T::RETURN,
                       T::IN, T::RANGE, T::TRUE, T::FALSE, T::BREAK,
                       T::CONTINUE, T::IMPORT, T::END_OF_FILE});
  CHECK(types("letter _x x1") ==
        std::vector<T>{T::IDENTIFIER, T::IDENTIFIER, T::IDENTIFIER,
                       T::END_OF_FILE});
}

TEST_CASE("lexer recognises every operator and punctuation mark", "[lexer]") {
  using T = TokenType;
  CHECK(types("+ - * / % = == ! != < <= > >= && ||") ==
        std::vector<T>{T::PLUS, T::MINUS, T::STAR, T::SLASH, T::PERCENT,
                       T::EQUAL, T::EQUAL_EQUAL, T::BANG, T::BANG_EQUAL,
                       T::LESS, T::LESS_EQUAL, T::GREATER, T::GREATER_EQUAL,
                       T::AND, T::OR, T::END_OF_FILE});
  CHECK(types("( ) { } [ ] , : ; .") ==
        std::vector<T>{T::LEFT_PAREN, T::RIGHT_PAREN, T::LEFT_BRACE,
                       T::RIGHT_BRACE, T::LEFT_BRACKET, T::RIGHT_BRACKET,
                       T::COMMA, T::COLON, T::SEMICOLON, T::DOT,
                       T::END_OF_FILE});
}

TEST_CASE("lexer separates a decimal point from field access", "[lexer]") {
  DiagnosticSink sink;
  std::vector<Token> tokens = lex("1.5 a.b 3.x", sink);
  CHECK(tokens[0].text == "1.5");
  CHECK(tokens[1].type == TokenType::IDENTIFIER);
  CHECK(tokens[2].type == TokenType::DOT);
  CHECK(tokens[4].text == "3");
  CHECK(tokens[5].type == TokenType::DOT);
}

TEST_CASE("lexer decodes escape sequences", "[lexer]") {
  DiagnosticSink sink;
  std::vector<Token> tokens = lex(R"("a\n\t\\\"\r\0\b\f")", sink);
  CHECK_FALSE(sink.hasErrors());
  CHECK(tokens[0].text == std::string("a\n\t\\\"\r\0\b\f", 9));
}

TEST_CASE("lexer skips both comment styles", "[lexer]") {
  using T = TokenType;
  CHECK(types("# one\nlet // two\nx") ==
        std::vector<T>{T::LET, T::IDENTIFIER, T::END_OF_FILE});
}

TEST_CASE("lexer tracks line and column", "[lexer]") {
  DiagnosticSink sink;
  std::vector<Token> tokens = lex("let\n  name", sink);
  CHECK(tokens[0].location.line == 1);
  CHECK(tokens[0].location.column == 1);
  CHECK(tokens[1].location.line == 2);
  CHECK(tokens[1].location.column == 3);
  CHECK(tokens[1].length == 4);
}

TEST_CASE("lexer reports an unterminated string", "[lexer]") {
  DiagnosticSink sink;
  lex("let s = \"abc", sink);
  REQUIRE(sink.diagnostics().size() == 1);
  CHECK(sink.diagnostics()[0].code == ErrorCode::LEX_UNTERMINATED_STRING);
  CHECK(sink.diagnostics()[0].location.column == 9);
}

TEST_CASE("an unterminated string ends at the end of its line", "[lexer]") {
  DiagnosticSink sink;
  std::vector<Token> tokens = lex("\"abc\nlet", sink);
  CHECK(sink.errorCount() == 1);
  CHECK(tokens[1].type == TokenType::LET);
}

TEST_CASE("lexer reports invalid characters and keeps going", "[lexer]") {
  DiagnosticSink sink;
  std::vector<Token> tokens = lex("let @ x & y", sink);
  CHECK(sink.errorCount() == 2);
  CHECK(sink.diagnostics()[0].code == ErrorCode::LEX_INVALID_CHARACTER);
  CHECK(tokens.size() == 4);
}

TEST_CASE("lexer reports invalid escapes", "[lexer]") {
  DiagnosticSink sink;
  lex(R"("a\qb")", sink);
  REQUIRE(sink.errorCount() == 1);
  CHECK(sink.diagnostics()[0].code == ErrorCode::LEX_INVALID_ESCAPE);
}

TEST_CASE("lexer enforces the integer literal range", "[lexer]") {
  DiagnosticSink ok;
  lex("2147483647 2147483648", ok);
  CHECK_FALSE(ok.hasErrors());

  DiagnosticSink bad;
  lex("2147483649 99999999999", bad);
  CHECK(bad.errorCount() == 2);
  CHECK(bad.diagnostics()[0].code == ErrorCode::LEX_NUMBER_OUT_OF_RANGE);
}

TEST_CASE("lexer rejects oversized string literals", "[lexer]") {
  DiagnosticSink sink;
  lex("\"" + std::string(1048577, 'x') + "\"", sink);
  REQUIRE(sink.errorCount() == 1);
  CHECK(sink.diagnostics()[0].code == ErrorCode::LEX_STRING_TOO_LONG);
}
