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

#include <stdint.h>
#include <Basic/Basic.h>

enum TypeKind : uint8_t {
    TYPE_ERROR,
    TYPE_BUILTIN_ANY,
    TYPE_BUILTIN_VOID,
    TYPE_BUILTIN_BOOL,
    TYPE_BUILTIN_INT,
    TYPE_BUILTIN_FLOAT,
    TYPE_BUILTIN_STRING,
    TYPE_BUILTIN_POINTER,
    TYPE_BUILTIN_ARRAY,
    TYPE_BUILTIN_OPERATION,
    TYPE_BUILTIN_FUNC,
    TYPE_BUILTIN_PREFIX_FUNC,
    TYPE_BUILTIN_INFIX_FUNC,
    TYPE_DECL_ENUM,
    TYPE_DECL_FUNC,
    TYPE_DECL_STRUCT,
};

// @Refactor reserve a fixed size of memory for all types inside the base type
//           and store all types in AST as values instead of pointers

struct Type {
    TypeKind kind = TYPE_ERROR;

    bool isIncomplete() const;

    bool isBuiltinPrefixFunc() const { return kind == TYPE_BUILTIN_PREFIX_FUNC; }
    bool isBuiltinInfixFunc() const { return kind == TYPE_BUILTIN_INFIX_FUNC; }
    bool isBuiltinOperation() const { return kind == TYPE_BUILTIN_OPERATION; }
};

struct BuiltinType : public Type {};

struct DeclType : public Type {
    jelly::StringRef name;
};

struct ErrorType : public Type {
    ErrorType() { kind = TYPE_ERROR; }
};

struct AnyType : public BuiltinType {
    AnyType() { kind = TYPE_BUILTIN_ANY; }
};

struct VoidType : public BuiltinType {
    VoidType() { kind = TYPE_BUILTIN_VOID; }
};

struct BoolType : public BuiltinType {
    BoolType() { kind = TYPE_BUILTIN_BOOL; }
};

struct IntegerType : public BuiltinType {
    bool isSigned;
    unsigned fixedWidth;

    IntegerType(bool isSigned, unsigned fixedWidth) :
    isSigned(isSigned),
    fixedWidth(fixedWidth) {
        kind = TYPE_BUILTIN_INT;
    }
};

enum FloatKind : uint8_t {
    FLOAT_IEEE16,
    FLOAT_IEEE32,
    FLOAT_IEEE64,
    FLOAT_IEEE80,
    FLOAT_IEEE128
};

struct FloatType : public BuiltinType {
    FloatKind floatKind;

    FloatType(FloatKind floatKind) :
    floatKind(floatKind) {
        kind = TYPE_BUILTIN_FLOAT;
    }

    unsigned bitWidth() const;
};

struct StringType : public BuiltinType {
    StringType() { kind = TYPE_BUILTIN_STRING; }
};

struct PointerType : public BuiltinType {
    unsigned depth;
    Type* pointeeType;

    PointerType(unsigned depth, Type* pointeeType) :
    depth(depth),
    pointeeType(pointeeType) {
        kind = TYPE_BUILTIN_POINTER;
    }
};

struct ArrayType : public Type {
    bool isStatic = false;
    jelly::APInt size = jelly::APInt(256, 0);
    Type* elementType = nullptr;

    ArrayType() { kind = TYPE_BUILTIN_ARRAY; }
};

struct ASTIntLit;
struct EnumType : public DeclType {
    jelly::SmallVector<ASTIntLit*, 0> memberValues;
    jelly::APInt nextMemberValue = jelly::APInt(64, 0);

    EnumType() { kind = TYPE_DECL_ENUM; }
};

struct StructType : public DeclType {
    jelly::StringMap<Type*> memberTypes;
    jelly::StringMap<unsigned> memberIndexes;

    StructType() { kind = TYPE_DECL_STRUCT; }
};

enum CallingConvention : uint8_t {
    CALLING_CONVENTION_DEFAULT,
    CALLING_CONVENTION_C,
    CALLING_CONVENTION_FAST,
    CALLING_CONVENTION_COLD,
    CALLING_CONVENTION_GHC,
    CALLING_CONVENTION_HIPE,
    CALLING_CONVENTION_WEBKIT_JSCC,
    CALLING_CONVENTION_ANYREG,
    CALLING_CONVENTION_PRESERVE_MOST,
    CALLING_CONVENTION_PRESERVE_ALL,
    CALLING_CONVENTION_CXX_FAT_TLS,
    CALLING_CONVENTION_SWIFT
};

struct FuncType : public DeclType {
    CallingConvention cc = CALLING_CONVENTION_DEFAULT;
    jelly::SmallVector<Type*, 0> paramTypes;
    Type* returnType = nullptr;

    FuncType() { kind = TYPE_DECL_FUNC; }
};

enum BuiltinOperation : uint8_t {
    BUILTIN_ASSIGNMENT_OPERATION
};

struct BuiltinOperationType : public Type {
    BuiltinOperation op;
    BuiltinOperationType(BuiltinOperation op) : op(op) {
        kind = TYPE_BUILTIN_OPERATION;
    }
};

// @Incomplete Add builtin types as wrapper for llvm functions
struct BuiltinFuncType : public FuncType {
    BuiltinFuncType() { kind = TYPE_BUILTIN_FUNC; }
};

struct BuiltinPrefixFuncType : public BuiltinFuncType {
    BuiltinPrefixFuncType() { kind = TYPE_BUILTIN_PREFIX_FUNC; }
};

struct BuiltinInfixFuncType : public BuiltinFuncType {
    BuiltinInfixFuncType() { kind = TYPE_BUILTIN_INFIX_FUNC; }
};