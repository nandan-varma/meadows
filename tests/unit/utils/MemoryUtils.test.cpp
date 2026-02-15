#include "utils/MemoryUtils.h"
#include "catch_amalgamated.hpp"
#include <cstring>

using namespace meadows;

TEST_CASE("SafeAlloc basic operations", "[memory]") {
  SECTION("Allocate single element") {
    auto *ptr = SafeAlloc::alloc<int>();
    REQUIRE(ptr != nullptr);
    *ptr = 42;
    CHECK(*ptr == 42);
    SafeAlloc::free(ptr);
  }

  SECTION("Allocate multiple elements") {
    auto *arr = SafeAlloc::alloc<int>(100);
    REQUIRE(arr != nullptr);
    for (int i = 0; i < 100; ++i) {
      arr[i] = i;
    }
    for (int i = 0; i < 100; ++i) {
      CHECK(arr[i] == i);
    }
    SafeAlloc::free(arr);
  }

  SECTION("Allocate zero elements returns nullptr") {
    auto *ptr = SafeAlloc::alloc<int>(0);
    CHECK(ptr == nullptr);
  }

  SECTION("Malloc zero size returns nullptr") {
    auto *ptr = SafeAlloc::malloc(0);
    CHECK(ptr == nullptr);
  }

  SECTION("Free nullptr is safe") { REQUIRE_NOTHROW(SafeAlloc::free(nullptr)); }
}

TEST_CASE("SafeAlloc size limits", "[memory]") {
  SECTION("Allocation under limit succeeds") {
    auto *ptr = SafeAlloc::alloc<char>(MAX_ALLOC_SIZE - 1);
    REQUIRE(ptr != nullptr);
    SafeAlloc::free(ptr);
  }

  SECTION("Allocation at limit succeeds") {
    auto *ptr = SafeAlloc::alloc<char>(MAX_ALLOC_SIZE);
    REQUIRE(ptr != nullptr);
    SafeAlloc::free(ptr);
  }

  SECTION("Allocation over limit throws") {
    CHECK_THROWS_AS(SafeAlloc::alloc<char>(MAX_ALLOC_SIZE + 1), std::bad_alloc);
  }

  SECTION("Malloc over limit throws") {
    CHECK_THROWS_AS(SafeAlloc::malloc(MAX_ALLOC_SIZE + 1), std::bad_alloc);
  }

  SECTION("Count overflow throws") {
    CHECK_THROWS_AS(SafeAlloc::alloc<int>(MAX_ALLOC_SIZE), std::bad_alloc);
  }
}

TEST_CASE("SafeAlloc different types", "[memory]") {
  SECTION("Allocate primitive types") {
    auto *intPtr = SafeAlloc::alloc<int>(10);
    auto *doublePtr = SafeAlloc::alloc<double>(10);
    auto *charPtr = SafeAlloc::alloc<char>(10);

    REQUIRE(intPtr != nullptr);
    REQUIRE(doublePtr != nullptr);
    REQUIRE(charPtr != nullptr);

    SafeAlloc::free(intPtr);
    SafeAlloc::free(doublePtr);
    SafeAlloc::free(charPtr);
  }

  SECTION("Allocate struct") {
    struct TestStruct {
      int a;
      double b;
      char c[100];
    };

    auto *ptr = SafeAlloc::alloc<TestStruct>(5);
    REQUIRE(ptr != nullptr);
    SafeAlloc::free(ptr);
  }
}

TEST_CASE("BoundsChecker array bounds", "[memory]") {
  int arr[100];

  SECTION("Valid indices pass") {
    for (int i = 0; i < 100; ++i) {
      REQUIRE_NOTHROW(BoundsChecker::checkArrayBounds(arr, i, 100));
    }
  }

  SECTION("Index at boundary throws") {
    CHECK_THROWS_AS(BoundsChecker::checkArrayBounds(arr, 100, 100),
                    std::out_of_range);
  }

  SECTION("Index beyond boundary throws") {
    CHECK_THROWS_AS(BoundsChecker::checkArrayBounds(arr, 101, 100),
                    std::out_of_range);
    CHECK_THROWS_AS(BoundsChecker::checkArrayBounds(arr, 1000, 100),
                    std::out_of_range);
  }

  SECTION("Max elements zero always throws") {
    CHECK_THROWS_AS(BoundsChecker::checkArrayBounds(arr, 0, 0),
                    std::out_of_range);
  }
}

TEST_CASE("BoundsChecker division", "[memory]") {
  SECTION("Non-zero divisor passes") {
    REQUIRE_NOTHROW(BoundsChecker::checkDivision(1));
    REQUIRE_NOTHROW(BoundsChecker::checkDivision(-1));
    REQUIRE_NOTHROW(BoundsChecker::checkDivision(INT_MAX));
    REQUIRE_NOTHROW(BoundsChecker::checkDivision(INT_MIN));
  }

  SECTION("Zero divisor throws") {
    CHECK_THROWS_AS(BoundsChecker::checkDivision(0), std::runtime_error);
  }
}

TEST_CASE("BoundsChecker string length", "[memory]") {
  SECTION("Valid string length passes") {
    REQUIRE_NOTHROW(BoundsChecker::checkStringLength(0));
    REQUIRE_NOTHROW(BoundsChecker::checkStringLength(100));
    REQUIRE_NOTHROW(BoundsChecker::checkStringLength(MAX_STRING_LENGTH));
  }

  SECTION("Excessive string length throws") {
    CHECK_THROWS_AS(BoundsChecker::checkStringLength(MAX_STRING_LENGTH + 1),
                    std::length_error);
  }
}

TEST_CASE("BoundsChecker file size", "[memory]") {
  SECTION("Valid file size passes") {
    REQUIRE_NOTHROW(BoundsChecker::checkFileSize(0));
    REQUIRE_NOTHROW(BoundsChecker::checkFileSize(1024));
    REQUIRE_NOTHROW(BoundsChecker::checkFileSize(MAX_ALLOC_SIZE));
  }

  SECTION("Excessive file size throws") {
    CHECK_THROWS_AS(BoundsChecker::checkFileSize(MAX_ALLOC_SIZE + 1),
                    std::length_error);
  }
}

TEST_CASE("StringValidator null byte detection", "[memory]") {
  SECTION("String without null bytes") {
    CHECK(StringValidator::containsNullByte("hello", 5) == false);
    CHECK(StringValidator::containsNullByte("", 0) == false);
    CHECK(StringValidator::containsNullByte("test", 4) == false);
  }

  SECTION("String with null bytes") {
    CHECK(StringValidator::containsNullByte("hel\0lo", 6) == true);
    CHECK(StringValidator::containsNullByte("\0", 1) == true);
    CHECK(StringValidator::containsNullByte("a\0b\0c", 5) == true);
  }

  SECTION("Null byte at end") {
    CHECK(StringValidator::containsNullByte("test\0", 5) == true);
  }
}

TEST_CASE("StringValidator valid identifiers", "[memory]") {
  SECTION("Valid identifiers") {
    CHECK(StringValidator::isValidIdentifier("x") == true);
    CHECK(StringValidator::isValidIdentifier("_var") == true);
    CHECK(StringValidator::isValidIdentifier("myVar") == true);
    CHECK(StringValidator::isValidIdentifier("MyVar") == true);
    CHECK(StringValidator::isValidIdentifier("var123") == true);
    CHECK(StringValidator::isValidIdentifier("_123") == true);
    CHECK(StringValidator::isValidIdentifier("a_b_c") == true);
    CHECK(StringValidator::isValidIdentifier("__private") == true);
  }

  SECTION("Invalid identifiers") {
    CHECK(StringValidator::isValidIdentifier("") == false);
    CHECK(StringValidator::isValidIdentifier(nullptr) == false);
    CHECK(StringValidator::isValidIdentifier("123") == false);
    CHECK(StringValidator::isValidIdentifier("1var") == false);
    CHECK(StringValidator::isValidIdentifier("my-var") == false);
    CHECK(StringValidator::isValidIdentifier("my.var") == false);
    CHECK(StringValidator::isValidIdentifier("my var") == false);
    CHECK(StringValidator::isValidIdentifier("my@var") == false);
    CHECK(StringValidator::isValidIdentifier("!var") == false);
  }

  SECTION("Single character identifiers") {
    CHECK(StringValidator::isValidIdentifier("a") == true);
    CHECK(StringValidator::isValidIdentifier("_") == true);
    CHECK(StringValidator::isValidIdentifier("A") == true);
    CHECK(StringValidator::isValidIdentifier("1") == false);
  }
}

TEST_CASE("StringValidator valid paths", "[memory]") {
  SECTION("Valid paths") {
    CHECK(StringValidator::isValidPath("/home/user/file.ms") == true);
    CHECK(StringValidator::isValidPath("./relative/path.ms") == true);
    CHECK(StringValidator::isValidPath("src/main.ms") == true);
    CHECK(StringValidator::isValidPath("file.ms") == true);
    CHECK(StringValidator::isValidPath("/a/b/c/d/e/f/g.ms") == true);
  }

  SECTION("Invalid paths") {
    CHECK(StringValidator::isValidPath("") == false);
    CHECK(StringValidator::isValidPath(nullptr) == false);
    CHECK(StringValidator::isValidPath("/../escape.ms") == false);
    CHECK(StringValidator::isValidPath("dir/../../../escape.ms") == false);
    CHECK(StringValidator::isValidPath("file;injection.ms") == false);
    CHECK(StringValidator::isValidPath("file|pipe.ms") == false);
    CHECK(StringValidator::isValidPath("file&background.ms") == false);
    CHECK(StringValidator::isValidPath("file$(cmd).ms") == false);
    CHECK(StringValidator::isValidPath("file`cmd`.ms") == false);
    CHECK(StringValidator::isValidPath("file<script>.ms") == false);
  }

  SECTION("Double dot in path") {
    CHECK(StringValidator::isValidPath("../escape.ms") == false);
    CHECK(StringValidator::isValidPath("a/b/../c.ms") == false);
    CHECK(StringValidator::isValidPath("a..b.ms") == true);
  }
}

TEST_CASE("MemoryUtils edge cases", "[memory]") {
  SECTION("Very large allocation count") {
    CHECK_THROWS_AS(SafeAlloc::alloc<char>(SIZE_MAX), std::bad_alloc);
  }

  SECTION("Allocation with struct of zero size") {
    struct EmptyStruct {};
    auto *ptr = SafeAlloc::alloc<EmptyStruct>(100);
    REQUIRE(ptr != nullptr);
    SafeAlloc::free(ptr);
  }

  SECTION("String validation with null pointer") {
    CHECK(StringValidator::containsNullByte(nullptr, 0) == false);
  }

  SECTION("Bounds check with null array (undefined but should not crash)") {
    REQUIRE_NOTHROW(BoundsChecker::checkArrayBounds<int>(nullptr, 0, 0));
  }

  SECTION("Maximum array elements") {
    CHECK_NOTHROW(
        BoundsChecker::checkArrayBounds<int>(nullptr, 0, MAX_ARRAY_ELEMENTS));
    CHECK_THROWS_AS(BoundsChecker::checkArrayBounds<int>(
                        nullptr, MAX_ARRAY_ELEMENTS + 1, MAX_ARRAY_ELEMENTS),
                    std::out_of_range);
  }
}

TEST_CASE("MemoryUtils constants", "[memory]") {
  SECTION("MAX_ALLOC_SIZE is reasonable") {
    CHECK(MAX_ALLOC_SIZE > 0);
    CHECK(MAX_ALLOC_SIZE <= 100 * 1024 * 1024);
  }

  SECTION("MAX_STRING_LENGTH is reasonable") {
    CHECK(MAX_STRING_LENGTH > 0);
    CHECK(MAX_STRING_LENGTH <= MAX_ALLOC_SIZE);
  }

  SECTION("MAX_ARRAY_ELEMENTS is reasonable") {
    CHECK(MAX_ARRAY_ELEMENTS > 0);
    CHECK(MAX_ARRAY_ELEMENTS <= MAX_ALLOC_SIZE / sizeof(int));
  }
}
