#ifndef __JELLY_TYPECHECKER__
#define __JELLY_TYPECHECKER__

#include <JellyCore/ASTContext.h>
#include <JellyCore/Allocator.h>
#include <JellyCore/Base.h>

JELLY_EXTERN_C_BEGIN

typedef struct _TypeChecker *TypeCheckerRef;

TypeCheckerRef TypeCheckerCreate(AllocatorRef allocator);

void TypeCheckerDestroy(TypeCheckerRef typeChecker);

void TypeCheckerValidateModule(TypeCheckerRef typeChecker, ASTContextRef context, ASTModuleDeclarationRef module);

JELLY_EXTERN_C_END

#endif
