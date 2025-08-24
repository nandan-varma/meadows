#include "testing/TestFramework.h"
#include <iostream>

using namespace meadows::testing;

int main(int argc, char *argv[]) {
  std::string testBasePath = "./tests/";
  std::string categoryFilter = "";
  bool quietMode = false;
  bool minimalMode = false;

  // Accept test path, optional category, and quiet mode as command line
  // arguments
  if (argc > 1) {
    testBasePath = argv[1];
    if (testBasePath.back() != '/') {
      testBasePath += '/';
    }
  }

  if (argc > 2) {
    categoryFilter = argv[2];
    if (categoryFilter == "quiet") {
      quietMode = true;
      categoryFilter = "";
    } else if (categoryFilter == "minimal") {
      minimalMode = true;
      categoryFilter = "";
    } else if (categoryFilter != "quiet" && categoryFilter != "minimal") {
      std::cout << "Filtering tests by category: " << categoryFilter
                << std::endl;
    }
  }

  if (argc > 3) {
    if (std::string(argv[3]) == "quiet") {
      quietMode = true;
    } else if (std::string(argv[3]) == "minimal") {
      minimalMode = true;
    }
  }

  TestFramework testFramework(testBasePath);
  testFramework.setQuietMode(quietMode);
  testFramework.setMinimalMode(minimalMode);

  // Add all test categories
  if (!quietMode && !minimalMode) {
    std::cout << "Setting up comprehensive test suite..." << std::endl;
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

  // Load tests from files (using relative paths that will be combined with
  // testBasePath)
  testFramework.loadTestsFromFile("test_lexer.mds", "Lexer");
  testFramework.loadTestsFromFile("test_parser_expressions.mds", "Expressions");
  testFramework.loadTestsFromFile("test_parser_statements.mds", "Statements");
  testFramework.loadTestsFromFile("test_functions.mds", "Functions");
  testFramework.loadTestsFromFile("test_classes.mds", "Classes");
  testFramework.loadTestsFromFile("test_edge_cases.mds", "Edge Cases");
  testFramework.loadTestsFromFile("comprehensive_test.mds", "Comprehensive");

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
