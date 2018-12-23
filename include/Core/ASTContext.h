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

#include "Core/AST.h"
#include "Core/Type.h"
#include "Core/Macros.h"

#include <stddef.h>

#include <llvm/ADT/StringRef.h>
#include <llvm/ADT/StringMap.h>
#include <llvm/Support/Allocator.h>

// @Incomplete ASTContext is leaking memory, add custom allocators for use-cases of ASTContext and refine the structure
struct ASTContext {
    llvm::MallocAllocator allocator;

    size_t pageSize;
    size_t nodeSize;
    size_t nodeCount;
    size_t nodesPerPage;
    llvm::SmallVector<void*, 0> nodePages;

    llvm::StringMap<int64_t> lexemeMap;
    llvm::SmallVector<llvm::StringRef, 0> lexemeValues;

    llvm::StringMap<Type*> types;
    llvm::SmallVector<FuncType*, 0> builtinFuncTypes;

    ASTBlock* root;

    Type typeError;
    Type typeAny;
    Type typeVoid;
    Type typeBool;
    Type typeUInt8;
    Type typeUInt16;
    Type typeUInt32;
    Type typeUInt64;
    Type typeInt8;
    Type typeInt16;
    Type typeInt32;
    Type typeInt64;
    Type typeFloat16;
    Type typeFloat32;
    Type typeFloat64;
    Type typeFloat80;
    Type typeFloat128;
    Type typeString;
    Type typeAnyPointer;

    ASTContext();
    ~ASTContext();

    void* allocNode();

    Lexeme getLexeme(llvm::StringRef text);

    ASTBlock* getRoot();
    llvm::SmallVector<FuncType*, 0>* getBuiltinFuncTypes();

    Type* getErrorType();
    Type* getAnyType();
    Type* getVoidType();
    Type* getBoolType();
    Type* getUInt8Type();
    Type* getUInt16Type();
    Type* getUInt32Type();
    Type* getUInt64Type();
    Type* getUIntType();
    Type* getInt8Type();
    Type* getInt16Type();
    Type* getInt32Type();
    Type* getInt64Type();
    Type* getIntType();
    Type* getFloat16Type();
    Type* getFloat32Type();
    Type* getFloat64Type();
    Type* getFloat80Type();
    Type* getFloat128Type();
    Type* getFloatType();
    Type* getStringType();
    Type* getAnyPointerType();
    Type* getEnumType(ASTEnumDecl* decl);
    Type* getPointerType(Type* pointeeType, uint64_t depth);
    Type* getStaticArrayType(Type* elementType, llvm::APInt size);
    Type* getDynamicArrayType(Type* elementType);
    Type* getFuncType(ASTFuncDecl* decl);
    Type* getStructType(llvm::StringRef name, llvm::StringMap<Type*> memberTypes);
    Type* findTypeByName(llvm::StringRef name);
};
