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

#pragma once

#include "AST/Identifier.h"
#include "AST/Operator.h"
#include "AST/Symbol.h"

#include <Basic/Basic.h>
#include <set>
#include <map>

namespace jelly {
namespace AST {

    class Node;
    class ModuleDeclaration;
    class Scope;
    class Symbol;
    class Type;

    class Context {
        friend class Node;
        friend class Scope;
        friend class Type;

        BumpPtrAllocator allocator;
        StringMap<Identifier> identifiers;
        StringMap<Operator> operators[3];
        std::set<Precedence, std::less<Precedence>> operatorPrecedenceSet;

        ModuleDeclaration* module;

        Symbol* errorType;
        Symbol* voidType;
        Symbol* boolType;
        Symbol* int8Type;
        Symbol* int16Type;
        Symbol* int32Type;
        Symbol* int64Type;
        Symbol* int128Type;
        Symbol* intType;
        Symbol* uint8Type;
        Symbol* uint16Type;
        Symbol* uint32Type;
        Symbol* uint64Type;
        Symbol* uint128Type;
        Symbol* uintType;
        Symbol* float16Type;
        Symbol* float32Type;
        Symbol* float64Type;
        Symbol* float80Type;
        Symbol* float128Type;
        Symbol* floatType;

        void registerOperator(Operator op);

    public:

        Context();

        Identifier getIdentifier(StringRef text);
        ModuleDeclaration* getModule();

        bool getOperator(StringRef name, Fixity fixity, Operator& op);
        bool hasOperator(StringRef text);

        Precedence getOperatorPrecedenceBefore(Precedence precedence);

        void setType(Node* node, Type* type);
        Type* lookupType(Node* node);
        Type* lookupTypeByName(Identifier name);

        Symbol* getErrorType() const;
        Symbol* getVoidType() const;
        Symbol* getBoolType() const;
        Symbol* getInt8Type() const;
        Symbol* getInt16Type() const;
        Symbol* getInt32Type() const;
        Symbol* getInt64Type() const;
        Symbol* getInt128Type() const;
        Symbol* getIntType() const;
        Symbol* getUInt8Type() const;
        Symbol* getUInt16Type() const;
        Symbol* getUInt32Type() const;
        Symbol* getUInt64Type() const;
        Symbol* getUInt128Type() const;
        Symbol* getUIntType() const;
        Symbol* getFloat16Type() const;
        Symbol* getFloat32Type() const;
        Symbol* getFloat64Type() const;
        Symbol* getFloat80Type() const;
        Symbol* getFloat128Type() const;
        Symbol* getFloatType() const;
        Symbol* getStringType() const;
        Symbol* getEnumerationType(ArrayRef<Symbol*> elements);
        Symbol* getFunctionType(ArrayRef<Symbol*> parameters, Symbol* result);
        Symbol* getStructureType(ArrayRef<Symbol*> members);
        Symbol* getArrayType(Symbol* element, bool isFixedSize = false, uint32_t size = 0);
        Symbol* getPointerType(Symbol* pointee, uint32_t depth = 1);
    };
}
}
