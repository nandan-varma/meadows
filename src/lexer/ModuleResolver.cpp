#include "ModuleResolver.h"
#include "../utils/Exceptions.h"
#include "../utils/PathValidation.h"
#include "Lexer.h"
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

namespace meadows {

std::vector<Token> ModuleResolver::resolve(const std::string &entryPath) {
  std::vector<Token> out;
  std::unordered_set<std::string> visited;
  resolveInto(entryPath, out, visited);
  out.push_back(Token(TokenType::EOF_TOKEN, "", 0, 0));
  return out;
}

void ModuleResolver::resolveInto(const std::string &path,
                                 std::vector<Token> &out,
                                 std::unordered_set<std::string> &visited) {
  std::error_code canonError;
  fs::path canonical = fs::weakly_canonical(path, canonError);
  std::string key = canonError ? path : canonical.string();
  if (visited.count(key)) return; // already imported — first-import-wins
  visited.insert(key);

  std::string err;
  if (!validateSourceFilePath(path, err)) {
    throw MeadowsException(ErrorCode::SYS_FILE_NOT_FOUND,
                           "Cannot import '" + path + "': " + err,
                           SourceLocation(path, 1, 1));
  }

  std::ifstream file(path);
  std::string source((std::istreambuf_iterator<char>(file)),
                     std::istreambuf_iterator<char>());

  Lexer lexer(source);
  std::vector<Token> tokens = lexer.tokenize();
  fs::path baseDir = fs::path(path).parent_path();

  for (size_t i = 0; i < tokens.size(); i++) {
    if (tokens[i].type == TokenType::EOF_TOKEN) continue; // caller adds one

    if (tokens[i].type != TokenType::IMPORT) {
      out.push_back(tokens[i]);
      continue;
    }

    if (i + 2 >= tokens.size() || tokens[i + 1].type != TokenType::STRING ||
        tokens[i + 2].type != TokenType::SEMICOLON) {
      throw MeadowsException(ErrorCode::PARSE_UNEXPECTED_TOKEN,
                             "Expected: import \"path.ms\";",
                             SourceLocation(path, tokens[i].line, tokens[i].column));
    }
    std::string importedPath = (baseDir / tokens[i + 1].value).string();
    resolveInto(importedPath, out, visited);
    i += 2; // skip the STRING and SEMICOLON already consumed above
  }
}

} // namespace meadows
