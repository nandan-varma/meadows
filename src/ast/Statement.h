#pragma once

#include "Expression.h"
#include <memory>
#include <vector>
#include <string>

namespace meadows {

// Expression statement
class ExpressionStatement : public Statement {
public:
    std::unique_ptr<Expression> expression;
    
    ExpressionStatement(std::unique_ptr<Expression> expression, const SourceLocation& location)
        : Statement(location), expression(std::move(expression)) {}
    
    void accept(ASTVisitor& visitor) override;
};

// Block statement
class Block : public Statement {
public:
    std::vector<std::unique_ptr<Statement>> statements;
    
    Block(std::vector<std::unique_ptr<Statement>> statements, const SourceLocation& location)
        : Statement(location), statements(std::move(statements)) {}
    
    void accept(ASTVisitor& visitor) override;
};

// If statement
class IfStatement : public Statement {
public:
    std::unique_ptr<Expression> condition;
    std::unique_ptr<Statement> thenBranch;
    std::unique_ptr<Statement> elseBranch; // nullable
    
    IfStatement(std::unique_ptr<Expression> condition, std::unique_ptr<Statement> thenBranch,
               std::unique_ptr<Statement> elseBranch, const SourceLocation& location)
        : Statement(location), condition(std::move(condition)), 
          thenBranch(std::move(thenBranch)), elseBranch(std::move(elseBranch)) {}
    
    void accept(ASTVisitor& visitor) override;
};

// While statement
class WhileStatement : public Statement {
public:
    std::unique_ptr<Expression> condition;
    std::unique_ptr<Statement> body;
    
    WhileStatement(std::unique_ptr<Expression> condition, std::unique_ptr<Statement> body,
                  const SourceLocation& location)
        : Statement(location), condition(std::move(condition)), body(std::move(body)) {}
    
    void accept(ASTVisitor& visitor) override;
};

// For statement
class ForStatement : public Statement {
public:
    std::string variable;
    std::unique_ptr<Expression> iterable;
    std::unique_ptr<Statement> body;
    
    ForStatement(const std::string& variable, std::unique_ptr<Expression> iterable,
                std::unique_ptr<Statement> body, const SourceLocation& location)
        : Statement(location), variable(variable), iterable(std::move(iterable)), 
          body(std::move(body)) {}
    
    void accept(ASTVisitor& visitor) override;
};

// Return statement
class ReturnStatement : public Statement {
public:
    std::unique_ptr<Expression> value; // nullable
    
    ReturnStatement(std::unique_ptr<Expression> value, const SourceLocation& location)
        : Statement(location), value(std::move(value)) {}
    
    void accept(ASTVisitor& visitor) override;
};

// Break statement
class BreakStatement : public Statement {
public:
    BreakStatement(const SourceLocation& location) : Statement(location) {}
    
    void accept(ASTVisitor& visitor) override;
};

// Continue statement
class ContinueStatement : public Statement {
public:
    ContinueStatement(const SourceLocation& location) : Statement(location) {}
    
    void accept(ASTVisitor& visitor) override;
};

// Pass statement
class PassStatement : public Statement {
public:
    PassStatement(const SourceLocation& location) : Statement(location) {}
    
    void accept(ASTVisitor& visitor) override;
};

// Function parameter
struct Parameter {
    std::string name;
    std::unique_ptr<Expression> defaultValue; // nullable
    SourceLocation location;
    
    Parameter(const std::string& name, std::unique_ptr<Expression> defaultValue, 
             const SourceLocation& location)
        : name(name), defaultValue(std::move(defaultValue)), location(location) {}
};

// Function definition
class FunctionDefinition : public Statement {
public:
    std::string name;
    std::vector<Parameter> parameters;
    std::unique_ptr<Statement> body;
    
    FunctionDefinition(const std::string& name, std::vector<Parameter> parameters,
                      std::unique_ptr<Statement> body, const SourceLocation& location)
        : Statement(location), name(name), parameters(std::move(parameters)), 
          body(std::move(body)) {}
    
    void accept(ASTVisitor& visitor) override;
};

// Class definition
class ClassDefinition : public Statement {
public:
    std::string name;
    std::vector<std::unique_ptr<Expression>> bases; // base classes
    std::unique_ptr<Statement> body;
    
    ClassDefinition(const std::string& name, std::vector<std::unique_ptr<Expression>> bases,
                   std::unique_ptr<Statement> body, const SourceLocation& location)
        : Statement(location), name(name), bases(std::move(bases)), body(std::move(body)) {}
    
    void accept(ASTVisitor& visitor) override;
};

// Import statement
class ImportStatement : public Statement {
public:
    std::vector<std::string> modules;
    std::vector<std::string> aliases; // same size as modules, empty string means no alias
    
    ImportStatement(std::vector<std::string> modules, std::vector<std::string> aliases,
                   const SourceLocation& location)
        : Statement(location), modules(std::move(modules)), aliases(std::move(aliases)) {}
    
    void accept(ASTVisitor& visitor) override;
};

// Program (root node)
class Program : public ASTNode {
public:
    std::vector<std::unique_ptr<Statement>> statements;
    
    Program(std::vector<std::unique_ptr<Statement>> statements, const SourceLocation& location)
        : ASTNode(location), statements(std::move(statements)) {}
    
    void accept(ASTVisitor& visitor) override;
};

} // namespace meadows
