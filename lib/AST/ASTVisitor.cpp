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

#include "AST/ASTVisitor.h"
#include "AST/ASTNodes.h"

void ASTVisitor::pre_visit_node(ASTNode* node) {
}

void ASTVisitor::post_visit_node(ASTNode* node) {
}

void ASTVisitor::visit_node(ASTNode* node) {
    assert(node);

    pre_visit_node(node);
    defer(post_visit_node(node));

    if (node->is_expression()) {
        auto expr = reinterpret_cast<ASTExpression*>(node);
        if (expr->substitution) {
            return visit_node(expr->substitution);
        }
    }

    switch (node->kind) {
        case AST_LOAD:
            return visit_load(reinterpret_cast<ASTLoad*>(node));

        case AST_LITERAL:
            return visit_literal(reinterpret_cast<ASTLiteral*>(node));

        case AST_FUNC:
            return visit_func(reinterpret_cast<ASTFunc*>(node));

        case AST_FUNC_SIGNATURE:
            return visit_func_signature(reinterpret_cast<ASTFuncSignature*>(node));

        case AST_BLOCK:
            return visit_block(reinterpret_cast<ASTBlock*>(node));

        case AST_PARAMETER:
            return visit_parameter(reinterpret_cast<ASTParameter*>(node));

        case AST_STRUCT:
            return visit_struct(reinterpret_cast<ASTStruct*>(node));

        case AST_VARIABLE:
            return visit_variable(reinterpret_cast<ASTVariable*>(node));

        case AST_ENUM:
            return visit_enum(reinterpret_cast<ASTEnum*>(node));

        case AST_ENUM_ELEMENT:
            return visit_enum_element(reinterpret_cast<ASTEnumElement*>(node));

        case AST_IDENTIFIER:
            return visit_identifier(reinterpret_cast<ASTIdentifier*>(node));

        case AST_UNARY:
            return visit_unary(reinterpret_cast<ASTUnaryExpression*>(node));

        case AST_BINARY:
            return visit_binary(reinterpret_cast<ASTBinaryExpression*>(node));

        case AST_CONTROL:
            return visit_control(reinterpret_cast<ASTControl*>(node));

        case AST_DEFER:
            return visit_defer(reinterpret_cast<ASTDefer*>(node));

        case AST_FOR:
            return visit_for(reinterpret_cast<ASTFor*>(node));

        case AST_GUARD:
            return visit_guard(reinterpret_cast<ASTGuard*>(node));

        case AST_IF:
            return visit_if(reinterpret_cast<ASTIf*>(node));

        case AST_SWITCH:
            return visit_switch(reinterpret_cast<ASTSwitch*>(node));

        case AST_SWITCH_CASE:
            return visit_switch_case(reinterpret_cast<ASTSwitchCase*>(node));

        case AST_LOOP:
            return visit_loop(reinterpret_cast<ASTLoop*>(node));

        case AST_CALL:
            return visit_call(reinterpret_cast<ASTCall*>(node));

        case AST_SUBSCRIPT:
            return visit_subscript(reinterpret_cast<ASTSubscript*>(node));

        case AST_TYPE:
            return visit_type(reinterpret_cast<ASTType*>(node));

        default: break;
    }
}

void ASTVisitor::visit_load(ASTLoad* node) {
    visit_children(node);
}

void ASTVisitor::visit_literal(ASTLiteral* node) {
    visit_children(node);
}

void ASTVisitor::visit_func(ASTFunc* node) {
    visit_children(node);
}

void ASTVisitor::visit_func_signature(ASTFuncSignature* node) {
    visit_children(node);
}

void ASTVisitor::visit_block(ASTBlock* node) {
    visit_children(node);
}

void ASTVisitor::visit_parameter(ASTParameter* node) {
    visit_children(node);
}

void ASTVisitor::visit_struct(ASTStruct* node) {
    visit_children(node);
}

void ASTVisitor::visit_variable(ASTVariable* node) {
    visit_children(node);
}

void ASTVisitor::visit_enum(ASTEnum* node) {
    visit_children(node);
}

void ASTVisitor::visit_enum_element(ASTEnumElement* node) {
    visit_children(node);
}

void ASTVisitor::visit_identifier(ASTIdentifier* node) {
    visit_children(node);
}

void ASTVisitor::visit_unary(ASTUnaryExpression* node) {
    visit_children(node);
}

void ASTVisitor::visit_binary(ASTBinaryExpression* node) {
    visit_children(node);
}

void ASTVisitor::visit_control(ASTControl* node) {
    visit_children(node);
}

void ASTVisitor::visit_defer(ASTDefer* node) {
    visit_children(node);
}

void ASTVisitor::visit_for(ASTFor* node) {
    visit_children(node);
}

void ASTVisitor::visit_guard(ASTGuard* node) {
    visit_children(node);
}

void ASTVisitor::visit_if(ASTIf* node) {
    visit_children(node);
}

void ASTVisitor::visit_switch(ASTSwitch* node) {
    visit_children(node);
}

void ASTVisitor::visit_switch_case(ASTSwitchCase* node) {
    visit_children(node);
}

void ASTVisitor::visit_loop(ASTLoop* node) {
    visit_children(node);
}

void ASTVisitor::visit_call(ASTCall* node) {
    visit_children(node);
}

void ASTVisitor::visit_subscript(ASTSubscript* node) {
    visit_children(node);
}

void ASTVisitor::visit_type(ASTType* node) {
    visit_children(node);
}

void ASTVisitor::visit_children(ASTNode* node) {
    switch (node->kind) {
        case AST_LOAD: {
            auto load = reinterpret_cast<ASTLoad*>(node);
            visit_node(load->literal);
        }   break;

        case AST_FUNC: {
            auto func = reinterpret_cast<ASTFunc*>(node);
            visit_node(func->name);
            visit_node(func->signature);
            visit_node(func->block);
        }   break;

        case AST_FUNC_SIGNATURE: {
            auto signature = reinterpret_cast<ASTFuncSignature*>(node);
            for (auto parameter : signature->parameters) {
                visit_node(parameter);
            }
            visit_node(signature->return_type);
        }   break;

        case AST_BLOCK: {
            auto block = reinterpret_cast<ASTBlock*>(node);
            for (auto statement : block->statements) {
                visit_node(statement);
            }
        }   break;

        case AST_PARAMETER: {
            auto parameter = reinterpret_cast<ASTParameter*>(node);
            visit_node(parameter->name);
            visit_node(parameter->type);
        }   break;

        case AST_STRUCT: {
            auto structure = reinterpret_cast<ASTStruct*>(node);
            visit_node(structure->name);
            visit_node(structure->block);
        }   break;

        case AST_VARIABLE: {
            auto variable = reinterpret_cast<ASTVariable*>(node);
            visit_node(variable->name);
            visit_node(variable->type);
            if (variable->assignment) {
                visit_node(variable->assignment);
            }
        }   break;

        case AST_ENUM: {
            auto enumeration = reinterpret_cast<ASTEnum*>(node);
            visit_node(enumeration->name);
            visit_node(enumeration->block);
        }   break;

        case AST_ENUM_ELEMENT: {
            auto element = reinterpret_cast<ASTEnumElement*>(node);
            visit_node(element->name);
            if (element->assignment) {
                visit_node(element->assignment);
            }
        }   break;

        case AST_UNARY: {
            auto unary = reinterpret_cast<ASTUnaryExpression*>(node);
            visit_node(unary->op_identifier);
            visit_node(unary->right);
        }   break;

        case AST_BINARY: {
            auto binary = reinterpret_cast<ASTBinaryExpression*>(node);
            visit_node(binary->op_identifier);
            visit_node(binary->left);
            visit_node(binary->right);
        }   break;

        case AST_CONTROL: {
            auto control = reinterpret_cast<ASTControl*>(node);
            if (control->expression) {
                visit_node(control->expression);
            }
        }   break;

        case AST_DEFER: {
            auto defer = reinterpret_cast<ASTDefer*>(node);
            visit_node(defer->expression);
        }   break;

        case AST_FOR: {
            auto for_stmt = reinterpret_cast<ASTFor*>(node);
            visit_node(for_stmt->iterator);
            visit_node(for_stmt->sequence);
            visit_node(for_stmt->block);
        }   break;

        case AST_GUARD: {
            auto guard = reinterpret_cast<ASTGuard*>(node);
            for (auto condition : guard->conditions) {
                visit_node(condition);
            }
            visit_node(guard->else_block);
        }   break;

        case AST_IF: {
            auto if_stmt = reinterpret_cast<ASTIf*>(node);
            for (auto condition : if_stmt->conditions) {
                visit_node(condition);
            }
            visit_node(if_stmt->block);
            switch (if_stmt->kind) {
                case AST_IF_ELSE: {
                    visit_node(if_stmt->else_block);
                }   break;

                case AST_IF_ELSE_IF: {
                    visit_node(if_stmt->else_if);
                }   break;

                default: break;
            }
        }   break;

        case AST_SWITCH: {
            auto switch_stmt = reinterpret_cast<ASTSwitch*>(node);
            visit_node(switch_stmt->expression);
            for (auto switch_case : switch_stmt->cases) {
                visit_node(switch_case);
            }
        }   break;

        case AST_SWITCH_CASE: {
            auto switch_case = reinterpret_cast<ASTSwitchCase*>(node);
            if (switch_case->case_kind == AST_SWITCH_CASE_CONDITION) {
                visit_node(switch_case->condition);
            }
            visit_node(switch_case->block);
        }   break;

        case AST_LOOP: {
            auto loop = reinterpret_cast<ASTLoop*>(node);
            for (auto condition : loop->conditions) {
                visit_node(condition);
            }
            visit_node(loop->block);
        }   break;

        case AST_CALL: {
            auto call = reinterpret_cast<ASTCall*>(node);
            visit_node(call->left);
            for (auto argument : call->arguments) {
                visit_node(argument);
            }
        }   break;

        case AST_SUBSCRIPT: {
            auto subscript = reinterpret_cast<ASTSubscript*>(node);
            visit_node(subscript->left);
            for (auto argument : subscript->arguments) {
                visit_node(argument);
            }
        }

        case AST_TYPE: {
            auto type = reinterpret_cast<ASTType*>(node);
            switch (type->type_kind) {
                case AST_TYPE_BUILTIN_POINTER: {
                    auto pointer = reinterpret_cast<ASTPointerType*>(node);
                    visit_node(pointer->pointee_type);
                }   break;

                case AST_TYPE_BUILTIN_ARRAY: {
                    auto array = reinterpret_cast<ASTArrayType*>(node);
                    visit_node(array->element_type);
                }   break;

                case AST_TYPE_PLACEHOLDER_TYPEOF: {
                    auto type_of = reinterpret_cast<ASTTypeOfType*>(node);
                    visit_node(type_of->expr);
                }   break;

                case AST_TYPE_PLACEHOLDER_OPAQUE: {
                    auto opaque = reinterpret_cast<ASTOpaqueType*>(node);
                    visit_node(opaque->identifier);
                }   break;

                case AST_TYPE_DECL_FUNC: {
                    auto func = reinterpret_cast<ASTFuncType*>(node);
                    for (auto parameter_type : func->parameter_types) {
                        visit_node(parameter_type);
                    }
                    visit_node(func->return_type);
                }   break;

                case AST_TYPE_DECL_STRUCT: {
                    auto struct_type = reinterpret_cast<ASTStructType*>(node);
                    for (auto member_type : struct_type->member_types) {
                        visit_node(member_type);
                    }
                }   break;

                default: break;
            }
        }   break;

        default: break;
    }
}
