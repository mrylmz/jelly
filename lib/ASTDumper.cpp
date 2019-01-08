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

#include "Core/ASTDumper.h"

// @Incomplete Add runtime informations like memory address and also other members like types
//             for now this implementation will be helpful to run tests for the Parser...

ASTDumper::ASTDumper(std::ostream& outputStream) : outputStream(outputStream) {
}

void ASTDumper::dumpModule(ASTModuleDecl* module) {
    outputStream << "ASTBlock\n"; // @Incomplete rename to ASTModule and update all affected unit tests
    indentation += 1;

    std::string indentText = "";
    for (auto i = 0; i < indentation - 1; i++) {
        indentText.append("  ");
    }

    if (indentation > 0) {
        indentText.append("| ");
    }

    for (auto it = module->declsBegin(); it != module->declsEnd(); it++) {
        outputStream << indentText;
        dumpNode(*it);
    }

    indentation -= 1;
}

void ASTDumper::dumpNode(ASTNode* node) {
    assert(node);
    switch (node->kind) {
        case AST_LOAD_DIRECTIVE:    return dumpLoad(reinterpret_cast<ASTLoadDirective*>(node));
        case AST_UNARY:             return dumpUnaryExpr(reinterpret_cast<ASTUnaryExpr*>(node));
        case AST_BINARY:            return dumpBinaryExpr(reinterpret_cast<ASTBinaryExpr*>(node));
        case AST_MEMBER_ACCESS:     return dumpMemberAccessExpr(reinterpret_cast<ASTMemberAccessExpr*>(node));
        case AST_IDENTIFIER:        return dumpIdentExpr(reinterpret_cast<ASTIdentExpr*>(node));
        case AST_CALL:              return dumpCallExpr(reinterpret_cast<ASTCallExpr*>(node));
        case AST_SUBSCRIPT:         return dumpSubscriptExpr(reinterpret_cast<ASTSubscriptExpr*>(node));
        case AST_NIL_LITERAL:       return dumpNilLiteral(reinterpret_cast<ASTNilLit*>(node));
        case AST_BOOL_LITERAL:      return dumpBoolLiteral(reinterpret_cast<ASTBoolLit*>(node));
        case AST_INT_LITERAL:       return dumpIntLiteral(reinterpret_cast<ASTIntLit*>(node));
        case AST_FLOAT_LITERAL:     return dumpFloatLiteral(reinterpret_cast<ASTFloatLit*>(node));
        case AST_STRING_LITERAL:    return dumpStringLiteral(reinterpret_cast<ASTStringLit*>(node));
        case AST_PARAM_DECL:        return dumpParamDecl(reinterpret_cast<ASTParamDecl*>(node));
        case AST_FUNC_DECL:         return dumpFuncDecl(reinterpret_cast<ASTFuncDecl*>(node));
        case AST_VALUE_DECL:        return dumpValueDecl(reinterpret_cast<ASTValueDecl*>(node));
        case AST_STRUCT_DECL:       return dumpStructDecl(reinterpret_cast<ASTStructDecl*>(node));
        case AST_ENUM_ELEMENT_DECL: return dumpEnumElementDecl(reinterpret_cast<ASTEnumElementDecl*>(node));
        case AST_ENUM_DECL:         return dumpEnumDecl(reinterpret_cast<ASTEnumDecl*>(node));
        case AST_COMPOUND_STMT:     return dumpCompoundStmt(reinterpret_cast<ASTCompoundStmt*>(node));
        case AST_BREAK:             return dumpBreakStmt(reinterpret_cast<ASTBreakStmt*>(node));
        case AST_CONTINUE:          return dumpContinueStmt(reinterpret_cast<ASTContinueStmt*>(node));
        case AST_FALLTHROUGH:       return dumpFallthroughStmt(reinterpret_cast<ASTFallthroughStmt*>(node));
        case AST_RETURN:            return dumpReturnStmt(reinterpret_cast<ASTReturnStmt*>(node));
        case AST_DEFER:             return dumpDeferStmt(reinterpret_cast<ASTDeferStmt*>(node));
        case AST_FOR:               return dumpForStmt(reinterpret_cast<ASTForStmt*>(node));
        case AST_GUARD:             return dumpGuardStmt(reinterpret_cast<ASTGuardStmt*>(node));
        case AST_IF:                return dumpIfStmt(reinterpret_cast<ASTIfStmt*>(node));
        case AST_DO:                return dumpDoStmt(reinterpret_cast<ASTDoStmt*>(node));
        case AST_WHILE:             return dumpWhileStmt(reinterpret_cast<ASTWhileStmt*>(node));
        case AST_SWITCH_CASE:       return dumpCaseStmt(reinterpret_cast<ASTCaseStmt*>(node));
        case AST_SWITCH:            return dumpSwitchStmt(reinterpret_cast<ASTSwitchStmt*>(node));
        case AST_ANY_TYPE_REF:      return dumpAnyTypeRef(reinterpret_cast<ASTAnyTypeRef*>(node));
        case AST_OPAQUE_TYPE_REF:   return dumpOpaqueTypeRef(reinterpret_cast<ASTOpaqueTypeRef*>(node));
        case AST_TYPEOF_TYPE_REF:   return dumpTypeOfTypeRef(reinterpret_cast<ASTTypeOfTypeRef*>(node));
        case AST_POINTER_TYPE_REF:  return dumpPointerTypeRef(reinterpret_cast<ASTPointerTypeRef*>(node));
        case AST_ARRAY_TYPE_REF:    return dumpArrayTypeRef(reinterpret_cast<ASTArrayTypeRef*>(node));
        default:                    llvm_unreachable("Invalid kind given for ASTNode!");
    }
}

void ASTDumper::dumpCompoundStmt(ASTCompoundStmt* stmt) {
    outputStream << "ASTBlock\n"; // @Incomplete rename to ASTCompoundStmt
    dumpChildren(stmt->stmts);
}

void ASTDumper::dumpLoad(ASTLoadDirective* directive) {
    outputStream << "ASTLoad { loadFilePath = '" << directive->loadFilePath.str() << "' }\n";
}

void ASTDumper::dumpUnaryExpr(ASTUnaryExpr* expr) {
    outputStream << "ASTUnaryExpr { operator = '" << expr->op.text.str() << "' }\n";
    dumpChildren({ expr->right });
}

void ASTDumper::dumpBinaryExpr(ASTBinaryExpr* expr) {
    outputStream << "ASTBinaryExpr { operator = '" << expr->op.text.str() << "' }\n";
    dumpChildren({ expr->left, expr->right });
}

void ASTDumper::dumpMemberAccessExpr(ASTMemberAccessExpr* expr) {
    outputStream << "ASTMemberAccessExpr { memberName = '" << expr->memberName->str() << "' }\n";
    dumpChildren({ expr->left });
}

void ASTDumper::dumpIdentExpr(ASTIdentExpr* expr) {
    outputStream << "ASTIdentExpr { declName = '" << expr->declName->str() << "' }\n";
}

void ASTDumper::dumpCallExpr(ASTCallExpr* expr) {
    outputStream << "ASTCallExpr\n";
    dumpChildren({ expr->left });
    dumpChildren(expr->args);
}

void ASTDumper::dumpSubscriptExpr(ASTSubscriptExpr* expr) {
    outputStream << "ASTSubscriptExpr\n";
    dumpChildren({ expr->left });
    dumpChildren(expr->args);
}

void ASTDumper::dumpNilLiteral(ASTNilLit* literal) {
    outputStream << "ASTNilLit\n";
}

void ASTDumper::dumpBoolLiteral(ASTBoolLit* literal) {
    outputStream << "ASTBoolLit { value = '" << literal->value << "' }\n";
}

void ASTDumper::dumpIntLiteral(ASTIntLit* literal) {
    outputStream << "ASTIntLit { value = '" << literal->value.toString(10, true) << "' }\n";
}

void ASTDumper::dumpFloatLiteral(ASTFloatLit* literal) {
    outputStream << "ASTFloatLit { value = '" << literal->value << "' }\n";
}

void ASTDumper::dumpStringLiteral(ASTStringLit* literal) {
    outputStream << "ASTStringLit { value = '" << literal->value.str() << "' }\n";
}

void ASTDumper::dumpParamDecl(ASTParamDecl* decl) {
    outputStream << "ASTParamDecl { name = '" << decl->name->str() << "' }\n";
    dumpChildren({ decl->typeRef });
}

void ASTDumper::dumpFuncDecl(ASTFuncDecl* decl) {
    outputStream << "ASTFuncDecl { name = '" << decl->name->str() << "' }\n";
    dumpChildren(decl->parameters);
    dumpChildren({ decl->returnTypeRef, decl->body });
}

void ASTDumper::dumpValueDecl(ASTValueDecl* decl) {
    outputStream << "ASTValueDecl { name = '" << decl->name->str() << "', isConstant = " << decl->isConstant << " }\n";
    dumpChildren({ decl->typeRef });
    if (decl->initializer) {
        dumpChildren({ decl->initializer });
    }
}

void ASTDumper::dumpStructDecl(ASTStructDecl* decl) {
    outputStream << "ASTStructDecl { name = '" << decl->name->str() << "' }\n";

    // @Incomplete remove ASTBlock + indentation
    indentation += 1;
    std::string indentText = "";
    for (auto i = 0; i < indentation - 1; i++) {
        indentText.append("  ");
    }

    if (indentation > 0) {
        indentText.append("| ");
    }

    outputStream << indentText;
    outputStream << "ASTBlock\n";
    dumpDeclContext(decl);

    indentation -= 1;
}

void ASTDumper::dumpEnumElementDecl(ASTEnumElementDecl* decl) {
    outputStream << "ASTEnumElementDecl { name = '" << decl->name->str() << "' }\n";
    if (decl->assignment) {
        dumpChildren({ decl->assignment });
    }
}

void ASTDumper::dumpEnumDecl(ASTEnumDecl* decl) {
    outputStream << "ASTEnumDecl { name = '" << decl->name->str() << "' }\n";

    // @Incomplete remove ASTBlock + indentation
    indentation += 1;
    std::string indentText = "";
    for (auto i = 0; i < indentation - 1; i++) {
        indentText.append("  ");
    }

    if (indentation > 0) {
        indentText.append("| ");
    }

    outputStream << indentText;
    outputStream << "ASTBlock\n";
    dumpDeclContext(decl);

    indentation -= 1;
}

void ASTDumper::dumpBreakStmt(ASTBreakStmt* stmt) {
    outputStream << "ASTBreakStmt\n";
}

void ASTDumper::dumpContinueStmt(ASTContinueStmt* stmt) {
    outputStream << "ASTContinueStmt\n";
}

void ASTDumper::dumpFallthroughStmt(ASTFallthroughStmt* stmt) {
    outputStream << "ASTFallthroughStmt\n";
}

void ASTDumper::dumpReturnStmt(ASTReturnStmt* stmt) {
    outputStream << "ASTReturnStmt\n";
    if (stmt->expr) {
        dumpChildren({ stmt->expr });
    }
}

void ASTDumper::dumpDeferStmt(ASTDeferStmt* stmt) {
    outputStream << "ASTDeferStmt\n";
    dumpChildren({ stmt->expr });
}

void ASTDumper::dumpForStmt(ASTForStmt* stmt) {
    outputStream << "ASTForStmt { elementName = '" << stmt->elementName->str() << "' }\n";
    dumpChildren({ stmt->sequenceExpr, stmt->body });
}

void ASTDumper::dumpGuardStmt(ASTGuardStmt* stmt) {
    outputStream << "ASTGuardStmt\n";
    dumpChildren({ stmt->condition, stmt->elseStmt });
}

void ASTDumper::dumpIfStmt(ASTIfStmt* stmt) {
    outputStream << "ASTIfStmt\n";
    dumpChildren({ stmt->condition, stmt->thenStmt });

    if (stmt->chainKind == AST_CHAIN_ELSE) {
        dumpChildren({ stmt->elseStmt });
    } else if (stmt->chainKind == AST_CHAIN_IF) {
        dumpChildren({ stmt->elseIf });
    }
}

void ASTDumper::dumpDoStmt(ASTDoStmt* stmt) {
    outputStream << "ASTDoStmt\n";
    dumpChildren({ stmt->condition, stmt->body });
}

void ASTDumper::dumpWhileStmt(ASTWhileStmt* stmt) {
    outputStream << "ASTWhileStmt\n";
    dumpChildren({ stmt->condition, stmt->body });
}

void ASTDumper::dumpCaseStmt(ASTCaseStmt* stmt) {
    outputStream << "ASTCaseStmt { caseKind = '";
    if (stmt->caseKind == AST_CASE_CONDITION) {
        outputStream << "case";
    } else {
        outputStream << "else";
    }
    outputStream << "' }\n";

    if (stmt->condition) {
        dumpChildren({ stmt->condition });
    }
    dumpChildren({ stmt->body });
}

void ASTDumper::dumpSwitchStmt(ASTSwitchStmt* stmt) {
    outputStream << "ASTSwitchStmt\n";
    dumpChildren({ stmt->expr });
    dumpChildren(stmt->cases);
}

void ASTDumper::dumpAnyTypeRef(ASTAnyTypeRef* typeRef) {
    outputStream << "ASTAnyTypeRef\n";
}

void ASTDumper::dumpOpaqueTypeRef(ASTOpaqueTypeRef* typeRef) {
    outputStream << "ASTOpaqueTypeRef { typeName = '" << typeRef->typeName->str() << "' }\n";
}

void ASTDumper::dumpTypeOfTypeRef(ASTTypeOfTypeRef* typeRef) {
    outputStream << "ASTTypeOfTypeRef\n";
    dumpChildren({ typeRef->expr });
}

void ASTDumper::dumpPointerTypeRef(ASTPointerTypeRef* typeRef) {
    outputStream << "ASTPointerTypeRef { depth = '" << typeRef->depth << "' }\n";
    dumpChildren({ typeRef->pointeeTypeRef });
}

void ASTDumper::dumpArrayTypeRef(ASTArrayTypeRef* typeRef) {
    outputStream << "ASTArrayTypeRef\n";
    dumpChildren({ typeRef->elementTypeRef });
    if (typeRef->sizeExpr) {
        dumpChildren({ typeRef->sizeExpr });
    }
}
