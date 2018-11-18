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

#include "AST/ASTNodes.h"
#include "AST/ASTContext.h"

void* ASTNode::operator new (size_t size, ASTContext* context) {
    return context->alloc_node();
}

bool ASTType::is_incomplete_type() const {
    if (is_placeholder_type()) {
        return true;
    }

    if (is_pointer_type()) {
        auto pointer_type = reinterpret_cast<const ASTPointerType*>(this);
        return pointer_type->pointee_type->is_incomplete_type();
    }

    if (is_array_type()) {
        auto array_type = reinterpret_cast<const ASTArrayType*>(this);
        return array_type->element_type->is_incomplete_type();
    }

    if (is_enum_type()) {
        // TODO: Check if any member is incomplete
        return false;
    }

    if (is_func_type()) {
        auto func_type = reinterpret_cast<const ASTFuncType*>(this);

        for (auto parameter_type : func_type->parameter_types) {
            if (parameter_type->is_incomplete_type()) {
                return true;
            }
        }

        return func_type->return_type->is_incomplete_type();
    }

    if (is_struct_type()) {
        auto struct_type = reinterpret_cast<const ASTStructType*>(this);

        for (auto member_type : struct_type->member_types) {
            if (member_type->is_incomplete_type()) {
                return true;
            }
        }

        return false;
    }

    return (
        is_builtin_type() ||
        is_unresolved_type() ||
        is_error_type()
    );
}

uint32_t ASTFloatType::bit_width() const {
    switch (float_kind) {
        case AST_FLOAT_IEEE16:  return 16;
        case AST_FLOAT_IEEE32:  return 32;
        case AST_FLOAT_IEEE64:  return 64;
        case AST_FLOAT_IEEE80:  return 80;
        case AST_FLOAT_IEEE128: return 128;
        case AST_FLOAT_PPC128:  return 128;
    }
}
