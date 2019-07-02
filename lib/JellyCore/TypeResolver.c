#include "JellyCore/ASTFunctions.h"
#include "JellyCore/Diagnostic.h"
#include "JellyCore/TypeResolver.h"

// TODO: @CandidateDeclarations Add support for resolution of candidate declarations to allow contextual resolutions
// TODO: Resolve control flow paths for loop, control, ... statements
// TODO: May rename to ASTResolver if it resolves more than just identifiers?

struct _TypeResolver {
    AllocatorRef allocator;
};

static inline void _TypeResolverResolveNode(TypeResolverRef resolver, ASTContextRef context, ASTNodeRef parent, ASTNodeRef node);

static inline ASTDeclarationRef _TypeResolverResolveDeclaration(TypeResolverRef resolver, ASTContextRef context, ScopeRef scope,
                                                                SourceRange location, StringRef name);

static inline void _TypeResolverResolveType(TypeResolverRef resolver, ASTContextRef context, ScopeRef scope, ASTTypeRef *type);

TypeResolverRef TypeResolverCreate(AllocatorRef allocator) {
    TypeResolverRef resolver = AllocatorAllocate(allocator, sizeof(struct _TypeResolver));
    resolver->allocator      = allocator;
    return resolver;
}

void TypeResolverDestroy(TypeResolverRef resolver) {
    AllocatorDeallocate(resolver->allocator, resolver);
}

void TypeResolverResolve(TypeResolverRef resolver, ASTContextRef context, ASTNodeRef node) {
    _TypeResolverResolveNode(resolver, context, NULL, node);
}

static inline void _TypeResolverResolveNode(TypeResolverRef resolver, ASTContextRef context, ASTNodeRef parent, ASTNodeRef node) {
    ScopeRef previousScope = SymbolTableGetCurrentScope(ASTContextGetSymbolTable(context));
    SymbolTableSetCurrentScope(ASTContextGetSymbolTable(context), node->scope);

    switch (node->tag) {
    case ASTTagSourceUnit: {
        ASTSourceUnitRef sourceUnit = (ASTSourceUnitRef)node;
        _TypeResolverResolveNode(resolver, context, node, (ASTNodeRef)sourceUnit->declarations);
        break;
    }

    case ASTTagLinkedList: {
        ASTLinkedListRef list = (ASTLinkedListRef)node;
        while (list) {
            // NOTE: We pass the parent of the list here because the list it self is a redundant information...
            _TypeResolverResolveNode(resolver, context, parent, (ASTNodeRef)list->node);
            list = list->next;
        }
        break;
    }

    case ASTTagArray: {
        ASTArrayRef array = (ASTArrayRef)node;
        for (Index index = 0; index < ASTArrayGetElementCount(array); index++) {
            // NOTE: We pass the parent of the array here because the array it self is a redundant information...
            _TypeResolverResolveNode(resolver, context, parent, (ASTNodeRef)ASTArrayGetElementAtIndex(array, index));
        }
        break;
    }

    case ASTTagLoadDirective: {
        ASTLoadDirectiveRef load = (ASTLoadDirectiveRef)node;
        _TypeResolverResolveNode(resolver, context, node, (ASTNodeRef)load->filePath);
        break;
    }

    case ASTTagBlock: {
        ASTBlockRef block = (ASTBlockRef)node;
        _TypeResolverResolveNode(resolver, context, node, (ASTNodeRef)block->statements);
        break;
    }

    case ASTTagIfStatement: {
        ASTIfStatementRef statement = (ASTIfStatementRef)node;
        _TypeResolverResolveNode(resolver, context, node, (ASTNodeRef)statement->condition);
        _TypeResolverResolveNode(resolver, context, node, (ASTNodeRef)statement->thenBlock);
        _TypeResolverResolveNode(resolver, context, node, (ASTNodeRef)statement->elseBlock);
        break;
    }

    case ASTTagLoopStatement: {
        ASTLoopStatementRef loop = (ASTLoopStatementRef)node;
        _TypeResolverResolveNode(resolver, context, node, (ASTNodeRef)loop->condition);
        _TypeResolverResolveNode(resolver, context, node, (ASTNodeRef)loop->loopBlock);
        break;
    }

    case ASTTagCaseStatement: {
        ASTCaseStatementRef statement = (ASTCaseStatementRef)node;
        if (statement->kind == ASTCaseKindConditional) {
            _TypeResolverResolveNode(resolver, context, node, (ASTNodeRef)statement->condition);
        }
        _TypeResolverResolveNode(resolver, context, node, (ASTNodeRef)statement->body);
        break;
    }

    case ASTTagSwitchStatement: {
        ASTSwitchStatementRef statement = (ASTSwitchStatementRef)node;
        _TypeResolverResolveNode(resolver, context, node, (ASTNodeRef)statement->argument);
        if (statement->cases) {
            _TypeResolverResolveNode(resolver, context, node, (ASTNodeRef)statement->cases);
        }
        break;
    }

    case ASTTagControlStatement: {
        ASTControlStatementRef control = (ASTControlStatementRef)node;
        if (control->kind == ASTControlKindReturn && control->result) {
            _TypeResolverResolveNode(resolver, context, node, (ASTNodeRef)control->result);
        }
        break;
    }

    case ASTTagUnaryExpression: {
        // TODO: Add builtin unary operations and add support for function overloading
        ASTUnaryExpressionRef unary = (ASTUnaryExpressionRef)node;
        _TypeResolverResolveNode(resolver, context, node, (ASTNodeRef)unary->arguments[0]);

        StringRef name       = ASTGetPrefixOperatorName(resolver->allocator, unary->op);
        ScopeRef globalScope = SymbolTableGetGlobalScope(ASTContextGetSymbolTable(context));
        SymbolRef symbol     = ScopeLookupSymbol(globalScope, name, NULL);
        if (symbol && symbol->declaration->tag == ASTTagFunctionDeclaration) {
            ASTFunctionDeclarationRef declaration = (ASTFunctionDeclarationRef)symbol->declaration;
            if (declaration->fixity == ASTFixityPrefix) {
                unary->base.type = declaration->type;
            }
        }

        if (!unary->base.type) {
            ReportError("Use of undeclared prefix operator");
            unary->base.type = (ASTTypeRef)ASTContextGetBuiltinType(context, ASTBuiltinTypeKindError);
        }

        StringDestroy(name);
        break;
    }

    case ASTTagBinaryExpression: {
        // TODO: Add builtin binary operations and add support for function overloading
        ASTBinaryExpressionRef binary = (ASTBinaryExpressionRef)node;
        _TypeResolverResolveNode(resolver, context, node, (ASTNodeRef)binary->arguments[0]);
        _TypeResolverResolveNode(resolver, context, node, (ASTNodeRef)binary->arguments[1]);

        StringRef name       = ASTGetInfixOperatorName(resolver->allocator, binary->op);
        ScopeRef globalScope = SymbolTableGetGlobalScope(ASTContextGetSymbolTable(context));
        SymbolRef symbol     = ScopeLookupSymbol(globalScope, name, NULL);
        if (symbol && symbol->declaration->tag == ASTTagFunctionDeclaration) {
            ASTFunctionDeclarationRef declaration = (ASTFunctionDeclarationRef)symbol->declaration;
            if (declaration->fixity == ASTFixityInfix) {
                binary->base.type = declaration->type;
            }
        }

        if (!binary->base.type) {
            ReportError("Use of undeclared infix operator");
            binary->base.type = (ASTTypeRef)ASTContextGetBuiltinType(context, ASTBuiltinTypeKindError);
        }

        StringDestroy(name);
        break;
    }

    case ASTTagIdentifierExpression: {
        ASTIdentifierExpressionRef expression = (ASTIdentifierExpressionRef)node;
        ScopeRef scope                        = node->scope;
        while (scope) {
            SymbolRef symbol = ScopeLookupSymbol(scope, expression->name, node->location.start);
            if (symbol) {
                switch (symbol->declaration->tag) {
                case ASTTagEnumerationDeclaration: {
                    ASTEnumerationDeclarationRef declaration = (ASTEnumerationDeclarationRef)symbol->declaration;
                    expression->base.type                    = declaration->type;
                    break;
                }
                case ASTTagFunctionDeclaration: {
                    ASTFunctionDeclarationRef declaration = (ASTFunctionDeclarationRef)symbol->declaration;
                    expression->base.type                 = declaration->type;
                    break;
                }
                case ASTTagStructureDeclaration: {
                    ASTStructureDeclarationRef declaration = (ASTStructureDeclarationRef)symbol->declaration;
                    expression->base.type                  = declaration->type;
                    break;
                }
                case ASTTagValueDeclaration: {
                    ASTValueDeclarationRef declaration = (ASTValueDeclarationRef)symbol->declaration;
                    expression->base.type              = declaration->type;
                    break;
                }

                default:
                    JELLY_UNREACHABLE("Unknown tag given for ASTDeclaration in TypeResolver!");
                    break;
                }

                expression->resolvedDeclaration = symbol->declaration;
            }

            scope = ScopeGetParent(scope);
        }

        if (expression->base.type == NULL) {
            expression->base.type = (ASTTypeRef)ASTContextGetBuiltinType(context, ASTBuiltinTypeKindError);
            ReportError("Use of undeclared identifier");
        }

        break;
    }

    case ASTTagMemberAccessExpression: {
        ASTMemberAccessExpressionRef expression = (ASTMemberAccessExpressionRef)node;
        _TypeResolverResolveNode(resolver, context, node, (ASTNodeRef)expression->argument);

        assert(expression->argument->type);
        if (expression->argument->type->tag == ASTTagStructureType) {
            ASTStructureTypeRef structType = (ASTStructureTypeRef)expression->argument->type;
            ScopeRef scope                 = structType->declaration->innerScope;
            SymbolRef symbol               = ScopeLookupSymbol(scope, expression->memberName, NULL);
            if (symbol) {
                switch (symbol->declaration->tag) {
                case ASTTagEnumerationDeclaration: {
                    ASTEnumerationDeclarationRef declaration = (ASTEnumerationDeclarationRef)symbol->declaration;
                    expression->base.type                    = declaration->type;
                    break;
                }
                case ASTTagFunctionDeclaration: {
                    ASTFunctionDeclarationRef declaration = (ASTFunctionDeclarationRef)symbol->declaration;
                    expression->base.type                 = declaration->type;
                    break;
                }
                case ASTTagStructureDeclaration: {
                    ASTStructureDeclarationRef declaration = (ASTStructureDeclarationRef)symbol->declaration;
                    expression->base.type                  = declaration->type;
                    break;
                }
                case ASTTagValueDeclaration: {
                    ASTValueDeclarationRef declaration = (ASTValueDeclarationRef)symbol->declaration;
                    expression->base.type              = declaration->type;
                    break;
                }

                default:
                    JELLY_UNREACHABLE("Unknown tag given for ASTDeclaration in TypeResolver!");
                    break;
                }

                expression->resolvedDeclaration = symbol->declaration;
            } else {
                expression->base.type = (ASTTypeRef)ASTContextGetBuiltinType(context, ASTBuiltinTypeKindError);
                ReportError("Use of undeclared member");
            }
        } else {
            expression->base.type = (ASTTypeRef)ASTContextGetBuiltinType(context, ASTBuiltinTypeKindError);

            if (expression->argument->type->tag != ASTTagBuiltinType &&
                ((ASTBuiltinTypeRef)expression->argument->type)->kind != ASTBuiltinTypeKindError) {
                ReportError("Cannot access named member of non structure type");
            }
        }

        break;
    }

    case ASTTagCallExpression: {
        ASTCallExpressionRef expression = (ASTCallExpressionRef)node;
        _TypeResolverResolveNode(resolver, context, node, (ASTNodeRef)expression->callee);
        _TypeResolverResolveNode(resolver, context, node, (ASTNodeRef)expression->arguments);

        assert(expression->callee->type);
        if (expression->callee->type->tag == ASTTagFunctionType) {
            ASTFunctionTypeRef funcType           = (ASTFunctionTypeRef)expression->callee->type;
            ASTFunctionDeclarationRef declaration = funcType->declaration;
            if (ASTArrayGetElementCount(declaration->parameters) == ASTArrayGetElementCount(expression->arguments)) {
                // TODO: Add type comparison as soon as overloading is supported
            } else {
                expression->base.type = (ASTTypeRef)ASTContextGetBuiltinType(context, ASTBuiltinTypeKindError);
                ReportError("Argument count mismatch");
            }
        } else {
            expression->base.type = (ASTTypeRef)ASTContextGetBuiltinType(context, ASTBuiltinTypeKindError);
            ReportError("Cannot call non function type");
        }

        break;
    }

    case ASTTagConstantExpression: { // TODO: @Next
        ASTConstantExpressionRef constant = (ASTConstantExpressionRef)node;
        if (constant->kind == ASTConstantKindNil) {
            constant->base.type = (ASTTypeRef)ASTContextCreatePointerType(
                context, SourceRangeNull(), (ASTTypeRef)ASTContextGetBuiltinType(context, ASTBuiltinTypeKindVoid));
        } else if (constant->kind == ASTConstantKindBool) {
            constant->base.type = (ASTTypeRef)ASTContextGetBuiltinType(context, ASTBuiltinTypeKindBool);
        } else if (constant->kind == ASTConstantKindInt) {
            constant->base.type = (ASTTypeRef)ASTContextGetBuiltinType(context, ASTBuiltinTypeKindInt);
        } else if (constant->kind == ASTConstantKindFloat) {
            constant->base.type = (ASTTypeRef)ASTContextGetBuiltinType(context, ASTBuiltinTypeKindFloat);
        } else if (constant->kind == ASTConstantKindString) {
            JELLY_UNREACHABLE("Implementation missing!");
        } else {
            JELLY_UNREACHABLE("Unknown kind given for ASTConstantExpression in Typer!");
        }
        break;
    }

    case ASTTagModuleDeclaration: {
        ASTModuleDeclarationRef module = (ASTModuleDeclarationRef)node;
        _TypeResolverResolveNode(resolver, context, node, (ASTNodeRef)module->sourceUnits);
        break;
    }

    case ASTTagEnumerationDeclaration: {
        ASTEnumerationDeclarationRef declaration = (ASTEnumerationDeclarationRef)node;
        _TypeResolverResolveNode(resolver, context, node, (ASTNodeRef)declaration->elements);
        break;
    }

    case ASTTagFunctionDeclaration: {
        ASTFunctionDeclarationRef declaration = (ASTFunctionDeclarationRef)node;
        if (declaration->parameters) {
            _TypeResolverResolveNode(resolver, context, node, (ASTNodeRef)declaration->parameters);
        }
        _TypeResolverResolveType(resolver, context, node->scope, &declaration->returnType);
        _TypeResolverResolveNode(resolver, context, node, (ASTNodeRef)declaration->body);
        break;
    }

    case ASTTagStructureDeclaration: {
        ASTStructureDeclarationRef declaration = (ASTStructureDeclarationRef)node;
        _TypeResolverResolveNode(resolver, context, node, (ASTNodeRef)declaration->values);
        break;
    }

    case ASTTagOpaqueDeclaration:
        break;

    case ASTTagValueDeclaration: {
        ASTValueDeclarationRef declaration = (ASTValueDeclarationRef)node;
        _TypeResolverResolveType(resolver, context, node->scope, &declaration->type);
        if (declaration->initializer) {
            _TypeResolverResolveNode(resolver, context, node, (ASTNodeRef)declaration->initializer);
        }
        break;
    }

    default:
        JELLY_UNREACHABLE("Unknown tag given for ASTNode in Typer!");
        break;
    }

    SymbolTableSetCurrentScope(ASTContextGetSymbolTable(context), previousScope);
}

static inline ASTDeclarationRef _TypeResolverResolveDeclaration(TypeResolverRef resolver, ASTContextRef context, ScopeRef scope,
                                                                SourceRange location, StringRef name) {
    while (scope) {
        SymbolRef symbol = ScopeLookupSymbol(scope, name, location.start);
        if (symbol) {
            switch (symbol->declaration->tag) {
            case ASTTagFunctionDeclaration:
            case ASTTagStructureDeclaration:
            case ASTTagOpaqueDeclaration:
            case ASTTagValueDeclaration:
                return symbol->declaration;

            default:
                break;
            }

            return symbol->declaration;
        }

        scope = ScopeGetParent(scope);
    }

    return NULL;
}

static inline void _TypeResolverResolveType(TypeResolverRef resolver, ASTContextRef context, ScopeRef scope, ASTTypeRef *type) {
    switch ((*type)->tag) {
    case ASTTagPointerType: {
        ASTPointerTypeRef pointerType = (ASTPointerTypeRef)(*type);
        _TypeResolverResolveType(resolver, context, scope, &pointerType->pointeeType);
        break;
    }

    case ASTTagArrayType: {
        ASTArrayTypeRef arrayType = (ASTArrayTypeRef)(*type);
        _TypeResolverResolveType(resolver, context, scope, &arrayType->elementType);

        if (arrayType->size) {
            _TypeResolverResolveNode(resolver, context, (ASTNodeRef)arrayType, (ASTNodeRef)arrayType->size);
        }
        break;
    }

    case ASTTagOpaqueType: {
        ASTOpaqueTypeRef opaqueType = (ASTOpaqueTypeRef)(*type);
        ScopeRef globalScope        = SymbolTableGetGlobalScope(ASTContextGetSymbolTable(context));
        SymbolRef symbol            = ScopeLookupSymbol(globalScope, opaqueType->name, NULL);
        if (symbol) {
            switch (symbol->declaration->tag) {
            case ASTTagEnumerationDeclaration: {
                ASTEnumerationDeclarationRef declaration = (ASTEnumerationDeclarationRef)symbol->declaration;
                *type                                    = declaration->type;
                break;
            }
            case ASTTagFunctionDeclaration: {
                ASTFunctionDeclarationRef declaration = (ASTFunctionDeclarationRef)symbol->declaration;
                *type                                 = declaration->type;
                break;
            }
            case ASTTagStructureDeclaration: {
                ASTStructureDeclarationRef declaration = (ASTStructureDeclarationRef)symbol->declaration;
                *type                                  = declaration->type;
                break;
            }
            case ASTTagValueDeclaration: {
                ASTValueDeclarationRef declaration = (ASTValueDeclarationRef)symbol->declaration;
                *type                              = declaration->type;
                break;
            }

            default:
                JELLY_UNREACHABLE("Unknown tag given for ASTDeclaration in TypeResolver!");
                break;
            }
        } else {
            *type = (ASTTypeRef)ASTContextGetBuiltinType(context, ASTBuiltinTypeKindError);
            ReportError("Use of unresolved type");
        }
        break;
    }

    case ASTTagBuiltinType:
    case ASTTagEnumerationType:
    case ASTTagStructureType:
        break;

    default:
        JELLY_UNREACHABLE("Unknown tag given for ASTType in Typer!");
        break;
    }
}

SymbolRef _TypeResolverResolveEnumerationElement(TypeResolverRef resolver, ASTModuleDeclarationRef module, StringRef name) {
    ScopeRef scope   = module->base.scope;
    SymbolRef result = NULL;

    for (Index index = 0; index < ScopeGetChildCount(scope); index++) {
        ScopeRef child = ScopeGetChildAtIndex(scope, index);
        if (ScopeGetKind(child) == ScopeKindEnumeration) {
            SymbolRef symbol = ScopeLookupSymbol(child, name, NULL);
            if (symbol) {
                ASTDeclarationRef declaration = symbol->declaration;
                assert(declaration);
                assert(declaration->tag == ASTTagValueDeclaration);

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
