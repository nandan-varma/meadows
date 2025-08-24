#pragma once

#include "../src/Compiler.h"
#include <chrono>
#include <fstream>
#include <functional>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace meadows {
namespace testing {

enum class TestResult { PASS, FAIL, ERROR };

struct TestCase {
  std::string name;
  std::string description;
  std::string source;
  std::string category;
  bool expectSuccess;
  std::function<bool(const Program &)> validator;

  TestCase(const std::string &n, const std::string &desc,
           const std::string &src, bool success = true,
           std::function<bool(const Program &)> val = nullptr,
           const std::string &cat = "General")
      : name(n), description(desc), source(src), category(cat),
        expectSuccess(success), validator(val) {}
};

struct TestMetrics {
  double compilationTime = 0.0;
  size_t memoryUsage = 0;
  bool compiled = false;
  bool executed = false;
  std::string errorMessage;
};

class TestFramework {
private:
  Compiler compiler;
  std::vector<TestCase> testCases;
  std::vector<std::pair<TestCase, TestResult>> testResults; // Store results for table display
  std::map<std::string, TestMetrics> testMetrics;
  int passCount = 0;
  int failCount = 0;
  int errorCount = 0;
  std::chrono::steady_clock::time_point startTime;
  std::string testBasePath = ".";
  bool quietMode = false;
  bool minimalMode = false;

public:
  TestFramework() = default;
  TestFramework(const std::string &basePath) : testBasePath(basePath) {}

  // Set base path for tests
  void setTestBasePath(const std::string &path) { testBasePath = path; }

  // Set quiet mode (suppress detailed output for shell script integration)
  void setQuietMode(bool quiet) { quietMode = quiet; }

  // Set minimal mode (only show test names with ✓/✗)
  void setMinimalMode(bool minimal) { minimalMode = minimal; }

  // Enhanced test case addition
  void addTest(const TestCase &test);
  void addLexerTests();
  void addParserTests();
  void addExpressionTests();
  void addStatementTests();
  void addFunctionTests();
  void addClassTests();
  void addErrorTests();
  void addIntegrationTests();

  // New comprehensive test categories
  void addCompilationTests();
  void addErrorHandlingTests();
  void addPerformanceTests();
  void addIRGenerationTests();
  void addApplicationTests();

  // Advanced test categories
  void addErrorRecoveryTests();
  void addSemanticTests();
  void addAdvancedLanguageTests();
  void addStressTests();
  void addBoundaryTests();
  void addRealWorldTests();
  void addEdgeCaseTests();

  // Enhanced test execution
  TestResult runSingleTest(const TestCase &test);
  void runAllTests();
  void runTestCategory(const std::string &category);

  // Performance and compilation testing
  bool testFileCompilation(const std::string &filename);
  bool testExecutableGeneration(const std::string &filename);
  double measureCompilationTime(const std::string &source);
  bool expectCompilationError(const std::string &source);

  // Results and reporting
  void printResults() const;
  void printDetailedResults() const;
  void printTableResults() const;
  void printTableHeader() const;
  void printTableRow(const TestCase &test, TestResult result) const;
  void printTableSeparator() const;
  bool allTestsPassed() const { return failCount == 0 && errorCount == 0; }

  // Statistics
  struct TestStats {
    int totalTests;
    int passedTests;
    int failedTests;
    int errorTests;
    double averageCompilationTime;
    double successRate;
    std::map<std::string, int> categoryStats;
  };

  TestStats getTestStats() const;

  // Utility methods
  std::string readTestFile(const std::string &filename);
  void loadTestsFromFile(const std::string &filename,
                         const std::string &category);
  std::vector<std::string> loadAllTestFiles(const std::string &directory);
  void cleanupTestArtifacts();
};

} // namespace testing
} // namespace meadows
