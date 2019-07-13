#include "JellyCore/ASTFunctions.h"
#include "JellyCore/ASTScope.h"
#include "JellyCore/Diagnostic.h"
#include "JellyCore/NameResolution.h"

enum _CandidateFunctionMatchKind {
    CandidateFunctionMatchKindNone,
    CandidateFunctionMatchKindName,
    CandidateFunctionMatchKindParameterCount,
    CandidateFunctionMatchKindParameterTypes,
    CandidateFunctionMatchKindExpectedType,
};
typedef enum _CandidateFunctionMatchKind CandidateFunctionMatchKind;

static inline void _AddSourceUnitRecordDeclarationsToScope(ASTContextRef context, ASTSourceUnitRef sourceUnit);
static inline Bool _ResolveDeclarationsOfFunctionSignature(ASTContextRef context, ASTFunctionDeclarationRef function);
static inline Bool _ResolveDeclarationsOfTypeAndSubstituteType(ASTContextRef context, ASTScopeRef scope, ASTTypeRef *type);
static inline void _PerformNameResolutionForEnumerationBody(ASTContextRef context, ASTEnumerationDeclarationRef enumeration);
static inline void _PerformNameResolutionForFunctionBody(ASTContextRef context, ASTFunctionDeclarationRef function);
static inline void _PerformNameResolutionForStructureBody(ASTContextRef context, ASTStructureDeclarationRef structure);
static inline void _PerformNameResolutionForNode(ASTContextRef context, ASTNodeRef node);
static inline void _PerformNameResolutionForExpression(ASTContextRef context, ASTExpressionRef expression);

void PerformNameResolution(ASTContextRef context, ASTModuleDeclarationRef module) {
    for (Index index = 0; index < ASTArrayGetElementCount(module->sourceUnits); index++) {
        ASTSourceUnitRef sourceUnit = (ASTSourceUnitRef)ASTArrayGetElementAtIndex(module->sourceUnits, index);
        _AddSourceUnitRecordDeclarationsToScope(context, sourceUnit);
    }

    for (Index index = 0; index < ASTArrayGetElementCount(module->sourceUnits); index++) {
        ASTSourceUnitRef sourceUnit = (ASTSourceUnitRef)ASTArrayGetElementAtIndex(module->sourceUnits, index);
        for (Index sourceUnitIndex = 0; sourceUnitIndex < ASTArrayGetElementCount(sourceUnit->declarations); sourceUnitIndex++) {
            ASTNodeRef child = (ASTNodeRef)ASTArrayGetElementAtIndex(sourceUnit->declarations, sourceUnitIndex);
            if (child->tag == ASTTagFunctionDeclaration) {
                ASTFunctionDeclarationRef function = (ASTFunctionDeclarationRef)child;
                if (_ResolveDeclarationsOfFunctionSignature(context, function)) {
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
                _PerformNameResolutionForEnumerationBody(context, enumeration);
                continue;
            }

            if (child->tag == ASTTagFunctionDeclaration) {
                ASTFunctionDeclarationRef function = (ASTFunctionDeclarationRef)child;
                _PerformNameResolutionForFunctionBody(context, function);
                continue;
            }

            if (child->tag == ASTTagStructureDeclaration) {
                ASTStructureDeclarationRef structure = (ASTStructureDeclarationRef)child;
                _PerformNameResolutionForStructureBody(context, structure);
                continue;
            }

            if (child->tag == ASTTagValueDeclaration) {
                ASTValueDeclarationRef value = (ASTValueDeclarationRef)child;
                if (value->initializer) {
                    _PerformNameResolutionForExpression(context, value->initializer);
                }
            }
        }
    }
}

static inline void _AddSourceUnitRecordDeclarationsToScope(ASTContextRef context, ASTSourceUnitRef sourceUnit) {
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

static inline Bool _ResolveDeclarationsOfFunctionSignature(ASTContextRef context, ASTFunctionDeclarationRef function) {
    Bool success = true;
    for (Index index = 0; index < ASTArrayGetElementCount(function->parameters); index++) {
        ASTValueDeclarationRef value = (ASTValueDeclarationRef)ASTArrayGetElementAtIndex(function->parameters, index);
        if (!_ResolveDeclarationsOfTypeAndSubstituteType(context, function->base.base.scope, &value->base.type)) {
            success = false;
        }
    }

    if (!_ResolveDeclarationsOfTypeAndSubstituteType(context, function->base.base.scope, &function->returnType)) {
        success = false;
    }

    return success;
}

static inline Bool _ResolveDeclarationsOfTypeAndSubstituteType(ASTContextRef context, ASTScopeRef scope, ASTTypeRef *type) {
    switch ((*type)->tag) {
    case ASTTagOpaqueType: {
        ASTOpaqueTypeRef opaque = (ASTOpaqueTypeRef)(*type);
        if (opaque->declaration) {
            *type = opaque->declaration->type;
            return true;
        }

        ASTScopeRef currentScope = scope;
        while (currentScope) {
            ASTDeclarationRef declaration = ASTScopeLookupDeclarationByName(currentScope, opaque->name);
            if (declaration) {
                opaque->declaration = declaration;
                *type               = declaration->type;
                return true;
            }

            currentScope = ASTScopeGetNextParentForLookup(currentScope);
        }

        *type = (ASTTypeRef)ASTContextGetBuiltinType(context, ASTBuiltinTypeKindError);
        ReportError("Use of unresolved type");
        return false;
    }

    case ASTTagPointerType: {
        ASTPointerTypeRef pointer = (ASTPointerTypeRef)(*type);
        return _ResolveDeclarationsOfTypeAndSubstituteType(context, scope, &pointer->pointeeType);
    }

    case ASTTagArrayType: {
        ASTArrayTypeRef array = (ASTArrayTypeRef)(*type);
        return _ResolveDeclarationsOfTypeAndSubstituteType(context, scope, &array->elementType);
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

static inline void _PerformNameResolutionForEnumerationBody(ASTContextRef context, ASTEnumerationDeclarationRef enumeration) {
    for (Index index = 0; index < ASTArrayGetElementCount(enumeration->elements); index++) {
        ASTValueDeclarationRef element = (ASTValueDeclarationRef)ASTArrayGetElementAtIndex(enumeration->elements, index);
        assert(element->base.base.tag == ASTTagValueDeclaration);
        assert(element->kind == ASTValueKindEnumerationElement);

        if (ASTScopeLookupDeclarationByName(enumeration->innerScope, element->base.name) == NULL) {
            ASTArrayAppendElement(enumeration->innerScope->declarations, element);
        } else {
            ReportError("Invalid redeclaration of identifier");
        }

        if (element->initializer) {
            _PerformNameResolutionForExpression(context, element->initializer);
        }
    }
}

static inline void _PerformNameResolutionForFunctionBody(ASTContextRef context, ASTFunctionDeclarationRef function) {
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
        _PerformNameResolutionForNode(context, node);
    }
}

static inline void _PerformNameResolutionForStructureBody(ASTContextRef context, ASTStructureDeclarationRef structure) {
    for (Index index = 0; index < ASTArrayGetElementCount(structure->values); index++) {
        ASTValueDeclarationRef value = (ASTValueDeclarationRef)ASTArrayGetElementAtIndex(structure->values, index);
        assert(value->base.base.tag == ASTTagValueDeclaration);
        assert(value->kind == ASTValueKindVariable);
        if (_ResolveDeclarationsOfTypeAndSubstituteType(context, value->base.base.scope, &value->base.type)) {
            if (ASTScopeLookupDeclarationByName(structure->innerScope, value->base.name) == NULL) {
                ASTArrayAppendElement(structure->innerScope->declarations, value);
            } else {
                ReportError("Invalid redeclaration of identifier");
            }
        }
    }
}

static inline void _PerformNameResolutionForNode(ASTContextRef context, ASTNodeRef node) {
    switch (node->tag) {
    case ASTTagBlock: {
        ASTBlockRef block = (ASTBlockRef)node;
        for (Index index = 0; index < ASTArrayGetElementCount(block->statements); index++) {
            ASTNodeRef child = (ASTNodeRef)ASTArrayGetElementAtIndex(block->statements, index);
            _PerformNameResolutionForNode(context, child);
        }
        break;
    }

    case ASTTagIfStatement: {
        ASTIfStatementRef statement        = (ASTIfStatementRef)node;
        statement->condition->expectedType = (ASTTypeRef)ASTContextGetBuiltinType(context, ASTBuiltinTypeKindBool);
        _PerformNameResolutionForExpression(context, statement->condition);
        _PerformNameResolutionForNode(context, (ASTNodeRef)statement->thenBlock);
        _PerformNameResolutionForNode(context, (ASTNodeRef)statement->elseBlock);
        break;
    }

    case ASTTagLoopStatement: {
        ASTLoopStatementRef statement      = (ASTLoopStatementRef)node;
        statement->condition->expectedType = (ASTTypeRef)ASTContextGetBuiltinType(context, ASTBuiltinTypeKindBool);
        _PerformNameResolutionForExpression(context, statement->condition);
        _PerformNameResolutionForNode(context, (ASTNodeRef)statement->loopBlock);
        break;
    }

    case ASTTagCaseStatement: {
        ASTCaseStatementRef statement = (ASTCaseStatementRef)node;
        if (statement->kind == ASTCaseKindConditional) {
            ASTScopeRef scope = node->scope;
            while (scope && scope->kind != ASTScopeKindSwitch) {
                scope = scope->parent;
            }

            if (scope && scope->kind == ASTScopeKindSwitch) {
                assert(scope->node && scope->node->tag == ASTTagSwitchStatement);
                ASTSwitchStatementRef switchStatement = (ASTSwitchStatementRef)scope->node;
                statement->condition->expectedType    = switchStatement->argument->type;
            }

            _PerformNameResolutionForExpression(context, statement->condition);

            // TODO: Try resolving an infix function with the signature:
            // `infix func == (lhs: $switch.argument.type, rhs: $switch.case.type) -> Bool`
        }

        _PerformNameResolutionForNode(context, (ASTNodeRef)statement->body);
        break;
    }

    case ASTTagSwitchStatement: {
        ASTSwitchStatementRef statement = (ASTSwitchStatementRef)node;
        _PerformNameResolutionForExpression(context, statement->argument);
        for (Index index = 0; index < ASTArrayGetElementCount(statement->cases); index++) {
            ASTNodeRef node = (ASTNodeRef)ASTArrayGetElementAtIndex(statement->cases, index);
            _PerformNameResolutionForNode(context, node);
        }
        break;
    }

    case ASTTagControlStatement: {
        ASTControlStatementRef statement = (ASTControlStatementRef)node;
        if (statement->kind == ASTControlKindReturn && statement->result) {
            ASTScopeRef scope = statement->base.scope;
            while (scope && scope->kind != ASTScopeKindFunction) {
                scope = scope->parent;
            }

            if (scope && scope->kind == ASTScopeKindFunction) {
                ASTFunctionDeclarationRef enclosingFunction = (ASTFunctionDeclarationRef)scope->node;
                statement->result->expectedType             = enclosingFunction->returnType;
            }

            _PerformNameResolutionForExpression(context, statement->result);
        }
        break;
    }

    case ASTTagUnaryExpression:
    case ASTTagBinaryExpression:
    case ASTTagIdentifierExpression:
    case ASTTagMemberAccessExpression:
    case ASTTagAssignmentExpression:
    case ASTTagCallExpression:
    case ASTTagConstantExpression: {
        ASTExpressionRef expression = (ASTExpressionRef)node;
        _PerformNameResolutionForExpression(context, expression);
        break;
    }

    case ASTTagValueDeclaration: {
        ASTValueDeclarationRef value = (ASTValueDeclarationRef)node;
        assert(value->kind == ASTValueKindVariable);

        if (_ResolveDeclarationsOfTypeAndSubstituteType(context, value->base.base.scope, &value->base.type)) {
            if (ASTScopeLookupDeclarationByName(value->base.base.scope, value->base.name) == NULL) {
                ASTArrayAppendElement(value->base.base.scope->declarations, value);
            } else {
                ReportError("Invalid redeclaration of identifier");
            }
        }

        if (value->initializer) {
            value->initializer->expectedType = value->base.type;
            _PerformNameResolutionForExpression(context, value->initializer);
        }
        break;
    }

    default:
        JELLY_UNREACHABLE("Invalid tag given for ASTNode!");
        break;
    }
}

static inline void _PerformNameResolutionForExpression(ASTContextRef context, ASTExpressionRef expression) {
    if (expression->base.tag == ASTTagUnaryExpression) {
        ASTUnaryExpressionRef unary = (ASTUnaryExpressionRef)expression;
        _PerformNameResolutionForExpression(context, unary->arguments[0]);

        ASTScopeRef globalScope                       = ASTContextGetGlobalScope(context);
        StringRef operatorName                        = ASTGetPrefixOperatorName(AllocatorGetSystemDefault(), unary->op);
        ASTFunctionDeclarationRef matchingDeclaration = NULL;
        for (Index index = 0; index < ASTArrayGetElementCount(globalScope->declarations); index++) {
            ASTDeclarationRef declaration = (ASTDeclarationRef)ASTArrayGetElementAtIndex(globalScope->declarations, index);
            if (declaration->base.tag != ASTTagFunctionDeclaration) {
                continue;
            }

            ASTFunctionDeclarationRef function = (ASTFunctionDeclarationRef)declaration;
            if (function->fixity != ASTFixityPrefix) {
                continue;
            }

            if (ASTArrayGetElementCount(function->parameters) != 1) {
                continue;
            }

            if (!StringIsEqual(function->base.name, operatorName)) {
                continue;
            }

            ASTValueDeclarationRef parameter = ASTArrayGetElementAtIndex(function->parameters, 0);
            if (!ASTTypeIsEqual(parameter->base.type, unary->arguments[0]->type)) {
                continue;
            }

            if (unary->base.expectedType) {
                if (!ASTTypeIsEqual(unary->base.expectedType, function->returnType)) {
                    continue;
                }
            }

            assert(!matchingDeclaration && "Candidate declarations is currently not supported for unary expressions!");
            matchingDeclaration = function;
        }

        if (!matchingDeclaration) {
            ReportError("Use of unresolved prefix function");
            unary->base.type = (ASTTypeRef)ASTContextGetBuiltinType(context, ASTBuiltinTypeKindError);
        } else {
            unary->opFunction = (ASTFunctionDeclarationRef)matchingDeclaration;
            unary->base.type  = unary->opFunction->returnType;
        }

        StringDestroy(operatorName);
        return;
    }

    if (expression->base.tag == ASTTagBinaryExpression) {
        ASTBinaryExpressionRef binary = (ASTBinaryExpressionRef)expression;
        _PerformNameResolutionForExpression(context, binary->arguments[0]);
        _PerformNameResolutionForExpression(context, binary->arguments[1]);
        ReportCritical("Binary expression are not supported at the moment!");
        binary->base.type = (ASTTypeRef)ASTContextGetBuiltinType(context, ASTBuiltinTypeKindError);
        return;
    }

    if (expression->base.tag == ASTTagIdentifierExpression) {
        ASTIdentifierExpressionRef identifier = (ASTIdentifierExpressionRef)expression;
        ASTDeclarationRef declaration         = ASTScopeLookupDeclarationInHierarchyByName(expression->base.scope, identifier->name);
        if (declaration) {
            identifier->base.type           = declaration->type;
            identifier->resolvedDeclaration = declaration;
        } else {
            if (identifier->base.expectedType && identifier->base.expectedType->tag == ASTTagEnumerationType) {
                ASTEnumerationTypeRef enumerationType    = (ASTEnumerationTypeRef)identifier->base.expectedType;
                ASTEnumerationDeclarationRef enumeration = enumerationType->declaration;

                declaration = ASTScopeLookupDeclarationByName(enumeration->innerScope, identifier->name);
                if (declaration) {
                    identifier->base.type           = declaration->type;
                    identifier->resolvedDeclaration = declaration;
                    identifier->resolvedEnumeration = enumeration;
                } else {
                    identifier->base.type = (ASTTypeRef)ASTContextGetBuiltinType(context, ASTBuiltinTypeKindError);
                    ReportError("Use of unresolved identifier");
                }
            } else {
                identifier->base.type = (ASTTypeRef)ASTContextGetBuiltinType(context, ASTBuiltinTypeKindError);
                ReportError("Use of unresolved identifier");
            }
        }
        return;
    }

    if (expression->base.tag == ASTTagMemberAccessExpression) {
        ASTMemberAccessExpressionRef memberAccess = (ASTMemberAccessExpressionRef)expression;
        _PerformNameResolutionForExpression(context, memberAccess->argument);

        assert(memberAccess->argument->type);
        ASTTypeRef type = memberAccess->argument->type;
        while (type && type->tag == ASTTagPointerType) {
            ASTPointerTypeRef pointerType = (ASTPointerTypeRef)type;
            memberAccess->pointerDepth += 1;
            type = pointerType->pointeeType;
        }

        if (type->tag == ASTTagStructureType) {
            ASTStructureTypeRef structType = (ASTStructureTypeRef)type;
            ASTDeclarationRef declaration  = ASTScopeLookupDeclarationByName(structType->declaration->innerScope, memberAccess->memberName);
            if (declaration) {
                memberAccess->base.type           = declaration->type;
                memberAccess->resolvedDeclaration = declaration;
            } else {
                memberAccess->base.type = (ASTTypeRef)ASTContextGetBuiltinType(context, ASTBuiltinTypeKindError);
                ReportError("Use of undeclared member");
            }
        } else {
            memberAccess->base.type = (ASTTypeRef)ASTContextGetBuiltinType(context, ASTBuiltinTypeKindError);
            if (type->tag != ASTTagBuiltinType && ((ASTBuiltinTypeRef)type)->kind != ASTBuiltinTypeKindError) {
                ReportError("Cannot access named member of non structure type");
            }
        }
        return;
    }

    if (expression->base.tag == ASTTagAssignmentExpression) {
        ASTAssignmentExpressionRef assignment = (ASTAssignmentExpressionRef)expression;
        _PerformNameResolutionForExpression(context, assignment->variable);
        assignment->expression->expectedType = assignment->variable->type;
        _PerformNameResolutionForExpression(context, assignment->expression);
        return;
    }

    if (expression->base.tag == ASTTagCallExpression) {
        ASTCallExpressionRef call = (ASTCallExpressionRef)expression;
        for (Index index = 0; index < ASTArrayGetElementCount(call->arguments); index++) {
            ASTExpressionRef argument = (ASTExpressionRef)ASTArrayGetElementAtIndex(call->arguments, index);
            _PerformNameResolutionForExpression(context, argument);
        }

        if (call->callee->base.tag == ASTTagIdentifierExpression) {
            ASTIdentifierExpressionRef identifier = (ASTIdentifierExpressionRef)call->callee;
            ASTScopeRef globalScope               = ASTContextGetGlobalScope(context);
            ASTDeclarationRef matchingDeclaration = NULL;
            CandidateFunctionMatchKind matchKind  = CandidateFunctionMatchKindNone;
            Index matchingParameterTypeCount      = 0;
            for (Index index = 0; index < ASTArrayGetElementCount(globalScope->declarations); index++) {
                ASTDeclarationRef declaration = (ASTDeclarationRef)ASTArrayGetElementAtIndex(globalScope->declarations, index);
                if (declaration->base.tag != ASTTagFunctionDeclaration) {
                    continue;
                }

                if (!StringIsEqual(declaration->name, identifier->name)) {
                    continue;
                }

                if (matchKind < CandidateFunctionMatchKindName) {
                    matchingDeclaration = declaration;
                    matchKind           = CandidateFunctionMatchKindName;
                }

                ASTFunctionDeclarationRef function = (ASTFunctionDeclarationRef)declaration;
                Index minParameterCheckCount = MIN(ASTArrayGetElementCount(function->parameters), ASTArrayGetElementCount(call->arguments));
                Index hasCorrectArgumentCount = ASTArrayGetElementCount(function->parameters) == ASTArrayGetElementCount(call->arguments);

                if (matchKind < CandidateFunctionMatchKindParameterCount && hasCorrectArgumentCount) {
                    matchingDeclaration = declaration;
                    matchKind           = CandidateFunctionMatchKindParameterCount;
                }

                if (call->base.expectedType) {
                    if (!ASTTypeIsEqual(call->base.expectedType, function->returnType)) {
                        continue;
                    }

                    if (matchKind < CandidateFunctionMatchKindExpectedType) {
                        matchingDeclaration = declaration;
                        matchKind           = CandidateFunctionMatchKindExpectedType;
                    }
                }

                Bool hasMatchingParameterTypes          = true;
                Index currentMatchingParameterTypeCount = 0;
                for (Index parameterIndex = 0; parameterIndex < minParameterCheckCount; parameterIndex++) {
                    ASTValueDeclarationRef parameter = (ASTValueDeclarationRef)ASTArrayGetElementAtIndex(function->parameters,
                                                                                                         parameterIndex);
                    ASTExpressionRef argument        = (ASTExpressionRef)ASTArrayGetElementAtIndex(call->arguments, parameterIndex);

                    assert(parameter->base.type);
                    assert(argument->type);

                    if (ASTTypeIsEqual(parameter->base.type, argument->type)) {
                        currentMatchingParameterTypeCount += 1;
                    } else {
                        hasMatchingParameterTypes = false;
                    }
                }

                if (hasMatchingParameterTypes && hasCorrectArgumentCount) {
                    ASTArrayAppendElement(identifier->candidateDeclarations, declaration);
                } else if (matchKind <= CandidateFunctionMatchKindParameterTypes &&
                           matchingParameterTypeCount < currentMatchingParameterTypeCount) {
                    matchingDeclaration        = declaration;
                    matchKind                  = CandidateFunctionMatchKindParameterTypes;
                    matchingParameterTypeCount = currentMatchingParameterTypeCount;
                }
            }

            Index candidateCount = ASTArrayGetElementCount(identifier->candidateDeclarations);
            if (candidateCount == 1) {
                identifier->resolvedDeclaration = (ASTDeclarationRef)ASTArrayGetElementAtIndex(identifier->candidateDeclarations, 0);
                identifier->base.type           = identifier->resolvedDeclaration->type;
            } else if (candidateCount == 0) {
                // Fall back to best matching declaration to emit better error reports in type checking phase
                if (matchingDeclaration) {
                    identifier->resolvedDeclaration = matchingDeclaration;
                    identifier->base.type           = identifier->resolvedDeclaration->type;
                } else {
                    ReportError("Use of unresolved identifier");
                    identifier->base.type = (ASTTypeRef)ASTContextGetBuiltinType(context, ASTBuiltinTypeKindError);
                }
            } else {
                ReportError("Ambigous use of identifier");
                identifier->base.type = (ASTTypeRef)ASTContextGetBuiltinType(context, ASTBuiltinTypeKindError);
            }
        } else {
            _PerformNameResolutionForExpression(context, call->callee);
        }

        assert(call->callee->type);

        if (call->callee->type->tag == ASTTagFunctionType) {
            ASTFunctionTypeRef functionType = (ASTFunctionTypeRef)call->callee->type;
            call->base.type                 = functionType->declaration->returnType;
        } else if (call->callee->type->tag != ASTTagFunctionType &&
                   (call->callee->type->tag != ASTTagBuiltinType ||
                    ((ASTBuiltinTypeRef)call->callee->type)->kind != ASTBuiltinTypeKindError)) {
            ReportError("Cannot call non function type");
            call->base.type = (ASTTypeRef)ASTContextGetBuiltinType(context, ASTBuiltinTypeKindError);
        } else {
            call->base.type = (ASTTypeRef)ASTContextGetBuiltinType(context, ASTBuiltinTypeKindError);
        }

        return;
    }

    if (expression->base.tag == ASTTagConstantExpression) {
        ASTConstantExpressionRef constant = (ASTConstantExpressionRef)expression;
        if (constant->kind == ASTConstantKindNil) {
            constant->base.type = (ASTTypeRef)ASTContextCreatePointerType(
                context, SourceRangeNull(), constant->base.base.scope,
                (ASTTypeRef)ASTContextGetBuiltinType(context, ASTBuiltinTypeKindVoid));
        } else if (constant->kind == ASTConstantKindBool) {
            constant->base.type = (ASTTypeRef)ASTContextGetBuiltinType(context, ASTBuiltinTypeKindBool);
        } else if (constant->kind == ASTConstantKindInt) {
            constant->base.type = (ASTTypeRef)ASTContextGetBuiltinType(context, ASTBuiltinTypeKindInt);
        } else if (constant->kind == ASTConstantKindFloat) {
            constant->base.type = (ASTTypeRef)ASTContextGetBuiltinType(context, ASTBuiltinTypeKindFloat);
        } else if (constant->kind == ASTConstantKindString) {
            ReportCritical("String literals are not supported at the moment!");
        } else {
            JELLY_UNREACHABLE("Unknown kind given for ASTConstantExpression in Typer!");
        }

        return;
    }

    JELLY_UNREACHABLE("Invalid tag given for ASTExpression!");
}
