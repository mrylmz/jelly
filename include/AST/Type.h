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

#include <stddef.h>

namespace jelly {
namespace AST {

    class Context;

    class Type {
        friend class Context;

    protected:

        enum class Kind {
            ErrorType,
            VoidType,
            IntegerType,
            FloatType,
            EnumerationType,
            FunctionType,
            StructureType,
            ArrayType,
            PointerType
        };

    private:

        Kind kind;

        Type() = delete;
        Type(Type&&) = delete;

        Type& operator = (Type&&) = delete;
        void operator delete (void* ptr) = delete;
        void operator delete [] (void* ptr) = delete;

    protected:

        Type(Kind kind);

    public:

        bool isErrorType() const;
        bool isVoidType() const;
        bool isIntegerType() const;
        bool isFloatType() const;
        bool isEnumerationType() const;
        bool isFunctionType() const;
        bool isStructureType() const;
        bool isArrayType() const;
        bool isPointerType() const;

        void* operator new (size_t size, Context* context);
    };
}
}
