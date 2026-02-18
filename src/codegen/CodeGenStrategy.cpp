/**
 * @file CodeGenStrategy.cpp
 * @brief Strategy Pattern implementation for CodeGen optimization levels
 */

#include "CodeGenStrategy.h"

namespace meadows {
namespace codegen {

void ReleaseCodeGenStrategy::optimizeModule(llvm::Module *module) {
  if (!module || !module->empty()) {
    return;
  }
}

void OptimizedCodeGenStrategy::optimizeModule(llvm::Module *module) {
  if (!module || !module->empty()) {
    return;
  }
}

std::unique_ptr<CodeGenStrategy> createStrategy(OptimizationLevel level) {
  switch (level) {
  case OptimizationLevel::DEBUG:
    return std::unique_ptr<CodeGenStrategy>(new DebugCodeGenStrategy());
  case OptimizationLevel::RELEASE:
    return std::unique_ptr<CodeGenStrategy>(new ReleaseCodeGenStrategy());
  case OptimizationLevel::OPTIMIZED:
    return std::unique_ptr<CodeGenStrategy>(new OptimizedCodeGenStrategy());
  default:
    return std::unique_ptr<CodeGenStrategy>(new DebugCodeGenStrategy());
  }
}

} // namespace codegen
} // namespace meadows
