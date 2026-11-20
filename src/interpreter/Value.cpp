#include "Value.h"

#include <cstdio>

namespace meadows {

namespace {

std::vector<Value>& pendingRelease() {
  static thread_local std::vector<Value> pending;
  return pending;
}

bool& draining() {
  static thread_local bool active = false;
  return active;
}

void releaseChildren(std::vector<Value>& children) {
  std::vector<Value>& pending = pendingRelease();
  for (Value& child : children) {
    if (child.isArray() || child.isObject()) {
      pending.push_back(std::move(child));
    }
  }
  if (draining()) {
    return;
  }
  draining() = true;
  while (!pending.empty()) {
    std::vector<Value> batch;
    batch.swap(pending);
  }
  draining() = false;
}

} // namespace

ArrayData::~ArrayData() {
  releaseChildren(items);
}

ObjectData::ObjectData(std::vector<std::pair<std::string, Value>> f)
    : fields(std::move(f)) {}

ObjectData::~ObjectData() {
  std::vector<Value> children;
  children.reserve(fields.size());
  for (auto& field : fields) {
    children.push_back(std::move(field.second));
  }
  releaseChildren(children);
}

Value Value::string(std::string value) {
  return Value(std::in_place_type<StringPtr>,
               std::make_shared<const std::string>(std::move(value)));
}

Value Value::array(std::vector<Value> items) {
  return Value(std::in_place_type<ArrayPtr>,
               std::make_shared<ArrayData>(std::move(items)));
}

Value Value::object(std::vector<std::pair<std::string, Value>> fields) {
  return Value(std::in_place_type<ObjectPtr>,
               std::make_shared<ObjectData>(std::move(fields)));
}

const char* Value::typeName() const {
  switch (type()) {
  case Type::Int:
    return "integer";
  case Type::Float:
    return "float";
  case Type::String:
    return "string";
  case Type::Array:
    return "array";
  case Type::Object:
    return "object";
  case Type::Function:
    return "function";
  }
  return "";
}

std::string Value::describe() const {
  std::string name = typeName();
  return (isInt() || isArray() || isObject() ? "an " : "a ") + name;
}

std::string formatFloat(double value) {
  char buffer[32];
  std::snprintf(buffer, sizeof(buffer), "%g", value);
  return buffer;
}

std::string Value::display() const {
  switch (type()) {
  case Type::Int:
    return std::to_string(asInt());
  case Type::Float:
    return formatFloat(asFloat());
  case Type::String:
    return asString();
  default:
    return "";
  }
}

bool Value::equals(const Value& left, const Value& right) {
  if (left.isNumber() && right.isNumber()) {
    return left.asDouble() == right.asDouble();
  }
  if (left.type() != right.type()) {
    return false;
  }
  switch (left.type()) {
  case Type::String:
    return left.asString() == right.asString();
  case Type::Array:
    return &left.asArray() == &right.asArray();
  case Type::Object:
    return &left.asObject() == &right.asObject();
  case Type::Function:
    return left.asFunction() == right.asFunction();
  default:
    return false;
  }
}

} // namespace meadows
