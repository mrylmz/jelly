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

#include <llvm/ADT/StringRef.h>
#include <iterator>
#include <type_traits>

struct ASTDecl;

class DeclIterator {
public:
    typedef ASTDecl*                  value_type;
    typedef int                       difference_type;
    typedef value_type*               pointer;
    typedef value_type&               reference;
    typedef std::forward_iterator_tag iterator_category;

    DeclIterator() : current(nullptr) {}
    DeclIterator(value_type ptr) : current(ptr) {}
    ~DeclIterator() {}

    DeclIterator operator ++ (int) { DeclIterator tmp(current); ++(*this); return tmp; }
    DeclIterator& operator ++ ();
    DeclIterator operator -- (int) { DeclIterator tmp(current); --(*this); return tmp; }
    DeclIterator& operator -- ();
    reference operator * () { return current; }
    DeclIterator operator + (difference_type v) const;
    bool operator == (const DeclIterator& rhs) const { return current == rhs.current; }
    bool operator != (const DeclIterator& rhs) const { return current != rhs.current; }

private:
    value_type current;
};

class DeclContext {
    ASTDecl* firstDecl = nullptr;
    ASTDecl* lastDecl = nullptr;
    DeclContext* parentContext = nullptr;

public:
    using decl_iterator = DeclIterator;
    using decl_reverse_iterator = std::reverse_iterator<decl_iterator>;

    decl_iterator declsBegin();
    decl_iterator declsLast();
    decl_iterator declsEnd();

    DeclContext* getDeclContext();
    void setDeclContext(DeclContext* declContext);

    void addDecl(ASTDecl* decl);
    bool containsDecl(ASTDecl *decl);
    ASTDecl* lookupDecl(llvm::StringRef name);
    ASTDecl* lookupDeclInHierarchy(llvm::StringRef name);
};
