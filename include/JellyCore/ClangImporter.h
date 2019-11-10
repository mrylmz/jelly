#ifndef __JELLY_CLANGIMPORT__
#define __JELLY_CLANGIMPORT__

#include <JellyCore/ASTContext.h>
#include <JellyCore/Allocator.h>
#include <JellyCore/Base.h>
#include <JellyCore/String.h>

JELLY_EXTERN_C_BEGIN

typedef struct _ClangImporter *ClangImporterRef;

ClangImporterRef ClangImporterCreate(AllocatorRef allocator, ASTContextRef context);

void ClangImporterDestroy(ClangImporterRef importer);

ASTModuleDeclarationRef ClangImporterImport(ClangImporterRef importer, StringRef filePath);

JELLY_EXTERN_C_END

#endif
