#include "JellyCore/Array.h"
#include "JellyCore/Dictionary.h"
#include "JellyCore/SymbolTable.h"

const Index kDefaultSymbolDictionaryCapacity = 65535;
const Index kDefaultSymbolArrayCapacity      = 65535;

struct _Symbol {
    SymbolID id;
};

struct _Scope {
    ScopeKind kind;
    ScopeID id;
    ScopeID parent;
    const Char *location;
    DictionaryRef symbols;
};
typedef struct _Scope *ScopeRef;

struct _SymbolTable {
    AllocatorRef allocator;
    ScopeID nextScopeID;
    ArrayRef scopes;
    SymbolID nextSymbolID;
    ArrayRef symbols;
};

SymbolTableRef SymbolTableCreate(AllocatorRef allocator) {
    SymbolTableRef table = (SymbolTableRef)AllocatorAllocate(allocator, sizeof(struct _SymbolTable));
    table->allocator     = allocator;
    table->nextScopeID   = kScopeGlobal + 1;
    table->scopes        = ArrayCreateEmpty(table->allocator, sizeof(struct _Scope), 8);
    table->nextSymbolID  = 0;
    table->symbols       = ArrayCreateEmpty(table->allocator, sizeof(struct _Symbol), kDefaultSymbolArrayCapacity);

    ScopeRef globalScope  = ArrayAppendUninitializedElement(table->scopes);
    globalScope->id       = kScopeGlobal;
    globalScope->parent   = -1;
    globalScope->location = NULL;
    globalScope->symbols  = CStringDictionaryCreate(table->allocator, kDefaultSymbolDictionaryCapacity);

    return table;
}

void SymbolTableDestroy(SymbolTableRef table) {
    for (Index index = 0; index < ArrayGetElementCount(table->scopes); index++) {
        ScopeRef scope = (ScopeRef)ArrayGetElementAtIndex(table->scopes, index);
        DictionaryDestroy(scope->symbols);
    }

    ArrayDestroy(table->scopes);
    AllocatorDeallocate(table->allocator, table);
}

ScopeID SymbolTableInsertScope(SymbolTableRef table, ScopeKind kind, ScopeID parent, const Char *location) {
    ScopeRef scope  = ArrayAppendUninitializedElement(table->scopes);
    scope->kind     = kind;
    scope->id       = table->nextScopeID;
    scope->parent   = parent;
    scope->location = location;
    scope->symbols  = CStringDictionaryCreate(table->allocator, kDefaultSymbolDictionaryCapacity);
    table->nextScopeID += 1;
    return scope->id;
}

ScopeID SymbolTableGetScopeParent(SymbolTableRef table, ScopeID id) {
    if (id == kScopeNull) {
        return kScopeNull;
    }

    ScopeRef scope = (ScopeRef)ArrayGetElementAtIndex(table->scopes, id);
    return scope->parent;
}

ScopeID SymbolTableGetEnclosingScopeParent(SymbolTableRef table, ScopeID id) {
    assert(0 <= id && id < ArrayGetElementCount(table->scopes));

    ScopeRef scope  = (ScopeRef)ArrayGetElementAtIndex(table->scopes, id);
    ScopeRef parent = NULL;
    if (scope->parent != kScopeNull) {
        parent = (ScopeRef)ArrayGetElementAtIndex(table->scopes, scope->parent);
    }

    while (parent) {
        switch (scope->kind) {
        case ScopeKindGlobal:
        case ScopeKindBranch:
        case ScopeKindLoop:
        case ScopeKindCase:
        case ScopeKindSwitch:
            return parent->id;

        case ScopeKindEnumeration:
        case ScopeKindFunction:
        case ScopeKindStructure:
            if (parent->kind == ScopeKindGlobal) {
                return parent->id;
            }
            break;

        case ScopeKindInitializer:
            if (parent->kind == ScopeKindGlobal || parent->kind == ScopeKindStructure) {
                return parent->id;
            }
            break;
        }

        if (parent->parent != kScopeNull) {
            parent = (ScopeRef)ArrayGetElementAtIndex(table->scopes, parent->parent);
        } else {
            parent = NULL;
        }
    }

    return NULL;
}

SymbolID SymbolTableInsertSymbol(SymbolTableRef table, ScopeID id, const Char *name) {
    assert(SymbolTableLookupSymbol(table, id, name) == kSymbolNull);

    SymbolRef symbol = ArrayAppendUninitializedElement(table->symbols);
    memset(symbol, 0, sizeof(struct _Symbol));
    symbol->id = id;

    assert(0 <= id && id < ArrayGetElementCount(table->scopes));
    ScopeRef scope = ArrayGetElementAtIndex(table->scopes, id);
    DictionaryInsert(scope->symbols, name, &symbol->id, sizeof(SymbolID));

    return symbol;
}

SymbolID SymbolTableLookupSymbol(SymbolTableRef table, ScopeID id, const Char *name) {
    assert(0 <= id && id < ArrayGetElementCount(table->scopes));
    ScopeRef scope = ArrayGetElementAtIndex(table->scopes, id);

    const void *ref = DictionaryLookup(scope->symbols, name);
    if (ref == NULL) {
        return kSymbolNull;
    }

    return *((SymbolID *)ref);
}

SymbolID SymbolTableLookupSymbolInHierarchy(SymbolTableRef table, ScopeID id, const Char *name) {
    assert(0 <= id && id < ArrayGetElementCount(table->scopes));

    SymbolID nextID = id;
    while (nextID != kScopeNull) {
        SymbolID symbol = SymbolTableLookupSymbol(table, nextID, name);
        if (symbol != kSymbolNull) {
            return symbol;
        } else {
            nextID = SymbolTableGetEnclosingScopeParent(table, nextID);
        }
    }

    return kSymbolNull;
}
