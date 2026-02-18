/**
 * @file CodeGenStrategy.h
 * @brief Strategy Pattern for CodeGen optimization levels
 *
 * Provides different code generation strategies that can be swapped at runtime:
 * - DebugStrategy: No optimizations, full debug info
 * - ReleaseStrategy: Basic optimizations
 * - OptimizedStrategy: Aggressive optimizations
 */

#ifndef CODEGEN_STRATEGY_H
#define CODEGEN_STRATEGY_H

#include <llvm/IR/Module.h>
#include <memory>
#include <string>
#include <vector>

namespace meadows {
namespace codegen {

enum class OptimizationLevel { DEBUG, RELEASE, OPTIMIZED };

class CodeGenStrategy {
public:
  virtual ~CodeGenStrategy() = default;

  virtual OptimizationLevel getLevel() const = 0;
  virtual std::string getLevelName() const = 0;

  virtual void optimizeModule(llvm::Module *module) = 0;
  virtual bool shouldPrintIR() const = 0;
  virtual bool shouldAddDebugInfo() const = 0;
  virtual bool shouldValidateIR() const = 0;
};

class DebugCodeGenStrategy : public CodeGenStrategy {
public:
  OptimizationLevel getLevel() const override {
    return OptimizationLevel::DEBUG;
  }
  std::string getLevelName() const override { return "debug"; }

  void optimizeModule(llvm::Module *module) override {}

  bool shouldPrintIR() const override { return true; }
  bool shouldAddDebugInfo() const override { return true; }
  bool shouldValidateIR() const override { return true; }
};

class ReleaseCodeGenStrategy : public CodeGenStrategy {
public:
  OptimizationLevel getLevel() const override {
    return OptimizationLevel::RELEASE;
  }
  std::string getLevelName() const override { return "release"; }

  void optimizeModule(llvm::Module *module) override;

  bool shouldPrintIR() const override { return false; }
  bool shouldAddDebugInfo() const override { return false; }
  bool shouldValidateIR() const override { return false; }
};

class OptimizedCodeGenStrategy : public CodeGenStrategy {
public:
  OptimizationLevel getLevel() const override {
    return OptimizationLevel::OPTIMIZED;
  }
  std::string getLevelName() const override { return "optimized"; }

  void optimizeModule(llvm::Module *module) override;

  bool shouldPrintIR() const override { return false; }
  bool shouldAddDebugInfo() const override { return false; }
  bool shouldValidateIR() const override { return true; }
};

std::unique_ptr<CodeGenStrategy> createStrategy(OptimizationLevel level);

} // namespace codegen
} // namespace meadows

#endif // CODEGEN_STRATEGY_H
