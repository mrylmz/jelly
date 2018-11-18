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

#include <AST/AST.h>
#include <Basic/Basic.h>
#include <vector>
#include "Parse/Parser.h"

// TODO: Check line-break requirements, do not allow consecutive statements on a line
// TODO: Check if symbols of unary expressions are right bound ! (unexpected: ~ value, expected: ~value)
// TODO: Write unit-tests for assignment of parents

void Parser::parse() {
    lexer.peek_next_token(token);

    parent_stack.push_back(context.root);
    defer(parent_stack.pop_back());

    ASTNode* node;
    while ((node = parse_top_level_node()) != nullptr) {
        context.root->statements.push_back(node);
    }
}

void Parser::consume_token() {
    lexer.lex(token);
    lexer.peek_next_token(token);
}

void Parser::report_error(const char* message) {
// TODO: Forward error messages to diagnostics engine!
    printf("Error: %s\n", message);
}

// MARK: - Top Level Declarations

/// grammar: top-level-node := directive | enum-declaration | func-declaration | struct-declaration | variable-declaration
ASTNode* Parser::parse_top_level_node() {
    switch (token.kind) {
        case TOKEN_KEYWORD_LOAD:
            return parse_directive();

        case TOKEN_KEYWORD_ENUM:
            return parse_enum_declaration();

        case TOKEN_KEYWORD_FUNC:
            return parse_func_declaration();

        case TOKEN_KEYWORD_STRUCT:
            return parse_struct_declaration();

        case TOKEN_KEYWORD_VAR:
        case TOKEN_KEYWORD_LET:
            return parse_variable_declaration();

        case TOKEN_EOF:
            return nullptr;

        default:
            report_error("Unexpected token found expected top level declaration!");
            return nullptr;
    }
}

// MARK: - Directives

/// grammar: directive := load-directive
ASTDirective* Parser::parse_directive() {
    switch (token.kind) {
        case TOKEN_KEYWORD_LOAD:
            return parse_load_directive();

        default:
            unreachable("Invalid token given for start of directive!");
    }

    return nullptr;
}

/// grammar: load-directive := "#load" string-literal
ASTDirective* Parser::parse_load_directive() {
    assert(token.is(TOKEN_KEYWORD_LOAD) && "Invalid token given for start of load directive!");
    consume_token();

    ASTLoad* load = new (&context) ASTLoad;
    load->parent = parent_stack.back();

    parent_stack.push_back(load);
    defer(parent_stack.pop_back());

    load->literal = parse_literal();
    if (load->literal == nullptr || load->literal->token_kind != TOKEN_LITERAL_STRING) {
        report_error("Expected string literal after load directive!");
        return nullptr;
    }

    return load;
}

// MARK: - Declarations

/// grammar: enum-declaration := "enum" identifier "{" [ enum-element { line-break enum-element } ] "}"
ASTDeclaration* Parser::parse_enum_declaration() {
    assert(token.is(TOKEN_KEYWORD_ENUM) && "Invalid token given for start of enum!");
    consume_token();

    ASTEnum* enumeration = new (&context) ASTEnum;
    enumeration->type = new (&context) ASTEnumType(enumeration);
    enumeration->parent = parent_stack.back();
    parent_stack.push_back(enumeration);
    defer(parent_stack.pop_back());

    enumeration->name = parse_identifier();
    if (enumeration->name == nullptr) {
        report_error("Expected identifier for name of enum declaration!");
        return nullptr;
    }

    if (!token.is('{')) {
        report_error("Expected '{' after name of enum declaration!");
        return nullptr;
    }
    consume_token();

    enumeration->block = new (&context) ASTBlock;
    enumeration->block->parent = parent_stack.back();

    if (!token.is('}')) {
        parent_stack.push_back(enumeration->block);
        defer(parent_stack.pop_back());

        while (true) {
            ASTEnumElement* element = parse_enum_element();
            if (element == nullptr) {
                return nullptr;
            }

            enumeration->block->statements.push_back(element);

            if (token.is('}')) {
                break;
            } else if (!token.is(TOKEN_KEYWORD_CASE)) {
                report_error("Expected '}' at end of enum declaration!");
                return nullptr;
            }
        }
    }
    consume_token();

    return enumeration;
}

/// grammar: func-declaration := func-signature block
ASTDeclaration* Parser::parse_func_declaration() {
    assert(token.is(TOKEN_KEYWORD_FUNC) && "Invalid token given for start of func declaration!");
    consume_token();

    ASTFunc* func = new (&context) ASTFunc;
    func->parent = parent_stack.back();
    parent_stack.push_back(func);
    defer(parent_stack.pop_back());

    func->name = parse_identifier();
    if (func->name == nullptr) {
        report_error("Expected identifier in function declaration!");
        return nullptr;
    }

    func->signature = parse_func_signature();
    if (func->signature == nullptr) {
        return nullptr;
    }

    auto func_type = new (&context) ASTFuncType(func);
    func->type = func_type;

    for (auto parameter : func->signature->parameters) {
        func_type->parameter_types.push_back(parameter->type);
    }

    func_type->return_type = func->signature->return_type;

    func->block = parse_block();
    if (func->block == nullptr) {
        return nullptr;
    }

    return func;
}

/// grammar: struct-declaration := "struct" identifier block
ASTDeclaration* Parser::parse_struct_declaration() {
    assert(token.is(TOKEN_KEYWORD_STRUCT) && "Invalid token given for start of struct!");
    consume_token();

    ASTStruct* structure = new (&context) ASTStruct;
    structure->parent = parent_stack.back();
    parent_stack.push_back(structure);
    defer(parent_stack.pop_back());

    structure->name = parse_identifier();
    if (structure->name == nullptr) {
        report_error("Expected identifier for name of struct declaration!");
        return nullptr;
    }

    structure->block = parse_block();
    if (structure->block == nullptr) {
        return nullptr;
    }

    auto struct_type = new (&context) ASTStructType(structure);
    structure->type = struct_type;

    for (auto stmt : structure->block->statements) {
        if (stmt->is(AST_VARIABLE)) {
            auto variable = reinterpret_cast<ASTVariable*>(stmt);
            struct_type->member_types.push_back(variable->type);
        }
    }

    return structure;
}

/// grammar: variable-declaration := ( "var" | "let" ) identifier ":" type-identifier [ "=" expression ]
ASTDeclaration* Parser::parse_variable_declaration() {
    assert(token.is(TOKEN_KEYWORD_VAR, TOKEN_KEYWORD_LET) && "Invalid token given for start of variable-declaration!");

    ASTVariable* variable = new (&context) ASTVariable;
    variable->parent = parent_stack.back();
    parent_stack.push_back(variable);
    defer(parent_stack.pop_back());

    if (token.is(TOKEN_KEYWORD_LET)) {
        variable->flags |= AST_VARIABLE_IS_CONSTANT;
    }
    consume_token();

    variable->name = parse_identifier();
    if (variable->name == nullptr) {
        report_error("Expected identifier for name of variable declaration!");
        return nullptr;
    }

    if (!token.is(':')) {
        report_error("Expected ':' after variable name identifier!");
        return nullptr;
    }
    consume_token();

    variable->type = parse_type();
    if (variable->type == nullptr) {
        report_error("Expected type of variable declaration!");
        return nullptr;
    }

    if (token.is(TOKEN_OPERATOR) && lexer.get_operator(token, OPERATOR_INFIX, op) && op.text.is_equal("=")) {
        consume_token();

        variable->assignment = parse_expression();
        if (variable->assignment == nullptr) {
            report_error("Expected expression after '=' assignment operator!");
            return nullptr;
        }
    }

    return variable;
}

// MARK: - Context Declarations

/// grammar: enum-element := "case" identifier [ "=" expression ]
ASTEnumElement* Parser::parse_enum_element() {
    if (!token.is(TOKEN_KEYWORD_CASE)) {
        report_error("Expected 'case' keyword at start of enum element!");
        return nullptr;
    }
    consume_token();

    ASTEnumElement* element = new (&context) ASTEnumElement;
    element->type = context.type_Int;
    element->parent = parent_stack.back();
    parent_stack.push_back(element);
    defer(parent_stack.pop_back());

    element->name = parse_identifier();
    if (element->name == nullptr) {
        report_error("Expected identifier for name of enum element!");
        return nullptr;
    }

    if (token.is(TOKEN_OPERATOR) && lexer.get_operator(token, OPERATOR_INFIX, op) && op.text.is_equal("=")) {
        consume_token();

        element->assignment = parse_expression();
        if (element->assignment == nullptr) {
            report_error("Expected expression after '=' assignment operator!");
            return nullptr;
        }
    }

    return element;
}

/// grammar: parameter := identifier ":" type-identifier
ASTParameter* Parser::parse_parameter() {
    ASTParameter* parameter = new (&context) ASTParameter;
    parameter->parent = parent_stack.back();
    parent_stack.push_back(parameter);
    defer(parent_stack.pop_back());

    parameter->name = parse_identifier();
    if (parameter->name == nullptr) {
        report_error("Expected identifier for name of parameter!");
        return nullptr;
    }

    if (!token.is(':')) {
        report_error("Expected ':' after name of parameter!");
        return nullptr;
    }
    consume_token();

    parameter->type = parse_type();
    if (parameter->type == nullptr) {
        report_error("Expected type of parameter!");
        return nullptr;
    }

    return parameter;
}

// MARK: - Signatures

/// grammar: func-signature := "func" identifier "(" [ parameter { "," parameter } ] ")" "->" type-identifier
ASTFuncSignature* Parser::parse_func_signature() {
    ASTFuncSignature* signature = new (&context) ASTFuncSignature;
    signature->parent = parent_stack.back();
    parent_stack.push_back(signature);
    defer(parent_stack.pop_back());

    if (!token.is('(')) {
        report_error("Expected '(' in parameter list of function declaration!");
        return nullptr;
    }

    consume_token();

    if (!token.is(')')) {
        while (true) {
            ASTParameter* parameter = parse_parameter();
            if (parameter == nullptr) {
                return nullptr;
            }

            signature->parameters.push_back(parameter);

            if (token.is(')')) {
                break;
            } else if (!token.is(',')) {
                report_error("Expected ')' or ',' in parameter list of function declaration!");
                return nullptr;
            }

            consume_token();
        }
    }

    consume_token();

    if (!token.is(TOKEN_ARROW)) {
        report_error("Expected '->' in function declaration!");
        return nullptr;
    }

    consume_token();

    signature->return_type = parse_type();
    if (signature->return_type == nullptr) {
        report_error("Expected return type of function declaration!");
        return nullptr;
    }

    return signature;
};

// MARK: - Literals

/// grammar: literal := integer-literal | float-literal | string-literal | "true" | "false" | "nil"
ASTLiteral* Parser::parse_literal() {
    ASTLiteral* literal = new (&context) ASTLiteral;
    literal->parent = parent_stack.back();
    parent_stack.push_back(literal);
    defer(parent_stack.pop_back());

    literal->token_kind = token.kind;
    literal->flags |= AST_EXPRESSION_IS_CONSTANT;

    switch (token.kind) {
        case TOKEN_LITERAL_INT:
            if (!token.text.convert_to_int(literal->int_value)) {
                report_error("Invalid integer literal!");
                return nullptr;
            }

            literal->type = context.type_Int;
            break;

        case TOKEN_LITERAL_FLOAT:
            if (!token.text.convert_to_double(literal->float_value)) {
                report_error("Invalid floating point literal!");
                return nullptr;
            }

            literal->type = context.type_Float;
            break;

        case TOKEN_LITERAL_STRING:
            assert(token.text.buffer_length >= 2 && "Invalid length of string literal text, has to contain at least \"\"");
            literal->string_value = token.text.slice(1, token.text.buffer_length - 2);
            literal->string_value = String(literal->string_value.copy_buffer(), literal->string_value.buffer_length);
            literal->type = context.type_String;
            break;

        case TOKEN_KEYWORD_TRUE:
            literal->token_kind = TOKEN_LITERAL_BOOL;
            literal->bool_value = true;
            literal->type = context.type_Bool;
            break;

        case TOKEN_KEYWORD_FALSE:
            literal->token_kind = TOKEN_LITERAL_BOOL;
            literal->bool_value = false;
            literal->type = context.type_Bool;
            break;

        case TOKEN_KEYWORD_NIL:
            literal->token_kind = TOKEN_LITERAL_NIL;
            literal->type = context.type_AnyPointer;
            break;

        default:
            return nullptr;
    }
    consume_token();

    return literal;
}

// MARK: - Expressions

/// grammar: expression := binary-expression | unary-expression | atom-expression
/// grammar: binary-expression := ( atom-expression | unary-expression | call-expression | subscript-expression ) infix-operator expression
ASTExpression* Parser::parse_expression(uint32_t precedence) {
    ASTExpression* left = parse_primary_expression();
    if (left == nullptr) {
        return nullptr;
    }

    if (!lexer.get_operator(token, OPERATOR_INFIX, op) && !lexer.get_operator(token, OPERATOR_POSTFIX, op)) {
        return left;
    }

    // TODO: Maintain parent stack for binary expressions !!!
    while (precedence < op.precedence) {
        if (op.kind == OPERATOR_INFIX || op.text.is_equal(".")) {
            consume_token();

            uint32_t next_precedence = op.precedence;
            if (op.associativity == ASSOCIATIVITY_RIGHT) {
                next_precedence = lexer.get_operator_precedence_before(next_precedence);
            }

            ASTBinaryExpression* right = new (&context) ASTBinaryExpression;
            right->parent = parent_stack.back(); // TODO: Check if this is correct !!!
            right->op = op;
            right->op_identifier = new (&context) ASTIdentifier;
            right->op_identifier->type = new (&context) ASTOpaqueType(right->op_identifier);
            right->op_identifier->parent = right;
            right->op_identifier->lexeme = context.get_lexeme(op.text);
            right->type = right->op_identifier->type;
#ifdef DEBUG
            right->op_identifier->debug_lexeme_text = context.get_lexeme_text(right->op_identifier->lexeme);
#endif

            right->left = left;
            left->parent = right;

            right->right = parse_expression(next_precedence);
            if (right->right == nullptr) {
                return nullptr;
            }
            right->right->parent = right;

            left = right;
        } else if (op.text.is_equal("()")) {
            left = parse_call_expression(left);
        } else if (op.text.is_equal("[]")) {
            left = parse_subscript_expression(left);
        } else {
            return nullptr;
        }

        if (!lexer.get_operator(token, OPERATOR_INFIX, op) && !lexer.get_operator(token, OPERATOR_POSTFIX, op)) {
            break;
        }
    }

    return left;
}

/// grammar: call-expression := expression "(" [ expression { "," expression } ] ")"
ASTExpression* Parser::parse_call_expression(ASTExpression* left) {
    assert(token.is('(') && "Invalid token given for start of call-expression");
    consume_token();

    ASTCall* call = new (&context) ASTCall;
    call->type = context.type_unknown;
    call->parent = parent_stack.back();
    parent_stack.push_back(call);
    defer(parent_stack.pop_back());

    call->left = left;

    if (!token.is(')')) {
        while (true) {
            ASTExpression* argument = parse_expression();
            if (argument == nullptr) {
                return nullptr;
            }

            call->arguments.push_back(argument);

            if (token.is(')')) {
                break;
            } else if (!token.is(',')) {
                report_error("Expected ')' or ',' in argument list of call-expression!");
                return nullptr;
            }

            consume_token();
        }
    }
    consume_token();

    return call;
}

/// grammar: subscript-expression := expression "[" [ expression { "," expression } ] "]"
ASTExpression* Parser::parse_subscript_expression(ASTExpression* left) {
    assert(token.is('[') && "Invalid token given for start of call-expression");
    consume_token();

    ASTSubscript* subscript = new (&context) ASTSubscript;
    subscript->parent = parent_stack.back();
    parent_stack.push_back(subscript);
    defer(parent_stack.pop_back());

    subscript->left = left;

    if (!token.is(']')) {
        while (true) {
            ASTExpression* argument = parse_expression();
            if (argument == nullptr) {
                return nullptr;
            }

            subscript->arguments.push_back(argument);

            if (token.is(']')) {
                break;
            } else if (!token.is(',')) {
                report_error("Expected ']' or ',' in argument list of subscript-expression!");
                return nullptr;
            }

            consume_token();
        }
    }
    consume_token();

    return subscript;
}

/// grammar: primary-expression := unary-expression | atom-expression
ASTExpression* Parser::parse_primary_expression() {
    if (token.is(TOKEN_OPERATOR) && lexer.get_operator(token, OPERATOR_PREFIX, op)) {
        return parse_unary_expression();
    }

    return parse_atom_expression();
}

/// grammar: unary-expression := prefix-operator expression
ASTExpression* Parser::parse_unary_expression() {
    assert(token.is(TOKEN_OPERATOR) && lexer.get_operator(token, OPERATOR_PREFIX, op) && "Invalid token given for start of unary-expression!");
    consume_token();

    ASTUnaryExpression* expression = new (&context) ASTUnaryExpression;
    expression->parent = parent_stack.back();
    parent_stack.push_back(expression);
    defer(parent_stack.pop_back());

    expression->op = op;
    expression->op_identifier = new (&context) ASTIdentifier;
    expression->op_identifier->type = new (&context) ASTOpaqueType(expression->op_identifier);
    expression->op_identifier->parent = parent_stack.back();
    expression->op_identifier->lexeme = context.get_lexeme(op.text);

    expression->type = expression->op_identifier->type;

#ifdef DEBUG
    expression->op_identifier->debug_lexeme_text = context.get_lexeme_text(expression->op_identifier->lexeme);
#endif

    expression->right = parse_expression();

    if (expression->right == nullptr) {
        report_error("Expected expression after prefix operator!");
        return nullptr;
    }

// TODO: Add `sizeof`, `alignof`, ... expressions

    return expression;
}

/// grammar: atom-expression := group-expression | literal-expression | identifier-expression
/// grammar: literal-expression := literal
/// grammar: identifier-expression := identifier
ASTExpression* Parser::parse_atom_expression() {
    ASTExpression* expression = nullptr;

    switch (token.kind) {
        case '(':
            expression = parse_group_expression();
            break;

        case TOKEN_LITERAL_INT:
        case TOKEN_LITERAL_FLOAT:
        case TOKEN_LITERAL_STRING:
        case TOKEN_KEYWORD_TRUE:
        case TOKEN_KEYWORD_FALSE:
        case TOKEN_KEYWORD_NIL:
            expression = parse_literal();
            break;

        case TOKEN_IDENTIFIER:
            expression = parse_identifier();
            break;

        default:
            break;
    }

    return expression;
}

/// grammar: group-expression := "(" expression ")"
ASTExpression* Parser::parse_group_expression() {
    assert(token.is('(') && "Invalid token given for start of group expression!");
    consume_token();

    ASTExpression* expression = parse_expression();
    if (expression == nullptr) {
        return nullptr;
    }

    if (!token.is(')')) {
        report_error("Expected ')' at end of group expression!");
        return nullptr;
    }
    consume_token();

    return expression;
}

// MARK: - Statements

/// grammar: statement := variable-declaration | control-statement | defer-statement | do-statement | for-statement | guard-statement | if-statement | switch-statement | while-statement | expression
ASTStatement* Parser::parse_statement() {
    switch (token.kind) {
        case TOKEN_KEYWORD_VAR:
        case TOKEN_KEYWORD_LET:
            return parse_variable_declaration();

        case TOKEN_KEYWORD_BREAK:
        case TOKEN_KEYWORD_CONTINUE:
        case TOKEN_KEYWORD_FALLTHROUGH:
        case TOKEN_KEYWORD_RETURN:
            return parse_control_statement();

        case TOKEN_KEYWORD_DEFER:
            return parse_defer_statement();

        case TOKEN_KEYWORD_DO:
            return parse_do_statement();

        case TOKEN_KEYWORD_FOR:
            return parse_for_statement();

        case TOKEN_KEYWORD_GUARD:
            return parse_guard_statement();

        case TOKEN_KEYWORD_IF:
            return parse_if_statement();

        case TOKEN_KEYWORD_SWITCH:
            return parse_switch_statement();

        case TOKEN_KEYWORD_WHILE:
            return parse_while_statement();

        default:
            return parse_expression();
    }
}

/// grammar: control-statement := return-statement | "fallthrough" | "break" | "continue"
/// grammar: return-statement := "return" [ expression ]
ASTStatement* Parser::parse_control_statement() {
    ASTControl* control = new (&context) ASTControl;
    control->parent = parent_stack.back();
    parent_stack.push_back(control);
    defer(parent_stack.pop_back());

    switch (token.kind) {
        case TOKEN_KEYWORD_BREAK:
            control->control_kind = AST_CONTROL_BREAK;
            break;

        case TOKEN_KEYWORD_CONTINUE:
            control->control_kind = AST_CONTROL_CONTINUE;
            break;

        case TOKEN_KEYWORD_FALLTHROUGH:
            control->control_kind = AST_CONTROL_FALLTHROUGH;
            break;

        case TOKEN_KEYWORD_RETURN:
            control->control_kind = AST_CONTROL_RETURN;
            break;

        default:
            unreachable("Invalid token given for start of control-statement!");
    }

    consume_token();

    if (control->control_kind == AST_CONTROL_RETURN) {
// TODO: Control lexer state here with unwinding on failure!
        control->expression = parse_expression();
    }

    return control;
}

/// grammar: defer-statement := "defer" expression
ASTStatement* Parser::parse_defer_statement() {
    assert(token.is(TOKEN_KEYWORD_DEFER) && "Invalid token given for start of defer-statement!");
    consume_token();

    ASTDefer* defer = new (&context) ASTDefer;
    defer->parent = parent_stack.back();
    parent_stack.push_back(defer);
    defer(parent_stack.pop_back());

    defer->expression = parse_expression();
    if (defer->expression == nullptr) {
        return nullptr;
    }

    return defer;
}

/// grammar: do-statement := "do" block "while" expression
ASTStatement* Parser::parse_do_statement() {
    assert(token.is(TOKEN_KEYWORD_DO) && "Invalid token given for start of do-statement");
    consume_token();

    ASTLoop* stmt = new (&context) ASTLoop;
    stmt->pre_check_conditions = false;
    stmt->parent = parent_stack.back();
    parent_stack.push_back(stmt);
    defer(parent_stack.pop_back());

    stmt->block = parse_block();
    if (stmt->block == nullptr) {
        return nullptr;
    }

    if (!token.is(TOKEN_KEYWORD_WHILE)) {
        report_error("Expected keyword 'while' after do block!");
        return nullptr;
    }
    consume_token();

    do {

        ASTExpression* condition = parse_expression();
        if (condition == nullptr) {
            return nullptr;
        }

        stmt->conditions.push_back(condition);

        if (!token.is(',')) {
            break;
        }
        consume_token();

    } while (true);

    return stmt;
}

/// grammar: for-statement := "for" identifier "in" expression block
ASTStatement* Parser::parse_for_statement() {
    assert(token.is(TOKEN_KEYWORD_FOR) && "Invalid token given for start of for-statement");
    consume_token();

    ASTFor* stmt = new (&context) ASTFor;
    stmt->parent = parent_stack.back();
    parent_stack.push_back(stmt);
    defer(parent_stack.pop_back());

    stmt->iterator = parse_identifier();
    if (stmt->iterator == nullptr) {
        return nullptr;
    }

    if (!token.is(TOKEN_KEYWORD_IN)) {
        report_error("Expected keyword in after for iterator");
        return nullptr;
    }
    consume_token();

    stmt->sequence = parse_expression();
    if (stmt->sequence == nullptr) {
        report_error("Expected expression for iterable sequence in for-statement");
        return nullptr;
    }

    stmt->block = parse_block();
    if (stmt->block == nullptr) {
        report_error("Expected iteration block in for-statement");
        return nullptr;
    }

    // TODO: Add iterator of for-statement as declaration into the scope !!!
    //       The current grammar of the for-statement will at least require minimalistic
    //       type inference to be declared correctly because the iterator and sequence
    //       are not explicitly typed ...

    return stmt;
}

/// grammar: guard-statement := "guard" expression { "," expression } else block
ASTStatement* Parser::parse_guard_statement() {
    assert(token.is(TOKEN_KEYWORD_GUARD) && "Invalid token given for start of guard-statement");
    consume_token();

    ASTGuard* guard = new (&context) ASTGuard;
    guard->parent = parent_stack.back();
    parent_stack.push_back(guard);
    defer(parent_stack.pop_back());

    do {

        ASTExpression* expression = parse_expression();
        if (expression == nullptr) {
            return nullptr;
        }

        guard->conditions.push_back(expression);

        if (!token.is(',')) {
            break;
        }
        consume_token();

    } while (true);

    if (!token.is(TOKEN_KEYWORD_ELSE)) {
        report_error("Expected keyword 'else' in guard-statement");
        return nullptr;
    }
    consume_token();

    guard->else_block = parse_block();
    if (guard->else_block == nullptr) {
        return nullptr;
    }

    // TODO: This check will not work because if-statements and other could encapsulate return-statements
    //       which is valid but isn't handled in this iteration ! There must be a more in depth checker
    //       because all branches in else-block must return !

//    bool has_return = false;
//    for (auto it = guard->else_block->statements.begin(); it != guard->else_block->statements.end(); it++) {
//        if ((*it)->kind == AST_CONTROL) {
//            ASTControl* control = reinterpret_cast<ASTControl*>(*it);
//            if (control->token_kind == TOKEN_KEYWORD_RETURN) {
//                has_return = true;
//            }
//        }
//    }
//
//    if (!has_return) {
//        report_error("Expected return-statement in else-block of guard-statement");
//        return nullptr;
//    }

    return guard;
}

/// grammar: if-statement := "if" expression { "," expression } block [ "else" ( if-statement | block ) ]
ASTStatement* Parser::parse_if_statement() {
    assert(token.is(TOKEN_KEYWORD_IF) && "Invalid token given for start of if-statement!");
    consume_token();

    ASTIf* stmt = new (&context) ASTIf;
    stmt->parent = parent_stack.back();
    parent_stack.push_back(stmt);
    defer(parent_stack.pop_back());

    do {

        ASTExpression* condition = parse_expression();
        if (condition == nullptr) {
            return nullptr;
        }

        stmt->conditions.push_back(condition);

        if (!token.is(',')) {
            break;
        }
        consume_token();

    } while (true);

    stmt->block = parse_block();
    if (stmt->block == nullptr) {
        return nullptr;
    }

    if (token.is(TOKEN_KEYWORD_ELSE)) {
        consume_token();

        if (token.is(TOKEN_KEYWORD_IF)) {
            stmt->if_kind = AST_IF_ELSE_IF;
            stmt->else_if = reinterpret_cast<ASTIf*>(parse_if_statement());
            if (stmt->else_if == nullptr) {
                return nullptr;
            }
        } else {
            stmt->if_kind = AST_IF_ELSE;
            stmt->else_block = parse_block();
            if (stmt->else_block == nullptr) {
                return nullptr;
            }
        }
    }

    return stmt;
}

/// grammar: switch-statement := "switch" expression "{" [ switch-case { line-break switch-case } ] "}"
ASTStatement* Parser::parse_switch_statement() {
    assert(token.is(TOKEN_KEYWORD_SWITCH) && "Invalid token given for start of switch-statement!");
    consume_token();

    ASTSwitch* stmt = new (&context) ASTSwitch;
    stmt->parent = parent_stack.back();
    parent_stack.push_back(stmt);
    defer(parent_stack.pop_back());

    stmt->expression = parse_expression();
    if (stmt->expression == nullptr) {
        return nullptr;
    }

    if (!token.is('{')) {
        report_error("Expected '{' after expression in switch-statement!");
        return nullptr;
    }
    consume_token();

    do {

        ASTSwitchCase* switch_case = parse_switch_case();
        if (switch_case == nullptr) {
            report_error("Expected switch-case in body of switch-statement!");
            return nullptr;
        }

        stmt->cases.push_back(switch_case);

        if (token.is('}')) {
            break;
        }

    } while (true);

    consume_token();

    return stmt;
}

/// grammar: while-statement := "while" expression { "," expression } block
ASTStatement* Parser::parse_while_statement() {
    assert(token.is(TOKEN_KEYWORD_WHILE) && "Invalid token given for start of while-statement!");
    consume_token();

    ASTLoop* stmt = new (&context) ASTLoop;
    stmt->pre_check_conditions = true;
    stmt->parent = parent_stack.back();
    parent_stack.push_back(stmt);
    defer(parent_stack.pop_back());

    do {

        ASTExpression* condition = parse_expression();
        if (condition == nullptr) {
            return nullptr;
        }

        stmt->conditions.push_back(condition);

        if (!token.is(',')) {
            break;
        }
        consume_token();

    } while (true);

    stmt->block = parse_block();
    if (stmt->block == nullptr) {
        return nullptr;
    }

    return stmt;
}

// MARK: - Block

/// grammar: block := "{" { statement } "}"
ASTBlock* Parser::parse_block() {
    if (!token.is('{')) {
        report_error("Expected '{' at start of block!");
        return nullptr;
    }
    consume_token();

    ASTBlock* block = new (&context) ASTBlock;
    block->parent = parent_stack.back();
    parent_stack.push_back(block);
    defer(parent_stack.pop_back());

    if (!token.is('}')) {
        while (true) {
            ASTStatement* statement = parse_statement();
            if (statement == nullptr) {
                return nullptr;
            }

            block->statements.push_back(statement);

            if (token.is('}')) {
                break;
            }
        }
    }
    consume_token();

    return block;
}

// MARK: - Identifiers

/// grammar: identifier := identifier-head { identifier-tail }
/// grammar: identifier-head := "a" ... "z" | "A" ... "Z" | "_"
/// grammar: identifier-tail := identifier-head | "0" ... "9"
ASTIdentifier* Parser::parse_identifier() {
    if (!token.is(TOKEN_IDENTIFIER)) {
        return nullptr;
    }

    ASTIdentifier* identifier = new (&context) ASTIdentifier;
    identifier->type = new (&context) ASTOpaqueType(identifier);
    identifier->parent = parent_stack.back();
    parent_stack.push_back(identifier);
    defer(parent_stack.pop_back());

    identifier->lexeme = context.get_lexeme(token.text);
    consume_token();

#ifdef DEBUG
    identifier->debug_lexeme_text = context.get_lexeme_text(identifier->lexeme);
#endif

    return identifier;
}

// MARK: - Types

/// grammar: type-identifier := identifier | any-type-identifier | pointer-type-identifier | array-type-identifier | type-of-type-identifier
/// grammar: any-type-identifier := "Any"
/// grammar: pointer-type-identifier := type-identifier "*"
/// grammar: array-type-identifier := type-identifier "[" [ expression ] "]"
/// grammar: type-of-type-identifier := "typeof" "(" expression ")"
ASTType* Parser::parse_type() {
    ASTType* type = nullptr;
//    type->parent = parent_stack.back();
//    parent_stack.push_back(type);

    switch (token.kind) {
        case TOKEN_KEYWORD_ANY:
            consume_token();
            type = context.type_Any;
            break;

        case TOKEN_IDENTIFIER: {
            auto identifier = parse_identifier();
            if (identifier == nullptr) {
                return nullptr;
            }

            type = new (&context) ASTOpaqueType(identifier);
        }   break;

        case TOKEN_KEYWORD_TYPEOF: {
            consume_token();

            if (!token.is('(')) {
                report_error("Expected ( after typeof keyword!");
                return nullptr;
            }
            consume_token();

            auto expr = parse_expression();
            if (expr == nullptr) {
                return nullptr;
            }

            type = new (&context) ASTTypeOfType(expr);

            if (!token.is(')')) {
                report_error("Expected ) after expression of typeof keyword!");
                return nullptr;
            }
            consume_token();
        }   break;

        default:
            report_error("Expected type identifier!");
            return nullptr;
    }

    bool finished = false;
    while (!finished) {
        switch (token.kind) {
            case TOKEN_OPERATOR: {
                if (!lexer.get_operator(token, OPERATOR_POSTFIX, op) || !op.text.is_equal("*")) {
                    finished = true;
                    break;
                }
                consume_token();

                uint32_t pointer_depth = 1;
                while (token.is(TOKEN_OPERATOR) && lexer.get_operator(token, OPERATOR_POSTFIX, op) && op.text.is_equal("*")) {
                    pointer_depth += 1;
                    consume_token();
                }

                type = new (&context) ASTPointerType(pointer_depth, type);
            }   break;

            case '[': {
                consume_token();

                if (token.is(']')) {
                    type = new (&context) ASTArrayType(false, nullptr, type);
                } else {
                    auto expr = parse_expression();
                    if (expr == nullptr) {
                        return nullptr;
                    }

                    // TODO: Check if expr is constant and assign is_static in a later phase !!!
                    type = new (&context) ASTArrayType(true, expr, type);
                }

                if (!token.is(']')) {
                    report_error("Expected ] after expression of array-type-identifier!");
                    return nullptr;
                }
                consume_token();
            }   break;

            default:
                finished = true;
                break;
        }
    }

    return type;
}

// MARK: - Helpers

/// grammar: switch-case := ( "case" expression | "else" ) ":" statement { line-break statement }
ASTSwitchCase* Parser::parse_switch_case() {
    if (!token.is(TOKEN_KEYWORD_CASE, TOKEN_KEYWORD_ELSE)) {
        return nullptr;
    }

    ASTSwitchCase* switch_case = new (&context) ASTSwitchCase;
    switch_case->parent = parent_stack.back();
    parent_stack.push_back(switch_case);
    defer(parent_stack.pop_back());

    if (token.is(TOKEN_KEYWORD_CASE)) {
        consume_token();
        switch_case->case_kind = AST_SWITCH_CASE_CONDITION;
        switch_case->condition = parse_expression();
        if (switch_case->condition == nullptr) {
            return nullptr;
        }
    } else {
        consume_token();
        switch_case->case_kind = AST_SWITCH_CASE_ELSE;
    }

    if (!token.is(':')) {
        report_error("Expected ':' in switch-case statement!");
        return nullptr;
    }
    consume_token();

    switch_case->block = new (&context) ASTBlock;
    switch_case->block->parent = parent_stack.back();
    {
        parent_stack.push_back(switch_case->block);
        defer(parent_stack.pop_back());

        do {

            ASTStatement* statement = parse_statement();
            if (statement == nullptr) {
                return nullptr;
            }

            switch_case->block->statements.push_back(statement);

            if (token.is(TOKEN_KEYWORD_CASE, TOKEN_KEYWORD_ELSE, '}')) {
                break;
            }

        } while (true);
    }

    return switch_case;
}
