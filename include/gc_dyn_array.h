#ifndef GC_DYNAMIC_ARRAY_H_
#define GC_DYNAMIC_ARRAY_H_

#include <string.h>

#include "gc_common.h"

/* -----------------------------------------------------------
 * GC Dynamic Array — a lightweight, type-agnostic, resizable array.
 *
 * Inspired by Tsoding's `nob.h` dyn array macros:
 * https://github.com/tsoding/nob.h/
 *
 * Features:
 *   - Dynamic resizing with exponential growth.
 *   - Append single or multiple elements.
 *   - Fast removal via swap with last element.
 *   - Foreach macro for convenient iteration.
 *   - Optional prefix stripping via GC_STRIP_PREFIX.
 *   - Fully customizable allocators and assert macros.
 *
 * Example usage:
 *
 *   #define GC_STRIP_PREFIX
 *   #include "gc_dynamic_array.h"
 *
 *   typedef struct {
 *       int *items;
 *       size_t count;
 *       size_t capacity;
 *   } IntArray;
 *
 *   int main(void) {
 *       IntArray arr = {0};
 *       da_append(&arr, 10);
 *       da_append(&arr, 20);
 *
 *       int *last = &da_last(&arr);
 *       printf("Last element: %d\n", *last);
 *
 *       da_foreach(int, x, &arr) {
 *           printf("Item: %d\n", *x);
 *       }
 *
 *       da_remove_unordered(&arr, 0);
 *       da_free(&arr);
 *   }
 * --------------------------------------------------------- */

/* -------------------- Configurable helpers -------------------- */

/* Default starting capacity */
#ifndef GC_INIT_CAP
#define GC_INIT_CAP 8
#endif

/* Helpers for static arrays */
#define GC_ARRAY_LEN(array) (sizeof(array) / sizeof(array[0]))
#define GC_ARRAY_GET(array, index) (GC_ASSERT((size_t)index < GC_ARRAY_LEN(array)), array[(size_t)index])

/* -------------------- Core dynamic array macros -------------------- */

/* Reserve enough capacity for at least `expected_capacity` elements */
#define gc_da_reserve(da, expected_capacity)                                                                                     \
    do {                                                                                                                         \
        if ((expected_capacity) > (da)->capacity) {                                                                              \
            if ((da)->capacity == 0) {                                                                                           \
                (da)->capacity = GC_INIT_CAP;                                                                                    \
            }                                                                                                                    \
            while ((expected_capacity) > (da)->capacity) {                                                                       \
                (da)->capacity *= 2;                                                                                             \
            }                                                                                                                    \
            void *new_items = GC_REALLOC((da)->items, (da)->capacity * sizeof(*(da)->items));                                    \
            GC_ASSERT(new_items != NULL && "Out of memory: buy more RAM!");                                                      \
            (da)->items = new_items;                                                                                             \
        }                                                                                                                        \
    } while (0)

/* Append a single item */
#define gc_da_append(da, item)                                                                                                   \
    do {                                                                                                                         \
        gc_da_reserve((da), (da)->count + 1);                                                                                    \
        (da)->items[(da)->count++] = (item);                                                                                     \
    } while (0)

/* Append multiple items from a buffer */
#define gc_da_append_many(da, new_items, new_count)                                                                              \
    do {                                                                                                                         \
        gc_da_reserve((da), (da)->count + (new_count));                                                                          \
        memcpy((da)->items + (da)->count, (new_items), (new_count) * sizeof(*(da)->items));                                      \
        (da)->count += (new_count);                                                                                              \
    } while (0)

/* Resize to new size (does not initialize new elements) */
#define gc_da_resize(da, new_size)                                                                                               \
    do {                                                                                                                         \
        gc_da_reserve((da), (new_size));                                                                                         \
        (da)->count = (new_size);                                                                                                \
    } while (0)

/* Get the last element */
#define gc_da_last(da) ((da)->items[(GC_ASSERT((da)->count > 0), (da)->count - 1)])

/* Remove element i by swapping with the last element (fast, unordered) */
#define gc_da_remove_unordered(da, i)                                                                                            \
    do {                                                                                                                         \
        size_t __idx = (i);                                                                                                      \
        GC_ASSERT(__idx < (da)->count);                                                                                          \
        (da)->items[__idx] = (da)->items[--(da)->count];                                                                         \
    } while (0)

/* Free the dynamic array */
#define gc_da_free(da)                                                                                                           \
    do {                                                                                                                         \
        if ((da).items) {                                                                                                        \
            GC_FREE((da).items);                                                                                                 \
            (da).items = NULL;                                                                                                   \
        }                                                                                                                        \
        (da).count = 0;                                                                                                          \
        (da).capacity = 0;                                                                                                       \
    } while (0)

/* Foreach macro over array elements */
#define gc_da_foreach(Type, it, da) for (Type *it = (da)->items; it < (da)->items + (da)->count; ++it)

/* -------------------- Strip prefix macros -------------------- */
#endif /* GC_DYNAMIC_ARRAY_H_ */

#ifndef GC_STRIP_PREFIX_GUARD_
#define GC_STRIP_PREFIX_GUARD_
#ifdef GC_STRIP_PREFIX
#define da_append gc_da_append
#define da_free gc_da_free
#define da_append_many gc_da_append_many
#define da_resize gc_da_resize
#define da_reserve gc_da_reserve
#define da_last gc_da_last
#define da_remove_unordered gc_da_remove_unordered
#define da_foreach gc_da_foreach
#endif
#endif
