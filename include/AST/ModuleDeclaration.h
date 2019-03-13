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

#include "AST/NamedDeclaration.h"
#include <Basic/Basic.h>

namespace jelly {
namespace AST {

    class EnumerationDeclaration;
    class FunctionDeclaration;
    class StructureDeclaration;
    class LoadDeclaration;
    class ValueDeclaration;

    class ModuleDeclaration final: public NamedDeclaration {
        Array<EnumerationDeclaration*> enumerations;
        Array<FunctionDeclaration*> functions;
        Array<StructureDeclaration*> structures;
        Array<LoadDeclaration*> loads;
        Array<ValueDeclaration*> values;

    public:

        ModuleDeclaration(Identifier name,
               ArrayRef<EnumerationDeclaration*> enumerations,
               ArrayRef<FunctionDeclaration*> functions,
               ArrayRef<StructureDeclaration*> structures,
               ArrayRef<LoadDeclaration*> loads,
               ArrayRef<ValueDeclaration*> values);

        void addDeclaration(Declaration* declaration);

        Declaration* lookupDeclaration(StringRef name) const;

        ArrayRef<EnumerationDeclaration*> getEnumerationDeclarations() const;
        ArrayRef<FunctionDeclaration*> getFunctionDeclarations() const;
        ArrayRef<StructureDeclaration*> getStructureDeclarations() const;
        ArrayRef<LoadDeclaration*> getLoadDeclarations() const;
        ArrayRef<ValueDeclaration*> getValueDeclarations() const;

        void accept(Visitor &visitor);
    };
}
}
