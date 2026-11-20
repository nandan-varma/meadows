#include <algorithm>
#include <map>

#include "Bytecode.h"

namespace meadows {

namespace {

class Compiler : public ExprVisitor, public StmtVisitor {
public:
  BytecodeProgram compile(StmtList& statements) {
    for (StmtPtr& stmt : statements) {
      if (auto* function = dynamic_cast<FuncStmt*>(stmt.get())) {
        out_.functions.push_back(std::make_unique<FunctionCode>());
        out_.functions.back()->name = function->name;
        out_.functions.back()->arity =
            static_cast<uint32_t>(function->params.size());
        functions_[function->name] = {out_.functions.size() - 1, function};
      }
    }
    for (auto& entry : functions_) {
      compileFunction(entry.second.index, *entry.second.declaration);
    }

    out_.functions.push_back(std::make_unique<FunctionCode>());
    out_.main = out_.functions.back().get();
    begin(*out_.main);
    pushScope();
    emitBody(statements);
    emit(Op::Halt);
    out_.main->slots = maxSlots_;
    return std::move(out_);
  }

private:
  struct Local {
    std::string name;
    uint32_t slot;
  };

  struct Scope {
    std::vector<Local> locals;
    uint32_t firstSlot;
  };

  struct Loop {
    uint32_t clearFrom;
    std::vector<size_t> breakJumps;
    std::vector<size_t> continueJumps;
  };

  struct FunctionEntry {
    size_t index;
    FuncStmt* declaration;
  };

  BytecodeProgram out_;
  std::map<std::string, FunctionEntry> functions_;
  std::map<std::string, int32_t> nameIndex_;
  FunctionCode* current_ = nullptr;
  std::vector<Scope> scopes_;
  std::vector<Loop> loops_;
  uint32_t nextSlot_ = 0;
  uint32_t maxSlots_ = 0;
  bool ended_ = false;

  void begin(FunctionCode& function) {
    current_ = &function;
    scopes_.clear();
    loops_.clear();
    nextSlot_ = 0;
    maxSlots_ = 0;
    ended_ = false;
  }

  void compileFunction(size_t index, FuncStmt& declaration) {
    FunctionCode& function = *out_.functions[index];
    begin(function);
    pushScope();
    for (const Param& param : declaration.params) {
      declare(param.name);
    }
    emitBody(declaration.body);
    if (!ended_) {
      emitConstant(Value::integer(0));
      emit(Op::Return);
    }
    function.slots = maxSlots_;
  }

  size_t here() const { return current_->code.size(); }

  size_t emit(Op op, int32_t a = 0, int32_t b = 0, uint32_t site = 0) {
    current_->code.push_back({op, a, b, site});
    return current_->code.size() - 1;
  }

  void patch(size_t jump) {
    current_->code[jump].a = static_cast<int32_t>(here());
  }

  void patchTo(size_t jump, size_t target) {
    current_->code[jump].a = static_cast<int32_t>(target);
  }

  uint32_t site(const Location& location) {
    out_.sites.push_back(location);
    return static_cast<uint32_t>(out_.sites.size() - 1);
  }

  int32_t name(const std::string& text) {
    auto found = nameIndex_.find(text);
    if (found != nameIndex_.end()) {
      return found->second;
    }
    out_.names.push_back(text);
    return nameIndex_[text] = static_cast<int32_t>(out_.names.size() - 1);
  }

  void emitConstant(Value value) {
    out_.constants.push_back(std::move(value));
    emit(Op::Constant, static_cast<int32_t>(out_.constants.size() - 1));
  }

  void pushScope() { scopes_.push_back({{}, nextSlot_}); }

  void emitClear(uint32_t from) {
    if (nextSlot_ > from) {
      emit(Op::Clear, static_cast<int32_t>(from),
           static_cast<int32_t>(nextSlot_ - from));
    }
  }

  void popScope() {
    if (!ended_) {
      emitClear(scopes_.back().firstSlot);
    }
    nextSlot_ = scopes_.back().firstSlot;
    scopes_.pop_back();
  }

  uint32_t allocate(uint32_t count) {
    uint32_t first = nextSlot_;
    nextSlot_ += count;
    maxSlots_ = std::max(maxSlots_, nextSlot_);
    return first;
  }

  uint32_t declare(const std::string& variable) {
    uint32_t slot = allocate(1);
    scopes_.back().locals.push_back({variable, slot});
    return slot;
  }

  const Local* findLocal(const std::string& variable) const {
    for (auto scope = scopes_.rbegin(); scope != scopes_.rend(); ++scope) {
      for (auto local = scope->locals.rbegin(); local != scope->locals.rend();
           ++local) {
        if (local->name == variable) {
          return &*local;
        }
      }
    }
    return nullptr;
  }

  void emitBody(StmtList& body) {
    for (StmtPtr& stmt : body) {
      if (ended_) {
        return;
      }
      stmt->accept(*this);
    }
  }

  void emitScopedBody(StmtList& body) {
    pushScope();
    emitBody(body);
    popScope();
  }

  void compile(Expr& expr) { expr.accept(*this); }

  void visitLiteralExpr(LiteralExpr& expr) override {
    switch (expr.kind) {
    case LiteralKind::Int:
      emitConstant(Value::integer(expr.intValue));
      break;
    case LiteralKind::Float:
      emitConstant(Value::real(expr.floatValue));
      break;
    case LiteralKind::String:
      emitConstant(Value::string(expr.stringValue));
      break;
    }
  }

  void visitVarExpr(VarExpr& expr) override {
    if (const Local* local = findLocal(expr.name)) {
      emit(Op::Load, static_cast<int32_t>(local->slot));
    } else {
      emit(Op::LoadFunction,
           static_cast<int32_t>(functions_.at(expr.name).index));
    }
  }

  void visitAssignExpr(AssignExpr& expr) override {
    if (auto* target = dynamic_cast<VarExpr*>(expr.target.get())) {
      compile(*expr.value);
      emit(Op::Assign, static_cast<int32_t>(findLocal(target->name)->slot));
    } else if (auto* element = dynamic_cast<IndexExpr*>(expr.target.get())) {
      compile(*element->object);
      compile(*element->index);
      compile(*expr.value);
      emit(Op::SetIndex, 0, 0, site(element->location));
    } else {
      auto& field = static_cast<FieldExpr&>(*expr.target);
      compile(*field.object);
      compile(*expr.value);
      emit(Op::SetField, name(field.name), 0, site(field.location));
    }
  }

  void visitBinaryExpr(BinaryExpr& expr) override {
    compile(*expr.left);
    compile(*expr.right);
    static const std::map<BinaryOp, Op> ops = {
        {BinaryOp::Add, Op::Add}, {BinaryOp::Sub, Op::Sub},
        {BinaryOp::Mul, Op::Mul}, {BinaryOp::Div, Op::Div},
        {BinaryOp::Mod, Op::Mod}, {BinaryOp::Eq, Op::Eq},
        {BinaryOp::Ne, Op::Ne},   {BinaryOp::Lt, Op::Lt},
        {BinaryOp::Le, Op::Le},   {BinaryOp::Gt, Op::Gt},
        {BinaryOp::Ge, Op::Ge}};
    emit(ops.at(expr.op), 0, 0, site(expr.location));
  }

  void visitUnaryExpr(UnaryExpr& expr) override {
    compile(*expr.operand);
    emit(expr.op == UnaryOp::Negate ? Op::Negate : Op::Not, 0, 0,
         site(expr.location));
  }

  void visitLogicalExpr(LogicalExpr& expr) override {
    bool isAnd = expr.op == LogicalOp::And;
    uint32_t where = site(expr.location);
    compile(*expr.left);
    size_t shortCircuit = emit(isAnd ? Op::AndShort : Op::OrShort, 0, 0, where);
    compile(*expr.right);
    emit(Op::ToBool, isAnd ? 0 : 1, 0, where);
    patch(shortCircuit);
  }

  void visitIndexExpr(IndexExpr& expr) override {
    compile(*expr.object);
    compile(*expr.index);
    emit(Op::Index, 0, 0, site(expr.location));
  }

  void visitFieldExpr(FieldExpr& expr) override {
    compile(*expr.object);
    emit(Op::Field, name(expr.name), 0, site(expr.location));
  }

  void visitCallExpr(CallExpr& expr) override {
    auto* callee = dynamic_cast<VarExpr*>(expr.callee.get());
    int32_t argc = static_cast<int32_t>(expr.args.size());
    if (callee && !findLocal(callee->name)) {
      if (const BuiltinInfo* builtin = findBuiltin(callee->name)) {
        for (ExprPtr& arg : expr.args) {
          compile(*arg);
        }
        emit(Op::CallBuiltin, static_cast<int32_t>(builtin->builtin), argc,
             site(expr.location));
        return;
      }
    }
    compile(*expr.callee);
    for (ExprPtr& arg : expr.args) {
      compile(*arg);
    }
    emit(Op::Call, argc, 0, site(expr.location));
  }

  void visitArrayExpr(ArrayExpr& expr) override {
    for (ExprPtr& element : expr.elements) {
      compile(*element);
    }
    emit(Op::MakeArray, static_cast<int32_t>(expr.elements.size()));
  }

  void visitObjectExpr(ObjectExpr& expr) override {
    std::vector<std::string> names;
    for (ObjectField& field : expr.fields) {
      compile(*field.value);
      names.push_back(field.name);
    }
    out_.shapes.push_back(std::move(names));
    emit(Op::MakeObject, static_cast<int32_t>(out_.shapes.size() - 1));
  }

  void visitExprStmt(ExprStmt& stmt) override {
    compile(*stmt.expr);
    emit(Op::Pop);
  }

  void visitLetStmt(LetStmt& stmt) override {
    compile(*stmt.initializer);
    emit(Op::Store, static_cast<int32_t>(declare(stmt.name)));
  }

  void visitFuncStmt(FuncStmt&) override {}

  void visitIfStmt(IfStmt& stmt) override {
    compile(*stmt.condition);
    size_t toElse = emit(Op::JumpIfFalse, 0, 0, site(stmt.condition->location));
    emitScopedBody(stmt.thenBody);
    bool thenEnded = ended_;
    size_t toEnd = thenEnded ? 0 : emit(Op::Jump);
    patch(toElse);
    ended_ = false;
    emitScopedBody(stmt.elseBody);
    if (!thenEnded) {
      patch(toEnd);
    }
    ended_ = false;
  }

  void visitWhileStmt(WhileStmt& stmt) override {
    size_t top = here();
    compile(*stmt.condition);
    size_t toExit = emit(Op::JumpIfFalse, 0, 0, site(stmt.condition->location));
    pushScope();
    loops_.push_back({nextSlot_, {}, {}});
    emitBody(stmt.body);
    popScope();
    if (!ended_) {
      patchTo(emit(Op::Jump), top);
    }
    patch(toExit);
    Loop loop = std::move(loops_.back());
    loops_.pop_back();
    for (size_t jump : loop.breakJumps) {
      patch(jump);
    }
    for (size_t jump : loop.continueJumps) {
      patchTo(jump, top);
    }
    ended_ = false;
  }

  void visitForStmt(ForStmt& stmt) override {
    compile(*stmt.start);
    compile(*stmt.end);
    pushScope();
    uint32_t counter = allocate(2);
    uint32_t variable = declare(stmt.var);
    emit(Op::ForInit, static_cast<int32_t>(counter), 0, site(stmt.location));
    size_t top = here();
    size_t toExit = emit(Op::ForNext, 0, static_cast<int32_t>(counter));
    loops_.push_back({variable, {}, {}});
    emitBody(stmt.body);

    ended_ = false;
    size_t step = here();
    emitClear(variable);
    emit(Op::ForStep, static_cast<int32_t>(counter));
    patchTo(emit(Op::Jump), top);
    patch(toExit);

    Loop loop = std::move(loops_.back());
    loops_.pop_back();
    for (size_t jump : loop.breakJumps) {
      patch(jump);
    }
    for (size_t jump : loop.continueJumps) {
      patchTo(jump, step);
    }
    nextSlot_ = scopes_.back().firstSlot;
    scopes_.pop_back();
    ended_ = false;
  }

  void visitReturnStmt(ReturnStmt& stmt) override {
    if (stmt.value) {
      compile(*stmt.value);
    } else {
      emitConstant(Value::integer(0));
    }
    emit(Op::Return);
    ended_ = true;
  }

  void visitBreakStmt(BreakStmt&) override {
    emitClear(loops_.back().clearFrom);
    loops_.back().breakJumps.push_back(emit(Op::Jump));
    ended_ = true;
  }

  void visitContinueStmt(ContinueStmt&) override {
    emitClear(loops_.back().clearFrom);
    loops_.back().continueJumps.push_back(emit(Op::Jump));
    ended_ = true;
  }

  void visitBlockStmt(BlockStmt& stmt) override {
    emitScopedBody(stmt.body);
    ended_ = false;
  }
};

} // namespace

BytecodeProgram compileProgram(StmtList& program) {
  return Compiler().compile(program);
}

} // namespace meadows
