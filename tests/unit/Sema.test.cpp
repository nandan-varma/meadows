#include <catch2/catch_test_macros.hpp>

#include "TestSupport.h"

using namespace meadows;
using testing::analyze;
using testing::codes;
using Codes = std::vector<ErrorCode>;

TEST_CASE("sema accepts a well-formed program", "[sema]") {
  Analysis a = analyze("func add(a, b) { return a + b; }\nprint(add(1, 2));");
  CHECK(a.ok());
  CHECK(codes(a).empty());
}

TEST_CASE("sema resolves names", "[sema]") {
  CHECK(codes(analyze("print(x);")) ==
        Codes{ErrorCode::SEM_UNDEFINED_VARIABLE});
  CHECK(codes(analyze("x = 1;")) == Codes{ErrorCode::SEM_UNDEFINED_VARIABLE});
  CHECK(codes(analyze("nothing(1);")) ==
        Codes{ErrorCode::SEM_UNDEFINED_FUNCTION});
  CHECK(codes(analyze("{ let a = 1; print(a); } print(a);")) ==
        Codes{ErrorCode::SEM_UNDEFINED_VARIABLE});
}

TEST_CASE("sema lets functions be called before they are defined",
          "[sema]") {
  CHECK(codes(analyze("print(f());\nfunc f() { return 1; }")).empty());
  CHECK(codes(analyze("func a() { return b(); }\nfunc b() { return 1; }\n"
                      "print(a());"))
            .empty());
}

TEST_CASE("sema keeps functions closed over their own scope", "[sema]") {
  Analysis a = analyze("let g = 1;\nfunc f() { return g; }\nprint(f());");
  CHECK(codes(a) == Codes{ErrorCode::SEM_UNDEFINED_VARIABLE,
                          ErrorCode::WARN_UNUSED_VARIABLE});
  CHECK(a.diagnostics.diagnostics()[0].help.find("parameter") !=
        std::string::npos);
}

TEST_CASE("sema rejects redefinitions", "[sema]") {
  CHECK(codes(analyze("let a = 1; let a = 2; print(a);")) ==
        Codes{ErrorCode::SEM_REDEFINED_VARIABLE});
  CHECK(codes(analyze("func f() { } func f() { }")) ==
        Codes{ErrorCode::SEM_REDEFINED_FUNCTION});
  CHECK(codes(analyze("func f(a, a) { return a; }")) ==
        Codes{ErrorCode::SEM_REDEFINED_VARIABLE});
}

TEST_CASE("sema gives every name one meaning", "[sema]") {
  CHECK(codes(analyze("func f() { } let f = 1;")) ==
        Codes{ErrorCode::SEM_NAME_CONFLICT});
  CHECK(codes(analyze("let print = 1;"))[0] == ErrorCode::SEM_NAME_CONFLICT);
  CHECK(codes(analyze("func len(x) { return 1; }"))[0] ==
        ErrorCode::SEM_NAME_CONFLICT);
  CHECK(codes(analyze("func f() { } f = 1;")) ==
        Codes{ErrorCode::SEM_ASSIGN_TO_NON_VARIABLE});
  CHECK(codes(analyze("let p = print;"))[0] == ErrorCode::SEM_BUILTIN_AS_VALUE);
}

TEST_CASE("sema checks argument counts for direct calls", "[sema]") {
  CHECK(codes(analyze("func f(a) { return a; } f(1, 2);")) ==
        Codes{ErrorCode::SEM_INVALID_ARGUMENT_COUNT});
  CHECK(codes(analyze("print();")) ==
        Codes{ErrorCode::SEM_INVALID_ARGUMENT_COUNT});
  CHECK(codes(analyze("print(push([1]));")) ==
        Codes{ErrorCode::SEM_INVALID_ARGUMENT_COUNT});
  CHECK(codes(analyze("func f(a) { return a; } let g = f; print(g(1, 2));"))
            .empty());
}

TEST_CASE("sema checks control-flow placement", "[sema]") {
  CHECK(codes(analyze("break;")) == Codes{ErrorCode::SEM_BREAK_OUTSIDE_LOOP});
  CHECK(codes(analyze("continue;")) ==
        Codes{ErrorCode::SEM_CONTINUE_OUTSIDE_LOOP});
  CHECK(codes(analyze("return 1;")) ==
        Codes{ErrorCode::SEM_RETURN_OUTSIDE_FUNCTION});
  CHECK(codes(analyze("while (1) { break; }")).empty());
  CHECK(codes(analyze("func f() { break; }")) ==
        Codes{ErrorCode::SEM_BREAK_OUTSIDE_LOOP});
}

TEST_CASE("sema checks object literals", "[sema]") {
  CHECK(codes(analyze("print({a: 1}.b);")) ==
        Codes{ErrorCode::SEM_UNKNOWN_FIELD});
  CHECK(codes(analyze("print({a: 1}.a);")).empty());
  CHECK(codes(analyze("let o = {a: 1, a: 2}; print(o.a);")) ==
        Codes{ErrorCode::SEM_DUPLICATE_FIELD});
}

TEST_CASE("sema warns about unused variables", "[sema]") {
  CHECK(codes(analyze("let a = 1;")) ==
        Codes{ErrorCode::WARN_UNUSED_VARIABLE});
  CHECK(codes(analyze("let _a = 1;")).empty());
  CHECK(codes(analyze("func f(a) { return 1; }")).empty());
  CHECK(analyze("let a = 1;").ok());
}

TEST_CASE("sema warns about shadowing and unreachable code", "[sema]") {
  CHECK(codes(analyze("let a = 1; { let a = 2; print(a); } print(a);")) ==
        Codes{ErrorCode::WARN_SHADOWING_VARIABLE});
  CHECK(codes(analyze("func f() { return 1; print(2); }")) ==
        Codes{ErrorCode::WARN_UNREACHABLE_CODE});
  CHECK(codes(analyze("func f(c) { if (c) { return 1; } else { return 2; } "
                      "print(3); }")) ==
        Codes{ErrorCode::WARN_UNREACHABLE_CODE});
  CHECK(codes(analyze("func f(c) { if (c) { return 1; } print(3); }"))
            .empty());
}

TEST_CASE("warning policy can silence warnings or promote them", "[sema]") {
  WarningPolicy quiet;
  quiet.disabled.insert(ErrorCode::WARN_UNUSED_VARIABLE);
  CHECK(codes(analyze("let a = 1;", quiet)).empty());

  WarningPolicy strict;
  strict.treatAsErrors = true;
  Analysis a = analyze("let a = 1;", strict);
  CHECK_FALSE(a.ok());
}
