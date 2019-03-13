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
#include "AST/Visitor.h"

using namespace jelly;
using namespace jelly::AST;

BlockStatement::BlockStatement(ArrayRef<Statement*> statements) :
Statement(Kind::Block) {
    for (auto statement : statements) {
        addStatement(statement);
    }
}

void BlockStatement::addDeclaration(Declaration* declaration) {
    assert(declaration->getParent() == nullptr);

    if (declaration->isValueDeclaration()) {
        Node::setParent(declaration, this);
        return values.push_back(reinterpret_cast<ValueDeclaration*>(declaration));
    }

    report_fatal_error("Invalid declaration type added to Module!");
}

ArrayRef<ValueDeclaration*> BlockStatement::getValueDeclarations() const {
    return values;
}

ArrayRef<Statement*> BlockStatement::getStatements() const {
    return statements;
}

Declaration* BlockStatement::lookupDeclaration(StringRef name) const {
    for (auto value : values) {
        if (value->getName()->equals(name)) {
            return value;
        }
    }

    return nullptr;
}

void BlockStatement::addStatement(Statement* statement) {
    assert(statement->getParent() == nullptr);

    if (statement->isValueDeclaration()) {
        addDeclaration(reinterpret_cast<ValueDeclaration*>(statement));
    } else {
        Node::setParent(statement, this);
    }

    statements.push_back(statement);
}

void BlockStatement::accept(Visitor &visitor) {
    visitor.visitBlockStatement(this);

    for (auto statement : getStatements()) {
        statement->accept(visitor);
    }
}
