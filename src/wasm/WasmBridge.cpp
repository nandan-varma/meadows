// Browser playground bridge: lex -> parse -> semantic analysis only.
// No LLVM codegen here — this build has no LLVM linked in, and the native
// compiler's final "clang++ as a subprocess" step has no browser equivalent.
#include <emscripten/bind.h>

#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "../ast/AST.h"
#include "../interpreter/Interpreter.h"
#include "../lexer/Lexer.h"
#include "../lexer/Token.h"
#include "../parser/Parser.h"
#include "../sema/SemanticAnalyzer.h"
#include "../utils/ASTPrinter.h"
#include "../utils/DiagnosticsCollector.h"
#include "../utils/ErrorFormatter.h"
#include "../utils/Exceptions.h"
#include "../utils/WarningManager.h"

namespace {

const char *tokenTypeName(TokenType type) {
  switch (type) {
    case TokenType::LET: return "LET";
    case TokenType::FUNC: return "FUNC";
    case TokenType::IF: return "IF";
    case TokenType::ELSE: return "ELSE";
    case TokenType::FOR: return "FOR";
    case TokenType::WHILE: return "WHILE";
    case TokenType::RETURN: return "RETURN";
    case TokenType::IN: return "IN";
    case TokenType::RANGE: return "RANGE";
    case TokenType::TRUE: return "TRUE";
    case TokenType::FALSE: return "FALSE";
    case TokenType::BREAK: return "BREAK";
    case TokenType::CONTINUE: return "CONTINUE";
    case TokenType::IDENTIFIER: return "IDENTIFIER";
    case TokenType::STRING: return "STRING";
    case TokenType::NUMBER: return "NUMBER";
    case TokenType::PLUS: return "PLUS";
    case TokenType::MINUS: return "MINUS";
    case TokenType::STAR: return "STAR";
    case TokenType::SLASH: return "SLASH";
    case TokenType::PERCENT: return "PERCENT";
    case TokenType::EQUAL: return "EQUAL";
    case TokenType::EQUAL_EQUAL: return "EQUAL_EQUAL";
    case TokenType::BANG: return "BANG";
    case TokenType::BANG_EQUAL: return "BANG_EQUAL";
    case TokenType::GREATER: return "GREATER";
    case TokenType::LESS: return "LESS";
    case TokenType::GREATER_EQUAL: return "GREATER_EQUAL";
    case TokenType::LESS_EQUAL: return "LESS_EQUAL";
    case TokenType::AND: return "AND";
    case TokenType::OR: return "OR";
    case TokenType::LEFT_PAREN: return "LEFT_PAREN";
    case TokenType::RIGHT_PAREN: return "RIGHT_PAREN";
    case TokenType::LEFT_BRACE: return "LEFT_BRACE";
    case TokenType::RIGHT_BRACE: return "RIGHT_BRACE";
    case TokenType::LEFT_BRACKET: return "LEFT_BRACKET";
    case TokenType::RIGHT_BRACKET: return "RIGHT_BRACKET";
    case TokenType::COMMA: return "COMMA";
    case TokenType::COLON: return "COLON";
    case TokenType::SEMICOLON: return "SEMICOLON";
    case TokenType::DOT: return "DOT";
    case TokenType::COMMENT: return "COMMENT";
    case TokenType::EOF_TOKEN: return "EOF";
  }
  return "UNKNOWN";
}

std::string formatTokens(const std::vector<Token> &tokens) {
  std::ostringstream out;
  for (const auto &tok : tokens) {
    out << tok.line << ":" << tok.column << "  " << tokenTypeName(tok.type);
    if (tok.type != TokenType::EOF_TOKEN) {
      out << "  '" << tok.value << "'";
    }
    out << "\n";
  }
  return out.str();
}

} // namespace

struct CompileResult {
  bool success = false;
  std::string tokens;
  std::string ast;
  std::string diagnostics;
  std::string output;
  bool ran = false;
  int exitCode = 0;
};

CompileResult compileSource(const std::string &source) {
  CompileResult result;

  // ErrorFormatter reads source lines back off disk for context; give it a
  // virtual file on Emscripten's in-memory filesystem rather than teaching it
  // about strings-as-files.
  const std::string virtualPath = "/playground.ms";
  {
    std::ofstream out(virtualPath, std::ios::binary | std::ios::trunc);
    out << source;
  }

  meadows::DiagnosticsCollector diagnostics;
  std::vector<Token> tokens;

  try {
    Lexer lexer(source);
    tokens = lexer.tokenize();
  } catch (const meadows::MeadowsException &e) {
    diagnostics.reportError(e.code(), e.message(), e.location());
  } catch (const std::exception &e) {
    diagnostics.reportError(meadows::ErrorCode::LEX_INVALID_CHARACTER, e.what(),
                             meadows::SourceLocation(virtualPath, 1, 1));
  }

  result.tokens = formatTokens(tokens);
  std::vector<std::unique_ptr<Stmt>> stmts;

  if (!tokens.empty() && !diagnostics.hasErrors()) {
    try {
      Parser parser(tokens, diagnostics);
      stmts = parser.parse();

      ASTPrinter printer;
      result.ast = printer.print(stmts);

      if (!diagnostics.hasErrors()) {
        meadows::WarningManager warnings;
        meadows::SemanticAnalyzer sema(diagnostics, warnings, virtualPath);
        sema.analyze(stmts);
      }
    } catch (const meadows::MeadowsException &e) {
      diagnostics.reportError(e.code(), e.message(), e.location());
    } catch (const std::exception &e) {
      diagnostics.reportError(meadows::ErrorCode::PARSE_UNEXPECTED_TOKEN,
                               e.what(), meadows::SourceLocation(virtualPath, 1, 1));
    }
  }

  meadows::ErrorFormatter::FormatOptions opts;
  opts.useColors = false;
  meadows::ErrorFormatter formatter(opts);
  result.diagnostics = formatter.formatMultiple(diagnostics.diagnostics(), virtualPath);
  result.success = !diagnostics.hasErrors();

  // Only run a program that lexed, parsed, and passed semantic analysis
  // cleanly — the same gate CodeGen would need before emitting IR.
  if (result.success && !stmts.empty()) {
    result.ran = true;
    std::string output;
    meadows::Interpreter interpreter(
        [&output](const std::string &chunk) { output += chunk; });
    result.exitCode = interpreter.run(stmts);
    result.output = std::move(output);
  }

  return result;
}

EMSCRIPTEN_BINDINGS(meadows_module) {
  emscripten::value_object<CompileResult>("CompileResult")
      .field("success", &CompileResult::success)
      .field("tokens", &CompileResult::tokens)
      .field("ast", &CompileResult::ast)
      .field("diagnostics", &CompileResult::diagnostics)
      .field("output", &CompileResult::output)
      .field("ran", &CompileResult::ran)
      .field("exitCode", &CompileResult::exitCode);

  emscripten::function("compileSource", &compileSource);
}
