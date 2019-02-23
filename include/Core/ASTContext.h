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

#include <Basic/Basic.h>

struct ASTContext {
    ASTContext();
    ~ASTContext();

    Lexeme getLexeme(jelly::StringRef text);

    ASTModuleDecl* getModule();

    jelly::StringMap<Type*>* getTypes();
    jelly::SmallVector<FuncType*, 0>* getBuiltinFuncTypes();

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
    Type* getStaticArrayType(Type* elementType, jelly::APInt size);
    Type* getDynamicArrayType(Type* elementType);
    FuncType* getFuncType(ASTFuncDecl* decl);
    FuncType* getFuncType(jelly::StringRef name, jelly::SmallVector<Type*, 0> paramTypes, Type* returnType);
    Type* getStructType(jelly::StringRef name, jelly::StringMap<Type*> memberTypes, jelly::StringMap<unsigned> memberIndexes);
    Type* findTypeByName(jelly::StringRef name);

    jelly::BumpPtrAllocator nodeAllocator;

private:
    friend struct ASTNode;

    jelly::BumpPtrAllocator lexemeAllocator;
    jelly::BumpPtrAllocator typeAllocator;

    ASTModuleDecl* module;

    jelly::SmallVector<ASTNode*, 0> nodes;
    jelly::StringMap<int64_t> lexemeMap;
    jelly::SmallVector<jelly::StringRef, 0> lexemeValues;

    jelly::StringMap<Type*> types;
    jelly::SmallVector<FuncType*, 0> builtinFuncTypes;

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
    BuiltinOperationType typeAssignmentOp;

    BuiltinPrefixFuncType* createBuiltinPrefixFuncType(jelly::StringRef name, Type* paramType, Type* returnType);
    BuiltinInfixFuncType* createBuiltinInfixFuncType(jelly::StringRef name, Type* lhsParamType, Type* rhsParamType, Type* returnType);
};
