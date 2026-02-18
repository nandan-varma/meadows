/**
 * @file TypeBuilder.h
 * @brief Builder Pattern for constructing complex types
 *
 * Provides a fluent interface for building function types, struct types,
 * and other complex types with many optional parameters.
 */

#ifndef TYPE_BUILDER_H
#define TYPE_BUILDER_H

#include <memory>
#include <string>
#include <vector>

#include "Types.h"

namespace meadows {
namespace types {

/**
 * @brief Builder for constructing function types
 *
 * Usage:
 *   auto funcType = FunctionTypeBuilder()
 *       .withReturnType(Type::i32())
 *       .withParam(Type::i32())
 *       .withParam(Type::string())
 *       .withVariadic()
 *       .build();
 */
class FunctionTypeBuilder {
public:
  FunctionTypeBuilder() = default;

  /**
   * @brief Set the return type
   */
  FunctionTypeBuilder &withReturnType(std::shared_ptr<Type> type);

  /**
   * @brief Add a parameter type
   */
  FunctionTypeBuilder &withParam(std::shared_ptr<Type> type);

  /**
   * @brief Set variadic flag
   */
  FunctionTypeBuilder &withVariadic(bool variadic = true);

  /**
   * @brief Build the function type
   * @throws std::runtime_error if return type not set
   */
  std::shared_ptr<FunctionType> build();

private:
  std::vector<std::shared_ptr<Type>> paramTypes_;
  std::shared_ptr<Type> returnType_;
  bool variadic_ = false;
};

/**
 * @brief Builder for constructing struct types
 *
 * Usage:
 *   auto structType = StructTypeBuilder("Point")
 *       .withField("x", Type::i32())
 *       .withField("y", Type::i32())
 *       .withTypeParam("T")
 *       .build();
 */
class StructTypeBuilder {
public:
  explicit StructTypeBuilder(std::string name);

  /**
   * @brief Add a field to the struct
   */
  StructTypeBuilder &withField(const std::string &name,
                               std::shared_ptr<Type> type);

  /**
   * @brief Add a generic type parameter
   */
  StructTypeBuilder &withTypeParam(const std::string &name);

  /**
   * @brief Build the struct type
   */
  std::shared_ptr<StructType> build();

private:
  std::string name_;
  std::vector<std::pair<std::string, std::shared_ptr<Type>>> fields_;
  std::vector<std::string> typeParams_;
};

/**
 * @brief Builder for constructing enum types
 *
 * Usage:
 *   auto enumType = EnumTypeBuilder("Option")
 *       .withVariant("Some", {Type::i32()})
 *       .withVariant("None", {})
 *       .withTypeParam("T")
 *       .build();
 */
class EnumTypeBuilder {
public:
  explicit EnumTypeBuilder(std::string name);

  /**
   * @brief Add a variant to the enum
   * @param name Variant name
   * @param types Associated types (empty for unit variants)
   */
  EnumTypeBuilder &withVariant(const std::string &name,
                               std::vector<std::shared_ptr<Type>> types);

  /**
   * @brief Add a simple unit variant
   */
  EnumTypeBuilder &withUnitVariant(const std::string &name);

  /**
   * @brief Add a generic type parameter
   */
  EnumTypeBuilder &withTypeParam(const std::string &name);

  /**
   * @brief Build the enum type
   */
  std::shared_ptr<EnumType> build();

private:
  std::string name_;
  std::vector<std::pair<std::string, std::vector<std::shared_ptr<Type>>>>
      variants_;
  std::vector<std::string> typeParams_;
};

/**
 * @brief Builder for constructing array types with optional size
 */
class ArrayTypeBuilder {
public:
  explicit ArrayTypeBuilder(std::shared_ptr<Type> elementType);

  /**
   * @brief Set fixed array size (0 for dynamic array)
   */
  ArrayTypeBuilder &withSize(size_t size);

  /**
   * @brief Build the array type
   */
  std::shared_ptr<ArrayType> build();

private:
  std::shared_ptr<Type> elementType_;
  size_t size_ = 0;
};

/**
 * @brief Builder for constructing generic types
 *
 * Usage:
 *   auto listType = GenericTypeBuilder("List")
 *       .withTypeParam(Type::i32())
 *       .build();
 */
class GenericTypeBuilder {
public:
  explicit GenericTypeBuilder(std::string name);

  /**
   * @brief Add a type parameter
   */
  GenericTypeBuilder &withTypeParam(std::shared_ptr<Type> type);

  /**
   * @brief Build the generic type
   */
  std::shared_ptr<GenericType> build();

private:
  std::string name_;
  std::vector<std::shared_ptr<Type>> typeParams_;
};

/**
 * @brief Builder for constructing reference types
 */
class RefTypeBuilder {
public:
  explicit RefTypeBuilder(std::shared_ptr<Type> innerType);

  /**
   * @brief Set mutability
   */
  RefTypeBuilder &setMutable(bool mut = true);

  /**
   * @brief Build the reference type
   */
  std::shared_ptr<RefType> build();

private:
  std::shared_ptr<Type> innerType_;
  bool mutable_ = false;
};

} // namespace types
} // namespace meadows

#endif // TYPE_BUILDER_H
