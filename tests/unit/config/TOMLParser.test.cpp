#include "config/TOMLParser.h"
#include "catch_amalgamated.hpp"

using namespace meadows::config;

TEST_CASE("TOMLParser value type factories", "[config]") {
  SECTION("String value") {
    auto val = TOMLValue::makeString("hello");
    CHECK(val.type == TOMLValue::STRING);
    CHECK(val.asString() == "hello");
  }

  SECTION("Integer value") {
    auto val = TOMLValue::makeInteger(42);
    CHECK(val.type == TOMLValue::INTEGER);
    CHECK(val.asInteger() == 42);
  }

  SECTION("Boolean value") {
    auto val = TOMLValue::makeBoolean(true);
    CHECK(val.type == TOMLValue::BOOLEAN);
    CHECK(val.asBoolean() == true);
  }

  SECTION("Table value") {
    auto val = TOMLValue::makeTable();
    CHECK(val.type == TOMLValue::TABLE);
    CHECK(val.asBoolean() == false);
    CHECK(val.tableValue.empty());
  }

  SECTION("Array value") {
    auto val = TOMLValue::makeArray();
    CHECK(val.type == TOMLValue::ARRAY);
    CHECK(val.arrayValue.empty());
  }
}

TEST_CASE("TOMLParser type checks", "[config]") {
  SECTION("String isString") {
    auto val = TOMLValue::makeString("test");
    CHECK(val.isString());
    CHECK_FALSE(val.isInteger());
    CHECK_FALSE(val.isBoolean());
    CHECK_FALSE(val.isTable());
    CHECK_FALSE(val.isArray());
  }

  SECTION("Integer isInteger") {
    auto val = TOMLValue::makeInteger(42);
    CHECK(val.isInteger());
    CHECK_FALSE(val.isString());
    CHECK_FALSE(val.isBoolean());
  }

  SECTION("Boolean isBoolean") {
    auto val = TOMLValue::makeBoolean(true);
    CHECK(val.isBoolean());
    CHECK_FALSE(val.isString());
    CHECK_FALSE(val.isInteger());
  }

  SECTION("Table isTable") {
    auto val = TOMLValue::makeTable();
    CHECK(val.isTable());
    CHECK_FALSE(val.isString());
  }

  SECTION("Array isArray") {
    auto val = TOMLValue::makeArray();
    CHECK(val.isArray());
    CHECK_FALSE(val.isString());
  }
}

TEST_CASE("TOMLParser parse simple key-value pairs", "[config]") {
  SECTION("String value") {
    TOMLParser parser;
    auto result = parser.parse("key = \"value\"");
    CHECK(parser.success());
    CHECK(result.isTable());
    CHECK(result.tableValue.find("key") != result.tableValue.end());
    CHECK(result.tableValue["key"].asString() == "value");
  }

  SECTION("Integer value") {
    TOMLParser parser;
    auto result = parser.parse("key = 42");
    CHECK(parser.success());
    CHECK(result.tableValue["key"].asInteger() == 42);
  }

  SECTION("Negative integer") {
    TOMLParser parser;
    auto result = parser.parse("key = -10");
    CHECK(parser.success());
    CHECK(result.tableValue["key"].asInteger() == -10);
  }

  SECTION("Boolean true") {
    TOMLParser parser;
    auto result = parser.parse("key = true");
    CHECK(parser.success());
    CHECK(result.tableValue["key"].asBoolean() == true);
  }

  SECTION("Boolean false") {
    TOMLParser parser;
    auto result = parser.parse("key = false");
    CHECK(parser.success());
    CHECK(result.tableValue["key"].asBoolean() == false);
  }
}

TEST_CASE("TOMLParser parse strings", "[config]") {
  SECTION("Basic string") {
    TOMLParser parser;
    auto result = parser.parse("key = \"hello world\"");
    CHECK(result.tableValue["key"].asString() == "hello world");
  }

  SECTION("Empty string") {
    TOMLParser parser;
    auto result = parser.parse("key = \"\"");
    CHECK(result.tableValue["key"].asString() == "");
  }

  SECTION("String with spaces") {
    TOMLParser parser;
    auto result = parser.parse("key = \"  spaced  \"");
    CHECK(result.tableValue["key"].asString() == "  spaced  ");
  }
}

TEST_CASE("TOMLParser parse arrays", "[config]") {
  SECTION("Empty array") {
    TOMLParser parser;
    auto result = parser.parse("key = []");
    CHECK(parser.success());
    CHECK(result.tableValue["key"].isArray());
    CHECK(result.tableValue["key"].arrayValue.empty());
  }

  SECTION("Integer array") {
    TOMLParser parser;
    auto result = parser.parse("key = [1, 2, 3]");
    CHECK(parser.success());
    auto &arr = result.tableValue["key"].arrayValue;
    CHECK(arr.size() == 3);
    CHECK(arr[0].asInteger() == 1);
    CHECK(arr[1].asInteger() == 2);
    CHECK(arr[2].asInteger() == 3);
  }

  SECTION("String array") {
    TOMLParser parser;
    auto result = parser.parse("key = [\"a\", \"b\", \"c\"]");
    CHECK(parser.success());
    auto &arr = result.tableValue["key"].arrayValue;
    CHECK(arr.size() == 3);
    CHECK(arr[0].asString() == "a");
    CHECK(arr[1].asString() == "b");
    CHECK(arr[2].asString() == "c");
  }

  SECTION("Array with trailing comma") {
    TOMLParser parser;
    auto result = parser.parse("key = [1, 2,]");
    CHECK(parser.success());
    auto &arr = result.tableValue["key"].arrayValue;
    CHECK(arr.size() == 2);
  }
}

TEST_CASE("TOMLParser parse tables", "[config]") {
  SECTION("Simple table") {
    TOMLParser parser;
    auto result = parser.parse(R"([table]
key = "value")");
    CHECK(parser.success());
    CHECK(result.tableValue.find("table") != result.tableValue.end());
    CHECK(result.tableValue["table"].isTable());
    CHECK(result.tableValue["table"].tableValue["key"].asString() == "value");
  }

  SECTION("Nested table") {
    TOMLParser parser;
    auto result = parser.parse(R"([a.b.c]
key = "value")");
    CHECK(parser.success());
    CHECK(result.tableValue.find("a") != result.tableValue.end());
    auto &a = result.tableValue["a"].tableValue;
    CHECK(a.find("b") != a.end());
    auto &b = a["b"].tableValue;
    CHECK(b.find("c") != b.end());
    auto &c = b["c"].tableValue;
    CHECK(c["key"].asString() == "value");
  }

  SECTION("Table with multiple keys") {
    TOMLParser parser;
    auto result = parser.parse(R"([table]
key1 = "value1"
key2 = 42
key3 = true)");
    CHECK(parser.success());
    auto &table = result.tableValue["table"].tableValue;
    CHECK(table["key1"].asString() == "value1");
    CHECK(table["key2"].asInteger() == 42);
    CHECK(table["key3"].asBoolean() == true);
  }
}

TEST_CASE("TOMLParser parse inline tables", "[config]") {
  SECTION("Simple inline table") {
    TOMLParser parser;
    auto result = parser.parse("key = {a = 1, b = 2}");
    CHECK(parser.success());
    auto &table = result.tableValue["key"].tableValue;
    CHECK(table["a"].asInteger() == 1);
    CHECK(table["b"].asInteger() == 2);
  }

  SECTION("Empty inline table") {
    TOMLParser parser;
    auto result = parser.parse("key = {}");
    CHECK(parser.success());
    CHECK(result.tableValue["key"].isTable());
    CHECK(result.tableValue["key"].tableValue.empty());
  }
}

TEST_CASE("TOMLParser parse table arrays", "[config]") {
  SECTION("Simple table array") {
    TOMLParser parser;
    auto result = parser.parse(R"([[products]]
name = "Hammer"
[[products]]
name = "Nail")");
    CHECK(parser.success());
    CHECK(result.tableValue.find("products") != result.tableValue.end());
    auto &arr = result.tableValue["products"].arrayValue;
    CHECK(arr.size() == 2);
    CHECK(arr[0].tableValue["name"].asString() == "Hammer");
    CHECK(arr[1].tableValue["name"].asString() == "Nail");
  }

  SECTION("Table array with multiple fields") {
    TOMLParser parser;
    auto result = parser.parse(R"([[item]]
name = "test"
count = 5)");
    CHECK(parser.success());
    auto &arr = result.tableValue["item"].arrayValue;
    CHECK(arr[0].tableValue["name"].asString() == "test");
    CHECK(arr[0].tableValue["count"].asInteger() == 5);
  }
}

TEST_CASE("TOMLParser comments", "[config]") {
  SECTION("Full line comment") {
    TOMLParser parser;
    auto result = parser.parse("# This is a comment\nkey = 1");
    CHECK(parser.success());
    CHECK(result.tableValue["key"].asInteger() == 1);
  }

  SECTION("Comment after value") {
    TOMLParser parser;
    auto result = parser.parse("key = 1 # This is a comment");
    CHECK(parser.success());
    CHECK(result.tableValue["key"].asInteger() == 1);
  }

  SECTION("Multiple comments") {
    TOMLParser parser;
    auto result = parser.parse("# Comment 1\n# Comment 2\nkey = 1");
    CHECK(parser.success());
    CHECK(result.tableValue["key"].asInteger() == 1);
  }
}

TEST_CASE("TOMLParser edge cases", "[config]") {
  SECTION("Empty document") {
    TOMLParser parser;
    auto result = parser.parse("");
    CHECK(parser.success());
    CHECK(result.isTable());
    CHECK(result.tableValue.empty());
  }

  SECTION("Whitespace only") {
    TOMLParser parser;
    auto result = parser.parse("   \n\t  ");
    CHECK(parser.success());
    CHECK(result.tableValue.empty());
  }

  SECTION("Key with underscore") {
    TOMLParser parser;
    auto result = parser.parse("my_key = 1");
    CHECK(parser.success());
    CHECK(result.tableValue["my_key"].asInteger() == 1);
  }

  SECTION("Key with number") {
    TOMLParser parser;
    auto result = parser.parse("key123 = 1");
    CHECK(parser.success());
    CHECK(result.tableValue["key123"].asInteger() == 1);
  }

  SECTION("Multiple key-value pairs") {
    TOMLParser parser;
    auto result = parser.parse("key1 = 1\nkey2 = 2\nkey3 = 3");
    CHECK(parser.success());
    CHECK(result.tableValue["key1"].asInteger() == 1);
    CHECK(result.tableValue["key2"].asInteger() == 2);
    CHECK(result.tableValue["key3"].asInteger() == 3);
  }
}

TEST_CASE("TOMLParser real-world examples", "[config]") {
  SECTION("Package configuration") {
    TOMLParser parser;
    auto result = parser.parse(R"([package]
name = "my-project"
version = "1.0.0"
edition = "2024"

[dependencies]
meadows = "1.0"
llvm = "17.0"

[build]
target = "x86_64-unknown-linux-gnu"
optimize = true)");

    CHECK(parser.success());
    CHECK(result.tableValue["package"].tableValue["name"].asString() ==
          "my-project");
    CHECK(result.tableValue["package"].tableValue["version"].asString() ==
          "1.0.0");
    CHECK(result.tableValue["dependencies"].tableValue["meadows"].asString() ==
          "1.0");
    CHECK(result.tableValue["build"].tableValue["optimize"].asBoolean() ==
          true);
  }

  SECTION("Compiler configuration") {
    TOMLParser parser;
    auto result = parser.parse(R"([compiler]
version = "1.0.1"

[compiler.options]
optimize = true
debug = false)");

    CHECK(parser.success());
    CHECK(result.tableValue["compiler"].tableValue["version"].asString() ==
          "1.0.1");
    CHECK(result.tableValue["compiler"]
              .tableValue["options"]
              .tableValue["optimize"]
              .asBoolean() == true);
  }
}
