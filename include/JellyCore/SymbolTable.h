#ifndef __JELLY_SYMBOLTABLE__
#define __JELLY_SYMBOLTABLE__

#include <JellyCore/Allocator.h>
#include <JellyCore/Base.h>

JELLY_EXTERN_C_BEGIN

enum _ScopeKind {
    ScopeKindGlobal,
    ScopeKindBranch,
    ScopeKindLoop,
    ScopeKindCase,
    ScopeKindSwitch,
    ScopeKindEnumeration,
    ScopeKindFunction,
    ScopeKindInitializer,
    ScopeKindStructure,
};
typedef enum _ScopeKind ScopeKind;

typedef Int64 ScopeID;

static ScopeID kScopeGlobal = 0;
static ScopeID kScopeNull   = -1;

typedef Int64 SymbolID;

static SymbolID kSymbolNull = -1;

typedef struct _Symbol *SymbolRef;

typedef struct _SymbolTable *SymbolTableRef;

SymbolTableRef SymbolTableCreate(AllocatorRef allocator);

void SymbolTableDestroy(SymbolTableRef table);

ScopeID SymbolTableInsertScope(SymbolTableRef table, ScopeKind kind, ScopeID parent, const Char *location);

ScopeID SymbolTableGetScopeParent(SymbolTableRef table, ScopeID id);

ScopeID SymbolTableGetEnclosingScopeParent(SymbolTableRef table, ScopeID id);

SymbolID SymbolTableInsertSymbol(SymbolTableRef table, ScopeID id, const Char *name);

SymbolID SymbolTableLookupSymbol(SymbolTableRef table, ScopeID id, const Char *name);

JELLY_EXTERN_C_END

#endif
