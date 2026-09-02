#ifndef CODEGEN_H
#define CODEGEN_H

/**
 * @file CodeGen.h
 * @brief LLVM IR code generation for the Meadows language.
 *
 * This file defines the CodeGen class that traverses the AST and generates
 * equivalent LLVM IR code. It uses the Visitor pattern to process different
 * AST node types and emits corresponding LLVM instructions.
 *
 * @ Features
 * - Generates valid LLVM IR for all supported language constructs
 * - Creates proper function definitions with correct signatures
 * - Handles control flow (if/while/for statements)
 * - Supports function calls and variable access
 * - Includes bounds checking and division-by-zero protection
 *
 * @ Limitations
 * - Arrays and objects are compile-time constants only
 * - No runtime memory allocation for dynamic structures
 * - Limited to i32 integer type for numeric operations
 */

#include "../ast/AST.h"
#include "MemoryUtils.h"
#include "StringUtils.h"
#include "SymbolTable.h"
#include "TypeUtils.h"
#include <llvm/Analysis/CGSCCPassManager.h>
#include <llvm/Analysis/LoopAnalysisManager.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/PassManager.h>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

/**
 * @class CodeGen
 * @brief Generates LLVM IR from Meadows AST.
 *
 * The CodeGen class traverses the AST using the Visitor pattern and emits
 * corresponding LLVM IR instructions. It maintains the LLVM context,
 * module, and IR builder needed for code generation.
 *
 * @ Usage
 * 1. Create CodeGen instance
 * 2. Call generate() with AST statements
 * 3. Call getModule() to retrieve the generated LLVM module
 */
class CodeGen : public ExprVisitor, public StmtVisitor {
public:
  explicit CodeGen(int optLevel = 0);

  void setOptLevel(int level) { optLevel_ = level; }

  void generate(const std::vector<std::unique_ptr<Stmt>> &statements);

  std::unique_ptr<llvm::Module> getModule();

private:
  int optLevel_ = 0;
  std::unique_ptr<llvm::LLVMContext> context;
  std::unique_ptr<llvm::Module> module;
  std::unique_ptr<llvm::IRBuilder<>> builder;
  SymbolTable symbolTable;
  std::unordered_map<std::string, llvm::Type *> variableTypes;

  // Arrays are fixed-size, so a variable's element count is always known at
  // compile time — this tracks it per variable name (set on `let`, copied on
  // `let b = a;`) so len() can return it as a constant instead of needing a
  // runtime length that arrays don't otherwise carry.
  std::unordered_map<std::string, size_t> arrayLengths_;

  // Object literals don't carry a name for their LLVM struct type, and a
  // plain llvm::Value* has no runtime record of which fields it has (opaque
  // pointers carry no pointee type). This records the shape a `let` bound to
  // an object literal (or another variable with a known shape), so field
  // access through a variable — not just an inline literal — can resolve.
  struct ObjectShape {
    llvm::StructType *type = nullptr;
    std::unordered_map<std::string, size_t> fieldIndex;
  };
  std::unordered_map<std::string, ObjectShape> objectShapes_;
  // Shape of the most recently generated inline ObjectExpr — visitLetStmt
  // and visitFieldAccessExpr read this immediately after visiting an
  // ObjectExpr node, before anything else can overwrite it.
  ObjectShape lastObjectShape_;

  llvm::Function *printfFunc;
  llvm::Function *mallocFunc;
  llvm::Function *freeFunc;
  llvm::Function *strlenFunc;
  llvm::Function *currentFunction;
  llvm::Value *exprResult;
  llvm::BasicBlock *currentBlock;

  std::vector<std::unordered_map<std::string, llvm::Value *>>
      variableScopeStack;

  llvm::Value *getStringLength(llvm::Value *str);
  llvm::Value *concatenateStrings(llvm::Value *left, llvm::Value *right);
  llvm::Value *compareStrings(llvm::Value *left, llvm::Value *right, bool equal);
  void validateArrayBounds(llvm::Value *array, llvm::Value *index);
  void validateDivision(llvm::Value *divisor);
  void generateRuntimeError(const std::string &message);
  void emitPrint(llvm::Value *val);
  void runOptimizationPasses();

  // Declares (signature-only) every top-level function before any body is
  // generated, so a call to a function defined later in the file — or two
  // functions calling each other — resolves. Mirrors the pre-pass
  // SemanticAnalyzer::analyze already does for name validation; without this,
  // CodeGen previously only created an llvm::Function when it reached that
  // FuncStmt in source order, so forward/mutual calls failed with "Undefined
  // function" even though semantic analysis had already accepted them.
  void declareFunctionSignatures(
      const std::vector<std::unique_ptr<Stmt>> &statements);

  void enterScope();
  void exitScope();
  void declareVariable(const std::string &name, llvm::Value *value);
  llvm::Value *lookupVariable(const std::string &name);
  bool variableExists(const std::string &name);

  template <typename... Args> [[noreturn]] void error(Args &&...args) {
    std::ostringstream oss;
    (oss << ... << std::forward<Args>(args));
    throw std::runtime_error(oss.str());
  }

  void visitLiteralExpr(LiteralExpr &expr) override;
  void visitVarExpr(VarExpr &expr) override;
  void visitAssignExpr(AssignExpr &expr) override;
  void visitBinaryExpr(BinaryExpr &expr) override;
  void visitUnaryExpr(UnaryExpr &expr) override;
  void visitLogicalExpr(LogicalExpr &expr) override;
  void visitIndexExpr(IndexExpr &expr) override;
  void visitFieldAccessExpr(FieldAccessExpr &expr) override;
  void visitCallExpr(CallExpr &expr) override;
  void visitArrayExpr(ArrayExpr &expr) override;
  void visitObjectExpr(ObjectExpr &expr) override;

  void visitExprStmt(ExprStmt &stmt) override;
  void visitLetStmt(LetStmt &stmt) override;
  void visitFuncStmt(FuncStmt &stmt) override;
  void visitIfStmt(IfStmt &stmt) override;
  void visitForStmt(ForStmt &stmt) override;
  void visitWhileStmt(WhileStmt &stmt) override;
  void visitReturnStmt(ReturnStmt &stmt) override;
  void visitBlockStmt(BlockStmt &stmt) override;
  void visitBreakStmt(BreakStmt &stmt) override;
  void visitContinueStmt(ContinueStmt &stmt) override;

  llvm::BasicBlock *breakBlock = nullptr;
  llvm::BasicBlock *continueBlock = nullptr;

  static constexpr size_t MAX_STRING_LENGTH = 1024 * 1024;
  static constexpr size_t MAX_ARRAY_ELEMENTS = 1000000;

  std::vector<llvm::Value *> allocatedStrings;
  void freeAllocatedStrings();
};

#endif