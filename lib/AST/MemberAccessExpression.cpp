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

#include "AST/MemberAccessExpression.h"
#include "AST/Visitor.h"

using namespace jelly::AST;

MemberAccessExpression::MemberAccessExpression(Expression* left, Identifier memberName) :
Expression(Kind::MemberAccessExpr),
left(nullptr),
memberName(memberName) {
    setLeft(left);
}

Expression* MemberAccessExpression::getLeft() const {
    return left;
}

void MemberAccessExpression::setLeft(Expression* left) {
    if (left) {
        left->setParent(this);
    }

    if (this->left) {
        this->left->setParent(nullptr);
    }

    this->left = left;
}

Identifier MemberAccessExpression::getMemberName() const {
    return memberName;
}

void MemberAccessExpression::setMemberName(Identifier memberName) {
    this->memberName = memberName;
}

void MemberAccessExpression::accept(Visitor &visitor) {
    visitor.visitMemberAccessExpression(this);

    if (getLeft()) {
        getLeft()->accept(visitor);
    }
}
