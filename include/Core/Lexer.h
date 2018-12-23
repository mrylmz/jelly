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

#include "Core/Token.h"
#include "Core/Operator.h"

#include <set>

#include <llvm/ADT/SmallVector.h>
#include <llvm/ADT/StringMap.h>

typedef bool (*CharPredicate)(char character);

struct LexerState {
    const char* bufferStart;
    const char* bufferEnd;
    const char* bufferPtr;
    const char* newLinePtr;
    Token nextToken;

    LexerState(const char* buffer);
};

struct Lexer {
    LexerState state;
    llvm::StringMap<uint32_t> directives;
    llvm::StringMap<uint32_t> keywords;
    llvm::StringMap<Operator> prefixOperators;
    llvm::StringMap<Operator> infixOperators;
    llvm::StringMap<Operator> postfixOperators;
    std::set<Precedence, std::less<Precedence>> operatorPrecedenceSet;

    Lexer(const char* buffer);
    Lexer(const Lexer&) = delete;

    Token lexToken();
    Token peekNextToken();

    bool getOperator(Token token, OperatorKind kind, Operator& op);
    bool getOperator(Token token, Operator& op, llvm::StringMap<Operator>& operators);
    bool hasOperator(llvm::StringRef text);
    Precedence getOperatorPrecedenceBefore(Precedence precedence);

    void registerOperator(Operator op);
    void registerOperator(Operator op, llvm::StringMap<Operator>& operators);
    void registerOperatorPrecedence(Precedence precedence);

    void formToken(unsigned kind, const char* tokenStart);

    bool advanceIf(CharPredicate predicate);
    bool advanceWhile(CharPredicate predicate);

    void skipToEndOfLine();
    bool skipMultilineCommentTail();
    void skipWhitespaceAndNewlines();

    void lexTokenImpl();
    void lexDirective();
    void lexIdentifierOrKeyword();
    void lexOperator();
    void lexNumericLiteral();
    void lexHexLiteral();
    void lexStringLiteral();

    void operator = (const Lexer&) = delete;
};
