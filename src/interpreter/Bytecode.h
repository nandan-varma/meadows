#ifndef MEADOWS_BYTECODE_H
#define MEADOWS_BYTECODE_H

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "../ast/AST.h"
#include "../sema/Builtins.h"
#include "../utils/SourceMap.h"
#include "Value.h"

namespace meadows {

enum class Op : uint8_t {
  Constant,
  Load,
  Store,
  Assign,
  LoadFunction,
  Pop,
  Add,
  Sub,
  Mul,
  Div,
  Mod,
  Eq,
  Ne,
  Lt,
  Le,
  Gt,
  Ge,
  Negate,
  Not,
  Jump,
  JumpIfFalse,
  AndShort,
  OrShort,
  ToBool,
  Index,
  SetIndex,
  Field,
  SetField,
  Call,
  CallBuiltin,
  MakeArray,
  MakeObject,
  Return,
  Clear,
  ForInit,
  ForNext,
  ForStep,
  Halt
};

struct Instruction {
  Op op;
  int32_t a = 0;
  int32_t b = 0;
  uint32_t site = 0;
};

struct FunctionCode {
  std::string name;
  uint32_t arity = 0;
  uint32_t slots = 0;
  std::vector<Instruction> code;
};

struct BytecodeProgram {
  std::vector<std::unique_ptr<FunctionCode>> functions;
  std::vector<Value> constants;
  std::vector<std::string> names;
  std::vector<std::vector<std::string>> shapes;
  std::vector<Location> sites;
  FunctionCode* main = nullptr;
};

BytecodeProgram compileProgram(StmtList& program);

} // namespace meadows

#endif
