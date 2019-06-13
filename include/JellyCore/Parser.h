#ifndef __JELLY_PARSER__
#define __JELLY_PARSER__

#include <JellyCore/ASTContext.h>
#include <JellyCore/Allocator.h>
#include <JellyCore/Base.h>

JELLY_EXTERN_C_BEGIN

typedef struct _Parser *ParserRef;

ParserRef ParserCreate(AllocatorRef allocator, ASTContextRef context);

void ParserDestroy(ParserRef parser);

ASTSourceUnitRef ParserParseSourceUnit(ParserRef parser, StringRef filePath, StringRef source);

JELLY_EXTERN_C_END

#endif
