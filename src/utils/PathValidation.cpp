#include "PathValidation.h"

#include <filesystem>

namespace fs = std::filesystem;

namespace meadows {

bool hasDangerousChars(const std::string &s) {
  // Excludes () so paths like "Program Files (x86)" are valid.
  // Shell metacharacters are harmless here because we use exec-style
  // invocation (never system()), but we still reject obvious injection chars.
  static constexpr const char *kDangerous = ";|&`${}[]<>!\\\"'\n\r\t";
  return s.find_first_of(kDangerous) != std::string::npos;
}

bool validateSourceFilePath(const std::string &path, std::string &err) {
  if (hasDangerousChars(path)) {
    err = "Invalid characters in file path";
    return false;
  }
  if (path.find("..") != std::string::npos) {
    err = "Path traversal not allowed (..)";
    return false;
  }

  fs::path p(path);
  if (p.extension().string() != kSourceFileExtension) {
    err = "File must have .ms extension";
    return false;
  }
  if (!fs::exists(p)) {
    err = "File does not exist: " + path;
    return false;
  }
  if (!fs::is_regular_file(p)) {
    err = "Not a regular file: " + path;
    return false;
  }

  try {
    if (fs::file_size(p) > kMaxSourceFileSize) {
      err = "File too large (max 10 MB)";
      return false;
    }
  } catch (const fs::filesystem_error &e) {
    err = "Cannot read file size: " + std::string(e.what());
    return false;
  }
  return true;
}

} // namespace meadows
