#ifndef MEADOWS_INTERPRETER_H
#define MEADOWS_INTERPRETER_H

#include <functional>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "../ast/AST.h"
#include "../sema/Builtins.h"
#include "../utils/SourceMap.h"
#include "Value.h"

namespace meadows {

struct RuntimeError {
  std::string message;
  Location location;
};

std::string formatRuntimeError(const RuntimeError& error,
                               const SourceMap& sources);

class Interpreter : public ExprVisitor, public StmtVisitor {
public:
  using OutputSink = std::function<void(std::string_view)>;

  explicit Interpreter(OutputSink output) : output_(std::move(output)) {}

  std::optional<RuntimeError> run(StmtList& program);

private:
  enum class Flow { Next, Break, Continue, Return };

  struct Binding {
    const std::string* name;
    Value value;
  };

  OutputSink output_;
  std::map<std::string, FuncStmt*> functions_;
  std::vector<Binding> bindings_;
  size_t frameBase_ = 0;
  int depth_ = 0;
  Flow flow_ = Flow::Next;
  Value result_;
  Value returned_;

  Value eval(Expr& expr);
  void execute(StmtList& body);
  void executeScoped(StmtList& body);
  Value* findVariable(const std::string& name);
  Value call(const Value& callee, std::vector<Value> args, Location site);
  Value callBuiltin(Builtin builtin, std::vector<Value>& args, Location site);
  Value arithmetic(BinaryOp op, const Value& left, const Value& right,
                   Location site);
  bool condition(const Value& value, Location site);
  [[noreturn]] void fail(std::string message, Location site);
  [[noreturn]] void unsupported(const char* op, const std::string& operands,
                                Location site);

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
