#include "SemanticAnalyzer.h"

namespace meadows {

SemanticAnalyzer::SemanticAnalyzer(DiagnosticsCollector &diagnostics,
                                   WarningManager &warnings)
    : diag_(diagnostics), warnings_(warnings) {}

bool SemanticAnalyzer::analyze(
    const std::vector<std::unique_ptr<Stmt>> &stmts) {
  scopes_.clear();
  functions_.clear();
  inFunction_ = false;
  inLoop_ = false;
  unreachable_ = false;

  enterScope();

  // Pre-pass: register all top-level function names so mutual/forward calls work
  for (const auto &stmt : stmts) {
    if (auto *fn = dynamic_cast<FuncStmt *>(stmt.get())) {
      functions_[fn->name] = fn->params.size();
    }
  }

  analyzeBlock(stmts);
  exitScope();

  return !diag_.hasErrors();
}

// ── Scope helpers ─────────────────────────────────────────────────────────────

void SemanticAnalyzer::enterScope() { scopes_.emplace_back(); }

void SemanticAnalyzer::exitScope() {
  if (scopes_.empty()) return;

  for (auto &[name, info] : scopes_.back()) {
    if (!info.used && !info.isParam) {
      reportWarning(ErrorCode::WARN_UNUSED_VARIABLE,
                    "Variable '" + name + "' declared but never used",
                    info.loc);
    }
  }
  scopes_.pop_back();
}

void SemanticAnalyzer::declareVar(const std::string &name,
                                  const SourceLocation &loc, bool isParam) {
  // Check for shadowing in an outer scope
  for (int i = static_cast<int>(scopes_.size()) - 2; i >= 0; --i) {
    if (scopes_[static_cast<size_t>(i)].count(name)) {
      reportWarning(ErrorCode::WARN_SHADOWING_VARIABLE,
                    "Variable '" + name + "' shadows a declaration in an outer scope",
                    loc);
      break;
    }
  }
  scopes_.back()[name] = {loc, false, isParam};
}

SemanticAnalyzer::VarInfo *SemanticAnalyzer::lookupVar(const std::string &name) {
  for (auto it = scopes_.rbegin(); it != scopes_.rend(); ++it) {
    auto found = it->find(name);
    if (found != it->end()) return &found->second;
  }
  return nullptr;
}

void SemanticAnalyzer::markVarUsed(const std::string &name) {
  for (auto it = scopes_.rbegin(); it != scopes_.rend(); ++it) {
    auto found = it->find(name);
    if (found != it->end()) {
      found->second.used = true;
      return;
    }
  }
}

void SemanticAnalyzer::reportError(ErrorCode code, const std::string &msg,
                                   const SourceLocation &loc) {
  diag_.reportError(code, msg, loc);
}

void SemanticAnalyzer::reportWarning(ErrorCode code, const std::string &msg,
                                     const SourceLocation &loc) {
  if (!warnings_.isEnabled(code)) return;
  if (warnings_.treatAsErrors()) {
    diag_.reportError(code, msg, loc);
  } else {
    diag_.reportWarning(code, msg, loc);
  }
}

// ── Block analysis ────────────────────────────────────────────────────────────

void SemanticAnalyzer::analyzeBlock(
    const std::vector<std::unique_ptr<Stmt>> &stmts) {
  bool wasUnreachable = unreachable_;
  unreachable_ = false;

  for (const auto &stmt : stmts) {
    if (unreachable_) {
      SourceLocation loc("", stmt->line, stmt->column);
      reportWarning(ErrorCode::WARN_UNREACHABLE_CODE,
                    "Unreachable statement", loc);
      break;
    }
    stmt->accept(*this);
  }

  unreachable_ = wasUnreachable || unreachable_;
}

// ── Expression visitors ───────────────────────────────────────────────────────

void SemanticAnalyzer::visitLiteralExpr(LiteralExpr &) {}

void SemanticAnalyzer::visitVarExpr(VarExpr &expr) {
  // Check if it's a known function name first (functions aren't in var scopes)
  if (functions_.count(expr.name)) return;

  SourceLocation loc("", expr.line, expr.column);
  if (!lookupVar(expr.name)) {
    reportError(ErrorCode::SEM_UNDEFINED_VARIABLE,
                "Undefined variable '" + expr.name + "'", loc);
    return;
  }
  markVarUsed(expr.name);
}

void SemanticAnalyzer::visitAssignExpr(AssignExpr &expr) {
  SourceLocation loc("", expr.line, expr.column);
  if (!lookupVar(expr.name)) {
    reportError(ErrorCode::SEM_UNDEFINED_VARIABLE,
                "Assignment to undefined variable '" + expr.name + "'", loc);
  } else {
    markVarUsed(expr.name);
  }
  expr.value->accept(*this);
}

void SemanticAnalyzer::visitBinaryExpr(BinaryExpr &expr) {
  expr.left->accept(*this);
  expr.right->accept(*this);
}

void SemanticAnalyzer::visitUnaryExpr(UnaryExpr &expr) {
  expr.operand->accept(*this);
}

void SemanticAnalyzer::visitLogicalExpr(LogicalExpr &expr) {
  expr.left->accept(*this);
  expr.right->accept(*this);
}

void SemanticAnalyzer::visitIndexExpr(IndexExpr &expr) {
  expr.array->accept(*this);
  expr.index->accept(*this);
}

void SemanticAnalyzer::visitFieldAccessExpr(FieldAccessExpr &expr) {
  expr.object->accept(*this);
}

void SemanticAnalyzer::visitCallExpr(CallExpr &expr) {
  auto *varExpr = dynamic_cast<VarExpr *>(expr.callee.get());
  if (!varExpr) {
    SourceLocation loc("", expr.line, expr.column);
    reportError(ErrorCode::SEM_INVALID_CALL_TARGET,
                "Callee must be a function name", loc);
    return;
  }

  SourceLocation loc("", varExpr->line, varExpr->column);

  // print is a built-in; validate arg count but skip function table lookup
  if (varExpr->name == "print") {
    if (expr.args.size() != 1) {
      reportError(ErrorCode::SEM_INVALID_ARGUMENT_COUNT,
                  "print() takes exactly 1 argument", loc);
    }
    for (auto &arg : expr.args) arg->accept(*this);
    return;
  }

  auto it = functions_.find(varExpr->name);
  if (it == functions_.end()) {
    reportError(ErrorCode::SEM_UNDEFINED_FUNCTION,
                "Undefined function '" + varExpr->name + "'", loc);
  } else if (it->second != expr.args.size()) {
    reportError(ErrorCode::SEM_INVALID_ARGUMENT_COUNT,
                "Function '" + varExpr->name + "' expects " +
                    std::to_string(it->second) + " arguments, got " +
                    std::to_string(expr.args.size()),
                loc);
  }
  for (auto &arg : expr.args) arg->accept(*this);
}

void SemanticAnalyzer::visitArrayExpr(ArrayExpr &expr) {
  for (auto &e : expr.elements) e->accept(*this);
}

void SemanticAnalyzer::visitObjectExpr(ObjectExpr &expr) {
  for (auto &[key, val] : expr.pairs) val->accept(*this);
}

// ── Statement visitors ────────────────────────────────────────────────────────

void SemanticAnalyzer::visitExprStmt(ExprStmt &stmt) {
  stmt.expr->accept(*this);
}

void SemanticAnalyzer::visitLetStmt(LetStmt &stmt) {
  stmt.initializer->accept(*this);
  SourceLocation loc("", stmt.line, stmt.column);
  declareVar(stmt.name, loc);
}

void SemanticAnalyzer::visitFuncStmt(FuncStmt &stmt) {
  // Register in function table (may already be there from pre-pass)
  functions_[stmt.name] = stmt.params.size();

  bool savedInFunction = inFunction_;
  bool savedInLoop = inLoop_;
  bool savedUnreachable = unreachable_;

  inFunction_ = true;
  inLoop_ = false;
  unreachable_ = false;

  enterScope();
  SourceLocation funcLoc("", stmt.line, stmt.column);
  for (const auto &param : stmt.params) {
    declareVar(param, funcLoc, /*isParam=*/true);
  }
  analyzeBlock(stmt.body);
  exitScope();

  inFunction_ = savedInFunction;
  inLoop_ = savedInLoop;
  unreachable_ = savedUnreachable;
}

void SemanticAnalyzer::visitIfStmt(IfStmt &stmt) {
  stmt.condition->accept(*this);

  bool savedUnreachable = unreachable_;
  unreachable_ = false;
  enterScope();
  analyzeBlock(stmt.thenBranch);
  exitScope();
  bool thenTerminates = unreachable_;

  unreachable_ = false;
  if (!stmt.elseBranch.empty()) {
    enterScope();
    analyzeBlock(stmt.elseBranch);
    exitScope();
  }
  bool elseTerminates = unreachable_;

  unreachable_ = savedUnreachable || (thenTerminates && elseTerminates);
}

void SemanticAnalyzer::visitForStmt(ForStmt &stmt) {
  stmt.rangeStart->accept(*this);
  stmt.rangeEnd->accept(*this);

  bool savedInLoop = inLoop_;
  bool savedUnreachable = unreachable_;
  inLoop_ = true;
  unreachable_ = false;

  enterScope();
  SourceLocation loc("", stmt.line, stmt.column);
  declareVar(stmt.var, loc);
  analyzeBlock(stmt.body);
  exitScope();

  inLoop_ = savedInLoop;
  unreachable_ = savedUnreachable;
}

void SemanticAnalyzer::visitWhileStmt(WhileStmt &stmt) {
  stmt.condition->accept(*this);

  bool savedInLoop = inLoop_;
  bool savedUnreachable = unreachable_;
  inLoop_ = true;
  unreachable_ = false;

  enterScope();
  analyzeBlock(stmt.body);
  exitScope();

  inLoop_ = savedInLoop;
  unreachable_ = savedUnreachable;
}

void SemanticAnalyzer::visitReturnStmt(ReturnStmt &stmt) {
  SourceLocation loc("", stmt.line, stmt.column);
  if (!inFunction_) {
    reportError(ErrorCode::SEM_RETURN_OUTSIDE_FUNCTION,
                "'return' outside function", loc);
  }
  if (stmt.value) stmt.value->accept(*this);
  unreachable_ = true;
}

void SemanticAnalyzer::visitBreakStmt(BreakStmt &stmt) {
  SourceLocation loc("", stmt.line, stmt.column);
  if (!inLoop_) {
    reportError(ErrorCode::SEM_BREAK_OUTSIDE_LOOP, "'break' outside loop", loc);
  }
  unreachable_ = true;
}

void SemanticAnalyzer::visitContinueStmt(ContinueStmt &stmt) {
  SourceLocation loc("", stmt.line, stmt.column);
  if (!inLoop_) {
    reportError(ErrorCode::SEM_CONTINUE_OUTSIDE_LOOP,
                "'continue' outside loop", loc);
  }
  unreachable_ = true;
}

void SemanticAnalyzer::visitBlockStmt(BlockStmt &stmt) {
  enterScope();
  analyzeBlock(stmt.body);
  exitScope();
}

} // namespace meadows
