#include "Compiler.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <getopt.h>

using namespace meadows;

std::string readFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file " << filename << std::endl;
        return "";
    }
    
    std::ostringstream content;
    content << file.rdbuf();
    return content.str();
}

void printUsage(const char* programName) {
    std::cout << "Usage: " << programName << " [options] [input_file]\n";
    std::cout << "Options:\n";
    std::cout << "  -i, --input <file>    Input source file\n";
    std::cout << "  -o, --output <file>   Output executable file\n";
    std::cout << "  -c, --compile         Compile to executable (default if -o specified)\n";
    std::cout << "  -p, --parse-only      Parse and show AST only\n";
    std::cout << "  -t, --tokens          Show tokenization output\n";
    std::cout << "  -h, --help           Show this help message\n";
    std::cout << "\nExamples:\n";
    std::cout << "  " << programName << " -i example.py -o example    # Compile to executable\n";
    std::cout << "  " << programName << " example.py                  # Parse and show AST\n";
    std::cout << "  " << programName << "                             # Run demo\n";
}

void demonstrateCompiler() {
    std::cout << "=== Meadows Compiler Demo ===" << std::endl;
    std::cout << "A Python-like language compiler frontend" << std::endl << std::endl;
    
    Compiler compiler;
    
    // Simple interactive example
    std::string source = R"(# Simple function definition and call
def fibonacci(n):
    if n <= 1:
        return n
    else:
        return fibonacci(n - 1) + fibonacci(n - 2)

# Class definition
class Calculator:
    def __init__(self, initial_value=0):
        self.value = initial_value
    
    def add(self, x):
        self.value = self.value + x
        return self.value
    
    def multiply(self, x):
        self.value = self.value * x
        return self.value

# Usage
calc = Calculator(10)
result = calc.add(5).multiply(2)
print("Result:", result)

# Loop example
for i in range(10):
    if i % 2 == 0:
        print("Even:", i)
    else:
        print("Odd:", i)
)";
    
    std::cout << "Compiling source code:" << std::endl;
    std::cout << "----------------------------------------" << std::endl;
    std::cout << source << std::endl;
    std::cout << "----------------------------------------" << std::endl << std::endl;
    
    // Compile the source
    auto program = compiler.compile(source, "demo.py");
    
    if (compiler.hasErrors()) {
        std::cout << "Compilation failed with errors:" << std::endl;
        compiler.getErrorReporter().printErrors();
    } else if (program) {
        std::cout << "Compilation successful!" << std::endl << std::endl;
        
        // Print the AST
        compiler.printAST(*program);
        
        std::cout << "Tokenization output:" << std::endl;
        auto tokens = compiler.tokenize(source, "demo.py");
        compiler.printTokens(tokens);
    }
}

int main(int argc, char* argv[]) {
    std::string inputFile;
    std::string outputFile;
    bool compileMode = false;
    bool parseOnly = false;
    bool showTokens = false;
    
    // Command line options
    static struct option long_options[] = {
        {"input",      required_argument, 0, 'i'},
        {"output",     required_argument, 0, 'o'},
        {"compile",    no_argument,       0, 'c'},
        {"parse-only", no_argument,       0, 'p'},
        {"tokens",     no_argument,       0, 't'},
        {"help",       no_argument,       0, 'h'},
        {0, 0, 0, 0}
    };
    
    int opt;
    while ((opt = getopt_long(argc, argv, "i:o:cpth", long_options, nullptr)) != -1) {
        switch (opt) {
            case 'i':
                inputFile = optarg;
                break;
            case 'o':
                outputFile = optarg;
                compileMode = true;
                break;
            case 'c':
                compileMode = true;
                break;
            case 'p':
                parseOnly = true;
                break;
            case 't':
                showTokens = true;
                break;
            case 'h':
                printUsage(argv[0]);
                return 0;
            default:
                printUsage(argv[0]);
                return 1;
        }
    }
    
    // Handle positional argument
    if (optind < argc && inputFile.empty()) {
        inputFile = argv[optind];
    }
    
    // If no input file specified, run demo
    if (inputFile.empty()) {
        demonstrateCompiler();
        
        std::cout << std::endl << "=== Running Test Cases ===" << std::endl;
        Compiler testCompiler;
        testCompiler.runTests();
        return 0;
    }
    
    // Read input file
    std::string source = readFile(inputFile);
    if (source.empty()) {
        return 1;
    }
    
    Compiler compiler;
    
    // Compilation mode
    if (compileMode) {
        if (outputFile.empty()) {
            // Default output file name
            size_t lastDot = inputFile.find_last_of('.');
            outputFile = (lastDot != std::string::npos) ? 
                        inputFile.substr(0, lastDot) : inputFile;
        }
        
        std::cout << "Compiling " << inputFile << " to " << outputFile << "..." << std::endl;
        
        if (compiler.generateExecutable(source, inputFile, outputFile)) {
            std::cout << "Successfully compiled to " << outputFile << std::endl;
            return 0;
        } else {
            std::cout << "Compilation failed:" << std::endl;
            compiler.getErrorReporter().printErrors();
            return 1;
        }
    }
    
    // Parse-only mode (default)
    auto program = compiler.compile(source, inputFile);
    
    if (compiler.hasErrors()) {
        compiler.getErrorReporter().printErrors();
        return 1;
    } else if (program) {
        std::cout << "Successfully parsed " << inputFile << std::endl;
        
        if (showTokens) {
            auto tokens = compiler.tokenize(source, inputFile);
            compiler.printTokens(tokens);
        }
        
        if (!compileMode) {
            compiler.printAST(*program);
        }
    }
    
    return 0;
}
