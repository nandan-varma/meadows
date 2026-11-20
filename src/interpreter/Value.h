#ifndef MEADOWS_VALUE_H
#define MEADOWS_VALUE_H

#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace meadows {

class FuncStmt;
class Value;

struct ArrayData {
  std::vector<Value> items;
  ArrayData() = default;
  explicit ArrayData(std::vector<Value> v) : items(std::move(v)) {}
  ArrayData(const ArrayData&) = delete;
  ArrayData& operator=(const ArrayData&) = delete;
  ~ArrayData();
};

struct ObjectData {
  std::vector<std::pair<std::string, Value>> fields;
  explicit ObjectData(std::vector<std::pair<std::string, Value>> f);
  ObjectData(const ObjectData&) = delete;
  ObjectData& operator=(const ObjectData&) = delete;
  ~ObjectData();
};

using StringPtr = std::shared_ptr<const std::string>;
using ArrayPtr = std::shared_ptr<ArrayData>;
using ObjectPtr = std::shared_ptr<ObjectData>;

class Value {
public:
  enum class Type { Int, Float, String, Array, Object, Function };

  Value() : data_(int32_t{0}) {}

  static Value integer(int32_t value) { return Value(value); }
  static Value real(double value) { return Value(value); }
  static Value string(std::string value);
  static Value array(std::vector<Value> items);
  static Value object(std::vector<std::pair<std::string, Value>> fields);
  static Value function(FuncStmt* declaration) {
    return Value(declaration);
  }

  Type type() const { return static_cast<Type>(data_.index()); }
  const char* typeName() const;
  std::string describe() const;
  bool isInt() const { return type() == Type::Int; }
  bool isFloat() const { return type() == Type::Float; }
  bool isNumber() const { return isInt() || isFloat(); }
  bool isString() const { return type() == Type::String; }
  bool isArray() const { return type() == Type::Array; }
  bool isObject() const { return type() == Type::Object; }
  bool isFunction() const { return type() == Type::Function; }

  int32_t asInt() const { return std::get<int32_t>(data_); }
  double asFloat() const { return std::get<double>(data_); }
  double asDouble() const { return isInt() ? asInt() : asFloat(); }
  const std::string& asString() const { return *std::get<StringPtr>(data_); }
  ArrayData& asArray() const { return *std::get<ArrayPtr>(data_); }
  ObjectData& asObject() const { return *std::get<ObjectPtr>(data_); }
  FuncStmt* asFunction() const {
    return std::get<FuncStmt*>(data_);
  }

  std::string display() const;
  static bool equals(const Value& left, const Value& right);

private:
  std::variant<int32_t, double, StringPtr, ArrayPtr, ObjectPtr,
               FuncStmt*>
      data_;

  explicit Value(int32_t v) : data_(v) {}
  explicit Value(double v) : data_(v) {}
  explicit Value(FuncStmt* v) : data_(v) {}
  template <typename T> Value(std::in_place_type_t<T> tag, T v)
      : data_(tag, std::move(v)) {}
};

std::string formatFloat(double value);

} // namespace meadows

#endif
