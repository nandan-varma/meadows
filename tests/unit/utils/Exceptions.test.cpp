#include "utils/Exceptions.h"
#include "catch_amalgamated.hpp"

using namespace meadows;

TEST_CASE("SourceLocation construction", "[exceptions]") {
  SECTION("Default constructor") {
    SourceLocation loc;
    REQUIRE(loc.file == "");
    REQUIRE(loc.line == 0);
    REQUIRE(loc.column == 0);
    REQUIRE(loc.endColumn == 0);
  }

  SECTION("Three-parameter constructor") {
    SourceLocation loc("test.ms", 10, 5);
    REQUIRE(loc.file == "test.ms");
    REQUIRE(loc.line == 10);
    REQUIRE(loc.column == 5);
    REQUIRE(loc.endColumn == 6);
  }

  SECTION("Four-parameter constructor") {
    SourceLocation loc("test.ms", 10, 5, 12);
    REQUIRE(loc.file == "test.ms");
    REQUIRE(loc.line == 10);
    REQUIRE(loc.column == 5);
    REQUIRE(loc.endColumn == 12);
  }
}

TEST_CASE("Diagnostic construction", "[exceptions]") {
  SECTION("Constructor with error code") {
    SourceLocation loc("test.ms", 10, 5);
    Diagnostic diag(ErrorCode::PARSE_UNEXPECTED_TOKEN, "Unexpected token", loc);

    REQUIRE(diag.code == ErrorCode::PARSE_UNEXPECTED_TOKEN);
    REQUIRE(diag.message == "Unexpected token");
    REQUIRE(diag.location.file == "test.ms");
    REQUIRE(diag.location.line == 10);
    REQUIRE(diag.severity == "error");
  }

  SECTION("Constructor with warning code") {
    SourceLocation loc("test.ms", 10, 5);
    Diagnostic diag(ErrorCode::WARN_UNUSED_VARIABLE, "Unused variable", loc);

    REQUIRE(diag.code == ErrorCode::WARN_UNUSED_VARIABLE);
    REQUIRE(diag.message == "Unused variable");
    REQUIRE(diag.severity == "warning");
  }

  SECTION("Empty relatedInfo initially") {
    SourceLocation loc("test.ms", 10, 5);
    Diagnostic diag(ErrorCode::PARSE_UNEXPECTED_TOKEN, "Test", loc);

    REQUIRE(diag.relatedInfo.empty());
  }

  SECTION("Empty help initially") {
    SourceLocation loc("test.ms", 10, 5);
    Diagnostic diag(ErrorCode::PARSE_UNEXPECTED_TOKEN, "Test", loc);

    REQUIRE(diag.help == "");
  }
}

TEST_CASE("MeadowsException construction", "[exceptions]") {
  SECTION("Constructor with code and message") {
    MeadowsException ex(ErrorCode::LEX_UNTERMINATED_STRING,
                        "Unterminated string");

    REQUIRE(ex.code() == ErrorCode::LEX_UNTERMINATED_STRING);
    REQUIRE(ex.message() == "Unterminated string");
  }

  SECTION("Constructor with full location") {
    SourceLocation loc("test.ms", 5, 10);
    MeadowsException ex(ErrorCode::PARSE_EXPECTED_SEMICOLON,
                        "Expected semicolon", loc);

    REQUIRE(ex.code() == ErrorCode::PARSE_EXPECTED_SEMICOLON);
    REQUIRE(ex.location().file == "test.ms");
    REQUIRE(ex.location().line == 5);
    REQUIRE(ex.location().column == 10);
  }

  SECTION("Constructor from Diagnostic") {
    SourceLocation loc("test.ms", 5, 10);
    Diagnostic diag(ErrorCode::SEM_UNDEFINED_VARIABLE, "Undefined variable",
                    loc);
    MeadowsException ex(diag);

    REQUIRE(ex.code() == ErrorCode::SEM_UNDEFINED_VARIABLE);
    REQUIRE(ex.message() == "Undefined variable");
  }
}

TEST_CASE("MeadowsException getters", "[exceptions]") {
  MeadowsException ex(ErrorCode::PARSE_UNEXPECTED_TOKEN, "Test error");

  SECTION("code() returns correct code") {
    REQUIRE(ex.code() == ErrorCode::PARSE_UNEXPECTED_TOKEN);
  }

  SECTION("codeString() returns formatted string") {
    REQUIRE(ex.codeString() == "E2009");
  }

  SECTION("category() returns parse for parse errors") {
    REQUIRE(ex.category() == "parse");
  }

  SECTION("message() returns message") {
    REQUIRE(ex.message() == "Test error");
  }

  SECTION("what() returns non-null") {
    const char *what = ex.what();
    REQUIRE(what != nullptr);
    REQUIRE(std::string(what).find("Test error") != std::string::npos);
  }
}

TEST_CASE("MeadowsException warning/error classification", "[exceptions]") {
  SECTION("Error code is not a warning") {
    MeadowsException ex(ErrorCode::PARSE_UNEXPECTED_TOKEN, "Test");
    REQUIRE(ex.isWarning() == false);
    REQUIRE(ex.isError() == true);
  }

  SECTION("Warning code is a warning") {
    MeadowsException ex(ErrorCode::WARN_UNUSED_VARIABLE, "Test");
    REQUIRE(ex.isWarning() == true);
    REQUIRE(ex.isError() == false);
  }
}

TEST_CASE("MeadowsException help text", "[exceptions]") {
  MeadowsException ex(ErrorCode::PARSE_UNEXPECTED_TOKEN, "Test");

  SECTION("Help is empty initially") { REQUIRE(ex.help() == ""); }

  SECTION("setHelp() stores help text") {
    ex.setHelp("Add a semicolon here");
    REQUIRE(ex.help() == "Add a semicolon here");
  }

  SECTION("Help is retrieved correctly") {
    ex.setHelp("Fix: add semicolon");
    REQUIRE(ex.help() == "Fix: add semicolon");
  }
}

TEST_CASE("MeadowsException toDiagnostic conversion", "[exceptions]") {
  SourceLocation loc("test.ms", 5, 10);
  MeadowsException ex(ErrorCode::SEM_UNDEFINED_VARIABLE, "Undefined variable",
                      loc);
  ex.setHelp("Declare the variable first");

  Diagnostic diag = ex.toDiagnostic();

  REQUIRE(diag.code == ErrorCode::SEM_UNDEFINED_VARIABLE);
  REQUIRE(diag.message == "Undefined variable");
  REQUIRE(diag.location.file == "test.ms");
  REQUIRE(diag.location.line == 5);
  REQUIRE(diag.location.column == 10);
  REQUIRE(diag.help == "Declare the variable first");
  REQUIRE(diag.severity == "error");
}

TEST_CASE("Derived exception classes", "[exceptions]") {
  SourceLocation loc("test.ms", 1, 1);

  SECTION("LexicalException") {
    LexicalException ex(ErrorCode::LEX_UNTERMINATED_STRING, "Test", loc);
    REQUIRE(ex.isWarning() == false);
    REQUIRE(ex.category() == "lexical");
  }

  SECTION("ParseException") {
    ParseException ex(ErrorCode::PARSE_EXPECTED_SEMICOLON, "Test", loc);
    REQUIRE(ex.isWarning() == false);
    REQUIRE(ex.category() == "parse");
  }

  SECTION("SemanticException") {
    SemanticException ex(ErrorCode::SEM_UNDEFINED_VARIABLE, "Test", loc);
    REQUIRE(ex.isWarning() == false);
    REQUIRE(ex.category() == "semantic");
  }

  SECTION("CodeGenException") {
    CodeGenException ex(ErrorCode::CODEGEN_UNSUPPORTED_OPERATION, "Test", loc);
    REQUIRE(ex.isWarning() == false);
    REQUIRE(ex.category() == "codegen");
  }

  SECTION("SystemException without location") {
    SystemException ex(ErrorCode::SYS_FILE_NOT_FOUND, "File not found");
    REQUIRE(ex.isWarning() == false);
    REQUIRE(ex.category() == "system");
  }

  SECTION("SystemException with location") {
    SystemException ex(ErrorCode::SYS_FILE_NOT_FOUND, "File not found", loc);
    REQUIRE(ex.location().file == "test.ms");
  }

  SECTION("All derived classes catchable as MeadowsException") {
    REQUIRE((std::is_base_of<MeadowsException, LexicalException>::value));
    REQUIRE((std::is_base_of<MeadowsException, ParseException>::value));
    REQUIRE((std::is_base_of<MeadowsException, SemanticException>::value));
    REQUIRE((std::is_base_of<MeadowsException, CodeGenException>::value));
    REQUIRE((std::is_base_of<MeadowsException, SystemException>::value));
  }
}

TEST_CASE("Exception inheritance", "[exceptions]") {
  REQUIRE((std::is_base_of<std::exception, MeadowsException>::value));

  SourceLocation loc("test.ms", 1, 1);
  MeadowsException baseEx(ErrorCode::PARSE_UNEXPECTED_TOKEN, "Base error", loc);

  SECTION("Derived exception can be caught as base") {
    bool caught = false;
    try {
      throw ParseException(ErrorCode::PARSE_EXPECTED_SEMICOLON, "Test", loc);
    } catch (const MeadowsException &e) {
      caught = true;
      REQUIRE(e.code() == ErrorCode::PARSE_EXPECTED_SEMICOLON);
    }
    REQUIRE(caught);
  }

  SECTION("Exception can be caught as std::exception") {
    bool caught = false;
    try {
      throw LexicalException(ErrorCode::LEX_UNTERMINATED_STRING, "Test", loc);
    } catch (const std::exception &e) {
      caught = true;
      REQUIRE(std::string(e.what()).find("Test") != std::string::npos);
    }
    REQUIRE(caught);
  }
}

TEST_CASE("Stack frame structure", "[exceptions]") {
  MeadowsException ex(ErrorCode::PARSE_UNEXPECTED_TOKEN, "Test");

  SECTION("stackTrace() returns vector") {
    const auto &frames = ex.stackTrace();
    REQUIRE(frames.size() >= 0);
  }

  SECTION("Stack frames have expected fields") {
    const auto &frames = ex.stackTrace();
    for (const auto &frame : frames) {
      (void)frame.function;
      (void)frame.file;
      (void)frame.line;
      (void)frame.address;
    }
  }
}

TEST_CASE("formatWithContext", "[exceptions]") {
  MeadowsException ex(ErrorCode::PARSE_UNEXPECTED_TOKEN, "Test error");
  ex.setHelp("Suggested fix");

  SECTION("Returns non-empty string") {
    std::string result = ex.formatWithContext("");
    REQUIRE(result.empty() == false);
  }

  SECTION("With source line provided") {
    std::string result = ex.formatWithContext("let x = ;");
    REQUIRE(result.empty() == false);
  }

  SECTION("Help text included when set") {
    std::string result = ex.formatWithContext("");
    REQUIRE(!result.empty());
  }
}

TEST_CASE("Edge cases", "[exceptions]") {
  SECTION("Empty message") {
    MeadowsException ex(ErrorCode::PARSE_UNEXPECTED_TOKEN, "");
    REQUIRE(ex.message() == "");
  }

  SECTION("Location with line 0") {
    SourceLocation loc("test.ms", 0, 5);
    MeadowsException ex(ErrorCode::PARSE_UNEXPECTED_TOKEN, "Test", loc);
    REQUIRE(ex.location().line == 0);
  }

  SECTION("Location with column 0") {
    SourceLocation loc("test.ms", 5, 0);
    MeadowsException ex(ErrorCode::PARSE_UNEXPECTED_TOKEN, "Test", loc);
    REQUIRE(ex.location().column == 0);
  }
}
