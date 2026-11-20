#include "Disassembler.h"

#include <algorithm>
#include <cstdio>

namespace meadows {

namespace {

const char* opName(Op op) {
  switch (op) {
  case Op::Constant:
    return "Constant";
  case Op::Load:
    return "Load";
  case Op::Store:
    return "Store";
  case Op::Assign:
    return "Assign";
  case Op::LoadFunction:
    return "LoadFunction";
  case Op::Pop:
    return "Pop";
  case Op::Add:
    return "Add";
  case Op::Sub:
    return "Sub";
  case Op::Mul:
    return "Mul";
  case Op::Div:
    return "Div";
  case Op::Mod:
    return "Mod";
  case Op::Eq:
    return "Eq";
  case Op::Ne:
    return "Ne";
  case Op::Lt:
    return "Lt";
  case Op::Le:
    return "Le";
  case Op::Gt:
    return "Gt";
  case Op::Ge:
    return "Ge";
  case Op::Negate:
    return "Negate";
  case Op::Not:
    return "Not";
  case Op::Jump:
    return "Jump";
  case Op::JumpIfFalse:
    return "JumpIfFalse";
  case Op::AndShort:
    return "AndShort";
  case Op::OrShort:
    return "OrShort";
  case Op::ToBool:
    return "ToBool";
  case Op::Index:
    return "Index";
  case Op::SetIndex:
    return "SetIndex";
  case Op::Field:
    return "Field";
  case Op::SetField:
    return "SetField";
  case Op::Call:
    return "Call";
  case Op::CallBuiltin:
    return "CallBuiltin";
  case Op::MakeArray:
    return "MakeArray";
  case Op::MakeObject:
    return "MakeObject";
  case Op::Return:
    return "Return";
  case Op::Clear:
    return "Clear";
  case Op::ForInit:
    return "ForInit";
  case Op::ForNext:
    return "ForNext";
  case Op::ForStep:
    return "ForStep";
  case Op::Halt:
    return "Halt";
  }
  return "";
}

const char* builtinName(int32_t builtin) {
  switch (static_cast<Builtin>(builtin)) {
  case Builtin::Print:
    return "print";
  case Builtin::Len:
    return "len";
  case Builtin::Str:
    return "str";
  case Builtin::Push:
    return "push";
  }
  return "";
}

std::string quote(const std::string& text) {
  std::string out = "\"";
  for (char c : text) {
    switch (c) {
    case '\n':
      out += "\\n";
      break;
    case '\t':
      out += "\\t";
      break;
    case '"':
      out += "\\\"";
      break;
    case '\\':
      out += "\\\\";
      break;
    default:
      out += c;
    }
  }
  return out + "\"";
}

std::string constant(const Value& value) {
  return value.isString() ? quote(value.asString()) : value.display();
}

class Disassembler {
public:
  explicit Disassembler(const BytecodeProgram& program) : program_(program) {}

  std::string run() {
    for (const auto& code : program_.functions) {
      function(*code);
    }
    return out_;
  }

private:
  const BytecodeProgram& program_;
  std::string out_;

  void function(const FunctionCode& code) {
    std::string title = code.name.empty() ? "main" : code.name;
    out_ += "func " + title + "  params " + std::to_string(code.arity) +
            "  slots " + std::to_string(code.slots) + "\n";
    for (size_t i = 0; i < code.code.size(); ++i) {
      instruction(i, code.code[i]);
    }
    out_ += "\n";
  }

  void instruction(size_t index, const Instruction& at) {
    char head[48];
    std::snprintf(head, sizeof head, "%5zu  %s", index, opName(at.op));
    std::string text = head;
    std::string detail = operands(at);
    if (!detail.empty()) {
      text.resize(std::max<size_t>(text.size() + 1, 20), ' ');
      text += detail;
    }
    if (hasSite(at.op)) {
      text += "  ; line " + std::to_string(program_.sites[at.site].line);
    }
    out_ += text + "\n";
  }

  static bool hasSite(Op op) {
    switch (op) {
    case Op::Add:
    case Op::Sub:
    case Op::Mul:
    case Op::Div:
    case Op::Mod:
    case Op::Eq:
    case Op::Ne:
    case Op::Lt:
    case Op::Le:
    case Op::Gt:
    case Op::Ge:
    case Op::JumpIfFalse:
    case Op::AndShort:
    case Op::OrShort:
    case Op::ToBool:
    case Op::Index:
    case Op::SetIndex:
    case Op::Field:
    case Op::SetField:
    case Op::Call:
    case Op::CallBuiltin:
    case Op::ForInit:
      return true;
    default:
      return false;
    }
  }

  std::string operands(const Instruction& at) const {
    const size_t a = static_cast<size_t>(at.a);
    switch (at.op) {
    case Op::Constant:
      return std::to_string(at.a) + "  " + constant(program_.constants[a]);
    case Op::Load:
    case Op::Store:
    case Op::Assign:
      return "slot " + std::to_string(at.a);
    case Op::LoadFunction:
      return program_.functions[a]->name;
    case Op::Jump:
    case Op::JumpIfFalse:
    case Op::AndShort:
    case Op::OrShort:
      return "-> " + std::to_string(at.a);
    case Op::ToBool:
      return at.a == 0 ? "&&" : "||";
    case Op::Field:
    case Op::SetField:
      return program_.names[a];
    case Op::Call:
      return std::to_string(at.a) + " args";
    case Op::CallBuiltin:
      return std::string(builtinName(at.a)) + "  " + std::to_string(at.b) +
             " args";
    case Op::MakeArray:
      return std::to_string(at.a) + " items";
    case Op::MakeObject: {
      std::string fields;
      for (const std::string& name : program_.shapes[a]) {
        fields += (fields.empty() ? "" : ", ") + name;
      }
      return "{" + fields + "}";
    }
    case Op::Clear:
      return "slots " + std::to_string(at.a) + ".." +
             std::to_string(at.a + at.b - 1);
    case Op::ForInit:
      return "slot " + std::to_string(at.a);
    case Op::ForNext:
      return "slot " + std::to_string(at.b) + "  exit -> " +
             std::to_string(at.a);
    case Op::ForStep:
      return "slot " + std::to_string(at.a);
    default:
      return "";
    }
  }
};

} // namespace

std::string disassemble(const BytecodeProgram& program) {
  return Disassembler(program).run();
}

} // namespace meadows
