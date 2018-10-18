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

#include "AST/ASTNodeKinds.h"

#include <Basic/Basic.h>
#include <Syntax/Syntax.h>
#include <vector>

#warning Revisit naming and structures of all ASTNodes

struct ASTContext;

struct ASTLexeme {
    ASTLexeme(int64_t index = -1) : index(index) {
    }

    int64_t index;
};

struct ASTNode {
    ASTNode() : kind(AST_UNKNOWN), flags(0) {}

    ASTNodeKind kind;
    uint32_t    flags;

#warning Replace with custom implementation of Array in Basic!
    template<typename Element>
    using Array = std::vector<Element>;

    bool is(ASTNodeKind kind) const {
        return this->kind == kind;
    }

    template <typename ...T>
    bool is(ASTNodeKind kind1, ASTNodeKind kind2, T... kinds) const {
        if (is(kind1)) {
            return true;
        }

        return is(kind2, kinds...);
    }

    void* operator new (size_t size, ASTContext* context);
    void  operator delete (void* ptr) = delete;
    void  operator delete [] (void* ptr) = delete;
};

struct ASTStatement : public ASTNode {
};

struct ASTDeclaration : public ASTStatement {
};

struct ASTExpression : public ASTStatement {
};

struct ASTUnaryExpression : public ASTExpression {
    ASTUnaryExpression() : op({}), right(nullptr) {
        kind = AST_UNARY;
    }

    Operator       op;
    ASTExpression* right;
};

struct ASTBinaryExpression : public ASTExpression {
    ASTBinaryExpression() {
        kind = AST_BINARY;
    }

    Operator       op;
    ASTExpression* left;
    ASTExpression* right;
};

struct ASTIdentifier : public ASTExpression {
    ASTIdentifier() : lexeme({}) {
        kind = AST_IDENTIFIER;
    }

    ASTLexeme lexeme;
};

struct ASTType : public ASTNode {
    ASTType() : type(nullptr), type_kind(AST_TYPE_UNKNOWN), identifier(nullptr) {
        kind = AST_TYPE;
    }

    ASTType*    type;
    ASTTypeKind type_kind;

    union {
        ASTIdentifier* identifier;
        ASTExpression* expression;
    };
};

struct ASTLiteral : public ASTExpression {
    ASTLiteral() : token_kind(TOKEN_UNKNOWN), string_value({}) {
        kind = AST_LITERAL;
    }

    uint32_t     token_kind;
    union {
        bool     bool_value;
        uint64_t int_value;
        double   float_value;
        String   string_value;
    };
};

struct ASTDirective : public ASTNode {
};

struct ASTLoad : public ASTDirective {
    ASTLoad() : literal(nullptr) {
        kind = AST_LOAD;
    }

    ASTLiteral* literal;
};

struct ASTParameter : public ASTNode {
    ASTParameter() : name(nullptr), type(nullptr) {
        kind = AST_PARAMETER;
    }

    ASTIdentifier* name;
    ASTType*       type;
};

struct ASTBlock : public ASTNode {
    ASTBlock() {
        kind = AST_BLOCK;
    }

    Array<ASTNode*> statements;
};

struct ASTFuncSignature : public ASTNode {
    ASTFuncSignature() : name(nullptr), return_type(nullptr) {
        kind = AST_FUNC_SIGNATURE;
    }

    ASTIdentifier*       name;
    Array<ASTParameter*> parameters;
    ASTType*             return_type;
};

struct ASTFunc : public ASTDeclaration {
    ASTFunc() : block(nullptr) {
        kind = AST_FUNC;
    }

    ASTFuncSignature* signature;
    ASTBlock*         block;
};

struct ASTVariable : public ASTDeclaration {
    ASTVariable() : name(nullptr), type(nullptr), assignment(nullptr) {
        kind = AST_VARIABLE;
    }

    ASTIdentifier* name;
    ASTType*       type;
    ASTExpression* assignment;
};

struct ASTStruct : public ASTDeclaration {
    ASTStruct() : name(nullptr), block(nullptr) {
        kind = AST_STRUCT;
    }

    ASTIdentifier*      name;
    ASTBlock*           block;
};

struct ASTEnumElement : public ASTNode {
    ASTEnumElement() : name(nullptr), assignment(nullptr) {
        kind = AST_ENUM_ELEMENT;
    }

    ASTIdentifier* name;
    ASTExpression* assignment;
};

struct ASTEnum : public ASTDeclaration {
    ASTEnum() : name(nullptr) {
        kind = AST_ENUM;
    }

    ASTIdentifier*         name;
    Array<ASTEnumElement*> elements;
};

struct ASTControl : public ASTStatement {
    ASTControl() : token_kind(TOKEN_UNKNOWN), expression(nullptr) {
        kind = AST_CONTROL;
    }

    // TODO: Replace with new ASTControlKind enum !!!
    uint32_t       token_kind;
    ASTExpression* expression;
};

struct ASTDefer : public ASTStatement {
    ASTDefer() {
        kind = AST_DEFER;
    }

    ASTExpression* expression;
};

struct ASTDo : public ASTStatement {
    ASTDo() {
        kind = AST_DO;
    }

    ASTBlock*             block;
    Array<ASTExpression*> conditions;
};

struct ASTFor : public ASTStatement {
    ASTFor() {
        kind = AST_FOR;
    }

    ASTIdentifier* iterator;
    ASTExpression* sequence;
    ASTBlock*      block;
};

struct ASTGuard : public ASTStatement {
    ASTGuard() {
        kind = AST_GUARD;
    }

    Array<ASTExpression*> conditions;
    ASTBlock*             else_block;
};

struct ASTIf : public ASTStatement {
    ASTIf() : if_kind(AST_IF_SINGLE) {
        kind = AST_IF;
    }

    Array<ASTExpression*> conditions;
    ASTBlock*             block;

    ASTIfKind if_kind;
    union {
        ASTBlock* else_block;
        ASTIf*    else_if;
    };
};

struct ASTSwitchCase : public ASTNode {
    ASTSwitchCase() {
        kind = AST_SWITCH_CASE;
    }

    ASTSwitchCaseKind    case_kind;
    ASTExpression*       condition;
    Array<ASTStatement*> statements;
};

struct ASTSwitch : public ASTStatement {
    ASTSwitch() {
        kind = AST_SWITCH;
    }

    ASTExpression*        expression;
    Array<ASTSwitchCase*> cases;
};

struct ASTWhile : public ASTStatement {
    ASTWhile() {
        kind = AST_WHILE;
    }

    Array<ASTExpression*> conditions;
    ASTBlock*             block;
};

struct ASTCall : public ASTExpression {
    ASTCall() {
        kind = AST_CALL;
    }

    ASTExpression*        left;
    Array<ASTExpression*> arguments;
};

struct ASTSubscript : public ASTExpression {
    ASTSubscript() {
        kind = AST_SUBSCRIPT;
    }

    ASTExpression*        left;
    Array<ASTExpression*> arguments;
};
