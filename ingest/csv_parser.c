#include "csv_parser.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void ct_str_trim(char *str) {
    if (!str) return;
    char *start = str;
    while (*start && isspace((unsigned char)*start)) {
        start++;
    }
    char *end = start + strlen(start);
    while (end > start && isspace((unsigned char)*(end - 1))) {
        end--;
    }
    size_t len = (size_t)(end - start);
    memmove(str, start, len);
    str[len] = '\0';
}

ct_status_t ct_csv_parse_line(const char *line, ct_csv_row_t *out_row) {
    if (!line || !out_row) return CT_ERR_INVARG;
    memset(out_row, 0, sizeof(ct_csv_row_t));

    size_t col = 0;
    size_t char_idx = 0;
    bool in_quotes = false;
    const char *p = line;

    while (*p && *p != '\r' && *p != '\n') {
        if (*p == '"') {
            if (in_quotes && *(p + 1) == '"') {
                /* Escaped quote */
                if (char_idx + 1 < CT_CSV_MAX_FIELD_LEN) {
                    out_row->fields[col][char_idx++] = '"';
                }
                p += 2;
                continue;
            } else {
                in_quotes = !in_quotes;
                p++;
                continue;
            }
        } else if (*p == ',' && !in_quotes) {
            out_row->fields[col][char_idx] = '\0';
            ct_str_trim(out_row->fields[col]);
            col++;
            char_idx = 0;
            if (col >= CT_CSV_MAX_COLS) break;
            p++;
            continue;
        } else {
            if (char_idx + 1 < CT_CSV_MAX_FIELD_LEN) {
                out_row->fields[col][char_idx++] = *p;
            }
            p++;
        }
    }

    if (col < CT_CSV_MAX_COLS) {
        out_row->fields[col][char_idx] = '\0';
        ct_str_trim(out_row->fields[col]);
        col++;
    }

    out_row->col_count = col;
    return CT_OK;
}

static bool is_leap_year(int year) {
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

static const int days_before_month[12] = {
    0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334
};

ct_status_t ct_parse_timestamp_utc(const char *time_str, int64_t *out_epoch) {
    if (!time_str || !out_epoch) return CT_ERR_INVARG;

    int year = 0, month = 0, day = 0, hour = 0, min = 0, sec = 0;
    char sep = ' ';
    /* Accept YYYY-MM-DD HH:MM:SS or YYYY-MM-DDTHH:MM:SS */
    int matched = sscanf(time_str, "%d-%d-%d%c%d:%d:%d", &year, &month, &day, &sep, &hour, &min, &sec);
    if (matched < 7) {
        /* Try without seconds */
        matched = sscanf(time_str, "%d-%d-%d%c%d:%d", &year, &month, &day, &sep, &hour, &min);
        if (matched < 6) {
            return CT_ERR_PARSE;
        }
        sec = 0;
    }

    if (year < 1970 || month < 1 || month > 12 || day < 1 || day > 31 ||
        hour < 0 || hour > 23 || min < 0 || min > 59 || sec < 0 || sec > 60) {
        return CT_ERR_PARSE;
    }

    /* Calculate days since 1970-01-01 */
    int64_t days = 0;
    for (int y = 1970; y < year; y++) {
        days += is_leap_year(y) ? 366 : 365;
    }

    days += days_before_month[month - 1];
    if (month > 2 && is_leap_year(year)) {
        days += 1;
    }
    days += (day - 1);

    int64_t epoch = days * 86400LL + (int64_t)hour * 3600LL + (int64_t)min * 60LL + (int64_t)sec;
    *out_epoch = epoch;
    return CT_OK;
}
