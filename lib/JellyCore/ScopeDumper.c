#include "JellyCore/ASTNodes.h"
#include "JellyCore/ScopeDumper.h"

struct _ScopeDumper {
    AllocatorRef allocator;
    FILE *target;
    Index indentation;
};

static inline void _ScopeDumperPrintIndentation(ScopeDumperRef dumper);
static inline void _ScopeDumperPrintCString(ScopeDumperRef dumper, const Char *string);
static inline void _ScopeDumperPrintScopeKind(ScopeDumperRef dumper, ScopeKind kind);
static inline void _ScopeDumperPrintSymbol(ScopeDumperRef dumper, SymbolRef symbol);
static inline void _ScopeDumperPrintType(ScopeDumperRef dumper, ASTTypeRef type);

ScopeDumperRef ScopeDumperCreate(AllocatorRef allocator, FILE *target) {
    ScopeDumperRef dumper = AllocatorAllocate(allocator, sizeof(struct _ScopeDumper));
    dumper->allocator     = allocator;
    dumper->target        = target;
    dumper->indentation   = 0;
    return dumper;
}

void ScopeDumperDestroy(ScopeDumperRef dumper) {
    AllocatorDeallocate(dumper->allocator, dumper);
}

void ScopeDumperDump(ScopeDumperRef dumper, ScopeRef scope) {
    _ScopeDumperPrintIndentation(dumper);
    _ScopeDumperPrintScopeKind(dumper, ScopeGetKind(scope));
    _ScopeDumperPrintCString(dumper, " {\n");

    dumper->indentation += 1;

    for (Index index = 0; index < ScopeGetSymbolCount(scope); index++) {
        SymbolRef symbol = ScopeGetSymbolAtIndex(scope, index);
        _ScopeDumperPrintIndentation(dumper);
        _ScopeDumperPrintSymbol(dumper, symbol);
        _ScopeDumperPrintCString(dumper, "\n");
    }

    if (ScopeGetSymbolCount(scope) > 0 && ScopeGetChildCount(scope) > 0) {
        _ScopeDumperPrintCString(dumper, "\n");
    }

    for (Index index = 0; index < ScopeGetChildCount(scope); index++) {
        ScopeRef child = ScopeGetChildAtIndex(scope, index);
        ScopeDumperDump(dumper, child);
        if (index + 1 < ScopeGetChildCount(scope)) {
            _ScopeDumperPrintCString(dumper, "\n");
        }
    }

    dumper->indentation -= 1;

    _ScopeDumperPrintIndentation(dumper);
    _ScopeDumperPrintCString(dumper, "}\n");
}

static inline void _ScopeDumperPrintIndentation(ScopeDumperRef dumper) {
    for (Index i = 0; i < dumper->indentation; i++) {
        fprintf(dumper->target, "%s", "  ");
    }
}

static inline void _ScopeDumperPrintCString(ScopeDumperRef dumper, const Char *string) {
    fprintf(dumper->target, "%s", string);
}

static inline void _ScopeDumperPrintScopeKind(ScopeDumperRef dumper, ScopeKind kind) {
    switch (kind) {
    case ScopeKindGlobal:
        return _ScopeDumperPrintCString(dumper, "[Global]");

    case ScopeKindBranch:
        return _ScopeDumperPrintCString(dumper, "[Branch]");

    case ScopeKindLoop:
        return _ScopeDumperPrintCString(dumper, "[Loop]");

    case ScopeKindCase:
        return _ScopeDumperPrintCString(dumper, "[Case]");

    case ScopeKindSwitch:
        return _ScopeDumperPrintCString(dumper, "[Switch]");

    case ScopeKindEnumeration:
        return _ScopeDumperPrintCString(dumper, "[Enumeration]");

    case ScopeKindFunction:
        return _ScopeDumperPrintCString(dumper, "[Function]");

    case ScopeKindStructure:
        return _ScopeDumperPrintCString(dumper, "[Structure]");

    default:
        break;
    }

    JELLY_UNREACHABLE("Unknown kind given for ScopeKind in ScopeDumper!");
}

static inline void _ScopeDumperPrintSymbol(ScopeDumperRef dumper, SymbolRef symbol) {
    _ScopeDumperPrintCString(dumper, StringGetCharacters(symbol->name));

    _ScopeDumperPrintCString(dumper, " = ");
    if (symbol->kind == SymbolKindLink) {
        _ScopeDumperPrintCString(dumper, StringGetCharacters(symbol->link->name));
    } else if (symbol->kind == SymbolKindType) {
        assert(symbol->type);
        _ScopeDumperPrintType(dumper, symbol->type);
    } else {
        _ScopeDumperPrintCString(dumper, "?");
    }
}

static inline void _ScopeDumperPrintType(ScopeDumperRef dumper, ASTTypeRef type) {
    switch (type->tag) {
    case ASTTagOpaqueType: {
        ASTOpaqueTypeRef opaque = (ASTOpaqueTypeRef)type;
        _ScopeDumperPrintCString(dumper, StringGetCharacters(opaque->name));
        _ScopeDumperPrintCString(dumper, "?");
        return;
    }

    case ASTTagPointerType: {
        ASTPointerTypeRef pointer = (ASTPointerTypeRef)type;
        _ScopeDumperPrintType(dumper, pointer->pointeeType);
        _ScopeDumperPrintCString(dumper, "*");
        return;
    }

    case ASTTagArrayType: {
        ASTArrayTypeRef array = (ASTArrayTypeRef)type;
        _ScopeDumperPrintType(dumper, array->elementType);
        _ScopeDumperPrintCString(dumper, "[]");
        return;
    }

    case ASTTagBuiltinType: {
        ASTBuiltinTypeRef builtin = (ASTBuiltinTypeRef)type;
        _ScopeDumperPrintCString(dumper, "type ");
        _ScopeDumperPrintCString(dumper, StringGetCharacters(builtin->name));
        return;
    }

    case ASTTagEnumerationType:
        return _ScopeDumperPrintCString(dumper, "enum");

    case ASTTagFunctionType: {
        ASTFunctionTypeRef func = (ASTFunctionTypeRef)type;
        _ScopeDumperPrintCString(dumper, "func (");
        ASTLinkedListRef parameters = func->parameters;
        while (parameters) {
            SymbolRef symbol = (SymbolRef)parameters->node;
            _ScopeDumperPrintCString(dumper, StringGetCharacters(symbol->name));
            parameters = parameters->next;
            if (parameters) {
                _ScopeDumperPrintCString(dumper, ", ");
            }
        }

        _ScopeDumperPrintCString(dumper, ") -> ");
        _ScopeDumperPrintCString(dumper, StringGetCharacters(func->result->name));
        return;
    }

    case ASTTagStructureType: {
        ASTStructureTypeRef structure = (ASTStructureTypeRef)type;
        assert(structure->declaration);
        _ScopeDumperPrintCString(dumper, "struct {\n");
        dumper->indentation += 1;
        ASTLinkedListRef values = structure->declaration->values;
        while (values) {
            ASTValueDeclarationRef value = (ASTValueDeclarationRef)values->node;
            _ScopeDumperPrintIndentation(dumper);
            _ScopeDumperPrintType(dumper, value->type);
            _ScopeDumperPrintCString(dumper, "\n");
            values = values->next;
        }

        dumper->indentation -= 1;
        _ScopeDumperPrintIndentation(dumper);
        _ScopeDumperPrintCString(dumper, "}");
        return;
    }

    case ASTTagApplicationType: {
        ASTApplicationTypeRef application = (ASTApplicationTypeRef)type;
        _ScopeDumperPrintCString(dumper, "apply ");
        _ScopeDumperPrintCString(dumper, StringGetCharacters(application->callee->name));
        _ScopeDumperPrintCString(dumper, " (");
        ASTLinkedListRef arguments = application->arguments;
        while (arguments) {
            SymbolRef symbol = (SymbolRef)arguments->node;
            _ScopeDumperPrintCString(dumper, StringGetCharacters(symbol->name));
            arguments = arguments->next;
            if (arguments) {
                _ScopeDumperPrintCString(dumper, ", ");
            }
        }
        _ScopeDumperPrintCString(dumper, ") -> ");
        _ScopeDumperPrintCString(dumper, StringGetCharacters(application->result->name));
        return;
    }

    default:
        break;
    }

    JELLY_UNREACHABLE("Unknown tag given for ASTTypeRef in ScopeDumper!");
}
