#include "JellyCore/ASTFunctions.h"
#include "JellyCore/ASTMangling.h"
#include "JellyCore/ASTSubstitution.h"
#include "JellyCore/Diagnostic.h"
#include "JellyCore/NameResolution.h"

// TODO: @ModuleSupport Add name resolution support for imported modules

enum _CandidateFunctionMatchKind {
    CandidateFunctionMatchKindNone,
    CandidateFunctionMatchKindName,
    CandidateFunctionMatchKindParameterCount,
    CandidateFunctionMatchKindExpectedType,
    CandidateFunctionMatchKindParameterTypes,
};
typedef enum _CandidateFunctionMatchKind CandidateFunctionMatchKind;

static inline void _AddSourceUnitRecordDeclarationsToScope(ASTContextRef context, ASTSourceUnitRef sourceUnit);
static inline Bool _ResolveDeclarationsOfInitializerDeclaration(ASTContextRef context, ASTInitializerDeclarationRef initializer);
static inline Bool _ResolveDeclarationsOfFunctionSignature(ASTContextRef context, ASTFunctionDeclarationRef function);
static inline Bool _ResolveDeclarationsOfTypeAndSubstituteType(ASTContextRef context, ScopeID scope, ASTTypeRef *type);
static inline void _PerformNameResolutionForEnumerationBody(ASTContextRef context, ASTEnumerationDeclarationRef enumeration);
static inline void _PerformNameResolutionForFunctionBody(ASTContextRef context, ASTFunctionDeclarationRef function);
static inline void _PerformNameResolutionForStructureBody(ASTContextRef context, ASTStructureDeclarationRef structure);
static inline void _PerformNameResolutionForNode(ASTContextRef context, ASTNodeRef node);
static inline void _PerformNameResolutionForExpression(ASTContextRef context, ASTExpressionRef expression, Bool reportErrors);
static inline ASTFunctionDeclarationRef _LookupInfixFunctionInScope(SymbolTableRef symbolTable, ScopeID scope, StringRef name,
                                                                    ArrayRef parameterTypes, ASTTypeRef resultType);
static inline ASTInitializerDeclarationRef _LookupInitializerInSymbolGroupByParameters(SymbolTableRef symbolTable, SymbolID symbolGroup,
                                                                                       ASTArrayRef parameters);
static inline ASTDeclarationRef _LookupDeclarationByNameOrMatchingFunctionSignature(SymbolTableRef symbolTable, ScopeID scope,
                                                                                    StringRef name, ASTArrayRef parameters);

void PerformNameResolution(ASTContextRef context, ASTModuleDeclarationRef module) {
    SymbolTableRef symbolTable = ASTContextGetSymbolTable(context);

    for (Index index = 0; index < ASTArrayGetElementCount(module->sourceUnits); index++) {
        ASTSourceUnitRef sourceUnit = (ASTSourceUnitRef)ASTArrayGetElementAtIndex(module->sourceUnits, index);
        _AddSourceUnitRecordDeclarationsToScope(context, sourceUnit);
    }

    BucketArrayRef enumerations = ASTContextGetAllNodes(context, ASTTagEnumerationDeclaration);
    for (Index index = 0; index < BucketArrayGetElementCount(enumerations); index++) {
        ASTEnumerationDeclarationRef enumeration = (ASTEnumerationDeclarationRef)BucketArrayGetElementAtIndex(enumerations, index);
        ASTArrayIteratorRef iterator             = ASTArrayGetIterator(enumeration->elements);
        while (iterator) {
            ASTValueDeclarationRef element = (ASTValueDeclarationRef)ASTArrayIteratorGetElement(iterator);
            element->base.type             = enumeration->base.type;
            iterator                       = ASTArrayIteratorNext(iterator);
        }
    }

    for (Index index = 0; index < ASTArrayGetElementCount(module->sourceUnits); index++) {
        ASTSourceUnitRef sourceUnit = (ASTSourceUnitRef)ASTArrayGetElementAtIndex(module->sourceUnits, index);
        for (Index sourceUnitIndex = 0; sourceUnitIndex < ASTArrayGetElementCount(sourceUnit->declarations); sourceUnitIndex++) {
            ASTNodeRef child = (ASTNodeRef)ASTArrayGetElementAtIndex(sourceUnit->declarations, sourceUnitIndex);
            if (child->tag == ASTTagFunctionDeclaration || child->tag == ASTTagForeignFunctionDeclaration ||
                child->tag == ASTTagIntrinsicFunctionDeclaration) {
                ASTFunctionDeclarationRef function = (ASTFunctionDeclarationRef)child;
                if (_ResolveDeclarationsOfFunctionSignature(context, function)) {
                    if (!_LookupDeclarationByNameOrMatchingFunctionSignature(symbolTable, child->scope, function->base.name,
                                                                             function->parameters)) {
                        SymbolID symbol  = SymbolTableInsertOrGetSymbolGroup(symbolTable, child->scope, function->base.name);
                        Index entryIndex = SymbolTableInsertSymbolGroupEntry(symbolTable, symbol);
                        SymbolTableSetSymbolGroupDefinition(symbolTable, symbol, entryIndex, child);
                    } else {
                        ReportError("Invalid redeclaration of identifier");
                    }
                }
            }

            if (child->tag == ASTTagStructureDeclaration) {
                ASTStructureDeclarationRef structure = (ASTStructureDeclarationRef)child;
                ASTArrayIteratorRef iterator         = ASTArrayGetIterator(structure->initializers);
                while (iterator) {
                    ASTInitializerDeclarationRef initializer = (ASTInitializerDeclarationRef)ASTArrayIteratorGetElement(iterator);
                    _PerformNameResolutionForNode(context, (ASTNodeRef)initializer);
                    if (_ResolveDeclarationsOfInitializerDeclaration(context, initializer)) {
                        SymbolID symbol = SymbolTableInsertOrGetSymbolGroup(symbolTable, structure->innerScope, initializer->base.name);
                        assert(symbol != kSymbolNull && SymbolTableIsSymbolGroup(symbolTable, symbol));
                        if (!_LookupInitializerInSymbolGroupByParameters(symbolTable, symbol, initializer->parameters)) {
                            Index entryIndex = SymbolTableInsertSymbolGroupEntry(symbolTable, symbol);
                            SymbolTableSetSymbolGroupDefinition(symbolTable, symbol, entryIndex, initializer);
                        } else {
                            ReportError("Invalid redeclaration of initializer");
                        }
                    }

                    iterator = ASTArrayIteratorNext(iterator);
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
                    _PerformNameResolutionForExpression(context, value->initializer, true);
                }
            }

            if (child->tag == ASTTagTypeAliasDeclaration) {
                ASTTypeAliasDeclarationRef alias = (ASTTypeAliasDeclarationRef)child;
                _ResolveDeclarationsOfTypeAndSubstituteType(context, alias->base.base.scope, &alias->base.type);
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

            if (child->tag == ASTTagStructureDeclaration) {
                ASTStructureDeclarationRef structure = (ASTStructureDeclarationRef)child;
                ASTArrayIteratorRef iterator         = ASTArrayGetIterator(structure->initializers);
                while (iterator) {
                    ASTInitializerDeclarationRef initializer = (ASTInitializerDeclarationRef)ASTArrayIteratorGetElement(iterator);
                    _PerformNameResolutionForNode(context, (ASTNodeRef)initializer->body);
                    iterator = ASTArrayIteratorNext(iterator);
                }
            }
        }
    }

    // Substitute predefined types with resolved members of the declaration...
    BucketArrayRef functionTypes = ASTContextGetAllNodes(context, ASTTagFunctionType);
    for (Index index = 0; index < BucketArrayGetElementCount(functionTypes); index++) {
        ASTFunctionTypeRef type = (ASTFunctionTypeRef)BucketArrayGetElementAtIndex(functionTypes, index);
        if (!type->declaration) {
            continue;
        }

        Index parameterCount = MIN(ASTArrayGetElementCount(type->parameterTypes), ASTArrayGetElementCount(type->declaration->parameters));
        for (Index parameterIndex = 0; parameterIndex < parameterCount; parameterIndex++) {
            ASTValueDeclarationRef parameter = (ASTValueDeclarationRef)ASTArrayGetElementAtIndex(type->declaration->parameters,
                                                                                                 parameterIndex);
            ASTArraySetElementAtIndex(type->parameterTypes, parameterIndex, parameter->base.type);
        }

        type->resultType = type->declaration->returnType;
    }

    ASTApplySubstitution(context, module);
}

static inline void _AddSourceUnitRecordDeclarationsToScope(ASTContextRef context, ASTSourceUnitRef sourceUnit) {
    SymbolTableRef symbolTable = ASTContextGetSymbolTable(context);

    for (Index index = 0; index < ASTArrayGetElementCount(sourceUnit->declarations); index++) {
        ASTNodeRef child = (ASTNodeRef)ASTArrayGetElementAtIndex(sourceUnit->declarations, index);
        if (child->tag == ASTTagTypeAliasDeclaration || child->tag == ASTTagEnumerationDeclaration ||
            child->tag == ASTTagStructureDeclaration ||
            (child->tag == ASTTagValueDeclaration && ((ASTValueDeclarationRef)child)->kind == ASTValueKindVariable)) {
            ASTDeclarationRef declaration = (ASTDeclarationRef)child;
            SymbolID symbol               = SymbolTableLookupSymbol(symbolTable, child->scope, declaration->name);
            if (symbol == kSymbolNull) {
                symbol = SymbolTableInsertSymbol(symbolTable, child->scope, declaration->name);
                SymbolTableSetSymbolDefinition(symbolTable, symbol, declaration);
            } else {
                ReportError("Invalid redeclaration of identifier");
            }
        }
    }
}

static inline Bool _ResolveDeclarationsOfInitializerDeclaration(ASTContextRef context, ASTInitializerDeclarationRef initializer) {
    Bool success = true;
    for (Index index = 0; index < ASTArrayGetElementCount(initializer->parameters); index++) {
        ASTValueDeclarationRef value = (ASTValueDeclarationRef)ASTArrayGetElementAtIndex(initializer->parameters, index);
        if (!_ResolveDeclarationsOfTypeAndSubstituteType(context, initializer->base.base.scope, &value->base.type)) {
            success = false;
        }
    }

    return success;
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

static inline Bool _ResolveDeclarationsOfTypeAndSubstituteType(ASTContextRef context, ScopeID scope, ASTTypeRef *type) {
    SymbolTableRef symbolTable = ASTContextGetSymbolTable(context);

    switch ((*type)->tag) {
    case ASTTagOpaqueType: {
        ASTOpaqueTypeRef opaque = (ASTOpaqueTypeRef)(*type);
        if (opaque->declaration) {
            assert(opaque->declaration->type->tag != ASTTagOpaqueType);
            *type = opaque->declaration->type;
            return true;
        }

        SymbolID symbol = SymbolTableLookupSymbolInHierarchy(symbolTable, scope, opaque->name);
        if (symbol != kSymbolNull && !SymbolTableIsSymbolGroup(symbolTable, symbol)) {
            opaque->declaration = (ASTDeclarationRef)SymbolTableGetSymbolDefinition(symbolTable, symbol);
            assert(opaque->declaration->type->tag != ASTTagOpaqueType);
            *type = opaque->declaration->type;
            return true;
        }

        // TODO: @Cleanup This hack is used as a workaround for now and should be removed after adding Module compilation support, which
        //       will allow to implicity import the builtin stdlib soon...
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
    SymbolTableRef symbolTable = ASTContextGetSymbolTable(context);

    for (Index index = 0; index < ASTArrayGetElementCount(enumeration->elements); index++) {
        ASTValueDeclarationRef element = (ASTValueDeclarationRef)ASTArrayGetElementAtIndex(enumeration->elements, index);
        assert(element->base.base.tag == ASTTagValueDeclaration);
        assert(element->kind == ASTValueKindEnumerationElement);

        SymbolID symbol = SymbolTableLookupSymbol(symbolTable, enumeration->innerScope, element->base.name);
        if (symbol == kSymbolNull) {
            symbol = SymbolTableInsertSymbol(symbolTable, enumeration->innerScope, element->base.name);
            SymbolTableSetSymbolDefinition(symbolTable, symbol, element);
        } else {
            ReportError("Invalid redeclaration of identifier");
        }

        if (element->initializer) {
            element->initializer->expectedType = element->base.type;
            _PerformNameResolutionForExpression(context, element->initializer, true);
        }
    }
}

static inline void _PerformNameResolutionForFunctionBody(ASTContextRef context, ASTFunctionDeclarationRef function) {
    SymbolTableRef symbolTable = ASTContextGetSymbolTable(context);

    for (Index parameterIndex = 0; parameterIndex < ASTArrayGetElementCount(function->parameters); parameterIndex++) {
        ASTValueDeclarationRef parameter = (ASTValueDeclarationRef)ASTArrayGetElementAtIndex(function->parameters, parameterIndex);
        SymbolID symbol                  = SymbolTableLookupSymbol(symbolTable, function->innerScope, parameter->base.name);
        if (symbol == kSymbolNull) {
            symbol = SymbolTableInsertSymbol(symbolTable, function->innerScope, parameter->base.name);
            SymbolTableSetSymbolDefinition(symbolTable, symbol, parameter);
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
    SymbolTableRef symbolTable = ASTContextGetSymbolTable(context);

    for (Index index = 0; index < ASTArrayGetElementCount(structure->values); index++) {
        ASTValueDeclarationRef value = (ASTValueDeclarationRef)ASTArrayGetElementAtIndex(structure->values, index);
        assert(value->base.base.tag == ASTTagValueDeclaration);
        assert(value->kind == ASTValueKindVariable);
        if (_ResolveDeclarationsOfTypeAndSubstituteType(context, value->base.base.scope, &value->base.type)) {
            SymbolID symbol = SymbolTableLookupSymbol(symbolTable, structure->innerScope, value->base.name);
            if (symbol == kSymbolNull) {
                symbol = SymbolTableInsertSymbol(symbolTable, structure->innerScope, value->base.name);
                SymbolTableSetSymbolDefinition(symbolTable, symbol, value);
            } else {
                ReportError("Invalid redeclaration of identifier");
            }
        }
    }
}

static inline void _PerformNameResolutionForNode(ASTContextRef context, ASTNodeRef node) {
    SymbolTableRef symbolTable = ASTContextGetSymbolTable(context);

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
        _PerformNameResolutionForExpression(context, statement->condition, true);
        _PerformNameResolutionForNode(context, (ASTNodeRef)statement->thenBlock);
        _PerformNameResolutionForNode(context, (ASTNodeRef)statement->elseBlock);
        break;
    }

    case ASTTagLoopStatement: {
        ASTLoopStatementRef statement      = (ASTLoopStatementRef)node;
        statement->condition->expectedType = (ASTTypeRef)ASTContextGetBuiltinType(context, ASTBuiltinTypeKindBool);
        _PerformNameResolutionForExpression(context, statement->condition, true);
        _PerformNameResolutionForNode(context, (ASTNodeRef)statement->loopBlock);
        break;
    }

    case ASTTagCaseStatement: {
        ASTCaseStatementRef statement = (ASTCaseStatementRef)node;
        if (statement->kind == ASTCaseKindConditional) {
            ASTTypeRef argumentType = NULL;
            ScopeID scope           = SymbolTableGetScopeOrParentOfKinds(symbolTable, node->scope, ScopeKindSwitch);
            if (scope != kScopeNull) {
                ASTNodeRef node = SymbolTableGetScopeUserdata(symbolTable, scope);
                assert(node && node->tag == ASTTagSwitchStatement);
                ASTSwitchStatementRef switchStatement = (ASTSwitchStatementRef)node;
                statement->condition->expectedType    = switchStatement->argument->type;
                argumentType                          = switchStatement->argument->type;
            }

            _PerformNameResolutionForExpression(context, statement->condition, true);

            ASTTypeRef conditionType = statement->condition->type;
            if (argumentType && argumentType->tag == ASTTagEnumerationType && statement->condition->type->tag == ASTTagEnumerationType) {
                ASTEnumerationTypeRef lhs = (ASTEnumerationTypeRef)argumentType;
                ASTEnumerationTypeRef rhs = (ASTEnumerationTypeRef)statement->condition->type;
                if (lhs->declaration == rhs->declaration) {
                    argumentType  = (ASTTypeRef)ASTContextGetBuiltinType(context, ASTBuiltinTypeKindInt64);
                    conditionType = argumentType;
                }
            }

            if (!argumentType) {
                argumentType = conditionType;
            }

            StringRef name          = StringCreate(AllocatorGetSystemDefault(), "==");
            ArrayRef parameterTypes = ArrayCreateEmpty(AllocatorGetSystemDefault(), sizeof(ASTTypeRef), 2);
            ArrayAppendElement(parameterTypes, &argumentType);
            ArrayAppendElement(parameterTypes, &conditionType);
            ASTFunctionDeclarationRef comparator = _LookupInfixFunctionInScope(
                symbolTable, kScopeGlobal, name, parameterTypes, (ASTTypeRef)ASTContextGetBuiltinType(context, ASTBuiltinTypeKindBool));
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
        _PerformNameResolutionForExpression(context, statement->argument, true);
        for (Index index = 0; index < ASTArrayGetElementCount(statement->cases); index++) {
            ASTNodeRef node = (ASTNodeRef)ASTArrayGetElementAtIndex(statement->cases, index);
            _PerformNameResolutionForNode(context, node);
        }
        break;
    }

    case ASTTagControlStatement: {
        ASTControlStatementRef statement = (ASTControlStatementRef)node;
        if (statement->kind == ASTControlKindReturn && statement->result) {
            ScopeID scope = SymbolTableGetScopeOrParentOfKinds(symbolTable, statement->base.scope, ScopeKindFunction);
            if (scope != kScopeNull) {
                ASTNodeRef node = SymbolTableGetScopeUserdata(symbolTable, scope);
                assert(node && (node->tag == ASTTagFunctionDeclaration || node->tag == ASTTagForeignFunctionDeclaration ||
                                node->tag == ASTTagIntrinsicFunctionDeclaration));
                ASTFunctionDeclarationRef enclosingFunction = (ASTFunctionDeclarationRef)node;
                statement->result->expectedType             = enclosingFunction->returnType;
            }

            _PerformNameResolutionForExpression(context, statement->result, true);
        }
        break;
    }

    case ASTTagUnaryExpression:
    case ASTTagBinaryExpression:
    case ASTTagIdentifierExpression:
    case ASTTagMemberAccessExpression:
    case ASTTagAssignmentExpression:
    case ASTTagCallExpression:
    case ASTTagConstantExpression:
    case ASTTagDereferenceExpression: {
        ASTExpressionRef expression = (ASTExpressionRef)node;
        _PerformNameResolutionForExpression(context, expression, true);
        break;
    }

    case ASTTagValueDeclaration: {
        ASTValueDeclarationRef value = (ASTValueDeclarationRef)node;
        assert(value->kind == ASTValueKindVariable);

        if (_ResolveDeclarationsOfTypeAndSubstituteType(context, value->base.base.scope, &value->base.type)) {
            SymbolID symbol = SymbolTableLookupSymbol(symbolTable, value->base.base.scope, value->base.name);
            if (symbol == kSymbolNull) {
                symbol = SymbolTableInsertSymbol(symbolTable, value->base.base.scope, value->base.name);
                SymbolTableSetSymbolDefinition(symbolTable, symbol, value);
            } else {
                ReportError("Invalid redeclaration of identifier");
            }
        }

        if (value->initializer) {
            value->initializer->expectedType = value->base.type;
            _PerformNameResolutionForExpression(context, value->initializer, true);
        }
        break;
    }

    case ASTTagInitializerDeclaration: {
        ASTInitializerDeclarationRef initializer = (ASTInitializerDeclarationRef)node;
        if (SymbolTableGetScopeKind(symbolTable, initializer->base.base.scope) == ScopeKindStructure) {
            ASTNodeRef node = SymbolTableGetScopeUserdata(symbolTable, initializer->base.base.scope);
            assert(node && node->tag == ASTTagStructureDeclaration);
            initializer->structure = (ASTStructureDeclarationRef)node;
            assert(initializer->structure->base.type);

            ArrayRef parameterTypes               = ArrayCreateEmpty(AllocatorGetSystemDefault(), sizeof(ASTTypeRef),
                                                       ASTArrayGetElementCount(initializer->parameters));
            ASTArrayIteratorRef parameterIterator = ASTArrayGetIterator(initializer->parameters);
            while (parameterIterator) {
                ASTValueDeclarationRef parameter = (ASTValueDeclarationRef)ASTArrayIteratorGetElement(parameterIterator);
                assert(parameter->base.type);
                ArrayAppendElement(parameterTypes, &parameter->base.type);

                SymbolID symbol = SymbolTableLookupSymbol(symbolTable, initializer->innerScope, parameter->base.name);
                if (symbol == kSymbolNull) {
                    symbol = SymbolTableInsertSymbol(symbolTable, initializer->innerScope, parameter->base.name);
                    SymbolTableSetSymbolDefinition(symbolTable, symbol, parameter);
                } else {
                    ReportError("Invalid redeclaration of identifier");
                }

                parameterIterator = ASTArrayIteratorNext(parameterIterator);
            }

            initializer->base.type = (ASTTypeRef)ASTContextCreateFunctionType(
                context, initializer->base.base.location, initializer->base.base.scope, parameterTypes, initializer->structure->base.type);
            ArrayDestroy(parameterTypes);

            StringRef implicitSelfName = StringCreate(AllocatorGetSystemDefault(), "self");
            initializer->implicitSelf  = ASTContextCreateValueDeclaration(context, initializer->base.base.location, initializer->innerScope,
                                                                         ASTValueKindVariable, implicitSelfName,
                                                                         initializer->structure->base.type, NULL);
            ASTArrayInsertElementAtIndex(initializer->body->statements, 0, initializer->implicitSelf);
            StringDestroy(implicitSelfName);
        } else {
            ReportError("Initializer can only be declared in a structure!");
            initializer->base.type = (ASTTypeRef)ASTContextGetBuiltinType(context, ASTBuiltinTypeKindError);
        }
        break;
    }

    default:
        JELLY_UNREACHABLE("Invalid tag given for ASTNode!");
        break;
    }
}

static inline Bool _IsNodeEqual(const void *elementLeft, const void *elementRight) {
    return elementLeft == elementRight;
}

static inline void _PerformNameResolutionForExpression(ASTContextRef context, ASTExpressionRef expression, Bool reportErrors) {
    SymbolTableRef symbolTable = ASTContextGetSymbolTable(context);

    if (expression->base.tag == ASTTagReferenceExpression) {
        ASTReferenceExpressionRef reference = (ASTReferenceExpressionRef)expression;
        _PerformNameResolutionForExpression(context, reference->argument, reportErrors);

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
        _PerformNameResolutionForExpression(context, dereference->argument, reportErrors);
        assert(dereference->argument->type);
        if (dereference->argument->type->tag == ASTTagBuiltinType &&
            ((ASTBuiltinTypeRef)dereference->argument->type)->kind == ASTBuiltinTypeKindError) {
            dereference->base.type = (ASTTypeRef)ASTContextGetBuiltinType(context, ASTBuiltinTypeKindError);
        } else {
            if (dereference->argument->type->tag == ASTTagPointerType) {
                ASTPointerTypeRef pointerType = (ASTPointerTypeRef)dereference->argument->type;
                dereference->base.type        = pointerType->pointeeType;
            } else {
                dereference->base.type = (ASTTypeRef)ASTContextGetBuiltinType(context, ASTBuiltinTypeKindError);
                if (reportErrors) {
                    ReportError("Cannot derefence expression of non pointer type");
                }
            }
        }
        return;
    }

    if (expression->base.tag == ASTTagIdentifierExpression) {
        ASTIdentifierExpressionRef identifier = (ASTIdentifierExpressionRef)expression;
        if (identifier->base.expectedType && identifier->base.expectedType->tag == ASTTagEnumerationType) {
            ASTEnumerationTypeRef enumerationType    = (ASTEnumerationTypeRef)identifier->base.expectedType;
            ASTEnumerationDeclarationRef enumeration = enumerationType->declaration;

            SymbolID symbol = SymbolTableLookupSymbol(symbolTable, enumeration->innerScope, identifier->name);
            if (symbol != kSymbolNull && !SymbolTableIsSymbolGroup(symbolTable, symbol)) {
                ASTDeclarationRef declaration = (ASTDeclarationRef)SymbolTableGetSymbolDefinition(symbolTable, symbol);
                assert(declaration);
                identifier->base.type           = declaration->type;
                identifier->resolvedDeclaration = declaration;
            } else {
                identifier->base.type = (ASTTypeRef)ASTContextGetBuiltinType(context, ASTBuiltinTypeKindError);
                if (reportErrors) {
                    ReportErrorFormat("Use of unresolved identifier '%s'", StringGetCharacters(identifier->name));
                }
            }
        } else {
            BucketArrayRef enumerations = ASTContextGetAllNodes(context, ASTTagEnumerationDeclaration);
            for (Index index = 0; index < BucketArrayGetElementCount(enumerations); index++) {
                ASTEnumerationDeclarationRef enumeration = (ASTEnumerationDeclarationRef)BucketArrayGetElementAtIndex(enumerations, index);
                SymbolID symbol                          = SymbolTableLookupSymbol(symbolTable, enumeration->innerScope, identifier->name);
                if (symbol != kSymbolNull && !SymbolTableIsSymbolGroup(symbolTable, symbol)) {
                    ASTDeclarationRef declaration = (ASTDeclarationRef)SymbolTableGetSymbolDefinition(symbolTable, symbol);
                    assert(declaration);
                    ASTArrayAppendElement(identifier->candidateDeclarations, declaration);
                }
            }

            SymbolID symbol = SymbolTableLookupSymbolInHierarchy(symbolTable, expression->base.scope, identifier->name);
            if (symbol != kSymbolNull) {
                if (SymbolTableIsSymbolGroup(symbolTable, symbol)) {
                    Index count = SymbolTableGetSymbolGroupEntryCount(symbolTable, symbol);
                    for (Index index = 0; index < count; index++) {
                        ASTDeclarationRef declaration = (ASTDeclarationRef)SymbolTableGetSymbolGroupDefinition(symbolTable, symbol, index);
                        assert(declaration);
                        ASTArrayAppendElement(identifier->candidateDeclarations, declaration);
                    }
                } else {
                    ASTDeclarationRef declaration = SymbolTableGetSymbolDefinition(symbolTable, symbol);
                    assert(declaration);
                    identifier->base.type           = declaration->type;
                    identifier->resolvedDeclaration = declaration;
                }
            } else {
                SymbolID *symbols;
                Index count;
                SymbolTableGetScopeSymbols(symbolTable, kScopeGlobal, &symbols, &count);
                for (Index index = 0; index < count; index++) {
                    if (!SymbolTableIsSymbolGroup(symbolTable, symbols[index])) {
                        ASTNodeRef child = (ASTNodeRef)SymbolTableGetSymbolDefinition(symbolTable, symbols[index]);
                        assert(child);
                        if (child->tag == ASTTagEnumerationDeclaration) {
                            ASTEnumerationDeclarationRef enumeration = (ASTEnumerationDeclarationRef)child;
                            SymbolID symbol = SymbolTableLookupSymbol(symbolTable, enumeration->innerScope, identifier->name);
                            if (symbol != kSymbolNull && !SymbolTableIsSymbolGroup(symbolTable, symbol)) {
                                ASTDeclarationRef declaration = (ASTDeclarationRef)SymbolTableGetSymbolDefinition(symbolTable, symbol);
                                assert(declaration);
                                ASTArrayAppendElement(identifier->candidateDeclarations, declaration);
                            }
                        }
                    }
                }
            }
        }

        if (!identifier->resolvedDeclaration) {
            if (ASTArrayGetElementCount(identifier->candidateDeclarations) == 1) {
                ASTDeclarationRef declaration   = ASTArrayGetElementAtIndex(identifier->candidateDeclarations, 0);
                identifier->base.type           = declaration->type;
                identifier->resolvedDeclaration = declaration;
            } else {
                // TODO: If count of candidateDeclarations is greater than 1, then continue matching candidates in outer expression or
                //       report ambiguous use of identifier error
                identifier->base.type = (ASTTypeRef)ASTContextGetBuiltinType(context, ASTBuiltinTypeKindError);
                if (reportErrors) {
                    if (ASTArrayGetElementCount(identifier->candidateDeclarations) > 0) {
                        ReportErrorFormat("Ambiguous use of identifier '%s'", StringGetCharacters(identifier->name));
                    } else {
                        ReportErrorFormat("Use of unresolved identifier '%s'", StringGetCharacters(identifier->name));
                    }
                }
            }
        }

        return;
    }

    if (expression->base.tag == ASTTagMemberAccessExpression) {
        ASTMemberAccessExpressionRef memberAccess = (ASTMemberAccessExpressionRef)expression;
        memberAccess->pointerDepth                = 0;

        _PerformNameResolutionForExpression(context, memberAccess->argument, reportErrors);

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
                if (reportErrors) {
                    ReportErrorFormat("Use of undeclared member '%s'", StringGetCharacters(memberAccess->memberName));
                }
            }
        } else if (type->tag == ASTTagArrayType && ((ASTArrayTypeRef)type)->size) {
            if (StringIsEqualToCString(memberAccess->memberName, "count")) {
                assert(!memberAccess->base.base.substitute);

                ASTExpressionRef count                      = ((ASTArrayTypeRef)type)->size;
                memberAccess->base.base.substitute          = (ASTNodeRef)count;
                memberAccess->base.base.substitute->primary = (ASTNodeRef)memberAccess;
                _PerformNameResolutionForExpression(context, count, reportErrors);
            } else {
                memberAccess->base.type = (ASTTypeRef)ASTContextGetBuiltinType(context, ASTBuiltinTypeKindError);
                if (reportErrors) {
                    ReportErrorFormat("Use of undeclared member '%s'", StringGetCharacters(memberAccess->memberName));
                }
            }
        } else {
            memberAccess->base.type = (ASTTypeRef)ASTContextGetBuiltinType(context, ASTBuiltinTypeKindError);
            if (type->tag != ASTTagBuiltinType && ((ASTBuiltinTypeRef)type)->kind != ASTBuiltinTypeKindError && reportErrors) {
                ReportError("Cannot access named member of non structure type");
            }
        }
        return;
    }

    if (expression->base.tag == ASTTagAssignmentExpression) {
        ASTAssignmentExpressionRef assignment = (ASTAssignmentExpressionRef)expression;
        _PerformNameResolutionForExpression(context, assignment->variable, reportErrors);
        assignment->expression->expectedType = assignment->variable->type;
        _PerformNameResolutionForExpression(context, assignment->expression, reportErrors);
        assignment->base.type = (ASTTypeRef)ASTContextGetBuiltinType(context, ASTBuiltinTypeKindVoid);
        return;
    }

    if (expression->base.tag == ASTTagCallExpression) {
        ASTCallExpressionRef call = (ASTCallExpressionRef)expression;
        for (Index index = 0; index < ASTArrayGetElementCount(call->arguments); index++) {
            ASTExpressionRef argument = (ASTExpressionRef)ASTArrayGetElementAtIndex(call->arguments, index);
            _PerformNameResolutionForExpression(context, argument, false);
        }

        // TODO: Disallow creation of infix functions for the cases which are implicitly handled by the compiler!
        if (call->fixity == ASTFixityInfix && (call->op.binary == ASTBinaryOperatorAdd || call->op.binary == ASTBinaryOperatorSubtract)) {
            assert(ASTArrayGetElementCount(call->arguments) == 2);
            ASTExpressionRef arguments[] = {ASTArrayGetElementAtIndex(call->arguments, 0), ASTArrayGetElementAtIndex(call->arguments, 1)};
            if (arguments[0]->type->tag == ASTTagPointerType && ASTTypeIsInteger(arguments[1]->type)) {
                ASTPointerTypeRef pointerType = (ASTPointerTypeRef)arguments[0]->type;
                if (ASTTypeIsVoid(pointerType->pointeeType)) {
                    call->base.type = (ASTTypeRef)ASTContextGetBuiltinType(context, ASTBuiltinTypeKindError);
                    if (reportErrors) {
                        ReportError("Cannot perform arithmetic operations on a 'Void' pointer");
                    }
                    return;
                }

                call->base.base.flags |= ASTFlagsIsPointerArithmetic;
                call->base.type         = arguments[0]->type;
                ArrayRef parameterTypes = ArrayCreateEmpty(AllocatorGetSystemDefault(), sizeof(ASTTypeRef), 2);
                ArrayAppendElement(parameterTypes, &arguments[0]->type);
                ArrayAppendElement(parameterTypes, &arguments[1]->type);
                call->callee->type = (ASTTypeRef)ASTContextCreateFunctionType(context, call->callee->base.location,
                                                                              call->callee->base.scope, parameterTypes, arguments[0]->type);
                ArrayDestroy(parameterTypes);
                return;
            }
        }

        if (call->callee->base.tag == ASTTagIdentifierExpression) {
            ASTIdentifierExpressionRef identifier = (ASTIdentifierExpressionRef)call->callee;
            SymbolID symbol = SymbolTableLookupSymbolInHierarchy(symbolTable, identifier->base.base.scope, identifier->name);
            if (symbol != kSymbolNull && !SymbolTableIsSymbolGroup(symbolTable, symbol)) {
                ASTDeclarationRef declaration = (ASTDeclarationRef)SymbolTableGetSymbolDefinition(symbolTable, symbol);
                assert(declaration);
                if (declaration->type->tag == ASTTagPointerType &&
                    ((ASTPointerTypeRef)declaration->type)->pointeeType->tag == ASTTagFunctionType) {
                    ASTArrayAppendElement(identifier->candidateDeclarations, declaration);
                }
            }

            ASTDeclarationRef matchingDeclaration  = NULL;
            CandidateFunctionMatchKind matchKind   = CandidateFunctionMatchKindNone;
            Index matchingParameterTypeCount       = 0;
            Index matchingParameterTypeConversions = UINTMAX_MAX;

            symbol = SymbolTableLookupSymbol(symbolTable, kScopeGlobal, identifier->name);
            if (symbol != kSymbolNull && !SymbolTableIsSymbolGroup(symbolTable, symbol)) {
                ASTDeclarationRef lookup = SymbolTableGetSymbolDefinition(symbolTable, symbol);
                assert(lookup);
                if (lookup->base.tag == ASTTagStructureDeclaration && call->fixity == ASTFixityNone) {
                    ASTStructureDeclarationRef structure    = (ASTStructureDeclarationRef)lookup;
                    ASTArrayIteratorRef initializerIterator = ASTArrayGetIterator(structure->initializers);
                    while (initializerIterator) {
                        ASTInitializerDeclarationRef initializer = (ASTInitializerDeclarationRef)ASTArrayIteratorGetElement(
                            initializerIterator);
                        if (matchKind < CandidateFunctionMatchKindName) {
                            matchingDeclaration = (ASTDeclarationRef)initializer;
                            matchKind           = CandidateFunctionMatchKindName;
                        }

                        Index minParameterCheckCount  = MIN(ASTArrayGetElementCount(initializer->parameters),
                                                           ASTArrayGetElementCount(call->arguments));
                        Index hasCorrectArgumentCount = ASTArrayGetElementCount(initializer->parameters) ==
                                                        ASTArrayGetElementCount(call->arguments);

                        if (matchKind < CandidateFunctionMatchKindParameterCount && hasCorrectArgumentCount) {
                            matchingDeclaration = (ASTDeclarationRef)initializer;
                            matchKind           = CandidateFunctionMatchKindParameterCount;
                        }

                        assert(structure->base.type);
                        if (call->base.expectedType && ASTTypeIsEqual(call->base.expectedType, structure->base.type)) {
                            if (matchKind < CandidateFunctionMatchKindExpectedType) {
                                matchingDeclaration = (ASTDeclarationRef)initializer;
                                matchKind           = CandidateFunctionMatchKindExpectedType;
                            }
                        }

                        Bool hasMatchingParameterTypes                = true;
                        Index currentMatchingParameterTypeCount       = 0;
                        Index currentMatchingParameterTypeConversions = 0;
                        for (Index parameterIndex = 0; parameterIndex < minParameterCheckCount; parameterIndex++) {
                            ASTValueDeclarationRef parameter = (ASTValueDeclarationRef)ASTArrayGetElementAtIndex(initializer->parameters,
                                                                                                                 parameterIndex);
                            ASTExpressionRef argument        = (ASTExpressionRef)ASTArrayGetElementAtIndex(call->arguments, parameterIndex);

                            assert(parameter->base.type);
                            assert(argument->type);

                            if (ASTTypeIsEqual(argument->type, parameter->base.type)) {
                                currentMatchingParameterTypeCount += 1;
                            } else if (ASTTypeIsImplicitlyConvertible(argument->type, parameter->base.type)) {
                                currentMatchingParameterTypeCount += 1;
                                currentMatchingParameterTypeConversions += 1;
                            } else {
                                hasMatchingParameterTypes = false;
                            }
                        }

                        // TODO: Converted types should also be added to candidateDeclarations and should be filtered by best matches,
                        //       if there are more than one solutions after the post checking pass, then a declaration will be ambiguous!
                        if (hasMatchingParameterTypes && hasCorrectArgumentCount && currentMatchingParameterTypeConversions == 0) {
                            ASTArrayAppendElement(identifier->candidateDeclarations, (ASTDeclarationRef)initializer);
                        } else if (matchKind <= CandidateFunctionMatchKindParameterTypes &&
                                   ((matchingParameterTypeCount < currentMatchingParameterTypeCount) ||
                                    ((matchingParameterTypeCount == currentMatchingParameterTypeCount) &&
                                     matchingParameterTypeConversions > currentMatchingParameterTypeConversions))) {
                            matchingDeclaration              = (ASTDeclarationRef)initializer;
                            matchKind                        = CandidateFunctionMatchKindParameterTypes;
                            matchingParameterTypeCount       = currentMatchingParameterTypeCount;
                            matchingParameterTypeConversions = currentMatchingParameterTypeConversions;
                        }

                        initializerIterator = ASTArrayIteratorNext(initializerIterator);
                    }
                }
            } else {
                SymbolID *symbols;
                Index count;
                SymbolTableGetScopeSymbols(symbolTable, kScopeGlobal, &symbols, &count);
                for (Index index = 0; index < count; index++) {
                    if (!SymbolTableIsSymbolGroup(symbolTable, symbols[index])) {
                        continue;
                    }

                    Index entryCount = SymbolTableGetSymbolGroupEntryCount(symbolTable, symbols[index]);
                    for (Index entryIndex = 0; entryIndex < entryCount; entryIndex++) {
                        ASTDeclarationRef declaration = (ASTDeclarationRef)SymbolTableGetSymbolGroupDefinition(symbolTable, symbols[index],
                                                                                                               entryIndex);
                        assert(declaration);

                        if (declaration->base.tag != ASTTagFunctionDeclaration &&
                            declaration->base.tag != ASTTagForeignFunctionDeclaration &&
                            declaration->base.tag != ASTTagIntrinsicFunctionDeclaration) {
                            continue;
                        }

                        ASTFunctionDeclarationRef function = (ASTFunctionDeclarationRef)declaration;
                        if (function->fixity != call->fixity) {
                            continue;
                        }

                        if (!StringIsEqual(declaration->name, identifier->name)) {
                            continue;
                        }

                        // TODO: For some reasons are the predefined operators visited twice, that should be impossible but we will fix it
                        //       for now by only storing distinct declarations into candidateDeclarations. It could be that the builtin
                        //       functions are not inserted correctly to the context
                        if (ASTArrayContainsElement(identifier->candidateDeclarations, &_IsNodeEqual, declaration)) {
                            continue;
                        }

                        if (matchKind < CandidateFunctionMatchKindName) {
                            matchingDeclaration = declaration;
                            matchKind           = CandidateFunctionMatchKindName;
                        }

                        Index minParameterCheckCount  = MIN(ASTArrayGetElementCount(function->parameters),
                                                           ASTArrayGetElementCount(call->arguments));
                        Index hasCorrectArgumentCount = ASTArrayGetElementCount(function->parameters) ==
                                                        ASTArrayGetElementCount(call->arguments);

                        if (matchKind < CandidateFunctionMatchKindParameterCount && hasCorrectArgumentCount) {
                            matchingDeclaration = declaration;
                            matchKind           = CandidateFunctionMatchKindParameterCount;
                        }

                        if (call->base.expectedType && ASTTypeIsEqual(call->base.expectedType, function->returnType)) {
                            if (matchKind < CandidateFunctionMatchKindExpectedType) {
                                matchingDeclaration = declaration;
                                matchKind           = CandidateFunctionMatchKindExpectedType;
                            }
                        }

                        Bool hasMatchingParameterTypes                = true;
                        Index currentMatchingParameterTypeCount       = 0;
                        Index currentMatchingParameterTypeConversions = 0;
                        for (Index parameterIndex = 0; parameterIndex < minParameterCheckCount; parameterIndex++) {
                            ASTValueDeclarationRef parameter = (ASTValueDeclarationRef)ASTArrayGetElementAtIndex(function->parameters,
                                                                                                                 parameterIndex);
                            ASTExpressionRef argument        = (ASTExpressionRef)ASTArrayGetElementAtIndex(call->arguments, parameterIndex);

                            assert(parameter->base.type);
                            assert(argument->type);

                            if (ASTTypeIsEqual(argument->type, parameter->base.type)) {
                                currentMatchingParameterTypeCount += 1;
                            } else if (ASTTypeIsImplicitlyConvertible(argument->type, parameter->base.type)) {
                                currentMatchingParameterTypeCount += 1;
                                currentMatchingParameterTypeConversions += 1;
                            } else {
                                hasMatchingParameterTypes = false;
                            }
                        }

                        // TODO: Converted types should also be added to candidateDeclarations and should be filtered by best matches, if
                        //       there are more than one solutions after the post checking pass, then a declaration will be ambiguous
                        if (hasMatchingParameterTypes && hasCorrectArgumentCount && currentMatchingParameterTypeConversions == 0) {
                            ASTArrayAppendElement(identifier->candidateDeclarations, declaration);
                        } else if (matchKind <= CandidateFunctionMatchKindParameterTypes &&
                                   ((matchingParameterTypeCount < currentMatchingParameterTypeCount) ||
                                    ((matchingParameterTypeCount == currentMatchingParameterTypeCount) &&
                                     matchingParameterTypeConversions > currentMatchingParameterTypeConversions))) {
                            matchingDeclaration              = declaration;
                            matchKind                        = CandidateFunctionMatchKindParameterTypes;
                            matchingParameterTypeCount       = currentMatchingParameterTypeCount;
                            matchingParameterTypeConversions = currentMatchingParameterTypeConversions;
                        }
                    }
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
                            _PerformNameResolutionForExpression(context, argument, reportErrors);
                        }
                    }
                } else {
                    identifier->base.type = (ASTTypeRef)ASTContextGetBuiltinType(context, ASTBuiltinTypeKindError);
                    if (reportErrors) {
                        ReportErrorFormat("Use of unresolved identifier '%s'", StringGetCharacters(identifier->name));
                    }
                }
            } else {
                identifier->base.type = (ASTTypeRef)ASTContextGetBuiltinType(context, ASTBuiltinTypeKindError);
                if (reportErrors) {
                    ReportError("Ambiguous use of identifier");
                }
            }

            if (identifier->resolvedDeclaration && identifier->resolvedDeclaration->base.tag == ASTTagInitializerDeclaration) {
                call->base.base.flags |= ASTFlagsCallIsInitialization;
            }
        } else {
            _PerformNameResolutionForExpression(context, call->callee, reportErrors);
        }

        assert(call->callee->type);
        _ResolveDeclarationsOfTypeAndSubstituteType(context, call->callee->base.scope, &call->callee->type);

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
            call->base.type = (ASTTypeRef)ASTContextGetBuiltinType(context, ASTBuiltinTypeKindError);
            if (reportErrors) {
                ReportError("Cannot call a non function type");
            }
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
                } else if (expectedType->kind == ASTBuiltinTypeKindFloat32 || expectedType->kind == ASTBuiltinTypeKindFloat64 ||
                           expectedType->kind == ASTBuiltinTypeKindFloat) {
                    constant->kind       = ASTConstantKindFloat;
                    constant->floatValue = (Float64)constant->intValue;
                    constant->base.type  = constant->base.expectedType;
                } else {
                    if (constant->minimumBitWidth < 64) {
                        constant->base.type = (ASTTypeRef)ASTContextGetBuiltinType(context, ASTBuiltinTypeKindInt64);
                    } else {
                        constant->base.type = (ASTTypeRef)ASTContextGetBuiltinType(context, ASTBuiltinTypeKindUInt64);
                    }
                }
            } else {
                if (constant->minimumBitWidth <= 8) {
                    constant->base.type = (ASTTypeRef)ASTContextGetBuiltinType(context, ASTBuiltinTypeKindUInt8);
                } else if (constant->minimumBitWidth <= 16) {
                    constant->base.type = (ASTTypeRef)ASTContextGetBuiltinType(context, ASTBuiltinTypeKindUInt16);
                } else if (constant->minimumBitWidth <= 32) {
                    constant->base.type = (ASTTypeRef)ASTContextGetBuiltinType(context, ASTBuiltinTypeKindUInt32);
                } else {
                    constant->base.type = (ASTTypeRef)ASTContextGetBuiltinType(context, ASTBuiltinTypeKindUInt64);
                }
            }
        } else if (constant->kind == ASTConstantKindFloat) {
            if (constant->base.expectedType && constant->base.expectedType->tag == ASTTagBuiltinType) {
                ASTBuiltinTypeRef expectedType = (ASTBuiltinTypeRef)constant->base.expectedType;
                if (expectedType->kind == ASTBuiltinTypeKindFloat32) {
                    constant->base.type = (ASTTypeRef)ASTContextGetBuiltinType(context, ASTBuiltinTypeKindFloat32);
                } else {
                    constant->base.type = (ASTTypeRef)ASTContextGetBuiltinType(context, ASTBuiltinTypeKindFloat64);
                }
            } else {
                constant->base.type = (ASTTypeRef)ASTContextGetBuiltinType(context, ASTBuiltinTypeKindFloat64);
            }
        } else if (constant->kind == ASTConstantKindString) {
            constant->base.type = (ASTTypeRef)ASTContextGetStringType(context);
        } else {
            JELLY_UNREACHABLE("Unknown kind given for ASTConstantExpression in Typer!");
        }

        return;
    }

    if (expression->base.tag == ASTTagSizeOfExpression) {
        ASTSizeOfExpressionRef sizeOf = (ASTSizeOfExpressionRef)expression;
        _ResolveDeclarationsOfTypeAndSubstituteType(context, sizeOf->base.base.scope, &sizeOf->sizeType);
        sizeOf->base.type = (ASTTypeRef)ASTContextGetBuiltinType(context, ASTBuiltinTypeKindInt);
        return;
    }

    if (expression->base.tag == ASTTagSubscriptExpression) {
        ASTSubscriptExpressionRef subscript = (ASTSubscriptExpressionRef)expression;
        _PerformNameResolutionForNode(context, (ASTNodeRef)subscript->expression);

        ASTArrayIteratorRef iterator = ASTArrayGetIterator(subscript->arguments);
        while (iterator) {
            ASTExpressionRef argument = (ASTExpressionRef)ASTArrayIteratorGetElement(iterator);
            _PerformNameResolutionForNode(context, (ASTNodeRef)argument);
            iterator = ASTArrayIteratorNext(iterator);
        }

        if (subscript->expression->type->tag == ASTTagArrayType) {
            ASTArrayTypeRef arrayType = (ASTArrayTypeRef)subscript->expression->type;
            subscript->base.type      = arrayType->elementType;
        } else if (!(subscript->expression->type->tag == ASTTagBuiltinType &&
                     ((ASTBuiltinTypeRef)subscript->expression->type)->kind == ASTBuiltinTypeKindError)) {
            subscript->base.type = (ASTTypeRef)ASTContextGetBuiltinType(context, ASTBuiltinTypeKindError);
            if (reportErrors) {
                ReportError("Subscript expressions are only supported for array types");
            }
        }
        return;
    }

    if (expression->base.tag == ASTTagTypeOperationExpression) {
        ASTTypeOperationExpressionRef typeExpression = (ASTTypeOperationExpressionRef)expression;
        _PerformNameResolutionForNode(context, (ASTNodeRef)typeExpression->expression);
        _ResolveDeclarationsOfTypeAndSubstituteType(context, typeExpression->base.base.scope, &typeExpression->argumentType);
        switch (typeExpression->op) {
        case ASTTypeOperationTypeCheck:
            ReportCritical("Type checking is currently not supported!");
            return;

        case ASTTypeOperationTypeCast:
        case ASTTypeOperationTypeBitcast:
            typeExpression->base.type = typeExpression->argumentType;
            return;
        }
    }

    JELLY_UNREACHABLE("Invalid tag given for ASTExpression!");
}

static inline ASTFunctionDeclarationRef _LookupInfixFunctionInScope(SymbolTableRef symbolTable, ScopeID scope, StringRef name,
                                                                    ArrayRef parameterTypes, ASTTypeRef resultType) {
    SymbolID symbol = SymbolTableLookupSymbol(symbolTable, scope, name);
    if (symbol == kSymbolNull || !SymbolTableIsSymbolGroup(symbolTable, symbol)) {
        return NULL;
    }

    Index count = SymbolTableGetSymbolGroupEntryCount(symbolTable, symbol);
    for (Index index = 0; index < count; index++) {
        ASTDeclarationRef declaration = SymbolTableGetSymbolGroupDefinition(symbolTable, symbol, index);
        assert(declaration);

        if (!StringIsEqual(declaration->name, name)) {
            continue;
        }

        if (declaration->base.tag != ASTTagFunctionDeclaration && declaration->base.tag != ASTTagForeignFunctionDeclaration &&
            declaration->base.tag != ASTTagIntrinsicFunctionDeclaration) {
            continue;
        }

        ASTFunctionDeclarationRef function = (ASTFunctionDeclarationRef)declaration;
        if (function->fixity != ASTFixityInfix) {
            continue;
        }

        if (ASTArrayGetElementCount(function->parameters) != ArrayGetElementCount(parameterTypes)) {
            continue;
        }

        if (!ASTTypeIsEqual(function->returnType, resultType)) {
            continue;
        }

        Bool hasSameParameters          = true;
        Index parameterIndex            = 0;
        ASTArrayIteratorRef lhsIterator = ASTArrayGetIterator(function->parameters);
        while (lhsIterator) {
            ASTValueDeclarationRef lhsParameter = (ASTValueDeclarationRef)ASTArrayIteratorGetElement(lhsIterator);
            ASTTypeRef parameterType            = *((ASTTypeRef *)ArrayGetElementAtIndex(parameterTypes, parameterIndex));

            if (!ASTTypeIsEqual(lhsParameter->base.type, parameterType)) {
                hasSameParameters = false;
                break;
            }

            lhsIterator = ASTArrayIteratorNext(lhsIterator);
            parameterIndex += 1;
        }

        if (hasSameParameters) {
            return function;
        }
    }

    return NULL;
}

static inline ASTInitializerDeclarationRef _LookupInitializerInSymbolGroupByParameters(SymbolTableRef symbolTable, SymbolID symbolGroup,
                                                                                       ASTArrayRef parameters) {
    assert(SymbolTableIsSymbolGroup(symbolTable, symbolGroup));
    Index count = SymbolTableGetSymbolGroupEntryCount(symbolTable, symbolGroup);
    for (Index index = 0; index < count; index++) {
        ASTDeclarationRef declaration = (ASTDeclarationRef)SymbolTableGetSymbolGroupDefinition(symbolTable, symbolGroup, index);
        if (declaration->base.tag != ASTTagInitializerDeclaration) {
            continue;
        }

        ASTInitializerDeclarationRef initializer = (ASTInitializerDeclarationRef)declaration;
        if (ASTArrayGetElementCount(initializer->parameters) != ASTArrayGetElementCount(parameters)) {
            continue;
        }

        Bool hasSameParameters          = true;
        ASTArrayIteratorRef lhsIterator = ASTArrayGetIterator(initializer->parameters);
        ASTArrayIteratorRef rhsIterator = ASTArrayGetIterator(parameters);
        while (lhsIterator && rhsIterator) {
            ASTValueDeclarationRef lhsParameter = (ASTValueDeclarationRef)ASTArrayIteratorGetElement(lhsIterator);
            ASTValueDeclarationRef rhsParameter = (ASTValueDeclarationRef)ASTArrayIteratorGetElement(rhsIterator);

            if (!ASTTypeIsEqual(lhsParameter->base.type, rhsParameter->base.type)) {
                hasSameParameters = false;
                break;
            }

            lhsIterator = ASTArrayIteratorNext(lhsIterator);
            rhsIterator = ASTArrayIteratorNext(rhsIterator);
        }

        if (hasSameParameters) {
            return initializer;
        }
    }

    return NULL;
}

static inline ASTDeclarationRef _LookupDeclarationByNameOrMatchingFunctionSignature(SymbolTableRef symbolTable, ScopeID scope,
                                                                                    StringRef name, ASTArrayRef parameters) {
    SymbolID symbol = SymbolTableLookupSymbol(symbolTable, scope, name);
    if (symbol != kSymbolNull) {
        if (!SymbolTableIsSymbolGroup(symbolTable, symbol)) {
            return SymbolTableGetSymbolDefinition(symbolTable, symbol);
        }

        Index entryCount = SymbolTableGetSymbolGroupEntryCount(symbolTable, symbol);
        for (Index index = 0; index < entryCount; index++) {
            ASTDeclarationRef declaration = (ASTDeclarationRef)SymbolTableGetSymbolGroupDefinition(symbolTable, symbol, index);
            assert(declaration);

            if (declaration->base.tag != ASTTagFunctionDeclaration && declaration->base.tag != ASTTagForeignFunctionDeclaration &&
                declaration->base.tag != ASTTagIntrinsicFunctionDeclaration) {
                return declaration;
            }

            ASTFunctionDeclarationRef function = (ASTFunctionDeclarationRef)declaration;
            if (ASTArrayGetElementCount(function->parameters) != ASTArrayGetElementCount(parameters)) {
                continue;
            }

            Bool hasSameParameters          = true;
            ASTArrayIteratorRef lhsIterator = ASTArrayGetIterator(function->parameters);
            ASTArrayIteratorRef rhsIterator = ASTArrayGetIterator(parameters);
            while (lhsIterator && rhsIterator) {
                ASTValueDeclarationRef lhsParameter = (ASTValueDeclarationRef)ASTArrayIteratorGetElement(lhsIterator);
                ASTValueDeclarationRef rhsParameter = (ASTValueDeclarationRef)ASTArrayIteratorGetElement(rhsIterator);

                if (!ASTTypeIsEqual(lhsParameter->base.type, rhsParameter->base.type)) {
                    hasSameParameters = false;
                    break;
                }

                lhsIterator = ASTArrayIteratorNext(lhsIterator);
                rhsIterator = ASTArrayIteratorNext(rhsIterator);
            }

            if (hasSameParameters) {
                return declaration;
            }
        }
    }

    return NULL;
}
