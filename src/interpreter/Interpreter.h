#ifndef MEADOWS_INTERPRETER_H
#define MEADOWS_INTERPRETER_H

#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "../ast/AST.h"
#include "../sema/Builtins.h"
#include "../utils/SourceMap.h"
#include "Bytecode.h"
#include "Value.h"

namespace meadows {

struct RuntimeError {
  std::string message;
  Location location;
};

std::string formatRuntimeError(const RuntimeError& error,
                               const SourceMap& sources);

class Interpreter {
public:
  using OutputSink = std::function<void(std::string_view)>;

  explicit Interpreter(OutputSink output) : output_(std::move(output)) {}

  std::optional<RuntimeError> run(StmtList& program);

private:
  struct Frame {
    const FunctionCode* code;
    size_t pc;
    size_t base;
  };

  OutputSink output_;
  const BytecodeProgram* program_ = nullptr;
  std::vector<Value> stack_;
  std::vector<Frame> frames_;

  void execute();
  Value pop();
  void push(Value value) { stack_.push_back(std::move(value)); }
  Location where(const Instruction& instruction) const;
  [[noreturn]] void fail(std::string message, const Instruction& at);
  [[noreturn]] void unsupported(const char* op, const std::string& operands,
                                const Instruction& at);

  void binary(const Instruction& at);
  void compare(const Instruction& at);
  Value arithmetic(BinaryOp op, const Value& left, const Value& right,
                   const Instruction& at);
  void index(const Instruction& at);
  void setIndex(const Instruction& at);
  void field(const Instruction& at);
  void setField(const Instruction& at);
  void call(const Instruction& at);
  void callBuiltin(const Instruction& at);
  void forInit(const Instruction& at);
};

} // namespace meadows

#endif
