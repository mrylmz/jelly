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
static inline void _ScopeDumperPrintBuiltinType(ScopeDumperRef dumper, ASTBuiltinTypeKind kind);

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
    if (symbol->declaration) {
        switch (symbol->declaration->tag) {
        case ASTTagModuleDeclaration: {
            ASTModuleDeclarationRef declaration = (ASTModuleDeclarationRef)symbol->declaration;
            _ScopeDumperPrintCString(dumper, "module");
            break;
        }
        case ASTTagEnumerationDeclaration: {
            ASTEnumerationDeclarationRef declaration = (ASTEnumerationDeclarationRef)symbol->declaration;
            _ScopeDumperPrintCString(dumper, "enum ");
            _ScopeDumperPrintCString(dumper, StringGetCharacters(declaration->name));
            break;
        }
        case ASTTagFunctionDeclaration: {
            ASTFunctionDeclarationRef declaration = (ASTFunctionDeclarationRef)symbol->declaration;
            _ScopeDumperPrintCString(dumper, "func ");
            _ScopeDumperPrintCString(dumper, StringGetCharacters(declaration->name));
            break;
        }
        case ASTTagStructureDeclaration: {
            ASTStructureDeclarationRef declaration = (ASTStructureDeclarationRef)symbol->declaration;
            _ScopeDumperPrintCString(dumper, "struct ");
            _ScopeDumperPrintCString(dumper, StringGetCharacters(declaration->name));
            _ScopeDumperPrintCString(dumper, " {\n");
            dumper->indentation += 1;
            for (Index index = 0; index < ASTArrayGetElementCount(declaration->values); index++) {
                ASTNodeRef child = (ASTNodeRef)ASTArrayGetElementAtIndex(declaration->values, index);
                assert(child->tag == ASTTagValueDeclaration);

                ASTValueDeclarationRef value = (ASTValueDeclarationRef)child;
                assert(value->kind == ASTValueKindVariable);

                _ScopeDumperPrintIndentation(dumper);
                _ScopeDumperPrintCString(dumper, StringGetCharacters(value->name));
                _ScopeDumperPrintCString(dumper, " = ");
                _ScopeDumperPrintType(dumper, value->type);
                _ScopeDumperPrintCString(dumper, "\n");
            }
            dumper->indentation -= 1;
            _ScopeDumperPrintIndentation(dumper);
            _ScopeDumperPrintCString(dumper, "}\n");
            break;
        }
        case ASTTagOpaqueDeclaration: {
            ASTOpaqueDeclarationRef declaration = (ASTOpaqueDeclarationRef)symbol->declaration;
            _ScopeDumperPrintCString(dumper, "opaque ");
            _ScopeDumperPrintCString(dumper, StringGetCharacters(declaration->name));
            break;
        }
        case ASTTagValueDeclaration: {
            ASTValueDeclarationRef declaration = (ASTValueDeclarationRef)symbol->declaration;
            _ScopeDumperPrintCString(dumper, "value ");
            _ScopeDumperPrintCString(dumper, StringGetCharacters(declaration->name));
            break;
        }

        default:
            JELLY_UNREACHABLE("Unknown tag given for ASTDeclarationRef in ScopeDumper!");
            break;
        }
    }
    //    if (symbol->kind == SymbolKindType) {
    //        assert(symbol->type);
    //        _ScopeDumperPrintType(dumper, symbol->type);
    //    } else {
    //        _ScopeDumperPrintCString(dumper, "?");
    //    }
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
        _ScopeDumperPrintBuiltinType(dumper, builtin->kind);
        return;
    }

    case ASTTagEnumerationType:
        return _ScopeDumperPrintCString(dumper, "enum");

    case ASTTagFunctionType: {
        ASTFunctionTypeRef func = (ASTFunctionTypeRef)type;
        assert(func->declaration);
        _ScopeDumperPrintCString(dumper, "func (");
        for (Index index = 0; index < ASTArrayGetElementCount(func->declaration->parameters); index++) {
            ASTNodeRef child = (ASTNodeRef)ASTArrayGetElementAtIndex(func->declaration->parameters, index);
            assert(child->tag == ASTTagValueDeclaration);

            ASTValueDeclarationRef value = (ASTValueDeclarationRef)child;
            assert(value->kind == ASTValueKindParameter);

            _ScopeDumperPrintType(dumper, value->type);
            if (index + 1 < ASTArrayGetElementCount(func->declaration->parameters)) {
                _ScopeDumperPrintCString(dumper, ", ");
            }
        }
        _ScopeDumperPrintCString(dumper, ") -> ");
        _ScopeDumperPrintType(dumper, func->declaration->returnType);
        return;
    }

    case ASTTagStructureType: {
        ASTStructureTypeRef structure = (ASTStructureTypeRef)type;
        assert(structure->declaration);
        _ScopeDumperPrintCString(dumper, "struct ");
        _ScopeDumperPrintCString(dumper, StringGetCharacters(structure->declaration->name));
        return;
    }

    default:
        break;
    }

    JELLY_UNREACHABLE("Unknown tag given for ASTTypeRef in ScopeDumper!");
}

static inline void _ScopeDumperPrintBuiltinType(ScopeDumperRef dumper, ASTBuiltinTypeKind kind) {
    switch (kind) {
    case ASTBuiltinTypeKindError:
        return _ScopeDumperPrintCString(dumper, "<error>");
    case ASTBuiltinTypeKindVoid:
        return _ScopeDumperPrintCString(dumper, "Void");
    case ASTBuiltinTypeKindBool:
        return _ScopeDumperPrintCString(dumper, "Bool");
    case ASTBuiltinTypeKindInt8:
        return _ScopeDumperPrintCString(dumper, "Int8");
    case ASTBuiltinTypeKindInt16:
        return _ScopeDumperPrintCString(dumper, "Int16");
    case ASTBuiltinTypeKindInt32:
        return _ScopeDumperPrintCString(dumper, "Int32");
    case ASTBuiltinTypeKindInt64:
        return _ScopeDumperPrintCString(dumper, "Int64");
    case ASTBuiltinTypeKindInt:
        return _ScopeDumperPrintCString(dumper, "Int");
    case ASTBuiltinTypeKindUInt8:
        return _ScopeDumperPrintCString(dumper, "UInt8");
    case ASTBuiltinTypeKindUInt16:
        return _ScopeDumperPrintCString(dumper, "UInt16");
    case ASTBuiltinTypeKindUInt32:
        return _ScopeDumperPrintCString(dumper, "UInt32");
    case ASTBuiltinTypeKindUInt64:
        return _ScopeDumperPrintCString(dumper, "UInt64");
    case ASTBuiltinTypeKindUInt:
        return _ScopeDumperPrintCString(dumper, "UInt");
    case ASTBuiltinTypeKindFloat32:
        return _ScopeDumperPrintCString(dumper, "Float32");
    case ASTBuiltinTypeKindFloat64:
        return _ScopeDumperPrintCString(dumper, "Float64");
    case ASTBuiltinTypeKindFloat:
        return _ScopeDumperPrintCString(dumper, "Float");
    default:
        JELLY_UNREACHABLE("Unknown kind given for ASTBuiltinTypeKind in ScopeDumper!");
        break;
    }
}
