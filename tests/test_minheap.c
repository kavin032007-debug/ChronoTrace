#include "test_framework.h"
#include "../engine/order/minheap.h"

int test_minheap_main(void) {
    printf("--- Running MinHeap Tests ---\n");
    ct_minheap_t heap;
    TEST_ASSERT(ct_minheap_init(&heap, 4, 10, NULL) == CT_OK, "Init minheap with decrease-key support");

    TEST_ASSERT(ct_minheap_insert(&heap, 1, 50.0, NULL) == CT_OK, "Insert id 1 prio 50");
    TEST_ASSERT(ct_minheap_insert(&heap, 2, 20.0, NULL) == CT_OK, "Insert id 2 prio 20");
    TEST_ASSERT(ct_minheap_insert(&heap, 3, 35.0, NULL) == CT_OK, "Insert id 3 prio 35");

    ct_heap_node_t min_node;
    TEST_ASSERT(ct_minheap_peek(&heap, &min_node) == CT_OK && min_node.id == 2, "Peek min is id 2 (prio 20)");

    /* Decrease key of id 1 from 50 to 10 */
    TEST_ASSERT(ct_minheap_decrease_key(&heap, 1, 10.0) == CT_OK, "Decrease key id 1 to 10");
    TEST_ASSERT(ct_minheap_peek(&heap, &min_node) == CT_OK && min_node.id == 1, "Peek min is now id 1 (prio 10)");

    TEST_ASSERT(ct_minheap_extract_min(&heap, &min_node) == CT_OK && min_node.id == 1, "Extract min 1");
    TEST_ASSERT(ct_minheap_extract_min(&heap, &min_node) == CT_OK && min_node.id == 2, "Extract min 2");
    TEST_ASSERT(ct_minheap_extract_min(&heap, &min_node) == CT_OK && min_node.id == 3, "Extract min 3");
    TEST_ASSERT(ct_minheap_is_empty(&heap), "Heap is empty");

    ct_minheap_free(&heap);
    return g_tests_failed;
}

#ifndef TEST_ALL_RUNNER
int main(void) {
    test_minheap_main();
    return g_tests_failed ? 1 : 0;
}
#endif
