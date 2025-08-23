#include "TestFramework.h"
#include "../ast/Expression.h"
#include "../ast/Statement.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <filesystem>
#include <chrono>

namespace meadows {
namespace testing {

void TestFramework::addTest(const TestCase& test) {
    testCases.push_back(test);
}

std::string TestFramework::readTestFile(const std::string& filename) {
    std::string fullPath = testBasePath;
    if (!fullPath.empty() && fullPath.back() != '/') {
        fullPath += "/";
    }
    fullPath += filename;
    std::ifstream file(fullPath);
    if (!file.is_open()) {
        if (!minimalMode && !quietMode) {
            std::cerr << "Warning: Could not open test file " << fullPath << std::endl;
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
        "lex_integers",
        "Tokenize integer literals",
        "x = 42\ny = -123\nz = 0\nw = 999",
        true,
        [](const Program& program) {
            return true;
        },
        "Lexer"
    ));
    
    addTest(TestCase(
        "lex_floats",
        "Tokenize float literals",
        "a = 3.14\nb = -2.5\nc = 0.0\nd = 123.456",
        true,
        nullptr,
        "Lexer"
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
    std::string comprehensive = readTestFile("tests/comprehensive_test.mds");
    if (!comprehensive.empty()) {
        addTest(TestCase(
            "integration_comprehensive",
            "Comprehensive language feature test",
            comprehensive,
            true
        ));
    }
    
    // Load existing examples
    std::string simple = readTestFile("simple_example.mds");
    if (!simple.empty()) {
        addTest(TestCase(
            "integration_simple_example",
            "Simple example from repository",
            simple,
            true
        ));
    }
    
    std::string example = readTestFile("example.mds");
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
    if (minimalMode) {
        // Minimal output: just test name with result
        std::cout << test.name;
    } else if (!quietMode) {
        std::cout << "Running test: " << test.name << " - " << test.description << std::endl;
    }
    
    compiler.clearErrors();
    auto program = compiler.compile(test.source, test.name + ".mds");
    
    bool hasCompileErrors = compiler.hasErrors();
    
    if (test.expectSuccess) {
        if (hasCompileErrors) {
            if (minimalMode) {
                std::cout << " ✗" << std::endl;
            } else if (!quietMode) {
                std::cout << "  FAIL: Expected success but got compilation errors:" << std::endl;
                compiler.getErrorReporter().printErrors();
            }
            return TestResult::FAIL;
        }
        
        if (!program) {
            if (minimalMode) {
                std::cout << " ✗" << std::endl;
            } else if (!quietMode) {
                std::cout << "  FAIL: Expected success but got null program" << std::endl;
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
                std::cout << "  FAIL: Expected compilation to fail but it succeeded" << std::endl;
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
        std::cout << "Running " << testCases.size() << " tests..." << std::endl << std::endl;
    }
    
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
            if (!quietMode) {
                std::cout << "  ERROR: Exception during test: " << e.what() << std::endl;
            }
            errorCount++;
        }
        if (!quietMode) {
            std::cout << std::endl;
        }
    }
    
    if (!quietMode && !minimalMode) {
        printResults();
        printDetailedResults();
    } else if (minimalMode) {
        auto stats = getTestStats();
        std::cout << "\nTotal tests: " << stats.totalTests << std::endl;
        std::cout << "Passed: " << stats.passedTests << " (" << std::fixed << std::setprecision(1) << stats.successRate << "%)" << std::endl;
        std::cout << "Failed: " << stats.failedTests << " (" << std::fixed << std::setprecision(1) << (100.0 - stats.successRate) << "%)" << std::endl;
        std::cout << "Success Rate: " << std::fixed << std::setprecision(1) << stats.successRate << "%" << std::endl;
    }
}

void TestFramework::printResults() const {
    auto stats = getTestStats();
    
    std::cout << "\n=== Test Results Summary ===" << std::endl;
    std::cout << "Total tests: " << stats.totalTests << std::endl;
    std::cout << "Passed: " << stats.passedTests << " (" << std::fixed << std::setprecision(1) << (stats.totalTests > 0 ? (double(stats.passedTests) / stats.totalTests * 100) : 0.0) << "%)" << std::endl;
    std::cout << "Failed: " << stats.failedTests << " (" << std::fixed << std::setprecision(1) << (stats.totalTests > 0 ? (double(stats.failedTests) / stats.totalTests * 100) : 0.0) << "%)" << std::endl;
    std::cout << "Errors: " << stats.errorTests << " (" << std::fixed << std::setprecision(1) << (stats.totalTests > 0 ? (double(stats.errorTests) / stats.totalTests * 100) : 0.0) << "%)" << std::endl;
    std::cout << "Success Rate: " << std::fixed << std::setprecision(1) << stats.successRate << "%" << std::endl;
    
    if (stats.averageCompilationTime > 0) {
        std::cout << "Average Compilation Time: " << std::fixed << std::setprecision(3) << stats.averageCompilationTime << "s" << std::endl;
    }
    
    std::cout << "\n=== Tests by Category ===" << std::endl;
    for (const auto& [category, count] : stats.categoryStats) {
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
    int passedBars = stats.totalTests > 0 ? (stats.passedTests * barWidth) / stats.totalTests : 0;
    int failedBars = stats.totalTests > 0 ? (stats.failedTests * barWidth) / stats.totalTests : 0;
    
    for (int i = 0; i < barWidth; ++i) {
        if (i < passedBars) {
            std::cout << "✓";
        } else if (i < passedBars + failedBars) {
            std::cout << "✗";
        } else {
            std::cout << "·";
        }
    }
    std::cout << "] " << std::fixed << std::setprecision(1) << stats.successRate << "%" << std::endl;
}

void TestFramework::printDetailedResults() const {
    auto stats = getTestStats();
    
    std::cout << "\n=== Detailed Test Statistics ===" << std::endl;
    
    // Overall statistics
    std::cout << "\n📊 Overall Performance:" << std::endl;
    std::cout << "  Total Tests: " << stats.totalTests << std::endl;
    std::cout << "  Success Rate: " << std::fixed << std::setprecision(2) << stats.successRate << "%" << std::endl;
    if (stats.averageCompilationTime > 0) {
        std::cout << "  Average Compilation Time: " << std::fixed << std::setprecision(3) << stats.averageCompilationTime << "s" << std::endl;
    }
    
    // Category breakdown with detailed stats
    std::cout << "\n📂 Test Categories Breakdown:" << std::endl;
    std::cout << std::string(60, '-') << std::endl;
    std::cout << std::left << std::setw(20) << "Category" 
              << std::setw(10) << "Count" 
              << std::setw(12) << "Percentage" 
              << std::setw(18) << "Visual" << std::endl;
    std::cout << std::string(60, '-') << std::endl;
    
    for (const auto& [category, count] : stats.categoryStats) {
        double percentage = stats.totalTests > 0 ? (double(count) / stats.totalTests * 100) : 0.0;
        
        // Create visual bar
        int barWidth = 15;
        int filledBars = static_cast<int>((percentage / 100.0) * barWidth);
        std::string bar = "[";
        for (int i = 0; i < barWidth; ++i) {
            bar += (i < filledBars) ? "█" : "░";
        }
        bar += "]";
        
        std::cout << std::left << std::setw(20) << category
                  << std::setw(10) << count
                  << std::setw(12) << (std::to_string(static_cast<int>(percentage)) + "%")
                  << std::setw(18) << bar << std::endl;
    }
    std::cout << std::string(60, '-') << std::endl;
    
    // Results breakdown
    std::cout << "\n🎯 Results Breakdown:" << std::endl;
    std::cout << "  ✅ Passed: " << stats.passedTests << " (" << std::fixed << std::setprecision(1) 
              << (stats.totalTests > 0 ? (double(stats.passedTests) / stats.totalTests * 100) : 0.0) << "%)" << std::endl;
    std::cout << "  ❌ Failed: " << stats.failedTests << " (" << std::fixed << std::setprecision(1) 
              << (stats.totalTests > 0 ? (double(stats.failedTests) / stats.totalTests * 100) : 0.0) << "%)" << std::endl;
    std::cout << "  ⚠️  Errors: " << stats.errorTests << " (" << std::fixed << std::setprecision(1) 
              << (stats.totalTests > 0 ? (double(stats.errorTests) / stats.totalTests * 100) : 0.0) << "%)" << std::endl;
    
    // Performance insights
    if (stats.averageCompilationTime > 0) {
        std::cout << "\n⚡ Performance Insights:" << std::endl;
        if (stats.averageCompilationTime < 1.0) {
            std::cout << "  🚀 Excellent compilation speed (< 1s average)" << std::endl;
        } else if (stats.averageCompilationTime < 5.0) {
            std::cout << "  ✅ Good compilation speed (< 5s average)" << std::endl;
        } else {
            std::cout << "  ⚠️  Slow compilation speed (> 5s average) - consider optimization" << std::endl;
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
        std::cout << "  ❌ Poor test quality, significant issues need attention" << std::endl;
    }
}

void TestFramework::loadTestsFromFile(const std::string& filename, const std::string& category) {
    std::string content = readTestFile(filename);
    if (!content.empty()) {
        addTest(TestCase(
            category + "_file_" + filename,
            "Test from file: " + filename,
            content,
            true,
            nullptr,
            category
        ));
    }
}

void TestFramework::addCompilationTests() {
    addTest(TestCase(
        "compile_simple",
        "Compile simple function to executable",
        readTestFile("test_compilation.mds"),
        true,
        [this](const Program& program) {
            return testFileCompilation("test_compilation.mds");
        },
        "Compilation"
    ));
    
    addTest(TestCase(
        "compile_classes",
        "Compile classes and methods",
        R"(
class TestClass:
    def __init__(self, value):
        self.value = value
    
    def get_double(self):
        return self.value * 2

obj = TestClass(21)
result = obj.get_double()
)",
        true,
        nullptr,
        "Compilation"
    ));
}

void TestFramework::addErrorHandlingTests() {
    addTest(TestCase(
        "error_syntax",
        "Handle syntax errors gracefully",
        R"(
def invalid_function()  # Missing colon
    return 42
)",
        false,  // Expect failure
        [this](const Program& program) {
            return expectCompilationError("def invalid_function()\n    return 42");
        },
        "Error Handling"
    ));
    
    addTest(TestCase(
        "error_indentation",
        "Handle indentation errors",
        R"(
def bad_indent():
return 42  # Wrong indentation
)",
        false,
        nullptr,
        "Error Handling"
    ));
}

void TestFramework::addPerformanceTests() {
    addTest(TestCase(
        "perf_arithmetic",
        "Performance test: arithmetic operations",
        readTestFile("test_performance.mds"),
        true,
        [this](const Program& program) {
            auto start = std::chrono::steady_clock::now();
            bool result = testFileCompilation("test_performance.mds");
            auto end = std::chrono::steady_clock::now();
            
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
            return result && duration.count() < 5000; // Should compile in under 5 seconds
        },
        "Performance"
    ));
}

void TestFramework::addIRGenerationTests() {
    addTest(TestCase(
        "ir_basic_function",
        "Generate IR for basic function",
        R"(
def simple_add(a, b):
    return a + b
)",
        true,
        nullptr,
        "IR Generation"
    ));
}

void TestFramework::addApplicationTests() {
    std::string testDir = "../tests/";
    
    addTest(TestCase(
        "app_calculator",
        "Real-world calculator application",
        readTestFile(testDir + "test_applications.mds"),
        true,
        [this](const Program& program) {
            return testFileCompilation("test_applications.mds");
        },
        "Applications"
    ));
}

// Enhanced compilation testing methods
bool TestFramework::testFileCompilation(const std::string& filename) {
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

bool TestFramework::testExecutableGeneration(const std::string& filename) {
    try {
        std::string source = readTestFile(filename);
        std::string outputPath = "test_" + filename.substr(0, filename.find('.'));
        
        return compiler.generateExecutable(source, filename, outputPath);
    } catch (...) {
        return false;
    }
}

double TestFramework::measureCompilationTime(const std::string& source) {
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

bool TestFramework::expectCompilationError(const std::string& source) {
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
    stats.successRate = stats.totalTests > 0 ? (double(stats.passedTests) / stats.totalTests * 100) : 0.0;
    
    // Calculate average compilation time
    double totalTime = 0.0;
    int timeCount = 0;
    for (const auto& [name, metrics] : testMetrics) {
        if (metrics.compilationTime > 0) {
            totalTime += metrics.compilationTime;
            timeCount++;
        }
    }
    stats.averageCompilationTime = timeCount > 0 ? totalTime / timeCount : 0.0;
    
    // Category statistics
    for (const auto& test : testCases) {
        stats.categoryStats[test.category]++;
    }
    
    return stats;
}

std::vector<std::string> TestFramework::loadAllTestFiles(const std::string& directory) {
    std::vector<std::string> testFiles;
    
    try {
        for (const auto& entry : std::filesystem::directory_iterator(directory)) {
            if (entry.is_regular_file() && entry.path().extension() == ".mds") {
                testFiles.push_back(entry.path().string());
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error loading test files: " << e.what() << std::endl;
    }
    
    return testFiles;
}

void TestFramework::runTestCategory(const std::string& category) {
    std::cout << "=== Running Tests for Category: " << category << " ===" << std::endl;
    
    // Count tests in this category
    int categoryTestCount = 0;
    for (const auto& test : testCases) {
        if (test.category == category) {
            categoryTestCount++;
        }
    }
    
    if (categoryTestCount == 0) {
        std::cout << "No tests found for category: " << category << std::endl;
        return;
    }
    
    std::cout << "Running " << categoryTestCount << " tests in category '" << category << "'..." << std::endl << std::endl;
    
    int categoryPassCount = 0;
    int categoryFailCount = 0;
    int categoryErrorCount = 0;
    
    for (const auto& test : testCases) {
        if (test.category == category) {
            try {
                TestResult result = runSingleTest(test);
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
            } catch (const std::exception& e) {
                std::cout << "  ERROR: Exception during test: " << e.what() << std::endl;
                categoryErrorCount++;
            }
            std::cout << std::endl;
        }
    }
    
    // Print category-specific results
    std::cout << "\n=== Category '" << category << "' Results ===" << std::endl;
    std::cout << "Total tests: " << categoryTestCount << std::endl;
    std::cout << "Passed: " << categoryPassCount << " (" << std::fixed << std::setprecision(1) 
              << (categoryTestCount > 0 ? (double(categoryPassCount) / categoryTestCount * 100) : 0.0) << "%)" << std::endl;
    std::cout << "Failed: " << categoryFailCount << " (" << std::fixed << std::setprecision(1) 
              << (categoryTestCount > 0 ? (double(categoryFailCount) / categoryTestCount * 100) : 0.0) << "%)" << std::endl;
    std::cout << "Errors: " << categoryErrorCount << " (" << std::fixed << std::setprecision(1) 
              << (categoryTestCount > 0 ? (double(categoryErrorCount) / categoryTestCount * 100) : 0.0) << "%)" << std::endl;
    
    double categorySuccessRate = categoryTestCount > 0 ? (double(categoryPassCount) / categoryTestCount * 100) : 0.0;
    std::cout << "Success Rate: " << std::fixed << std::setprecision(1) << categorySuccessRate << "%" << std::endl;
    
    // Visual progress bar for this category
    std::cout << "\nCategory Progress: [";
    int barWidth = 30;
    int passedBars = categoryTestCount > 0 ? (categoryPassCount * barWidth) / categoryTestCount : 0;
    int failedBars = categoryTestCount > 0 ? (categoryFailCount * barWidth) / categoryTestCount : 0;
    
    for (int i = 0; i < barWidth; ++i) {
        if (i < passedBars) {
            std::cout << "✓";
        } else if (i < passedBars + failedBars) {
            std::cout << "✗";
        } else {
            std::cout << "·";
        }
    }
    std::cout << "] " << std::fixed << std::setprecision(1) << categorySuccessRate << "%" << std::endl;
    
    if (categoryPassCount == categoryTestCount) {
        std::cout << "🎉 All tests in category '" << category << "' passed!" << std::endl;
    } else {
        std::cout << "❌ Some tests in category '" << category << "' failed." << std::endl;
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
