#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define GC_STRIP_PREFIX
#include "../gc_string_view.h"
#include "../gc_hash_table.h"
#include "../gc_string_builder.h"
#include "../gc_filesystem.h"
#include "../gc_logger.h"
#include "../gc_time.h"

// Type-safe string->size_t hash
GC_HASH_DEFINE_SV(str_count, size_t)

typedef struct {
    char *key;
    size_t value;
} FreqKV;

int compare_freq_desc(const void *a, const void *b) {
    const FreqKV *ka = a;
    const FreqKV *kb = b;
    if (kb->value > ka->value) return 1;
    if (kb->value < ka->value) return -1;
    return 0;
}

int main(void) {
    const char *file_path = "t8.shakespeare.txt";
    String_Builder buf = {0};

    GC_LOG_INFO("Analysing %s", file_path);

    if (!read_entire_file(file_path, &buf)) {
        sb_free(&buf);
        return 1;
    }

    String_View content = { buf.items, buf.count };

    GC_LOG_INFO("Size: %zu bytes", content.count);

    struct timespec begin = get_time_monotonic();

    gc_hash_str_count str_ht;
    gc_hash_str_count_init(&str_ht, 16);

    // Tokenize and count
    while (content.count > 0) {
        String_View token = sv_chop_by_space(&content);
        if (token.count == 0) continue;

        size_t freq;
        if (gc_hash_str_count_get(&str_ht, token, &freq)) {
            gc_hash_str_count_set(&str_ht, token, freq + 1);
        } else {
            gc_hash_str_count_set(&str_ht, token, 1);
        }
    }

    // collect for sorting
    size_t num_items = 0;
    for (size_t i = 0; i < str_ht.base.capacity; i++) {
        if (str_ht.base.items[i].state == GC_SLOT_OCCUPIED) num_items++;
    }

    FreqKV *freqs = malloc(num_items * sizeof(FreqKV));
    size_t idx = 0;
    for (size_t i = 0; i < str_ht.base.capacity; i++) {
        if (str_ht.base.items[i].state == GC_SLOT_OCCUPIED) {
            freqs[idx++] = (FreqKV){
                .key = str_ht.base.items[i].key,
                .value = *(size_t*)str_ht.base.items[i].value
            };
        }
    }

    qsort(freqs, num_items, sizeof(FreqKV), compare_freq_desc);

    struct timespec end = get_time_monotonic();

    size_t words_to_print = 10;
    printf("Top words:\n");
    for (size_t i = 0; i < num_items && i < words_to_print; i++)
        printf("  %zu: %s => %zu\n", i, freqs[i].key, freqs[i].value);

    GC_LOG_INFO("Elapsed time: %lfs", delta_secs(begin, end));

    free(freqs);
    gc_hash_str_count_free(&str_ht);
    return 0;
}