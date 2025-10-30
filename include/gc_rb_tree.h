#ifndef GC_RB_TREE_GENERIC_H_
#define GC_RB_TREE_GENERIC_H_
#include <stdbool.h>
#include <stddef.h>

#define GC_RB_TREE_SLICE_DESCENDING 0x8000

typedef enum { RED, BLACK } RBTreeNodeColor;

typedef enum {
  GC_RB_TREE_OK = 0,
  GC_RB_TREE_ERR_BST = 1,          // BST property violation
  GC_RB_TREE_ERR_RED = 2,          // Red node has red child
  GC_RB_TREE_ERR_BLACK_HEIGHT = 3, // Black-height mismatch
  GC_RB_TREE_ERR_INVALID_TREE = -1,
  GC_RB_TREE_ERR_NIL_COLOR = -2,
  GC_RB_TREE_ERR_GENERIC = -4
} RBTreeStatus;

typedef struct RBTreeNode {
  void *data;
  RBTreeNodeColor color;
  struct RBTreeNode *left;
  struct RBTreeNode *right;
  struct RBTreeNode *parent;
} RBTreeNode;

typedef int (*RBTreeCompareFunc)(void *a, void *b);
typedef int (*RBTreePredicateFunc)(void *data, void *user_data);

typedef struct RBTree {
  RBTreeNode *root;
  RBTreeNode *nil;       // sentinel
  RBTreeCompareFunc cmp; // comparison function
} RBTree;

// Tree management
RBTree *gc_rb_tree_create(RBTreeCompareFunc cmp);
void gc_rb_tree_destroy(RBTree *tree, void (*free_data)(void *));
void gc_rb_tree_clean_up(RBTree *tree, void (*free_data)(void *));
void gc_rb_tree_free_all_nodes(RBTree *tree, void (*free_data)(void *));

// Insert / delete / search
void gc_rb_tree_insert(RBTree *tree, void *data);
void gc_rb_tree_delete(RBTree *tree, RBTreeNode *node,
                       void (*free_data)(void *));
RBTreeNode *gc_rb_tree_search(const RBTree *tree, void *data);
void *gc_rb_tree_find(const RBTree *tree, void *key);

// Printing utilities
void gc_rb_tree_inorder_print(RBTree *tree, const RBTreeNode *node,
                              void (*print_data)(void *));
void gc_rb_tree_print_structure(RBTree *tree, void (*print_data)(void *));
void gc_rb_tree_print_level_order(RBTree *tree, void (*print_data)(void *));

RBTreeStatus gc_rb_tree_validate(RBTree *tree);

const char *gc_rb_tree_error_str(int code);

// Helpers
RBTreeNode *gc_rb_tree_min(const RBTree *tree);
RBTreeNode *gc_rb_tree_max(const RBTree *tree);
RBTreeNode *gc_rb_tree_successor(const RBTree *tree, const RBTreeNode *node);
RBTreeNode *gc_rb_tree_predecessor(const RBTree *tree, const RBTreeNode *node);

// Iterator API
typedef struct {
  const RBTree *tree;
  RBTreeNode *current;
} RBTreeIterator;

// Forward iteration
RBTreeIterator gc_rb_tree_iterator_begin(const RBTree *tree);
void gc_rb_tree_iterator_next(RBTreeIterator *it);
bool gc_rb_tree_iterator_valid(const RBTreeIterator *it);
void *gc_rb_tree_iterator_data(const RBTreeIterator *it);

// Reverse iteration
RBTreeIterator gc_rb_tree_iterator_rbegin(const RBTree *tree);
void gc_rb_tree_iterator_prev(RBTreeIterator *it);

// Range iteration
typedef enum {
  GC_RB_RANGE_INCLUSIVE_LOW = 1 << 0,
  GC_RB_RANGE_INCLUSIVE_HIGH = 1 << 1
} RBTreeRangeFlags;

typedef struct {
  const RBTree *tree;
  RBTreeNode *current;
  void *low;
  void *high;
  int flags;
  bool reverse;
} RBTreeRangeIterator;

// Forward iterator
RBTreeRangeIterator gc_rb_tree_iterator_range_begin(const RBTree *tree,
                                                    void *low, void *high,
                                                    int flags);

// Reverse iterator
RBTreeRangeIterator gc_rb_tree_iterator_range_rbegin(const RBTree *tree,
                                                     void *low, void *high,
                                                     int flags);

void gc_rb_tree_iterator_range_next(RBTreeRangeIterator *it);
bool gc_rb_tree_iterator_range_valid(const RBTreeRangeIterator *it);
void *gc_rb_tree_iterator_range_data(const RBTreeRangeIterator *it);

// --- Range Slicing ---
// TODO: Allow custom allocators here
void **gc_rb_tree_slice(const RBTree *tree,
                        void *low, void *high,
                        int flags,
                        size_t *out_count);

/**
 * Returns an array of pointers to data elements satisfying `pred`.
 * If `out_count` is provided, it's set to the number of matches.
 * Caller owns the returned array (free() it when done).
 */
void **gc_rb_tree_filter(const RBTree *tree,
                         RBTreePredicateFunc pred,
                         void *user_data,
                         size_t *out_count);

void **gc_rb_tree_filter_with_da(const RBTree *tree,
                                 RBTreePredicateFunc pred,
                                 void *user_data,
                                 size_t *out_count);

// --- Range foreach iteration macros ---

#define GC_RB_TREE_FOREACH_RANGE(tree, low, high, flags, var)                  \
  for (RBTreeRangeIterator _it =                                               \
           gc_rb_tree_iterator_range_begin(tree, low, high, flags);            \
       gc_rb_tree_iterator_range_valid(&_it) &&                                \
       ((var = gc_rb_tree_iterator_range_data(&_it)), 1);                      \
       gc_rb_tree_iterator_range_next(&_it))

#define GC_RB_TREE_FOREACH_RRANGE(tree, low, high, flags, var)                 \
  for (RBTreeRangeIterator _it =                                               \
           gc_rb_tree_iterator_range_rbegin(tree, low, high, flags);           \
       gc_rb_tree_iterator_range_valid(&_it) &&                                \
       ((var = gc_rb_tree_iterator_range_data(&_it)), 1);                      \
       gc_rb_tree_iterator_range_next(&_it))

#if __STDC_VERSION__ >= 201112L
#define GC_RB_TREE_FOREACH_AUTO(tree, low, high, flags, var)                   \
  for (_Generic((tree), const RBTree *: RBTreeRangeIterator)                   \
           _it = gc_rb_tree_iterator_range_begin(tree, low, high, flags);      \
       gc_rb_tree_iterator_range_valid(&_it) &&                                \
       ((var = gc_rb_tree_iterator_range_data(&_it)), 1);                      \
       gc_rb_tree_iterator_range_next(&_it))
#endif

#define GC_RB_TREE_FOREACH_ALL(tree, var)                                      \
  for (RBTreeRangeIterator _it = gc_rb_tree_iterator_range_begin(              \
           tree, NULL, NULL,                                                   \
           GC_RB_RANGE_INCLUSIVE_LOW | GC_RB_RANGE_INCLUSIVE_HIGH);            \
       gc_rb_tree_iterator_range_valid(&_it) &&                                \
       ((var = gc_rb_tree_iterator_range_data(&_it)), 1);                      \
       gc_rb_tree_iterator_range_next(&_it))

#endif // GC_RB_TREE_GENERIC_H_
