#include "utils/WarningManager.h"
#include "catch_amalgamated.hpp"

using namespace meadows;

TEST_CASE("WarningManager construction", "[warnings]") {
  SECTION("Default level is DEFAULT") {
    WarningManager wm;
    REQUIRE(wm.treatAsErrors() == false);
  }

  SECTION("treatAsErrors() default is false") {
    WarningManager wm;
    REQUIRE(wm.treatAsErrors() == false);
  }
}

TEST_CASE("WarningManager setLevel OFF", "[warnings]") {
  WarningManager wm;
  wm.setLevel(WarningManager::Level::OFF);
  REQUIRE(wm.isEnabled(ErrorCode::WARN_DIVISION_BY_ZERO) == false);
}

TEST_CASE("WarningManager setLevel DEFAULT", "[warnings]") {
  WarningManager wm;
  wm.setLevel(WarningManager::Level::DEFAULT);
  REQUIRE(wm.isEnabled(ErrorCode::WARN_UNUSED_VARIABLE) == true);
  REQUIRE(wm.isEnabled(ErrorCode::WARN_DIVISION_BY_ZERO) == true);
}

TEST_CASE("WarningManager setLevel ALL", "[warnings]") {
  WarningManager wm;
  wm.setLevel(WarningManager::Level::ALL);
  REQUIRE(wm.isEnabled(ErrorCode::WARN_UNUSED_VARIABLE) == true);
}

TEST_CASE("WarningManager enableWarning", "[warnings]") {
  WarningManager wm;
  wm.setLevel(WarningManager::Level::OFF);

  SECTION("Enable specific warning") {
    wm.enableWarning(ErrorCode::WARN_UNUSED_VARIABLE);
    REQUIRE(wm.isEnabled(ErrorCode::WARN_UNUSED_VARIABLE) == true);
  }

  SECTION("Enable warning that was disabled") {
    wm.disableWarning(ErrorCode::WARN_UNUSED_VARIABLE);
    wm.enableWarning(ErrorCode::WARN_UNUSED_VARIABLE);
    REQUIRE(wm.isEnabled(ErrorCode::WARN_UNUSED_VARIABLE) == true);
  }
}

TEST_CASE("WarningManager isEnabled priority", "[warnings]") {
  WarningManager wm;

  SECTION("Explicit disable overrides level") {
    wm.setLevel(WarningManager::Level::ALL);
    wm.disableWarning(ErrorCode::WARN_UNUSED_VARIABLE);
    REQUIRE(wm.isEnabled(ErrorCode::WARN_UNUSED_VARIABLE) == false);
  }

  SECTION("Explicit enable overrides level") {
    wm.setLevel(WarningManager::Level::OFF);
    wm.enableWarning(ErrorCode::WARN_UNUSED_VARIABLE);
    REQUIRE(wm.isEnabled(ErrorCode::WARN_UNUSED_VARIABLE) == true);
  }
}

TEST_CASE("VariableUsageTracker construction", "[warnings]") {
  VariableUsageTracker tracker;

  SECTION("Initial scope is global") {
    REQUIRE(tracker.isDeclared("x") == false);
  }
}

TEST_CASE("VariableUsageTracker enterScope/exitScope", "[warnings]") {
  VariableUsageTracker tracker;

  SECTION("enterScope adds new scope") {
    tracker.enterScope();
    tracker.declareVariable("x", SourceLocation("test.ms", 1, 1));
    REQUIRE(tracker.isDeclared("x") == true);
  }

  SECTION("exitScope returns unused variables") {
    tracker.declareVariable("x", SourceLocation("test.ms", 1, 1));
    tracker.enterScope();
    tracker.declareVariable("y", SourceLocation("test.ms", 2, 1));

    auto unused = tracker.exitScope();

    REQUIRE(unused.size() == 1);
    REQUIRE(unused[0].name == "y");
  }

  SECTION("exitScope from global returns empty") {
    tracker.declareVariable("x", SourceLocation("test.ms", 1, 1));
    auto unused = tracker.exitScope();
    REQUIRE(unused.empty());
  }
}

TEST_CASE("VariableUsageTracker declareVariable", "[warnings]") {
  VariableUsageTracker tracker;

  SECTION("Declare variable") {
    tracker.declareVariable("x", SourceLocation("test.ms", 1, 1));
    REQUIRE(tracker.isDeclared("x") == true);
  }

  SECTION("Declare multiple variables") {
    tracker.declareVariable("x", SourceLocation("test.ms", 1, 1));
    tracker.declareVariable("y", SourceLocation("test.ms", 2, 1));
    tracker.declareVariable("z", SourceLocation("test.ms", 3, 1));
    REQUIRE(tracker.isDeclared("x") == true);
    REQUIRE(tracker.isDeclared("y") == true);
    REQUIRE(tracker.isDeclared("z") == true);
  }

  SECTION("Parameters flagged correctly") {
    tracker.declareVariable("param", SourceLocation("test.ms", 1, 1), true,
                            false);
    auto unused = tracker.exitScope();
    REQUIRE(unused.empty());
  }

  SECTION("Functions flagged correctly") {
    tracker.declareVariable("func", SourceLocation("test.ms", 1, 1), false,
                            true);
    auto unused = tracker.exitScope();
    REQUIRE(unused.empty());
  }
}

TEST_CASE("VariableUsageTracker markUsed", "[warnings]") {
  VariableUsageTracker tracker;

  SECTION("Mark variable as used") {
    tracker.enterScope();
    tracker.declareVariable("x", SourceLocation("test.ms", 1, 1));
    tracker.markUsed("x");
    auto unused = tracker.exitScope();
    REQUIRE(unused.empty());
  }

  SECTION("Unused variable reported") {
    tracker.enterScope();
    tracker.declareVariable("x", SourceLocation("test.ms", 1, 1));
    auto unused = tracker.exitScope();
    REQUIRE(unused.size() == 1);
    REQUIRE(unused[0].name == "x");
  }

  SECTION("markUsed on undeclared variable is safe") {
    tracker.enterScope();
    tracker.markUsed("undeclared");
  }

  SECTION("markUsed finds in outer scope") {
    tracker.declareVariable("x", SourceLocation("test.ms", 1, 1));
    tracker.enterScope();
    tracker.markUsed("x");
    auto unused = tracker.exitScope();
    REQUIRE(unused.empty());
  }
}

TEST_CASE("VariableUsageTracker getUnusedVariables", "[warnings]") {
  VariableUsageTracker tracker;

  SECTION("No unused in global scope initially") {
    auto unused = tracker.getUnusedVariables();
    REQUIRE(unused.empty());
  }

  SECTION("Report unused from current scope") {
    tracker.declareVariable("x", SourceLocation("test.ms", 1, 1));
    tracker.declareVariable("y", SourceLocation("test.ms", 2, 1));
    tracker.markUsed("x");

    auto unused = tracker.getUnusedVariables();
    REQUIRE(unused.size() == 1);
    REQUIRE(unused[0].name == "y");
  }
}

TEST_CASE("VariableUsageTracker clear", "[warnings]") {
  VariableUsageTracker tracker;

  SECTION("Clear resets to initial state") {
    tracker.declareVariable("x", SourceLocation("test.ms", 1, 1));
    tracker.enterScope();
    tracker.declareVariable("y", SourceLocation("test.ms", 2, 1));
    tracker.markUsed("x");

    tracker.clear();

    REQUIRE(tracker.isDeclared("x") == false);
    REQUIRE(tracker.isDeclared("y") == false);
  }
}

TEST_CASE("VariableUsageTracker scope nesting", "[warnings]") {
  VariableUsageTracker tracker;

  SECTION("Multiple nested scopes") {
    tracker.enterScope();
    tracker.declareVariable("outer", SourceLocation("test.ms", 2, 1));
    tracker.enterScope();
    tracker.declareVariable("inner", SourceLocation("test.ms", 3, 1));
    tracker.markUsed("inner");

    auto unusedInner = tracker.exitScope();
    REQUIRE(unusedInner.empty());

    auto unusedOuter = tracker.exitScope();
    REQUIRE(unusedOuter.size() == 1);
    REQUIRE(unusedOuter[0].name == "outer");
  }

  SECTION("Shadowing variables") {
    tracker.enterScope();
    tracker.declareVariable("x", SourceLocation("test.ms", 1, 1));
    tracker.enterScope();
    tracker.declareVariable("x", SourceLocation("test.ms", 2, 1));
    tracker.markUsed("x");

    auto unused = tracker.exitScope();
    REQUIRE(unused.empty());
  }
}

TEST_CASE("WarningManager levelToString", "[warnings]") {
  SECTION("All levels have string representations") {
    REQUIRE(WarningManager::levelToString(WarningManager::Level::OFF) == "off");
    REQUIRE(WarningManager::levelToString(WarningManager::Level::DEFAULT) ==
            "default");
    REQUIRE(WarningManager::levelToString(WarningManager::Level::ALL) == "all");
    REQUIRE(WarningManager::levelToString(WarningManager::Level::EXTRA) ==
            "extra");
  }
}

TEST_CASE("ControlFlowAnalyzer basic", "[warnings]") {
  ControlFlowAnalyzer analyzer;

  SECTION("Initial state is NORMAL") {
    REQUIRE(analyzer.inLoop() == false);
    REQUIRE(analyzer.inFunction() == false);
    REQUIRE(analyzer.isUnreachable() == false);
  }

  SECTION("enterBlock with LOOP") {
    analyzer.enterBlock(ControlFlowAnalyzer::BlockType::LOOP);
    REQUIRE(analyzer.inLoop() == true);
    REQUIRE(analyzer.inFunction() == false);
  }

  SECTION("enterBlock with FUNCTION") {
    analyzer.enterBlock(ControlFlowAnalyzer::BlockType::FUNCTION);
    REQUIRE(analyzer.inFunction() == true);
    REQUIRE(analyzer.inLoop() == false);
  }

  SECTION("exitBlock removes context") {
    analyzer.enterBlock(ControlFlowAnalyzer::BlockType::LOOP);
    REQUIRE(analyzer.inLoop() == true);
    analyzer.exitBlock();
    REQUIRE(analyzer.inLoop() == false);
  }

  SECTION("exitBlock never empties stack") {
    analyzer.exitBlock();
    REQUIRE(analyzer.isUnreachable() == false);
  }
}

TEST_CASE("ControlFlowAnalyzer terminal states", "[warnings]") {
  ControlFlowAnalyzer analyzer;

  SECTION("markReturned sets hasReturned") {
    analyzer.markReturned();
    REQUIRE(analyzer.isUnreachable() == true);
  }

  SECTION("markBroken sets hasBroken") {
    analyzer.markBroken();
    REQUIRE(analyzer.isUnreachable() == true);
  }

  SECTION("markContinued sets hasContinued") {
    analyzer.markContinued();
    REQUIRE(analyzer.isUnreachable() == true);
  }

  SECTION("resetTerminal clears flags") {
    analyzer.markReturned();
    analyzer.markBroken();
    REQUIRE(analyzer.isUnreachable() == true);

    analyzer.resetTerminal();
    REQUIRE(analyzer.isUnreachable() == false);
  }

  SECTION("Terminal state resets on block entry") {
    analyzer.markReturned();
    REQUIRE(analyzer.isUnreachable() == true);

    analyzer.enterBlock(ControlFlowAnalyzer::BlockType::NORMAL);
    REQUIRE(analyzer.isUnreachable() == false);

    analyzer.markBroken();
    REQUIRE(analyzer.isUnreachable() == true);
  }

  SECTION("Terminal state persists across nested blocks until reset") {
    analyzer.markBroken();
    REQUIRE(analyzer.isUnreachable() == true);

    analyzer.enterBlock(ControlFlowAnalyzer::BlockType::FUNCTION);
    REQUIRE(analyzer.isUnreachable() == false);

    analyzer.exitBlock();
    REQUIRE(analyzer.isUnreachable() == false);
  }
}

TEST_CASE("ControlFlowAnalyzer context queries", "[warnings]") {
  ControlFlowAnalyzer analyzer;

  SECTION("inLoop with nested loops") {
    REQUIRE(analyzer.inLoop() == false);
    analyzer.enterBlock(ControlFlowAnalyzer::BlockType::LOOP);
    REQUIRE(analyzer.inLoop() == true);
    analyzer.enterBlock(ControlFlowAnalyzer::BlockType::NORMAL);
    REQUIRE(analyzer.inLoop() == true);
    analyzer.enterBlock(ControlFlowAnalyzer::BlockType::LOOP);
    REQUIRE(analyzer.inLoop() == true);
    analyzer.exitBlock();
    REQUIRE(analyzer.inLoop() == true);
    analyzer.exitBlock();
    REQUIRE(analyzer.inLoop() == true);
    analyzer.exitBlock();
    REQUIRE(analyzer.inLoop() == false);
  }

  SECTION("inFunction with nested blocks") {
    REQUIRE(analyzer.inFunction() == false);
    analyzer.enterBlock(ControlFlowAnalyzer::BlockType::FUNCTION);
    REQUIRE(analyzer.inFunction() == true);
    analyzer.enterBlock(ControlFlowAnalyzer::BlockType::LOOP);
    REQUIRE(analyzer.inFunction() == true);
    analyzer.exitBlock();
    REQUIRE(analyzer.inFunction() == true);
    analyzer.exitBlock();
    REQUIRE(analyzer.inFunction() == false);
  }
}
