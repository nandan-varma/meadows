#ifndef MEMORYUTILS_H
#define MEMORYUTILS_H

#include <cstddef>
#include <utility>

namespace MemoryUtils {

template <typename T> class ScopedAlloc {
public:
  ScopedAlloc(T *ptr) : ptr_(ptr) {}
  ~ScopedAlloc() {
    if (ptr_) {
      delete ptr_;
    }
  }

  T *get() const { return ptr_; }
  T *release() {
    T *tmp = ptr_;
    ptr_ = nullptr;
    return tmp;
  }

  ScopedAlloc(const ScopedAlloc &) = delete;
  ScopedAlloc &operator=(const ScopedAlloc &) = delete;

  ScopedAlloc(ScopedAlloc &&other) noexcept : ptr_(other.ptr_) {
    other.ptr_ = nullptr;
  }

  ScopedAlloc &operator=(ScopedAlloc &&other) noexcept {
    if (this != &other) {
      if (ptr_)
        delete ptr_;
      ptr_ = other.ptr_;
      other.ptr_ = nullptr;
    }
    return *this;
  }

private:
  T *ptr_;
};

class ScopeTracker {
public:
  void enterScope() { scopeDepth_++; }
  void exitScope() {
    if (scopeDepth_ > 0)
      scopeDepth_--;
  }
  size_t currentDepth() const { return scopeDepth_; }

private:
  size_t scopeDepth_ = 0;
};

} // namespace MemoryUtils

#endif
