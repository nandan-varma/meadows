#include "TypeBuilder.h"

#include <stdexcept>

namespace meadows {
namespace types {

// ========== FunctionTypeBuilder ==========

FunctionTypeBuilder &
FunctionTypeBuilder::withReturnType(std::shared_ptr<Type> type) {
  returnType_ = type;
  return *this;
}

FunctionTypeBuilder &
FunctionTypeBuilder::withParam(std::shared_ptr<Type> type) {
  paramTypes_.push_back(type);
  return *this;
}

FunctionTypeBuilder &FunctionTypeBuilder::withVariadic(bool variadic) {
  variadic_ = variadic;
  return *this;
}

std::shared_ptr<FunctionType> FunctionTypeBuilder::build() {
  if (!returnType_) {
    throw std::runtime_error(
        "FunctionTypeBuilder: return type must be set before building");
  }

  std::vector<std::unique_ptr<Type>> params;
  for (auto &param : paramTypes_) {
    params.push_back(param->clone());
  }

  auto func = std::shared_ptr<FunctionType>(
      new FunctionType(std::move(params), returnType_->clone()));
  // Note: variadic_ flag would need to be stored in FunctionType
  // For now, we just return the function type
  return func;
}

// ========== StructTypeBuilder ==========

StructTypeBuilder::StructTypeBuilder(std::string name)
    : name_(std::move(name)) {
  if (name_.empty()) {
    throw std::runtime_error("StructTypeBuilder: name cannot be empty");
  }
}

StructTypeBuilder &StructTypeBuilder::withField(const std::string &name,
                                                std::shared_ptr<Type> type) {
  if (name.empty()) {
    throw std::runtime_error("StructTypeBuilder: field name cannot be empty");
  }
  fields_.emplace_back(name, type);
  return *this;
}

StructTypeBuilder &StructTypeBuilder::withTypeParam(const std::string &name) {
  if (name.empty()) {
    throw std::runtime_error(
        "StructTypeBuilder: type parameter name cannot be empty");
  }
  typeParams_.push_back(name);
  return *this;
}

std::shared_ptr<StructType> StructTypeBuilder::build() {
  auto structType = std::shared_ptr<StructType>(new StructType(name_));

  for (auto &field : fields_) {
    structType->addField(field.first, field.second->clone());
  }

  structType->typeParams = typeParams_;
  return structType;
}

// ========== EnumTypeBuilder ==========

EnumTypeBuilder::EnumTypeBuilder(std::string name) : name_(std::move(name)) {
  if (name_.empty()) {
    throw std::runtime_error("EnumTypeBuilder: name cannot be empty");
  }
}

EnumTypeBuilder &
EnumTypeBuilder::withVariant(const std::string &name,
                             std::vector<std::shared_ptr<Type>> types) {
  if (name.empty()) {
    throw std::runtime_error("EnumTypeBuilder: variant name cannot be empty");
  }
  variants_.emplace_back(name, std::move(types));
  return *this;
}

EnumTypeBuilder &EnumTypeBuilder::withUnitVariant(const std::string &name) {
  return withVariant(name, {});
}

EnumTypeBuilder &EnumTypeBuilder::withTypeParam(const std::string &name) {
  if (name.empty()) {
    throw std::runtime_error(
        "EnumTypeBuilder: type parameter name cannot be empty");
  }
  typeParams_.push_back(name);
  return *this;
}

std::shared_ptr<EnumType> EnumTypeBuilder::build() {
  auto enumType = std::shared_ptr<EnumType>(new EnumType(name_));

  for (auto &variant : variants_) {
    std::vector<std::unique_ptr<Type>> types;
    for (auto &type : variant.second) {
      types.push_back(type->clone());
    }
    enumType->addVariant(variant.first, std::move(types));
  }

  enumType->typeParams = typeParams_;
  return enumType;
}

// ========== ArrayTypeBuilder ==========

ArrayTypeBuilder::ArrayTypeBuilder(std::shared_ptr<Type> elementType)
    : elementType_(std::move(elementType)) {
  if (!elementType_) {
    throw std::runtime_error("ArrayTypeBuilder: element type cannot be null");
  }
}

ArrayTypeBuilder &ArrayTypeBuilder::withSize(size_t size) {
  size_ = size;
  return *this;
}

std::shared_ptr<ArrayType> ArrayTypeBuilder::build() {
  // Note: size_ is not used in current ArrayType implementation
  // but could be added in the future
  return ArrayType::make(elementType_);
}

// ========== GenericTypeBuilder ==========

GenericTypeBuilder::GenericTypeBuilder(std::string name)
    : name_(std::move(name)) {
  if (name_.empty()) {
    throw std::runtime_error("GenericTypeBuilder: name cannot be empty");
  }
}

GenericTypeBuilder &
GenericTypeBuilder::withTypeParam(std::shared_ptr<Type> type) {
  typeParams_.push_back(type);
  return *this;
}

std::shared_ptr<GenericType> GenericTypeBuilder::build() {
  std::vector<std::unique_ptr<Type>> params;
  for (auto &param : typeParams_) {
    params.push_back(param->clone());
  }
  return std::shared_ptr<GenericType>(
      new GenericType(name_, std::move(params)));
}

// ========== RefTypeBuilder ==========

RefTypeBuilder::RefTypeBuilder(std::shared_ptr<Type> innerType)
    : innerType_(std::move(innerType)) {
  if (!innerType_) {
    throw std::runtime_error("RefTypeBuilder: inner type cannot be null");
  }
}

RefTypeBuilder &RefTypeBuilder::setMutable(bool mut) {
  mutable_ = mut;
  return *this;
}

std::shared_ptr<RefType> RefTypeBuilder::build() {
  return std::shared_ptr<RefType>(new RefType(innerType_->clone(), mutable_));
}

} // namespace types
} // namespace meadows
