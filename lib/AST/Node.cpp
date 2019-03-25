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

#include "AST/Context.h"
#include "AST/Node.h"

using namespace jelly::AST;

Node::Node(Kind kind) :
kind(kind),
parent(nullptr) {

}

Node::Kind Node::getKind() const {
    return kind;
}

Node* Node::getParent() const {
    return parent;
}

void Node::setParent(Node* parent) {
    assert(parent == nullptr || getParent() == nullptr);

    this->parent = parent;
}

Scope* Node::getScope() {
    if (parent) {
        return parent->getScope();
    }

    return nullptr;
}

bool Node::isStatement() const {
    return Kind::_StmtBegin < kind && kind < Kind::_StmtEnd;
}

bool Node::isBlockStatement() const {
    return Kind::Block == kind;
}

bool Node::isBranchStatement() const {
    return Kind::_BranchStmtBegin < kind && kind < Kind::_BranchStmtEnd;
}

bool Node::isGuardStatement() const {
    return Kind::GuardStmt == kind;
}

bool Node::isIfStatement() const {
    return Kind::IfStmt == kind;
}

bool Node::isLoopStatement() const {
    return Kind::_LoopStmtBegin < kind && kind < Kind::_LoopStmtEnd;
}

bool Node::isDoStatement() const {
    return Kind::DoStmt == kind;
}

bool Node::isWhileStatement() const {
    return Kind::WhileStmt == kind;
}

bool Node::isCaseStatement() const {
    return Kind::_CaseStmtBegin < kind && kind < Kind::_CaseStmtEnd;
}

bool Node::isConditionalCaseStatement() const {
    return Kind::ConditionalCaseStmt == kind;
}

bool Node::isElseCaseStatement() const {
    return Kind::ElseCaseStmt == kind;
}

bool Node::isControlStatement() const {
    return Kind::_ControlStmtBegin < kind && kind < Kind::_ControlStmtEnd;
}

bool Node::isBreakStatement() const {
    return Kind::BreakStmt == kind;
}

bool Node::isContinueStatement() const {
    return Kind::ContinueStmt == kind;
}

bool Node::isFallthroughStatement() const {
    return Kind::FallthroughStmt == kind;
}

bool Node::isReturnStatement() const {
    return Kind::ReturnStmt == kind;
}

bool Node::isDeferStatement() const {
    return Kind::Defer == kind;
}

bool Node::isDeclaration() const {
    return Kind::_DeclBegin < kind && kind < Kind::_DeclEnd;
}

bool Node::isLoadDeclaration() const {
    return Kind::LoadDecl == kind;
}

bool Node::isNamedDeclaration() const {
    return Kind::_NamedDeclBegin < kind && kind < Kind::_NamedDeclEnd;
}

bool Node::isModuleDeclaration() const {
    return Kind::Module == kind;
}

bool Node::isEnumerationElementDeclaration() const {
    return Kind::EnumerationElement == kind;
}

bool Node::isParameterDeclaration() const {
    return Kind::Parameter == kind;
}

bool Node::isTypeDeclaration() const {
    return Kind::_TypeDeclBegin < kind && kind < Kind::_TypeDeclEnd;
}

bool Node::isEnumerationDeclaration() const {
    return Kind::Enumeration == kind;
}

bool Node::isFunctionDeclaration() const {
    return Kind::Function == kind;
}

bool Node::isStructureDeclaration() const {
    return Kind::Structure == kind;
}

bool Node::isValueDeclaration() const {
    return Kind::_ValueDeclBegin < kind && kind < Kind::_ValueDeclEnd;
}

bool Node::isConstantDeclaration() const {
    return Kind::Constant == kind;
}

bool Node::isVariableDeclaration() const {
    return Kind::Variable == kind;
}

bool Node::isExpression() const {
    return Kind::_ExprBegin < kind && kind < Kind::_ExprEnd;
}

bool Node::isUnaryExpression() const {
    return Kind::UnaryExpr == kind;
}

bool Node::isBinaryExpression() const {
    return Kind::BinaryExpr == kind;
}

bool Node::isIdentifierExpression() const {
    return Kind::IdentifierExpr == kind;
}

bool Node::isMemberAccessExpression() const {
    return Kind::MemberAccessExpr == kind;
}

bool Node::isCallExpression() const {
    return Kind::CallExpr == kind;
}

bool Node::isLiteral() const {
    return Kind::_LitBegin < kind && kind < Kind::_LitEnd;
}

bool Node::isNilLiteral() const {
    return Kind::NilLit == kind;
}

bool Node::isBoolLiteral() const {
    return Kind::BoolLit == kind;
}

bool Node::isIntLiteral() const {
    return Kind::IntLit == kind;
}

bool Node::isFloatLiteral() const {
    return Kind::FloatLit == kind;
}

bool Node::isStringLiteral() const {
    return Kind::StringLit == kind;
}

bool Node::isSwitchStatement() const {
    return Kind::SwitchStmt == kind;
}

bool Node::isTypeRef() const {
    return Kind::_TypeRefBegin < kind && kind < Kind::_TypeRefEnd;
}

bool Node::isOpaqueTypeRef() const {
    return Kind::OpaqueTypeRef == kind;
}

bool Node::isTypeOfTypeRef() const {
    return Kind::TypeOfTypeRef == kind;
}

bool Node::isPointerTypeRef() const {
    return Kind::PointerTypeRef == kind;
}

bool Node::isArrayTypeRef() const {
    return Kind::ArrayTypeRef == kind;
}

bool Node::containsChild(Node* node) const {
    return node->getParent() == this;
}

void* Node::operator new (size_t size, Context* context) {
    return context->allocator.Allocate(size, 8);
}
