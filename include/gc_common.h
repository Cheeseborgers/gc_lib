#ifndef GC_COMMON_H_
#define GC_COMMON_H_

#ifndef GCDEF
#define GCDEF static inline
#endif

#ifdef _WIN32
#    define GC_LINE_END "\r\n"
#else
#    define GC_LINE_END "\n"
#endif

/*
* Optional macro configuration:
*   - GC_ASSERT(expr) : custom assert macro (default: assert)
*   - GC_REALLOC(ptr, size) : custom realloc (default: realloc)
*   - GC_FREE(ptr) : custom free (default: free)
*   - GC_INIT_CAP : initial capacity (default: 8)
*/
#ifndef GC_REALLOC
#include <stdlib.h>
#define GC_REALLOC realloc
#endif

#ifndef GC_MALLOC
#include <stdlib.h>
#define GC_MALLOC malloc
#endif

#ifndef GC_FREE
#include <stdlib.h>
#define GC_FREE free
#endif

#ifndef GC_ASSERT
#include <assert.h>
#define GC_ASSERT(expr) assert(expr)
#endif

#define GC_UNUSED(value) (void)(value)

#define GC_TODO(message)                                                                                                      \
do {                                                                                                                         \
fprintf(stderr, "%s:%d: TODO: %s\n", __FILE__, __LINE__, message);                                                       \
abort();                                                                                                                 \
} while (0)

#define GC_UNREACHABLE(message)                                                                                               \
do {                                                                                                                         \
fprintf(stderr, "%s:%d: UNREACHABLE: %s\n", __FILE__, __LINE__, message);                                                \
abort();                                                                                                                 \
} while (0)

#define gc_return_defer(value) do { result = (value); goto defer; } while(0)

#endif //GC_COMMON_H_