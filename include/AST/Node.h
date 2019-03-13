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

#include <stddef.h>

namespace jelly {
namespace AST {

    class Context;
    class Dumper;
    class Visitor;

    class Node {
        friend class Dumper;

    protected:

        enum class Kind {
            _StmtBegin,
             Block,
             _BranchStmtBegin,
              GuardStmt,
              IfStmt,
              _LoopStmtBegin,
               DoStmt,
               WhileStmt,
              _LoopStmtEnd,
             _BranchStmtEnd,
             _CaseStmtBegin,
              ConditionalCaseStmt,
              ElseCaseStmt,
             _CaseStmtEnd,
             _ControlStmtBegin,
              BreakStmt,
              ContinueStmt,
              FallthroughStmt,
              ReturnStmt,
             _ControlStmtEnd,
             Defer,
             _DeclBegin,
              LoadDecl,
              _NamedDeclBegin,
               Module,
               EnumerationElement,
               Parameter,
               _TypeDeclBegin,
                Enumeration,
                Function,
                Structure,
               _TypeDeclEnd,
               _ValueDeclBegin,
                Constant,
                Variable,
               _ValueDeclEnd,
              _NamedDeclEnd,
             _DeclEnd,
             _ExprBegin,
              UnaryExpr,
              BinaryExpr,
              IdentifierExpr,
              MemberAccessExpr,
              CallExpr,
              _LitBegin,
               NilLit,
               BoolLit,
               IntLit,
               FloatLit,
               StringLit,
              _LitEnd,
             _ExprEnd,
             SwitchStmt,
            _StmtEnd,
            _TypeRefBegin,
             OpaqueTypeRef,
             TypeOfTypeRef,
             PointerTypeRef,
             ArrayTypeRef,
            _TypeRefEnd
        };

    private:
        Kind kind;
        Node* parent;

        Node() = delete;
        Node(Node&&) = delete;

        Node& operator = (Node&&) = delete;
        void operator delete (void* ptr) = delete;
        void operator delete [] (void* ptr) = delete;

    protected:

        Node(Kind kind);

        Kind getKind() const;

        void setParent(Node* parent);

        static void setParent(Node* node, Node* parent);

    public:

        Node* getParent() const;

        bool isStatement() const;
        bool isBlockStatement() const;
        bool isBranchStatement() const;
        bool isGuardStatement() const;
        bool isIfStatement() const;
        bool isLoopStatement() const;
        bool isDoStatement() const;
        bool isWhileStatement() const;
        bool isCaseStatement() const;
        bool isConditionalCaseStatement() const;
        bool isElseCaseStatement() const;
        bool isControlStatement() const;
        bool isBreakStatement() const;
        bool isContinueStatement() const;
        bool isFallthroughStatement() const;
        bool isReturnStatement() const;
        bool isDeferStatement() const;
        bool isDeclaration() const;
        bool isLoadDeclaration() const;
        bool isNamedDeclaration() const;
        bool isModuleDeclaration() const;
        bool isEnumerationElementDeclaration() const;
        bool isParameterDeclaration() const;
        bool isTypeDeclaration() const;
        bool isEnumerationDeclaration() const;
        bool isFunctionDeclaration() const;
        bool isStructureDeclaration() const;
        bool isValueDeclaration() const;
        bool isConstantDeclaration() const;
        bool isVariableDeclaration() const;
        bool isExpression() const;
        bool isUnaryExpression() const;
        bool isBinaryExpression() const;
        bool isIdentifierExpression() const;
        bool isMemberAccessExpression() const;
        bool isCallExpression() const;
        bool isLiteral() const;
        bool isNilLiteral() const;
        bool isBoolLiteral() const;
        bool isIntLiteral() const;
        bool isFloatLiteral() const;
        bool isStringLiteral() const;
        bool isSwitchStatement() const;
        bool isTypeRef() const;
        bool isOpaqueTypeRef() const;
        bool isTypeOfTypeRef() const;
        bool isPointerTypeRef() const;
        bool isArrayTypeRef() const;

        bool contains(Node* node) const;

        void* operator new (size_t size, Context* context);

        virtual void accept(Visitor &visitor) = 0;
    };
}
}
