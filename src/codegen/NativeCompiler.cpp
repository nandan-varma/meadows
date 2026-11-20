#include "NativeCompiler.h"

#include <llvm/Support/Program.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <vector>

#include "RuntimeSources.h"

namespace fs = std::filesystem;

namespace meadows {

namespace {

std::string findClang() {
  if (const char* configured = std::getenv("MEADOWS_CLANG")) {
    return configured;
  }
  for (const char* candidate : {"clang-17", "clang", "clang++-17", "clang++"}) {
    if (auto found = llvm::sys::findProgramByName(candidate)) {
      return *found;
    }
  }
  return {};
}

int run(const std::vector<std::string>& command) {
  std::vector<char*> argv;
  for (const std::string& part : command) {
    argv.push_back(const_cast<char*>(part.c_str()));
  }
  argv.push_back(nullptr);

  pid_t child = fork();
  if (child < 0) {
    return -1;
  }
  if (child == 0) {
    execvp(argv[0], argv.data());
    _exit(127);
  }
  int status = 0;
  if (waitpid(child, &status, 0) < 0) {
    return -1;
  }
  return WIFEXITED(status) ? WEXITSTATUS(status) : -1;
}

class ScratchDirectory {
public:
  ScratchDirectory() {
    std::string pattern = (fs::temp_directory_path() / "meadows-XXXXXX").string();
    if (mkdtemp(pattern.data())) {
      path_ = pattern;
    }
  }
  ~ScratchDirectory() {
    if (!path_.empty()) {
      std::error_code ignored;
      fs::remove_all(path_, ignored);
    }
  }
  ScratchDirectory(const ScratchDirectory&) = delete;
  ScratchDirectory& operator=(const ScratchDirectory&) = delete;

  bool valid() const { return !path_.empty(); }
  fs::path path(const std::string& name) const { return fs::path(path_) / name; }

private:
  std::string path_;
};

} // namespace

bool buildExecutable(const NativeBuild& build, std::string& error) {
  std::string clang = findClang();
  if (clang.empty()) {
    error = "clang was not found; install LLVM or set MEADOWS_CLANG";
    return false;
  }

  ScratchDirectory scratch;
  if (!scratch.valid()) {
    error = "cannot create a temporary directory";
    return false;
  }
  for (const RuntimeFile& file : runtimeFiles()) {
    std::ofstream(scratch.path(file.name)) << file.contents;
  }
  fs::path program = scratch.path("program.ll");
  std::ofstream(program) << build.ir;

  int status =
      run({clang, "-O" + std::to_string(build.optLevel), "-pthread", "-w",
           "-x", "ir", program.string(), "-x", "c",
           scratch.path("meadows_rt.c").string(), "-I",
           scratch.path("").string(), "-o", build.output, "-lm"});
  if (status == 127) {
    error = "cannot run '" + clang + "'";
    return false;
  }
  if (status != 0) {
    error = "clang failed with exit status " + std::to_string(status);
    return false;
  }
  return true;
}

} // namespace meadows
