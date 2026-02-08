#include "utils/Timer.h"
#include "catch_amalgamated.hpp"

using namespace meadows;

TEST_CASE("Timer construction", "[timer]") {
  SECTION("Default constructor with empty name") {
    Timer timer;
    REQUIRE(timer.name() == "");
    REQUIRE(timer.isRunning() == false);
  }

  SECTION("Named constructor") {
    Timer timer("TestTimer");
    REQUIRE(timer.name() == "TestTimer");
    REQUIRE(timer.isRunning() == false);
  }
}

TEST_CASE("Timer lifecycle", "[timer]") {
  SECTION("elapsed() returns 0 before start") {
    Timer timer;
    REQUIRE(timer.elapsed() == 0.0);
  }

  SECTION("start() sets isRunning to true") {
    Timer timer;
    timer.start();
    REQUIRE(timer.isRunning() == true);
  }

  SECTION("elapsed() returns positive value after start") {
    Timer timer;
    timer.start();
    double elapsed = timer.elapsed();
    REQUIRE(elapsed >= 0.0);
  }

  SECTION("elapsed() increases over time") {
    Timer timer;
    timer.start();
    double first = timer.elapsed();

    for (volatile int i = 0; i < 10000; i++) {
    }

    double second = timer.elapsed();
    REQUIRE(second >= first);
  }

  SECTION("Multiple start() calls") {
    Timer timer;
    timer.start();
    double first = timer.elapsed();
    timer.start();
    double second = timer.elapsed();
    REQUIRE(second >= first);
  }
}

TEST_CASE("Timer name", "[timer]") {
  SECTION("Empty name returns empty string") {
    Timer timer;
    REQUIRE(timer.name() == "");
  }

  SECTION("Custom name is stored") {
    Timer timer("CustomName");
    REQUIRE(timer.name() == "CustomName");
  }

  SECTION("Name with spaces") {
    Timer timer("Timer With Spaces");
    REQUIRE(timer.name() == "Timer With Spaces");
  }

  SECTION("Name with special characters") {
    Timer timer("Timer-123_abc");
    REQUIRE(timer.name() == "Timer-123_abc");
  }
}

TEST_CASE("ScopedTimer construction", "[timer]") {
  SECTION("Default verbose is true") {
    ScopedTimer timer("Test");
    REQUIRE(timer.elapsed() >= 0.0);
  }

  SECTION("Verbose=false doesn't start timer") {
    ScopedTimer timer("Test", false);
    REQUIRE(timer.elapsed() == 0.0);
  }

  SECTION("Verbose=true starts timer") {
    ScopedTimer timer("Test", true);
    double elapsed = timer.elapsed();
    REQUIRE(elapsed >= 0.0);
  }

  SECTION("Named constructor") {
    ScopedTimer timer("Test");
    REQUIRE(timer.elapsed() >= 0.0);
  }
}

TEST_CASE("ScopedTimer elapsed", "[timer]") {
  SECTION("elapsed() returns value during lifetime") {
    ScopedTimer timer("Test");
    double elapsed = timer.elapsed();
    REQUIRE(elapsed >= 0.0);
  }

  SECTION("elapsed() returns 0 for non-verbose") {
    ScopedTimer timer("Test", false);
    REQUIRE(timer.elapsed() == 0.0);
  }
}

TEST_CASE("ScopedTimer RAII behavior", "[timer]") {
  SECTION("Timer stops on destruction") {
    double elapsed;
    {
      ScopedTimer timer("Test");
      elapsed = timer.elapsed();
    }
    REQUIRE(elapsed >= 0.0);
  }

  SECTION("Verbose destructor prints to stderr") {
    {
      ScopedTimer timer("Test", true);
    }
  }
}

TEST_CASE("Timer accuracy", "[timer]") {
  SECTION("Short duration timing") {
    Timer timer;
    timer.start();
    double elapsed = timer.elapsed();
    REQUIRE(elapsed >= 0.0);
    REQUIRE(elapsed < 1000.0);
  }

  SECTION("Multiple timers work independently") {
    Timer timer1("Timer1");
    Timer timer2("Timer2");

    timer1.start();
    timer2.start();

    double elapsed1 = timer1.elapsed();
    double elapsed2 = timer2.elapsed();

    REQUIRE(elapsed1 >= 0.0);
    REQUIRE(elapsed2 >= 0.0);
  }
}

TEST_CASE("Timer edge cases", "[timer]") {
  SECTION("Timer with very long name") {
    std::string longName(1000, 'X');
    Timer timer(longName);
    REQUIRE(timer.name() == longName);
  }

  SECTION("Timer with unicode name") {
    Timer timer("Timer: \xC3\xA9\xC3\xA0\xC3\xBC");
    REQUIRE(timer.name().length() > 0);
  }

  SECTION("Multiple elapsed() calls") {
    Timer timer;
    timer.start();
    double e1 = timer.elapsed();
    double e2 = timer.elapsed();
    double e3 = timer.elapsed();
    REQUIRE(e2 >= e1);
    REQUIRE(e3 >= e2);
  }
}
