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

#include <string>
#include "Parse/Lexer.h"

#warning Add UTF-8 support!

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
#define WHITESPACE_OR_NEWLINE   ' ': case '\r': case '\n': case '\t'
#define ESCAPED                 '0': case '\\': case 'r': case 'n': case 't': case '"': case '\''

static inline bool char_is_alpha_numeric(char character) {
    switch (character) {
        case ALPHA_NUMERIC: return true;
        default:            return false;
    }
}

static inline bool char_is_start_of_identifier(char character) {
    switch (character) {
        case IDENTIFIER_HEAD: return true;
        default:              return false;
    }
}

static inline bool char_is_continuation_of_identifier(char character) {
    switch (character) {
        case IDENTIFIER_BODY: return true;
        default:              return false;
    }
}

static inline bool char_is_punctuation(char character) {
    switch (character) {
        // Special handling for '.' to let it forward to lex_operator and make '...' or '..<' possible!
        case PUNCTUATION: case '.': return true;
        default:                    return false;
    }
}

static inline bool char_is_operator(char character) {
    switch (character) {
        case OPERATOR: return true;
        default:       return false;
    }
}

static inline bool char_is_whitespace_or_newline(char character) {
    switch (character) {
        case WHITESPACE_OR_NEWLINE: return true;
        default:                    return false;
    }
}

static inline bool char_is_binary_digit(char character) {
    switch (character) {
        case BINARY_DIGIT: return true;
        default:           return false;
    }
}

static inline bool char_is_octal_digit(char character) {
    switch (character) {
        case OCTAL_DIGIT: return true;
        default:          return false;
    }
}

static inline bool char_is_decimal_digit(char character) {
    switch (character) {
        case DECIMAL_DIGIT: return true;
        default:            return false;
    }
}

static inline bool char_is_hexadecimal_digit(char character) {
    switch (character) {
        case HEX_DIGIT: return true;
        default:        return false;
    }
}

Lexer::Lexer(const char* buffer) :
buffer_start(buffer),
buffer_end(buffer + strlen(buffer)),
buffer_ptr(buffer) {

    directives.set("#load", TOKEN_KEYWORD_LOAD);

    keywords.set("func", TOKEN_KEYWORD_FUNC);
    keywords.set("enum", TOKEN_KEYWORD_ENUM);
    keywords.set("struct", TOKEN_KEYWORD_STRUCT);
    keywords.set("var", TOKEN_KEYWORD_VAR);
    keywords.set("let", TOKEN_KEYWORD_LET);

    keywords.set("break", TOKEN_KEYWORD_BREAK);
    keywords.set("case", TOKEN_KEYWORD_CASE);
    keywords.set("continue", TOKEN_KEYWORD_CONTINUE);
    keywords.set("defer", TOKEN_KEYWORD_DEFER);
    keywords.set("do", TOKEN_KEYWORD_DO);
    keywords.set("else", TOKEN_KEYWORD_ELSE);
    keywords.set("fallthrough", TOKEN_KEYWORD_FALLTHROUGH);
    keywords.set("for", TOKEN_KEYWORD_FOR);
    keywords.set("guard", TOKEN_KEYWORD_GUARD);
    keywords.set("if", TOKEN_KEYWORD_IF);
    keywords.set("in", TOKEN_KEYWORD_IN);
    keywords.set("return", TOKEN_KEYWORD_RETURN);
    keywords.set("switch", TOKEN_KEYWORD_SWITCH);
    keywords.set("while", TOKEN_KEYWORD_WHILE);

    keywords.set("as", TOKEN_OPERATOR);
    keywords.set("Any", TOKEN_KEYWORD_ANY);
    keywords.set("false", TOKEN_KEYWORD_FALSE);
    keywords.set("is", TOKEN_OPERATOR);
    keywords.set("nil", TOKEN_KEYWORD_NIL);
    keywords.set("true", TOKEN_KEYWORD_TRUE);
    keywords.set("typeof", TOKEN_KEYWORD_TYPEOF);

    register_operator(Operator(OPERATOR_PREFIX, "!"));
    register_operator(Operator(OPERATOR_PREFIX, "~"));
    register_operator(Operator(OPERATOR_PREFIX, "+"));
    register_operator(Operator(OPERATOR_PREFIX, "-"));

    register_operator(Operator(OPERATOR_INFIX, "<<", ASSOCIATIVITY_NONE, 900));
    register_operator(Operator(OPERATOR_INFIX, ">>", ASSOCIATIVITY_NONE, 900));

    register_operator(Operator(OPERATOR_INFIX, "*", ASSOCIATIVITY_LEFT, 800));
    register_operator(Operator(OPERATOR_INFIX, "/", ASSOCIATIVITY_LEFT, 800));
    register_operator(Operator(OPERATOR_INFIX, "%", ASSOCIATIVITY_LEFT, 800));
    register_operator(Operator(OPERATOR_INFIX, "&", ASSOCIATIVITY_LEFT, 800));

    register_operator(Operator(OPERATOR_INFIX, "+", ASSOCIATIVITY_LEFT, 700));
    register_operator(Operator(OPERATOR_INFIX, "-", ASSOCIATIVITY_LEFT, 700));
    register_operator(Operator(OPERATOR_INFIX, "|", ASSOCIATIVITY_LEFT, 700));
    register_operator(Operator(OPERATOR_INFIX, "^", ASSOCIATIVITY_LEFT, 700));

    register_operator(Operator(OPERATOR_INFIX, "..<", ASSOCIATIVITY_NONE, 600));
    register_operator(Operator(OPERATOR_INFIX, "...", ASSOCIATIVITY_NONE, 600));

    register_operator(Operator(OPERATOR_INFIX, "is", ASSOCIATIVITY_LEFT, 500));
    register_operator(Operator(OPERATOR_INFIX, "as", ASSOCIATIVITY_LEFT, 500));

    register_operator(Operator(OPERATOR_INFIX, "<", ASSOCIATIVITY_NONE, 400));
    register_operator(Operator(OPERATOR_INFIX, "<=", ASSOCIATIVITY_NONE, 400));
    register_operator(Operator(OPERATOR_INFIX, ">", ASSOCIATIVITY_NONE, 400));
    register_operator(Operator(OPERATOR_INFIX, ">=", ASSOCIATIVITY_NONE, 400));
    register_operator(Operator(OPERATOR_INFIX, "==", ASSOCIATIVITY_NONE, 400));
    register_operator(Operator(OPERATOR_INFIX, "!=", ASSOCIATIVITY_NONE, 400));

    register_operator(Operator(OPERATOR_INFIX, "&&", ASSOCIATIVITY_LEFT, 300));

    register_operator(Operator(OPERATOR_INFIX, "||", ASSOCIATIVITY_LEFT, 200));

    register_operator(Operator(OPERATOR_INFIX, "=", ASSOCIATIVITY_RIGHT, 100));
    register_operator(Operator(OPERATOR_INFIX, "*=", ASSOCIATIVITY_RIGHT, 100));
    register_operator(Operator(OPERATOR_INFIX, "/=", ASSOCIATIVITY_RIGHT, 100));
    register_operator(Operator(OPERATOR_INFIX, "%=", ASSOCIATIVITY_RIGHT, 100));
    register_operator(Operator(OPERATOR_INFIX, "+=", ASSOCIATIVITY_RIGHT, 100));
    register_operator(Operator(OPERATOR_INFIX, "-=", ASSOCIATIVITY_RIGHT, 100));
    register_operator(Operator(OPERATOR_INFIX, "<<=", ASSOCIATIVITY_RIGHT, 100));
    register_operator(Operator(OPERATOR_INFIX, ">>=", ASSOCIATIVITY_RIGHT, 100));
    register_operator(Operator(OPERATOR_INFIX, "&=", ASSOCIATIVITY_RIGHT, 100));
    register_operator(Operator(OPERATOR_INFIX, "|=", ASSOCIATIVITY_RIGHT, 100));
    register_operator(Operator(OPERATOR_INFIX, "^=", ASSOCIATIVITY_RIGHT, 100));

    // Pointer Type Operator
    register_operator(Operator(OPERATOR_POSTFIX, "*"));
    register_operator(Operator(OPERATOR_POSTFIX, "()", ASSOCIATIVITY_LEFT, 1000, true));

    // Subscripting:        OP_POSTFIX, [], ASSOC_LEFT, 0
    // Element selection:   OP_POSTFIX, . , ASSOC_LEFT, 0

    lex_next();
}

void Lexer::lex(Token &token) {
    token = next_token;

    if (!next_token.is(TOKEN_EOF)) {
        lex_next();
    }
}

void Lexer::peek_next_token(Token& token) {
    token = next_token;
}

bool Lexer::get_operator(const Token& token, OperatorKind kind, Operator& op) {
    StringMap<size_t, 256>* operator_keys;
    Array<Operator>*        operator_values;

    switch (kind) {
        case OPERATOR_PREFIX:
            operator_keys = &prefix_operator_keys;
            operator_values = &prefix_operator_values;
            break;

        case OPERATOR_INFIX:
            operator_keys = &infix_operator_keys;
            operator_values = &infix_operator_values;
            break;

        case OPERATOR_POSTFIX:
            operator_keys = &postfix_operator_keys;
            operator_values = &postfix_operator_values;
            break;

        default:
            return false;
    }

    size_t index;
    if (operator_keys->get(token.text, index)) {
        op = operator_values->at(index);
        return true;
    }

    return false;
}

uint32_t Lexer::get_operator_precedence_before(uint32_t precedence) {
    auto it = operator_precedence_set.lower_bound(precedence);
    if (it != operator_precedence_set.begin()) {
        return *(it--);
    }

    return 0;
}

void Lexer::register_operator(const Operator& op) {
    assert(!op.text.is_equal("->")     && "'->' is reserved and cannot be added as operator!");
    assert(!op.text.is_equal(".")      && "'.' is reserved and cannot be added as operator!");
    assert(!op.text.is_equal("//")     && "'//' is reserved and cannot be added as operator!");
    assert(!op.text.is_equal("/*")     && "'/*' is reserved and cannot be added as operator!");
    assert(!op.text.is_equal("*/")     && "'*/' is reserved and cannot be added as operator!");

    StringMap<size_t, 256>* operator_keys;
    Array<Operator>*        operator_values;

    switch (op.kind) {
        case OPERATOR_PREFIX:
            operator_keys = &prefix_operator_keys;
            operator_values = &prefix_operator_values;
            break;

        case OPERATOR_INFIX:
            operator_keys = &infix_operator_keys;
            operator_values = &infix_operator_values;
            register_operator_precedence(op.precedence);
            break;

        case OPERATOR_POSTFIX:
            operator_keys = &postfix_operator_keys;
            operator_values = &postfix_operator_values;
            register_operator_precedence(op.precedence);
            break;

        default:
            assert(false && "Invalid operator kind given!");
    }

    size_t index;

    String text = op.text;
    if (op.can_have_arguments) {
        text = text.prefix(1);
    }

    assert(!operator_keys->get(text, index) && "Overriding operators is not allowed!");

    index = operator_values->size();
    operator_keys->set(text, index);
    operator_values->push_back(op);
}

void Lexer::register_operator_precedence(uint32_t precedence) {
    operator_precedence_set.insert(precedence);
}

void Lexer::form_token(unsigned kind, const char* token_start) {
    next_token.kind = kind;
    next_token.text = String(token_start, buffer_ptr - token_start);
}

bool Lexer::advance_if(Predicate predicate) {
    if (predicate(*buffer_ptr)) {
        assert(buffer_ptr != buffer_end && "Predicate is going out of buffer range!");

        buffer_ptr += 1;
        return true;
    }

    return false;
}

bool Lexer::advance_while(Predicate predicate) {
    bool result = advance_if(predicate);

    while (predicate(*buffer_ptr)) {
        assert(buffer_ptr != buffer_end && "Predicate is going out of buffer range!");

        buffer_ptr += 1;
    }

    return result;
}

void Lexer::skip_to_end_of_line() {
    while (buffer_ptr != buffer_end && *buffer_ptr != '\n' && *buffer_ptr != '\r') {
        buffer_ptr += 1;
    }
}

bool Lexer::skip_multiline_comment_tail() {
    assert((*(buffer_ptr - 2) == '/' && *(buffer_ptr - 1) == '*') && "Invalid start of multiline comment!");

    buffer_ptr -= 1;

    do {

        buffer_ptr += 1;

        if (buffer_ptr[0] == '*' && buffer_ptr[1] == '/') {
            buffer_ptr += 2;
            return true;
        }

        if (buffer_ptr[0] == '/' && buffer_ptr[1] == '*') {
            buffer_ptr += 2;

            if (!skip_multiline_comment_tail()) {
                return false;
            }
        }

    } while (buffer_ptr != buffer_end);

    return false;
}

void Lexer::lex_next() {
    const char* token_start = buffer_ptr;

    if (buffer_ptr == buffer_end) {
        return form_token(TOKEN_EOF, token_start);
    }

    if (advance_while(&char_is_whitespace_or_newline)) {
        return lex_next();
    }

    switch (*buffer_ptr) {
        case DECIMAL_DIGIT:
            buffer_ptr += 1;
            return lex_numeric_literal();

        case DIRECTIVE_HEAD:
            buffer_ptr += 1;
            return lex_directive();

        case STRING_HEAD:
            buffer_ptr += 1;
            return lex_string_literal();

        case IDENTIFIER_HEAD:
            buffer_ptr += 1;
            return lex_identifier_or_keyword();

        case OPERATOR:
            buffer_ptr += 1;
            return lex_operator();

        case PUNCTUATION:
            buffer_ptr += 1;
            return form_token(*token_start, token_start);

        case '\0':
            if (buffer_ptr != buffer_end) {
                buffer_ptr += 1;
            }

            return form_token(TOKEN_EOF, token_start);

        default:
            if (buffer_ptr != buffer_end) {
                buffer_ptr += 1;
            }

            return form_token(TOKEN_UNKNOWN, token_start);
    }
}

void Lexer::lex_directive() {
    assert(*(buffer_ptr - 1) == '#' && "Invalid start of directive!");

    const char* token_start = buffer_ptr - 1;

    // Skip hash bang
    if (*buffer_ptr == '!' && token_start == buffer_start) {
        skip_to_end_of_line();
        return lex_next();
    }

    if (!advance_while(char_is_continuation_of_identifier)) {
        return form_token(TOKEN_UNKNOWN, token_start);
    }

    String   text(token_start, buffer_ptr - token_start);
    uint32_t kind;

    if (!directives.get(text, kind)) {
        return form_token(TOKEN_UNKNOWN, token_start);
    }

    return form_token(kind, token_start);
}

void Lexer::lex_identifier_or_keyword() {
    assert(char_is_start_of_identifier(*(buffer_ptr - 1)) && "Invalid start of identifier or keyword!");

    const char* token_start = buffer_ptr - 1;

    advance_while(&char_is_continuation_of_identifier);

    String   text(token_start, buffer_ptr - token_start);
    uint32_t kind;

    if (!keywords.get(text, kind)) {
        kind = TOKEN_IDENTIFIER;
    }

    return form_token(kind, token_start);
}

void Lexer::lex_operator() {
    assert(char_is_operator(*(buffer_ptr - 1)) && "Invalid start of operator!");

    const char*  token_start = buffer_ptr - 1;

    if (*token_start == '-' && *buffer_ptr == '>') {
        buffer_ptr += 1;
        return form_token(TOKEN_ARROW, token_start);
    }

    if (*token_start == '/' && *buffer_ptr == '/') {
        skip_to_end_of_line();
        return lex_next();
    }

    if (*token_start == '/' && *buffer_ptr == '*') {
        buffer_ptr += 1;

        if (!skip_multiline_comment_tail()) {
            return form_token(TOKEN_UNKNOWN, token_start);
        }

        return lex_next();
    }

    advance_while(&char_is_operator);

    String text;
    bool   found;

    do {

        text = String(token_start, buffer_ptr - token_start);
        found = (
            prefix_operator_keys.has(text) ||
            infix_operator_keys.has(text) ||
            postfix_operator_keys.has(text)
        );

        buffer_ptr -= 1;

    } while (buffer_ptr != token_start && !found);

    // Previous loop will count -1 off the token end so re-add +1
    buffer_ptr += 1;

    if (!found) {
        if (char_is_punctuation(*token_start)) {
            return form_token(*token_start, token_start);
        }

        return form_token(TOKEN_UNKNOWN, token_start);
    }

    return form_token(TOKEN_OPERATOR, token_start);
}

void Lexer::lex_numeric_literal() {
    assert(char_is_decimal_digit(*(buffer_ptr - 1)) && "Expected start of numeric literal to be a decimal digit!");

    const char* token_start = buffer_ptr - 1;

    auto form_error_token = [&]() {
        // Consume all possible character continuations of the literal
        // except '.' following an operator, which could form an operator together.

        if (*buffer_ptr == '.' && !char_is_operator(buffer_ptr[1])) {
            buffer_ptr += 1;
        }

        while (advance_if(char_is_alpha_numeric)) {
            if (*buffer_ptr == '.') {
                if (char_is_operator(buffer_ptr[1])) {
                    break;
                } else {
                    buffer_ptr += 1;
                }
            }
        }

        return form_token(TOKEN_UNKNOWN, token_start);
    };

    auto lex_integer_literal = [&](Predicate predicate) {
        if (!advance_while(predicate)) {
            return form_error_token();
        }

        if (advance_while(char_is_continuation_of_identifier)) {
            return form_error_token();
        }

        return form_token(TOKEN_LITERAL_INT, token_start);
    };

    if (*token_start == '0' && *buffer_ptr == 'b') {
        buffer_ptr += 1;
        return lex_integer_literal(char_is_binary_digit);
    }

    if (*token_start == '0' && *buffer_ptr == 'o') {
        buffer_ptr += 1;
        return lex_integer_literal(char_is_octal_digit);
    }

    if (*token_start == '0' && *buffer_ptr == 'x') {
        buffer_ptr += 1;
        return lex_hex_literal();
    }

    advance_while(char_is_decimal_digit);

    if (*buffer_ptr != '.' && *buffer_ptr != 'e' && *buffer_ptr != 'E') {
        if (advance_while(char_is_continuation_of_identifier)) {
            return form_error_token();
        }

        return form_token(TOKEN_LITERAL_INT, token_start);
    }

    if (*buffer_ptr == '.') {
        if (!char_is_decimal_digit(buffer_ptr[1])) {
            if (!char_is_operator(buffer_ptr[1])) {
                return form_error_token();
            }

            return form_token(TOKEN_LITERAL_INT, token_start);
        }

        buffer_ptr += 1;
        advance_while(char_is_decimal_digit);
    }

    if (*buffer_ptr == 'e' || *buffer_ptr == 'E') {
        buffer_ptr += 1;

        if (*buffer_ptr == '+' || *buffer_ptr == '-') {
            buffer_ptr += 1;
        }

        if (!advance_while(char_is_decimal_digit)) {
            return form_error_token();
        }
    }

    if (advance_while(char_is_continuation_of_identifier)) {
        return form_error_token();
    }

    return form_token(TOKEN_LITERAL_FLOAT, token_start);
}

void Lexer::lex_hex_literal() {
    assert(*(buffer_ptr - 2) == '0' && *(buffer_ptr - 1) == 'x' && "Expected prefix of hex literal to be '0x'!");

    const char* token_start = buffer_ptr - 1;

    auto form_error_token = [&]() {
        // Consume all possible character continuations of the literal
        // except '.' following an operator, which could form an operator together.

        if (*buffer_ptr == '.' && !char_is_operator(buffer_ptr[1])) {
            buffer_ptr += 1;
        }

        while (advance_if(char_is_alpha_numeric)) {
            if (*buffer_ptr == '.') {
                if (char_is_operator(buffer_ptr[1])) {
                    break;
                } else {
                    buffer_ptr += 1;
                }
            }
        }

        return form_token(TOKEN_UNKNOWN, token_start);
    };

    if (!advance_while(char_is_hexadecimal_digit)) {
        return form_error_token();
    }

    if (*buffer_ptr != '.' && *buffer_ptr != 'p' && *buffer_ptr != 'P') {
        if (advance_while(char_is_continuation_of_identifier)) {
            return form_error_token();
        }

        return form_token(TOKEN_LITERAL_INT, token_start);
    }

    if (*buffer_ptr == '.') {
        if (!char_is_hexadecimal_digit(buffer_ptr[1])) {
            if (!char_is_operator(buffer_ptr[1])) {
                return form_error_token();
            }

            return form_token(TOKEN_LITERAL_INT, token_start);
        }

        buffer_ptr += 1;
        advance_while(char_is_hexadecimal_digit);
    }

    if (*buffer_ptr == 'p' || *buffer_ptr == 'P') {
        buffer_ptr += 1;

        if (*buffer_ptr == '+' || *buffer_ptr == '-') {
            buffer_ptr += 1;
        }

        if (!advance_while(char_is_hexadecimal_digit)) {
            return form_error_token();
        }
    }

    if (advance_while(char_is_continuation_of_identifier)) {
        return form_error_token();
    }

    return form_token(TOKEN_LITERAL_FLOAT, token_start);
}

void Lexer::lex_string_literal() {
    assert(*(buffer_ptr - 1) == '"' && "Invalid start of string literal!");

    const char* token_start = buffer_ptr - 1;
    bool        is_valid = true;

    do {

        if (buffer_ptr == buffer_end) {
             return form_token(TOKEN_UNKNOWN, token_start);
        }

        switch (*buffer_ptr) {
            case '\n': case '\r': case '\0':
                return form_token(TOKEN_UNKNOWN, token_start);

            case '"':
                buffer_ptr += 1;

                if (!is_valid) {
                    return form_token(TOKEN_UNKNOWN, token_start);
                }

                return form_token(TOKEN_LITERAL_STRING, token_start);

            case '\\':
                buffer_ptr += 1;

                switch (*buffer_ptr) {
                    case ESCAPED:
                        buffer_ptr += 1;
                        break;

                    default:
                        is_valid = false;
                        break;
                }

                break;

            default:
                buffer_ptr += 1;
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
#undef IDENTIFIER_HEAD
#undef IDENTIFIER_BODY
#undef PUNCTUATION
#undef OPERATOR
#undef WHITESPACE_OR_NEWLINE
