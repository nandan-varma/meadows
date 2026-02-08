#ifndef MEADOWS_MEMORY_UTILS_H
#define MEADOWS_MEMORY_UTILS_H

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <new>
#include <stdexcept>

namespace meadows {

constexpr size_t MAX_ALLOC_SIZE = 1024 * 1024 * 1024;
constexpr size_t MAX_STRING_LENGTH = 1024 * 1024;
constexpr size_t MAX_ARRAY_ELEMENTS = 1000000;

class SafeAlloc {
public:
  static void *malloc(size_t size) {
    if (size == 0) {
      return nullptr;
    }

    if (size > MAX_ALLOC_SIZE) {
      throw std::bad_alloc();
    }

    void *ptr = std::malloc(size);
    if (!ptr) {
      throw std::bad_alloc();
    }

    return ptr;
  }

  static void free(void *ptr) noexcept {
    if (ptr) {
      std::free(ptr);
    }
  }

  template <typename T> static T *alloc(size_t count = 1) {
    if (count == 0) {
      return nullptr;
    }

    if (count > MAX_ALLOC_SIZE / sizeof(T)) {
      throw std::bad_alloc();
    }

    void *ptr = SafeAlloc::malloc(count * sizeof(T));
    return static_cast<T *>(ptr);
  }
};

class BoundsChecker {
public:
  template <typename T>
  static void checkArrayBounds(const T *array, size_t index,
                               size_t max_elements) {
    if (index >= max_elements) {
      throw std::out_of_range("Array index out of bounds");
    }
  }

  static void checkDivision(int divisor) {
    if (divisor == 0) {
      throw std::runtime_error("Division by zero");
    }
  }

  static void checkStringLength(size_t length) {
    if (length > MAX_STRING_LENGTH) {
      throw std::length_error("String exceeds maximum length");
    }
  }

  static void checkFileSize(size_t size) {
    if (size > MAX_ALLOC_SIZE) {
      throw std::length_error("File size exceeds maximum allowed size");
    }
  }
};

class StringValidator {
public:
  static bool containsNullByte(const char *str, size_t len) {
    for (size_t i = 0; i < len; ++i) {
      if (str[i] == '\0') {
        return true;
      }
    }
    return false;
  }

  static bool isValidIdentifier(const char *str) {
    if (!str || !*str) {
      return false;
    }

    if (!(str[0] == '_' || (str[0] >= 'a' && str[0] <= 'z') ||
          (str[0] >= 'A' && str[0] <= 'Z'))) {
      return false;
    }

    for (size_t i = 1; str[i]; ++i) {
      if (!(str[i] == '_' || (str[i] >= 'a' && str[i] <= 'z') ||
            (str[i] >= 'A' && str[i] <= 'Z') ||
            (str[i] >= '0' && str[i] <= '9'))) {
        return false;
      }
    }

    return true;
  }

  static bool isValidPath(const char *path) {
    if (!path) {
      return false;
    }

    if (path[0] == '\0') {
      return false;
    }

    if (path[0] == '/' && path[1] == '.') {
      return false;
    }

    for (size_t i = 0; path[i]; ++i) {
      if (path[i] == '.' && path[i + 1] == '.' &&
          (path[i + 2] == '/' || path[i + 2] == '\0')) {
        return false;
      }

      if (path[i] == ';' || path[i] == '|' || path[i] == '&' ||
          path[i] == '$' || path[i] == '(' || path[i] == ')' ||
          path[i] == '{' || path[i] == '}' || path[i] == '[' ||
          path[i] == ']' || path[i] == '<' || path[i] == '>' ||
          path[i] == '!') {
        return false;
      }
    }

    return true;
  }
};

} // namespace meadows

#endif // MEADOWS_MEMORY_UTILS_H
