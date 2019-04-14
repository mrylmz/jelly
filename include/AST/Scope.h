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

#pragma once

#include "AST/SymbolTable.h"

#include <Basic/Basic.h>
#include <map>

namespace jelly {
namespace AST {

    class BlockStatement;
    class EnumerationDeclaration;
    class FunctionDeclaration;
    class Node;
    class NamedDeclaration;
    class ModuleDeclaration;
    class StructureDeclaration;

    class Scope {
        friend class BlockStatement;
        friend class EnumerationDeclaration;
        friend class FunctionDeclaration;
        friend class ModuleDeclaration;
        friend class StructureDeclaration;

    public:
        enum class Kind : uint8_t {
            BlockStatement,
            EnumerationDeclaration,
            FunctionDeclaration,
            ModuleDeclaration,
            StructureDeclaration,
        };

        enum class BlockKind: uint8_t {
            Default,
            If,
            Guard,
            Case,
            Loop
        };

    private:

        Kind kind;
        Node* head;
        StringMap<NamedDeclaration*> declarations;
        std::map<Node*, NamedDeclaration*> declarationBindings;
        SymbolTable symbolTable;

        Scope(Kind kind, Node* head);

        Scope(Scope &&) = delete;
        Scope &operator=(Scope &&) = delete;
        Scope(const Scope &) = delete;
        Scope &operator=(const Scope &) = delete;

    public:

        Kind getKind() const;

        BlockKind getBlockKind() const;

        Scope* getParent() const;

        Node* getHead() const;

        SourceBuffer getSourceBuffer() const;

        SourceRange getSourceRange() const;

        ArrayRef<Scope*> getChildren() const;

        bool insertDeclaration(NamedDeclaration* declaration);

        NamedDeclaration* lookupDeclaration(StringRef name);
        NamedDeclaration* lookupDeclarationInParentHierarchy(StringRef name);

        void bindDeclaration(Node* node, NamedDeclaration* declaration);

        NamedDeclaration* lookupDeclaration(Node* node);

        SymbolTable* getSymbolTable();

        void *operator new(size_t bytes) = delete;
        void operator delete(void *data) = delete;
    };
}
}
