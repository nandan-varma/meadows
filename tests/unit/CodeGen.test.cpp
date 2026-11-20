#include <catch2/catch_test_macros.hpp>

#include <filesystem>

#include "TestSupport.h"
#include "codegen/CodeGen.h"
#include "utils/PathValidation.h"

using namespace meadows;
namespace fs = std::filesystem;

namespace {

std::string generate(const std::string& source, std::string* error = nullptr) {
  Analysis analysis = testing::analyze(source);
  REQUIRE(analysis.ok());
  CodeGen codegen(analysis.sources);
  std::string ir = codegen.generate(analysis.program);
  if (error) {
    *error = codegen.verificationError();
  }
  return ir;
}

bool contains(const std::string& text, const std::string& part) {
  return text.find(part) != std::string::npos;
}

} // namespace

TEST_CASE("codegen emits an entry point and one function per declaration",
          "[codegen]") {
  std::string ir = generate("func add(a, b) { return a + b; }\nprint(add(1, 2));");
  CHECK(contains(ir, "define i32 @main()"));
  CHECK(contains(ir, "@mrt_run"));
  CHECK(contains(ir, "define internal void @ms.add(ptr"));
  CHECK(contains(ir, "call void @mrt_add"));
}

TEST_CASE("codegen interns string literals and diagnostic sites", "[codegen]") {
  std::string ir = generate("print(\"same\"); print(\"same\");");
  size_t first = ir.find("c\"same\\00\"");
  REQUIRE(first != std::string::npos);
  CHECK(ir.find("c\"same\\00\"", first + 1) == std::string::npos);
}

TEST_CASE("codegen keeps embedded NUL bytes in string literals", "[codegen]") {
  std::string ir = generate("print(\"a\\0b\");");
  CHECK(contains(ir, "c\"a\\00b\\00\""));
}

TEST_CASE("codegen releases locals on every exit path", "[codegen]") {
  std::string ir = generate(
      "func f(n) { let s = \"x\"; for (i in range(0, n)) { "
      "let t = s + \"y\"; if (i == 2) { break; } if (i == 1) { continue; } } "
      "if (n == 0) { return s; } return \"z\"; }\nprint(f(3));");
  CHECK(contains(ir, "call void @mrt_release"));
}

TEST_CASE("codegen produces verifiable IR for every conformance program",
          "[codegen]") {
  fs::path root = fs::path(MEADOWS_CONFORMANCE_DIR);
  size_t checked = 0;
  for (const fs::directory_entry& entry :
       fs::recursive_directory_iterator(root)) {
    if (entry.path().extension() != ".ms" ||
        entry.path().parent_path().filename() == "diagnostics" ||
        entry.path().parent_path().filename() == "modules") {
      continue;
    }
    std::string text;
    REQUIRE(readSourceFile(entry.path().string(), text) == ReadResult::OK);
    Analysis analysis = analyzeSource(entry.path().string(), text);
    REQUIRE(analysis.ok());
    CodeGen codegen(analysis.sources);
    std::string ir = codegen.generate(analysis.program);
    INFO(entry.path().string() << ": " << codegen.verificationError());
    CHECK_FALSE(ir.empty());
    ++checked;
  }
  CHECK(checked > 40);
}
