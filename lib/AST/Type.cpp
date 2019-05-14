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
#include "AST/Type.h"

using namespace jelly::AST;

Type::Type(Kind kind) :
kind(kind) {

}

bool Type::isErrorType() const {
    return Kind::ErrorType == kind;
}

bool Type::isVoidType() const {
    return Kind::VoidType == kind;
}

bool Type::isIntegerType() const {
    return Kind::IntegerType == kind;
}

bool Type::isFloatType() const {
    return Kind::FloatType == kind;
}

bool Type::isEnumerationType() const {
    return Kind::EnumerationType == kind;
}

bool Type::isFunctionType() const {
    return Kind::FunctionType == kind;
}

bool Type::isStructureType() const {
    return Kind::StructureType == kind;
}

bool Type::isArrayType() const {
    return Kind::ArrayType == kind;
}

bool Type::isPointerType() const {
    return Kind::PointerType == kind;
}

void* Type::operator new (size_t size, Context* context) {
    return context->allocator.Allocate(size, 8);
}