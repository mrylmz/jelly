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
#include "AST/Expression.h"
#include "AST/Visitor.h"

using namespace jelly::AST;

ArrayTypeRef::ArrayTypeRef(TypeRef* elementTypeRef, Expression* value) :
TypeRef(Kind::ArrayTypeRef),
elementTypeRef(elementTypeRef),
value(value) {

}

TypeRef* ArrayTypeRef::getElementTypeRef() const {
    return elementTypeRef;
}

void ArrayTypeRef::setElementTypeRef(TypeRef* elementTypeRef) {
    this->elementTypeRef = elementTypeRef;
}

Expression* ArrayTypeRef::getValue() const {
    return value;
}

void ArrayTypeRef::setValue(Expression* value) {
    this->value = value;
}

void ArrayTypeRef::accept(Visitor &visitor) {
    visitor.visitArrayTypeRef(this);

    if (getElementTypeRef()) {
        getElementTypeRef()->accept(visitor);
    }

    if (getValue()) {
        getValue()->accept(visitor);
    }
}
