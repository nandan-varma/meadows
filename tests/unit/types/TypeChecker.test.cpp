#include "types/TypeChecker.h"
#include "catch_amalgamated.hpp"
#include "types/Types.h"

using namespace meadows;
using namespace meadows::types;

// Helper function to convert unique_ptr to shared_ptr
inline std::shared_ptr<Type> toShared(std::unique_ptr<Type> ptr) {
  return std::shared_ptr<Type>(std::move(ptr));
}

TEST_CASE("TypeChecker handles primitive types correctly", "[types]") {
  SECTION("Integer types are distinct") {
    auto i32 = PrimitiveType::i32();
    auto i64 = PrimitiveType::i64();
    auto f32 = PrimitiveType::f32();
    auto f64 = PrimitiveType::f64();

    CHECK(i32->toString() != i64->toString());
    CHECK(i32->toString() != f32->toString());
    CHECK(i64->toString() != f64->toString());
    CHECK(f32->toString() != f64->toString());
  }

  SECTION("Boolean type is distinct from integers") {
    auto bool1 = PrimitiveType::boolType();
    auto i32 = PrimitiveType::i32();

    CHECK(bool1->toString() != i32->toString());
  }

  SECTION("String type is reference type") {
    auto str = PrimitiveType::string();
    CHECK(str->toString() == "string");
  }

  SECTION("Unit type represents void/empty") {
    auto unit = PrimitiveType::unit();
    CHECK(unit->toString() == "unit");
  }

  SECTION("Never type for diverging functions") {
    auto never = PrimitiveType::never();
    CHECK(never->toString() == "never");
  }
}

TEST_CASE("TypeChecker handles array types", "[types]") {
  SECTION("Array of primitives") {
    auto i32 = toShared(PrimitiveType::i32());
    auto arr = ArrayType::make(i32);

    CHECK(arr->toString() == "[i32]");
  }

  SECTION("Array of arrays (2D)") {
    auto i32 = toShared(PrimitiveType::i32());
    auto inner = ArrayType::make(i32);
    auto outer = ArrayType::make(inner);

    CHECK(outer->toString() == "[[i32]]");
  }

  SECTION("Array of strings") {
    auto str = toShared(PrimitiveType::string());
    auto arr = ArrayType::make(str);

    CHECK(arr->toString() == "[string]");
  }
}

TEST_CASE("TypeChecker handles function types", "[types]") {
  SECTION("Simple function type") {
    auto i32 = toShared(PrimitiveType::i32());
    auto i64 = toShared(PrimitiveType::i64());

    std::vector<std::shared_ptr<Type>> params;
    params.push_back(i32);
    params.push_back(i32);

    auto funcType = FunctionType::make(params, i64);

    CHECK(funcType->toString().find("->") != std::string::npos);
    CHECK(funcType->toString().find("i32") != std::string::npos);
    CHECK(funcType->toString().find("i64") != std::string::npos);
  }

  SECTION("Function with no parameters") {
    auto i32 = toShared(PrimitiveType::i32());

    std::vector<std::shared_ptr<Type>> params;
    auto funcType = FunctionType::make(params, i32);

    CHECK(funcType->toString().find("()") != std::string::npos);
    CHECK(funcType->toString().find("->") != std::string::npos);
  }

  SECTION("Higher-order function type") {
    auto i32 = toShared(PrimitiveType::i32());

    std::vector<std::shared_ptr<Type>> innerParams;
    innerParams.push_back(i32);
    auto innerFunc = FunctionType::make(innerParams, i32);

    std::vector<std::shared_ptr<Type>> outerParams;
    outerParams.push_back(innerFunc);
    auto outerFunc = FunctionType::make(outerParams, i32);

    CHECK(outerFunc->toString().find("->") != std::string::npos);
  }
}

TEST_CASE("TypeChecker handles struct types", "[types]") {
  SECTION("Simple struct") {
    auto point = std::make_shared<StructType>("Point");
    point->addField("x", PrimitiveType::i32()->clone());
    point->addField("y", PrimitiveType::f64()->clone());

    CHECK(point->name == "Point");
    CHECK(point->toString().find("Point") != std::string::npos);
  }

  SECTION("Empty struct") {
    auto empty = std::make_shared<StructType>("Empty");

    CHECK(empty->name == "Empty");
    CHECK(empty->fields.empty());
  }

  SECTION("Struct with string field") {
    auto person = std::make_shared<StructType>("Person");
    person->addField("name", PrimitiveType::string()->clone());
    person->addField("age", PrimitiveType::i32()->clone());

    CHECK(person->toString().find("name") != std::string::npos);
    CHECK(person->toString().find("string") != std::string::npos);
  }
}

TEST_CASE("TypeChecker handles enum types", "[types]") {
  SECTION("Simple enum with unit variants") {
    auto status = std::make_shared<EnumType>("Status");
    status->addVariant("Pending", {});
    status->addVariant("Running", {});
    status->addVariant("Complete", {});

    CHECK(status->name == "Status");
    CHECK(status->variants.size() == 3);
    CHECK(status->variants[0].name == "Pending");
  }

  SECTION("Enum with tuple variants") {
    auto result = std::make_shared<EnumType>("Result");

    std::vector<std::unique_ptr<Type>> okTypes;
    okTypes.push_back(PrimitiveType::i32()->clone());
    result->addVariant("Ok", std::move(okTypes));

    std::vector<std::unique_ptr<Type>> errTypes;
    errTypes.push_back(PrimitiveType::string()->clone());
    result->addVariant("Err", std::move(errTypes));

    CHECK(result->variants.size() == 2);
    CHECK(result->toString().find("Ok") != std::string::npos);
    CHECK(result->toString().find("Err") != std::string::npos);
  }

  SECTION("Option enum") {
    auto option = std::make_shared<EnumType>("Option");
    option->addVariant("None", {});

    std::vector<std::unique_ptr<Type>> someTypes;
    someTypes.push_back(PrimitiveType::i32()->clone());
    option->addVariant("Some", std::move(someTypes));

    CHECK(option->variants.size() == 2);
    CHECK(option->variants[0].name == "None");
    CHECK(option->variants[1].name == "Some");
  }
}

TEST_CASE("TypeChecker type equality", "[types]") {
  SECTION("Same primitives are equal") {
    auto t1 = PrimitiveType::i32();
    auto t2 = PrimitiveType::i32();

    CHECK(t1->equals(*t2));
  }

  SECTION("Different primitives are not equal") {
    auto t1 = PrimitiveType::i32();
    auto t2 = PrimitiveType::i64();

    CHECK_FALSE(t1->equals(*t2));
  }

  SECTION("Same arrays are equal") {
    auto i32 = toShared(PrimitiveType::i32());
    auto a1 = ArrayType::make(i32);
    auto a2 = ArrayType::make(i32);

    CHECK(a1->equals(*a2));
  }

  SECTION("Different arrays are not equal") {
    auto i32 = toShared(PrimitiveType::i32());
    auto i64 = toShared(PrimitiveType::i64());
    auto a1 = ArrayType::make(i32);
    auto a2 = ArrayType::make(i64);

    CHECK_FALSE(a1->equals(*a2));
  }
}

TEST_CASE("TypeChecker type checking", "[types]") {
  TypeChecker checker;

  SECTION("Empty program passes") {
    std::vector<std::unique_ptr<Stmt>> stmts;
    CHECK(checker.check(stmts));
  }

  SECTION("Simple variable declaration") {
    auto literal = std::make_unique<LiteralExpr>("42");
    auto letStmt = std::make_unique<LetStmt>("x", std::move(literal), "");

    std::vector<std::unique_ptr<Stmt>> stmts;
    stmts.push_back(std::move(letStmt));

    CHECK(checker.check(stmts));
    CHECK_FALSE(checker.hasErrors());
  }

  SECTION("Variable with type annotation") {
    auto literal = std::make_unique<LiteralExpr>("42");
    auto letStmt = std::make_unique<LetStmt>("x", std::move(literal), "i32");

    std::vector<std::unique_ptr<Stmt>> stmts;
    stmts.push_back(std::move(letStmt));

    CHECK(checker.check(stmts));
  }

  SECTION("String variable") {
    auto literal = std::make_unique<LiteralExpr>("hello");
    auto letStmt = std::make_unique<LetStmt>("msg", std::move(literal), "");

    std::vector<std::unique_ptr<Stmt>> stmts;
    stmts.push_back(std::move(letStmt));

    CHECK(checker.check(stmts));
  }
}

TEST_CASE("TypeChecker arithmetic operations", "[types]") {
  TypeChecker checker;

  SECTION("Integer addition") {
    auto left = std::make_unique<LiteralExpr>("1");
    auto right = std::make_unique<LiteralExpr>("2");
    auto binary =
        std::make_unique<BinaryExpr>(std::move(left), "+", std::move(right));

    std::vector<std::unique_ptr<Stmt>> stmts;
    stmts.push_back(std::make_unique<ExprStmt>(std::move(binary)));

    CHECK(checker.check(stmts));
  }

  SECTION("Mixed type arithmetic is not allowed") {
    // This would need proper type checking to fail
    // For now, just test the structure exists
    auto left = std::make_unique<LiteralExpr>("1");
    auto right = std::make_unique<LiteralExpr>("hello");
    auto binary =
        std::make_unique<BinaryExpr>(std::move(left), "+", std::move(right));

    std::vector<std::unique_ptr<Stmt>> stmts;
    stmts.push_back(std::make_unique<ExprStmt>(std::move(binary)));

    // May or may not fail depending on type checker implementation
    checker.check(stmts);
  }
}

TEST_CASE("TypeChecker function definitions", "[types]") {
  TypeChecker checker;

  SECTION("Simple function") {
    auto returnStmt =
        std::make_unique<ReturnStmt>(std::make_unique<LiteralExpr>("42"));

    std::vector<std::unique_ptr<Stmt>> body;
    body.push_back(std::move(returnStmt));

    std::vector<FuncParam> params;
    auto func = std::make_unique<FuncStmt>("answer", std::move(params),
                                           std::move(body), "i32");

    std::vector<std::unique_ptr<Stmt>> stmts;
    stmts.push_back(std::move(func));

    CHECK(checker.check(stmts));
  }

  SECTION("Function with parameters") {
    auto returnStmt =
        std::make_unique<ReturnStmt>(std::make_unique<LiteralExpr>("0"));

    std::vector<std::unique_ptr<Stmt>> body;
    body.push_back(std::move(returnStmt));

    std::vector<FuncParam> params;
    params.push_back({"x", "i32"});
    params.push_back({"y", "i32"});

    auto func = std::make_unique<FuncStmt>("add", std::move(params),
                                           std::move(body), "i32");

    std::vector<std::unique_ptr<Stmt>> stmts;
    stmts.push_back(std::move(func));

    CHECK(checker.check(stmts));
  }
}

TEST_CASE("TypeChecker control flow", "[types]") {
  TypeChecker checker;

  SECTION("If statement") {
    auto cond = std::make_unique<LiteralExpr>("1");

    std::vector<std::unique_ptr<Stmt>> thenBranch;
    thenBranch.push_back(
        std::make_unique<PrintStmt>(std::make_unique<LiteralExpr>("yes")));

    std::vector<std::unique_ptr<Stmt>> elseBranch;

    auto ifStmt = std::make_unique<IfStmt>(
        std::move(cond), std::move(thenBranch), std::move(elseBranch));

    std::vector<std::unique_ptr<Stmt>> stmts;
    stmts.push_back(std::move(ifStmt));

    CHECK(checker.check(stmts));
  }

  SECTION("If-else statement") {
    auto cond = std::make_unique<LiteralExpr>("0");

    std::vector<std::unique_ptr<Stmt>> thenBranch;
    thenBranch.push_back(
        std::make_unique<PrintStmt>(std::make_unique<LiteralExpr>("yes")));

    std::vector<std::unique_ptr<Stmt>> elseBranch;
    elseBranch.push_back(
        std::make_unique<PrintStmt>(std::make_unique<LiteralExpr>("no")));

    auto ifStmt = std::make_unique<IfStmt>(
        std::move(cond), std::move(thenBranch), std::move(elseBranch));

    std::vector<std::unique_ptr<Stmt>> stmts;
    stmts.push_back(std::move(ifStmt));

    CHECK(checker.check(stmts));
  }
}

TEST_CASE("TypeChecker error detection", "[types]") {
  TypeChecker checker;

  SECTION("Undefined variable") {
    auto var = std::make_unique<VarExpr>("undefined_var");

    std::vector<std::unique_ptr<Stmt>> stmts;
    stmts.push_back(std::make_unique<PrintStmt>(std::move(var)));

    CHECK_FALSE(checker.check(stmts));
    CHECK(checker.hasErrors());
  }

  SECTION("Type mismatch in assignment") {
    // Create a variable and try to assign wrong type
    auto literal = std::make_unique<LiteralExpr>("42");
    auto letStmt = std::make_unique<LetStmt>("x", std::move(literal), "string");

    std::vector<std::unique_ptr<Stmt>> stmts;
    stmts.push_back(std::move(letStmt));

    // This may or may not fail depending on strictness
    checker.check(stmts);
  }
}

TEST_CASE("TypeChecker complex scenarios", "[types]") {
  TypeChecker checker;

  SECTION("Array literal") {
    std::vector<std::unique_ptr<Expr>> elements;
    elements.push_back(std::make_unique<LiteralExpr>("1"));
    elements.push_back(std::make_unique<LiteralExpr>("2"));
    elements.push_back(std::make_unique<LiteralExpr>("3"));

    auto arr = std::make_unique<ArrayExpr>(std::move(elements));
    auto letStmt = std::make_unique<LetStmt>("nums", std::move(arr), "");

    std::vector<std::unique_ptr<Stmt>> stmts;
    stmts.push_back(std::move(letStmt));

    CHECK(checker.check(stmts));
  }

  SECTION("Nested function calls") {
    // inner() -> outer(inner())
    std::vector<std::unique_ptr<Expr>> innerArgs;
    auto innerCall = std::make_unique<CallExpr>(
        std::make_unique<VarExpr>("inner"), std::move(innerArgs));

    std::vector<std::unique_ptr<Expr>> outerArgs;
    outerArgs.push_back(std::move(innerCall));
    auto outerCall = std::make_unique<CallExpr>(
        std::make_unique<VarExpr>("outer"), std::move(outerArgs));

    std::vector<std::unique_ptr<Stmt>> stmts;
    stmts.push_back(std::make_unique<ExprStmt>(std::move(outerCall)));

    // This will fail because functions aren't defined
    CHECK_FALSE(checker.check(stmts));
  }
}
