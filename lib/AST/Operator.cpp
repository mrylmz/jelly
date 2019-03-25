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

#include "AST/Operator.h"

using namespace jelly;
using namespace jelly::AST;

Operator Operator::CreatePrefix(StringRef symbol) {
    return Operator(symbol, Associativity::None, Fixity::Prefix, 250, false);
}

Operator Operator::CreateInfix(StringRef symbol,
                               Associativity associativity,
                               Precedence precedence,
                               bool assignment) {
    return Operator(symbol, associativity, Fixity::Infix, precedence, assignment);
}

Operator Operator::CreatePostfix(StringRef symbol,
                                 Associativity associativity,
                                 Precedence precedence) {
    return Operator(symbol, associativity, Fixity::Postfix, precedence, false);
}


Operator::Operator(StringRef symbol,
                   Associativity associativity,
                   Fixity fixity,
                   Precedence precedence,
                   bool assignment) :
symbol(symbol),
associativity(associativity),
fixity(fixity),
precedence(precedence),
assignment(assignment) {
}

StringRef Operator::getSymbol() const {
    return symbol;
}

Associativity Operator::getAssociativity() const {
    return associativity;
}

Fixity Operator::getFixity() const {
    return fixity;
}

Precedence Operator::getPrecedence() const {
    return precedence;
}

bool Operator::isAssignment() const {
    return assignment;
}

// MARK: - Prefix Operators
const Operator Operator::LogicalNot = Operator::CreatePrefix("!");
const Operator Operator::BitwiseNot = Operator::CreatePrefix("~");
const Operator Operator::UnaryPlus  = Operator::CreatePrefix("+");
const Operator Operator::UnaryMinus = Operator::CreatePrefix("-");

// MARK: - Infix Operators
const Operator Operator::BitwiseLeftShift        = Operator::CreateInfix("<<", Associativity::None, 900);
const Operator Operator::BitwiseRightShift       = Operator::CreateInfix(">>", Associativity::None, 900);
const Operator Operator::Multiply                = Operator::CreateInfix("*",  Associativity::Left, 800);
const Operator Operator::Divide                  = Operator::CreateInfix("/",  Associativity::Left, 800);
const Operator Operator::Reminder                = Operator::CreateInfix("%",  Associativity::Left, 800);
const Operator Operator::BitwiseAnd              = Operator::CreateInfix("&",  Associativity::Left, 800);
const Operator Operator::Add                     = Operator::CreateInfix("+",  Associativity::Left, 700);
const Operator Operator::Subtract                = Operator::CreateInfix("-",  Associativity::Left, 700);
const Operator Operator::BitwiseOr               = Operator::CreateInfix("|",  Associativity::Left, 700);
const Operator Operator::BitwiseXor              = Operator::CreateInfix("^",  Associativity::Left, 700);
//INFIX_OPERATOR("..<",  ASSOCIATIVITY_NONE,  600,  false, false)
//INFIX_OPERATOR("...",  ASSOCIATIVITY_NONE,  600,  false, false)
const Operator Operator::TypeCheck               = Operator::CreateInfix("is", Associativity::Left, 500);
const Operator Operator::TypeCast                = Operator::CreateInfix("as", Associativity::Left, 500);
const Operator Operator::LessThan                = Operator::CreateInfix("<",  Associativity::None, 400);
const Operator Operator::LessThanEqual           = Operator::CreateInfix("<=", Associativity::None, 400);
const Operator Operator::GreaterThan             = Operator::CreateInfix(">",  Associativity::None, 400);
const Operator Operator::GreaterThanEqual        = Operator::CreateInfix(">=", Associativity::None, 400);
const Operator Operator::Equal                   = Operator::CreateInfix("==", Associativity::None, 400);
const Operator Operator::NotEqual                = Operator::CreateInfix("!=", Associativity::None, 400);
const Operator Operator::LogicalAnd              = Operator::CreateInfix("&&", Associativity::Left, 300);
const Operator Operator::LogicalOr               = Operator::CreateInfix("||", Associativity::Left, 200);
const Operator Operator::Assign                  = Operator::CreateInfix("=",  Associativity::Right, 100, true);
const Operator Operator::MultiplyAssign          = Operator::CreateInfix("*=", Associativity::Right, 100, true);
const Operator Operator::DivideAssign            = Operator::CreateInfix("/=", Associativity::Right, 100, true);
const Operator Operator::ReminderAssign          = Operator::CreateInfix("%=", Associativity::Right, 100, true);;
const Operator Operator::AddAssign               = Operator::CreateInfix("+=", Associativity::Right, 100, true);;
const Operator Operator::SubtractAssign          = Operator::CreateInfix("-=", Associativity::Right, 100, true);;
const Operator Operator::BitwiseLeftShiftAssign  = Operator::CreateInfix("<<=", Associativity::Right, 100, true);;
const Operator Operator::BitwiseRightShiftAssign = Operator::CreateInfix(">>=", Associativity::Right, 100, true);;
const Operator Operator::BitwiseAndAssign        = Operator::CreateInfix("&=", Associativity::Right, 100, true);;
const Operator Operator::BitwiseOrAssign         = Operator::CreateInfix("|=", Associativity::Right, 100, true);;
const Operator Operator::BitwiseXorAssign        = Operator::CreateInfix("^=", Associativity::Right, 100, true);;

// MARK: - Postfix Operators
const Operator Operator::Selector    = Operator::CreatePostfix(".",  Associativity::Left, 1000);
const Operator Operator::TypePointer = Operator::CreatePostfix("*",  Associativity::Left, 1000);

// @Todo Store full symbol in Operator but scan for opening part in Lexer!
const Operator Operator::Call        = Operator::CreatePostfix("(" /* ) */, Associativity::Left, 1000);
const Operator Operator::Subscript   = Operator::CreatePostfix("[" /* ] */, Associativity::Left, 1000);

bool Operator::operator == (Operator other) const {
    return (getSymbol() == other.getSymbol() &&
            getAssociativity() == other.getAssociativity() &&
            getFixity() == other.getFixity() &&
            getPrecedence() == other.getPrecedence() &&
            isAssignment() == other.isAssignment());
}

bool Operator::operator != (Operator other) const {
    return !(*this == other);
}
