#ifndef TIMER_H
#define TIMER_H

#include <chrono>
#include <iostream>
#include <string>

namespace meadows {

constexpr double MICROSECONDS_PER_MILLISECOND = 1000.0;

/**
 * @brief Simple timer for performance measurements
 */
class Timer {
private:
  std::chrono::high_resolution_clock::time_point start_;
  std::string name_;
  bool running_;

public:
  explicit Timer(const std::string &name = "") : name_(name), running_(false) {}

  /**
   * @brief Start the timer
   */
  void start() {
    start_ = std::chrono::high_resolution_clock::now();
    running_ = true;
  }

  /**
   * @brief Stop the timer and return elapsed time in milliseconds
   */
  double elapsed() const {
    if (!running_)
      return 0.0;
    auto end = std::chrono::high_resolution_clock::now();
    auto duration =
        std::chrono::duration_cast<std::chrono::microseconds>(end - start_);
    return duration.count() / MICROSECONDS_PER_MILLISECOND;
  }

  /**
   * @brief Get timer name
   */
  const std::string &name() const { return name_; }

  /**
   * @brief Check if timer is running
   */
  bool isRunning() const { return running_; }
};

/**
 * @brief RAII timer that prints on destruction (for verbose mode)
 */
class ScopedTimer {
private:
  Timer timer_;
  bool verbose_;

public:
  ScopedTimer(const std::string &name, bool verbose = true)
      : timer_(name), verbose_(verbose) {
    if (verbose_) {
      timer_.start();
    }
  }

  ~ScopedTimer() {
    if (verbose_ && timer_.isRunning()) {
      std::cerr << "[" << timer_.name() << "] " << timer_.elapsed() << "ms"
                << std::endl;
    }
  }

  double elapsed() const { return timer_.elapsed(); }
};

} // namespace meadows

#endif
