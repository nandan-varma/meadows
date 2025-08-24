#pragma once

#include "../common/ErrorReporter.h"
#include "ASTVisitor.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Value.h"
#include "llvm/IR/Verifier.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Target/TargetMachine.h"

#include <memory>
#include <string>
#include <unordered_map>

namespace meadows {

class CodeGenerator : public ASTVisitor {
private:
  std::unique_ptr<llvm::LLVMContext> context;
  std::unique_ptr<llvm::Module> module;
  std::unique_ptr<llvm::IRBuilder<>> builder;
  ErrorReporter &errorReporter;

  // Symbol tables
  std::unordered_map<std::string, llvm::Value *> namedValues;
  std::unordered_map<std::string, llvm::Function *> functions;

  // Current function being generated
  llvm::Function *currentFunction;

  // Result of the last expression evaluation
  llvm::Value *lastValue;

  // Helper methods
  llvm::Type *getInt32Type();
  llvm::Type *getDoubleType();
  llvm::Type *getInt8PtrType();
  llvm::Type *getBoolType();

  llvm::Value *createPrintfCall(const std::string &format,
                                const std::vector<llvm::Value *> &args = {});
  llvm::Function *getOrCreatePrintfFunction();
  llvm::Function *createMainWrapper();

  // Type conversion helpers
  llvm::Value *convertToDouble(llvm::Value *value);
  llvm::Value *convertToInt(llvm::Value *value);

public:
  CodeGenerator(ErrorReporter &errorReporter,
                const std::string &moduleName = "meadows_program");
  ~CodeGenerator() = default;

  // Generate LLVM IR
  std::unique_ptr<llvm::Module> generateIR(Program &program);

  // Generate object file
  bool generateObjectFile(const std::string &filename);

  // Generate executable
  bool generateExecutable(const std::string &objectFile,
                          const std::string &executableFile);

  // Print LLVM IR
  void printIR();

  // Expression visitors
  void visit(IntegerLiteral &node) override;
  void visit(FloatLiteral &node) override;
  void visit(StringLiteral &node) override;
  void visit(BooleanLiteral &node) override;
  void visit(NoneLiteral &node) override;
  void visit(ListLiteral &node) override;
  void visit(Identifier &node) override;
  void visit(BinaryExpression &node) override;
  void visit(UnaryExpression &node) override;
  void visit(FunctionCall &node) override;
  void visit(AttributeAccess &node) override;
  void visit(IndexAccess &node) override;
  void visit(Assignment &node) override;

  // Statement visitors
  void visit(ExpressionStatement &node) override;
  void visit(Block &node) override;
  void visit(IfStatement &node) override;
  void visit(WhileStatement &node) override;
  void visit(ForStatement &node) override;
  void visit(ReturnStatement &node) override;
  void visit(BreakStatement &node) override;
  void visit(ContinueStatement &node) override;
  void visit(PassStatement &node) override;
  void visit(FunctionDefinition &node) override;
  void visit(ClassDefinition &node) override;
  void visit(ImportStatement &node) override;
  void visit(Program &node) override;
};

} // namespace meadows
