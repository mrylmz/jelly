#ifndef __JELLY_NAMERESOLUTION__
#define __JELLY_NAMERESOLUTION__

#include <JellyCore/ASTContext.h>
#include <JellyCore/ASTNodes.h>
#include <JellyCore/Allocator.h>
#include <JellyCore/Base.h>

JELLY_EXTERN_C_BEGIN

// TODO: Create following type resolution passes
// # Name Resolution
// - (1): Add all enum, struct, var declarations to their scope
// - (2): Resolve all parameter and result type refs of func declarations and add func declarations to their scope
// - (3): Repeat steps (1) and (2) for all declaration bodies including param, enum element declarations
// # Expression Type Resolution
// - (1):

void PerformNameResolution(ASTModuleDeclarationRef module);

JELLY_EXTERN_C_END

#endif
