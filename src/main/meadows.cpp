#include "../codegen/CodeGen.h"
#include "../lexer/Lexer.h"
#include "../lsp/LSPInterface.h"
#include "../parser/Parser.h"
#include "../sema/SemanticAnalyzer.h"
#include "../utils/ASTPrinter.h"
#include "../utils/DiagnosticsCollector.h"
#include "../utils/ErrorFormatter.h"
#include "../utils/Timer.h"
#include "../utils/WarningManager.h"
#include <CLI/CLI.hpp>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/Program.h>
#include <llvm/Support/raw_ostream.h>
#include <string>
#include <unordered_map>
#include <vector>
#ifdef _WIN32
#include <process.h>
#else
#include <sys/wait.h>
#include <unistd.h>
#endif

namespace fs = std::filesystem;

// ── Constants ─────────────────────────────────────────────────────────────────

static constexpr std::uintmax_t MAX_FILE_SIZE = 10 * 1024 * 1024;
static constexpr const char *FILE_EXTENSION = ".ms";

// Table-driven warning name → ErrorCode mapping
static const std::unordered_map<std::string, meadows::ErrorCode> WARNING_NAMES = {
    {"unused-variable",   meadows::ErrorCode::WARN_UNUSED_VARIABLE},
    {"unused-function",   meadows::ErrorCode::WARN_UNUSED_FUNCTION},
    {"unreachable-code",  meadows::ErrorCode::WARN_UNREACHABLE_CODE},
    {"shadowing",         meadows::ErrorCode::WARN_SHADOWING_VARIABLE},
    {"deprecated",        meadows::ErrorCode::WARN_DEPRECATED_FEATURE},
    {"div-by-zero",       meadows::ErrorCode::WARN_DIVISION_BY_ZERO},
    {"array-bounds",      meadows::ErrorCode::WARN_ARRAY_BOUNDS},
};

// ── Path validation ───────────────────────────────────────────────────────────

static bool hasDangerousChars(const std::string &s) {
  // Excludes () so paths like "Program Files (x86)" are valid.
  // Shell metacharacters are harmless here because we use exec-style
  // invocation (never system()), but we still reject obvious injection chars.
  static constexpr const char *DANGEROUS = ";|&`${}[]<>!\\\"'\n\r\t";
  return s.find_first_of(DANGEROUS) != std::string::npos;
}

static bool validateInputFile(const std::string &path, std::string &err) {
  if (hasDangerousChars(path)) { err = "Invalid characters in file path"; return false; }
  if (path.find("..") != std::string::npos) { err = "Path traversal not allowed (..)"; return false; }

  fs::path p(path);
  auto ext = p.extension().string();
  if (ext != FILE_EXTENSION) { err = "File must have .ms extension"; return false; }
  if (!fs::exists(p)) { err = "File does not exist: " + path; return false; }
  if (!fs::is_regular_file(p)) { err = "Not a regular file: " + path; return false; }

  try {
    if (fs::file_size(p) > MAX_FILE_SIZE) { err = "File too large (max 10 MB)"; return false; }
  } catch (const fs::filesystem_error &e) {
    err = "Cannot read file size: " + std::string(e.what());
    return false;
  }
  return true;
}

static bool validateOutputPath(const std::string &path, std::string &err) {
  if (hasDangerousChars(path)) { err = "Invalid characters in output path"; return false; }
  if (path.find("..") != std::string::npos) { err = "Path traversal not allowed (..)"; return false; }
  return true;
}

// ── Clang invocation ──────────────────────────────────────────────────────────

static std::string findClangPlusPlus() {
  // Explicit override via environment variable
  if (const char *env = std::getenv("MEADOWS_CLANG"))
    return env;

  // Try versioned names first so we pick the right LLVM version
  for (const char *candidate :
       {"clang++-17", "clang++-18", "clang++-16", "clang++"}) {
    auto found = llvm::sys::findProgramByName(candidate);
    if (found)
      return std::move(*found);
  }
  return {};
}

static int compileWithClang(const std::string &llFile, const std::string &outFile) {
  std::string clangPath = findClangPlusPlus();
  if (clangPath.empty()) return 127;

#ifdef _WIN32
  const char *args[] = {clangPath.c_str(), llFile.c_str(), "-o", outFile.c_str(), nullptr};
  int ret = static_cast<int>(_spawnvp(_P_WAIT, clangPath.c_str(), args));
  return (ret == -1) ? -1 : ret;
#else
  pid_t pid = fork();
  if (pid < 0) return -1;
  if (pid == 0) {
    const char *args[] = {clangPath.c_str(), llFile.c_str(), "-o", outFile.c_str(), nullptr};
    execvp(clangPath.c_str(), const_cast<char *const *>(args));
    _exit(127);
  }
  int status = 0;
  if (waitpid(pid, &status, 0) == -1) return -1;
  return WIFEXITED(status) ? WEXITSTATUS(status) : -1;
#endif
}

// ── Warning flag processing ───────────────────────────────────────────────────

static void applyWarningFlags(const std::vector<std::string> &flags,
                               meadows::WarningManager &wm) {
  for (const auto &f : flags) {
    if (f == "-Wall")        wm.setLevel(meadows::WarningManager::Level::ALL);
    else if (f == "-Wextra") wm.setLevel(meadows::WarningManager::Level::EXTRA);
    else if (f == "-Werror") wm.setTreatAsErrors(true);
    else if (f.size() > 4 && f.substr(0, 4) == "-Wno") {
      // Accept both "-Wno-name" and "-Wno name"
      std::string name = (f[4] == '-') ? f.substr(5) : f.substr(4);
      auto it = WARNING_NAMES.find(name);
      if (it != WARNING_NAMES.end()) wm.disableWarning(it->second);
      else std::cerr << "Warning: Unknown warning name '" << name << "'\n";
    }
  }
}

// ── main ──────────────────────────────────────────────────────────────────────

int main(int argc, char *argv[]) {
  CLI::App app{"Meadows Compiler", "meadows"};
  app.set_version_flag("-V,--version", MEADOWS_VERSION);

  std::string inputFile;
  std::string outputFile;
  bool verbose = false;
  bool dumpAst = false;
  bool dumpIr = false;
  bool lspMode = false;
  int optLevel = 0;

  app.add_option("file", inputFile, "Source file (.ms)")->required();
  app.add_option("-o,--output", outputFile, "Output executable path");
  app.add_flag("-v,--verbose", verbose, "Show compilation phases and timing");
  app.add_flag("--dump-ast", dumpAst, "Print AST and exit");
  app.add_flag("--dump-ir",  dumpIr,  "Print LLVM IR and exit");
  app.add_flag("--lsp-diagnostics", lspMode,
               "Output JSON diagnostics for LSP clients");
  app.add_option("-O,--opt-level", optLevel, "Optimization level (0-3)")
      ->check(CLI::Range(0, 3));

  // Collect -W* flags before CLI11 sees them (they aren't standard options)
  std::vector<std::string> warningFlags;
  std::vector<char *> filteredArgv;
  filteredArgv.push_back(argv[0]);
  for (int i = 1; i < argc; ++i) {
    std::string a = argv[i];
    if (a == "-Wall" || a == "-Wextra" || a == "-Werror" ||
        (a.size() > 2 && a[0] == '-' && a[1] == 'W')) {
      warningFlags.push_back(a);
    } else {
      filteredArgv.push_back(argv[i]);
    }
  }
  int filteredArgc = static_cast<int>(filteredArgv.size());

  CLI11_PARSE(app, filteredArgc, filteredArgv.data());

  meadows::WarningManager warningManager;
  applyWarningFlags(warningFlags, warningManager);

  // ── LSP mode ─────────────────────────────────────────────────────────────────

  if (lspMode) {
    LSPInterface lsp;
    std::ifstream f(inputFile);
    if (!f) {
      lsp.emitDiagnostics(inputFile, {}, {"Cannot open file: " + inputFile});
      return 0;
    }
    std::string src((std::istreambuf_iterator<char>(f)),
                    std::istreambuf_iterator<char>());
    meadows::DiagnosticsCollector diag;
    try {
      Lexer lexer(src);
      auto tokens = lexer.tokenize();
      Parser parser(tokens, diag);
      auto stmts = parser.parse();
      if (!diag.hasErrors()) {
        meadows::SemanticAnalyzer sema(diag, warningManager);
        sema.analyze(stmts);
      }
    } catch (const std::exception &e) {
      meadows::SourceLocation loc(inputFile, 1, 1);
      diag.reportError(meadows::ErrorCode::PARSE_UNEXPECTED_TOKEN,
                       e.what(), loc);
    }
    lsp.emitDiagnostics(inputFile, diag.diagnostics());
    return 0;
  }

  // ── Validate input ────────────────────────────────────────────────────────────

  std::string errMsg;
  if (!validateInputFile(inputFile, errMsg)) {
    std::cerr << "Error: " << errMsg << "\n";
    return 1;
  }

  // Determine output paths
  fs::path inPath(inputFile);
  std::string llFile = inPath.string() + ".ll";
  if (outputFile.empty())
    outputFile = inPath.stem().string() + ".out";

  if (!dumpAst && !dumpIr) {
    if (!validateOutputPath(llFile, errMsg) ||
        !validateOutputPath(outputFile, errMsg)) {
      std::cerr << "Error: " << errMsg << "\n";
      return 1;
    }
  }

  // ── Read source ───────────────────────────────────────────────────────────────

  std::ifstream file(inputFile);
  if (!file) {
    std::cerr << "Error: Cannot open " << inputFile << "\n";
    return 1;
  }
  std::string source((std::istreambuf_iterator<char>(file)),
                     std::istreambuf_iterator<char>());

  meadows::DiagnosticsCollector diag;
  meadows::ErrorFormatter formatter;

  try {
    // ── Lex ────────────────────────────────────────────────────────────────────

    meadows::Timer lexTimer;
    if (verbose) { std::cerr << "[lex] Starting...\n"; lexTimer.start(); }

    Lexer lexer(source);
    std::vector<Token> tokens;
    try {
      tokens = lexer.tokenize();
    } catch (const std::exception &e) {
      meadows::SourceLocation loc(inputFile, 1, 1);
      diag.reportError(meadows::ErrorCode::LEX_INVALID_CHARACTER, e.what(), loc);
      std::cerr << formatter.formatMultiple(diag.diagnostics(), inputFile);
      return 1;
    }

    if (verbose)
      std::cerr << "[lex] " << tokens.size() << " tokens ("
                << lexTimer.elapsed() << " ms)\n";

    // ── Parse ──────────────────────────────────────────────────────────────────

    meadows::Timer parseTimer;
    if (verbose) { std::cerr << "[parse] Starting...\n"; parseTimer.start(); }

    Parser parser(tokens, diag);
    auto stmts = parser.parse();

    if (diag.hasErrors()) {
      std::cerr << formatter.formatMultiple(diag.diagnostics(), inputFile);
      return 1;
    }

    if (verbose)
      std::cerr << "[parse] " << stmts.size() << " statements ("
                << parseTimer.elapsed() << " ms)\n";

    if (dumpAst) {
      ASTPrinter printer;
      std::cout << printer.print(stmts);
      return 0;
    }

    // ── Semantic analysis ──────────────────────────────────────────────────────

    meadows::Timer semaTimer;
    if (verbose) { std::cerr << "[sema] Starting...\n"; semaTimer.start(); }

    meadows::SemanticAnalyzer sema(diag, warningManager);
    sema.analyze(stmts);

    // Emit warnings even when continuing
    for (const auto &d : diag.diagnostics()) {
      if (d.severity == "warning") {
        std::cerr << formatter.formatMultiple({d}, inputFile);
      }
    }

    if (diag.hasErrors()) {
      std::cerr << formatter.formatMultiple(diag.diagnostics(), inputFile);
      return 1;
    }

    if (verbose)
      std::cerr << "[sema] Done (" << semaTimer.elapsed() << " ms)\n";

    // ── Code generation ────────────────────────────────────────────────────────

    meadows::Timer codegenTimer;
    if (verbose) { std::cerr << "[codegen] Starting...\n"; codegenTimer.start(); }

    CodeGen codegen(optLevel);
    try {
      codegen.generate(stmts);
    } catch (const std::exception &e) {
      std::cerr << "Code generation error: " << e.what() << "\n";
      return 1;
    }

    auto module = codegen.getModule();
    if (!module) {
      std::cerr << "Error: Failed to generate LLVM module\n";
      return 1;
    }

    if (verbose)
      std::cerr << "[codegen] Done (" << codegenTimer.elapsed() << " ms)\n";

    if (dumpIr) {
      module->print(llvm::outs(), nullptr);
      return 0;
    }

    // Verify module
    std::string verifyErr;
    llvm::raw_string_ostream vs(verifyErr);
    if (llvm::verifyModule(*module, &vs)) {
      std::cerr << "LLVM module verification failed: " << verifyErr << "\n";
      return 1;
    }

    // Write .ll to temp location
    std::error_code ec;
    llvm::raw_fd_ostream out(llFile, ec);
    if (ec) {
      std::cerr << "Error: Cannot write " << llFile << "\n";
      return 1;
    }
    module->print(out, nullptr);
    out.close();

    // Compile to executable
    int ret = compileWithClang(llFile, outputFile);
    fs::remove(llFile); // clean up intermediate .ll

    if (ret == 0) {
      if (verbose) std::cout << "Compiled to " << outputFile << "\n";
    } else if (ret == 127) {
      std::cerr << "Error: clang++ not found\n";
      return 1;
    } else {
      std::cerr << "Error: clang++ exited with code " << ret << "\n";
      return 1;
    }

  } catch (const std::exception &e) {
    std::cerr << "Unexpected error: " << e.what() << "\n";
    return 1;
  }

  return 0;
}
