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

// @Refactor Rename all decls to defns because they are definitions and add decls for signatures only ...

#include "Core/DeclContext.h"
#include "Core/Lexeme.h"
#include "Core/Operator.h"

#include <llvm/ADT/ArrayRef.h>
#include <llvm/ADT/APInt.h>
#include <llvm/ADT/StringMap.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/ADT/SmallVector.h>

enum ASTNodeKind : uint8_t {
    AST_UNKNOWN,
    AST_LOAD,
    AST_NIL_LITERAL,
    AST_BOOL_LITERAL,
    AST_INT_LITERAL,
    AST_FLOAT_LITERAL,
    AST_STRING_LITERAL,
    AST_FUNC,
    AST_PREFIX_FUNC,
    AST_INFIX_FUNC,
    AST_PARAMETER,
    AST_STRUCT,
    AST_VAR,
    AST_LET,
    AST_ENUM,
    AST_ENUM_ELEMENT,
    AST_MODULE,
    AST_IDENTIFIER,
    AST_UNARY,
    AST_BINARY,
    AST_MEMBER_ACCESS,
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
    AST_CALL,
    AST_SUBSCRIPT,
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

    ASTNode() { kind = AST_UNKNOWN; }
    ASTNode(ASTNode&&) = delete;
    ASTNode& operator = (ASTNode&&) = delete;

    void* operator new (size_t size, ASTContext* context);
    void operator delete (void* ptr) = delete;
    void operator delete [] (void* ptr) = delete;

    virtual void destroy() {};

    bool isDecl() const;
};

struct ASTStmt : public ASTNode {};

struct ASTExpr : public ASTStmt {
    bool isCheckedConstant = false;
    bool isConstant = false;
    Type* type = nullptr;
    llvm::SmallVector<Type*, 0> candidateTypes;

    virtual void destroy() override {
        ASTStmt::destroy();
        candidateTypes.~SmallVector();
    }
};

struct ASTDecl : public ASTStmt {
    Lexeme name;
    Type* type = nullptr;
    ASTDecl* nextDeclInContext = nullptr;
    ASTDecl* previousDeclInContext = nullptr;
};

struct ASTLoad;
struct ASTFuncDecl;
struct ASTPrefixFuncDecl;
struct ASTInfixFuncDecl;
struct ASTEnumDecl;
struct ASTStructDecl;
struct ASTValueDecl;

struct ASTModule : public ASTDecl, public DeclContext {
    ASTModule() : DeclContext(AST_MODULE) { kind = AST_MODULE; }
};

struct ASTUnaryExpr : public ASTExpr {
    Operator op;
    ASTExpr* right = nullptr;

    ASTUnaryExpr() { kind = AST_UNARY; }
};

struct ASTBinaryExpr : public ASTExpr {
    Operator op;
    ASTExpr* left = nullptr;
    ASTExpr* right = nullptr;

    ASTBinaryExpr() { kind = AST_BINARY; }
};

struct ASTMemberAccessExpr : public ASTExpr {
    ASTExpr* left = nullptr;
    Lexeme memberName;
    unsigned memberIndex = 0;

    ASTMemberAccessExpr() { kind = AST_MEMBER_ACCESS; }
};

struct ASTIdentExpr : public ASTExpr {
    Lexeme declName;
    ASTDecl* decl = nullptr;

    ASTIdentExpr() { kind = AST_IDENTIFIER; }
};

struct ASTLit : public ASTExpr {};

struct ASTNilLit : ASTLit {
    ASTNilLit() { kind = AST_NIL_LITERAL; }
};

struct ASTBoolLit : ASTLit {
    bool value = false;

    ASTBoolLit() { kind = AST_BOOL_LITERAL; }
};

struct ASTIntLit : ASTLit {
    llvm::APInt value = llvm::APInt(64, 0);

    ASTIntLit() { kind = AST_INT_LITERAL; }

    virtual void destroy() override {
        ASTLit::destroy();
        value.~APInt();
    }
};

struct ASTFloatLit : ASTLit {
    double value = 0;

    ASTFloatLit() { kind = AST_FLOAT_LITERAL; }
};

struct ASTStringLit : ASTLit {
    llvm::StringRef value;

    ASTStringLit() { kind = AST_STRING_LITERAL; }
};

struct ASTLoad : public ASTDecl {
    ASTStringLit* string = nullptr;

    ASTLoad() { kind = AST_LOAD; }
};

struct ASTParamDecl : public ASTDecl {
    ASTTypeRef* typeRef = nullptr;

    ASTParamDecl() { kind = AST_PARAMETER; }
};

struct ASTCompoundStmt : public ASTStmt {
    llvm::ArrayRef<ASTStmt*> stmts;

    ASTCompoundStmt(ASTContext* context, llvm::ArrayRef<ASTStmt*> stmts);
};

struct ASTFuncDecl : public ASTDecl, public DeclContext {
    llvm::SmallVector<ASTParamDecl*, 0> params;
    ASTTypeRef* returnTypeRef = nullptr;
    ASTCompoundStmt* body = nullptr;

    ASTFuncDecl() : DeclContext(AST_FUNC) { kind = AST_FUNC; }

    virtual void destroy() override {
        ASTDecl::destroy();
        params.~SmallVector();
    }
};

struct ASTPrefixFuncDecl : public ASTFuncDecl {
    ASTPrefixFuncDecl() { kind = AST_PREFIX_FUNC; }
};

struct ASTInfixFuncDecl : public ASTFuncDecl {
    ASTInfixFuncDecl() { kind = AST_INFIX_FUNC; }
};

struct ASTValueDecl : public ASTDecl {
    ASTTypeRef* typeRef = nullptr;
    ASTExpr* assignment = nullptr;
};

struct ASTVarDecl : public ASTValueDecl {
    ASTVarDecl() { kind = AST_VAR; }
};

struct ASTLetDecl : public ASTValueDecl {
    ASTLetDecl() { kind = AST_LET; }
};

struct ASTStructDecl : public ASTDecl, public DeclContext {
    ASTStructDecl() : DeclContext(AST_STRUCT) { kind = AST_STRUCT; }
};

struct ASTEnumElementDecl : public ASTDecl {
    ASTExpr* assignment = nullptr;

    ASTEnumElementDecl() { kind = AST_ENUM_ELEMENT; }
};

struct ASTEnumDecl : public ASTDecl, public DeclContext {
    ASTEnumDecl() : DeclContext(AST_ENUM) { kind = AST_ENUM; }
};

struct ASTCtrlStmt : public ASTStmt {};

struct ASTBreakStmt : ASTCtrlStmt {
    ASTBreakStmt() { kind = AST_BREAK; }
};

struct ASTContinueStmt : ASTCtrlStmt {
    ASTContinueStmt() { kind = AST_CONTINUE; }
};

struct ASTFallthroughStmt : ASTCtrlStmt {
    ASTFallthroughStmt() { kind = AST_FALLTHROUGH; }
};

struct ASTReturnStmt : ASTCtrlStmt {
    ASTExpr* expr = nullptr;

    ASTReturnStmt() { kind = AST_RETURN; }
};

struct ASTDeferStmt : public ASTStmt {
    ASTExpr* expr = nullptr;

    ASTDeferStmt() { kind = AST_DEFER; }
};

struct ASTForStmt : public ASTStmt {
    Lexeme elementName;
    ASTExpr* sequenceExpr = nullptr;
    ASTCompoundStmt* body = nullptr;

    ASTForStmt() { kind = AST_FOR; }
};

struct ASTBranchStmt : public ASTStmt {
    ASTExpr* condition = nullptr;
};

struct ASTGuardStmt : public ASTBranchStmt {
    ASTCompoundStmt* elseStmt = nullptr;

    ASTGuardStmt() { kind = AST_GUARD; }
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

    ASTIfStmt() { kind = AST_IF; }
};

struct ASTLoopStmt : public ASTBranchStmt {
    ASTCompoundStmt* body = nullptr;
};

struct ASTDoStmt : public ASTLoopStmt {
    ASTDoStmt() { kind = AST_DO; }
};

struct ASTWhileStmt : public ASTLoopStmt {
    ASTWhileStmt() { kind = AST_WHILE; }
};

enum ASTCaseKind : uint8_t {
    AST_CASE_CONDITION,
    AST_CASE_ELSE
};

struct ASTCaseStmt : public ASTStmt, public DeclContext {
    ASTCaseKind caseKind;
    ASTExpr* condition = nullptr;
    ASTCompoundStmt* body = nullptr;

    ASTCaseStmt() : DeclContext(AST_SWITCH_CASE) { kind = AST_SWITCH_CASE; }
};

struct ASTSwitchStmt : public ASTStmt {
    ASTExpr* expr = nullptr;
    llvm::SmallVector<ASTCaseStmt*, 0> cases;

    ASTSwitchStmt() { kind = AST_SWITCH; }

    virtual void destroy() override {
        ASTStmt::destroy();
        cases.~SmallVector();
    }
};

struct ASTCallExpr : public ASTExpr {
    ASTExpr* left = nullptr;
    llvm::SmallVector<ASTExpr*, 0> args;

    ASTCallExpr() { kind = AST_CALL; }

    virtual void destroy() override {
        ASTExpr::destroy();
        args.~SmallVector();
    }
};

struct ASTSubscriptExpr : public ASTExpr {
    ASTExpr* left = nullptr;
    llvm::SmallVector<ASTExpr*, 0> args;

    ASTSubscriptExpr() { kind = AST_SUBSCRIPT; }

    virtual void destroy() override {
        ASTExpr::destroy();
        args.~SmallVector();
    }
};

struct ASTTypeRef : public ASTNode {
    Type* type = nullptr;
};

struct ASTAnyTypeRef : public ASTTypeRef {
    ASTAnyTypeRef() { kind = AST_ANY_TYPE_REF; }
};

struct ASTOpaqueTypeRef : public ASTTypeRef {
    Lexeme typeName;
    ASTDecl* decl = nullptr;

    ASTOpaqueTypeRef() { kind = AST_OPAQUE_TYPE_REF; }
};

struct ASTTypeOfTypeRef : public ASTTypeRef {
    ASTExpr* expr = nullptr;

    ASTTypeOfTypeRef() { kind = AST_TYPEOF_TYPE_REF; }
};

struct ASTPointerTypeRef : public ASTTypeRef {
    ASTTypeRef* pointeeTypeRef = nullptr;
    unsigned depth = 0;

    ASTPointerTypeRef() { kind = AST_POINTER_TYPE_REF; }
};

struct ASTArrayTypeRef : public ASTTypeRef {
    ASTTypeRef* elementTypeRef = nullptr;
    ASTExpr* sizeExpr = nullptr;

    ASTArrayTypeRef() { kind = AST_ARRAY_TYPE_REF; }
};
