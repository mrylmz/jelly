#include "JellyCore/ASTFunctions.h"
#include "JellyCore/ASTScope.h"
#include "JellyCore/Diagnostic.h"
#include "JellyCore/NameResolution.h"

// TODO: @CandidateDeclarations Add support for resolution of candidate declarations to allow contextual resolutions
// TODO: Resolve control flow paths for loop, control, ... statements

static inline void _AddSourceUnitRecordDeclarationsToScope(ASTSourceUnitRef sourceUnit);
static inline Bool _ResolveDeclarationsOfFunctionSignature(ASTFunctionDeclarationRef function);
static inline Bool _ResolveDeclarationsOfType(ASTScopeRef scope, ASTTypeRef type);
static inline void _PerformNameResolutionForEnumerationBody(ASTEnumerationDeclarationRef enumeration);
static inline void _PerformNameResolutionForFunctionBody(ASTFunctionDeclarationRef function);
static inline void _PerformNameResolutionForStructureBody(ASTStructureDeclarationRef structure);
static inline void _PerformNameResolutionForNode(ASTNodeRef node);
static inline void _PerformNameResolutionForExpression(ASTExpressionRef expression);

void PerformNameResolution(ASTModuleDeclarationRef module) {
    for (Index index = 0; index < ASTArrayGetElementCount(module->sourceUnits); index++) {
        ASTSourceUnitRef sourceUnit = (ASTSourceUnitRef)ASTArrayGetElementAtIndex(module->sourceUnits, index);
        _AddSourceUnitRecordDeclarationsToScope(sourceUnit);
    }

    for (Index index = 0; index < ASTArrayGetElementCount(module->sourceUnits); index++) {
        ASTSourceUnitRef sourceUnit = (ASTSourceUnitRef)ASTArrayGetElementAtIndex(module->sourceUnits, index);
        for (Index sourceUnitIndex = 0; sourceUnitIndex < ASTArrayGetElementCount(sourceUnit->declarations); sourceUnitIndex++) {
            ASTNodeRef child = (ASTNodeRef)ASTArrayGetElementAtIndex(sourceUnit->declarations, sourceUnitIndex);
            if (child->tag == ASTTagFunctionDeclaration) {
                ASTFunctionDeclarationRef function = (ASTFunctionDeclarationRef)child;
                if (_ResolveDeclarationsOfFunctionSignature(function)) {
                    if (ASTScopeLookupDeclarationByNameOrMatchingFunctionSignature(child->scope, function->base.name, function->parameters,
                                                                                   function->returnType) == NULL) {
                        ASTArrayAppendElement(child->scope->declarations, (ASTDeclarationRef)child);
                    } else {
                        ReportError("Invalid redeclaration of identifier");
                    }
                }
            }
        }
    }

    for (Index index = 0; index < ASTArrayGetElementCount(module->sourceUnits); index++) {
        ASTSourceUnitRef sourceUnit = (ASTSourceUnitRef)ASTArrayGetElementAtIndex(module->sourceUnits, index);
        for (Index sourceUnitIndex = 0; sourceUnitIndex < ASTArrayGetElementCount(sourceUnit->declarations); sourceUnitIndex++) {
            ASTNodeRef child = (ASTNodeRef)ASTArrayGetElementAtIndex(sourceUnit->declarations, sourceUnitIndex);
            if (child->tag == ASTTagEnumerationDeclaration) {
                ASTEnumerationDeclarationRef enumeration = (ASTEnumerationDeclarationRef)child;
                _PerformNameResolutionForEnumerationBody(enumeration);
                continue;
            }

            if (child->tag == ASTTagFunctionDeclaration) {
                ASTFunctionDeclarationRef function = (ASTFunctionDeclarationRef)child;
                _PerformNameResolutionForFunctionBody(function);
                continue;
            }

            if (child->tag == ASTTagStructureDeclaration) {
                ASTStructureDeclarationRef structure = (ASTStructureDeclarationRef)child;
                _PerformNameResolutionForStructureBody(structure);
                continue;
            }
        }
    }
}

static inline void _AddSourceUnitRecordDeclarationsToScope(ASTSourceUnitRef sourceUnit) {
    for (Index index = 0; index < ASTArrayGetElementCount(sourceUnit->declarations); index++) {
        ASTNodeRef child = (ASTNodeRef)ASTArrayGetElementAtIndex(sourceUnit->declarations, index);
        if (child->tag == ASTTagEnumerationDeclaration || child->tag == ASTTagStructureDeclaration) {
            ASTDeclarationRef declaration = (ASTDeclarationRef)child;
            if (ASTScopeLookupDeclarationByName(child->scope, declaration->name) == NULL) {
                ASTArrayAppendElement(child->scope->declarations, declaration);
            } else {
                ReportError("Invalid redeclaration of identifier");
            }
        }

        if (child->tag == ASTTagValueDeclaration) {
            ASTValueDeclarationRef value = (ASTValueDeclarationRef)child;
            if (value->kind == ASTValueKindVariable) {
                if (ASTScopeLookupDeclarationByName(child->scope, value->base.name) == NULL) {
                    ASTArrayAppendElement(child->scope->declarations, (ASTDeclarationRef)value);
                } else {
                    ReportError("Invalid redeclaration of identifier");
                }
            }
        }
    }
}

static inline Bool _ResolveDeclarationsOfFunctionSignature(ASTFunctionDeclarationRef function) {
    Bool success = true;
    for (Index index = 0; index < ASTArrayGetElementCount(function->parameters); index++) {
        ASTValueDeclarationRef value = (ASTValueDeclarationRef)ASTArrayGetElementAtIndex(function->parameters, index);
        success                      = success && _ResolveDeclarationsOfType(function->base.base.scope, value->base.type);
    }

    success = success && _ResolveDeclarationsOfType(function->base.base.scope, function->returnType);
    return success;
}

static inline Bool _ResolveDeclarationsOfType(ASTScopeRef scope, ASTTypeRef type) {
    switch (type->tag) {
    case ASTTagOpaqueType: {
        ASTOpaqueTypeRef opaque  = (ASTOpaqueTypeRef)type;
        ASTScopeRef currentScope = scope;
        while (currentScope) {
            ASTDeclarationRef declaration = ASTScopeLookupDeclarationByName(currentScope, opaque->name);
            if (declaration) {
                opaque->declaration = declaration;
                return true;
            }

            currentScope = ASTScopeGetNextParentForLookup(currentScope);
        }

        ReportError("Use of unresolved type");
        return false;
    }

    case ASTTagPointerType: {
        ASTPointerTypeRef pointer = (ASTPointerTypeRef)type;
        return _ResolveDeclarationsOfType(scope, pointer->pointeeType);
    }

    case ASTTagArrayType: {
        ASTArrayTypeRef array = (ASTArrayTypeRef)type;
        return _ResolveDeclarationsOfType(scope, array->elementType);
    }

    case ASTTagBuiltinType:
    case ASTTagEnumerationType:
    case ASTTagFunctionType:
    case ASTTagStructureType:
        return true;

    default:
        JELLY_UNREACHABLE("Invalid tag given for ASTType");
        return false;
    }
}

static inline void _PerformNameResolutionForEnumerationBody(ASTEnumerationDeclarationRef enumeration) {
    for (Index index = 0; index < ASTArrayGetElementCount(enumeration->elements); index++) {
        ASTValueDeclarationRef element = (ASTValueDeclarationRef)ASTArrayGetElementAtIndex(enumeration->elements, index);
        assert(element->base.base.tag == ASTTagValueDeclaration);
        assert(element->kind == ASTValueKindEnumerationElement);

        if (ASTScopeLookupDeclarationByName(enumeration->innerScope, element->base.name) == NULL) {
            ASTArrayAppendElement(enumeration->innerScope->declarations, element);
        } else {
            ReportError("Invalid redeclaration of identifier");
        }
    }
}

static inline void _PerformNameResolutionForFunctionBody(ASTFunctionDeclarationRef function) {
    for (Index parameterIndex = 0; parameterIndex < ASTArrayGetElementCount(function->parameters); parameterIndex++) {
        ASTValueDeclarationRef parameter = (ASTValueDeclarationRef)ASTArrayGetElementAtIndex(function->parameters, parameterIndex);
        if (ASTScopeLookupDeclarationByName(function->innerScope, parameter->base.name) == NULL) {
            ASTArrayAppendElement(function->innerScope->declarations, parameter);
        } else {
            ReportError("Invalid redeclaration of identifier");
        }
    }

    for (Index index = 0; index < ASTArrayGetElementCount(function->body->statements); index++) {
        ASTNodeRef node = (ASTNodeRef)ASTArrayGetElementAtIndex(function->body->statements, index);
        _PerformNameResolutionForNode(node);
    }
}

static inline void _PerformNameResolutionForStructureBody(ASTStructureDeclarationRef structure) {
    for (Index index = 0; index < ASTArrayGetElementCount(structure->values); index++) {
        ASTValueDeclarationRef value = (ASTValueDeclarationRef)ASTArrayGetElementAtIndex(structure->values, index);
        assert(value->base.base.tag == ASTTagValueDeclaration);
        assert(value->kind == ASTValueKindVariable);
        if (_ResolveDeclarationsOfType(value->base.base.scope, value->base.type)) {
            if (ASTScopeLookupDeclarationByName(structure->innerScope, value->base.name) == NULL) {
                ASTArrayAppendElement(structure->innerScope->declarations, value);
            } else {
                ReportError("Invalid redeclaration of identifier");
            }
        }
    }
}

static inline void _PerformNameResolutionForNode(ASTNodeRef node) {
    switch (node->tag) {
    case ASTTagBlock: {
        ASTBlockRef block = (ASTBlockRef)node;
        for (Index index = 0; index < ASTArrayGetElementCount(block->statements); index++) {
            ASTNodeRef child = (ASTNodeRef)ASTArrayGetElementAtIndex(block->statements, index);
            _PerformNameResolutionForNode(child);
        }
        break;
    }

    case ASTTagIfStatement: {
        ASTIfStatementRef statement = (ASTIfStatementRef)node;
        _PerformNameResolutionForExpression(statement->condition);
        _PerformNameResolutionForNode((ASTNodeRef)statement->thenBlock);
        _PerformNameResolutionForNode((ASTNodeRef)statement->elseBlock);
        break;
    }

    case ASTTagLoopStatement: {
        ASTLoopStatementRef statement = (ASTLoopStatementRef)node;
        _PerformNameResolutionForExpression(statement->condition);
        _PerformNameResolutionForNode((ASTNodeRef)statement->loopBlock);
        break;
    }

    case ASTTagCaseStatement: {
        ASTCaseStatementRef statement = (ASTCaseStatementRef)node;
        if (statement->kind == ASTCaseKindConditional) {
            _PerformNameResolutionForExpression(statement->condition);
        }
        _PerformNameResolutionForNode((ASTNodeRef)statement->body);
        break;
    }

    case ASTTagSwitchStatement: {
        ASTSwitchStatementRef statement = (ASTSwitchStatementRef)node;
        _PerformNameResolutionForExpression(statement->argument);
        for (Index index = 0; index < ASTArrayGetElementCount(statement->cases); index++) {
            ASTNodeRef node = (ASTNodeRef)ASTArrayGetElementAtIndex(statement->cases, index);
            _PerformNameResolutionForNode(node);
        }
        break;
    }

    case ASTTagControlStatement: {
        ASTControlStatementRef statement = (ASTControlStatementRef)node;
        if (statement->kind == ASTControlKindReturn && statement->result) {
            _PerformNameResolutionForExpression(statement->result);
        }
        break;
    }

    case ASTTagUnaryExpression:
    case ASTTagBinaryExpression:
    case ASTTagIdentifierExpression:
    case ASTTagMemberAccessExpression:
    case ASTTagCallExpression:
    case ASTTagConstantExpression: {
        ASTExpressionRef expression = (ASTExpressionRef)node;
        _PerformNameResolutionForExpression(expression);
        break;
    }

    case ASTTagValueDeclaration: {
        ASTValueDeclarationRef value = (ASTValueDeclarationRef)node;
        assert(value->kind == ASTValueKindVariable);

        if (_ResolveDeclarationsOfType(value->base.base.scope, value->base.type)) {
            if (ASTScopeLookupDeclarationByName(value->base.base.scope, value->base.name) == NULL) {
                ASTArrayAppendElement(value->base.base.scope->declarations, value);
            } else {
                ReportError("Invalid redeclaration of identifier");
            }
        }

        break;
    }

    default:
        JELLY_UNREACHABLE("Invalid tag given for ASTNode!");
        break;
    }
}

static inline void _PerformNameResolutionForExpression(ASTExpressionRef expression) {
    // TODO: Add support for overloaded functions!!!

    if (expression->base.tag == ASTTagUnaryExpression) {
        ASTUnaryExpressionRef unary = (ASTUnaryExpressionRef)expression;
        _PerformNameResolutionForExpression(unary->arguments[0]);
        StringRef name = ASTGetPrefixOperatorName(AllocatorGetSystemDefault(), unary->op);
        // TODO: Resolve declaration for unary operation
        StringDestroy(name);
        return;
    }

    if (expression->base.tag == ASTTagBinaryExpression) {
        ASTBinaryExpressionRef binary = (ASTBinaryExpressionRef)expression;
        _PerformNameResolutionForExpression(binary->arguments[0]);
        _PerformNameResolutionForExpression(binary->arguments[1]);
        StringRef name = ASTGetInfixOperatorName(AllocatorGetSystemDefault(), binary->op);
        // TODO: Resolve declaration for binary operation
        StringDestroy(name);
        return;
    }

    if (expression->base.tag == ASTTagIdentifierExpression) {
        ASTIdentifierExpressionRef identifier = (ASTIdentifierExpressionRef)expression;
        ASTDeclarationRef declaration         = ASTScopeLookupDeclarationInHierarchyByName(expression->base.scope, identifier->name);
        if (declaration) {
            identifier->resolvedDeclaration = declaration;
        } else {
            ReportError("Use of unresolved identifier");
        }
        return;
    }

    if (expression->base.tag == ASTTagMemberAccessExpression) {
        ASTMemberAccessExpressionRef memberAccess = (ASTMemberAccessExpressionRef)expression;
        _PerformNameResolutionForExpression(memberAccess->argument);
        // TODO: @Next Incomplete...
        return;
    }

    if (expression->base.tag == ASTTagCallExpression) {
        ASTCallExpressionRef call = (ASTCallExpressionRef)expression;
        if (call->callee->base.tag == ASTTagIdentifierExpression) {
            ASTIdentifierExpressionRef identifier = (ASTIdentifierExpressionRef)call->callee;
            // TODO: @Next Incomplete...
        } else {
            _PerformNameResolutionForExpression(call->callee);
        }

        // TODO: @Next Incomplete...
        return;
    }

    if (expression->base.tag == ASTTagConstantExpression) {
        ASTConstantExpressionRef constant = (ASTConstantExpressionRef)expression;
        // TODO: @Next Incomplete...
        return;
    }

    JELLY_UNREACHABLE("Invalid tag given for ASTExpression!");
}
