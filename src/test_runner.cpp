#include "testing/TestFramework.h"
#include <iostream>
#include <filesystem>
#include <cstdlib>

using namespace meadows::testing;

void showUsage(const char* programName) {
    std::cout << "Meadows Compiler Test Runner\n";
    std::cout << "Usage: " << programName << " [options]\n\n";
    std::cout << "Options:\n";
    std::cout << "  -h, --help         Show this help message\n";
    std::cout << "  -q, --quiet        Run in quiet mode (minimal output)\n";
    std::cout << "  -m, --minimal      Run in minimal mode (very brief output)\n";
    std::cout << "  -c, --category <name>  Run only tests in specified category\n";
    std::cout << "  -p, --path <path>  Set test base path (default: ./tests/)\n";
    std::cout << "  --build-check      Check if build is needed and exit with status\n";
    std::cout << "\nExamples:\n";
    std::cout << "  " << programName << "                    # Run all tests\n";
    std::cout << "  " << programName << " -q                 # Run quietly\n";
    std::cout << "  " << programName << " -c \"Lexer\"         # Run only Lexer tests\n";
    std::cout << "  " << programName << " -p /custom/path/   # Use custom test path\n";
}

bool checkBuildStatus() {
    // Check if build directory and executable exist
    if (!std::filesystem::exists("build")) {
        std::cout << "BUILD_NEEDED: No build directory found\n";
        return false;
    }
    
    if (!std::filesystem::exists("build/meadows")) {
        std::cout << "BUILD_NEEDED: meadows executable not found\n";
        return false;
    }
    
    if (!std::filesystem::exists("build/meadows_test")) {
        std::cout << "BUILD_NEEDED: meadows_test executable not found\n";
        return false;
    }
    
    // Check if CMakeLists.txt is newer than the build
    if (std::filesystem::exists("CMakeLists.txt")) {
        auto cmakelists_time = std::filesystem::last_write_time("CMakeLists.txt");
        auto executable_time = std::filesystem::last_write_time("build/meadows_test");
        if (cmakelists_time > executable_time) {
            std::cout << "BUILD_NEEDED: CMakeLists.txt is newer than build\n";
            return false;
        }
    }
    
    std::cout << "BUILD_OK: Build is up to date\n";
    return true;
}

int main(int argc, char *argv[]) {
  std::string testBasePath = "./tests/";
  std::string categoryFilter = "";
  bool quietMode = false;
  bool minimalMode = false;
  bool buildCheckOnly = false;

  // Parse command line arguments
  for (int i = 1; i < argc; i++) {
    std::string arg = argv[i];
    
    if (arg == "-h" || arg == "--help") {
      showUsage(argv[0]);
      return 0;
    } else if (arg == "-q" || arg == "--quiet") {
      quietMode = true;
    } else if (arg == "-m" || arg == "--minimal") {
      minimalMode = true;
    } else if (arg == "--build-check") {
      buildCheckOnly = true;
    } else if ((arg == "-c" || arg == "--category") && i + 1 < argc) {
      categoryFilter = argv[++i];
    } else if ((arg == "-p" || arg == "--path") && i + 1 < argc) {
      testBasePath = argv[++i];
      if (testBasePath.back() != '/') {
        testBasePath += '/';
      }
    } else if (i == 1 && arg.find('-') != 0) {
      // Backward compatibility: first non-option argument is test path
      testBasePath = arg;
      if (testBasePath.back() != '/') {
        testBasePath += '/';
      }
    } else if (i == 2 && arg.find('-') != 0) {
      // Backward compatibility: second argument could be category or mode
      if (arg == "quiet") {
        quietMode = true;
      } else if (arg == "minimal") {
        minimalMode = true;
      } else {
        categoryFilter = arg;
      }
    } else if (i == 3 && arg.find('-') != 0) {
      // Backward compatibility: third argument for mode
      if (arg == "quiet") {
        quietMode = true;
      } else if (arg == "minimal") {
        minimalMode = true;
      }
    }
  }

  // Handle build check mode
  if (buildCheckOnly) {
    return checkBuildStatus() ? 0 : 1;
  }

  // Initialize test framework
  TestFramework testFramework(testBasePath);
  testFramework.setQuietMode(quietMode);
  testFramework.setMinimalMode(minimalMode);

  // Add status message for non-quiet modes
  if (!quietMode && !minimalMode) {
    std::cout << "=== Meadows Compiler Test Suite ===" << std::endl;
    std::cout << "Test base path: " << testBasePath << std::endl;
    if (!categoryFilter.empty()) {
      std::cout << "Category filter: " << categoryFilter << std::endl;
    }
    std::cout << "Setting up comprehensive test suite..." << std::endl;
  } else if (!categoryFilter.empty() && !quietMode) {
    std::cout << "Filtering tests by category: " << categoryFilter << std::endl;
  }

  // Basic language tests
  testFramework.addLexerTests();
  testFramework.addExpressionTests();
  testFramework.addStatementTests();
  testFramework.addFunctionTests();
  testFramework.addClassTests();
  testFramework.addErrorTests();
  testFramework.addIntegrationTests();

  // Advanced comprehensive tests
  testFramework.addCompilationTests();
  testFramework.addErrorHandlingTests();
  testFramework.addPerformanceTests();
  testFramework.addIRGenerationTests();
  testFramework.addApplicationTests();

  // New comprehensive test categories
  testFramework.addErrorRecoveryTests();
  testFramework.addSemanticTests();
  testFramework.addAdvancedLanguageTests();
  testFramework.addStressTests();
  testFramework.addBoundaryTests();
  testFramework.addRealWorldTests();
  testFramework.addEdgeCaseTests();

  // Load tests from files (using relative paths that will be combined with
  // testBasePath)
  testFramework.loadTestsFromFile("test_lexer.mds", "Lexer");
  testFramework.loadTestsFromFile("test_parser_expressions.mds", "Expressions");
  testFramework.loadTestsFromFile("test_parser_statements.mds", "Statements");
  testFramework.loadTestsFromFile("test_functions.mds", "Functions");
  testFramework.loadTestsFromFile("test_classes.mds", "Classes");
  testFramework.loadTestsFromFile("test_edge_cases.mds", "Edge Cases");
  testFramework.loadTestsFromFile("comprehensive_test.mds", "Comprehensive");
  
  // Load additional test files
  testFramework.loadTestsFromFile("test_control_flow.mds", "Control Flow");
  testFramework.loadTestsFromFile("test_data_types.mds", "Data Types");
  testFramework.loadTestsFromFile("test_precedence.mds", "Precedence");

  // Load new comprehensive test files
  testFramework.loadTestsFromFile("test_compilation.mds", "Compilation");
  testFramework.loadTestsFromFile("test_error_handling.mds", "Error Handling");
  testFramework.loadTestsFromFile("test_performance.mds", "Performance");
  testFramework.loadTestsFromFile("test_ir_generation.mds", "IR Generation");
  testFramework.loadTestsFromFile("test_integration.mds", "Integration");
  testFramework.loadTestsFromFile("test_applications.mds", "Applications");

  // Run tests - either all or specific category
  if (!categoryFilter.empty()) {
    testFramework.runTestCategory(categoryFilter);
  } else {
    testFramework.runAllTests();

    // Generate HTML report for full test runs (only if not in quiet mode or
    // minimal mode)
    if (!quietMode && !minimalMode) {
      // Print summary
      testFramework.printResults();
    }
  }

  return testFramework.allTestsPassed() ? 0 : 1;
}
