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

#include <vector>
#include <set>

#include <Basic/Basic.h>
#include <Syntax/Syntax.h>

struct Lexer {
    Lexer(const char* buffer);

    void lex(Token& token);
    void peek_next_token(Token& token);

    bool     get_operator(const Token& token, OperatorKind kind, Operator& op);
    uint32_t get_operator_precedence_before(uint32_t precedence);

private:
    using Predicate = bool (*)(char character);

    template<typename T>
    using Array = std::vector<T>;

    template<typename T>
    using Set = std::set<T, std::less<T>>;

    Lexer(const Lexer&) = delete;
    void operator=(const Lexer&) = delete;

    const char* buffer_start;
    const char* buffer_end;
    const char* buffer_ptr;

    StringMap<uint32_t, 256> directives;
    StringMap<uint32_t, 256> keywords;

    StringMap<size_t, 256>   prefix_operator_keys;
    Array<Operator>          prefix_operator_values;
    StringMap<size_t, 256>   infix_operator_keys;
    Array<Operator>          infix_operator_values;
    StringMap<size_t, 256>   postfix_operator_keys;
    Array<Operator>          postfix_operator_values;
    Set<uint32_t>            operator_precedence_set;

    Token next_token;

    void register_operator(const Operator& op);
    void register_operator_precedence(uint32_t precedence);

    void form_token(unsigned kind, const char* token_start);

    bool advance_if(Predicate predicate);
    bool advance_while(Predicate predicate);

    void skip_to_end_of_line();
    bool skip_multiline_comment_tail();

    void lex_next();
    void lex_directive();
    void lex_identifier_or_keyword();
    void lex_operator();
    void lex_numeric_literal();
    void lex_hex_literal();
    void lex_string_literal();
};
