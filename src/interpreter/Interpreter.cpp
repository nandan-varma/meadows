#include "Interpreter.h"

#include <cctype>
#include <cmath>
#include <cstdint>
#include <stdexcept>
#include <utility>

namespace meadows {

Interpreter::Interpreter(OutputSink output) : output_(std::move(output)) {}

Interpreter::ScopeGuard::ScopeGuard(Interpreter &i, Scope *parent) : interp(i) {
  interp.pushScope(parent);
}

Interpreter::ScopeGuard::~ScopeGuard() { interp.popScope(); }

Interpreter::Scope *Interpreter::pushScope(Scope *parent) {
  scopeStack_.push_back(std::make_unique<Scope>());
  scopeStack_.back()->parent = parent;
  currentScope_ = scopeStack_.back().get();
  return currentScope_;
}

void Interpreter::popScope() {
  scopeStack_.pop_back();
  currentScope_ = scopeStack_.empty() ? nullptr : scopeStack_.back().get();
}

Value *Interpreter::lookup(const std::string &name) {
  for (Scope *s = currentScope_; s; s = s->parent) {
    auto it = s->vars.find(name);
    if (it != s->vars.end()) return &it->second;
  }
  return nullptr;
}

void Interpreter::bumpStep() {
  if (++steps_ > kMaxSteps) {
    runtimeError(
        "PlaygroundError: execution step limit exceeded (possible infinite loop)");
  }
}

[[noreturn]] void Interpreter::runtimeError(const std::string &message) {
  throw InterpreterError{message};
}

void Interpreter::requireInt(const Value &v, const char *context) {
  if (!v.isInt()) {
    runtimeError(std::string("RuntimeError: ") + context +
                 " must be an integer, got " + v.typeName());
  }
}

void Interpreter::requireNumeric(const Value &v, const char *context) {
  if (!v.isNumeric()) {
    runtimeError(std::string("RuntimeError: ") + context +
                 " must be a number, got " + v.typeName());
  }
}

Value Interpreter::eval(Expr &expr) {
  bumpStep();
  expr.accept(*this);
  return std::move(result_);
}

void Interpreter::execStmts(const std::vector<std::unique_ptr<Stmt>> &stmts) {
  for (auto &s : stmts) {
    bumpStep();
    s->accept(*this);
  }
}

int Interpreter::run(const std::vector<std::unique_ptr<Stmt>> &statements) {
  pushScope(nullptr);
  globalScope_ = currentScope_;

  // Hoist top-level function declarations so forward/mutual calls resolve —
  // mirrors SemanticAnalyzer::analyze's pre-pass, which registers the same
  // names for validation before the rest of the file is checked.
  for (auto &stmt : statements) {
    if (auto *fn = dynamic_cast<FuncStmt *>(stmt.get())) {
      functions_[fn->name] = fn;
    }
  }

  try {
    execStmts(statements);
  } catch (const InterpreterError &e) {
    output_(e.message + "\n");
    return -1;
  } catch (const ReturnSignal &) {
    // `return` at top level: SemanticAnalyzer rejects this (E3011) for any
    // program that reaches interpretation, but stay defensive.
  } catch (const BreakSignal &) {
  } catch (const ContinueSignal &) {
  }
  return 0;
}

// ── Expressions ────────────────────────────────────────────────────────────

void Interpreter::visitLiteralExpr(LiteralExpr &expr) {
  switch (expr.kind) {
    case LiteralKind::Int:
      try {
        result_ = Value::ofInt(std::stoi(expr.value));
      } catch (const std::out_of_range &) {
        // Matches CodeGen::visitLiteralExpr's overflow clamp.
        result_ = Value::ofInt(INT32_MAX);
      }
      return;
    case LiteralKind::Float:
      result_ = Value::ofFloat(std::stod(expr.value));
      return;
    case LiteralKind::Str:
      result_ = Value::ofStr(expr.value);
      return;
  }
}

void Interpreter::visitVarExpr(VarExpr &expr) {
  Value *v = lookup(expr.name);
  if (!v) runtimeError("RuntimeError: undefined variable '" + expr.name + "'");
  result_ = *v;
}

void Interpreter::visitAssignExpr(AssignExpr &expr) {
  if (auto *varTarget = dynamic_cast<VarExpr *>(expr.target.get())) {
    Value v = eval(*expr.value);
    Value *slot = lookup(varTarget->name);
    if (!slot)
      runtimeError("RuntimeError: assignment to undefined variable '" +
                   varTarget->name + "'");
    *slot = v;
    result_ = v;
    return;
  }

  if (auto *indexTarget = dynamic_cast<IndexExpr *>(expr.target.get())) {
    Value arr = eval(*indexTarget->array);
    Value idx = eval(*indexTarget->index);
    if (!arr.isArray())
      runtimeError("RuntimeError: cannot index a " + std::string(arr.typeName()));
    requireInt(idx, "array index");

    int32_t i = idx.asInt();
    auto &elems = arr.asArray(); // aliases the same shared_ptr<ValueArray>
                                 // as the variable this Value was read from
    if (i < 0 || static_cast<size_t>(i) >= elems.size())
      runtimeError("RuntimeError: Array index out of bounds");

    Value v = eval(*expr.value);
    elems[static_cast<size_t>(i)] = v;
    result_ = v;
    return;
  }

  if (auto *fieldTarget = dynamic_cast<FieldAccessExpr *>(expr.target.get())) {
    Value obj = eval(*fieldTarget->object);
    if (!obj.isObject())
      runtimeError("RuntimeError: cannot access field '" +
                   fieldTarget->fieldName + "' on a " +
                   std::string(obj.typeName()));
    auto &fields = obj.asObject();
    auto it = fields.find(fieldTarget->fieldName);
    if (it == fields.end())
      runtimeError("RuntimeError: object has no field '" +
                   fieldTarget->fieldName + "'");

    Value v = eval(*expr.value);
    it->second = v;
    result_ = v;
    return;
  }

  runtimeError("RuntimeError: unsupported assignment target");
}

void Interpreter::visitBinaryExpr(BinaryExpr &expr) {
  Value left = eval(*expr.left);
  Value right = eval(*expr.right);

  if (expr.op == "+") {
    if (left.isStr() || right.isStr()) {
      std::string a = left.displayString();
      std::string b = right.displayString();
      // Repeated concatenation in a loop is O(n) per step — bound the result
      // size so that's O(n) total steps of bounded cost, not unbounded
      // memory growth. Matches CodeGen.h's MAX_STRING_LENGTH.
      if (a.size() + b.size() > kMaxStringLength)
        runtimeError("RuntimeError: string exceeds maximum length");
      result_ = Value::ofStr(a + b);
    } else if (left.isFloat() || right.isFloat()) {
      requireNumeric(left, "operand of '+'");
      requireNumeric(right, "operand of '+'");
      result_ = Value::ofFloat(left.asDouble() + right.asDouble());
    } else {
      requireInt(left, "operand of '+'");
      requireInt(right, "operand of '+'");
      result_ = Value::ofInt(static_cast<int32_t>(
          static_cast<uint32_t>(left.asInt()) + static_cast<uint32_t>(right.asInt())));
    }
    return;
  }

  if (expr.op == "==") {
    result_ = Value::ofInt(left.equals(right) ? 1 : 0);
    return;
  }
  if (expr.op == "!=") {
    result_ = Value::ofInt(left.equals(right) ? 0 : 1);
    return;
  }

  // Every remaining operator requires two numbers. If either is a Float,
  // the Int operand (if any) promotes to Float — matches CodeGen's use of
  // CreateSIToFP in the same situation.
  requireNumeric(left, "operand of binary operator");
  requireNumeric(right, "operand of binary operator");

  if (left.isFloat() || right.isFloat()) {
    double a = left.asDouble();
    double b = right.asDouble();
    if (expr.op == "-") {
      result_ = Value::ofFloat(a - b);
    } else if (expr.op == "*") {
      result_ = Value::ofFloat(a * b);
    } else if (expr.op == "/") {
      if (b == 0.0) runtimeError("RuntimeError: Division by zero");
      result_ = Value::ofFloat(a / b);
    } else if (expr.op == "%") {
      if (b == 0.0) runtimeError("RuntimeError: Division by zero");
      result_ = Value::ofFloat(std::fmod(a, b));
    } else if (expr.op == ">") {
      result_ = Value::ofInt(a > b ? 1 : 0);
    } else if (expr.op == "<") {
      result_ = Value::ofInt(a < b ? 1 : 0);
    } else if (expr.op == ">=") {
      result_ = Value::ofInt(a >= b ? 1 : 0);
    } else if (expr.op == "<=") {
      result_ = Value::ofInt(a <= b ? 1 : 0);
    } else {
      runtimeError("RuntimeError: unknown binary operator '" + expr.op + "'");
    }
    return;
  }

  int32_t a = left.asInt();
  int32_t b = right.asInt();

  if (expr.op == "-") {
    result_ = Value::ofInt(static_cast<int32_t>(static_cast<uint32_t>(a) - static_cast<uint32_t>(b)));
  } else if (expr.op == "*") {
    result_ = Value::ofInt(static_cast<int32_t>(static_cast<uint32_t>(a) * static_cast<uint32_t>(b)));
  } else if (expr.op == "/") {
    if (b == 0) runtimeError("RuntimeError: Division by zero");
    result_ = Value::ofInt(a / b);
  } else if (expr.op == "%") {
    if (b == 0) runtimeError("RuntimeError: Division by zero");
    result_ = Value::ofInt(a % b);
  } else if (expr.op == ">") {
    result_ = Value::ofInt(a > b ? 1 : 0);
  } else if (expr.op == "<") {
    result_ = Value::ofInt(a < b ? 1 : 0);
  } else if (expr.op == ">=") {
    result_ = Value::ofInt(a >= b ? 1 : 0);
  } else if (expr.op == "<=") {
    result_ = Value::ofInt(a <= b ? 1 : 0);
  } else {
    runtimeError("RuntimeError: unknown binary operator '" + expr.op + "'");
  }
}

void Interpreter::visitUnaryExpr(UnaryExpr &expr) {
  Value operand = eval(*expr.operand);
  if (expr.op == "-") {
    // Truthiness (!, &&, ||, conditions) stays Int-only in both backends —
    // CodeGen's CreateCondBr/ICmpEQ-against-i32-0 pattern would need
    // restructuring to accept a double operand there. Negation has no such
    // constraint, so it promotes like the binary arithmetic operators do.
    requireNumeric(operand, "operand of unary '-'");
    result_ = operand.isFloat()
                 ? Value::ofFloat(-operand.asFloat())
                 : Value::ofInt(static_cast<int32_t>(
                       0u - static_cast<uint32_t>(operand.asInt())));
  } else if (expr.op == "!") {
    requireInt(operand, "operand of unary '!'");
    result_ = Value::ofInt(operand.asInt() == 0 ? 1 : 0);
  } else {
    runtimeError("RuntimeError: unknown unary operator '" + expr.op + "'");
  }
}

void Interpreter::visitLogicalExpr(LogicalExpr &expr) {
  // Value-forwarding, not strict boolean reduction — matches the PHI nodes
  // CodeGen::visitLogicalExpr builds: AND yields 0 or the right operand's
  // value; OR yields the left operand's value or the right operand's value.
  Value left = eval(*expr.left);
  requireInt(left, "operand of logical operator");

  if (expr.op == LogicalOperator::AND) {
    if (left.asInt() == 0) {
      result_ = Value::ofInt(0);
      return;
    }
    Value right = eval(*expr.right);
    requireInt(right, "operand of logical operator");
    result_ = right;
  } else {
    if (left.asInt() != 0) {
      result_ = left;
      return;
    }
    Value right = eval(*expr.right);
    requireInt(right, "operand of logical operator");
    result_ = right;
  }
}

void Interpreter::visitIndexExpr(IndexExpr &expr) {
  Value arr = eval(*expr.array);
  Value idx = eval(*expr.index);
  if (!arr.isArray())
    runtimeError("RuntimeError: cannot index a " + std::string(arr.typeName()));
  requireInt(idx, "array index");

  int32_t i = idx.asInt();
  const auto &elems = arr.asArray();
  if (i < 0 || static_cast<size_t>(i) >= elems.size())
    runtimeError("RuntimeError: Array index out of bounds");
  result_ = elems[static_cast<size_t>(i)];
}

void Interpreter::visitFieldAccessExpr(FieldAccessExpr &expr) {
  Value obj = eval(*expr.object);
  if (!obj.isObject())
    runtimeError("RuntimeError: cannot access field '" + expr.fieldName +
                 "' on a " + obj.typeName());
  const auto &fields = obj.asObject();
  auto it = fields.find(expr.fieldName);
  if (it == fields.end())
    runtimeError("RuntimeError: object has no field '" + expr.fieldName + "'");
  result_ = it->second;
}

void Interpreter::visitCallExpr(CallExpr &expr) {
  auto *varExpr = dynamic_cast<VarExpr *>(expr.callee.get());
  if (!varExpr) runtimeError("RuntimeError: only direct function calls are supported");

  std::vector<Value> args;
  args.reserve(expr.args.size());
  for (auto &arg : expr.args) args.push_back(eval(*arg));

  static const std::unordered_map<std::string, bool> kBuiltins = {
      {"print", true}, {"len", true}, {"str", true}};
  if (kBuiltins.count(varExpr->name)) {
    result_ = callBuiltin(varExpr->name, std::move(args));
    return;
  }

  auto it = functions_.find(varExpr->name);
  if (it == functions_.end())
    runtimeError("RuntimeError: undefined function '" + varExpr->name + "'");
  result_ = callFunction(*it->second, std::move(args));
}

Value Interpreter::callBuiltin(const std::string &name, std::vector<Value> args) {
  if (name == "print") {
    const Value &v = args[0];
    if (!v.isNumeric() && !v.isStr())
      runtimeError("RuntimeError: print() does not support a " +
                   std::string(v.typeName()) + " argument");
    output_(v.displayString() + "\n");
    return Value::ofInt(0);
  }
  if (name == "len") {
    const Value &v = args[0];
    if (v.isStr()) return Value::ofInt(static_cast<int32_t>(v.asStr().size()));
    if (v.isArray()) return Value::ofInt(static_cast<int32_t>(v.asArray().size()));
    runtimeError("RuntimeError: len() argument must be a string or array");
  }
  if (name == "str") {
    const Value &v = args[0];
    if (!v.isNumeric())
      runtimeError("RuntimeError: str() argument must be a number");
    return Value::ofStr(v.displayString());
  }
  runtimeError("RuntimeError: unknown builtin '" + name + "'");
}

Value Interpreter::callFunction(FuncStmt &fn, std::vector<Value> args) {
  if (++callDepth_ > kMaxCallDepth) {
    --callDepth_;
    runtimeError(
        "PlaygroundError: maximum call depth exceeded (possible infinite recursion)");
  }
  struct DepthGuard {
    int &depth;
    ~DepthGuard() { --depth; }
  } depthGuard{callDepth_};

  // Function scopes chain to the global scope, not the caller's locals —
  // Meadows functions are flat/global, matching SemanticAnalyzer's single
  // function-name table and CodeGen's module-level llvm::Function lookup.
  ScopeGuard guard(*this, globalScope_);
  for (size_t i = 0; i < fn.params.size(); i++) {
    currentScope_->vars[fn.params[i]] = std::move(args[i]);
  }

  try {
    execStmts(fn.body);
  } catch (const ReturnSignal &r) {
    return r.value;
  }
  return Value::ofInt(0); // implicit fallthrough returns 0, matching CodeGen.
}

void Interpreter::visitArrayExpr(ArrayExpr &expr) {
  auto arr = std::make_shared<ValueArray>();
  arr->reserve(expr.elements.size());
  for (auto &e : expr.elements) arr->push_back(eval(*e));
  result_ = Value::ofArray(std::move(arr));
}

void Interpreter::visitObjectExpr(ObjectExpr &expr) {
  auto obj = std::make_shared<ValueObject>();
  for (auto &[key, valueExpr] : expr.pairs) {
    (*obj)[key] = eval(*valueExpr);
  }
  result_ = Value::ofObject(std::move(obj));
}

// ── Statements ─────────────────────────────────────────────────────────────

void Interpreter::visitExprStmt(ExprStmt &stmt) { eval(*stmt.expr); }

void Interpreter::visitLetStmt(LetStmt &stmt) {
  Value v = eval(*stmt.initializer);
  currentScope_->vars[stmt.name] = std::move(v);
}

void Interpreter::visitFuncStmt(FuncStmt &stmt) { functions_[stmt.name] = &stmt; }

void Interpreter::visitIfStmt(IfStmt &stmt) {
  Value cond = eval(*stmt.condition);
  requireInt(cond, "if condition");
  if (cond.asInt() != 0) {
    ScopeGuard guard(*this, currentScope_);
    execStmts(stmt.thenBranch);
  } else if (!stmt.elseBranch.empty()) {
    ScopeGuard guard(*this, currentScope_);
    execStmts(stmt.elseBranch);
  }
}

void Interpreter::visitForStmt(ForStmt &stmt) {
  Value startVal = eval(*stmt.rangeStart);
  Value endVal = eval(*stmt.rangeEnd);
  requireInt(startVal, "for-loop range start");
  requireInt(endVal, "for-loop range end");
  int32_t end = endVal.asInt();

  ScopeGuard guard(*this, currentScope_);
  Scope *loopScope = currentScope_;

  for (int32_t cur = startVal.asInt(); cur < end; cur++) {
    loopScope->vars[stmt.var] = Value::ofInt(cur);
    bumpStep();
    try {
      execStmts(stmt.body);
    } catch (const BreakSignal &) {
      break;
    } catch (const ContinueSignal &) {
      continue;
    }
  }
}

void Interpreter::visitWhileStmt(WhileStmt &stmt) {
  Scope *outerScope = currentScope_;
  ScopeGuard guard(*this, outerScope);
  Scope *bodyScope = currentScope_;

  while (true) {
    currentScope_ = outerScope;
    Value cond = eval(*stmt.condition);
    requireInt(cond, "while condition");
    currentScope_ = bodyScope;
    if (cond.asInt() == 0) break;

    bumpStep();
    try {
      execStmts(stmt.body);
    } catch (const BreakSignal &) {
      break;
    } catch (const ContinueSignal &) {
      continue;
    }
  }
}

void Interpreter::visitReturnStmt(ReturnStmt &stmt) {
  Value v = stmt.value ? eval(*stmt.value) : Value::ofInt(0);
  throw ReturnSignal{std::move(v)};
}

void Interpreter::visitBreakStmt(BreakStmt &) { throw BreakSignal{}; }

void Interpreter::visitContinueStmt(ContinueStmt &) { throw ContinueSignal{}; }

void Interpreter::visitBlockStmt(BlockStmt &stmt) {
  ScopeGuard guard(*this, currentScope_);
  execStmts(stmt.body);
}

} // namespace meadows
