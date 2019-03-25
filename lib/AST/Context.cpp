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

#include "AST/Context.h"
#include "AST/ErrorType.h"
#include "AST/FloatType.h"
#include "AST/IntegerType.h"
#include "AST/ModuleDeclaration.h"
#include "AST/Scope.h"
#include "AST/VoidType.h"

using namespace jelly::AST;

Context::Context() {
    module = new (this) ModuleDeclaration(getIdentifier(""), {});

    errorType = new (this) ErrorType();
    voidType = new (this) VoidType();
    boolType = new (this) IntegerType(true, 1);
    int8Type = new (this) IntegerType(true, 8);
    int16Type = new (this) IntegerType(true, 16);
    int32Type = new (this) IntegerType(true, 32);
    int64Type = new (this) IntegerType(true, 64);
    int128Type = new (this) IntegerType(true, 128);
    uint8Type = new (this) IntegerType(false, 8);
    uint16Type = new (this) IntegerType(false, 16);
    uint32Type = new (this) IntegerType(false, 32);
    uint64Type = new (this) IntegerType(false, 64);
    uint128Type = new (this) IntegerType(false, 128);
    float16Type = new (this) FloatType(FloatType::Kind::Float16);
    float32Type = new (this) FloatType(FloatType::Kind::Float32);
    float64Type = new (this) FloatType(FloatType::Kind::Float64);
    float80Type = new (this) FloatType(FloatType::Kind::Float80);
    float128Type = new (this) FloatType(FloatType::Kind::Float128);

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
