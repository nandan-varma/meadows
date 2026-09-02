#include "Value.h"

#include <sstream>
#include <utility>

namespace meadows {

Value Value::ofInt(int32_t v) {
  Value r;
  r.kind_ = Kind::Int;
  r.i_ = v;
  return r;
}

Value Value::ofStr(std::string v) {
  Value r;
  r.kind_ = Kind::Str;
  r.s_ = std::move(v);
  return r;
}

Value Value::ofArray(std::shared_ptr<ValueArray> v) {
  Value r;
  r.kind_ = Kind::Array;
  r.arr_ = std::move(v);
  return r;
}

Value Value::ofObject(std::shared_ptr<ValueObject> v) {
  Value r;
  r.kind_ = Kind::Object;
  r.obj_ = std::move(v);
  return r;
}

const char *Value::typeName() const {
  switch (kind_) {
    case Kind::Int: return "integer";
    case Kind::Str: return "string";
    case Kind::Array: return "array";
    case Kind::Object: return "object";
  }
  return "unknown";
}

std::string Value::displayString() const {
  switch (kind_) {
    case Kind::Int:
      return std::to_string(i_);
    case Kind::Str:
      return s_;
    case Kind::Array: {
      std::ostringstream oss;
      oss << "[";
      for (size_t i = 0; i < arr_->size(); i++) {
        if (i) oss << ", ";
        oss << (*arr_)[i].displayString();
      }
      oss << "]";
      return oss.str();
    }
    case Kind::Object: {
      std::ostringstream oss;
      oss << "{";
      bool first = true;
      for (const auto &[key, val] : *obj_) {
        if (!first) oss << ", ";
        first = false;
        oss << key << ": " << val.displayString();
      }
      oss << "}";
      return oss.str();
    }
  }
  return "";
}

bool Value::equals(const Value &other) const {
  if (kind_ != other.kind_) return false;
  switch (kind_) {
    case Kind::Int:
      return i_ == other.i_;
    case Kind::Str:
      return s_ == other.s_;
    case Kind::Array: {
      if (arr_->size() != other.arr_->size()) return false;
      for (size_t i = 0; i < arr_->size(); i++) {
        if (!(*arr_)[i].equals((*other.arr_)[i])) return false;
      }
      return true;
    }
    case Kind::Object: {
      if (obj_->size() != other.obj_->size()) return false;
      for (const auto &[key, val] : *obj_) {
        auto it = other.obj_->find(key);
        if (it == other.obj_->end() || !val.equals(it->second)) return false;
      }
      return true;
    }
  }
  return false;
}

} // namespace meadows
