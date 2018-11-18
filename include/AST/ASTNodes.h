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
struct ASTType;

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

    ASTNodeKind kind;
    uint32_t    flags;

    // The parent is not usable inside ASTType !!!
    ASTNode*    parent;

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

    bool has_flags(uint32_t flags) const {
        return (this->flags & flags) != 0;
    }

    void set_flags(uint32_t flags) {
        this->flags |= flags;
    }

    void clear_flags(uint32_t flags) {
        this->flags &= ~flags;
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

    template <typename ...T>
    ASTNode* find_parent_of_kind(ASTNodeKind kind1, T... kinds) const {
        auto parent = this->parent;
        while (parent) {
            if (parent->is(kind1, kinds...)) {
                return parent;
            }

            parent = parent->parent;
        }

        return nullptr;
    }

    void* operator new (size_t size, ASTContext* context);
    void  operator delete (void* ptr) = delete;
    void  operator delete [] (void* ptr) = delete;
};

struct ASTStatement : public ASTNode {};

struct ASTExpression : public ASTStatement {
    ASTExpression() : type(nullptr), substitution(nullptr) {
    }

    ASTType*       type;
    ASTExpression* substitution;
};

struct ASTDirective : public ASTNode {};

struct ASTDeclaration : public ASTStatement {
    ASTDeclaration() : name(nullptr), type(nullptr) {
    }

    ASTIdentifier* name;
    ASTType*       type;
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

#ifdef DEBUG
    String debug_lexeme_text;
#endif
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
    ASTParameter() {
        kind = AST_PARAMETER;
    }
};

struct ASTBlock : public ASTNode {
    ASTBlock() {
        kind = AST_BLOCK;
    }

    Array<ASTNode*> statements;

    // TODO: May split type definitions from variable declarations ???
    std::map<int64_t, ASTType**> symbols;
};

// TODO: Maybe this shouldn't derive from ASTDeclaration?
//       currently ASTFunc and ASTFuncSignature have a name
//       but one of them is redundant ...
struct ASTFuncSignature : public ASTNode {
    ASTFuncSignature() : return_type(nullptr) {
        kind = AST_FUNC_SIGNATURE;
    }

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
    ASTVariable() : type(nullptr), assignment(nullptr) {
        kind = AST_VARIABLE;
    }

    ASTType*       type;
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

    ASTBlock*                block;
    std::vector<ASTLiteral*> all_values;
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

    ASTBlock* else_block;
};

struct ASTIf : public ASTBranch {
    ASTIf() : if_kind(AST_IF_SINGLE) {
        kind = AST_IF;
    }

    bool has_else_block() const {
        if (if_kind == AST_IF_ELSE) {
            return true;
        }

        if (if_kind == AST_IF_ELSE_IF) {
            return else_if->has_else_block();
        }

        return false;
    }

    ASTBlock* block;
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

    bool      pre_check_conditions;
    ASTBlock* block;
};

struct ASTSwitchCase : public ASTNode {
    ASTSwitchCase() {
        kind = AST_SWITCH_CASE;
    }

    ASTSwitchCaseKind case_kind;
    ASTExpression*    condition;
    ASTBlock*         block;
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

// MARK: - Types

struct ASTType : public ASTNode {
    ASTType() :
    type_kind(AST_TYPE_ERROR) {
        kind = AST_TYPE;
    }

    ASTTypeKind type_kind;

    bool is_type(ASTTypeKind kind) const {
        return this->type_kind == kind;
    }

    template <typename ...T>
    bool is_type(ASTTypeKind kind1, ASTTypeKind kind2, T... kinds) const {
        if (is_type(kind1)) {
            return true;
        }

        return is_type(kind2, kinds...);
    }

    bool is_incomplete_type() const;

    bool is_placeholder_type() const {
        return is_type(
           AST_TYPE_PLACEHOLDER_TYPEOF,
           AST_TYPE_PLACEHOLDER_OPAQUE
       );
    }

    bool is_builtin_type() const {
        return is_type(
            AST_TYPE_BUILTIN_ANY,
            AST_TYPE_BUILTIN_VOID,
            AST_TYPE_BUILTIN_BOOL,
            AST_TYPE_BUILTIN_INT,
            AST_TYPE_BUILTIN_FLOAT,
            AST_TYPE_BUILTIN_POINTER,
            AST_TYPE_BUILTIN_ARRAY
        );
    }

    bool is_decl_type() const {
        return is_type(
            AST_TYPE_DECL_ENUM,
            AST_TYPE_DECL_FUNC,
            AST_TYPE_DECL_STRUCT
        );
    }

    bool is_unresolved_type() const {
        return is_type(AST_TYPE_UNRESOLVED);
    };

    bool is_error_type() const {
        return is_type(AST_TYPE_ERROR);
    }

    bool is_typeof_type() const {
        return is_type(AST_TYPE_PLACEHOLDER_TYPEOF);
    }

    bool is_opaque_type() const {
        return is_type(AST_TYPE_PLACEHOLDER_OPAQUE);
    }

    bool is_any_type() const {
        return is_type(AST_TYPE_BUILTIN_ANY);
    }

    bool is_void_type() const {
        return is_type(AST_TYPE_BUILTIN_VOID);
    }

    bool is_bool_type() const {
        return is_type(AST_TYPE_BUILTIN_BOOL);
    }

    bool is_integer_type() const {
        return is_type(AST_TYPE_BUILTIN_INT);
    }

    bool is_float_type() const {
        return is_type(AST_TYPE_BUILTIN_FLOAT);
    }

    bool is_pointer_type() const {
        return is_type(AST_TYPE_BUILTIN_POINTER);
    }

    bool is_array_type() const {
        return is_type(AST_TYPE_BUILTIN_ARRAY);
    }

    bool is_enum_type() const {
        return is_type(AST_TYPE_DECL_ENUM);
    }

    bool is_func_type() const {
        return is_type(AST_TYPE_DECL_FUNC);
    }

    bool is_struct_type() const {
        return is_type(AST_TYPE_DECL_STRUCT);
    }
};

struct ASTBuiltinType : public ASTType {
};

struct ASTPlaceholderType : public ASTType {
};

struct ASTDeclType : public ASTType {
    ASTDeclType(ASTDeclaration* decl) :
    decl(decl) {
    }

    ASTDeclaration* decl;
};

// MARK: Synthetic Types

struct ASTUnknownType : public ASTType {
    ASTUnknownType() {
        type_kind = AST_TYPE_UNKNOWN;
    }
};

struct ASTErrorType : public ASTType {
    ASTErrorType() {
        type_kind = AST_TYPE_ERROR;
    }
};

struct ASTUnresolvedType : public ASTType {
    ASTUnresolvedType() {
        type_kind = AST_TYPE_UNRESOLVED;
    }
};

// MARK: Builtin Types

struct ASTAnyType : public ASTBuiltinType {
    ASTAnyType() {
        type_kind = AST_TYPE_BUILTIN_ANY;
    }
};

struct ASTVoidType : public ASTBuiltinType {
    ASTVoidType() {
        type_kind = AST_TYPE_BUILTIN_VOID;
    }
};

struct ASTBoolType : public ASTBuiltinType {
    ASTBoolType() {
        type_kind = AST_TYPE_BUILTIN_BOOL;
    }
};

struct ASTIntegerType : public ASTBuiltinType {
    ASTIntegerType(bool is_signed, bool is_fixed_width, bool is_pointer_width, uint32_t fixed_width) :
    is_signed(is_signed),
    is_fixed_width(is_fixed_width),
    is_pointer_width(is_pointer_width),
    fixed_width(fixed_width) {
        type_kind = AST_TYPE_BUILTIN_INT;
    }

    bool     is_signed;
    bool     is_fixed_width;
    bool     is_pointer_width;
    uint32_t fixed_width;
};

struct ASTFloatType : public ASTBuiltinType {
    ASTFloatType(ASTFloatKind float_kind) :
    float_kind(float_kind) {
        type_kind = AST_TYPE_BUILTIN_FLOAT;
    }

    ASTFloatKind float_kind;

    uint32_t bit_width() const;
};

struct ASTStringType : public ASTBuiltinType {
    ASTStringType() {
        type_kind = AST_TYPE_BUILTIN_STRING;
    }
};

struct ASTPointerType : public ASTBuiltinType {
    ASTPointerType(uint32_t pointer_depth, ASTType* pointee_type) :
    pointer_depth(pointer_depth),
    pointee_type(pointee_type) {
        type_kind = AST_TYPE_BUILTIN_POINTER;
    }

    uint32_t pointer_depth;
    ASTType* pointee_type;
};

struct ASTArrayType : public ASTType {
    ASTArrayType(bool is_static, ASTExpression* size_expr, ASTType* element_type) :
    is_static(is_static),
    size(0),
    size_expr(size_expr),
    element_type(element_type) {
        type_kind = AST_TYPE_BUILTIN_ARRAY;
    }

    bool           is_static;
    uint64_t       size;
    ASTExpression* size_expr;
    ASTType*       element_type;
};

// MARK: Placeholder Types

struct ASTTypeOfType : public ASTPlaceholderType {
    ASTTypeOfType(ASTExpression* expr) :
    expr(expr) {
        type_kind = AST_TYPE_PLACEHOLDER_TYPEOF;
    }

    ASTExpression* expr;
};

struct ASTOpaqueType : public ASTPlaceholderType {
    ASTOpaqueType(ASTIdentifier* identifier) :
    identifier(identifier) {
        type_kind = AST_TYPE_PLACEHOLDER_OPAQUE;
    }

    ASTIdentifier* identifier;
};

// MARK: Decl Types

struct ASTEnumType : public ASTDeclType {
    ASTEnumType(ASTEnum* decl) :
    ASTDeclType(decl) {
        type_kind = AST_TYPE_DECL_ENUM;
    }
};

struct ASTFuncType : public ASTDeclType {
    ASTFuncType(ASTFunc* decl) :
    ASTDeclType(decl),
    calling_convention(AST_CALLING_CONVENTION_DEFAULT),
    parameter_types({}),
    return_type(nullptr) {
        type_kind = AST_TYPE_DECL_FUNC;
    }

    ASTCallingConvention  calling_convention;
    std::vector<ASTType*> parameter_types;
    ASTType*              return_type;
};

struct ASTStructType : public ASTDeclType {
    ASTStructType(ASTStruct* decl) :
    ASTDeclType(decl),
    member_types({}) {
        type_kind = AST_TYPE_DECL_STRUCT;
    }

    std::vector<ASTType*> member_types;
};
