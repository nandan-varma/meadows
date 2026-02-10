#include "Types.h"
#include <sstream>

namespace meadows {
namespace types {

int TypeVariable::nextId = 0;

// Type helper methods
bool Type::isPrimitive() const {
  return dynamic_cast<const PrimitiveType *>(this) != nullptr;
}

bool Type::isArray() const {
  return dynamic_cast<const ArrayType *>(this) != nullptr;
}

bool Type::isFunction() const {
  return dynamic_cast<const FunctionType *>(this) != nullptr;
}

bool Type::isGeneric() const {
  return dynamic_cast<const GenericType *>(this) != nullptr ||
         dynamic_cast<const TypeVariable *>(this) != nullptr;
}

bool Type::isVariable() const {
  return dynamic_cast<const TypeVariable *>(this) != nullptr;
}

// PrimitiveType implementation
void PrimitiveType::accept(TypeVisitor &visitor) {
  visitor.visitPrimitiveType(*this);
}

std::string PrimitiveType::toString() const {
  switch (kind) {
  case PrimitiveKind::I32:
    return "i32";
  case PrimitiveKind::I64:
    return "i64";
  case PrimitiveKind::F32:
    return "f32";
  case PrimitiveKind::F64:
    return "f64";
  case PrimitiveKind::BOOL:
    return "bool";
  case PrimitiveKind::STRING:
    return "string";
  case PrimitiveKind::UNIT:
    return "()";
  case PrimitiveKind::NEVER:
    return "never";
  default:
    return "unknown";
  }
}

bool PrimitiveType::equals(const Type &other) const {
  auto *p = dynamic_cast<const PrimitiveType *>(&other);
  return p != nullptr && kind == p->kind;
}

std::unique_ptr<Type> PrimitiveType::clone() const {
  return std::make_unique<PrimitiveType>(kind);
}

// Factory methods
std::unique_ptr<PrimitiveType> PrimitiveType::i32() {
  return std::make_unique<PrimitiveType>(PrimitiveKind::I32);
}

std::unique_ptr<PrimitiveType> PrimitiveType::i64() {
  return std::make_unique<PrimitiveType>(PrimitiveKind::I64);
}

std::unique_ptr<PrimitiveType> PrimitiveType::f32() {
  return std::make_unique<PrimitiveType>(PrimitiveKind::F32);
}

std::unique_ptr<PrimitiveType> PrimitiveType::f64() {
  return std::make_unique<PrimitiveType>(PrimitiveKind::F64);
}

std::unique_ptr<PrimitiveType> PrimitiveType::boolType() {
  return std::make_unique<PrimitiveType>(PrimitiveKind::BOOL);
}

std::unique_ptr<PrimitiveType> PrimitiveType::string() {
  return std::make_unique<PrimitiveType>(PrimitiveKind::STRING);
}

std::unique_ptr<PrimitiveType> PrimitiveType::unit() {
  return std::make_unique<PrimitiveType>(PrimitiveKind::UNIT);
}

std::unique_ptr<PrimitiveType> PrimitiveType::never() {
  return std::make_unique<PrimitiveType>(PrimitiveKind::NEVER);
}

// ArrayType implementation
void ArrayType::accept(TypeVisitor &visitor) { visitor.visitArrayType(*this); }

std::string ArrayType::toString() const {
  return "[" + elementType->toString() + "]";
}

bool ArrayType::equals(const Type &other) const {
  auto *a = dynamic_cast<const ArrayType *>(&other);
  return a != nullptr && elementType->equals(*a->elementType);
}

std::unique_ptr<Type> ArrayType::clone() const {
  return std::make_unique<ArrayType>(elementType->clone());
}

// FunctionType implementation
void FunctionType::accept(TypeVisitor &visitor) {
  visitor.visitFunctionType(*this);
}

std::string FunctionType::toString() const {
  std::ostringstream oss;
  oss << "(";
  for (size_t i = 0; i < paramTypes.size(); ++i) {
    if (i > 0)
      oss << ", ";
    oss << paramTypes[i]->toString();
  }
  oss << ") -> " << returnType->toString();
  return oss.str();
}

bool FunctionType::equals(const Type &other) const {
  auto *f = dynamic_cast<const FunctionType *>(&other);
  if (f == nullptr)
    return false;
  if (paramTypes.size() != f->paramTypes.size())
    return false;
  if (!returnType->equals(*f->returnType))
    return false;
  for (size_t i = 0; i < paramTypes.size(); ++i) {
    if (!paramTypes[i]->equals(*f->paramTypes[i]))
      return false;
  }
  return true;
}

std::unique_ptr<Type> FunctionType::clone() const {
  std::vector<std::unique_ptr<Type>> clonedParams;
  for (const auto &param : paramTypes) {
    clonedParams.push_back(param->clone());
  }
  return std::make_unique<FunctionType>(std::move(clonedParams),
                                        returnType->clone());
}

// TypeVariable implementation
void TypeVariable::accept(TypeVisitor &visitor) {
  visitor.visitTypeVariable(*this);
}

std::string TypeVariable::toString() const {
  if (instance) {
    return instance->toString();
  }
  if (name.empty()) {
    return "_" + std::to_string(id);
  }
  return name;
}

bool TypeVariable::equals(const Type &other) const {
  auto *v = dynamic_cast<const TypeVariable *>(&other);
  if (v == nullptr) {
    // If we have an instance, compare with that
    if (instance) {
      return instance->equals(other);
    }
    return false;
  }
  // Same variable if same ID
  return id == v->id;
}

std::unique_ptr<Type> TypeVariable::clone() const {
  auto cloned = std::make_unique<TypeVariable>(name);
  cloned->id = id;
  if (instance) {
    cloned->instance = instance->clone();
  }
  return cloned;
}

// GenericType implementation
void GenericType::accept(TypeVisitor &visitor) {
  visitor.visitGenericType(*this);
}

std::string GenericType::toString() const {
  std::ostringstream oss;
  oss << name << "<";
  for (size_t i = 0; i < typeParams.size(); ++i) {
    if (i > 0)
      oss << ", ";
    oss << typeParams[i]->toString();
  }
  oss << ">";
  return oss.str();
}

bool GenericType::equals(const Type &other) const {
  auto *g = dynamic_cast<const GenericType *>(&other);
  if (g == nullptr || name != g->name ||
      typeParams.size() != g->typeParams.size()) {
    return false;
  }
  for (size_t i = 0; i < typeParams.size(); ++i) {
    if (!typeParams[i]->equals(*g->typeParams[i]))
      return false;
  }
  return true;
}

std::unique_ptr<Type> GenericType::clone() const {
  std::vector<std::unique_ptr<Type>> clonedParams;
  for (const auto &param : typeParams) {
    clonedParams.push_back(param->clone());
  }
  return std::make_unique<GenericType>(name, std::move(clonedParams));
}

// StructType implementation
void StructType::addField(const std::string &fieldName,
                          std::unique_ptr<Type> type) {
  fields[fieldName] = std::move(type);
  fieldOrder.push_back(fieldName);
}

const Type *StructType::getField(const std::string &fieldName) const {
  auto it = fields.find(fieldName);
  if (it != fields.end()) {
    return it->second.get();
  }
  return nullptr;
}

void StructType::accept(TypeVisitor &visitor) {
  visitor.visitStructType(*this);
}

std::string StructType::toString() const {
  std::ostringstream oss;
  oss << name;
  if (!typeParams.empty()) {
    oss << "<";
    for (size_t i = 0; i < typeParams.size(); ++i) {
      if (i > 0)
        oss << ", ";
      oss << typeParams[i];
    }
    oss << ">";
  }
  return oss.str();
}

bool StructType::equals(const Type &other) const {
  auto *s = dynamic_cast<const StructType *>(&other);
  return s != nullptr && name == s->name;
}

std::unique_ptr<Type> StructType::clone() const {
  auto cloned = std::make_unique<StructType>(name);
  cloned->typeParams = typeParams;
  for (const auto &fieldName : fieldOrder) {
    cloned->addField(fieldName, fields.at(fieldName)->clone());
  }
  return cloned;
}

// EnumType implementation
void EnumType::addVariant(const std::string &variantName,
                          std::vector<std::unique_ptr<Type>> types) {
  Variant v;
  v.name = variantName;
  v.types = std::move(types);
  variants.push_back(std::move(v));
}

void EnumType::accept(TypeVisitor &visitor) { visitor.visitEnumType(*this); }

std::string EnumType::toString() const {
  std::ostringstream oss;
  oss << name;
  if (!typeParams.empty()) {
    oss << "<";
    for (size_t i = 0; i < typeParams.size(); ++i) {
      if (i > 0)
        oss << ", ";
      oss << typeParams[i];
    }
    oss << ">";
  }
  return oss.str();
}

bool EnumType::equals(const Type &other) const {
  auto *e = dynamic_cast<const EnumType *>(&other);
  return e != nullptr && name == e->name;
}

std::unique_ptr<Type> EnumType::clone() const {
  auto cloned = std::make_unique<EnumType>(name);
  cloned->typeParams = typeParams;
  for (const auto &variant : variants) {
    std::vector<std::unique_ptr<Type>> clonedTypes;
    for (const auto &t : variant.types) {
      clonedTypes.push_back(t->clone());
    }
    cloned->addVariant(variant.name, std::move(clonedTypes));
  }
  return cloned;
}

// RefType implementation
void RefType::accept(TypeVisitor &visitor) { visitor.visitRefType(*this); }

std::string RefType::toString() const {
  return (mutable_ ? "&mut " : "&") + innerType->toString();
}

bool RefType::equals(const Type &other) const {
  auto *r = dynamic_cast<const RefType *>(&other);
  return r != nullptr && mutable_ == r->mutable_ &&
         innerType->equals(*r->innerType);
}

std::unique_ptr<Type> RefType::clone() const {
  return std::make_unique<RefType>(innerType->clone(), mutable_);
}

// Substitution implementation
std::shared_ptr<Type>
Substitution::apply(const std::shared_ptr<Type> &type) const {
  auto *var = dynamic_cast<TypeVariable *>(type.get());
  if (var) {
    auto it = mapping.find(var->name);
    if (it != mapping.end()) {
      return apply(it->second);
    }
    // Check for instance in type variable
    if (var->instance) {
      auto applied = apply(var->instance);
      var->instance = applied;
      return applied;
    }
  }
  return type;
}

void Substitution::compose(const Substitution &other) {
  // Apply other to all our mappings
  for (auto &pair : mapping) {
    pair.second = other.apply(pair.second);
  }
  // Add other's mappings that we don't have
  for (const auto &pair : other.mapping) {
    if (mapping.find(pair.first) == mapping.end()) {
      mapping[pair.first] = pair.second;
    }
  }
}

bool Substitution::has(const std::string &varName) const {
  return mapping.find(varName) != mapping.end();
}

void Substitution::add(const std::string &varName, std::shared_ptr<Type> type) {
  mapping[varName] = type;
}

} // namespace types
} // namespace meadows
