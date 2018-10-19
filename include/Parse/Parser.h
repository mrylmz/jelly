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

#include <AST/AST.h>

#include "Parse/Lexer.h"

#warning Replace error messages with error types

struct Parser {
    Parser(ASTContext& context, Lexer& lexer) : context(context), lexer(lexer) {
    }

    ~Parser() {
    }

    void parse();

private:
    template<typename Element>
    using Array = std::vector<Element>;

    Operator         op;
    Token            token;
    Array<ASTBlock*> scope_stack;

    ASTContext& context;
    Lexer&      lexer;

    void consume_token();

    void report_error(const char* message);

    ASTBlock* get_current_scope();

    bool scope_get_declaration(ASTBlock* block, ASTLexeme lexeme, ASTDeclaration*& declaration);
    bool scope_add_declaration(ASTBlock* block, ASTDeclaration* declaration);
    void scope_add_unresolved_identifier(ASTBlock* block, ASTIdentifier* identifier);

    // MARK: - Top Level Declarations

    ASTNode* parse_top_level_node();

    // MARK: - Directives

    ASTDirective* parse_directive();
    ASTDirective* parse_load_directive();

    // MARK: - Declarations

    ASTDeclaration* parse_enum_declaration();
    ASTDeclaration* parse_func_declaration();
    ASTDeclaration* parse_struct_declaration();
    ASTDeclaration* parse_variable_declaration();

    // MARK: - Signatures

    ASTFuncSignature* parse_func_signature();

    // MARK: - Literals

    ASTLiteral* parse_literal();

    // MARK: - Expressions

    ASTExpression* parse_expression(uint32_t precedence = 0);
    ASTExpression* parse_call_expression(ASTExpression* left);
    ASTExpression* parse_subscript_expression(ASTExpression* left);
    ASTExpression* parse_primary_expression();
    ASTExpression* parse_unary_expression();
    ASTExpression* parse_atom_expression();
    ASTExpression* parse_group_expression();

    // MARK: - Statements

    ASTStatement* parse_statement();
    ASTStatement* parse_control_statement();
    ASTStatement* parse_defer_statement();
    ASTStatement* parse_do_statement();
    ASTStatement* parse_for_statement();
    ASTStatement* parse_guard_statement();
    ASTStatement* parse_if_statement();
    ASTStatement* parse_switch_statement();
    ASTStatement* parse_while_statement();

    // MARK: - Block

    ASTBlock* parse_block();

    // MARK: - Identifiers

    ASTIdentifier* parse_identifier();

    // MARK: - Types

    ASTType* parse_type();

    // MARK: - Helpers

    ASTEnumElement* parse_enum_element();
    ASTParameter*   parse_parameter();
    ASTSwitchCase*  parse_switch_case();
};
