#include "JellyCore/ASTFunctions.h"
#include "JellyCore/ASTScope.h"
#include "JellyCore/Diagnostic.h"
#include "JellyCore/TypeChecker.h"

#define _GuardValidateOnce(__NODE__)                                                                                                       \
    if ((((ASTNodeRef)__NODE__)->flags & ASTFlagsIsValidated) > 0) {                                                                       \
        return;                                                                                                                            \
    } else {                                                                                                                               \
        ((ASTNodeRef)__NODE__)->flags |= ASTFlagsIsValidated;                                                                              \
    }

// TODO: Add validation for types like expecting a constant expression for the size of an array type
// TODO: Emit warning for unreachable code which will be determined by preceding nodes with the flag ASTFlagsBlockHasTerminator

struct _TypeChecker {
    AllocatorRef allocator;
};

static inline void _TypeCheckerValidateSourceUnit(TypeCheckerRef typeChecker, ASTContextRef context, ASTSourceUnitRef sourceUnit);
static inline void _TypeCheckerValidateTopLevelNode(TypeCheckerRef typeChecker, ASTContextRef context, ASTNodeRef node);
static inline void _TypeCheckerValidateEnumerationDeclaration(TypeCheckerRef typeChecker, ASTContextRef context,
                                                              ASTEnumerationDeclarationRef declaration);
static inline void _TypeCheckerValidateFunctionDeclaration(TypeCheckerRef typeChecker, ASTContextRef context,
                                                           ASTFunctionDeclarationRef declaration);
static inline void _TypeCheckerValidateForeignFunctionDeclaration(TypeCheckerRef typeChecker, ASTContextRef context,
                                                                  ASTFunctionDeclarationRef declaration);
static inline void _TypeCheckerValidateIntrinsicFunctionDeclaration(TypeCheckerRef typeChecker, ASTContextRef context,
                                                                    ASTFunctionDeclarationRef declaration);
static inline void _TypeCheckerValidateStructureDeclaration(TypeCheckerRef typeChecker, ASTContextRef context,
                                                            ASTStructureDeclarationRef declaration);
static inline void _TypeCheckerValidateVariableDeclaration(TypeCheckerRef typeChecker, ASTContextRef context,
                                                           ASTValueDeclarationRef declaration);
static inline void _TypeCheckerValidateStatement(TypeCheckerRef typeChecker, ASTContextRef context, ASTNodeRef node);
static inline void _TypeCheckerValidateSwitchStatement(TypeCheckerRef typeChecker, ASTContextRef context, ASTSwitchStatementRef statement);
static inline void _TypeCheckerValidateExpression(TypeCheckerRef typeChecker, ASTContextRef context, ASTExpressionRef expression);
static inline void _TypeCheckerValidateBlock(TypeCheckerRef typeChecker, ASTContextRef context, ASTBlockRef block);

static inline void _CheckCyclicStorageInStructureDeclaration(ASTContextRef context, ASTStructureDeclarationRef declaration,
                                                             ArrayRef parents);
static inline void _CheckIsBlockAlwaysReturning(ASTBlockRef block);
static inline void _CheckIsSwitchExhaustive(TypeCheckerRef typeChecker, ASTSwitchStatementRef statement);
static inline Bool _ASTTypeIsError(ASTTypeRef type);
static inline Bool _ASTTypeIsEqualOrError(ASTTypeRef lhs, ASTTypeRef rhs);
static inline Bool _ASTExpressionIsLValue(ASTExpressionRef expression);

TypeCheckerRef TypeCheckerCreate(AllocatorRef allocator) {
    TypeCheckerRef typeChecker = AllocatorAllocate(allocator, sizeof(struct _TypeChecker));
    typeChecker->allocator     = allocator;
    return typeChecker;
}

void TypeCheckerDestroy(TypeCheckerRef typeChecker) {
    AllocatorDeallocate(typeChecker->allocator, typeChecker);
}

void TypeCheckerValidateModule(TypeCheckerRef typeChecker, ASTContextRef context, ASTModuleDeclarationRef module) {
    _GuardValidateOnce(module);

    for (Index index = 0; index < ASTArrayGetElementCount(module->sourceUnits); index++) {
        ASTSourceUnitRef sourceUnit = (ASTSourceUnitRef)ASTArrayGetElementAtIndex(module->sourceUnits, index);
        _TypeCheckerValidateSourceUnit(typeChecker, context, sourceUnit);
    }

    if (DiagnosticEngineGetMessageCount(DiagnosticLevelError) > 0 || DiagnosticEngineGetMessageCount(DiagnosticLevelCritical) > 0) {
        return;
    }

    // Lookup entry point of program
    Bool hasError = false;
    for (Index sourceUnitIndex = 0; sourceUnitIndex < ASTArrayGetElementCount(module->sourceUnits); sourceUnitIndex++) {
        ASTSourceUnitRef sourceUnit = (ASTSourceUnitRef)ASTArrayGetElementAtIndex(module->sourceUnits, sourceUnitIndex);
        for (Index index = 0; index < ASTArrayGetElementCount(sourceUnit->declarations); index++) {
            ASTDeclarationRef declaration = (ASTDeclarationRef)ASTArrayGetElementAtIndex(sourceUnit->declarations, index);
            if (declaration->base.tag != ASTTagFunctionDeclaration) {
                continue;
            }

            if (!StringIsEqual(declaration->name, module->entryPointName)) {
                continue;
            }

            if (module->entryPoint) {
                ReportError("Invalid redeclaration of program entry point");
                hasError = true;
                break;
            }

            ASTFunctionDeclarationRef function = (ASTFunctionDeclarationRef)declaration;

            if (ASTArrayGetElementCount(function->parameters) != 0) {
                ReportError("Expected no parameters for program entry point");
                hasError = true;
                break;
            }

            if (!_ASTTypeIsEqualOrError(function->returnType, (ASTTypeRef)ASTContextGetBuiltinType(context, ASTBuiltinTypeKindVoid))) {
                ReportError("Return type of program entry point is not 'Void'");
                hasError = true;
                break;
            }

            module->entryPoint = function;
        }

        if (hasError) {
            break;
        }
    }

    if (!hasError && !module->entryPoint) {
        ReportError("No entry point specified for module");
    }
}

static inline void _TypeCheckerValidateSourceUnit(TypeCheckerRef typeChecker, ASTContextRef context, ASTSourceUnitRef sourceUnit) {
    _GuardValidateOnce(sourceUnit);

    for (Index index = 0; index < ASTArrayGetElementCount(sourceUnit->declarations); index++) {
        ASTNodeRef node = (ASTNodeRef)ASTArrayGetElementAtIndex(sourceUnit->declarations, index);
        _TypeCheckerValidateTopLevelNode(typeChecker, context, node);
    }
}

static inline void _TypeCheckerValidateTopLevelNode(TypeCheckerRef typeChecker, ASTContextRef context, ASTNodeRef node) {
    if (node->tag == ASTTagLoadDirective) {
        return;
    }

    if (node->tag == ASTTagEnumerationDeclaration) {
        return _TypeCheckerValidateEnumerationDeclaration(typeChecker, context, (ASTEnumerationDeclarationRef)node);
    }

    if (node->tag == ASTTagFunctionDeclaration) {
        return _TypeCheckerValidateFunctionDeclaration(typeChecker, context, (ASTFunctionDeclarationRef)node);
    }

    if (node->tag == ASTTagForeignFunctionDeclaration) {
        return _TypeCheckerValidateForeignFunctionDeclaration(typeChecker, context, (ASTFunctionDeclarationRef)node);
    }

    if (node->tag == ASTTagIntrinsicFunctionDeclaration) {
        return _TypeCheckerValidateIntrinsicFunctionDeclaration(typeChecker, context, (ASTFunctionDeclarationRef)node);
    }

    if (node->tag == ASTTagStructureDeclaration) {
        return _TypeCheckerValidateStructureDeclaration(typeChecker, context, (ASTStructureDeclarationRef)node);
    }

    if (node->tag == ASTTagValueDeclaration) {
        return _TypeCheckerValidateVariableDeclaration(typeChecker, context, (ASTValueDeclarationRef)node);
    }

    JELLY_UNREACHABLE("Invalid tag given for ASTNode!");
}

static inline void _TypeCheckerValidateEnumerationDeclaration(TypeCheckerRef typeChecker, ASTContextRef context,
                                                              ASTEnumerationDeclarationRef declaration) {
    _GuardValidateOnce(declaration);

    ArrayRef values        = ArrayCreateEmpty(typeChecker->allocator, sizeof(UInt64), ASTArrayGetElementCount(declaration->elements));
    UInt64 nextMemberValue = 0;
    for (Index index = 0; index < ASTArrayGetElementCount(declaration->elements); index++) {
        ASTValueDeclarationRef element = (ASTValueDeclarationRef)ASTArrayGetElementAtIndex(declaration->elements, index);
        assert(element->kind == ASTValueKindEnumerationElement);

        if (_ASTTypeIsError(element->base.type)) {
            continue;
        }

        if (!element->initializer) {
            ASTConstantExpressionRef constant = ASTContextCreateConstantIntExpression(context, SourceRangeNull(), element->base.base.scope,
                                                                                      nextMemberValue);
            constant->base.type               = (ASTTypeRef)ASTContextGetBuiltinType(context, ASTBuiltinTypeKindInt);
            element->initializer              = (ASTExpressionRef)constant;
        }

        _TypeCheckerValidateExpression(typeChecker, context, element->initializer);

        if (_ASTTypeIsError(element->initializer->type)) {
            element->base.type = (ASTTypeRef)ASTContextGetBuiltinType(context, ASTBuiltinTypeKindError);
            continue;
        }

        if (!ASTTypeIsEqual(element->base.type, element->initializer->type)) {
            ReportError("Element initializer has mismatching type");
            continue;
        }

        if (element->initializer->base.tag != ASTTagConstantExpression) {
            ReportError("Element initializer has to be a constant value");
            continue;
        }

        ASTConstantExpressionRef constant = (ASTConstantExpressionRef)element->initializer;
        assert(constant->kind == ASTConstantKindInt);

        Bool isOverlappingOtherElementValue = false;
        for (Index valueIndex = 0; valueIndex < ArrayGetElementCount(values); valueIndex++) {
            UInt64 value = *((UInt64 *)ArrayGetElementAtIndex(values, valueIndex));
            if (value == constant->intValue) {
                isOverlappingOtherElementValue = true;
                break;
            }
        }

        if (isOverlappingOtherElementValue) {
            ReportErrorFormat("Invalid reuse of value %llu for different enum elements", constant->intValue);
        } else {
            ArrayAppendElement(values, &constant->intValue);
            nextMemberValue = constant->intValue + 1;
        }
    }
}

static inline void _TypeCheckerValidateFunctionDeclaration(TypeCheckerRef typeChecker, ASTContextRef context,
                                                           ASTFunctionDeclarationRef declaration) {
    _GuardValidateOnce(declaration);

    for (Index index = 0; index < ASTArrayGetElementCount(declaration->parameters); index++) {
        ASTValueDeclarationRef parameter = (ASTValueDeclarationRef)ASTArrayGetElementAtIndex(declaration->parameters, index);
        assert(parameter->base.type);

        if (parameter->base.type->tag == ASTTagBuiltinType) {
            ASTBuiltinTypeRef builtinType = (ASTBuiltinTypeRef)parameter->base.type;
            if (builtinType->kind == ASTBuiltinTypeKindVoid) {
                parameter->base.type = (ASTTypeRef)ASTContextGetBuiltinType(context, ASTBuiltinTypeKindError);
                ReportError("Cannot pass 'Void' type as parameter");
            }
        }
    }

    assert(declaration->returnType->tag != ASTTagOpaqueType);
    Bool requiresReturnValue = true;
    if (declaration->returnType->tag == ASTTagBuiltinType) {
        ASTBuiltinTypeRef builtinType = (ASTBuiltinTypeRef)declaration->returnType;
        if (builtinType->kind == ASTBuiltinTypeKindVoid) {
            requiresReturnValue = false;
        }
    }

    _CheckIsBlockAlwaysReturning(declaration->body);
    if (requiresReturnValue && !(declaration->body->base.flags & ASTFlagsStatementIsAlwaysReturning)) {
        ReportError("Not all code paths return a value");
    }

    for (Index index = 0; index < ASTArrayGetElementCount(declaration->body->statements); index++) {
        ASTNodeRef child = (ASTNodeRef)ASTArrayGetElementAtIndex(declaration->body->statements, index);
        _TypeCheckerValidateStatement(typeChecker, context, child);
    }
}

static inline void _TypeCheckerValidateForeignFunctionDeclaration(TypeCheckerRef typeChecker, ASTContextRef context,
                                                                  ASTFunctionDeclarationRef declaration) {
    _GuardValidateOnce(declaration);

    for (Index index = 0; index < ASTArrayGetElementCount(declaration->parameters); index++) {
        ASTValueDeclarationRef parameter = (ASTValueDeclarationRef)ASTArrayGetElementAtIndex(declaration->parameters, index);
        assert(parameter->base.type);

        if (parameter->base.type->tag == ASTTagBuiltinType) {
            ASTBuiltinTypeRef builtinType = (ASTBuiltinTypeRef)parameter->base.type;
            if (builtinType->kind == ASTBuiltinTypeKindVoid) {
                parameter->base.type = (ASTTypeRef)ASTContextGetBuiltinType(context, ASTBuiltinTypeKindError);
                ReportError("Cannot pass 'Void' type as parameter");
            }
        }
    }

    assert(declaration->returnType->tag != ASTTagOpaqueType);
    Bool requiresReturnValue = true;
    if (declaration->returnType->tag == ASTTagBuiltinType) {
        ASTBuiltinTypeRef builtinType = (ASTBuiltinTypeRef)declaration->returnType;
        if (builtinType->kind == ASTBuiltinTypeKindVoid) {
            requiresReturnValue = false;
        }
    }
}

static inline void _TypeCheckerValidateIntrinsicFunctionDeclaration(TypeCheckerRef typeChecker, ASTContextRef context,
                                                                    ASTFunctionDeclarationRef declaration) {
    _GuardValidateOnce(declaration);

    for (Index index = 0; index < ASTArrayGetElementCount(declaration->parameters); index++) {
        ASTValueDeclarationRef parameter = (ASTValueDeclarationRef)ASTArrayGetElementAtIndex(declaration->parameters, index);
        assert(parameter->base.type);

        if (parameter->base.type->tag == ASTTagBuiltinType) {
            ASTBuiltinTypeRef builtinType = (ASTBuiltinTypeRef)parameter->base.type;
            if (builtinType->kind == ASTBuiltinTypeKindVoid) {
                parameter->base.type = (ASTTypeRef)ASTContextGetBuiltinType(context, ASTBuiltinTypeKindError);
                ReportError("Cannot pass 'Void' type as parameter");
            }
        }
    }

    assert(declaration->returnType->tag != ASTTagOpaqueType);
    Bool requiresReturnValue = true;
    if (declaration->returnType->tag == ASTTagBuiltinType) {
        ASTBuiltinTypeRef builtinType = (ASTBuiltinTypeRef)declaration->returnType;
        if (builtinType->kind == ASTBuiltinTypeKindVoid) {
            requiresReturnValue = false;
        }
    }
}

static inline void _TypeCheckerValidateStructureDeclaration(TypeCheckerRef typeChecker, ASTContextRef context,
                                                            ASTStructureDeclarationRef declaration) {
    _GuardValidateOnce(declaration);

    ArrayRef parents = ArrayCreateEmpty(typeChecker->allocator, sizeof(ASTDeclarationRef), 8);
    ArrayAppendElement(parents, &declaration);
    _CheckCyclicStorageInStructureDeclaration(context, declaration, parents);
    ArrayDestroy(parents);

    for (Index index = 0; index < ASTArrayGetElementCount(declaration->values); index++) {
        ASTValueDeclarationRef value = (ASTValueDeclarationRef)ASTArrayGetElementAtIndex(declaration->values, index);
        assert(value->base.type);

        if (value->base.type->tag == ASTTagBuiltinType) {
            ASTBuiltinTypeRef builtinType = (ASTBuiltinTypeRef)value->base.type;
            if (builtinType->kind == ASTBuiltinTypeKindVoid) {
                value->base.type = (ASTTypeRef)ASTContextGetBuiltinType(context, ASTBuiltinTypeKindError);
                ReportError("Cannot store 'Void' type as member");
            }
        }
    }
}

static inline void _TypeCheckerValidateVariableDeclaration(TypeCheckerRef typeChecker, ASTContextRef context,
                                                           ASTValueDeclarationRef declaration) {
    assert(declaration->kind == ASTValueKindVariable);
    _GuardValidateOnce(declaration);

    if (declaration->initializer) {
        _TypeCheckerValidateExpression(typeChecker, context, declaration->initializer);

        if (!_ASTTypeIsEqualOrError(declaration->base.type, declaration->initializer->type)) {
            ReportErrorFormat("Assignment expression of '%s' has mismatching type", StringGetCharacters(declaration->base.name));
        }
    }
}

static inline void _TypeCheckerValidateStatement(TypeCheckerRef typeChecker, ASTContextRef context, ASTNodeRef node) {
    switch (node->tag) {
    case ASTTagIfStatement: {
        ASTIfStatementRef statement = (ASTIfStatementRef)node;
        _TypeCheckerValidateExpression(typeChecker, context, statement->condition);

        assert(statement->condition->type);
        if (!_ASTTypeIsEqualOrError(statement->condition->type, (ASTTypeRef)ASTContextGetBuiltinType(context, ASTBuiltinTypeKindBool))) {
            ReportError("Expected type Bool for condition of if statement");
        }

        _TypeCheckerValidateBlock(typeChecker, context, statement->thenBlock);
        _TypeCheckerValidateBlock(typeChecker, context, statement->elseBlock);
        return;
    }

    case ASTTagLoopStatement: {
        ASTLoopStatementRef statement = (ASTLoopStatementRef)node;
        _TypeCheckerValidateExpression(typeChecker, context, statement->condition);

        assert(statement->condition->type);
        if (!_ASTTypeIsEqualOrError(statement->condition->type, (ASTTypeRef)ASTContextGetBuiltinType(context, ASTBuiltinTypeKindBool))) {
            ReportError("Expected type Bool for condition of loop statement");
        }

        _TypeCheckerValidateBlock(typeChecker, context, statement->loopBlock);
        return;
    }

    case ASTTagCaseStatement: {
        ASTCaseStatementRef statement = (ASTCaseStatementRef)node;
        ASTScopeRef scope             = statement->base.scope;
        while (scope && (scope->kind != ASTScopeKindSwitch)) {
            scope = ASTScopeGetNextParentForLookup(scope);
        }

        if (scope) {
            assert(scope->node->tag == ASTTagSwitchStatement);
            statement->enclosingSwitch = (ASTSwitchStatementRef)scope->node;
        } else {
            ReportError("'case' is only allowed inside a switch");
        }

        if (ASTArrayGetElementCount(statement->body->statements) < 1) {
            ReportError("Switch case should contain at least one statement");
        }

        switch (statement->kind) {
        case ASTCaseKindConditional: {
            _TypeCheckerValidateExpression(typeChecker, context, statement->condition);
            // TODO: Check if type is comparable with switch argument type
            break;
        }

        case ASTCaseKindElse: {
            break;
        }

        default:
            JELLY_UNREACHABLE("Invalid kind given for ASTCaseStatement");
            break;
        }

        _TypeCheckerValidateBlock(typeChecker, context, statement->body);
        return;
    }

    case ASTTagSwitchStatement: {
        return _TypeCheckerValidateSwitchStatement(typeChecker, context, (ASTSwitchStatementRef)node);
    }

    case ASTTagControlStatement: {
        ASTControlStatementRef control = (ASTControlStatementRef)node;
        switch (control->kind) {
        case ASTControlKindBreak: {
            ASTScopeRef scope = control->base.scope;
            while (scope && (scope->kind != ASTScopeKindLoop && scope->kind != ASTScopeKindSwitch)) {
                scope = ASTScopeGetNextParentForLookup(scope);
            }

            if (scope) {
                control->enclosingNode = scope->node;
            } else {
                ReportError("'break' is only allowed inside a switch or loop");
            }
            break;
        }

        case ASTControlKindContinue: {
            ASTScopeRef scope = control->base.scope;
            while (scope && (scope->kind != ASTScopeKindLoop)) {
                scope = ASTScopeGetNextParentForLookup(scope);
            }

            if (scope) {
                control->enclosingNode = scope->node;
            } else {
                ReportError("'continue' is only allowed inside a loop");
            }
            break;
        }

        case ASTControlKindFallthrough: {
            ASTScopeRef scope = control->base.scope;
            while (scope && (scope->kind != ASTScopeKindCase)) {
                scope = ASTScopeGetNextParentForLookup(scope);
            }

            if (scope) {
                control->enclosingNode = scope->node;
            } else {
                ReportError("'fallthrough' is only allowed inside a case");
            }
            break;
        }

        case ASTControlKindReturn: {
            if (control->result) {
                _TypeCheckerValidateExpression(typeChecker, context, control->result);
            }

            ASTScopeRef scope = control->base.scope;
            while (scope && (scope->kind != ASTScopeKindFunction)) {
                scope = ASTScopeGetNextParentForLookup(scope);
            }

            if (scope) {
                control->enclosingNode = scope->node;

                assert(control->enclosingNode->tag == ASTTagFunctionDeclaration);
                ASTFunctionDeclarationRef function = (ASTFunctionDeclarationRef)control->enclosingNode;

                ASTTypeRef resultType = (ASTTypeRef)ASTContextGetBuiltinType(context, ASTBuiltinTypeKindVoid);
                if (control->result) {
                    assert(control->result->type);
                    resultType = control->result->type;
                }

                if (!_ASTTypeIsEqualOrError(resultType, function->returnType)) {
                    ReportError("Type mismatch in return statement");
                }
            } else {
                ReportError("'return' is only allowed inside a function");
            }
            break;
        }

        default:
            JELLY_UNREACHABLE("Invalid kind given for ASTControlStatement");
            break;
        }
        return;
    }

    case ASTTagUnaryExpression:
    case ASTTagBinaryExpression:
    case ASTTagAssignmentExpression:
    case ASTTagIdentifierExpression:
    case ASTTagMemberAccessExpression:
    case ASTTagCallExpression:
    case ASTTagConstantExpression: {
        return _TypeCheckerValidateExpression(typeChecker, context, (ASTExpressionRef)node);
    }

    case ASTTagValueDeclaration: {
        return _TypeCheckerValidateVariableDeclaration(typeChecker, context, (ASTValueDeclarationRef)node);
    }

    default:
        JELLY_UNREACHABLE("Invalid tag given for ASTNode!");
        break;
    }
}

static inline void _TypeCheckerValidateSwitchStatement(TypeCheckerRef typeChecker, ASTContextRef context, ASTSwitchStatementRef statement) {
    _GuardValidateOnce(statement);

    _TypeCheckerValidateExpression(typeChecker, context, statement->argument);
    Bool containsElseCase = false;
    for (Index index = 0; index < ASTArrayGetElementCount(statement->cases); index++) {
        ASTNodeRef child = (ASTNodeRef)ASTArrayGetElementAtIndex(statement->cases, index);
        assert(child->tag == ASTTagCaseStatement);
        ASTCaseStatementRef caseStatement = (ASTCaseStatementRef)child;
        _TypeCheckerValidateStatement(typeChecker, context, child);

        if (caseStatement->kind == ASTCaseKindElse) {
            if (index + 1 < ASTArrayGetElementCount(statement->cases)) {
                ReportError("The 'else' case has to be the last case of a switch statement");
            }

            if (containsElseCase) {
                ReportError("There can only be a single 'else' case inside a switch statement");
            }

            containsElseCase = true;
        }
    }

    _CheckIsSwitchExhaustive(typeChecker, statement);
    if (!(statement->base.flags & ASTFlagsSwitchIsExhaustive)) {
        ReportError("Switch statement must be exhaustive");
    }
}

static inline void _TypeCheckerValidateExpression(TypeCheckerRef typeChecker, ASTContextRef context, ASTExpressionRef expression) {
    _GuardValidateOnce(expression);

    switch (expression->base.tag) {
    case ASTTagReferenceExpression: {
        // TODO: Validate expression
        break;
    }

    case ASTTagDereferenceExpression: {
        // TODO: Validate expression
        break;
    }

    case ASTTagUnaryExpression: {
        // TODO: Validate expression
        break;
    }

    case ASTTagBinaryExpression: {
        // TODO: Validate expression
        break;
    }

    case ASTTagIdentifierExpression: {
        break;
    }

    case ASTTagMemberAccessExpression: {
        break;
    }

    case ASTTagAssignmentExpression: {
        ASTAssignmentExpressionRef assignment = (ASTAssignmentExpressionRef)expression;
        _TypeCheckerValidateExpression(typeChecker, context, assignment->variable);
        _TypeCheckerValidateExpression(typeChecker, context, assignment->expression);

        if (!_ASTExpressionIsLValue(assignment->variable)) {
            ReportError("Left hand side of assignment expression is not assignable");
        }

        assert(assignment->variable->type);
        assert(assignment->expression->type);
        if (!_ASTTypeIsEqualOrError(assignment->variable->type, assignment->expression->type)) {
            ReportError("Assignment expression has mismatching type");
        }

        // TODO: If operation is a compound assignment then check if that operation is available for the given variable and expression type
        break;
    }

    case ASTTagCallExpression: {
        ASTCallExpressionRef call = (ASTCallExpressionRef)expression;
        _TypeCheckerValidateExpression(typeChecker, context, call->callee);
        for (Index index = 0; index < ASTArrayGetElementCount(call->arguments); index++) {
            ASTExpressionRef argument = (ASTExpressionRef)ASTArrayGetElementAtIndex(call->arguments, index);
            _TypeCheckerValidateExpression(typeChecker, context, argument);
        }

        if (!_ASTTypeIsError(call->callee->type)) {
            if (call->callee->type->tag == ASTTagFunctionType) {
                ASTFunctionTypeRef functionType    = (ASTFunctionTypeRef)call->callee->type;
                ASTFunctionDeclarationRef function = (ASTFunctionDeclarationRef)functionType->declaration;
                assert(function->fixity == ASTFixityNone);

                _TypeCheckerValidateFunctionDeclaration(typeChecker, context, function);

                if (ASTArrayGetElementCount(call->arguments) == ASTArrayGetElementCount(function->parameters)) {
                    for (Index index = 0; index < ASTArrayGetElementCount(function->parameters); index++) {
                        ASTValueDeclarationRef parameter = (ASTValueDeclarationRef)ASTArrayGetElementAtIndex(function->parameters, index);
                        ASTExpressionRef argument        = (ASTExpressionRef)ASTArrayGetElementAtIndex(call->arguments, index);

                        if (!_ASTTypeIsEqualOrError(parameter->base.type, argument->type)) {
                            ReportErrorFormat("Mismatching type for parameter '%s'", StringGetCharacters(parameter->base.name));
                        }
                    }
                } else {
                    ReportErrorFormat("Invalid argument count expected '%zu' found '%zu'", ASTArrayGetElementCount(function->parameters),
                                      ASTArrayGetElementCount(call->arguments));
                }

            } else {
                ReportError("Cannot call a non function type");
            }
        }
        break;
    }

    case ASTTagConstantExpression: {
        break;
    }

    default:
        JELLY_UNREACHABLE("Invalid tag given for ASTExpression");
        break;
    }
}

static inline void _TypeCheckerValidateBlock(TypeCheckerRef typeChecker, ASTContextRef context, ASTBlockRef block) {
    _GuardValidateOnce(block);

    for (Index index = 0; index < ASTArrayGetElementCount(block->statements); index++) {
        ASTNodeRef statement = ASTArrayGetElementAtIndex(block->statements, index);
        _TypeCheckerValidateStatement(typeChecker, context, statement);

        // Every control statement is a terminator
        if (statement->tag == ASTTagControlStatement) {
            block->base.flags |= ASTFlagsBlockHasTerminator;
        }
    }
}

static inline void _CheckCyclicStorageInStructureDeclaration(ASTContextRef context, ASTStructureDeclarationRef declaration,
                                                             ArrayRef parents) {
    for (Index index = 0; index < ASTArrayGetElementCount(declaration->values); index++) {
        ASTNodeRef child = (ASTNodeRef)ASTArrayGetElementAtIndex(declaration->values, index);
        assert(child->tag == ASTTagValueDeclaration);

        ASTValueDeclarationRef value = (ASTValueDeclarationRef)child;
        assert(value->kind == ASTValueKindVariable);
        assert(value->base.type && value->base.type->tag != ASTTagOpaqueType);

        if (value->base.type->tag == ASTTagStructureType) {
            ASTStructureTypeRef valueType = (ASTStructureTypeRef)value->base.type;
            assert(valueType->declaration);

            for (Index parentIndex = 0; parentIndex < ArrayGetElementCount(parents); parentIndex++) {
                ASTStructureDeclarationRef parent = *((ASTStructureDeclarationRef *)ArrayGetElementAtIndex(parents, parentIndex));
                if (parent == valueType->declaration) {
                    ReportError("Struct cannot store a variable of same type recursively");
                    declaration->base.base.flags |= ASTFlagsStructureHasCyclicStorage;
                    return;
                }
            }

            ArrayAppendElement(parents, &valueType->declaration);
            _CheckCyclicStorageInStructureDeclaration(context, valueType->declaration, parents);
        }
    }
}

static inline void _CheckIsBlockAlwaysReturning(ASTBlockRef block) {
    if (block->base.flags & ASTFlagsStatementIsAlwaysReturning) {
        return;
    }

    Bool isAlwaysReturning = false;
    ASTScopeRef scope      = block->base.scope;
    for (Index index = 0; index < ASTArrayGetElementCount(block->statements); index++) {
        ASTNodeRef statement = (ASTNodeRef)ASTArrayGetElementAtIndex(block->statements, index);

        if (statement->tag == ASTTagControlStatement) {
            ASTControlStatementRef control = (ASTControlStatementRef)statement;
            if (control->kind == ASTControlKindReturn) {
                isAlwaysReturning = true;
            }

            if (control->kind == ASTControlKindContinue) {
                ASTScopeRef loopScope = scope;
                while (loopScope && loopScope->kind != ASTScopeKindLoop) {
                    loopScope = ASTScopeGetNextParentForLookup(loopScope);
                }

                if (loopScope && loopScope->kind == ASTScopeKindLoop) {
                    isAlwaysReturning = true;
                }
            }
        }

        if (statement->tag == ASTTagIfStatement) {
            if (statement->flags & ASTFlagsStatementIsAlwaysReturning) {
                isAlwaysReturning = true;
                continue;
            }

            ASTIfStatementRef ifStatement = (ASTIfStatementRef)statement;
            _CheckIsBlockAlwaysReturning(ifStatement->thenBlock);
            _CheckIsBlockAlwaysReturning(ifStatement->elseBlock);

            if ((ifStatement->thenBlock->base.flags & ASTFlagsStatementIsAlwaysReturning) &&
                (ifStatement->elseBlock->base.flags & ASTFlagsStatementIsAlwaysReturning)) {
                ifStatement->base.flags |= ASTFlagsStatementIsAlwaysReturning;
            }

            if (statement->flags & ASTFlagsStatementIsAlwaysReturning) {
                isAlwaysReturning = true;
            }
        }

        if (statement->tag == ASTTagSwitchStatement) {
            if (statement->flags & ASTFlagsStatementIsAlwaysReturning) {
                isAlwaysReturning = true;
                continue;
            }

            ASTSwitchStatementRef switchStatement = (ASTSwitchStatementRef)statement;
            Bool isSwitchAlwaysReturning          = true;
            for (Index index = 0; index < ASTArrayGetElementCount(switchStatement->cases); index++) {
                ASTCaseStatementRef child = (ASTCaseStatementRef)ASTArrayGetElementAtIndex(switchStatement->cases, index);
                _CheckIsBlockAlwaysReturning(child->body);
                if (!(child->body->base.flags & ASTFlagsStatementIsAlwaysReturning)) {
                    isSwitchAlwaysReturning = false;
                }
            }

            if (isSwitchAlwaysReturning) {
                switchStatement->base.flags |= ASTFlagsStatementIsAlwaysReturning;
            }

            if (switchStatement->base.flags & ASTFlagsStatementIsAlwaysReturning) {
                isAlwaysReturning = true;
            }
        }
    }

    if (isAlwaysReturning) {
        block->base.flags |= ASTFlagsStatementIsAlwaysReturning;
    }
}

// TODO: Verify do we have to check `break` statements explicity, if there is any then the switch is not exhaustive!
static inline void _CheckIsSwitchExhaustive(TypeCheckerRef typeChecker, ASTSwitchStatementRef statement) {
    assert(ASTArrayGetElementCount(statement->cases) > 0);

    // The type checker requires the else-case of the switch to be always the last one so checking the last statement first for an else case
    // will be enough assuming that an error will be reported if the else is not the last case
    ASTCaseStatementRef lastCaseStatement = ASTArrayGetElementAtIndex(statement->cases, ASTArrayGetElementCount(statement->cases) - 1);
    if (lastCaseStatement->kind == ASTCaseKindElse) {
        statement->base.flags |= ASTFlagsSwitchIsExhaustive;
        return;
    }

    assert(statement->argument->type && statement->argument->type->tag != ASTTagOpaqueType);
    if (statement->argument->type->tag == ASTTagEnumerationType) {
        ASTEnumerationTypeRef enumerationType    = (ASTEnumerationTypeRef)statement->argument->type;
        ASTEnumerationDeclarationRef enumeration = enumerationType->declaration;

        ArrayRef intValues = ArrayCreateEmpty(typeChecker->allocator, sizeof(UInt64), ASTArrayGetElementCount(enumeration->elements));
        for (Index index = 0; index < ASTArrayGetElementCount(enumeration->elements); index++) {
            ASTValueDeclarationRef element = (ASTValueDeclarationRef)ASTArrayGetElementAtIndex(enumeration->elements, index);
            assert(element->initializer && element->initializer->base.tag == ASTTagConstantExpression);

            ASTConstantExpressionRef constant = (ASTConstantExpressionRef)element->initializer;
            assert(constant->kind == ASTConstantKindInt);

            ArrayAppendElement(intValues, &constant->intValue);
        }

        for (Index index = 0; index < ASTArrayGetElementCount(statement->cases); index++) {
            ASTCaseStatementRef child = (ASTCaseStatementRef)ASTArrayGetElementAtIndex(statement->cases, index);
            if (child->condition->base.tag == ASTTagIdentifierExpression) {
                ASTIdentifierExpressionRef identifier = (ASTIdentifierExpressionRef)child->condition;
                assert(identifier->resolvedDeclaration && identifier->resolvedDeclaration->base.tag == ASTTagValueDeclaration);

                ASTValueDeclarationRef value = (ASTValueDeclarationRef)identifier->resolvedDeclaration;
                assert(value->initializer && value->initializer->base.tag == ASTTagConstantExpression);

                ASTConstantExpressionRef initializer = (ASTConstantExpressionRef)value->initializer;
                assert(initializer->kind == ASTConstantKindInt);

                for (Index valueIndex = 0; valueIndex < ArrayGetElementCount(intValues); valueIndex++) {
                    UInt64 value = *((UInt64 *)ArrayGetElementAtIndex(intValues, valueIndex));
                    if (value == initializer->intValue) {
                        ArrayRemoveElementAtIndex(intValues, valueIndex);
                        break;
                    }
                }
            }
        }

        if (ArrayGetElementCount(intValues) == 0) {
            statement->base.flags |= ASTFlagsSwitchIsExhaustive;
        }

        ArrayDestroy(intValues);
    } else if (statement->argument->type->tag == ASTTagBuiltinType) {
        ASTBuiltinTypeRef type = (ASTBuiltinTypeRef)statement->argument->type;
        if (type->kind == ASTBuiltinTypeKindBool) {
            ArrayRef boolValues = ArrayCreateEmpty(typeChecker->allocator, sizeof(Bool), 2);
            Bool trueValue      = true;
            Bool falseValue     = false;
            ArrayAppendElement(boolValues, &trueValue);
            ArrayAppendElement(boolValues, &falseValue);
            for (Index index = 0; index < ASTArrayGetElementCount(statement->cases); index++) {
                ASTCaseStatementRef child = (ASTCaseStatementRef)ASTArrayGetElementAtIndex(statement->cases, index);
                if (child->condition->base.tag == ASTTagConstantExpression) {
                    ASTConstantExpressionRef constant = (ASTConstantExpressionRef)child->condition;
                    if (constant->kind == ASTConstantKindBool) {
                        for (Index valueIndex = 0; valueIndex < ArrayGetElementCount(boolValues); valueIndex++) {
                            Bool value = *((Bool *)ArrayGetElementAtIndex(boolValues, valueIndex));
                            if (value == constant->boolValue) {
                                ArrayRemoveElementAtIndex(boolValues, valueIndex);
                                break;
                            }
                        }
                    }
                }
            }

            if (ArrayGetElementCount(boolValues) == 0) {
                statement->base.flags |= ASTFlagsSwitchIsExhaustive;
            }

            ArrayDestroy(boolValues);
        }
    }
}

static inline Bool _ASTTypeIsError(ASTTypeRef type) {
    assert(type);

    if (type->tag == ASTTagBuiltinType) {
        ASTBuiltinTypeRef builtin = (ASTBuiltinTypeRef)type;
        return builtin->kind == ASTBuiltinTypeKindError;
    }

    return false;
}

static inline Bool _ASTTypeIsEqualOrError(ASTTypeRef lhs, ASTTypeRef rhs) {
    if (_ASTTypeIsError(lhs) || _ASTTypeIsError(rhs)) {
        return true;
    }

    return ASTTypeIsEqual(lhs, rhs);
}

static inline Bool _ASTExpressionIsLValue(ASTExpressionRef expression) {
    // TODO: This is only partially correct and doesn't cover every assignable value...
    assert(expression->type && expression->type->tag != ASTTagOpaqueType);

    // We do not allow unary, binary, assignment and call expressions to be assignable even if they would be valid lvalues
    switch (expression->base.tag) {
    case ASTTagConstantExpression:
    case ASTTagUnaryExpression:
    case ASTTagBinaryExpression:
    case ASTTagCallExpression:
    case ASTTagAssignmentExpression:
        return false;

    case ASTTagIdentifierExpression: {
        ASTIdentifierExpressionRef identifier = (ASTIdentifierExpressionRef)expression;
        assert(identifier->resolvedDeclaration);

        if (identifier->resolvedDeclaration->base.tag == ASTTagValueDeclaration) {
            ASTValueDeclarationRef value = (ASTValueDeclarationRef)identifier->resolvedDeclaration;
            if (value->kind == ASTValueKindVariable) {
                return true;
            }
        }

        return false;
    }

    case ASTTagMemberAccessExpression: {
        ASTMemberAccessExpressionRef memberAccess = (ASTMemberAccessExpressionRef)expression;
        if (_ASTExpressionIsLValue(memberAccess->argument)) {
            return true;
        }

        return false;
    }

    default:
        JELLY_UNREACHABLE("Invalid tag given for ASTExpression");
        break;
    }
}
