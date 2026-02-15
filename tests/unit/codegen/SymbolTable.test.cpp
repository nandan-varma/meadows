#include "codegen/SymbolTable.h"
#include "catch_amalgamated.hpp"
#include <llvm/IR/Constants.h>
#include <llvm/IR/LLVMContext.h>

TEST_CASE("SymbolTable basic operations", "[symboltable]") {
  SymbolTable table;

  SECTION("Empty table has no symbols") {
    CHECK(table.lookup("x") == nullptr);
    CHECK(table.exists("x") == false);
    CHECK(table.currentScopeLevel() == -1);
    CHECK(table.scopeDepth() == 0);
  }

  SECTION("Declare global variable") {
    auto context = std::make_unique<llvm::LLVMContext>();
    auto value = llvm::ConstantInt::get(llvm::Type::getInt32Ty(*context), 42);

    CHECK(table.declare("global_var", value) == true);
    CHECK(table.exists("global_var") == true);
    CHECK(table.lookup("global_var") == value);
  }

  SECTION("Cannot redeclare in same scope") {
    auto context = std::make_unique<llvm::LLVMContext>();
    auto value1 = llvm::ConstantInt::get(llvm::Type::getInt32Ty(*context), 1);
    auto value2 = llvm::ConstantInt::get(llvm::Type::getInt32Ty(*context), 2);

    CHECK(table.declare("x", value1) == true);
    CHECK(table.declare("x", value2) == false);
    CHECK(table.lookup("x") == value1);
  }
}

TEST_CASE("SymbolTable scope management", "[symboltable]") {
  SymbolTable table;
  auto context = std::make_unique<llvm::LLVMContext>();

  SECTION("Enter and exit scope") {
    CHECK(table.scopeDepth() == 0);

    table.enterScope();
    CHECK(table.scopeDepth() == 1);
    CHECK(table.currentScopeLevel() == 0);

    table.enterScope();
    CHECK(table.scopeDepth() == 2);
    CHECK(table.currentScopeLevel() == 1);

    table.exitScope();
    CHECK(table.scopeDepth() == 1);
    CHECK(table.currentScopeLevel() == 0);

    table.exitScope();
    CHECK(table.scopeDepth() == 0);
    CHECK(table.currentScopeLevel() == -1);
  }

  SECTION("Exit empty scope is safe") {
    table.exitScope();
    CHECK(table.scopeDepth() == 0);
  }

  SECTION("Multiple enters and exits") {
    for (int i = 0; i < 100; ++i) {
      table.enterScope();
    }
    CHECK(table.scopeDepth() == 100);

    for (int i = 0; i < 100; ++i) {
      table.exitScope();
    }
    CHECK(table.scopeDepth() == 0);
  }
}

TEST_CASE("SymbolTable variable shadowing", "[symboltable]") {
  SymbolTable table;
  auto context = std::make_unique<llvm::LLVMContext>();

  auto globalVal = llvm::ConstantInt::get(llvm::Type::getInt32Ty(*context), 0);
  auto localVal1 = llvm::ConstantInt::get(llvm::Type::getInt32Ty(*context), 1);
  auto localVal2 = llvm::ConstantInt::get(llvm::Type::getInt32Ty(*context), 2);

  SECTION("Global scope variables") {
    table.declare("x", globalVal);
    CHECK(table.lookup("x") == globalVal);
    CHECK(table.lookupCurrentScope("x") == globalVal);
  }

  SECTION("Local scope shadows global") {
    table.declare("x", globalVal);

    table.enterScope();
    CHECK(table.lookup("x") == globalVal);

    table.declare("x", localVal1);
    CHECK(table.lookup("x") == localVal1);
    CHECK(table.lookupCurrentScope("x") == localVal1);

    table.exitScope();
    CHECK(table.lookup("x") == globalVal);
  }

  SECTION("Nested shadowing") {
    table.declare("x", globalVal);

    table.enterScope();
    table.declare("x", localVal1);
    CHECK(table.lookup("x") == localVal1);

    table.enterScope();
    table.declare("x", localVal2);
    CHECK(table.lookup("x") == localVal2);

    table.exitScope();
    CHECK(table.lookup("x") == localVal1);

    table.exitScope();
    CHECK(table.lookup("x") == globalVal);
  }
}

TEST_CASE("SymbolTable lookup operations", "[symboltable]") {
  SymbolTable table;
  auto context = std::make_unique<llvm::LLVMContext>();

  auto val1 = llvm::ConstantInt::get(llvm::Type::getInt32Ty(*context), 1);
  auto val2 = llvm::ConstantInt::get(llvm::Type::getInt32Ty(*context), 2);
  auto val3 = llvm::ConstantInt::get(llvm::Type::getInt32Ty(*context), 3);

  SECTION("Lookup in current scope only") {
    table.enterScope();
    table.declare("x", val1);
    CHECK(table.lookupCurrentScope("x") == val1);

    table.enterScope();
    CHECK(table.lookupCurrentScope("x") == nullptr);
    CHECK(table.lookup("x") == val1);

    table.exitScope();
    table.exitScope();
  }

  SECTION("Lookup traverses scope chain") {
    table.enterScope();
    table.declare("a", val1);

    table.enterScope();
    table.declare("b", val2);

    table.enterScope();
    table.declare("c", val3);

    CHECK(table.lookup("a") == val1);
    CHECK(table.lookup("b") == val2);
    CHECK(table.lookup("c") == val3);

    table.exitScope();
    table.exitScope();
    table.exitScope();
  }

  SECTION("Lookup undefined variable") {
    CHECK(table.lookup("undefined") == nullptr);
    CHECK(table.lookupCurrentScope("undefined") == nullptr);
  }
}

TEST_CASE("SymbolTable exists operation", "[symboltable]") {
  SymbolTable table;
  auto context = std::make_unique<llvm::LLVMContext>();
  auto val = llvm::ConstantInt::get(llvm::Type::getInt32Ty(*context), 42);

  SECTION("Exists checks all scopes") {
    table.enterScope();
    table.declare("local", val);
    CHECK(table.exists("local") == true);

    table.enterScope();
    CHECK(table.exists("local") == true);

    table.exitScope();
    table.exitScope();
  }

  SECTION("Does not exist") {
    CHECK(table.exists("nonexistent") == false);

    table.enterScope();
    CHECK(table.exists("nonexistent") == false);
    table.exitScope();
  }
}

TEST_CASE("SymbolTable complex scenarios", "[symboltable]") {
  SymbolTable table;
  auto context = std::make_unique<llvm::LLVMContext>();

  SECTION("Multiple variables in same scope") {
    table.enterScope();

    for (int i = 0; i < 100; ++i) {
      auto val = llvm::ConstantInt::get(llvm::Type::getInt32Ty(*context), i);
      table.declare("var" + std::to_string(i), val);
    }

    for (int i = 0; i < 100; ++i) {
      auto result = table.lookup("var" + std::to_string(i));
      REQUIRE(result != nullptr);
    }

    table.exitScope();
  }

  SECTION("Variables in different scopes") {
    auto val0 = llvm::ConstantInt::get(llvm::Type::getInt32Ty(*context), 0);
    auto val1 = llvm::ConstantInt::get(llvm::Type::getInt32Ty(*context), 1);
    auto val2 = llvm::ConstantInt::get(llvm::Type::getInt32Ty(*context), 2);

    table.enterScope();
    table.declare("x", val0);

    table.enterScope();
    table.declare("y", val1);

    table.enterScope();
    table.declare("z", val2);

    CHECK(table.lookup("x") == val0);
    CHECK(table.lookup("y") == val1);
    CHECK(table.lookup("z") == val2);

    table.exitScope();
    table.exitScope();
    table.exitScope();
  }

  SECTION("Redeclaration attempt fails") {
    table.enterScope();

    auto val1 = llvm::ConstantInt::get(llvm::Type::getInt32Ty(*context), 1);
    auto val2 = llvm::ConstantInt::get(llvm::Type::getInt32Ty(*context), 2);

    CHECK(table.declare("x", val1) == true);
    CHECK(table.declare("x", val2) == false);

    table.exitScope();
  }
}

TEST_CASE("SymbolTable scope level tracking", "[symboltable]") {
  SymbolTable table;

  SECTION("Scope levels increment correctly") {
    CHECK(table.currentScopeLevel() == -1);

    table.enterScope();
    CHECK(table.currentScopeLevel() == 0);

    table.enterScope();
    CHECK(table.currentScopeLevel() == 1);

    table.enterScope();
    CHECK(table.currentScopeLevel() == 2);
  }

  SECTION("Scope levels decrement correctly") {
    table.enterScope();
    table.enterScope();
    table.enterScope();
    CHECK(table.currentScopeLevel() == 2);

    table.exitScope();
    CHECK(table.currentScopeLevel() == 1);

    table.exitScope();
    CHECK(table.currentScopeLevel() == 0);

    table.exitScope();
    CHECK(table.currentScopeLevel() == -1);
  }

  SECTION("Scope depth tracking") {
    CHECK(table.scopeDepth() == 0);

    for (int i = 1; i <= 50; ++i) {
      table.enterScope();
      CHECK(table.scopeDepth() == i);
    }

    for (int i = 49; i >= 0; --i) {
      table.exitScope();
      CHECK(table.scopeDepth() == i);
    }
  }
}

TEST_CASE("SymbolTable edge cases", "[symboltable]") {
  SymbolTable table;
  auto context = std::make_unique<llvm::LLVMContext>();

  SECTION("Empty variable name") {
    auto val = llvm::ConstantInt::get(llvm::Type::getInt32Ty(*context), 1);
    CHECK(table.declare("", val) == true);
    CHECK(table.exists("") == true);
  }

  SECTION("Long variable name") {
    std::string longName(1000, 'a');
    auto val = llvm::ConstantInt::get(llvm::Type::getInt32Ty(*context), 1);
    CHECK(table.declare(longName, val) == true);
    CHECK(table.exists(longName) == true);
  }

  SECTION("Special characters in names") {
    auto val = llvm::ConstantInt::get(llvm::Type::getInt32Ty(*context), 1);

    std::vector<std::string> names = {"_var", "var_123",   "VAR",   "vAr",
                                      "_",    "__private", "a_b_c", "x1y2z3"};

    for (const auto &name : names) {
      CHECK(table.declare(name, val) == true);
      CHECK(table.exists(name) == true);
    }
  }

  SECTION("Null value handling") {
    table.enterScope();
    CHECK(table.declare("null_var", nullptr) == true);
    CHECK(table.lookup("null_var") == nullptr);
    CHECK(table.exists("null_var") == true);
    table.exitScope();
  }

  SECTION("Rapid scope changes") {
    for (int i = 0; i < 1000; ++i) {
      table.enterScope();
      table.exitScope();
    }
    CHECK(table.scopeDepth() == 0);
  }
}
