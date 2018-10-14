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

enum OperatorKind : uint8_t {
    OPERATOR_INVALID,
    OPERATOR_PREFIX,
    OPERATOR_INFIX,
    OPERATOR_POSTFIX
};

enum Associativity : uint8_t {
    ASSOCIATIVITY_NONE,
    ASSOCIATIVITY_LEFT,
    ASSOCIATIVITY_RIGHT
};

struct Operator {
    OperatorKind  kind;
    String        text;
    Associativity associativity;
    uint32_t      precedence;
    bool          can_have_arguments;

    Operator() : kind(OPERATOR_INVALID), text({}), associativity(ASSOCIATIVITY_NONE), precedence(250), can_have_arguments(false) {}

    Operator(OperatorKind kind, String text, Associativity associativity = ASSOCIATIVITY_NONE, uint32_t precedence = 250, bool can_have_arguments = false) :
    kind(kind), text(text), associativity(associativity), precedence(precedence), can_have_arguments(can_have_arguments) {}

    bool is_valid() const {
        return kind != OPERATOR_INVALID;
    }
};
