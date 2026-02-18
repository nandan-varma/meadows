#include "catch_amalgamated.hpp"
#include "modules/ModuleCompiler.h"
#include <fstream>
#include <memory>

using namespace meadows;

TEST_CASE("Stdlib module parsing", "[stdlib]") {
  SECTION("Parse std.math module") {
    std::string mathSource = "module std.math;\n"
                             "func abs(n: i32) -> i32 {\n"
                             "    if (n < 0) { return -n; }\n"
                             "    return n;\n"
                             "}\n";

    ModuleCompileConfig config;
    config.projectRoot = "/tmp/test";
    ModuleCompiler compiler(config);
    auto result = compiler.compileFromSource(mathSource, "std.math");

    REQUIRE(result.success == true);
    REQUIRE(result.compiledModules.size() == 1);
    CHECK(result.compiledModules[0].name.toString() == "std.math");
    REQUIRE(result.compiledModules[0].exportedFunctions.size() == 1);
    CHECK(result.compiledModules[0].exportedFunctions[0] == "abs");
  }

  SECTION("Parse std.string module") {
    std::string stringSource =
        "module std.string;\n"
        "func length(s: string) -> i32 { return 0; }\n"
        "func to_upper(s: string) -> string { return \"\"; }\n";

    ModuleCompileConfig config;
    config.projectRoot = "/tmp/test";
    ModuleCompiler compiler(config);
    auto result = compiler.compileFromSource(stringSource, "std.string");

    REQUIRE(result.success == true);
    CHECK(result.compiledModules[0].exportedFunctions.size() == 2);
  }

  SECTION("Parse std.io module with imports") {
    std::string ioSource =
        "module std.io;\n"
        "func read_file(path: string) -> string { return \"\"; }\n"
        "func write_file(path: string, content: string) -> bool { return true; "
        "}\n";

    ModuleCompileConfig config;
    config.projectRoot = "/tmp/test";
    ModuleCompiler compiler(config);
    auto result = compiler.compileFromSource(ioSource, "std.io");

    REQUIRE(result.success == true);
    REQUIRE(result.compiledModules[0].exportedFunctions.size() == 2);
  }
}

TEST_CASE("Module dependency resolution", "[stdlib][module]") {
  SECTION("Resolve std.math when imported") {
    std::string mainSource = "module main;\n"
                             "import std.math;\n"
                             "func main() -> i32 {\n"
                             "    return std.math.abs(-5);\n"
                             "}\n";

    ModuleCompileConfig config;
    config.projectRoot = "/tmp/test";
    config.stdlibPath = "/tmp/test/stdlib";

    std::filesystem::create_directories("/tmp/test/stdlib/std");
    std::ofstream mathFile("/tmp/test/stdlib/std/math.ms");
    mathFile << "module std.math;\n";
    mathFile
        << "func abs(n: i32) -> i32 { if (n < 0) { return -n; } return n; }\n";
    mathFile.close();

    ModuleCompiler compiler(config);
    auto result = compiler.compileFromSource(mainSource, "main");

    REQUIRE(result.success == true);
    CHECK(result.compiledModules.size() >= 1);
  }
}

TEST_CASE("Stdlib functions with type annotations", "[stdlib]") {
  SECTION("Functions with primitive types") {
    std::string source =
        "module test;\n"
        "func add(a: i32, b: i32) -> i32 { return a + b; }\n"
        "func multiply(a: f64, b: f64) -> f64 { return a * b; }\n"
        "func is_positive(n: i32) -> bool { return n > 0; }\n";

    ModuleCompileConfig config;
    config.projectRoot = "/tmp/test";
    ModuleCompiler compiler(config);
    auto result = compiler.compileFromSource(source, "test");

    REQUIRE(result.success == true);
    REQUIRE(result.compiledModules[0].exportedFunctions.size() == 3);
  }
}

TEST_CASE("Integration with existing parser tests", "[stdlib][parser]") {
  SECTION("Parse complex stdlib function") {
    std::string source = "module std.utils;\n"
                         "func process(data: string, count: i32) -> string {\n"
                         "    return data;\n"
                         "}\n";

    ModuleCompileConfig config;
    config.projectRoot = "/tmp/test";
    ModuleCompiler compiler(config);
    auto result = compiler.compileFromSource(source, "std.utils");

    REQUIRE(result.success == true);
  }
}
