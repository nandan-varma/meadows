#ifndef MEADOWS_PARSER_H
#define MEADOWS_PARSER_H

#include <vector>

#include "../ast/AST.h"
#include "../lexer/Token.h"
#include "../utils/Diagnostics.h"

namespace meadows {

struct ImportRequest {
  size_t position;
  std::string path;
  Location location;
};

struct ParsedFile {
  StmtList statements;
  std::vector<ImportRequest> imports;
};

class Parser {
public:
  static constexpr uint32_t MAX_NESTING = 256;

  Parser(std::vector<Token> tokens, DiagnosticSink& diagnostics);

  ParsedFile parse();

private:
  struct ParseAbort {};

  class DepthScope {
  public:
    explicit DepthScope(Parser& parser);
    ~DepthScope() { --parser_.depth_; }

  private:
    Parser& parser_;
  };

  std::vector<Token> tokens_;
  DiagnosticSink& diagnostics_;
  size_t current_ = 0;
  uint32_t depth_ = 0;
  bool abandoned_ = false;

  const Token& peek() const { return tokens_[current_]; }
  const Token& previous() const { return tokens_[current_ - 1]; }
  bool check(TokenType type) const { return peek().type == type; }
  bool atEnd() const { return abandoned_ || check(TokenType::END_OF_FILE); }
  const Token& advance();
  bool match(TokenType type);
  const Token& expect(TokenType type, ErrorCode code, const std::string& what);
  Location expectedLocation() const;

  void error(ErrorCode code, const std::string& message, Location location,
             uint32_t length, const std::string& help = {});
  [[noreturn]] void fail(ErrorCode code, const std::string& message,
                         Location location, uint32_t length,
                         const std::string& help = {});
  void synchronize(bool insideBlock);

  void parseTopLevel(ParsedFile& file);
  StmtPtr parseStatement(bool topLevel);
  StmtPtr parseLet();
  StmtPtr parseFunc();
  StmtPtr parseIf();
  StmtPtr parseFor();
  StmtPtr parseWhile();
  StmtPtr parseReturn();
  StmtPtr parseExpressionStatement();
  StmtList parseBlock();
  ImportRequest parseImport(size_t position);

  ExprPtr parseExpression();
  ExprPtr parseAssignment();
  ExprPtr parseBinary(int level);
  ExprPtr parseUnary();
  ExprPtr parsePostfix();
  ExprPtr parsePrimary();
  ExprPtr parseNumber(const Token& token, bool negated);
  ExprPtr parseObject(const Token& open);
  std::vector<ExprPtr> parseArguments();

  ExprPtr bounded(ExprPtr expr, Location location);
};

} // namespace meadows

#endif
