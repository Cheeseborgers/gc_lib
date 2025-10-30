#include <stdio.h>

#define GC_STRIP_PREFIX
#include "gc_dyn_array.h"
#include "gc_string_builder.h"
#include "gc_string_view.h"

int main(void)
{
    // 1. Initialize the builder
    String_Builder sb = {0}; // zero-initialize count, capacity, items

    // 2. Append C strings
    sb_append_cstr(&sb, "Hello");
    sb_append_cstr(&sb, ", ");

    // 3. Append formatted text
    sb_appendf(&sb, "world %d!", 2025);

    // 4. Convert to null-terminated C string
    const char *result = sb_to_cstr(&sb);
    printf("%s\n", result); // Output: Hello, world 2025!

    String_View sv = sv_from_cstr(result);
    printf("Name: " SV_Fmt "\n", SV_Arg(sv));

    // 5. Free resources
    sb_free(&sb);

    return 0;
}
