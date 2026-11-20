#include <CLI/CLI.hpp>

#include <cstdio>
#include <iostream>
#include <iterator>
#include <unistd.h>

#include "../ast/AstPrinter.h"
#include "../driver/Frontend.h"
#include "../interpreter/Interpreter.h"
#include "../utils/PathValidation.h"

namespace {

using namespace meadows;

struct Options {
  std::string input;
  std::string output;
  int optLevel = 0;
  bool run = false;
  bool dumpAst = false;
  bool dumpIr = false;
  bool lspDiagnostics = false;
  bool readStdin = false;
  bool verbose = false;
  std::vector<std::string> warnings;
};

bool buildWarningPolicy(const std::vector<std::string>& flags,
                        WarningPolicy& policy) {
  for (const std::string& flag : flags) {
    if (flag == "error") {
      policy.treatAsErrors = true;
      continue;
    }
    if (flag.rfind("no-", 0) == 0) {
      std::string name = flag.substr(3);
      bool known = false;
      for (ErrorCode code : {ErrorCode::WARN_UNUSED_VARIABLE,
                             ErrorCode::WARN_UNREACHABLE_CODE,
                             ErrorCode::WARN_SHADOWING_VARIABLE}) {
        if (name == warningName(code)) {
          policy.disabled.insert(code);
          known = true;
        }
      }
      if (known) {
        continue;
      }
    }
    std::cerr << "error: unknown warning option '-W" << flag << "'\n";
    return false;
  }
  return true;
}

void writeOut(std::string_view text) {
  std::fwrite(text.data(), 1, text.size(), stdout);
}

int execute(Analysis& analysis) {
  Interpreter interpreter(writeOut);
  std::optional<RuntimeError> error = interpreter.run(analysis.program);
  std::fflush(stdout);
  if (error) {
    std::cerr << formatRuntimeError(*error, analysis.sources);
    return 1;
  }
  return 0;
}

} // namespace

int main(int argc, char** argv) {
  CLI::App app{"Meadows compiler", "Meadows"};
  app.set_version_flag("-V,--version", MEADOWS_VERSION);

  Options options;
  app.add_option("file", options.input, "Source file (.ms)")->required();
  app.add_option("-o,--output", options.output, "Output executable path");
  app.add_option("-O,--opt-level", options.optLevel, "Optimization level")
      ->check(CLI::Range(0, 3));
  app.add_flag("--run", options.run, "Interpret the program instead of compiling");
  app.add_flag("--dump-ast", options.dumpAst, "Print the syntax tree and exit");
  app.add_flag("--dump-ir", options.dumpIr, "Print the LLVM IR and exit");
  app.add_flag("--lsp-diagnostics", options.lspDiagnostics,
               "Print diagnostics as JSON for editor integration");
  app.add_flag("--stdin", options.readStdin,
               "Read the source from standard input; the file argument names it");
  app.add_flag("-v,--verbose", options.verbose, "Report each compilation phase");
  app.add_option("-W", options.warnings, "Warning options: -Werror, -Wno-<name>")
      ->allow_extra_args(false);
  CLI11_PARSE(app, argc, argv);

  WarningPolicy policy;
  if (!buildWarningPolicy(options.warnings, policy)) {
    return 1;
  }

  std::string text;
  if (options.readStdin) {
    text.assign(std::istreambuf_iterator<char>(std::cin),
                std::istreambuf_iterator<char>());
  } else if (ReadResult read = readSourceFile(options.input, text);
             read != ReadResult::OK) {
    std::cerr << "error: cannot read '" << options.input
              << "': " << describeReadFailure(read) << "\n";
    return 1;
  }

  Analysis analysis = analyzeSource(options.input, std::move(text), policy);

  if (options.lspDiagnostics) {
    std::cout << "{\"diagnostics\":"
              << diagnosticsToJson(analysis.diagnostics.diagnostics(),
                                   analysis.sources)
              << "}\n";
    return 0;
  }

  FormatOptions format;
  format.color = isatty(STDERR_FILENO) != 0;
  std::cerr << formatDiagnostics(analysis.diagnostics.diagnostics(),
                                 analysis.sources, format);
  if (!analysis.ok()) {
    return 1;
  }

  if (options.dumpAst) {
    std::cout << AstPrinter().print(analysis.program);
    return 0;
  }
  if (options.run) {
    return execute(analysis);
  }
  std::cerr << "error: native compilation is unavailable in this build\n";
  return 1;
}
