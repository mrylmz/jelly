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

#include "AST/StructureDeclaration.h"
#include "AST/ValueDeclaration.h"
#include "AST/Visitor.h"

using namespace jelly;
using namespace jelly::AST;

StructureDeclaration::StructureDeclaration(Identifier name, ArrayRef<ValueDeclaration*> children) :
TypeDeclaration(Kind::Structure, name, nullptr),
scope(Scope::Kind::StructureDeclaration, this) {
    for (auto child : children) {
        addChild(child);
    }
}

Scope* StructureDeclaration::getScope() {
    return &scope;
}

ArrayRef<ValueDeclaration*> StructureDeclaration::getChildren() const {
    return children;
}

void StructureDeclaration::addChild(ValueDeclaration *child) {
    child->setParent(this);
    children.push_back(child);
}

void StructureDeclaration::accept(Visitor &visitor) {
    visitor.visitStructureDeclaration(this);

    for (auto child : getChildren()) {
        child->accept(visitor);
    }
}
