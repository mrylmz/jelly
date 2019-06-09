#ifndef __JELLY_SOURCERANGE__
#define __JELLY_SOURCERANGE__

#include <JellyCore/Base.h>

JELLY_EXTERN_C_BEGIN

struct _SourceRange {
    const Char *start;
    const Char *end;
};
typedef struct _SourceRange SourceRange;

SourceRange SourceRangeMake(const Char *start, const Char *end);

const SourceRange kSourceRangeNull = {NULL, NULL};

JELLY_EXTERN_C_END

#endif
