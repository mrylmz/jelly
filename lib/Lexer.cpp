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

#include "Core/Lexer.h"

#include <llvm/ADT/StringRef.h>
#include <llvm/Support/ErrorHandling.h>

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
#define PUNCTUATION             '(': case ')': case ':': case '[': case ']': case '{': case '}': case ',' /*: case '.' is handled in lex_operator */
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

static inline bool charIsPunctuation(char character) {
    switch (character) {
        // Special handling for '.' to let it forward to lex_operator and make '...' or '..<' possible!
        case PUNCTUATION: case '.': return true;
        default:                    return false;
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

Lexer::Lexer(const char* buffer) : state(buffer) {
    directives.try_emplace("#load", TOKEN_KEYWORD_LOAD);

    keywords.try_emplace("func", TOKEN_KEYWORD_FUNC);
    keywords.try_emplace("enum", TOKEN_KEYWORD_ENUM);
    keywords.try_emplace("struct", TOKEN_KEYWORD_STRUCT);
    keywords.try_emplace("var", TOKEN_KEYWORD_VAR);
    keywords.try_emplace("let", TOKEN_KEYWORD_LET);

    keywords.try_emplace("break", TOKEN_KEYWORD_BREAK);
    keywords.try_emplace("case", TOKEN_KEYWORD_CASE);
    keywords.try_emplace("continue", TOKEN_KEYWORD_CONTINUE);
    keywords.try_emplace("defer", TOKEN_KEYWORD_DEFER);
    keywords.try_emplace("do", TOKEN_KEYWORD_DO);
    keywords.try_emplace("else", TOKEN_KEYWORD_ELSE);
    keywords.try_emplace("fallthrough", TOKEN_KEYWORD_FALLTHROUGH);
    keywords.try_emplace("for", TOKEN_KEYWORD_FOR);
    keywords.try_emplace("guard", TOKEN_KEYWORD_GUARD);
    keywords.try_emplace("if", TOKEN_KEYWORD_IF);
    keywords.try_emplace("in", TOKEN_KEYWORD_IN);
    keywords.try_emplace("return", TOKEN_KEYWORD_RETURN);
    keywords.try_emplace("switch", TOKEN_KEYWORD_SWITCH);
    keywords.try_emplace("while", TOKEN_KEYWORD_WHILE);

    keywords.try_emplace("as", TOKEN_OPERATOR);
    keywords.try_emplace("Any", TOKEN_KEYWORD_ANY);
    keywords.try_emplace("false", TOKEN_KEYWORD_FALSE);
    keywords.try_emplace("is", TOKEN_OPERATOR);
    keywords.try_emplace("nil", TOKEN_KEYWORD_NIL);
    keywords.try_emplace("true", TOKEN_KEYWORD_TRUE);
    keywords.try_emplace("typeof", TOKEN_KEYWORD_TYPEOF);
    // TODO: [1] Reserve Builtin types as keywords !!!

#define PREFIX_OPERATOR(__SYMBOL__) \
    registerOperator(Operator(OPERATOR_PREFIX, __SYMBOL__));

#define INFIX_OPERATOR(__SYMBOL__, __ASSOCIATIVITY__, __PRECEDENCE__, __CAN_HAVE_ARGS__, __IS_ASSIGNMENT__) \
    registerOperator(Operator(OPERATOR_INFIX, __SYMBOL__, __ASSOCIATIVITY__, __PRECEDENCE__, __CAN_HAVE_ARGS__, __IS_ASSIGNMENT__));

#define POSTFIX_OPERATOR(__SYMBOL__, __ASSOCIATIVITY__, __PRECEDENCE__, __CAN_HAVE_ARGS__, __IS_ASSIGNMENT__) \
    registerOperator(Operator(OPERATOR_POSTFIX, __SYMBOL__, __ASSOCIATIVITY__, __PRECEDENCE__, __CAN_HAVE_ARGS__, __IS_ASSIGNMENT__));

#include "Core/Operators.def"

    state.nextToken.line = 1;

    lexTokenImpl();
}

Token Lexer::lexToken() {
    Token token = state.nextToken;
    if (!state.nextToken.is(TOKEN_EOF)) {
        lexTokenImpl();
    }

    return token;
}

Token Lexer::peekNextToken() {
    return state.nextToken;
}

bool Lexer::getOperator(llvm::StringRef name, OperatorKind kind, Operator& op) {
    switch (kind) {
        case OPERATOR_PREFIX:  return getOperator(name, op, prefixOperators);
        case OPERATOR_INFIX:   return getOperator(name, op, infixOperators);
        case OPERATOR_POSTFIX: return getOperator(name, op, postfixOperators);
        default:               llvm_unreachable("Invalid kind given for OperatorKind!");
    }
}

bool Lexer::getOperator(llvm::StringRef name, Operator& op, llvm::StringMap<Operator>& operators) {
    auto it = operators.find(name);
    if (it != operators.end()) {
        op = it->getValue();
        return true;
    }

    return false;
}

bool Lexer::hasOperator(llvm::StringRef text) {
    return prefixOperators.count(text) > 0 || infixOperators.count(text) > 0 || postfixOperators.count(text) > 0;
}

Precedence Lexer::getOperatorPrecedenceBefore(Precedence precedence) {
    auto it = operatorPrecedenceSet.lower_bound(precedence);
    if (it != operatorPrecedenceSet.begin()) {
        return *(it--);
    }

    return 0;
}

void Lexer::registerOperator(Operator op) {
    switch (op.kind) {
        case OPERATOR_PREFIX:  return registerOperator(op, prefixOperators);
        case OPERATOR_INFIX:   return registerOperator(op, infixOperators);
        case OPERATOR_POSTFIX: return registerOperator(op, postfixOperators);
        default:               llvm_unreachable("Invalid kind given for OperatorKind!");
    }
}

void Lexer::registerOperator(Operator op, llvm::StringMap<Operator>& operators) {
    assert(!op.text.equals("->") && "'->' is reserved and cannot be added as operator!");
    assert(!op.text.equals("//") && "'//' is reserved and cannot be added as operator!");
    assert(!op.text.equals("/*") && "'/*' is reserved and cannot be added as operator!");
    assert(!op.text.equals("*/") && "'*/' is reserved and cannot be added as operator!");

    auto text = op.canHaveArgs ? op.text.take_front(1) : op.text;
    bool success = operators.try_emplace(text, op).second;
    assert(success && "Overriding operators is not allowed!");

    if (!success) {
        llvm::report_fatal_error("Internal compiler error!");
    }

    registerOperatorPrecedence(op.precedence);
}

void Lexer::registerOperatorPrecedence(Precedence precedence) {
    operatorPrecedenceSet.insert(precedence);
}

void Lexer::formToken(unsigned kind, const char* tokenStart) {
    state.nextToken.kind = kind;
    state.nextToken.text = llvm::StringRef(tokenStart, state.bufferPtr - tokenStart);
    state.nextToken.column = tokenStart - state.newLinePtr;
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
                state.nextToken.line += 1;
                break;

            default:
                return;
        }
    }
}

void Lexer::lexTokenImpl() {
    if (state.bufferPtr == state.bufferEnd) {
        return formToken(TOKEN_EOF, state.bufferPtr);
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

        case PUNCTUATION:
            state.bufferPtr += 1;
            return formToken(*tokenStart, tokenStart);

        case '\0':
            if (state.bufferPtr != state.bufferEnd) {
                state.bufferPtr += 1;
            }

            return formToken(TOKEN_EOF, tokenStart);

        default:
            if (state.bufferPtr != state.bufferEnd) {
                state.bufferPtr += 1;
            }

            return formToken(TOKEN_UNKNOWN, tokenStart);
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
        return formToken(TOKEN_UNKNOWN, tokenStart);
    }

    llvm::StringRef text(tokenStart, state.bufferPtr - tokenStart);

    auto it = directives.find(text);
    if (it == directives.end()) {
        return formToken(TOKEN_UNKNOWN, tokenStart);
    }

    return formToken(it->getValue(), tokenStart);
}

void Lexer::lexIdentifierOrKeyword() {
    assert(charIsStartOfIdentifier(*(state.bufferPtr - 1)) && "Invalid start of identifier or keyword!");

    const char* tokenStart = state.bufferPtr - 1;

    advanceWhile(&charIsContinuationOfIdentifier);

    llvm::StringRef text(tokenStart, state.bufferPtr - tokenStart);

    auto it = keywords.find(text);
    if (it == keywords.end()) {
        return formToken(TOKEN_IDENTIFIER, tokenStart);
    }

    return formToken(it->getValue(), tokenStart);
}

void Lexer::lexOperator() {
    assert(charIsOperator(*(state.bufferPtr - 1)) && "Invalid start of operator!");

    const char* tokenStart = state.bufferPtr - 1;

    if (*tokenStart == '-' && *state.bufferPtr == '>') {
        state.bufferPtr += 1;
        return formToken(TOKEN_ARROW, tokenStart);
    }

    if (*tokenStart == '/' && *state.bufferPtr == '/') {
        skipToEndOfLine();
        return lexTokenImpl();
    }

    if (*tokenStart == '/' && *state.bufferPtr == '*') {
        state.bufferPtr += 1;

        if (!skipMultilineCommentTail()) {
            return formToken(TOKEN_UNKNOWN, tokenStart);
        }

        return lexTokenImpl();
    }

    advanceWhile(&charIsOperator);

    bool found;
    do {
        llvm::StringRef text(tokenStart, state.bufferPtr - tokenStart);
        found = hasOperator(text);
        state.bufferPtr -= 1;
    } while (state.bufferPtr != tokenStart && !found);

    state.bufferPtr += 1; // Previous loop will count -1 off the token end so re-add +1

    if (!found) {
        if (charIsPunctuation(*tokenStart)) {
            return formToken(*tokenStart, tokenStart);
        }

        return formToken(TOKEN_UNKNOWN, tokenStart);
    }

    return formToken(TOKEN_OPERATOR, tokenStart);
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

        return formToken(TOKEN_UNKNOWN, tokenStart);
    };

    auto lexIntegerLiteral = [&](CharPredicate predicate) {
        if (!advanceWhile(predicate)) {
            return formErrorToken();
        }

        if (advanceWhile(charIsContinuationOfIdentifier)) {
            return formErrorToken();
        }

        return formToken(TOKEN_LITERAL_INT, tokenStart);
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

        return formToken(TOKEN_LITERAL_INT, tokenStart);
    }

    if (*state.bufferPtr == '.') {
        if (!charIsDecimalDigit(state.bufferPtr[1])) {
            if (!charIsOperator(state.bufferPtr[1])) {
                return formErrorToken();
            }

            return formToken(TOKEN_LITERAL_INT, tokenStart);
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

    return formToken(TOKEN_LITERAL_FLOAT, tokenStart);
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

        return formToken(TOKEN_UNKNOWN, tokenStart);
    };

    if (!advanceWhile(charIsHexadecimalDigit)) {
        return formErrorToken();
    }

    if (*state.bufferPtr != '.' && *state.bufferPtr != 'p' && *state.bufferPtr != 'P') {
        if (advanceWhile(charIsContinuationOfIdentifier)) {
            return formErrorToken();
        }

        return formToken(TOKEN_LITERAL_INT, tokenStart);
    }

    if (*state.bufferPtr == '.') {
        if (!charIsHexadecimalDigit(state.bufferPtr[1])) {
            if (!charIsOperator(state.bufferPtr[1])) {
                return formErrorToken();
            }

            return formToken(TOKEN_LITERAL_INT, tokenStart);
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

    return formToken(TOKEN_LITERAL_FLOAT, tokenStart);
}

void Lexer::lexStringLiteral() {
    assert(*(state.bufferPtr - 1) == '"' && "Invalid start of string literal!");

    const char* tokenStart = state.bufferPtr - 1;

    bool isValid = true;
    do {

        if (state.bufferPtr == state.bufferEnd) {
             return formToken(TOKEN_UNKNOWN, tokenStart);
        }

        switch (*state.bufferPtr) {
            case '\n': case '\r': case '\0':
                return formToken(TOKEN_UNKNOWN, tokenStart);

            case '"':
                state.bufferPtr += 1;

                if (!isValid) {
                    return formToken(TOKEN_UNKNOWN, tokenStart);
                }

                return formToken(TOKEN_LITERAL_STRING, tokenStart);

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
#undef PUNCTUATION
#undef OPERATOR
#undef WHITESPACE
#undef NEWLINE
#undef ESCAPED
