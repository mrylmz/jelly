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
struct ASTPrinter : private ASTVisitor {
    using OutputStream = std::ostream;

    ASTPrinter(OutputStream& output_stream) : current_context(nullptr), output_stream(output_stream), indentation_level(0) {
    }

    void print(const ASTContext& context);

private:
    const ASTContext* current_context;
    OutputStream&     output_stream;
    uint32_t          indentation_level;

    virtual void visit(const ASTNode *node);
    virtual void visit(const ASTLoad* node);
    virtual void visit(const ASTLiteral* node);
    virtual void visit(const ASTFunc* node);
    virtual void visit(const ASTFuncSignature* node);
    virtual void visit(const ASTBlock* node);
    virtual void visit(const ASTParameter* node);
    virtual void visit(const ASTStruct* node);
    virtual void visit(const ASTVariable* node);
    virtual void visit(const ASTEnum* node);
    virtual void visit(const ASTEnumElement* node);
    virtual void visit(const ASTIdentifier* node);
    virtual void visit(const ASTUnaryExpression* node);
    virtual void visit(const ASTBinaryExpression* node);
    virtual void visit(const ASTControl* node);
    virtual void visit(const ASTType* node);
    virtual void visit(const ASTDefer* node);
    virtual void visit(const ASTFor* node);
    virtual void visit(const ASTGuard* node);
    virtual void visit(const ASTIf* node);
    virtual void visit(const ASTSwitch* node);
    virtual void visit(const ASTSwitchCase* node);
    virtual void visit(const ASTLoop* node);
    virtual void visit(const ASTCall* node);
    virtual void visit(const ASTSubscript* node);

    void print_indentation();
    void print_kind(const ASTNode* node);
    void print_raw(const String string);
    void print_raw(uint64_t value);
    void print_raw(double value);
};
