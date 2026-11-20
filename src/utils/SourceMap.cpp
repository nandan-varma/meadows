#include "SourceMap.h"

namespace meadows {

uint32_t SourceMap::add(std::string path, std::string text) {
  File file{std::move(path), std::move(text), {0}};
  for (size_t i = 0; i < file.text.size(); ++i) {
    if (file.text[i] == '\n') {
      file.lineStarts.push_back(i + 1);
    }
  }
  files_.push_back(std::move(file));
  return static_cast<uint32_t>(files_.size() - 1);
}

std::string_view SourceMap::lineText(uint32_t file, uint32_t line) const {
  const File& f = files_[file];
  if (line == 0 || line > f.lineStarts.size()) {
    return {};
  }
  size_t start = f.lineStarts[line - 1];
  size_t end = line < f.lineStarts.size() ? f.lineStarts[line] - 1
                                          : f.text.size();
  if (end > start && f.text[end - 1] == '\r') {
    --end;
  }
  return std::string_view(f.text).substr(start, end - start);
}

std::string formatLocation(const SourceMap& sources, const Location& loc) {
  return sources.path(loc.file) + ":" + std::to_string(loc.line) + ":" +
         std::to_string(loc.column);
}

} // namespace meadows
