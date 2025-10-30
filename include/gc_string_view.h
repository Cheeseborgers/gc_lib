#ifndef GC_STRING_VIEW_H_
#define GC_STRING_VIEW_H_

#include <ctype.h>
#include <stdbool.h>
#include <string.h>

/* -----------------------------------------------------------
 * String_View — a lightweight, non-owning view into a string.
 *
 * Inspired by Tsoding's `nob.h` string view:
 * https://github.com/tsoding/nob.h/
 *
 * Features:
 *   - Non-owning: does not allocate or free memory.
 *   - Easy slicing, chopping, and trimming.
 *   - Comparison and prefix/suffix checks.
 *   - Compatible with printf via SV_Fmt and SV_Arg macros.
 *
 * Example usage:
 *
 *   #define GC_STRIP_PREFIX
 *   #include "gc_string_view.h"
 *
 *   int main(void) {
 *       String_View sv = gc_sv_from_cstr("  hello world  ");
 *       String_View trimmed = gc_sv_trim(sv);
 *       printf("Trimmed: "SV_Fmt"\n", SV_Arg(trimmed));
 *
 *       String_View first_word = gc_sv_chop_by_delim(&trimmed, ' ');
 *       printf("First word: "SV_Fmt"\n", SV_Arg(first_word));
 *
 *       if (gc_sv_starts_with(first_word, gc_sv_from_cstr("hello"))) {
 *           puts("Starts with 'hello'!");
 *       }
 *   }
 *
 * Optional: define GCDEF to control linkage and inlining:
 *   #define GCDEF static inline
 *   #include "gc_string_view.h"
 * --------------------------------------------------------- */
#ifndef GCDEF
#define GCDEF static inline
#endif

/* -------------------- printf helpers -------------------- */
// Print String_View using standard printf
#ifndef SV_Fmt
#define SV_Fmt "%.*s"
#endif
#ifndef SV_Arg
#define SV_Arg(sv) (int)(sv).count, (sv).data
#endif

/* ---------------------- String_View type ---------------------- */
typedef struct {
    const char *data;   // pointer to the first character
    size_t count;       // number of characters in the view
} String_View;

/* -------------------- Core operations -------------------- */
GCDEF String_View gc_sv_from_parts(const char *data, size_t count)
{
    String_View sv;
    sv.data = data;
    sv.count = count;
    return sv;
}

GCDEF String_View gc_sv_from_cstr(const char *cstr)
{
    return gc_sv_from_parts(cstr, strlen(cstr));
}

GCDEF char *sv_to_cstr(String_View sv) {
    char *buf = malloc(sv.count + 1);
    memcpy(buf, sv.data, sv.count);
    buf[sv.count] = 0;
    return buf;
}

GCDEF String_View gc_sv_chop_by_delim(String_View *sv, char delim)
{
    size_t i = 0;
    while (i < sv->count && sv->data[i] != delim) i++;
    String_View result = gc_sv_from_parts(sv->data, i);

    if (i < sv->count) {
        sv->count -= i + 1;
        sv->data  += i + 1;
    } else {
        sv->count -= i;
        sv->data  += i;
    }

    return result;
}

GCDEF String_View gc_sv_chop_by_space(String_View *sv)
{
    size_t i = 0;
    while (i < sv->count && !isspace(sv->data[i])) {
        i += 1;
    }
    String_View result = gc_sv_from_parts(sv->data, i);

    if (i < sv->count) {
        sv->count -= i + 1;
        sv->data  += i + 1;
    } else {
        sv->count -= i;
        sv->data  += i;
    }

    return result;
}

GCDEF String_View gc_sv_chop_left(String_View *sv, size_t n)
{
    if (n > sv->count) n = sv->count;
    String_View result = gc_sv_from_parts(sv->data, n);
    sv->data  += n;
    sv->count -= n;
    return result;
}

GCDEF String_View gc_sv_trim_left(String_View sv)
{
    size_t i = 0;
    while (i < sv.count && isspace(sv.data[i])) i++;
    return gc_sv_from_parts(sv.data + i, sv.count - i);
}

GCDEF String_View gc_sv_trim_right(String_View sv)
{
    size_t i = 0;
    while (i < sv.count && isspace(sv.data[sv.count - 1 - i])) i++;
    return gc_sv_from_parts(sv.data, sv.count - i);
}

GCDEF String_View gc_sv_trim(String_View sv)
{
    return gc_sv_trim_right(gc_sv_trim_left(sv));
}

/* -------------------- Comparison / inspection -------------------- */
GCDEF bool gc_sv_eq(String_View a, String_View b)
{
    if (a.count != b.count) return false;
    return memcmp(a.data, b.data, a.count) == 0;
}

GCDEF bool gc_sv_starts_with(String_View sv, String_View prefix)
{
    if (prefix.count > sv.count) return false;
    return gc_sv_eq(gc_sv_from_parts(sv.data, prefix.count), prefix);
}

GCDEF bool gc_sv_end_with(String_View sv, const char *cstr)
{
    size_t cstr_count = strlen(cstr);
    if (sv.count < cstr_count) return false;
    size_t start = sv.count - cstr_count;
    return gc_sv_eq(gc_sv_from_parts(sv.data + start, cstr_count),
                    gc_sv_from_cstr(cstr));
}

/* -------------------- Strip prefix macros -------------------- */
#endif // GC_STRING_VIEW_H_

#ifndef GC_SV_STRIP_PREFIX_GUARD_
#define GC_SV_STRIP_PREFIX_GUARD_
#ifdef GC_STRIP_PREFIX
#define sv_from_parts gc_sv_from_parts
#define sv_from_cstr gc_sv_from_cstr
#define sv_chop_by_delim gc_sv_chop_by_delim
#define sv_chop_by_space gc_sv_chop_by_space
#define sv_chop_left gc_sv_chop_left
#define sv_trim_left gc_sv_trim_left
#define sv_trim_right gc_sv_trim_right
#define sv_trim gc_sv_trim
#define sv_eq gc_sv_eq
#define sv_starts_with gc_sv_starts_with
#define sv_end_with gc_sv_end_with
#endif
#endif
