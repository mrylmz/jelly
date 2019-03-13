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
#include "AST/FunctionDeclaration.h"
#include "AST/ParameterDeclaration.h"
#include "AST/TypeRef.h"
#include "AST/Visitor.h"

using namespace jelly;
using namespace jelly::AST;

FunctionDeclaration::FunctionDeclaration(Identifier name, ArrayRef<ParameterDeclaration*> parameters, TypeRef* returnTypeRef, BlockStatement* body) :
TypeDeclaration(Kind::Function, name, nullptr),
returnTypeRef(returnTypeRef),
body(body) {
    for (auto parameter : parameters) { addDeclaration(parameter); }
}

ArrayRef<ParameterDeclaration*> FunctionDeclaration::getParameterDeclarations() const {
    return parameters;
}

void FunctionDeclaration::addDeclaration(ParameterDeclaration* parameter) {
    assert(parameter->getParent() == nullptr);

    Node::setParent(parameter, this);
    parameters.push_back(parameter);
}

ParameterDeclaration* FunctionDeclaration::lookupDeclaration(StringRef name) const {
    for (auto parameter : parameters) {
        if (parameter->getName()->equals(name)) {
            return parameter;
        }
    }

    return nullptr;
}

TypeRef* FunctionDeclaration::getReturnTypeRef() const {
    return returnTypeRef;
}

void FunctionDeclaration::setReturnTypeRef(TypeRef* returnTypeRef) {
    this->returnTypeRef = returnTypeRef;
}

BlockStatement* FunctionDeclaration::getBody() const {
    return body;
}

void FunctionDeclaration::setBody(BlockStatement* body) {
    this->body = body;
}

bool FunctionDeclaration::isDefinition() const {
    return getBody() != nullptr;
}

void FunctionDeclaration::accept(Visitor &visitor) {
    visitor.visitFunctionDeclaration(this);

    for (auto parameter : getParameterDeclarations()) {
        parameter->accept(visitor);
    }

    if (getReturnTypeRef()) {
        getReturnTypeRef()->accept(visitor);
    }

    if (getBody()) {
        getBody()->accept(visitor);
    }
}
