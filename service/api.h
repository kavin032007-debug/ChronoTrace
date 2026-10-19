#ifndef CHRONOTRACE_API_H
#define CHRONOTRACE_API_H

#include "../ingest/storage.h"
#include "../third_party/cJSON.h"

/* API handlers returning cJSON* objects. Caller must cJSON_Delete() returned pointer. */
cJSON* ct_api_handle_import(ct_dataset_t *ds, const char *body, size_t body_len);
cJSON* ct_api_handle_get_events(ct_dataset_t *ds, const char *query_string);
cJSON* ct_api_handle_get_event_by_id(ct_dataset_t *ds, const char *event_id);
cJSON* ct_api_handle_get_entity_by_id(ct_dataset_t *ds, const char *entity_id);
cJSON* ct_api_handle_search(ct_dataset_t *ds, const char *prefix);
cJSON* ct_api_handle_graph(ct_dataset_t *ds, const char *query_string);
cJSON* ct_api_handle_path(ct_dataset_t *ds, const char *query_string);
cJSON* ct_api_handle_groups(ct_dataset_t *ds);
cJSON* ct_api_handle_timeline(ct_dataset_t *ds, const char *query_string);
cJSON* ct_api_handle_bursts(ct_dataset_t *ds, const char *query_string);
cJSON* ct_api_handle_integrity_verify(ct_dataset_t *ds);
cJSON* ct_api_handle_create_report(ct_dataset_t *ds, const char *body);
cJSON* ct_api_handle_get_report_by_id(ct_dataset_t *ds, const char *report_id);
cJSON* ct_api_execute_import_files(ct_dataset_t *ds, const char *ev_path, const char *ent_path, const char *rel_path);

/* Utility to parse query parameter value from query string (e.g. "?a=1&b=2") */
bool ct_get_query_param(const char *qs, const char *param, char *out_val, size_t out_val_size);

#endif /* CHRONOTRACE_API_H */
