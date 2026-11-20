#ifndef MEADOWS_DISASSEMBLER_H
#define MEADOWS_DISASSEMBLER_H

#include <string>

#include "Bytecode.h"

namespace meadows {

std::string disassemble(const BytecodeProgram& program);

} // namespace meadows

#endif
