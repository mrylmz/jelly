//
// MIT License
//
// Copyright (c) 2018 Murat Yilmaz
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//

#include "AST/ArrayType.h"
#include "AST/Context.h"
#include "AST/EnumerationType.h"
#include "AST/ErrorType.h"
#include "AST/FloatType.h"
#include "AST/FunctionType.h"
#include "AST/IntegerType.h"
#include "AST/ModuleDeclaration.h"
#include "AST/PointerType.h"
#include "AST/Scope.h"
#include "AST/StructureType.h"
#include "AST/VoidType.h"

using namespace jelly::AST;

Context::Context() {
    module = new (this) ModuleDeclaration(getIdentifier(""), {});

    auto symbolTable = module->getScope()->getSymbolTable();
    errorType = symbolTable->createUniqueSymbol();
    errorType->setType(new (this) ErrorType());

    voidType = symbolTable->getOrInsert("Void");
    voidType->setType(new (this) VoidType());

    boolType = symbolTable->getOrInsert("Bool");
    boolType->setType(new (this) IntegerType(true, 1));

    int8Type = symbolTable->getOrInsert("Int8");
    int8Type->setType(new (this) IntegerType(true, 8));

    int16Type = symbolTable->getOrInsert("Int16");
    int16Type->setType(new (this) IntegerType(true, 16));

    int32Type = symbolTable->getOrInsert("Int32");
    int32Type->setType(new (this) IntegerType(true, 32));

    int64Type = symbolTable->getOrInsert("Int64");
    int64Type->setType(new (this) IntegerType(true, 64));

    int128Type = symbolTable->getOrInsert("Int128");
    int128Type->setType(new (this) IntegerType(true, 128));

    intType = symbolTable->getOrInsert("Int");
    intType->setType(int64Type->getType());

    uint8Type = symbolTable->getOrInsert("UInt8");
    uint8Type->setType(new (this) IntegerType(false, 8));

    uint16Type = symbolTable->getOrInsert("UInt16");
    uint16Type->setType(new (this) IntegerType(false, 16));

    uint32Type = symbolTable->getOrInsert("UInt32");
    uint32Type->setType(new (this) IntegerType(false, 32));

    uint64Type = symbolTable->getOrInsert("UInt64");
    uint64Type->setType(new (this) IntegerType(false, 64));

    uint128Type = symbolTable->getOrInsert("UInt128");
    uint128Type->setType(new (this) IntegerType(false, 128));

    uintType = symbolTable->getOrInsert("UInt");
    uintType->setType(uint64Type->getType());

    float16Type = symbolTable->getOrInsert("Float16");
    float16Type->setType(new (this) FloatType(FloatType::Kind::Float16));

    float32Type = symbolTable->getOrInsert("Float32");
    float32Type->setType(new (this) FloatType(FloatType::Kind::Float32));

    float64Type = symbolTable->getOrInsert("Float64");
    float64Type->setType(new (this) FloatType(FloatType::Kind::Float64));

    float80Type = symbolTable->getOrInsert("Float80");
    float80Type->setType(new (this) FloatType(FloatType::Kind::Float80));

    float128Type = symbolTable->getOrInsert("Float128");
    float128Type->setType(new (this) FloatType(FloatType::Kind::Float128));

    floatType = symbolTable->getOrInsert("Float");
    floatType->setType(float64Type->getType());

    // MARK: - Prefix Operators
    registerOperator(Operator::LogicalNot);
    registerOperator(Operator::BitwiseNot);
    registerOperator(Operator::UnaryPlus);
    registerOperator(Operator::UnaryMinus);

    // MARK: - Infix Operators
    registerOperator(Operator::BitwiseLeftShift);
    registerOperator(Operator::BitwiseRightShift);
    registerOperator(Operator::Multiply);
    registerOperator(Operator::Divide);
    registerOperator(Operator::Reminder);
    registerOperator(Operator::BitwiseAnd);
    registerOperator(Operator::Add);
    registerOperator(Operator::Subtract);
    registerOperator(Operator::BitwiseOr);
    registerOperator(Operator::BitwiseXor);
    registerOperator(Operator::TypeCheck);
    registerOperator(Operator::TypeCast);
    registerOperator(Operator::LessThan);
    registerOperator(Operator::LessThanEqual);
    registerOperator(Operator::GreaterThan);
    registerOperator(Operator::GreaterThanEqual);
    registerOperator(Operator::Equal);
    registerOperator(Operator::NotEqual);
    registerOperator(Operator::LogicalAnd);
    registerOperator(Operator::LogicalOr);
    registerOperator(Operator::Assign);
    registerOperator(Operator::MultiplyAssign);
    registerOperator(Operator::DivideAssign);
    registerOperator(Operator::ReminderAssign);
    registerOperator(Operator::AddAssign);
    registerOperator(Operator::SubtractAssign);
    registerOperator(Operator::BitwiseLeftShiftAssign);
    registerOperator(Operator::BitwiseRightShiftAssign);
    registerOperator(Operator::BitwiseAndAssign);
    registerOperator(Operator::BitwiseOrAssign);
    registerOperator(Operator::BitwiseXorAssign);

    // MARK: - Postfix Operators
    registerOperator(Operator::Selector);
    registerOperator(Operator::TypePointer);
    registerOperator(Operator::Call);
    registerOperator(Operator::Subscript);
}

void Context::registerOperator(Operator op) {
    assert(!op.getSymbol().equals("->") && "'->' is reserved and cannot be added as operator!");
    assert(!op.getSymbol().equals("//") && "'//' is reserved and cannot be added as operator!");
    assert(!op.getSymbol().equals("/*") && "'/*' is reserved and cannot be added as operator!");
    assert(!op.getSymbol().equals("*/") && "'*/' is reserved and cannot be added as operator!");

    // TODO: Add check for can have arguments for postfix operators (), [] ...
    bool success = operators[(uint8_t)op.getFixity()].try_emplace(op.getSymbol(), op).second;
    if (!success) {
        report_fatal_error("Internal compiler error!");
    }

    operatorPrecedenceSet.insert(op.getPrecedence());
}

Identifier Context::getIdentifier(StringRef text) {
    auto it = identifiers.find(text);
    if (it != identifiers.end()) {
        return it->getValue();
    }

    auto key = text.copy(allocator);
    auto identifier = Identifier(identifiers.getNumItems() + 1, key);
    identifiers.try_emplace(key, identifier);
    return identifier;
}

ModuleDeclaration* Context::getModule() {
    return module;
}

bool Context::getOperator(StringRef name, Fixity fixity, Operator& op) {
    auto it = operators[(uint8_t)fixity].find(name);
    if (it != operators[(uint8_t)fixity].end()) {
        op = it->getValue();
        return true;
    }

    return false;
}

bool Context::hasOperator(StringRef text) {
    for (auto i = 0; i < 3; i++) {
        if (operators[i].count(text) > 0) {
            return true;
        }
    }

    return false;
}

Precedence Context::getOperatorPrecedenceBefore(Precedence precedence) {
    auto it = operatorPrecedenceSet.lower_bound(precedence);
    if (it != operatorPrecedenceSet.begin()) {
        return *(it--);
    }

    return 0;
}

Symbol* Context::getErrorType() const {
    return errorType;
}

Symbol* Context::getVoidType() const {
    return voidType;
}

Symbol* Context::getBoolType() const {
    return boolType;
}

Symbol* Context::getInt8Type() const {
    return int8Type;
}

Symbol* Context::getInt16Type() const {
    return int16Type;
}

Symbol* Context::getInt32Type() const {
    return int32Type;
}

Symbol* Context::getInt64Type() const {
    return int64Type;
}

Symbol* Context::getInt128Type() const {
    return int128Type;
}

Symbol* Context::getIntType() const {
    return intType;
}

Symbol* Context::getUInt8Type() const {
    return uint8Type;
}

Symbol* Context::getUInt16Type() const {
    return uint16Type;
}

Symbol* Context::getUInt32Type() const {
    return uint32Type;
}

Symbol* Context::getUInt64Type() const {
    return uint64Type;
}

Symbol* Context::getUInt128Type() const {
    return uint128Type;
}

Symbol* Context::getUIntType() const {
    return uintType;
}

Symbol* Context::getFloat16Type() const {
    return float16Type;
}

Symbol* Context::getFloat32Type() const {
    return float32Type;
}

Symbol* Context::getFloat64Type() const {
    return float64Type;
}

Symbol* Context::getFloat80Type() const {
    return float80Type;
}

Symbol* Context::getFloat128Type() const {
    return float128Type;
}

Symbol* Context::getFloatType() const {
    return floatType;
}

Symbol* Context::getStringType() const {
    report_fatal_error("Implementation missing!");
    return nullptr;
}

// @Todo verify if typed symbols should be unique!!!

Symbol* Context::getEnumerationType(ArrayRef<Symbol*> elements) {
    auto symbolTable = module->getScope()->getSymbolTable();
    auto symbol = symbolTable->createUniqueSymbol();
    symbol->setType(new (this) EnumerationType(elements));
    return symbol;
}

Symbol* Context::getFunctionType(ArrayRef<Symbol*> parameters, Symbol* result) {
    auto symbolTable = module->getScope()->getSymbolTable();
    auto symbol = symbolTable->createUniqueSymbol();
    symbol->setType(new (this) FunctionType(parameters, result));
    return symbol;
}

Symbol* Context::getStructureType(ArrayRef<Symbol*> members) {
    auto symbolTable = module->getScope()->getSymbolTable();
    auto symbol = symbolTable->createUniqueSymbol();
    symbol->setType(new (this) StructureType(members));
    return symbol;
}

Symbol* Context::getArrayType(Symbol* element, bool isFixedSize, uint32_t size) {
    auto symbolTable = module->getScope()->getSymbolTable();
    auto symbol = symbolTable->createUniqueSymbol();
    symbol->setType(new (this) ArrayType(element, isFixedSize, size));
    return symbol;
}

Symbol* Context::getPointerType(Symbol* pointee, uint32_t depth) {
    // @Todo verify if typed symbols should be unique!!!
    auto symbolTable = module->getScope()->getSymbolTable();
    for (auto symbol : symbolTable->getSymbols()) {
        if (symbol->getType() && symbol->getType()->isPointerType()) {
            auto type = reinterpret_cast<PointerType*>(symbol->getType());
            if (type->getPointee() == pointee && type->getDepth() == depth) {
                return symbol;
            }
        }
    }

    auto symbol = symbolTable->createUniqueSymbol();
    symbol->setType(new (this) PointerType(pointee, depth));
    return symbol;
}

