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

#include "Core/CodeManager.h"

ASTExpr* CodeManager::evaluateConstExpr(ASTExpr *expr) {
    switch (expr->kind) {
        case AST_IDENTIFIER: {
            auto ident = reinterpret_cast<ASTIdentExpr*>(expr);
            if (ident->decl && ident->decl->kind == AST_VALUE_DECL && ident->decl->parent == context.getModule()) {
                auto valueDecl = reinterpret_cast<ASTValueDecl*>(ident->decl);
                if (!valueDecl->isConstant || !valueDecl->initializer) {
                    return nullptr;
                }

                // @Stability this may could fall into infinite recursion if the assignment is refering to the same ASTIdentExpr ?!
                return evaluateConstExpr(valueDecl->initializer);
            }
        }   break;

        case AST_NIL_LITERAL:
        case AST_BOOL_LITERAL:
        case AST_INT_LITERAL:
        case AST_FLOAT_LITERAL:
        case AST_STRING_LITERAL:
            return expr;

        default:
            // @Incomplete other expr types are missing!
            return nullptr;
    }

    return nullptr;
}
