#include <assert.h>

#include "../gc_rb_tree.h"
#include "../gc_common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    int id;
    char name[32];
} Person;

int person_cmp(void *a, void *b) {
    Person *pa = (Person *)a;
    Person *pb = (Person *)b;
    return (pa->id > pb->id) - (pa->id < pb->id);
}

void print_person(void *data) {
    Person *p = (Person *)data;
    printf("[%d: %s] ", p->id, p->name);
}

// Callback to print int payloads
void print_int(void *data) {
    printf("%d ", *(int *)data);  // add a space for readability
}

int int_cmp(void *a, void *b) {
    int ai = *(int *)a;
    int bi = *(int *)b;
    return (ai > bi) - (ai < bi);  // returns 1, 0, or -1
}

void assert_tree_valid(RBTree *tree) {
    RBTreeStatus res = gc_rb_tree_validate(tree);
    GC_ASSERT(res == GC_RB_TREE_OK && "RB-tree invariant violated!");
}

int less_than_20(void *data, void *user_data) {
    int val = *(int *)data;
    int limit = *(int *)user_data;
    return val < limit;
}

void test_filtered_slice() {
    printf("\nRunning filtered slice test...\n");

    // Create integer tree
    RBTree *tree = gc_rb_tree_create(int_cmp);

    // Insert values
    int values[] = {10, 20, 30, 15, 25, 5, 1};
    for (size_t i = 0; i < sizeof(values)/sizeof(values[0]); i++) {
        int *val = GC_MALLOC(sizeof(int));
        *val = values[i];
        gc_rb_tree_insert(tree, val);
    }

    size_t n = 0;
    int limit = 20;

    // Filter using predicate
    //void **filtered = gc_rb_tree_filter(tree, less_than_20, &limit, &n);
    void **filtered = gc_rb_tree_filter_with_da(tree, less_than_20, &limit, &n);

    printf("Filtered (<20): %zu elements\n", n);
    if (filtered) {
        for (size_t i = 0; i < n; i++) {
            printf("%d ", *(int *)filtered[i]);
        }
        printf("\n");
        free(filtered);
    } else {
        printf("<none>\n");
    }

    // Slice example: values between 5 and 25 inclusive
    int low = 5, high = 25;
    size_t slice_count = 0;
    void **slice = gc_rb_tree_slice(tree, &low, &high,
                                    GC_RB_RANGE_INCLUSIVE_LOW | GC_RB_RANGE_INCLUSIVE_HIGH,
                                    &slice_count);

    printf("Slice [5,25]: %zu elements\n", slice_count);
    if (slice) {
        for (size_t i = 0; i < slice_count; i++) {
            printf("%d ", *(int *)slice[i]);
        }
        printf("\n");
        free(slice);
    } else {
        printf("<none>\n");
    }

    // Cleanup all nodes
    gc_rb_tree_free_all_nodes(tree, GC_FREE);
    gc_rb_tree_destroy(tree, NULL);
}

void test_int_tree() {
    // Create tree
    RBTree *tree = gc_rb_tree_create(int_cmp);

    // Insert nodes
    int values[] = {10, 20, 30, 15, 25, 5, 1};
    int n_insert = sizeof(values) / sizeof(values[0]);

    printf("Inserting nodes:\n");
    for (int i = 0; i < n_insert; i++) {
        printf("Insert %d\n", values[i]);
        int *val = GC_MALLOC(sizeof(int));
        *val = values[i];
        gc_rb_tree_insert(tree, val);
        assert_tree_valid(tree);

        // Print structure
        gc_rb_tree_print_structure(tree, print_int);

        // Validate
        RBTreeStatus err = gc_rb_tree_validate(tree);
        if (err != GC_RB_TREE_OK) {
            printf("RB-tree validation failed: %s (code %d)\n", gc_rb_tree_error_str(err), err);
        }
        printf("\n");
    }

    // Level-order print
    printf("Level-order print:\n");
    gc_rb_tree_print_level_order(tree, print_int);
    printf("\n");

    int key = 25;
    int *found = gc_rb_tree_find(tree, &key);
    if (found) {
        printf("Found key %d via find(): %d\n", key, *found);
    } else {
        printf("Key %d not found via find()\n", key);
    }

    // Delete some nodes
    int to_delete[] = {20, 10};
    const int n_delete = sizeof(to_delete) / sizeof(to_delete[0]);

    printf("Deleting nodes:\n");
    for (int i = 0; i < n_delete; i++) {
        printf("Delete %d\n", to_delete[i]);
        RBTreeNode *node = gc_rb_tree_search(tree, &to_delete[i]);
        if (node != tree->nil) {
            gc_rb_tree_delete(tree, node, GC_FREE);
            assert_tree_valid(tree);
        } else {
            printf("Node %d not found!\n", to_delete[i]);
        }

        // Print structure
        gc_rb_tree_print_structure(tree, print_int);

        // Validate
        RBTreeStatus err = gc_rb_tree_validate(tree);
        if (err != GC_RB_TREE_OK) {
            printf("RB-tree validation failed: %s (code %d)\n", gc_rb_tree_error_str(err), err);
        }
        printf("\n");
    }

    // Final prints
    printf("Final tree (inorder traversal):\n");
    gc_rb_tree_inorder_print(tree, tree->root, print_int);

    printf("\nFinal tree structure:\n");
    gc_rb_tree_print_structure(tree, print_int);

    printf("\nFinal level-order print:\n");
    gc_rb_tree_print_level_order(tree, print_int);

    assert_tree_valid(tree);

    // Cleanup
    gc_rb_tree_destroy(tree, GC_FREE);
}

void test_struct_tree() {
    RBTree *tree = gc_rb_tree_create(person_cmp);

    // Insert some people
    Person *alice = GC_MALLOC(sizeof(Person));
    alice->id = 1; strcpy(alice->name, "Alice");
    gc_rb_tree_insert(tree, alice);

    Person *bob = GC_MALLOC(sizeof(Person));
    bob->id = 2; strcpy(bob->name, "Bob");
    gc_rb_tree_insert(tree, bob);

    Person *carol = GC_MALLOC(sizeof(Person));
    carol->id = 3; strcpy(carol->name, "Carol");
    gc_rb_tree_insert(tree, carol);

    assert_tree_valid(tree);

    printf("Tree structure:\n");
    gc_rb_tree_print_structure(tree, print_person);

    // Delete Bob
    RBTreeNode *node = gc_rb_tree_search(tree, bob);
    gc_rb_tree_delete(tree, node, GC_FREE);

    assert_tree_valid(tree);

    printf("\nAfter deleting Bob:\n");
    gc_rb_tree_print_structure(tree, print_person);

    Person key_person;
    key_person.id = 3; // Carol's id
    Person *found_person = gc_rb_tree_find(tree, &key_person);
    if (found_person) {
        printf("Found person via find(): %d %s\n", found_person->id, found_person->name);
    } else {
        printf("Person with id %d not found\n", key_person.id);
    }

    key_person.id = 2; // Bob
    found_person = gc_rb_tree_find(tree, &key_person);
    if (!found_person) printf("Bob correctly not found after deletion\n");

    assert_tree_valid(tree);

    // Cleanup all nodes
    gc_rb_tree_free_all_nodes(tree, GC_FREE);
    gc_rb_tree_destroy(tree, NULL); // tree itself is GC_FREEd, no need to GC_FREE data again
}

int main(void) {
    printf("Running integer RB-tree test...\n");
    test_int_tree();
    printf("Integer tree passed all assertions!\n\n");

    printf("Running struct RB-tree test...\n");
    test_struct_tree();
    printf("Struct tree passed all assertions!\n");

    test_filtered_slice();

    return 0;
}

