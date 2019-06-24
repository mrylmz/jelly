#include "JellyCore/Array.h"
#include "JellyCore/SymbolTable.h"

// TODO: Fix memory leaks...

struct _Symbol {
    StringRef name;
    SourceRange location;
    ASTNodeRef node;
    ASTTypeRef type;
};
typedef struct _Symbol Symbol;

struct _Scope {
    ScopeKind kind;
    ScopeRef parent;
    SourceRange location;
    ArrayRef children;
    ArrayRef symbols;
    UInt64 uniqueSymbolID;
};

struct _SymbolTable {
    AllocatorRef allocator;
    ArrayRef scopes;
    ScopeRef currentScope;
};

ScopeRef _SymbolTableCreateScope(SymbolTableRef symbolTable, ScopeKind kind, ScopeRef parent);

Index _ScopeGetVirtualEnd(ScopeRef scope, const Char *virtualEndOfScope);

Bool _ArrayIsSymbolLocationOrderedAscending(const void *lhs, const void *rhs);

SymbolTableRef SymbolTableCreate(AllocatorRef allocator) {
    SymbolTableRef symbolTable = AllocatorAllocate(allocator, sizeof(struct _SymbolTable));
    assert(symbolTable);
    symbolTable->allocator = allocator;
    // TODO: @Bug Reallocation of Array causes dangling pointers of Scope(s)...
    symbolTable->scopes = ArrayCreateEmpty(allocator, sizeof(struct _Scope), 1024);
    assert(symbolTable->scopes);
    symbolTable->currentScope = _SymbolTableCreateScope(symbolTable, ScopeKindGlobal, NULL);
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

Index ScopeGetChildCount(ScopeRef scope) {
    return ArrayGetElementCount(scope->children);
}

ScopeRef ScopeGetChildAtIndex(ScopeRef scope, Index index) {
    return *((ScopeRef *)ArrayGetElementAtIndex(scope->children, index));
}

SymbolRef ScopeInsertSymbol(ScopeRef scope, StringRef name, SourceRange location) {
    if (ScopeLookupSymbol(scope, name, NULL) != NULL) {
        return NULL;
    }

    SymbolRef symbol = ArrayAppendUninitializedElement(scope->symbols);
    memset(symbol, 0, sizeof(struct _Symbol));
    symbol->name     = name;
    symbol->location = location;

    if (scope->location.start == NULL) {
        scope->location = location;
    } else {
        scope->location.start = MIN(scope->location.start, location.start);
        scope->location.end   = MAX(scope->location.end, location.end);
    }

    return symbol;
}

SymbolRef ScopeInsertUniqueSymbol(ScopeRef scope, SourceRange location) {
    StringRef name = StringCreate(AllocatorGetSystemDefault(), "$");
    Char buffer[20];
    snprintf(&buffer[0], 20, "%lld", scope->uniqueSymbolID);
    StringAppend(name, buffer);
    scope->uniqueSymbolID += 1;

    return ScopeInsertSymbol(scope, name, location);
}

SymbolRef ScopeLookupSymbol(ScopeRef scope, StringRef name, const Char *virtualEndOfScope) {
    for (Index index = 0; index < ScopeGetSymbolCount(scope); index++) {
        SymbolRef symbol = ArrayGetElementAtIndex(scope->symbols, index);
        if (StringIsEqual(symbol->name, name) && (symbol->location.start < virtualEndOfScope || scope->kind == ScopeKindGlobal)) {
            return symbol;
        }
    }

    return NULL;
}

StringRef SymbolGetName(SymbolRef symbol) {
    return symbol->name;
}

ASTNodeRef SymbolGetNode(SymbolRef symbol) {
    return symbol->node;
}

void SymbolSetNode(SymbolRef symbol, ASTNodeRef node) {
    symbol->node = node;
}

ASTTypeRef SymbolGetType(SymbolRef symbol) {
    return symbol->type;
}

void SymbolSetType(SymbolRef symbol, ASTTypeRef type) {
    symbol->type = type;
}

ScopeRef _SymbolTableCreateScope(SymbolTableRef symbolTable, ScopeKind kind, ScopeRef parent) {
    ScopeRef scope  = ArrayAppendUninitializedElement(symbolTable->scopes);
    scope->kind     = kind;
    scope->parent   = parent;
    scope->location = SourceRangeMake(NULL, NULL);
    // TODO: @Bug Reallocation of Array causes dangling pointers of Scope(s)...
    scope->children       = ArrayCreateEmpty(symbolTable->allocator, sizeof(ScopeRef), 1024);
    scope->symbols        = ArrayCreateEmpty(symbolTable->allocator, sizeof(struct _Symbol), 1024);
    scope->uniqueSymbolID = 1;

    if (parent) {
        ArrayAppendElement(parent->children, &scope);
    }

    return scope;
}

Bool _ArrayIsSymbolLocationOrderedAscending(const void *lhs, const void *rhs) {
    SymbolRef a = (SymbolRef)lhs;
    SymbolRef b = (SymbolRef)rhs;

    if (a->location.start == b->location.start) {
        return a->location.end < b->location.end;
    }

    return a->location.start < b->location.start;
}
