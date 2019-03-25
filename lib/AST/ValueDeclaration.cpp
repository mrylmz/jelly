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

#include "AST/Expression.h"
#include "AST/TypeRef.h"
#include "AST/ValueDeclaration.h"

using namespace jelly::AST;

ValueDeclaration::ValueDeclaration(Kind kind, Identifier name, TypeRef* typeRef, Expression* initializer) :
NamedDeclaration(kind, name),
typeRef(nullptr),
initializer(nullptr) {
    setTypeRef(typeRef);
    setInitializer(initializer);
}

TypeRef* ValueDeclaration::getTypeRef() const {
    return typeRef;
}

void ValueDeclaration::setTypeRef(TypeRef* typeRef) {
    if (typeRef) {
        typeRef->setParent(this);
    }

    if (this->typeRef) {
        this->typeRef->setParent(nullptr);
    }

    this->typeRef = typeRef;
}

Expression* ValueDeclaration::getInitializer() const {
    return initializer;
}

void ValueDeclaration::setInitializer(Expression* initializer) {
    if (initializer) {
        initializer->setParent(this);
    }

    if (this->initializer) {
        this->initializer->setParent(nullptr);
    }

    this->initializer = initializer;
}
