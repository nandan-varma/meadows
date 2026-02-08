#ifndef DIAGNOSTICS_COLLECTOR_H
#define DIAGNOSTICS_COLLECTOR_H

#include "../utils/Exceptions.h"
#include <functional>
#include <vector>

namespace meadows {

/**
 * @brief Collects diagnostics (errors and warnings) during compilation
 *
 * This class allows the compiler to accumulate multiple errors and warnings
 * instead of stopping at the first error. It supports error recovery by
 * tracking whether errors are fatal or recoverable.
 */
class DiagnosticsCollector {
public:
  using DiagnosticHandler = std::function<void(const Diagnostic &)>;

private:
  std::vector<Diagnostic> diagnostics_;
  bool hasErrors_ = false;
  bool hasFatals_ = false;
  DiagnosticHandler handler_;

  // Maximum number of diagnostics to collect (prevents runaway errors)
  static constexpr size_t MAX_DIAGNOSTICS = 100;

public:
  DiagnosticsCollector() = default;
  explicit DiagnosticsCollector(DiagnosticHandler handler)
      : handler_(handler) {}

  /**
   * @brief Report an error
   */
  void reportError(ErrorCode code, const std::string &message,
                   const SourceLocation &location);

  /**
   * @brief Report a warning
   */
  void reportWarning(ErrorCode code, const std::string &message,
                     const SourceLocation &location);

  /**
   * @brief Report a diagnostic directly
   */
  void report(const Diagnostic &diagnostic);

  /**
   * @brief Check if any errors have been reported
   */
  bool hasErrors() const { return hasErrors_; }

  /**
   * @brief Check if any fatal errors have been reported
   */
  bool hasFatals() const { return hasFatals_; }

  /**
   * @brief Get total diagnostic count
   */
  size_t count() const { return diagnostics_.size(); }

  /**
   * @brief Get error count only
   */
  size_t errorCount() const;

  /**
   * @brief Get warning count only
   */
  size_t warningCount() const;

  /**
   * @brief Get all diagnostics
   */
  const std::vector<Diagnostic> &diagnostics() const { return diagnostics_; }

  /**
   * @brief Get diagnostics (non-const)
   */
  std::vector<Diagnostic> &diagnostics() { return diagnostics_; }

  /**
   * @brief Clear all diagnostics
   */
  void clear();

  /**
   * @brief Check if we've reached the maximum diagnostic limit
   */
  bool atLimit() const { return diagnostics_.size() >= MAX_DIAGNOSTICS; }

  /**
   * @brief Throw exception if there are errors (for non-recoverable situations)
   */
  void throwIfErrors() const;

  /**
   * @brief Get the last diagnostic added
   */
  const Diagnostic *last() const {
    return diagnostics_.empty() ? nullptr : &diagnostics_.back();
  }
};

/**
 * @brief RAII helper to temporarily suppress diagnostics
 */
class DiagnosticSuppressor {
  DiagnosticsCollector &collector_;
  std::vector<Diagnostic> saved_;
  bool active_;

public:
  explicit DiagnosticSuppressor(DiagnosticsCollector &collector)
      : collector_(collector), active_(true) {
    saved_ = std::move(collector.diagnostics());
    collector.diagnostics().clear();
  }

  ~DiagnosticSuppressor() {
    if (active_) {
      restore();
    }
  }

  void restore() {
    collector_.diagnostics() = std::move(saved_);
    active_ = false;
  }

  const std::vector<Diagnostic> &suppressed() const {
    return collector_.diagnostics();
  }
};

} // namespace meadows

#endif
