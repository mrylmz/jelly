#ifndef __JELLY_PARSER__
#define __JELLY_PARSER__

#include <JellyCore/ASTContext.h>
#include <JellyCore/Allocator.h>
#include <JellyCore/Base.h>

#warning TODO: Add support for character literals!
#warning TODO: Allow parameter swapping for functions like `==` where the algorithm for swapped arguments would be equivalent!

JELLY_EXTERN_C_BEGIN

typedef struct _Parser *ParserRef;

ParserRef ParserCreate(AllocatorRef allocator, ASTContextRef context);

void ParserDestroy(ParserRef parser);

ASTSourceUnitRef ParserParseSourceUnit(ParserRef parser, StringRef filePath, StringRef source);
ASTSourceUnitRef ParserParseModuleSourceUnit(ParserRef parser, ASTModuleDeclarationRef module, StringRef filePath, StringRef source);
ASTModuleDeclarationRef ParserParseModuleDeclaration(ParserRef parser, StringRef filePath, StringRef source);

JELLY_EXTERN_C_END

#endif
