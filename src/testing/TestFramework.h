#pragma once

#include "../src/Compiler.h"
#include <string>
#include <vector>
#include <functional>
#include <iostream>

namespace meadows {
namespace testing {

enum class TestResult {
    PASS,
    FAIL,
    ERROR
};

struct TestCase {
    std::string name;
    std::string description;
    std::string source;
    bool expectSuccess;
    std::function<bool(const Program&)> validator;
    
    TestCase(const std::string& n, const std::string& desc, const std::string& src, 
             bool success = true, std::function<bool(const Program&)> val = nullptr)
        : name(n), description(desc), source(src), expectSuccess(success), validator(val) {}
};

class TestFramework {
private:
    Compiler compiler;
    std::vector<TestCase> testCases;
    int passCount = 0;
    int failCount = 0;
    int errorCount = 0;
    
public:
    TestFramework() = default;
    
    // Add test cases
    void addTest(const TestCase& test);
    void addLexerTests();
    void addParserTests();
    void addExpressionTests();
    void addStatementTests();
    void addFunctionTests();
    void addClassTests();
    void addErrorTests();
    void addIntegrationTests();
    
    // Run tests
    TestResult runSingleTest(const TestCase& test);
    void runAllTests();
    void runTestCategory(const std::string& category);
    
    // Results
    void printResults() const;
    void printDetailedResults() const;
    bool allTestsPassed() const { return failCount == 0 && errorCount == 0; }
    
    // Utility methods
    std::string readTestFile(const std::string& filename);
    void loadTestsFromFile(const std::string& filename, const std::string& category);
};

} // namespace testing
} // namespace meadows
