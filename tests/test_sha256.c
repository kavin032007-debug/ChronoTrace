#include "test_framework.h"
#include "../engine/integrity/sha256.h"

int test_sha256_main(void) {
    printf("--- Running SHA-256 Standard NIST Vector Tests ---\n");

    char hex[CT_SHA256_HEX_LEN];

    /* Test 1: Empty string */
    ct_sha256_buffer_hex((const uint8_t*)"", 0, hex);
    TEST_ASSERT_STR_EQ(hex, "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855", "SHA-256 of empty string matches standard vector");

    /* Test 2: "abc" */
    ct_sha256_buffer_hex((const uint8_t*)"abc", 3, hex);
    TEST_ASSERT_STR_EQ(hex, "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad", "SHA-256 of 'abc' matches standard vector");

    return g_tests_failed;
}

#ifndef TEST_ALL_RUNNER
int main(void) {
    test_sha256_main();
    return g_tests_failed ? 1 : 0;
}
#endif
