#include "ErrorReporter.h"
#include <iomanip>

namespace meadows {

void ErrorReporter::reportError(ErrorLevel level, const std::string& message, 
                               const SourceLocation& location, const std::string& context) {
    errors.emplace_back(level, message, location, context);
    
    if (level == ErrorLevel::ERROR || level == ErrorLevel::FATAL) {
        hasErrors_ = true;
    }
    
    if (level == ErrorLevel::FATAL) {
        hasFatalErrors_ = true;
    }
}

void ErrorReporter::warning(const std::string& message, const SourceLocation& location, 
                           const std::string& context) {
    reportError(ErrorLevel::WARNING, message, location, context);
}

void ErrorReporter::error(const std::string& message, const SourceLocation& location, 
                         const std::string& context) {
    reportError(ErrorLevel::ERROR, message, location, context);
}

void ErrorReporter::fatal(const std::string& message, const SourceLocation& location, 
                         const std::string& context) {
    reportError(ErrorLevel::FATAL, message, location, context);
}

void ErrorReporter::printErrors(std::ostream& out) const {
    for (const auto& error : errors) {
        out << formatError(error) << std::endl;
        if (!error.context.empty()) {
            out << error.context << std::endl;
        }
        out << std::endl;
    }
    
    if (hasErrors_) {
        out << "Compilation failed with " << errors.size() << " error(s)." << std::endl;
    }
}

void ErrorReporter::clear() {
    errors.clear();
    hasErrors_ = false;
    hasFatalErrors_ = false;
}

std::string ErrorReporter::levelToString(ErrorLevel level) {
    switch (level) {
        case ErrorLevel::WARNING: return "Warning";
        case ErrorLevel::ERROR: return "Error";
        case ErrorLevel::FATAL: return "Fatal Error";
        default: return "Unknown";
    }
}

std::string ErrorReporter::formatError(const CompilerError& error) {
    std::string result = levelToString(error.level) + ": ";
    
    if (!error.location.filename.empty()) {
        result += error.location.filename + ":";
    }
    
    result += std::to_string(error.location.line) + ":" + 
              std::to_string(error.location.column) + ": " + error.message;
    
    return result;
}

} // namespace meadows
