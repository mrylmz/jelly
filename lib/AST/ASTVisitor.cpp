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
#include "AST/ASTVisitor.h"
#include "AST/ASTNodes.h"

void ASTVisitor::visit(const ASTNode* node) {
    if (node == nullptr) {
        return;
    }

    switch (node->kind) {
        case AST_LOAD:
            return visit(reinterpret_cast<const ASTLoad*>(node));

        case AST_LITERAL:
            return visit(reinterpret_cast<const ASTLiteral*>(node));

        case AST_FUNC:
            return visit(reinterpret_cast<const ASTFunc*>(node));

        case AST_FUNC_SIGNATURE:
            return visit(reinterpret_cast<const ASTFuncSignature*>(node));

        case AST_BLOCK:
            return visit(reinterpret_cast<const ASTBlock*>(node));

        case AST_PARAMETER:
            return visit(reinterpret_cast<const ASTParameter*>(node));

        case AST_STRUCT:
            return visit(reinterpret_cast<const ASTStruct*>(node));

        case AST_VARIABLE:
            return visit(reinterpret_cast<const ASTVariable*>(node));

        case AST_ENUM:
            return visit(reinterpret_cast<const ASTEnum*>(node));

        case AST_ENUM_ELEMENT:
            return visit(reinterpret_cast<const ASTEnumElement*>(node));

        case AST_IDENTIFIER:
            return visit(reinterpret_cast<const ASTIdentifier*>(node));

        case AST_UNARY:
            return visit(reinterpret_cast<const ASTUnaryExpression*>(node));

        case AST_BINARY:
            return visit(reinterpret_cast<const ASTBinaryExpression*>(node));

        case AST_CONTROL:
            return visit(reinterpret_cast<const ASTControl*>(node));

        case AST_TYPE:
            return visit(reinterpret_cast<const ASTType*>(node));

        case AST_DEFER:
            return visit(reinterpret_cast<const ASTDefer*>(node));

        case AST_FOR:
            return visit(reinterpret_cast<const ASTFor*>(node));

        case AST_GUARD:
            return visit(reinterpret_cast<const ASTGuard*>(node));

        case AST_IF:
            return visit(reinterpret_cast<const ASTIf*>(node));

        case AST_SWITCH:
            return visit(reinterpret_cast<const ASTSwitch*>(node));

        case AST_SWITCH_CASE:
            return visit(reinterpret_cast<const ASTSwitchCase*>(node));

        case AST_LOOP:
            return visit(reinterpret_cast<const ASTLoop*>(node));

        case AST_CALL:
            return visit(reinterpret_cast<const ASTCall*>(node));

        case AST_SUBSCRIPT:
            return visit(reinterpret_cast<const ASTSubscript*>(node));

        default:
            return;
    }
}
