#ifndef MEADOWS_SOURCE_MAP_H
#define MEADOWS_SOURCE_MAP_H

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace meadows {

struct Location {
  uint32_t file = 0;
  uint32_t line = 0;
  uint32_t column = 0;
};

class SourceMap {
public:
  uint32_t add(std::string path, std::string text);
  const std::string& path(uint32_t file) const { return files_[file].path; }
  const std::string& text(uint32_t file) const { return files_[file].text; }
  std::string_view lineText(uint32_t file, uint32_t line) const;

private:
  struct File {
    std::string path;
    std::string text;
    std::vector<size_t> lineStarts;
  };
  std::vector<File> files_;
};

std::string formatLocation(const SourceMap& sources, const Location& loc);

} // namespace meadows

#endif
