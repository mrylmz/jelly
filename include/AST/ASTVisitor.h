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

// TODO: Rename visit methods adding type names as suffix
// TODO: Add more useful visitor logic to fully cover visitor-pattern.

struct ASTVisitor {
    virtual void visit(const ASTNode* node);
    virtual void visit(const ASTLoad* node) = 0;
    virtual void visit(const ASTLiteral* node) = 0;
    virtual void visit(const ASTFunc* node) = 0;
    virtual void visit(const ASTFuncSignature* node) = 0;
    virtual void visit(const ASTBlock* node) = 0;
    virtual void visit(const ASTParameter* node) = 0;
    virtual void visit(const ASTStruct* node) = 0;
    virtual void visit(const ASTVariable* node) = 0;
    virtual void visit(const ASTEnum* node) = 0;
    virtual void visit(const ASTEnumElement* node) = 0;
    virtual void visit(const ASTIdentifier* node) = 0;
    virtual void visit(const ASTUnaryExpression* node) = 0;
    virtual void visit(const ASTBinaryExpression* node) = 0;
    virtual void visit(const ASTControl* node) = 0;
    virtual void visit(const ASTType* node) = 0;
    virtual void visit(const ASTDefer* node) = 0;
    virtual void visit(const ASTFor* node) = 0;
    virtual void visit(const ASTGuard* node) = 0;
    virtual void visit(const ASTIf* node) = 0;
    virtual void visit(const ASTSwitch* node) = 0;
    virtual void visit(const ASTSwitchCase* node) = 0;
    virtual void visit(const ASTLoop* node) = 0;
    virtual void visit(const ASTCall* node) = 0;
    virtual void visit(const ASTSubscript* node) = 0;
};
