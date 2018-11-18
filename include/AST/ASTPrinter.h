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

#pragma once

#include "AST/ASTVisitor.h"

#include <stdint.h>
#include <ostream>

struct String;
struct ASTContext;
struct ASTNode;
struct ASTLoad;
struct ASTLiteral;
struct ASTFunc;
struct ASTFuncSignature;
struct ASTBlock;
struct ASTParameter;
struct ASTStruct;
struct ASTVariable;
struct ASTEnum;
struct ASTEnumElement;
struct ASTIdentifier;
struct ASTUnaryExpression;
struct ASTBinaryExpression;
struct ASTControl;
struct ASTType;
struct ASTDefer;
struct ASTFor;
struct ASTGuard;
struct ASTIf;
struct ASTSwitch;
struct ASTSwitchCase;
struct ASTLoop;
struct ASTCall;
struct ASTSubscript;

// TODO: Add unit tests with at runtime generated AST representations !!!
struct ASTPrinter {
    using OutputStream = std::ostream;

    ASTPrinter(OutputStream& output_stream) : current_context(nullptr), output_stream(output_stream), indentation_level(0) {
    }

    void print(const ASTContext& context);

private:
    const ASTContext* current_context;
    OutputStream&     output_stream;
    uint32_t          indentation_level;

    void visit(const ASTNode *node);
    void visit(const ASTLoad* node);
    void visit(const ASTLiteral* node);
    void visit(const ASTFunc* node);
    void visit(const ASTFuncSignature* node);
    void visit(const ASTBlock* node);
    void visit(const ASTParameter* node);
    void visit(const ASTStruct* node);
    void visit(const ASTVariable* node);
    void visit(const ASTEnum* node);
    void visit(const ASTEnumElement* node);
    void visit(const ASTIdentifier* node);
    void visit(const ASTUnaryExpression* node);
    void visit(const ASTBinaryExpression* node);
    void visit(const ASTControl* node);
    void visit(const ASTType* node);
    void visit(const ASTDefer* node);
    void visit(const ASTFor* node);
    void visit(const ASTGuard* node);
    void visit(const ASTIf* node);
    void visit(const ASTSwitch* node);
    void visit(const ASTSwitchCase* node);
    void visit(const ASTLoop* node);
    void visit(const ASTCall* node);
    void visit(const ASTSubscript* node);

    void print_indentation();
    void print_kind(const ASTNode* node);
    void print_raw(const String string);
    void print_raw(uint64_t value);
    void print_raw(double value);
};
