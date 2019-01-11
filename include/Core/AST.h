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

#include "Core/DeclContext.h"
#include "Core/Lexeme.h"
#include "Core/Operator.h"

#include <llvm/ADT/ArrayRef.h>
#include <llvm/ADT/APInt.h>
#include <llvm/ADT/StringRef.h>

enum ASTNodeKind : uint8_t {
    AST_UNINITIALIZED,
    AST_LOAD_DIRECTIVE,
    AST_NIL_LITERAL,
    AST_BOOL_LITERAL,
    AST_INT_LITERAL,
    AST_FLOAT_LITERAL,
    AST_STRING_LITERAL,
    AST_FUNC_DECL,
    AST_PARAM_DECL,
    AST_STRUCT_DECL,
    AST_VALUE_DECL,
    AST_ENUM_DECL,
    AST_ENUM_ELEMENT_DECL,
    AST_MODULE_DECL,
    AST_IDENTIFIER,
    AST_UNARY,
    AST_BINARY,
    AST_MEMBER_ACCESS,
    AST_CALL,
    AST_SUBSCRIPT,
    AST_COMPOUND_STMT,
    AST_BREAK,
    AST_CONTINUE,
    AST_FALLTHROUGH,
    AST_RETURN,
    AST_DEFER,
    AST_FOR,
    AST_GUARD,
    AST_IF,
    AST_SWITCH,
    AST_SWITCH_CASE,
    AST_DO,
    AST_WHILE,
    AST_ANY_TYPE_REF,
    AST_OPAQUE_TYPE_REF,
    AST_TYPEOF_TYPE_REF,
    AST_POINTER_TYPE_REF,
    AST_ARRAY_TYPE_REF
};

struct ASTContext;
struct ASTIdentExpr;
struct ASTTypeRef;
struct Type;

// @Incomplete store source range information in ASTNode

struct ASTNode {
    ASTNodeKind kind;
    ASTNode* parent = nullptr;

    // @Refactor declContext should be removed from nodes completely because it is only required for the type resolution
    //           by storing the current context in the Sema pass while traversing the AST it is possible to keep track
    //           of the current context of any node by using a top down tarversal, for now we will keep this redundancy
    //           to delay the introduction of complexity inside the sema phase...
    DeclContext* declContext = nullptr;
    bool isValidated = false;

    ASTNode(ASTNodeKind kind) {
        this->kind = kind;
    }

    ASTNode() = delete;
    ASTNode(ASTNode&&) = delete;
    ASTNode& operator = (ASTNode&&) = delete;

    void* operator new (size_t size, ASTContext* context);
    void operator delete (void* ptr) = delete;
    void operator delete [] (void* ptr) = delete;

    // @Refactor The destroy method is currently only used for freeing of APInt
    //           refactor the usage of APInt so that this can be deleted!
    //           The ASTContext is storing every node which is created
    //           to just call empty -destroy methods!
    virtual void destroy() {};

    bool isDecl() const;
};

struct ASTStmt : public ASTNode {
    ASTStmt(ASTNodeKind kind) : ASTNode(kind) {}
};

struct ASTExpr : public ASTStmt {
    bool isCheckedConstant = false;
    bool isConstant = false;
    Type* type = nullptr;

    ASTExpr(ASTNodeKind kind) : ASTStmt(kind) {}
};

struct ASTDecl : public ASTStmt {
    ASTDecl* nextDeclInContext = nullptr;
    ASTDecl* previousDeclInContext = nullptr;

    ASTDecl(ASTNodeKind kind) : ASTStmt(kind) {}

    bool isNamedDecl() const {
        return kind == AST_MODULE_DECL
        || kind == AST_ENUM_DECL
        || kind == AST_ENUM_ELEMENT_DECL
        || kind == AST_FUNC_DECL
        || kind == AST_PARAM_DECL
        || kind == AST_STRUCT_DECL
        || kind == AST_VALUE_DECL;
    }

    bool isFunc() const { return kind == AST_FUNC_DECL; }
};

struct ASTLoadDirective : public ASTDecl {
    llvm::StringRef loadFilePath;

    ASTLoadDirective() : ASTDecl(AST_LOAD_DIRECTIVE) {}
};

struct ASTNamedDecl : public ASTDecl {
    Lexeme name;
    Type* type = nullptr;

    ASTNamedDecl(ASTNodeKind kind) : ASTDecl(kind) {}
};

struct ASTModuleDecl : public ASTNamedDecl, public DeclContext {
    ASTModuleDecl() : ASTNamedDecl(AST_MODULE_DECL), DeclContext(AST_MODULE_DECL) { }
};

struct ASTParamDecl;
struct ASTCompoundStmt;

struct ASTEnumElementDecl : public ASTNamedDecl {
    ASTExpr* assignment = nullptr;

    ASTEnumElementDecl() : ASTNamedDecl(AST_ENUM_ELEMENT_DECL) { }
};

struct ASTEnumDecl : public ASTNamedDecl, public DeclContext {
    ASTEnumDecl() : ASTNamedDecl(AST_ENUM_DECL), DeclContext(AST_ENUM_DECL) { }
};

struct ASTFuncDecl : public ASTNamedDecl, public DeclContext {
    llvm::ArrayRef<ASTParamDecl*> parameters;
    ASTTypeRef* returnTypeRef = nullptr;
    ASTCompoundStmt* body = nullptr;

    ASTFuncDecl() : ASTNamedDecl(AST_FUNC_DECL), DeclContext(AST_FUNC_DECL) { }
};

struct ASTValueDecl : public ASTNamedDecl {
    ASTTypeRef* typeRef = nullptr;
    ASTExpr* initializer = nullptr;
    bool isConstant;

    ASTValueDecl(bool isConstant) : ASTNamedDecl(AST_VALUE_DECL), isConstant(isConstant) {}
};

struct ASTStructDecl : public ASTNamedDecl, public DeclContext {
    ASTStructDecl() : ASTNamedDecl(AST_STRUCT_DECL), DeclContext(AST_STRUCT_DECL) { }
};

struct ASTParamDecl : public ASTNamedDecl {
    ASTTypeRef* typeRef = nullptr;

    ASTParamDecl() : ASTNamedDecl(AST_PARAM_DECL) { }
};

struct ASTUnaryExpr : public ASTExpr {
    Operator op;
    ASTExpr* right = nullptr;

    ASTUnaryExpr() : ASTExpr(AST_UNARY) { }
};

struct ASTBinaryExpr : public ASTExpr {
    Operator op;
    ASTExpr* left = nullptr;
    ASTExpr* right = nullptr;

    ASTBinaryExpr() : ASTExpr(AST_BINARY) { }

    bool isAssignment() const { return op.isAssignment; }
};

struct ASTIdentExpr : public ASTExpr {
    Lexeme declName;
    ASTNamedDecl* decl = nullptr;

    ASTIdentExpr() : ASTExpr(AST_IDENTIFIER) { }
};

struct ASTMemberAccessExpr : public ASTExpr {
    ASTExpr* left = nullptr;
    Lexeme memberName;
    ASTDecl* memberDecl = nullptr;
    unsigned memberIndex = 0;

    ASTMemberAccessExpr() : ASTExpr(AST_MEMBER_ACCESS) { }
};

struct ASTCallExpr : public ASTExpr {
    ASTExpr* left = nullptr;
    llvm::ArrayRef<ASTExpr*> args;

    ASTCallExpr() : ASTExpr(AST_CALL) { }
};

struct ASTSubscriptExpr : public ASTExpr {
    ASTExpr* left = nullptr;
    llvm::ArrayRef<ASTExpr*> args;

    ASTSubscriptExpr() : ASTExpr(AST_SUBSCRIPT) { }
};

struct ASTLit : public ASTExpr {
    ASTLit(ASTNodeKind kind) : ASTExpr(kind) {}
};

struct ASTNilLit : ASTLit {
    ASTNilLit() : ASTLit(AST_NIL_LITERAL) { }
};

struct ASTBoolLit : ASTLit {
    bool value = false;

    ASTBoolLit() : ASTLit(AST_BOOL_LITERAL) { }
};

struct ASTIntLit : ASTLit {
    llvm::APInt value = llvm::APInt(64, 0);

    ASTIntLit() : ASTLit(AST_INT_LITERAL) { }

    virtual void destroy() override {
        ASTLit::destroy();
        value.~APInt();
    }
};

struct ASTFloatLit : ASTLit {
    double value = 0;

    ASTFloatLit() : ASTLit(AST_FLOAT_LITERAL) { }
};

struct ASTStringLit : ASTLit {
    llvm::StringRef value;

    ASTStringLit() : ASTLit(AST_STRING_LITERAL) { }
};

struct ASTCompoundStmt : public ASTStmt {
    llvm::ArrayRef<ASTStmt*> stmts;

    ASTCompoundStmt() : ASTStmt(AST_COMPOUND_STMT) { }
};

struct ASTCtrlStmt : public ASTStmt {
    ASTCtrlStmt(ASTNodeKind kind) : ASTStmt(kind) {}
};

struct ASTBreakStmt : ASTCtrlStmt {
    ASTBreakStmt() : ASTCtrlStmt(AST_BREAK) { }
};

struct ASTContinueStmt : ASTCtrlStmt {
    ASTContinueStmt() : ASTCtrlStmt(AST_CONTINUE) { }
};

struct ASTFallthroughStmt : ASTCtrlStmt {
    ASTFallthroughStmt() : ASTCtrlStmt(AST_FALLTHROUGH) { }
};

struct ASTReturnStmt : ASTCtrlStmt {
    ASTExpr* expr = nullptr;

    ASTReturnStmt() : ASTCtrlStmt(AST_RETURN) { }
};

struct ASTDeferStmt : public ASTStmt {
    ASTExpr* expr = nullptr;

    ASTDeferStmt() : ASTStmt(AST_DEFER) { }
};

struct ASTForStmt : public ASTStmt {
    Lexeme elementName;
    ASTExpr* sequenceExpr = nullptr;
    ASTCompoundStmt* body = nullptr;

    ASTForStmt() : ASTStmt(AST_FOR) { }
};

struct ASTBranchStmt : public ASTStmt {
    ASTExpr* condition = nullptr;

    ASTBranchStmt(ASTNodeKind kind) : ASTStmt(kind) { }
};

struct ASTGuardStmt : public ASTBranchStmt {
    ASTCompoundStmt* elseStmt = nullptr;

    ASTGuardStmt() : ASTBranchStmt(AST_GUARD) { }
};

enum ASTChainKind : uint8_t {
    AST_CHAIN_NONE,
    AST_CHAIN_ELSE,
    AST_CHAIN_IF
};

struct ASTIfStmt : public ASTBranchStmt {
    ASTCompoundStmt* thenStmt = nullptr;
    ASTChainKind chainKind = AST_CHAIN_NONE;

    union {
        ASTCompoundStmt* elseStmt;
        ASTIfStmt* elseIf;
    };

    ASTIfStmt() : ASTBranchStmt(AST_IF) { }
};

struct ASTLoopStmt : public ASTBranchStmt {
    ASTCompoundStmt* body = nullptr;

    ASTLoopStmt(ASTNodeKind kind) : ASTBranchStmt(kind) { }
};

struct ASTDoStmt : public ASTLoopStmt {
    ASTDoStmt() : ASTLoopStmt(AST_DO) { }
};

struct ASTWhileStmt : public ASTLoopStmt {
    ASTWhileStmt() : ASTLoopStmt(AST_WHILE) { }
};

enum ASTCaseKind : uint8_t {
    AST_CASE_CONDITION,
    AST_CASE_ELSE
};

struct ASTCaseStmt : public ASTStmt, public DeclContext {
    ASTCaseKind caseKind;
    ASTExpr* condition = nullptr;
    ASTCompoundStmt* body = nullptr;

    ASTCaseStmt() : ASTStmt(AST_SWITCH_CASE), DeclContext(AST_SWITCH_CASE) { }
};

struct ASTSwitchStmt : public ASTStmt {
    ASTExpr* expr = nullptr;
    llvm::ArrayRef<ASTCaseStmt*> cases;

    ASTSwitchStmt() : ASTStmt(AST_SWITCH) { }
};

struct ASTTypeRef : public ASTNode {
    Type* type = nullptr;

    ASTTypeRef(ASTNodeKind kind) : ASTNode(kind) { }
};

struct ASTAnyTypeRef : public ASTTypeRef {
    ASTAnyTypeRef() : ASTTypeRef(AST_ANY_TYPE_REF) { }
};

struct ASTOpaqueTypeRef : public ASTTypeRef {
    Lexeme typeName;
    ASTNamedDecl* decl = nullptr;

    ASTOpaqueTypeRef() : ASTTypeRef(AST_OPAQUE_TYPE_REF) { }
};

struct ASTTypeOfTypeRef : public ASTTypeRef {
    ASTExpr* expr = nullptr;

    ASTTypeOfTypeRef() : ASTTypeRef(AST_TYPEOF_TYPE_REF) { }
};

struct ASTPointerTypeRef : public ASTTypeRef {
    ASTTypeRef* pointeeTypeRef = nullptr;
    unsigned depth = 0;

    ASTPointerTypeRef() : ASTTypeRef(AST_POINTER_TYPE_REF) { }
};

struct ASTArrayTypeRef : public ASTTypeRef {
    ASTTypeRef* elementTypeRef = nullptr;
    ASTExpr* sizeExpr = nullptr;

    ASTArrayTypeRef() : ASTTypeRef(AST_ARRAY_TYPE_REF) { }
};
