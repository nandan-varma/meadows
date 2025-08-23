#include "../ast/Expression.h"
#include "../ast/Statement.h"
#include "../visitor/ASTVisitor.h"

namespace meadows {

// Expression accept methods
void IntegerLiteral::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void FloatLiteral::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void StringLiteral::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void BooleanLiteral::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void NoneLiteral::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void ListLiteral::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void Identifier::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void BinaryExpression::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void UnaryExpression::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void FunctionCall::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void AttributeAccess::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void IndexAccess::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void Assignment::accept(ASTVisitor& visitor) { visitor.visit(*this); }

// Statement accept methods
void ExpressionStatement::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void Block::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void IfStatement::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void WhileStatement::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void ForStatement::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void ReturnStatement::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void BreakStatement::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void ContinueStatement::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void PassStatement::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void FunctionDefinition::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void ClassDefinition::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void ImportStatement::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void Program::accept(ASTVisitor& visitor) { visitor.visit(*this); }

} // namespace meadows
