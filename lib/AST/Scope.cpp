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

#include "AST/Context.h"
#include "AST/ModuleDeclaration.h"
#include "AST/NamedDeclaration.h"
#include "AST/Node.h"
#include "AST/Scope.h"

using namespace jelly;
using namespace jelly::AST;

Scope::Scope(Kind kind, Node* head) :
kind(kind),
head(head) {
    assert(head);
}

Scope::Kind Scope::getKind() const {
    return kind;
}

Scope::BlockKind Scope::getBlockKind() const {
    if (head->isBlockStatement() && head->getParent()) {
        auto parent = head->getParent();
        if (parent->isLoopStatement()) {
            return BlockKind::Loop;
        }

        if (parent->isGuardStatement()) {
            return BlockKind::Guard;
        }

        if (parent->isIfStatement()) {
            return BlockKind::If;
        }

        if (parent->isCaseStatement()) {
            return BlockKind::Case;
        }
    }

    return BlockKind::Default;
}

Scope* Scope::getParent() const {
    auto parent = head->getParent();
    while (parent) {
        auto scope = parent->getScope();
        if (scope != this) {
            return scope;
        }
    }

    return nullptr;
}

Node* Scope::getHead() const {
    return head;
}

SourceBuffer Scope::getSourceBuffer() const {
    // @Todo return source buffer of scope!
    report_fatal_error("Implementation missing!");
}

SourceRange Scope::getSourceRange() const {
    // @Todo return source range of scope!
    report_fatal_error("Implementation missing!");
}

ArrayRef<Scope*> Scope::getChildren() const {
    // @Todo return child scopes by traversing children of head!
    report_fatal_error("Implementation missing!");
}

bool Scope::insertDeclaration(NamedDeclaration* declaration) {
    if (lookupDeclaration(declaration->getName())) {
        return false;
    }

    auto success = declarations.try_emplace(declaration->getName(), declaration).second;
    if (success) {
        return true;
    }

    return false;
}

NamedDeclaration* Scope::lookupDeclaration(StringRef name) {
    return declarations.lookup(name);
}
