#ifndef MEADOWS_PATH_VALIDATION_H
#define MEADOWS_PATH_VALIDATION_H

#include <cstdint>
#include <string>

namespace meadows {

constexpr std::uintmax_t MAX_SOURCE_FILE_SIZE = 10 * 1024 * 1024;
constexpr const char* SOURCE_FILE_EXTENSION = ".ms";

enum class ReadResult { OK, NOT_FOUND, TOO_LARGE, UNREADABLE };

bool validateImportPath(const std::string& path, std::string& error);
ReadResult readSourceFile(const std::string& path, std::string& text);
const char* describeReadFailure(ReadResult result);

} // namespace meadows

#endif
