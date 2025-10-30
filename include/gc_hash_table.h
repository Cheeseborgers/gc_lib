#ifndef GC_HASH_TABLE_H_
#define GC_HASH_TABLE_H_

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gc_common.h"

/* ===========================================================================
 * GC_HASH_TABLE_H — A flexible, type-safe hash table library for C.
 *
 * Supports:
 *   1. Heap-allocated string keys (char* or String_View)
 *   2. Integer keys stored on the heap
 *   3. Inline integer keys (fully stored in the table)
 *   4. Any value type (via template macros)
 *
 * Features:
 *   - Open addressing with linear probing
 *   - Tombstone handling for removals
 *   - Automatic growth/shrink based on load factor
 *   - Custom hash and equality functions
 *   - Type-safe macros for simple usage
 *
 * Optional: define GCDEF to control linkage and inlining:
 *   #define GCDEF static inline
 *
 * ---------------------------------------------------------------------------
 * Example usage:
 *
 *   // ---------------- Heap string keys ----------------
 *   static inline void *copy_str(char *s) { return strdup(s); }
 *   static inline void free_str(void *s) { free(s); }
 *   GC_HASH_DEFINE_STR(str_count, size_t, copy_str, free_str)
 *
 *   gc_hash_str_count ht;
 *   gc_hash_str_count_init(&ht, 16);
 *   gc_hash_str_count_set(&ht, strdup("hello"), 1);
 *   size_t val;
 *   if (gc_hash_str_count_get(&ht, "hello", &val)) { printf("%zu\n", val); }
 *   gc_hash_str_count_free(&ht);
 *
 *   // ---------------- String_View keys ----------------
 *   #include "gc_string_view.h"
 *   GC_HASH_DEFINE_SV(sv_count, size_t)
 *
 *   gc_hash_sv_count ht2;
 *   gc_hash_sv_count_init(&ht2, 16);
 *   String_View sv = gc_sv_from_cstr("world");
 *   gc_hash_sv_count_set(&ht2, sv, 42);
 *   size_t val2;
 *   if (gc_hash_sv_count_get(&ht2, sv, &val2)) { printf("%zu\n", val2); }
 *   gc_hash_sv_count_free(&ht2);
 *
 *   // ---------------- Heap integer keys ----------------
 *   GC_HASH_DEFINE_INT(int_count, size_t)
 *
 *   gc_hash_int_count ht3;
 *   gc_hash_int_count_init(&ht3, 16);
 *   gc_hash_int_count_set(&ht3, 42, 99);
 *   size_t val3;
 *   if (gc_hash_int_count_get(&ht3, 42, &val3)) { printf("%zu\n", val3); }
 *   gc_hash_int_count_free(&ht3);
 *
 *   // ---------------- Inline integer keys ----------------
 *   GC_HASH_DEFINE_INLINE(inline_count, size_t)
 *
 *   gc_hash_inline_count ht4;
 *   gc_hash_inline_count_init(&ht4, 16);
 *   gc_hash_inline_count_set(&ht4, 7, 123);
 *   size_t val4;
 *   if (gc_hash_inline_count_get(&ht4, 7, &val4)) { printf("%zu\n", val4); }
 *   gc_hash_inline_count_free(&ht4);
 *
 *   Note: See bottom of file for more examples, cheatsheet and tips
 *
 * ========================================================================== */

// ======================== BASIC TYPES ==========================

/* Represents the state of a hash table slot */
typedef enum {
  GC_SLOT_EMPTY,       // No key/value here
  GC_SLOT_OCCUPIED,    // Slot contains a valid key/value
  GC_SLOT_TOMBSTONE    // Slot was occupied but deleted
} GC_HashSlotState;

/* Represents one key/value entry in the table */
typedef struct {
  void *key;
  void *value;
  GC_HashSlotState state;
} GC_HashEntry;

/* Hash function type (returns uint32_t hash for a given key) */
typedef uint32_t (*gc_hash_fn)(const void *key, size_t len);

/* Equality function type for comparing keys */
typedef bool (*gc_eq_fn)(const void *a, const void *b, size_t len);
typedef struct {
  GC_HashEntry *items;  // Array of entries
  size_t count;         // Number of occupied slots
  size_t capacity;      // Total slots in array
  gc_hash_fn hash_func; // Function to hash keys
  gc_eq_fn eq_func;     // Function to compare keys
  size_t key_size;      // Size of fixed-size keys (0 for variable length)
} GC_HashTable;

#define GC_HASH_MIN_CAPACITY 16U
#define GC_HASH_MAX_LOAD_FACTOR 0.60  // Grow table if load > 60%

// ======================== HASH FUNCTIONS =======================
/* djb2 string hash (null-terminated or fixed-length) */
GCDEF uint32_t gc_hash_djb2(const void *key, size_t len) {
  const unsigned char *str = key;
  uint32_t hash = 5381;
  for (size_t i = 0; len == 0 ? str[i] != 0 : i < len; i++)
    hash = ((hash << 5) + hash) + str[i];
  return hash;
}

/* 32-bit integer hash with good avalanche */
GCDEF uint32_t gc_hash_int32(const void *key, size_t len) {
  (void)len;
  uint32_t v = *(const uint32_t *)key;
  v ^= v >> 17;
  v *= 0xed5ad4bb;
  v ^= v >> 11;
  v *= 0xac4c1b51;
  v ^= v >> 15;
  v *= 0x31848bab;
  v ^= v >> 14;
  return v;
}


/* String equality comparison */
GCDEF bool gc_eq_string(const void *a, const void *b, size_t len) {
  (void)len;
  return strcmp(a, b) == 0;
}

/* Integer equality comparison */
GCDEF bool gc_eq_int32(const void *a, const void *b, size_t len) {
  (void)len;
  return *(const int32_t *)a == *(const int32_t *)b;
}

// ======================== INTERNAL HELPERS ====================

/* Ensure capacity is at least minimum */
GCDEF size_t gc_hash_normalize_capacity(size_t cap) {
  if (cap < GC_HASH_MIN_CAPACITY)
    cap = GC_HASH_MIN_CAPACITY;
  return cap;
}

/* Forward declarations for growth/shrink */
GCDEF void gc_hash_ensure_growth(GC_HashTable *ht);
GCDEF void gc_hash_ensure_shrink(GC_HashTable *ht);
GCDEF void gc_hash_resize(GC_HashTable *ht, size_t new_capacity);

// ======================== CORE API ============================

/* Initialize a generic hash table */
GCDEF void gc_hash_init(GC_HashTable *ht, size_t capacity,
                                gc_hash_fn hash_func, gc_eq_fn eq_func,
                                size_t key_size) {
  capacity = gc_hash_normalize_capacity(capacity);
  ht->items = calloc(capacity, sizeof(GC_HashEntry));
  ht->count = 0;
  ht->capacity = capacity;
  ht->hash_func = hash_func ? hash_func : gc_hash_djb2;
  ht->eq_func = eq_func;
  ht->key_size = key_size;
}

/* Free all keys/values and reset table */
GCDEF void gc_hash_free(GC_HashTable *ht) {
  if (!ht)
    return;
  for (size_t i = 0; i < ht->capacity; i++) {
    if (ht->items[i].state == GC_SLOT_OCCUPIED) {
      free(ht->items[i].key);
      free(ht->items[i].value);
    }
  }
  free(ht->items);
  ht->items = NULL;
  ht->count = 0;
  ht->capacity = 0;
}

/* Get load factor (0.0–1.0) */
GCDEF double gc_hash_load_factor(const GC_HashTable *ht) {
  return ht->capacity ? (double)ht->count / (double)ht->capacity : 0.0;
}

// ----------------- INSERT / FIND -----------------------------
/* Insert key/value (overwrites if key exists) */
GCDEF void gc_hash_insert(GC_HashTable *ht, void *key, void *value,
                                  size_t key_len) {
  if (ht->capacity == 0)
    gc_hash_init(ht, GC_HASH_MIN_CAPACITY, ht->hash_func, ht->eq_func,
                 ht->key_size);
  gc_hash_ensure_growth(ht);
  if (!key_len)
    key_len = ht->key_size;

  uint32_t h = ht->hash_func(key, key_len) % ht->capacity;
  ssize_t first_tombstone = -1;

  for (;;) {
    GC_HashEntry *e = &ht->items[h];
    if (e->state == GC_SLOT_EMPTY) {
      if (first_tombstone >= 0)
        e = &ht->items[first_tombstone];
      e->key = key;
      e->value = value;
      e->state = GC_SLOT_OCCUPIED;
      ht->count++;
      return;
    }
    if (e->state == GC_SLOT_TOMBSTONE && first_tombstone < 0)
      first_tombstone = h;
    else if (e->state == GC_SLOT_OCCUPIED &&
             ht->eq_func(e->key, key, key_len)) {
      free(e->value);
      e->value = value;
      free(key);
      return;
    }
    h = (h + 1) % ht->capacity;
  }
}

/* Find entry for a key */
GCDEF GC_HashEntry *gc_hash_find(GC_HashTable *ht, const void *key,
                                         size_t key_len) {
  if (!ht || ht->capacity == 0)
    return NULL;
  if (!key_len)
    key_len = ht->key_size;

  uint32_t h = ht->hash_func(key, key_len) % ht->capacity;
  for (;;) {
    GC_HashEntry *e = &ht->items[h];
    if (e->state == GC_SLOT_EMPTY)
      return NULL;
    if (e->state == GC_SLOT_OCCUPIED && ht->eq_func(e->key, key, key_len))
      return e;
    h = (h + 1) % ht->capacity;
  }
}

/* Remove a key/value from table */
GCDEF bool gc_hash_remove(GC_HashTable *ht, const void *key,
                                  size_t key_len) {
  if (!ht || ht->capacity == 0)
    return false;
  if (!key_len)
    key_len = ht->key_size;

  uint32_t h = ht->hash_func(key, key_len) % ht->capacity;
  for (;;) {
    GC_HashEntry *e = &ht->items[h];
    if (e->state == GC_SLOT_EMPTY)
      return false;
    if (e->state == GC_SLOT_OCCUPIED && ht->eq_func(e->key, key, key_len)) {
      free(e->key);
      free(e->value);
      e->key = NULL;
      e->value = NULL;
      e->state = GC_SLOT_TOMBSTONE;
      ht->count--;
      gc_hash_ensure_shrink(ht);
      return true;
    }
    h = (h + 1) % ht->capacity;
  }
}

// ----------------- RESIZE HELPERS ----------------------------

GCDEF void gc_hash_ensure_growth(GC_HashTable *ht) {
  if (gc_hash_load_factor(ht) >= GC_HASH_MAX_LOAD_FACTOR)
    gc_hash_resize(ht, ht->capacity * 2);
}

GCDEF void gc_hash_ensure_shrink(GC_HashTable *ht) {
  if (gc_hash_load_factor(ht) < 0.1 && ht->capacity > GC_HASH_MIN_CAPACITY) {
    size_t new_cap = ht->capacity / 2;
    if (new_cap < GC_HASH_MIN_CAPACITY)
      new_cap = GC_HASH_MIN_CAPACITY;
    gc_hash_resize(ht, new_cap);
  }
}

GCDEF void gc_hash_resize(GC_HashTable *ht, size_t new_capacity) {
  GC_HashEntry *old_items = ht->items;
  size_t old_capacity = ht->capacity;

  gc_hash_init(ht, new_capacity, ht->hash_func, ht->eq_func, ht->key_size);

  for (size_t i = 0; i < old_capacity; i++) {
    if (old_items[i].state == GC_SLOT_OCCUPIED) {
      gc_hash_insert(ht, old_items[i].key, old_items[i].value, ht->key_size);
    }
  }
  free(old_items);
}

// ======================== TYPE-SAFE GENERATORS ===================

// ---------------- String key (heap-owned char*) -----------------
#define GC_HASH_DEFINE_STR(NAME, VALUE_TYPE, KEY_COPY_FN, KEY_FREE_FN)         \
  typedef struct {                                                             \
    GC_HashTable base;                                                         \
  } gc_hash_##NAME;                                                            \
  static inline void gc_hash_##NAME##_init(gc_hash_##NAME *ht,                 \
                                           size_t capacity) {                  \
    gc_hash_init(&ht->base, capacity, gc_hash_djb2, gc_eq_string, 0);          \
  }                                                                            \
  static inline void gc_hash_##NAME##_set(gc_hash_##NAME *ht, char *key,       \
                                          VALUE_TYPE val) {                    \
    void *kcopy = KEY_COPY_FN(key);                                            \
    VALUE_TYPE *vstore = malloc(sizeof(VALUE_TYPE));                           \
    *vstore = val;                                                             \
    gc_hash_insert(&ht->base, kcopy, vstore, 0);                               \
  }                                                                            \
  static inline bool gc_hash_##NAME##_get(gc_hash_##NAME *ht, char *key,       \
                                          VALUE_TYPE *out) {                   \
    GC_HashEntry *e = gc_hash_find(&ht->base, key, 0);                         \
    if (!e)                                                                    \
      return false;                                                            \
    *out = *(VALUE_TYPE *)e->value;                                            \
    return true;                                                               \
  }                                                                            \
  static inline bool gc_hash_##NAME##_remove(gc_hash_##NAME *ht, char *key) {  \
    return gc_hash_remove(&ht->base, key, 0);                                  \
  }                                                                            \
  static inline void gc_hash_##NAME##_free(gc_hash_##NAME *ht) {               \
    gc_hash_free(&ht->base);                                                   \
  }

// --------------- String_View key (table owns heap copy) ----------------
#define GC_HASH_DEFINE_SV(NAME, VALUE_TYPE)                                    \
  typedef struct {                                                             \
    GC_HashTable base;                                                         \
  } gc_hash_##NAME;                                                            \
                                                                               \
  static inline void gc_hash_##NAME##_init(gc_hash_##NAME *ht, size_t cap) {   \
    gc_hash_init(&ht->base, cap, gc_hash_djb2, gc_eq_string, 0);               \
  }                                                                            \
                                                                               \
  static inline void gc_hash_##NAME##_set(gc_hash_##NAME *ht, String_View key, \
                                          VALUE_TYPE val) {                    \
    char *kcopy = sv_to_cstr(key);                                             \
    VALUE_TYPE *vstore = malloc(sizeof(VALUE_TYPE));                           \
    *vstore = val;                                                             \
    gc_hash_insert(&ht->base, kcopy, vstore, 0);                               \
  }                                                                            \
                                                                               \
  static inline bool gc_hash_##NAME##_get(gc_hash_##NAME *ht, String_View key, \
                                          VALUE_TYPE *out) {                   \
    char *kcopy = sv_to_cstr(key);                                             \
    GC_HashEntry *e = gc_hash_find(&ht->base, kcopy, 0);                       \
    free(kcopy);                                                               \
    if (!e)                                                                    \
      return false;                                                            \
    *out = *(VALUE_TYPE *)e->value;                                            \
    return true;                                                               \
  }                                                                            \
                                                                               \
  static inline bool gc_hash_##NAME##_remove(gc_hash_##NAME *ht,               \
                                             String_View key) {                \
    char *kcopy = sv_to_cstr(key);                                             \
    bool res = gc_hash_remove(&ht->base, kcopy, 0);                            \
    free(kcopy);                                                               \
    return res;                                                                \
  }                                                                            \
                                                                               \
  static inline void gc_hash_##NAME##_free(gc_hash_##NAME *ht) {               \
    gc_hash_free(&ht->base);                                                   \
  }

// ---------------- Inline integer keys (e.g., socket FDs) ----------------
#define GC_HASH_DEFINE_INT(NAME, VALUE_TYPE)                                   \
  typedef struct {                                                             \
    int key;                                                                   \
    VALUE_TYPE value;                                                          \
    GC_HashSlotState state;                                                    \
  } gc_hash_##NAME##_entry;                                                    \
  typedef struct {                                                             \
    gc_hash_##NAME##_entry *items;                                             \
    size_t count;                                                              \
    size_t capacity;                                                           \
  } gc_hash_##NAME;                                                            \
  static inline void gc_hash_##NAME##_init(gc_hash_##NAME *ht,                 \
                                           size_t capacity) {                  \
    capacity = gc_hash_normalize_capacity(capacity);                           \
    ht->items = calloc(capacity, sizeof(gc_hash_##NAME##_entry));              \
    ht->count = 0;                                                             \
    ht->capacity = capacity;                                                   \
  }                                                                            \
  static inline void gc_hash_##NAME##_free(gc_hash_##NAME *ht) {               \
    free(ht->items);                                                           \
    ht->items = NULL;                                                          \
    ht->count = 0;                                                             \
    ht->capacity = 0;                                                          \
  }                                                                            \
  static inline void gc_hash_##NAME##_set(gc_hash_##NAME *ht, int key,         \
                                          VALUE_TYPE val) {                    \
    if (ht->capacity == 0)                                                     \
      gc_hash_##NAME##_init(ht, 16);                                           \
    size_t h = gc_hash_int32(&key, sizeof(int)) % ht->capacity;                \
    size_t first_tombstone = SIZE_MAX;                                         \
    for (;;) {                                                                 \
      gc_hash_##NAME##_entry *e = &ht->items[h];                               \
      if (e->state == GC_SLOT_EMPTY) {                                         \
        if (first_tombstone != SIZE_MAX)                                       \
          e = &ht->items[first_tombstone];                                     \
        e->key = key;                                                          \
        e->value = val;                                                        \
        e->state = GC_SLOT_OCCUPIED;                                           \
        ht->count++;                                                           \
        return;                                                                \
      }                                                                        \
      if (e->state == GC_SLOT_TOMBSTONE && first_tombstone == SIZE_MAX)        \
        first_tombstone = h;                                                   \
      else if (e->state == GC_SLOT_OCCUPIED && e->key == key) {                \
        e->value = val;                                                        \
        return;                                                                \
      }                                                                        \
      h = (h + 1) % ht->capacity;                                              \
    }                                                                          \
  }                                                                            \
  static inline bool gc_hash_##NAME##_get(gc_hash_##NAME *ht, int key,         \
                                          VALUE_TYPE *out) {                   \
    if (ht->capacity == 0)                                                     \
      return false;                                                            \
    size_t h = gc_hash_int32(&key, sizeof(int)) % ht->capacity;                \
    for (;;) {                                                                 \
      gc_hash_##NAME##_entry *e = &ht->items[h];                               \
      if (e->state == GC_SLOT_EMPTY)                                           \
        return false;                                                          \
      if (e->state == GC_SLOT_OCCUPIED && e->key == key) {                     \
        *out = e->value;                                                       \
        return true;                                                           \
      }                                                                        \
      h = (h + 1) % ht->capacity;                                              \
    }                                                                          \
  }                                                                            \
  static inline bool gc_hash_##NAME##_remove(gc_hash_##NAME *ht, int key) {    \
    if (ht->capacity == 0)                                                     \
      return false;                                                            \
    size_t h = gc_hash_int32(&key, sizeof(int)) % ht->capacity;                \
    for (;;) {                                                                 \
      gc_hash_##NAME##_entry *e = &ht->items[h];                               \
      if (e->state == GC_SLOT_EMPTY)                                           \
        return false;                                                          \
      if (e->state == GC_SLOT_OCCUPIED && e->key == key) {                     \
        e->state = GC_SLOT_TOMBSTONE;                                          \
        ht->count--;                                                           \
        return true;                                                           \
      }                                                                        \
      h = (h + 1) % ht->capacity;                                              \
    }                                                                          \
  }

// ---------------- Fully inline integer keys + values ----------------
#define GC_HASH_DEFINE_INLINE(NAME, VALUE_TYPE)                                \
  typedef struct {                                                             \
    int key;                                                                   \
    VALUE_TYPE value;                                                          \
    GC_HashSlotState state;                                                    \
  } gc_hash_##NAME##_entry;                                                    \
  typedef struct {                                                             \
    gc_hash_##NAME##_entry *items;                                             \
    size_t count;                                                              \
    size_t capacity;                                                           \
  } gc_hash_##NAME;                                                            \
  static inline void gc_hash_##NAME##_init(gc_hash_##NAME *ht,                 \
                                           size_t capacity) {                  \
    capacity = gc_hash_normalize_capacity(capacity);                           \
    ht->items = calloc(capacity, sizeof(gc_hash_##NAME##_entry));              \
    ht->count = 0;                                                             \
    ht->capacity = capacity;                                                   \
  }                                                                            \
  static inline void gc_hash_##NAME##_free(gc_hash_##NAME *ht) {               \
    free(ht->items);                                                           \
    ht->items = NULL;                                                          \
    ht->count = 0;                                                             \
    ht->capacity = 0;                                                          \
  }                                                                            \
  static inline void gc_hash_##NAME##_set(gc_hash_##NAME *ht, int key,         \
                                          VALUE_TYPE val) {                    \
    if (ht->capacity == 0)                                                     \
      gc_hash_##NAME##_init(ht, 16);                                           \
    size_t h = gc_hash_int32(&key, sizeof(int)) % ht->capacity;                \
    size_t first_tombstone = SIZE_MAX;                                         \
    for (;;) {                                                                 \
      gc_hash_##NAME##_entry *e = &ht->items[h];                               \
      if (e->state == GC_SLOT_EMPTY) {                                         \
        if (first_tombstone != SIZE_MAX)                                       \
          e = &ht->items[first_tombstone];                                     \
        e->key = key;                                                          \
        e->value = val;                                                        \
        e->state = GC_SLOT_OCCUPIED;                                           \
        ht->count++;                                                           \
        return;                                                                \
      }                                                                        \
      if (e->state == GC_SLOT_TOMBSTONE && first_tombstone == SIZE_MAX)        \
        first_tombstone = h;                                                   \
      else if (e->state == GC_SLOT_OCCUPIED && e->key == key) {                \
        e->value = val;                                                        \
        return;                                                                \
      }                                                                        \
      h = (h + 1) % ht->capacity;                                              \
    }                                                                          \
  }                                                                            \
  static inline bool gc_hash_##NAME##_get(gc_hash_##NAME *ht, int key,         \
                                          VALUE_TYPE *out) {                   \
    if (ht->capacity == 0)                                                     \
      return false;                                                            \
    size_t h = gc_hash_int32(&key, sizeof(int)) % ht->capacity;                \
    for (;;) {                                                                 \
      gc_hash_##NAME##_entry *e = &ht->items[h];                               \
      if (e->state == GC_SLOT_EMPTY)                                           \
        return false;                                                          \
      if (e->state == GC_SLOT_OCCUPIED && e->key == key) {                     \
        *out = e->value;                                                       \
        return true;                                                           \
      }                                                                        \
      h = (h + 1) % ht->capacity;                                              \
    }                                                                          \
  }                                                                            \
  static inline bool gc_hash_##NAME##_remove(gc_hash_##NAME *ht, int key) {    \
    if (ht->capacity == 0)                                                     \
      return false;                                                            \
    size_t h = gc_hash_int32(&key, sizeof(int)) % ht->capacity;                \
    for (;;) {                                                                 \
      gc_hash_##NAME##_entry *e = &ht->items[h];                               \
      if (e->state == GC_SLOT_EMPTY)                                           \
        return false;                                                          \
      if (e->state == GC_SLOT_OCCUPIED && e->key == key) {                     \
        e->state = GC_SLOT_TOMBSTONE;                                          \
        ht->count--;                                                           \
        return true;                                                           \
      }                                                                        \
      h = (h + 1) % ht->capacity;                                              \
    }                                                                          \
  }

#endif // GC_HASH_TABLE_H_

#ifndef GC_HASH_TABLE_STRIP_PREFIX_GUARD_
#define GC_HASH_TABLE_STRIP_PREFIX_GUARD_
#ifdef GC_STRIP_PREFIX
// String-keyed hash table aliases
#define str_count gc_hash_str_count

// Integer-keyed heap hash table aliases
#define int_table gc_hash_int_table

// Fully inline integer table aliases
#define inline_table gc_hash_inline_table
#endif
#endif

/* ===========================================================================
 * GC_HASH_TABLE QUICK CHEAT SHEET
 *
 * Example usage for each key type:
 *
 * ---------------------------------------------------------------------------
 * 1. Heap string key (char*)
 *
 *   static inline void *copy_str(char *s) { return strdup(s); }
 *   static inline void free_str(void *s) { free(s); }
 *   GC_HASH_DEFINE_STR(str_count, size_t, copy_str, free_str)
 *
 *   gc_hash_str_count ht;
 *   gc_hash_str_count_init(&ht, 16);
 *   gc_hash_str_count_set(&ht, strdup("hello"), 1);
 *   size_t val;
 *   gc_hash_str_count_get(&ht, "hello", &val);
 *   gc_hash_str_count_free(&ht);
 *
 * ---------------------------------------------------------------------------
 * 2. String_View key (table owns heap copy)
 *
 *   #include "gc_string_view.h"
 *   GC_HASH_DEFINE_SV(sv_count, size_t)
 *
 *   gc_hash_sv_count ht2;
 *   gc_hash_sv_count_init(&ht2, 16);
 *   String_View sv = gc_sv_from_cstr("world");
 *   gc_hash_sv_count_set(&ht2, sv, 42);
 *   size_t val2;
 *   gc_hash_sv_count_get(&ht2, sv, &val2);
 *   gc_hash_sv_count_free(&ht2);
 *
 * ---------------------------------------------------------------------------
 * 3. Heap integer key
 *
 *   GC_HASH_DEFINE_INT(int_count, size_t)
 *
 *   gc_hash_int_count ht3;
 *   gc_hash_int_count_init(&ht3, 16);
 *   gc_hash_int_count_set(&ht3, 42, 99);
 *   size_t val3;
 *   gc_hash_int_count_get(&ht3, 42, &val3);
 *   gc_hash_int_count_free(&ht3);
 *
 * ---------------------------------------------------------------------------
 * 4. Inline integer keys (fully stored in table)
 *
 *   GC_HASH_DEFINE_INLINE(inline_count, size_t)
 *
 *   gc_hash_inline_count ht4;
 *   gc_hash_inline_count_init(&ht4, 16);
 *   gc_hash_inline_count_set(&ht4, 7, 123);
 *   size_t val4;
 *   gc_hash_inline_count_get(&ht4, 7, &val4);
 *   gc_hash_inline_count_free(&ht4);
 *
 * ----------------------------------------------------------------------------
 * GC_HASH_TABLE COMMON PITFALLS & TIPS
 *
 * 1. Heap string keys (char*):
 *    - Always provide a copy function (e.g., strdup) and a free function.
 *    - The table takes ownership of the key and value.
 *    - Forgetting to free keys or values yourself is fine—the table handles it.
 *
 * 2. String_View keys:
 *    - These are lightweight, non-owning views into existing strings.
 *    - The table stores a heap copy internally, so you don’t need to free your
 *      original String_View buffer.
 *    - Be cautious: converting a String_View to char* (sv_to_cstr) always allocates.
 *
 * 3. Heap integer keys:
 *    - The table stores a pointer to the heap-allocated int key.
 *    - Don’t use stack variables as keys directly—they will be freed by the table.
 *    - Use this when you want consistent ownership semantics like strings.
 *
 * 4. Inline integer keys (fully stored in table):
 *    - Keys and values live entirely inside the table; no heap allocation.
 *    - Best for simple integer→value maps (e.g., file descriptors, IDs).
 *    - No need to free keys manually; only call *_free() to free the table array.
 *
 * 5. General tips:
 *    - Always call *_init() before using the table.
 *    - Always call *_free() when done to prevent leaks.
 *    - Access values via *_get() and *_set() for type safety.
 *    - Avoid modifying keys in place after insertion—they may break hashing.
 *    - Hash tables grow automatically; inline tables do not shrink automatically
 *      (except GC_HashTable based ones), so consider initial capacity.
 *
 * 6. Resizing & load factor:
 *    - Tables automatically grow when load factor ≥ 0.6.
 *    - Tables shrink when load factor < 0.1 (only for heap-based tables).
 *    - Inline tables do linear probing, so keep capacities reasonable for performance.
 *
 * ========================================================================== */


/*
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define GC_STRIP_PREFIX
#include "gc_string_view.h"
#include "gc_hash_table.h"
#include "gc_string_builder.h"
#include "gc_filesystem.h"
#include "gc_logger.h"
#include "gc_time.h"


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

// Second example

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#define GC_STRIP_PREFIX
#include "gc_string_view.h"
#include "gc_hash_table.h" // your unified hash macros

// ---------------------- Helpers for String_View keys ----------------------
static inline void *key_copy_sv(String_View sv) { return sv_to_cstr(sv); }
static inline void key_free_sv(void *k) { free(k); }

// ---------------------- Define Hash Tables ----------------------

// String key -> size_t value (word count)
GC_HASH_DEFINE_STR(str_count, size_t, key_copy_sv, key_free_sv)

// Heap integer key -> struct value
typedef struct { int fd; char name[32]; } SockInfo;
GC_HASH_DEFINE_INT(sock_table, SockInfo)

// Inline integer key -> size_t value
GC_HASH_DEFINE_INLINE(inline_table, size_t)

int main(void) {
    // 1) Count words in a string using String_View and str_count
    const char *text = "to be or not to be that is the question";
    String_View content = gc_sv_from_cstr(text);

    gc_hash_str_count word_ht;
    gc_hash_str_count_init(&word_ht, 16);

    while (content.count > 0) {
        String_View token = sv_chop_by_space(&content);
        if (token.count == 0) continue;

        char *tok = sv_to_cstr(token);
        size_t freq;
        if (gc_hash_str_count_get(&word_ht, tok, &freq)) {
            gc_hash_str_count_set(&word_ht, tok, freq + 1);
            free(tok);
        } else {
            gc_hash_str_count_set(&word_ht, tok, 1);
        }
    }

    printf("Word counts:\n");
    for (size_t i = 0; i < word_ht.base.capacity; i++) {
        if (word_ht.base.items[i].state == GC_SLOT_OCCUPIED) {
            printf("  %s => %zu\n", (char*)word_ht.base.items[i].key,
                                      *(size_t*)word_ht.base.items[i].value);
        }
    }
    gc_hash_str_count_free(&word_ht);

    // 2) Heap integer key -> struct value (socket table)
    sock_table shtable;
    gc_hash_sock_table_init(&shtable, 16);

    SockInfo s1 = { .fd = 3, .name = "socket3" };
    gc_hash_sock_table_set(&shtable, 3, s1);
    SockInfo s2 = { .fd = 5, .name = "socket5" };
    gc_hash_sock_table_set(&shtable, 5, s2);

    SockInfo found;
    if (gc_hash_sock_table_get(&shtable, 5, &found))
        printf("Found socket %d: %s\n", found.fd, found.name);

    gc_hash_sock_table_free(&shtable);

    // 3) Inline integer key -> value
    inline_table itable;
    gc_hash_inline_table_init(&itable, 16);
    gc_hash_inline_table_set(&itable, 42, 999);
    gc_hash_inline_table_set(&itable, 7, 123);

    size_t val;
    if (gc_hash_inline_table_get(&itable, 42, &val))
        printf("Inline table: key 42 -> value %zu\n", val);

    gc_hash_inline_table_free(&itable);

    return 0;
}
*/