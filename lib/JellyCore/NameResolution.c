#include "JellyCore/ASTFunctions.h"
#include "JellyCore/ASTScope.h"
#include "JellyCore/Diagnostic.h"
#include "JellyCore/NameResolution.h"

// TODO: @Bug Type all enumeration elements with the enumeration type and replace it with lowered int type in the backend

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
            if (child->tag == ASTTagFunctionDeclaration || child->tag == ASTTagForeignFunctionDeclaration ||
                child->tag == ASTTagIntrinsicFunctionDeclaration) {
                ASTFunctionDeclarationRef function = (ASTFunctionDeclarationRef)child;
                if (_ResolveDeclarationsOfFunctionSignature(context, function)) {
                    if (ASTScopeLookupDeclarationByNameOrMatchingFunctionSignature(child->scope, function->base.name, function->fixity,
                                                                                   function->parameters, function->returnType) == NULL) {
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

            if (child->tag == ASTTagStructureDeclaration) {
                ASTStructureDeclarationRef structure = (ASTStructureDeclarationRef)child;
                _PerformNameResolutionForStructureBody(context, structure);
                continue;
            }

            if (child->tag == ASTTagValueDeclaration) {
                ASTValueDeclarationRef value = (ASTValueDeclarationRef)child;
                _ResolveDeclarationsOfTypeAndSubstituteType(context, value->base.base.scope, &value->base.type);

                if (value->initializer) {
                    value->initializer->expectedType = value->base.type;
                    _PerformNameResolutionForExpression(context, value->initializer);
                }
            }
        }
    }

    for (Index index = 0; index < ASTArrayGetElementCount(module->sourceUnits); index++) {
        ASTSourceUnitRef sourceUnit = (ASTSourceUnitRef)ASTArrayGetElementAtIndex(module->sourceUnits, index);
        for (Index sourceUnitIndex = 0; sourceUnitIndex < ASTArrayGetElementCount(sourceUnit->declarations); sourceUnitIndex++) {
            ASTNodeRef child = (ASTNodeRef)ASTArrayGetElementAtIndex(sourceUnit->declarations, sourceUnitIndex);
            if (child->tag == ASTTagFunctionDeclaration) {
                ASTFunctionDeclarationRef function = (ASTFunctionDeclarationRef)child;
                _PerformNameResolutionForFunctionBody(context, function);
                continue;
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

        // TODO: @Cleanup This hack is used as a workaround for now and should be removed after adding Module compilation support, which
        // will allow to implicity import the builtin stdlib soon...
        if (StringIsEqualToCString(opaque->name, "String")) {
            *type = (ASTTypeRef)ASTContextGetStringType(context);
            return true;
        }

        *type = (ASTTypeRef)ASTContextGetBuiltinType(context, ASTBuiltinTypeKindError);
        ReportErrorFormat("Use of unresolved type '%s'", StringGetCharacters(opaque->name));
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

    case ASTTagFunctionType: {
        ASTFunctionTypeRef func      = (ASTFunctionTypeRef)(*type);
        ASTArrayIteratorRef iterator = ASTArrayGetIterator(func->parameterTypes);
        Bool success                 = true;
        while (iterator) {
            if (!_ResolveDeclarationsOfTypeAndSubstituteType(context, scope, (ASTTypeRef *)ASTArrayIteratorGetElementPointer(iterator))) {
                success = false;
            }

            iterator = ASTArrayIteratorNext(iterator);
        }

        if (!_ResolveDeclarationsOfTypeAndSubstituteType(context, scope, &func->resultType)) {
            success = false;
        }

        return success;
    }

    case ASTTagBuiltinType:
    case ASTTagEnumerationType:
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
            element->initializer->expectedType = element->base.type;
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

            // TODO: Currently we are resolving infix function where the switch argument and case condition has to have the same type
            //       but it should also be possible to compare cases by different types it there is a matching infix function '=='
            // `infix func == (lhs: $switch.argument.type, rhs: $switch.case.type) -> Bool`

            ASTScopeRef globalScope = ASTContextGetGlobalScope(context);
            StringRef name          = StringCreate(AllocatorGetSystemDefault(), "==");
            ArrayRef parameterTypes = ArrayCreateEmpty(AllocatorGetSystemDefault(), sizeof(ASTTypeRef), 2);
            ArrayAppendElement(parameterTypes, &statement->condition->type);
            ArrayAppendElement(parameterTypes, &statement->condition->type);
            ASTFunctionDeclarationRef comparator = ASTScopeLookupInfixFunction(
                globalScope, name, parameterTypes, (ASTTypeRef)ASTContextGetBuiltinType(context, ASTBuiltinTypeKindBool));
            if (comparator) {
                statement->comparator = comparator;
            } else {
                ReportError("'case' condition is not comparable with 'switch' argument");
            }

            ArrayDestroy(parameterTypes);
            StringDestroy(name);
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
    if (expression->base.tag == ASTTagReferenceExpression) {
        ASTReferenceExpressionRef reference = (ASTReferenceExpressionRef)expression;
        _PerformNameResolutionForExpression(context, reference->argument);

        if (reference->argument->type->tag == ASTTagBuiltinType &&
            ((ASTBuiltinTypeRef)reference->argument->type)->kind == ASTBuiltinTypeKindError) {
            reference->base.type = (ASTTypeRef)ASTContextGetBuiltinType(context, ASTBuiltinTypeKindError);
        } else {
            reference->base.type = (ASTTypeRef)ASTContextCreatePointerType(context, reference->base.base.location,
                                                                           reference->base.base.scope, reference->argument->type);
        }
        return;
    }

    if (expression->base.tag == ASTTagDereferenceExpression) {
        ASTDereferenceExpressionRef dereference = (ASTDereferenceExpressionRef)expression;
        _PerformNameResolutionForExpression(context, dereference->argument);
        if (dereference->argument->type->tag == ASTTagBuiltinType &&
            ((ASTBuiltinTypeRef)dereference->argument->type)->kind == ASTBuiltinTypeKindError) {
            dereference->base.type = (ASTTypeRef)ASTContextGetBuiltinType(context, ASTBuiltinTypeKindError);
        } else {
            if (dereference->argument->type->tag == ASTTagPointerType) {
                ASTPointerTypeRef pointerType = (ASTPointerTypeRef)dereference->argument->type;
                dereference->base.type        = pointerType->pointeeType;
            } else {
                ReportError("Cannot derefence expression of non pointer type");
                dereference->base.type = (ASTTypeRef)ASTContextGetBuiltinType(context, ASTBuiltinTypeKindError);
            }
        }
        return;
    }

    if (expression->base.tag == ASTTagUnaryExpression) {
        ASTUnaryExpressionRef unary = (ASTUnaryExpressionRef)expression;
        // TODO: Allow candidate types for builtin integer types!
        _PerformNameResolutionForExpression(context, unary->arguments[0]);

        ASTScopeRef globalScope                       = ASTContextGetGlobalScope(context);
        StringRef operatorName                        = ASTGetPrefixOperatorName(AllocatorGetSystemDefault(), unary->op);
        ASTFunctionDeclarationRef matchingDeclaration = NULL;
        for (Index index = 0; index < ASTArrayGetElementCount(globalScope->declarations); index++) {
            ASTDeclarationRef declaration = (ASTDeclarationRef)ASTArrayGetElementAtIndex(globalScope->declarations, index);
            if (declaration->base.tag != ASTTagFunctionDeclaration && declaration->base.tag != ASTTagForeignFunctionDeclaration &&
                declaration->base.tag != ASTTagIntrinsicFunctionDeclaration) {
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
            ReportErrorFormat("Use of unresolved prefix function '%s'", StringGetCharacters(operatorName));
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

        if ((binary->arguments[0]->type->tag == ASTTagBuiltinType &&
             ((ASTBuiltinTypeRef)binary->arguments[0]->type)->kind == ASTBuiltinTypeKindError) ||
            (binary->arguments[1]->type->tag == ASTTagBuiltinType &&
             ((ASTBuiltinTypeRef)binary->arguments[1]->type)->kind == ASTBuiltinTypeKindError)) {
            binary->base.type = (ASTTypeRef)ASTContextGetBuiltinType(context, ASTBuiltinTypeKindError);
            return;
        }

        ASTScopeRef globalScope                       = ASTContextGetGlobalScope(context);
        StringRef operatorName                        = ASTGetInfixOperatorName(AllocatorGetSystemDefault(), binary->op);
        ASTFunctionDeclarationRef matchingDeclaration = NULL;
        for (Index index = 0; index < ASTArrayGetElementCount(globalScope->declarations); index++) {
            ASTDeclarationRef declaration = (ASTDeclarationRef)ASTArrayGetElementAtIndex(globalScope->declarations, index);
            if (declaration->base.tag != ASTTagFunctionDeclaration && declaration->base.tag != ASTTagForeignFunctionDeclaration &&
                declaration->base.tag != ASTTagIntrinsicFunctionDeclaration) {
                continue;
            }

            ASTFunctionDeclarationRef function = (ASTFunctionDeclarationRef)declaration;
            if (function->fixity != ASTFixityInfix) {
                continue;
            }

            if (ASTArrayGetElementCount(function->parameters) != 2) {
                continue;
            }

            if (!StringIsEqual(function->base.name, operatorName)) {
                continue;
            }

            ASTValueDeclarationRef lhsParameter = ASTArrayGetElementAtIndex(function->parameters, 0);
            if (!ASTTypeIsEqual(lhsParameter->base.type, binary->arguments[0]->type)) {
                continue;
            }

            ASTValueDeclarationRef rhsParameter = ASTArrayGetElementAtIndex(function->parameters, 1);
            if (!ASTTypeIsEqual(rhsParameter->base.type, binary->arguments[1]->type)) {
                continue;
            }

            if (binary->base.expectedType) {
                if (!ASTTypeIsEqual(binary->base.expectedType, function->returnType)) {
                    continue;
                }
            }

            if (matchingDeclaration) {
                ReportError("Candidate declarations is currently not supported for binary expressions!");
                binary->base.type = (ASTTypeRef)ASTContextGetBuiltinType(context, ASTBuiltinTypeKindError);
                return;
            }

            matchingDeclaration = function;
        }

        if (!matchingDeclaration) {
            ReportErrorFormat("Use of unresolved infix function '%s'", StringGetCharacters(operatorName));
            binary->base.type = (ASTTypeRef)ASTContextGetBuiltinType(context, ASTBuiltinTypeKindError);
        } else {
            binary->opFunction = (ASTFunctionDeclarationRef)matchingDeclaration;
            binary->base.type  = binary->opFunction->returnType;
        }

        StringDestroy(operatorName);
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
                } else {
                    identifier->base.type = (ASTTypeRef)ASTContextGetBuiltinType(context, ASTBuiltinTypeKindError);
                    ReportErrorFormat("Use of unresolved identifier '%s'", StringGetCharacters(identifier->name));
                }
            } else {
                ASTScopeRef globalScope      = ASTContextGetGlobalScope(context);
                ASTArrayIteratorRef iterator = ASTArrayGetIterator(globalScope->declarations);
                while (iterator) {
                    ASTNodeRef child = (ASTNodeRef)ASTArrayIteratorGetElement(iterator);
                    if (child->tag == ASTTagEnumerationDeclaration) {
                        ASTEnumerationDeclarationRef enumeration = (ASTEnumerationDeclarationRef)child;
                        ASTDeclarationRef declaration = ASTScopeLookupDeclarationByName(enumeration->innerScope, identifier->name);
                        if (declaration) {
                            ASTArrayAppendElement(identifier->candidateDeclarations, declaration);
                        }
                    }

                    iterator = ASTArrayIteratorNext(iterator);
                }

                if (ASTArrayGetElementCount(identifier->candidateDeclarations) == 1) {
                    ASTDeclarationRef declaration   = ASTArrayGetElementAtIndex(identifier->candidateDeclarations, 0);
                    identifier->base.type           = declaration->type;
                    identifier->resolvedDeclaration = declaration;
                } else {
                    // TODO: If count of candidateDeclarations is greater than 1, then continue matching candidates in outer expression or
                    // report ambigous use of identifier error
                    identifier->base.type = (ASTTypeRef)ASTContextGetBuiltinType(context, ASTBuiltinTypeKindError);
                    ReportErrorFormat("Use of unresolved identifier '%s'", StringGetCharacters(identifier->name));
                }
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
            ASTStructureTypeRef structType               = (ASTStructureTypeRef)type;
            ASTStructureDeclarationRef structDeclaration = structType->declaration;
            for (Index index = 0; index < ASTArrayGetElementCount(structDeclaration->values); index++) {
                ASTValueDeclarationRef value = (ASTValueDeclarationRef)ASTArrayGetElementAtIndex(structDeclaration->values, index);
                if (StringIsEqual(value->base.name, memberAccess->memberName)) {
                    memberAccess->memberIndex         = index;
                    memberAccess->base.type           = value->base.type;
                    memberAccess->resolvedDeclaration = (ASTDeclarationRef)value;
                    break;
                }
            }

            if (memberAccess->memberIndex < 0) {
                memberAccess->base.type = (ASTTypeRef)ASTContextGetBuiltinType(context, ASTBuiltinTypeKindError);
                ReportErrorFormat("Use of undeclared member '%s'", StringGetCharacters(memberAccess->memberName));
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
            ASTDeclarationRef declaration = ASTScopeLookupDeclarationInHierarchyByName(identifier->base.base.scope, identifier->name);
            if (declaration && declaration->type->tag == ASTTagPointerType &&
                ((ASTPointerTypeRef)declaration->type)->pointeeType->tag == ASTTagFunctionType) {
                ASTArrayAppendElement(identifier->candidateDeclarations, declaration);
            }

            ASTScopeRef globalScope               = ASTContextGetGlobalScope(context);
            ASTDeclarationRef matchingDeclaration = NULL;
            CandidateFunctionMatchKind matchKind  = CandidateFunctionMatchKindNone;
            Index matchingParameterTypeCount      = 0;
            for (Index index = 0; index < ASTArrayGetElementCount(globalScope->declarations); index++) {
                ASTDeclarationRef declaration = (ASTDeclarationRef)ASTArrayGetElementAtIndex(globalScope->declarations, index);
                if (declaration->base.tag != ASTTagFunctionDeclaration && declaration->base.tag != ASTTagForeignFunctionDeclaration &&
                    declaration->base.tag != ASTTagIntrinsicFunctionDeclaration) {
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

                    // TODO: @Hack Remove this soon we are re-resolving the arguments after resolving a good matching declaration here...
                    //    ...this problem should be handled by assigning candidate types to arguments and performing resolution based on
                    //    candidate types
                    if (matchingDeclaration->base.tag == ASTTagFunctionDeclaration ||
                        matchingDeclaration->base.tag == ASTTagForeignFunctionDeclaration ||
                        matchingDeclaration->base.tag == ASTTagIntrinsicFunctionDeclaration) {
                        ASTFunctionDeclarationRef function = (ASTFunctionDeclarationRef)matchingDeclaration;
                        Index maxArgumentCount             = MIN(ASTArrayGetElementCount(call->arguments),
                                                     ASTArrayGetElementCount(function->parameters));
                        for (Index index = 0; index < maxArgumentCount; index++) {
                            ASTValueDeclarationRef parameter = (ASTValueDeclarationRef)ASTArrayGetElementAtIndex(function->parameters,
                                                                                                                 index);

                            ASTExpressionRef argument = (ASTExpressionRef)ASTArrayGetElementAtIndex(call->arguments, index);
                            argument->expectedType    = parameter->base.type;
                            _PerformNameResolutionForExpression(context, argument);
                        }
                    }
                } else {
                    ReportErrorFormat("Use of unresolved identifier '%s'", StringGetCharacters(identifier->name));
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
            call->base.type                 = functionType->resultType;
        } else if (call->callee->type->tag == ASTTagPointerType &&
                   ((ASTPointerTypeRef)call->callee->type)->pointeeType->tag == ASTTagFunctionType) {
            ASTPointerTypeRef pointerType   = (ASTPointerTypeRef)call->callee->type;
            ASTFunctionTypeRef functionType = (ASTFunctionTypeRef)pointerType->pointeeType;
            call->base.type                 = functionType->resultType;
        } else if ((call->callee->type->tag != ASTTagBuiltinType ||
                    ((ASTBuiltinTypeRef)call->callee->type)->kind != ASTBuiltinTypeKindError)) {
            ReportError("Cannot call a non function type");
            call->base.type = (ASTTypeRef)ASTContextGetBuiltinType(context, ASTBuiltinTypeKindError);
        } else {
            call->base.type = (ASTTypeRef)ASTContextGetBuiltinType(context, ASTBuiltinTypeKindError);
        }

        return;
    }

    if (expression->base.tag == ASTTagConstantExpression) {
        ASTConstantExpressionRef constant = (ASTConstantExpressionRef)expression;
        constant->base.base.flags |= ASTFlagsIsConstantEvaluable;

        if (constant->kind == ASTConstantKindNil) {
            if (constant->base.expectedType && constant->base.expectedType->tag == ASTTagPointerType) {
                constant->base.type = constant->base.expectedType;
            } else {
                constant->base.type = (ASTTypeRef)ASTContextCreatePointerType(
                    context, SourceRangeNull(), constant->base.base.scope,
                    (ASTTypeRef)ASTContextGetBuiltinType(context, ASTBuiltinTypeKindVoid));
            }
        } else if (constant->kind == ASTConstantKindBool) {
            constant->base.type = (ASTTypeRef)ASTContextGetBuiltinType(context, ASTBuiltinTypeKindBool);
        } else if (constant->kind == ASTConstantKindInt) {
            if (constant->base.expectedType && constant->base.expectedType->tag == ASTTagBuiltinType) {
                ASTBuiltinTypeRef expectedType = (ASTBuiltinTypeRef)constant->base.expectedType;
                if ((expectedType->kind == ASTBuiltinTypeKindUInt || expectedType->kind == ASTBuiltinTypeKindUInt64) &&
                    0 <= constant->minimumBitWidth && constant->minimumBitWidth <= 64) {
                    constant->base.type = constant->base.expectedType;
                } else if ((expectedType->kind == ASTBuiltinTypeKindInt || expectedType->kind == ASTBuiltinTypeKindInt64) &&
                           0 <= constant->minimumBitWidth && constant->minimumBitWidth < 64) {
                    constant->base.type = constant->base.expectedType;
                } else if ((expectedType->kind == ASTBuiltinTypeKindUInt32 && 0 <= constant->minimumBitWidth &&
                            constant->minimumBitWidth <= 32)) {
                    constant->base.type = constant->base.expectedType;
                } else if ((expectedType->kind == ASTBuiltinTypeKindInt32 && 0 <= constant->minimumBitWidth &&
                            constant->minimumBitWidth <= 31)) {
                    constant->base.type = constant->base.expectedType;
                } else if ((expectedType->kind == ASTBuiltinTypeKindUInt16 && 0 <= constant->minimumBitWidth &&
                            constant->minimumBitWidth <= 16)) {
                    constant->base.type = constant->base.expectedType;
                } else if ((expectedType->kind == ASTBuiltinTypeKindInt16 && 0 <= constant->minimumBitWidth &&
                            constant->minimumBitWidth <= 15)) {
                    constant->base.type = constant->base.expectedType;
                } else if ((expectedType->kind == ASTBuiltinTypeKindUInt8 && 0 <= constant->minimumBitWidth &&
                            constant->minimumBitWidth <= 8)) {
                    constant->base.type = constant->base.expectedType;
                } else if ((expectedType->kind == ASTBuiltinTypeKindInt8 && 0 <= constant->minimumBitWidth &&
                            constant->minimumBitWidth <= 7)) {
                    constant->base.type = constant->base.expectedType;
                } else {
                    if (constant->minimumBitWidth < 64) {
                        constant->base.type = (ASTTypeRef)ASTContextGetBuiltinType(context, ASTBuiltinTypeKindInt64);
                    } else {
                        constant->base.type = (ASTTypeRef)ASTContextGetBuiltinType(context, ASTBuiltinTypeKindUInt64);
                    }
                }
            } else {
                if (constant->minimumBitWidth < 64) {
                    constant->base.type = (ASTTypeRef)ASTContextGetBuiltinType(context, ASTBuiltinTypeKindInt64);
                } else {
                    constant->base.type = (ASTTypeRef)ASTContextGetBuiltinType(context, ASTBuiltinTypeKindUInt64);
                }
            }
        } else if (constant->kind == ASTConstantKindFloat) {
            constant->base.type = (ASTTypeRef)ASTContextGetBuiltinType(context, ASTBuiltinTypeKindFloat);
        } else if (constant->kind == ASTConstantKindString) {
            constant->base.type = (ASTTypeRef)ASTContextGetStringType(context);
        } else {
            JELLY_UNREACHABLE("Unknown kind given for ASTConstantExpression in Typer!");
        }

        return;
    }

    JELLY_UNREACHABLE("Invalid tag given for ASTExpression!");
}
