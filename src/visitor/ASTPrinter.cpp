#include "ASTPrinter.h"
#include "../ast/Expression.h"
#include "../ast/Statement.h"

namespace meadows {

void ASTPrinter::indent() {
  for (int i = 0; i < indentLevel; i++) {
    output << "  ";
  }
}

void ASTPrinter::print(const std::string &text) { output << text; }

void ASTPrinter::println(const std::string &text) {
  indent();
  output << text << "\n";
}

// Expression visitors
void ASTPrinter::visit(IntegerLiteral &node) {
  print("IntegerLiteral(" + std::to_string(node.value) + ")");
}

void ASTPrinter::visit(FloatLiteral &node) {
  print("FloatLiteral(" + std::to_string(node.value) + ")");
}

void ASTPrinter::visit(StringLiteral &node) {
  print("StringLiteral(\"" + node.value + "\")");
}

void ASTPrinter::visit(BooleanLiteral &node) {
  print("BooleanLiteral(" + std::string(node.value ? "true" : "false") + ")");
}

void ASTPrinter::visit(NoneLiteral &node) { print("NoneLiteral"); }

void ASTPrinter::visit(ListLiteral &node) {
  print("ListLiteral([");
  for (size_t i = 0; i < node.elements.size(); i++) {
    if (i > 0)
      print(", ");
    node.elements[i]->accept(*this);
  }
  print("])");
}

void ASTPrinter::visit(Identifier &node) {
  print("Identifier(" + node.name + ")");
}

void ASTPrinter::visit(BinaryExpression &node) {
  print("BinaryExpression(");
  node.left->accept(*this);

  std::string opStr;
  switch (node.operator_) {
  case BinaryOp::ADD:
    opStr = "+";
    break;
  case BinaryOp::SUBTRACT:
    opStr = "-";
    break;
  case BinaryOp::MULTIPLY:
    opStr = "*";
    break;
  case BinaryOp::DIVIDE:
    opStr = "/";
    break;
  case BinaryOp::MODULO:
    opStr = "%";
    break;
  case BinaryOp::POWER:
    opStr = "**";
    break;
  case BinaryOp::EQUAL:
    opStr = "==";
    break;
  case BinaryOp::NOT_EQUAL:
    opStr = "!=";
    break;
  case BinaryOp::LESS_THAN:
    opStr = "<";
    break;
  case BinaryOp::LESS_EQUAL:
    opStr = "<=";
    break;
  case BinaryOp::GREATER_THAN:
    opStr = ">";
    break;
  case BinaryOp::GREATER_EQUAL:
    opStr = ">=";
    break;
  case BinaryOp::AND:
    opStr = "and";
    break;
  case BinaryOp::OR:
    opStr = "or";
    break;
  }

  print(" " + opStr + " ");
  node.right->accept(*this);
  print(")");
}

void ASTPrinter::visit(UnaryExpression &node) {
  std::string opStr;
  switch (node.operator_) {
    case UnaryOp::MINUS:
      opStr = "-";
      break;
    case UnaryOp::NOT:
      opStr = "not";
      break;
    case UnaryOp::PLUS:
      opStr = "+";
      break;
  }
  print("UnaryExpression(" + opStr + " ");
  node.operand->accept(*this);
  print(")");
}

void ASTPrinter::visit(FunctionCall &node) {
  print("FunctionCall(");
  node.function->accept(*this);
  print(", [");
  for (size_t i = 0; i < node.arguments.size(); i++) {
    if (i > 0)
      print(", ");
    node.arguments[i]->accept(*this);
  }
  print("])");
}

void ASTPrinter::visit(AttributeAccess &node) {
  print("AttributeAccess(");
  node.object->accept(*this);
  print("." + node.attribute + ")");
}

void ASTPrinter::visit(IndexAccess &node) {
  print("IndexAccess(");
  node.object->accept(*this);
  print("[");
  node.index->accept(*this);
  print("])");
}

void ASTPrinter::visit(Assignment &node) {
  print("Assignment(");
  node.target->accept(*this);
  print(" = ");
  node.value->accept(*this);
  print(")");
}

// Statement visitors
void ASTPrinter::visit(ExpressionStatement &node) {
  println("ExpressionStatement:");
  indentLevel++;
  indent();
  node.expression->accept(*this);
  print("\n");
  indentLevel--;
}

void ASTPrinter::visit(Block &node) {
  println("Block:");
  indentLevel++;
  for (auto &stmt : node.statements) {
    stmt->accept(*this);
  }
  indentLevel--;
}

void ASTPrinter::visit(IfStatement &node) {
  println("IfStatement:");
  indentLevel++;

  println("Condition:");
  indentLevel++;
  indent();
  node.condition->accept(*this);
  print("\n");
  indentLevel--;

  println("Then:");
  indentLevel++;
  node.thenBranch->accept(*this);
  indentLevel--;

  if (node.elseBranch) {
    println("Else:");
    indentLevel++;
    node.elseBranch->accept(*this);
    indentLevel--;
  }

  indentLevel--;
}

void ASTPrinter::visit(WhileStatement &node) {
  println("WhileStatement:");
  indentLevel++;

  println("Condition:");
  indentLevel++;
  indent();
  node.condition->accept(*this);
  print("\n");
  indentLevel--;

  println("Body:");
  indentLevel++;
  node.body->accept(*this);
  indentLevel--;

  indentLevel--;
}

void ASTPrinter::visit(ForStatement &node) {
  println("ForStatement:");
  indentLevel++;

  println("Variable: " + node.variable);

  println("Iterable:");
  indentLevel++;
  indent();
  node.iterable->accept(*this);
  print("\n");
  indentLevel--;

  println("Body:");
  indentLevel++;
  node.body->accept(*this);
  indentLevel--;

  indentLevel--;
}

void ASTPrinter::visit(ReturnStatement &node) {
  println("ReturnStatement:");
  if (node.value) {
    indentLevel++;
    indent();
    node.value->accept(*this);
    print("\n");
    indentLevel--;
  }
}

void ASTPrinter::visit(BreakStatement &node) { println("BreakStatement"); }

void ASTPrinter::visit(ContinueStatement &node) {
  println("ContinueStatement");
}

void ASTPrinter::visit(PassStatement &node) { println("PassStatement"); }

void ASTPrinter::visit(FunctionDefinition &node) {
  println("FunctionDefinition: " + node.name);
  indentLevel++;

  println("Parameters:");
  indentLevel++;
  for (const auto &param : node.parameters) {
    indent();
    print(param.name);
    if (param.defaultValue) {
      print(" = ");
      param.defaultValue->accept(*this);
    }
    print("\n");
  }
  indentLevel--;

  println("Body:");
  indentLevel++;
  node.body->accept(*this);
  indentLevel--;

  indentLevel--;
}

void ASTPrinter::visit(ClassDefinition &node) {
  println("ClassDefinition: " + node.name);
  indentLevel++;

  if (!node.bases.empty()) {
    println("Bases:");
    indentLevel++;
    for (auto &base : node.bases) {
      indent();
      base->accept(*this);
      print("\n");
    }
    indentLevel--;
  }

  println("Body:");
  indentLevel++;
  node.body->accept(*this);
  indentLevel--;

  indentLevel--;
}

void ASTPrinter::visit(ImportStatement &node) {
  println("ImportStatement:");
  indentLevel++;
  for (size_t i = 0; i < node.modules.size(); i++) {
    indent();
    print(node.modules[i]);
    if (!node.aliases[i].empty()) {
      print(" as " + node.aliases[i]);
    }
    print("\n");
  }
  indentLevel--;
}

void ASTPrinter::visit(Program &node) {
  println("Program:");
  indentLevel++;
  for (auto &stmt : node.statements) {
    stmt->accept(*this);
  }
  indentLevel--;
}

} // namespace meadows
