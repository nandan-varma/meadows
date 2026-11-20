#include "Interpreter.h"

#include <cmath>

#include "../runtime/Limits.h"

namespace meadows {

namespace {

struct Abort {
  RuntimeError error;
};

std::string typeNames(const Value& left, const Value& right) {
  return std::string(left.typeName()) + " and " + right.typeName();
}

std::string arguments(size_t count) {
  return std::to_string(count) + (count == 1 ? " argument" : " arguments");
}

BinaryOp binaryOpFor(Op op) {
  switch (op) {
  case Op::Add: return BinaryOp::Add;
  case Op::Sub: return BinaryOp::Sub;
  case Op::Mul: return BinaryOp::Mul;
  case Op::Div: return BinaryOp::Div;
  case Op::Mod: return BinaryOp::Mod;
  case Op::Lt: return BinaryOp::Lt;
  case Op::Le: return BinaryOp::Le;
  case Op::Gt: return BinaryOp::Gt;
  default: return BinaryOp::Ge;
  }
}

} // namespace

std::string formatRuntimeError(const RuntimeError& error,
                               const SourceMap& sources) {
  return "RuntimeError: " + error.message + "\n  at " +
         formatLocation(sources, error.location) + "\n";
}

Location Interpreter::where(const Instruction& instruction) const {
  return program_->sites[instruction.site];
}

void Interpreter::fail(std::string message, const Instruction& at) {
  throw Abort{RuntimeError{std::move(message), where(at)}};
}

void Interpreter::unsupported(const char* op, const std::string& operands,
                              const Instruction& at) {
  fail(std::string("Operator '") + op + "' cannot be applied to " + operands,
       at);
}

std::optional<RuntimeError> Interpreter::run(StmtList& program) {
  BytecodeProgram bytecode = compileProgram(program);
  program_ = &bytecode;
  stack_.clear();
  frames_.clear();
  std::optional<RuntimeError> failure;
  try {
    execute();
  } catch (const Abort& abort) {
    failure = abort.error;
  }
  stack_.clear();
  frames_.clear();
  program_ = nullptr;
  return failure;
}

Value Interpreter::pop() {
  Value value = std::move(stack_.back());
  stack_.pop_back();
  return value;
}

void Interpreter::execute() {
  stack_.resize(program_->main->slots);
  frames_.push_back({program_->main, 0, 0});

  while (true) {
    Frame& frame = frames_.back();
    const Instruction& at = frame.code->code[frame.pc++];
    switch (at.op) {
    case Op::Constant:
      push(program_->constants[static_cast<size_t>(at.a)]);
      break;
    case Op::Load:
      push(stack_[frame.base + static_cast<size_t>(at.a)]);
      break;
    case Op::Store:
      stack_[frame.base + static_cast<size_t>(at.a)] = pop();
      break;
    case Op::Assign:
      stack_[frame.base + static_cast<size_t>(at.a)] = stack_.back();
      break;
    case Op::LoadFunction:
      push(Value::function(
          program_->functions[static_cast<size_t>(at.a)].get()));
      break;
    case Op::Pop:
      stack_.pop_back();
      break;
    case Op::Add:
    case Op::Sub:
    case Op::Mul:
    case Op::Div:
    case Op::Mod:
      binary(at);
      break;
    case Op::Eq:
    case Op::Ne: {
      Value right = pop();
      Value left = pop();
      bool same = Value::equals(left, right);
      push(Value::integer(at.op == Op::Eq ? same : !same));
      break;
    }
    case Op::Lt:
    case Op::Le:
    case Op::Gt:
    case Op::Ge:
      compare(at);
      break;
    case Op::Negate: {
      Value operand = pop();
      if (operand.isFloat()) {
        push(Value::real(-operand.asFloat()));
      } else if (operand.isInt()) {
        push(Value::integer(static_cast<int32_t>(
            0u - static_cast<uint32_t>(operand.asInt()))));
      } else {
        unsupported("-", operand.typeName(), at);
      }
      break;
    }
    case Op::Not: {
      Value operand = pop();
      if (!operand.isInt()) {
        unsupported("!", operand.typeName(), at);
      }
      push(Value::integer(operand.asInt() == 0));
      break;
    }
    case Op::Jump:
      frame.pc = static_cast<size_t>(at.a);
      break;
    case Op::JumpIfFalse: {
      Value test = pop();
      if (!test.isInt()) {
        fail(std::string("Condition must be an integer, got ") +
                 test.typeName(),
             at);
      }
      if (test.asInt() == 0) {
        frame.pc = static_cast<size_t>(at.a);
      }
      break;
    }
    case Op::AndShort:
    case Op::OrShort: {
      bool isAnd = at.op == Op::AndShort;
      Value left = pop();
      if (!left.isInt()) {
        unsupported(isAnd ? "&&" : "||", left.typeName(), at);
      }
      if ((left.asInt() != 0) != isAnd) {
        push(Value::integer(!isAnd));
        frame.pc = static_cast<size_t>(at.a);
      }
      break;
    }
    case Op::ToBool: {
      Value right = pop();
      if (!right.isInt()) {
        unsupported(at.a == 0 ? "&&" : "||", right.typeName(), at);
      }
      push(Value::integer(right.asInt() != 0));
      break;
    }
    case Op::Index:
      index(at);
      break;
    case Op::SetIndex:
      setIndex(at);
      break;
    case Op::Field:
      field(at);
      break;
    case Op::SetField:
      setField(at);
      break;
    case Op::Call:
      call(at);
      break;
    case Op::CallBuiltin:
      callBuiltin(at);
      break;
    case Op::MakeArray: {
      size_t count = static_cast<size_t>(at.a);
      std::vector<Value> items(count);
      for (size_t i = count; i > 0; --i) {
        items[i - 1] = pop();
      }
      push(Value::array(std::move(items)));
      break;
    }
    case Op::MakeObject: {
      const std::vector<std::string>& names =
          program_->shapes[static_cast<size_t>(at.a)];
      std::vector<std::pair<std::string, Value>> fields(names.size());
      for (size_t i = names.size(); i > 0; --i) {
        fields[i - 1] = {names[i - 1], pop()};
      }
      push(Value::object(std::move(fields)));
      break;
    }
    case Op::Return: {
      Value result = pop();
      size_t base = frame.base;
      frames_.pop_back();
      stack_.resize(base - 1);
      push(std::move(result));
      break;
    }
    case Op::Clear:
      for (int32_t i = 0; i < at.b; ++i) {
        stack_[frame.base + static_cast<size_t>(at.a + i)] = Value();
      }
      break;
    case Op::ForInit:
      forInit(at);
      break;
    case Op::ForNext: {
      size_t counter = frame.base + static_cast<size_t>(at.b);
      int32_t current = stack_[counter].asInt();
      if (current >= stack_[counter + 1].asInt()) {
        frame.pc = static_cast<size_t>(at.a);
      } else {
        stack_[counter + 2] = Value::integer(current);
      }
      break;
    }
    case Op::ForStep: {
      size_t counter = frame.base + static_cast<size_t>(at.a);
      stack_[counter] = Value::integer(stack_[counter].asInt() + 1);
      break;
    }
    case Op::Halt:
      return;
    }
  }
}

void Interpreter::forInit(const Instruction& at) {
  Value end = pop();
  Value start = pop();
  if (!start.isInt() || !end.isInt()) {
    const Value& bad = start.isInt() ? end : start;
    fail(std::string("Range bounds must be integers, got ") + bad.typeName(),
         at);
  }
  size_t counter = frames_.back().base + static_cast<size_t>(at.a);
  stack_[counter] = std::move(start);
  stack_[counter + 1] = std::move(end);
}

Value Interpreter::arithmetic(BinaryOp op, const Value& left,
                              const Value& right, const Instruction& at) {
  const char* symbol = binaryOpSymbol(op);

  if (op == BinaryOp::Add && (left.isString() || right.isString())) {
    bool printable = (left.isString() || left.isNumber()) &&
                     (right.isString() || right.isNumber());
    if (!printable) {
      unsupported(symbol, typeNames(left, right), at);
    }
    std::string joined = left.display() + right.display();
    if (joined.size() > MEADOWS_MAX_STRING_LENGTH) {
      fail("String exceeds the maximum length", at);
    }
    return Value::string(std::move(joined));
  }

  if (!left.isNumber() || !right.isNumber()) {
    unsupported(symbol, typeNames(left, right), at);
  }

  if (left.isFloat() || right.isFloat()) {
    double a = left.asDouble();
    double b = right.asDouble();
    switch (op) {
    case BinaryOp::Add: return Value::real(a + b);
    case BinaryOp::Sub: return Value::real(a - b);
    case BinaryOp::Mul: return Value::real(a * b);
    default:
      if (b == 0.0) {
        fail("Division by zero", at);
      }
      return Value::real(op == BinaryOp::Div ? a / b : std::fmod(a, b));
    }
  }

  int32_t a = left.asInt();
  int32_t b = right.asInt();
  auto wrap = [](int64_t value) {
    return Value::integer(static_cast<int32_t>(static_cast<uint32_t>(value)));
  };
  switch (op) {
  case BinaryOp::Add: return wrap(int64_t{a} + b);
  case BinaryOp::Sub: return wrap(int64_t{a} - b);
  case BinaryOp::Mul: return wrap(int64_t{a} * b);
  default:
    if (b == 0) {
      fail("Division by zero", at);
    }
    if (op == BinaryOp::Div) {
      return wrap(int64_t{a} / b);
    }
    return Value::integer(b == -1 ? 0 : a % b);
  }
}

void Interpreter::binary(const Instruction& at) {
  Value right = pop();
  Value left = pop();
  push(arithmetic(binaryOpFor(at.op), left, right, at));
}

void Interpreter::compare(const Instruction& at) {
  Value right = pop();
  Value left = pop();
  if (!left.isNumber() || !right.isNumber()) {
    unsupported(binaryOpSymbol(binaryOpFor(at.op)), typeNames(left, right),
                at);
  }
  double a = left.asDouble();
  double b = right.asDouble();
  bool holds = at.op == Op::Lt   ? a < b
               : at.op == Op::Le ? a <= b
               : at.op == Op::Gt ? a > b
                                 : a >= b;
  push(Value::integer(holds));
}

void Interpreter::index(const Instruction& at) {
  Value position = pop();
  Value object = pop();
  if (!object.isArray()) {
    fail("Cannot index " + object.describe(), at);
  }
  if (!position.isInt()) {
    fail(std::string("Array index must be an integer, got ") +
             position.typeName(),
         at);
  }
  const std::vector<Value>& items = object.asArray().items;
  int32_t i = position.asInt();
  if (i < 0 || static_cast<size_t>(i) >= items.size()) {
    fail("Array index " + std::to_string(i) + " out of bounds for length " +
             std::to_string(items.size()),
         at);
  }
  push(items[static_cast<size_t>(i)]);
}

void Interpreter::setIndex(const Instruction& at) {
  Value value = pop();
  Value position = pop();
  Value object = pop();
  if (!object.isArray()) {
    fail("Cannot index " + object.describe(), at);
  }
  if (!position.isInt()) {
    fail(std::string("Array index must be an integer, got ") +
             position.typeName(),
         at);
  }
  std::vector<Value>& items = object.asArray().items;
  int32_t i = position.asInt();
  if (i < 0 || static_cast<size_t>(i) >= items.size()) {
    fail("Array index " + std::to_string(i) + " out of bounds for length " +
             std::to_string(items.size()),
         at);
  }
  items[static_cast<size_t>(i)] = value;
  push(std::move(value));
}

void Interpreter::field(const Instruction& at) {
  const std::string& name = program_->names[static_cast<size_t>(at.a)];
  Value object = pop();
  if (!object.isObject()) {
    fail("Cannot access field '" + name + "' on " + object.describe(), at);
  }
  for (const auto& entry : object.asObject().fields) {
    if (entry.first == name) {
      push(entry.second);
      return;
    }
  }
  fail("Object has no field '" + name + "'", at);
}

void Interpreter::setField(const Instruction& at) {
  const std::string& name = program_->names[static_cast<size_t>(at.a)];
  Value value = pop();
  Value object = pop();
  if (!object.isObject()) {
    fail("Cannot access field '" + name + "' on " + object.describe(), at);
  }
  for (auto& entry : object.asObject().fields) {
    if (entry.first == name) {
      entry.second = value;
      push(std::move(value));
      return;
    }
  }
  fail("Object has no field '" + name + "'", at);
}

void Interpreter::call(const Instruction& at) {
  size_t argc = static_cast<size_t>(at.a);
  size_t calleeIndex = stack_.size() - argc - 1;
  const Value& callee = stack_[calleeIndex];
  if (!callee.isFunction()) {
    fail("Cannot call " + callee.describe(), at);
  }
  const FunctionCode* function = callee.asFunction();
  if (function->arity != argc) {
    fail("Function '" + function->name + "' takes " +
             arguments(function->arity) + ", got " + std::to_string(argc),
         at);
  }
  if (frames_.size() > MEADOWS_MAX_CALL_DEPTH) {
    fail("Maximum call depth exceeded", at);
  }
  size_t base = calleeIndex + 1;
  stack_.resize(base + function->slots);
  frames_.push_back({function, 0, base});
}

void Interpreter::callBuiltin(const Instruction& at) {
  size_t argc = static_cast<size_t>(at.b);
  std::vector<Value> args(argc);
  for (size_t i = argc; i > 0; --i) {
    args[i - 1] = pop();
  }
  const Value& first = args[0];

  switch (static_cast<Builtin>(at.a)) {
  case Builtin::Print:
    if (!first.isNumber() && !first.isString()) {
      fail("print() cannot print " + first.describe(), at);
    }
    output_(first.display() + "\n");
    push(Value());
    break;
  case Builtin::Len:
    if (first.isString()) {
      push(Value::integer(static_cast<int32_t>(first.asString().size())));
    } else if (first.isArray()) {
      push(Value::integer(static_cast<int32_t>(first.asArray().items.size())));
    } else {
      fail(std::string("len() requires a string or an array, got ") +
               first.typeName(),
           at);
    }
    break;
  case Builtin::Str:
    if (!first.isNumber()) {
      fail(std::string("str() requires a number, got ") + first.typeName(), at);
    }
    push(Value::string(first.display()));
    break;
  case Builtin::Push: {
    if (!first.isArray()) {
      fail(std::string("push() requires an array as its first argument, got ") +
               first.typeName(),
           at);
    }
    std::vector<Value>& items = first.asArray().items;
    if (items.size() >= MEADOWS_MAX_ARRAY_LENGTH) {
      fail("Array exceeds the maximum length", at);
    }
    items.push_back(args[1]);
    push(first);
    break;
  }
  }
}

} // namespace meadows
