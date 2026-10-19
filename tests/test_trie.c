#include "test_framework.h"
#include "../engine/text/trie.h"

int test_trie_main(void) {
    printf("--- Running Trie Prefix Search Tests ---\n");
    ct_trie_t trie;
    TEST_ASSERT(ct_trie_init(&trie) == CT_OK, "Init trie");

    TEST_ASSERT(ct_trie_insert(&trie, "U01", "U01", "User A", "user", 0) == CT_OK, "Insert U01");
    TEST_ASSERT(ct_trie_insert(&trie, "User A", "U01", "User A", "user", 0) == CT_OK, "Insert User A");
    TEST_ASSERT(ct_trie_insert(&trie, "USB_CONNECT", "E102", "usb_connect", "event", 1) == CT_OK, "Insert USB_CONNECT");
    TEST_ASSERT(ct_trie_insert(&trie, "User Baker", "U02", "User Baker", "user", 2) == CT_OK, "Insert User Baker");

    /* Prefix search for "us" should match "User A", "User Baker", and "USB_CONNECT" */
    ct_suggestion_t *sugg = NULL;
    size_t count = 0;
    TEST_ASSERT(ct_trie_search_prefix(&trie, "us", 10, &sugg, &count) == CT_OK, "Search prefix 'us'");
    TEST_ASSERT(count == 3, "Prefix 'us' returned 3 suggestions");

    /* Prefix search for "U0" should match "U01" */
    ct_suggestions_free(sugg);
    sugg = NULL;
    TEST_ASSERT(ct_trie_search_prefix(&trie, "U0", 10, &sugg, &count) == CT_OK, "Search prefix 'U0'");
    TEST_ASSERT(count == 1, "Prefix 'U0' returned 1 suggestion");
    if (count == 1) {
        TEST_ASSERT_STR_EQ(sugg[0].id, "U01", "Suggestion ID is U01");
    }

    ct_suggestions_free(sugg);
    ct_trie_free(&trie);
    return g_tests_failed;
}

#ifndef TEST_ALL_RUNNER
int main(void) {
    test_trie_main();
    return g_tests_failed ? 1 : 0;
}
#endif
