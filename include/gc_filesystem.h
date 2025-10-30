#ifndef GC_FILESYSTEM_H_
#define GC_FILESYSTEM_H_

#include <stdio.h>
#include <stdbool.h>

#include "gc_common.h"
#include "gc_string_builder.h"

/*
 * Read the entire file into a String_Builder.
 * Returns true if successful, false otherwise.
 * The String_Builder will contain the raw bytes of the file.
 */
GCDEF bool gc_read_entire_file(const char *filepath, String_Builder *sb) {
    FILE *file = fopen(filepath, "rb"); // open in binary mode for cross-platform
    if (!file) return false;

    char buffer[4096];
    size_t n;
    while ((n = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        gc_sb_append_buf(sb, buffer, n);
    }

    if (ferror(file)) {
        fclose(file);
        return false;
    }

    fclose(file);

    // Optional: null-terminate the buffer for convenience
    gc_sb_append_char(sb, '\0');
    sb->count--; // don't count the null terminator as part of length

    return true;
}

#endif // GC_FILESYSTEM_H_

#ifndef GC_FILESYSTEM_STRIP_PREFIX_GUARD_
#define GC_STRING_BUILDER_STRIP_PREFIX_GUARD_
#ifdef GC_STRIP_PREFIX
#define read_entire_file gc_read_entire_file
#endif // GC_STRIP_PREFIX
#endif // GC_FILESYSTEM_STRIP_PREFIX_GUARD_
