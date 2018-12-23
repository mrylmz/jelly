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

#include "Core/Type.h"

bool Type::isIncomplete() const {
    switch (kind) {
        case TYPE_ERROR:
        case TYPE_BUILTIN_VOID:
            return true;

        case TYPE_BUILTIN_ANY:
        case TYPE_BUILTIN_BOOL:
        case TYPE_BUILTIN_INT:
        case TYPE_BUILTIN_FLOAT:
        case TYPE_BUILTIN_STRING:
            return false;

        case TYPE_BUILTIN_POINTER: {
            auto PointerTy = reinterpret_cast<const PointerType*>(this);
            return PointerTy->pointeeType->isIncomplete();
        }

        case TYPE_BUILTIN_ARRAY: {
            auto ArrayTy = reinterpret_cast<const ArrayType*>(this);
            return ArrayTy->elementType->isIncomplete();
        }

        case TYPE_BUILTIN_FUNC_CAST:
            llvm::report_fatal_error("Implementation missing!");

        case TYPE_DECL_ENUM:
            return false;

        case TYPE_DECL_FUNC: {
            auto FuncTy = reinterpret_cast<const FuncType*>(this);
            for (auto ParamTy : FuncTy->paramTypes) {
                if (ParamTy->isIncomplete()) {
                    return true;
                }
            }
            return false;
        }

        case TYPE_DECL_STRUCT: {
            auto StructTy = reinterpret_cast<const StructType*>(this);
            for (auto It = StructTy->memberTypes.begin(); It != StructTy->memberTypes.end(); It++) {
                if (It->getValue()->isIncomplete()) {
                    return true;
                }
            }
            return false;
        }
    }
}

unsigned FloatType::bitWidth() const {
    switch (floatKind) {
        case FLOAT_IEEE16:  return 16;
        case FLOAT_IEEE32:  return 32;
        case FLOAT_IEEE64:  return 64;
        case FLOAT_IEEE80:  return 80;
        case FLOAT_IEEE128: return 128;
        case FLOAT_PPC128:  return 128;
    }
}
