#pragma once

#include "lexer/Lexer.h"
#include "parser/Parser.h"
#include "ast/Expression.h"
#include "ast/Statement.h"
#include "visitor/ASTPrinter.h"
#include "visitor/CodeGenerator.h"
#include "common/ErrorReporter.h"
#include <string>
#include <memory>

namespace meadows {

class Compiler {
private:
    ErrorReporter errorReporter;
    
public:
    Compiler() = default;
    
    // Compile from source code
    std::unique_ptr<Program> compile(const std::string& source, const std::string& filename = "");
    
    // Individual phases
    std::vector<Token> tokenize(const std::string& source, const std::string& filename = "");
    std::unique_ptr<Program> parse(const std::vector<Token>& tokens);
    
    // Code generation
    bool generateExecutable(const std::string& source, const std::string& inputFilename, 
                           const std::string& outputFilename);
    
    // Utility methods
    void printTokens(const std::vector<Token>& tokens) const;
    void printAST(const Program& program) const;
    
    // Error handling
    const ErrorReporter& getErrorReporter() const { return errorReporter; }
    void clearErrors() { errorReporter.clear(); }
    bool hasErrors() const { return errorReporter.hasErrors(); }
    
    // Test different source examples
    void runTests();
};

} // namespace meadows
