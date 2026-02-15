#include "types/Types.h"
#include "catch_amalgamated.hpp"

using namespace meadows::types;

// Helper to convert unique_ptr to shared_ptr
inline std::shared_ptr<Type> toShared(std::unique_ptr<Type> ptr) {
  return std::shared_ptr<Type>(std::move(ptr));
}

TEST_CASE("Types primitive type factories", "[types]") {
  SECTION("i32 factory creates correct type") {
    auto i32 = PrimitiveType::i32();
    CHECK(i32->kind == PrimitiveKind::I32);
    CHECK(i32->toString() == "i32");
  }

  SECTION("i64 factory creates correct type") {
    auto i64 = PrimitiveType::i64();
    CHECK(i64->kind == PrimitiveKind::I64);
    CHECK(i64->toString() == "i64");
  }

  SECTION("f32 factory creates correct type") {
    auto f32 = PrimitiveType::f32();
    CHECK(f32->kind == PrimitiveKind::F32);
    CHECK(f32->toString() == "f32");
  }

  SECTION("f64 factory creates correct type") {
    auto f64 = PrimitiveType::f64();
    CHECK(f64->kind == PrimitiveKind::F64);
    CHECK(f64->toString() == "f64");
  }

  SECTION("bool factory creates correct type") {
    auto b = PrimitiveType::boolType();
    CHECK(b->kind == PrimitiveKind::BOOL);
    CHECK(b->toString() == "bool");
  }

  SECTION("string factory creates correct type") {
    auto s = PrimitiveType::string();
    CHECK(s->kind == PrimitiveKind::STRING);
    CHECK(s->toString() == "string");
  }

  SECTION("unit factory creates correct type") {
    auto u = PrimitiveType::unit();
    CHECK(u->kind == PrimitiveKind::UNIT);
    CHECK(u->toString() == "unit");
  }

  SECTION("never factory creates correct type") {
    auto n = PrimitiveType::never();
    CHECK(n->kind == PrimitiveKind::NEVER);
    CHECK(n->toString() == "never");
  }
}

TEST_CASE("Types primitive equality", "[types]") {
  SECTION("Same primitives are equal") {
    auto t1 = PrimitiveType::i32();
    auto t2 = PrimitiveType::i32();
    CHECK(t1->equals(*t2));
    CHECK(t2->equals(*t1));
  }

  SECTION("Different primitives are not equal") {
    auto i32 = PrimitiveType::i32();
    auto i64 = PrimitiveType::i64();
    auto f32 = PrimitiveType::f32();
    auto boolean = PrimitiveType::boolType();

    CHECK_FALSE(i32->equals(*i64));
    CHECK_FALSE(i32->equals(*f32));
    CHECK_FALSE(i32->equals(*boolean));
    CHECK_FALSE(i64->equals(*f32));
  }

  SECTION("Primitive equals handles non-primitive") {
    auto i32 = PrimitiveType::i32();
    auto arr = ArrayType::make(toShared(PrimitiveType::i32()));
    CHECK_FALSE(i32->equals(*arr));
  }
}

TEST_CASE("Types primitive cloning", "[types]") {
  SECTION("Clone preserves primitive kind") {
    auto original = PrimitiveType::i32();
    auto cloned = original->clone();
    CHECK(cloned->equals(*original));
  }

  SECTION("Clone creates independent copy") {
    auto original = PrimitiveType::i32();
    auto cloned = original->clone();
    CHECK(cloned.get() != original.get());
  }
}

TEST_CASE("Types array operations", "[types]") {
  SECTION("Array of primitive") {
    auto arr = ArrayType::make(toShared(PrimitiveType::i32()));
    CHECK(arr->toString() == "[i32]");
  }

  SECTION("Array of array (2D)") {
    auto inner = ArrayType::make(toShared(PrimitiveType::i32()));
    auto outer = ArrayType::make(inner);
    CHECK(outer->toString() == "[[i32]]");
  }

  SECTION("Array of array (3D)") {
    auto inner = ArrayType::make(toShared(PrimitiveType::i32()));
    auto middle = ArrayType::make(inner);
    auto outer = ArrayType::make(middle);
    CHECK(outer->toString() == "[[[i32]]]");
  }

  SECTION("Array equality - same element type") {
    auto a1 = ArrayType::make(toShared(PrimitiveType::i32()));
    auto a2 = ArrayType::make(toShared(PrimitiveType::i32()));
    CHECK(a1->equals(*a2));
  }

  SECTION("Array equality - different element type") {
    auto a1 = ArrayType::make(toShared(PrimitiveType::i32()));
    auto a2 = ArrayType::make(toShared(PrimitiveType::i64()));
    CHECK_FALSE(a1->equals(*a2));
  }

  SECTION("Array getElementType") {
    auto arr = ArrayType::make(toShared(PrimitiveType::i32()));
    auto elem = arr->getElementType();
    CHECK(elem->equals(*PrimitiveType::i32()));
  }

  SECTION("Array clone") {
    auto original = ArrayType::make(toShared(PrimitiveType::i32()));
    auto cloned = original->clone();
    CHECK(cloned->equals(*original));
  }
}

TEST_CASE("Types function operations", "[types]") {
  SECTION("Simple function type") {
    std::vector<std::shared_ptr<Type>> params;
    params.push_back(toShared(PrimitiveType::i32()));
    auto ret = toShared(PrimitiveType::i64());
    auto func = FunctionType::make(params, ret);

    auto str = func->toString();
    CHECK(str.find("i32") != std::string::npos);
    CHECK(str.find("i64") != std::string::npos);
  }

  SECTION("Function with multiple parameters") {
    std::vector<std::shared_ptr<Type>> params;
    params.push_back(toShared(PrimitiveType::i32()));
    params.push_back(toShared(PrimitiveType::i32()));
    auto ret = toShared(PrimitiveType::i32());
    auto func = FunctionType::make(params, ret);

    CHECK(func->paramTypes.size() == 2);
  }

  SECTION("Function with no parameters") {
    std::vector<std::shared_ptr<Type>> params;
    auto ret = toShared(PrimitiveType::unit());
    auto func = FunctionType::make(params, ret);

    CHECK(func->paramTypes.empty());
  }

  SECTION("Higher-order function") {
    std::vector<std::shared_ptr<Type>> innerParams;
    innerParams.push_back(toShared(PrimitiveType::i32()));
    auto innerFunc =
        FunctionType::make(innerParams, toShared(PrimitiveType::i32()));

    std::vector<std::shared_ptr<Type>> outerParams;
    outerParams.push_back(innerFunc);
    auto outerFunc =
        FunctionType::make(outerParams, toShared(PrimitiveType::i32()));

    CHECK(outerFunc->paramTypes.size() == 1);
  }

  SECTION("Function equality - same signature") {
    std::vector<std::shared_ptr<Type>> params;
    params.push_back(toShared(PrimitiveType::i32()));
    auto ret = toShared(PrimitiveType::i64());
    auto f1 = FunctionType::make(params, ret);
    auto f2 = FunctionType::make(params, ret);

    CHECK(f1->equals(*f2));
  }

  SECTION("Function equality - different return") {
    std::vector<std::shared_ptr<Type>> params;
    params.push_back(toShared(PrimitiveType::i32()));
    auto f1 = FunctionType::make(params, toShared(PrimitiveType::i32()));
    auto f2 = FunctionType::make(params, toShared(PrimitiveType::i64()));

    CHECK_FALSE(f1->equals(*f2));
  }

  SECTION("Function getParamType") {
    std::vector<std::shared_ptr<Type>> params;
    params.push_back(toShared(PrimitiveType::i32()));
    params.push_back(toShared(PrimitiveType::i64()));
    auto func = FunctionType::make(params, toShared(PrimitiveType::unit()));

    CHECK(func->getParamType(0)->equals(*PrimitiveType::i32()));
    CHECK(func->getParamType(1)->equals(*PrimitiveType::i64()));
    CHECK(func->getParamType(2) == nullptr);
  }

  SECTION("Function getReturnType") {
    auto func = FunctionType::make({}, toShared(PrimitiveType::i32()));
    CHECK(func->getReturnType()->equals(*PrimitiveType::i32()));
  }
}

TEST_CASE("Types struct operations", "[types]") {
  SECTION("Empty struct") {
    auto s = std::make_shared<StructType>("Empty");
    CHECK(s->name == "Empty");
    CHECK(s->fields.empty());
    CHECK(s->toString().find("Empty") != std::string::npos);
  }

  SECTION("Struct with fields") {
    auto point = std::make_shared<StructType>("Point");
    point->addField("x", PrimitiveType::i32()->clone());
    point->addField("y", PrimitiveType::i32()->clone());

    CHECK(point->fields.size() == 2);
    CHECK(point->getField("x") != nullptr);
    CHECK(point->getField("y") != nullptr);
    CHECK(point->getField("z") == nullptr);
  }

  SECTION("Struct field types preserved") {
    auto person = std::make_shared<StructType>("Person");
    person->addField("name", PrimitiveType::string()->clone());
    person->addField("age", PrimitiveType::i32()->clone());

    auto nameType = person->getField("name");
    auto ageType = person->getField("age");

    REQUIRE(nameType != nullptr);
    REQUIRE(ageType != nullptr);
    CHECK(nameType->equals(*PrimitiveType::string()));
    CHECK(ageType->equals(*PrimitiveType::i32()));
  }

  SECTION("Struct equality - same fields") {
    auto s1 = std::make_shared<StructType>("Point");
    s1->addField("x", PrimitiveType::i32()->clone());

    auto s2 = std::make_shared<StructType>("Point");
    s2->addField("x", PrimitiveType::i32()->clone());

    CHECK(s1->equals(*s2));
  }

  SECTION("Struct equality - different field types") {
    auto s1 = std::make_shared<StructType>("Point");
    s1->addField("x", PrimitiveType::i32()->clone());

    auto s2 = std::make_shared<StructType>("Point");
    s2->addField("x", PrimitiveType::f64()->clone());

    CHECK_FALSE(s1->equals(*s2));
  }

  SECTION("Struct clone") {
    auto original = std::make_shared<StructType>("Point");
    original->addField("x", PrimitiveType::i32()->clone());
    original->addField("y", PrimitiveType::i32()->clone());

    auto cloned = original->clone();
    CHECK(cloned->equals(*original));
  }
}

TEST_CASE("Types enum operations", "[types]") {
  SECTION("Empty enum") {
    auto e = std::make_shared<EnumType>("Empty");
    CHECK(e->name == "Empty");
    CHECK(e->variants.empty());
  }

  SECTION("Enum with unit variants") {
    auto status = std::make_shared<EnumType>("Status");
    status->addVariant("Pending", {});
    status->addVariant("Running", {});
    status->addVariant("Complete", {});

    CHECK(status->variants.size() == 3);
    CHECK(status->variants[0].name == "Pending");
    CHECK(status->variants[1].name == "Running");
    CHECK(status->variants[2].name == "Complete");
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
    CHECK(result->variants[0].types.size() == 1);
    CHECK(result->variants[1].types.size() == 1);
  }

  SECTION("Enum equality - same variants") {
    auto e1 = std::make_shared<EnumType>("Status");
    e1->addVariant("Ok", {});

    auto e2 = std::make_shared<EnumType>("Status");
    e2->addVariant("Ok", {});

    CHECK(e1->equals(*e2));
  }

  SECTION("Enum equality - different variant count") {
    auto e1 = std::make_shared<EnumType>("Status");
    e1->addVariant("Ok", {});

    auto e2 = std::make_shared<EnumType>("Status");
    e2->addVariant("Ok", {});
    e2->addVariant("Err", {});

    CHECK_FALSE(e1->equals(*e2));
  }

  SECTION("Enum clone") {
    auto original = std::make_shared<EnumType>("Result");
    std::vector<std::unique_ptr<Type>> types;
    types.push_back(PrimitiveType::i32()->clone());
    original->addVariant("Ok", std::move(types));

    auto cloned = original->clone();
    CHECK(cloned->equals(*original));
  }
}

TEST_CASE("Types type variable operations", "[types]") {
  SECTION("Type variable with name") {
    TypeVariable tv("a");
    CHECK(tv.name == "a");
    CHECK(tv.toString() == "'a");
  }

  SECTION("Type variable without instance") {
    TypeVariable tv("a");
    CHECK(tv.instance == nullptr);
  }

  SECTION("Type variable with instance") {
    TypeVariable tv("a");
    tv.instance = toShared(PrimitiveType::i32());
    CHECK(tv.instance != nullptr);
  }

  SECTION("Type variable isGeneric") {
    TypeVariable tv1("'a");
    CHECK(tv1.isGeneric());

    TypeVariable tv2("a");
    CHECK_FALSE(tv2.isGeneric());

    TypeVariable tv3("");
    CHECK_FALSE(tv3.isGeneric());
  }

  SECTION("Type variable equality - no instance") {
    TypeVariable tv1("a");
    TypeVariable tv2("a");
    CHECK(tv1.equals(tv2));
  }

  SECTION("Type variable equality - different names") {
    TypeVariable tv1("a");
    TypeVariable tv2("b");
    CHECK_FALSE(tv1.equals(tv2));
  }
}

TEST_CASE("Types generic type operations", "[types]") {
  SECTION("Generic type with one parameter") {
    std::vector<std::unique_ptr<Type>> params;
    params.push_back(PrimitiveType::i32()->clone());
    GenericType gt("Box", std::move(params));

    CHECK(gt.name == "Box");
    CHECK(gt.typeParams.size() == 1);
  }

  SECTION("Generic type with multiple parameters") {
    std::vector<std::unique_ptr<Type>> params;
    params.push_back(PrimitiveType::i32()->clone());
    params.push_back(PrimitiveType::string()->clone());
    GenericType gt("Pair", std::move(params));

    CHECK(gt.typeParams.size() == 2);
  }

  SECTION("Generic type getTypeParam") {
    std::vector<std::unique_ptr<Type>> params;
    params.push_back(PrimitiveType::i32()->clone());
    GenericType gt("Box", std::move(params));

    auto param = gt.getTypeParam(0);
    CHECK(param != nullptr);
    CHECK(param->equals(*PrimitiveType::i32()));
    CHECK(gt.getTypeParam(1) == nullptr);
  }

  SECTION("Generic type equality") {
    std::vector<std::unique_ptr<Type>> params1;
    params1.push_back(PrimitiveType::i32()->clone());
    GenericType gt1("Box", std::move(params1));

    std::vector<std::unique_ptr<Type>> params2;
    params2.push_back(PrimitiveType::i32()->clone());
    GenericType gt2("Box", std::move(params2));

    CHECK(gt1.equals(gt2));
  }
}

TEST_CASE("Types reference type operations", "[types]") {
  SECTION("Immutable reference") {
    RefType rt(PrimitiveType::i32()->clone(), false);
    CHECK_FALSE(rt.mutable_);
    CHECK(rt.toString().find("&") != std::string::npos);
  }

  SECTION("Mutable reference") {
    RefType rt(PrimitiveType::i32()->clone(), true);
    CHECK(rt.mutable_);
    CHECK(rt.toString().find("&mut") != std::string::npos);
  }

  SECTION("Reference equality - same inner type") {
    RefType r1(PrimitiveType::i32()->clone(), false);
    RefType r2(PrimitiveType::i32()->clone(), false);
    CHECK(r1.equals(r2));
  }

  SECTION("Reference equality - different mutability") {
    RefType r1(PrimitiveType::i32()->clone(), false);
    RefType r2(PrimitiveType::i32()->clone(), true);
    CHECK_FALSE(r1.equals(r2));
  }
}

TEST_CASE("Types type helper methods", "[types]") {
  SECTION("isPrimitive") {
    CHECK(PrimitiveType::i32()->isPrimitive());
    CHECK_FALSE(ArrayType::make(toShared(PrimitiveType::i32()))->isPrimitive());
  }

  SECTION("isArray") {
    CHECK(ArrayType::make(toShared(PrimitiveType::i32()))->isArray());
    CHECK_FALSE(PrimitiveType::i32()->isArray());
  }

  SECTION("isFunction") {
    auto func = FunctionType::make({}, toShared(PrimitiveType::unit()));
    CHECK(func->isFunction());
    CHECK_FALSE(PrimitiveType::i32()->isFunction());
  }

  SECTION("isGeneric") {
    TypeVariable tv("'a");
    CHECK(tv.isGeneric());

    auto prim = PrimitiveType::i32();
    CHECK_FALSE(prim->isGeneric());
  }

  SECTION("isVariable") {
    TypeVariable tv("a");
    CHECK(tv.isVariable());

    auto prim = PrimitiveType::i32();
    CHECK_FALSE(prim->isVariable());
  }
}

TEST_CASE("Types complex type scenarios", "[types]") {
  SECTION("Array of functions") {
    auto func = FunctionType::make({toShared(PrimitiveType::i32())},
                                   toShared(PrimitiveType::i32()));
    auto arr = ArrayType::make(func);
    CHECK(arr->toString().find("[") != std::string::npos);
  }

  SECTION("Function returning array") {
    std::vector<std::shared_ptr<Type>> params;
    auto ret = ArrayType::make(toShared(PrimitiveType::i32()));
    auto func = FunctionType::make(params, ret);
    CHECK(func->toString().find("[") != std::string::npos);
  }

  SECTION("Nested generic types") {
    std::vector<std::unique_ptr<Type>> innerParams;
    innerParams.push_back(PrimitiveType::i32()->clone());
    GenericType inner("Box", std::move(innerParams));

    // Box<Box<i32>>
    std::vector<std::unique_ptr<Type>> outerParams;
    outerParams.push_back(inner.clone());
    GenericType outer("Box", std::move(outerParams));

    CHECK(outer.typeParams.size() == 1);
  }

  SECTION("Struct with function field") {
    auto callbacks = std::make_shared<StructType>("Callbacks");
    auto funcType = FunctionType::make({toShared(PrimitiveType::i32())},
                                       toShared(PrimitiveType::unit()));
    callbacks->addField("onClick", funcType->clone());

    CHECK(callbacks->getField("onClick") != nullptr);
  }
}
