//
// MIT License
//
// Copyright (c) 2019 Murat Yilmaz
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

#include "AST/Associativity.h"
#include "AST/Fixity.h"
#include <Basic/Basic.h>

namespace jelly {
namespace AST {

    typedef unsigned Precedence;

    class Operator final {
        StringRef symbol;
        Associativity associativity;
        Fixity fixity;
        Precedence precedence;
        bool assignment;

        Operator(StringRef symbol,
                 Associativity associativity,
                 Fixity fixity,
                 Precedence precedence,
                 bool assignment);

        static Operator CreatePrefix(StringRef symbol);

        static Operator CreateInfix(StringRef symbol,
                                    Associativity associativity,
                                    Precedence precedence,
                                    bool assignment = false);

        static Operator CreatePostfix(StringRef symbol,
                                      Associativity associativity,
                                      Precedence precedence);

    public:

        StringRef getSymbol() const;
        Associativity getAssociativity() const;
        Fixity getFixity() const;
        Precedence getPrecedence() const;
        bool isAssignment() const;

        // MARK: - Prefix Operators
        static const Operator LogicalNot;
        static const Operator BitwiseNot;
        static const Operator UnaryPlus;
        static const Operator UnaryMinus;

        // MARK: - Infix Operators
        static const Operator BitwiseLeftShift;
        static const Operator BitwiseRightShift;
        static const Operator Multiply;
        static const Operator Divide;
        static const Operator Reminder;
        static const Operator BitwiseAnd;
        static const Operator Add;
        static const Operator Subtract;
        static const Operator BitwiseOr;
        static const Operator BitwiseXor;
        static const Operator TypeCheck;
        static const Operator TypeCast;
        static const Operator LessThan;
        static const Operator LessThanEqual;
        static const Operator GreaterThan;
        static const Operator GreaterThanEqual;
        static const Operator Equal;
        static const Operator NotEqual;
        static const Operator LogicalAnd;
        static const Operator LogicalOr;
        static const Operator Assign;
        static const Operator MultiplyAssign;
        static const Operator DivideAssign;
        static const Operator ReminderAssign;
        static const Operator AddAssign;
        static const Operator SubtractAssign;
        static const Operator BitwiseLeftShiftAssign;
        static const Operator BitwiseRightShiftAssign;
        static const Operator BitwiseAndAssign;
        static const Operator BitwiseOrAssign;
        static const Operator BitwiseXorAssign;

        // MARK: - Postfix Operators
        static const Operator Selector;
        static const Operator TypePointer;
        static const Operator Call;
        static const Operator Subscript;

        bool operator == (Operator other) const;
    };
}
}
