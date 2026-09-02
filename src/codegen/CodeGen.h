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
 * - Array/object element count is fixed at compile time (push() allocates a
 *   new, larger array rather than growing one in place — see the "push"
 *   case in visitCallExpr)
 * - Numeric type is i32, or f64 for float literals/arithmetic — no other
 *   numeric width
 */

#include "../ast/AST.h"
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
  // `let b = a;` or `arr = push(arr, x);`) so len() can return it as a
  // constant instead of needing a runtime length that arrays don't
  // otherwise carry. push() relies on the same fact: since it always grows
  // by exactly one element, its result's length is old length + 1, known at
  // compile time too — see visitCallExpr's "push" case.
  std::unordered_map<std::string, size_t> arrayLengths_;
  // Set by the "push" case in visitCallExpr; consumed by visitLetStmt /
  // visitAssignExpr right after evaluating a `push(...)` initializer/value,
  // to keep arrayLengths_ accurate across a rebind.
  size_t lastPushedArrayLength_ = 0;

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

  // If either operand is a double, promotes the other (via CreateSIToFP) so
  // both are — used by visitBinaryExpr's arithmetic/comparison operators to
  // give Int/Float a small numeric tower, matching Value's asDouble()
  // promotion in the interpreter. Returns true if promotion happened (i.e.
  // this is float arithmetic), false if both operands were already the same
  // (non-double) type.
  bool promoteToFloatIfMixed(llvm::Value *&left, llvm::Value *&right);

  // Resolves `arg` to an array's element count and generates its pointer
  // value, when `arg` is an inline array literal or a variable known (via
  // arrayLengths_) to hold one. Returns false — leaving outLen/outPtr
  // untouched and *not* generating `arg` — for anything else (e.g. a
  // string), so callers can fall back to their own handling. Shared by
  // len() and push() so their array-resolution logic can't drift apart.
  bool resolveArrayLength(Expr &arg, size_t &outLen, llvm::Value *&outPtr);

  // True if `e` is a call to the push() builtin — used by visitLetStmt and
  // visitAssignExpr to know when to consume lastPushedArrayLength_.
  static bool isPushCall(Expr *e);

  // Resolves a FieldAccessExpr's struct type and field index, whether its
  // object is an inline literal or a variable declared from one. Shared by
  // the read path (visitFieldAccessExpr) and the write path
  // (visitAssignExpr's FieldAccessExpr-target case) so they can't drift.
  // `expr.object` must already have been visited before calling this.
  // Throws (via the error() helper) if the target's shape can't be resolved
  // — an object literal or a variable declared directly from one are the
  // only supported cases; see visitFieldAccessExpr's doc comment.
  void resolveFieldAccess(FieldAccessExpr &expr, llvm::StructType *&structType,
                          size_t &fieldIndex);
  // `arrayLen` is the array's compile-time-known element count (from
  // resolveArrayLength/arrayLengths_), not something read from the array's
  // memory — arrays don't carry a runtime length header. An earlier version
  // of this function loaded the first i32 at `array`'s address expecting it
  // to be a length prefix that was never actually written there, so it was
  // silently comparing the index against the array's *first element's
  // value* instead of its length.
  void validateArrayBounds(llvm::Value *index, llvm::Value *arrayLen);
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

  std::vector<llvm::Value *> allocatedStrings;
  void freeAllocatedStrings();
};

#endif