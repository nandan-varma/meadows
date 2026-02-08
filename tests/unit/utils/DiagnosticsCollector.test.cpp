#include "utils/DiagnosticsCollector.h"
#include "catch_amalgamated.hpp"

using namespace meadows;

TEST_CASE("DiagnosticsCollector basic construction", "[diagnostics]") {
  SECTION("Default constructor") {
    DiagnosticsCollector collector;
    REQUIRE(collector.count() == 0);
    REQUIRE(collector.hasErrors() == false);
    REQUIRE(collector.hasFatals() == false);
    REQUIRE(collector.errorCount() == 0);
    REQUIRE(collector.warningCount() == 0);
  }

  SECTION("Constructor with handler") {
    bool handlerCalled = false;
    auto handler = [&handlerCalled](const Diagnostic &) {
      handlerCalled = true;
    };
    DiagnosticsCollector collector(handler);
    REQUIRE(collector.count() == 0);
  }
}

TEST_CASE("DiagnosticsCollector reportError", "[diagnostics]") {
  DiagnosticsCollector collector;
  SourceLocation loc("test.ms", 10, 5);

  SECTION("Report single error") {
    collector.reportError(ErrorCode::PARSE_UNEXPECTED_TOKEN, "Unexpected token",
                          loc);

    REQUIRE(collector.count() == 1);
    REQUIRE(collector.hasErrors() == true);
    REQUIRE(collector.errorCount() == 1);
    REQUIRE(collector.warningCount() == 0);
  }

  SECTION("Multiple errors") {
    collector.reportError(ErrorCode::PARSE_UNEXPECTED_TOKEN, "Error 1", loc);
    collector.reportError(ErrorCode::LEX_UNTERMINATED_STRING, "Error 2", loc);

    REQUIRE(collector.count() == 2);
    REQUIRE(collector.hasErrors() == true);
    REQUIRE(collector.errorCount() == 2);
  }
}

TEST_CASE("DiagnosticsCollector reportWarning", "[diagnostics]") {
  DiagnosticsCollector collector;
  SourceLocation loc("test.ms", 10, 5);

  SECTION("Report single warning") {
    collector.reportWarning(ErrorCode::WARN_UNUSED_VARIABLE, "Unused variable",
                            loc);

    REQUIRE(collector.count() == 1);
    REQUIRE(collector.hasErrors() == false);
    REQUIRE(collector.warningCount() == 1);
    REQUIRE(collector.errorCount() == 0);
  }

  SECTION("Multiple warnings") {
    collector.reportWarning(ErrorCode::WARN_UNUSED_VARIABLE, "Warning 1", loc);
    collector.reportWarning(ErrorCode::WARN_UNREACHABLE_CODE, "Warning 2", loc);

    REQUIRE(collector.count() == 2);
    REQUIRE(collector.warningCount() == 2);
  }
}

TEST_CASE("DiagnosticsCollector report (direct)", "[diagnostics]") {
  DiagnosticsCollector collector;
  SourceLocation loc("test.ms", 10, 5);
  Diagnostic diag(ErrorCode::PARSE_UNEXPECTED_TOKEN, "Test message", loc);

  SECTION("Report Diagnostic directly") {
    collector.report(diag);

    REQUIRE(collector.count() == 1);
    REQUIRE(collector.hasErrors() == true);
  }
}

TEST_CASE("DiagnosticsCollector mixed errors and warnings", "[diagnostics]") {
  DiagnosticsCollector collector;
  SourceLocation loc("test.ms", 10, 5);

  SECTION("Error then warning") {
    collector.reportError(ErrorCode::PARSE_UNEXPECTED_TOKEN, "Error", loc);
    collector.reportWarning(ErrorCode::WARN_UNUSED_VARIABLE, "Warning", loc);

    REQUIRE(collector.count() == 2);
    REQUIRE(collector.errorCount() == 1);
    REQUIRE(collector.warningCount() == 1);
    REQUIRE(collector.hasErrors() == true);
  }

  SECTION("Warning then error") {
    collector.reportWarning(ErrorCode::WARN_UNUSED_VARIABLE, "Warning", loc);
    collector.reportError(ErrorCode::PARSE_UNEXPECTED_TOKEN, "Error", loc);

    REQUIRE(collector.count() == 2);
    REQUIRE(collector.errorCount() == 1);
    REQUIRE(collector.warningCount() == 1);
  }
}

TEST_CASE("DiagnosticsCollector hasFatals", "[diagnostics]") {
  DiagnosticsCollector collector;
  SourceLocation loc("test.ms", 10, 5);

  SECTION("Regular errors not fatals") {
    collector.reportError(ErrorCode::PARSE_UNEXPECTED_TOKEN, "Error", loc);
    REQUIRE(collector.hasFatals() == false);
  }

  SECTION("System errors are fatals") {
    collector.reportError(ErrorCode::SYS_FILE_NOT_FOUND, "File not found", loc);
    REQUIRE(collector.hasFatals() == true);
  }

  SECTION("Mixed") {
    collector.reportError(ErrorCode::PARSE_UNEXPECTED_TOKEN, "Parse error",
                          loc);
    collector.reportError(ErrorCode::SYS_FILE_NOT_FOUND, "System error", loc);

    REQUIRE(collector.hasErrors() == true);
    REQUIRE(collector.hasFatals() == true);
  }
}

TEST_CASE("DiagnosticsCollector clear", "[diagnostics]") {
  DiagnosticsCollector collector;
  SourceLocation loc("test.ms", 10, 5);

  SECTION("Clear after errors") {
    collector.reportError(ErrorCode::PARSE_UNEXPECTED_TOKEN, "Error", loc);
    collector.reportWarning(ErrorCode::WARN_UNUSED_VARIABLE, "Warning", loc);

    REQUIRE(collector.count() == 2);
    REQUIRE(collector.hasErrors() == true);
    REQUIRE(collector.hasFatals() == false);

    collector.clear();

    REQUIRE(collector.count() == 0);
    REQUIRE(collector.hasErrors() == false);
    REQUIRE(collector.hasFatals() == false);
    REQUIRE(collector.errorCount() == 0);
    REQUIRE(collector.warningCount() == 0);
  }

  SECTION("Clear twice is safe") {
    collector.reportError(ErrorCode::PARSE_UNEXPECTED_TOKEN, "Error", loc);
    collector.clear();
    collector.clear();

    REQUIRE(collector.count() == 0);
  }
}

TEST_CASE("DiagnosticsCollector atLimit", "[diagnostics]") {
  DiagnosticsCollector collector;
  SourceLocation loc("test.ms", 10, 5);

  SECTION("Below limit") {
    for (size_t i = 0; i < 50; i++) {
      collector.reportError(ErrorCode::PARSE_UNEXPECTED_TOKEN, "Error", loc);
    }
    REQUIRE(collector.atLimit() == false);
  }

  SECTION("At 100 diagnostics limit") {
    for (size_t i = 0; i < 100; i++) {
      collector.reportError(ErrorCode::PARSE_UNEXPECTED_TOKEN, "Error", loc);
    }
    REQUIRE(collector.atLimit() == true);
  }
}

TEST_CASE("DiagnosticsCollector last", "[diagnostics]") {
  DiagnosticsCollector collector;
  SourceLocation loc("test.ms", 10, 5);

  SECTION("Last returns nullptr when empty") {
    REQUIRE(collector.last() == nullptr);
  }

  SECTION("Last returns last diagnostic") {
    SourceLocation loc1("test1.ms", 1, 1);
    SourceLocation loc2("test2.ms", 2, 2);
    SourceLocation loc3("test3.ms", 3, 3);

    collector.reportError(ErrorCode::PARSE_UNEXPECTED_TOKEN, "First", loc1);
    collector.reportError(ErrorCode::LEX_UNTERMINATED_STRING, "Second", loc2);
    collector.reportError(ErrorCode::SEM_UNDEFINED_VARIABLE, "Third", loc3);

    const Diagnostic *last = collector.last();
    REQUIRE(last != nullptr);
    REQUIRE(last->message == "Third");
    REQUIRE(last->location.file == "test3.ms");
  }
}

TEST_CASE("DiagnosticsCollector diagnostics accessor", "[diagnostics]") {
  DiagnosticsCollector collector;
  SourceLocation loc("test.ms", 10, 5);

  SECTION("Const accessor returns reference") {
    collector.reportError(ErrorCode::PARSE_UNEXPECTED_TOKEN, "Error", loc);
    const auto &diags = collector.diagnostics();
    REQUIRE(diags.size() == 1);
  }

  SECTION("Non-const accessor allows modification") {
    collector.reportError(ErrorCode::PARSE_UNEXPECTED_TOKEN, "Error", loc);
    auto &diags = collector.diagnostics();
    REQUIRE(diags.size() == 1);
  }
}

TEST_CASE("DiagnosticsCollector throwIfErrors", "[diagnostics]") {
  SECTION("Does not throw when no errors") {
    DiagnosticsCollector collector;
    REQUIRE_NOTHROW(collector.throwIfErrors());
  }

  SECTION("Throws when errors exist") {
    DiagnosticsCollector collector;
    SourceLocation loc("test.ms", 10, 5);
    collector.reportError(ErrorCode::PARSE_UNEXPECTED_TOKEN, "Error", loc);

    REQUIRE_THROWS_AS(collector.throwIfErrors(), MeadowsException);
  }

  SECTION("Throws MeadowsException with correct code") {
    DiagnosticsCollector collector;
    SourceLocation loc("test.ms", 10, 5);
    collector.reportError(ErrorCode::PARSE_UNEXPECTED_TOKEN, "Test error", loc);

    try {
      collector.throwIfErrors();
      REQUIRE(false); // Should not reach here
    } catch (const MeadowsException &e) {
      REQUIRE(e.code() == ErrorCode::PARSE_UNEXPECTED_TOKEN);
    }
  }
}

TEST_CASE("DiagnosticsCollector handler callback", "[diagnostics]") {
  int callCount = 0;
  Diagnostic lastDiag(ErrorCode::SUCCESS, "", SourceLocation("", 0, 0));

  auto handler = [&callCount, &lastDiag](const Diagnostic &diag) {
    callCount++;
    lastDiag = diag;
  };

  DiagnosticsCollector collector(handler);
  SourceLocation loc("test.ms", 10, 5);

  SECTION("Handler called on reportError") {
    collector.reportError(ErrorCode::PARSE_UNEXPECTED_TOKEN, "Error", loc);
    REQUIRE(callCount == 1);
    REQUIRE(lastDiag.message == "Error");
  }

  SECTION("Handler called on reportWarning") {
    collector.reportWarning(ErrorCode::WARN_UNUSED_VARIABLE, "Warning", loc);
    REQUIRE(callCount == 1);
    REQUIRE(lastDiag.message == "Warning");
  }

  SECTION("Handler called on report") {
    Diagnostic diag(ErrorCode::SEM_UNDEFINED_VARIABLE, "Direct", loc);
    collector.report(diag);
    REQUIRE(callCount == 1);
    REQUIRE(lastDiag.message == "Direct");
  }
}

TEST_CASE("DiagnosticSuppressor RAII", "[diagnostics]") {
  DiagnosticsCollector collector;
  SourceLocation loc("test.ms", 10, 5);

  SECTION("Constructor suppresses diagnostics") {
    collector.reportError(ErrorCode::PARSE_UNEXPECTED_TOKEN, "Before", loc);

    {
      DiagnosticSuppressor suppressor(collector);
      REQUIRE(collector.count() == 0);
      collector.reportError(ErrorCode::LEX_UNTERMINATED_STRING, "During", loc);
      REQUIRE(collector.count() == 1);
    }

    REQUIRE(collector.count() == 1);
  }

  SECTION("restore() restores diagnostics") {
    collector.reportError(ErrorCode::PARSE_UNEXPECTED_TOKEN, "Before", loc);

    {
      DiagnosticSuppressor suppressor(collector);
      collector.reportError(ErrorCode::LEX_UNTERMINATED_STRING, "During", loc);
      suppressor.restore();
      REQUIRE(collector.count() == 1);
      REQUIRE(collector.last()->message == "Before");
    }

    REQUIRE(collector.count() == 1);
  }

  SECTION("suppressed() returns suppressed diagnostics") {
    collector.reportError(ErrorCode::PARSE_UNEXPECTED_TOKEN, "Before", loc);

    {
      DiagnosticSuppressor suppressor(collector);
      collector.reportError(ErrorCode::LEX_UNTERMINATED_STRING, "During", loc);
      const auto &suppressed = suppressor.suppressed();
      REQUIRE(suppressed.size() == 1);
      REQUIRE(suppressed[0].message == "During");
    }
  }
}

TEST_CASE("Diagnostic limit behavior", "[diagnostics]") {
  DiagnosticsCollector collector;
  SourceLocation loc("test.ms", 10, 5);

  SECTION("Can report exactly 100 diagnostics") {
    for (size_t i = 0; i < 100; i++) {
      collector.reportError(ErrorCode::PARSE_UNEXPECTED_TOKEN, "Error", loc);
    }
    REQUIRE(collector.count() == 100);
    REQUIRE(collector.atLimit() == true);
  }

  SECTION("Diagnostics beyond limit are dropped") {
    for (size_t i = 0; i < 105; i++) {
      collector.reportError(ErrorCode::PARSE_UNEXPECTED_TOKEN, "Error", loc);
    }
    REQUIRE(collector.count() == 100);
  }
}

TEST_CASE("Edge cases", "[diagnostics]") {
  DiagnosticsCollector collector;

  SECTION("Empty file in source location") {
    SourceLocation loc("", 1, 1);
    collector.reportError(ErrorCode::PARSE_UNEXPECTED_TOKEN, "Error", loc);
    REQUIRE(collector.last()->location.file == "");
  }

  SECTION("Zero line in location") {
    SourceLocation loc("test.ms", 0, 1);
    collector.reportError(ErrorCode::PARSE_UNEXPECTED_TOKEN, "Error", loc);
    REQUIRE(collector.last()->location.line == 0);
  }

  SECTION("Zero column in location") {
    SourceLocation loc("test.ms", 1, 0);
    collector.reportError(ErrorCode::PARSE_UNEXPECTED_TOKEN, "Error", loc);
    REQUIRE(collector.last()->location.column == 0);
  }
}
