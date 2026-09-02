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

inline bool isBooleanType(llvm::Value *val) {
  return val->getType()->isIntegerTy(1);
}

} // namespace TypeUtils

#endif
