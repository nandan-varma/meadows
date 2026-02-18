/**
 * @file ErrorHandler.h
 * @brief Chain of Responsibility Pattern for error handling
 *
 * Provides a chain of error processors that can each handle different
 * types of errors. Each processor can either handle the error or pass
 * it to the next processor in the chain.
 */

#ifndef ERROR_HANDLER_H
#define ERROR_HANDLER_H

#include <iostream>
#include <memory>
#include <vector>

#include "../errors/CompilationError.h"

namespace meadows {

// Forward declarations
class ErrorProcessor;

/**
 * @brief Context for error processing
 *
 * Carries additional information that processors might need
 */
struct ErrorContext {
  std::string sourceFile;
  std::string sourceLine; // The actual source code line
  bool hasSourceContext = false;
  int errorCount = 0;
  int warningCount = 0;
  int maxErrors = 100; // Stop after this many errors

  void incrementError() { errorCount++; }
  void incrementWarning() { warningCount++; }
  bool shouldStop() const { return errorCount >= maxErrors; }
};

/**
 * @brief Interface for error processors in the chain
 *
 * Implements Chain of Responsibility pattern - each processor
 * can handle the error or pass it to the next processor.
 */
class ErrorProcessor {
public:
  virtual ~ErrorProcessor() = default;

  /**
   * @brief Check if this processor can handle the error
   */
  virtual bool canHandle(const CompilationError &error) = 0;

  /**
   * @brief Process the error
   * @return true if error was handled, false to pass to next processor
   */
  virtual bool process(const CompilationError &error,
                       ErrorContext &context) = 0;

  /**
   * @brief Get processor name for debugging
   */
  virtual std::string getName() const = 0;

  void setNext(std::unique_ptr<ErrorProcessor> next) {
    next_ = std::move(next);
  }

  ErrorProcessor *getNext() const { return next_.get(); }

  /**
   * @brief Internal handle method that checks canHandle then processes
   */
  virtual bool handle(const CompilationError &error, ErrorContext &context) {
    if (canHandle(error)) {
      return process(error, context);
    }
    return passToNext(error, context);
  }

protected:
  /**
   * @brief Pass error to next processor in chain
   */
  bool passToNext(const CompilationError &error, ErrorContext &context) {
    if (next_) {
      return next_->handle(error, context);
    }
    return false; // No processor handled it
  }

  std::unique_ptr<ErrorProcessor> next_;
};

/**
 * @brief Filters errors by severity
 */
class SeverityFilterProcessor : public ErrorProcessor {
public:
  explicit SeverityFilterProcessor(Severity minSeverity)
      : minSeverity_(minSeverity) {}

  bool canHandle(const CompilationError &error) override {
    return static_cast<int>(error.getSeverity()) >=
           static_cast<int>(minSeverity_);
  }

  bool process(const CompilationError &error, ErrorContext &context) override {
    // Pass through to next processor if severity is high enough
    return passToNext(error, context);
  }

  std::string getName() const override { return "SeverityFilter"; }

private:
  Severity minSeverity_;
};

/**
 * @brief Counts errors and warnings
 */
class CounterProcessor : public ErrorProcessor {
public:
  bool canHandle(const CompilationError &error) override {
    return true; // Handle all errors
  }

  bool process(const CompilationError &error, ErrorContext &context) override {
    if (error.isError()) {
      context.incrementError();
    } else if (error.isWarning()) {
      context.incrementWarning();
    }
    return passToNext(error, context);
  }

  std::string getName() const override { return "Counter"; }
};

/**
 * @brief Limits total number of errors
 */
class LimitProcessor : public ErrorProcessor {
public:
  bool canHandle(const CompilationError &error) override {
    return true; // Check limit for all errors
  }

  bool process(const CompilationError &error, ErrorContext &context) override {
    if (context.shouldStop()) {
      // Don't process more errors, we've reached the limit
      return true; // Handled by dropping
    }
    return passToNext(error, context);
  }

  std::string getName() const override { return "Limit"; }
};

/**
 * @brief Formats and outputs errors to stderr
 */
class ConsoleOutputProcessor : public ErrorProcessor {
public:
  bool canHandle(const CompilationError &error) override {
    return true; // Output all errors
  }

  bool process(const CompilationError &error, ErrorContext &context) override {
    std::cerr << error.format() << std::endl;
    return passToNext(error, context);
  }

  std::string getName() const override { return "ConsoleOutput"; }
};

/**
 * @brief Collects errors for later retrieval (e.g., for LSP)
 */
class CollectorProcessor : public ErrorProcessor {
public:
  bool canHandle(const CompilationError &error) override {
    return true; // Collect all errors
  }

  bool process(const CompilationError &error, ErrorContext &context) override {
    errors_.push_back(error);
    return passToNext(error, context);
  }

  std::string getName() const override { return "Collector"; }

  const std::vector<CompilationError> &getErrors() const { return errors_; }
  void clear() { errors_.clear(); }

private:
  std::vector<CompilationError> errors_;
};

/**
 * @brief Handler that manages the chain of processors
 *
 * This is the main entry point for error handling.
 */
class ErrorHandler {
public:
  ErrorHandler();

  /**
   * @brief Add a processor to the end of the chain
   */
  void addProcessor(std::unique_ptr<ErrorProcessor> processor);

  /**
   * @brief Handle a compilation error
   * @return true if error was handled by any processor
   */
  bool handle(const CompilationError &error);

  /**
   * @brief Handle multiple errors
   */
  void handle(const std::vector<CompilationError> &errors);

  /**
   * @brief Get the error context.
   */
  const ErrorContext &getContext() const { return context_; }

  /**
   * @brief Get error count.
   */
  int getErrorCount() const { return context_.errorCount; }

  /**
   * @brief Check if there are errors.
   */
  bool hasErrors() const { return context_.errorCount > 0; }

  /**
   * @brief Reset the handler state
   */
  void reset();

  /**
   * @brief Check if we've reached the error limit
   */
  bool hasReachedLimit() const { return context_.shouldStop(); }

  /**
   * @brief Set maximum number of errors before stopping
   */
  void setMaxErrors(int max) { context_.maxErrors = max; }

  /**
   * @brief Create standard handler chain (counter -> limit -> console)
   */
  static std::unique_ptr<ErrorHandler> createStandardHandler();

  /**
   * @brief Create handler chain for IDE/LSP (collects all errors)
   */
  static std::unique_ptr<ErrorHandler> createLSPHandler();

private:
  std::unique_ptr<ErrorProcessor> chain_;
  ErrorContext context_;

  void appendProcessor(std::unique_ptr<ErrorProcessor> processor);
};

} // namespace meadows

#endif // ERROR_HANDLER_H
