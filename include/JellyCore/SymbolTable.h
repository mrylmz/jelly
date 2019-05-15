#ifndef __JELLY_SYMBOLTABLE__
#define __JELLY_SYMBOLTABLE__

#include <JellyCore/Base.h>
#include <JellyCore/Allocator.h>
#include <JellyCore/SourceRange.h>
#include <JellyCore/String.h>

JELLY_EXTERN_C_BEGIN

enum _ScopeKind {
    ScopeKindGlobal,
};
typedef enum _ScopeKind ScopeKind;

typedef struct _Scope* ScopeRef;
typedef struct _Symbol* SymbolRef;
typedef struct _SymbolTable* SymbolTableRef;

SymbolTableRef SymbolTableCreate(AllocatorRef allocator);
void SymbolTableDestroy(SymbolTableRef symbolTable);

ScopeRef SymbolTableGetGlobalScope(SymbolTableRef symbolTable);
ScopeRef SymbolTableGetCurrentScope(SymbolTableRef symbolTable);

ScopeRef SymbolTablePushScope(SymbolTableRef symbolTable, ScopeKind scopeKind);
ScopeRef SymbolTablePopScope(SymbolTableRef symbolTable);

ScopeKind ScopeGetKind(ScopeRef scope);
ScopeRef ScopeGetParent(ScopeRef scope);

SymbolRef ScopeInsertSymbol(ScopeRef scope, StringRef name, SourceRange location);
SymbolRef ScopeLookupSymbol(ScopeRef scope, StringRef name, const Char *virtualEndOfScope);

StringRef SymbolGetName(SymbolRef symbol);

JELLY_EXTERN_C_END

#endif
