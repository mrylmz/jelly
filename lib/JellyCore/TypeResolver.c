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

static inline Bool _TypeResolverResolveType(TypeResolverRef resolver, ASTContextRef context, ScopeRef scope, SymbolRef symbol,
                                            StringRef name);

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

    case ASTTagLookupList:
        break;

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
            // TODO: Add constraint that control->result has to be equal to enclosing func->result!
        }
        break;
    }

    case ASTTagUnaryExpression: {
        ASTUnaryExpressionRef unary = (ASTUnaryExpressionRef)node;
        _TypeResolverResolveNode(resolver, context, node, (ASTNodeRef)unary->arguments[0]);
        break;
    }

    case ASTTagBinaryExpression: {
        ASTBinaryExpressionRef binary = (ASTBinaryExpressionRef)node;
        _TypeResolverResolveNode(resolver, context, node, (ASTNodeRef)binary->arguments[0]);
        _TypeResolverResolveNode(resolver, context, node, (ASTNodeRef)binary->arguments[1]);
        break;
    }

    case ASTTagIdentifierExpression: {
        ASTIdentifierExpressionRef identifier = (ASTIdentifierExpressionRef)node;
        ASTLookupListRef lookup               = identifier->base.lookup;
        while (lookup) {
            if (ASTArrayGetElementCount(lookup->candidates) == 1) {
                identifier->resolvedDeclaration = (ASTDeclarationRef)ASTArrayGetElementAtIndex(lookup->candidates, 0);
            } else if (ASTArrayGetElementCount(lookup->candidates) > 1) {
                ReportError("Ambigous use of identifier");
            }

            lookup = lookup->next;
        }

        if (!identifier->resolvedDeclaration) {
            ReportError("Unresolved identifier");
        }
        break;
    }

    case ASTTagMemberAccessExpression: {
        ASTMemberAccessExpressionRef expression = (ASTMemberAccessExpressionRef)node;
        // TODO: If there is no good fit as candidate then resolve to the next best for better error reports in type checker
        ASTLookupListRef lookup = expression->argument->lookup;
        while (lookup) {
            for (Index index = 0; index < ASTArrayGetElementCount(lookup->candidates); index++) {
                ASTDeclarationRef candidate = ASTArrayGetElementAtIndex(lookup->candidates, index);
                Bool remove                 = false;
                if (candidate->tag == ASTTagStructureDeclaration) {
                    ASTStructureDeclarationRef structure = (ASTStructureDeclarationRef)candidate;
                    for (Index index = 0; index < ASTArrayGetElementCount(structure->values); index++) {
                        ASTNodeRef child = ASTArrayGetElementAtIndex(structure->values, index);
                        assert(child->tag == ASTTagValueDeclaration);

                        ASTValueDeclarationRef value = (ASTValueDeclarationRef)child;
                        assert(value->base.tag == ASTValueKindVariable);

                        if (StringIsEqual(value->name, expression->memberName)) {
                            break;
                        }
                    }

                } else {
                    remove = true;
                }

                if (remove) {
                    ASTArrayRemoveElementAtIndex(lookup->candidates, index);
                    index -= 1;
                }
            }

            lookup = lookup->next;
        }

        _TypeResolverResolveNode(resolver, context, node, (ASTNodeRef)expression->argument);
        break;
    }

    case ASTTagCallExpression: {
        ASTCallExpressionRef call = (ASTCallExpressionRef)node;
        // TODO: @CandidateFilter
        //
        //  (1): call.callee == func
        //  (2): call.callee.parameters.count == call.arguments.count
        //  (3): call.callee.parameters[i].declaration == call.arguments[i].declaration

        _TypeResolverResolveNode(resolver, context, node, (ASTNodeRef)call->callee);

        if (call->arguments) {
            _TypeResolverResolveNode(resolver, context, node, (ASTNodeRef)call->arguments);
        }

        break;
    }

    case ASTTagConstantExpression: {
        ASTConstantExpressionRef constant = (ASTConstantExpressionRef)node;

        if (constant->kind == ASTConstantKindNil) {
        } else if (constant->kind == ASTConstantKindBool) {
        } else if (constant->kind == ASTConstantKindInt) {
        } else if (constant->kind == ASTConstantKindFloat) {
        } else if (constant->kind == ASTConstantKindString) {
        } else {
            JELLY_UNREACHABLE("Unknown kind given for ASTConstantExpression in Typer!");
        }
        break;
    }

    case ASTTagModuleDeclaration: {
        ASTModuleDeclarationRef module = (ASTModuleDeclarationRef)node;
        if (module->sourceUnits) {
            _TypeResolverResolveNode(resolver, context, node, (ASTNodeRef)module->sourceUnits);
        }
        break;
    }

    case ASTTagEnumerationDeclaration: {
        ASTEnumerationDeclarationRef declaration = (ASTEnumerationDeclarationRef)node;
        if (declaration->elements) {
            _TypeResolverResolveNode(resolver, context, node, (ASTNodeRef)declaration->elements);
        }
        break;
    }

    case ASTTagFunctionDeclaration: {
        ASTFunctionDeclarationRef declaration = (ASTFunctionDeclarationRef)node;
        if (declaration->parameters) {
            _TypeResolverResolveNode(resolver, context, node, (ASTNodeRef)declaration->parameters);
        }
        _TypeResolverResolveNode(resolver, context, node, (ASTNodeRef)declaration->returnType);
        _TypeResolverResolveNode(resolver, context, node, (ASTNodeRef)declaration->body);
        break;
    }

    case ASTTagStructureDeclaration: {
        ASTStructureDeclarationRef declaration = (ASTStructureDeclarationRef)node;
        if (declaration->values) {
            _TypeResolverResolveNode(resolver, context, node, (ASTNodeRef)declaration->values);
        }
        break;
    }

    case ASTTagOpaqueDeclaration:
        break;

    case ASTTagValueDeclaration: {
        ASTValueDeclarationRef declaration = (ASTValueDeclarationRef)node;
        _TypeResolverResolveNode(resolver, context, node, (ASTNodeRef)declaration->type);
        if (declaration->initializer) {
            _TypeResolverResolveNode(resolver, context, node, (ASTNodeRef)declaration->initializer);
        }
        break;
    }

    case ASTTagPointerType: {
        ASTPointerTypeRef type = (ASTPointerTypeRef)node;
        _TypeResolverResolveNode(resolver, context, node, (ASTNodeRef)type->pointeeType);
        break;
    }

    case ASTTagArrayType: {
        ASTArrayTypeRef type = (ASTArrayTypeRef)node;
        _TypeResolverResolveNode(resolver, context, node, (ASTNodeRef)type->elementType);
        if (type->size) {
            _TypeResolverResolveNode(resolver, context, node, (ASTNodeRef)type->size);
        }
        break;
    }

    case ASTTagOpaqueType: {
        ASTOpaqueTypeRef type = (ASTOpaqueTypeRef)node;
        // What kind of declaration are we looking here for?
        break;
    }

    case ASTTagBuiltinType:
    case ASTTagEnumerationType:
    case ASTTagFunctionType:
    case ASTTagStructureType:
    case ASTTagApplicationType:
        break;

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
            switch (symbol->node->tag) {
            case ASTTagFunctionDeclaration:
            case ASTTagStructureDeclaration:
            case ASTTagOpaqueDeclaration:
            case ASTTagValueDeclaration:
                return symbol->node;

            default:
                break;
            }

            return symbol->node;
        }

        scope = ScopeGetParent(scope);
    }

    return NULL;
}

static inline void _TypeResolverResolveSymbol(TypeResolverRef resolver, ASTContextRef context, ScopeRef scope, SymbolRef symbol) {
}

static inline Bool _TypeResolverResolveType(TypeResolverRef resolver, ASTContextRef context, ScopeRef scope, SymbolRef symbol,
                                            StringRef name) {
    //    while (scope) {
    //        SymbolRef child = ScopeLookupSymbol(scope, name, symbol->location.start);
    //        while (child && child->kind == SymbolKindLink) {
    //            child = child->link;
    //        }
    //
    //        if (child && child->kind == SymbolKindType) {
    //            symbol->kind = SymbolKindType;
    //            symbol->type = child->type;
    //            return true;
    //        }
    //
    //        scope = ScopeGetParent(scope);
    //    }
    //
    //    symbol->kind = SymbolKindType;
    //    symbol->type = (ASTTypeRef)ASTContextGetBuiltinType(context, ASTBuiltinTypeKindError);
    //    return false;
}

SymbolRef _TypeResolverResolveEnumerationElement(TypeResolverRef resolver, ASTModuleDeclarationRef module, StringRef name) {
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
