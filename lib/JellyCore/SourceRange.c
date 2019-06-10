#include "JellyCore/SourceRange.h"

SourceRange SourceRangeMake(const Char *start, const Char *end) {
    SourceRange range;
    range.start = start;
    range.end   = end;
    return range;
}

SourceRange SourceRangeNull(void) {
    SourceRange range;
    range.start = NULL;
    range.end   = NULL;
    return range;
}
