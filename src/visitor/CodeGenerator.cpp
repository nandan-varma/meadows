#include "CodeGenerator.h"
#include "../ast/Expression.h"
#include "../ast/Statement.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Program.h"
#include "llvm/TargetParser/Host.h"
#include "llvm/Transforms/Utils/Cloning.h"
#include <iostream>
#include <cstdlib>

namespace meadows {

CodeGenerator::CodeGenerator(ErrorReporter& errorReporter, const std::string& moduleName)
    : errorReporter(errorReporter), currentFunction(nullptr), lastValue(nullptr) {
    context = std::make_unique<llvm::LLVMContext>();
    module = std::make_unique<llvm::Module>(moduleName, *context);
    builder = std::make_unique<llvm::IRBuilder<>>(*context);
}

llvm::Type* CodeGenerator::getInt32Type() {
    return llvm::Type::getInt32Ty(*context);
}

llvm::Type* CodeGenerator::getDoubleType() {
    return llvm::Type::getDoubleTy(*context);
}

llvm::Type* CodeGenerator::getInt8PtrType() {
    return llvm::PointerType::get(*context, 0);
}

llvm::Type* CodeGenerator::getBoolType() {
    return llvm::Type::getInt1Ty(*context);
}

llvm::Function* CodeGenerator::getOrCreatePrintfFunction() {
    llvm::Function* printfFunc = module->getFunction("printf");
    if (!printfFunc) {
        llvm::FunctionType* printfType = llvm::FunctionType::get(
            getInt32Type(),
            {getInt8PtrType()},
            true // varargs
        );
        printfFunc = llvm::Function::Create(
            printfType,
            llvm::Function::ExternalLinkage,
            "printf",
            module.get()
        );
    }
    return printfFunc;
}

llvm::Value* CodeGenerator::createPrintfCall(const std::string& format, const std::vector<llvm::Value*>& args) {
    llvm::Function* printfFunc = getOrCreatePrintfFunction();
    
    // Create format string constant using modern approach
    llvm::Constant* formatStr = llvm::ConstantDataArray::getString(
        module->getContext(), format, true);
    llvm::GlobalVariable* formatGlobal = new llvm::GlobalVariable(
        *module, formatStr->getType(), true,
        llvm::GlobalValue::PrivateLinkage, formatStr, ".str");
    formatGlobal->setUnnamedAddr(llvm::GlobalValue::UnnamedAddr::Global);
    
    // Get pointer to the string
    llvm::Value* formatPtr = builder->CreateConstGEP2_32(
        formatStr->getType(), formatGlobal, 0, 0, "");
    
    // Prepare arguments
    std::vector<llvm::Value*> printfArgs;
    printfArgs.push_back(formatPtr);
    printfArgs.insert(printfArgs.end(), args.begin(), args.end());
    
    return builder->CreateCall(printfFunc, printfArgs);
}

llvm::Value* CodeGenerator::convertToDouble(llvm::Value* value) {
    if (value->getType()->isDoubleTy()) {
        return value;
    } else if (value->getType()->isIntegerTy(32)) {
        return builder->CreateSIToFP(value, getDoubleType());
    } else if (value->getType()->isIntegerTy(1)) {
        llvm::Value* intVal = builder->CreateZExt(value, getInt32Type());
        return builder->CreateSIToFP(intVal, getDoubleType());
    }
    return value;
}

llvm::Value* CodeGenerator::convertToInt(llvm::Value* value) {
    if (value->getType()->isIntegerTy(32)) {
        return value;
    } else if (value->getType()->isDoubleTy()) {
        return builder->CreateFPToSI(value, getInt32Type());
    } else if (value->getType()->isIntegerTy(1)) {
        return builder->CreateZExt(value, getInt32Type());
    }
    return value;
}

std::unique_ptr<llvm::Module> CodeGenerator::generateIR(Program& program) {
    
    program.accept(*this);
    
    // Verify the module
    std::string errorStr;
    llvm::raw_string_ostream errorStream(errorStr);
    if (llvm::verifyModule(*module, &errorStream)) {
        errorReporter.error("Module verification failed: " + errorStr, 
                           SourceLocation(1, 1, ""));
        return nullptr;
    }
    
    // Create a clone of the module to return, keeping the original
    auto clonedModule = llvm::CloneModule(*module);
    return clonedModule;
}

bool CodeGenerator::generateObjectFile(const std::string& filename) {
    try {
        // Generate IR file instead of object file directly
        std::string irFilename = filename + ".ll";
        std::error_code errorCode;
        llvm::raw_fd_ostream dest(irFilename, errorCode, llvm::sys::fs::OF_None);
        if (errorCode) {
            errorReporter.error("Could not create IR file: " + errorCode.message(), 
                               SourceLocation(1, 1, ""));
            return false;
        }
        
        // Print the module as LLVM IR
        module->print(dest, nullptr);
        dest.flush();
        dest.close();
        
        // Use clang to compile IR to object file
        std::vector<llvm::StringRef> args = {
            "clang",
            "-c",
            irFilename,
            "-o", filename
        };
        
        std::string errorMsg;
        int result = llvm::sys::ExecuteAndWait(
            llvm::sys::findProgramByName("clang").get(),
            args,
            std::nullopt,
            {},
            0,
            0,
            &errorMsg
        );
        
        // Clean up IR file
        std::remove(irFilename.c_str());
        
        if (result != 0) {
            errorReporter.error("Failed to compile IR to object file: " + errorMsg, 
                               SourceLocation(1, 1, ""));
            return false;
        }
        
        return true;
        
    } catch (const std::exception& e) {
        errorReporter.error("Exception in generateObjectFile: " + std::string(e.what()), 
                           SourceLocation(1, 1, ""));
        return false;
    } catch (...) {
        errorReporter.error("Unknown exception in generateObjectFile", 
                           SourceLocation(1, 1, ""));
        return false;
    }
}

bool CodeGenerator::generateExecutable(const std::string& objectFile, const std::string& executableFile) {
    // Use clang to link the object file
    std::vector<llvm::StringRef> args = {
        "clang",
        objectFile,
        "-o", executableFile
    };
    
    std::string errorMsg;
    int result = llvm::sys::ExecuteAndWait(
        llvm::sys::findProgramByName("clang").get(),
        args,
        std::nullopt,
        {},
        0,
        0,
        &errorMsg
    );
    
    if (result != 0) {
        errorReporter.error("Failed to link executable: " + errorMsg, 
                           SourceLocation(1, 1, ""));
        return false;
    }
    
    return true;
}

void CodeGenerator::printIR() {
    module->print(llvm::outs(), nullptr);
}

// Expression visitors
void CodeGenerator::visit(IntegerLiteral& node) {
    lastValue = llvm::ConstantInt::get(getInt32Type(), node.value);
}

void CodeGenerator::visit(FloatLiteral& node) {
    lastValue = llvm::ConstantFP::get(getDoubleType(), node.value);
}

void CodeGenerator::visit(StringLiteral& node) {
    // Create string constant using modern approach
    llvm::Constant* strConstant = llvm::ConstantDataArray::getString(
        module->getContext(), node.value, true);
    llvm::GlobalVariable* strGlobal = new llvm::GlobalVariable(
        *module, strConstant->getType(), true,
        llvm::GlobalValue::PrivateLinkage, strConstant, ".str");
    strGlobal->setUnnamedAddr(llvm::GlobalValue::UnnamedAddr::Global);
    
    // Get pointer to the string
    lastValue = builder->CreateConstGEP2_32(
        strConstant->getType(), strGlobal, 0, 0, "");
}

void CodeGenerator::visit(BooleanLiteral& node) {
    lastValue = llvm::ConstantInt::get(getBoolType(), node.value ? 1 : 0);
}

void CodeGenerator::visit(NoneLiteral& node) {
    lastValue = llvm::ConstantInt::get(getInt32Type(), 0);
}

void CodeGenerator::visit(Identifier& node) {
    llvm::Value* value = namedValues[node.name];
    if (!value) {
        errorReporter.error("Unknown variable: " + node.name, node.location);
        lastValue = nullptr;
        return;
    }
    
    // If it's an alloca, load the value
    if (llvm::isa<llvm::AllocaInst>(value)) {
        // In LLVM 20+, we need to specify the load type explicitly
        // Get the allocated type from the alloca instruction
        llvm::AllocaInst* allocaInst = llvm::cast<llvm::AllocaInst>(value);
        llvm::Type* loadType = allocaInst->getAllocatedType();
        lastValue = builder->CreateLoad(loadType, value, node.name);
    } else {
        lastValue = value;
    }
}

void CodeGenerator::visit(BinaryExpression& node) {
    node.left->accept(*this);
    llvm::Value* left = lastValue;
    
    node.right->accept(*this);
    llvm::Value* right = lastValue;
    
    if (!left || !right) {
        lastValue = nullptr;
        return;
    }
    
    // Handle arithmetic operations
    switch (node.operator_) {
        case BinaryOp::ADD:
            if (left->getType()->isDoubleTy() || right->getType()->isDoubleTy()) {
                left = convertToDouble(left);
                right = convertToDouble(right);
                lastValue = builder->CreateFAdd(left, right, "addtmp");
            } else {
                lastValue = builder->CreateAdd(left, right, "addtmp");
            }
            break;
        case BinaryOp::SUBTRACT:
            if (left->getType()->isDoubleTy() || right->getType()->isDoubleTy()) {
                left = convertToDouble(left);
                right = convertToDouble(right);
                lastValue = builder->CreateFSub(left, right, "subtmp");
            } else {
                lastValue = builder->CreateSub(left, right, "subtmp");
            }
            break;
        case BinaryOp::MULTIPLY:
            if (left->getType()->isDoubleTy() || right->getType()->isDoubleTy()) {
                left = convertToDouble(left);
                right = convertToDouble(right);
                lastValue = builder->CreateFMul(left, right, "multmp");
            } else {
                lastValue = builder->CreateMul(left, right, "multmp");
            }
            break;
        case BinaryOp::DIVIDE:
            left = convertToDouble(left);
            right = convertToDouble(right);
            lastValue = builder->CreateFDiv(left, right, "divtmp");
            break;
        case BinaryOp::MODULO:
            left = convertToInt(left);
            right = convertToInt(right);
            lastValue = builder->CreateSRem(left, right, "modtmp");
            break;
        case BinaryOp::EQUAL:
            if (left->getType()->isDoubleTy() || right->getType()->isDoubleTy()) {
                left = convertToDouble(left);
                right = convertToDouble(right);
                lastValue = builder->CreateFCmpOEQ(left, right, "eqtmp");
            } else {
                lastValue = builder->CreateICmpEQ(left, right, "eqtmp");
            }
            break;
        case BinaryOp::NOT_EQUAL:
            if (left->getType()->isDoubleTy() || right->getType()->isDoubleTy()) {
                left = convertToDouble(left);
                right = convertToDouble(right);
                lastValue = builder->CreateFCmpONE(left, right, "netmp");
            } else {
                lastValue = builder->CreateICmpNE(left, right, "netmp");
            }
            break;
        case BinaryOp::LESS_THAN:
            if (left->getType()->isDoubleTy() || right->getType()->isDoubleTy()) {
                left = convertToDouble(left);
                right = convertToDouble(right);
                lastValue = builder->CreateFCmpOLT(left, right, "lttmp");
            } else {
                lastValue = builder->CreateICmpSLT(left, right, "lttmp");
            }
            break;
        case BinaryOp::LESS_EQUAL:
            if (left->getType()->isDoubleTy() || right->getType()->isDoubleTy()) {
                left = convertToDouble(left);
                right = convertToDouble(right);
                lastValue = builder->CreateFCmpOLE(left, right, "letmp");
            } else {
                lastValue = builder->CreateICmpSLE(left, right, "letmp");
            }
            break;
        case BinaryOp::GREATER_THAN:
            if (left->getType()->isDoubleTy() || right->getType()->isDoubleTy()) {
                left = convertToDouble(left);
                right = convertToDouble(right);
                lastValue = builder->CreateFCmpOGT(left, right, "gttmp");
            } else {
                lastValue = builder->CreateICmpSGT(left, right, "gttmp");
            }
            break;
        case BinaryOp::GREATER_EQUAL:
            if (left->getType()->isDoubleTy() || right->getType()->isDoubleTy()) {
                left = convertToDouble(left);
                right = convertToDouble(right);
                lastValue = builder->CreateFCmpOGE(left, right, "getmp");
            } else {
                lastValue = builder->CreateICmpSGE(left, right, "getmp");
            }
            break;
        case BinaryOp::AND:
            left = convertToInt(left);
            right = convertToInt(right);
            lastValue = builder->CreateAnd(left, right, "andtmp");
            break;
        case BinaryOp::OR:
            left = convertToInt(left);
            right = convertToInt(right);
            lastValue = builder->CreateOr(left, right, "ortmp");
            break;
        default:
            errorReporter.error("Unsupported binary operator", node.location);
            lastValue = nullptr;
    }
}

void CodeGenerator::visit(UnaryExpression& node) {
    node.operand->accept(*this);
    llvm::Value* operand = lastValue;
    
    if (!operand) {
        lastValue = nullptr;
        return;
    }
    
    switch (node.operator_) {
        case UnaryOp::MINUS:
            if (operand->getType()->isDoubleTy()) {
                lastValue = builder->CreateFNeg(operand, "negtmp");
            } else {
                lastValue = builder->CreateNeg(operand, "negtmp");
            }
            break;
        case UnaryOp::NOT:
            operand = convertToInt(operand);
            lastValue = builder->CreateNot(operand, "nottmp");
            break;
        default:
            errorReporter.error("Unsupported unary operator", node.location);
            lastValue = nullptr;
    }
}

void CodeGenerator::visit(FunctionCall& node) {
    // Handle built-in print function
    if (auto identifierFunc = dynamic_cast<Identifier*>(node.function.get())) {
        if (identifierFunc->name == "print") {
            std::vector<llvm::Value*> args;
            std::string format;
            
            for (size_t i = 0; i < node.arguments.size(); i++) {
                node.arguments[i]->accept(*this);
                if (lastValue) {
                    if (i > 0) format += " ";
                    
                    if (lastValue->getType()->isIntegerTy(32)) {
                        format += "%d";
                        args.push_back(lastValue);
                    } else if (lastValue->getType()->isDoubleTy()) {
                        format += "%f";
                        args.push_back(lastValue);
                    } else if (lastValue->getType()->isPointerTy()) {
                        format += "%s";
                        args.push_back(lastValue);
                    }
                }
            }
            format += "\n";
            
            lastValue = createPrintfCall(format, args);
            return;
        }
    }
    
    // Handle user-defined functions
    if (auto identifierFunc = dynamic_cast<Identifier*>(node.function.get())) {
        llvm::Function* function = functions[identifierFunc->name];
        if (!function) {
            errorReporter.error("Unknown function: " + identifierFunc->name, node.location);
            lastValue = nullptr;
            return;
        }
        
        if (function->arg_size() != node.arguments.size()) {
            errorReporter.error("Incorrect number of arguments", node.location);
            lastValue = nullptr;
            return;
        }
        
        std::vector<llvm::Value*> args;
        for (auto& arg : node.arguments) {
            arg->accept(*this);
            if (lastValue) {
                args.push_back(lastValue);
            }
        }
        
        lastValue = builder->CreateCall(function, args);
    } else {
        errorReporter.error("Function calls only supported for identifiers", node.location);
        lastValue = nullptr;
    }
}

void CodeGenerator::visit(AttributeAccess& node) {
    // Simplified attribute access - just treat as identifier for now
    errorReporter.error("Attribute access not yet implemented in code generation", node.location);
    lastValue = nullptr;
}

void CodeGenerator::visit(IndexAccess& node) {
    errorReporter.error("Index access not yet implemented in code generation", node.location);
    lastValue = nullptr;
}

void CodeGenerator::visit(Assignment& node) {
    if (auto identifier = dynamic_cast<Identifier*>(node.target.get())) {
        node.value->accept(*this);
        llvm::Value* value = lastValue;
        
        if (!value) {
            return;
        }
        
        // Check if variable already exists
        llvm::Value* variable = namedValues[identifier->name];
        if (!variable) {
            // Create new alloca
            llvm::Function* function = builder->GetInsertBlock()->getParent();
            if (!function) {
                errorReporter.error("No current function for alloca", node.location);
                return;
            }
            llvm::IRBuilder<> tmpBuilder(&function->getEntryBlock(), function->getEntryBlock().begin());
            variable = tmpBuilder.CreateAlloca(value->getType(), nullptr, identifier->name);
            namedValues[identifier->name] = variable;
        }
        
        builder->CreateStore(value, variable);
        lastValue = value;
    } else {
        errorReporter.error("Assignment only supported for identifiers", node.location);
        lastValue = nullptr;
    }
}

// Statement visitors
void CodeGenerator::visit(ExpressionStatement& node) {
    node.expression->accept(*this);
}

void CodeGenerator::visit(Block& node) {
    for (auto& stmt : node.statements) {
        // Skip remaining statements if current block is already terminated
        if (builder->GetInsertBlock()->getTerminator()) {
            break;
        }
        stmt->accept(*this);
    }
}

void CodeGenerator::visit(IfStatement& node) {
    node.condition->accept(*this);
    llvm::Value* condition = lastValue;
    
    if (!condition) {
        return;
    }
    
    // Convert condition to boolean
    if (!condition->getType()->isIntegerTy(1)) {
        condition = convertToInt(condition);
        condition = builder->CreateICmpNE(condition, llvm::ConstantInt::get(getInt32Type(), 0), "ifcond");
    }
    
    llvm::Function* function = builder->GetInsertBlock()->getParent();
    
    llvm::BasicBlock* thenBB = llvm::BasicBlock::Create(*context, "then", function);
    llvm::BasicBlock* elseBB = node.elseBranch ? llvm::BasicBlock::Create(*context, "else") : nullptr;
    llvm::BasicBlock* mergeBB = llvm::BasicBlock::Create(*context, "ifcont");
    
    if (elseBB) {
        builder->CreateCondBr(condition, thenBB, elseBB);
    } else {
        builder->CreateCondBr(condition, thenBB, mergeBB);
    }
    
    // Generate then block
    builder->SetInsertPoint(thenBB);
    node.thenBranch->accept(*this);
    // Only add branch if block isn't already terminated
    if (!builder->GetInsertBlock()->getTerminator()) {
        builder->CreateBr(mergeBB);
    }
    
    // Generate else block if it exists
    if (elseBB) {
        function->insert(function->end(), elseBB);
        builder->SetInsertPoint(elseBB);
        node.elseBranch->accept(*this);
        // Only add branch if block isn't already terminated
        if (!builder->GetInsertBlock()->getTerminator()) {
            builder->CreateBr(mergeBB);
        }
    }
    
    // Generate merge block
    function->insert(function->end(), mergeBB);
    builder->SetInsertPoint(mergeBB);
}

void CodeGenerator::visit(WhileStatement& node) {
    llvm::Function* function = builder->GetInsertBlock()->getParent();
    
    llvm::BasicBlock* condBB = llvm::BasicBlock::Create(*context, "whilecond", function);
    llvm::BasicBlock* bodyBB = llvm::BasicBlock::Create(*context, "whilebody");
    llvm::BasicBlock* afterBB = llvm::BasicBlock::Create(*context, "afterwhile");
    
    builder->CreateBr(condBB);
    
    // Condition block
    builder->SetInsertPoint(condBB);
    node.condition->accept(*this);
    llvm::Value* condition = lastValue;
    
    if (!condition) {
        return;
    }
    
    if (!condition->getType()->isIntegerTy(1)) {
        condition = convertToInt(condition);
        condition = builder->CreateICmpNE(condition, llvm::ConstantInt::get(getInt32Type(), 0), "whilecond");
    }
    
    builder->CreateCondBr(condition, bodyBB, afterBB);
    
    // Body block
    function->insert(function->end(), bodyBB);
    builder->SetInsertPoint(bodyBB);
    node.body->accept(*this);
    builder->CreateBr(condBB);
    
    // After block
    function->insert(function->end(), afterBB);
    builder->SetInsertPoint(afterBB);
}

void CodeGenerator::visit(ForStatement& node) {
    errorReporter.error("For loops not yet implemented in code generation", node.location);
}

void CodeGenerator::visit(ReturnStatement& node) {
    if (node.value) {
        node.value->accept(*this);
        if (lastValue) {
            builder->CreateRet(lastValue);
        }
    } else {
        builder->CreateRetVoid();
    }
}

void CodeGenerator::visit(BreakStatement& node) {
    errorReporter.error("Break statements not yet implemented in code generation", node.location);
}

void CodeGenerator::visit(ContinueStatement& node) {
    errorReporter.error("Continue statements not yet implemented in code generation", node.location);
}

void CodeGenerator::visit(PassStatement& node) {
    // Pass statements do nothing
}

void CodeGenerator::visit(FunctionDefinition& node) {
    // Create function type
    std::vector<llvm::Type*> paramTypes;
    for (const auto& param : node.parameters) {
        paramTypes.push_back(getInt32Type()); // Simplified: all parameters are int32
    }
    
    llvm::FunctionType* funcType = llvm::FunctionType::get(getInt32Type(), paramTypes, false);
    llvm::Function* function = llvm::Function::Create(funcType, llvm::Function::ExternalLinkage, 
                                                     node.name, module.get());
    
    functions[node.name] = function;
    
    // Set parameter names and create entry block
    auto argIt = function->arg_begin();
    for (const auto& param : node.parameters) {
        argIt->setName(param.name);
        ++argIt;
    }
    
    llvm::BasicBlock* entryBB = llvm::BasicBlock::Create(*context, "entry", function);
    builder->SetInsertPoint(entryBB);
    
    // Save previous state
    auto prevNamedValues = namedValues;
    llvm::Function* prevFunction = currentFunction;
    currentFunction = function;
    
    // Create allocas for parameters
    argIt = function->arg_begin();
    for (const auto& param : node.parameters) {
        llvm::AllocaInst* alloca = builder->CreateAlloca(getInt32Type(), nullptr, param.name);
        builder->CreateStore(&*argIt, alloca);
        namedValues[param.name] = alloca;
        ++argIt;
    }
    
    // Generate function body
    node.body->accept(*this);
    
    // Add return if not present
    if (!builder->GetInsertBlock()->getTerminator()) {
        builder->CreateRet(llvm::ConstantInt::get(getInt32Type(), 0));
    }
    
    // Restore previous state
    namedValues = prevNamedValues;
    currentFunction = prevFunction;
}

void CodeGenerator::visit(ClassDefinition& node) {
    errorReporter.error("Class definitions not yet implemented in code generation", node.location);
}

void CodeGenerator::visit(ImportStatement& node) {
    // Import statements are ignored in code generation
}

void CodeGenerator::visit(Program& node) {
    // Generate all function definitions first
    for (auto& stmt : node.statements) {
        if (auto funcDef = dynamic_cast<FunctionDefinition*>(stmt.get())) {
            funcDef->accept(*this);
        }
    }
    
    // Create main function
    llvm::FunctionType* mainType = llvm::FunctionType::get(getInt32Type(), {}, false);
    llvm::Function* mainFunc = llvm::Function::Create(mainType, llvm::Function::ExternalLinkage, 
                                                     "main", module.get());
    
    llvm::BasicBlock* entryBB = llvm::BasicBlock::Create(*context, "entry", mainFunc);
    builder->SetInsertPoint(entryBB);
    
    currentFunction = mainFunc;
    
    // Generate all non-function statements in main
    for (auto& stmt : node.statements) {
        if (!dynamic_cast<FunctionDefinition*>(stmt.get()) && 
            !dynamic_cast<ClassDefinition*>(stmt.get()) &&
            !dynamic_cast<ImportStatement*>(stmt.get())) {
            stmt->accept(*this);
        }
    }
    
    // Return 0 from main
    builder->CreateRet(llvm::ConstantInt::get(getInt32Type(), 0));
}

} // namespace meadows
