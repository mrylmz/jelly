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

#include <AST/AST.h>
#include <Basic/Basic.h>

struct Sema {
    CodeManager* codeManager;
    jelly::AST::Context* context;
    jelly::DiagnosticEngine* diagnosticEngine;

    Sema(CodeManager* codeManager);

    void validateAST();

private:
    // @Todo migrate to new AST interface
    /*
    void resolveType(ASTTypeRef* TypeRef);

    void inferTypeOfNode(ASTNode* node);
    void inferTypeOfLiteral(ASTLit* literal);
    void inferTypeOfValueDecl(ASTValueDecl* var);
    void inferTypeOfIdentExpr(ASTIdentExpr* expr);
    void inferTypeOfStmts(ASTCompoundStmt* stmt);
    void inferTypeOfUnaryExpr(ASTUnaryExpr* expr);
    void inferTypeOfBinaryExpr(ASTBinaryExpr* expr);
    void inferTypeOfMemberAccessExpr(ASTMemberAccessExpr* expr);
    void inferTypeOfCallExpr(ASTCallExpr* expr);
    void inferTypeOfSubscriptExpr(ASTSubscriptExpr* expr);

    void typeFuncDecl(ASTFuncDecl* decl);
    void typeParamDecl(ASTParamDecl* decl);
    void typeStructDecl(ASTStructDecl* decl);
    void typeEnumDecl(ASTEnumDecl* decl);
    void typeEnumElementDecl(ASTEnumElementDecl* decl);

    bool checkCyclicStorageInStructDecl(ASTStructDecl* structDecl, jelly::SmallVector<ASTStructDecl*, 0>* parentDecls);

    void typeCheckNode(ASTNode* node);
    void typeCheckFuncDecl(ASTFuncDecl* decl);
    void typeCheckFuncBody(ASTFuncDecl* decl);
    void typeCheckParamDecl(ASTParamDecl* decl);
    void typeCheckStructDecl(ASTStructDecl* decl);
    void typeCheckStructMembers(ASTStructDecl* decl);
    void typeCheckValueDecl(ASTValueDecl* decl);
    void typeCheckEnumDecl(ASTEnumDecl* decl);
    void typeCheckEnumElementDecl(ASTEnumElementDecl* decl);
    void typeCheckCompoundStmt(ASTCompoundStmt* stmt);
    void typeCheckIdentExpr(ASTIdentExpr* expr);
    void typeCheckUnaryExpr(ASTUnaryExpr* expr);
    void typeCheckBinaryExpr(ASTBinaryExpr* expr);
    void typeCheckMemberAccessExpr(ASTMemberAccessExpr* expr);
    void typeCheckCallExpr(ASTCallExpr* expr);
    void typeCheckSubscriptExpr(ASTSubscriptExpr* expr);
    void typeCheckExpr(ASTExpr* expr);
    void typeCheckBreakStmt(ASTBreakStmt* stmt);
    void typeCheckContinueStmt(ASTContinueStmt* stmt);
    void typeCheckFallthroughStmt(ASTFallthroughStmt* stmt);
    void typeCheckReturnStmt(ASTReturnStmt* stmt);
    void typeCheckDeferStmt(ASTDeferStmt* stmt);
    void typeCheckForStmt(ASTForStmt* stmt);
    void typeCheckGuardStmt(ASTGuardStmt* stmt);
    void typeCheckIfStmt(ASTIfStmt* stmt);
    void typeCheckSwitchStmt(ASTSwitchStmt* stmt);
    void typeCheckCaseStmt(ASTCaseStmt* stmt);
    void typeCheckDoStmt(ASTDoStmt* stmt);
    void typeCheckWhileStmt(ASTWhileStmt* stmt);
    void typeCheckConditions(ASTBranchStmt* stmt);

    bool isCompoundStmtAlwaysReturning(ASTCompoundStmt* stmt);
    bool isIfStmtAlwaysReturning(ASTIfStmt* stmt);
    bool isSwitchStmtAlwaysRetuning(ASTSwitchStmt* stmt);
    void checkIsSwitchStmtExhaustive(ASTSwitchStmt* stmt);
    bool isExprLValue(ASTExpr* expr);
     */
};
