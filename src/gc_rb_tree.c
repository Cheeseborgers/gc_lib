#include "gc_rb_tree.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "gc_common.h"
#include "gc_dyn_array.h"

// ---------------------- Internal helpers ----------------------
static RBTreeNode *gc_rb_tree_alloc_node(const RBTree *tree, void *data) {
  RBTreeNode *n = GC_MALLOC(sizeof(RBTreeNode));
  if (!n)
    abort();
  n->data = data;
  n->color = RED;
  n->left = n->right = n->parent = tree->nil;
  return n;
}

static void gc_rb_tree_free_nodes(RBTree *tree, RBTreeNode *node,
                                  void (*free_data)(void *)) {
  if (node == tree->nil)
    return;

  gc_rb_tree_free_nodes(tree, node->left, free_data);
  gc_rb_tree_free_nodes(tree, node->right, free_data);

  if (free_data)
    free_data(node->data);

  GC_FREE(node);
}

static void gc_rb_tree_left_rotate(RBTree *tree, RBTreeNode *x) {
  RBTreeNode *y = x->right;
  x->right = y->left;
  if (y->left != tree->nil)
    y->left->parent = x;

  y->parent = x->parent;
  if (x->parent == tree->nil)
    tree->root = y;
  else if (x == x->parent->left)
    x->parent->left = y;
  else
    x->parent->right = y;

  y->left = x;
  x->parent = y;
}

static void gc_rb_tree_right_rotate(RBTree *tree, RBTreeNode *y) {
  RBTreeNode *x = y->left;
  y->left = x->right;
  if (x->right != tree->nil)
    x->right->parent = y;

  x->parent = y->parent;
  if (y->parent == tree->nil)
    tree->root = x;
  else if (y == y->parent->right)
    y->parent->right = x;
  else
    y->parent->left = x;

  x->right = y;
  y->parent = x;
}

static void gc_rb_tree_insert_fixup(RBTree *tree, RBTreeNode *k) {
  while (k->parent->color == RED) {
    RBTreeNode *uncle;
    if (k->parent == k->parent->parent->left) {
      uncle = k->parent->parent->right;
      if (uncle->color == RED) {
        k->parent->color = BLACK;
        uncle->color = BLACK;
        k->parent->parent->color = RED;
        k = k->parent->parent;
      } else {
        if (k == k->parent->right) {
          k = k->parent;
          gc_rb_tree_left_rotate(tree, k);
        }
        k->parent->color = BLACK;
        k->parent->parent->color = RED;
        gc_rb_tree_right_rotate(tree, k->parent->parent);
      }
    } else {
      uncle = k->parent->parent->left;
      if (uncle->color == RED) {
        k->parent->color = BLACK;
        uncle->color = BLACK;
        k->parent->parent->color = RED;
        k = k->parent->parent;
      } else {
        if (k == k->parent->left) {
          k = k->parent;
          gc_rb_tree_right_rotate(tree, k);
        }
        k->parent->color = BLACK;
        k->parent->parent->color = RED;
        gc_rb_tree_left_rotate(tree, k->parent->parent);
      }
    }
  }
  tree->root->color = BLACK;
}

// Returns -1 on error, otherwise black-height of subtree
static int gc_rb_tree_validate_rec(RBTree *tree, const RBTreeNode *node,
                                   int *err) {
  if (node == tree->nil)
    return 1; // black-height of nil is 1

  // Red property: red node cannot have red children
  if (node->color == RED) {
    if (node->left->color == RED || node->right->color == RED) {
      *err = 2; // red violation
      return -1;
    }
  }

  // BST property: left < node < right
  if (node->left != tree->nil && tree->cmp(node->left->data, node->data) >= 0) {
    *err = 1; // BST violation
    return -1;
  }
  if (node->right != tree->nil &&
      tree->cmp(node->right->data, node->data) <= 0) {
    *err = 1; // BST violation
    return -1;
  }

  // Recurse into left and right
  const int left_bh = gc_rb_tree_validate_rec(tree, node->left, err);
  if (left_bh < 0)
    return -1;

  const int right_bh = gc_rb_tree_validate_rec(tree, node->right, err);
  if (right_bh < 0)
    return -1;

  if (left_bh != right_bh) {
    *err = 3; // black-height mismatch
    return -1;
  }

  return left_bh + (node->color == BLACK ? 1 : 0);
}

// ---------------------- Tree API ----------------------

RBTree *gc_rb_tree_create(RBTreeCompareFunc cmp) {
  RBTree *tree = GC_MALLOC(sizeof(RBTree));
  if (!tree)
    abort();

  tree->nil = GC_MALLOC(sizeof(RBTreeNode));
  if (!tree->nil)
    abort();
  tree->nil->color = BLACK;
  tree->nil->left = tree->nil->right = tree->nil->parent = tree->nil;
  tree->root = tree->nil;
  tree->cmp = cmp;

  return tree;
}

void gc_rb_tree_clean_up(RBTree *tree, void (*free_data)(void *)) {
  if (!tree)
    return;

  if (tree->root != tree->nil) {
    gc_rb_tree_free_nodes(tree, tree->root, free_data);
    tree->root = tree->nil;
  }

  if (tree->nil) {
    GC_FREE(tree->nil);
    tree->nil = NULL;
  }
}

// Frees all nodes and their payloads, but keeps the tree itself and sentinel
// intact
void gc_rb_tree_free_all_nodes(RBTree *tree, void (*free_data)(void *)) {
  if (!tree || tree->root == tree->nil)
    return;
  gc_rb_tree_free_nodes(tree, tree->root, free_data);
  tree->root = tree->nil; // reset root for reuse
}

void gc_rb_tree_destroy(RBTree *tree, void (*free_data)(void *)) {
  if (!tree)
    return;
  gc_rb_tree_clean_up(tree, free_data);
  GC_FREE(tree->nil);
  GC_FREE(tree);
}

// ---------------------- Insertion ----------------------

void gc_rb_tree_insert(RBTree *tree, void *data) {
  RBTreeNode *z = gc_rb_tree_alloc_node(tree, data);
  RBTreeNode *y = tree->nil;
  RBTreeNode *x = tree->root;

  while (x != tree->nil) {
    y = x;
    if (tree->cmp(z->data, x->data) < 0)
      x = x->left;
    else
      x = x->right;
  }

  z->parent = y;
  if (y == tree->nil)
    tree->root = z;
  else if (tree->cmp(z->data, y->data) < 0)
    y->left = z;
  else
    y->right = z;

  gc_rb_tree_insert_fixup(tree, z);
}

// ---------------------- Search ----------------------

RBTreeNode *gc_rb_tree_search(const RBTree *tree, void *data) {
  RBTreeNode *x = tree->root;
  while (x != tree->nil) {
    const int cmp = tree->cmp(data, x->data);
    if (cmp == 0)
      return x;
    if (cmp < 0)
      x = x->left;
    else
      x = x->right;
  }
  return tree->nil;
}

void *gc_rb_tree_find(const RBTree *tree, void *key) {
  const RBTreeNode *node = gc_rb_tree_search(tree, key);
  return (node != tree->nil) ? node->data : NULL;
}

// ---------------------- Deletion ----------------------

static void gc_rb_tree_transplant(RBTree *tree, const RBTreeNode *node,
                                  RBTreeNode *v) {
  if (node->parent == tree->nil)
    tree->root = v;
  else if (node == node->parent->left)
    node->parent->left = v;
  else
    node->parent->right = v;
  v->parent = node->parent;
}

static RBTreeNode *gc_rb_tree_minimum(const RBTree *tree, RBTreeNode *node) {
  while (node->left != tree->nil)
    node = node->left;
  return node;
}

static RBTreeNode *gc_rb_tree_maximum(const RBTree *tree, RBTreeNode *x) {
  while (x->right != tree->nil)
    x = x->right;
  return x;
}

static void gc_rb_tree_delete_fixup(RBTree *tree, RBTreeNode *x) {
  while (x != tree->root && x->color == BLACK) {
    RBTreeNode *w;
    if (x == x->parent->left) {
      w = x->parent->right;
      if (w->color == RED) {
        w->color = BLACK;
        x->parent->color = RED;
        gc_rb_tree_left_rotate(tree, x->parent);
        w = x->parent->right;
      }
      if (w->left->color == BLACK && w->right->color == BLACK) {
        w->color = RED;
        x = x->parent;
      } else {
        if (w->right->color == BLACK) {
          w->left->color = BLACK;
          w->color = RED;
          gc_rb_tree_right_rotate(tree, w);
          w = x->parent->right;
        }
        w->color = x->parent->color;
        x->parent->color = BLACK;
        w->right->color = BLACK;
        gc_rb_tree_left_rotate(tree, x->parent);
        x = tree->root;
      }
    } else {
      w = x->parent->left;
      if (w->color == RED) {
        w->color = BLACK;
        x->parent->color = RED;
        gc_rb_tree_right_rotate(tree, x->parent);
        w = x->parent->left;
      }
      if (w->right->color == BLACK && w->left->color == BLACK) {
        w->color = RED;
        x = x->parent;
      } else {
        if (w->left->color == BLACK) {
          w->right->color = BLACK;
          w->color = RED;
          gc_rb_tree_left_rotate(tree, w);
          w = x->parent->left;
        }
        w->color = x->parent->color;
        x->parent->color = BLACK;
        w->left->color = BLACK;
        gc_rb_tree_right_rotate(tree, x->parent);
        x = tree->root;
      }
    }
  }
  x->color = BLACK;
}

void gc_rb_tree_delete(RBTree *tree, RBTreeNode *node,
                       void (*free_data)(void *)) {
  if (!tree || node == tree->nil)
    return;

  RBTreeNode *y = node;
  RBTreeNodeColor y_original_color = y->color;
  RBTreeNode *x;

  if (node->left == tree->nil) {
    x = node->right;
    gc_rb_tree_transplant(tree, node, node->right);
  } else if (node->right == tree->nil) {
    x = node->left;
    gc_rb_tree_transplant(tree, node, node->left);
  } else {
    y = gc_rb_tree_minimum(tree, node->right);
    y_original_color = y->color;
    x = y->right;
    if (y->parent == node)
      x->parent = y;
    else {
      gc_rb_tree_transplant(tree, y, y->right);
      y->right = node->right;
      y->right->parent = y;
    }
    gc_rb_tree_transplant(tree, node, y);
    y->left = node->left;
    y->left->parent = y;
    y->color = node->color;
  }

  if (y_original_color == BLACK)
    gc_rb_tree_delete_fixup(tree, x);

  if (free_data)
    free_data(node->data);
  GC_FREE(node);
}

// ---------------------- Printing ----------------------

void gc_rb_tree_inorder_print(RBTree *tree, const RBTreeNode *node,
                              void (*print_data)(void *)) {
  if (!tree || node == tree->nil)
    return;
  gc_rb_tree_inorder_print(tree, node->left, print_data);
  print_data(node->data);
  gc_rb_tree_inorder_print(tree, node->right, print_data);
}

static void gc_rb_tree_print_structure_rec(RBTree *tree, const RBTreeNode *node,
                                           const char *prefix, bool is_left,
                                           void (*print_data)(void *)) {
  if (!tree || node == tree->nil)
    return;

  printf("%s", prefix);
  printf(is_left ? "├── " : "└── ");

  print_data(node->data);
  printf(" (%s)\n", node->color == RED ? "R" : "B");

  char new_prefix[256];
  snprintf(new_prefix, sizeof(new_prefix), "%s%s", prefix,
           is_left ? "│   " : "    ");

  gc_rb_tree_print_structure_rec(tree, node->left, new_prefix, true,
                                 print_data);
  gc_rb_tree_print_structure_rec(tree, node->right, new_prefix, false,
                                 print_data);
}

void gc_rb_tree_print_structure(RBTree *tree, void (*print_data)(void *)) {
  if (!tree || tree->root == tree->nil) {
    printf("<empty tree>\n");
    return;
  }

  gc_rb_tree_print_structure_rec(tree, tree->root, "", false, print_data);
}

typedef struct QueueNode {
  RBTreeNode *tree_node;
  struct QueueNode *next;
} QueueNode;

typedef struct Queue {
  QueueNode *front;
  QueueNode *rear;
} Queue;

// Initialize empty queue
static void queue_init(Queue *q) { q->front = q->rear = NULL; }

// Enqueue a tree node
static void enqueue(Queue *q, RBTreeNode *n) {
  QueueNode *qn = GC_MALLOC(sizeof(QueueNode));
  if (!qn)
    abort();
  qn->tree_node = n;
  qn->next = NULL;
  if (!q->rear) {
    q->front = q->rear = qn;
  } else {
    q->rear->next = qn;
    q->rear = qn;
  }
}

// Dequeue a tree node (returns NULL if empty)
static RBTreeNode *dequeue(Queue *q) {
  if (!q->front)
    return NULL;
  QueueNode *tmp = q->front;
  RBTreeNode *n = tmp->tree_node;
  q->front = q->front->next;
  if (!q->front)
    q->rear = NULL;
  GC_FREE(tmp);
  return n;
}

// Level-order (BFS) print
void gc_rb_tree_print_level_order(RBTree *tree, void (*print_data)(void *)) {
  if (!tree || tree->root == tree->nil) {
    printf("<empty tree>\n");
    return;
  }

  Queue q;
  queue_init(&q);
  enqueue(&q, tree->root);

  RBTreeNode *node;
  while ((node = dequeue(&q))) {
    if (node == tree->nil)
      continue;

    print_data(node->data);
    printf(" (%s)  ", node->color == RED ? "R" : "B");

    enqueue(&q, node->left);
    enqueue(&q, node->right);
  }
  printf("\n");
}

RBTreeStatus gc_rb_tree_validate(RBTree *tree) {
  if (!tree || !tree->root || !tree->nil)
    return GC_RB_TREE_ERR_INVALID_TREE;

  if (tree->nil->color != BLACK)
    return GC_RB_TREE_ERR_NIL_COLOR;

  int err = 0;
  const int bh = gc_rb_tree_validate_rec(tree, tree->root, &err);

  if (bh < 0 && err == 0)
    return GC_RB_TREE_ERR_GENERIC;

  return (RBTreeStatus)err; // 0 means valid
}

const char *gc_rb_tree_error_str(int code) {
  switch (code) {
  case GC_RB_TREE_OK:
    return "No error";
  case GC_RB_TREE_ERR_BST:
    return "BST property violation";
  case GC_RB_TREE_ERR_RED:
    return "Red node has red child";
  case GC_RB_TREE_ERR_BLACK_HEIGHT:
    return "Black-height mismatch";
  case GC_RB_TREE_ERR_INVALID_TREE:
    return "Invalid tree pointer";
  case GC_RB_TREE_ERR_NIL_COLOR:
    return "Nil sentinel not black";
  case GC_RB_TREE_ERR_GENERIC:
    return "Generic failure";
  default:
    return "Unknown error";
  }
}

RBTreeNode *gc_rb_tree_min(const RBTree *tree) {
  if (!tree || tree->root == tree->nil)
    return NULL;
  RBTreeNode *node = tree->root;
  while (node->left != tree->nil)
    node = node->left;
  return node;
}

RBTreeNode *gc_rb_tree_max(const RBTree *tree) {
  if (!tree || tree->root == tree->nil)
    return NULL;
  RBTreeNode *node = tree->root;
  while (node->right != tree->nil)
    node = node->right;
  return node;
}

RBTreeNode *gc_rb_tree_successor(const RBTree *tree, const RBTreeNode *node) {
  if (node->right != tree->nil)
    return gc_rb_tree_minimum(tree, node->right);
  RBTreeNode *y = node->parent;
  while (y != tree->nil && node == y->right) {
    node = y;
    y = y->parent;
  }
  return (y != tree->nil) ? y : NULL;
}

RBTreeNode *gc_rb_tree_predecessor(const RBTree *tree, const RBTreeNode *node) {
  if (node->left != tree->nil)
    return gc_rb_tree_maximum(tree, node->left);
  RBTreeNode *y = node->parent;
  while (y != tree->nil && node == y->left) {
    node = y;
    y = y->parent;
  }
  return (y != tree->nil) ? y : NULL;
}

// Iterator API
// Initialize iterator at smallest node
RBTreeIterator gc_rb_tree_iterator_begin(const RBTree *tree) {
  RBTreeIterator it = {tree, NULL};
  if (tree && tree->root != tree->nil) {
    it.current = gc_rb_tree_min(tree);
  }
  return it;
}

RBTreeIterator gc_rb_tree_iterator_rbegin(const RBTree *tree) {
  RBTreeIterator it = {tree, NULL};
  if (tree && tree->root != tree->nil) {
    it.current = gc_rb_tree_max(tree);
  }
  return it;
}

void gc_rb_tree_iterator_next(RBTreeIterator *it) {
  if (it && it->tree && it->current)
    it->current = gc_rb_tree_successor(it->tree, it->current);
}

void gc_rb_tree_iterator_prev(RBTreeIterator *it) {
  if (it && it->tree && it->current)
    it->current = gc_rb_tree_predecessor(it->tree, it->current);
}

bool gc_rb_tree_iterator_valid(const RBTreeIterator *it) {
  return it && it->current && it->current != it->tree->nil;
}

void *gc_rb_tree_iterator_data(const RBTreeIterator *it) {
  if (!it || !it->current || it->current == it->tree->nil)
    return NULL;
  return it->current->data;
}

static RBTreeNode *gc_rb_tree_lower_bound(const RBTree *tree, void *key,
                                          int inclusive) {
  RBTreeNode *x = tree->root;
  RBTreeNode *res = tree->nil;
  while (x != tree->nil) {
    const int cmp = tree->cmp(x->data, key);
    if (cmp > 0 || (inclusive && cmp == 0)) {
      res = x;
      x = x->left;
    } else {
      x = x->right;
    }
  }
  return (res != tree->nil) ? res : NULL;
}

RBTreeRangeIterator gc_rb_tree_iterator_range_begin(const RBTree *tree,
                                                    void *low, void *high,
                                                    int flags) {
  RBTreeRangeIterator it = {tree, NULL, low, high, flags};
  if (!tree || tree->root == tree->nil || !tree->cmp)
    return it;

  it.current =
      gc_rb_tree_lower_bound(tree, low, flags & GC_RB_RANGE_INCLUSIVE_LOW);

  // Check upper bound validity
  if (it.current) {
    const int cmp = tree->cmp(it.current->data, high);
    if (cmp > 0 || (!(flags & GC_RB_RANGE_INCLUSIVE_HIGH) && cmp == 0))
      it.current = NULL;
  }

  return it;
}

static RBTreeNode *gc_rb_tree_upper_bound(const RBTree *tree, void *key,
                                          int inclusive) {
  RBTreeNode *x = tree->root;
  RBTreeNode *res = tree->nil;
  while (x != tree->nil) {
    const int cmp = tree->cmp(x->data, key);
    if (cmp < 0 || (inclusive && cmp == 0)) {
      res = x;
      x = x->right;
    } else {
      x = x->left;
    }
  }
  return (res != tree->nil) ? res : NULL;
}

RBTreeRangeIterator gc_rb_tree_iterator_range_rbegin(const RBTree *tree,
                                                     void *low, void *high,
                                                     int flags) {
  RBTreeRangeIterator it = {tree, NULL, low, high, flags, 1};
  if (!tree || tree->root == tree->nil || !tree->cmp)
    return it;

  it.current =
      gc_rb_tree_upper_bound(tree, high, flags & GC_RB_RANGE_INCLUSIVE_HIGH);

  if (it.current) {
    const int cmp = tree->cmp(it.current->data, low);
    if (cmp < 0 || (!(flags & GC_RB_RANGE_INCLUSIVE_LOW) && cmp == 0))
      it.current = NULL;
  }

  return it;
}

void gc_rb_tree_iterator_range_next(RBTreeRangeIterator *it) {
  if (!it || !it->tree || !it->current)
    return;

  if (it->reverse)
    it->current = gc_rb_tree_predecessor(it->tree, it->current);
  else
    it->current = gc_rb_tree_successor(it->tree, it->current);

  if (!it->current)
    return;

  if (it->reverse) {
    const int cmp = it->tree->cmp(it->current->data, it->low);
    if (cmp < 0 || (!(it->flags & GC_RB_RANGE_INCLUSIVE_LOW) && cmp == 0))
      it->current = NULL;
  } else {
    const int cmp = it->tree->cmp(it->current->data, it->high);
    if (cmp > 0 || (!(it->flags & GC_RB_RANGE_INCLUSIVE_HIGH) && cmp == 0))
      it->current = NULL;
  }
}

bool gc_rb_tree_iterator_range_valid(const RBTreeRangeIterator *it) {
  if (!it || !it->current || it->current == it->tree->nil)
    return false;

  void *data = it->current->data;
  if (!data)
    return false;

  if (!it->reverse) {
    int cmp_high = it->high ? it->tree->cmp(data, it->high) : -1;
    if (cmp_high > 0 ||
        (!(it->flags & GC_RB_RANGE_INCLUSIVE_HIGH) && cmp_high == 0))
      return false;
  } else {
    int cmp_low = it->low ? it->tree->cmp(data, it->low) : 1;
    if (cmp_low < 0 ||
        (!(it->flags & GC_RB_RANGE_INCLUSIVE_LOW) && cmp_low == 0))
      return false;
  }

  return true;
}

void *gc_rb_tree_iterator_range_data(const RBTreeRangeIterator *it) {
  return (it && it->current && it->current != it->tree->nil) ? it->current->data
                                                             : NULL;
}

void **gc_rb_tree_slice(const RBTree *tree, void *low, void *high, int flags,
                        size_t *out_count) {
  if (!tree || tree->root == tree->nil) {
    if (out_count)
      *out_count = 0;
    return NULL;
  }

  RBTreeRangeIterator it =
      (flags & GC_RB_TREE_SLICE_DESCENDING)
          ? gc_rb_tree_iterator_range_rbegin(tree, low, high, flags)
          : gc_rb_tree_iterator_range_begin(tree, low, high, flags);

  // First pass: count elements in range
  size_t count = 0;
  RBTreeRangeIterator tmp = it;
  while (gc_rb_tree_iterator_range_valid(&tmp)) {
    count++;
    gc_rb_tree_iterator_range_next(&tmp);
  }

  if (count == 0) {
    if (out_count)
      *out_count = 0;
    return NULL;
  }

  // Allocate array
  void **results = GC_MALLOC(sizeof(void *) * (count + 1));
  if (!results) {
    if (out_count)
      *out_count = 0;
    return NULL;
  }

  // Second pass: fill array
  size_t i = 0;
  while (gc_rb_tree_iterator_range_valid(&it)) {
    results[i++] = gc_rb_tree_iterator_range_data(&it);
    gc_rb_tree_iterator_range_next(&it);
  }

  results[i] = NULL;
  if (out_count)
    *out_count = count;
  return results;
}

void **gc_rb_tree_filter(const RBTree *tree, RBTreePredicateFunc pred,
                         void *user_data, size_t *out_count) {
  if (!tree || tree->root == tree->nil || !pred) {
    if (out_count)
      *out_count = 0;
    return NULL;
  }

  // First pass: count matches
  size_t count = 0;
  RBTreeIterator it = gc_rb_tree_iterator_begin(tree);
  while (gc_rb_tree_iterator_valid(&it)) {
    void *data = gc_rb_tree_iterator_data(&it);
    if (pred(data, user_data))
      count++;
    gc_rb_tree_iterator_next(&it);
  }

  if (count == 0) {
    if (out_count)
      *out_count = 0;
    return NULL;
  }

  // Second pass: collect matches
  void **results = GC_MALLOC(sizeof(void *) * (count + 1));
  if (!results) {
    if (out_count)
      *out_count = 0;
    return NULL;
  }

  size_t i = 0;
  it = gc_rb_tree_iterator_begin(tree);
  while (gc_rb_tree_iterator_valid(&it)) {
    void *data = gc_rb_tree_iterator_data(&it);
    if (pred(data, user_data))
      results[i++] = data;
    gc_rb_tree_iterator_next(&it);
  }

  results[i] = NULL;
  if (out_count)
    *out_count = count;
  return results;
}

void **gc_rb_tree_filter_with_da(const RBTree *tree,
                                 RBTreePredicateFunc pred,
                                 void *user_data,
                                 size_t *out_count)
{
  if (!tree || tree->root == tree->nil || !pred) {
    if (out_count) *out_count = 0;
    return NULL;
  }

  // Define a proper dynamic array struct for void* pointers
  typedef struct VoidPtrArray {
    void **items;
    size_t count;
    size_t capacity;
  } VoidPtrArray;

  VoidPtrArray results = {0};

  RBTreeIterator it = gc_rb_tree_iterator_begin(tree);
  while (gc_rb_tree_iterator_valid(&it)) {
    void *data = gc_rb_tree_iterator_data(&it);
    if (pred(data, user_data)) {
      gc_da_append(&results, data);  // use your macro
    }
    gc_rb_tree_iterator_next(&it);
  }

  if (out_count) *out_count = results.count;

  if (results.count == 0) {
    gc_da_free(results);
    return NULL;
  }

  return results.items; // caller must free()
}

