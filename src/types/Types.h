/**
 * @file Types.h
 * @brief Type system definitions for the Meadows language.
 *
 * Defines the type hierarchy and type operations including:
 * - Primitive types (i32, i64, f32, f64, bool, string)
 * - Composite types (arrays, functions, tuples)
 * - User-defined types (structs, enums)
 * - Generic types with type parameters
 * - Type variables for inference
 */

#ifndef TYPES_H
#define TYPES_H

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace meadows {
namespace types {

// Forward declarations
class Type;
class TypeVisitor;

/**
 * @brief Base class for all types in the type system.
 */
class Type {
public:
  virtual ~Type() = default;
  virtual void accept(TypeVisitor &visitor) = 0;
  virtual std::string toString() const = 0;
  virtual bool equals(const Type &other) const = 0;
  virtual std::unique_ptr<Type> clone() const = 0;

  // Helper methods
  bool isPrimitive() const;
  bool isArray() const;
  bool isFunction() const;
  bool isGeneric() const;
  bool isVariable() const;
};

/**
 * @brief Primitive types built into the language.
 */
enum class PrimitiveKind {
  I32,    // 32-bit signed integer
  I64,    // 64-bit signed integer
  F32,    // 32-bit floating point
  F64,    // 64-bit floating point
  BOOL,   // Boolean
  STRING, // String (immutable)
  UNIT,   // Unit type ()
  NEVER   // Never type (diverging functions)
};

class PrimitiveType : public Type {
public:
  PrimitiveKind kind;

  explicit PrimitiveType(PrimitiveKind k) : kind(k) {}

  void accept(TypeVisitor &visitor) override;
  std::string toString() const override;
  bool equals(const Type &other) const override;
  std::unique_ptr<Type> clone() const override;

  // Factory methods
  static std::unique_ptr<PrimitiveType> i32();
  static std::unique_ptr<PrimitiveType> i64();
  static std::unique_ptr<PrimitiveType> f32();
  static std::unique_ptr<PrimitiveType> f64();
  static std::unique_ptr<PrimitiveType> boolType();
  static std::unique_ptr<PrimitiveType> string();
  static std::unique_ptr<PrimitiveType> unit();
  static std::unique_ptr<PrimitiveType> never();
};

/**
 * @brief Array type [T].
 */
class ArrayType : public Type {
public:
  std::unique_ptr<Type> elementType;

  explicit ArrayType(std::unique_ptr<Type> elem)
      : elementType(std::move(elem)) {}

  static std::shared_ptr<ArrayType> make(std::shared_ptr<Type> elem) {
    return std::shared_ptr<ArrayType>(new ArrayType(elem->clone()));
  }

  std::shared_ptr<Type> getElementType() const {
    return elementType ? elementType->clone() : nullptr;
  }

  void accept(TypeVisitor &visitor) override;
  std::string toString() const override;
  bool equals(const Type &other) const override;
  std::unique_ptr<Type> clone() const override;
};

/**
 * @brief Function type (T1, T2, ...) -> R.
 */
class FunctionType : public Type {
public:
  std::vector<std::unique_ptr<Type>> paramTypes;
  std::unique_ptr<Type> returnType;

  FunctionType(std::vector<std::unique_ptr<Type>> params,
               std::unique_ptr<Type> ret)
      : paramTypes(std::move(params)), returnType(std::move(ret)) {}

  static std::shared_ptr<FunctionType>
  make(const std::vector<std::shared_ptr<Type>> &params,
       std::shared_ptr<Type> ret) {
    std::vector<std::unique_ptr<Type>> paramOwned;
    for (auto p : params) {
      paramOwned.push_back(p->clone());
    }
    return std::shared_ptr<FunctionType>(
        new FunctionType(std::move(paramOwned), ret->clone()));
  }

  std::shared_ptr<Type> getParamType(size_t i) const {
    if (i < paramTypes.size()) {
      return paramTypes[i]->clone();
    }
    return nullptr;
  }

  std::shared_ptr<Type> getReturnType() const {
    return returnType ? returnType->clone() : nullptr;
  }

  void accept(TypeVisitor &visitor) override;
  std::string toString() const override;
  bool equals(const Type &other) const override;
  std::unique_ptr<Type> clone() const override;
};

/**
 * @brief Type variable for inference (e.g., 'a, 'b).
 */
class TypeVariable : public Type {
public:
  std::string name;
  mutable std::shared_ptr<Type> instance; // For unification
  mutable int id;                         // Unique ID for display

  static int nextId;

  explicit TypeVariable(std::string n) : name(std::move(n)), id(nextId++) {}

  void accept(TypeVisitor &visitor) override;
  std::string toString() const override;
  bool equals(const Type &other) const override;
  std::unique_ptr<Type> clone() const override;

  bool isGeneric() const { return !name.empty() && name[0] == '\''; }
};

/**
 * @brief Generic type with parameters (e.g., List<T>, Option<T>).
 */
class GenericType : public Type {
public:
  std::string name;
  std::vector<std::unique_ptr<Type>> typeParams;

  GenericType(std::string n, std::vector<std::unique_ptr<Type>> params)
      : name(std::move(n)), typeParams(std::move(params)) {}

  std::shared_ptr<Type> getTypeParam(size_t i) const {
    if (i < typeParams.size()) {
      return typeParams[i]->clone();
    }
    return nullptr;
  }

  void accept(TypeVisitor &visitor) override;
  std::string toString() const override;
  bool equals(const Type &other) const override;
  std::unique_ptr<Type> clone() const override;
};

/**
 * @brief User-defined struct type.
 */
class StructType : public Type {
public:
  std::string name;
  std::unordered_map<std::string, std::unique_ptr<Type>> fields;
  std::vector<std::string> fieldOrder; // Preserve declaration order
  std::vector<std::string> typeParams;

  StructType(std::string n) : name(std::move(n)) {}

  void addField(const std::string &fieldName, std::unique_ptr<Type> type);
  const Type *getField(const std::string &fieldName) const;

  void accept(TypeVisitor &visitor) override;
  std::string toString() const override;
  bool equals(const Type &other) const override;
  std::unique_ptr<Type> clone() const override;
};

/**
 * @brief Enum/sum type (algebraic data type).
 */
class EnumType : public Type {
public:
  struct Variant {
    std::string name;
    std::vector<std::unique_ptr<Type>> types;
  };

  std::string name;
  std::vector<Variant> variants;
  std::vector<std::string> typeParams;

  explicit EnumType(std::string n) : name(std::move(n)) {}

  void addVariant(const std::string &variantName,
                  std::vector<std::unique_ptr<Type>> types);

  void accept(TypeVisitor &visitor) override;
  std::string toString() const override;
  bool equals(const Type &other) const override;
  std::unique_ptr<Type> clone() const override;
};

/**
 * @brief Reference type (for mutable references).
 */
class RefType : public Type {
public:
  std::unique_ptr<Type> innerType;
  bool mutable_;

  RefType(std::unique_ptr<Type> inner, bool mut)
      : innerType(std::move(inner)), mutable_(mut) {}

  void accept(TypeVisitor &visitor) override;
  std::string toString() const override;
  bool equals(const Type &other) const override;
  std::unique_ptr<Type> clone() const override;
};

/**
 * @brief Type visitor for operations on types.
 */
class TypeVisitor {
public:
  virtual ~TypeVisitor() = default;
  virtual void visitPrimitiveType(PrimitiveType &type) = 0;
  virtual void visitArrayType(ArrayType &type) = 0;
  virtual void visitFunctionType(FunctionType &type) = 0;
  virtual void visitTypeVariable(TypeVariable &type) = 0;
  virtual void visitGenericType(GenericType &type) = 0;
  virtual void visitStructType(StructType &type) = 0;
  virtual void visitEnumType(EnumType &type) = 0;
  virtual void visitRefType(RefType &type) = 0;
};

/**
 * @brief Type constraint for Hindley-Milner inference.
 */
struct TypeConstraint {
  std::shared_ptr<Type> left;
  std::shared_ptr<Type> right;
  int line;
  int column;
  std::string context;
};

/**
 * @brief Type substitution (mapping from type variables to types).
 */
class Substitution {
public:
  std::unordered_map<std::string, std::shared_ptr<Type>> mapping;

  std::shared_ptr<Type> apply(const std::shared_ptr<Type> &type) const;
  void compose(const Substitution &other);
  bool has(const std::string &varName) const;
  void add(const std::string &varName, std::shared_ptr<Type> type);
};

} // namespace types
} // namespace meadows

#endif // TYPES_H
