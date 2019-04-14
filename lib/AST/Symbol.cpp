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

#include "AST/Symbol.h"
#include "AST/SymbolTable.h"

using namespace jelly;
using namespace jelly::AST;

Symbol::Symbol(uint32_t index) :
index(index),
node(nullptr),
type(nullptr) {

}

Node* Symbol::getNode() const {
    return node;
}

void Symbol::setNode(Node* node) {
    this->node = node;
}

StringRef Symbol::getName() const {
    return name;
}

Declaration* Symbol::getDeclaration() const {
    return declaration;
}

void Symbol::setDeclaration(Declaration* declaration) {
    this->declaration = declaration;
}

Type* Symbol::getType() const {
    return type;
}

void Symbol::setType(Type* type) {
    this->type = type;
}

void* Symbol::operator new (size_t size, SymbolTable* symbolTable) {
    return symbolTable->allocator.Allocate(size, 8);
}
