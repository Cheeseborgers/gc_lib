#include <stddef.h>
#include <stdio.h>

#define GC_STRIP_PREFIX
#include "gc_dyn_array.h"

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

int main(void)
{
    Ints nums = {0};
    Floats data = {0};

    for (int i = 0; i < 100; ++i) {
        da_append(&nums, i * 10);
        da_append(&data, (float)i * 1.5f);
    }

    printf("nums.count = %zu cap = %zu\n", nums.count, nums.capacity);
    printf("data.count = %zu cap = %zu\n", data.count, data.capacity);

    da_foreach(int, n, &nums)
    {
        size_t index = n - nums.items;
        printf("[%zu] %d, %.2f\n", index, *n, data.items[index]);
    }

    da_free(nums);
    da_free(data);
    return 0;
}
