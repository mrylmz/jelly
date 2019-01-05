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

#include "Core/AST.h"
#include "Core/DeclContext.h"

DeclIterator& DeclIterator::operator ++ () {
    current = current->nextDeclInContext;
    return *this;
}

DeclIterator& DeclIterator::operator -- () {
    current = current->previousDeclInContext;
    return *this;
}

DeclIterator DeclIterator::operator + (DeclIterator::difference_type v) const {
    DeclIterator tmp(*this);
    for (auto i = 0; i < v; i++) { tmp++; }
    return tmp;
}

DeclContext::decl_iterator DeclContext::declsBegin() {
    return decl_iterator(firstDecl);
}

DeclContext::decl_iterator DeclContext::declsLast() {
    return decl_iterator(lastDecl);
}

DeclContext::decl_iterator DeclContext::declsEnd() {
    return decl_iterator();
}

DeclContext* DeclContext::getDeclContext() {
    return parentContext;
}

void DeclContext::setDeclContext(DeclContext *declContext) {
    parentContext = declContext;
}

void DeclContext::addDecl(ASTDecl* decl) {
    if (!firstDecl) {
        firstDecl = decl;
        lastDecl = decl;
        return;
    }

    lastDecl->nextDeclInContext = decl;
    decl->previousDeclInContext = lastDecl;
    lastDecl = decl;
}

bool DeclContext::containsDecl(ASTDecl *decl) {
    for (auto it = declsBegin(); it != declsEnd(); it++) {
        if ((*it) == decl) {
            return true;
        }
    }

    return false;
}

ASTDecl* DeclContext::lookupDecl(llvm::StringRef name) {
    for (auto it = declsBegin(); it != declsEnd(); it++) {
        if ((*it)->name == name) {
            return *it;
        }
    }

    return nullptr;
}

ASTDecl* DeclContext::lookupDeclInHierarchy(llvm::StringRef name) {
    auto context = this;
    while (context) {
        auto decl = context->lookupDecl(name);
        if (decl) {
            return decl;
        }

        context = context->getDeclContext();
    }

    return nullptr;
}
