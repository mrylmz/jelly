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

// @Incomplete Add UTF-8 support
// @Incomplete Lex leading and trailing trivia(s)

#define BINARY_DIGIT            '0'...'1'
#define OCTAL_DIGIT             '0'...'7'
#define DECIMAL_DIGIT           '0'...'9'
#define HEX_DIGIT               DECIMAL_DIGIT: case 'a'...'f': case 'A'...'F'
#define ALPHA                   'a'...'z': case 'A'...'Z'
#define ALPHA_NUMERIC           ALPHA: case DECIMAL_DIGIT
#define DIRECTIVE_HEAD          '#'
#define STRING_HEAD             '"'
#define IDENTIFIER_HEAD         ALPHA: case '_'
#define IDENTIFIER_BODY         IDENTIFIER_HEAD: case DECIMAL_DIGIT
#define OPERATOR                '/': case '=': case '-': case '+': case '!': case '*': case '%': case '.': case \
                                '<': case '>': case '&': case '|': case '^': case '~': case '?'
#define WHITESPACE              0x09: case 0x20
#define NEWLINE                 0x0A: case 0x0B: case 0x0C: case 0x0D
#define ESCAPED                 '0': case '\\': case 'r': case 'n': case 't': case '"': case '\''

static inline bool charIsAlphaNumeric(char character) {
    switch (character) {
        case ALPHA_NUMERIC: return true;
        default:            return false;
    }
}

static inline bool charIsStartOfIdentifier(char character) {
    switch (character) {
        case IDENTIFIER_HEAD: return true;
        default:              return false;
    }
}

static inline bool charIsContinuationOfIdentifier(char character) {
    switch (character) {
        case IDENTIFIER_BODY: return true;
        default:              return false;
    }
}

static inline bool charIsOperator(char character) {
    switch (character) {
        case OPERATOR: return true;
        default:       return false;
    }
}

static inline bool charIsBinaryDigit(char character) {
    switch (character) {
        case BINARY_DIGIT: return true;
        default:           return false;
    }
}

static inline bool charIsOctalDigit(char character) {
    switch (character) {
        case OCTAL_DIGIT: return true;
        default:          return false;
    }
}

static inline bool charIsDecimalDigit(char character) {
    switch (character) {
        case DECIMAL_DIGIT: return true;
        default:            return false;
    }
}

static inline bool charIsHexadecimalDigit(char character) {
    switch (character) {
        case HEX_DIGIT: return true;
        default:        return false;
    }
}

LexerState::LexerState(const char* buffer) :
bufferStart(buffer),
bufferEnd(buffer + strlen(buffer)),
bufferPtr(buffer),
newLinePtr(buffer) {
}

LexerState::LexerState(jelly::SourceBuffer buffer) :
bufferStart(buffer.getBufferStart()),
bufferEnd(buffer.getBufferEnd()),
bufferPtr(buffer.getBufferStart()),
newLinePtr(buffer.getBufferStart()) {
}

Lexer::Lexer(const char* buffer) : state(buffer) {
    init();
}

Lexer::Lexer(jelly::SourceBuffer buffer) : state(buffer) {
    init();
}

void Lexer::init() {
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
    // TODO: [1] Reserve Builtin types as keywords !!!

    state.nextToken.setLine(1);

    lexTokenImpl();
}

LexerState Lexer::getState() const {
    return state;
}

void Lexer::setState(LexerState state) {
    this->state = state;
}

Token Lexer::lexToken() {
    Token token = state.nextToken;
    if (!state.nextToken.is(Token::Kind::EndOfFile)) {
        lexTokenImpl();
    }

    return token;
}

Token Lexer::peekNextToken() {
    return state.nextToken;
}

void Lexer::formToken(Token::Kind kind, const char* tokenStart) {
    state.nextToken.setKind(kind);
    state.nextToken.setText(StringRef(tokenStart, state.bufferPtr - tokenStart));
    state.nextToken.setColumn(tokenStart - state.newLinePtr);
}

bool Lexer::advanceIf(CharPredicate predicate) {
    if (predicate(*state.bufferPtr)) {
        assert(state.bufferPtr != state.bufferEnd && "Predicate is going out of buffer range!");
        state.bufferPtr += 1;
        return true;
    }

    return false;
}

bool Lexer::advanceWhile(CharPredicate predicate) {
    bool result = advanceIf(predicate);

    while (predicate(*state.bufferPtr)) {
        assert(state.bufferPtr != state.bufferEnd && "Predicate is going out of buffer range!");
        state.bufferPtr += 1;
    }

    return result;
}

void Lexer::skipToEndOfLine() {
    while (state.bufferPtr != state.bufferEnd && *state.bufferPtr != '\n' && *state.bufferPtr != '\r') {
        state.bufferPtr += 1;
    }
}

bool Lexer::skipMultilineCommentTail() {
    assert((*(state.bufferPtr - 2) == '/' && *(state.bufferPtr - 1) == '*') && "Invalid start of multiline comment!");
    state.bufferPtr -= 1;

    do {
        state.bufferPtr += 1;

        if (state.bufferPtr[0] == '*' && state.bufferPtr[1] == '/') {
            state.bufferPtr += 2;
            return true;
        }

        if (state.bufferPtr[0] == '/' && state.bufferPtr[1] == '*') {
            state.bufferPtr += 2;

            if (!skipMultilineCommentTail()) {
                return false;
            }
        }

    } while (state.bufferPtr != state.bufferEnd);

    return false;
}

void Lexer::skipWhitespaceAndNewlines() {
    while (state.bufferPtr != state.bufferEnd) {
        switch (*state.bufferPtr) {
            case WHITESPACE:
                state.bufferPtr += 1;
                break;

            case NEWLINE:
                state.bufferPtr += 1;
                state.newLinePtr = state.bufferPtr;
                state.nextToken.setLine(state.nextToken.getLine() + 1);
                break;

            default:
                return;
        }
    }
}

void Lexer::lexTokenImpl() {
    if (state.bufferPtr == state.bufferEnd) {
        return formToken(Token::Kind::EndOfFile, state.bufferPtr);
    }

    skipWhitespaceAndNewlines();

    const char* tokenStart = state.bufferPtr;

    switch (*state.bufferPtr) {
        case DECIMAL_DIGIT:
            state.bufferPtr += 1;
            return lexNumericLiteral();

        case DIRECTIVE_HEAD:
            state.bufferPtr += 1;
            return lexDirective();

        case STRING_HEAD:
            state.bufferPtr += 1;
            return lexStringLiteral();

        case IDENTIFIER_HEAD:
            state.bufferPtr += 1;
            return lexIdentifierOrKeyword();

        case OPERATOR:
            state.bufferPtr += 1;
            return lexOperator();

        case '(':
            state.bufferPtr += 1;
            return formToken(Token::Kind::LeftParenthesis, tokenStart);

        case ')':
            state.bufferPtr += 1;
            return formToken(Token::Kind::RightParenthesis, tokenStart);

        case ':':
            state.bufferPtr += 1;
            return formToken(Token::Kind::Colon, tokenStart);

        case '[':
            state.bufferPtr += 1;
            return formToken(Token::Kind::LeftBracket, tokenStart);

        case ']':
            state.bufferPtr += 1;
            return formToken(Token::Kind::RightBracket, tokenStart);

        case '{':
            state.bufferPtr += 1;
            return formToken(Token::Kind::LeftBrace, tokenStart);

        case '}':
            state.bufferPtr += 1;
            return formToken(Token::Kind::RightBrace, tokenStart);

        case ',':
            state.bufferPtr += 1;
            return formToken(Token::Kind::Comma, tokenStart);

        case '\0':
            if (state.bufferPtr != state.bufferEnd) {
                state.bufferPtr += 1;
            }

            return formToken(Token::Kind::EndOfFile, tokenStart);

        default:
            if (state.bufferPtr != state.bufferEnd) {
                state.bufferPtr += 1;
            }

            return formToken(Token::Kind::Unknown, tokenStart);
    }
}

void Lexer::lexDirective() {
    assert(*(state.bufferPtr - 1) == '#' && "Invalid start of directive!");

    const char* tokenStart = state.bufferPtr - 1;

    // Skip hash bang
    if (*state.bufferPtr == '!' && tokenStart == state.bufferStart) {
        skipToEndOfLine();
        return lexTokenImpl();
    }

    if (!advanceWhile(charIsContinuationOfIdentifier)) {
        return formToken(Token::Kind::Unknown, tokenStart);
    }

    jelly::StringRef text(tokenStart, state.bufferPtr - tokenStart);

    auto it = directives.find(text);
    if (it == directives.end()) {
        return formToken(Token::Kind::Unknown, tokenStart);
    }

    return formToken(it->getValue(), tokenStart);
}

void Lexer::lexIdentifierOrKeyword() {
    assert(charIsStartOfIdentifier(*(state.bufferPtr - 1)) && "Invalid start of identifier or keyword!");

    const char* tokenStart = state.bufferPtr - 1;

    advanceWhile(&charIsContinuationOfIdentifier);

    jelly::StringRef text(tokenStart, state.bufferPtr - tokenStart);

    auto it = keywords.find(text);
    if (it == keywords.end()) {
        return formToken(Token::Kind::Identifier, tokenStart);
    }

    return formToken(it->getValue(), tokenStart);
}

void Lexer::lexOperator() {
    assert(charIsOperator(*(state.bufferPtr - 1)) && "Invalid start of operator!");

    const char* tokenStart = state.bufferPtr - 1;

    if (*tokenStart == '-' && *state.bufferPtr == '>') {
        state.bufferPtr += 1;
        return formToken(Token::Kind::Arrow, tokenStart);
    }

    if (*tokenStart == '/' && *state.bufferPtr == '/') {
        skipToEndOfLine();
        return lexTokenImpl();
    }

    if (*tokenStart == '/' && *state.bufferPtr == '*') {
        state.bufferPtr += 1;

        if (!skipMultilineCommentTail()) {
            return formToken(Token::Kind::Unknown, tokenStart);
        }

        return lexTokenImpl();
    }

    advanceWhile(&charIsOperator);

    bool found;
    do {
        jelly::StringRef text(tokenStart, state.bufferPtr - tokenStart);
        found = context->hasOperator(text);
        state.bufferPtr -= 1;
    } while (state.bufferPtr != tokenStart && !found);

    state.bufferPtr += 1; // Previous loop will count -1 off the token end so re-add +1

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

            // Special handling for '.' to let it forward to lex_operator and make '...' or '..<' possible!
            case '.': return formToken(Token::Kind::Dot, tokenStart);
            default:  return formToken(Token::Kind::Unknown, tokenStart);
        }
    }

    return formToken(Token::Kind::Operator, tokenStart);
}

void Lexer::lexNumericLiteral() {
    assert(charIsDecimalDigit(*(state.bufferPtr - 1)) && "Expected start of numeric literal to be a decimal digit!");

    const char* tokenStart = state.bufferPtr - 1;

    auto formErrorToken = [&]() {
        // Consume all possible character continuations of the literal
        // except '.' following an operator, which could form an operator together.

        if (*state.bufferPtr == '.' && !charIsOperator(state.bufferPtr[1])) {
            state.bufferPtr += 1;
        }

        while (advanceIf(charIsAlphaNumeric)) {
            if (*state.bufferPtr == '.') {
                if (charIsOperator(state.bufferPtr[1])) {
                    break;
                } else {
                    state.bufferPtr += 1;
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

    if (*tokenStart == '0' && *state.bufferPtr == 'b') {
        state.bufferPtr += 1;
        return lexIntegerLiteral(charIsBinaryDigit);
    }

    if (*tokenStart == '0' && *state.bufferPtr == 'o') {
        state.bufferPtr += 1;
        return lexIntegerLiteral(charIsOctalDigit);
    }

    if (*tokenStart == '0' && *state.bufferPtr == 'x') {
        state.bufferPtr += 1;
        return lexHexLiteral();
    }

    advanceWhile(charIsDecimalDigit);

    if (*state.bufferPtr != '.' && *state.bufferPtr != 'e' && *state.bufferPtr != 'E') {
        if (advanceWhile(charIsContinuationOfIdentifier)) {
            return formErrorToken();
        }

        return formToken(Token::Kind::LiteralInt, tokenStart);
    }

    if (*state.bufferPtr == '.') {
        if (!charIsDecimalDigit(state.bufferPtr[1])) {
            if (!charIsOperator(state.bufferPtr[1])) {
                return formErrorToken();
            }

            return formToken(Token::Kind::LiteralInt, tokenStart);
        }

        state.bufferPtr += 1;
        advanceWhile(charIsDecimalDigit);
    }

    if (*state.bufferPtr == 'e' || *state.bufferPtr == 'E') {
        state.bufferPtr += 1;

        if (*state.bufferPtr == '+' || *state.bufferPtr == '-') {
            state.bufferPtr += 1;
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
    assert(*(state.bufferPtr - 2) == '0' && *(state.bufferPtr - 1) == 'x' && "Expected prefix of hex literal to be '0x'!");

    const char* tokenStart = state.bufferPtr - 1;

    auto formErrorToken = [&]() {
        // Consume all possible character continuations of the literal
        // except '.' following an operator, which could form an operator together.

        if (*state.bufferPtr == '.' && !charIsOperator(state.bufferPtr[1])) {
            state.bufferPtr += 1;
        }

        while (advanceIf(charIsAlphaNumeric)) {
            if (*state.bufferPtr == '.') {
                if (charIsOperator(state.bufferPtr[1])) {
                    break;
                } else {
                    state.bufferPtr += 1;
                }
            }
        }

        return formToken(Token::Kind::Unknown, tokenStart);
    };

    if (!advanceWhile(charIsHexadecimalDigit)) {
        return formErrorToken();
    }

    if (*state.bufferPtr != '.' && *state.bufferPtr != 'p' && *state.bufferPtr != 'P') {
        if (advanceWhile(charIsContinuationOfIdentifier)) {
            return formErrorToken();
        }

        return formToken(Token::Kind::LiteralInt, tokenStart);
    }

    if (*state.bufferPtr == '.') {
        if (!charIsHexadecimalDigit(state.bufferPtr[1])) {
            if (!charIsOperator(state.bufferPtr[1])) {
                return formErrorToken();
            }

            return formToken(Token::Kind::LiteralInt, tokenStart);
        }

        state.bufferPtr += 1;
        advanceWhile(charIsHexadecimalDigit);
    }

    if (*state.bufferPtr == 'p' || *state.bufferPtr == 'P') {
        state.bufferPtr += 1;

        if (*state.bufferPtr == '+' || *state.bufferPtr == '-') {
            state.bufferPtr += 1;
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
    assert(*(state.bufferPtr - 1) == '"' && "Invalid start of string literal!");

    const char* tokenStart = state.bufferPtr - 1;

    bool isValid = true;
    do {

        if (state.bufferPtr == state.bufferEnd) {
            return formToken(Token::Kind::Unknown, tokenStart);
        }

        switch (*state.bufferPtr) {
            case '\n': case '\r': case '\0':
                return formToken(Token::Kind::Unknown, tokenStart);

            case '"':
                state.bufferPtr += 1;

                if (!isValid) {
                    return formToken(Token::Kind::Unknown, tokenStart);
                }

                return formToken(Token::Kind::LiteralString, tokenStart);

            case '\\':
                state.bufferPtr += 1;

                switch (*state.bufferPtr) {
                    case ESCAPED:
                        state.bufferPtr += 1;
                        break;

                    default:
                        isValid = false;
                        break;
                }

                break;

            default:
                state.bufferPtr += 1;
                break;
        }

    } while (true);
}

#undef BINARY_DIGIT
#undef OCTAL_DIGIT
#undef DECIMAL_DIGIT
#undef HEX_DIGIT
#undef ALPHA
#undef ALPHA_NUMERIC
#undef DIRECTIVE_HEAD
#undef STRING_HEAD
#undef IDENTIFIER_HEAD
#undef IDENTIFIER_BODY
#undef OPERATOR
#undef WHITESPACE
#undef NEWLINE
#undef ESCAPED
