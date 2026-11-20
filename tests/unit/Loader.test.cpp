#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <fstream>

#include "TestSupport.h"
#include "loader/ModuleLoader.h"
#include "utils/PathValidation.h"

using namespace meadows;
namespace fs = std::filesystem;

namespace {

class Project {
public:
  Project() {
    root_ = fs::temp_directory_path() /
            ("meadows-loader-" + std::to_string(counter_++) + "-" +
             std::to_string(reinterpret_cast<uintptr_t>(this)));
    fs::create_directories(root_);
  }
  ~Project() { fs::remove_all(root_); }

  std::string write(const std::string& name, const std::string& text) {
    fs::path path = root_ / name;
    fs::create_directories(path.parent_path());
    std::ofstream(path) << text;
    return path.string();
  }

  testing::Execution run(const std::string& entry) {
    std::string path = root_ / entry;
    std::string text;
    REQUIRE(readSourceFile(path, text) == ReadResult::OK);
    Analysis analysis = analyzeSource(path, text);
    testing::Execution execution;
    if (!analysis.ok()) {
      execution.output = formatDiagnostics(analysis.diagnostics.diagnostics(),
                                           analysis.sources);
      return execution;
    }
    Interpreter interpreter(
        [&](std::string_view out) { execution.output += out; });
    execution.error = interpreter.run(analysis.program);
    return execution;
  }

  std::vector<ErrorCode> errors(const std::string& entry) {
    std::string path = root_ / entry;
    std::string text;
    REQUIRE(readSourceFile(path, text) == ReadResult::OK);
    return testing::codes(analyzeSource(path, text));
  }

private:
  static inline int counter_ = 0;
  fs::path root_;
};

} // namespace

TEST_CASE("imports splice a module at the position of the import", "[loader]") {
  Project p;
  p.write("lib.ms", "func twice(n) { return n * 2; }\nprint(\"lib\");");
  p.write("main.ms",
          "print(\"before\");\nimport \"lib.ms\";\nprint(twice(4));");
  CHECK(p.run("main.ms").output == "before\nlib\n8\n");
}

TEST_CASE("a module is imported only once", "[loader]") {
  Project p;
  p.write("shared.ms", "print(\"shared\");");
  p.write("a.ms", "import \"shared.ms\";");
  p.write("b.ms", "import \"shared.ms\";");
  p.write("main.ms", "import \"a.ms\"; import \"b.ms\"; import \"shared.ms\";");
  CHECK(p.run("main.ms").output == "shared\n");
}

TEST_CASE("circular imports terminate", "[loader]") {
  Project p;
  p.write("a.ms", "import \"b.ms\"; print(\"a\");");
  p.write("b.ms", "import \"a.ms\"; print(\"b\");");
  CHECK(p.run("a.ms").output == "b\na\n");
}

TEST_CASE("imports resolve relative to the importing file", "[loader]") {
  Project p;
  p.write("sub/inner.ms", "print(\"inner\");");
  p.write("sub/mid.ms", "import \"inner.ms\"; print(\"mid\");");
  p.write("main.ms", "import \"sub/mid.ms\";");
  CHECK(p.run("main.ms").output == "inner\nmid\n");
}

TEST_CASE("modules share one global namespace", "[loader]") {
  Project p;
  p.write("lib.ms", "func dup() { return 1; }");
  p.write("main.ms", "import \"lib.ms\"; func dup() { return 2; }");
  CHECK(p.errors("main.ms") ==
        std::vector<ErrorCode>{ErrorCode::SEM_REDEFINED_FUNCTION});
}

TEST_CASE("import failures are diagnosed at the import", "[loader]") {
  Project p;
  p.write("main.ms", "import \"missing.ms\";");
  CHECK(p.errors("main.ms") ==
        std::vector<ErrorCode>{ErrorCode::SYS_FILE_NOT_FOUND});

  p.write("up.ms", "import \"../x.ms\";");
  CHECK(p.errors("up.ms") ==
        std::vector<ErrorCode>{ErrorCode::SYS_INVALID_PATH});

  p.write("ext.ms", "import \"lib.txt\";");
  CHECK(p.errors("ext.ms") ==
        std::vector<ErrorCode>{ErrorCode::SYS_INVALID_PATH});

  p.write("abs.ms", "import \"/etc/passwd.ms\";");
  CHECK(p.errors("abs.ms") ==
        std::vector<ErrorCode>{ErrorCode::SYS_INVALID_PATH});
}

TEST_CASE("diagnostics in a module name that module", "[loader]") {
  Project p;
  std::string lib = p.write("lib.ms", "let x = ;");
  std::string main = p.write("main.ms", "import \"lib.ms\";");
  std::string text;
  REQUIRE(readSourceFile(main, text) == ReadResult::OK);
  Analysis a = analyzeSource(main, text);
  REQUIRE(a.diagnostics.diagnostics().size() == 1);
  CHECK(a.sources.path(a.diagnostics.diagnostics()[0].location.file) == lib);
}

TEST_CASE("import chains are bounded", "[loader]") {
  Project p;
  for (int i = 0; i < 70; ++i) {
    p.write("m" + std::to_string(i) + ".ms",
            "import \"m" + std::to_string(i + 1) + ".ms\";");
  }
  p.write("m70.ms", "");
  CHECK(p.errors("m0.ms") ==
        std::vector<ErrorCode>{ErrorCode::SYS_INVALID_PATH});
}
