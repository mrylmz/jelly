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

enum ASTNodeKind : uint8_t {
    AST_UNKNOWN,
    AST_LOAD,
    AST_LITERAL,
    AST_FUNC,
    AST_FUNC_SIGNATURE,
    AST_BLOCK,
    AST_PARAMETER,
    AST_STRUCT,
    AST_VARIABLE,
    AST_ENUM,
    AST_ENUM_ELEMENT,
    AST_IDENTIFIER,
    AST_UNARY,
    AST_BINARY,
    AST_CONTROL,
    AST_TYPE_REF,
    AST_DEFER,
    AST_FOR,
    AST_GUARD,
    AST_IF,
    AST_SWITCH,
    AST_SWITCH_CASE,
    AST_LOOP,
    AST_CALL,
    AST_SUBSCRIPT
};

enum ASTTypeRefKind : uint8_t {
    AST_TYPE_REF_UNKNOWN,
    AST_TYPE_REF_IDENTIFIER,
    AST_TYPE_REF_ANY,
    AST_TYPE_REF_TYPEOF,
    AST_TYPE_REF_POINTER,
    AST_TYPE_REF_ARRAY
};

enum ASTIfKind : uint8_t {
    AST_IF_SINGLE,
    AST_IF_ELSE,
    AST_IF_ELSE_IF
};

enum ASTSwitchCaseKind : uint8_t {
    AST_SWITCH_CASE_CONDITION,
    AST_SWITCH_CASE_ELSE
};

enum ASTControlKind : uint8_t {
    AST_CONTROL_UNKNOWN,
    AST_CONTROL_BREAK,
    AST_CONTROL_CONTINUE,
    AST_CONTROL_FALLTHROUGH,
    AST_CONTROL_RETURN
};

enum ASTTypeKind : uint8_t {
    AST_TYPE_UNRESOLVED,
    AST_TYPE_ERROR,
    AST_TYPE_ANY,
//    AST_TYPE_INTEGER,
//    AST_TYPE_FLOAT,
    AST_TYPE_ARRAY,
    AST_TYPE_POINTER,
    AST_TYPE_ENUM,
    AST_TYPE_ENUM_ELEMENT,
    AST_TYPE_STRUCT,
    AST_TYPE_FUNCTION
};
