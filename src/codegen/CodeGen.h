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
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
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
  CodeGen(bool optimize = false);

  void setOptimize(bool optimize) { optimize_ = optimize; }

  void generate(const std::vector<std::unique_ptr<Stmt>> &statements);

  std::unique_ptr<llvm::Module> getModule();

private:
  bool optimize_ = false;
  std::unique_ptr<llvm::LLVMContext> context;
  std::unique_ptr<llvm::Module> module;
  std::unique_ptr<llvm::IRBuilder<>> builder;
  SymbolTable symbolTable;
  std::unordered_map<std::string, llvm::Type *> variableTypes;
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
  void validateArrayBounds(llvm::Value *array, llvm::Value *index);
  void validateDivision(llvm::Value *divisor);
  void generateRuntimeError(const std::string &message);

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
  void visitPrintStmt(PrintStmt &stmt) override;
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