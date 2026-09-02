#ifndef INTERPRETER_VALUE_H
#define INTERPRETER_VALUE_H

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace meadows {

class Value;
using ValueArray = std::vector<Value>;
using ValueObject = std::map<std::string, Value>;

/**
 * A dynamically-typed runtime value for the tree-walking Interpreter.
 *
 * Meadows values come in four kinds (docs/LANGUAGE.md): Integer, String,
 * Array, Object. There is no separate boolean type — comparisons and
 * logical operators produce 0/1 integers, matching the native LLVM
 * backend's representation (see CodeGen::visitBinaryExpr / visitLogicalExpr).
 *
 * Arrays and objects have reference semantics (shared_ptr-backed): `let b =
 * a;` aliases the same underlying storage, matching the native backend
 * where a `let` of an array/object variable copies a pointer, not the
 * pointee (CodeGen::visitVarExpr loads the alloca'd pointer value).
 */
class Value {
public:
  enum class Kind { Int, Str, Array, Object };

  Value() : kind_(Kind::Int), i_(0) {}

  static Value ofInt(int32_t v);
  static Value ofStr(std::string v);
  static Value ofArray(std::shared_ptr<ValueArray> v);
  static Value ofObject(std::shared_ptr<ValueObject> v);

  Kind kind() const { return kind_; }
  bool isInt() const { return kind_ == Kind::Int; }
  bool isStr() const { return kind_ == Kind::Str; }
  bool isArray() const { return kind_ == Kind::Array; }
  bool isObject() const { return kind_ == Kind::Object; }

  int32_t asInt() const { return i_; }
  const std::string &asStr() const { return s_; }
  const ValueArray &asArray() const { return *arr_; }
  ValueArray &asArray() { return *arr_; }
  const ValueObject &asObject() const { return *obj_; }
  ValueObject &asObject() { return *obj_; }

  const char *typeName() const;

  /** Human-readable form used by print() and by array/object rendering. */
  std::string displayString() const;

  bool equals(const Value &other) const;

private:
  Kind kind_;
  int32_t i_ = 0;
  std::string s_;
  std::shared_ptr<ValueArray> arr_;
  std::shared_ptr<ValueObject> obj_;
};

} // namespace meadows

#endif
