#include "config/Config.h"
#include "catch_amalgamated.hpp"
#include "config/TOMLParser.h"

using namespace meadows::config;

TEST_CASE("TOML Parser basic types", "[config]") {
  TOMLParser parser;

  SECTION("Parse string value") {
    TOMLValue root = parser.parse(R"(name = "test")");
    REQUIRE(parser.success());
    REQUIRE(root.tableValue["name"].asString() == "test");
  }

  SECTION("Parse integer value") {
    TOMLValue root = parser.parse(R"(count = 42)");
    REQUIRE(parser.success());
    REQUIRE(root.tableValue["count"].asInteger() == 42);
  }

  SECTION("Parse boolean value") {
    TOMLValue root = parser.parse(R"(enabled = true)");
    REQUIRE(parser.success());
    REQUIRE(root.tableValue["enabled"].asBoolean() == true);
  }

  SECTION("Parse false value") {
    TOMLValue root = parser.parse(R"(disabled = false)");
    REQUIRE(parser.success());
    REQUIRE(root.tableValue["disabled"].asBoolean() == false);
  }
}

TEST_CASE("TOML Parser tables", "[config]") {
  TOMLParser parser;

  SECTION("Parse simple table") {
    std::string toml = R"([project]
name = "myapp"
version = "1.0.0")";
    TOMLValue root = parser.parse(toml);
    REQUIRE(parser.success());
    REQUIRE(root.tableValue["project"].isTable());
    REQUIRE(root.tableValue["project"].tableValue["name"].asString() ==
            "myapp");
    REQUIRE(root.tableValue["project"].tableValue["version"].asString() ==
            "1.0.0");
  }

  SECTION("Parse nested table") {
    std::string toml = R"([project]
name = "test"

[build]
opt-level = 2)";
    TOMLValue root = parser.parse(toml);
    REQUIRE(parser.success());
    REQUIRE(root.tableValue["project"].tableValue["name"].asString() == "test");
    REQUIRE(root.tableValue["build"].tableValue["opt-level"].asInteger() == 2);
  }
}

TEST_CASE("TOML Parser arrays", "[config]") {
  TOMLParser parser;

  SECTION("Parse string array") {
    std::string toml = R"(flags = ["-Wall", "-Wextra"])";
    TOMLValue root = parser.parse(toml);
    REQUIRE(parser.success());
    REQUIRE(root.tableValue["flags"].isArray());
    REQUIRE(root.tableValue["flags"].arrayValue.size() == 2);
    REQUIRE(root.tableValue["flags"].arrayValue[0].asString() == "-Wall");
  }

  SECTION("Parse integer array") {
    std::string toml = R"(numbers = [1, 2, 3])";
    TOMLValue root = parser.parse(toml);
    REQUIRE(parser.success());
    REQUIRE(root.tableValue["numbers"].arrayValue.size() == 3);
    REQUIRE(root.tableValue["numbers"].arrayValue[1].asInteger() == 2);
  }

  SECTION("Parse empty array") {
    std::string toml = R"(empty = [])";
    TOMLValue root = parser.parse(toml);
    REQUIRE(parser.success());
    REQUIRE(root.tableValue["empty"].arrayValue.empty());
  }
}

TEST_CASE("TOML Parser inline tables", "[config]") {
  TOMLParser parser;

  SECTION("Parse inline table") {
    std::string toml = R"(point = { x = 1, y = 2 })";
    TOMLValue root = parser.parse(toml);
    REQUIRE(parser.success());
    REQUIRE(root.tableValue["point"].isTable());
    REQUIRE(root.tableValue["point"].tableValue["x"].asInteger() == 1);
    REQUIRE(root.tableValue["point"].tableValue["y"].asInteger() == 2);
  }
}

TEST_CASE("TOML Parser comments", "[config]") {
  TOMLParser parser;

  SECTION("Parse with comments") {
    std::string toml = R"(# This is a comment
name = "test"  # inline comment
# Another comment
version = "1.0")";
    TOMLValue root = parser.parse(toml);
    REQUIRE(parser.success());
    REQUIRE(root.tableValue["name"].asString() == "test");
    REQUIRE(root.tableValue["version"].asString() == "1.0");
  }
}

TEST_CASE("TOML Parser errors", "[config]") {
  TOMLParser parser;

  SECTION("Invalid syntax") {
    TOMLValue root = parser.parse(R"(name = )");
    REQUIRE(!parser.success());
  }

  SECTION("Unterminated string") {
    TOMLValue root = parser.parse(R"(name = "test)");
    REQUIRE(!parser.success());
  }
}

TEST_CASE("Config loading", "[config]") {
  Config config;

  SECTION("Load from string") {
    std::string toml = R"([project]
name = "myapp"
version = "1.0.0"
edition = "2024"

[build]
target = "native"
opt-level = 2
debug = true)";

    REQUIRE(config.loadFromString(toml));
    REQUIRE(config.isLoaded());
    REQUIRE(config.project.name == "myapp");
    REQUIRE(config.project.version == "1.0.0");
    REQUIRE(config.project.edition == "2024");
    REQUIRE(config.build.target == "native");
    REQUIRE(config.build.optLevel == 2);
    REQUIRE(config.build.debug == true);
  }

  SECTION("Validation passes with valid config") {
    std::string toml = R"([project]
name = "test")";
    config.loadFromString(toml);
    REQUIRE(config.validate().empty());
  }

  SECTION("Validation fails without name") {
    Config emptyConfig;
    REQUIRE(emptyConfig.validate() == "No configuration loaded");
  }
}

TEST_CASE("Config dependencies", "[config]") {
  Config config;

  SECTION("Load simple dependency") {
    std::string toml = R"([project]
name = "myapp"

[dependencies]
stdlib = "1.0.0")";

    REQUIRE(config.loadFromString(toml));
    REQUIRE(config.dependencies.size() == 1);
    REQUIRE(config.dependencies[0].name == "stdlib");
    REQUIRE(config.dependencies[0].version == "1.0.0");
  }

  SECTION("Load complex dependency") {
    std::string toml = R"([project]
name = "myapp"

[dependencies]
http = { git = "https://github.com/test/http", branch = "main" })";

    REQUIRE(config.loadFromString(toml));
    REQUIRE(config.dependencies.size() == 1);
    REQUIRE(config.dependencies[0].name == "http");
    REQUIRE(config.dependencies[0].gitUrl == "https://github.com/test/http");
    REQUIRE(config.dependencies[0].gitBranch == "main");
  }

  SECTION("Get dependency by name") {
    std::string toml = R"([project]
name = "myapp"

[dependencies]
stdlib = "1.0.0"
http = "2.0.0")";

    config.loadFromString(toml);
    const Dependency *dep = config.getDependency("http");
    REQUIRE(dep != nullptr);
    REQUIRE(dep->version == "2.0.0");
    REQUIRE(config.getDependency("nonexistent") == nullptr);
  }
}

TEST_CASE("Build profiles", "[config]") {
  SECTION("Debug profile") {
    auto profile = BuildProfile::debugProfile();
    REQUIRE(profile.name == "debug");
    REQUIRE(profile.optLevel == 0);
    REQUIRE(profile.debug == true);
    REQUIRE(profile.lto == false);
  }

  SECTION("Release profile") {
    auto profile = BuildProfile::releaseProfile();
    REQUIRE(profile.name == "release");
    REQUIRE(profile.optLevel == 3);
    REQUIRE(profile.debug == false);
    REQUIRE(profile.lto == true);
  }

  SECTION("Test profile") {
    auto profile = BuildProfile::testProfile();
    REQUIRE(profile.name == "test");
    REQUIRE(profile.optLevel == 0);
    REQUIRE(profile.debug == true);
  }
}

TEST_CASE("Lock file operations", "[config]") {
  LockFile lockFile;

  SECTION("Empty lock file save and load") {
    std::string testPath = "/tmp/meadows_test.lock";
    REQUIRE(lockFile.save(testPath));
    REQUIRE(lockFile.exists(testPath));

    // For now just verify the file was created
    // Full parsing will be implemented in Phase 4
  }

  SECTION("Lock file with dependencies") {
    std::string testPath = "/tmp/meadows_test2.lock";
    lockFile.dependencies = {
        {"stdlib", "1.0.0", "registry+https://registry.meadows.dev", "abc123"},
        {"http", "2.0.0", "git+https://github.com/test/http", ""}};

    REQUIRE(lockFile.save(testPath));
    REQUIRE(lockFile.exists(testPath));

    // For now just verify the file was created
    // Full parsing will be implemented in Phase 4
  }
}
