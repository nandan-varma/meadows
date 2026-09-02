#ifndef INTERPRETER_H
#define INTERPRETER_H

#include "../ast/AST.h"
#include "Value.h"

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace meadows {

/**
 * Raised for a documented Meadows runtime error (division by zero,
 * out-of-bounds index, unknown field, ...) or an interpreter-imposed safety
 * limit (step/recursion budget — necessary because this runs synchronously
 * on the browser's main thread with no way to preempt a runaway script).
 */
struct InterpreterError {
  std::string message;
};

/**
 * Tree-walking interpreter for the Meadows AST.
 *
 * This is a second, independent backend from CodeGen (LLVM IR + a clang++
 * subprocess): it exists because the native pipeline's final step — shelling
 * out to a real clang++ — has no equivalent inside a browser's WebAssembly
 * sandbox. Rather than faking execution, this interprets the same AST the
 * semantic analyzer already validated.
 *
 * It implements the full dynamic language described in docs/LANGUAGE.md.
 * Where the current LLVM backend narrows the language for implementation
 * reasons (function parameters fixed to i32, array elements i32-only, object
 * field access unreliable through a variable — see "Current limitations" in
 * docs/LANGUAGE.md), this interpreter does not carry those restrictions:
 * values of any kind flow through functions, arrays, and fields.
 *
 * Operator semantics (including the value-forwarding behavior of `&&`/`||`)
 * are matched against CodeGen.cpp's generated IR, not reimplemented from
 * scratch, so a program's interpreted output matches what running the
 * compiled binary would print.
 */
class Interpreter : public ExprVisitor, public StmtVisitor {
public:
  using OutputSink = std::function<void(const std::string &)>;

  explicit Interpreter(OutputSink output);

  /**
   * Runs top-level statements. Returns 0 on normal completion, -1 if an
   * InterpreterError was raised — mirroring the process exit code the
   * compiled binary returns for the same documented runtime errors
   * (CodeGen::generateRuntimeError ends the function with `ret i32 -1`).
   * The error text is written to the output sink before returning, matching
   * how the compiled binary prints it (via printf) before exiting.
   */
  int run(const std::vector<std::unique_ptr<Stmt>> &statements);

private:
  struct Scope {
    std::unordered_map<std::string, Value> vars;
    Scope *parent = nullptr;
  };

  struct ReturnSignal {
    Value value;
  };
  struct BreakSignal {};
  struct ContinueSignal {};

  // Interpretation runs synchronously on the caller's thread with no
  // preemption available (critically, inside a browser tab). These bound a
  // runaway script (infinite loop / unbounded recursion) to a hard failure
  // instead of a hung or crashed page. kMaxCallDepth is deliberately well
  // under what the interpreter's own (deep, multi-frame-per-call) C++
  // recursion can sustain within the Wasm stack configured for this build
  // (see -sSTACK_SIZE in CMakeLists.txt) — the counter must trip before the
  // real stack does, or a runaway program crashes the module instead of
  // failing gracefully.
  static constexpr size_t kMaxSteps = 200'000;
  static constexpr int kMaxCallDepth = 300;
  // Matches CodeGen.h's MAX_STRING_LENGTH — bounds the cost of repeated
  // concatenation in a loop, which is O(n) per step.
  static constexpr size_t kMaxStringLength = 1024 * 1024;

  OutputSink output_;
  std::vector<std::unique_ptr<Scope>> scopeStack_;
  Scope *globalScope_ = nullptr;
  Scope *currentScope_ = nullptr;
  std::unordered_map<std::string, FuncStmt *> functions_;
  Value result_;
  size_t steps_ = 0;
  int callDepth_ = 0;

  struct ScopeGuard {
    Interpreter &interp;
    ScopeGuard(Interpreter &i, Scope *parent);
    ~ScopeGuard();
  };

  Scope *pushScope(Scope *parent);
  void popScope();

  Value *lookup(const std::string &name);
  void bumpStep();

  Value eval(Expr &expr);
  void execStmts(const std::vector<std::unique_ptr<Stmt>> &stmts);
  Value callFunction(FuncStmt &fn, std::vector<Value> args);
  Value callBuiltin(const std::string &name, std::vector<Value> args);

  [[noreturn]] void runtimeError(const std::string &message);
  void requireInt(const Value &v, const char *context);
  void requireNumeric(const Value &v, const char *context);

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
  void visitBreakStmt(BreakStmt &stmt) override;
  void visitContinueStmt(ContinueStmt &stmt) override;
  void visitBlockStmt(BlockStmt &stmt) override;
};

} // namespace meadows

#endif
