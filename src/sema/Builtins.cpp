#include "Builtins.h"

namespace meadows {

namespace {

constexpr BuiltinInfo BUILTINS[] = {{"print", Builtin::Print, 1},
                                    {"len", Builtin::Len, 1},
                                    {"str", Builtin::Str, 1},
                                    {"push", Builtin::Push, 2}};

} // namespace

const BuiltinInfo* findBuiltin(const std::string& name) {
  for (const BuiltinInfo& info : BUILTINS) {
    if (name == info.name) {
      return &info;
    }
  }
  return nullptr;
}

} // namespace meadows
