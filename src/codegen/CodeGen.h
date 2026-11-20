#ifndef MEADOWS_CODEGEN_H
#define MEADOWS_CODEGEN_H

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>

#include <map>
#include <string>
#include <tuple>
#include <vector>

#include "../ast/AST.h"
#include "../utils/SourceMap.h"

namespace meadows {

class CodeGen : public ExprVisitor, public StmtVisitor {
public:
  explicit CodeGen(const SourceMap& sources);

  std::string generate(StmtList& program);
  const std::string& verificationError() const { return verificationError_; }

private:
  struct FunctionInfo {
    llvm::Function* function;
    llvm::GlobalVariable* descriptor;
  };

  struct Local {
    std::string name;
    llvm::Value* slot;
  };

  struct LoopTarget {
    size_t scopeDepth;
    llvm::BasicBlock* breakBlock;
    llvm::BasicBlock* continueBlock;
  };

  const SourceMap& sources_;
  llvm::LLVMContext context_;
  llvm::Module module_;
  llvm::IRBuilder<> builder_;
  llvm::StructType* valueType_;
  llvm::PointerType* pointerType_;
  llvm::IntegerType* int32Type_;
  std::string verificationError_;

  std::map<std::string, llvm::GlobalVariable*> stringLiterals_;
  std::map<std::string, llvm::GlobalVariable*> cStrings_;
  std::map<std::tuple<uint32_t, uint32_t, uint32_t>, llvm::GlobalVariable*>
      sites_;
  std::map<std::string, FunctionInfo> functions_;

  std::vector<std::vector<Local>> scopes_;
  std::vector<LoopTarget> loops_;
  llvm::BasicBlock* entryBlock_ = nullptr;
  llvm::Value* returnSlot_ = nullptr;
  llvm::Value* result_ = nullptr;

  llvm::Value* int32(int32_t value);
  llvm::Value* slot();
  llvm::Value* arrayOfSlots(size_t count);
  llvm::Value* elementSlot(llvm::Value* array, size_t count, size_t index);
  llvm::GlobalVariable* cString(const std::string& text);
  llvm::GlobalVariable* stringLiteral(const std::string& text);
  llvm::GlobalVariable* shape(const std::vector<ObjectField>& fields);
  llvm::Value* site(const Location& location);
  llvm::CallInst* runtime(const char* name, std::vector<llvm::Value*> args,
                          bool returnsInt = false);
  void move(llvm::Value* target, llvm::Value* source);
  void release(llvm::Value* slot);
  llvm::Value* intSlot(int32_t value);
  llvm::Value* eval(Expr& expr);
  llvm::Value* evalIntoArray(std::vector<ExprPtr>& exprs, size_t count);

  bool terminated() const;
  void pushScope() { scopes_.emplace_back(); }
  void popScope();
  void declare(const std::string& name, llvm::Value* slot);
  llvm::Value* findLocal(const std::string& name) const;
  void releaseDownTo(size_t depth);
  void emitBody(StmtList& body);
  void emitScopedBody(StmtList& body);
  void emitReturn(llvm::Value* valueSlot);

  void declareFunctions(StmtList& program);
  void emitFunction(FuncStmt& stmt);
  void emitMain(StmtList& program);
  llvm::Value* emitCondition(Expr& condition);
  void emitCall(CallExpr& expr);
  void emitBuiltin(const std::string& name, CallExpr& expr);

  void visitLiteralExpr(LiteralExpr& expr) override;
  void visitVarExpr(VarExpr& expr) override;
  void visitAssignExpr(AssignExpr& expr) override;
  void visitBinaryExpr(BinaryExpr& expr) override;
  void visitUnaryExpr(UnaryExpr& expr) override;
  void visitLogicalExpr(LogicalExpr& expr) override;
  void visitIndexExpr(IndexExpr& expr) override;
  void visitFieldExpr(FieldExpr& expr) override;
  void visitCallExpr(CallExpr& expr) override;
  void visitArrayExpr(ArrayExpr& expr) override;
  void visitObjectExpr(ObjectExpr& expr) override;

  void visitExprStmt(ExprStmt& stmt) override;
  void visitLetStmt(LetStmt& stmt) override;
  void visitFuncStmt(FuncStmt& stmt) override;
  void visitIfStmt(IfStmt& stmt) override;
  void visitForStmt(ForStmt& stmt) override;
  void visitWhileStmt(WhileStmt& stmt) override;
  void visitReturnStmt(ReturnStmt& stmt) override;
  void visitBreakStmt(BreakStmt& stmt) override;
  void visitContinueStmt(ContinueStmt& stmt) override;
  void visitBlockStmt(BlockStmt& stmt) override;
};

} // namespace meadows

#endif
