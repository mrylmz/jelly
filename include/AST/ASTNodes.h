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

#include "AST/ASTEnums.h"
#include "AST/ASTFlags.h"

#include <Basic/Basic.h>
#include <Syntax/Syntax.h>
#include <vector>
#include <map>

struct ASTContext;
struct ASTNode;
struct ASTDeclaration;
struct ASTIdentifier;
struct ASTFunc;
struct ASTStruct;
struct ASTEnum;
struct ASTBlock;
struct ASTTypeRef;

struct ASTLexeme {
    int64_t index = -1;

    bool operator == (const ASTLexeme &other) const {
        return index == other.index;
    }
};

struct ASTNode {
    ASTNode() : kind(AST_UNKNOWN), flags(0), parent(nullptr) {}

// TODO: Replace with custom implementation of Array in Basic!
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

    bool is_expression() const {
        return is(
            AST_UNARY,
            AST_BINARY,
            AST_IDENTIFIER,
            AST_LITERAL,
            AST_CALL,
            AST_SUBSCRIPT
        );
    }

    ASTBlock* get_parent_block() const {
        ASTNode* next = parent;

        while (next) {
            if (next->kind == AST_BLOCK) {
                return reinterpret_cast<ASTBlock*>(next);
            }

            next = next->parent;
        }

        return nullptr;
    }

    void* operator new (size_t size, ASTContext* context);
    void  operator delete (void* ptr) = delete;
    void  operator delete [] (void* ptr) = delete;

    ASTNodeKind kind;
    uint32_t    flags;
    ASTNode*    parent;
};

struct ASTStatement : public ASTNode {};

struct ASTExpression : public ASTStatement {
    ASTExpression() : substitution(nullptr) {
    }

    ASTExpression* substitution;
};

struct ASTDirective : public ASTNode {};

struct ASTDeclaration : public ASTStatement {
    ASTDeclaration() : name(nullptr) {
    }

    ASTIdentifier* name;
};

struct ASTUnaryExpression : public ASTExpression {
    ASTUnaryExpression() : op({}), op_identifier(nullptr), right(nullptr) {
        kind = AST_UNARY;
    }

    Operator       op;
    ASTIdentifier* op_identifier;
    ASTExpression* right;
};

struct ASTBinaryExpression : public ASTExpression {
    ASTBinaryExpression() : op_identifier(nullptr), left(nullptr), right(nullptr) {
        kind = AST_BINARY;
    }

    Operator       op;
    ASTIdentifier* op_identifier;
    ASTExpression* left;
    ASTExpression* right;
};

struct ASTIdentifier : public ASTExpression {
    ASTIdentifier() : lexeme({}) {
        kind = AST_IDENTIFIER;
    }

    ASTLexeme lexeme;
};

struct ASTTypeRef : public ASTNode {
    ASTTypeRef() : type_ref_kind(AST_TYPE_REF_UNKNOWN), base_ref(nullptr), identifier(nullptr) {
        kind = AST_TYPE_REF;
    }

    ASTTypeRefKind type_ref_kind;
    ASTTypeRef*    base_ref;
    union {
        ASTIdentifier*  identifier;
        ASTExpression*  expression;
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

struct ASTLoad : public ASTDirective {
    ASTLoad() : literal(nullptr) {
        kind = AST_LOAD;
    }

    ASTLiteral* literal;
};

struct ASTParameter : public ASTDeclaration {
    ASTParameter() : type_ref(nullptr) {
        kind = AST_PARAMETER;
    }

    uint32_t    position;
    ASTTypeRef* type_ref;
};

struct ASTBlock : public ASTNode {
    ASTBlock() {
        kind = AST_BLOCK;
    }

    Array<ASTNode*> statements;

    // Scope Members
    using SymbolTable = std::map<int64_t, ASTDeclaration*>;

    SymbolTable symbols;
};

// TODO: Maybe this shouldn't derive from ASTDeclaration?
//       currently ASTFunc and ASTFuncSignature have a name
//       but one of them is redundant ...
struct ASTFuncSignature : public ASTDeclaration {
    ASTFuncSignature() : return_type_ref(nullptr) {
        kind = AST_FUNC_SIGNATURE;
    }

    Array<ASTParameter*> parameters;
    ASTTypeRef*          return_type_ref;
};

struct ASTFunc : public ASTDeclaration {
    ASTFunc() : block(nullptr) {
        kind = AST_FUNC;
    }

    ASTFuncSignature* signature;
    ASTBlock*         block;
};

struct ASTVariable : public ASTDeclaration {
    ASTVariable() : type_ref(nullptr), assignment(nullptr) {
        kind = AST_VARIABLE;
    }

    ASTTypeRef*    type_ref;
    ASTExpression* assignment;
};

struct ASTStruct : public ASTDeclaration {
    ASTStruct() : block(nullptr) {
        kind = AST_STRUCT;
    }

    ASTBlock* block;
};

struct ASTEnumElement : public ASTDeclaration {
    ASTEnumElement() : assignment(nullptr) {
        kind = AST_ENUM_ELEMENT;
    }

    ASTExpression* assignment;
};

struct ASTEnum : public ASTDeclaration {
    ASTEnum() {
        kind = AST_ENUM;
    }

    ASTBlock* block;
};

struct ASTControl : public ASTStatement {
    ASTControl() : control_kind(AST_CONTROL_UNKNOWN), expression(nullptr) {
        kind = AST_CONTROL;
    }

    ASTControlKind control_kind;
    ASTExpression* expression;
};

struct ASTDefer : public ASTStatement {
    ASTDefer() {
        kind = AST_DEFER;
    }

    ASTExpression* expression;
};

struct ASTFor : public ASTStatement {
    ASTFor() {
        kind = AST_FOR;
    }

    ASTIdentifier* iterator;
    ASTExpression* sequence;
    ASTBlock*      block;
};

struct ASTBranch : public ASTStatement {
    Array<ASTExpression*> conditions;
};

struct ASTGuard : public ASTBranch {
    ASTGuard() {
        kind = AST_GUARD;
    }

    ASTBlock*             else_block;
};

struct ASTIf : public ASTBranch {
    ASTIf() : if_kind(AST_IF_SINGLE) {
        kind = AST_IF;
    }

    ASTBlock*             block;

    ASTIfKind if_kind;
    union {
        ASTBlock* else_block;
        ASTIf*    else_if;
    };
};

struct ASTLoop : public ASTBranch {
    ASTLoop() : pre_check_conditions(true), block(nullptr) {
        kind = AST_LOOP;
    }

    bool                  pre_check_conditions;
    ASTBlock*             block;
};

struct ASTSwitchCase : public ASTNode {
    ASTSwitchCase() {
        kind = AST_SWITCH_CASE;
    }

    ASTSwitchCaseKind    case_kind;
    ASTExpression*       condition;
    ASTBlock*            block;
};

struct ASTSwitch : public ASTStatement {
    ASTSwitch() {
        kind = AST_SWITCH;
    }

    ASTExpression*        expression;
    Array<ASTSwitchCase*> cases;
};

struct ASTCall : public ASTExpression {
    ASTCall() : left(nullptr) {
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
