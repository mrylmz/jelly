#include "JellyCore/Array.h"
#include "JellyCore/SymbolTable.h"

struct _Symbol {
    StringRef name;
    SourceRange location;
};
typedef struct _Symbol Symbol;

struct _Scope {
    ScopeKind kind;
    ScopeRef parent;
    SourceRange sourceRange;
    ArrayRef symbols;
};

struct _SymbolTable {
    AllocatorRef allocator;
    ArrayRef scopes;
    ScopeRef currentScope;
};

ScopeRef _SymbolTableCreateScope(SymbolTableRef symbolTable, ScopeKind kind, ScopeRef parent);

Index _ScopeGetVirtualEnd(ScopeRef scope, const Char *virtualEndOfScope);

SymbolTableRef SymbolTableCreate(AllocatorRef allocator) {
    SymbolTableRef symbolTable = AllocatorAllocate(allocator, sizeof(struct _SymbolTable));
    assert(symbolTable);
    symbolTable->allocator     = allocator;
    symbolTable->scopes        = ArrayCreateEmpty(allocator, sizeof(struct _Scope), 8);
    symbolTable->currentScope  = _SymbolTableCreateScope(symbolTable, ScopeKindGlobal, NULL);
    return symbolTable;
}

void SymbolTableDestroy(SymbolTableRef symbolTable) {
    for (Index index = 0; index < ArrayGetElementCount(symbolTable->scopes); index++) {
        ScopeRef scope = ArrayGetElementAtIndex(symbolTable->scopes, index);
        ArrayDestroy(scope->symbols);
    }

    ArrayDestroy(symbolTable->scopes);
    AllocatorDeallocate(symbolTable->allocator, symbolTable);
}

ScopeRef SymbolTableGetGlobalScope(SymbolTableRef symbolTable) {
    return (ScopeRef)ArrayGetElementAtIndex(symbolTable->scopes, 0);
}

ScopeRef SymbolTableGetCurrentScope(SymbolTableRef symbolTable) {
    return symbolTable->currentScope;
}

ScopeRef SymbolTablePushScope(SymbolTableRef symbolTable, ScopeKind scopeKind) {
    ScopeRef currentScope = _SymbolTableCreateScope(symbolTable, scopeKind, symbolTable->currentScope);
    assert(currentScope);
    symbolTable->currentScope = currentScope;
    return symbolTable->currentScope;
}

ScopeRef SymbolTablePopScope(SymbolTableRef symbolTable) {
    assert(symbolTable->currentScope->parent);
    symbolTable->currentScope = symbolTable->currentScope->parent;
    return symbolTable->currentScope;
}

ScopeKind ScopeGetKind(ScopeRef scope) {
    return scope->kind;
}

ScopeRef ScopeGetParent(ScopeRef scope) {
    return scope->parent;
}

SymbolRef ScopeInsertSymbol(ScopeRef scope, StringRef name, SourceRange location) {
    if (ScopeLookupSymbol(scope, name, NULL) != NULL) {
        return NULL;
    }

    // @SortedSymbolInsertion
    // @TODO For now we assume that all symbols are inserted in ascending location order
    //       but it could be possible that this will not always be the case so we should
    //       do a binary insert in asscending location order or sort the array after insertion
    SymbolRef symbol = ArrayAppendUninitializedElement(scope->symbols);
    symbol->name     = name;
    symbol->location = location;

    if (scope->sourceRange.start == NULL) {
        scope->sourceRange = location;
    } else {
        scope->sourceRange.start = MIN(scope->sourceRange.start, location.start);
        scope->sourceRange.end   = MAX(scope->sourceRange.end, location.end);
    }

    return symbol;
}

SymbolRef ScopeLookupSymbol(ScopeRef scope, StringRef name, const Char *virtualEndOfScope) {
    for (Index index = 0; index < _ScopeGetVirtualEnd(scope, virtualEndOfScope); index++) {
        SymbolRef symbol = ArrayGetElementAtIndex(scope->symbols, index);
        if (StringIsEqual(symbol->name, name)) {
            return symbol;
        }
    }

    return NULL;
}

StringRef SymbolGetName(SymbolRef symbol) {
    return symbol->name;
}

ScopeRef _SymbolTableCreateScope(SymbolTableRef symbolTable, ScopeKind kind, ScopeRef parent) {
    ScopeRef scope     = ArrayAppendUninitializedElement(symbolTable->scopes);
    scope->kind        = kind;
    scope->parent      = parent;
    scope->sourceRange = SourceRangeMake(NULL, NULL);
    scope->symbols     = ArrayCreateEmpty(symbolTable->allocator, sizeof(struct _Symbol), 8);
    return scope;
}

Index _ScopeGetVirtualEnd(ScopeRef scope, const Char *virtualEndOfScope) {
    if (scope->kind == ScopeKindGlobal || virtualEndOfScope == NULL) {
        return ArrayGetElementCount(scope->symbols);
    }

    // TODO: See @SortedSymbolInsertion
    Index symbolCount = ArrayGetElementCount(scope->symbols);
    if (symbolCount > 0) {
        for (Index index = symbolCount - 1; index > 0; index--) {
            SymbolRef symbol = ArrayGetElementAtIndex(scope->symbols, index);
            if (symbol->location.start < virtualEndOfScope) {
                return index + 1;
            }
        }
    }

    return 0;
}
