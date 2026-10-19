#include "test_framework.h"
#include "../engine/integrity/integrity.h"

int test_integrity_main(void) {
    printf("--- Running Evidence Integrity Verification Tests ---\n");

    const char *names[3] = { "events.csv", "entities.csv", "relationships.csv" };
    const char *paths[3] = { "data/events.csv", "data/entities.csv", "data/relationships.csv" };

    ct_integrity_manifest_t manifest;
    ct_status_t st = ct_integrity_init_manifest(&manifest, names, paths, 3);
    TEST_ASSERT(st == CT_OK, "Initialize baseline manifest from data CSVs");
    TEST_ASSERT(manifest.file_count == 3, "Manifest tracks 3 files");

    /* Verify against unchanged files */
    ct_integrity_report_t report;
    TEST_ASSERT(ct_integrity_verify(&manifest, paths, 3, &report) == CT_OK, "Verify integrity against baseline");
    TEST_ASSERT(report.all_files_match, "All files match baseline");
    TEST_ASSERT(report.manifest_root_matches, "Combined manifest root matches baseline fingerprint");

    ct_integrity_report_free(&report);
    ct_integrity_manifest_free(&manifest);
    return g_tests_failed;
}

#ifndef TEST_ALL_RUNNER
int main(void) {
    test_integrity_main();
    return g_tests_failed ? 1 : 0;
}
#endif
