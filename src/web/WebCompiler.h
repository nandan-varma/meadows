#pragma once

#include "../Compiler.h"
#include "../common/ErrorReporter.h"
#include <string>
#include <memory>

namespace meadows {

/**
 * WebAssembly-compatible interface for the Meadows compiler
 * Provides simplified JSON-based API for web integration
 */
class WebCompiler {
private:
    std::unique_ptr<Compiler> compiler;
    std::unique_ptr<ErrorReporter> errorReporter;

public:
    WebCompiler();
    
    /**
     * Compile source code and return JSON result
     * @param source The Meadows source code
     * @return JSON string containing compilation results
     */
    std::string compile(const std::string& source);

private:
    std::string tokensToJson(const std::vector<Token>& tokens);
    std::string astToJson(const Program& program);
    std::string irToString(llvm::Module& module);
    std::string simulateExecution(const std::string& source);
    std::string createSuccessResponse(const std::string& tokens, const std::string& ast, 
                                    const std::string& ir, const std::string& execution);
    std::string createErrorResponse(const std::string& message, 
                                   const std::vector<std::string>& errors = {});
    std::string escapeJson(const std::string& str);
    std::string tokenTypeToString(TokenType type);
};

} // namespace meadows
