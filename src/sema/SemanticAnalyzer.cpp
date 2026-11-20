#include "SemanticAnalyzer.h"

#include "Builtins.h"

namespace meadows {

namespace {

std::string arguments(size_t count) {
  return std::to_string(count) + (count == 1 ? " argument" : " arguments");
}

uint32_t nameLength(const std::string& name) {
  return static_cast<uint32_t>(name.size());
}

} // namespace

void SemanticAnalyzer::analyze(StmtList& statements) {
  scopes_.clear();
  functions_.clear();
  topLevelVariables_.clear();
  inFunction_ = false;
  inLoop_ = false;
  unreachable_ = false;

  for (const StmtPtr& stmt : statements) {
    if (auto* func = dynamic_cast<FuncStmt*>(stmt.get())) {
      if (findBuiltin(func->name)) {
        diagnostics_.report(ErrorCode::SEM_NAME_CONFLICT,
                            "'" + func->name + "' is a built-in function",
                            func->nameLocation, nameLength(func->name));
      } else if (!functions_.emplace(func->name, func->params.size()).second) {
        diagnostics_.report(ErrorCode::SEM_REDEFINED_FUNCTION,
                            "Function '" + func->name + "' is already defined",
                            func->nameLocation, nameLength(func->name));
      }
    } else if (auto* let = dynamic_cast<LetStmt*>(stmt.get())) {
      topLevelVariables_.insert(let->name);
    }
  }

  enterScope();
  analyzeBody(statements);
  exitScope();
}

void SemanticAnalyzer::exitScope() {
  for (const Variable& variable : scopes_.back()) {
    if (!variable.used && !variable.isParam && variable.name[0] != '_') {
      diagnostics_.report(ErrorCode::WARN_UNUSED_VARIABLE,
                          "Variable '" + variable.name +
                              "' is declared but never used",
                          variable.location, nameLength(variable.name),
                          "Prefix the name with '_' to silence this warning");
    }
  }
  scopes_.pop_back();
}

bool SemanticAnalyzer::conflictsWithCallable(const std::string& name,
                                             Location location) {
  const char* kind = findBuiltin(name)        ? "a built-in function"
                     : functions_.count(name) ? "a function"
                                              : nullptr;
  if (!kind) {
    return false;
  }
  diagnostics_.report(ErrorCode::SEM_NAME_CONFLICT,
                      "'" + name + "' is already defined as " + kind, location,
                      nameLength(name));
  return true;
}

void SemanticAnalyzer::declare(const std::string& name, Location location,
                               bool isParam) {
  if (conflictsWithCallable(name, location)) {
    return;
  }
  for (const Variable& existing : scopes_.back()) {
    if (existing.name == name) {
      diagnostics_.report(ErrorCode::SEM_REDEFINED_VARIABLE,
                          "Variable '" + name +
                              "' is already declared in this scope",
                          location, nameLength(name));
      return;
    }
  }
  if (shadowsOuterScope(name)) {
    diagnostics_.report(ErrorCode::WARN_SHADOWING_VARIABLE,
                        "Variable '" + name +
                            "' shadows a declaration in an outer scope",
                        location, nameLength(name));
  }
  scopes_.back().push_back({name, location, false, isParam});
}

bool SemanticAnalyzer::shadowsOuterScope(const std::string& name) const {
  for (size_t i = 0; i + 1 < scopes_.size(); ++i) {
    for (const Variable& outer : scopes_[i]) {
      if (outer.name == name) {
        return true;
      }
    }
  }
  return false;
}

SemanticAnalyzer::Variable* SemanticAnalyzer::lookup(const std::string& name) {
  for (auto scope = scopes_.rbegin(); scope != scopes_.rend(); ++scope) {
    for (Variable& variable : *scope) {
      if (variable.name == name) {
        return &variable;
      }
    }
  }
  return nullptr;
}

void SemanticAnalyzer::reportUndefined(ErrorCode code, const std::string& what,
                                       const std::string& name,
                                       const Expr& expr) {
  std::string help;
  if (inFunction_ && topLevelVariables_.count(name)) {
    help = "Functions cannot access top-level variables; pass '" + name +
           "' as a parameter";
  }
  diagnostics_.report(code, "Undefined " + what + " '" + name + "'",
                      expr.location, nameLength(name), help);
}

void SemanticAnalyzer::analyzeBody(StmtList& body) {
  bool outerUnreachable = unreachable_;
  unreachable_ = false;
  for (StmtPtr& stmt : body) {
    if (unreachable_) {
      diagnostics_.report(ErrorCode::WARN_UNREACHABLE_CODE,
                          "Unreachable statement", stmt->location);
      break;
    }
    stmt->accept(*this);
  }
  unreachable_ = outerUnreachable || unreachable_;
}

void SemanticAnalyzer::visitLiteralExpr(LiteralExpr&) {}

void SemanticAnalyzer::visitVarExpr(VarExpr& expr) {
  if (Variable* variable = lookup(expr.name)) {
    variable->used = true;
  } else if (findBuiltin(expr.name)) {
    diagnostics_.report(ErrorCode::SEM_BUILTIN_AS_VALUE,
                        "Built-in function '" + expr.name +
                            "' can only be called, not used as a value",
                        expr.location, nameLength(expr.name));
  } else if (!functions_.count(expr.name)) {
    reportUndefined(ErrorCode::SEM_UNDEFINED_VARIABLE, "variable", expr.name,
                    expr);
  }
}

void SemanticAnalyzer::visitAssignExpr(AssignExpr& expr) {
  if (auto* target = dynamic_cast<VarExpr*>(expr.target.get())) {
    if (Variable* variable = lookup(target->name)) {
      variable->used = true;
    } else if (findBuiltin(target->name) || functions_.count(target->name)) {
      diagnostics_.report(ErrorCode::SEM_ASSIGN_TO_NON_VARIABLE,
                          "Cannot assign to function '" + target->name + "'",
                          target->location, nameLength(target->name));
    } else {
      reportUndefined(ErrorCode::SEM_UNDEFINED_VARIABLE, "variable",
                      target->name, *target);
    }
  } else {
    expr.target->accept(*this);
  }
  expr.value->accept(*this);
}

void SemanticAnalyzer::visitBinaryExpr(BinaryExpr& expr) {
  expr.left->accept(*this);
  expr.right->accept(*this);
}

void SemanticAnalyzer::visitUnaryExpr(UnaryExpr& expr) {
  expr.operand->accept(*this);
}

void SemanticAnalyzer::visitLogicalExpr(LogicalExpr& expr) {
  expr.left->accept(*this);
  expr.right->accept(*this);
}

void SemanticAnalyzer::visitIndexExpr(IndexExpr& expr) {
  expr.object->accept(*this);
  expr.index->accept(*this);
}

void SemanticAnalyzer::visitFieldExpr(FieldExpr& expr) {
  expr.object->accept(*this);
  if (auto* literal = dynamic_cast<ObjectExpr*>(expr.object.get())) {
    for (const ObjectField& field : literal->fields) {
      if (field.name == expr.name) {
        return;
      }
    }
    diagnostics_.report(ErrorCode::SEM_UNKNOWN_FIELD,
                        "Object has no field '" + expr.name + "'",
                        expr.location);
  }
}

void SemanticAnalyzer::visitCallExpr(CallExpr& expr) {
  auto* callee = dynamic_cast<VarExpr*>(expr.callee.get());
  if (!callee) {
    expr.callee->accept(*this);
  } else if (Variable* variable = lookup(callee->name)) {
    variable->used = true;
  } else if (const BuiltinInfo* builtin = findBuiltin(callee->name)) {
    if (expr.args.size() != builtin->arity) {
      diagnostics_.report(ErrorCode::SEM_INVALID_ARGUMENT_COUNT,
                          callee->name + "() takes " +
                              arguments(builtin->arity) + ", got " +
                              std::to_string(expr.args.size()),
                          callee->location, nameLength(callee->name));
    }
  } else if (auto function = functions_.find(callee->name);
             function != functions_.end()) {
    if (expr.args.size() != function->second) {
      diagnostics_.report(ErrorCode::SEM_INVALID_ARGUMENT_COUNT,
                          "Function '" + callee->name + "' takes " +
                              arguments(function->second) + ", got " +
                              std::to_string(expr.args.size()),
                          callee->location, nameLength(callee->name));
    }
  } else {
    reportUndefined(ErrorCode::SEM_UNDEFINED_FUNCTION, "function", callee->name,
                    *callee);
  }
  for (ExprPtr& arg : expr.args) {
    arg->accept(*this);
  }
}

void SemanticAnalyzer::visitArrayExpr(ArrayExpr& expr) {
  for (ExprPtr& element : expr.elements) {
    element->accept(*this);
  }
}

void SemanticAnalyzer::visitObjectExpr(ObjectExpr& expr) {
  std::set<std::string> seen;
  for (ObjectField& field : expr.fields) {
    if (!seen.insert(field.name).second) {
      diagnostics_.report(ErrorCode::SEM_DUPLICATE_FIELD,
                          "Field '" + field.name +
                              "' is defined more than once in this object",
                          field.location, nameLength(field.name));
    }
    field.value->accept(*this);
  }
}

void SemanticAnalyzer::visitExprStmt(ExprStmt& stmt) {
  stmt.expr->accept(*this);
}

void SemanticAnalyzer::visitLetStmt(LetStmt& stmt) {
  stmt.initializer->accept(*this);
  declare(stmt.name, stmt.nameLocation, false);
}

void SemanticAnalyzer::visitFuncStmt(FuncStmt& stmt) {
  std::vector<Scope> outer = std::move(scopes_);
  scopes_.clear();
  bool outerInFunction = inFunction_;
  bool outerInLoop = inLoop_;
  bool outerUnreachable = unreachable_;
  inFunction_ = true;
  inLoop_ = false;

  enterScope();
  for (const Param& param : stmt.params) {
    declare(param.name, param.location, true);
  }
  analyzeBody(stmt.body);
  exitScope();

  scopes_ = std::move(outer);
  inFunction_ = outerInFunction;
  inLoop_ = outerInLoop;
  unreachable_ = outerUnreachable;
}

void SemanticAnalyzer::visitIfStmt(IfStmt& stmt) {
  stmt.condition->accept(*this);
  bool outerUnreachable = unreachable_;

  unreachable_ = false;
  enterScope();
  analyzeBody(stmt.thenBody);
  exitScope();
  bool thenTerminates = unreachable_;

  unreachable_ = false;
  enterScope();
  analyzeBody(stmt.elseBody);
  exitScope();
  bool elseTerminates = !stmt.elseBody.empty() && unreachable_;

  unreachable_ = outerUnreachable || (thenTerminates && elseTerminates);
}

void SemanticAnalyzer::visitForStmt(ForStmt& stmt) {
  stmt.start->accept(*this);
  stmt.end->accept(*this);

  bool outerInLoop = inLoop_;
  bool outerUnreachable = unreachable_;
  inLoop_ = true;
  unreachable_ = false;
  enterScope();
  declare(stmt.var, stmt.varLocation, false);
  analyzeBody(stmt.body);
  exitScope();
  inLoop_ = outerInLoop;
  unreachable_ = outerUnreachable;
}

void SemanticAnalyzer::visitWhileStmt(WhileStmt& stmt) {
  stmt.condition->accept(*this);

  bool outerInLoop = inLoop_;
  bool outerUnreachable = unreachable_;
  inLoop_ = true;
  unreachable_ = false;
  enterScope();
  analyzeBody(stmt.body);
  exitScope();
  inLoop_ = outerInLoop;
  unreachable_ = outerUnreachable;
}

void SemanticAnalyzer::visitReturnStmt(ReturnStmt& stmt) {
  if (!inFunction_) {
    diagnostics_.report(ErrorCode::SEM_RETURN_OUTSIDE_FUNCTION,
                        "'return' outside of a function", stmt.location, 6);
  }
  if (stmt.value) {
    stmt.value->accept(*this);
  }
  unreachable_ = true;
}

void SemanticAnalyzer::visitBreakStmt(BreakStmt& stmt) {
  if (!inLoop_) {
    diagnostics_.report(ErrorCode::SEM_BREAK_OUTSIDE_LOOP,
                        "'break' outside of a loop", stmt.location, 5);
  }
  unreachable_ = true;
}

void SemanticAnalyzer::visitContinueStmt(ContinueStmt& stmt) {
  if (!inLoop_) {
    diagnostics_.report(ErrorCode::SEM_CONTINUE_OUTSIDE_LOOP,
                        "'continue' outside of a loop", stmt.location, 8);
  }
  unreachable_ = true;
}

void SemanticAnalyzer::visitBlockStmt(BlockStmt& stmt) {
  enterScope();
  analyzeBody(stmt.body);
  exitScope();
}

} // namespace meadows
