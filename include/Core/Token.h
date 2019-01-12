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

#include <llvm/ADT/StringRef.h>

// @Refactor replace reserved ASCII tokens with named enum cases to reduce the size of a Token
enum {
    // 0 - 256 reserved for character tokens
    TOKEN_UNKNOWN = 257,
    TOKEN_EOF,
    TOKEN_IDENTIFIER,
    TOKEN_OPERATOR,
    TOKEN_ARROW,
    TOKEN_KEYWORD_LOAD,
    TOKEN_KEYWORD_FUNC,
    TOKEN_KEYWORD_ENUM,
    TOKEN_KEYWORD_STRUCT,
    TOKEN_KEYWORD_VAR,
    TOKEN_KEYWORD_LET,
    TOKEN_KEYWORD_BREAK,
    TOKEN_KEYWORD_CASE,
    TOKEN_KEYWORD_CONTINUE,
    TOKEN_KEYWORD_DEFER,
    TOKEN_KEYWORD_DO,
    TOKEN_KEYWORD_ELSE,
    TOKEN_KEYWORD_FALLTHROUGH,
    TOKEN_KEYWORD_FOR,
    TOKEN_KEYWORD_GUARD,
    TOKEN_KEYWORD_IF,
    TOKEN_KEYWORD_IN,
    TOKEN_KEYWORD_RETURN,
    TOKEN_KEYWORD_SWITCH,
    TOKEN_KEYWORD_WHILE,
    TOKEN_KEYWORD_ANY,
    TOKEN_KEYWORD_FALSE,
    TOKEN_KEYWORD_NIL,
    TOKEN_KEYWORD_TRUE,
    TOKEN_KEYWORD_TYPEOF,
    TOKEN_LITERAL_INT,
    TOKEN_LITERAL_FLOAT,
    TOKEN_LITERAL_STRING
};

struct Token {
    unsigned kind;
    unsigned line;
    unsigned column;
    llvm::StringRef text;

    Token() : kind(TOKEN_UNKNOWN) {}

    bool is(unsigned kind) const {
        return this->kind == kind;
    }

    template <typename ...T>
    bool is(unsigned kind1, unsigned kind2, T... kinds) const {
        if (is(kind1)) { return true; }
        return is(kind2, kinds...);
    }
};
