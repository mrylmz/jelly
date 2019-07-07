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

Bool SourceRangeIsEqual(SourceRange range, const Char *string) {
    Index length = range.end - range.start;

    if (strlen(string) != length) {
        return false;
    }

    if (strncmp(string, range.start, length) != 0) {
        return false;
    }

    return true;
}

Index SourceRangeLength(SourceRange range) {
    return range.end - range.start;
}
