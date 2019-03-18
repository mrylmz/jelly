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
#include <Basic/Basic.h>
#include <set>
#include <map>

namespace jelly {
namespace AST {

    class Node;
    class ModuleDeclaration;
    class Type;

    class Context {
        friend class Node;
        friend class Type;

        BumpPtrAllocator allocator;
        StringMap<Identifier> identifiers;
        StringMap<Operator> operators[3];
        std::set<Precedence, std::less<Precedence>> operatorPrecedenceSet;
        std::map<Node*, Type*> typeTable;

        ModuleDeclaration* module;

        Type* errorType;
        Type* voidType;
        Type* boolType;
        Type* int8Type;
        Type* int16Type;
        Type* int32Type;
        Type* int64Type;
        Type* int128Type;
        Type* uint8Type;
        Type* uint16Type;
        Type* uint32Type;
        Type* uint64Type;
        Type* uint128Type;
        Type* float16Type;
        Type* float32Type;
        Type* float64Type;
        Type* float80Type;
        Type* float128Type;

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

        Type* getErrorType() const;
        Type* getVoidType() const;
        Type* getBoolType() const;
        Type* getInt8Type() const;
        Type* getInt16Type() const;
        Type* getInt32Type() const;
        Type* getInt64Type() const;
        Type* getInt128Type() const;
        Type* getUInt8Type() const;
        Type* getUInt16Type() const;
        Type* getUInt32Type() const;
        Type* getUInt64Type() const;
        Type* getUInt128Type() const;
        Type* getFloat16Type() const;
        Type* getFloat32Type() const;
        Type* getFloat64Type() const;
        Type* getFloat80Type() const;
        Type* getFloat128Type() const;
        Type* getEnumerationType(StringMap<int64_t> elements);
        Type* getFunctionType(Array<StringRef> parameterNames, Array<Type*> parameterTypes, Type* returnType);
        Type* getStructureType(Array<StringRef> memberNames, Array<Type*> memberTypes);
        Type* getArrayType(Type* elementType, bool isFixedSize = false, uint32_t size = 0);
        Type* getPointerType(Type* pointeeType, uint32_t depth = 1);
    };
}
}
