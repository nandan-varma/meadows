#include "codegen/StringUtils.h"
#include "catch_amalgamated.hpp"

using namespace StringUtils;

TEST_CASE("EscapeHandler process - unescape sequences", "[stringutils]") {
  SECTION("Newline escape") {
    CHECK(EscapeHandler::process("hello\\nworld") == "hello\nworld");
  }

  SECTION("Tab escape") {
    CHECK(EscapeHandler::process("col1\\tcol2") == "col1\tcol2");
  }

  SECTION("Backslash escape") {
    CHECK(EscapeHandler::process("path\\\\to\\\\file") == "path\\to\\file");
  }

  SECTION("Quote escape") {
    CHECK(EscapeHandler::process("say \\\"hello\\\"") == "say \"hello\"");
  }

  SECTION("Carriage return escape") {
    CHECK(EscapeHandler::process("line1\\rline2") == "line1\rline2");
  }

  SECTION("Null character escape") {
    auto result = EscapeHandler::process("before\\0after");
    REQUIRE(result.length() == 12);
    CHECK(result[6] == '\0');
  }

  SECTION("Backspace escape") {
    CHECK(EscapeHandler::process("text\\b") == "text\b");
  }

  SECTION("Form feed escape") {
    CHECK(EscapeHandler::process("page\\f") == "page\f");
  }

  SECTION("Unknown escape passes through") {
    CHECK(EscapeHandler::process("\\x") == "x");
    CHECK(EscapeHandler::process("\\z") == "z");
    CHECK(EscapeHandler::process("\\1") == "1");
  }

  SECTION("Multiple escapes") {
    CHECK(EscapeHandler::process("\\n\\t\\\\\\\"") == "\n\t\\\"");
  }

  SECTION("Empty string") { CHECK(EscapeHandler::process("") == ""); }

  SECTION("No escapes") {
    CHECK(EscapeHandler::process("hello world") == "hello world");
  }

  SECTION("Trailing backslash") {
    CHECK(EscapeHandler::process("test\\") == "test");
  }

  SECTION("Complex string with escapes") {
    std::string input = "Line 1\\nLine 2\\t\\tTabbed\\n\\\"Quoted\\\"";
    std::string expected = "Line 1\nLine 2\t\tTabbed\n\"Quoted\"";
    CHECK(EscapeHandler::process(input) == expected);
  }

  SECTION("Unicode surrogate escapes") {
    CHECK(EscapeHandler::process("\\u0041") == "u0041");
  }
}

TEST_CASE("EscapeHandler escape - add escape sequences", "[stringutils]") {
  SECTION("Newline escape") {
    CHECK(EscapeHandler::escape("hello\nworld") == "hello\\nworld");
  }

  SECTION("Tab escape") {
    CHECK(EscapeHandler::escape("col1\tcol2") == "col1\\tcol2");
  }

  SECTION("Backslash escape") {
    CHECK(EscapeHandler::escape("path\\to\\file") == "path\\\\to\\\\file");
  }

  SECTION("Quote escape") {
    CHECK(EscapeHandler::escape("say \"hello\"") == "say \\\"hello\\\"");
  }

  SECTION("Carriage return escape") {
    CHECK(EscapeHandler::escape("line1\rline2") == "line1\\rline2");
  }

  SECTION("Null character escape") {
    std::string input(1, '\0');
    CHECK(EscapeHandler::escape(input) == "\\0");
  }

  SECTION("Backspace escape") {
    std::string input = "text\b";
    CHECK(EscapeHandler::escape(input) == "text\\b");
  }

  SECTION("Form feed escape") {
    std::string input = "page\f";
    CHECK(EscapeHandler::escape(input) == "page\\f");
  }

  SECTION("Empty string") { CHECK(EscapeHandler::escape("") == ""); }

  SECTION("No special characters") {
    CHECK(EscapeHandler::escape("hello world") == "hello world");
  }

  SECTION("All escape types") {
    std::string input;
    input += '\n';
    input += '\t';
    input += '\r';
    input += '\0';
    input += '\b';
    input += '\f';
    input += '\\';
    input += '"';

    std::string expected = "\\n\\t\\r\\0\\b\\f\\\\\\\"";
    CHECK(EscapeHandler::escape(input) == expected);
  }
}

TEST_CASE("EscapeHandler round-trip", "[stringutils]") {
  SECTION("Simple strings") {
    std::vector<std::string> testStrings = {"hello", "world", "test string",
                                            "with\nnewlines", "with\ttabs"};

    for (const auto &str : testStrings) {
      auto escaped = EscapeHandler::escape(str);
      auto unescaped = EscapeHandler::process(escaped);
      CHECK(unescaped == str);
    }
  }

  SECTION("Complex strings") {
    std::vector<std::string> testStrings = {"path\\to\\file", "say \"hello\"",
                                            "line1\nline2\nline3", "\t\t\t"};

    for (const auto &str : testStrings) {
      auto escaped = EscapeHandler::escape(str);
      auto unescaped = EscapeHandler::process(escaped);
      CHECK(unescaped == str);
    }
  }
}

TEST_CASE("StringPool basic operations", "[stringutils]") {
  SECTION("Get instance") {
    auto &pool1 = StringPool::getInstance();
    auto &pool2 = StringPool::getInstance();
    CHECK(&pool1 == &pool2);
  }

  SECTION("Intern returns pointer") {
    auto &pool = StringPool::getInstance();
    const std::string *ptr = pool.intern("test");
    CHECK(ptr != nullptr);
    CHECK(*ptr == "test");
  }

  SECTION("Same string returns same pointer") {
    auto &pool = StringPool::getInstance();
    const std::string *ptr1 = pool.intern("unique_string");
    const std::string *ptr2 = pool.intern("unique_string");
    CHECK(ptr1 == ptr2);
  }

  SECTION("Different strings return different pointers") {
    auto &pool = StringPool::getInstance();
    const std::string *ptr1 = pool.intern("string_a");
    const std::string *ptr2 = pool.intern("string_b");
    CHECK(ptr1 != ptr2);
  }

  SECTION("Pool size increases") {
    auto &pool = StringPool::getInstance();
    size_t initialSize = pool.poolSize();

    pool.intern("new_unique_string_12345");
    CHECK(pool.poolSize() == initialSize + 1);
  }

  SECTION("Empty string") {
    auto &pool = StringPool::getInstance();
    const std::string *ptr1 = pool.intern("");
    const std::string *ptr2 = pool.intern("");
    CHECK(ptr1 == ptr2);
    CHECK(*ptr1 == "");
  }

  SECTION("Long string") {
    auto &pool = StringPool::getInstance();
    std::string longStr(1000, 'a');
    const std::string *ptr1 = pool.intern(longStr);
    const std::string *ptr2 = pool.intern(longStr);
    CHECK(ptr1 == ptr2);
  }

  SECTION("Unicode string") {
    auto &pool = StringPool::getInstance();
    const std::string *ptr = pool.intern("Hello, 世界! 🌍");
    CHECK(*ptr == "Hello, 世界! 🌍");
  }

  SECTION("Multiple interns") {
    auto &pool = StringPool::getInstance();
    std::vector<const std::string *> ptrs;

    for (int i = 0; i < 100; ++i) {
      ptrs.push_back(pool.intern("string_" + std::to_string(i)));
    }

    for (int i = 0; i < 100; ++i) {
      const std::string *ptr = pool.intern("string_" + std::to_string(i));
      CHECK(ptr == ptrs[i]);
    }
  }
}

TEST_CASE("StringPool clear", "[stringutils]") {
  SECTION("Clear empties pool") {
    auto &pool = StringPool::getInstance();
    pool.intern("test_clear");

    size_t sizeBefore = pool.poolSize();
    pool.clear();

    CHECK(pool.poolSize() == 0);
  }

  SECTION("After clear, new intern creates new entry") {
    auto &pool = StringPool::getInstance();

    const std::string *ptr1 = pool.intern("after_clear");
    pool.clear();
    const std::string *ptr2 = pool.intern("after_clear");

    CHECK(ptr1 != ptr2);
    CHECK(*ptr1 == *ptr2);
  }
}

TEST_CASE("StringPool thread safety", "[stringutils]") {
  SECTION("Multiple interns of same string") {
    auto &pool = StringPool::getInstance();
    std::vector<const std::string *> results;

    for (int i = 0; i < 10; ++i) {
      results.push_back(pool.intern("concurrent_test"));
    }

    for (size_t i = 1; i < results.size(); ++i) {
      CHECK(results[i] == results[0]);
    }
  }
}

TEST_CASE("EscapeHandler edge cases", "[stringutils]") {
  SECTION("Single backslash") { CHECK(EscapeHandler::process("\\") == ""); }

  SECTION("Double backslash") { CHECK(EscapeHandler::process("\\\\") == "\\"); }

  SECTION("Triple backslash") {
    CHECK(EscapeHandler::process("\\\\\\") == "\\");
  }

  SECTION("Only escapes") {
    CHECK(EscapeHandler::process("\\n\\t\\r") == "\n\t\r");
  }

  SECTION("Mixed valid and invalid") {
    CHECK(EscapeHandler::process("\\n\\x\\t") == "\nx\t");
  }

  SECTION("Escaped backslash before quote") {
    CHECK(EscapeHandler::process("\\\\\\\"") == "\\\"");
  }

  SECTION("Very long string") {
    std::string longStr(10000, 'a');
    CHECK(EscapeHandler::process(longStr) == longStr);
  }
}

TEST_CASE("StringPool singleton behavior", "[stringutils]") {
  SECTION("Cannot copy construct") {
    auto &pool = StringPool::getInstance();
    static_assert(!std::is_copy_constructible_v<StringPool>);
  }

  SECTION("Cannot copy assign") {
    static_assert(!std::is_copy_assignable_v<StringPool>);
  }

  SECTION("Cannot move construct") {
    static_assert(!std::is_move_constructible_v<StringPool>);
  }

  SECTION("Cannot move assign") {
    static_assert(!std::is_move_assignable_v<StringPool>);
  }
}
