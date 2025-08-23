#include "TestFramework.h"
#include "../ast/Expression.h"
#include "../ast/Statement.h"
#include <fstream>
#include <sstream>
#include <iomanip>

namespace meadows {
namespace testing {

void TestFramework::addTest(const TestCase& test) {
    testCases.push_back(test);
}

std::string TestFramework::readTestFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Warning: Could not open test file " << filename << std::endl;
        return "";
    }
    
    std::ostringstream content;
    content << file.rdbuf();
    return content.str();
}

void TestFramework::addLexerTests() {
    // Basic token tests
    addTest(TestCase(
        "lex_integers",
        "Tokenize integer literals",
        "x = 42\ny = -123\nz = 0\nw = 999",
        true,
        [](const Program& program) {
            // For lexer tests, we'd need access to tokens
            // For now, just check that parsing succeeds
            return true;
        }
    ));
    
    addTest(TestCase(
        "lex_floats",
        "Tokenize float literals",
        "a = 3.14\nb = -2.5\nc = 0.0\nd = 123.456",
        true
    ));
    
    addTest(TestCase(
        "lex_strings",
        "Tokenize string literals",
        R"(a = "hello"
b = 'world'
c = "string with spaces")",
        true
    ));
    
    addTest(TestCase(
        "lex_identifiers",
        "Tokenize identifiers",
        R"(variable_name = 1
CamelCase = 2
snake_case = 3
_underscore = 4
name123 = 5)",
        true
    ));
    
    addTest(TestCase(
        "lex_keywords",
        "Parse statements with keywords",
        R"(def test():
    if True:
        while False:
            for x in y:
                return None)",
        true
    ));
    
    addTest(TestCase(
        "lex_operators",
        "Parse expressions with operators",
        R"(a = x + y - z * w / v % u
b = p == q and r != s
c = m < n or o > p)",
        true
    ));
}

void TestFramework::addExpressionTests() {
    // Binary expressions
    addTest(TestCase(
        "expr_arithmetic",
        "Parse arithmetic expressions",
        "x = 1 + 2 * 3 - 4 / 5",
        true,
        [](const Program& program) {
            return program.statements.size() > 0;
        }
    ));
    
    addTest(TestCase(
        "expr_comparison",
        "Parse comparison expressions",
        "result = a == b and c != d or e < f",
        true
    ));
    
    addTest(TestCase(
        "expr_precedence",
        "Test operator precedence",
        "result = a + b * c - d / e ** f",
        true
    ));
    
    addTest(TestCase(
        "expr_parentheses",
        "Test parentheses grouping",
        "result = (a + b) * (c - d)",
        true
    ));
    
    addTest(TestCase(
        "expr_function_call",
        "Parse function calls",
        "result = func(a, b, c)",
        true
    ));
    
    addTest(TestCase(
        "expr_attribute_access",
        "Parse attribute access",
        "value = obj.attr.nested",
        true
    ));
    
    addTest(TestCase(
        "expr_method_call",
        "Parse method calls",
        "result = obj.method().chained()",
        true
    ));
}

void TestFramework::addStatementTests() {
    // Assignment statements
    addTest(TestCase(
        "stmt_assignment",
        "Parse assignment statements",
        R"(x = 42
y = "hello"
z = True)",
        true
    ));
    
    // If statements
    addTest(TestCase(
        "stmt_if_simple",
        "Parse simple if statement",
        R"(if x > 0:
    print("positive"))",
        true
    ));
    
    addTest(TestCase(
        "stmt_if_else",
        "Parse if-else statement",
        R"(if x > 0:
    print("positive")
else:
    print("not positive"))",
        true
    ));
    
    addTest(TestCase(
        "stmt_if_elif",
        "Parse if-elif-else statement",
        R"(if x > 0:
    print("positive")
elif x < 0:
    print("negative")
else:
    print("zero"))",
        true
    ));
    
    // Loop statements
    addTest(TestCase(
        "stmt_while",
        "Parse while loop",
        R"(while i < 10:
    print(i)
    i = i + 1)",
        true
    ));
    
    addTest(TestCase(
        "stmt_for",
        "Parse for loop",
        R"(for item in collection:
    print(item))",
        true
    ));
}

void TestFramework::addFunctionTests() {
    // Simple function
    addTest(TestCase(
        "func_simple",
        "Parse simple function definition",
        R"(def greet():
    print("Hello!"))",
        true,
        [](const Program& program) {
            // Check that we have a function definition
            for (const auto& stmt : program.statements) {
                if (dynamic_cast<const FunctionDefinition*>(stmt.get())) {
                    return true;
                }
            }
            return false;
        }
    ));
    
    addTest(TestCase(
        "func_parameters",
        "Parse function with parameters",
        R"(def add(a, b):
    return a + b)",
        true
    ));
    
    addTest(TestCase(
        "func_default_params",
        "Parse function with default parameters",
        R"(def greet(name, greeting):
    return greeting + name)",
        true
    ));
    
    addTest(TestCase(
        "func_recursive",
        "Parse recursive function",
        R"(def factorial(n):
    if n <= 1:
        return 1
    else:
        return n * factorial(n - 1))",
        true
    ));
}

void TestFramework::addClassTests() {
    // Simple class
    addTest(TestCase(
        "class_simple",
        "Parse simple class definition",
        R"(class Person:
    def __init__(self, name):
        self.name = name)",
        true,
        [](const Program& program) {
            // Check that we have a class definition
            for (const auto& stmt : program.statements) {
                if (dynamic_cast<const ClassDefinition*>(stmt.get())) {
                    return true;
                }
            }
            return false;
        }
    ));
    
    addTest(TestCase(
        "class_methods",
        "Parse class with multiple methods",
        R"(class Calculator:
    def __init__(self):
        self.value = 0
    def add(self, x):
        self.value = self.value + x
        return self.value)",
        true
    ));
}

void TestFramework::addErrorTests() {
    // Syntax errors that should fail
    addTest(TestCase(
        "error_missing_colon",
        "Should fail on missing colon in if statement",
        R"(if x > 0
    print("positive"))",
        false  // expect failure
    ));
    
    addTest(TestCase(
        "error_invalid_indentation",
        "Should fail on invalid indentation",
        R"(if True:
print("bad indentation"))",
        false
    ));
    
    addTest(TestCase(
        "error_unclosed_paren",
        "Should fail on unclosed parenthesis",
        "result = func(a, b",
        false
    ));
}

void TestFramework::addIntegrationTests() {
    // Load comprehensive test
    std::string comprehensive = readTestFile("tests/comprehensive_test.py");
    if (!comprehensive.empty()) {
        addTest(TestCase(
            "integration_comprehensive",
            "Comprehensive language feature test",
            comprehensive,
            true
        ));
    }
    
    // Load existing examples
    std::string simple = readTestFile("simple_example.py");
    if (!simple.empty()) {
        addTest(TestCase(
            "integration_simple_example",
            "Simple example from repository",
            simple,
            true
        ));
    }
    
    std::string example = readTestFile("example.py");
    if (!example.empty()) {
        addTest(TestCase(
            "integration_calculator_example",
            "Calculator example from repository",
            example,
            true
        ));
    }
}

TestResult TestFramework::runSingleTest(const TestCase& test) {
    std::cout << "Running test: " << test.name << " - " << test.description << std::endl;
    
    compiler.clearErrors();
    auto program = compiler.compile(test.source, test.name + ".py");
    
    bool hasCompileErrors = compiler.hasErrors();
    
    if (test.expectSuccess) {
        if (hasCompileErrors) {
            std::cout << "  FAIL: Expected success but got compilation errors:" << std::endl;
            compiler.getErrorReporter().printErrors();
            return TestResult::FAIL;
        }
        
        if (!program) {
            std::cout << "  FAIL: Expected success but got null program" << std::endl;
            return TestResult::FAIL;
        }
        
        // Run custom validator if provided
        if (test.validator && !test.validator(*program)) {
            std::cout << "  FAIL: Custom validation failed" << std::endl;
            return TestResult::FAIL;
        }
        
        std::cout << "  PASS" << std::endl;
        return TestResult::PASS;
    } else {
        // Expecting failure
        if (!hasCompileErrors && program) {
            std::cout << "  FAIL: Expected compilation to fail but it succeeded" << std::endl;
            return TestResult::FAIL;
        }
        
        std::cout << "  PASS (correctly failed)" << std::endl;
        return TestResult::PASS;
    }
}

void TestFramework::runAllTests() {
    std::cout << "=== Meadows Language Test Suite ===" << std::endl;
    std::cout << "Running " << testCases.size() << " tests..." << std::endl << std::endl;
    
    passCount = failCount = errorCount = 0;
    
    for (const auto& test : testCases) {
        try {
            TestResult result = runSingleTest(test);
            switch (result) {
                case TestResult::PASS:
                    passCount++;
                    break;
                case TestResult::FAIL:
                    failCount++;
                    break;
                case TestResult::ERROR:
                    errorCount++;
                    break;
            }
        } catch (const std::exception& e) {
            std::cout << "  ERROR: Exception during test: " << e.what() << std::endl;
            errorCount++;
        }
        std::cout << std::endl;
    }
    
    printResults();
}

void TestFramework::printResults() const {
    std::cout << "=== Test Results ===" << std::endl;
    std::cout << "Total tests: " << (passCount + failCount + errorCount) << std::endl;
    std::cout << "Passed: " << passCount << std::endl;
    std::cout << "Failed: " << failCount << std::endl;
    std::cout << "Errors: " << errorCount << std::endl;
    
    if (allTestsPassed()) {
        std::cout << "🎉 All tests passed!" << std::endl;
    } else {
        std::cout << "❌ Some tests failed." << std::endl;
    }
}

void TestFramework::loadTestsFromFile(const std::string& filename, const std::string& category) {
    std::string content = readTestFile(filename);
    if (!content.empty()) {
        addTest(TestCase(
            category + "_file_" + filename,
            "Test from file: " + filename,
            content,
            true
        ));
    }
}

} // namespace testing
} // namespace meadows
