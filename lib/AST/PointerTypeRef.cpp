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

#include "AST/PointerTypeRef.h"
#include "AST/Visitor.h"

#include <assert.h>

using namespace jelly::AST;

PointerTypeRef::PointerTypeRef(TypeRef* pointeeTypeRef, uint32_t depth) :
TypeRef(Kind::PointerTypeRef),
pointeeTypeRef(nullptr),
depth(depth) {
    assert(depth > 0);
    setPointeeTypeRef(pointeeTypeRef);
}

TypeRef* PointerTypeRef::getPointeeTypeRef() const {
    return pointeeTypeRef;
}

void PointerTypeRef::setPointeeTypeRef(TypeRef* pointeeTypeRef) {
    if (pointeeTypeRef) {
        pointeeTypeRef->setParent(this);
    }

    if (this->pointeeTypeRef) {
        this->pointeeTypeRef->setParent(nullptr);
    }

    this->pointeeTypeRef = pointeeTypeRef;
}

uint32_t PointerTypeRef::getDepth() const {
    return depth;
}

void PointerTypeRef::setDepth(uint32_t depth) {
    assert(depth > 0);
    this->depth = depth;
}

void PointerTypeRef::accept(Visitor &visitor) {
    visitor.visitPointerTypeRef(this);

    if (getPointeeTypeRef()) {
        getPointeeTypeRef()->accept(visitor);
    }
}
