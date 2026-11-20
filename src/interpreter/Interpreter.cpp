#include "Interpreter.h"

#include <cmath>
#include <limits>

#include "../runtime/Limits.h"
#include "LargeStack.h"

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

} // namespace

std::string formatRuntimeError(const RuntimeError& error,
                               const SourceMap& sources) {
  return "RuntimeError: " + error.message + "\n  at " +
         formatLocation(sources, error.location) + "\n";
}

void Interpreter::fail(std::string message, Location site) {
  throw Abort{RuntimeError{std::move(message), site}};
}

void Interpreter::unsupported(const char* op, const std::string& operands,
                              Location site) {
  fail(std::string("Operator '") + op + "' cannot be applied to " + operands,
       site);
}

std::optional<RuntimeError> Interpreter::run(StmtList& program) {
  functions_.clear();
  bindings_.clear();
  frameBase_ = 0;
  depth_ = 0;
  flow_ = Flow::Next;
  for (StmtPtr& stmt : program) {
    if (auto* func = dynamic_cast<FuncStmt*>(stmt.get())) {
      functions_[func->name] = func;
    }
  }
  std::optional<RuntimeError> failure;
  runOnLargeStack([&]() {
    try {
      execute(program);
    } catch (const Abort& abort) {
      failure = abort.error;
    }
  });
  return failure;
}

Value Interpreter::eval(Expr& expr) {
  expr.accept(*this);
  return std::move(result_);
}

void Interpreter::execute(StmtList& body) {
  for (StmtPtr& stmt : body) {
    stmt->accept(*this);
    if (flow_ != Flow::Next) {
      return;
    }
  }
}

void Interpreter::executeScoped(StmtList& body) {
  size_t mark = bindings_.size();
  execute(body);
  bindings_.resize(mark);
}

Value* Interpreter::findVariable(const std::string& name) {
  for (size_t i = bindings_.size(); i > frameBase_; --i) {
    Binding& binding = bindings_[i - 1];
    if (*binding.name == name) {
      return &binding.value;
    }
  }
  return nullptr;
}

bool Interpreter::condition(const Value& value, Location site) {
  if (!value.isInt()) {
    fail(std::string("Condition must be an integer, got ") + value.typeName(),
         site);
  }
  return value.asInt() != 0;
}

void Interpreter::visitLiteralExpr(LiteralExpr& expr) {
  switch (expr.kind) {
  case LiteralKind::Int:
    result_ = Value::integer(expr.intValue);
    break;
  case LiteralKind::Float:
    result_ = Value::real(expr.floatValue);
    break;
  case LiteralKind::String:
    result_ = Value::string(expr.stringValue);
    break;
  }
}

void Interpreter::visitVarExpr(VarExpr& expr) {
  if (Value* variable = findVariable(expr.name)) {
    result_ = *variable;
  } else {
    result_ = Value::function(functions_.at(expr.name));
  }
}

void Interpreter::visitAssignExpr(AssignExpr& expr) {
  if (auto* target = dynamic_cast<VarExpr*>(expr.target.get())) {
    Value value = eval(*expr.value);
    *findVariable(target->name) = value;
    result_ = std::move(value);
    return;
  }

  if (auto* target = dynamic_cast<IndexExpr*>(expr.target.get())) {
    Value object = eval(*target->object);
    Value index = eval(*target->index);
    Value value = eval(*expr.value);
    if (!object.isArray()) {
      fail("Cannot index " + object.describe(), target->location);
    }
    if (!index.isInt()) {
      fail(std::string("Array index must be an integer, got ") +
               index.typeName(),
           target->location);
    }
    std::vector<Value>& items = object.asArray().items;
    int32_t i = index.asInt();
    if (i < 0 || static_cast<size_t>(i) >= items.size()) {
      fail("Array index " + std::to_string(i) + " out of bounds for length " +
               std::to_string(items.size()),
           target->location);
    }
    items[static_cast<size_t>(i)] = value;
    result_ = std::move(value);
    return;
  }

  auto& target = static_cast<FieldExpr&>(*expr.target);
  Value object = eval(*target.object);
  Value value = eval(*expr.value);
  if (!object.isObject()) {
    fail("Cannot access field '" + target.name + "' on " + object.describe(),
         target.location);
  }
  for (auto& field : object.asObject().fields) {
    if (field.first == target.name) {
      field.second = value;
      result_ = std::move(value);
      return;
    }
  }
  fail("Object has no field '" + target.name + "'", target.location);
}

Value Interpreter::arithmetic(BinaryOp op, const Value& left,
                              const Value& right, Location site) {
  const char* symbol = binaryOpSymbol(op);

  if (op == BinaryOp::Add && (left.isString() || right.isString())) {
    bool printable = (left.isString() || left.isNumber()) &&
                     (right.isString() || right.isNumber());
    if (!printable) {
      unsupported(symbol, typeNames(left, right), site);
    }
    std::string joined = left.display() + right.display();
    if (joined.size() > MEADOWS_MAX_STRING_LENGTH) {
      fail("String exceeds the maximum length", site);
    }
    return Value::string(std::move(joined));
  }

  if (!left.isNumber() || !right.isNumber()) {
    unsupported(symbol, typeNames(left, right), site);
  }

  if (left.isFloat() || right.isFloat()) {
    double a = left.asDouble();
    double b = right.asDouble();
    switch (op) {
    case BinaryOp::Add: return Value::real(a + b);
    case BinaryOp::Sub: return Value::real(a - b);
    case BinaryOp::Mul: return Value::real(a * b);
    case BinaryOp::Div:
    case BinaryOp::Mod:
      if (b == 0.0) {
        fail("Division by zero", site);
      }
      return Value::real(op == BinaryOp::Div ? a / b : std::fmod(a, b));
    default: unsupported(symbol, typeNames(left, right), site);
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
  case BinaryOp::Div:
  case BinaryOp::Mod:
    if (b == 0) {
      fail("Division by zero", site);
    }
    if (op == BinaryOp::Div) {
      return wrap(int64_t{a} / b);
    }
    return Value::integer(b == -1 ? 0 : a % b);
  default: unsupported(symbol, typeNames(left, right), site);
  }
}

void Interpreter::visitBinaryExpr(BinaryExpr& expr) {
  Value left = eval(*expr.left);
  Value right = eval(*expr.right);

  switch (expr.op) {
  case BinaryOp::Eq:
    result_ = Value::integer(Value::equals(left, right));
    return;
  case BinaryOp::Ne:
    result_ = Value::integer(!Value::equals(left, right));
    return;
  case BinaryOp::Lt:
  case BinaryOp::Le:
  case BinaryOp::Gt:
  case BinaryOp::Ge: {
    if (!left.isNumber() || !right.isNumber()) {
      unsupported(binaryOpSymbol(expr.op), typeNames(left, right),
                  expr.location);
    }
    double a = left.asDouble();
    double b = right.asDouble();
    bool holds = expr.op == BinaryOp::Lt   ? a < b
                 : expr.op == BinaryOp::Le ? a <= b
                 : expr.op == BinaryOp::Gt ? a > b
                                           : a >= b;
    result_ = Value::integer(holds);
    return;
  }
  default:
    result_ = arithmetic(expr.op, left, right, expr.location);
  }
}

void Interpreter::visitUnaryExpr(UnaryExpr& expr) {
  Value operand = eval(*expr.operand);
  const char* symbol = unaryOpSymbol(expr.op);
  if (expr.op == UnaryOp::Negate) {
    if (operand.isFloat()) {
      result_ = Value::real(-operand.asFloat());
    } else if (operand.isInt()) {
      result_ = Value::integer(
          static_cast<int32_t>(0u - static_cast<uint32_t>(operand.asInt())));
    } else {
      unsupported(symbol, operand.typeName(), expr.location);
    }
  } else {
    if (!operand.isInt()) {
      unsupported(symbol, operand.typeName(), expr.location);
    }
    result_ = Value::integer(operand.asInt() == 0);
  }
}

void Interpreter::visitLogicalExpr(LogicalExpr& expr) {
  auto truthy = [&](Expr& operand) {
    Value value = eval(operand);
    if (!value.isInt()) {
      unsupported(logicalOpSymbol(expr.op), value.typeName(), expr.location);
    }
    return value.asInt() != 0;
  };
  bool isAnd = expr.op == LogicalOp::And;
  bool left = truthy(*expr.left);
  if (left != isAnd) {
    result_ = Value::integer(left);
    return;
  }
  result_ = Value::integer(truthy(*expr.right));
}

void Interpreter::visitIndexExpr(IndexExpr& expr) {
  Value object = eval(*expr.object);
  Value index = eval(*expr.index);
  if (!object.isArray()) {
    fail("Cannot index " + object.describe(), expr.location);
  }
  if (!index.isInt()) {
    fail(std::string("Array index must be an integer, got ") + index.typeName(),
         expr.location);
  }
  const std::vector<Value>& items = object.asArray().items;
  int32_t i = index.asInt();
  if (i < 0 || static_cast<size_t>(i) >= items.size()) {
    fail("Array index " + std::to_string(i) + " out of bounds for length " +
             std::to_string(items.size()),
         expr.location);
  }
  result_ = items[static_cast<size_t>(i)];
}

void Interpreter::visitFieldExpr(FieldExpr& expr) {
  Value object = eval(*expr.object);
  if (!object.isObject()) {
    fail("Cannot access field '" + expr.name + "' on " + object.describe(),
         expr.location);
  }
  for (const auto& field : object.asObject().fields) {
    if (field.first == expr.name) {
      result_ = field.second;
      return;
    }
  }
  fail("Object has no field '" + expr.name + "'", expr.location);
}

void Interpreter::visitCallExpr(CallExpr& expr) {
  const BuiltinInfo* builtin = nullptr;
  Value callee;
  auto* name = dynamic_cast<VarExpr*>(expr.callee.get());
  if (name && !findVariable(name->name)) {
    builtin = findBuiltin(name->name);
  }
  if (!builtin) {
    callee = eval(*expr.callee);
  }

  std::vector<Value> args;
  args.reserve(expr.args.size());
  for (ExprPtr& arg : expr.args) {
    args.push_back(eval(*arg));
  }

  result_ = builtin ? callBuiltin(builtin->builtin, args, expr.location)
                    : call(callee, std::move(args), expr.location);
}

Value Interpreter::call(const Value& callee, std::vector<Value> args,
                        Location site) {
  if (!callee.isFunction()) {
    fail("Cannot call " + callee.describe(), site);
  }
  FuncStmt& function = *callee.asFunction();
  if (args.size() != function.params.size()) {
    fail("Function '" + function.name + "' takes " +
             arguments(function.params.size()) + ", got " +
             std::to_string(args.size()),
         site);
  }
  if (depth_ >= MEADOWS_MAX_CALL_DEPTH || stackNearlyExhausted()) {
    fail("Maximum call depth exceeded", site);
  }

  ++depth_;
  size_t savedBase = frameBase_;
  size_t mark = bindings_.size();
  frameBase_ = mark;
  for (size_t i = 0; i < args.size(); ++i) {
    bindings_.push_back({&function.params[i].name, std::move(args[i])});
  }

  returned_ = Value();
  execute(function.body);
  Value result = flow_ == Flow::Return ? std::move(returned_) : Value();
  flow_ = Flow::Next;

  bindings_.resize(mark);
  frameBase_ = savedBase;
  --depth_;
  return result;
}

Value Interpreter::callBuiltin(Builtin builtin, std::vector<Value>& args,
                               Location site) {
  const Value& first = args[0];
  switch (builtin) {
  case Builtin::Print:
    if (!first.isNumber() && !first.isString()) {
      fail("print() cannot print " + first.describe(), site);
    }
    output_(first.display() + "\n");
    return Value();
  case Builtin::Len:
    if (first.isString()) {
      return Value::integer(static_cast<int32_t>(first.asString().size()));
    }
    if (first.isArray()) {
      return Value::integer(static_cast<int32_t>(first.asArray().items.size()));
    }
    fail(std::string("len() requires a string or an array, got ") +
             first.typeName(),
         site);
  case Builtin::Str:
    if (!first.isNumber()) {
      fail(std::string("str() requires a number, got ") + first.typeName(),
           site);
    }
    return Value::string(first.display());
  case Builtin::Push: {
    if (!first.isArray()) {
      fail(std::string("push() requires an array as its first argument, got ") +
               first.typeName(),
           site);
    }
    std::vector<Value>& items = first.asArray().items;
    if (items.size() >= MEADOWS_MAX_ARRAY_LENGTH) {
      fail("Array exceeds the maximum length", site);
    }
    items.push_back(args[1]);
    return first;
  }
  }
  return Value();
}

void Interpreter::visitArrayExpr(ArrayExpr& expr) {
  std::vector<Value> items;
  items.reserve(expr.elements.size());
  for (ExprPtr& element : expr.elements) {
    items.push_back(eval(*element));
  }
  result_ = Value::array(std::move(items));
}

void Interpreter::visitObjectExpr(ObjectExpr& expr) {
  std::vector<std::pair<std::string, Value>> fields;
  fields.reserve(expr.fields.size());
  for (ObjectField& field : expr.fields) {
    fields.emplace_back(field.name, eval(*field.value));
  }
  result_ = Value::object(std::move(fields));
}

void Interpreter::visitExprStmt(ExprStmt& stmt) { eval(*stmt.expr); }

void Interpreter::visitLetStmt(LetStmt& stmt) {
  Value value = eval(*stmt.initializer);
  bindings_.push_back({&stmt.name, std::move(value)});
}

void Interpreter::visitFuncStmt(FuncStmt&) {}

void Interpreter::visitIfStmt(IfStmt& stmt) {
  Value test = eval(*stmt.condition);
  executeScoped(condition(test, stmt.condition->location) ? stmt.thenBody
                                                          : stmt.elseBody);
}

void Interpreter::visitForStmt(ForStmt& stmt) {
  Value start = eval(*stmt.start);
  Value end = eval(*stmt.end);
  if (!start.isInt() || !end.isInt()) {
    const Value& bad = start.isInt() ? end : start;
    fail(std::string("Range bounds must be integers, got ") + bad.typeName(),
         stmt.location);
  }
  int32_t limit = end.asInt();
  size_t mark = bindings_.size();
  for (int32_t i = start.asInt(); i < limit; ++i) {
    bindings_.push_back({&stmt.var, Value::integer(i)});
    execute(stmt.body);
    bindings_.resize(mark);
    if (flow_ == Flow::Break) {
      flow_ = Flow::Next;
      break;
    }
    if (flow_ == Flow::Continue) {
      flow_ = Flow::Next;
    } else if (flow_ == Flow::Return) {
      return;
    }
  }
}

void Interpreter::visitWhileStmt(WhileStmt& stmt) {
  while (true) {
    Value test = eval(*stmt.condition);
    if (!condition(test, stmt.condition->location)) {
      return;
    }
    executeScoped(stmt.body);
    if (flow_ == Flow::Break) {
      flow_ = Flow::Next;
      return;
    }
    if (flow_ == Flow::Continue) {
      flow_ = Flow::Next;
    } else if (flow_ == Flow::Return) {
      return;
    }
  }
}

void Interpreter::visitReturnStmt(ReturnStmt& stmt) {
  returned_ = stmt.value ? eval(*stmt.value) : Value();
  flow_ = Flow::Return;
}

void Interpreter::visitBreakStmt(BreakStmt&) { flow_ = Flow::Break; }

void Interpreter::visitContinueStmt(ContinueStmt&) { flow_ = Flow::Continue; }

void Interpreter::visitBlockStmt(BlockStmt& stmt) { executeScoped(stmt.body); }

} // namespace meadows
