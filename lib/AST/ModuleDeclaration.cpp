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

#include "AST/EnumerationDeclaration.h"
#include "AST/FunctionDeclaration.h"
#include "AST/LoadDeclaration.h"
#include "AST/ModuleDeclaration.h"
#include "AST/StructureDeclaration.h"
#include "AST/ValueDeclaration.h"
#include "AST/Visitor.h"

using namespace jelly;
using namespace jelly::AST;

ModuleDeclaration::ModuleDeclaration(Identifier name,
               ArrayRef<EnumerationDeclaration*> enumerations,
               ArrayRef<FunctionDeclaration*> functions,
               ArrayRef<StructureDeclaration*> structures,
               ArrayRef<LoadDeclaration*> loads,
               ArrayRef<ValueDeclaration*> values) :
NamedDeclaration(Kind::Module, name) {
    for (auto enumeration : enumerations) { addDeclaration(enumeration); }
    for (auto function : functions) { addDeclaration(function); }
    for (auto structure : structures) { addDeclaration(structure); }
    for (auto load : loads) { addDeclaration(load); }
    for (auto value : values) { addDeclaration(value); }
}

void ModuleDeclaration::addDeclaration(Declaration* declaration) {
    assert(declaration->getParent() == nullptr);

    if (declaration->isEnumerationDeclaration()) {
        Node::setParent(declaration, this);
        return enumerations.push_back(reinterpret_cast<EnumerationDeclaration*>(declaration));
    }

    if (declaration->isFunctionDeclaration()) {
        Node::setParent(declaration, this);
        return functions.push_back(reinterpret_cast<FunctionDeclaration*>(declaration));
    }

    if (declaration->isStructureDeclaration()) {
        Node::setParent(declaration, this);
        return structures.push_back(reinterpret_cast<StructureDeclaration*>(declaration));
    }

    if (declaration->isLoadDeclaration()) {
        Node::setParent(declaration, this);
        return loads.push_back(reinterpret_cast<LoadDeclaration*>(declaration));
    }

    if (declaration->isValueDeclaration()) {
        Node::setParent(declaration, this);
        return values.push_back(reinterpret_cast<ValueDeclaration*>(declaration));
    }

    report_fatal_error("Invalid declaration type added to Module!");
}

Declaration* ModuleDeclaration::lookupDeclaration(StringRef name) const {
    for (auto enumeration : enumerations) {
        if (enumeration->getName()->equals(name)) {
            return enumeration;
        }
    }

    for (auto function : functions) {
        if (function->getName()->equals(name)) {
            return function;
        }
    }

    for (auto structure : structures) {
        if (structure->getName()->equals(name)) {
            return structure;
        }
    }

    for (auto value : values) {
        if (value->getName()->equals(name)) {
            return value;
        }
    }

    return nullptr;
}

ArrayRef<EnumerationDeclaration*> ModuleDeclaration::getEnumerationDeclarations() const {
    return enumerations;
}

ArrayRef<FunctionDeclaration*> ModuleDeclaration::getFunctionDeclarations() const {
    return functions;
}

ArrayRef<StructureDeclaration*> ModuleDeclaration::getStructureDeclarations() const {
    return structures;
}

ArrayRef<LoadDeclaration*> ModuleDeclaration::getLoadDeclarations() const {
    return loads;
}

ArrayRef<ValueDeclaration*> ModuleDeclaration::getValueDeclarations() const {
    return values;
}

void ModuleDeclaration::accept(Visitor &visitor) {
    visitor.visitModuleDeclaration(this);

    for (auto declaration : getEnumerationDeclarations()) {
        declaration->accept(visitor);
    }

    for (auto declaration : getFunctionDeclarations()) {
        declaration->accept(visitor);
    }

    for (auto declaration : getStructureDeclarations()) {
        declaration->accept(visitor);
    }

    for (auto declaration : getLoadDeclarations()) {
        declaration->accept(visitor);
    }

    for (auto declaration : getValueDeclarations()) {
        declaration->accept(visitor);
    }
}
