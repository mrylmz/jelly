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

#include "Core/CodeManager.h"
#include "Core/Defer.h"
#include "Core/Sema.h"
#include "Core/Type.h"

#include <llvm/ADT/SetVector.h>

// @Stability Add a strict pattern if all invalid nodes will get the ErrorType assigned or not (See TypeResolution)
// @Incomplete Add value categories for assignments see like (prvalue, xvalue, and lvalue) in c++

Sema::Sema(CodeManager* codeManager) : codeManager(codeManager), context(&codeManager->context), diag(&codeManager->diag) {
}

void Sema::validateAST() {
    inferTypeOfNode(context->getRoot());
    typeCheckNode(context->getRoot());
}

// @Incomplete if inner expressions contain CandidateTypes
//             then outer infer func has to reduce those and report errors if neccessary
void Sema::resolveType(ASTTypeRef* typeRef) {
    if (typeRef->type) { return; }

    switch (typeRef->kind) {
        case AST_ANY_TYPE_REF:
            typeRef->type = context->getAnyType();
            return;

        case AST_OPAQUE_TYPE_REF: {
            auto opaqueTypeRef = reinterpret_cast<ASTOpaqueTypeRef*>(typeRef);

            // @Refactor Decl resolution is also used on other places extract it to a function ...
            if (!opaqueTypeRef->decl) {
                auto block = opaqueTypeRef->getParentBlock();
                while (block) {
                    auto it = block->decls.find(opaqueTypeRef->typeName.text);
                    if (it != block->decls.end()) {
                        opaqueTypeRef->decl = it->getValue();
                        break;
                    }

                    block = block->getParentBlock();
                }
            }

            if (opaqueTypeRef->decl) {
                inferTypeOfNode(opaqueTypeRef->decl);
                opaqueTypeRef->type = opaqueTypeRef->decl->type;
                return;
            }

            opaqueTypeRef->type = context->findTypeByName(opaqueTypeRef->typeName.text);
            if (opaqueTypeRef->type) {
                return;
            }

            opaqueTypeRef->type = context->getErrorType();
            diag->report(DIAG_ERROR, "Couldn't resolve type '{0}'", opaqueTypeRef->typeName.text);
        }   return;

        case AST_TYPEOF_TYPE_REF: {
            auto typeOfTypeRef = reinterpret_cast<ASTTypeOfTypeRef*>(typeRef);
            typeCheckExpr(typeOfTypeRef->expr);
            typeOfTypeRef->type = typeOfTypeRef->expr->type;
        } return;

        case AST_POINTER_TYPE_REF: {
            auto pointerTypeRef = reinterpret_cast<ASTPointerTypeRef*>(typeRef);
            resolveType(pointerTypeRef->pointeeTypeRef);
            pointerTypeRef->type = context->getPointerType(pointerTypeRef->pointeeTypeRef->type, pointerTypeRef->depth);
        }   return;

        case AST_ARRAY_TYPE_REF: {
            auto arrayTypeRef = reinterpret_cast<ASTArrayTypeRef*>(typeRef);
            resolveType(arrayTypeRef->elementTypeRef);
            if (arrayTypeRef->elementTypeRef->type == context->getErrorType()) {
                arrayTypeRef->type = context->getErrorType();
                return;
            }

            if (arrayTypeRef->sizeExpr) {
                typeCheckExpr(arrayTypeRef->sizeExpr);
                if (arrayTypeRef->sizeExpr->type == context->getErrorType()) {
                    arrayTypeRef->type = context->getErrorType();
                    return;
                }

                auto constExpr = codeManager->evaluateConstExpr(arrayTypeRef->sizeExpr);
                if (!constExpr) {
                    arrayTypeRef->type = context->getErrorType();
                    return;
                }

                if (constExpr->type != context->getIntType()) {
                    arrayTypeRef->type = context->getErrorType();
                    diag->report(DIAG_ERROR, "Invalid type '{0}' given for array size expression expected Int", arrayTypeRef->type);
                    return;
                }

                assert(constExpr->kind == AST_INT_LITERAL);
                auto intLit = reinterpret_cast<ASTIntLit*>(constExpr);
                if (!intLit->value.sge(0)) {
                    arrayTypeRef->type = context->getErrorType();
                    diag->report(DIAG_ERROR, "The array size has to be greater than or equal to 0 but found '{0}'", intLit->value);
                    return;
                }

                arrayTypeRef->type = context->getStaticArrayType(arrayTypeRef->elementTypeRef->type, intLit->value);
            } else {
                arrayTypeRef->type = context->getDynamicArrayType(arrayTypeRef->elementTypeRef->type);
            }

        }   return;

        default:
            llvm_unreachable("Invalid Kind given for ASTTypeRef!");
    }
}

void Sema::inferTypeOfNode(ASTNode* node) {
    switch (node->kind) {
        case AST_LOAD:
        case AST_BREAK:
        case AST_CONTINUE:
        case AST_FALLTHROUGH:
            return;

        case AST_ANY_TYPE_REF:
        case AST_OPAQUE_TYPE_REF:
        case AST_TYPEOF_TYPE_REF:
        case AST_POINTER_TYPE_REF:
        case AST_ARRAY_TYPE_REF:
            return resolveType(reinterpret_cast<ASTTypeRef*>(node));

        case AST_NIL_LITERAL:
        case AST_BOOL_LITERAL:
        case AST_INT_LITERAL:
        case AST_FLOAT_LITERAL:
        case AST_STRING_LITERAL:
            return inferTypeOfLiteral(reinterpret_cast<ASTLit*>(node));

        case AST_VAR:
            return inferTypeOfVarDecl(reinterpret_cast<ASTVarDecl*>(node));

        case AST_LET:
            return inferTypeOfLetDecl(reinterpret_cast<ASTLetDecl*>(node));

        case AST_IDENTIFIER:
            return inferTypeOfIdentExpr(reinterpret_cast<ASTIdentExpr*>(node));

        case AST_BLOCK:
            return inferTypeOfStmts(reinterpret_cast<ASTBlock*>(node));

        case AST_UNARY:
            return inferTypeOfUnaryExpr(reinterpret_cast<ASTUnaryExpr*>(node));

        case AST_BINARY:
            return inferTypeOfBinaryExpr(reinterpret_cast<ASTBinaryExpr*>(node));

        case AST_MEMBER_ACCESS:
            return inferTypeOfMemberAccessExpr(reinterpret_cast<ASTMemberAccessExpr*>(node));

        case AST_CALL:
            return inferTypeOfCallExpr(reinterpret_cast<ASTCallExpr*>(node));

        case AST_SUBSCRIPT:
            return inferTypeOfSubscriptExpr(reinterpret_cast<ASTSubscriptExpr*>(node));

        case AST_RETURN: {
            auto returnStmt = reinterpret_cast<ASTReturnStmt*>(node);
            if (returnStmt->expr) {
                inferTypeOfNode(returnStmt->expr);
            }
        }   return;

        case AST_DEFER: {
            auto defer = reinterpret_cast<ASTDeferStmt*>(node);
            inferTypeOfNode(defer->expr);
        }   return;

        case AST_FOR:
            llvm::report_fatal_error("Implementation missing!");

        case AST_GUARD: {
            auto guard = reinterpret_cast<ASTGuardStmt*>(node);
            for (auto condition : guard->conditions) {
                inferTypeOfNode(condition);
                if (condition->type == context->getErrorType()) {
                    return;
                }
            }

            inferTypeOfNode(guard->elseBlock);
        }   return;

        case AST_IF: {
            auto ifStmt = reinterpret_cast<ASTIfStmt*>(node);
            for (auto condition : ifStmt->conditions) {
                inferTypeOfNode(condition);
                if (condition->type == context->getErrorType()) {
                    return;
                }
            }

            if (ifStmt->chainKind == AST_CHAIN_ELSE) {
                inferTypeOfNode(ifStmt->elseBlock);
            } else if (ifStmt->chainKind == AST_CHAIN_IF) {
                inferTypeOfNode(ifStmt->elseIf);
            }

            inferTypeOfStmts(ifStmt->block);
        }   return;

        case AST_SWITCH: {
            auto switchStmt = reinterpret_cast<ASTSwitchStmt*>(node);
            inferTypeOfNode(switchStmt->expr);
            if (switchStmt->expr->type == context->getErrorType()) {
                return;
            }

            for (auto caseStmt : switchStmt->cases) {
                inferTypeOfNode(caseStmt);
            }
        }   return;

        case AST_SWITCH_CASE: {
            auto caseStmt = reinterpret_cast<ASTCaseStmt*>(node);
            if (caseStmt->caseKind == AST_CASE_CONDITION) {
                inferTypeOfNode(caseStmt->condition);
                if (caseStmt->condition->type == context->getErrorType()) {
                    return;
                }
            }

            inferTypeOfStmts(caseStmt->block);
        }   return;

        case AST_DO: {
            auto doStmt = reinterpret_cast<ASTDoStmt*>(node);
            for (auto condition : doStmt->conditions) {
                inferTypeOfNode(condition);
                if (condition->type == context->getErrorType()) {
                    return;
                }
            }
            inferTypeOfStmts(doStmt->block);
        }   return;

        case AST_WHILE: {
            auto whileStmt = reinterpret_cast<ASTWhileStmt*>(node);
            for (auto condition : whileStmt->conditions) {
                inferTypeOfNode(condition);
                if (condition->type == context->getErrorType()) {
                    return;
                }
            }
            inferTypeOfStmts(whileStmt->block);
        }   return;

        case AST_FUNC:
            return typeFuncDecl(reinterpret_cast<ASTFuncDecl*>(node));

        case AST_PREFIX_FUNC:
            return typePrefixFuncDecl(reinterpret_cast<ASTPrefixFuncDecl*>(node));

        case AST_INFIX_FUNC:
            return typeInfixFuncDecl(reinterpret_cast<ASTInfixFuncDecl*>(node));

        case AST_PARAMETER:
            return typeParamDecl(reinterpret_cast<ASTParamDecl*>(node));

        case AST_STRUCT:
            return typeStructDecl(reinterpret_cast<ASTStructDecl*>(node));

        case AST_ENUM:
            return typeEnumDecl(reinterpret_cast<ASTEnumDecl*>(node));

        case AST_ENUM_ELEMENT:
            return typeEnumElementDecl(reinterpret_cast<ASTEnumElementDecl*>(node));

        default:
            llvm_unreachable("Invalid Kind given for ASTNode!");
    }
}

void Sema::inferTypeOfLiteral(ASTLit* literal) {
    if (literal->type) { return; }

    switch (literal->kind) {
        case AST_NIL_LITERAL:
            literal->type = context->getAnyPointerType();
            break;

        case AST_BOOL_LITERAL:
            literal->type = context->getBoolType();
            break;

        case AST_INT_LITERAL:
            literal->type = context->getIntType();
            break;

        case AST_FLOAT_LITERAL:
            literal->type = context->getFloatType();
            break;

        case AST_STRING_LITERAL:
            literal->type = context->getStringType();
            break;

        default:
            llvm_unreachable("Invalid Kind given for ASTLit!");
    }
}

void Sema::inferTypeOfVarDecl(ASTVarDecl* var) {
    if (var->type) { return; }

    if (var->assignment) {
        inferTypeOfNode(var->assignment);
        assert(var->assignment->candidateTypes.empty() && "Type inference feature is not fully implemented");
    }

    resolveType(var->typeRef);
    var->type = var->typeRef->type;
}

void Sema::inferTypeOfLetDecl(ASTLetDecl* let) {
    if (let->type) { return; }

    if (let->assignment) {
        inferTypeOfNode(let->assignment);
        assert(let->assignment->candidateTypes.empty() && "Type inference feature is not fully implemented");
    }

    resolveType(let->typeRef);
    let->type = let->typeRef->type;
}

void Sema::inferTypeOfIdentExpr(ASTIdentExpr* expr) {
    auto block = expr->getParentBlock();
    while (block) {
        auto It = block->decls.find(expr->declName.text);
        if (It != block->decls.end()) {
            expr->decl = It->getValue();
            inferTypeOfNode(expr->decl);
            expr->type = expr->decl->type;
            return;
        }

        block = block->getParentBlock();
    }

    // @Refactor may export all enum elements to the global scope?
    for (auto stmt : context->getRoot()->stmts) {
        if (stmt->kind == AST_ENUM) {
            auto enumDecl = reinterpret_cast<ASTEnumDecl*>(stmt);
            auto it = enumDecl->block->decls.find(expr->declName.text);
            if (it != enumDecl->block->decls.end() && it->getValue()->kind == AST_ENUM_ELEMENT) {
                expr->decl = it->getValue();
                inferTypeOfNode(expr->decl);
                expr->type = expr->decl->type;
                return;
            }
        }
    }

    expr->type = context->getErrorType();
    diag->report(DIAG_ERROR, "Unresolved identifier '{0}'", expr->declName.text);
}

void Sema::inferTypeOfStmts(ASTBlock* block) {
    for (auto it = block->stmts.rbegin(); it != block->stmts.rend(); it++) {
        inferTypeOfNode(*it);
    }
}

void Sema::inferTypeOfUnaryExpr(ASTUnaryExpr* expr) {
    if (expr->type || !expr->candidateTypes.empty()) { return; }

    inferTypeOfNode(expr->right);
    if (expr->right->type == context->getErrorType()) {
        expr->type = context->getErrorType();
        return;
    }

    auto it = context->getRoot()->decls.find(expr->op.text);
    if (it != context->getRoot()->decls.end()) {
        auto decl = it->getValue();
        if (decl->kind == AST_PREFIX_FUNC) {
            auto funcDecl = reinterpret_cast<ASTPrefixFuncDecl*>(decl);
            typePrefixFuncDecl(funcDecl);
            if (funcDecl->type == context->getErrorType()) {
                expr->type = context->getErrorType();
                return;
            }

            assert(funcDecl->params.size() == 1 && "Wellformed ASTPrefixFuncDecl can only contain 1 parameter!");
            if (funcDecl->params[0]->type == expr->right->type) {
                expr->candidateTypes.push_back(funcDecl->type);
            }
        }
    }

    // @Incomplete this can become relevant soon if function overloading is implemented...
//    for (auto it = context->getRoot()->decls.begin(); it != context->getRoot()->decls.end(); it++) {
//        auto decl = it->getValue();
//        if (decl->kind != AST_PREFIX_FUNC) { continue; }
//
//        auto prefixFuncDecl = reinterpret_cast<ASTPrefixFuncDecl*>(decl);
//        typePrefixFuncDecl(prefixFuncDecl);
//
//        if (prefixFuncDecl->params.size() != 1) { continue; }
//        if (prefixFuncDecl->params[0]->type != expr->right->type) { continue; }
//
//        expr->candidateTypes.push_back(prefixFuncDecl->type);
//    }
//
//    for (auto funcType : *context->getBuiltinFuncTypes()) {
//        if (funcType->name != expr->op.text) { continue; }
//        if (funcType->paramTypes.size() != 1) { continue; }
//        if (funcType->paramTypes[0] != expr->right->type) { continue; }
//
//        expr->candidateTypes.push_back(funcType);
//    }

    if (expr->candidateTypes.empty()) {
        expr->type = context->getErrorType();
        diag->report(DIAG_ERROR, "Couldn't resolve prefix function '{0}'", expr->op.text);
        return;
    }

    if (expr->candidateTypes.size() == 1) {
        expr->type = expr->candidateTypes[0];
        return;
    }

    llvm::SetVector<Type*> returnTypes;
    for (auto candidateType : expr->candidateTypes) {
        if (returnTypes.count(candidateType)) {
            expr->type = context->getErrorType();
            diag->report(DIAG_ERROR, "Type of '{0}' is ambigous in this context", expr->op.text);
            return;
        }

        returnTypes.insert(candidateType);
    }
}

void Sema::inferTypeOfBinaryExpr(ASTBinaryExpr* expr) {
    if (expr->type || !expr->candidateTypes.empty()) { return; }

    inferTypeOfNode(expr->left);
    inferTypeOfNode(expr->right);

    if (expr->left->type == context->getErrorType() || expr->right->type == context->getErrorType()) {
        expr->type = context->getErrorType();
        return;
    }

    auto it = context->getRoot()->decls.find(expr->op.text);
    if (it != context->getRoot()->decls.end()) {
        auto decl = it->getValue();
        if (decl->kind == AST_INFIX_FUNC) {
            auto funcDecl = reinterpret_cast<ASTInfixFuncDecl*>(decl);
            typeInfixFuncDecl(funcDecl);
            if (funcDecl->type == context->getErrorType()) {
                expr->type = context->getErrorType();
                return;
            }

            assert(funcDecl->params.size() == 2 && "Wellformed ASTInfixFuncDecl can only contain 2 parameters!");
            if (funcDecl->params[0]->type == expr->left->type && funcDecl->params[1]->type == expr->right->type) {
                expr->candidateTypes.push_back(funcDecl->type);
            }
        }
    }

//    for (auto funcType : *context->getBuiltinFuncTypes()) {
//        if (funcType->name != expr->op.text) { continue; }
//        if (funcType->paramTypes.size() != 2) { continue; }
//        if (funcType->paramTypes[0] != expr->left->type) { continue; }
//        if (funcType->paramTypes[1] != expr->right->type) { continue; }
//
//        expr->candidateTypes.push_back(funcType);
//    }

    if (expr->candidateTypes.empty()) {
        expr->type = context->getErrorType();
        diag->report(DIAG_ERROR, "Couldn't resolve infix function '{0}'", expr->op.text);
        return;
    }

    if (expr->candidateTypes.size() == 1) {
        expr->type = expr->candidateTypes[0];
        return;
    }

    llvm::SetVector<Type*> returnTypes;
    for (auto candidateType : expr->candidateTypes) {
        if (returnTypes.count(candidateType)) {
            expr->type = context->getErrorType();
            diag->report(DIAG_ERROR, "Type of '{0}' is ambigous in this context", expr->op.text);
            return;
        }

        returnTypes.insert(candidateType);
    }
}

void Sema::inferTypeOfMemberAccessExpr(ASTMemberAccessExpr* expr) {
    if (expr->type) { return; }

    inferTypeOfNode(expr->left);
    if (expr->left->type == context->getErrorType()) {
        expr->type = context->getErrorType();
        return;
    }

    if (expr->left->type->kind != TYPE_DECL_STRUCT) {
        expr->type = context->getErrorType();
        diag->report(DIAG_ERROR, "Cannot access member of non struct types");
        return;
    }

    auto structType = reinterpret_cast<StructType*>(expr->left->type);
    auto memberTypeIt = structType->memberTypes.find(expr->memberName.text);
    if (memberTypeIt == structType->memberTypes.end()) {
        expr->type = context->getErrorType();
        diag->report(DIAG_ERROR, "No member named '{0}' found in struct type", expr->memberName.text);
        return;
    }

    // @Bug memberTypeIt->getValue() is returning nil but there should be a type if it is registered in the table...
    //      it could also be caused by move semantics...
    assert(memberTypeIt->getValue() && "MemberTypes in StructType shouldn't contain nil!");
    expr->type = memberTypeIt->getValue();
}

void Sema::inferTypeOfCallExpr(ASTCallExpr* expr) {
    if (expr->type) { return; }

    inferTypeOfNode(expr->left);
    if (expr->left->type == context->getErrorType()) {
        expr->type = context->getErrorType();
        return;
    }

    // @Incomplete expr->left could contain candidateTypes which could be filtered based on argument types...

    if (expr->left->type->kind != TYPE_DECL_FUNC) {
        expr->type = context->getErrorType();
        diag->report(DIAG_ERROR, "Cannot call a non function type");
        return;
    }

    auto funcType = reinterpret_cast<FuncType*>(expr->left->type);
    for (auto argument : expr->args) {
        inferTypeOfNode(argument);
        if (argument->type == context->getErrorType()) {
            expr->type = context->getErrorType();
            return;
        }

        // @Incomplete infer types of arguments top-down if candidateTypes has size > 1
    }

    expr->type = funcType->returnType;
}

void Sema::inferTypeOfSubscriptExpr(ASTSubscriptExpr* expr) {
    if (expr->type) { return; }

    llvm::report_fatal_error("Implementation missing!");
}

void Sema::typeFuncDecl(ASTFuncDecl* decl) {
    if (decl->type) { return; }

    for (auto paramDecl : decl->params) {
        typeParamDecl(paramDecl);

        if (paramDecl->type == context->getErrorType()) {
            decl->type = context->getErrorType();
            return;
        }
    }

    inferTypeOfStmts(decl->block);
    resolveType(decl->returnTypeRef);
    if (decl->returnTypeRef->type == context->getErrorType()) {
        decl->type = context->getErrorType();
        return;
    }

    decl->type = context->getFuncType(decl);
}

void Sema::typePrefixFuncDecl(ASTPrefixFuncDecl* decl) {
    if (decl->type) { return; }

    if (decl->params.size() != 1) {
        decl->type = context->getErrorType();
        diag->report(DIAG_ERROR, "Prefix function '{0}' must contain 1 parameter", decl->name.text);
        return;
    }

    typeFuncDecl(decl);
}

void Sema::typeInfixFuncDecl(ASTInfixFuncDecl* decl) {
    if (decl->type) { return; }

    if (decl->params.size() != 2) {
        decl->type = context->getErrorType();
        diag->report(DIAG_ERROR, "Infix function '{0}' must contain 2 parameters", decl->name.text);
        return;
    }

    typeFuncDecl(decl);
}

void Sema::typeParamDecl(ASTParamDecl* decl) {
    if (decl->type) { return; }

    resolveType(decl->typeRef);
    decl->type = decl->typeRef->type;
}

void Sema::typeStructDecl(ASTStructDecl* decl) {
    if (decl->type) { return; }

    llvm::SmallVector<ASTStructDecl*, 0> parentsForCyclicStorageCheck;
    if (checkCyclicStorageInStructDecl(decl, &parentsForCyclicStorageCheck)) {
        decl->type = context->getErrorType();
        return;
    }

    llvm::StringMap<Type*> memberTypes;
    for (auto it = decl->block->decls.begin(); it != decl->block->decls.end(); it++) {
        auto memberDecl = it->getValue();
        inferTypeOfNode(memberDecl);

        if (memberDecl->type == context->getErrorType()) {
            decl->type = context->getErrorType();
            return;
        }

        if (memberDecl->kind == AST_VAR || memberDecl->kind == AST_LET) {
            assert(memberDecl->type && "Type shouldn't be nil when written to MemberTypes!");
            memberTypes.try_emplace(memberDecl->name.text, memberDecl->type);
        }
    }

    decl->type = context->getStructType(decl->name.text, memberTypes);
}

void Sema::typeEnumDecl(ASTEnumDecl* decl) {
    if (decl->type) { return; }

    inferTypeOfStmts(decl->block);
    decl->type = context->getEnumType(decl);
}

void Sema::typeEnumElementDecl(ASTEnumElementDecl* decl) {
    if (decl->type) { return; }

    if (decl->assignment) {
        inferTypeOfNode(decl->assignment);
    }

    decl->type = context->getIntType();
}

bool Sema::checkCyclicStorageInStructDecl(ASTStructDecl* structDecl, llvm::SmallVector<ASTStructDecl*, 0>* parentDecls) {
    for (auto declIt = structDecl->block->decls.begin(); declIt != structDecl->block->decls.end(); declIt++) {
        auto decl = declIt->getValue();
        if (decl->kind == AST_VAR || decl->kind == AST_LET) {
            auto opaqueDecl = reinterpret_cast<ASTOpaqueDecl*>(decl);
            if (opaqueDecl->typeRef->kind == AST_OPAQUE_TYPE_REF) {
                auto opaqueTypeRef = reinterpret_cast<ASTOpaqueTypeRef*>(opaqueDecl->typeRef);
                if (!opaqueTypeRef->decl) {
                    auto block = opaqueTypeRef->getParentBlock();
                    while (block) {
                        auto blockIt = block->decls.find(opaqueTypeRef->typeName.text);
                        if (blockIt != block->decls.end()) {
                            opaqueTypeRef->decl = blockIt->getValue();
                            break;
                        }

                        block = block->getParentBlock();
                    }
                }

                if (opaqueTypeRef->decl && opaqueTypeRef->decl->kind == AST_STRUCT) {
                    auto memberDecl = reinterpret_cast<ASTStructDecl*>(opaqueTypeRef->decl);

                    for (auto parentDecl : *parentDecls) {
                        if (parentDecl == memberDecl) {
                            opaqueTypeRef->type = context->getErrorType();
                            decl->type = context->getErrorType();
                            diag->report(DIAG_ERROR, "Struct cannot store a variable of same type recursively");
                            return true;
                        }
                    }

                    parentDecls->push_back(memberDecl);
                    if (checkCyclicStorageInStructDecl(memberDecl, parentDecls)) {
                        return true;
                    }
                }
            }
        }
    }
    return false;
}

void Sema::typeCheckNode(ASTNode* node) {
    if (node->isValidated) { return; }
    defer(node->isValidated = true);

    switch (node->kind) {
        case AST_UNKNOWN:        return llvm::report_fatal_error("Internal compiler error!");
        case AST_LOAD:           return;
        case AST_NIL_LITERAL:    return;
        case AST_BOOL_LITERAL:   return;
        case AST_INT_LITERAL:    return;
        case AST_FLOAT_LITERAL:  return;
        case AST_STRING_LITERAL: return;
        case AST_FUNC:           return typeCheckFuncDecl(reinterpret_cast<ASTFuncDecl*>(node));
        case AST_PARAMETER:      return typeCheckParamDecl(reinterpret_cast<ASTParamDecl*>(node));
        case AST_STRUCT:         return typeCheckStructDecl(reinterpret_cast<ASTStructDecl*>(node));
        case AST_VAR:            return typeCheckVarDecl(reinterpret_cast<ASTVarDecl*>(node));
        case AST_LET:            return typeCheckLetDecl(reinterpret_cast<ASTLetDecl*>(node));
        case AST_ENUM:           return typeCheckEnumDecl(reinterpret_cast<ASTEnumDecl*>(node));
        case AST_ENUM_ELEMENT:   return typeCheckEnumElementDecl(reinterpret_cast<ASTEnumElementDecl*>(node));
        case AST_BLOCK:          return typeCheckBlock(reinterpret_cast<ASTBlock*>(node));
        case AST_IDENTIFIER:     return typeCheckIdentExpr(reinterpret_cast<ASTIdentExpr*>(node));
        case AST_UNARY:          return typeCheckUnaryExpr(reinterpret_cast<ASTUnaryExpr*>(node));
        case AST_BINARY:         return typeCheckBinaryExpr(reinterpret_cast<ASTBinaryExpr*>(node));
        case AST_MEMBER_ACCESS:  return typeCheckMemberAccessExpr(reinterpret_cast<ASTMemberAccessExpr*>(node));
        case AST_CALL:           return typeCheckCallExpr(reinterpret_cast<ASTCallExpr*>(node));
        case AST_SUBSCRIPT:      return typeCheckSubscriptExpr(reinterpret_cast<ASTSubscriptExpr*>(node));
        case AST_BREAK:          return typeCheckBreakStmt(reinterpret_cast<ASTBreakStmt*>(node));
        case AST_CONTINUE:       return typeCheckContinueStmt(reinterpret_cast<ASTContinueStmt*>(node));
        case AST_FALLTHROUGH:    return typeCheckFallthroughStmt(reinterpret_cast<ASTFallthroughStmt*>(node));
        case AST_RETURN:         return typeCheckReturnStmt(reinterpret_cast<ASTReturnStmt*>(node));
        case AST_DEFER:          return typeCheckDeferStmt(reinterpret_cast<ASTDeferStmt*>(node));
        case AST_FOR:            return typeCheckForStmt(reinterpret_cast<ASTForStmt*>(node));
        case AST_GUARD:          return typeCheckGuardStmt(reinterpret_cast<ASTGuardStmt*>(node));
        case AST_IF:             return typeCheckIfStmt(reinterpret_cast<ASTIfStmt*>(node));
        case AST_SWITCH:         return typeCheckSwitchStmt(reinterpret_cast<ASTSwitchStmt*>(node));
        case AST_SWITCH_CASE:    return typeCheckCaseStmt(reinterpret_cast<ASTCaseStmt*>(node));
        case AST_DO:             return typeCheckDoStmt(reinterpret_cast<ASTDoStmt*>(node));
        case AST_WHILE:          return typeCheckWhileStmt(reinterpret_cast<ASTWhileStmt*>(node));
        default:                 llvm_unreachable("Invalid Kind given for ASTNode!");
    }
}

void Sema::typeCheckFuncDecl(ASTFuncDecl* decl) {
    if (decl->isValidated) { return; }
    defer(decl->isValidated = true);

    if (decl->type == context->getErrorType()) {
        return;
    }

    if (decl->type->isIncomplete()) {
        diag->report(DIAG_ERROR, "Declaration has incomplete type");
    }

    for (auto paramDecl : decl->params) {
        typeCheckParamDecl(paramDecl);
    }

    typeCheckFuncBlock(decl);
}

void Sema::typeCheckFuncBlock(ASTFuncDecl* decl) {
    if (decl->block->isValidated) { return; }
    defer(decl->block->isValidated = true);

    for (auto stmt : decl->block->stmts) {
        bool isStmtAllowed =
        stmt->kind == AST_NIL_LITERAL ||
        stmt->kind == AST_BOOL_LITERAL ||
        stmt->kind == AST_INT_LITERAL ||
        stmt->kind == AST_FLOAT_LITERAL ||
        stmt->kind == AST_STRING_LITERAL ||
        stmt->kind == AST_VAR ||
        stmt->kind == AST_LET ||
        stmt->kind == AST_IDENTIFIER ||
        stmt->kind == AST_UNARY ||
        stmt->kind == AST_BINARY ||
        stmt->kind == AST_MEMBER_ACCESS ||
        stmt->kind == AST_RETURN ||
        stmt->kind == AST_DEFER ||
        stmt->kind == AST_FOR ||
        stmt->kind == AST_GUARD ||
        stmt->kind == AST_IF ||
        stmt->kind == AST_SWITCH ||
        stmt->kind == AST_DO ||
        stmt->kind == AST_WHILE ||
        stmt->kind == AST_CALL ||
        stmt->kind == AST_SUBSCRIPT;

        if (!isStmtAllowed) {
            diag->report(DIAG_ERROR, "Statement is not allowed inside of func declaration");
        } else {
            typeCheckNode(stmt);
        }
    }

    if (decl->returnTypeRef->type != context->getVoidType() && decl->returnTypeRef->type != context->getErrorType()) {
        if (!isBlockAlwaysReturning(decl->block)) {
            decl->type = context->getErrorType();
            diag->report(DIAG_ERROR, "Not all code paths return a value");
        }
    }
}

void Sema::typeCheckParamDecl(ASTParamDecl* decl) {
    if (decl->isValidated) { return; }
    defer(decl->isValidated = true);

    if (decl->type == context->getErrorType()) {
        return;
    }

    // @Incomplete ...
}

void Sema::typeCheckStructDecl(ASTStructDecl* decl) {
    if (decl->isValidated) { return; }
    defer(decl->isValidated = true);

    if (decl->type == context->getErrorType()) {
        return;
    }

    if (decl->type->isIncomplete()) {
        diag->report(DIAG_ERROR, "Declaration has incomplete type");
    }

    typeCheckStructBlock(decl);

    // @Incomplete ...
}

void Sema::typeCheckStructBlock(ASTStructDecl* decl) {
    if (decl->block->isValidated) { return; }
    defer(decl->block->isValidated = true);

    for (auto stmt : decl->block->stmts) {
        bool isStmtAllowed =
        stmt->kind == AST_VAR ||
        stmt->kind == AST_LET;

        if (!isStmtAllowed) {
            diag->report(DIAG_ERROR, "Statement is not allowed inside of struct declaration");
        } else {
            typeCheckNode(stmt);
        }
    }
}

void Sema::typeCheckVarDecl(ASTVarDecl* decl) {
    if (decl->isValidated) { return; }
    defer(decl->isValidated = true);

    if (decl->assignment) {
        typeCheckExpr(decl->assignment);

        if (decl->type != decl->assignment->type && decl->assignment->type != context->getErrorType()) {
            decl->type = context->getErrorType();
            diag->report(DIAG_ERROR, "Assignment expression of '{0}' has mismatching type", decl->name.text);
        }
    }
}

void Sema::typeCheckLetDecl(ASTLetDecl* decl) {
    if (decl->isValidated) { return; }
    defer(decl->isValidated = true);

    if (decl->assignment) {
        typeCheckExpr(decl->assignment);

        if (decl->type != decl->assignment->type && decl->assignment->type != context->getErrorType()) {
            decl->type = context->getErrorType();
            diag->report(DIAG_ERROR, "Assignment expression of '{0}' has mismatching type", decl->name.text);
        }
    } else {
        decl->type = context->getErrorType();
        diag->report(DIAG_ERROR, "Expected assignment expression for '{0}'", decl->name.text);
    }
}

void Sema::typeCheckEnumDecl(ASTEnumDecl* decl) {
    if (decl->isValidated) { return; }
    defer(decl->isValidated = true);

    typeCheckEnumBlock(decl);
}

void Sema::typeCheckEnumBlock(ASTEnumDecl* decl) {
    if (decl->block->isValidated) { return; }
    defer(decl->block->isValidated = true);

    for (auto stmt : decl->block->stmts) {
        if (stmt->kind == AST_ENUM_ELEMENT) {
            auto elementDecl = reinterpret_cast<ASTEnumElementDecl*>(stmt);
            typeCheckEnumElementDecl(elementDecl);
        } else {
            diag->report(DIAG_ERROR, "Only enum elements are allowed inside of enum declaration");
        }
    }
}

void Sema::typeCheckEnumElementDecl(ASTEnumElementDecl* decl) {
    if (decl->isValidated) { return; }
    defer(decl->isValidated = true);

    if (decl->type == context->getErrorType()) {
        return;
    }

    if (!decl->parent || decl->parent->kind != AST_BLOCK) {
        diag->report(DIAG_ERROR, "Enum element '{0}' can only be declared inside of an enum", decl->name.text);
        return;
    }

    auto parentBlock = reinterpret_cast<ASTBlock*>(decl->parent);
    if (!parentBlock->parent || parentBlock->parent->kind != AST_ENUM) {
        diag->report(DIAG_ERROR, "Enum element '{0}' can only be declared inside of an enum", decl->name.text);
        return;
    }

    auto enumDecl = reinterpret_cast<ASTEnumDecl*>(parentBlock->parent);
    if (enumDecl->type == context->getErrorType()) {
        return;
    }

    assert(enumDecl->type && enumDecl->type->kind == TYPE_DECL_ENUM);
    auto enumType = reinterpret_cast<struct EnumType*>(enumDecl->type);

    if (decl->assignment) {
        typeCheckExpr(decl->assignment);

        if (decl->assignment->type == context->getErrorType()) {
            decl->type = context->getErrorType();
            return;
        }

        if (decl->type != decl->assignment->type) {
            diag->report(DIAG_ERROR, "Assignment expression of '{0}' has mismatching type", decl->name.text);
            return;
        }

        auto constExpr = codeManager->evaluateConstExpr(decl->assignment);
        if (!constExpr) {
            return;
        }

        assert(constExpr->kind == AST_INT_LITERAL);
        auto intLit = reinterpret_cast<ASTIntLit*>(constExpr);
        bool isOverlappingOtherElementValue = false;
        for (auto memberValue : enumType->memberValues) {
            if (memberValue->value == intLit->value) {
                isOverlappingOtherElementValue = true;
                break;
            }
        }

        if (isOverlappingOtherElementValue) {
            diag->report(DIAG_ERROR, "Invalid reuse of value {0} for different enum elements", intLit->value);
        } else {
            enumType->memberValues.push_back(intLit);
            enumType->nextMemberValue = intLit->value + 1; // See @EnumCheckInSequencialOrder
        }
    } else {
        // @Bug @EnumCheckInSequencialOrder
        //
        // Calculate value of enum element based on previous element
        // it could be that the elements will not the validated in sequential order.
        // which would lead to non-deterministic invalid value assignments to elements
        auto intLit = new (context) ASTIntLit;
        intLit->parent = decl;
        intLit->type = decl->type;
        intLit->value = enumType->nextMemberValue;

        decl->assignment = intLit;

        enumType->memberValues.push_back(intLit);
        enumType->nextMemberValue += 1;
    }
}

void Sema::typeCheckBlock(ASTBlock* block) {
    if (block->isValidated) { return; }
    defer(block->isValidated = true);

    switch (block->scope.kind) {
        case SCOPE_STRUCT: {
            assert(block->parent->kind == AST_STRUCT);
            auto structDecl = reinterpret_cast<ASTStructDecl*>(block->parent);
            return typeCheckStructBlock(structDecl);
        }

        case SCOPE_ENUM: {
            assert(block->parent->kind == AST_ENUM);
            auto Enum = reinterpret_cast<ASTEnumDecl*>(block->parent);
            return typeCheckEnumBlock(Enum);
        }

        case SCOPE_FUNC: {
            assert(block->parent->kind == AST_FUNC);
            auto funcDecl = reinterpret_cast<ASTFuncDecl*>(block->parent);
            return typeCheckFuncBlock(funcDecl);
        }

        case SCOPE_GLOBAL:
        case SCOPE_BRANCH:
        case SCOPE_LOOP:
        case SCOPE_SWITCH:
            for (auto stmt : block->stmts) {
                typeCheckNode(stmt);
            }
            return;
    }
}

void Sema::typeCheckIdentExpr(ASTIdentExpr* expr) {
    if (expr->isValidated) { return; }
    defer(expr->isValidated = true);

    // @Incomplete ...
}

void Sema::typeCheckUnaryExpr(ASTUnaryExpr* expr) {
    if (expr->isValidated) { return; }
    defer(expr->isValidated = true);

    if (expr->type == context->getErrorType()) {
        return;
    }

    llvm::report_fatal_error("Implementation missing!");
}

void Sema::typeCheckBinaryExpr(ASTBinaryExpr* expr) {
    if (expr->isValidated) { return; }
    defer(expr->isValidated = true);

    if (expr->type == context->getErrorType()) {
        return;
    }

    llvm::report_fatal_error("Implementation missing!");
}

void Sema::typeCheckMemberAccessExpr(ASTMemberAccessExpr* expr) {
    if (expr->isValidated) { return; }
    defer(expr->isValidated = true);

    // @Incomplete ...
}

void Sema::typeCheckCallExpr(ASTCallExpr* expr) {
    if (expr->isValidated) { return; }
    defer(expr->isValidated = true);

    if (expr->type == context->getErrorType()) {
        return;
    }

    llvm::report_fatal_error("Implementation missing!");
}

void Sema::typeCheckSubscriptExpr(ASTSubscriptExpr* expr) {
    if (expr->isValidated) { return; }
    defer(expr->isValidated = true);

    if (expr->type == context->getErrorType()) {
        return;
    }

    llvm::report_fatal_error("Implementation missing!");
}

void Sema::typeCheckExpr(ASTExpr* expr) {
    if (expr->isValidated) { return; }
    defer(expr->isValidated = true);

    switch (expr->kind) {
        case AST_NIL_LITERAL:    return;
        case AST_BOOL_LITERAL:   return;
        case AST_INT_LITERAL:    return;
        case AST_FLOAT_LITERAL:  return;
        case AST_STRING_LITERAL: return;
        case AST_IDENTIFIER:     return typeCheckIdentExpr(reinterpret_cast<ASTIdentExpr*>(expr));
        case AST_UNARY:          return typeCheckUnaryExpr(reinterpret_cast<ASTUnaryExpr*>(expr));
        case AST_BINARY:         return typeCheckBinaryExpr(reinterpret_cast<ASTBinaryExpr*>(expr));
        case AST_MEMBER_ACCESS:  return typeCheckMemberAccessExpr(reinterpret_cast<ASTMemberAccessExpr*>(expr));
        case AST_CALL:           return typeCheckCallExpr(reinterpret_cast<ASTCallExpr*>(expr));
        case AST_SUBSCRIPT:      return typeCheckSubscriptExpr(reinterpret_cast<ASTSubscriptExpr*>(expr));
        default:                 llvm_unreachable("Invalid Kind given for ASTExpr!");
    }
}

void Sema::typeCheckBreakStmt(ASTBreakStmt* stmt) {
    if (stmt->isValidated) { return; }
    defer(stmt->isValidated = true);

    // @Refactor @EnclosingScope Temporary solution for enclosing scope handling
    bool isBreakStmtAllowed = false;
    for (auto scopeBlock = stmt->getParentBlock(); scopeBlock; scopeBlock = scopeBlock->getParentBlock()) {
        if (scopeBlock->scope.kind == SCOPE_LOOP || scopeBlock->scope.kind == SCOPE_SWITCH) {
            isBreakStmtAllowed = true;
            break;
        }

        if (scopeBlock->scope.kind != SCOPE_BRANCH) {
            break;
        }
    }

    if (!isBreakStmtAllowed) {
        diag->report(DIAG_ERROR, "'break' is only allowed inside a loop or switch");
    }
}

void Sema::typeCheckContinueStmt(ASTContinueStmt* stmt) {
    if (stmt->isValidated) { return; }
    defer(stmt->isValidated = true);

    // @Refactor @EnclosingScope Temporary solution for enclosing scope handling
    bool isContinueStmtAllowed = false;
    for (auto scopeBlock = stmt->getParentBlock(); scopeBlock; scopeBlock = scopeBlock->getParentBlock()) {
        if (scopeBlock->scope.kind == SCOPE_LOOP) {
            isContinueStmtAllowed = true;
            break;
        }

        if (scopeBlock->scope.kind != SCOPE_BRANCH && scopeBlock->scope.kind != SCOPE_SWITCH) {
            break;
        }
    }

    if (!isContinueStmtAllowed) {
        diag->report(DIAG_ERROR, "'continue' is only allowed inside a loop");
    }
}

void Sema::typeCheckFallthroughStmt(ASTFallthroughStmt* stmt) {
    if (stmt->isValidated) { return; }
    defer(stmt->isValidated = true);

    // @Refactor @EnclosingScope Temporary solution for enclosing scope handling
    bool isFallthroughStmtAllowed = false;
    for (auto scopeBlock = stmt->getParentBlock(); scopeBlock; scopeBlock = scopeBlock->getParentBlock()) {
        if (scopeBlock->scope.kind == SCOPE_SWITCH) {
            isFallthroughStmtAllowed = true;
            break;
        }

        if (scopeBlock->scope.kind != SCOPE_BRANCH) {
            break;
        }
    }

    if (!isFallthroughStmtAllowed) {
        diag->report(DIAG_ERROR, "'fallthrough' is only allowed inside a switch");
    }
}

void Sema::typeCheckReturnStmt(ASTReturnStmt* stmt) {
    if (stmt->isValidated) { return; }
    defer(stmt->isValidated = true);

    if (stmt->expr) {
        typeCheckExpr(stmt->expr);
    }

    ASTFuncDecl* enclosingFuncDecl = nullptr;
    auto stmtParent = stmt->parent;
    while (stmtParent) {
        if (stmtParent->kind == AST_FUNC) {
            enclosingFuncDecl = reinterpret_cast<ASTFuncDecl*>(stmtParent);
            break;
        }

        if (stmtParent->kind == AST_STRUCT) {
            break;
        }

        stmtParent = stmtParent->parent;
    }

    if (!enclosingFuncDecl) {
        diag->report(DIAG_ERROR, "Statement is only allowed inside of function declaration");
        return;
    }

    if (enclosingFuncDecl->type == context->getErrorType()) {
        return;
    }

    if (stmt->expr && enclosingFuncDecl->returnTypeRef->type != stmt->expr->type) {
        diag->report(DIAG_ERROR, "Type mismatch in return statement");
    } else if (!stmt->expr && enclosingFuncDecl->returnTypeRef->type != context->getVoidType()) {
        diag->report(DIAG_ERROR, "Expected expression after return statement");
    }
}

void Sema::typeCheckDeferStmt(ASTDeferStmt* stmt) {
    llvm::report_fatal_error("Implementation missing!");
}

void Sema::typeCheckForStmt(ASTForStmt* stmt) {
    llvm::report_fatal_error("Implementation missing!");
}

void Sema::typeCheckGuardStmt(ASTGuardStmt* stmt) {
    if (stmt->isValidated) { return; }
    defer(stmt->isValidated = true);

    typeCheckConditions(stmt);
    typeCheckBlock(stmt->elseBlock);

    if (!isBlockAlwaysReturning(stmt->elseBlock)) {
        diag->report(DIAG_ERROR, "Not all code paths return a value");
    }
}

void Sema::typeCheckIfStmt(ASTIfStmt* stmt) {
    if (stmt->isValidated) { return; }
    defer(stmt->isValidated = true);

    typeCheckConditions(stmt);
    typeCheckBlock(stmt->block);

    switch (stmt->chainKind) {
        case AST_CHAIN_NONE:
            break;

        case AST_CHAIN_ELSE:
            typeCheckBlock(stmt->elseBlock);
            break;

        case AST_CHAIN_IF:
            typeCheckIfStmt(stmt->elseIf);
            break;
    }
}

void Sema::typeCheckSwitchStmt(ASTSwitchStmt* stmt) {
    if (stmt->isValidated) { return; }
    defer(stmt->isValidated = true);

    typeCheckExpr(stmt->expr);
    if (stmt->expr->type == context->getErrorType()) {
        return;
    }

    bool containsElseCase = false;
    bool containsMissplacedElseCase = false;
    bool containsMultipleElseCases = false;
    for (auto It = stmt->cases.begin(); It != stmt->cases.end(); It++) {
        auto caseStmt = *It;
        typeCheckCaseStmt(caseStmt);

        if (caseStmt->caseKind == AST_CASE_ELSE) {
            if (containsElseCase) {
                containsMultipleElseCases = true;
            } else {
                containsElseCase = true;
            }

            if (It != stmt->cases.end() - 1) {
                containsMissplacedElseCase = true;
            }
        }
    }

    if (containsMissplacedElseCase) {
        diag->report(DIAG_ERROR, "Switch cases cannot appear after else case of switch statement");
    } else {
        checkIsSwitchStmtExhaustive(stmt);
    }
}

void Sema::typeCheckCaseStmt(ASTCaseStmt* stmt) {
    if (stmt->isValidated) { return; }
    defer(stmt->isValidated = true);

    if (stmt->condition) {
        typeCheckExpr(stmt->condition);
    }

    if (stmt->block->stmts.empty()) {
        diag->report(DIAG_ERROR, "Switch case should contain at least one statement");
    }

    typeCheckBlock(stmt->block);

    //    auto ConstExpr = EvaluateConstExpr(Diag, Case->Condition);
    //    if (!ConstExpr) {
    //        Case->Condition->Type = nullptr;
    //    }
    //
    //    Case->Condition = ConstExpr;
    //
    //    if (!compiler->interpreter.evaluate_constant_expression(Case->Condition)) {
    //        return false;
    //    }
    //
    //    auto Expr = Case->Condition;
    //    assert(Expr->Kind == AST_INT_LITERAL);
    //
    //    auto Int = reinterpret_cast<ASTIntLit*>(Expr);
    //    std::remove_if(AllMemberValues.begin(), AllMemberValues.end(), [&](ASTIntLit* Member) {
    //        return Int->Value == Member->Value;
    //    });
}

void Sema::typeCheckDoStmt(ASTDoStmt* stmt) {
    llvm::report_fatal_error("Implementation missing!");
}

void Sema::typeCheckWhileStmt(ASTWhileStmt* stmt) {
    if (stmt->isValidated) { return; }
    defer(stmt->isValidated = true);

    typeCheckConditions(stmt);
    typeCheckBlock(stmt->block);
}

void Sema::typeCheckConditions(ASTBranchStmt* stmt) {
    for (auto condition : stmt->conditions) {
        typeCheckExpr(condition);

        if (condition->type != context->getBoolType() && condition->type != context->getErrorType()) {
            condition->type = context->getErrorType();
            diag->report(DIAG_ERROR, "Mismatching condition type expected 'Bool'");
        }
    }
}

bool Sema::isBlockAlwaysReturning(ASTBlock* block) {
    // @Incomplete @Stability @EnclosingScope
    // There could always be the case that the outer for-statement
    // won't be the enclosing scope of this block.
    // The solution would be to differentiate ASTBlock from a Scope
    // or add more detailed contextual searching of enclosing nodes.
    //
    // Example:
    //
    //    for element in sequence {
    //        func myLocalFunc() -> Void {
    //
    //            // The following continue statement shouldn't be allowed
    //            // but find_parent_of_kind(AST_FOR) will always succeed
    //            // because it is not scope aware...
    //
    //            if element == 1 { continue }
    //        }
    //    }

    // @Refactor @EnclosingScope Temporary solution for enclosing scope handling
    bool isContinueStmtAllowed = false;
    for (auto scopeBlock = block; scopeBlock; scopeBlock = scopeBlock->getParentBlock()) {
        if (scopeBlock->scope.kind == SCOPE_LOOP) {
            isContinueStmtAllowed = true;
            break;
        }

        if (scopeBlock->scope.kind != SCOPE_BRANCH && scopeBlock->scope.kind != SCOPE_SWITCH) {
            break;
        }
    }

    for (auto stmt : block->stmts) {
        if (stmt->kind == AST_RETURN || (stmt->kind == AST_CONTINUE && isContinueStmtAllowed)) {
            return true;
        }

        if (stmt->kind == AST_IF) {
            auto ifStmt = reinterpret_cast<ASTIfStmt*>(stmt);
            if (isIfStmtAlwaysReturning(ifStmt)) {
                return true;
            }
        }

        if (stmt->kind == AST_SWITCH) {
            auto switchStmt = reinterpret_cast<ASTSwitchStmt*>(stmt);
            if (isSwitchStmtAlwaysRetuning(switchStmt)) {
                return true;
            }
        }
    }

    return false;
}

bool Sema::isIfStmtAlwaysReturning(ASTIfStmt* stmt) {
    switch (stmt->chainKind) {
        case AST_CHAIN_NONE:
            return false;

        case AST_CHAIN_ELSE:
            return isBlockAlwaysReturning(stmt->block) && isBlockAlwaysReturning(stmt->elseBlock);

        case AST_CHAIN_IF:
            return isBlockAlwaysReturning(stmt->block) && isIfStmtAlwaysReturning(stmt->elseIf);
    }
}

bool Sema::isSwitchStmtAlwaysRetuning(ASTSwitchStmt* stmt) {
    for (auto caseStmt : stmt->cases) {
        if (!isBlockAlwaysReturning(caseStmt->block)) {
            return false;
        }
    }

    return true;
}

void Sema::checkIsSwitchStmtExhaustive(ASTSwitchStmt* stmt) {
    assert(stmt->cases.size() > 0 && "ASTSwitch with no cases shouldn't reach here in the sema-phase!");
    assert(stmt->expr->isValidated);

    // The type checker requires the else-case of the switch to be always the last one
    // so checking the last statement first for an else case will be enough
    // assuming that an error will be reported if the else is not the last case
    auto lastCaseStmt = stmt->cases.back();
    if (lastCaseStmt->caseKind == AST_CASE_ELSE) {
        return;
    }

    auto exprType = stmt->expr->type;
    if (!exprType || exprType->kind != TYPE_DECL_ENUM) {
        // We'll not guarantee non enum types to be exhaustive
        // so we will force the switch to contain an else branch
        diag->report(DIAG_ERROR, "Switch statement must be exhaustive");
    } else {
        auto enumType = reinterpret_cast<struct EnumType*>(exprType);
        auto allMemberValues = enumType->memberValues;

        for (auto caseStmt : stmt->cases) {
            assert(caseStmt->caseKind == AST_CASE_CONDITION);
            assert(caseStmt->isValidated);

            if (caseStmt->condition->kind == AST_IDENTIFIER) {
                auto identExpr = reinterpret_cast<ASTIdentExpr*>(caseStmt->condition);
                if (identExpr->decl && identExpr->decl->kind == AST_ENUM_ELEMENT) {
                    auto enumElementDecl = reinterpret_cast<ASTEnumElementDecl*>(identExpr->decl);
                    assert(enumElementDecl->assignment);
                    assert(enumElementDecl->assignment->kind == AST_INT_LITERAL);

                    auto intLit = reinterpret_cast<ASTIntLit*>(enumElementDecl->assignment);
                    for (auto it = allMemberValues.begin(); it != allMemberValues.end(); it++) {
                        if ((*it)->value == intLit->value) {
                            allMemberValues.erase(it);
                            break;
                        }
                    }
                }
            }
        }

        if (!allMemberValues.empty()) {
            diag->report(DIAG_ERROR, "Switch statement must be exhaustive");
        }
    }
}
