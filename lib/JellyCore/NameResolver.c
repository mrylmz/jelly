#include "JellyCore/Diagnostic.h"
#include "JellyCore/NameResolver.h"

// TODO: @CandidateDeclarations Add support for resolution of candidate declarations to allow contextual resolutions
// TODO: Resolve control flow paths for loop, control, ... statements
// TODO: May rename to ASTResolver if it resolves more than just identifiers?

struct _NameResolver {
    AllocatorRef allocator;
};

SymbolRef _NameResolverResolveDeclaration(NameResolverRef resolver, ScopeRef scope, SourceRange location, StringRef name);
SymbolRef _NameResolverResolveEnumerationElement(NameResolverRef resolver, ASTModuleDeclarationRef module, StringRef name);

NameResolverRef NameResolverCreate(AllocatorRef allocator) {
    NameResolverRef resolver = AllocatorAllocate(allocator, sizeof(struct _NameResolver));
    resolver->allocator      = allocator;
    return resolver;
}

void NameResolverDestroy(NameResolverRef resolver) {
    AllocatorDeallocate(resolver->allocator, resolver);
}

void NameResolverResolve(NameResolverRef resolver, ASTContextRef context, ASTNodeRef node) {
    switch (node->tag) {
    case ASTTagSourceUnit: {
        ASTSourceUnitRef sourceUnit = (ASTSourceUnitRef)node;
        if (sourceUnit->declarations) {
            NameResolverResolve(resolver, context, (ASTNodeRef)sourceUnit->declarations);
        }
        return;
    }

    case ASTTagLinkedList: {
        ASTLinkedListRef list = (ASTLinkedListRef)node;
        while (list) {
            NameResolverResolve(resolver, context, list->node);
            list = list->next;
        }
        return;
    }

    case ASTTagBlock: {
        ASTBlockRef block = (ASTBlockRef)node;
        if (block->statements) {
            NameResolverResolve(resolver, context, (ASTNodeRef)block->statements);
        }
        return;
    }

    case ASTTagIfStatement: {
        ASTIfStatementRef statement = (ASTIfStatementRef)node;
        NameResolverResolve(resolver, context, (ASTNodeRef)statement->condition);
        NameResolverResolve(resolver, context, (ASTNodeRef)statement->thenBlock);
        NameResolverResolve(resolver, context, (ASTNodeRef)statement->elseBlock);
        return;
    }

    case ASTTagLoopStatement: {
        ASTLoopStatementRef loop = (ASTLoopStatementRef)node;
        NameResolverResolve(resolver, context, (ASTNodeRef)loop->condition);
        NameResolverResolve(resolver, context, (ASTNodeRef)loop->loopBlock);
        return;
    }

    case ASTTagCaseStatement: {
        ASTCaseStatementRef statement = (ASTCaseStatementRef)node;
        if (statement->kind == ASTCaseKindConditional) {
            NameResolverResolve(resolver, context, (ASTNodeRef)statement->condition);
            NameResolverResolve(resolver, context, (ASTNodeRef)statement->body);
        } else if (statement->kind == ASTCaseKindElse) {
            NameResolverResolve(resolver, context, (ASTNodeRef)statement->body);
        } else {
            JELLY_UNREACHABLE("Unknown kind given for ASTCaseKind in NameResolver!");
        }
        return;
    }

    case ASTTagSwitchStatement: {
        ASTSwitchStatementRef statement = (ASTSwitchStatementRef)node;
        NameResolverResolve(resolver, context, (ASTNodeRef)statement->argument);
        if (statement->cases) {
            NameResolverResolve(resolver, context, (ASTNodeRef)statement->cases);
        }
        return;
    }

    case ASTTagControlStatement: {
        ASTControlStatementRef control = (ASTControlStatementRef)node;
        if (control->kind == ASTControlKindReturn && control->result) {
            NameResolverResolve(resolver, context, (ASTNodeRef)control->result);
        }
        return;
    }

    case ASTTagUnaryExpression: {
        ASTUnaryExpressionRef unary = (ASTUnaryExpressionRef)node;
        NameResolverResolve(resolver, context, (ASTNodeRef)unary->arguments[0]);
        return;
    }

    case ASTTagBinaryExpression: {
        ASTBinaryExpressionRef binary = (ASTBinaryExpressionRef)node;
        NameResolverResolve(resolver, context, (ASTNodeRef)binary->arguments[0]);
        NameResolverResolve(resolver, context, (ASTNodeRef)binary->arguments[1]);

        SymbolRef callee           = ScopeInsertUniqueSymbol(binary->base.base.scope, binary->base.base.location);
        SymbolRef result           = ScopeInsertUniqueSymbol(binary->base.base.scope, binary->base.base.location);
        ASTApplicationTypeRef type = (ASTApplicationTypeRef)ASTContextCreateApplicationType(context, binary->base.base.location, callee,
                                                                                            NULL, result);
        binary->base.symbol->kind  = SymbolKindType;
        binary->base.symbol->type  = (ASTTypeRef)type;
        return;
    }

    case ASTTagIdentifierExpression: {
        ASTIdentifierExpressionRef expression = (ASTIdentifierExpressionRef)node;
        SymbolRef symbol = _NameResolverResolveDeclaration(resolver, expression->base.base.scope, expression->base.base.location,
                                                           expression->name);

        if (!symbol) {
            symbol = _NameResolverResolveEnumerationElement(resolver, ASTContextGetModule(context), expression->name);
        }

        if (symbol) {
            // TODO: Expression symbol should not be overridden, instead we could link the symbols to each other...
            expression->base.symbol = symbol;
        } else {
            expression->base.symbol->kind = SymbolKindType;
            expression->base.symbol->type = (ASTTypeRef)ASTContextGetBuiltinType(context, ASTBuiltinTypeKindError);
            ReportError("Unresolved identifier");
        }
        return;
    }

    case ASTTagMemberAccessExpression: {
        ASTMemberAccessExpressionRef expression = (ASTMemberAccessExpressionRef)node;
        NameResolverResolve(resolver, context, (ASTNodeRef)expression->argument);
        // TODO: Check if expression is structure declaration and resolve declaration of member
        return;
    }

    case ASTTagCallExpression: {
        ASTCallExpressionRef call = (ASTCallExpressionRef)node;
        NameResolverResolve(resolver, context, (ASTNodeRef)call->callee);
        if (call->arguments) {
            NameResolverResolve(resolver, context, (ASTNodeRef)call->arguments);
        }

        // TODO: Check if callee is function declaration
        SymbolRef result        = ScopeInsertUniqueSymbol(call->base.base.scope, call->base.base.location);
        call->base.symbol->kind = SymbolKindType;
        call->base.symbol->type = (ASTTypeRef)ASTContextCreateApplicationType(context, call->base.base.location, call->callee->symbol,
                                                                              call->arguments, result);
        return;
    }

    case ASTTagModuleDeclaration: {
        ASTModuleDeclarationRef module = (ASTModuleDeclarationRef)node;
        if (module->sourceUnits) {
            NameResolverResolve(resolver, context, (ASTNodeRef)module->sourceUnits);
        }
        return;
    }

    case ASTTagEnumerationDeclaration: {
        ASTEnumerationDeclarationRef enumeration = (ASTEnumerationDeclarationRef)node;
        NameResolverResolve(resolver, context, (ASTNodeRef)enumeration->elements);
        return;
    }

    case ASTTagFunctionDeclaration: {
        ASTFunctionDeclarationRef func = (ASTFunctionDeclarationRef)node;
        if (func->parameters) {
            NameResolverResolve(resolver, context, (ASTNodeRef)func->parameters);
        }

        NameResolverResolve(resolver, context, (ASTNodeRef)func->returnType);
        NameResolverResolve(resolver, context, (ASTNodeRef)func->body);
        return;
    }

    case ASTTagStructureDeclaration: {
        ASTStructureDeclarationRef structure = (ASTStructureDeclarationRef)node;
        if (structure->values) {
            NameResolverResolve(resolver, context, (ASTNodeRef)structure->values);
        }
        return;
    }

    case ASTTagValueDeclaration: {
        ASTValueDeclarationRef value = (ASTValueDeclarationRef)node;
        if (value->kind == ASTValueKindVariable || value->kind == ASTValueKindParameter || value->kind == ASTValueKindEnumerationElement) {
            if (value->initializer) {
                NameResolverResolve(resolver, context, (ASTNodeRef)value->initializer);
            }
            NameResolverResolve(resolver, context, value->type);
        } else {
            JELLY_UNREACHABLE("Unknown kind given for ASTValueDeclaration in NameResolver!");
        }
        return;
    }

    case ASTTagPointerType: {
        ASTPointerTypeRef pointerType = (ASTPointerTypeRef)node;
        NameResolverResolve(resolver, context, (ASTNodeRef)pointerType->pointeeType);
        return;
    }

    case ASTTagArrayType: {
        ASTArrayTypeRef arrayType = (ASTArrayTypeRef)node;
        if (arrayType->size) {
            NameResolverResolve(resolver, context, (ASTNodeRef)arrayType->size);
        }
        NameResolverResolve(resolver, context, (ASTNodeRef)arrayType->elementType);
        return;
    }

    case ASTTagOpaqueType: {
        ASTOpaqueTypeRef opaque = (ASTOpaqueTypeRef)node;
        SymbolRef symbol        = NULL;
        ScopeRef scope          = opaque->base.scope;
        while (scope) {
            symbol = ScopeLookupSymbol(scope, opaque->name, opaque->base.location.start);
            if (symbol) {
                ASTNodeRef node = symbol->node;
                assert(node);

                if (node->tag == ASTTagStructureDeclaration || node->tag == ASTTagEnumerationDeclaration) {
                    break;
                } else {
                    symbol = NULL;
                }
            }

            scope = ScopeGetParent(scope);
        }

        if (symbol) {
            opaque->declaration = (ASTDeclarationRef)symbol->node;
            assert(opaque->declaration);
        } else {
            ReportError("Use of undeclared type");
        }
        return;
    }

    case ASTTagLoadDirective:
    case ASTTagConstantExpression:
    case ASTTagOpaqueDeclaration:
    case ASTTagBuiltinType:
        return;

    default:
        JELLY_UNREACHABLE("Unknown tag given for ASTNode in NameResolver!");
    }
}

SymbolRef _NameResolverResolveDeclaration(NameResolverRef resolver, ScopeRef scope, SourceRange location, StringRef name) {
    SymbolRef symbol = NULL;
    while (scope) {
        symbol = ScopeLookupSymbol(scope, name, location.start);
        if (symbol) {
            ASTNodeRef node = symbol->node;
            assert(node);
            Bool isDeclaration = (node->tag == ASTTagValueDeclaration || node->tag == ASTTagModuleDeclaration ||
                                  node->tag == ASTTagOpaqueDeclaration || node->tag == ASTTagFunctionDeclaration ||
                                  node->tag == ASTTagStructureDeclaration || node->tag == ASTTagEnumerationDeclaration);
            if (isDeclaration) {
                break;
            } else {
                symbol = NULL;
            }
        }

        scope = ScopeGetParent(scope);
    }

    return symbol;
}

SymbolRef _NameResolverResolveEnumerationElement(NameResolverRef resolver, ASTModuleDeclarationRef module, StringRef name) {
    ScopeRef scope   = module->base.scope;
    SymbolRef result = NULL;

    for (Index index = 0; index < ScopeGetChildCount(scope); index++) {
        ScopeRef child = ScopeGetChildAtIndex(scope, index);
        if (ScopeGetKind(child) == ScopeKindEnumeration) {
            SymbolRef symbol = ScopeLookupSymbol(child, name, NULL);
            if (symbol) {
                ASTNodeRef node = symbol->node;
                assert(node);
                assert(node->tag == ASTTagValueDeclaration);

                if (result) {
                    // TODO: Remove after implementing @CandidateDeclarations
                    ReportCritical("Ambigous enumeration cases are not supported yet!");
                    return result;
                }

                result = symbol;
            }
        }
    }

    return result;
}
