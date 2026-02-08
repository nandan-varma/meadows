#include "../codegen/CodeGen.h"
#include "../lexer/Lexer.h"
#include "../lsp/LSPInterface.h"
#include "../parser/Parser.h"
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
    // Child process - execute clang++
    // Using execvp for security (no shell interpolation)
    const char *args[] = {"clang++", inputFile.c_str(), "-o",
                          outputFile.c_str(), nullptr};
    execvp("clang++", const_cast<char *const *>(args));
    // If we get here, execvp failed
    _exit(127);
  } else if (pid > 0) {
    // Parent process - wait for child
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
    // Fork failed
    return -1;
  }
}

void printHelp() {
  std::cout << "Meadows Compiler v1.0.0\n\n";
  std::cout << "Usage: meadows [OPTIONS] <file.ms>\n\n";
  std::cout << "Options:\n";
  std::cout << "  -h, --help                Show this help message\n";
  std::cout
      << "  -v, --verbose             Show compilation phases and timing\n";
  std::cout << "  --dump-ast                Print AST and exit\n";
  std::cout << "  --dump-ir                 Print LLVM IR and exit\n";
  std::cout << "  --lsp-diagnostics <file>  Output LSP diagnostics as JSON\n";
  std::cout << "\nWarning Options:\n";
  std::cout << "  -Wall                     Enable all common warnings\n";
  std::cout << "  -Wextra                   Enable extra warnings\n";
  std::cout << "  -Werror                   Treat warnings as errors\n";
  std::cout << "  -Wno-<warning>            Disable specific warning\n";
  std::cout << "\nExamples:\n";
  std::cout << "  meadows program.ms              Compile program.ms\n";
  std::cout
      << "  meadows -v program.ms           Compile with verbose output\n";
  std::cout << "  meadows --dump-ast program.ms   Print AST tree\n";
  std::cout << "  meadows --dump-ir program.ms    Print LLVM IR\n";
  std::cout << "  meadows -Wall program.ms        Enable all warnings\n";
  std::cout << "  meadows -Wall -Werror prog.ms   Treat warnings as errors\n";
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    printHelp();
    return 1;
  }

  // Parse command-line options
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
      // Disable specific warning
      std::string warningName = arg.substr(5);
      // Map warning names to codes
      if (warningName == "unused-variable") {
        warningManager.disableWarning(meadows::ErrorCode::WARN_UNUSED_VARIABLE);
      } else if (warningName == "unreachable-code") {
        warningManager.disableWarning(
            meadows::ErrorCode::WARN_UNREACHABLE_CODE);
      }
    } else if (arg[0] != '-') {
      // This is the input file
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

  // Validate input file
  std::string errorMsg;
  if (!validateInputFile(filePath, errorMsg)) {
    if (lspMode) {
      // Output JSON error for LSP mode
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
      // Lexical analysis
      Lexer lexer(source);
      tokens = lexer.tokenize();

      // Parsing with error recovery - collect multiple errors
      Parser parser(tokens, diagnostics);
      auto statements = parser.parse();

    } catch (const std::exception &e) {
      // Handle any unexpected errors
      meadows::SourceLocation loc(filePath, 1, 1);
      diagnostics.reportError(meadows::ErrorCode::PARSE_UNEXPECTED_TOKEN,
                              std::string(e.what()), loc);
    }

    // Output LSP diagnostics
    LSPInterface lsp;
    lsp.emitDiagnostics(filePath, diagnostics.diagnostics());
    return 0;
  }

  // Normal compilation mode
  // Validate output filenames
  std::string outputFile = filePath + ".ll";
  std::string exeFile = filePath + ".out";

  if (!dumpAst && !dumpIr) {
    if (!validateOutputFilename(outputFile, errorMsg) ||
        !validateOutputFilename(exeFile, errorMsg)) {
      std::cerr << "Error: " << errorMsg << std::endl;
      return 1;
    }
  }

  // Use diagnostics collector for better error reporting
  meadows::DiagnosticsCollector diagnostics;

  try {
    // Track timing if verbose mode
    meadows::Timer lexTimer, parseTimer, codegenTimer;

    // Lexical analysis
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
      // Format and display error
      meadows::ErrorFormatter formatter;
      std::cerr << formatter.formatMultiple(diagnostics.diagnostics(),
                                            filePath);
      return 1;
    }

    if (verbose) {
      double elapsed = lexTimer.elapsed();
      std::cerr << "[lex] Tokenized " << tokens.size() << " tokens (" << elapsed
                << "ms)" << std::endl;
    }

    // Parsing with error recovery
    if (verbose) {
      std::cerr << "[parse] Starting parsing..." << std::endl;
      parseTimer.start();
    }

    Parser parser(tokens, diagnostics);
    std::vector<std::unique_ptr<Stmt>> statements;
    statements = parser.parse();

    // Check for parse errors
    if (diagnostics.hasErrors()) {
      meadows::ErrorFormatter formatter;
      std::cerr << formatter.formatMultiple(diagnostics.diagnostics(),
                                            filePath);
      return 1;
    }

    if (verbose) {
      double elapsed = parseTimer.elapsed();
      std::cerr << "[parse] Parsed " << statements.size() << " statements ("
                << elapsed << "ms)" << std::endl;
    }

    // Dump AST if requested
    if (dumpAst) {
      ASTPrinter printer;
      std::cout << printer.print(statements);
      return 0;
    }

    // Code generation
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
      double elapsed = codegenTimer.elapsed();
      std::cerr << "[codegen] Generated LLVM module (" << elapsed << "ms)"
                << std::endl;
    }

    // Dump IR if requested
    if (dumpIr) {
      module->print(llvm::outs(), nullptr);
      return 0;
    }

    // Verify module before writing
    std::string verifyError;
    llvm::raw_string_ostream verifyStream(verifyError);
    if (llvm::verifyModule(*module, &verifyStream)) {
      std::cerr << "LLVM module verification failed: " << verifyError
                << std::endl;
      return 1;
    }

    // Write LLVM IR
    std::error_code EC;
    llvm::raw_fd_ostream out(outputFile, EC);
    if (EC) {
      std::cerr << "Error: Cannot open output file " << outputFile << std::endl;
      return 1;
    }
    module->print(out, nullptr);
    out.close();

    // Compile to executable using secure method
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
