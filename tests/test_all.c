#define TEST_ALL_RUNNER
#include "test_framework.h"

int g_tests_run = 0;
int g_tests_failed = 0;

int test_vector_main(void);
int test_queue_main(void);
int test_stack_main(void);
int test_deque_main(void);
int test_arena_main(void);
int test_hashmap_main(void);
int test_lru_main(void);
int test_graph_main(void);
int test_bfs_main(void);
int test_dfs_main(void);
int test_minheap_main(void);
int test_dijkstra_main(void);
int test_dsu_main(void);
int test_tarjan_main(void);
int test_sort_main(void);
int test_search_main(void);
int test_kway_main(void);
int test_timeline_main(void);
int test_trie_main(void);
int test_sha256_main(void);
int test_merkle_main(void);
int test_integrity_main(void);
int test_import_main(void);

int main(void) {
    printf("====================================================\n");
    printf("     ChronoTrace Core Engine Test Suite\n");
    printf("====================================================\n");

    test_vector_main();
    test_queue_main();
    test_stack_main();
    test_deque_main();
    test_arena_main();
    test_hashmap_main();
    test_lru_main();
    test_graph_main();
    test_bfs_main();
    test_dfs_main();
    test_minheap_main();
    test_dijkstra_main();
    test_dsu_main();
    test_tarjan_main();
    test_sort_main();
    test_search_main();
    test_kway_main();
    test_timeline_main();
    test_trie_main();
    test_sha256_main();
    test_merkle_main();
    test_integrity_main();
    test_import_main();

    printf("\n====================================================\n");
    printf("  SUMMARY: %d tests run, %d failed.\n", g_tests_run, g_tests_failed);
    printf("====================================================\n");

    return g_tests_failed ? 1 : 0;
}
