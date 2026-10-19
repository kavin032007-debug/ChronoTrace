#ifndef CHRONOTRACE_CSV_PARSER_H
#define CHRONOTRACE_CSV_PARSER_H

#include "../include/common.h"

#define CT_CSV_MAX_COLS 32
#define CT_CSV_MAX_FIELD_LEN 256

typedef struct {
    char fields[CT_CSV_MAX_COLS][CT_CSV_MAX_FIELD_LEN];
    size_t col_count;
} ct_csv_row_t;

/* Parses a single CSV line into tokens, handling quoted strings */
ct_status_t ct_csv_parse_line(const char *line, ct_csv_row_t *out_row);

/* Helper to trim leading/trailing whitespace */
void ct_str_trim(char *str);

/* Helper to parse ISO / standard UTC timestamp (e.g. "2026-10-19 10:00:00" or "2026-10-19T10:00:00Z") to epoch */
ct_status_t ct_parse_timestamp_utc(const char *time_str, int64_t *out_epoch);

#endif /* CHRONOTRACE_CSV_PARSER_H */
