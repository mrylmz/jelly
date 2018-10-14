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

#include <gtest/gtest.h>
#include <AST/AST.h>
#include <Parse/Parse.h>

#define EXPECT_AST_IDENTIFIER_TEXT_EQ(__AST__,__STRING__)   \
EXPECT_NE(__AST__, nullptr);                                \
if (__AST__ != nullptr) {                                   \
    EXPECT_TRUE(__AST__->text.is_equal(__STRING__));        \
}

TEST(Parser, parse_load_directive$valid_literal) {
    Lexer    lexer("#load \"path/to/Source.jelly\"");
    Parser   parser(lexer);

    ASTNode* node = parser.parse();
    EXPECT_NE(node, nullptr);

    if (node != nullptr) {
        EXPECT_EQ(node->kind, AST_LOAD);
        ASTLoad* load = reinterpret_cast<ASTLoad*>(node);
        EXPECT_NE(load->literal, nullptr);

        if (load->literal != nullptr) {
            EXPECT_EQ(load->literal->token_kind, TOKEN_LITERAL_STRING);
            EXPECT_TRUE(load->literal->string_value.is_equal("path/to/Source.jelly"));
        }
    }
}

TEST(Parser, parse_load_directive$invalid_literal) {
    Lexer    lexer("#load 1389123");
    Parser   parser(lexer);

    ASTNode* node = parser.parse();
    EXPECT_EQ(node, nullptr);
}

TEST(Parser, parse_func_decl$missing_name) {
    Lexer    lexer("func () -> Void {}");
    Parser   parser(lexer);

    ASTNode* node = parser.parse();
    EXPECT_EQ(node, nullptr);
}

TEST(Parser, parse_func_decl$missing_opening_parenthesis) {
    Lexer    lexer("func name) -> Void {}");
    Parser   parser(lexer);

    ASTNode* node = parser.parse();
    EXPECT_EQ(node, nullptr);
}

TEST(Parser, parse_func_decl$missing_parameter) {
    Lexer    lexer("func name( -> Void {}");
    Parser   parser(lexer);

    ASTNode* node = parser.parse();
    EXPECT_EQ(node, nullptr);
}

TEST(Parser, parse_func_decl$missing_closing_parenthesis) {
    Lexer    lexer("func name(x: Int -> Void {}");
    Parser   parser(lexer);

    ASTNode* node = parser.parse();
    EXPECT_EQ(node, nullptr);
}

TEST(Parser, parse_func_decl$missing_parameter_delimiter) {
    Lexer    lexer("func name(x: Int y: Int) -> Void {}");
    Parser   parser(lexer);

    ASTNode* node = parser.parse();
    EXPECT_EQ(node, nullptr);
}

TEST(Parser, parse_func_decl$missing_arrow) {
    Lexer    lexer("func name(x: Int, y: Int) Void {}");
    Parser   parser(lexer);

    ASTNode* node = parser.parse();
    EXPECT_EQ(node, nullptr);
}

TEST(Parser, parse_func_decl$missing_return_type) {
    Lexer    lexer("func name(x: Int, y: Int) -> {}");
    Parser   parser(lexer);

    ASTNode* node = parser.parse();
    EXPECT_EQ(node, nullptr);
}

TEST(Parser, parse_func_decl$missing_block) {
    Lexer    lexer("func name(x: Int, y: Int) -> Void");
    Parser   parser(lexer);

    ASTNode* node = parser.parse();
    EXPECT_EQ(node, nullptr);
}

TEST(Parser, parse_func_decl$missing_block_closing_parenthesis) {
    Lexer    lexer("func name(x: Int, y: Int) -> Void {");
    Parser   parser(lexer);

    ASTNode* node = parser.parse();
    EXPECT_EQ(node, nullptr);
}

TEST(Parser, parse_func_declaration$missing_colon_in_parameter) {
    const char* source = "func main(x Int) -> Void {}";
    Lexer       lexer(source);
    Parser      parser(lexer);

    ASTNode* node = parser.parse();
    EXPECT_EQ(node, nullptr);
}

TEST(Parser, parse_func_declaration$missing_type_name_in_parameter) {
    const char* source = "func main(x:, y: Int) -> Void {}";
    Lexer       lexer(source);
    Parser      parser(lexer);

    ASTNode* node = parser.parse();
    EXPECT_EQ(node, nullptr);
}

TEST(Parser, parse_func_decl$parameters_none) {
    const char* source = "func main() -> Void {}";
    Lexer       lexer(source);
    Parser      parser(lexer);

    ASTNode* node = parser.parse();
    EXPECT_NE(node, nullptr);

    if (node != nullptr) {
        EXPECT_EQ(node->kind, AST_FUNC);
        ASTFunc* func = reinterpret_cast<ASTFunc*>(node);
        EXPECT_NE(func->signature, nullptr);

        if (func->signature != nullptr) {
            EXPECT_AST_IDENTIFIER_TEXT_EQ(func->signature->name, "main");
            EXPECT_TRUE(func->signature->parameters.empty());
            EXPECT_AST_IDENTIFIER_TEXT_EQ(func->signature->return_type_name, "Void");
            EXPECT_NE(func->block, nullptr);

            if (func->block != nullptr) {
                EXPECT_TRUE(func->block->statements.empty());
            }
        }
    }
}

TEST(Parser, parse_func_decl$parameters_int) {
    const char* source = "func do_nothing(param0: Int) -> Int {}";
    Lexer       lexer(source);
    Parser      parser(lexer);

    ASTNode* node = parser.parse();
    EXPECT_NE(node, nullptr);

    if (node != nullptr) {
        EXPECT_EQ(node->kind, AST_FUNC);
        ASTFunc* func = reinterpret_cast<ASTFunc*>(node);
        EXPECT_NE(func->signature, nullptr);

        if (func->signature != nullptr) {
            EXPECT_AST_IDENTIFIER_TEXT_EQ(func->signature->name, "do_nothing");
            EXPECT_EQ(func->signature->parameters.size(), 1);
            if (func->signature->parameters.size() == 1) {
                ASTParameter* parameter = func->signature->parameters.at(0);
                EXPECT_NE(parameter, nullptr);

                if (parameter != nullptr) {
                    EXPECT_AST_IDENTIFIER_TEXT_EQ(parameter->name, "param0");
                    EXPECT_AST_IDENTIFIER_TEXT_EQ(parameter->type_name, "Int");
                }
            }

            EXPECT_AST_IDENTIFIER_TEXT_EQ(func->signature->return_type_name, "Int");
            EXPECT_NE(func->block, nullptr);
        }

        if (func->block != nullptr) {
            EXPECT_TRUE(func->block->statements.empty());
        }
    }
}

TEST(Parser, parse_func_decl$parameters_int_bool) {
    const char* source = "func my_func(param0: Int, param1: Bool) -> Void {}";
    Lexer       lexer(source);
    Parser      parser(lexer);

    ASTNode* node = parser.parse();
    EXPECT_NE(node, nullptr);

    if (node != nullptr) {
        EXPECT_EQ(node->kind, AST_FUNC);
        ASTFunc* func = reinterpret_cast<ASTFunc*>(node);

        EXPECT_NE(func->signature, nullptr);
        if (func->signature != nullptr) {
            EXPECT_AST_IDENTIFIER_TEXT_EQ(func->signature->name, "my_func");

            EXPECT_EQ(func->signature->parameters.size(), 2);
            if (func->signature->parameters.size() == 2) {
                ASTParameter* parameter = func->signature->parameters.at(0);
                EXPECT_NE(parameter, nullptr);

                if (parameter != nullptr) {
                    EXPECT_AST_IDENTIFIER_TEXT_EQ(parameter->name, "param0");
                    EXPECT_AST_IDENTIFIER_TEXT_EQ(parameter->type_name, "Int");
                }

                parameter = func->signature->parameters.at(1);
                EXPECT_NE(parameter, nullptr);

                if (parameter != nullptr) {
                    EXPECT_AST_IDENTIFIER_TEXT_EQ(parameter->name, "param1");
                    EXPECT_AST_IDENTIFIER_TEXT_EQ(parameter->type_name, "Bool");
                }
            }

            EXPECT_AST_IDENTIFIER_TEXT_EQ(func->signature->return_type_name, "Void");

        }

        EXPECT_NE(func->block, nullptr);
        if (func->block != nullptr) {
            EXPECT_TRUE(func->block->statements.empty());
        }
    }
}

TEST(Parser, parse_struct_decl$empty) {
    const char* source = "struct MyStruct { }";
    Lexer       lexer(source);
    Parser      parser(lexer);

    ASTNode* node = parser.parse();
    EXPECT_NE(node, nullptr);

    if (node != nullptr) {
        EXPECT_EQ(node->kind, AST_STRUCT);
        ASTStruct* structure = reinterpret_cast<ASTStruct*>(node);
        EXPECT_AST_IDENTIFIER_TEXT_EQ(structure->name, "MyStruct");
        EXPECT_TRUE(structure->variables.empty());
    }
}

TEST(Parser, parse_struct_decl$variables_int) {
    const char* source = "struct myStruct { var x: Int }";
    Lexer       lexer(source);
    Parser      parser(lexer);

    ASTNode* node = parser.parse();
    EXPECT_NE(node, nullptr);

    if (node != nullptr) {
        EXPECT_EQ(node->kind, AST_STRUCT);
        ASTStruct* structure = reinterpret_cast<ASTStruct*>(node);
        EXPECT_AST_IDENTIFIER_TEXT_EQ(structure->name, "myStruct");
        EXPECT_EQ(structure->variables.size(), 1);

        if (structure->variables.size() == 1) {
            ASTVariable* var = structure->variables.at(0);
            EXPECT_NE(var, nullptr);

            if (var != nullptr) {
                EXPECT_TRUE((var->flags & AST_VARIABLE_IS_CONSTANT) == 0);
                EXPECT_AST_IDENTIFIER_TEXT_EQ(var->name, "x");

                EXPECT_NE(var->type, nullptr);
                if (var->type != nullptr) {
                    EXPECT_EQ(var->type->type_kind, AST_TYPE_IDENTIFIER);

                    if (var->type->type_kind == AST_TYPE_IDENTIFIER) {
                        EXPECT_AST_IDENTIFIER_TEXT_EQ(var->type->identifier, "Int");
                    }
                }
            }
        }
    }
}

TEST(Parser, parse_struct_decl$variables_int_bool_string) {
    const char* source = "struct myStruct { var x: Int\n let isActive: Bool\n var name: String }";
    Lexer       lexer(source);
    Parser      parser(lexer);

    ASTNode* node = parser.parse();
    EXPECT_NE(node, nullptr);

    if (node != nullptr) {
        EXPECT_EQ(node->kind, AST_STRUCT);
        ASTStruct* structure = reinterpret_cast<ASTStruct*>(node);
        EXPECT_AST_IDENTIFIER_TEXT_EQ(structure->name, "myStruct");
        EXPECT_EQ(structure->variables.size(), 3);

        if (structure->variables.size() == 3) {
            ASTVariable* var = structure->variables.at(0);
            EXPECT_NE(var, nullptr);

            if (var != nullptr) {
                EXPECT_TRUE((var->flags & AST_VARIABLE_IS_CONSTANT) == 0);
                EXPECT_AST_IDENTIFIER_TEXT_EQ(var->name, "x");

                EXPECT_NE(var->type, nullptr);
                if (var->type != nullptr) {
                    EXPECT_EQ(var->type->type_kind, AST_TYPE_IDENTIFIER);

                    if (var->type->type_kind == AST_TYPE_IDENTIFIER) {
                        EXPECT_AST_IDENTIFIER_TEXT_EQ(var->type->identifier, "Int");
                    }
                }
            }

            var = structure->variables.at(1);
            EXPECT_NE(var, nullptr);

            if (var != nullptr) {
                EXPECT_TRUE((var->flags & AST_VARIABLE_IS_CONSTANT) == 1);
                EXPECT_AST_IDENTIFIER_TEXT_EQ(var->name, "isActive");

                EXPECT_NE(var->type, nullptr);
                if (var->type != nullptr) {
                    EXPECT_EQ(var->type->type_kind, AST_TYPE_IDENTIFIER);

                    if (var->type->type_kind == AST_TYPE_IDENTIFIER) {
                        EXPECT_AST_IDENTIFIER_TEXT_EQ(var->type->identifier, "Bool");
                    }
                }
            }

            var = structure->variables.at(2);
            EXPECT_NE(var, nullptr);

            if (var != nullptr) {
                EXPECT_TRUE((var->flags & AST_VARIABLE_IS_CONSTANT) == 0);
                EXPECT_AST_IDENTIFIER_TEXT_EQ(var->name, "name");

                EXPECT_NE(var->type, nullptr);
                if (var->type != nullptr) {
                    EXPECT_EQ(var->type->type_kind, AST_TYPE_IDENTIFIER);

                    if (var->type->type_kind == AST_TYPE_IDENTIFIER) {
                        EXPECT_AST_IDENTIFIER_TEXT_EQ(var->type->identifier, "String");
                    }
                }
            }
        }
    }
}

TEST(Parser, parse_struct_decl$missing_name) {
    const char* source = "struct { }";
    Lexer       lexer(source);
    Parser      parser(lexer);

    ASTNode* node = parser.parse();
    EXPECT_EQ(node, nullptr);
}

TEST(Parser, parse_struct_decl$missing_block) {
    const char* source = "struct MyStruct";
    Lexer       lexer(source);
    Parser      parser(lexer);

    ASTNode* node = parser.parse();
    EXPECT_EQ(node, nullptr);
}

TEST(Parser, parse_struct_decl$invalid_block_statement) {
    const char* source = "struct MyStruct { \n"
                         "    var x: Int    \n"
                         "    x = 10        \n"
                         "}";
    Lexer       lexer(source);
    Parser      parser(lexer);

    ASTNode* node = parser.parse();
    EXPECT_EQ(node, nullptr);
}

//TEST(Parser, parse_struct_decl$consecutive_declarations_on_a_line) {
//    const char* source = "struct Test { var x: Int var y: Int }";
//    Lexer       lexer(source);
//    Parser      parser(lexer);
//
//    ASTNode* node = parser.parse();
//    EXPECT_EQ(node, nullptr);
//}

//TEST(Parser, parse_enum_decl$consecutive_enum_elements_on_a_line) {
//    const char* source = "enum Test { case x case y }";
//    Lexer       lexer(source);
//    Parser      parser(lexer);
//
//    ASTNode* node = parser.parse();
//    EXPECT_EQ(node, nullptr);
//}

TEST(Parser, parse_enum_decl$empty_enum) {
    const char* source = "enum Test {}";
    Lexer       lexer(source);
    Parser      parser(lexer);

    ASTNode* node = parser.parse();
    EXPECT_NE(node, nullptr);

    if (node != nullptr) {
        EXPECT_EQ(node->kind, AST_ENUM);

        if (node->kind == AST_ENUM) {
            ASTEnum* enumeration = reinterpret_cast<ASTEnum*>(node);
            EXPECT_AST_IDENTIFIER_TEXT_EQ(enumeration->name, "Test");
            EXPECT_EQ(enumeration->elements.size(), 0);
        }
    }
}

TEST(Parser, parse_enum_decl$with_cases) {
    const char* source = "enum Test {\n case a\n case b }";
    Lexer       lexer(source);
    Parser      parser(lexer);

    ASTNode* node = parser.parse();
    EXPECT_NE(node, nullptr);

    if (node != nullptr) {
        EXPECT_EQ(node->kind, AST_ENUM);

        if (node->kind == AST_ENUM) {
            ASTEnum* enumeration = reinterpret_cast<ASTEnum*>(node);
            EXPECT_AST_IDENTIFIER_TEXT_EQ(enumeration->name, "Test");
            EXPECT_EQ(enumeration->elements.size(), 2);

            if (enumeration->elements.size() == 2) {
                ASTEnumElement* element = enumeration->elements.at(0);
                EXPECT_NE(element, nullptr);

                if (element != nullptr) {
                    EXPECT_AST_IDENTIFIER_TEXT_EQ(element->name, "a");
                    EXPECT_EQ(element->assignment, nullptr);
                }

                element = enumeration->elements.at(1);
                EXPECT_NE(element, nullptr);

                if (element != nullptr) {
                    EXPECT_AST_IDENTIFIER_TEXT_EQ(element->name, "b");
                    EXPECT_EQ(element->assignment, nullptr);
                }
            }
        }
    }
}

TEST(Parser, parse_enum_decl$with_int_literal_assignments) {
    const char* source = "enum Test {\n case a = 0\n case b = 1 }";
    Lexer       lexer(source);
    Parser      parser(lexer);

    ASTNode* node = parser.parse();
    EXPECT_NE(node, nullptr);

    if (node != nullptr) {
        EXPECT_EQ(node->kind, AST_ENUM);

        if (node->kind == AST_ENUM) {
            ASTEnum* enumeration = reinterpret_cast<ASTEnum*>(node);
            EXPECT_AST_IDENTIFIER_TEXT_EQ(enumeration->name, "Test");
            EXPECT_EQ(enumeration->elements.size(), 2);

            if (enumeration->elements.size() == 2) {
                ASTEnumElement* element = enumeration->elements.at(0);
                EXPECT_NE(element, nullptr);

                if (element != nullptr) {
                    EXPECT_AST_IDENTIFIER_TEXT_EQ(element->name, "a");
                    EXPECT_NE(element->assignment, nullptr);

                    if (element->assignment != nullptr) {
                        EXPECT_EQ(element->assignment->kind, AST_LITERAL);

                        ASTLiteral* literal = reinterpret_cast<ASTLiteral*>(element->assignment);
                        EXPECT_EQ(literal->token_kind, TOKEN_LITERAL_INT);

                        if (literal->token_kind == TOKEN_LITERAL_INT) {
                            EXPECT_EQ(literal->int_value, 0);
                        }
                    }
                }

                element = enumeration->elements.at(1);
                EXPECT_NE(element, nullptr);

                if (element != nullptr) {
                    EXPECT_AST_IDENTIFIER_TEXT_EQ(element->name, "b");
                    EXPECT_NE(element->assignment, nullptr);

                    if (element->assignment != nullptr) {
                        EXPECT_EQ(element->assignment->kind, AST_LITERAL);

                        ASTLiteral* literal = reinterpret_cast<ASTLiteral*>(element->assignment);
                        EXPECT_EQ(literal->token_kind, TOKEN_LITERAL_INT);

                        if (literal->token_kind == TOKEN_LITERAL_INT) {
                            EXPECT_EQ(literal->int_value, 1);
                        }
                    }
                }
            }
        }
    }
}

TEST(Parser, parse_enum_decl$missing_name) {
    const char* source = "enum {}";
    Lexer       lexer(source);
    Parser      parser(lexer);

    ASTNode* node = parser.parse();
    EXPECT_EQ(node, nullptr);
}

TEST(Parser, parse_enum_decl$missing_opening_parenthesis) {
    const char* source = "enum MyEnum }";
    Lexer       lexer(source);
    Parser      parser(lexer);

    ASTNode* node = parser.parse();
    EXPECT_EQ(node, nullptr);
}

TEST(Parser, parse_enum_decl$missing_closing_parenthesis) {
    const char* source = "enum MyEnum { \n"
                         "case first    \n";
    Lexer       lexer(source);
    Parser      parser(lexer);

    ASTNode* node = parser.parse();
    EXPECT_EQ(node, nullptr);
}

TEST(Parser, parse_enum_decl$missing_expression_after_assignment) {
    const char* source = "enum MyEnum { \n"
                         "case first =  \n"
                         "}";
    Lexer       lexer(source);
    Parser      parser(lexer);

    ASTNode* node = parser.parse();
    EXPECT_EQ(node, nullptr);
}

TEST(Parser, parse_enum_decl$missing_case_keyword) {
    const char* source = "enum MyEnum { \n"
                         "first = 0     \n"
                         "}";
    Lexer       lexer(source);
    Parser      parser(lexer);

    ASTNode* node = parser.parse();
    EXPECT_EQ(node, nullptr);
}

TEST(Parser, parse_enum_decl$missing_element_name) {
    const char* source = "enum MyEnum { \n"
                         "case          \n"
                         "case second   \n"
                         "}";
    Lexer       lexer(source);
    Parser      parser(lexer);

    ASTNode* node = parser.parse();
    EXPECT_EQ(node, nullptr);
}

TEST(Parser, parse_variable_decl$assign_group_expression) {
    const char* source = "var x: Int = (1)";
    Lexer       lexer(source);
    Parser      parser(lexer);

    ASTNode* node = parser.parse();
    EXPECT_NE(node, nullptr);

    if (node != nullptr) {
        EXPECT_EQ(node->kind, AST_VARIABLE);
        EXPECT_FALSE((node->flags & AST_VARIABLE_IS_CONSTANT));

        ASTVariable* var = reinterpret_cast<ASTVariable*>(node);
        EXPECT_AST_IDENTIFIER_TEXT_EQ(var->name, "x");

        EXPECT_NE(var->type, nullptr);
        if (var->type != nullptr) {
            EXPECT_EQ(var->type->type_kind, AST_TYPE_IDENTIFIER);

            if (var->type->type_kind == AST_TYPE_IDENTIFIER) {
                EXPECT_AST_IDENTIFIER_TEXT_EQ(var->type->identifier, "Int");
            }
        }

        EXPECT_NE(var->assignment, nullptr);
        if (var->assignment != nullptr) {
            EXPECT_EQ(var->assignment->kind, AST_LITERAL);

            ASTLiteral* literal = reinterpret_cast<ASTLiteral*>(var->assignment);
            EXPECT_EQ(literal->token_kind, TOKEN_LITERAL_INT);
            EXPECT_EQ(literal->int_value, 1);
        }
    }
}

TEST(Parser, parse_variable_declaration$missing_name) {
    const char* source = "var : Int";
    Lexer       lexer(source);
    Parser      parser(lexer);

    ASTNode* node = parser.parse();
    EXPECT_EQ(node, nullptr);
}

TEST(Parser, parse_variable_declaration$missing_colon) {
    const char* source = "var x Int";
    Lexer       lexer(source);
    Parser      parser(lexer);

    ASTNode* node = parser.parse();
    EXPECT_EQ(node, nullptr);
}

TEST(Parser, parse_variable_declaration$missing_type_name) {
    const char* source = "var x: = 0";
    Lexer       lexer(source);
    Parser      parser(lexer);

    ASTNode* node = parser.parse();
    EXPECT_EQ(node, nullptr);
}

TEST(Parser, parse_variable_declaration$missing_expression) {
    const char* source = "var x: Int =";
    Lexer       lexer(source);
    Parser      parser(lexer);

    ASTNode* node = parser.parse();
    EXPECT_EQ(node, nullptr);
}

TEST(Parser, parse_literal$integer_literal_overflow) {
    const char* source = "var x: Int = 991829381928391829389128391823981928391283918239812938";
    Lexer       lexer(source);
    Parser      parser(lexer);

    ASTNode* node = parser.parse();
    EXPECT_EQ(node, nullptr);
}

TEST(Parser, parse_literal$float_literal) {
    const char* source = "var x: Double = 0.5";
    Lexer       lexer(source);
    Parser      parser(lexer);

    ASTNode* node = parser.parse();

    EXPECT_NE(node, nullptr);
    if (node != nullptr) {

        EXPECT_EQ(node->kind, AST_VARIABLE);
        if (node->kind == AST_VARIABLE) {

            ASTVariable* var = reinterpret_cast<ASTVariable*>(node);
            EXPECT_AST_IDENTIFIER_TEXT_EQ(var->name, "x");

            EXPECT_NE(var->type, nullptr);
            if (var->type != nullptr) {
                EXPECT_EQ(var->type->type_kind, AST_TYPE_IDENTIFIER);

                if (var->type->type_kind == AST_TYPE_IDENTIFIER) {
                    EXPECT_AST_IDENTIFIER_TEXT_EQ(var->type->identifier, "Double");
                }
            }

            EXPECT_NE(var->assignment, nullptr);
            if (var->assignment != nullptr) {

                EXPECT_EQ(var->assignment->kind, AST_LITERAL);
                if (var->assignment->kind == AST_LITERAL) {

                    ASTLiteral* literal = reinterpret_cast<ASTLiteral*>(var->assignment);
                    EXPECT_EQ(literal->token_kind, TOKEN_LITERAL_FLOAT);
                    EXPECT_EQ(literal->float_value, 0.5);
                }
            }
        }
    }
}

TEST(Parser, parse_literal$bool_literal_true) {
    const char* source = "var isActive: Bool = true";
    Lexer       lexer(source);
    Parser      parser(lexer);

    ASTNode* node = parser.parse();

    EXPECT_NE(node, nullptr);
    if (node != nullptr) {

        EXPECT_EQ(node->kind, AST_VARIABLE);
        if (node->kind == AST_VARIABLE) {

            ASTVariable* var = reinterpret_cast<ASTVariable*>(node);
            EXPECT_AST_IDENTIFIER_TEXT_EQ(var->name, "isActive");

            EXPECT_NE(var->type, nullptr);
            if (var->type != nullptr) {
                EXPECT_EQ(var->type->type_kind, AST_TYPE_IDENTIFIER);

                if (var->type->type_kind == AST_TYPE_IDENTIFIER) {
                    EXPECT_AST_IDENTIFIER_TEXT_EQ(var->type->identifier, "Bool");
                }
            }

            EXPECT_NE(var->assignment, nullptr);
            if (var->assignment != nullptr) {

                EXPECT_EQ(var->assignment->kind, AST_LITERAL);
                if (var->assignment->kind == AST_LITERAL) {

                    ASTLiteral* literal = reinterpret_cast<ASTLiteral*>(var->assignment);
                    EXPECT_EQ(literal->token_kind, TOKEN_LITERAL_BOOL);
                    EXPECT_EQ(literal->bool_value, true);
                }
            }
        }
    }
}

TEST(Parser, parse_literal$bool_literal_false) {
    const char* source = "var isActive: Bool = false";
    Lexer       lexer(source);
    Parser      parser(lexer);

    ASTNode* node = parser.parse();

    EXPECT_NE(node, nullptr);
    if (node != nullptr) {

        EXPECT_EQ(node->kind, AST_VARIABLE);
        if (node->kind == AST_VARIABLE) {

            ASTVariable* var = reinterpret_cast<ASTVariable*>(node);
            EXPECT_AST_IDENTIFIER_TEXT_EQ(var->name, "isActive");

            EXPECT_NE(var->type, nullptr);
            if (var->type != nullptr) {
                EXPECT_EQ(var->type->type_kind, AST_TYPE_IDENTIFIER);

                if (var->type->type_kind == AST_TYPE_IDENTIFIER) {
                    EXPECT_AST_IDENTIFIER_TEXT_EQ(var->type->identifier, "Bool");
                }
            }

            EXPECT_NE(var->assignment, nullptr);
            if (var->assignment != nullptr) {

                EXPECT_EQ(var->assignment->kind, AST_LITERAL);
                if (var->assignment->kind == AST_LITERAL) {

                    ASTLiteral* literal = reinterpret_cast<ASTLiteral*>(var->assignment);
                    EXPECT_EQ(literal->token_kind, TOKEN_LITERAL_BOOL);
                    EXPECT_EQ(literal->bool_value, false);
                }
            }
        }
    }
}

TEST(Parser, parse_literal$nil_literal) {
    const char* source = "var mem: Void* = nil";
    Lexer       lexer(source);
    Parser      parser(lexer);

    ASTNode* node = parser.parse();

    EXPECT_NE(node, nullptr);
    if (node != nullptr) {

        EXPECT_EQ(node->kind, AST_VARIABLE);
        if (node->kind == AST_VARIABLE) {

            ASTVariable* var = reinterpret_cast<ASTVariable*>(node);
            EXPECT_AST_IDENTIFIER_TEXT_EQ(var->name, "mem");

            EXPECT_NE(var->type, nullptr);
            if (var->type != nullptr) {
                EXPECT_EQ(var->type->type_kind, AST_TYPE_POINTER);

                if (var->type->type_kind == AST_TYPE_POINTER) {
                    EXPECT_NE(var->type->type, nullptr);

                    if (var->type->type) {
                        EXPECT_EQ(var->type->type->type_kind, AST_TYPE_IDENTIFIER);
                        EXPECT_AST_IDENTIFIER_TEXT_EQ(var->type->type->identifier, "Void");
                    }
                }
            }

            EXPECT_NE(var->assignment, nullptr);
            if (var->assignment != nullptr) {

                EXPECT_EQ(var->assignment->kind, AST_LITERAL);
                if (var->assignment->kind == AST_LITERAL) {

                    ASTLiteral* literal = reinterpret_cast<ASTLiteral*>(var->assignment);
                    EXPECT_EQ(literal->token_kind, TOKEN_LITERAL_NIL);
                }
            }
        }
    }
}

TEST(Parser, parse_expression$unary_plus) {
    const char* source = "var x: Int = -10";
    Lexer       lexer(source);
    Parser      parser(lexer);

    ASTNode* node = parser.parse();

    EXPECT_NE(node, nullptr);
    if (node != nullptr) {

        EXPECT_EQ(node->kind, AST_VARIABLE);
        if (node->kind == AST_VARIABLE) {

            ASTVariable* var = reinterpret_cast<ASTVariable*>(node);
            EXPECT_AST_IDENTIFIER_TEXT_EQ(var->name, "x");

            EXPECT_NE(var->type, nullptr);
            if (var->type != nullptr) {
                EXPECT_EQ(var->type->type_kind, AST_TYPE_IDENTIFIER);

                if (var->type->type_kind == AST_TYPE_IDENTIFIER) {
                    EXPECT_AST_IDENTIFIER_TEXT_EQ(var->type->identifier, "Int");
                }
            }

            EXPECT_NE(var->assignment, nullptr);
            if (var->assignment != nullptr) {

                EXPECT_EQ(var->assignment->kind, AST_UNARY);
                if (var->assignment->kind == AST_UNARY) {

                    ASTUnaryExpression* unary = reinterpret_cast<ASTUnaryExpression*>(var->assignment);
                    EXPECT_TRUE(unary->op.text.is_equal("-"));

                    EXPECT_NE(unary->right, nullptr);
                    if (unary->right != nullptr) {

                        EXPECT_EQ(unary->right->kind, AST_LITERAL);
                        if (unary->right->kind == AST_LITERAL) {

                            ASTLiteral* literal = reinterpret_cast<ASTLiteral*>(unary->right);
                            EXPECT_EQ(literal->token_kind, TOKEN_LITERAL_INT);
                            EXPECT_EQ(literal->int_value, 10);
                        }
                    }
                }
            }
        }
    }
}

TEST(Parser, parse_expression$invalid_infix_operator) {
    const char* source = "var x: Int = -10 ! 0";
    Lexer       lexer(source);
    Parser      parser(lexer);

    ASTNode* node = parser.parse();
    EXPECT_EQ(node, nullptr);
}

TEST(Parser, parse_expression$invalid_prefix_operator) {
    const char* source = "var x: Int = <<10";
    Lexer       lexer(source);
    Parser      parser(lexer);

    ASTNode* node = parser.parse();
    EXPECT_EQ(node, nullptr);
}

TEST(Parser, parse_expression$missing_right_expression) {
    const char* source = "var x: Int = -10 + ";
    Lexer       lexer(source);
    Parser      parser(lexer);

    ASTNode* node = parser.parse();
    EXPECT_EQ(node, nullptr);
}

TEST(Parser, parse_group_expression$empty) {
    const char* source = "var x: Int = ()";
    Lexer       lexer(source);
    Parser      parser(lexer);

    ASTNode* node = parser.parse();
    EXPECT_EQ(node, nullptr);
}

TEST(Parser, parse_group_expression$missing_closing_parenthesis) {
    const char* source = "var x: Int = (10 + (2 - 1)";
    Lexer       lexer(source);
    Parser      parser(lexer);

    ASTNode* node = parser.parse();
    EXPECT_EQ(node, nullptr);
}

TEST(Parser, parse_control_statement$with_break) {
    const char* source = "func main() -> Void { break }";
    Lexer       lexer(source);
    Parser      parser(lexer);

    ASTNode* node = parser.parse();

    EXPECT_NE(node, nullptr);
    if (node != nullptr) {

        EXPECT_EQ(node->kind, AST_FUNC);
        if (node->kind == AST_FUNC) {

            ASTFunc* func = reinterpret_cast<ASTFunc*>(node);
            EXPECT_NE(func->block, nullptr);
            if (func->block != nullptr) {

                EXPECT_EQ(func->block->statements.size(), 1);
                if (func->block->statements.size() == 1) {

                    ASTStatement* stmt = func->block->statements.at(0);
                    EXPECT_EQ(stmt->kind, AST_CONTROL);
                    if (stmt->kind == AST_CONTROL) {

                        ASTControl* control = reinterpret_cast<ASTControl*>(stmt);
                        EXPECT_EQ(control->token_kind, TOKEN_KEYWORD_BREAK);
                        EXPECT_EQ(control->expression, nullptr);
                    }
                }
            }
        }
    }
}

TEST(Parser, parse_control_statement$with_continue) {
    const char* source = "func main() -> Void { continue }";
    Lexer       lexer(source);
    Parser      parser(lexer);

    ASTNode* node = parser.parse();

    EXPECT_NE(node, nullptr);
    if (node != nullptr) {

        EXPECT_EQ(node->kind, AST_FUNC);
        if (node->kind == AST_FUNC) {

            ASTFunc* func = reinterpret_cast<ASTFunc*>(node);
            EXPECT_NE(func->block, nullptr);
            if (func->block != nullptr) {

                EXPECT_EQ(func->block->statements.size(), 1);
                if (func->block->statements.size() == 1) {

                    ASTStatement* stmt = func->block->statements.at(0);
                    EXPECT_EQ(stmt->kind, AST_CONTROL);
                    if (stmt->kind == AST_CONTROL) {

                        ASTControl* control = reinterpret_cast<ASTControl*>(stmt);
                        EXPECT_EQ(control->token_kind, TOKEN_KEYWORD_CONTINUE);
                        EXPECT_EQ(control->expression, nullptr);
                    }
                }
            }
        }
    }
}

TEST(Parser, parse_control_statement$with_fallthrough) {
    const char* source = "func main() -> Void { fallthrough }";
    Lexer       lexer(source);
    Parser      parser(lexer);

    ASTNode* node = parser.parse();

    EXPECT_NE(node, nullptr);
    if (node != nullptr) {

        EXPECT_EQ(node->kind, AST_FUNC);
        if (node->kind == AST_FUNC) {

            ASTFunc* func = reinterpret_cast<ASTFunc*>(node);
            EXPECT_NE(func->block, nullptr);
            if (func->block != nullptr) {

                EXPECT_EQ(func->block->statements.size(), 1);
                if (func->block->statements.size() == 1) {

                    ASTStatement* stmt = func->block->statements.at(0);
                    EXPECT_EQ(stmt->kind, AST_CONTROL);
                    if (stmt->kind == AST_CONTROL) {

                        ASTControl* control = reinterpret_cast<ASTControl*>(stmt);
                        EXPECT_EQ(control->token_kind, TOKEN_KEYWORD_FALLTHROUGH);
                        EXPECT_EQ(control->expression, nullptr);
                    }
                }
            }
        }
    }
}

TEST(Parser, parse_control_statement$with_return) {
    const char* source = "func main() -> Void { return }";
    Lexer       lexer(source);
    Parser      parser(lexer);

    ASTNode* node = parser.parse();

    EXPECT_NE(node, nullptr);
    if (node != nullptr) {

        EXPECT_EQ(node->kind, AST_FUNC);
        if (node->kind == AST_FUNC) {

            ASTFunc* func = reinterpret_cast<ASTFunc*>(node);
            EXPECT_NE(func->block, nullptr);
            if (func->block != nullptr) {

                EXPECT_EQ(func->block->statements.size(), 1);
                if (func->block->statements.size() == 1) {

                    ASTStatement* stmt = func->block->statements.at(0);
                    EXPECT_EQ(stmt->kind, AST_CONTROL);
                    if (stmt->kind == AST_CONTROL) {

                        ASTControl* control = reinterpret_cast<ASTControl*>(stmt);
                        EXPECT_EQ(control->token_kind, TOKEN_KEYWORD_RETURN);
                        EXPECT_EQ(control->expression, nullptr);
                    }
                }
            }
        }
    }
}

TEST(Parser, parse_control_statement$with_return_and_expression) {
    const char* source = "func main() -> Void { return 1 }";
    Lexer       lexer(source);
    Parser      parser(lexer);

    ASTNode* node = parser.parse();

    EXPECT_NE(node, nullptr);
    if (node != nullptr) {

        EXPECT_EQ(node->kind, AST_FUNC);
        if (node->kind == AST_FUNC) {

            ASTFunc* func = reinterpret_cast<ASTFunc*>(node);
            EXPECT_NE(func->block, nullptr);
            if (func->block != nullptr) {

                EXPECT_EQ(func->block->statements.size(), 1);
                if (func->block->statements.size() == 1) {

                    ASTStatement* stmt = func->block->statements.at(0);
                    EXPECT_EQ(stmt->kind, AST_CONTROL);
                    if (stmt->kind == AST_CONTROL) {

                        ASTControl* control = reinterpret_cast<ASTControl*>(stmt);
                        EXPECT_EQ(control->token_kind, TOKEN_KEYWORD_RETURN);

                        EXPECT_NE(control->expression, nullptr);
                        if (control->expression != nullptr) {

                            EXPECT_EQ(control->expression->kind, AST_LITERAL);
                            if (control->expression->kind == AST_LITERAL) {

                                ASTLiteral* literal = reinterpret_cast<ASTLiteral*>(control->expression);
                                EXPECT_EQ(literal->token_kind, TOKEN_LITERAL_INT);
                                EXPECT_EQ(literal->int_value, 1);
                            }
                        }
                    }
                }
            }
        }
    }
}

TEST(Parser, parse_type$identifier) {
    const char* source = "var x: Int = 0";
    Lexer       lexer(source);
    Parser      parser(lexer);

    ASTNode* node = parser.parse();
    EXPECT_NE(node, nullptr);

    if (node != nullptr) {

        EXPECT_EQ(node->kind, AST_VARIABLE);
        if (node->kind == AST_VARIABLE) {
            ASTVariable* var = reinterpret_cast<ASTVariable*>(node);

            EXPECT_NE(var->type, nullptr);
            if (var->type != nullptr) {
                EXPECT_EQ(var->type->type_kind, AST_TYPE_IDENTIFIER);
                EXPECT_NE(var->type->identifier, nullptr);

                if (var->type->identifier != nullptr) {
                    EXPECT_AST_IDENTIFIER_TEXT_EQ(var->type->identifier, "Int");
                }
            }
        }
    }
}

TEST(Parser, parse_type$any) {
    const char* source = "var x: Any = 0";
    Lexer       lexer(source);
    Parser      parser(lexer);

    ASTNode* node = parser.parse();
    EXPECT_NE(node, nullptr);

    if (node != nullptr) {

        EXPECT_EQ(node->kind, AST_VARIABLE);
        if (node->kind == AST_VARIABLE) {
            ASTVariable* var = reinterpret_cast<ASTVariable*>(node);

            EXPECT_NE(var->type, nullptr);
            if (var->type != nullptr) {
                EXPECT_EQ(var->type->type_kind, AST_TYPE_ANY);
            }
        }
    }
}

TEST(Parser, parse_type$pointer) {
    const char* source = "var x: Void* = nil";
    Lexer       lexer(source);
    Parser      parser(lexer);

    ASTNode* node = parser.parse();
    EXPECT_NE(node, nullptr);

    if (node != nullptr) {

        EXPECT_EQ(node->kind, AST_VARIABLE);
        if (node->kind == AST_VARIABLE) {
            ASTVariable* var = reinterpret_cast<ASTVariable*>(node);

            EXPECT_NE(var->type, nullptr);
            if (var->type) {
                EXPECT_EQ(var->type->type_kind, AST_TYPE_POINTER);
                EXPECT_NE(var->type->type, nullptr);

                if (var->type->type) {
                    EXPECT_EQ(var->type->type->type_kind, AST_TYPE_IDENTIFIER);
                    EXPECT_AST_IDENTIFIER_TEXT_EQ(var->type->type->identifier, "Void");
                }
            }
        }
    }
}

TEST(Parser, parse_type$pointer_pointer) {
    const char* source = "var x: Void** = nil";
    Lexer       lexer(source);
    Parser      parser(lexer);

    ASTNode* node = parser.parse();
    EXPECT_NE(node, nullptr);

    if (node != nullptr) {

        EXPECT_EQ(node->kind, AST_VARIABLE);
        if (node->kind == AST_VARIABLE) {
            ASTVariable* var = reinterpret_cast<ASTVariable*>(node);
            ASTType*     type = var->type;

            EXPECT_NE(type, nullptr);
            if (type) {
                EXPECT_EQ(type->type_kind, AST_TYPE_POINTER);

                type = type->type;
                EXPECT_NE(type, nullptr);
                if (type) {
                    EXPECT_EQ(type->type_kind, AST_TYPE_POINTER);

                    type = type->type;
                    EXPECT_NE(type, nullptr);
                    if (type) {
                        EXPECT_EQ(type->type_kind, AST_TYPE_IDENTIFIER);
                        EXPECT_AST_IDENTIFIER_TEXT_EQ(type->identifier, "Void");
                    }
                }
            }
        }
    }
}

TEST(Parser, parse_type$array) {
    const char* source = "var x: Void[] = nil";
    Lexer       lexer(source);
    Parser      parser(lexer);

    ASTNode* node = parser.parse();
    EXPECT_NE(node, nullptr);

    if (node != nullptr) {

        EXPECT_EQ(node->kind, AST_VARIABLE);
        if (node->kind == AST_VARIABLE) {
            ASTVariable* var = reinterpret_cast<ASTVariable*>(node);
            ASTType*     type = var->type;

            EXPECT_NE(type, nullptr);
            if (type) {
                EXPECT_EQ(type->type_kind, AST_TYPE_ARRAY);
                EXPECT_EQ(type->expression, nullptr);

                type = type->type;
                EXPECT_NE(type, nullptr);
                if (type) {
                    EXPECT_EQ(type->type_kind, AST_TYPE_IDENTIFIER);
                    EXPECT_AST_IDENTIFIER_TEXT_EQ(type->identifier, "Void");
                }
            }
        }
    }
}

TEST(Parser, parse_type$pointer_pointer_array) {
    const char* source = "var x: Void**[] = nil";
    Lexer       lexer(source);
    Parser      parser(lexer);

    ASTNode* node = parser.parse();
    EXPECT_NE(node, nullptr);

    if (node != nullptr) {
        EXPECT_EQ(node->kind, AST_VARIABLE);
        if (node->kind == AST_VARIABLE) {
            ASTVariable* var = reinterpret_cast<ASTVariable*>(node);
            ASTType*     type = var->type;

            EXPECT_NE(type, nullptr);
            if (type) {
                EXPECT_EQ(type->type_kind, AST_TYPE_ARRAY);
                EXPECT_EQ(type->expression, nullptr);

                EXPECT_NE(type->type, nullptr);
                type = type->type;
                if (type) {
                    EXPECT_EQ(type->type_kind, AST_TYPE_POINTER);
                    EXPECT_NE(type->type, nullptr);
                    type = type->type;
                    if (type) {
                        EXPECT_EQ(type->type_kind, AST_TYPE_POINTER);
                        EXPECT_NE(type->type, nullptr);
                        type = type->type;
                        if (type) {
                            EXPECT_EQ(type->type_kind, AST_TYPE_IDENTIFIER);
                            EXPECT_AST_IDENTIFIER_TEXT_EQ(type->identifier, "Void");
                        }
                    }
                }
            }
        }
    }
}

TEST(Parser, parse_type$array_missing_closing_bracket) {
    const char* source = "var x: Void[10 = nil";
    Lexer       lexer(source);
    Parser      parser(lexer);

    ASTNode* node = parser.parse();
    EXPECT_EQ(node, nullptr);
}

TEST(Parser, parse_type$sized_array) {
    const char* source = "var x: Void[10] = nil";
    Lexer       lexer(source);
    Parser      parser(lexer);

    ASTNode* node = parser.parse();
    EXPECT_NE(node, nullptr);

    if (node != nullptr) {

        EXPECT_EQ(node->kind, AST_VARIABLE);
        if (node->kind == AST_VARIABLE) {
            ASTVariable* var = reinterpret_cast<ASTVariable*>(node);
            ASTType*     type = var->type;

            EXPECT_NE(type, nullptr);
            if (type) {
                EXPECT_EQ(type->type_kind, AST_TYPE_ARRAY);
                EXPECT_NE(type->expression, nullptr);

                if (type->expression) {
                    EXPECT_EQ(type->expression->kind, AST_LITERAL);

                    if (type->expression->kind == AST_LITERAL) {
                        ASTLiteral* literal = reinterpret_cast<ASTLiteral*>(type->expression);
                        EXPECT_EQ(literal->token_kind, TOKEN_LITERAL_INT);
                        EXPECT_EQ(literal->int_value, 10);
                    }
                }

                type = type->type;
                EXPECT_NE(type, nullptr);
                if (type) {
                    EXPECT_EQ(type->type_kind, AST_TYPE_IDENTIFIER);
                    EXPECT_AST_IDENTIFIER_TEXT_EQ(type->identifier, "Void");
                }
            }
        }
    }
}

TEST(Parser, parse_type$typeof) {
    const char* source = "var x: typeof(true) = false";
    Lexer       lexer(source);
    Parser      parser(lexer);

    ASTNode* node = parser.parse();
    EXPECT_NE(node, nullptr);

    if (node != nullptr) {

        EXPECT_EQ(node->kind, AST_VARIABLE);
        if (node->kind == AST_VARIABLE) {
            ASTVariable* var = reinterpret_cast<ASTVariable*>(node);
            ASTType*     type = var->type;

            EXPECT_NE(type, nullptr);
            if (type) {
                EXPECT_EQ(type->type_kind, AST_TYPE_TYPEOF);
                EXPECT_NE(type->expression, nullptr);

                if (type->expression) {
                    EXPECT_EQ(type->expression->kind, AST_LITERAL);

                    if (type->expression->kind == AST_LITERAL) {
                        ASTLiteral* literal = reinterpret_cast<ASTLiteral*>(type->expression);
                        EXPECT_EQ(literal->token_kind, TOKEN_LITERAL_BOOL);
                        EXPECT_EQ(literal->bool_value, true);
                    }
                }
            }
        }
    }
}

TEST(Parser, parse_defer_statement$defer_assign_zero) {
    const char* source =
    "func main() -> Void {  \n"
    "   var value: Int = 10 \n"
    "   defer value = 0     \n"
    "}";
    Lexer       lexer(source);
    Parser      parser(lexer);

    ASTNode* node = parser.parse();
    EXPECT_NE(node, nullptr);

    if (node != nullptr) {
        EXPECT_EQ(node->kind, AST_FUNC);

        ASTFunc* func = reinterpret_cast<ASTFunc*>(node);
        EXPECT_NE(func->block, nullptr);

        if (func->block) {
            EXPECT_EQ(func->block->statements.size(), 2);
            if (func->block->statements.size() == 2) {
                node = func->block->statements.at(1);
                EXPECT_EQ(node->kind, AST_DEFER);

                ASTDefer* defer = reinterpret_cast<ASTDefer*>(node);
                EXPECT_NE(defer->expression, nullptr);
                if (defer->expression) {
                    EXPECT_EQ(defer->expression->kind, AST_BINARY);
                }
            }
        }
    }
}

#warning Add expectations below...

TEST(Parser, parse_do_while_statement) {
    const char* source =
    "func main() -> Void {  \n"
    "   do { } while true   \n"
    "}";
    Lexer       lexer(source);
    Parser      parser(lexer);

    ASTNode* node = parser.parse();
    EXPECT_NE(node, nullptr);
}

TEST(Parser, parse_do_while_statement$missing_block) {
    const char* source =
    "func main() -> Void {  \n"
    "   do while true   \n"
    "}";
    Lexer       lexer(source);
    Parser      parser(lexer);

    ASTNode* node = parser.parse();
    EXPECT_EQ(node, nullptr);
}

TEST(Parser, parse_do_while_statement$missing_keyword_while) {
    const char* source =
    "func main() -> Void {  \n"
    "   do {} true   \n"
    "}";
    Lexer       lexer(source);
    Parser      parser(lexer);

    ASTNode* node = parser.parse();
    EXPECT_EQ(node, nullptr);
}

TEST(Parser, parse_do_while_statement$missing_expression) {
    const char* source =
    "func main() -> Void {  \n"
    "   do {} while true,   \n"
    "}";
    Lexer       lexer(source);
    Parser      parser(lexer);

    ASTNode* node = parser.parse();
    EXPECT_EQ(node, nullptr);
}

TEST(Parser, parse_for_statement) {
    const char* source =
    "func main() -> Void {  \n"
    "   for i in sequence { \n"
    "   }                   \n"
    "}";
    Lexer       lexer(source);
    Parser      parser(lexer);

    ASTNode* node = parser.parse();
    EXPECT_NE(node, nullptr);
}

TEST(Parser, parse_for_statement$missing_iterator) {
    const char* source =
    "func main() -> Void {  \n"
    "   for in sequence { \n"
    "   }                   \n"
    "}";
    Lexer       lexer(source);
    Parser      parser(lexer);

    ASTNode* node = parser.parse();
    EXPECT_EQ(node, nullptr);
}

TEST(Parser, parse_for_statement$missing_in_keyword) {
    const char* source =
    "func main() -> Void {  \n"
    "   for i sequence { \n"
    "   }                   \n"
    "}";
    Lexer       lexer(source);
    Parser      parser(lexer);

    ASTNode* node = parser.parse();
    EXPECT_EQ(node, nullptr);
}

TEST(Parser, parse_for_statement$missing_expression) {
    const char* source =
    "func main() -> Void {  \n"
    "   for i in { \n"
    "   }                   \n"
    "}";
    Lexer       lexer(source);
    Parser      parser(lexer);

    ASTNode* node = parser.parse();
    EXPECT_EQ(node, nullptr);
}

TEST(Parser, parse_for_statement$missing_block) {
    const char* source =
    "func main() -> Void {  \n"
    "   for i in sequence   \n"
    "                       \n"
    "}";
    Lexer       lexer(source);
    Parser      parser(lexer);

    ASTNode* node = parser.parse();
    EXPECT_EQ(node, nullptr);
}

TEST(Parser, parse_guard_statement) {
    const char* source =
    "func main() -> Void {          \n"
    "   guard true else { return }  \n"
    "}";
    Lexer       lexer(source);
    Parser      parser(lexer);

    ASTNode* node = parser.parse();
    EXPECT_NE(node, nullptr);
}

TEST(Parser, parse_guard_statement$multiple_conditions) {
    const char* source =
    "func main() -> Void {                      \n"
    "   guard true, true, true else { return }  \n"
    "}";
    Lexer       lexer(source);
    Parser      parser(lexer);

    ASTNode* node = parser.parse();
    EXPECT_NE(node, nullptr);
}

TEST(Parser, parse_guard_statement$missing_expression) {
    const char* source =
    "func main() -> Void {                      \n"
    "   guard else { return }                   \n"
    "}";
    Lexer       lexer(source);
    Parser      parser(lexer);

    ASTNode* node = parser.parse();
    EXPECT_EQ(node, nullptr);
}

TEST(Parser, parse_guard_statement$missing_delimiter) {
    const char* source =
    "func main() -> Void {                      \n"
    "   guard true, true true else { return }   \n"
    "}";
    Lexer       lexer(source);
    Parser      parser(lexer);

    ASTNode* node = parser.parse();
    EXPECT_EQ(node, nullptr);
}

TEST(Parser, parse_guard_statement$missing_else) {
    const char* source =
    "func main() -> Void {                      \n"
    "   guard true, true { return }             \n"
    "}";
    Lexer       lexer(source);
    Parser      parser(lexer);

    ASTNode* node = parser.parse();
    EXPECT_EQ(node, nullptr);
}

TEST(Parser, parse_guard_statement$missing_block) {
    const char* source =
    "func main() -> Void {                      \n"
    "   guard true, true else                   \n"
    "}";
    Lexer       lexer(source);
    Parser      parser(lexer);

    ASTNode* node = parser.parse();
    EXPECT_EQ(node, nullptr);
}

TEST(Parser, parse_if_statement) {
    const char* source =
    "func main() -> Void {  \n"
    "   if condition {}     \n"
    "}";
    Lexer       lexer(source);
    Parser      parser(lexer);

    ASTNode* node = parser.parse();
    EXPECT_NE(node, nullptr);
}

TEST(Parser, parse_if_statement$multiple_conditions) {
    const char* source =
    "func main() -> Void {                          \n"
    "   if condition0, condition1, condition2 {}    \n"
    "}";
    Lexer       lexer(source);
    Parser      parser(lexer);

    ASTNode* node = parser.parse();
    EXPECT_NE(node, nullptr);
}

TEST(Parser, parse_if_statement$else_block) {
    const char* source =
    "func main() -> Void {                          \n"
    "   if condition0, condition1, condition2 {}    \n"
    "   else {}                                     \n"
    "}";
    Lexer       lexer(source);
    Parser      parser(lexer);

    ASTNode* node = parser.parse();
    EXPECT_NE(node, nullptr);
}

TEST(Parser, parse_if_statement$else_if) {
    const char* source =
    "func main() -> Void {                          \n"
    "   if condition0, condition1, condition2 {}    \n"
    "   else if condition0, condition1 {}           \n"
    "}";
    Lexer       lexer(source);
    Parser      parser(lexer);

    ASTNode* node = parser.parse();
    EXPECT_NE(node, nullptr);
}

TEST(Parser, parse_if_statement$missing_delimiter) {
    const char* source =
    "func main() -> Void {                          \n"
    "   if condition0 condition1, condition2 {}     \n"
    "   else if condition0, condition1 {}           \n"
    "}";
    Lexer       lexer(source);
    Parser      parser(lexer);

    ASTNode* node = parser.parse();
    EXPECT_EQ(node, nullptr);
}

TEST(Parser, parse_if_statement$missing_block) {
    const char* source =
    "func main() -> Void {                          \n"
    "   if condition0, condition1, condition2       \n"
    "   else if condition0, condition1 {}           \n"
    "}";
    Lexer       lexer(source);
    Parser      parser(lexer);

    ASTNode* node = parser.parse();
    EXPECT_EQ(node, nullptr);
}

TEST(Parser, parse_if_statement$missing_else) {
    const char* source =
    "func main() -> Void {                          \n"
    "   if condition0, condition1, condition2 {}    \n"
    "   {}                                          \n"
    "}";
    Lexer       lexer(source);
    Parser      parser(lexer);

    ASTNode* node = parser.parse();
    EXPECT_EQ(node, nullptr);
}

TEST(Parser, parse_if_statement$missing_else_block) {
    const char* source =
    "func main() -> Void {                          \n"
    "   if condition0, condition1, condition2 {}    \n"
    "   else                                        \n"
    "}";
    Lexer       lexer(source);
    Parser      parser(lexer);

    ASTNode* node = parser.parse();
    EXPECT_EQ(node, nullptr);
}

TEST(Parser, parse_if_statement$missing_else_if_conditions) {
    const char* source =
    "func main() -> Void {                          \n"
    "   if condition0, condition1, condition2 {}    \n"
    "   else if {}                                  \n"
    "}";
    Lexer       lexer(source);
    Parser      parser(lexer);

    ASTNode* node = parser.parse();
    EXPECT_EQ(node, nullptr);
}

TEST(Parser, parse_switch_statement$no_cases) {
    const char* source =
    "func main() -> Void {  \n"
    "   switch expr { }     \n"
    "}";
    Lexer       lexer(source);
    Parser      parser(lexer);

    ASTNode* node = parser.parse();
    EXPECT_EQ(node, nullptr);
}

TEST(Parser, parse_switch_statement$with_case) {
    const char* source =
    "func main() -> Void {  \n"
    "   switch expr {       \n"
    "   case 0: return      \n"
    "   }                   \n"
    "}";
    Lexer       lexer(source);
    Parser      parser(lexer);

    ASTNode* node = parser.parse();
    EXPECT_NE(node, nullptr);
}

TEST(Parser, parse_switch_statement$with_multiple_cases) {
    const char* source =
    "func main() -> Void {  \n"
    "   switch expr {       \n"
    "   case 0: return      \n"
    "   case 1: return      \n"
    "   case 2: return      \n"
    "   }                   \n"
    "}";
    Lexer       lexer(source);
    Parser      parser(lexer);

    ASTNode* node = parser.parse();
    EXPECT_NE(node, nullptr);
}

TEST(Parser, parse_switch_statement$with_else) {
    const char* source =
    "func main() -> Void {  \n"
    "   switch expr {       \n"
    "   case 0: return      \n"
    "   case 1: return      \n"
    "   else:   return      \n"
    "   }                   \n"
    "}";
    Lexer       lexer(source);
    Parser      parser(lexer);

    ASTNode* node = parser.parse();
    EXPECT_NE(node, nullptr);
}

TEST(Parser, parse_switch_statement$missing_case_statement) {
    const char* source =
    "func main() -> Void {  \n"
    "   switch expr {       \n"
    "   case 0:             \n"
    "   case 1: return      \n"
    "   else:   return      \n"
    "   }                   \n"
    "}";
    Lexer       lexer(source);
    Parser      parser(lexer);

    ASTNode* node = parser.parse();
    EXPECT_EQ(node, nullptr);
}

TEST(Parser, parse_switch_statement$missing_expression) {
    const char* source =
    "func main() -> Void {  \n"
    "   switch {            \n"
    "   case 0:             \n"
    "   case 1: return      \n"
    "   else:   return      \n"
    "   }                   \n"
    "}";
    Lexer       lexer(source);
    Parser      parser(lexer);

    ASTNode* node = parser.parse();
    EXPECT_EQ(node, nullptr);
}

TEST(Parser, parse_switch_statement$missing_opening_brace) {
    const char* source =
    "func main() -> Void {  \n"
    "   switch expr         \n"
    "   case 0:             \n"
    "   case 1: return      \n"
    "   else:   return      \n"
    "   }                   \n"
    "}";
    Lexer       lexer(source);
    Parser      parser(lexer);

    ASTNode* node = parser.parse();
    EXPECT_EQ(node, nullptr);
}

TEST(Parser, parse_switch_statement$missing_closing_brace) {
    const char* source =
    "func main() -> Void {  \n"
    "   switch {            \n"
    "   case 0:             \n"
    "   case 1: return      \n"
    "   else:   return      \n"
    "   return              \n"
    "}";
    Lexer       lexer(source);
    Parser      parser(lexer);

    ASTNode* node = parser.parse();
    EXPECT_EQ(node, nullptr);
}

TEST(Parser, parse_switch_statement$missing_case_condition) {
    const char* source =
    "func main() -> Void {  \n"
    "   switch expr {       \n"
    "   case:   return      \n"
    "   case 1: return      \n"
    "   else:   return      \n"
    "   }                   \n"
    "}";
    Lexer       lexer(source);
    Parser      parser(lexer);

    ASTNode* node = parser.parse();
    EXPECT_EQ(node, nullptr);
}

TEST(Parser, parse_switch_statement$missing_case_delimiter) {
    const char* source =
    "func main() -> Void {  \n"
    "   switch expr {       \n"
    "   case 0  return      \n"
    "   case 1: return      \n"
    "   else:   return      \n"
    "   }                   \n"
    "}";
    Lexer       lexer(source);
    Parser      parser(lexer);

    ASTNode* node = parser.parse();
    EXPECT_EQ(node, nullptr);
}

TEST(Parser, parse_while_statement) {
    const char* source =
    "func main() -> Void {  \n"
    "   while true { }      \n"
    "}";
    Lexer       lexer(source);
    Parser      parser(lexer);

    ASTNode* node = parser.parse();
    EXPECT_NE(node, nullptr);
}

TEST(Parser, parse_while_statement$multiple_conditions) {
    const char* source =
    "func main() -> Void {                              \n"
    "   while condition0, condition1, condition2 { }    \n"
    "}";
    Lexer       lexer(source);
    Parser      parser(lexer);

    ASTNode* node = parser.parse();
    EXPECT_NE(node, nullptr);
}

TEST(Parser, parse_while_statement$missing_delimiter) {
    const char* source =
    "func main() -> Void {                             \n"
    "   while condition0, condition1 condition2 { }    \n"
    "}";
    Lexer       lexer(source);
    Parser      parser(lexer);

    ASTNode* node = parser.parse();
    EXPECT_EQ(node, nullptr);
}

TEST(Parser, parse_while_statement$missing_condition) {
    const char* source =
    "func main() -> Void {  \n"
    "   while { }           \n"
    "}";
    Lexer       lexer(source);
    Parser      parser(lexer);

    ASTNode* node = parser.parse();
    EXPECT_EQ(node, nullptr);
}
