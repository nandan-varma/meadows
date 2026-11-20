#ifndef MEADOWS_NATIVE_COMPILER_H
#define MEADOWS_NATIVE_COMPILER_H

#include <string>

namespace meadows {

struct NativeBuild {
  std::string ir;
  std::string output;
  int optLevel = 0;
};

bool buildExecutable(const NativeBuild& build, std::string& error);

} // namespace meadows

#endif
