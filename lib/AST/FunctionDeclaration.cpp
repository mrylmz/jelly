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
returnTypeRef(nullptr),
body(nullptr),
scope(Scope::Kind::FunctionDeclaration, this) {
    for (auto parameter : parameters) {
        addParameter(parameter);
    }

    setReturnTypeRef(returnTypeRef);
    setBody(body);
}

Scope* FunctionDeclaration::getScope() {
    return &scope;
}

ArrayRef<ParameterDeclaration*> FunctionDeclaration::getParameters() const {
    return parameters;
}

void FunctionDeclaration::addParameter(ParameterDeclaration *parameter) {
    parameter->setParent(this);
    parameters.push_back(parameter);
}

TypeRef* FunctionDeclaration::getReturnTypeRef() const {
    return returnTypeRef;
}

void FunctionDeclaration::setReturnTypeRef(TypeRef* returnTypeRef) {
    if (returnTypeRef) {
        returnTypeRef->setParent(this);
    }

    if (this->returnTypeRef) {
        this->returnTypeRef->setParent(nullptr);
    }

    this->returnTypeRef = returnTypeRef;
}

BlockStatement* FunctionDeclaration::getBody() const {
    return body;
}

void FunctionDeclaration::setBody(BlockStatement* body) {
    if (body) {
        body->setParent(this);
    }

    if (this->body) {
        this->body->setParent(nullptr);
    }

    this->body = body;
}

bool FunctionDeclaration::isDefinition() const {
    return getBody() != nullptr;
}

void FunctionDeclaration::accept(Visitor &visitor) {
    visitor.visitFunctionDeclaration(this);

    for (auto parameter : getParameters()) {
        parameter->accept(visitor);
    }

    if (getReturnTypeRef()) {
        getReturnTypeRef()->accept(visitor);
    }

    if (getBody()) {
        getBody()->accept(visitor);
    }
}
