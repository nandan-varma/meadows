#ifndef INTERPRETER_VALUE_H
#define INTERPRETER_VALUE_H

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>

class FuncStmt;

namespace meadows {

class Value;
using ValueArray = std::vector<Value>;
using ValueObject = std::map<std::string, Value>;

/**
 * A dynamically-typed runtime value for the tree-walking Interpreter.
 *
 * Meadows values come in six kinds (docs/LANGUAGE.md): Integer, Float,
 * String, Array, Object, Function. There is no separate boolean type —
 * comparisons and logical operators produce 0/1 integers, matching the
 * native LLVM backend's representation (see CodeGen::visitBinaryExpr /
 * visitLogicalExpr).
 *
 * Function is interpreter-only — a first-class reference to a top-level
 * FuncStmt, letting a function be aliased through a variable and called
 * indirectly (`let f = add; f(1, 2);`). The native backend doesn't have
 * this: every function parameter is a fixed i32, so there's no type a
 * function pointer could occupy without a much larger type-system change —
 * see docs/LANGUAGE.md's "Current limitations".
 *
 * Int and Float participate in a small numeric tower: arithmetic and
 * ordering between them promote the Int operand to Float (matching
 * CodeGen's use of CreateSIToFP — see CodeGen::visitBinaryExpr), and
 * equals() considers 1 == 1.0 true for the same reason.
 *
 * Arrays and objects have reference semantics (shared_ptr-backed): `let b =
 * a;` aliases the same underlying storage, matching the native backend
 * where a `let` of an array/object variable copies a pointer, not the
 * pointee (CodeGen::visitVarExpr loads the alloca'd pointer value).
 */
class Value {
public:
  enum class Kind { Int, Float, Str, Array, Object, Function };

  Value() : kind_(Kind::Int), i_(0) {}

  static Value ofInt(int32_t v);
  static Value ofFloat(double v);
  static Value ofStr(std::string v);
  static Value ofArray(std::shared_ptr<ValueArray> v);
  static Value ofObject(std::shared_ptr<ValueObject> v);
  static Value ofFunction(::FuncStmt *fn);

  Kind kind() const { return kind_; }
  bool isInt() const { return kind_ == Kind::Int; }
  bool isFloat() const { return kind_ == Kind::Float; }
  bool isNumeric() const { return isInt() || isFloat(); }
  bool isStr() const { return kind_ == Kind::Str; }
  bool isArray() const { return kind_ == Kind::Array; }
  bool isObject() const { return kind_ == Kind::Object; }
  bool isFunction() const { return kind_ == Kind::Function; }

  int32_t asInt() const { return i_; }
  double asFloat() const { return f_; }
  /** Numeric value as a double, promoting Int if needed. Only valid when
   * isNumeric() is true. */
  double asDouble() const { return isFloat() ? f_ : static_cast<double>(i_); }
  const std::string &asStr() const { return s_; }
  const ValueArray &asArray() const { return *arr_; }
  ValueArray &asArray() { return *arr_; }
  const ValueObject &asObject() const { return *obj_; }
  ValueObject &asObject() { return *obj_; }
  ::FuncStmt *asFunction() const { return fn_; }

  const char *typeName() const;

  /** Human-readable form used by print() and by array/object rendering.
   * Floats are formatted with "%g" — chosen so a compiled program's printf
   * output and the interpreter's output are byte-identical, since both
   * ultimately go through the same libc *printf family. */
  std::string displayString() const;

  bool equals(const Value &other) const;

private:
  Kind kind_;
  int32_t i_ = 0;
  double f_ = 0.0;
  ::FuncStmt *fn_ = nullptr;
  std::string s_;
  std::shared_ptr<ValueArray> arr_;
  std::shared_ptr<ValueObject> obj_;
};

} // namespace meadows

#endif
