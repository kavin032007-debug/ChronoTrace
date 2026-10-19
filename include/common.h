#ifndef CHRONOTRACE_COMMON_H
#define CHRONOTRACE_COMMON_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

/* Status codes */
typedef enum {
    CT_OK = 0,
    CT_ERR_NOMEM = 1,
    CT_ERR_INVARG = 2,
    CT_ERR_NOTFOUND = 3,
    CT_ERR_CAPACITY = 4,
    CT_ERR_PARSE = 5,
    CT_ERR_IO = 6,
    CT_ERR_DUPLICATE = 7,
    CT_ERR_VALIDATION = 8,
    CT_ERR_EMPTY = 9
} ct_status_t;

/* String bounds */
#define CT_MAX_ID_LEN 64
#define CT_MAX_TYPE_LEN 64
#define CT_MAX_NAME_LEN 128
#define CT_MAX_RELATION_LEN 64
#define CT_MAX_TIME_STR_LEN 32
#define CT_MAX_REASON_LEN 256
#define CT_SHA256_HEX_LEN 65

#endif /* CHRONOTRACE_COMMON_H */
