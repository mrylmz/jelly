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

#include "Core/Operator.h"
#include "Core/Token.h"

#include <set>

#include <Basic/Basic.h>

typedef bool (*CharPredicate)(char character);

struct LexerState {
    const char* bufferStart;
    const char* bufferEnd;
    const char* bufferPtr;
    const char* newLinePtr;
    Token nextToken;

    LexerState(const char* buffer);
    LexerState(jelly::SourceBuffer buffer);
};

struct Lexer {
    LexerState state;
    jelly::StringMap<uint32_t> directives;
    jelly::StringMap<uint32_t> keywords;
    jelly::StringMap<Operator> prefixOperators;
    jelly::StringMap<Operator> infixOperators;
    jelly::StringMap<Operator> postfixOperators;
    std::set<Precedence, std::less<Precedence>> operatorPrecedenceSet;

    Lexer(const char* buffer);
    Lexer(jelly::SourceBuffer buffer);
    Lexer(const Lexer&) = delete;

    void init();

    Token lexToken();
    Token peekNextToken();

    bool getOperator(jelly::StringRef name, OperatorKind kind, Operator& op);
    bool getOperator(jelly::StringRef name, Operator& op, jelly::StringMap<Operator>& operators);
    bool hasOperator(jelly::StringRef text);
    Precedence getOperatorPrecedenceBefore(Precedence precedence);

    void registerOperator(Operator op);
    void registerOperator(Operator op, jelly::StringMap<Operator>& operators);
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
