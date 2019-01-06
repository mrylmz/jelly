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

struct ASTContext {
    ASTContext();
    ~ASTContext();

    Lexeme getLexeme(llvm::StringRef text);

    ASTModule* getModule();

    llvm::StringMap<Type*>* getTypes();
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
    Type* getStructType(llvm::StringRef name, llvm::StringMap<Type*> memberTypes, llvm::StringMap<unsigned> memberIndexes);
    Type* findTypeByName(llvm::StringRef name);

private:
    friend struct ASTNode;
    friend struct ASTCompoundStmt;

    llvm::BumpPtrAllocator nodeAllocator;
    llvm::BumpPtrAllocator lexemeAllocator;

    ASTModule* module;

    llvm::SmallVector<ASTNode*, 0> nodes;
    llvm::StringMap<int64_t> lexemeMap;
    llvm::SmallVector<llvm::StringRef, 0> lexemeValues;

    llvm::StringMap<Type*> types;
    llvm::SmallVector<FuncType*, 0> builtinFuncTypes;

    ErrorType typeError;
    AnyType typeAny;
    VoidType typeVoid;
    BoolType typeBool;
    IntegerType typeUInt8;
    IntegerType typeUInt16;
    IntegerType typeUInt32;
    IntegerType typeUInt64;
    IntegerType typeInt8;
    IntegerType typeInt16;
    IntegerType typeInt32;
    IntegerType typeInt64;
    FloatType typeFloat16;
    FloatType typeFloat32;
    FloatType typeFloat64;
    FloatType typeFloat80;
    FloatType typeFloat128;
    StringType typeString;
    PointerType typeAnyPointer;
};
