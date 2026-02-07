#ifndef TYPEUTILS_H
#define TYPEUTILS_H

#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>

namespace TypeUtils {

inline bool isIntegerType(llvm::Value *val) {
  return val->getType()->isIntegerTy();
}

inline bool isPointerType(llvm::Value *val) {
  return val->getType()->isPointerTy();
}

inline bool isStringType(llvm::Value *val) {
  return val->getType()->isPointerTy();
}

inline bool isArrayType(llvm::Value *val) {
  return val->getType()->isArrayTy();
}

inline bool isBooleanType(llvm::Value *val) {
  return val->getType()->isIntegerTy(1);
}

inline bool isIntegerOrPointer(llvm::Value *val) {
  auto *type = val->getType();
  return type->isIntegerTy() || type->isPointerTy();
}

} // namespace TypeUtils

#endif
