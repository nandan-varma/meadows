#ifndef MEADOWS_BUILTINS_H
#define MEADOWS_BUILTINS_H

#include <cstddef>
#include <string>

namespace meadows {

enum class Builtin { Print, Len, Str, Push };

struct BuiltinInfo {
  const char* name;
  Builtin builtin;
  size_t arity;
};

const BuiltinInfo* findBuiltin(const std::string& name);

} // namespace meadows

#endif
