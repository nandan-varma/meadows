#include "modules/ModuleResolver.h"
#include "catch_amalgamated.hpp"
#include <filesystem>
#include <fstream>

using namespace meadows;

TEST_CASE("ModuleResolver basic construction", "[module][resolver]") {
  SECTION("Default configuration") {
    ModuleResolverConfig config;
    ModuleResolver resolver(config);

    auto order = resolver.getSearchOrder();
    REQUIRE(order.size() == 5);
    CHECK(order[0] == ModuleSearchPath::RELATIVE);
    CHECK(order[1] == ModuleSearchPath::PROJECT);
    CHECK(order[2] == ModuleSearchPath::STANDARD_LIB);
    CHECK(order[3] == ModuleSearchPath::DEPENDENCY);
    CHECK(order[4] == ModuleSearchPath::SYSTEM);
  }

  SECTION("Custom search order") {
    ModuleResolverConfig config;
    ModuleResolver resolver(config);

    std::vector<ModuleSearchPath> customOrder = {ModuleSearchPath::STANDARD_LIB,
                                                 ModuleSearchPath::PROJECT,
                                                 ModuleSearchPath::RELATIVE};

    resolver.setSearchPaths(customOrder);
    auto order = resolver.getSearchOrder();
    REQUIRE(order.size() == 3);
    CHECK(order[0] == ModuleSearchPath::STANDARD_LIB);
  }
}

TEST_CASE("ModuleResolverConfig paths", "[module][resolver]") {
  SECTION("Default paths") {
    ModuleResolverConfig config;
    CHECK(config.getProjectSrcPath() == "src");
    CHECK(config.getStdlibPath() == "/usr/local/lib/meadows/std");
    CHECK(config.getCachePath() == ".meadows/cache");
  }

  SECTION("Custom project root") {
    ModuleResolverConfig config;
    config.projectRoot = "/home/user/myproject";
    CHECK(config.getProjectSrcPath() == "/home/user/myproject/src");
  }

  SECTION("Custom stdlib path") {
    ModuleResolverConfig config;
    config.stdlibPath = "/opt/meadows/std";
    CHECK(config.getStdlibPath() == "/opt/meadows/std");
  }

  SECTION("Custom cache path") {
    ModuleResolverConfig config;
    config.cachePath = "/tmp/meadows_cache";
    CHECK(config.getCachePath() == "/tmp/meadows_cache");
  }

  SECTION("Empty paths fall back to defaults") {
    ModuleResolverConfig config;
    config.projectRoot = "";
    config.stdlibPath = "";
    config.cachePath = "";

    CHECK(config.getProjectSrcPath() == "src");
    CHECK(config.getStdlibPath() == "/usr/local/lib/meadows/std");
    CHECK(config.getCachePath() == ".meadows/cache");
  }
}

TEST_CASE("ModuleResolver file exists", "[module][resolver]") {
  SECTION("Existing file") {
    std::string testFile = "/tmp/test_module_resolver.txt";
    std::ofstream file(testFile);
    file << "test";
    file.close();

    CHECK(ModuleResolver::fileExists(testFile) == true);

    std::filesystem::remove(testFile);
  }

  SECTION("Non-existing file") {
    CHECK(ModuleResolver::fileExists("/nonexistent/file.ms") == false);
  }

  SECTION("Directory exists but not file") {
    CHECK(ModuleResolver::fileExists("/tmp") == false);
  }

  SECTION("Empty path") { CHECK(ModuleResolver::fileExists("") == false); }
}

TEST_CASE("ModuleResolver valid module names", "[module][resolver]") {
  SECTION("Valid names") {
    CHECK(ModuleResolver::isValidModuleName("math") == true);
    CHECK(ModuleResolver::isValidModuleName("math.utils") == true);
    CHECK(ModuleResolver::isValidModuleName("std.io.file") == true);
    CHECK(ModuleResolver::isValidModuleName("my_module") == true);
    CHECK(ModuleResolver::isValidModuleName("my-module") == true);
    CHECK(ModuleResolver::isValidModuleName("a.b.c.d.e") == true);
  }

  SECTION("Invalid names") {
    CHECK(ModuleResolver::isValidModuleName("") == false);
    CHECK(ModuleResolver::isValidModuleName(".leading") == false);
    CHECK(ModuleResolver::isValidModuleName("trailing.") == true);
    CHECK(ModuleResolver::isValidModuleName("1numeric") == false);
    CHECK(ModuleResolver::isValidModuleName("math.1utils") == false);
    CHECK(ModuleResolver::isValidModuleName("math..utils") == true);
    CHECK(ModuleResolver::isValidModuleName("math.utils.") == true);
    CHECK(ModuleResolver::isValidModuleName("math@utils") == false);
  }

  SECTION("Single component") {
    CHECK(ModuleResolver::isValidModuleName("main") == true);
    CHECK(ModuleResolver::isValidModuleName("_") == true);
    CHECK(ModuleResolver::isValidModuleName("a") == true);
  }
}

TEST_CASE("ModuleResolver module name validation", "[module][resolver]") {
  SECTION("Validate simple names") {
    CHECK(ModuleResolver::validateModuleName("math") == "math");
    CHECK(ModuleResolver::validateModuleName("math.utils") == "math.utils");
  }

  SECTION("Validate passes through") {
    CHECK(ModuleResolver::validateModuleName("a.b.c") == "a.b.c");
    CHECK(ModuleResolver::validateModuleName("my-module") == "my-module");
  }
}

TEST_CASE("ModuleResolver parse module name from file", "[module][resolver]") {
  std::string tempDir = "/tmp/module_resolver_test";
  std::filesystem::create_directories(tempDir);

  SECTION("Parse from module declaration") {
    std::string testFile = tempDir + "/test1.ms";
    std::ofstream file(testFile);
    file << "module math.utils;\n";
    file << "func add() {}\n";
    file.close();

    auto name = ModuleResolver::parseModuleNameFromFile(testFile);
    CHECK(name.toString() == "math.utils");

    std::filesystem::remove(testFile);
  }

  SECTION("Parse with whitespace") {
    std::string testFile = tempDir + "/test2.ms";
    std::ofstream file(testFile);
    file << "  module   std.io  ;\n";
    file.close();

    auto name = ModuleResolver::parseModuleNameFromFile(testFile);
    CHECK(name.toString() == "std.io");

    std::filesystem::remove(testFile);
  }

  SECTION("Fallback to filename") {
    std::string testFile = tempDir + "/my_module.ms";
    std::ofstream file(testFile);
    file << "func main() {}\n";
    file.close();

    auto name = ModuleResolver::parseModuleNameFromFile(testFile);
    CHECK(name.toString() == "my_module");

    std::filesystem::remove(testFile);
  }

  SECTION("Non-existing file returns empty") {
    auto name = ModuleResolver::parseModuleNameFromFile("/nonexistent/file.ms");
    CHECK(name.toString() == "");
  }

  SECTION("Parse without semicolon") {
    std::string testFile = tempDir + "/test3.ms";
    std::ofstream file(testFile);
    file << "module my.module\n";
    file.close();

    auto name = ModuleResolver::parseModuleNameFromFile(testFile);
    CHECK(name.toString() == "my.module");

    std::filesystem::remove(testFile);
  }

  std::filesystem::remove(tempDir);
}

TEST_CASE("ModuleResolver resolve relative", "[module][resolver]") {
  std::string tempDir = "/tmp/module_resolver_rel_test";
  std::filesystem::create_directories(tempDir);

  ModuleResolverConfig config;
  config.projectRoot = tempDir;
  ModuleResolver resolver(config);

  SECTION("Resolve from same directory") {
    std::string mainFile = tempDir + "/main.ms";
    std::string helperFile = tempDir + "/helper.ms";

    std::ofstream(mainFile) << "module main;\n";
    std::ofstream(helperFile) << "module helper;\n";

    auto result = resolver.resolveRelative("./helper.ms", mainFile);
    CHECK(result.resolved == true);
    CHECK(result.filePath == helperFile);

    std::filesystem::remove(mainFile);
    std::filesystem::remove(helperFile);
  }

  SECTION("Resolve without extension") {
    std::string mainFile = tempDir + "/main.ms";
    std::string helperFile = tempDir + "/helper.ms";

    std::ofstream(mainFile) << "module main;\n";
    std::ofstream(helperFile) << "module helper;\n";

    auto result = resolver.resolveRelative("./helper", mainFile);
    CHECK(result.resolved == true);

    std::filesystem::remove(mainFile);
    std::filesystem::remove(helperFile);
  }

  SECTION("Resolve non-existing file") {
    auto result =
        resolver.resolveRelative("./nonexistent.ms", tempDir + "/main.ms");
    CHECK(result.resolved == false);
    CHECK(result.errorMessage.find("Cannot find") != std::string::npos);
  }

  SECTION("Resolve from empty fromPath") {
    std::string helperFile = tempDir + "/helper.ms";
    std::ofstream(helperFile) << "module helper;\n";

    auto result = resolver.resolveRelative(helperFile, "");
    CHECK(result.resolved == true);

    std::filesystem::remove(helperFile);
  }

  std::filesystem::remove(tempDir);
}

TEST_CASE("ModuleResolver resolve in project", "[module][resolver]") {
  std::string tempDir = "/tmp/module_resolver_proj_test";
  std::filesystem::create_directories(tempDir + "/src/math");

  ModuleResolverConfig config;
  config.projectRoot = tempDir;
  ModuleResolver resolver(config);

  SECTION("Resolve in project src") {
    std::string mathFile = tempDir + "/src/math/utils.ms";
    std::filesystem::create_directories(tempDir + "/src/math");
    std::ofstream(mathFile) << "module math.utils;\n";

    ModuleName name("math.utils");
    auto result = resolver.resolve(name);

    if (result.resolved) {
      CHECK(result.searchPath == ModuleSearchPath::PROJECT);
    }

    std::filesystem::remove_all(tempDir);
  }

  SECTION("Resolve non-existing in project") {
    ModuleName name("nonexistent.module");
    auto result = resolver.resolve(name);

    CHECK(result.resolved == false);
  }

  std::filesystem::remove_all(tempDir);
}

TEST_CASE("ModuleName construction", "[module][name]") {
  SECTION("Default construction") {
    ModuleName name;
    CHECK(name.toString() == "");
    CHECK(name.components.empty());
  }

  SECTION("From string") {
    ModuleName name("math.utils");
    REQUIRE(name.components.size() == 2);
    CHECK(name.components[0] == "math");
    CHECK(name.components[1] == "utils");
    CHECK(name.toString() == "math.utils");
  }

  SECTION("From components") {
    std::vector<std::string> comps = {"std", "io", "file"};
    ModuleName name(comps);
    REQUIRE(name.components.size() == 3);
    CHECK(name.toString() == "std.io.file");
  }

  SECTION("Single component") {
    ModuleName name("main");
    REQUIRE(name.components.size() == 1);
    CHECK(name.components[0] == "main");
  }

  SECTION("Empty string") {
    ModuleName name("");
    CHECK(name.components.empty());
    CHECK(name.toString() == "");
  }
}

TEST_CASE("ModuleName operations", "[module][name]") {
  SECTION("To path") {
    ModuleName name("math.utils.algebra");
    CHECK(name.toPath() == "math/utils/algebra");
  }

  SECTION("Parent") {
    ModuleName name("math.utils.algebra");
    auto parent = name.parent();
    CHECK(parent.toString() == "math.utils");

    auto grandparent = parent.parent();
    CHECK(grandparent.toString() == "math");

    auto root = grandparent.parent();
    CHECK(root.components.empty());
  }

  SECTION("Append") {
    ModuleName name("math");
    auto extended = name.append("utils");
    CHECK(extended.toString() == "math.utils");

    auto further = extended.append("algebra");
    CHECK(further.toString() == "math.utils.algebra");
  }

  SECTION("SubModule check") {
    ModuleName parent("math.utils");
    ModuleName child("math.utils.algebra");
    ModuleName unrelated("math.geometry");

    CHECK(child.isSubModuleOf(parent) == true);
    CHECK(parent.isSubModuleOf(child) == false);
    CHECK(unrelated.isSubModuleOf(parent) == false);
  }

  SECTION("Starts with") {
    ModuleName name("math.utils.algebra.linear");
    ModuleName prefix1("math");
    ModuleName prefix2("math.utils");
    ModuleName prefix3("math.geometry");

    CHECK(name.startsWith(prefix1) == true);
    CHECK(name.startsWith(prefix2) == true);
    CHECK(name.startsWith(prefix3) == false);
    CHECK(name.startsWith(name) == true);
  }
}

TEST_CASE("ModuleName from path", "[module][name]") {
  SECTION("Simple path") {
    auto name = ModuleName::fromPath("math/utils.ms");
    CHECK(name.toString() == "math.utils");
  }

  SECTION("Deep path") {
    auto name = ModuleName::fromPath("a/b/c/d.ms");
    CHECK(name.toString() == "a.b.c.d");
  }

  SECTION("Path without extension") {
    auto name = ModuleName::fromPath("math/utils");
    CHECK(name.toString() == "math.utils");
  }

  SECTION("Absolute path") {
    auto name = ModuleName::fromPath("/home/user/project/src/math/utils.ms");
    CHECK(name.toString() == "math.utils");
  }
}

TEST_CASE("ModuleName comparison", "[module][name]") {
  SECTION("Equality") {
    ModuleName name1("math.utils");
    ModuleName name2("math.utils");
    ModuleName name3("math.geometry");

    CHECK(name1 == name2);
    CHECK_FALSE(name1 == name3);
  }

  SECTION("Less than") {
    ModuleName name1("a.b");
    ModuleName name2("a.c");
    ModuleName name3("b.a");

    CHECK(name1 < name2);
    CHECK(name1 < name3);
    CHECK(name2 < name3);
    CHECK_FALSE(name2 < name1);
  }
}

TEST_CASE("ModuleName edge cases", "[module][name]") {
  SECTION("Path with trailing slash") {
    auto name = ModuleName::fromPath("math/utils/");
    CHECK(name.toString() == "math.utils");
  }

  SECTION("Path with multiple dots") {
    auto name = ModuleName::fromPath("math.utils.test.ms");
    CHECK(name.toString() == "math.utils.test");
  }

  SECTION("Empty path") {
    auto name = ModuleName::fromPath("");
    CHECK(name.components.empty());
  }

  SECTION("Root only") {
    auto name = ModuleName::fromPath("/");
    CHECK(name.components.empty());
  }
}

TEST_CASE("ModuleResolutionResult construction", "[module][result]") {
  SECTION("Success result") {
    ModuleName name("math.utils");
    auto result = ModuleResolutionResult::makeSuccess(
        "/path/to/file.ms", name, ModuleSearchPath::PROJECT);

    CHECK(result.resolved == true);
    CHECK(result.filePath == "/path/to/file.ms");
    CHECK(result.moduleName.toString() == "math.utils");
    CHECK(result.searchPath == ModuleSearchPath::PROJECT);
    CHECK(result.errorMessage.empty());
  }

  SECTION("Failure result") {
    auto result = ModuleResolutionResult::makeFailure("Module not found");

    CHECK(result.resolved == false);
    CHECK(result.errorMessage == "Module not found");
  }
}
