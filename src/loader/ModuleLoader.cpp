#include "ModuleLoader.h"

#include <filesystem>
#include <map>

#include "../lexer/Lexer.h"
#include "../utils/PathValidation.h"

namespace fs = std::filesystem;

namespace meadows {

namespace {

std::string canonicalKey(const std::string& path) {
  std::error_code ec;
  fs::path canonical = fs::weakly_canonical(path, ec);
  return ec ? path : canonical.string();
}

} // namespace

StmtList ModuleLoader::load(const std::string& path, std::string text) {
  loaded_.insert(canonicalKey(path));
  return parseModule(path, std::move(text), 0);
}

StmtList ModuleLoader::parseModule(const std::string& path, std::string text,
                                   int depth) {
  uint32_t file = sources_.add(path, std::move(text));
  size_t errorsBefore = diagnostics_.errorCount();
  Lexer lexer(sources_.text(file), file, diagnostics_);
  std::vector<Token> tokens = lexer.tokenize();
  ParsedFile parsed;
  if (diagnostics_.errorCount() == errorsBefore) {
    parsed = Parser(std::move(tokens), diagnostics_).parse();
  }

  StmtList result;
  size_t nextImport = 0;
  auto spliceImportsAt = [&](size_t position) {
    while (nextImport < parsed.imports.size() &&
           parsed.imports[nextImport].position == position) {
      splice(result, parsed.imports[nextImport++], path, depth);
    }
  };
  for (size_t i = 0; i < parsed.statements.size(); ++i) {
    spliceImportsAt(i);
    result.push_back(std::move(parsed.statements[i]));
  }
  spliceImportsAt(parsed.statements.size());
  return result;
}

void ModuleLoader::splice(StmtList& into, const ImportRequest& request,
                          const std::string& importer, int depth) {
  auto fail = [&](ErrorCode code, const std::string& reason) {
    diagnostics_.report(code, "Cannot import '" + request.path + "': " + reason,
                        request.location, 6);
  };

  std::string reason;
  if (!validateImportPath(request.path, reason)) {
    fail(ErrorCode::SYS_INVALID_PATH, reason);
    return;
  }
  if (depth >= MAX_IMPORT_DEPTH) {
    fail(ErrorCode::SYS_INVALID_PATH, "imports are nested too deeply");
    return;
  }

  std::string resolved =
      (fs::path(importer).parent_path() / request.path).lexically_normal();
  if (!loaded_.insert(canonicalKey(resolved)).second) {
    return;
  }

  std::string text;
  ReadResult read = readSourceFile(resolved, text);
  if (read != ReadResult::OK) {
    static const std::map<ReadResult, ErrorCode> codes = {
        {ReadResult::NOT_FOUND, ErrorCode::SYS_FILE_NOT_FOUND},
        {ReadResult::TOO_LARGE, ErrorCode::SYS_FILE_TOO_LARGE},
        {ReadResult::UNREADABLE, ErrorCode::SYS_IO_ERROR}};
    fail(codes.at(read), describeReadFailure(read));
    return;
  }
  StmtList imported = parseModule(resolved, std::move(text), depth + 1);
  for (StmtPtr& stmt : imported) {
    into.push_back(std::move(stmt));
  }
}

} // namespace meadows
