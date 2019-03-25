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

#include "AST/BlockStatement.h"
#include "AST/ConditionalCaseStatement.h"
#include "AST/Expression.h"
#include "AST/Visitor.h"

using namespace jelly::AST;

ConditionalCaseStatement::ConditionalCaseStatement(Expression* condition, BlockStatement* body) :
CaseStatement(Kind::ConditionalCaseStmt, body),
condition(nullptr) {
    setCondition(condition);
}

Expression* ConditionalCaseStatement::getCondition() const {
    return condition;
}

void ConditionalCaseStatement::setCondition(Expression* condition) {
    if (condition) {
        condition->setParent(this);
    }

    if (this->condition) {
        this->condition->setParent(nullptr);
    }

    this->condition = condition;
}

void ConditionalCaseStatement::accept(Visitor &visitor) {
    visitor.visitConditionalCaseStatement(this);

    if (getCondition()) {
        getCondition()->accept(visitor);
    }

    if (getBody()) {
        getBody()->accept(visitor);
    }
}
