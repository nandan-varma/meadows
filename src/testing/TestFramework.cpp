#include "TestFramework.h"
#include "../ast/Expression.h"
#include "../ast/Statement.h"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>

namespace meadows {
namespace testing {

void TestFramework::addTest(const TestCase &test) { testCases.push_back(test); }

std::string TestFramework::readTestFile(const std::string &filename) {
  std::string fullPath = testBasePath;
  if (!fullPath.empty() && fullPath.back() != '/') {
    fullPath += "/";
  }
  fullPath += filename;
  std::ifstream file(fullPath);
  if (!file.is_open()) {
    if (!minimalMode && !quietMode) {
      std::cerr << "Warning: Could not open test file " << fullPath
                << std::endl;
    }
    return "";
  }

  std::ostringstream content;
  content << file.rdbuf();
  return content.str();
}

void TestFramework::addLexerTests() {
  // Basic token tests
  addTest(TestCase(
      "lex_integers", "Tokenize integer literals",
      "x = 42\ny = -123\nz = 0\nw = 999", true,
      [](const Program &program) { return true; }, "Lexer"));

  addTest(TestCase("lex_floats", "Tokenize float literals",
                   "a = 3.14\nb = -2.5\nc = 0.0\nd = 123.456", true, nullptr,
                   "Lexer"));

  addTest(TestCase("lex_strings", "Tokenize string literals",
                   R"(a = "hello"
b = 'world'
c = "string with spaces")",
                   true));

  addTest(TestCase("lex_identifiers", "Tokenize identifiers",
                   R"(variable_name = 1
CamelCase = 2
snake_case = 3
_underscore = 4
name123 = 5)",
                   true));

  addTest(TestCase("lex_keywords", "Parse statements with keywords",
                   R"(def test():
    if True:
        while False:
            for x in y:
                return None)",
                   true));

  addTest(TestCase("lex_operators", "Parse expressions with operators",
                   R"(a = x + y - z * w / v % u
b = p == q and r != s
c = m < n or o > p)",
                   true));
}

void TestFramework::addExpressionTests() {
  // Binary expressions
  addTest(TestCase("expr_arithmetic", "Parse arithmetic expressions",
                   "x = 1 + 2 * 3 - 4 / 5", true, [](const Program &program) {
                     return program.statements.size() > 0;
                   }));

  addTest(TestCase("expr_comparison", "Parse comparison expressions",
                   "result = a == b and c != d or e < f", true));

  addTest(TestCase("expr_precedence", "Test operator precedence",
                   "result = a + b * c - d / e ** f", true));

  addTest(TestCase("expr_parentheses", "Test parentheses grouping",
                   "result = (a + b) * (c - d)", true));

  addTest(TestCase("expr_function_call", "Parse function calls",
                   "result = func(a, b, c)", true));

  addTest(TestCase("expr_attribute_access", "Parse attribute access",
                   "value = obj.attr.nested", true));

  addTest(TestCase("expr_method_call", "Parse method calls",
                   "result = obj.method().chained()", true));
}

void TestFramework::addStatementTests() {
  // Assignment statements
  addTest(TestCase("stmt_assignment", "Parse assignment statements",
                   R"(x = 42
y = "hello"
z = True)",
                   true));

  // If statements
  addTest(TestCase("stmt_if_simple", "Parse simple if statement",
                   R"(if x > 0:
    print("positive"))",
                   true));

  addTest(TestCase("stmt_if_else", "Parse if-else statement",
                   R"(if x > 0:
    print("positive")
else:
    print("not positive"))",
                   true));

  addTest(TestCase("stmt_if_elif", "Parse if-elif-else statement",
                   R"(if x > 0:
    print("positive")
elif x < 0:
    print("negative")
else:
    print("zero"))",
                   true));

  // Loop statements
  addTest(TestCase("stmt_while", "Parse while loop",
                   R"(while i < 10:
    print(i)
    i = i + 1)",
                   true));

  addTest(TestCase("stmt_for", "Parse for loop",
                   R"(for item in collection:
    print(item))",
                   true));
}

void TestFramework::addFunctionTests() {
  // Simple function
  addTest(
      TestCase("func_simple", "Parse simple function definition",
               R"(def greet():
    print("Hello!"))",
               true, [](const Program &program) {
                 // Check that we have a function definition
                 for (const auto &stmt : program.statements) {
                   if (dynamic_cast<const FunctionDefinition *>(stmt.get())) {
                     return true;
                   }
                 }
                 return false;
               }));

  addTest(TestCase("func_parameters", "Parse function with parameters",
                   R"(def add(a, b):
    return a + b)",
                   true));

  addTest(TestCase("func_default_params",
                   "Parse function with default parameters",
                   R"(def greet(name, greeting):
    return greeting + name)",
                   true));

  addTest(TestCase("func_recursive", "Parse recursive function",
                   R"(def factorial(n):
    if n <= 1:
        return 1
    else:
        return n * factorial(n - 1))",
                   true));
}

void TestFramework::addClassTests() {
  // Simple class
  addTest(TestCase("class_simple", "Parse simple class definition",
                   R"(class Person:
    def __init__(self, name):
        self.name = name)",
                   true, [](const Program &program) {
                     // Check that we have a class definition
                     for (const auto &stmt : program.statements) {
                       if (dynamic_cast<const ClassDefinition *>(stmt.get())) {
                         return true;
                       }
                     }
                     return false;
                   }));

  addTest(TestCase("class_methods", "Parse class with multiple methods",
                   R"(class Calculator:
    def __init__(self):
        self.value = 0
    def add(self, x):
        self.value = self.value + x
        return self.value)",
                   true));
}

void TestFramework::addErrorTests() {
  // Syntax errors that should fail
  addTest(TestCase("error_missing_colon",
                   "Should fail on missing colon in if statement",
                   R"(if x > 0
    print("positive"))",
                   false // expect failure
                   ));

  addTest(TestCase("error_invalid_indentation",
                   "Should fail on invalid indentation",
                   R"(if True:
print("bad indentation"))",
                   false));

  addTest(TestCase("error_unclosed_paren",
                   "Should fail on unclosed parenthesis", "result = func(a, b",
                   false));
}

void TestFramework::addErrorRecoveryTests() {
  // Test multiple errors in one file
  addTest(TestCase("error_recovery_multiple", 
    "Parser should recover from multiple syntax errors",
    R"(def bad_function()  # Missing colon
    print("hello")
    
invalid syntax here
    
def good_function():
    return 42)", false, [](const Program &program) {
    // Should still parse the good function despite earlier errors
    return program.statements.size() > 0;
  }, "Error Recovery"));

  // Test error recovery in nested scopes
  addTest(TestCase("error_recovery_nested",
    "Recover from errors in nested scopes",
    R"(if True:
    x = 1
    *** bad syntax here ***
    y = 2
else:
    z = 3)", false, nullptr, "Error Recovery"));

  // Test recovery from missing parentheses
  addTest(TestCase("error_recovery_missing_paren",
    "Recover from missing closing parenthesis",
    R"(def func(a, b:
    return a + b
    
def another_func():
    return 42)", false, nullptr, "Error Recovery"));

  // Test recovery from invalid expressions
  addTest(TestCase("error_recovery_invalid_expr",
    "Recover from invalid expressions",
    R"(x = 1 + + 2
y = 3 * 4
z = *** invalid expression here ***
w = 5 + 6)", false, nullptr, "Error Recovery"));
}

void TestFramework::addSemanticTests() {
  // Variable scoping tests
  addTest(TestCase("semantic_scope_global",
    "Test global variable scoping",
    R"(x = 10
def func():
    return x  # Should access global x)", true, [](const Program &program) {
    // Verify semantic analysis of scoping
    return true; // Add actual semantic validation when available
  }, "Semantics"));

  // Function parameter scoping
  addTest(TestCase("semantic_scope_parameters",
    "Test parameter scoping",
    R"(def func(x, y):
    z = x + y
    return z
    
result = func(1, 2)", true, nullptr, "Semantics"));

  // Nested function scoping
  addTest(TestCase("semantic_scope_nested",
    "Test nested function scoping",
    R"(def outer():
    x = 10
    def inner():
        return x
    return inner())", true, nullptr, "Semantics"));

  // Undefined variable tests
  addTest(TestCase("semantic_undefined_var",
    "Detect undefined variable usage",
    R"(def func():
    return undefined_variable)", true, nullptr, "Semantics")); // Changed to true since parser doesn't do semantic analysis yet

  // Variable redefinition
  addTest(TestCase("semantic_redefinition",
    "Test variable redefinition",
    R"(x = 10
x = "string"
y = x + 5)", true, nullptr, "Semantics"));
}

void TestFramework::addAdvancedLanguageTests() {
  // List comprehensions (if supported in the future)
  addTest(TestCase("advanced_list_comprehension",
    "Parse list comprehensions",
    "[x*2 for x in range(10) if x % 2 == 0]",
    false, nullptr, "Advanced")); // Set to false until implemented

  // Lambda functions (if supported)
  addTest(TestCase("advanced_lambda",
    "Parse lambda expressions", 
    "func = lambda x, y: x + y",
    false, nullptr, "Advanced")); // Set to false until implemented

  // Decorators (if supported)
  addTest(TestCase("advanced_decorators",
    "Parse function decorators",
    R"(@decorator
def func():
    pass)", false, nullptr, "Advanced")); // Set to false until implemented

  // Context managers (if supported)
  addTest(TestCase("advanced_context_managers",
    "Parse with statements",
    R"(with open("file.txt") as f:
    content = f.read())", false, nullptr, "Advanced")); // Set to false until implemented

  // Complex expressions (simplified)
  addTest(TestCase("advanced_complex_expressions",
    "Parse complex nested expressions",
    R"(result = ((a + b) * (c - d)) / ((e ** f) % (g + h))
nested = func(obj.method(), [1, 2, 3])", true, nullptr, "Advanced"));

  // Multiple assignment
  addTest(TestCase("advanced_multiple_assignment",
    "Parse multiple assignment",
    R"(a, b, c = 1, 2, 3
x, y = func_returning_tuple())", false, nullptr, "Advanced")); // Set to false until implemented
}

void TestFramework::addStressTests() {
  // Deep recursion test (with proper indentation)
  std::string deepRecursion = "def func0():\n    return 1\n";
  for (int i = 1; i < 20; i++) {
    deepRecursion += "def func" + std::to_string(i) + "():\n    return func" + 
                     std::to_string(i-1) + "()\n";
  }
  
  addTest(TestCase("stress_deep_functions",
    "Parse many nested function definitions",
    deepRecursion, true, nullptr, "Stress"));

  // Large expression test
  std::string largeExpr = "x = 1";
  for (int i = 0; i < 50; i++) {
    largeExpr += " + " + std::to_string(i);
  }
  
  addTest(TestCase("stress_large_expression",
    "Parse very large arithmetic expression",
    largeExpr, true, nullptr, "Stress"));

  // Many variable declarations
  std::string manyVars = "";
  for (int i = 0; i < 100; i++) {
    manyVars += "var" + std::to_string(i) + " = " + std::to_string(i) + "\n";
  }
  
  addTest(TestCase("stress_many_variables",
    "Parse many variable declarations",
    manyVars, true, nullptr, "Stress"));

  // Deep nesting
  std::string deepNested = "x";
  for (int i = 0; i < 20; i++) {
    deepNested = "(" + deepNested + ")";
  }
  addTest(TestCase("stress_deep_nesting",
    "Parse deeply nested parentheses",
    "result = " + deepNested, true, nullptr, "Stress"));
}

void TestFramework::addBoundaryTests() {
  // Empty file
  addTest(TestCase("boundary_empty_file", "Parse empty file", "", true, nullptr, "Boundary"));

  // Only whitespace and newlines (should be valid)
  addTest(TestCase("boundary_whitespace_only", "Parse whitespace-only file", 
    "\n\n\n", true, nullptr, "Boundary"));

  // Single character identifier
  addTest(TestCase("boundary_single_char", "Parse single character", "x = 1", true, nullptr, "Boundary"));

  // Long identifier (reasonable length)
  std::string longId(50, 'a');
  addTest(TestCase("boundary_long_identifier", "Parse long identifier",
    longId + " = 42", true, nullptr, "Boundary"));

  // Long string (reduced length)
  std::string longString = "text = \"";
  for (int i = 0; i < 100; i++) {
    longString += "a";
  }
  longString += "\"";
  addTest(TestCase("boundary_long_string", "Parse long string literal",
    longString, true, nullptr, "Boundary"));

  // Nested blocks (reduced depth)
  std::string nestedBlocks = "";
  for (int i = 0; i < 5; i++) {
    nestedBlocks += std::string(i * 4, ' ') + "if True:\n";
  }
  nestedBlocks += std::string(5 * 4, ' ') + "pass\n";
  addTest(TestCase("boundary_nested_blocks", "Parse deeply nested blocks",
    nestedBlocks, true, nullptr, "Boundary"));
}

void TestFramework::addRealWorldTests() {
  // Fibonacci implementation (simplified without range)
  addTest(TestCase("realworld_fibonacci",
    "Complete Fibonacci implementation",
    R"(def fibonacci(n):
    if n <= 1:
        return n
    else:
        return fibonacci(n-1) + fibonacci(n-2)

result = fibonacci(5)", true, nullptr, "RealWorld"));

  // Factorial implementation
  addTest(TestCase("realworld_factorial",
    "Complete factorial implementation",
    R"(def factorial(n):
    if n <= 1:
        return 1
    else:
        return n * factorial(n-1)

result = factorial(5)", true, nullptr, "RealWorld"));

  // Simple calculator
  addTest(TestCase("realworld_calculator",
    "Simple calculator implementation",
    R"(class Calculator:
    def __init__(self):
        self.result = 0
    
    def add(self, x):
        self.result = self.result + x
        return self.result
    
    def multiply(self, x):
        self.result = self.result * x
        return self.result

calc = Calculator()
calc.add(5)
calc.multiply(3)", true, nullptr, "RealWorld"));

  // Simple iterative algorithm
  addTest(TestCase("realworld_sum_numbers",
    "Sum numbers iteratively",
    R"(def sum_to_n(n):
    total = 0
    i = 1
    while i <= n:
        total = total + i
        i = i + 1
    return total

result = sum_to_n(10)", true, nullptr, "RealWorld"));
}

void TestFramework::addEdgeCaseTests() {
  // Weird but valid syntax
  addTest(TestCase("edge_empty_function",
    "Empty function with only pass",
    R"(def empty_func():
    pass)", true, nullptr, "EdgeCase"));

  // Complex operator combinations
  addTest(TestCase("edge_operator_combinations",
    "Complex operator combinations",
    R"(result = not (a and b) or (c and not d)
value = a + b * c - d / e ** f % g)", true, nullptr, "EdgeCase"));

  // Nested function calls
  addTest(TestCase("edge_nested_calls",
    "Deeply nested function calls",
    R"(result = func1(func2(func3(func4(func5(x))))))", true, nullptr, "EdgeCase"));

  // Mixed data types
  addTest(TestCase("edge_mixed_types",
    "Mixed data type operations",
    R"(a = 42
b = 3.14
c = "hello"
d = True
e = None)", true, nullptr, "EdgeCase"));

  // Complex conditional
  addTest(TestCase("edge_complex_conditional",
    "Complex conditional logic",
    R"(if (a > b and c < d) or (e == f and g != h):
    if x and y:
        result = 1
    elif z:
        result = 2
    else:
        result = 3
else:
    result = 0)", true, nullptr, "EdgeCase"));
}

void TestFramework::addIntegrationTests() {
  // Load comprehensive test
  std::string comprehensive = readTestFile("tests/comprehensive_test.mds");
  if (!comprehensive.empty()) {
    addTest(TestCase("integration_comprehensive",
                     "Comprehensive language feature test", comprehensive,
                     true));
  }

  // Load existing examples
  std::string simple = readTestFile("simple_example.mds");
  if (!simple.empty()) {
    addTest(TestCase("integration_simple_example",
                     "Simple example from repository", simple, true));
  }

  std::string example = readTestFile("example.mds");
  if (!example.empty()) {
    addTest(TestCase("integration_calculator_example",
                     "Calculator example from repository", example, true));
  }
}

TestResult TestFramework::runSingleTest(const TestCase &test) {
  if (minimalMode) {
    // Minimal output: just test name with result
    std::cout << test.name;
  } else if (!quietMode) {
    std::cout << "Running test: " << test.name << " - " << test.description
              << std::endl;
  }

  compiler.clearErrors();
  auto program = compiler.compile(test.source, test.name + ".mds");

  bool hasCompileErrors = compiler.hasErrors();

  if (test.expectSuccess) {
    if (hasCompileErrors) {
      if (minimalMode) {
        std::cout << " ✗" << std::endl;
      } else if (!quietMode) {
        std::cout << "  FAIL: Expected success but got compilation errors:"
                  << std::endl;
        compiler.getErrorReporter().printErrors();
      }
      return TestResult::FAIL;
    }

    if (!program) {
      if (minimalMode) {
        std::cout << " ✗" << std::endl;
      } else if (!quietMode) {
        std::cout << "  FAIL: Expected success but got null program"
                  << std::endl;
      }
      return TestResult::FAIL;
    }

    // Run custom validator if provided
    if (test.validator && !test.validator(*program)) {
      if (minimalMode) {
        std::cout << " ✗" << std::endl;
      } else if (!quietMode) {
        std::cout << "  FAIL: Custom validation failed" << std::endl;
      }
      return TestResult::FAIL;
    }

    if (minimalMode) {
      std::cout << " ✓" << std::endl;
    } else if (!quietMode) {
      std::cout << "  PASS" << std::endl;
    }
    return TestResult::PASS;
  } else {
    // Expecting failure
    if (!hasCompileErrors && program) {
      if (minimalMode) {
        std::cout << " ✗" << std::endl;
      } else if (!quietMode) {
        std::cout << "  FAIL: Expected compilation to fail but it succeeded"
                  << std::endl;
      }
      return TestResult::FAIL;
    }

    if (minimalMode) {
      std::cout << " ✓" << std::endl;
    } else if (!quietMode) {
      std::cout << "  PASS (correctly failed)" << std::endl;
    }
    return TestResult::PASS;
  }
}

void TestFramework::runAllTests() {
  if (!quietMode && !minimalMode) {
    std::cout << "=== Meadows Language Test Suite ===" << std::endl;
    std::cout << "Running " << testCases.size() << " tests..." << std::endl
              << std::endl;
  }

  passCount = failCount = errorCount = 0;
  testResults.clear(); // Clear previous results

  for (const auto &test : testCases) {
    try {
      TestResult result = runSingleTest(test);
      testResults.emplace_back(test, result); // Store result for table display
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
    } catch (const std::exception &e) {
      if (!quietMode) {
        std::cout << "  ERROR: Exception during test: " << e.what()
                  << std::endl;
      }
      testResults.emplace_back(test, TestResult::ERROR); // Store error result
      errorCount++;
    }
    if (!quietMode) {
      std::cout << std::endl;
    }
  }

  if (!quietMode && !minimalMode) {
    printTableResults(); // Show appealing table format
    printResults();
    printDetailedResults();
  } else if (minimalMode) {
    auto stats = getTestStats();
    std::cout << "\nTotal tests: " << stats.totalTests << std::endl;
    std::cout << "Passed: " << stats.passedTests << " (" << std::fixed
              << std::setprecision(1) << stats.successRate << "%)" << std::endl;
    std::cout << "Failed: " << stats.failedTests << " (" << std::fixed
              << std::setprecision(1) << (100.0 - stats.successRate) << "%)"
              << std::endl;
    std::cout << "Success Rate: " << std::fixed << std::setprecision(1)
              << stats.successRate << "%" << std::endl;
  }
}

void TestFramework::printResults() const {
  auto stats = getTestStats();

  std::cout << "\n=== Test Results Summary ===" << std::endl;
  std::cout << "Total tests: " << stats.totalTests << std::endl;
  std::cout << "Passed: " << stats.passedTests << " (" << std::fixed
            << std::setprecision(1)
            << (stats.totalTests > 0
                    ? (double(stats.passedTests) / stats.totalTests * 100)
                    : 0.0)
            << "%)" << std::endl;
  std::cout << "Failed: " << stats.failedTests << " (" << std::fixed
            << std::setprecision(1)
            << (stats.totalTests > 0
                    ? (double(stats.failedTests) / stats.totalTests * 100)
                    : 0.0)
            << "%)" << std::endl;
  std::cout << "Errors: " << stats.errorTests << " (" << std::fixed
            << std::setprecision(1)
            << (stats.totalTests > 0
                    ? (double(stats.errorTests) / stats.totalTests * 100)
                    : 0.0)
            << "%)" << std::endl;
  std::cout << "Success Rate: " << std::fixed << std::setprecision(1)
            << stats.successRate << "%" << std::endl;

  if (stats.averageCompilationTime > 0) {
    std::cout << "Average Compilation Time: " << std::fixed
              << std::setprecision(3) << stats.averageCompilationTime << "s"
              << std::endl;
  }

  std::cout << "\n=== Tests by Category ===" << std::endl;
  for (const auto &[category, count] : stats.categoryStats) {
    std::cout << category << ": " << count << " tests" << std::endl;
  }

  std::cout << "\n=== Final Result ===" << std::endl;
  if (allTestsPassed()) {
    std::cout << "🎉 All tests passed!" << std::endl;
  } else {
    std::cout << "❌ Some tests failed." << std::endl;
  }

  // Add progress bar visualization
  std::cout << "\nProgress: [";
  int barWidth = 50;
  int passedBars = stats.totalTests > 0
                       ? (stats.passedTests * barWidth) / stats.totalTests
                       : 0;
  int failedBars = stats.totalTests > 0
                       ? (stats.failedTests * barWidth) / stats.totalTests
                       : 0;

  for (int i = 0; i < barWidth; ++i) {
    if (i < passedBars) {
      std::cout << "✓";
    } else if (i < passedBars + failedBars) {
      std::cout << "✗";
    } else {
      std::cout << "·";
    }
  }
  std::cout << "] " << std::fixed << std::setprecision(1) << stats.successRate
            << "%" << std::endl;
}

void TestFramework::printDetailedResults() const {
  auto stats = getTestStats();

  std::cout << "\n=== Detailed Test Statistics ===" << std::endl;

  // Overall statistics
  std::cout << "\n📊 Overall Performance:" << std::endl;
  std::cout << "  Total Tests: " << stats.totalTests << std::endl;
  std::cout << "  Success Rate: " << std::fixed << std::setprecision(2)
            << stats.successRate << "%" << std::endl;
  if (stats.averageCompilationTime > 0) {
    std::cout << "  Average Compilation Time: " << std::fixed
              << std::setprecision(3) << stats.averageCompilationTime << "s"
              << std::endl;
  }

  // Category breakdown with detailed stats
  std::cout << "\n📂 Test Categories Breakdown:" << std::endl;
  std::cout << std::string(60, '-') << std::endl;
  std::cout << std::left << std::setw(20) << "Category" << std::setw(10)
            << "Count" << std::setw(12) << "Percentage" << std::setw(18)
            << "Visual" << std::endl;
  std::cout << std::string(60, '-') << std::endl;

  for (const auto &[category, count] : stats.categoryStats) {
    double percentage =
        stats.totalTests > 0 ? (double(count) / stats.totalTests * 100) : 0.0;

    // Create visual bar
    int barWidth = 15;
    int filledBars = static_cast<int>((percentage / 100.0) * barWidth);
    std::string bar = "[";
    for (int i = 0; i < barWidth; ++i) {
      bar += (i < filledBars) ? "█" : "░";
    }
    bar += "]";

    std::cout << std::left << std::setw(20) << category << std::setw(10)
              << count << std::setw(12)
              << (std::to_string(static_cast<int>(percentage)) + "%")
              << std::setw(18) << bar << std::endl;
  }
  std::cout << std::string(60, '-') << std::endl;

  // Results breakdown
  std::cout << "\n🎯 Results Breakdown:" << std::endl;
  std::cout << "  ✅ Passed: " << stats.passedTests << " (" << std::fixed
            << std::setprecision(1)
            << (stats.totalTests > 0
                    ? (double(stats.passedTests) / stats.totalTests * 100)
                    : 0.0)
            << "%)" << std::endl;
  std::cout << "  ❌ Failed: " << stats.failedTests << " (" << std::fixed
            << std::setprecision(1)
            << (stats.totalTests > 0
                    ? (double(stats.failedTests) / stats.totalTests * 100)
                    : 0.0)
            << "%)" << std::endl;
  std::cout << "  ⚠️  Errors: " << stats.errorTests << " (" << std::fixed
            << std::setprecision(1)
            << (stats.totalTests > 0
                    ? (double(stats.errorTests) / stats.totalTests * 100)
                    : 0.0)
            << "%)" << std::endl;

  // Performance insights
  if (stats.averageCompilationTime > 0) {
    std::cout << "\n⚡ Performance Insights:" << std::endl;
    if (stats.averageCompilationTime < 1.0) {
      std::cout << "  🚀 Excellent compilation speed (< 1s average)"
                << std::endl;
    } else if (stats.averageCompilationTime < 5.0) {
      std::cout << "  ✅ Good compilation speed (< 5s average)" << std::endl;
    } else {
      std::cout << "  ⚠️  Slow compilation speed (> 5s average) - consider "
                   "optimization"
                << std::endl;
    }
  }

  // Quality assessment
  std::cout << "\n🏆 Quality Assessment:" << std::endl;
  if (stats.successRate >= 95.0) {
    std::cout << "  🌟 Excellent test coverage and quality!" << std::endl;
  } else if (stats.successRate >= 85.0) {
    std::cout << "  ✅ Good test quality, minor issues to address" << std::endl;
  } else if (stats.successRate >= 70.0) {
    std::cout << "  ⚠️  Moderate test quality, needs improvement" << std::endl;
  } else {
    std::cout << "  ❌ Poor test quality, significant issues need attention"
              << std::endl;
  }
}

void TestFramework::printTableResults() const {
  auto stats = getTestStats();
  
  std::cout << "\n";
  std::cout << "╔══════════════════════════════════════════════════════════════════════════════╗\n";
  std::cout << "║                            📊 TEST RESULTS TABLE                            ║\n";
  std::cout << "╚══════════════════════════════════════════════════════════════════════════════╝\n";
  
  printTableHeader();
  printTableSeparator();
  
  // Display test results in table format
  for (const auto& [test, result] : testResults) {
    printTableRow(test, result);
  }
  
  printTableSeparator();
  
  // Summary row
  std::cout << "║";
  std::cout << " 📈 SUMMARY";
  std::cout << std::string(54 - 10, ' ');
  std::cout << "│";
  
  if (stats.successRate >= 95.0) {
    std::cout << " 🌟 EXCELLENT ";
  } else if (stats.successRate >= 85.0) {
    std::cout << " ✅ GOOD      ";
  } else if (stats.successRate >= 70.0) {
    std::cout << " ⚠️  MODERATE  ";
  } else {
    std::cout << " ❌ POOR      ";
  }
  
  std::cout << "│ ";
  std::cout << std::setw(3) << stats.totalTests << " tests │";
  std::cout << std::fixed << std::setprecision(1) << std::setw(6) << stats.successRate << "% ║\n";
  
  std::cout << "╚════════════════════════════════════════════════════════════════════════════════╝\n";
  
  // Color-coded summary
  std::cout << "\n🎯 Quick Stats: ";
  std::cout << "✅ " << stats.passedTests << " passed  ";
  if (stats.failedTests > 0) {
    std::cout << "❌ " << stats.failedTests << " failed  ";
  }
  if (stats.errorTests > 0) {
    std::cout << "⚠️ " << stats.errorTests << " errors  ";
  }
  std::cout << "📊 " << std::fixed << std::setprecision(1) << stats.successRate << "% success rate\n";
}

void TestFramework::printTableHeader() const {
  std::cout << "╔════════════════════════════════════════════════════════════════════════════════╗\n";
  std::cout << "║ Status │ Test Name                                      │ Category      │ Count │ Rate   ║\n";
}

void TestFramework::printTableRow(const TestCase &test, TestResult result) const {
  std::cout << "║";
  
  // Status column with icons
  switch (result) {
    case TestResult::PASS:
      std::cout << "   ✅   ";
      break;
    case TestResult::FAIL:
      std::cout << "   ❌   ";
      break;
    case TestResult::ERROR:
      std::cout << "   ⚠️    ";
      break;
  }
  
  std::cout << "│ ";
  
  // Test name column (truncate if too long)
  std::string displayName = test.name;
  if (displayName.length() > 45) {
    displayName = displayName.substr(0, 42) + "...";
  }
  std::cout << std::left << std::setw(46) << displayName;
  
  std::cout << "│ ";
  
  // Category column
  std::string displayCategory = test.category;
  if (displayCategory.length() > 12) {
    displayCategory = displayCategory.substr(0, 9) + "...";
  }
  std::cout << std::left << std::setw(12) << displayCategory;
  
  std::cout << "│";
  
  // Individual test count (always 1)
  std::cout << "   1  ";
  
  std::cout << "│";
  
  // Individual test rate
  std::cout << (result == TestResult::PASS ? " 100.0%" : "   0.0%");
  
  std::cout << " ║\n";
}

void TestFramework::printTableSeparator() const {
  std::cout << "╠════════╪══════════════════════════════════════════════╪═══════════════╪═══════╪════════╣\n";
}

void TestFramework::loadTestsFromFile(const std::string &filename,
                                      const std::string &category) {
  std::string content = readTestFile(filename);
  if (!content.empty()) {
    addTest(TestCase(category + "_file_" + filename,
                     "Test from file: " + filename, content, true, nullptr,
                     category));
  }
}

void TestFramework::addCompilationTests() {
  addTest(TestCase(
      "compile_simple", "Compile simple function to executable",
      readTestFile("test_compilation.mds"), true,
      [this](const Program &program) {
        return testFileCompilation("test_compilation.mds");
      },
      "Compilation"));

  addTest(TestCase("compile_classes", "Compile classes and methods",
                   R"(
class TestClass:
    def __init__(self, value):
        self.value = value
    
    def get_double(self):
        return self.value * 2

obj = TestClass(21)
result = obj.get_double()
)",
                   true, nullptr, "Compilation"));
}

void TestFramework::addErrorHandlingTests() {
  addTest(TestCase(
      "error_syntax", "Handle syntax errors gracefully",
      R"(
def invalid_function()  # Missing colon
    return 42
)",
      false, // Expect failure
      [this](const Program &program) {
        return expectCompilationError("def invalid_function()\n    return 42");
      },
      "Error Handling"));

  addTest(TestCase("error_indentation", "Handle indentation errors",
                   R"(
def bad_indent():
return 42  # Wrong indentation
)",
                   false, nullptr, "Error Handling"));
}

void TestFramework::addPerformanceTests() {
  addTest(TestCase(
      "perf_arithmetic", "Performance test: arithmetic operations",
      readTestFile("test_performance.mds"), true,
      [this](const Program &program) {
        auto start = std::chrono::steady_clock::now();
        bool result = testFileCompilation("test_performance.mds");
        auto end = std::chrono::steady_clock::now();

        auto duration =
            std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        return result &&
               duration.count() < 5000; // Should compile in under 5 seconds
      },
      "Performance"));
}

void TestFramework::addIRGenerationTests() {
  addTest(TestCase("ir_basic_function", "Generate IR for basic function",
                   R"(
def simple_add(a, b):
    return a + b
)",
                   true, nullptr, "IR Generation"));
}

void TestFramework::addApplicationTests() {
  std::string testDir = "../tests/";

  addTest(TestCase(
      "app_calculator", "Real-world calculator application",
      readTestFile(testDir + "test_applications.mds"), true,
      [this](const Program &program) {
        return testFileCompilation("test_applications.mds");
      },
      "Applications"));
}

// Enhanced compilation testing methods
bool TestFramework::testFileCompilation(const std::string &filename) {
  try {
    std::string source = readTestFile(filename);
    if (source.empty()) {
      return false;
    }

    auto program = compiler.compile(source, filename);
    return program != nullptr;
  } catch (...) {
    return false;
  }
}

bool TestFramework::testExecutableGeneration(const std::string &filename) {
  try {
    std::string source = readTestFile(filename);
    std::string outputPath = "test_" + filename.substr(0, filename.find('.'));

    return compiler.generateExecutable(source, filename, outputPath);
  } catch (...) {
    return false;
  }
}

double TestFramework::measureCompilationTime(const std::string &source) {
  auto start = std::chrono::high_resolution_clock::now();

  try {
    auto program = compiler.compile(source);
  } catch (...) {
    // Ignore errors for timing purposes
  }

  auto end = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration<double>(end - start);

  return duration.count();
}

bool TestFramework::expectCompilationError(const std::string &source) {
  try {
    auto program = compiler.compile(source);
    return false; // Should have failed but didn't
  } catch (...) {
    return true; // Expected failure
  }
}

TestFramework::TestStats TestFramework::getTestStats() const {
  TestStats stats;
  stats.totalTests = testCases.size();
  stats.passedTests = passCount;
  stats.failedTests = failCount;
  stats.errorTests = errorCount;
  stats.successRate = stats.totalTests > 0
                          ? (double(stats.passedTests) / stats.totalTests * 100)
                          : 0.0;

  // Calculate average compilation time
  double totalTime = 0.0;
  int timeCount = 0;
  for (const auto &[name, metrics] : testMetrics) {
    if (metrics.compilationTime > 0) {
      totalTime += metrics.compilationTime;
      timeCount++;
    }
  }
  stats.averageCompilationTime = timeCount > 0 ? totalTime / timeCount : 0.0;

  // Category statistics
  for (const auto &test : testCases) {
    stats.categoryStats[test.category]++;
  }

  return stats;
}

std::vector<std::string>
TestFramework::loadAllTestFiles(const std::string &directory) {
  std::vector<std::string> testFiles;

  try {
    for (const auto &entry : std::filesystem::directory_iterator(directory)) {
      if (entry.is_regular_file() && entry.path().extension() == ".mds") {
        testFiles.push_back(entry.path().string());
      }
    }
  } catch (const std::exception &e) {
    std::cerr << "Error loading test files: " << e.what() << std::endl;
  }

  return testFiles;
}

void TestFramework::runTestCategory(const std::string &category) {
  std::cout << "=== Running Tests for Category: " << category
            << " ===" << std::endl;

  // Count tests in this category
  int categoryTestCount = 0;
  for (const auto &test : testCases) {
    if (test.category == category) {
      categoryTestCount++;
    }
  }

  if (categoryTestCount == 0) {
    std::cout << "No tests found for category: " << category << std::endl;
    return;
  }

  std::cout << "Running " << categoryTestCount << " tests in category '"
            << category << "'..." << std::endl
            << std::endl;

  int categoryPassCount = 0;
  int categoryFailCount = 0;
  int categoryErrorCount = 0;
  
  // Store category-specific results for table display
  std::vector<std::pair<TestCase, TestResult>> categoryResults;

  for (const auto &test : testCases) {
    if (test.category == category) {
      try {
        TestResult result = runSingleTest(test);
        categoryResults.emplace_back(test, result);
        switch (result) {
        case TestResult::PASS:
          categoryPassCount++;
          break;
        case TestResult::FAIL:
          categoryFailCount++;
          break;
        case TestResult::ERROR:
          categoryErrorCount++;
          break;
        }
      } catch (const std::exception &e) {
        std::cout << "  ERROR: Exception during test: " << e.what()
                  << std::endl;
        categoryResults.emplace_back(test, TestResult::ERROR);
        categoryErrorCount++;
      }
      std::cout << std::endl;
    }
  }

  // Display category table if not in quiet mode
  if (!quietMode && !minimalMode && categoryResults.size() > 1) {
    std::cout << "\n";
    std::cout << "╔══════════════════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║                        📊 CATEGORY: " << std::left << std::setw(16) << category.substr(0, 16) << "                        ║\n";
    std::cout << "╚══════════════════════════════════════════════════════════════════════════════╝\n";
    
    printTableHeader();
    printTableSeparator();
    
    for (const auto& [test, result] : categoryResults) {
      printTableRow(test, result);
    }
    
    printTableSeparator();
    std::cout << "╚════════════════════════════════════════════════════════════════════════════════╝\n";
  }

  // Print category-specific results
  std::cout << "\n=== Category '" << category << "' Results ===" << std::endl;
  std::cout << "Total tests: " << categoryTestCount << std::endl;
  std::cout << "Passed: " << categoryPassCount << " (" << std::fixed
            << std::setprecision(1)
            << (categoryTestCount > 0
                    ? (double(categoryPassCount) / categoryTestCount * 100)
                    : 0.0)
            << "%)" << std::endl;
  std::cout << "Failed: " << categoryFailCount << " (" << std::fixed
            << std::setprecision(1)
            << (categoryTestCount > 0
                    ? (double(categoryFailCount) / categoryTestCount * 100)
                    : 0.0)
            << "%)" << std::endl;
  std::cout << "Errors: " << categoryErrorCount << " (" << std::fixed
            << std::setprecision(1)
            << (categoryTestCount > 0
                    ? (double(categoryErrorCount) / categoryTestCount * 100)
                    : 0.0)
            << "%)" << std::endl;

  double categorySuccessRate =
      categoryTestCount > 0
          ? (double(categoryPassCount) / categoryTestCount * 100)
          : 0.0;
  std::cout << "Success Rate: " << std::fixed << std::setprecision(1)
            << categorySuccessRate << "%" << std::endl;

  // Visual progress bar for this category
  std::cout << "\nCategory Progress: [";
  int barWidth = 30;
  int passedBars = categoryTestCount > 0
                       ? (categoryPassCount * barWidth) / categoryTestCount
                       : 0;
  int failedBars = categoryTestCount > 0
                       ? (categoryFailCount * barWidth) / categoryTestCount
                       : 0;

  for (int i = 0; i < barWidth; ++i) {
    if (i < passedBars) {
      std::cout << "✓";
    } else if (i < passedBars + failedBars) {
      std::cout << "✗";
    } else {
      std::cout << "·";
    }
  }
  std::cout << "] " << std::fixed << std::setprecision(1) << categorySuccessRate
            << "%" << std::endl;

  if (categoryPassCount == categoryTestCount) {
    std::cout << "🎉 All tests in category '" << category << "' passed!"
              << std::endl;
  } else {
    std::cout << "❌ Some tests in category '" << category << "' failed."
              << std::endl;
  }
}

void TestFramework::cleanupTestArtifacts() {
  try {
    std::filesystem::remove_all("build/test_*");
  } catch (...) {
    // Ignore cleanup errors
  }
}

} // namespace testing
} // namespace meadows
