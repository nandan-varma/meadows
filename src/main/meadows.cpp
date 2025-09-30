#include <iostream>
#include <fstream>
#include <string>
#include "../lexer/Lexer.h"
#include "../parser/Parser.h"
#include "../codegen/CodeGen.h"
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/TargetSelect.h>

int main(int argc, char* argv[]) {

    if (argc != 2) {
        std::cerr << "Usage: meadows <file.ms>" << std::endl;
        return 1;
    }

    std::ifstream file(argv[1]);
    if (!file) {
        std::cerr << "Cannot open file " << argv[1] << std::endl;
        return 1;
    }

    std::string source((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    Lexer lexer(source);
    auto tokens = lexer.tokenize();
    Parser parser(tokens);
    auto statements = parser.parse();
    CodeGen codegen;
    codegen.generate(statements);
    auto module = codegen.getModule();
    std::string outputFile = std::string(argv[1]) + ".ll";
    std::error_code EC;
    llvm::raw_fd_ostream out(outputFile, EC);
    if (EC) {
        std::cerr << "Cannot open output file " << outputFile << std::endl;
        return 1;
    }
    module->print(out, nullptr);
    out.close();
    std::string exeFile = std::string(argv[1]) + ".out";
    std::string cmd = "clang++ " + outputFile + " -o " + exeFile;
    int ret = system(cmd.c_str());
    if (ret == 0) {
        std::cout << "Compiled successfully to " << exeFile << std::endl;
    } else {
        std::cerr << "Compilation failed" << std::endl;
        return 1;
    }
    return 0;
}