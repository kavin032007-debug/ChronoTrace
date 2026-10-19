#ifndef CHRONOTRACE_REPORT_H
#define CHRONOTRACE_REPORT_H

#include "storage.h"

typedef struct {
    char report_id[64];
    char created_at[CT_MAX_TIME_STR_LEN];
    char title[128];
    char summary[512];
    char **selected_event_ids;
    size_t event_count;
    char path_from[CT_MAX_ID_LEN];
    char path_to[CT_MAX_ID_LEN];
} ct_report_request_t;

ct_status_t ct_report_generate_html(const ct_dataset_t *ds, const ct_report_request_t *req,
                                    char **out_html, size_t *out_len);

ct_status_t ct_report_generate_json(const ct_dataset_t *ds, const ct_report_request_t *req,
                                    char **out_json, size_t *out_len);

#endif /* CHRONOTRACE_REPORT_H */
