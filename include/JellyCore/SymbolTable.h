#ifndef __JELLY_SYMBOLTABLE__
#define __JELLY_SYMBOLTABLE__

#include <JellyCore/ASTNodes.h>
#include <JellyCore/Allocator.h>
#include <JellyCore/Base.h>
#include <JellyCore/SourceRange.h>
#include <JellyCore/String.h>

JELLY_EXTERN_C_BEGIN

typedef struct _Scope *ScopeRef;
typedef struct _SymbolTable *SymbolTableRef;

enum _SymbolKind {
    SymbolKindNone,
    SymbolKindType,
    SymbolKindLink,
};
typedef enum _SymbolKind SymbolKind;

struct _Symbol {
    StringRef name;
    ASTNodeRef node;
};
typedef struct _Symbol *SymbolRef;

enum _ScopeKind {
    ScopeKindGlobal,
    ScopeKindBranch,
    ScopeKindLoop,
    ScopeKindCase,
    ScopeKindSwitch,
    ScopeKindEnumeration,
    ScopeKindFunction,
    ScopeKindStructure,
};
typedef enum _ScopeKind ScopeKind;

SymbolTableRef SymbolTableCreate(AllocatorRef allocator);
void SymbolTableDestroy(SymbolTableRef symbolTable);

ScopeRef SymbolTableGetGlobalScope(SymbolTableRef symbolTable);

void SymbolTableSetCurrentScope(SymbolTableRef symbolTable, ScopeRef scope);
ScopeRef SymbolTableGetCurrentScope(SymbolTableRef symbolTable);

ScopeRef SymbolTablePushScope(SymbolTableRef symbolTable, ScopeKind scopeKind);
ScopeRef SymbolTablePopScope(SymbolTableRef symbolTable);

ScopeKind ScopeGetKind(ScopeRef scope);
ScopeRef ScopeGetParent(ScopeRef scope);
Index ScopeGetChildCount(ScopeRef scope);
ScopeRef ScopeGetChildAtIndex(ScopeRef scope, Index index);
Index ScopeGetSymbolCount(ScopeRef scope);
SymbolRef ScopeGetSymbolAtIndex(ScopeRef scope, Index index);

SymbolRef ScopeInsertSymbol(ScopeRef scope, StringRef name, ASTNodeRef node);
SymbolRef ScopeInsertUniqueSymbol(ScopeRef scope, ASTNodeRef node);
SymbolRef ScopeLookupSymbol(ScopeRef scope, StringRef name, const Char *virtualEndOfScope);

JELLY_EXTERN_C_END

#endif
