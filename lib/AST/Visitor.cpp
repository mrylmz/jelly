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

#include "AST/ArrayTypeRef.h"
#include "AST/BinaryExpression.h"
#include "AST/BlockStatement.h"
#include "AST/BoolLiteral.h"
#include "AST/BranchStatement.h"
#include "AST/BreakStatement.h"
#include "AST/CallExpression.h"
#include "AST/CaseStatement.h"
#include "AST/ConditionalCaseStatement.h"
#include "AST/ConstantDeclaration.h"
#include "AST/ContinueStatement.h"
#include "AST/ControlStatement.h"
#include "AST/Declaration.h"
#include "AST/DeferStatement.h"
#include "AST/DoStatement.h"
#include "AST/ElseCaseStatement.h"
#include "AST/EnumerationDeclaration.h"
#include "AST/EnumerationElementDeclaration.h"
#include "AST/Expression.h"
#include "AST/FallthroughStatement.h"
#include "AST/FloatLiteral.h"
#include "AST/FunctionDeclaration.h"
#include "AST/GuardStatement.h"
#include "AST/IdentifierExpression.h"
#include "AST/IfStatement.h"
#include "AST/IntLiteral.h"
#include "AST/LoadDirective.h"
#include "AST/LoopStatement.h"
#include "AST/MemberAccessExpression.h"
#include "AST/ModuleDeclaration.h"
#include "AST/NamedDeclaration.h"
#include "AST/NilLiteral.h"
#include "AST/Node.h"
#include "AST/OpaqueTypeRef.h"
#include "AST/ParameterDeclaration.h"
#include "AST/PointerTypeRef.h"
#include "AST/ReturnStatement.h"
#include "AST/Statement.h"
#include "AST/StringLiteral.h"
#include "AST/StructureDeclaration.h"
#include "AST/SubscriptExpression.h"
#include "AST/SwitchStatement.h"
#include "AST/TypeDeclaration.h"
#include "AST/TypeOfTypeRef.h"
#include "AST/TypeRef.h"
#include "AST/UnaryExpression.h"
#include "AST/ValueDeclaration.h"
#include "AST/VariableDeclaration.h"
#include "AST/Visitor.h"
#include "AST/WhileStatement.h"

using namespace jelly::AST;

void Visitor::visitNode(Node* node) {
    node->accept(*this);
}

void Visitor::visitStatement(Statement* statement) { }

void Visitor::visitBlockStatement(BlockStatement* statement) {
    visitStatement(statement);
}

void Visitor::visitBranchStatement(BranchStatement* statement) {
    visitStatement(statement);
}

void Visitor::visitGuardStatement(GuardStatement* statement) {
    visitBranchStatement(statement);
}

void Visitor::visitIfStatement(IfStatement* statement) {
    visitBranchStatement(statement);
}

void Visitor::visitLoopStatement(LoopStatement* statement) {
    visitBranchStatement(statement);
}

void Visitor::visitDoStatement(DoStatement* statement) {
    visitLoopStatement(statement);
}

void Visitor::visitWhileStatement(WhileStatement* statement) {
    visitLoopStatement(statement);
}

void Visitor::visitCaseStatement(CaseStatement* statement) {
    visitStatement(statement);
}

void Visitor::visitConditionalCaseStatement(ConditionalCaseStatement* statement) {
    visitCaseStatement(statement);
}

void Visitor::visitElseCaseStatement(ElseCaseStatement* statement) {
    visitCaseStatement(statement);
}

void Visitor::visitControlStatement(ControlStatement* statement) {
    visitStatement(statement);
}

void Visitor::visitBreakStatement(BreakStatement* statement) {
    visitControlStatement(statement);
}

void Visitor::visitContinueStatement(ContinueStatement* statement) {
    visitControlStatement(statement);
}

void Visitor::visitFallthroughStatement(FallthroughStatement* statement) {
    visitControlStatement(statement);
}

void Visitor::visitReturnStatement(ReturnStatement* statement) {
    visitControlStatement(statement);
}

void Visitor::visitDeferStatement(DeferStatement* statement) {
    visitStatement(statement);
}

void Visitor::visitSwitchStatement(SwitchStatement* statement) {
    visitStatement(statement);
}

void Visitor::visitDeclaration(Declaration* declaration) {
    visitStatement(declaration);
}

void Visitor::visitLoadDirective(LoadDirective* declaration) {
    visitDeclaration(declaration);
}

void Visitor::visitNamedDeclaration(NamedDeclaration* declaration) {
    visitDeclaration(declaration);
}

void Visitor::visitModuleDeclaration(ModuleDeclaration* declaration) {
    visitNamedDeclaration(declaration);
}

void Visitor::visitEnumerationElementDeclaration(EnumerationElementDeclaration* declaration) {
    visitNamedDeclaration(declaration);
}

void Visitor::visitParameterDeclaration(ParameterDeclaration* declaration) {
    visitNamedDeclaration(declaration);
}

void Visitor::visitTypeDeclaration(TypeDeclaration* declaration) {
    visitNamedDeclaration(declaration);
}

void Visitor::visitEnumerationDeclaration(EnumerationDeclaration* declaration) {
    visitTypeDeclaration(declaration);
}

void Visitor::visitFunctionDeclaration(FunctionDeclaration* declaration) {
    visitTypeDeclaration(declaration);
}

void Visitor::visitStructureDeclaration(StructureDeclaration* declaration) {
    visitTypeDeclaration(declaration);
}

void Visitor::visitValueDeclaration(ValueDeclaration* declaration) {
    visitNamedDeclaration(declaration);
}

void Visitor::visitConstantDeclaration(ConstantDeclaration* declaration) {
    visitValueDeclaration(declaration);
}

void Visitor::visitVariableDeclaration(VariableDeclaration* declaration) {
    visitValueDeclaration(declaration);
}

void Visitor::visitExpression(Expression* expression) {
    visitStatement(expression);
}

void Visitor::visitUnaryExpression(UnaryExpression* expression) {
    visitExpression(expression);
}

void Visitor::visitBinaryExpression(BinaryExpression* expression) {
    visitExpression(expression);
}

void Visitor::visitIdentifierExpression(IdentifierExpression* expression) {
    visitExpression(expression);
}

void Visitor::visitMemberAccessExpression(MemberAccessExpression* expression) {
    visitExpression(expression);
}

void Visitor::visitCallExpression(CallExpression* expression) {
    visitExpression(expression);
}

void Visitor::visitSubscriptExpression(SubscriptExpression* expression) {
    visitExpression(expression);
}

void Visitor::visitLiteral(Literal* literal) {
    visitExpression(literal);
}

void Visitor::visitNilLiteral(NilLiteral* literal) {
    visitLiteral(literal);
}

void Visitor::visitBoolLiteral(BoolLiteral* literal) {
    visitLiteral(literal);
}

void Visitor::visitIntLiteral(IntLiteral* literal) {
    visitLiteral(literal);
}

void Visitor::visitFloatLiteral(FloatLiteral* literal) {
    visitLiteral(literal);
}

void Visitor::visitStringLiteral(StringLiteral* literal) {
    visitLiteral(literal);
}

void Visitor::visitTypeRef(TypeRef* typeRef) { }

void Visitor::visitOpaqueTypeRef(OpaqueTypeRef* typeRef) {
    visitTypeRef(typeRef);
}

void Visitor::visitTypeOfTypeRef(TypeOfTypeRef* typeRef) {
    visitTypeRef(typeRef);
}

void Visitor::visitPointerTypeRef(PointerTypeRef* typeRef) {
    visitTypeRef(typeRef);
}

void Visitor::visitArrayTypeRef(ArrayTypeRef* typeRef) {
    visitTypeRef(typeRef);
}

