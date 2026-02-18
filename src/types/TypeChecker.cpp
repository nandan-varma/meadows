#include "TypeChecker.h"
#include "../utils/ErrorCodes.h"
#include <set>
#include <sstream>

namespace meadows {
namespace types {

TypeChecker::TypeChecker() {
  env_ = std::make_shared<ScopedEnv>();
  initBuiltins();
  typeVarCounter_ = 0;
}

void TypeChecker::initBuiltins() {
  i32_ = std::make_shared<PrimitiveType>(PrimitiveKind::I32);
  i64_ = std::make_shared<PrimitiveType>(PrimitiveKind::I64);
  f32_ = std::make_shared<PrimitiveType>(PrimitiveKind::F32);
  f64_ = std::make_shared<PrimitiveType>(PrimitiveKind::F64);
  bool_ = std::make_shared<PrimitiveType>(PrimitiveKind::BOOL);
  string_ = std::make_shared<PrimitiveType>(PrimitiveKind::STRING);
  unit_ = std::make_shared<PrimitiveType>(PrimitiveKind::UNIT);

  auto fnPrintI32 =
      FunctionTypeBuilder().withReturnType(unit_).withParam(i32_).build();
  bind("print_i32", fnPrintI32);

  auto fnPrintString =
      FunctionTypeBuilder().withReturnType(unit_).withParam(string_).build();
  bind("print_string", fnPrintString);
}

std::shared_ptr<Type> TypeChecker::freshTypeVar() {
  auto var = std::make_shared<TypeVariable>("_");
  var->id = typeVarCounter_++;
  return var;
}

std::shared_ptr<Type> TypeChecker::getPrimitiveType(const std::string &name) {
  if (name == "i32")
    return i32_;
  if (name == "i64")
    return i64_;
  if (name == "f32")
    return f32_;
  if (name == "f64")
    return f64_;
  if (name == "bool")
    return bool_;
  if (name == "string")
    return string_;
  if (name == "()")
    return unit_;
  return nullptr;
}

void TypeChecker::reset() {
  env_ = std::make_shared<ScopedEnv>();
  constraints_.clear();
  subst_ = Substitution();
  exprTypes_.clear();
  stmtTypes_.clear();
  errors_.clear();
  functionReturnTypes_.clear();
  typeVarCounter_ = 0;
  initBuiltins();
}

bool TypeChecker::check(const std::vector<std::unique_ptr<Stmt>> &statements) {
  try {
    reset();

    for (const auto &stmt : statements) {
      inferStmt(stmt.get());
    }

    bool unified = unify();
    return unified && errors_.empty();
  } catch (const std::exception &e) {
    errors_.push_back(std::string("Type checker error: ") + e.what());
    return false;
  } catch (...) {
    errors_.push_back("Type checker error: unknown exception");
    return false;
  }
}

std::shared_ptr<Type> TypeChecker::getInferredType(const Expr *expr) const {
  auto it = exprTypes_.find(expr);
  if (it != exprTypes_.end()) {
    return subst_.apply(it->second);
  }
  return nullptr;
}

void TypeChecker::inferStmt(Stmt *stmt) { stmt->accept(*this); }

void TypeChecker::checkBlock(
    const std::vector<std::unique_ptr<Stmt>> &statements) {
  for (const auto &stmt : statements) {
    inferStmt(stmt.get());
  }
}

void TypeChecker::addConstraint(std::shared_ptr<Type> t1,
                                std::shared_ptr<Type> t2,
                                const std::string &context) {
  Constraint c;
  c.t1 = t1;
  c.t2 = t2;
  c.context = context;
  c.line = currentLine_;
  c.column = currentColumn_;
  constraints_.push_back(c);
}

void TypeChecker::reportError(const std::string &message) {
  errors_.push_back("Line " + std::to_string(currentLine_) + ", Column " +
                    std::to_string(currentColumn_) + ": " + message);
}

void TypeChecker::reportTypeMismatch(std::shared_ptr<Type> expected,
                                     std::shared_ptr<Type> actual,
                                     const std::string &context) {
  std::ostringstream oss;
  oss << context << " - expected " << expected->toString() << " but found "
      << actual->toString();
  reportError(oss.str());
}

void TypeChecker::bind(const std::string &name, std::shared_ptr<Type> type) {
  TypeScheme scheme;
  scheme.typeParams = {};
  scheme.type = type;
  env_->bindings[name] = scheme;
}

std::shared_ptr<Type> TypeChecker::lookup(const std::string &name) const {
  auto it = env_->bindings.find(name);
  if (it != env_->bindings.end()) {
    return it->second.type;
  }
  if (env_->parent) {
    auto parentEnv = env_->parent;
    while (parentEnv) {
      auto pit = parentEnv->bindings.find(name);
      if (pit != parentEnv->bindings.end()) {
        return pit->second.type;
      }
      parentEnv = parentEnv->parent;
    }
  }
  return nullptr;
}

std::shared_ptr<TypeChecker::ScopedEnv> TypeChecker::extendEnv() const {
  auto newEnv = std::make_shared<ScopedEnv>();
  newEnv->parent = env_;
  return newEnv;
}

std::shared_ptr<Type> TypeChecker::inferExpr(Expr *expr) {
  if (!expr) {
    return freshTypeVar();
  }
  expr->accept(*this);
  auto it = exprTypes_.find(expr);
  if (it != exprTypes_.end()) {
    return subst_.apply(it->second);
  }
  return freshTypeVar();
}

void TypeChecker::visitLiteralExpr(LiteralExpr &expr) {
  if (isdigit(expr.value[0])) {
    exprTypes_[&expr] = i32_;
  } else if (expr.value[0] == '"') {
    exprTypes_[&expr] = string_;
  } else if (expr.value == "true" || expr.value == "false") {
    exprTypes_[&expr] = bool_;
  } else {
    exprTypes_[&expr] = unit_;
  }
}

void TypeChecker::visitVarExpr(VarExpr &expr) {
  auto type = lookup(expr.name);
  if (type) {
    exprTypes_[&expr] = type;
  } else {
    reportError("Undefined variable: " + expr.name);
    exprTypes_[&expr] = i32_;
  }
}

void TypeChecker::visitAssignExpr(AssignExpr &expr) {
  auto varType = lookup(expr.name);
  auto valueType = inferExpr(expr.value.get());

  if (varType && valueType) {
    addConstraint(varType, valueType, "Assignment type mismatch");
  }

  exprTypes_[&expr] = unit_;
}

void TypeChecker::visitBinaryExpr(BinaryExpr &expr) {
  auto leftType = inferExpr(expr.left.get());
  auto rightType = inferExpr(expr.right.get());

  if (!leftType || !rightType) {
    exprTypes_[&expr] = freshTypeVar();
    return;
  }

  exprTypes_[&expr] = leftType;

  if (expr.op == "+" || expr.op == "-" || expr.op == "*" || expr.op == "/") {
    addConstraint(leftType, rightType, "Arithmetic operand type mismatch");
  } else if (expr.op == "==" || expr.op == "!=") {
    addConstraint(leftType, rightType, "Comparison operand type mismatch");
    exprTypes_[&expr] = bool_;
  } else if (expr.op == "<" || expr.op == ">" || expr.op == "<=" ||
             expr.op == ">=") {
    addConstraint(leftType, rightType, "Comparison operand type mismatch");
    exprTypes_[&expr] = bool_;
  }
}

void TypeChecker::visitUnaryExpr(UnaryExpr &expr) {
  auto operandType = inferExpr(expr.operand.get());
  exprTypes_[&expr] = operandType;

  if (expr.op == "!") {
    addConstraint(operandType, bool_, "Logical NOT requires boolean");
  }
}

void TypeChecker::visitTryExpr(TryExpr &expr) {
  auto innerType = inferExpr(expr.expr.get());
  exprTypes_[&expr] = freshTypeVar();
}

void TypeChecker::visitLogicalExpr(LogicalExpr &expr) {
  auto leftType = inferExpr(expr.left.get());
  auto rightType = inferExpr(expr.right.get());

  addConstraint(leftType, bool_, "Logical operator requires boolean operands");
  addConstraint(rightType, bool_, "Logical operator requires boolean operands");

  exprTypes_[&expr] = bool_;
}

void TypeChecker::visitIndexExpr(IndexExpr &expr) {
  auto arrayType = inferExpr(expr.array.get());
  auto indexType = inferExpr(expr.index.get());

  addConstraint(indexType, i32_, "Array index must be integer");

  auto elemType = freshTypeVar();
  auto expectedArray = ArrayTypeBuilder(elemType).build();
  addConstraint(arrayType, expectedArray, "Index operation on non-array");

  exprTypes_[&expr] = elemType;
}

void TypeChecker::visitFieldAccessExpr(FieldAccessExpr &expr) {
  inferExpr(expr.object.get());
  exprTypes_[&expr] = freshTypeVar();
}

void TypeChecker::visitCallExpr(CallExpr &expr) {
  std::string funcName;
  if (auto *varExpr = dynamic_cast<VarExpr *>(expr.callee.get())) {
    funcName = varExpr->name;
  }

  auto calleeType = funcName.empty() ? nullptr : lookup(funcName);

  if (!calleeType) {
    if (!funcName.empty()) {
      reportError("Undefined function: " + funcName);
    }
    exprTypes_[&expr] = i32_;
    return;
  }

  auto fnType = std::dynamic_pointer_cast<FunctionType>(calleeType);
  if (fnType) {
    if (fnType->paramTypes.size() != expr.args.size()) {
      std::ostringstream oss;
      oss << "Function expects " << fnType->paramTypes.size()
          << " arguments but got " << expr.args.size();
      reportError(oss.str());
    }

    size_t count = std::min(fnType->paramTypes.size(), expr.args.size());
    for (size_t i = 0; i < count; ++i) {
      auto argType = inferExpr(expr.args[i].get());
      auto paramType = fnType->getParamType(i);
      if (paramType && argType) {
        addConstraint(paramType, argType, "Function argument type mismatch");
      }
    }

    auto retType = fnType->getReturnType();
    exprTypes_[&expr] = retType ? retType : freshTypeVar();
  } else {
    exprTypes_[&expr] = freshTypeVar();
  }
}

void TypeChecker::visitArrayExpr(ArrayExpr &expr) {
  if (expr.elements.empty()) {
    exprTypes_[&expr] = ArrayTypeBuilder(freshTypeVar()).build();
    return;
  }

  auto firstType = inferExpr(expr.elements[0].get());
  for (size_t i = 1; i < expr.elements.size(); ++i) {
    auto elemType = inferExpr(expr.elements[i].get());
    addConstraint(firstType, elemType, "Array element type mismatch");
  }

  exprTypes_[&expr] = ArrayTypeBuilder(firstType).build();
}

void TypeChecker::visitObjectExpr(ObjectExpr &expr) {
  exprTypes_[&expr] = freshTypeVar();
}

void TypeChecker::visitMatchExpr(MatchExpr &expr) {
  auto scrutineeType = inferExpr(expr.scrutinee.get());

  bool hasWildcard = false;
  std::set<std::string> coveredEnumVariants;

  std::shared_ptr<Type> resultType = nullptr;
  for (auto &arm : expr.arms) {
    auto armType = inferExpr(arm.body.get());

    if (arm.pattern->kind == PatternKind::WILDCARD) {
      hasWildcard = true;
    }

    if (arm.pattern->kind == PatternKind::ENUM &&
        !arm.pattern->typeName.empty()) {
      coveredEnumVariants.insert(arm.pattern->name);
    }

    if (!resultType) {
      resultType = armType;
    }
  }

  if (!resultType) {
    resultType = unit_;
  }

  if (!hasWildcard) {
    if (auto *enumType = dynamic_cast<EnumType *>(scrutineeType.get())) {
      const auto &enumName = enumType->name;
      if (definedEnums_.find(enumName) != definedEnums_.end()) {
        const auto &expectedVariants = definedEnums_[enumName];
        std::set<std::string> expectedSet(expectedVariants.begin(),
                                          expectedVariants.end());

        std::vector<std::string> missing;
        std::set_difference(
            expectedSet.begin(), expectedSet.end(), coveredEnumVariants.begin(),
            coveredEnumVariants.end(), std::back_inserter(missing));

        if (!missing.empty()) {
          std::ostringstream oss;
          oss << "non-exhaustive match: missing variants: ";
          for (size_t i = 0; i < missing.size(); ++i) {
            if (i > 0)
              oss << ", ";
            oss << missing[i];
          }
          errors_.push_back(oss.str());
        }
      }
    } else if (scrutineeType == i32_ || scrutineeType == i64_) {
      errors_.push_back(
          "non-exhaustive match on integer type: add wildcard arm '_'");
    } else if (scrutineeType == bool_) {
      if (coveredEnumVariants.empty()) {
        errors_.push_back("non-exhaustive match on bool: add wildcard arm '_'");
      }
    }
  }

  exprTypes_[&expr] = resultType;
}

void TypeChecker::visitEnumVariantExpr(EnumVariantExpr &expr) {
  auto enumType = std::make_shared<EnumType>(expr.enumName);
  std::vector<std::unique_ptr<Type>> argTypes;
  for (size_t i = 0; i < expr.args.size(); ++i) {
    auto argType = inferExpr(expr.args[i].get());
    argTypes.push_back(argType->clone());
  }
  enumType->addVariant(expr.variantName, std::move(argTypes));
  exprTypes_[&expr] = enumType;
}

void TypeChecker::visitExprStmt(ExprStmt &stmt) {
  inferExpr(stmt.expr.get());
  stmtTypes_[&stmt] = unit_;
}

void TypeChecker::visitLetStmt(LetStmt &stmt) {
  auto initType = inferExpr(stmt.initializer.get());

  if (!stmt.typeAnnotation.empty()) {
    auto annotatedType = getPrimitiveType(stmt.typeAnnotation);
    if (annotatedType) {
      addConstraint(annotatedType, initType,
                    "Variable type annotation mismatch");
      bind(stmt.name, annotatedType);
    } else {
      bind(stmt.name, initType);
    }
  } else {
    bind(stmt.name, initType);
  }

  stmtTypes_[&stmt] = unit_;
}

void TypeChecker::visitFuncStmt(FuncStmt &stmt) {
  std::vector<std::shared_ptr<Type>> paramTypes;
  for (const auto &param : stmt.params) {
    if (!param.typeAnnotation.empty()) {
      auto pt = getPrimitiveType(param.typeAnnotation);
      paramTypes.push_back(pt ? pt : freshTypeVar());
    } else {
      paramTypes.push_back(freshTypeVar());
    }
  }

  auto returnType = stmt.returnTypeAnnotation.empty()
                        ? freshTypeVar()
                        : getPrimitiveType(stmt.returnTypeAnnotation);

  auto fnTypeBuilder = FunctionTypeBuilder().withReturnType(returnType);
  for (const auto &paramType : paramTypes) {
    fnTypeBuilder.withParam(paramType);
  }
  auto fnType = fnTypeBuilder.build();
  bind(stmt.name, fnType);

  auto prevEnv = env_;
  env_ = extendEnv();

  for (size_t i = 0; i < stmt.params.size(); ++i) {
    env_->bindings[stmt.params[i].name] = {std::vector<std::string>{},
                                           paramTypes[i]};
  }

  functionReturnTypes_.push_back(returnType);
  checkBlock(stmt.body);
  functionReturnTypes_.pop_back();

  env_ = prevEnv;

  stmtTypes_[&stmt] = unit_;
}

void TypeChecker::visitIfStmt(IfStmt &stmt) {
  auto condType = inferExpr(stmt.condition.get());

  // Allow numeric types as conditions (coerce to bool)
  auto prim = std::dynamic_pointer_cast<PrimitiveType>(condType);
  if (prim &&
      (prim->kind == PrimitiveKind::I32 || prim->kind == PrimitiveKind::I64 ||
       prim->kind == PrimitiveKind::F32 || prim->kind == PrimitiveKind::F64)) {
    // Allow numeric conditions - they coerce to bool
  } else {
    addConstraint(condType, bool_, "If condition must be boolean");
  }

  checkBlock(stmt.thenBranch);
  checkBlock(stmt.elseBranch);

  stmtTypes_[&stmt] = unit_;
}

void TypeChecker::visitForStmt(ForStmt &stmt) {
  auto startType = inferExpr(stmt.rangeStart.get());
  auto endType = inferExpr(stmt.rangeEnd.get());

  addConstraint(startType, i32_, "Range start must be integer");
  addConstraint(endType, i32_, "Range end must be integer");

  auto prevEnv = env_;
  env_ = extendEnv();
  env_->bindings[stmt.var] = {std::vector<std::string>{}, i32_};

  checkBlock(stmt.body);

  env_ = prevEnv;

  stmtTypes_[&stmt] = unit_;
}

void TypeChecker::visitWhileStmt(WhileStmt &stmt) {
  auto condType = inferExpr(stmt.condition.get());
  addConstraint(condType, bool_, "While condition must be boolean");

  checkBlock(stmt.body);

  stmtTypes_[&stmt] = unit_;
}

void TypeChecker::visitReturnStmt(ReturnStmt &stmt) {
  auto retType = inferExpr(stmt.value.get());

  if (!functionReturnTypes_.empty()) {
    auto expected = functionReturnTypes_.back();
    addConstraint(expected, retType, "Return type mismatch");
  }

  stmtTypes_[&stmt] = unit_;
}

void TypeChecker::visitBreakStmt(BreakStmt &stmt) { stmtTypes_[&stmt] = unit_; }

void TypeChecker::visitContinueStmt(ContinueStmt &stmt) {
  stmtTypes_[&stmt] = unit_;
}

void TypeChecker::visitBlockStmt(BlockStmt &stmt) {
  auto prevEnv = env_;
  env_ = extendEnv();
  checkBlock(stmt.body);
  env_ = prevEnv;

  stmtTypes_[&stmt] = unit_;
}

void TypeChecker::visitPrintStmt(PrintStmt &stmt) {
  inferExpr(stmt.expr.get());
  stmtTypes_[&stmt] = unit_;
}

void TypeChecker::visitTypeDefStmt(TypeDefStmt &stmt) {
  if (stmt.isEnum) {
    std::vector<std::string> variants;
    for (const auto &variant : stmt.variants) {
      variants.push_back(variant.first);
    }
    definedEnums_[stmt.name] = variants;
  }
  stmtTypes_[&stmt] = unit_;
}

bool TypeChecker::unify() {
  for (const auto &constraint : constraints_) {
    auto t1 = subst_.apply(constraint.t1);
    auto t2 = subst_.apply(constraint.t2);

    if (!t1 || !t2) {
      continue;
    }

    if (!unifyPair(t1, t2)) {
      reportTypeMismatch(t1, t2, constraint.context);
      return false;
    }
  }
  return true;
}

bool TypeChecker::unifyPair(std::shared_ptr<Type> t1,
                            std::shared_ptr<Type> t2) {
  if (!t1 || !t2) {
    return false;
  }

  auto var1 = dynamic_cast<TypeVariable *>(t1.get());
  auto var2 = dynamic_cast<TypeVariable *>(t2.get());

  if (var1 && var2) {
    if (var1->id != var2->id) {
      subst_.add(var1->name, t2);
      return true;
    }
    return true;
  }

  if (var1) {
    if (occursIn(var1->name, t2.get())) {
      return false;
    }
    subst_.add(var1->name, t2);
    return true;
  }

  if (var2) {
    if (occursIn(var2->name, t1.get())) {
      return false;
    }
    subst_.add(var2->name, t1);
    return true;
  }

  auto p1 = dynamic_cast<PrimitiveType *>(t1.get());
  auto p2 = dynamic_cast<PrimitiveType *>(t2.get());
  if (p1 && p2) {
    return p1->kind == p2->kind;
  }

  auto a1 = dynamic_cast<ArrayType *>(t1.get());
  auto a2 = dynamic_cast<ArrayType *>(t2.get());
  if (a1 && a2) {
    auto e1 = a1->getElementType();
    auto e2 = a2->getElementType();
    if (!e1 || !e2)
      return false;
    return unifyPair(e1, e2);
  }

  auto f1 = dynamic_cast<FunctionType *>(t1.get());
  auto f2 = dynamic_cast<FunctionType *>(t2.get());
  if (f1 && f2) {
    if (f1->paramTypes.size() != f2->paramTypes.size()) {
      return false;
    }
    auto r1 = f1->getReturnType();
    auto r2 = f2->getReturnType();
    if (r1 && r2) {
      if (!unifyPair(r1, r2)) {
        return false;
      }
    }
    for (size_t i = 0; i < f1->paramTypes.size(); ++i) {
      auto p1t = f1->getParamType(i);
      auto p2t = f2->getParamType(i);
      if (!p1t || !p2t)
        continue;
      if (!unifyPair(p1t, p2t)) {
        return false;
      }
    }
    return true;
  }

  auto g1 = dynamic_cast<GenericType *>(t1.get());
  auto g2 = dynamic_cast<GenericType *>(t2.get());
  if (g1 && g2) {
    if (g1->name != g2->name ||
        g1->typeParams.size() != g2->typeParams.size()) {
      return false;
    }
    for (size_t i = 0; i < g1->typeParams.size(); ++i) {
      if (!unifyPair(g1->getTypeParam(i), g2->getTypeParam(i))) {
        return false;
      }
    }
    return true;
  }

  return false;
}

bool TypeChecker::occursIn(const std::string &varName, const Type *type) {
  if (!type) {
    return false;
  }

  if (auto *var = dynamic_cast<const TypeVariable *>(type)) {
    if (var->name == varName) {
      return true;
    }
    if (var->instance) {
      return occursIn(varName, var->instance.get());
    }
  }

  if (auto *arr = dynamic_cast<const ArrayType *>(type)) {
    if (!arr->elementType)
      return false;
    return occursIn(varName, arr->elementType.get());
  }

  if (auto *fn = dynamic_cast<const FunctionType *>(type)) {
    if (fn->returnType && occursIn(varName, fn->returnType.get())) {
      return true;
    }
    for (const auto &param : fn->paramTypes) {
      if (param && occursIn(varName, param.get())) {
        return true;
      }
    }
  }

  if (auto *gen = dynamic_cast<const GenericType *>(type)) {
    for (const auto &tp : gen->typeParams) {
      if (tp && occursIn(varName, tp.get())) {
        return true;
      }
    }
  }

  if (auto *ref = dynamic_cast<const RefType *>(type)) {
    return occursIn(varName, ref->innerType.get());
  }

  return false;
}

void TypeChecker::visitModuleStmt(ModuleStmt &stmt) {
  stmtTypes_[&stmt] = unit_;
}

void TypeChecker::visitImportStmt(ImportStmt &stmt) {
  stmtTypes_[&stmt] = unit_;
}

void TypeChecker::visitExportStmt(ExportStmt &stmt) {
  stmtTypes_[&stmt] = unit_;
}

void TypeChecker::visitExternStmt(ExternStmt &stmt) {
  stmtTypes_[&stmt] = unit_;
}

} // namespace types
} // namespace meadows
