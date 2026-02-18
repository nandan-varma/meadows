/**
 * @file CompilationEvents.h
 * @brief Observer Pattern for compilation pipeline events
 *
 * Provides event types and observer interface for compilation pipeline events.
 * Useful for IDE integration, progress reporting, and logging.
 */

#ifndef COMPILATION_EVENTS_H
#define COMPILATION_EVENTS_H

#include <algorithm>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace meadows {

enum class CompilationPhase {
  INITIALIZATION,
  LEXING,
  PARSING,
  TYPE_CHECKING,
  CODEGEN,
  OPTIMIZATION,
  LINKING,
  COMPLETE,
  ERROR
};

struct CompilationEvent {
  CompilationPhase phase;
  std::string message;
  int line = 0;
  int column = 0;
  double progress = 0.0;
};

class CompilationObserver {
public:
  virtual ~CompilationObserver() = default;

  virtual void onCompilationEvent(const CompilationEvent &event) = 0;
  virtual void onPhaseStart(CompilationPhase phase) = 0;
  virtual void onPhaseComplete(CompilationPhase phase) = 0;
  virtual void onError(const std::string &error, int line = 0,
                       int column = 0) = 0;
  virtual void onProgress(double progress, const std::string &message) = 0;
};

class CompilationSubject {
public:
  void addObserver(std::shared_ptr<CompilationObserver> observer) {
    observers_.push_back(observer);
  }

  void removeObserver(std::shared_ptr<CompilationObserver> observer) {
    auto it = std::find(observers_.begin(), observers_.end(), observer);
    if (it != observers_.end()) {
      observers_.erase(it);
    }
  }

protected:
  void notifyEvent(const CompilationEvent &event) {
    for (const auto &observer : observers_) {
      observer->onCompilationEvent(event);
    }
  }

  void notifyPhaseStart(CompilationPhase phase) {
    for (const auto &observer : observers_) {
      observer->onPhaseStart(phase);
    }
  }

  void notifyPhaseComplete(CompilationPhase phase) {
    for (const auto &observer : observers_) {
      observer->onPhaseComplete(phase);
    }
  }

  void notifyError(const std::string &error, int line = 0, int column = 0) {
    for (const auto &observer : observers_) {
      observer->onError(error, line, column);
    }
  }

  void notifyProgress(double progress, const std::string &message) {
    for (const auto &observer : observers_) {
      observer->onProgress(progress, message);
    }
  }

private:
  std::vector<std::shared_ptr<CompilationObserver>> observers_;
};

class LoggingObserver : public CompilationObserver {
public:
  explicit LoggingObserver(bool verbose = false) : verbose_(verbose) {}

  void onCompilationEvent(const CompilationEvent &event) override {
    if (verbose_) {
      printProgress(event.progress, event.message);
    }
  }

  void onPhaseStart(CompilationPhase phase) override {
    printPhase(phase, "Starting");
  }

  void onPhaseComplete(CompilationPhase phase) override {
    printPhase(phase, "Complete");
  }

  void onError(const std::string &error, int line, int column) override {
    std::cerr << "[ERROR]";
    if (line > 0) {
      std::cerr << " (" << line << ":" << column << ")";
    }
    std::cerr << " " << error << std::endl;
  }

  void onProgress(double progress, const std::string &message) override {
    printProgress(progress, message);
  }

private:
  void printPhase(CompilationPhase phase, const char *status) {
    std::cout << "[" << status << "] " << phaseToString(phase) << std::endl;
  }

  void printProgress(double progress, const std::string &message) {
    std::cout << "[" << static_cast<int>(progress * 100) << "%] " << message
              << std::endl;
  }

  std::string phaseToString(CompilationPhase phase) {
    switch (phase) {
    case CompilationPhase::INITIALIZATION:
      return "Initialization";
    case CompilationPhase::LEXING:
      return "Lexing";
    case CompilationPhase::PARSING:
      return "Parsing";
    case CompilationPhase::TYPE_CHECKING:
      return "Type Checking";
    case CompilationPhase::CODEGEN:
      return "Code Generation";
    case CompilationPhase::OPTIMIZATION:
      return "Optimization";
    case CompilationPhase::LINKING:
      return "Linking";
    case CompilationPhase::COMPLETE:
      return "Complete";
    case CompilationPhase::ERROR:
      return "Error";
    default:
      return "Unknown";
    }
  }

  bool verbose_;
};

} // namespace meadows

#endif // COMPILATION_EVENTS_H
