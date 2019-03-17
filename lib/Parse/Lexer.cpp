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

#include "Parse/Lexer.h"

using namespace jelly;
using namespace jelly::AST;
using namespace jelly::Parse;

// @Incomplete Add UTF-8 support
// @Incomplete Lex leading and trailing trivia(s)

static inline bool charIsAlphaNumeric(char character) {
    switch (character) {
        case 'a'...'z':
        case 'A'...'Z': 
        case '0'...'9': return true;
        default:        return false;
    }
}

static inline bool charIsStartOfIdentifier(char character) {
    switch (character) {
        case 'a'...'z':
        case 'A'...'Z':
        case '_':       return true;
        default:        return false;
    }
}

static inline bool charIsContinuationOfIdentifier(char character) {
    switch (character) {
        case 'a'...'z':
        case 'A'...'Z':
        case '_':
        case '0'...'9': return true;
        default:        return false;
    }
}

static inline bool charIsOperator(char character) {
    switch (character) {
        case '/':
        case '=':
        case '-':
        case '+':
        case '!':
        case '*':
        case '%':
        case '.':
        case '<':
        case '>':
        case '&':
        case '|':
        case '^':
        case '~':
        case '?': return true;
        default:  return false;
    }
}

static inline bool charIsBinaryDigit(char character) {
    switch (character) {
        case '0'...'1': return true;
        default:        return false;
    }
}

static inline bool charIsOctalDigit(char character) {
    switch (character) {
        case '0'...'7': return true;
        default:        return false;
    }
}

static inline bool charIsDecimalDigit(char character) {
    switch (character) {
        case '0'...'9': return true;
        default:        return false;
    }
}

static inline bool charIsHexadecimalDigit(char character) {
    switch (character) {
        case '0'...'9':
        case 'a'...'f':
        case 'A'...'F': return true;
        default:        return false;
    }
}

Lexer::State::State(SourceBuffer buffer) :
buffer(buffer),
bufferPointer(buffer.getBufferStart()),
newlinePointer(buffer.getBufferStart()) {

}

Lexer::Lexer(jelly::AST::Context* context, jelly::SourceBuffer buffer) :
context(context),
state(buffer) {
    directives.try_emplace("#load", Token::Kind::KeywordLoad);

    keywords.try_emplace("func", Token::Kind::KeywordFunc);
    keywords.try_emplace("enum", Token::Kind::KeywordEnum);
    keywords.try_emplace("struct", Token::Kind::KeywordStruct);
    keywords.try_emplace("var", Token::Kind::KeywordVar);
    keywords.try_emplace("let", Token::Kind::KeywordLet);

    keywords.try_emplace("break", Token::Kind::KeywordBreak);
    keywords.try_emplace("case", Token::Kind::KeywordCase);
    keywords.try_emplace("continue", Token::Kind::KeywordContinue);
    keywords.try_emplace("defer", Token::Kind::KeywordDefer);
    keywords.try_emplace("do", Token::Kind::KeywordDo);
    keywords.try_emplace("else", Token::Kind::KeywordElse);
    keywords.try_emplace("fallthrough", Token::Kind::KeywordFallthrough);
    keywords.try_emplace("guard", Token::Kind::KeywordGuard);
    keywords.try_emplace("if", Token::Kind::KeywordIf);
    keywords.try_emplace("return", Token::Kind::KeywordReturn);
    keywords.try_emplace("switch", Token::Kind::KeywordSwitch);
    keywords.try_emplace("while", Token::Kind::KeywordWhile);

    keywords.try_emplace("as", Token::Kind::Operator);
    keywords.try_emplace("false", Token::Kind::KeywordFalse);
    keywords.try_emplace("is", Token::Kind::Operator);
    keywords.try_emplace("nil", Token::Kind::KeywordNil);
    keywords.try_emplace("true", Token::Kind::KeywordTrue);
    keywords.try_emplace("typeof", Token::Kind::KeywordTypeof);

    state.token.setColumn(0);
    state.token.setLine(1);

    lexTokenImpl();
}

void Lexer::formToken(Token::Kind kind, const char* tokenStart) {
    state.token.setKind(kind);
    state.token.setText(StringRef(tokenStart, state.bufferPointer - tokenStart));
    state.token.setColumn(tokenStart - state.newlinePointer);
}

bool Lexer::advanceIf(CharPredicate predicate) {
    if (predicate(*state.bufferPointer)) {
        assert(state.bufferPointer != state.buffer.getBufferEnd() && "Predicate is going out of buffer range!");
        state.bufferPointer += 1;
        return true;
    }

    return false;
}

bool Lexer::advanceWhile(CharPredicate predicate) {
    bool result = advanceIf(predicate);

    while (predicate(*state.bufferPointer)) {
        assert(state.bufferPointer != state.buffer.getBufferEnd() && "Predicate is going out of buffer range!");
        state.bufferPointer += 1;
    }

    return result;
}

void Lexer::skipToEndOfLine() {
    while (state.bufferPointer != state.buffer.getBufferEnd() && *state.bufferPointer != '\n' && *state.bufferPointer != '\r') {
        state.bufferPointer += 1;
    }
}

bool Lexer::skipMultilineCommentTail() {
    assert((*(state.bufferPointer - 2) == '/' && *(state.bufferPointer - 1) == '*') && "Invalid start of multiline comment!");
    state.bufferPointer -= 1;

    do {
        state.bufferPointer += 1;

        if (state.bufferPointer[0] == '*' && state.bufferPointer[1] == '/') {
            state.bufferPointer += 2;
            return true;
        }

        if (state.bufferPointer[0] == '/' && state.bufferPointer[1] == '*') {
            state.bufferPointer += 2;

            if (!skipMultilineCommentTail()) {
                return false;
            }
        }

    } while (state.bufferPointer != state.buffer.getBufferEnd());

    return false;
}

void Lexer::skipWhitespaceAndNewlines() {
    while (state.bufferPointer != state.buffer.getBufferEnd()) {
        switch (*state.bufferPointer) {
            case 0x09:
            case 0x20:
                state.bufferPointer += 1;
                break;

            case 0x0A:
            case 0x0B:
            case 0x0C:
            case 0x0D:
                state.bufferPointer += 1;
                state.token.setLine(state.token.getLine() + 1);
                break;

            default:
                return;
        }
    }
}

void Lexer::lexTokenImpl() {
    if (state.bufferPointer == state.buffer.getBufferEnd()) {
        return formToken(Token::Kind::EndOfFile, state.bufferPointer);
    }

    skipWhitespaceAndNewlines();

    const char* tokenStart = state.bufferPointer;

    switch (*state.bufferPointer) {
        case '0'...'9':
            state.bufferPointer += 1;
            return lexNumericLiteral();

        case '#':
            state.bufferPointer += 1;
            return lexDirective();

        case '"':
            state.bufferPointer += 1;
            return lexStringLiteral();

        case 'a'...'z':
        case 'A'...'Z':
        case '_':
            state.bufferPointer += 1;
            return lexIdentifierOrKeyword();

        case '/':
        case '=':
        case '-':
        case '+':
        case '!':
        case '*':
        case '%':
        case '.':
        case '<':
        case '>':
        case '&':
        case '|':
        case '^':
        case '~':
        case '?':
            state.bufferPointer += 1;
            return lexOperator();

        case '(':
            state.bufferPointer += 1;
            return formToken(Token::Kind::LeftParenthesis, tokenStart);

        case ')':
            state.bufferPointer += 1;
            return formToken(Token::Kind::RightParenthesis, tokenStart);

        case ':':
            state.bufferPointer += 1;
            return formToken(Token::Kind::Colon, tokenStart);

        case '[':
            state.bufferPointer += 1;
            return formToken(Token::Kind::LeftBracket, tokenStart);

        case ']':
            state.bufferPointer += 1;
            return formToken(Token::Kind::RightBracket, tokenStart);

        case '{':
            state.bufferPointer += 1;
            return formToken(Token::Kind::LeftBrace, tokenStart);

        case '}':
            state.bufferPointer += 1;
            return formToken(Token::Kind::RightBrace, tokenStart);

        case ',':
            state.bufferPointer += 1;
            return formToken(Token::Kind::Comma, tokenStart);

        case '\0':
            if (state.bufferPointer != state.buffer.getBufferEnd()) {
                state.bufferPointer += 1;
            }

            return formToken(Token::Kind::EndOfFile, tokenStart);

        default:
            if (state.bufferPointer != state.buffer.getBufferEnd()) {
                state.bufferPointer += 1;
            }

            return formToken(Token::Kind::Unknown, tokenStart);
    }
}

void Lexer::lexDirective() {
    assert(*(state.bufferPointer - 1) == '#' && "Invalid start of directive!");

    const char* tokenStart = state.bufferPointer - 1;

    // Skip hash bang
    if (*state.bufferPointer == '!' && tokenStart == state.buffer.getBufferStart()) {
        skipToEndOfLine();
        return lexTokenImpl();
    }

    if (!advanceWhile(charIsContinuationOfIdentifier)) {
        return formToken(Token::Kind::Unknown, tokenStart);
    }

    jelly::StringRef text(tokenStart, state.bufferPointer - tokenStart);

    auto it = directives.find(text);
    if (it == directives.end()) {
        return formToken(Token::Kind::Unknown, tokenStart);
    }

    return formToken(it->getValue(), tokenStart);
}

void Lexer::lexIdentifierOrKeyword() {
    assert(charIsStartOfIdentifier(*(state.bufferPointer - 1)) && "Invalid start of identifier or keyword!");

    const char* tokenStart = state.bufferPointer - 1;

    advanceWhile(&charIsContinuationOfIdentifier);

    jelly::StringRef text(tokenStart, state.bufferPointer - tokenStart);

    auto it = keywords.find(text);
    if (it == keywords.end()) {
        return formToken(Token::Kind::Identifier, tokenStart);
    }

    return formToken(it->getValue(), tokenStart);
}

void Lexer::lexOperator() {
    assert(charIsOperator(*(state.bufferPointer - 1)) && "Invalid start of operator!");

    const char* tokenStart = state.bufferPointer - 1;

    if (*tokenStart == '-' && *state.bufferPointer == '>') {
        state.bufferPointer += 1;
        return formToken(Token::Kind::Arrow, tokenStart);
    }

    if (*tokenStart == '/' && *state.bufferPointer == '/') {
        skipToEndOfLine();
        return lexTokenImpl();
    }

    if (*tokenStart == '/' && *state.bufferPointer == '*') {
        state.bufferPointer += 1;

        if (!skipMultilineCommentTail()) {
            return formToken(Token::Kind::Unknown, tokenStart);
        }

        return lexTokenImpl();
    }

    advanceWhile(&charIsOperator);

    bool found;
    do {
        jelly::StringRef text(tokenStart, state.bufferPointer - tokenStart);
        found = context->hasOperator(text);
        state.bufferPointer -= 1;
    } while (state.bufferPointer != tokenStart && !found);

    state.bufferPointer += 1; // Previous loop will count -1 off the token end so re-add +1

    if (!found) {
        switch (*tokenStart) {
            case '(': return formToken(Token::Kind::LeftParenthesis, tokenStart);
            case ')': return formToken(Token::Kind::RightParenthesis, tokenStart);
            case ':': return formToken(Token::Kind::Colon, tokenStart);
            case '[': return formToken(Token::Kind::LeftBracket, tokenStart);
            case ']': return formToken(Token::Kind::RightBracket, tokenStart);
            case '{': return formToken(Token::Kind::LeftBrace, tokenStart);
            case '}': return formToken(Token::Kind::RightBrace, tokenStart);
            case ',': return formToken(Token::Kind::Comma, tokenStart);
            case '.': return formToken(Token::Kind::Dot, tokenStart);
            default:  return formToken(Token::Kind::Unknown, tokenStart);
        }
    }

    return formToken(Token::Kind::Operator, tokenStart);
}

void Lexer::lexNumericLiteral() {
    assert(charIsDecimalDigit(*(state.bufferPointer - 1)) && "Expected start of numeric literal to be a decimal digit!");

    const char* tokenStart = state.bufferPointer - 1;

    auto formErrorToken = [&]() {
        // Consume all possible character continuations of the literal
        // except '.' following an operator, which could form an operator together.

        if (*state.bufferPointer == '.' && !charIsOperator(state.bufferPointer[1])) {
            state.bufferPointer += 1;
        }

        while (advanceIf(charIsAlphaNumeric)) {
            if (*state.bufferPointer == '.') {
                if (charIsOperator(state.bufferPointer[1])) {
                    break;
                } else {
                    state.bufferPointer += 1;
                }
            }
        }

        return formToken(Token::Kind::Unknown, tokenStart);
    };

    auto lexIntegerLiteral = [&](CharPredicate predicate) {
        if (!advanceWhile(predicate)) {
            return formErrorToken();
        }

        if (advanceWhile(charIsContinuationOfIdentifier)) {
            return formErrorToken();
        }

        return formToken(Token::Kind::LiteralInt, tokenStart);
    };

    if (*tokenStart == '0' && *state.bufferPointer == 'b') {
        state.bufferPointer += 1;
        return lexIntegerLiteral(charIsBinaryDigit);
    }

    if (*tokenStart == '0' && *state.bufferPointer == 'o') {
        state.bufferPointer += 1;
        return lexIntegerLiteral(charIsOctalDigit);
    }

    if (*tokenStart == '0' && *state.bufferPointer == 'x') {
        state.bufferPointer += 1;
        return lexHexLiteral();
    }

    advanceWhile(charIsDecimalDigit);

    if (*state.bufferPointer != '.' && *state.bufferPointer != 'e' && *state.bufferPointer != 'E') {
        if (advanceWhile(charIsContinuationOfIdentifier)) {
            return formErrorToken();
        }

        return formToken(Token::Kind::LiteralInt, tokenStart);
    }

    if (*state.bufferPointer == '.') {
        if (!charIsDecimalDigit(state.bufferPointer[1])) {
            if (!charIsOperator(state.bufferPointer[1])) {
                return formErrorToken();
            }

            return formToken(Token::Kind::LiteralInt, tokenStart);
        }

        state.bufferPointer += 1;
        advanceWhile(charIsDecimalDigit);
    }

    if (*state.bufferPointer == 'e' || *state.bufferPointer == 'E') {
        state.bufferPointer += 1;

        if (*state.bufferPointer == '+' || *state.bufferPointer == '-') {
            state.bufferPointer += 1;
        }

        if (!advanceWhile(charIsDecimalDigit)) {
            return formErrorToken();
        }
    }

    if (advanceWhile(charIsContinuationOfIdentifier)) {
        return formErrorToken();
    }

    return formToken(Token::Kind::LiteralFloat, tokenStart);
}

void Lexer::lexHexLiteral() {
    assert(*(state.bufferPointer - 2) == '0' && *(state.bufferPointer - 1) == 'x' && "Expected prefix of hex literal to be '0x'!");

    const char* tokenStart = state.bufferPointer - 1;

    auto formErrorToken = [&]() {
        // Consume all possible character continuations of the literal
        // except '.' following an operator, which could form an operator together.

        if (*state.bufferPointer == '.' && !charIsOperator(state.bufferPointer[1])) {
            state.bufferPointer += 1;
        }

        while (advanceIf(charIsAlphaNumeric)) {
            if (*state.bufferPointer == '.') {
                if (charIsOperator(state.bufferPointer[1])) {
                    break;
                } else {
                    state.bufferPointer += 1;
                }
            }
        }

        return formToken(Token::Kind::Unknown, tokenStart);
    };

    if (!advanceWhile(charIsHexadecimalDigit)) {
        return formErrorToken();
    }

    if (*state.bufferPointer != '.' && *state.bufferPointer != 'p' && *state.bufferPointer != 'P') {
        if (advanceWhile(charIsContinuationOfIdentifier)) {
            return formErrorToken();
        }

        return formToken(Token::Kind::LiteralInt, tokenStart);
    }

    if (*state.bufferPointer == '.') {
        if (!charIsHexadecimalDigit(state.bufferPointer[1])) {
            if (!charIsOperator(state.bufferPointer[1])) {
                return formErrorToken();
            }

            return formToken(Token::Kind::LiteralInt, tokenStart);
        }

        state.bufferPointer += 1;
        advanceWhile(charIsHexadecimalDigit);
    }

    if (*state.bufferPointer == 'p' || *state.bufferPointer == 'P') {
        state.bufferPointer += 1;

        if (*state.bufferPointer == '+' || *state.bufferPointer == '-') {
            state.bufferPointer += 1;
        }

        if (!advanceWhile(charIsHexadecimalDigit)) {
            return formErrorToken();
        }
    }

    if (advanceWhile(charIsContinuationOfIdentifier)) {
        return formErrorToken();
    }

    return formToken(Token::Kind::LiteralFloat, tokenStart);
}

void Lexer::lexStringLiteral() {
    assert(*(state.bufferPointer - 1) == '"' && "Invalid start of string literal!");

    const char* tokenStart = state.bufferPointer - 1;

    bool isValid = true;
    do {

        if (state.bufferPointer == state.buffer.getBufferEnd()) {
            return formToken(Token::Kind::Unknown, tokenStart);
        }

        switch (*state.bufferPointer) {
            case '\n':
            case '\r':
            case '\0':
                return formToken(Token::Kind::Unknown, tokenStart);

            case '"':
                state.bufferPointer += 1;

                if (!isValid) {
                    return formToken(Token::Kind::Unknown, tokenStart);
                }

                return formToken(Token::Kind::LiteralString, tokenStart);

            case '\\':
                state.bufferPointer += 1;

                switch (*state.bufferPointer) {
                    case '0':
                    case '\\':
                    case 'r':
                    case 'n':
                    case 't':
                    case '"':
                    case '\'':
                        state.bufferPointer += 1;
                        break;

                    default:
                        isValid = false;
                        break;
                }

                break;

            default:
                state.bufferPointer += 1;
                break;
        }

    } while (true);
}

Context* Lexer::getContext() const {
    return context;
}

Lexer::State Lexer::getState() const {
    return state;
}

void Lexer::setState(Lexer::State state) {
    this->state = state;
}

Token Lexer::lexToken() {
    Token token = state.token;
    if (!state.token.is(Token::Kind::EndOfFile)) {
        lexTokenImpl();
    }

    return token;
}

Token Lexer::peekNextToken() {
    return state.token;
}
