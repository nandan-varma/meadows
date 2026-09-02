#include "Value.h"

#include <cstdio>
#include <sstream>
#include <utility>

namespace meadows {

namespace {
// snprintf with "%g", not std::to_string (which always emits 6 fixed
// decimals for a double) — matches CodeGen's printf("%g\n", ...) exactly,
// since both go through the same libc *printf family.
std::string formatFloat(double v) {
  char buf[64];
  std::snprintf(buf, sizeof(buf), "%g", v);
  return std::string(buf);
}
} // namespace

Value Value::ofInt(int32_t v) {
  Value r;
  r.kind_ = Kind::Int;
  r.i_ = v;
  return r;
}

Value Value::ofFloat(double v) {
  Value r;
  r.kind_ = Kind::Float;
  r.f_ = v;
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

Value Value::ofFunction(::FuncStmt *fn) {
  Value r;
  r.kind_ = Kind::Function;
  r.fn_ = fn;
  return r;
}

const char *Value::typeName() const {
  switch (kind_) {
    case Kind::Int: return "integer";
    case Kind::Float: return "float";
    case Kind::Str: return "string";
    case Kind::Array: return "array";
    case Kind::Object: return "object";
    case Kind::Function: return "function";
  }
  return "unknown";
}

std::string Value::displayString() const {
  switch (kind_) {
    case Kind::Int:
      return std::to_string(i_);
    case Kind::Float:
      return formatFloat(f_);
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
    case Kind::Function:
      return "<function>";
  }
  return "";
}

bool Value::equals(const Value &other) const {
  // Int/Float compare across kinds by promoting Int to double — matches the
  // promotion arithmetic and ordering comparisons use (see
  // CodeGen::visitBinaryExpr), so 1 == 1.0 is true, consistent with `1 +
  // 1.0` being a Float. No other kind pair is comparable.
  if (isNumeric() && other.isNumeric()) return asDouble() == other.asDouble();
  if (kind_ != other.kind_) return false;
  switch (kind_) {
    case Kind::Int:
    case Kind::Float:
      return false; // unreachable: both numeric kinds are handled above
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
    case Kind::Function:
      return fn_ == other.fn_;
  }
  return false;
}

} // namespace meadows
