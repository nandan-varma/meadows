#include "utils/WarningManager.h"
#include <catch2/catch_all.hpp>

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
  REQUIRE(wm.isEnabled(ErrorCode::WARN_UNUSED_VARIABLE) == false);
}

TEST_CASE("WarningManager setLevel DEFAULT", "[warnings]") {
  WarningManager wm;
  wm.setLevel(WarningManager::Level::DEFAULT);
  REQUIRE(wm.isEnabled(ErrorCode::WARN_UNUSED_VARIABLE) == true);
  REQUIRE(wm.isEnabled(ErrorCode::WARN_DIVISION_BY_ZERO) == true);
  REQUIRE(wm.isEnabled(ErrorCode::WARN_SHADOWING_VARIABLE) == false);
}

TEST_CASE("WarningManager setLevel ALL", "[warnings]") {
  WarningManager wm;
  wm.setLevel(WarningManager::Level::ALL);
  REQUIRE(wm.isEnabled(ErrorCode::WARN_UNUSED_VARIABLE) == true);
  REQUIRE(wm.isEnabled(ErrorCode::WARN_SHADOWING_VARIABLE) == true);
  REQUIRE(wm.isEnabled(ErrorCode::WARN_UNUSED_FUNCTION) == true);
}

TEST_CASE("WarningManager setLevel EXTRA", "[warnings]") {
  WarningManager wm;
  wm.setLevel(WarningManager::Level::EXTRA);
  REQUIRE(wm.isEnabled(ErrorCode::WARN_UNUSED_VARIABLE) == true);
  REQUIRE(wm.isEnabled(ErrorCode::WARN_ARRAY_BOUNDS) == true);
}

TEST_CASE("WarningManager enableWarning", "[warnings]") {
  WarningManager wm;
  wm.setLevel(WarningManager::Level::OFF);

  SECTION("Enable specific warning when level is OFF") {
    wm.enableWarning(ErrorCode::WARN_UNUSED_VARIABLE);
    REQUIRE(wm.isEnabled(ErrorCode::WARN_UNUSED_VARIABLE) == true);
    REQUIRE(wm.isEnabled(ErrorCode::WARN_UNREACHABLE_CODE) == false);
  }

  SECTION("Enable warning that was disabled") {
    wm.disableWarning(ErrorCode::WARN_UNUSED_VARIABLE);
    wm.enableWarning(ErrorCode::WARN_UNUSED_VARIABLE);
    REQUIRE(wm.isEnabled(ErrorCode::WARN_UNUSED_VARIABLE) == true);
  }
}

TEST_CASE("WarningManager disableWarning", "[warnings]") {
  WarningManager wm;
  wm.setLevel(WarningManager::Level::DEFAULT);
  REQUIRE(wm.isEnabled(ErrorCode::WARN_UNUSED_VARIABLE) == true);

  wm.disableWarning(ErrorCode::WARN_UNUSED_VARIABLE);
  REQUIRE(wm.isEnabled(ErrorCode::WARN_UNUSED_VARIABLE) == false);
}

TEST_CASE("WarningManager isEnabled priority", "[warnings]") {
  SECTION("Explicit disable overrides level") {
    WarningManager wm;
    wm.setLevel(WarningManager::Level::ALL);
    wm.disableWarning(ErrorCode::WARN_UNUSED_VARIABLE);
    REQUIRE(wm.isEnabled(ErrorCode::WARN_UNUSED_VARIABLE) == false);
  }

  SECTION("Explicit enable overrides level") {
    WarningManager wm;
    wm.setLevel(WarningManager::Level::OFF);
    wm.enableWarning(ErrorCode::WARN_UNUSED_VARIABLE);
    REQUIRE(wm.isEnabled(ErrorCode::WARN_UNUSED_VARIABLE) == true);
  }
}

TEST_CASE("WarningManager treatAsErrors", "[warnings]") {
  WarningManager wm;
  REQUIRE(wm.treatAsErrors() == false);
  wm.setTreatAsErrors(true);
  REQUIRE(wm.treatAsErrors() == true);
  wm.setTreatAsErrors(false);
  REQUIRE(wm.treatAsErrors() == false);
}

TEST_CASE("WarningManager levelToString", "[warnings]") {
  REQUIRE(WarningManager::levelToString(WarningManager::Level::OFF) == "off");
  REQUIRE(WarningManager::levelToString(WarningManager::Level::DEFAULT) == "default");
  REQUIRE(WarningManager::levelToString(WarningManager::Level::ALL) == "all");
  REQUIRE(WarningManager::levelToString(WarningManager::Level::EXTRA) == "extra");
}
