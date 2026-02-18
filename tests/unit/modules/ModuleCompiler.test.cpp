#include "modules/ModuleCompiler.h"
#include "catch_amalgamated.hpp"
#include "modules/ModuleResolver.h"
#include <fstream>
#include <memory>

using namespace meadows;

TEST_CASE("ModuleCompiler basic operations", "[module][compiler]") {
  SECTION("Compiler initializes with config") {
    ModuleCompileConfig config;
    config.projectRoot = "/tmp/test_project";
    config.verbose = true;

    ModuleCompiler compiler(config);
    (void)compiler; // Suppress unused variable warning
  }

  SECTION("compileFromSource parses module") {
    ModuleCompileConfig config;
    config.projectRoot = "/tmp/test_project";

    ModuleCompiler compiler(config);
    auto result =
        compiler.compileFromSource("module test.math;\n"
                                   "func add(a: i32, b: i32) -> i32 {\n"
                                   "    return a + b;\n"
                                   "}\n",
                                   "test.math");

    REQUIRE(result.success == true);
    REQUIRE(result.compiledModules.size() == 1);
    CHECK(result.compiledModules[0].name.toString() == "test.math");
    CHECK(result.compiledModules[0].sourcePath == "<memory>");
  }

  SECTION("compileFromSource extracts exports") {
    ModuleCompileConfig config;
    config.projectRoot = "/tmp/test_project";

    ModuleCompiler compiler(config);
    auto result = compiler.compileFromSource(
        "module test.math;\n"
        "func add(a: i32, b: i32) -> i32 { return a + b; }\n"
        "func mul(a: i32, b: i32) -> i32 { return a * b; }\n",
        "test.math");

    REQUIRE(result.success == true);
    REQUIRE(result.compiledModules[0].exportedFunctions.size() == 2);
  }

  SECTION("compileFromSource handles syntax errors") {
    ModuleCompileConfig config;
    config.projectRoot = "/tmp/test_project";

    ModuleCompiler compiler(config);
    auto result =
        compiler.compileFromSource("module test.math;\n"
                                   "func add(a: i32, b: i32) -> i32 {\n"
                                   "    return a +\n" // Missing operand
                                   "}\n",
                                   "test.math");

    REQUIRE(result.success == false);
    REQUIRE(result.errors.size() > 0);
  }
}

TEST_CASE("CompiledModuleInfo structure", "[module]") {
  SECTION("Default values are correct") {
    CompiledModuleInfo info;
    CHECK(info.isDirty == true);
    CHECK(info.hasErrors == false);
    CHECK(info.sourcePath.empty());
    CHECK(info.dependencies.empty());
    CHECK(info.exportedFunctions.empty());
    CHECK(info.exportedTypes.empty());
  }

  SECTION("Module info can be populated") {
    CompiledModuleInfo info;
    info.name = ModuleName("test.module");
    info.sourcePath = "/src/test/module.ms";
    info.outputPath = "/src/test/module.ms.o";
    info.irPath = "/src/test/module.ms.ll";
    info.isDirty = false;
    info.exportedFunctions.push_back("testFunc");
    info.exportedTypes.push_back("TestType");

    CHECK(info.name.toString() == "test.module");
    CHECK(info.sourcePath == "/src/test/module.ms");
    CHECK(info.isDirty == false);
    CHECK(info.exportedFunctions.size() == 1);
    CHECK(info.exportedTypes.size() == 1);
  }
}

TEST_CASE("ModuleCompileResult structure", "[module]") {
  SECTION("makeSuccess creates successful result") {
    auto result = ModuleCompileResult::makeSuccess("/path/to/output");

    CHECK(result.success == true);
    CHECK(result.outputFile == "/path/to/output");
    CHECK(result.errors.empty());
    CHECK(result.warnings.empty());
  }

  SECTION("makeFailure creates failed result") {
    std::vector<std::string> errors = {"Error 1", "Error 2"};
    auto result = ModuleCompileResult::makeFailure(errors);

    CHECK(result.success == false);
    CHECK(result.errors.size() == 2);
    CHECK(result.errors[0] == "Error 1");
    CHECK(result.errors[1] == "Error 2");
  }
}

TEST_CASE("ModuleCompileConfig structure", "[module]") {
  SECTION("Default values are correct") {
    ModuleCompileConfig config;

    CHECK(config.projectRoot.empty());
    CHECK(config.entryPoint.empty());
    CHECK(config.outputDir.empty());
    CHECK(config.verbose == false);
    CHECK(config.enableCache == true);
    CHECK(config.runAfterCompile == false);
    CHECK(config.stdlibPath.empty());
    CHECK(config.searchPaths.empty());
  }

  SECTION("Config can be modified") {
    ModuleCompileConfig config;
    config.projectRoot = "/home/user/project";
    config.verbose = true;
    config.enableCache = false;

    CHECK(config.projectRoot == "/home/user/project");
    CHECK(config.verbose == true);
    CHECK(config.enableCache == false);
  }
}
