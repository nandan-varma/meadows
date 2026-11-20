#ifndef MEADOWS_RUNTIME_SOURCES_H
#define MEADOWS_RUNTIME_SOURCES_H

#include <string>
#include <vector>

namespace meadows {

struct RuntimeFile {
  const char* name;
  const char* contents;
};

const std::vector<RuntimeFile>& runtimeFiles();

} // namespace meadows

#endif
