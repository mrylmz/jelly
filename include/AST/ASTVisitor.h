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
struct ASTDefer;
struct ASTFor;
struct ASTGuard;
struct ASTIf;
struct ASTSwitch;
struct ASTSwitchCase;
struct ASTLoop;
struct ASTCall;
struct ASTSubscript;
struct ASTType;

struct ASTVisitor {
    virtual void pre_visit_node(ASTNode* node);
    virtual void post_visit_node(ASTNode* node);

    void         visit_node(ASTNode* node);
    virtual void visit_load(ASTLoad* node);
    virtual void visit_literal(ASTLiteral* node);
    virtual void visit_func(ASTFunc* node);
    virtual void visit_func_signature(ASTFuncSignature* node);
    virtual void visit_block(ASTBlock* node);
    virtual void visit_parameter(ASTParameter* node);
    virtual void visit_struct(ASTStruct* node);
    virtual void visit_variable(ASTVariable* node);
    virtual void visit_enum(ASTEnum* node);
    virtual void visit_enum_element(ASTEnumElement* node);
    virtual void visit_identifier(ASTIdentifier* node);
    virtual void visit_unary(ASTUnaryExpression* node);
    virtual void visit_binary(ASTBinaryExpression* node);
    virtual void visit_control(ASTControl* node);
    virtual void visit_defer(ASTDefer* node);
    virtual void visit_for(ASTFor* node);
    virtual void visit_guard(ASTGuard* node);
    virtual void visit_if(ASTIf* node);
    virtual void visit_switch(ASTSwitch* node);
    virtual void visit_switch_case(ASTSwitchCase* node);
    virtual void visit_loop(ASTLoop* node);
    virtual void visit_call(ASTCall* node);
    virtual void visit_subscript(ASTSubscript* node);
    virtual void visit_type(ASTType* node);

private:
    void visit_children(ASTNode* node);
};
