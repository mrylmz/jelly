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
#include <string>

#define PRINT_ARRAY(__ARRAY__)                                   \
print_raw("[\n");                                                \
indentation_level += 1;                                          \
for (auto it = __ARRAY__.begin(); it != __ARRAY__.end(); it++) { \
    print_indentation();                                         \
    visit(*it);                                                  \
                                                                 \
    if (it != __ARRAY__.end() - 1) {                             \
        print_raw("\n");                                         \
    }                                                            \
}                                                                \
indentation_level -= 1;                                          \
print_raw("\n");                                                 \
print_indentation();                                             \
print_raw("]");

void ASTPrinter::print(const ASTContext& context) {
    current_context = &context;

    print_raw("(\n");
    indentation_level += 1;
    print_indentation();
    print_raw("ROOT = ");
    visit(reinterpret_cast<const ASTNode*>(context.root));
    indentation_level -= 1;
    print_raw("\n)\n");
}

void ASTPrinter::visit(const ASTNode *node) {
    if (node == nullptr) {
        print_raw("NULL()");
        return;
    }

    ASTVisitor::visit(node);
}

void ASTPrinter::visit(const ASTLoad* node) {
    print_kind(node);
    print_raw("(\n");
    indentation_level += 1;

    print_indentation();
    print_raw("LITERAL = ");
    visit(node->literal);

    indentation_level -= 1;
    print_raw("\n");
    print_indentation();
    print_raw(")");
}

void ASTPrinter::visit(const ASTLiteral* node) {
    print_kind(node);
    print_raw("(\n");
    indentation_level += 1;

    print_indentation();
    print_raw("VALUE = ");
    switch (node->token_kind) {
        case TOKEN_LITERAL_NIL:
            print_raw("NIL()");
            break;

        case TOKEN_LITERAL_BOOL:
            print_raw("BOOL(");
            if (node->bool_value) {
                print_raw("TRUE");
            } else {
                print_raw("FALSE");
            }
            print_raw(")");
            break;

        case TOKEN_LITERAL_INT:
            print_raw("INT(");
            print_raw(node->int_value);
            print_raw(")");
            break;

        case TOKEN_LITERAL_FLOAT:
            print_raw("FLOAT(");
            print_raw(node->float_value);
            print_raw(")");
            break;

        case TOKEN_LITERAL_STRING:
            print_raw("STRING(");
            print_raw(node->string_value);
            print_raw(")");
            break;
    }

    indentation_level -= 1;
    print_raw("\n");
    print_indentation();
    print_raw(")");
}

void ASTPrinter::visit(const ASTFunc* node) {
    print_kind(node);
    print_raw("(\n");
    indentation_level += 1;

    print_indentation();
    print_raw("SIGNATURE = ");
    visit(node->signature);
    print_raw("\n");

    print_indentation();
    print_raw("BLOCK = ");
    visit(node->block);

    indentation_level -= 1;
    print_raw("\n");
    print_indentation();
    print_raw(")");
}

void ASTPrinter::visit(const ASTFuncSignature* node) {
    print_kind(node);
    print_raw("(\n");
    indentation_level += 1;

    print_indentation();
    print_raw("NAME = ");
    visit(node->name);
    print_raw("\n");

    print_indentation();
    print_raw("PARAMETERS = ");
    PRINT_ARRAY(node->parameters);
    print_raw("\n");

    print_indentation();
    print_raw("RETURN_TYPE = ");
    visit(node->return_type);

    indentation_level -= 1;
    print_raw("\n");
    print_indentation();
    print_raw(")");
}

void ASTPrinter::visit(const ASTBlock* node) {
    print_kind(node);
    print_raw("(\n");
    indentation_level += 1;

    print_indentation();
    print_raw("STATEMENTS = ");
    PRINT_ARRAY(node->statements);

    indentation_level -= 1;
    print_raw("\n");
    print_indentation();
    print_raw(")");
}

void ASTPrinter::visit(const ASTParameter* node) {
    print_kind(node);
    print_raw("(\n");
    indentation_level += 1;

    print_indentation();
    print_raw("NAME = ");
    visit(reinterpret_cast<const ASTNode*>(node->name));
    print_raw("\n");

    print_indentation();
    print_raw("TYPE = ");
    visit(reinterpret_cast<const ASTNode*>(node->type));

    indentation_level -= 1;
    print_raw("\n");
    print_indentation();
    print_raw(")");
}

void ASTPrinter::visit(const ASTStruct* node) {
    print_kind(node);
    print_raw("(\n");
    indentation_level += 1;

    print_indentation();
    print_raw("NAME = ");
    visit(reinterpret_cast<const ASTNode*>(node->name));
    print_raw("\n");

    print_indentation();
    print_raw("BLOCK = ");
    visit(reinterpret_cast<const ASTNode*>(node->block));

    indentation_level -= 1;
    print_raw("\n");
    print_indentation();
    print_raw(")");
}

void ASTPrinter::visit(const ASTVariable* node) {
    print_kind(node);
    print_raw("(\n");
    indentation_level += 1;

    print_indentation();
    print_raw("NAME = ");
    visit(node->name);
    print_raw("\n");

    print_indentation();
    print_raw("IS_CONSTANT = ");
    if (node->flags & AST_VARIABLE_IS_CONSTANT) {
        print_raw("TRUE");
    } else {
        print_raw("FALSE");
    }
    print_raw("\n");

    print_indentation();
    print_raw("TYPE = ");
    visit(node->type);
    print_raw("\n");

    print_indentation();
    print_raw("ASSIGNMENT = ");
    visit(node->assignment);

    indentation_level -= 1;
    print_raw("\n");
    print_indentation();
    print_raw(")");
}

void ASTPrinter::visit(const ASTEnum* node) {
    print_kind(node);
    print_raw("(\n");
    indentation_level += 1;

    print_indentation();
    print_raw("NAME = ");
    visit(reinterpret_cast<const ASTNode*>(node->name));
    print_raw("\n");

    print_indentation();
    print_raw("ELEMENTS = ");
    PRINT_ARRAY(node->elements);

    indentation_level -= 1;
    print_raw("\n");
    print_indentation();
    print_raw(")");
}

void ASTPrinter::visit(const ASTEnumElement* node) {
    print_kind(node);
    print_raw("(\n");
    indentation_level += 1;

    print_indentation();
    print_raw("NAME = ");
    visit(reinterpret_cast<const ASTNode*>(node->name));
    print_raw("\n");

    print_indentation();
    print_raw("ASSIGNMENT = ");
    visit(reinterpret_cast<const ASTNode*>(node->assignment));

    indentation_level -= 1;
    print_raw("\n");
    print_indentation();
    print_raw(")");
}

void ASTPrinter::visit(const ASTIdentifier* node) {
    print_kind(node);
    print_raw("(\n");
    indentation_level += 1;

    print_indentation();
    print_raw("TEXT = STRING(");
    print_raw(current_context->get_lexeme_text(node->lexeme));
    print_raw(")");

    indentation_level -= 1;
    print_raw("\n");
    print_indentation();
    print_raw(")");
}

void ASTPrinter::visit(const ASTUnaryExpression* node) {
    print_kind(node);
    print_raw("(\n");
    indentation_level += 1;

    print_indentation();
    print_raw("OPERATOR = ");
    print_raw(node->op.text);
    print_raw("\n");

    print_indentation();
    print_raw("RIGHT = ");
    visit(reinterpret_cast<const ASTNode*>(node->right));

    indentation_level -= 1;
    print_raw("\n");
    print_indentation();
    print_raw(")");
}

void ASTPrinter::visit(const ASTBinaryExpression* node) {
    print_kind(node);
    print_raw("(\n");
    indentation_level += 1;

    print_indentation();
    print_raw("OPERATOR = ");
    print_raw(node->op.text);
    print_raw("\n");

    print_indentation();
    print_raw("LEFT = ");
    visit(reinterpret_cast<const ASTNode*>(node->left));
    print_raw("\n");

    print_indentation();
    print_raw("RIGHT = ");
    visit(reinterpret_cast<const ASTNode*>(node->right));

    indentation_level -= 1;
    print_raw("\n");
    print_indentation();
    print_raw(")");
}

void ASTPrinter::visit(const ASTControl* node) {
    print_kind(node);
    print_raw("(\n");
    indentation_level += 1;

    print_indentation();
    switch (node->token_kind) {
        case TOKEN_KEYWORD_BREAK:
            print_raw("KIND = BREAK()");
            break;

        case TOKEN_KEYWORD_CONTINUE:
            print_raw("KIND = CONTINUE()");
            break;

        case TOKEN_KEYWORD_FALLTHROUGH:
            print_raw("KIND = FALLTHROUGH()");
            break;

        case TOKEN_KEYWORD_RETURN:
            print_raw("KIND = RETURN(\n");
            indentation_level += 1;
            print_indentation();
            visit(reinterpret_cast<const ASTNode*>(node->expression));
            indentation_level -= 1;
            print_raw("\n");
            print_indentation();
            print_raw(")");
            break;

        default:
            break;
    }

    indentation_level -= 1;
    print_raw("\n");
    print_indentation();
    print_raw(")");
}

void ASTPrinter::visit(const ASTType* node) {
    print_kind(node);
    print_raw("(\n");
    indentation_level += 1;

    print_indentation();
    switch (node->type_kind) {
        case AST_TYPE_UNKNOWN:
            print_raw("KIND = UNKNOWN()");
            break;

        case AST_TYPE_IDENTIFIER:
            print_raw("KIND = ");
            visit(reinterpret_cast<const ASTNode*>(node->identifier));
            break;

        case AST_TYPE_ANY:
            print_raw("KIND = ANY()");
            break;

        case AST_TYPE_TYPEOF:
            print_raw("KIND = TYPEOF(\n");
            indentation_level += 1;

            print_indentation();
            visit(reinterpret_cast<const ASTNode*>(node->expression));
            print_raw("\n");

            indentation_level -= 1;
            print_indentation();
            print_raw(")");
            break;

        case AST_TYPE_POINTER:
            print_raw("KIND = POINTER(\n");
            indentation_level += 1;

            print_indentation();
            visit(reinterpret_cast<const ASTNode*>(node->type));
            print_raw("\n");

            indentation_level -= 1;
            print_indentation();
            print_raw(")");
            break;

        case AST_TYPE_ARRAY:
            print_raw("KIND = ARRAY(\n");
            indentation_level += 1;

            print_indentation();
            print_raw("TYPE = ");
            visit(reinterpret_cast<const ASTNode*>(node->type));
            print_raw("\n");

            print_indentation();
            print_raw("SIZE = ");
            visit(reinterpret_cast<const ASTNode*>(node->expression));
            print_raw("\n");

            indentation_level -= 1;
            print_indentation();
            print_raw(")");
            break;

        default:
            break;
    }

    indentation_level -= 1;
    print_raw("\n");
    print_indentation();
    print_raw(")");
}

void ASTPrinter::visit(const ASTDefer* node) {
    print_kind(node);
    print_raw("(\n");
    indentation_level += 1;

    print_indentation();
    print_raw("EXPRESSION = ");
    visit(reinterpret_cast<const ASTNode*>(node->expression));

    indentation_level -= 1;
    print_raw("\n");
    print_indentation();
    print_raw(")");
}

void ASTPrinter::visit(const ASTDo* node) {
    print_kind(node);
    print_raw("(\n");
    indentation_level += 1;

    print_indentation();
    print_raw("BLOCK = ");
    visit(reinterpret_cast<ASTNode*>(node->block));
    print_raw("\n");

    print_indentation();
    print_raw("CONDITIONS = ");
    PRINT_ARRAY(node->conditions);

    indentation_level -= 1;
    print_raw("\n");
    print_indentation();
    print_raw(")");
}

void ASTPrinter::visit(const ASTFor* node) {
    print_kind(node);
    print_raw("(\n");
    indentation_level += 1;

    print_indentation();
    print_raw("ITERATOR = ");
    visit(reinterpret_cast<const ASTNode*>(node->iterator));
    print_raw("\n");

    print_indentation();
    print_raw("SEQUENCE = ");
    visit(reinterpret_cast<const ASTNode*>(node->sequence));
    print_raw("\n");

    print_indentation();
    print_raw("BLOCK = ");
    visit(reinterpret_cast<const ASTNode*>(node->block));

    indentation_level -= 1;
    print_raw("\n");
    print_indentation();
    print_raw(")");
}

void ASTPrinter::visit(const ASTGuard* node) {
    print_kind(node);
    print_raw("(\n");
    indentation_level += 1;

    print_indentation();
    print_raw("CONDITIONS = ");
    PRINT_ARRAY(node->conditions);
    print_raw("\n");

    print_indentation();
    print_raw("ELSE_BLOCK = ");
    visit(reinterpret_cast<const ASTNode*>(node->else_block));

    indentation_level -= 1;
    print_raw("\n");
    print_indentation();
    print_raw(")");
}

void ASTPrinter::visit(const ASTIf* node) {
    print_kind(node);
    print_raw("(\n");
    indentation_level += 1;

    print_indentation();
    print_raw("CONDITIONS = ");
    PRINT_ARRAY(node->conditions);
    print_raw("\n");

    print_indentation();
    print_raw("BLOCK = ");
    visit(reinterpret_cast<const ASTNode*>(node->block));
    print_raw("\n");

    print_indentation();
    print_raw("ELSE = ");
    switch (node->if_kind) {
        case AST_IF_SINGLE:
            print_raw("NULL()");
            break;

        case AST_IF_ELSE:
            visit(reinterpret_cast<const ASTNode*>(node->else_block));
            break;

        case AST_IF_ELSE_IF:
            visit(reinterpret_cast<const ASTNode*>(node->else_if));
            break;

        default:
            print_raw("UNSPECIFIED()");
            break;
    }

    indentation_level -= 1;
    print_raw("\n");
    print_indentation();
    print_raw(")");
}

void ASTPrinter::visit(const ASTSwitch* node) {
    print_kind(node);
    print_raw("(\n");
    indentation_level += 1;

    print_indentation();
    print_raw("EXPRESSION = ");
    visit(reinterpret_cast<const ASTNode*>(node->expression));
    print_raw("\n");

    print_indentation();
    print_raw("CASES = ");
    PRINT_ARRAY(node->cases);

    indentation_level -= 1;
    print_raw("\n");
    print_indentation();
    print_raw(")");
}

void ASTPrinter::visit(const ASTSwitchCase* node) {
    print_kind(node);
    print_raw("(\n");
    indentation_level += 1;

    print_indentation();
    print_raw("KIND = ");
    switch (node->case_kind) {
        case AST_SWITCH_CASE_CONDITION:
            print_raw("CASE(\n");
            indentation_level += 1;
            print_indentation();
            print_raw("CONDITION = ");
            visit(reinterpret_cast<const ASTNode*>(node->condition));
            indentation_level -= 1;
            print_raw("\n");
            print_indentation();
            print_raw(")");
            break;

        case AST_SWITCH_CASE_ELSE:
            print_raw("ELSE()");
            break;

        default:
            break;
    }
    print_raw("\n");

    print_indentation();
    print_raw("STATEMENTS = ");
    PRINT_ARRAY(node->statements);

    indentation_level -= 1;
    print_raw("\n");
    print_indentation();
    print_raw(")");
}

void ASTPrinter::visit(const ASTWhile* node) {
    print_kind(node);
    print_raw("(\n");
    indentation_level += 1;

    print_indentation();
    print_raw("CONDITIONS = ");
    PRINT_ARRAY(node->conditions);
    print_raw("\n");

    print_indentation();
    print_raw("BLOCK = ");
    visit(reinterpret_cast<const ASTNode*>(node->block));

    indentation_level -= 1;
    print_raw("\n");
    print_indentation();
    print_raw(")");
}

void ASTPrinter::visit(const ASTCall* node) {
    print_kind(node);
    print_raw("(\n");
    indentation_level += 1;

    print_indentation();
    print_raw("LEFT = ");
    visit(reinterpret_cast<const ASTNode*>(node->left));
    print_raw("\n");

    print_indentation();
    print_raw("ARGUMENTS = ");
    PRINT_ARRAY(node->arguments);

    indentation_level -= 1;
    print_raw("\n");
    print_indentation();
    print_raw(")");
}

void ASTPrinter::visit(const ASTSubscript* node) {
    print_kind(node);
    print_raw("(\n");
    indentation_level += 1;

    print_indentation();
    print_raw("LEFT = ");
    visit(reinterpret_cast<const ASTNode*>(node->left));
    print_raw("\n");

    print_indentation();
    print_raw("ARGUMENTS = ");
    PRINT_ARRAY(node->arguments);

    indentation_level -= 1;
    print_raw("\n");
    print_indentation();
    print_raw(")");
}

void ASTPrinter::print_kind(const ASTNode* node) {
    switch (node->kind) {
        case AST_UNKNOWN:
            return print_raw("AST_UNKNOWN");

        case AST_LOAD:
            return print_raw("AST_LOAD");

        case AST_LITERAL:
            return print_raw("AST_LITERAL");

        case AST_FUNC:
            return print_raw("AST_FUNC");

        case AST_FUNC_SIGNATURE:
            return print_raw("AST_FUNC_SIGNATURE");

        case AST_BLOCK:
            return print_raw("AST_BLOCK");

        case AST_PARAMETER:
            return print_raw("AST_PARAMETER");

        case AST_STRUCT:
            return print_raw("AST_STRUCT");

        case AST_VARIABLE:
            return print_raw("AST_VARIABLE");

        case AST_ENUM:
            return print_raw("AST_ENUM");

        case AST_ENUM_ELEMENT:
            return print_raw("AST_ENUM_ELEMENT");

        case AST_IDENTIFIER:
            return print_raw("AST_IDENTIFIER");

        case AST_UNARY:
            return print_raw("AST_UNARY");

        case AST_BINARY:
            return print_raw("AST_BINARY");

        case AST_CONTROL:
            return print_raw("AST_CONTROL");

        case AST_TYPE:
            return print_raw("AST_TYPE");

        case AST_DEFER:
            return print_raw("AST_DEFER");

        case AST_DO:
            return print_raw("AST_DO");

        case AST_FOR:
            return print_raw("AST_FOR");

        case AST_GUARD:
            return print_raw("AST_GUARD");

        case AST_IF:
            return print_raw("AST_IF");

        case AST_SWITCH:
            return print_raw("AST_SWITCH");

        case AST_SWITCH_CASE:
            return print_raw("AST_SWITCH_CASE");

        case AST_WHILE:
            return print_raw("AST_WHILE");

        case AST_CALL:
            return print_raw("AST_CALL");

        case AST_SUBSCRIPT:
            return print_raw("AST_SUBSCRIPT");

        default:
            return print_raw("UNSPECIFIED");
    }
}

void ASTPrinter::print_indentation() {
    for (uint32_t i = 0; i < indentation_level * 4; i++) {
        output_stream << ' ';
    }
}

void ASTPrinter::print_raw(String string) {
    std::string copy(string.buffer_start, string.buffer_length);
    output_stream << copy;
}

void ASTPrinter::print_raw(uint64_t value) {
    output_stream << value;
}

void ASTPrinter::print_raw(double value) {
    output_stream << value;
}

#undef PRINT_ARRAY
