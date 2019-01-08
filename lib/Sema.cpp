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

Sema::Sema(CodeManager* codeManager) :
codeManager(codeManager),
context(&codeManager->context),
diag(&codeManager->diag) {
}

void Sema::validateAST() {
    auto module = context->getModule();

    // @Stability This should be implemented with a reverse_iterator but the std::reverse_iterator is causing an
    //            stack memory corruption so we fallback to declsLast but comparing it to declsEnd is semantically incorrect!
    for (auto it = module->declsLast(); it != module->declsEnd(); it--) {
        inferTypeOfNode(*it);
    }

    for (auto it = module->declsBegin(); it != module->declsEnd(); it++) {
        typeCheckNode(*it);
    }
}

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
                auto decl = opaqueTypeRef->declContext->lookupDeclInHierarchy(opaqueTypeRef->typeName);
                if (decl && decl->isNamedDecl()) {
                    opaqueTypeRef->decl = reinterpret_cast<ASTNamedDecl*>(decl);
                }
            }

            if (opaqueTypeRef->decl) {
                inferTypeOfNode(opaqueTypeRef->decl);
                opaqueTypeRef->type = opaqueTypeRef->decl->type;
                return;
            }

            opaqueTypeRef->type = context->findTypeByName(opaqueTypeRef->typeName);
            if (opaqueTypeRef->type) {
                return;
            }

            opaqueTypeRef->type = context->getErrorType();
            diag->report(DIAG_ERROR, "Couldn't resolve type '{0}'", opaqueTypeRef->typeName);
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

        case AST_COMPOUND_STMT:
            return inferTypeOfStmts(reinterpret_cast<ASTCompoundStmt*>(node));

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
            inferTypeOfNode(guard->condition);
            inferTypeOfNode(guard->elseStmt);
        }   return;

        case AST_IF: {
            auto ifStmt = reinterpret_cast<ASTIfStmt*>(node);
            inferTypeOfNode(ifStmt->condition);

            if (ifStmt->chainKind == AST_CHAIN_ELSE) {
                inferTypeOfNode(ifStmt->elseStmt);
            } else if (ifStmt->chainKind == AST_CHAIN_IF) {
                inferTypeOfNode(ifStmt->elseIf);
            }

            inferTypeOfStmts(ifStmt->thenStmt);
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

            inferTypeOfStmts(caseStmt->body);
        }   return;

        case AST_DO: {
            auto doStmt = reinterpret_cast<ASTDoStmt*>(node);
            inferTypeOfNode(doStmt->condition);
            inferTypeOfStmts(doStmt->body);
        }   return;

        case AST_WHILE: {
            auto whileStmt = reinterpret_cast<ASTWhileStmt*>(node);
            inferTypeOfNode(whileStmt->condition);
            inferTypeOfStmts(whileStmt->body);
        }   return;

        case AST_FUNC:
        case AST_PREFIX_FUNC:
        case AST_INFIX_FUNC:
            return typeFuncDecl(reinterpret_cast<ASTFuncDecl*>(node));

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

// @Incomplete reinfer type of literal based on the context it is used in
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
    }

    resolveType(var->typeRef);
    var->type = var->typeRef->type;
}

void Sema::inferTypeOfLetDecl(ASTLetDecl* let) {
    if (let->type) { return; }

    if (let->assignment) {
        inferTypeOfNode(let->assignment);
    }

    resolveType(let->typeRef);
    let->type = let->typeRef->type;
}

void Sema::inferTypeOfIdentExpr(ASTIdentExpr* expr) {
    auto decl = expr->declContext->lookupDeclInHierarchy(expr->declName);
    if (decl && decl->isNamedDecl()) {
        expr->decl = reinterpret_cast<ASTNamedDecl*>(decl);
        inferTypeOfNode(expr->decl);
        expr->type = expr->decl->type;
        return;
    }

    auto module = context->getModule();
    for (auto it = module->declsBegin(); it != module->declsEnd(); it++) {
        if ((*it)->kind == AST_ENUM) {
            auto enumDecl = reinterpret_cast<ASTEnumDecl*>(*it);
            auto decl = enumDecl->lookupDecl(expr->declName);
            if (decl) {
                assert(decl->kind == AST_ENUM_ELEMENT);
                expr->decl = reinterpret_cast<ASTEnumElementDecl*>(decl);
                inferTypeOfNode(expr->decl);
                expr->type = expr->decl->type;
                return;
            }
        }
    }

    expr->type = context->getErrorType();
    diag->report(DIAG_ERROR, "Unresolved identifier '{0}'", expr->declName);
}

void Sema::inferTypeOfStmts(ASTCompoundStmt* stmt) {
    for (auto it = stmt->stmts.rbegin(); it != stmt->stmts.rend(); it++) {
        inferTypeOfNode(*it);
    }
}

void Sema::inferTypeOfUnaryExpr(ASTUnaryExpr* expr) {
    if (expr->type) { return; }

    inferTypeOfNode(expr->right);
    if (expr->right->type == context->getErrorType()) {
        expr->type = context->getErrorType();
        return;
    }

    auto decl = context->getModule()->lookupDecl(expr->op.text);
    if (decl->isPrefixFunc()) {
        auto funcDecl = reinterpret_cast<ASTFuncDecl*>(decl);
        typeFuncDecl(funcDecl);
        if (funcDecl->type == context->getErrorType()) {
            expr->type = context->getErrorType();
            return;
        }

        assert(funcDecl->parameters.size() == 1 && "Wellformed ASTPrefixFuncDecl can only contain 1 parameter!");
        if (funcDecl->parameters[0]->type == expr->right->type) {
            expr->type = funcDecl->type;
        } else {
            expr->type = context->getErrorType();
            diag->report(DIAG_ERROR, "Couldn't resolve prefix function '{0}'", expr->op.text);
        }
    }
}

void Sema::inferTypeOfBinaryExpr(ASTBinaryExpr* expr) {
    if (expr->type) { return; }

    inferTypeOfNode(expr->left);
    inferTypeOfNode(expr->right);

    if (expr->left->type == context->getErrorType() || expr->right->type == context->getErrorType()) {
        expr->type = context->getErrorType();
        return;
    }

    auto decl = context->getModule()->lookupDecl(expr->op.text);
    if (decl->isInfixFunc()) {
        auto funcDecl = reinterpret_cast<ASTFuncDecl*>(decl);
        typeFuncDecl(funcDecl);
        if (funcDecl->type == context->getErrorType()) {
            expr->type = context->getErrorType();
            return;
        }

        assert(funcDecl->parameters.size() == 2 && "Wellformed ASTInfixFuncDecl can only contain 2 parameters!");
        if (funcDecl->parameters[0]->type == expr->left->type && funcDecl->parameters[1]->type == expr->right->type) {
            expr->type = funcDecl->type;
        } else {
            expr->type = context->getErrorType();
            diag->report(DIAG_ERROR, "Couldn't resolve infix function '{0}'", expr->op.text);
        }
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
    auto memberTypeIt = structType->memberTypes.find(expr->memberName);
    if (memberTypeIt == structType->memberTypes.end()) {
        expr->type = context->getErrorType();
        diag->report(DIAG_ERROR, "No member named '{0}' found in struct type", expr->memberName);
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
    }

    expr->type = funcType->returnType;
}

void Sema::inferTypeOfSubscriptExpr(ASTSubscriptExpr* expr) {
    if (expr->type) { return; }

    llvm::report_fatal_error("Implementation missing!");
}

void Sema::typeFuncDecl(ASTFuncDecl* decl) {
    if (decl->type) { return; }

    if (decl->isPrefixFunc() && decl->parameters.size() != 1) {
        decl->type = context->getErrorType();
        diag->report(DIAG_ERROR, "Prefix function '{0}' must contain 1 parameter", decl->name);
        return;
    }

    if (decl->isInfixFunc() && decl->parameters.size() != 2) {
        decl->type = context->getErrorType();
        diag->report(DIAG_ERROR, "Infix function '{0}' must contain 2 parameters", decl->name);
        return;
    }

    for (auto paramDecl : decl->parameters) {
        typeParamDecl(paramDecl);

        if (paramDecl->type == context->getErrorType()) {
            decl->type = context->getErrorType();
            return;
        }
    }

    inferTypeOfStmts(decl->body);
    resolveType(decl->returnTypeRef);
    if (decl->returnTypeRef->type == context->getErrorType()) {
        decl->type = context->getErrorType();
        return;
    }

    decl->type = context->getFuncType(decl);
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
    llvm::StringMap<unsigned> memberIndexes;

    unsigned memberIndex = 0;
    for (auto it = decl->declsLast(); it != decl->declsEnd(); it--) {
        assert((*it)->kind == AST_VAR || (*it)->kind == AST_LET);
        auto memberDecl = reinterpret_cast<ASTValueDecl*>(*it);
        inferTypeOfNode(memberDecl);

        if (memberDecl->type == context->getErrorType()) {
            decl->type = context->getErrorType();
            return;
        }

        if (memberDecl->kind == AST_VAR || memberDecl->kind == AST_LET) {
            assert(memberDecl->type && "Type shouldn't be nil when written to MemberTypes!");
            memberTypes.try_emplace(memberDecl->name, memberDecl->type);
            memberIndexes.try_emplace(memberDecl->name, memberIndex);
            memberIndex += 1;
        }

        // @Incomplete report error if member is not a var or let !
    }

    auto it = context->getTypes()->find(decl->name);
    if (it != context->getTypes()->end()) {
        decl->type = context->getErrorType();
        diag->report(DIAG_ERROR, "Invalid redeclaration of '{0}'", decl->name);
        return;
    }

    decl->type = context->getStructType(decl->name, memberTypes, memberIndexes);
}

void Sema::typeEnumDecl(ASTEnumDecl* decl) {
    if (decl->type) { return; }

    for (auto it = decl->declsLast(); it != decl->declsEnd(); it--) {
        inferTypeOfNode(*it);
    }

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
    for (auto it = structDecl->declsBegin(); it != structDecl->declsEnd(); it++) {
        assert((*it)->kind == AST_VAR || (*it)->kind == AST_LET);
        auto decl = reinterpret_cast<ASTValueDecl*>(*it);

        auto valueDecl = reinterpret_cast<ASTValueDecl*>(decl);
        if (valueDecl->typeRef->kind == AST_OPAQUE_TYPE_REF) {
            auto opaqueTypeRef = reinterpret_cast<ASTOpaqueTypeRef*>(valueDecl->typeRef);
            if (!opaqueTypeRef->decl) {
                auto foundDecl = structDecl->lookupDeclInHierarchy(opaqueTypeRef->typeName);
                if (foundDecl && foundDecl->isNamedDecl()) {
                    opaqueTypeRef->decl = reinterpret_cast<ASTNamedDecl*>(foundDecl);
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
    return false;
}

void Sema::typeCheckNode(ASTNode* node) {
    if (node->isValidated) { return; }
    defer(node->isValidated = true);

    switch (node->kind) {
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

    for (auto paramDecl : decl->parameters) {
        typeCheckParamDecl(paramDecl);
    }

    typeCheckFuncBody(decl);
}

void Sema::typeCheckFuncBody(ASTFuncDecl* decl) {
    if (decl->body->isValidated) { return; }
    defer(decl->body->isValidated = true);

    for (auto stmt : decl->body->stmts) {
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
        if (!isCompoundStmtAlwaysReturning(decl->body)) {
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

    typeCheckStructMembers(decl);

    // @Incomplete ...
}

void Sema::typeCheckStructMembers(ASTStructDecl* decl) {
    for (auto it = decl->declsBegin(); it != decl->declsEnd(); it++) {
        typeCheckNode(*it);
    }
}

void Sema::typeCheckVarDecl(ASTVarDecl* decl) {
    if (decl->isValidated) { return; }
    defer(decl->isValidated = true);

    if (decl->assignment) {
        typeCheckExpr(decl->assignment);

        if (decl->type != decl->assignment->type && decl->assignment->type != context->getErrorType()) {
            decl->type = context->getErrorType();
            diag->report(DIAG_ERROR, "Assignment expression of '{0}' has mismatching type", decl->name);
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
            diag->report(DIAG_ERROR, "Assignment expression of '{0}' has mismatching type", decl->name);
        }
    } else {
        decl->type = context->getErrorType();
        diag->report(DIAG_ERROR, "Expected assignment expression for '{0}'", decl->name);
    }
}

void Sema::typeCheckEnumDecl(ASTEnumDecl* decl) {
    if (decl->isValidated) { return; }
    defer(decl->isValidated = true);

    for (auto it = decl->declsBegin(); it != decl->declsEnd(); it++) {
        auto memberDecl = *it;
        assert(memberDecl->kind == AST_ENUM_ELEMENT);
        auto elementDecl = reinterpret_cast<ASTEnumElementDecl*>(memberDecl);
        typeCheckEnumElementDecl(elementDecl);
    }
}

void Sema::typeCheckEnumElementDecl(ASTEnumElementDecl* decl) {
    if (decl->isValidated) { return; }
    defer(decl->isValidated = true);

    if (decl->type == context->getErrorType()) {
        return;
    }

    if (!decl->declContext->isEnumDecl()) {
        diag->report(DIAG_ERROR, "Enum element '{0}' can only be declared inside of an enum", decl->name);
        return;
    }

    auto enumDecl = static_cast<ASTEnumDecl*>(decl->declContext);
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
            diag->report(DIAG_ERROR, "Assignment expression of '{0}' has mismatching type", decl->name);
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

void Sema::typeCheckCompoundStmt(ASTCompoundStmt* stmt) {
    if (stmt->isValidated) { return; }
    defer(stmt->isValidated = true);

    for (auto child : stmt->stmts) {
        typeCheckNode(child);
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

    if (expr->left->type == context->getErrorType()) {
        return;
    }

    assert(expr->left->type->kind == TYPE_DECL_STRUCT);
    auto structType = reinterpret_cast<StructType*>(expr->left->type);
    expr->memberIndex = structType->memberIndexes.lookup(expr->memberName);

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
    auto parent = stmt->parent;
    while (parent) {
        if (parent->isDecl()) {
            break;
        }

        if (parent->kind == AST_SWITCH || parent->kind == AST_WHILE || parent->kind == AST_DO || parent->kind == AST_FOR) {
            isBreakStmtAllowed = true;
            break;
        }

        parent = parent->parent;
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
    auto parent = stmt->parent;
    while (parent) {
        if (parent->isDecl()) {
            break;
        }

        if (parent->kind == AST_WHILE || parent->kind == AST_DO || parent->kind == AST_FOR) {
            isContinueStmtAllowed = true;
            break;
        }

        parent = parent->parent;
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
    auto parent = stmt->parent;
    while (parent) {
        if (parent->isDecl()) {
            break;
        }

        if (parent->kind == AST_SWITCH_CASE) {
            isFallthroughStmtAllowed = true;
            break;
        }

        parent = parent->parent;
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
    typeCheckCompoundStmt(stmt->elseStmt);

    if (!isCompoundStmtAlwaysReturning(stmt->elseStmt)) {
        diag->report(DIAG_ERROR, "Not all code paths return a value");
    }
}

void Sema::typeCheckIfStmt(ASTIfStmt* stmt) {
    if (stmt->isValidated) { return; }
    defer(stmt->isValidated = true);

    typeCheckConditions(stmt);
    typeCheckCompoundStmt(stmt->thenStmt);

    switch (stmt->chainKind) {
        case AST_CHAIN_NONE:
            break;

        case AST_CHAIN_ELSE:
            typeCheckCompoundStmt(stmt->elseStmt);
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

    if (stmt->body->stmts.empty()) {
        diag->report(DIAG_ERROR, "Switch case should contain at least one statement");
    }

    typeCheckCompoundStmt(stmt->body);

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
    typeCheckCompoundStmt(stmt->body);
}

void Sema::typeCheckConditions(ASTBranchStmt* stmt) {
    typeCheckExpr(stmt->condition);

    if (stmt->condition->type != context->getBoolType() && stmt->condition->type != context->getErrorType()) {
        stmt->condition->type = context->getErrorType();
        diag->report(DIAG_ERROR, "Mismatching condition type expected 'Bool'");
    }
}

bool Sema::isCompoundStmtAlwaysReturning(ASTCompoundStmt* stmt) {
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
    auto parent = stmt->parent;
    while (parent) {
        if (parent->isDecl()) {
            break;
        }

        if (parent->kind == AST_DO || parent->kind == AST_WHILE) {
            isContinueStmtAllowed = true;
            break;
        }

        parent = parent->parent;
    }

    for (auto child : stmt->stmts) {
        if (child->kind == AST_RETURN || (child->kind == AST_CONTINUE && isContinueStmtAllowed)) {
            return true;
        }

        if (child->kind == AST_IF) {
            auto ifStmt = reinterpret_cast<ASTIfStmt*>(child);
            if (isIfStmtAlwaysReturning(ifStmt)) {
                return true;
            }
        }

        if (child->kind == AST_SWITCH) {
            auto switchStmt = reinterpret_cast<ASTSwitchStmt*>(child);
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
            return isCompoundStmtAlwaysReturning(stmt->thenStmt) && isCompoundStmtAlwaysReturning(stmt->elseStmt);

        case AST_CHAIN_IF:
            return isCompoundStmtAlwaysReturning(stmt->thenStmt) && isIfStmtAlwaysReturning(stmt->elseIf);
    }
}

bool Sema::isSwitchStmtAlwaysRetuning(ASTSwitchStmt* stmt) {
    for (auto caseStmt : stmt->cases) {
        if (!isCompoundStmtAlwaysReturning(caseStmt->body)) {
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
