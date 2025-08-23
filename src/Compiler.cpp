#include "Compiler.h"
#include <iostream>
#include <sstream>

namespace meadows {

std::unique_ptr<Program> Compiler::compile(const std::string& source, const std::string& filename) {
    try {
        // Lexical analysis
        auto tokens = tokenize(source, filename);
        if (errorReporter.hasErrors()) {
            return nullptr;
        }
        
        // Parsing
        auto program = parse(tokens);
        if (errorReporter.hasErrors()) {
            return nullptr;
        }
        
        return program;
    } catch (const std::exception& e) {
        errorReporter.fatal(e.what(), SourceLocation(1, 1, filename));
        return nullptr;
    }
}

std::vector<Token> Compiler::tokenize(const std::string& source, const std::string& filename) {
    try {
        Lexer lexer(source, filename);
        return lexer.tokenize();
    } catch (const std::exception& e) {
        errorReporter.error("Lexical error: " + std::string(e.what()), 
                           SourceLocation(1, 1, filename));
        return {};
    }
}

std::unique_ptr<Program> Compiler::parse(const std::vector<Token>& tokens) {
    try {
        Parser parser(tokens, errorReporter);
        return parser.parse();
    } catch (const ParseError& e) {
        errorReporter.error("Parse error: " + std::string(e.what()), e.location);
        return nullptr;
    } catch (const std::exception& e) {
        errorReporter.error("Parse error: " + std::string(e.what()), 
                           SourceLocation(1, 1, ""));
        return nullptr;
    }
}

bool Compiler::generateExecutable(const std::string& source, const std::string& inputFilename, 
                                 const std::string& outputFilename) {
    try {
        // Parse the source code
        auto program = compile(source, inputFilename);
        if (!program || hasErrors()) {
            return false;
        }
        
#ifndef BUILD_WASM
        // Generate LLVM IR
        CodeGenerator codeGen(errorReporter, "meadows_program");
        auto module = codeGen.generateIR(*program);
        if (!module || hasErrors()) {
            return false;
        }
        
        // Generate object file
        std::string objectFile = outputFilename + ".o";
        if (!codeGen.generateObjectFile(objectFile)) {
            return false;
        }
        
        // Generate executable
        if (!codeGen.generateExecutable(objectFile, outputFilename)) {
            return false;
        }
        
        // Clean up object file
        std::remove(objectFile.c_str());
#else
        // WebAssembly build - no code generation
        errorReporter.error("Code generation not available in WebAssembly build", 
                           SourceLocation(1, 1, inputFilename));
        return false;
#endif
        
        return true;
    } catch (const std::exception& e) {
        errorReporter.fatal(e.what(), SourceLocation(1, 1, inputFilename));
        return false;
    }
}

void Compiler::printTokens(const std::vector<Token>& tokens) const {
    std::cout << "=== TOKENS ===" << std::endl;
    for (const auto& token : tokens) {
        if (token.type == TokenType::NEWLINE) {
            std::cout << "NEWLINE" << std::endl;
        } else if (token.type == TokenType::INDENT) {
            std::cout << "INDENT" << std::endl;
        } else if (token.type == TokenType::DEDENT) {
            std::cout << "DEDENT" << std::endl;
        } else if (token.type == TokenType::EOF_TOKEN) {
            std::cout << "EOF" << std::endl;
        } else {
            std::cout << token.toString() << std::endl;
        }
    }
    std::cout << std::endl;
}

void Compiler::printAST(const Program& program) const {
    std::cout << "=== AST ===" << std::endl;
    ASTPrinter printer;
    const_cast<Program&>(program).accept(printer);
    std::cout << printer.getOutput() << std::endl;
}

void Compiler::runTests() {
    std::vector<std::string> testCases = {
        // Simple expression
        "x = 42\n",
        
        // Function definition
        R"(def greet(name):
    print("Hello, " + name)
)",
        
        // If statement
        R"(if x > 0:
    print("positive")
else:
    print("non-positive")
)",
        
        // While loop
        R"(while i < 10:
    print(i)
    i = i + 1
)",
        
        // For loop
        R"(for item in items:
    process(item)
)",
        
        // Class definition
        R"(class Person:
    def __init__(self, name):
        self.name = name
    
    def greet(self):
        return "Hello, I'm " + self.name
)",
        
        // Complex expression
        "result = (a + b) * c ** 2 - func(x, y)\n",
        
        // Function with parameters
        R"(def calculate(x, y=10, z=None):
    if z is None:
        return x + y
    else:
        return x + y + z
)"
    };
    
    for (size_t i = 0; i < testCases.size(); i++) {
        std::cout << "=== Test Case " << (i + 1) << " ===" << std::endl;
        std::cout << "Source:" << std::endl;
        std::cout << testCases[i] << std::endl;
        
        clearErrors();
        auto program = compile(testCases[i], "test" + std::to_string(i + 1) + ".mds");
        
        if (hasErrors()) {
            std::cout << "Compilation failed:" << std::endl;
            errorReporter.printErrors();
        } else if (program) {
            printAST(*program);
        }
        
        std::cout << "===================" << std::endl << std::endl;
    }
}

} // namespace meadows
