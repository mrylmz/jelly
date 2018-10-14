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
    AST_TYPE,
    AST_DEFER,
    AST_DO,
    AST_FOR,
    AST_GUARD,
    AST_IF,
    AST_SWITCH,
    AST_SWITCH_CASE,
    AST_WHILE,
    AST_CALL,
    AST_SUBSCRIPT,
};

enum ASTTypeKind : uint8_t {
    AST_TYPE_UNKNOWN,
    AST_TYPE_IDENTIFIER,
    AST_TYPE_ANY,
    AST_TYPE_TYPEOF,
    AST_TYPE_POINTER,
    AST_TYPE_ARRAY
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
