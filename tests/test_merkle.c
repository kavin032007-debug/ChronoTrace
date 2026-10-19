#include "test_framework.h"
#include "../engine/integrity/merkle.h"

int test_merkle_main(void) {
    printf("--- Running Merkle Tree Tests ---\n");

    ct_merkle_leaf_t leaves[3] = {
        { .filename = "events.csv", .hash_hex = "1111111111111111111111111111111111111111111111111111111111111111" },
        { .filename = "entities.csv", .hash_hex = "2222222222222222222222222222222222222222222222222222222222222222" },
        { .filename = "relationships.csv", .hash_hex = "3333333333333333333333333333333333333333333333333333333333333333" }
    };

    ct_merkle_tree_t tree;
    TEST_ASSERT(ct_merkle_build(&tree, leaves, 3) == CT_OK, "Build Merkle tree from 3 leaves");
    TEST_ASSERT(tree.root != NULL && strlen(tree.root_hash_hex) == 64, "Tree computed valid 64-char root hash");

    /* Verify leaf matches */
    TEST_ASSERT(ct_merkle_verify_leaf(&tree, 0, "1111111111111111111111111111111111111111111111111111111111111111"), "Leaf 0 matches");
    TEST_ASSERT(!ct_merkle_verify_leaf(&tree, 0, "9999999999999999999999999999999999999999999999999999999999999999"), "Mismatched leaf returns false");

    /* Detect changed file */
    const char *modified_hashes[3] = {
        "1111111111111111111111111111111111111111111111111111111111111111",
        "changed_hash_222222222222222222222222222222222222222222222222222",
        "3333333333333333333333333333333333333333333333333333333333333333"
    };

    int32_t changed_idx = ct_merkle_find_changed_file(&tree, modified_hashes, 3);
    TEST_ASSERT(changed_idx == 1, "Merkle tree accurately identified leaf 1 (entities.csv) as modified");

    ct_merkle_free(&tree);
    return g_tests_failed;
}

#ifndef TEST_ALL_RUNNER
int main(void) {
    test_merkle_main();
    return g_tests_failed ? 1 : 0;
}
#endif
