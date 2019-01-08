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

#include "Core/AST.h"
#include "Core/ASTContext.h"

void* ASTNode::operator new (size_t size, ASTContext* context) {
    auto memory = context->nodeAllocator.Allocate(size, 8);
    context->nodes.push_back(reinterpret_cast<ASTNode*>(memory));
    return memory;
}

bool ASTNode::isDecl() const {
    return
    kind == AST_LOAD_DIRECTIVE || 
    kind == AST_PARAM_DECL ||
    kind == AST_FUNC_DECL ||
    kind == AST_VALUE_DECL ||
    kind == AST_STRUCT_DECL ||
    kind == AST_ENUM_ELEMENT_DECL ||
    kind == AST_ENUM_DECL;
}

ASTCompoundStmt::ASTCompoundStmt(ASTContext* context, llvm::ArrayRef<ASTStmt*> stmts) : ASTStmt(AST_COMPOUND_STMT) {
    this->stmts = stmts.copy(context->nodeAllocator);
}

ASTFuncDecl::ASTFuncDecl(ASTContext* context, llvm::ArrayRef<ASTParamDecl*> parameters) :
ASTNamedDecl(AST_FUNC_DECL),
DeclContext(AST_FUNC_DECL) {
    this->parameters = parameters.copy(context->nodeAllocator);
}


ASTSwitchStmt::ASTSwitchStmt(ASTContext* context, llvm::ArrayRef<ASTCaseStmt*> cases) : ASTStmt(AST_SWITCH) {
    this->cases = cases.copy(context->nodeAllocator);
}

ASTCallExpr::ASTCallExpr(ASTContext* context, llvm::ArrayRef<ASTExpr*> args) : ASTExpr(AST_CALL) {
    this->args = args.copy(context->nodeAllocator);
}

ASTSubscriptExpr::ASTSubscriptExpr(ASTContext* context, llvm::ArrayRef<ASTExpr*> arguments) : ASTExpr(AST_SUBSCRIPT) {
    this->args = arguments.copy(context->nodeAllocator);
}

ASTLoadDirective::ASTLoadDirective(ASTContext* context, llvm::StringRef loadFilePath) : ASTDecl(AST_LOAD_DIRECTIVE) {
    this->loadFilePath = loadFilePath.copy(context->nodeAllocator);
}
