#include "testing/TestFramework.h"
#include <iostream>

using namespace meadows::testing;

int main(int argc, char* argv[]) {
    std::string testBasePath = "./tests/";
    std::string categoryFilter = "";
    bool quietMode = false;
    
    // Accept test path, optional category, and quiet mode as command line arguments
    if (argc > 1) {
        testBasePath = argv[1];
        if (testBasePath.back() != '/') {
            testBasePath += '/';
        }
    }
    
    if (argc > 2) {
        categoryFilter = argv[2];
        if (categoryFilter != "quiet") {
            std::cout << "Filtering tests by category: " << categoryFilter << std::endl;
        }
    }
    
    if (argc > 3 && std::string(argv[3]) == "quiet") {
        quietMode = true;
    } else if (argc > 2 && std::string(argv[2]) == "quiet") {
        quietMode = true;
        categoryFilter = "";
    }
    
    TestFramework testFramework(testBasePath);
    testFramework.setQuietMode(quietMode);
    
    // Add all test categories
    if (!quietMode) {
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
    
    // Load tests from files (using relative paths that will be combined with testBasePath)
    testFramework.loadTestsFromFile("test_lexer.py", "Lexer");
    testFramework.loadTestsFromFile("test_parser_expressions.py", "Expressions");
    testFramework.loadTestsFromFile("test_parser_statements.py", "Statements");
    testFramework.loadTestsFromFile("test_functions.py", "Functions");
    testFramework.loadTestsFromFile("test_classes.py", "Classes");
    testFramework.loadTestsFromFile("test_edge_cases.py", "Edge Cases");
    testFramework.loadTestsFromFile("comprehensive_test.py", "Comprehensive");
    
    // Load new comprehensive test files
    testFramework.loadTestsFromFile("test_compilation.py", "Compilation");
    testFramework.loadTestsFromFile("test_error_handling.py", "Error Handling");
    testFramework.loadTestsFromFile("test_performance.py", "Performance");
    testFramework.loadTestsFromFile("test_ir_generation.py", "IR Generation");
    testFramework.loadTestsFromFile("test_integration.py", "Integration");
    testFramework.loadTestsFromFile("test_applications.py", "Applications");
    
    // Run tests - either all or specific category
    if (!categoryFilter.empty()) {
        testFramework.runTestCategory(categoryFilter);
    } else {
        testFramework.runAllTests();
        
        // Generate HTML report for full test runs (only if not in quiet mode)
        if (!quietMode) {            
            // Print summary
            testFramework.printResults();
        }
    }
    
    return testFramework.allTestsPassed() ? 0 : 1;
}
