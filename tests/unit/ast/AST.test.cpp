#include "ast/AST.h"
#include <catch2/catch_all.hpp>
#include <memory>

TEST_CASE("AST node types are correct", "[ast]") {
  SECTION("LiteralExpr stores string value") {
    auto literal = std::make_unique<LiteralExpr>("42");
    CHECK(literal->value == "42");
  }

  SECTION("VarExpr stores name") {
    auto var = std::make_unique<VarExpr>("myVar");
    CHECK(var->name == "myVar");
  }

  SECTION("BinaryExpr stores operator") {
    auto left = std::make_unique<LiteralExpr>("5");
    auto right = std::make_unique<LiteralExpr>("3");
    auto binary =
        std::make_unique<BinaryExpr>(std::move(left), "+", std::move(right));
    CHECK(binary->op == "+");
  }

  SECTION("UnaryExpr stores operator") {
    auto operand = std::make_unique<LiteralExpr>("5");
    auto unary = std::make_unique<UnaryExpr>("-", std::move(operand));
    CHECK(unary->op == "-");
  }
}

TEST_CASE("AST node hierarchy", "[ast]") {
  SECTION("All expressions derive from Expr") {
    std::unique_ptr<Expr> literal = std::make_unique<LiteralExpr>("test");
    std::unique_ptr<Expr> var = std::make_unique<VarExpr>("x");
    std::unique_ptr<Expr> binary =
        std::make_unique<BinaryExpr>(std::make_unique<LiteralExpr>("1"), "+",
                                     std::make_unique<LiteralExpr>("2"));
    std::unique_ptr<Expr> unary =
        std::make_unique<UnaryExpr>("-", std::make_unique<LiteralExpr>("5"));
    std::unique_ptr<Expr> call = std::make_unique<CallExpr>(
        std::make_unique<VarExpr>("foo"), std::vector<std::unique_ptr<Expr>>());

    CHECK(literal != nullptr);
    CHECK(var != nullptr);
    CHECK(binary != nullptr);
    CHECK(unary != nullptr);
    CHECK(call != nullptr);
  }

  SECTION("All statements derive from Stmt") {
    std::unique_ptr<Stmt> exprStmt =
        std::make_unique<ExprStmt>(std::make_unique<LiteralExpr>("1"));
    std::unique_ptr<Stmt> letStmt =
        std::make_unique<LetStmt>("x", std::make_unique<LiteralExpr>("5"));

    CHECK(exprStmt != nullptr);
    CHECK(letStmt != nullptr);
  }
}

TEST_CASE("AST collections", "[ast]") {
  SECTION("ArrayExpr stores elements") {
    std::vector<std::unique_ptr<Expr>> elements;
    elements.push_back(std::make_unique<LiteralExpr>("1"));
    elements.push_back(std::make_unique<LiteralExpr>("2"));
    elements.push_back(std::make_unique<LiteralExpr>("3"));

    auto array = std::make_unique<ArrayExpr>(std::move(elements));
    CHECK(array->elements.size() == 3);
  }

  SECTION("ObjectExpr stores key-value pairs") {
    std::unordered_map<std::string, std::unique_ptr<Expr>> pairs;
    pairs["name"] = std::make_unique<LiteralExpr>("Alice");
    pairs["age"] = std::make_unique<LiteralExpr>("30");

    auto obj = std::make_unique<ObjectExpr>(std::move(pairs));
    CHECK(obj->pairs.size() == 2);
  }

  SECTION("FuncStmt stores parameters and body") {
    std::vector<std::string> params = {"a", "b", "c"};
    std::vector<std::unique_ptr<Stmt>> body;
    body.push_back(std::make_unique<ReturnStmt>(std::make_unique<BinaryExpr>(
        std::make_unique<VarExpr>("a"), "+", std::make_unique<VarExpr>("b"))));

    auto func =
        std::make_unique<FuncStmt>("add", std::move(params), std::move(body));
    CHECK(func->params.size() == 3);
    CHECK(func->body.size() == 1);
  }
}

TEST_CASE("IfStmt branches", "[ast]") {
  SECTION("IfStmt with only then branch") {
    auto condition = std::make_unique<LiteralExpr>("1");
    std::vector<std::unique_ptr<Stmt>> thenBranch;
    thenBranch.push_back(
        std::make_unique<ExprStmt>(std::make_unique<LiteralExpr>("true")));

    auto ifStmt = std::make_unique<IfStmt>(std::move(condition),
                                           std::move(thenBranch), {});
    CHECK(ifStmt->thenBranch.size() == 1);
    CHECK(ifStmt->elseBranch.empty());
  }

  SECTION("IfStmt with both branches") {
    auto condition = std::make_unique<LiteralExpr>("1");
    std::vector<std::unique_ptr<Stmt>> thenBranch;
    thenBranch.push_back(
        std::make_unique<ExprStmt>(std::make_unique<LiteralExpr>("then")));
    std::vector<std::unique_ptr<Stmt>> elseBranch;
    elseBranch.push_back(
        std::make_unique<ExprStmt>(std::make_unique<LiteralExpr>("else")));

    auto ifStmt = std::make_unique<IfStmt>(
        std::move(condition), std::move(thenBranch), std::move(elseBranch));
    CHECK(ifStmt->thenBranch.size() == 1);
    CHECK(ifStmt->elseBranch.size() == 1);
  }
}

TEST_CASE("Loop constructs", "[ast]") {
  SECTION("ForStmt stores range and body") {
    auto forStmt =
        std::make_unique<ForStmt>("i", std::make_unique<LiteralExpr>("0"),
                                  std::make_unique<LiteralExpr>("10"),
                                  std::vector<std::unique_ptr<Stmt>>());

    CHECK(forStmt->var == "i");
    CHECK(forStmt->body.empty());
  }

  SECTION("WhileStmt stores condition and body") {
    auto condition = std::make_unique<LiteralExpr>("1");
    std::vector<std::unique_ptr<Stmt>> body;
    body.push_back(
        std::make_unique<ExprStmt>(std::make_unique<LiteralExpr>("loop")));

    auto whileStmt =
        std::make_unique<WhileStmt>(std::move(condition), std::move(body));
    CHECK(whileStmt->body.size() == 1);
  }
}
