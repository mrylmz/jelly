#include "JellyCore/Base.h"

void __jelly_unreachable(const Char *message, const Char *file, Index line) {
    if (message) {
        fprintf(stderr, "UNREACHABLE: '%s' in %s at line %zu!\n", message, file, line);
    }

    abort();

#if __has_builtin(__builtin_unreachable) || (JELLY_GCC_VERSION >= 40500)
    __builtin_unreachable();
#elif defined(_MSC_VER)
    __assume(false);
#endif
}
