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
#include "CodeGenStrategy.h"
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
  explicit CodeGen(::meadows::codegen::OptimizationLevel level =
                       ::meadows::codegen::OptimizationLevel::DEBUG);

  void
  setStrategy(std::unique_ptr<::meadows::codegen::CodeGenStrategy> strategy);
  void setOptimizationLevel(::meadows::codegen::OptimizationLevel level);

  void generate(const std::vector<std::unique_ptr<Stmt>> &statements);

  std::unique_ptr<llvm::Module> getModule();

private:
  std::unique_ptr<::meadows::codegen::CodeGenStrategy> strategy_;
  std::unique_ptr<llvm::LLVMContext> context;
  std::unique_ptr<llvm::Module> module;
  std::unique_ptr<llvm::IRBuilder<>> builder;
  SymbolTable symbolTable;
  std::unordered_map<std::string, llvm::Type *> variableTypes;
  llvm::Function *printfFunc;
  llvm::Function *mallocFunc;
  llvm::Function *freeFunc;
  llvm::Function *strlenFunc;
  llvm::Function *setArgsFunc;
  llvm::Function *currentFunction;
  llvm::Value *exprResult;
  llvm::BasicBlock *currentBlock;

  std::vector<std::unordered_map<std::string, llvm::Value *>>
      variableScopeStack;

  std::unordered_map<std::string, std::string> externNameMapping;
  std::unordered_map<std::string, llvm::StructType *> enumTypes_;
  std::unordered_map<std::string, llvm::StructType *> structTypes_;
  std::unordered_map<std::string, std::vector<std::string>> definedEnums_;

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
  llvm::Type *getTypeFromAnnotation(const std::string &annotation);

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
  void visitTryExpr(TryExpr &expr) override;
  void visitLogicalExpr(LogicalExpr &expr) override;
  void visitIndexExpr(IndexExpr &expr) override;
  void visitFieldAccessExpr(FieldAccessExpr &expr) override;
  void visitCallExpr(CallExpr &expr) override;
  void visitArrayExpr(ArrayExpr &expr) override;
  void visitObjectExpr(ObjectExpr &expr) override;
  void visitMatchExpr(MatchExpr &expr) override;
  llvm::Value *generateMatchArm(
      Pattern &pattern, llvm::Value *scrutinee, llvm::Value *scrutineeAlloca,
      Expr &body,
      std::unordered_map<std::string, llvm::Value *> &matchBindings);
  void visitEnumVariantExpr(EnumVariantExpr &expr) override;

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
  void visitTypeDefStmt(TypeDefStmt &stmt) override;
  void visitModuleStmt(ModuleStmt &stmt) override;
  void visitImportStmt(ImportStmt &stmt) override;
  void visitExportStmt(ExportStmt &stmt) override;
  void visitExternStmt(ExternStmt &stmt) override;

  llvm::BasicBlock *breakBlock = nullptr;
  llvm::BasicBlock *continueBlock = nullptr;

  void generateElseBranch(const std::vector<std::unique_ptr<Stmt>> &elseBranch,
                          llvm::BasicBlock *endBB);

  static constexpr size_t MAX_STRING_LENGTH = 1024 * 1024;
  static constexpr size_t MAX_ARRAY_ELEMENTS = 1000000;

  std::vector<llvm::Value *> allocatedStrings;
  void freeAllocatedStrings();
};

#endif