#ifndef GC_STRING_BUILDER_H_
#define GC_STRING_BUILDER_H_

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "gc_common.h"

#include "gc_dyn_array.h"

/* -----------------------------------------------------------
 * String_Builder — a dynamic string builder for C
 * Inspired by Tsoding's `nob.h` string builder.
 * https://github.com/tsoding/nob.h/
 *
 * Features:
 *  - Append C strings, raw buffers, or formatted text
 *  - Automatically grows underlying dynamic array
 *  - Converts to null-terminated C string
 *  - Optional "strip prefix" macros for cleaner names
 *
 * Example usage:
 *
 *   #define GC_STRIP_PREFIX
 *   #include "gc_string_builder.h"
 *
 *   int main(void) {
 *       String_Builder sb = {0};
 *       sb_append_cstr(&sb, "Hello ");
 *       sb_appendf(&sb, "world %d!", 2025);
 *       printf("%s\n", sb_to_cstr(&sb));
 *       sb_free(&sb);
 *       return 0;
 *   }
 *
 * Optional: define GCDEF to control linkage:
 *   #define GCDEF static inline
 *   #include "gc_string_builder.h"
 *
 * --------------------------------------------------------- */
#ifndef GCDEF
#define GCDEF static inline
#endif

typedef struct {
  char *items;     // backing dynamic array
  size_t count;    // current string length
  size_t capacity; // capacity of the array
} String_Builder;

// Append a null-terminated C string
GCDEF void gc_sb_append_cstr(String_Builder *sb, const char *s) {
  size_t len = strlen(s);
  gc_da_append_many(sb, s, len);
}

GCDEF void gc_sb_append_char(String_Builder *sb, char c) {
  gc_da_append(sb, c);
}

// Append raw buffer of length `len`
GCDEF void gc_sb_append_buf(String_Builder *sb, const char *buf,
                                 size_t len) {
  gc_da_append_many(sb, buf, len);
}

// Append formatted string like printf
GCDEF void gc_sb_appendf(String_Builder *sb, const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);

  // Try to print into remaining space
  gc_da_reserve(sb, sb->count + 128);
  for (;;) {
    size_t capacity = sb->capacity - sb->count;
    int written = vsnprintf(sb->items + sb->count, capacity, fmt, args);
    if (written < 0)
      break; // error
    if ((size_t)written < capacity) {
      sb->count += written;
      break;
    }
    // not enough space, reserve more
    gc_da_reserve(sb, sb->count + written + 1);
  }

  va_end(args);
}

// Ensure the string is null-terminated
GCDEF char *gc_sb_to_cstr(String_Builder *sb) {
  gc_da_append(sb, '\0'); // append null terminator
  sb->count--;            // don't count the terminator in length
  return sb->items;
}

// Free the builder
GCDEF void gc_sb_free(String_Builder *sb) { gc_da_free(*sb); }

#endif // GC_STRING_BUILDER_H_

#ifndef GC_STRING_BUILDER_STRIP_PREFIX_GUARD_
#define GC_STRING_BUILDER_STRIP_PREFIX_GUARD_
#ifdef GC_STRIP_PREFIX
#define sb_append_cstr gc_sb_append_cstr
#define sb_append_buf gc_sb_append_buf
#define sb_appendf gc_sb_appendf
#define sb_to_cstr gc_sb_to_cstr
#define sb_free gc_sb_free
#endif // GC_STRIP_PREFIX
#endif // GC_STRING_BUILDER_STRIP_PREFIX_GUARD_
