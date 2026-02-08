#include "../../../src/lexer/Lexer.h"
#include "../../../src/parser/Parser.h"
#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
  if (size < 1 || size > 100000) {
    return 0;
  }

  std::string input(reinterpret_cast<const char *>(data), size);

  try {
    Lexer lexer(input);
    auto tokens = lexer.tokenize();

    Parser parser(tokens);
    parser.parse();
  } catch (const std::exception &) {
  }

  return 0;
}
