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

#include "Core/ASTContext.h"

#include <unistd.h>

#include <llvm/Support/ErrorHandling.h>

static size_t allNodeSizes[] = {
    sizeof(ASTNode),
    sizeof(ASTStmt),
    sizeof(ASTExpr),
    sizeof(ASTDirective),
    sizeof(ASTDecl),
    sizeof(ASTUnaryExpr),
    sizeof(ASTBinaryExpr),
    sizeof(ASTMemberAccessExpr),
    sizeof(ASTIdentExpr),
    sizeof(ASTLit),
    sizeof(ASTNilLit),
    sizeof(ASTBoolLit),
    sizeof(ASTIntLit),
    sizeof(ASTFloatLit),
    sizeof(ASTStringLit),
    sizeof(ASTLoad),
    sizeof(ASTBlock),
    sizeof(ASTParamDecl),
    sizeof(ASTFuncDecl),
    sizeof(ASTPrefixFuncDecl),
    sizeof(ASTInfixFuncDecl),
    sizeof(ASTOpaqueDecl),
    sizeof(ASTVarDecl),
    sizeof(ASTLetDecl),
    sizeof(ASTStructDecl),
    sizeof(ASTEnumElementDecl),
    sizeof(ASTEnumDecl),
    sizeof(ASTCtrlStmt),
    sizeof(ASTBreakStmt),
    sizeof(ASTContinueStmt),
    sizeof(ASTFallthroughStmt),
    sizeof(ASTReturnStmt),
    sizeof(ASTDeferStmt),
    sizeof(ASTForStmt),
    sizeof(ASTBranchStmt),
    sizeof(ASTGuardStmt),
    sizeof(ASTIfStmt),
    sizeof(ASTLoopStmt),
    sizeof(ASTDoStmt),
    sizeof(ASTWhileStmt),
    sizeof(ASTCaseStmt),
    sizeof(ASTSwitchStmt),
    sizeof(ASTCallExpr),
    sizeof(ASTSubscriptExpr),
    sizeof(ASTTypeRef),
    sizeof(ASTAnyTypeRef),
    sizeof(ASTOpaqueTypeRef),
    sizeof(ASTTypeOfTypeRef),
    sizeof(ASTPointerTypeRef),
    sizeof(ASTArrayTypeRef)
};

ASTContext::ASTContext() {
    size_t containerSize = allNodeSizes[0];
    for (auto i = 1; i < sizeof(allNodeSizes) / sizeof(size_t); i++) {
        containerSize = std::max(containerSize, allNodeSizes[i]);
    }

    pageSize = sysconf(_SC_PAGESIZE);
    nodeSize = 1;
    while (nodeSize < containerSize) { nodeSize *= 2; }
    nodeCount = 0;
    nodesPerPage = pageSize / nodeSize;

    while (nodesPerPage <= 0) {
        pageSize *= 2;
        nodesPerPage = pageSize / nodeSize;
    }

    void* buffer = malloc(pageSize);
    if (buffer == nullptr) {
        llvm::report_bad_alloc_error("Memory allocation failed!");
    }

    nodePages.push_back((uint8_t*)buffer);

    root = new (this) ASTBlock;
    root->scope.kind = SCOPE_GLOBAL;

    typeError = ErrorType();
    typeAny = AnyType();
    typeVoid = VoidType();
    typeBool = BoolType();
    typeUInt8 = IntegerType(false, true, false, 8);
    typeUInt16 = IntegerType(false, true, false, 16);
    typeUInt32 = IntegerType(false, true, false, 32);
    typeUInt64 = IntegerType(false, true, false, 64);
    typeInt8 = IntegerType(true, true, false, 8);
    typeInt16 = IntegerType(true, true, false, 16);
    typeInt32 = IntegerType(true, true, false, 32);
    typeInt64 = IntegerType(true, true, false, 64);
    typeFloat16 = FloatType(FLOAT_IEEE16);
    typeFloat32 = FloatType(FLOAT_IEEE32);
    typeFloat64 = FloatType(FLOAT_IEEE64);
    typeFloat80 = FloatType(FLOAT_IEEE80);
    typeFloat128 = FloatType(FLOAT_IEEE128);
    typeString = StringType();
    typeAnyPointer = PointerType(1, &typeAny);

    types.try_emplace("Any", &typeAny);
    types.try_emplace("Void", &typeVoid);
    types.try_emplace("Bool", &typeBool);
    types.try_emplace("UInt8", &typeUInt8);
    types.try_emplace("UInt16", &typeUInt16);
    types.try_emplace("UInt32", &typeUInt32);
    types.try_emplace("UInt64", &typeUInt64);
    types.try_emplace("UInt", &typeUInt64);
    types.try_emplace("Int8", &typeInt8);
    types.try_emplace("Int16", &typeInt16);
    types.try_emplace("Int32", &typeInt32);
    types.try_emplace("Int64", &typeInt64);
    types.try_emplace("Int", &typeInt64);
    types.try_emplace("Float16", &typeFloat16);
    types.try_emplace("Float32", &typeFloat32);
    types.try_emplace("Float64", &typeFloat64);
    types.try_emplace("Float80", &typeFloat80);
    types.try_emplace("Float128", &typeFloat128);
    types.try_emplace("Float", &typeFloat64);
    types.try_emplace("String", &typeString);
}

ASTContext::~ASTContext() {
    for (auto key : lexemeValues) {
        allocator.Deallocate(key.begin(), key.size());
    }

    for (auto it = nodePages.begin(); it != nodePages.end(); it++) {
        free(*it);
    }
}

void* ASTContext::allocNode()  {
    size_t pageIndex = nodeCount / nodesPerPage;
    size_t nodeIndex = nodeCount - pageIndex * nodesPerPage;

    if (pageIndex >= nodePages.size()) {
        void* buffer = malloc(pageSize);
        if (buffer == nullptr) {
            llvm::report_bad_alloc_error("Memory allocation failed!");
        }

        nodePages.push_back(buffer);
    }

    uint8_t* buffer = (uint8_t*)nodePages[pageIndex];
    uint8_t* pointer = buffer + nodeIndex * nodeSize;

    nodeCount += 1;
    return pointer;
}

Lexeme ASTContext::getLexeme(llvm::StringRef text) {
    Lexeme lexeme;
    lexeme.index = lexemeMap.lookup(text);

    if (lexeme.index > 0) {
        lexeme.text = lexemeValues[lexeme.index - 1];
        return lexeme;
    }

    lexeme.text = text.copy(allocator);
    lexeme.index = lexemeValues.size() + 1;
    lexemeValues.push_back(lexeme.text);
    lexemeMap.try_emplace(lexeme.text, lexeme.index);
    return lexeme;
}

ASTBlock* ASTContext::getRoot() {
    return root;
}

llvm::SmallVector<FuncType*, 0>* ASTContext::getBuiltinFuncTypes() {
    return &builtinFuncTypes;
}

Type* ASTContext::getErrorType() {
    return &typeError;
}

Type* ASTContext::getAnyType() {
    return &typeAny;
}

Type* ASTContext::getVoidType() {
    return &typeVoid;
}

Type* ASTContext::getBoolType() {
    return &typeBool;
}

Type* ASTContext::getUInt8Type() {
    return &typeUInt8;
}

Type* ASTContext::getUInt16Type() {
    return &typeUInt16;
}

Type* ASTContext::getUInt32Type() {
    return &typeUInt32;
}

Type* ASTContext::getUInt64Type() {
    return &typeUInt64;
}

Type* ASTContext::getUIntType() {
    return getUInt64Type();
}

Type* ASTContext::getInt8Type() {
    return &typeInt8;
}

Type* ASTContext::getInt16Type() {
    return &typeInt16;
}

Type* ASTContext::getInt32Type() {
    return &typeInt32;
}

Type* ASTContext::getInt64Type() {
    return &typeInt64;
}

Type* ASTContext::getIntType() {
    return getInt64Type();
}

Type* ASTContext::getFloat16Type() {
    return &typeFloat16;
}

Type* ASTContext::getFloat32Type() {
    return &typeFloat32;
}

Type* ASTContext::getFloat64Type() {
    return &typeFloat64;
}

Type* ASTContext::getFloat80Type() {
    return &typeFloat80;
}

Type* ASTContext::getFloat128Type() {
    return &typeFloat128;
}

Type* ASTContext::getFloatType() {
    return getFloat64Type();
}

Type* ASTContext::getStringType() {
    return &typeString;
}

Type* ASTContext::getAnyPointerType() {
    return &typeAnyPointer;
}

Type* ASTContext::getEnumType(ASTEnumDecl* decl) {
    // @Incomplete build a unique type
    auto type = new EnumType;
    type->name = decl->name.text;
    return type;
}

Type* ASTContext::getPointerType(Type* pointeeType, uint64_t depth) {
    assert(depth > 0 && "Cannot create a ASTPointerType with depth smaller than 1!");

    if (pointeeType->kind == TYPE_BUILTIN_POINTER) {
        auto type = reinterpret_cast<PointerType*>(pointeeType);
        return getPointerType(type->pointeeType, depth + 1);
    }

    // @Incomplete build a unique type
    auto type = new PointerType(depth, pointeeType);
    return type;
}

Type* ASTContext::getStaticArrayType(Type* elementType, llvm::APInt size) {
    // @Incomplete build a unique type
    auto type = new ArrayType;
    type->isStatic = true;
    type->size = size;
    type->elementType = elementType;
    return type;
}

Type* ASTContext::getDynamicArrayType(Type* elementType) {
    // @Incomplete build a unique type
    auto type = new ArrayType;
    type->isStatic = false;
    type->elementType = elementType;
    return type;
}

Type* ASTContext::getFuncType(ASTFuncDecl* decl) {
    // @Incomplete build a unique type
    auto type = new FuncType;
    type->name = decl->name.text;
    for (auto param : decl->params) {
        type->paramTypes.push_back(param->type);
    }

    type->returnType = decl->returnTypeRef->type;
    return type;
}

Type* ASTContext::getStructType(llvm::StringRef name, llvm::StringMap<Type*> memberTypes) {
    // @Incomplete build a unique type
    auto type = new StructType;
    type->name = name; // @Refactor pass only a ASTStructDecl as parameter, this will also give a ASTContext stored Name
    type->memberTypes = memberTypes;
    return type;
}

Type* ASTContext::findTypeByName(llvm::StringRef name) {
    auto it = types.find(name);
    if (it != types.end()) {
        return it->getValue();
    }
    return nullptr;
}
