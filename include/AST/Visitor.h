//
// MIT License
//
// Copyright (c) 2019 Murat Yilmaz
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

namespace jelly {
namespace AST {

    class Statement;
    class BlockStatement;
    class BranchStatement;
    class GuardStatement;
    class IfStatement;
    class LoopStatement;
    class DoStatement;
    class WhileStatement;
    class CaseStatement;
    class ConditionalCaseStatement;
    class ElseCaseStatement;
    class ControlStatement;
    class BreakStatement;
    class ContinueStatement;
    class FallthroughStatement;
    class ReturnStatement;
    class DeferStatement;
    class SwitchStatement;
    class Declaration;
    class LoadDeclaration;
    class NamedDeclaration;
    class ModuleDeclaration;
    class EnumerationElementDeclaration;
    class ParameterDeclaration;
    class TypeDeclaration;
    class EnumerationDeclaration;
    class FunctionDeclaration;
    class StructureDeclaration;
    class ValueDeclaration;
    class ConstantDeclaration;
    class VariableDeclaration;
    class Expression;
    class UnaryExpression;
    class BinaryExpression;
    class IdentifierExpression;
    class MemberAccessExpression;
    class CallExpression;
    class SubscriptExpression;
    class Literal;
    class NilLiteral;
    class Node;
    class BoolLiteral;
    class IntLiteral;
    class FloatLiteral;
    class StringLiteral;
    class TypeRef;
    class OpaqueTypeRef;
    class TypeOfTypeRef;
    class PointerTypeRef;
    class ArrayTypeRef;

    class Visitor {

    public:

        void visitNode(Node* node);
        
        virtual void visitStatement(Statement* statement);
        virtual void visitBlockStatement(BlockStatement* statement);
        virtual void visitBranchStatement(BranchStatement* statement);
        virtual void visitGuardStatement(GuardStatement* statement);
        virtual void visitIfStatement(IfStatement* statement);
        virtual void visitLoopStatement(LoopStatement* statement);
        virtual void visitDoStatement(DoStatement* statement);
        virtual void visitWhileStatement(WhileStatement* statement);
        virtual void visitCaseStatement(CaseStatement* statement);
        virtual void visitConditionalCaseStatement(ConditionalCaseStatement* statement);
        virtual void visitElseCaseStatement(ElseCaseStatement* statement);
        virtual void visitControlStatement(ControlStatement* statement);
        virtual void visitBreakStatement(BreakStatement* statement);
        virtual void visitContinueStatement(ContinueStatement* statement);
        virtual void visitFallthroughStatement(FallthroughStatement* statement);
        virtual void visitReturnStatement(ReturnStatement* statement);
        virtual void visitDeferStatement(DeferStatement* statement);
        virtual void visitSwitchStatement(SwitchStatement* statement);

        virtual void visitDeclaration(Declaration* declaration);
        virtual void visitLoadDeclaration(LoadDeclaration* declaration);
        virtual void visitNamedDeclaration(NamedDeclaration* declaration);
        virtual void visitModuleDeclaration(ModuleDeclaration* declaration);
        virtual void visitEnumerationElementDeclaration(EnumerationElementDeclaration* declaration);
        virtual void visitParameterDeclaration(ParameterDeclaration* declaration);
        virtual void visitTypeDeclaration(TypeDeclaration* declaration);
        virtual void visitEnumerationDeclaration(EnumerationDeclaration* declaration);
        virtual void visitFunctionDeclaration(FunctionDeclaration* declaration);
        virtual void visitStructureDeclaration(StructureDeclaration* declaration);
        virtual void visitValueDeclaration(ValueDeclaration* declaration);
        virtual void visitConstantDeclaration(ConstantDeclaration* declaration);
        virtual void visitVariableDeclaration(VariableDeclaration* declaration);

        virtual void visitExpression(Expression* expression);
        virtual void visitUnaryExpression(UnaryExpression* expression);
        virtual void visitBinaryExpression(BinaryExpression* expression);
        virtual void visitIdentifierExpression(IdentifierExpression* expression);
        virtual void visitMemberAccessExpression(MemberAccessExpression* expression);
        virtual void visitCallExpression(CallExpression* expression);
        virtual void visitSubscriptExpression(SubscriptExpression* expression);

        virtual void visitLiteral(Literal* literal);
        virtual void visitNilLiteral(NilLiteral* literal);
        virtual void visitBoolLiteral(BoolLiteral* literal);
        virtual void visitIntLiteral(IntLiteral* literal);
        virtual void visitFloatLiteral(FloatLiteral* literal);
        virtual void visitStringLiteral(StringLiteral* literal);

        virtual void visitTypeRef(TypeRef* typeRef);
        virtual void visitOpaqueTypeRef(OpaqueTypeRef* typeRef);
        virtual void visitTypeOfTypeRef(TypeOfTypeRef* typeRef);
        virtual void visitPointerTypeRef(PointerTypeRef* typeRef);
        virtual void visitArrayTypeRef(ArrayTypeRef* typeRef);
    };
}
}
