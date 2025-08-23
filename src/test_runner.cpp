#include "testing/TestFramework.h"
#include <iostream>

using namespace meadows::testing;

int main() {
    TestFramework testFramework;
    
    // Add all test categories
    std::cout << "Setting up test suite..." << std::endl;
    
    testFramework.addLexerTests();
    testFramework.addExpressionTests();
    testFramework.addStatementTests();
    testFramework.addFunctionTests();
    testFramework.addClassTests();
    testFramework.addErrorTests();
    testFramework.addIntegrationTests();
    
    // Load tests from files
    testFramework.loadTestsFromFile("tests/test_lexer.py", "lexer");
    testFramework.loadTestsFromFile("tests/test_parser_expressions.py", "expressions");
    testFramework.loadTestsFromFile("tests/test_parser_statements.py", "statements");
    testFramework.loadTestsFromFile("tests/test_functions.py", "functions");
    testFramework.loadTestsFromFile("tests/test_classes.py", "classes");
    testFramework.loadTestsFromFile("tests/test_edge_cases.py", "edge_cases");
    testFramework.loadTestsFromFile("tests/comprehensive_test.py", "comprehensive");
    
    // Run all tests
    testFramework.runAllTests();
    
    return testFramework.allTestsPassed() ? 0 : 1;
}
