#ifndef __JELLY_SYMBOLTABLE__
#define __JELLY_SYMBOLTABLE__

#include <JellyCore/Allocator.h>
#include <JellyCore/Base.h>
#include <JellyCore/SourceRange.h>
#include <JellyCore/String.h>

JELLY_EXTERN_C_BEGIN

typedef struct _ASTNode *ASTNodeRef;
typedef struct _ASTNode *ASTTypeRef;

enum _SymbolKind {
    SymbolKindNone,
    SymbolKindType,
    SymbolKindLink,
};
typedef enum _SymbolKind SymbolKind;

struct _Symbol {
    StringRef name;
    SourceRange location;
    ASTNodeRef node; // TODO: Remove node!!!

    SymbolKind kind;
    union {
        ASTTypeRef type;
        struct _Symbol *link;
    };
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

typedef struct _Scope *ScopeRef;
typedef struct _SymbolTable *SymbolTableRef;

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

SymbolRef ScopeInsertSymbol(ScopeRef scope, StringRef name, SourceRange location);
SymbolRef ScopeInsertUniqueSymbol(ScopeRef scope, SourceRange location);
SymbolRef ScopeLookupSymbol(ScopeRef scope, StringRef name, const Char *virtualEndOfScope);

JELLY_EXTERN_C_END

#endif
