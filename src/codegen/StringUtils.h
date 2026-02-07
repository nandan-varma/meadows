#ifndef STRINGUTILS_H
#define STRINGUTILS_H

#include <memory>
#include <string>
#include <unordered_map>

namespace StringUtils {

class EscapeHandler {
public:
  static std::string process(const std::string &input);
  static std::string escape(const std::string &input);
};

class StringPool {
public:
  static StringPool &getInstance();

  const std::string *intern(const std::string &str);
  size_t poolSize() const { return pool_.size(); }
  void clear() { pool_.clear(); }

  StringPool(const StringPool &) = delete;
  StringPool &operator=(const StringPool &) = delete;

private:
  StringPool() = default;
  std::unordered_map<std::string, std::string> pool_;
};

} // namespace StringUtils

#endif
