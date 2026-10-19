#include "test_framework.h"
#include "../engine/core/arena.h"

int test_arena_main(void) {
    printf("--- Running Arena Tests ---\n");
    ct_arena_t arena;
    TEST_ASSERT(ct_arena_init(&arena, 1024) == CT_OK, "Init arena with 1KB blocks");

    void *p1 = ct_arena_alloc(&arena, 128);
    TEST_ASSERT(p1 != NULL, "Allocated 128 bytes from arena");

    char *str = ct_arena_strdup(&arena, "ChronoTrace Investigation Record");
    TEST_ASSERT(str && strcmp(str, "ChronoTrace Investigation Record") == 0, "Strdup in arena succeeded");

    /* Allocate larger than block size to trigger new block */
    void *large = ct_arena_alloc(&arena, 2048);
    TEST_ASSERT(large != NULL, "Allocated 2KB block in arena");

    ct_arena_reset(&arena);
    void *p2 = ct_arena_alloc(&arena, 64);
    TEST_ASSERT(p2 != NULL, "Allocated 64 bytes after arena reset");

    ct_arena_free(&arena);
    TEST_ASSERT(arena.head == NULL && arena.total_allocated == 0, "Arena freed");
    return g_tests_failed;
}

#ifndef TEST_ALL_RUNNER
int main(void) {
    test_arena_main();
    return g_tests_failed ? 1 : 0;
}
#endif
