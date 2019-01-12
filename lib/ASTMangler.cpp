//
// MIT License
//
// Copyright (c) 2019 Murat Yilmaz
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

#include "Core/AST.h"
#include "Core/ASTMangler.h"

/* Builtin Type Encoding:
 *
 * <prefix>   = 'B'
 * Any        = <prefix> , 'G'
 * Void       = <prefix> , 'V'
 * Bool       = <prefix> , 'B'
 * UInt       = <prefix> , 'U' , <width>
 * Int        = <prefix> , 'I' , <width>
 * Float      = <prefix> , 'F' , <width>
 * String     = <prefix> , 'S'
 * Pointer<T> = <prefix> , 'P' , <depth> , <T>
 * Array<T>   = <prefix> , 'A' , optional <size> , <T>
 * Operation  = <prefix> , 'O' , <operation>
 * Function   = <prefix> , 'C' , <name>
 * PrefixFunc = <prefix> , 'K' , <func>
 * InfixFunc  = <prefix> , 'J' , <func>
 *
 * Builtin Operation Encoding:
 *
 * "="        = 'A'
 *
 * Type Encoding:
 *
 * <prefix>   = 'G'
 * Enum       = <prefix> , 'E' , <name>
 * Func       = <prefix> , 'F' , <name> , optional <parameter-types ...> , <return-type>
 * Struct     = <prefix> , 'S' , <name>
 *
 * String Encoding:
 *
 * <string>   = <size> , <data>
 *
 * Named Declaration Encoding:
 *
 * <prefix>    = '_J'
 * Func        = <prefix> , <parent> , 'F' , <func-type>
 * Struct      = <prefix> , <parent> , 'S' , <struct-type>
 * Enum        = <prefix> , <parent> , 'E' , <enum-type>
 * EnumElement = <prefix> , <parent> , 'C' , <name>
 * Module      = <prefix> , <parent> , 'M' , <name>
 */

std::string ASTMangler::getDeclKindPrefix(ASTNamedDecl* decl) {
    switch (decl->kind) {
        case AST_FUNC_DECL:         return "F";
        case AST_STRUCT_DECL:       return "S";
        case AST_ENUM_DECL:         return "E";
        case AST_ENUM_ELEMENT_DECL: return "C";
        case AST_MODULE_DECL:       return "M";
        default:                    llvm_unreachable("Decl isn't supported by ASTMangler!");
    }
}

std::string ASTMangler::getMangledDeclName(ASTNamedDecl* decl) {
    std::string buffer;

    if (decl->parent && decl->parent->isNamedDecl()) {
        auto parentDecl = reinterpret_cast<ASTNamedDecl*>(decl->parent);
        buffer.append(getMangledDeclName(parentDecl));
    }

    buffer.append(getDeclKindPrefix(decl));

    switch (decl->kind) {
        case AST_FUNC_DECL:
        case AST_STRUCT_DECL:
        case AST_ENUM_DECL:
            buffer.append(mangleType(decl->type));
            break;

        case AST_ENUM_ELEMENT_DECL:
        case AST_MODULE_DECL:
            buffer.append(std::to_string(decl->name->size()));
            buffer.append(decl->name->str());
            break;

        default:
            llvm_unreachable("Decl isn't supported by ASTMangler!");
    }

    return buffer;
}

std::string ASTMangler::mangleDecl(ASTNamedDecl* decl) {
    std::string buffer("_J");
    buffer.append(getMangledDeclName(decl));
    return buffer;
}

std::string ASTMangler::getTypeKindPrefix(TypeKind kind) {
    switch (kind) {
        case TYPE_BUILTIN_ANY:
        case TYPE_BUILTIN_VOID:
        case TYPE_BUILTIN_BOOL:
        case TYPE_BUILTIN_INT:
        case TYPE_BUILTIN_FLOAT:
        case TYPE_BUILTIN_STRING:
        case TYPE_BUILTIN_POINTER:
        case TYPE_BUILTIN_ARRAY:
        case TYPE_BUILTIN_OPERATION:
        case TYPE_BUILTIN_FUNC:
        case TYPE_BUILTIN_PREFIX_FUNC:
        case TYPE_BUILTIN_INFIX_FUNC:
            return "B";

        case TYPE_DECL_ENUM:
        case TYPE_DECL_FUNC:
        case TYPE_DECL_STRUCT:
            return "G";

        default:
            llvm_unreachable("Type isn't supported by ASTMangler!");
    }
}

std::string ASTMangler::mangleType(Type *type) {
    std::string buffer(getTypeKindPrefix(type->kind));

    switch (type->kind) {
        case TYPE_BUILTIN_ANY:  buffer.append("G"); break;
        case TYPE_BUILTIN_VOID: buffer.append("V"); break;
        case TYPE_BUILTIN_BOOL: buffer.append("B"); break;
        case TYPE_BUILTIN_INT: {
            auto intType = reinterpret_cast<IntegerType*>(type);
            buffer.append(intType->isSigned ? "I" : "U");
            buffer.append(std::to_string(intType->fixedWidth));
        }   break;

        case TYPE_BUILTIN_FLOAT: {
            auto floatType = reinterpret_cast<FloatType*>(type);
            buffer.append("F");
            buffer.append(std::to_string(floatType->bitWidth()));
        }   break;

        case TYPE_BUILTIN_STRING: buffer.append("S"); break;
        case TYPE_BUILTIN_POINTER: {
            auto pointerType = reinterpret_cast<PointerType*>(type);
            buffer.append("P");
            buffer.append(std::to_string(pointerType->depth));
            buffer.append(mangleType(pointerType->pointeeType));
        }   break;

        case TYPE_BUILTIN_ARRAY: {
            auto arrayType = reinterpret_cast<ArrayType*>(type);
            buffer.append("A");
            if (arrayType->isStatic) {
                buffer.append(arrayType->size.toString(10, false));
            }
            buffer.append(mangleType(arrayType->elementType));
        }   break;

        case TYPE_BUILTIN_OPERATION: {
            auto opType = reinterpret_cast<BuiltinOperationType*>(type);
            buffer.append("O");
            switch (opType->op) {
                case BUILTIN_ASSIGNMENT_OPERATION: buffer.append("A"); break;
                default: llvm_unreachable("Builtin operation is not supported by ASTMangler!");
            }
            buffer.append(std::to_string(opType->op));
        }   break;

        case TYPE_BUILTIN_FUNC: {
            auto funcType = reinterpret_cast<BuiltinFuncType*>(type);
            buffer.append("C");
            buffer.append(std::to_string(funcType->name.size()));
            buffer.append(funcType->name.str());
        }   break;

        case TYPE_BUILTIN_PREFIX_FUNC: {
            auto funcType = reinterpret_cast<BuiltinPrefixFuncType*>(type);
            buffer.append("K");
            buffer.append(std::to_string(funcType->name.size()));
            buffer.append(funcType->name.str());
        }   break;

        case TYPE_BUILTIN_INFIX_FUNC: {
            auto funcType = reinterpret_cast<BuiltinInfixFuncType*>(type);
            buffer.append("J");
            buffer.append(std::to_string(funcType->name.size()));
            buffer.append(funcType->name.str());
        }   break;

        case TYPE_DECL_ENUM: {
            auto enumType = reinterpret_cast<EnumType*>(type);
            buffer.append("E");
            buffer.append(std::to_string(enumType->name.size()));
            buffer.append(enumType->name.str());
        }   break;

        case TYPE_DECL_FUNC: {
            auto funcType = reinterpret_cast<FuncType*>(type);
            buffer.append("F");
            buffer.append(std::to_string(funcType->name.size()));
            buffer.append(funcType->name.str());

            for (auto paramType : funcType->paramTypes) {
                buffer.append(mangleType(paramType));
            }

            buffer.append(mangleType(funcType->returnType));
        }   break;

        case TYPE_DECL_STRUCT: {
            auto structType = reinterpret_cast<StructType*>(type);
            buffer.append("S");
            buffer.append(std::to_string(structType->name.size()));
            buffer.append(structType->name.str());
        }   break;

        default:
            llvm_unreachable("Type isn't supported by ASTMangler!");
    }

    return buffer;
}
