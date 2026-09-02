#include "lexer/ModuleResolver.h"

#include "utils/Exceptions.h"
#include <catch2/catch_all.hpp>
#include <filesystem>
#include <fstream>

using namespace meadows;
namespace fs = std::filesystem;

namespace {

// A scratch directory under the system temp dir, unique per test run,
// removed on destruction — ModuleResolver operates on real files (import
// paths are resolved relative to the importing file's directory on disk),
// so there's no way to test it against in-memory strings alone.
struct TempDir {
  fs::path path;
  TempDir() {
    path = fs::temp_directory_path() /
           fs::path("meadows_module_resolver_test_" +
                    std::to_string(reinterpret_cast<uintptr_t>(this)));
    fs::create_directories(path);
  }
  ~TempDir() { std::error_code ec; fs::remove_all(path, ec); }

  fs::path write(const std::string &name, const std::string &content) {
    fs::path p = path / name;
    std::ofstream(p) << content;
    return p;
  }
};

} // namespace

TEST_CASE("ModuleResolver: a file with no imports returns its own tokens",
         "[lexer][modules]") {
  TempDir dir;
  auto main = dir.write("main.ms", "print(1);");

  auto tokens = ModuleResolver::resolve(main.string());
  // print ( 1 ) ; EOF
  REQUIRE(tokens.size() == 6);
  CHECK(tokens[0].type == TokenType::IDENTIFIER);
  CHECK(tokens[0].value == "print");
  CHECK(tokens.back().type == TokenType::EOF_TOKEN);
}

TEST_CASE("ModuleResolver: import splices the imported file's tokens in place",
         "[lexer][modules]") {
  TempDir dir;
  dir.write("util.ms", "func square(x) { return x * x; }");
  auto main = dir.write("main.ms", "import \"util.ms\";\nprint(square(5));");

  auto tokens = ModuleResolver::resolve(main.string());
  // The FUNC keyword from util.ms should appear, and there should be
  // exactly one EOF_TOKEN (the one ModuleResolver::resolve appends) even
  // though two files were lexed.
  bool hasFunc = false;
  int eofCount = 0;
  for (auto &t : tokens) {
    if (t.type == TokenType::FUNC) hasFunc = true;
    if (t.type == TokenType::EOF_TOKEN) eofCount++;
  }
  CHECK(hasFunc);
  CHECK(eofCount == 1);
  // No IMPORT token should survive into the resolved stream — the parser
  // never needs to know imports happened.
  for (auto &t : tokens) CHECK(t.type != TokenType::IMPORT);
}

TEST_CASE("ModuleResolver: diamond imports resolve the shared file once",
         "[lexer][modules]") {
  TempDir dir;
  dir.write("shared.ms", "func base() { return 1; }");
  dir.write("b.ms", "import \"shared.ms\";\nfunc fromB() { return base(); }");
  dir.write("c.ms", "import \"shared.ms\";\nfunc fromC() { return base(); }");
  auto main = dir.write("main.ms", "import \"b.ms\";\nimport \"c.ms\";\nprint(1);");

  auto tokens = ModuleResolver::resolve(main.string());
  int baseDefCount = 0;
  for (size_t i = 0; i + 1 < tokens.size(); i++) {
    if (tokens[i].type == TokenType::IDENTIFIER && tokens[i].value == "base" &&
        tokens[i + 1].type == TokenType::LEFT_PAREN) {
      // Distinguish the definition (`func base(`) from a call (`base(`) by
      // checking two tokens back for FUNC.
      if (i >= 1 && tokens[i - 1].type == TokenType::FUNC) baseDefCount++;
    }
  }
  CHECK(baseDefCount == 1);
}

TEST_CASE("ModuleResolver: circular imports don't infinite-loop", "[lexer][modules]") {
  TempDir dir;
  dir.write("a.ms", "import \"b.ms\";\nfunc fromA() { return 1; }");
  dir.write("b.ms", "import \"a.ms\";\nfunc fromB() { return 2; }");

  auto tokens = ModuleResolver::resolve((dir.path / "a.ms").string());
  CHECK_FALSE(tokens.empty());
  CHECK(tokens.back().type == TokenType::EOF_TOKEN);
}

TEST_CASE("ModuleResolver: importing a nonexistent file throws", "[lexer][modules]") {
  TempDir dir;
  auto main = dir.write("main.ms", "import \"missing.ms\";");
  REQUIRE_THROWS_AS(ModuleResolver::resolve(main.string()), MeadowsException);
}

TEST_CASE("ModuleResolver: path traversal in an import is rejected",
         "[lexer][modules]") {
  TempDir dir;
  auto main = dir.write("main.ms", "import \"../../../etc/passwd.ms\";");
  REQUIRE_THROWS_AS(ModuleResolver::resolve(main.string()), MeadowsException);
}

TEST_CASE("ModuleResolver: malformed import statement throws a clear error",
         "[lexer][modules]") {
  TempDir dir;
  auto main = dir.write("main.ms", "import util;"); // missing quotes
  REQUIRE_THROWS_AS(ModuleResolver::resolve(main.string()), MeadowsException);
}
