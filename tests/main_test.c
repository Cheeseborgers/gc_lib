#include "gc_hash_table.h"
#include <stdio.h>



int main(void) {
    GC_HashTable ht;
    gc_hash_init_string(&ht, 8);

    gc_hash_insert(&ht, "apple", "red", 0);
    gc_hash_insert(&ht, "banana", "yellow", 0);
    gc_hash_insert(&ht, "cherry", "red", 0);
    gc_hash_insert(&ht, "date", "brown", 0);

    printf("Before deletion:\n");
    GC_HashIter it;
    gc_hash_iter_init(&it, &ht);
    while (gc_hash_iter_next(&it)) {
        printf("  %s -> %s\n", (char*)it.entry->key, (char*)it.entry->value);
    }

    printf("\nDeleting entries with value == \"red\"...\n");
    gc_hash_iter_init(&it, &ht);
    while (gc_hash_iter_next(&it)) {
        if (strcmp((char*)it.entry->value, "red") == 0) {
            gc_hash_iter_remove(&it);
        }
    }

    printf("\nAfter deletion:\n");
    gc_hash_iter_init(&it, &ht);
    while (gc_hash_iter_next(&it)) {
        printf("  %s -> %s\n", (char*)it.entry->key, (char*)it.entry->value);
    }

    gc_hash_free(&ht);
    return 0;
}

/*
#define GC_STRIP_PREFIX
#include "gc_dyn_array.h"
#include "gc_filesystem.h"
#include "gc_hash_table.h"
#include "gc_logger.h"
#include "gc_string_builder.h"
#include "gc_string_view.h"
#include "gc_time.h"

typedef struct {
  String_View key;
  size_t value;
  bool occupied;
} FreqKV;

typedef struct {
  FreqKV *items;
  size_t count;
  size_t capacity;
} FreqKVs;

int compare_freqkv_count_ascending(const void *a, const void *b) {
  const FreqKV *akv = a;
  const FreqKV *bkv = b;
  return (int)bkv->value - (int)akv->value;
}

bool gc_hash_analysis(String_View content, const char *file_path) {
  GC_LOG_INFO("Analysing %s", file_path);
  GC_LOG_INFO("Size: %zu bytes", content.count);

  struct timespec begin = get_time_monotonic();

  HashTable ht;
  gc_hash_init(&ht, 100000, NULL);

  size_t count = 0;
  while (content.count > 0) {
    content = sv_trim_left(content);
    String_View token = sv_chop_by_space(&content);
    gc_hash_inc(&ht, token);
    count++;
  }

  // collect into array for sorting
  FreqKVs freq = {0};
  for (size_t i = 0; i < ht.capacity; ++i)
    if (ht.items[i].occupied) {
      da_append(&freq,
                ((FreqKV){
                    .key = ht.items[i].key,
                    .value = ht.items[i].value, // copy the real frequency
                }));
    }

  qsort(freq.items, freq.count, sizeof(freq.items[0]), compare_freqkv_count_ascending);

  struct timespec end = get_time_monotonic();

  GC_LOG_INFO("Top 10 tokens:");
  for (size_t i = 0; i < 10 && i < freq.count; ++i) {
    GC_LOG_INFO("     %zu: " SV_Fmt " => %zu", i, SV_Arg(freq.items[i].key),
                freq.items[i].value);
  }

  GC_LOG_INFO("Elapsed time: %lfs", delta_secs(begin, end));

  gc_hash_free(&ht);
  da_free(freq);
  return true;
}

int main(void) {
  const char *file_path = "t8.shakespeare.txt";
  String_Builder buf = {0};

  if (!read_entire_file(file_path, &buf)) {
    sb_free(&buf);
    return 1;
  }

  String_View content = {.data = buf.items, .count = buf.count};

  if (!gc_hash_analysis(content, file_path)) {
    sb_free(&buf);
    return 1;
  }

  sb_free(&buf);
  return 0;
}
*/

/*
typedef struct {
    int *items;
    size_t count;
    size_t capacity;
} Ints;

typedef struct {
    float *items;
    size_t count;
    size_t capacity;
} Floats;
int main3(void) {
    // 1. Initialize the builder
    String_Builder sb = {0};  // zero-initialize count, capacity, items

    // 2. Append C strings
    sb_append_cstr(&sb, "Hello");
    sb_append_cstr(&sb, ", ");

    // 3. Append formatted text
    sb_appendf(&sb, "world %d!", 2025);

    // 4. Convert to null-terminated C string
    const char *result = sb_to_cstr(&sb);
    printf("%s\n", result);  // Output: Hello, world 2025!


    String_View sv = sv_from_cstr(result);
    printf("Name: "SV_Fmt"\n", SV_Arg(sv));

    // 5. Free resources
    sb_free(&sb);

    return 0;
}

int main2(void) {
    Ints nums = {0};
    Floats data = {0};

    for (int i = 0; i < 100; ++i) {
        da_append(&nums, i * 10);
        da_append(&data, (float)i * 1.5f);
    }

    printf("nums.count = %zu cap = %zu\n", nums.count, nums.capacity);
    printf("data.count = %zu cap = %zu\n", data.count, data.capacity);

    da_foreach(int, n, &nums) {
        size_t index = n - nums.items;
        printf("[%zu] %d, %.2f\n", index, *n, data.items[index]);
    }

    da_free(nums);
    da_free(data);
    return 0;
}
*/
