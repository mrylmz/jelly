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

#include "Core/AST.h"

#include <ostream>

struct ASTDumper {
    std::ostream& outputStream;
    unsigned indentation = 0;

    ASTDumper(std::ostream& outputStream);

    void dumpModule(ASTModuleDecl* module);

private:

    void dumpNode(ASTNode* node);

    void dumpChildren(jelly::SmallVector<ASTNode*, 0> children) {
        dumpChildren(jelly::makeArrayRef(children));
    }

    template<typename T>
    void dumpChildren(jelly::ArrayRef<T*> children) {
        indentation += 1;

        std::string indentText = "";
        for (auto i = 0; i < indentation - 1; i++) {
            indentText.append("  ");
        }

        if (indentation > 0) {
            indentText.append("| ");
        }

        for (auto child : children) {
            outputStream << indentText;
            dumpNode(child);
        }
        indentation -= 1;
    }

    void dumpDeclContext(DeclContext* declContext) {
        indentation += 1;

        std::string indentText = "";
        for (auto i = 0; i < indentation - 1; i++) {
            indentText.append("  ");
        }

        if (indentation > 0) {
            indentText.append("| ");
        }

        for (auto it = declContext->declsBegin(); it != declContext->declsEnd(); it++) {
            outputStream << indentText;
            dumpNode(*it);
        }

        indentation -= 1;
    }

    void dumpCompoundStmt(ASTCompoundStmt* stmt);
    void dumpLoad(ASTLoadDirective* directive);
    void dumpUnaryExpr(ASTUnaryExpr* expr);
    void dumpBinaryExpr(ASTBinaryExpr* expr);
    void dumpMemberAccessExpr(ASTMemberAccessExpr* expr);
    void dumpIdentExpr(ASTIdentExpr* expr);
    void dumpCallExpr(ASTCallExpr* expr);
    void dumpSubscriptExpr(ASTSubscriptExpr* expr);
    void dumpNilLiteral(ASTNilLit* literal);
    void dumpBoolLiteral(ASTBoolLit* literal);
    void dumpIntLiteral(ASTIntLit* literal);
    void dumpFloatLiteral(ASTFloatLit* literal);
    void dumpStringLiteral(ASTStringLit* literal);
    void dumpParamDecl(ASTParamDecl* decl);
    void dumpFuncDecl(ASTFuncDecl* decl);
    void dumpValueDecl(ASTValueDecl* decl);
    void dumpStructDecl(ASTStructDecl* decl);
    void dumpEnumElementDecl(ASTEnumElementDecl* decl);
    void dumpEnumDecl(ASTEnumDecl* decl);
    void dumpBreakStmt(ASTBreakStmt* stmt);
    void dumpContinueStmt(ASTContinueStmt* stmt);
    void dumpFallthroughStmt(ASTFallthroughStmt* stmt);
    void dumpReturnStmt(ASTReturnStmt* stmt);
    void dumpDeferStmt(ASTDeferStmt* stmt);
    void dumpForStmt(ASTForStmt* stmt);
    void dumpGuardStmt(ASTGuardStmt* stmt);
    void dumpIfStmt(ASTIfStmt* stmt);
    void dumpDoStmt(ASTDoStmt* stmt);
    void dumpWhileStmt(ASTWhileStmt* stmt);
    void dumpCaseStmt(ASTCaseStmt* stmt);
    void dumpSwitchStmt(ASTSwitchStmt* stmt);
    void dumpAnyTypeRef(ASTAnyTypeRef* typeRef);
    void dumpOpaqueTypeRef(ASTOpaqueTypeRef* typeRef);
    void dumpTypeOfTypeRef(ASTTypeOfTypeRef* typeRef);
    void dumpPointerTypeRef(ASTPointerTypeRef* typeRef);
    void dumpArrayTypeRef(ASTArrayTypeRef* typeRef);
};
