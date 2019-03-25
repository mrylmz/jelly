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

#include "AST/BinaryExpression.h"
#include "AST/Visitor.h"

using namespace jelly::AST;

BinaryExpression::BinaryExpression(Operator op, Expression* left, Expression* right) :
Expression(Kind::BinaryExpr),
op(op),
left(nullptr),
right(nullptr) {
    setLeft(left);
    setRight(right);
}

Operator BinaryExpression::getOperator() const {
    return op;
}

void BinaryExpression::setOperator(Operator op) {
    this->op = op;
}

Expression* BinaryExpression::getLeft() const {
    return left;
}

void BinaryExpression::setLeft(Expression* left) {
    if (left) {
        left->setParent(this);
    }

    if (this->left) {
        this->left->setParent(nullptr);
    }

    this->left = left;
}

Expression* BinaryExpression::getRight() const {
    return right;
}

void BinaryExpression::setRight(Expression* right) {
    if (right) {
        right->setParent(this);
    }

    if (this->right) {
        this->right->setParent(nullptr);
    }

    this->right = right;
}

void BinaryExpression::accept(Visitor &visitor) {
    visitor.visitBinaryExpression(this);

    if (getLeft()) {
        getLeft()->accept(visitor);
    }

    if (getRight()) {
        getRight()->accept(visitor);
    }
}
