#include "../ast/AST.h"
#include "../codegen/CodeGen.h"
#include "../config/Config.h"
#include "../lexer/Lexer.h"
#include "../lsp/LSPInterface.h"
#include "../parser/Parser.h"
#include "../types/TypeChecker.h"
#include "../utils/ASTPrinter.h"
#include "../utils/DiagnosticsCollector.h"
#include "../utils/ErrorFormatter.h"
#include "../utils/Timer.h"
#include "../utils/WarningManager.h"
#include <algorithm>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/raw_ostream.h>
#include <string>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

namespace fs = std::filesystem;

using meadows::config::BuildProfile;
using meadows::config::Config;
using meadows::config::LockFile;

std::string getStdlibPath() {
  auto exeDir = std::filesystem::current_path();
  return (exeDir / "src" / "stdlib").string();
}

std::string getStdlibCPath() {
  auto exeDir = std::filesystem::current_path();
  return (exeDir / "src" / "stdlib" / "c" / "meadows_stdlib.c").string();
}

bool validatePathSecurity(const fs::path &filepath, std::string &errorMsg);

constexpr std::uintmax_t MAX_FILE_SIZE = 10 * 1024 * 1024;
constexpr int MAX_FILENAME_LENGTH = 3;
constexpr const char *FILE_EXTENSION = ".ms";

constexpr const char *DANGEROUS_CHARS = ";|&`$(){}[]<>!\\\"'\n\r\t";

bool containsDangerousChars(const std::string &str) {
  return str.find_first_of(DANGEROUS_CHARS) != std::string::npos;
}

bool validatePathSecurity(const fs::path &filepath, std::string &errorMsg);

bool validateInputFile(const std::string &filepath, std::string &errorMsg) {
  fs::path p(filepath);

  if (!validatePathSecurity(p, errorMsg)) {
    return false;
  }

  std::string filename = p.filename().string();

  if (filename.length() < MAX_FILENAME_LENGTH ||
      filename.substr(filename.length() - strlen(FILE_EXTENSION)) !=
          FILE_EXTENSION) {
    errorMsg = "File must have .ms extension";
    return false;
  }

  if (!fs::exists(filepath)) {
    errorMsg = "File does not exist: " + filepath;
    return false;
  }

  if (!fs::is_regular_file(filepath)) {
    errorMsg = "Path is not a regular file: " + filepath;
    return false;
  }

  try {
    auto size = fs::file_size(filepath);
    if (size > MAX_FILE_SIZE) {
      errorMsg = "File too large (max 10MB)";
      return false;
    }
  } catch (const fs::filesystem_error &e) {
    errorMsg = "Cannot read file size: " + std::string(e.what());
    return false;
  }

  return true;
}

bool validateOutputFilename(const std::string &filename,
                            std::string &errorMsg) {
  // Check for dangerous characters (command injection)
  if (containsDangerousChars(filename)) {
    errorMsg = "Invalid characters in output filename";
    return false;
  }

  // Check for path traversal in the entire path
  if (filename.find("..") != std::string::npos) {
    errorMsg = "Invalid path in output filename";
    return false;
  }

  return true;
}

bool validatePathSecurity(const fs::path &filepath, std::string &errorMsg) {
  std::string pathStr = filepath.string();

  if (containsDangerousChars(pathStr)) {
    errorMsg = "Invalid characters in file path";
    return false;
  }

  if (pathStr.find("..") != std::string::npos) {
    errorMsg = "Path traversal not allowed (..)";
    return false;
  }

  return true;
}

int compileWithClang(const std::string &inputFile,
                     const std::string &outputFile) {
  pid_t pid = fork();
  if (pid == 0) {
    std::string stdlibPath = getStdlibCPath();
    const char *args[] = {"clang++", inputFile.c_str(),  stdlibPath.c_str(),
                          "-o",      outputFile.c_str(), nullptr};
    execvp("clang++", const_cast<char *const *>(args));
    _exit(127);
  } else if (pid > 0) {
    int status;
    pid_t result = waitpid(pid, &status, 0);
    if (result == -1) {
      return -1;
    }
    if (WIFEXITED(status)) {
      return WEXITSTATUS(status);
    }
    return -1;
  } else {
    return -1;
  }
}

void printHelp() {
  std::cout << "Meadows Compiler v1.0.0\n\n";
  std::cout << "USAGE:\n";
  std::cout << "  meadows [COMMAND] [OPTIONS] [ARGS...]\n\n";
  std::cout << "COMMANDS:\n";
  std::cout
      << "  build              Build the project (requires meadows.toml)\n";
  std::cout << "  run                Build and run the project\n";
  std::cout << "  test               Run tests\n";
  std::cout
      << "  init               Create a new project in current directory\n";
  std::cout << "  <file.ms>          Compile a single file (legacy mode)\n\n";
  std::cout << "OPTIONS:\n";
  std::cout << "  -h, --help         Show this help message\n";
  std::cout << "  -v, --verbose      Show compilation phases and timing\n";
  std::cout << "  --release          Build in release mode\n";
  std::cout << "  --debug            Build in debug mode (default)\n";
  std::cout << "  --dump-ast         Print AST and exit\n";
  std::cout << "  --dump-ir          Print LLVM IR and exit\n";
  std::cout << "  --lsp-diagnostics <file>  Output LSP diagnostics as JSON\n";
  std::cout << "\nWARNING OPTIONS:\n";
  std::cout << "  -Wall              Enable all common warnings\n";
  std::cout << "  -Wextra            Enable extra warnings\n";
  std::cout << "  -Werror            Treat warnings as errors\n";
  std::cout << "  -Wno-<warning>     Disable specific warning\n";
  std::cout << "\nEXAMPLES:\n";
  std::cout << "  meadows build                    Build project in current "
               "directory\n";
  std::cout << "  meadows build --release          Build in release mode\n";
  std::cout << "  meadows run                      Build and run the project\n";
  std::cout << "  meadows test                     Run all tests\n";
  std::cout << "  meadows lint <file>              Lint a file\n";
  std::cout << "  meadows init                     Initialize new project\n";
  std::cout
      << "  meadows program.ms               Compile single file (legacy)\n";
}

int cmdInit([[maybe_unused]] bool verbose) {
  std::string configPath = "./meadows.toml";

  // Check if already initialized
  std::ifstream checkFile(configPath);
  if (checkFile.good()) {
    std::cerr << "Error: Project already initialized (meadows.toml exists)"
              << std::endl;
    return 1;
  }
  checkFile.close();

  // Get current directory name as default project name
  char cwdBuf[1024];
  std::string projectName = "my-project";
  if (getcwd(cwdBuf, sizeof(cwdBuf)) != nullptr) {
    std::string cwd(cwdBuf);
    size_t pos = cwd.find_last_of("/\\");
    if (pos != std::string::npos && pos + 1 < cwd.length()) {
      projectName = cwd.substr(pos + 1);
    }
  }

  // Create meadows.toml
  std::ofstream configFile(configPath);
  if (!configFile.is_open()) {
    std::cerr << "Error: Cannot create meadows.toml" << std::endl;
    return 1;
  }

  configFile << "[project]\n";
  configFile << "name = \"" << projectName << "\"\n";
  configFile << "version = \"0.1.0\"\n";
  configFile << "edition = \"2024\"\n";
  configFile << "\n";
  configFile << "[build]\n";
  configFile << "target = \"native\"\n";
  configFile << "opt-level = 2\n";
  configFile << "debug = true\n";
  configFile << "entry-point = \"src/main.ms\"\n";
  configFile << "output-dir = \"build\"\n";
  configFile << "\n";
  configFile << "[dependencies]\n";
  configFile << "# Add dependencies here\n";
  configFile << "# example = \"1.0.0\"\n";

  configFile.close();

  // Create src directory and main.ms
  std::error_code ec;
  if (!fs::create_directories("src", ec)) {
    std::cerr << "Warning: Could not create src directory: " << ec.message()
              << std::endl;
  } else {
    std::ofstream mainFile("src/main.ms");
    if (mainFile.is_open()) {
      mainFile << "# " << projectName << " main entry point\n";
      mainFile << "\n";
      mainFile << "func main() {\n";
      mainFile << "    print \"Hello from " << projectName << "!\";\n";
      mainFile << "}\n";
      mainFile.close();
    }
  }

  std::cout << "Initialized new Meadows project: " << projectName << std::endl;
  std::cout << "Created: meadows.toml, src/main.ms" << std::endl;

  return 0;
}

int cmdBuild(bool verbose, bool releaseMode) {
  Config config;

  if (!config.loadFromCurrentDirectory()) {
    std::cerr << "Error: No meadows.toml found in current directory or parents"
              << std::endl;
    std::cerr << "Run 'meadows init' to create a new project" << std::endl;
    return 1;
  }

  std::string validationError = config.validate();
  if (!validationError.empty()) {
    std::cerr << "Error: Invalid configuration: " << validationError
              << std::endl;
    return 1;
  }

  if (verbose) {
    std::cout << "Building project: " << config.project.name << " v"
              << config.project.version << std::endl;
    std::cout << "Profile: " << (releaseMode ? "release" : "debug")
              << std::endl;
  }

  std::string projectRoot = config.getProjectRoot();
  std::string entryPoint = projectRoot + "/" + config.build.entryPoint;
  std::string outputDir = projectRoot + "/" + config.build.outputDir;
  std::string outputFile = outputDir + "/" + config.project.name + ".ll";
  std::string exeFile = outputDir + "/" + config.project.name;

  // Create output directory
  std::error_code ec;
  if (!fs::create_directories(outputDir, ec)) {
    std::cerr << "Error: Cannot create output directory: " << outputDir << ": "
              << ec.message() << std::endl;
    return 1;
  }

  // Check if entry point exists
  std::ifstream entryFile(entryPoint);
  if (!entryFile.is_open()) {
    std::cerr << "Error: Entry point not found: " << entryPoint << std::endl;
    return 1;
  }

  // Read source
  std::string source((std::istreambuf_iterator<char>(entryFile)),
                     std::istreambuf_iterator<char>());
  entryFile.close();

  // Compile using the existing logic
  meadows::DiagnosticsCollector diagnostics;

  try {
    // Lexical analysis
    if (verbose) {
      std::cerr << "[lex] Tokenizing..." << std::endl;
    }

    Lexer lexer(source);
    std::vector<Token> tokens;
    try {
      tokens = lexer.tokenize();
    } catch (const std::exception &e) {
      std::cerr << "Lexical error: " << e.what() << std::endl;
      return 1;
    }

    // Parsing
    if (verbose) {
      std::cerr << "[parse] Parsing..." << std::endl;
    }

    Parser parser(tokens);
    auto statements = parser.parse();

    if (diagnostics.hasErrors()) {
      std::cerr << "Parse errors found" << std::endl;
      return 1;
    }

    // Code generation
    if (verbose) {
      std::cerr << "[codegen] Generating LLVM IR..." << std::endl;
    }

    CodeGen codegen;
    try {
      codegen.generate(statements);
    } catch (const std::runtime_error &e) {
      std::cerr << "Code generation error: " << e.what() << std::endl;
      return 1;
    }

    auto module = codegen.getModule();
    if (!module) {
      std::cerr << "Error: Failed to generate LLVM module" << std::endl;
      return 1;
    }

    // Verify and write LLVM IR
    std::string verifyError;
    llvm::raw_string_ostream verifyStream(verifyError);
    if (llvm::verifyModule(*module, &verifyStream)) {
      std::cerr << "LLVM module verification failed: " << verifyError
                << std::endl;
      return 1;
    }

    std::error_code EC;
    llvm::raw_fd_ostream out(outputFile, EC);
    if (EC) {
      std::cerr << "Error: Cannot write to " << outputFile << std::endl;
      return 1;
    }
    module->print(out, nullptr);
    out.close();

    // Compile to executable
    int ret = compileWithClang(outputFile, exeFile);
    if (ret != 0) {
      std::cerr << "Error: clang++ compilation failed" << std::endl;
      return 1;
    }

    if (verbose) {
      std::cout << "Built successfully: " << exeFile << std::endl;
    } else {
      std::cout << "   Compiling " << config.project.name << " v"
                << config.project.version << std::endl;
    }

  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}

int cmdRun(bool verbose, bool releaseMode) {
  int buildResult = cmdBuild(verbose, releaseMode);
  if (buildResult != 0) {
    return buildResult;
  }

  Config config;
  config.loadFromCurrentDirectory();

  std::string projectRoot = config.getProjectRoot();
  std::string exeFile =
      projectRoot + "/" + config.build.outputDir + "/" + config.project.name;

  if (verbose) {
    std::cout << "Running: " << exeFile << std::endl;
  }

  // Execute the program using fork+exec (secure alternative to system())
  pid_t pid = fork();
  if (pid == 0) {
    // Child process
    char *args[] = {const_cast<char *>(exeFile.c_str()), nullptr};
    execvp(exeFile.c_str(), args);
    _exit(127); // execvp only returns on error
  } else if (pid > 0) {
    // Parent process - wait for child
    int status;
    if (waitpid(pid, &status, 0) == -1) {
      return -1;
    }
    if (WIFEXITED(status)) {
      return WEXITSTATUS(status);
    }
    return -1;
  } else {
    return -1;
  }
}

int cmdTest([[maybe_unused]] bool verbose) {
  std::cout << "Running tests..." << std::endl;
  std::cout << "Note: Test runner not yet fully implemented" << std::endl;
  return 0;
}

int cmdLint(const std::string &filePath) {
  std::cout << "Running linter on " << filePath << "..." << std::endl;

  // Validate input file
  std::string errorMsg;
  if (!validateInputFile(filePath, errorMsg)) {
    std::cerr << "Error: " << errorMsg << std::endl;
    return 1;
  }

  // Open file
  std::ifstream file(filePath);
  if (!file) {
    std::cerr << "Error: Cannot open file " << filePath << std::endl;
    return 1;
  }

  // Read source
  std::string source((std::istreambuf_iterator<char>(file)),
                     std::istreambuf_iterator<char>());

  meadows::DiagnosticsCollector diagnostics;

  try {
    // Lex
    Lexer lexer(source);
    std::vector<Token> tokens = lexer.tokenize();

    // Parse
    Parser parser(tokens, diagnostics);
    parser.setSourcePath(filePath);
    parser.setStdlibPath(getStdlibPath());
    auto statements = parser.parse();

    if (diagnostics.hasErrors()) {
      meadows::ErrorFormatter formatter;
      std::cerr << formatter.formatMultiple(diagnostics.diagnostics(),
                                            filePath);
      return 1;
    }

    // Run lint checks
    int lintWarnings = 0;

    for (const auto &stmt : statements) {
      // Check let statements for uninitialized variables
      if (auto *letStmt = dynamic_cast<LetStmt *>(stmt.get())) {
        if (letStmt->initializer == nullptr) {
          std::cout << "[lint] Warning: Variable '" << letStmt->name
                    << "' declared but not initialized" << std::endl;
          lintWarnings++;
        }
      }

      // Check function statements for empty bodies
      if (auto *funcStmt = dynamic_cast<FuncStmt *>(stmt.get())) {
        if (funcStmt->body.empty()) {
          std::cout << "[lint] Warning: Empty function body '" << funcStmt->name
                    << "'" << std::endl;
          lintWarnings++;
        }
      }
    }

    std::cout << "Linting complete. Found " << lintWarnings << " warnings."
              << std::endl;
    return 0;

  } catch (const std::exception &e) {
    std::cerr << "Error during linting: " << e.what() << std::endl;
    return 1;
  }
}

int compileSingleFile(
    const std::string &filePath, bool verbose, bool dumpAst, bool dumpIr,
    bool lspMode, [[maybe_unused]] meadows::WarningManager &warningManager) {
  // Validate input file
  std::string errorMsg;
  if (!validateInputFile(filePath, errorMsg)) {
    if (lspMode) {
      std::vector<std::string> errors;
      errors.push_back("Validation error: " + errorMsg);
      LSPInterface lsp;
      lsp.emitDiagnostics(filePath, {}, errors);
      return 0;
    }
    std::cerr << "Error: " << errorMsg << std::endl;
    return 1;
  }

  // Open file
  std::ifstream file(filePath);
  if (!file) {
    std::string errorStr = "Cannot open file " + filePath;
    if (lspMode) {
      std::vector<std::string> errors;
      errors.push_back(errorStr);
      LSPInterface lsp;
      lsp.emitDiagnostics(filePath, {}, errors);
      return 0;
    }
    std::cerr << "Error: " << errorStr << std::endl;
    return 1;
  }

  // Read source
  std::string source((std::istreambuf_iterator<char>(file)),
                     std::istreambuf_iterator<char>());

  // In LSP mode, we skip the normal compilation and just validate
  if (lspMode) {
    meadows::DiagnosticsCollector diagnostics;
    std::vector<Token> tokens;

    try {
      Lexer lexer(source);
      tokens = lexer.tokenize();
      Parser parser(tokens, diagnostics);
      auto statements = parser.parse();
    } catch (const std::exception &e) {
      meadows::SourceLocation loc(filePath, 1, 1);
      diagnostics.reportError(meadows::ErrorCode::PARSE_UNEXPECTED_TOKEN,
                              std::string(e.what()), loc);
    }

    LSPInterface lsp;
    lsp.emitDiagnostics(filePath, diagnostics.diagnostics());
    return 0;
  }

  // Normal compilation mode
  std::string outputFile = filePath + ".ll";
  std::string exeFile = filePath + ".out";

  if (!dumpAst && !dumpIr) {
    if (!validateOutputFilename(outputFile, errorMsg) ||
        !validateOutputFilename(exeFile, errorMsg)) {
      std::cerr << "Error: " << errorMsg << std::endl;
      return 1;
    }
  }

  meadows::DiagnosticsCollector diagnostics;

  try {
    meadows::Timer lexTimer, parseTimer, codegenTimer;

    if (verbose) {
      std::cerr << "[lex] Starting lexical analysis..." << std::endl;
      lexTimer.start();
    }

    Lexer lexer(source);
    std::vector<Token> tokens;
    try {
      tokens = lexer.tokenize();
    } catch (const std::exception &e) {
      meadows::SourceLocation loc(filePath, 1, 1);
      diagnostics.reportError(meadows::ErrorCode::LEX_INVALID_CHARACTER,
                              std::string(e.what()), loc);
      meadows::ErrorFormatter formatter;
      std::cerr << formatter.formatMultiple(diagnostics.diagnostics(),
                                            filePath);
      return 1;
    }

    if (verbose) {
      std::cerr << "[lex] Tokenized " << tokens.size() << " tokens"
                << std::endl;
    }

    if (verbose) {
      std::cerr << "[parse] Starting parsing..." << std::endl;
      parseTimer.start();
    }

    Parser parser(tokens, diagnostics);
    parser.setSourcePath(filePath);
    parser.setStdlibPath(getStdlibPath());
    auto statements = parser.parse();

    if (diagnostics.hasErrors()) {
      meadows::ErrorFormatter formatter;
      std::cerr << formatter.formatMultiple(diagnostics.diagnostics(),
                                            filePath);
      return 1;
    }

    if (verbose) {
      std::cerr << "[parse] Parsed " << statements.size() << " statements"
                << std::endl;
    }

    if (verbose) {
      std::cerr << "[typecheck] Starting type checking..." << std::endl;
    }

    try {
      meadows::types::TypeChecker typeChecker;
      if (!typeChecker.check(statements)) {
        std::cerr << "Type checking failed:" << std::endl;
        for (const auto &err : typeChecker.getErrors()) {
          std::cerr << "  - " << err << std::endl;
        }
        return 1;
      }
      if (verbose) {
        std::cerr << "[typecheck] Type checking passed" << std::endl;
      }
    } catch (const std::exception &e) {
      if (verbose) {
        std::cerr << "[typecheck] Skipped (exception: " << e.what() << ")"
                  << std::endl;
      }
    }

    if (dumpAst) {
      ASTPrinter printer;
      std::cout << printer.print(statements);
      return 0;
    }

    if (verbose) {
      std::cerr << "[codegen] Starting code generation..." << std::endl;
      codegenTimer.start();
    }

    CodeGen codegen;
    try {
      codegen.generate(statements);
    } catch (const std::runtime_error &e) {
      std::cerr << "Code generation error: " << e.what() << std::endl;
      return 1;
    }

    auto module = codegen.getModule();
    if (!module) {
      std::cerr << "Error: Failed to generate LLVM module" << std::endl;
      return 1;
    }

    if (verbose) {
      std::cerr << "[codegen] Generated LLVM module" << std::endl;
    }

    if (dumpIr) {
      module->print(llvm::outs(), nullptr);
      return 0;
    }

    std::string verifyError;
    llvm::raw_string_ostream verifyStream(verifyError);
    if (llvm::verifyModule(*module, &verifyStream)) {
      std::cerr << "LLVM module verification failed: " << verifyError
                << std::endl;
      return 1;
    }

    std::error_code EC;
    llvm::raw_fd_ostream out(outputFile, EC);
    if (EC) {
      std::cerr << "Error: Cannot open output file " << outputFile << std::endl;
      return 1;
    }
    module->print(out, nullptr);
    out.close();

    int ret = compileWithClang(outputFile, exeFile);
    if (ret == 0) {
      std::cout << "Compiled successfully to " << exeFile << std::endl;
    } else if (ret == 127) {
      std::cerr << "Error: Failed to execute clang++ (not found)" << std::endl;
      return 1;
    } else {
      std::cerr << "Error: Compilation failed with exit code " << ret
                << std::endl;
      return 1;
    }

  } catch (const std::exception &e) {
    std::cerr << "Unexpected error: " << e.what() << std::endl;
    return 1;
  } catch (...) {
    std::cerr << "Unknown critical error occurred" << std::endl;
    return 2;
  }

  return 0;
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    printHelp();
    return 1;
  }

  std::string firstArg = argv[1];

  // Check for commands first
  if (firstArg == "init") {
    bool verbose = (argc > 2 && (std::string(argv[2]) == "-v" ||
                                 std::string(argv[2]) == "--verbose"));
    return cmdInit(verbose);
  }

  if (firstArg == "build") {
    bool verbose = false;
    bool releaseMode = false;
    for (int i = 2; i < argc; i++) {
      std::string arg = argv[i];
      if (arg == "-v" || arg == "--verbose")
        verbose = true;
      if (arg == "--release")
        releaseMode = true;
      if (arg == "--debug")
        releaseMode = false;
    }
    return cmdBuild(verbose, releaseMode);
  }

  if (firstArg == "run") {
    bool verbose = false;
    bool releaseMode = false;
    for (int i = 2; i < argc; i++) {
      std::string arg = argv[i];
      if (arg == "-v" || arg == "--verbose")
        verbose = true;
      if (arg == "--release")
        releaseMode = true;
    }
    return cmdRun(verbose, releaseMode);
  }

  if (firstArg == "test") {
    bool verbose = false;
    for (int i = 2; i < argc; i++) {
      if (std::string(argv[i]) == "-v" || std::string(argv[i]) == "--verbose")
        verbose = true;
    }
    return cmdTest(verbose);
  }

  // Lint command - analyze code without compilation
  if (firstArg == "lint") {
    if (argc < 3) {
      std::cerr << "Error: lint requires a file path" << std::endl;
      return 1;
    }
    std::string filePath = argv[2];
    return cmdLint(filePath);
  }

  // Handle legacy single-file compilation
  bool lspMode = false;
  bool verbose = false;
  bool dumpAst = false;
  bool dumpIr = false;
  std::string filePath;
  meadows::WarningManager warningManager;

  for (int i = 1; i < argc; i++) {
    std::string arg = argv[i];

    if (arg == "-h" || arg == "--help") {
      printHelp();
      return 0;
    } else if (arg == "-v" || arg == "--verbose") {
      verbose = true;
    } else if (arg == "--dump-ast") {
      dumpAst = true;
    } else if (arg == "--dump-ir") {
      dumpIr = true;
    } else if (arg == "--lsp-diagnostics") {
      if (i + 1 >= argc) {
        std::cerr << "Error: --lsp-diagnostics requires a file path"
                  << std::endl;
        return 1;
      }
      lspMode = true;
      filePath = argv[++i];
    } else if (arg == "-Wall") {
      warningManager.setLevel(meadows::WarningManager::Level::ALL);
    } else if (arg == "-Wextra") {
      warningManager.setLevel(meadows::WarningManager::Level::EXTRA);
    } else if (arg == "-Werror") {
      warningManager.setTreatAsErrors(true);
    } else if (arg.substr(0, 5) == "-Wno-") {
      std::string warningName = arg.substr(5);
      if (warningName == "unused-variable") {
        warningManager.disableWarning(meadows::ErrorCode::WARN_UNUSED_VARIABLE);
      } else if (warningName == "unreachable-code") {
        warningManager.disableWarning(
            meadows::ErrorCode::WARN_UNREACHABLE_CODE);
      }
    } else if (arg[0] != '-') {
      filePath = arg;
    } else {
      std::cerr << "Error: Unknown option " << arg << std::endl;
      std::cerr << "Use -h or --help for usage information" << std::endl;
      return 1;
    }
  }

  if (filePath.empty()) {
    std::cerr << "Error: No input file specified" << std::endl;
    std::cerr << "Use -h or --help for usage information" << std::endl;
    return 1;
  }

  return compileSingleFile(filePath, verbose, dumpAst, dumpIr, lspMode,
                           warningManager);
}
