#include "../codegen/CodeGen.h"
#include "../lexer/Lexer.h"
#include "../parser/Parser.h"
#include <algorithm>
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

  if (filename.length() < 3 ||
      filename.substr(filename.length() - 3) != ".ms") {
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

int main(int argc, char *argv[]) {
  if (argc != 2) {
    std::cerr << "Usage: meadows <file.ms>" << std::endl;
    return 1;
  }

  // Validate input file
  std::string errorMsg;
  if (!validateInputFile(argv[1], errorMsg)) {
    std::cerr << "Error: " << errorMsg << std::endl;
    return 1;
  }

  // Open file
  std::ifstream file(argv[1]);
  if (!file) {
    std::cerr << "Error: Cannot open file " << argv[1] << std::endl;
    return 1;
  }

  // Read source
  std::string source((std::istreambuf_iterator<char>(file)),
                     std::istreambuf_iterator<char>());

  // Validate output filenames
  std::string outputFile = std::string(argv[1]) + ".ll";
  std::string exeFile = std::string(argv[1]) + ".out";

  if (!validateOutputFilename(outputFile, errorMsg) ||
      !validateOutputFilename(exeFile, errorMsg)) {
    std::cerr << "Error: " << errorMsg << std::endl;
    return 1;
  }

  try {
    // Lexical analysis
    Lexer lexer(source);
    std::vector<Token> tokens;
    try {
      tokens = lexer.tokenize();
    } catch (const std::exception &e) {
      std::cerr << "Lexical error: " << e.what() << std::endl;
      return 1;
    }

    // Parsing
    Parser parser(tokens);
    std::vector<std::unique_ptr<Stmt>> statements;
    try {
      statements = parser.parse();
    } catch (const std::runtime_error &e) {
      std::cerr << "Parse error: " << e.what() << std::endl;
      return 1;
    }

    // Code generation
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
