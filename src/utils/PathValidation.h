#ifndef PATH_VALIDATION_H
#define PATH_VALIDATION_H

#include <cstdint>
#include <string>

namespace meadows {

constexpr std::uintmax_t kMaxSourceFileSize = 10 * 1024 * 1024;
constexpr const char *kSourceFileExtension = ".ms";

/** Shell/path metacharacters rejected in any file path this compiler reads
 * or writes — shared by the CLI's own input/output paths and by
 * ModuleResolver's import paths. */
bool hasDangerousChars(const std::string &s);

/** Validates a `.ms` source file: no dangerous characters, no `..` path
 * traversal, correct extension, exists, is a regular file, under the size
 * limit. Used for both the CLI's entry file and each `import`ed file. */
bool validateSourceFilePath(const std::string &path, std::string &err);

} // namespace meadows

#endif
