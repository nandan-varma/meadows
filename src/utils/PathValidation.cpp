#include "PathValidation.h"

#include <filesystem>
#include <fstream>
#include <iterator>

namespace fs = std::filesystem;

namespace meadows {

bool validateImportPath(const std::string& path, std::string& error) {
  fs::path candidate(path);
  if (candidate.extension() != SOURCE_FILE_EXTENSION) {
    error = "imported files must have the .ms extension";
    return false;
  }
  if (candidate.is_absolute()) {
    error = "absolute import paths are not allowed";
    return false;
  }
  for (const fs::path& part : candidate) {
    if (part == "..") {
      error = "import paths must not contain '..'";
      return false;
    }
  }
  return true;
}

ReadResult readSourceFile(const std::string& path, std::string& text) {
  std::error_code ec;
  if (!fs::is_regular_file(path, ec)) {
    return ReadResult::NOT_FOUND;
  }
  if (fs::file_size(path, ec) > MAX_SOURCE_FILE_SIZE) {
    return ReadResult::TOO_LARGE;
  }
  std::ifstream file(path, std::ios::binary);
  if (!file) {
    return ReadResult::UNREADABLE;
  }
  text.assign(std::istreambuf_iterator<char>(file),
              std::istreambuf_iterator<char>());
  return ReadResult::OK;
}

const char* describeReadFailure(ReadResult result) {
  switch (result) {
  case ReadResult::NOT_FOUND:
    return "no such file";
  case ReadResult::TOO_LARGE:
    return "file is larger than 10 MB";
  case ReadResult::UNREADABLE:
    return "file cannot be read";
  case ReadResult::OK:
    break;
  }
  return "";
}

} // namespace meadows
