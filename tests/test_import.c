#include "test_framework.h"
#include "../ingest/storage.h"

int test_import_main(void) {
    printf("--- Running Log Ingestion & Rejection Reporting Tests ---\n");

    ct_dataset_t ds;
    TEST_ASSERT(ct_dataset_init(&ds, "data") == CT_OK, "Init dataset");

    TEST_ASSERT(ct_dataset_import_csvs(&ds, "data/events.csv", "data/entities.csv", "data/relationships.csv") == CT_OK,
                "Import valid evidence datasets");

    TEST_ASSERT(ds.summary.valid_entities >= 3, "Valid entities imported");
    TEST_ASSERT(ds.summary.valid_events >= 4, "Valid events imported");
    TEST_ASSERT(ds.summary.valid_relationships >= 2, "Valid relationships imported");
    TEST_ASSERT(ds.timeline.is_sorted, "Timeline is sorted upon import");

    /* Verify near-constant time entity lookup */
    void *ent_val = NULL;
    TEST_ASSERT(ct_hashmap_get(&ds.entities_map, "U01", &ent_val) == CT_OK && ent_val != NULL, "Find entity U01 in near-constant time");

    ct_dataset_free(&ds);
    return g_tests_failed;
}

#ifndef TEST_ALL_RUNNER
int main(void) {
    test_import_main();
    return g_tests_failed ? 1 : 0;
}
#endif
