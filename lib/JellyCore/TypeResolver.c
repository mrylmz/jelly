#include "JellyCore/Diagnostic.h"
#include "JellyCore/TypeResolver.h"

// TODO: @CandidateDeclarations Add support for resolution of candidate declarations to allow contextual resolutions
// TODO: Resolve control flow paths for loop, control, ... statements
// TODO: May rename to ASTResolver if it resolves more than just identifiers?

struct _TypeResolver {
    AllocatorRef allocator;
};

static inline void _TypeResolverResolveSymbol(TypeResolverRef resolver, ASTContextRef context, ScopeRef scope, SymbolRef symbol);
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

void TypeResolverResolve(TypeResolverRef resolver, ASTContextRef context, ScopeRef scope) {
    for (Index index = ScopeGetSymbolCount(scope); index > 0; index--) {
        SymbolRef symbol = ScopeGetSymbolAtIndex(scope, index - 1);
        _TypeResolverResolveSymbol(resolver, context, scope, symbol);
    }

    for (Index index = ScopeGetChildCount(scope); index > 0; index--) {
        ScopeRef child = ScopeGetChildAtIndex(scope, index - 1);
        TypeResolverResolve(resolver, context, child);
    }
}

static inline void _TypeResolverResolveSymbol(TypeResolverRef resolver, ASTContextRef context, ScopeRef scope, SymbolRef symbol) {
    if (symbol->kind != SymbolKindType) {
        return;
    }

    switch (symbol->type->tag) {
    case ASTTagOpaqueType: {
        ASTOpaqueTypeRef type = (ASTOpaqueTypeRef)symbol->type;
        if (!_TypeResolverResolveType(resolver, context, scope, symbol, type->name)) {
            symbol->type = (ASTTypeRef)ASTContextGetBuiltinType(context, ASTBuiltinTypeKindError);
            ReportError("Unresolved identifier");
        }
        return;
    }

    case ASTTagPointerType: {
        ASTPointerTypeRef type = (ASTPointerTypeRef)symbol->type;
        // TODO: Resolve pointee ???
        return;
    }

    case ASTTagArrayType: {
        ASTArrayTypeRef type = (ASTArrayTypeRef)symbol->type;
        // TODO: Resolve element ???
        return;
    }

    case ASTTagBuiltinType: {
        ASTBuiltinTypeRef type = (ASTBuiltinTypeRef)symbol->type;

        return;
    }

    case ASTTagEnumerationType: {
        ASTEnumerationTypeRef type = (ASTEnumerationTypeRef)symbol->type;

        return;
    }

    case ASTTagFunctionType: {
        ASTFunctionTypeRef type = (ASTFunctionTypeRef)symbol->type;

        return;
    }

    case ASTTagStructureType: {
        ASTStructureTypeRef type = (ASTStructureTypeRef)symbol->type;

        return;
    }

    case ASTTagApplicationType: {
        ASTApplicationTypeRef type = (ASTApplicationTypeRef)symbol->type;
        ASTLinkedListRef list      = type->arguments;
        while (list) {
            _TypeResolverResolveSymbol(resolver, context, scope, (SymbolRef)list->node);
            list = list->next;
        }
        _TypeResolverResolveSymbol(resolver, context, scope, type->callee);
        _TypeResolverResolveSymbol(resolver, context, scope, type->result);
        return;
    }

    default:
        JELLY_UNREACHABLE("Unknown tag given for ASTNode in TypeResolver!");
        break;
    }
}

static inline Bool _TypeResolverResolveType(TypeResolverRef resolver, ASTContextRef context, ScopeRef scope, SymbolRef symbol,
                                            StringRef name) {
    while (scope) {
        SymbolRef child = ScopeLookupSymbol(scope, name, symbol->location.start);
        while (child && child->kind == SymbolKindLink) {
            child = child->link;
        }

        if (child && child->kind == SymbolKindType) {
            symbol->kind = SymbolKindType;
            symbol->type = child->type;
            return true;
        }

        scope = ScopeGetParent(scope);
    }

    return false;
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
