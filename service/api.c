#include "api.h"
#include "../engine/graph/bfs.h"
#include "../engine/graph/dfs.h"
#include "../engine/graph/dijkstra.h"
#include "../engine/graph/tarjan.h"
#include "../engine/order/search.h"
#include "../ingest/csv_parser.h"
#include "../ingest/report.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

bool ct_get_query_param(const char *qs, const char *param, char *out_val, size_t out_val_size) {
    if (!qs || !param || !out_val || out_val_size == 0) return false;
    out_val[0] = '\0';

    if (*qs == '?') qs++;

    size_t param_len = strlen(param);
    const char *p = qs;

    while (*p) {
        if (strncmp(p, param, param_len) == 0 && p[param_len] == '=') {
            const char *val_start = p + param_len + 1;
            const char *val_end = strchr(val_start, '&');
            if (!val_end) val_end = val_start + strlen(val_start);

            size_t len = (size_t)(val_end - val_start);
            if (len >= out_val_size) len = out_val_size - 1;

            /* Copy and decode URL %20 etc */
            size_t out_idx = 0;
            for (size_t i = 0; i < len; i++) {
                if (val_start[i] == '%' && i + 2 < len) {
                    char hex[3] = { val_start[i+1], val_start[i+2], '\0' };
                    out_val[out_idx++] = (char)strtol(hex, NULL, 16);
                    i += 2;
                } else if (val_start[i] == '+') {
                    out_val[out_idx++] = ' ';
                } else {
                    out_val[out_idx++] = val_start[i];
                }
            }
            out_val[out_idx] = '\0';
            return true;
        }
        const char *next = strchr(p, '&');
        if (!next) break;
        p = next + 1;
    }
    return false;
}

cJSON* ct_api_build_import_response(ct_dataset_t *ds, ct_status_t st) {
    cJSON *root = cJSON_CreateObject();
    if (st != CT_OK && ds->summary.valid_events == 0 && ds->summary.valid_entities == 0) {
        cJSON_AddStringToObject(root, "status", "error");
        cJSON_AddStringToObject(root, "error", "Failed to import datasets. No valid records could be processed.");
        cJSON_AddStringToObject(root, "message", "Failed to import datasets. No valid records could be processed.");
        cJSON_AddNumberToObject(root, "code", st);
    } else {
        cJSON_AddStringToObject(root, "status", "success");
        cJSON_AddNumberToObject(root, "dataset_version", (double)ds->version);
    }

    cJSON *counts = cJSON_CreateObject();
    cJSON_AddNumberToObject(counts, "events", ds->summary.valid_events);
    cJSON_AddNumberToObject(counts, "entities", ds->summary.valid_entities);
    cJSON_AddNumberToObject(counts, "relationships", ds->summary.valid_relationships);
    cJSON_AddItemToObject(root, "counts", counts);

    cJSON *summary = cJSON_CreateObject();
    cJSON_AddNumberToObject(summary, "total_rows_read", ds->summary.total_rows_read);
    cJSON_AddNumberToObject(summary, "valid_entities", ds->summary.valid_entities);
    cJSON_AddNumberToObject(summary, "valid_events", ds->summary.valid_events);
    cJSON_AddNumberToObject(summary, "valid_relationships", ds->summary.valid_relationships);
    cJSON_AddNumberToObject(summary, "rejected_rows", ds->summary.rejected_rows);
    cJSON_AddItemToObject(root, "summary", summary);

    cJSON *rej_simple = cJSON_CreateArray();
    cJSON *rej_full = cJSON_CreateArray();
    for (size_t i = 0; i < ds->rejections.size; i++) {
        const ct_rejection_t *rej = (const ct_rejection_t*)ct_vector_get(&ds->rejections, i);
        if (rej) {
            cJSON *item1 = cJSON_CreateObject();
            cJSON_AddStringToObject(item1, "file", rej->source_file);
            cJSON_AddNumberToObject(item1, "line", rej->line_number);
            cJSON_AddStringToObject(item1, "reason", rej->reason);
            cJSON_AddStringToObject(item1, "raw_line", rej->raw_line);
            cJSON_AddItemToArray(rej_simple, item1);

            cJSON *item2 = cJSON_CreateObject();
            cJSON_AddStringToObject(item2, "source_file", rej->source_file);
            cJSON_AddNumberToObject(item2, "line_number", rej->line_number);
            cJSON_AddStringToObject(item2, "reason", rej->reason);
            cJSON_AddStringToObject(item2, "raw_line", rej->raw_line);
            cJSON_AddItemToArray(rej_full, item2);
        }
    }
    cJSON_AddItemToObject(root, "rejected", rej_simple);
    cJSON_AddItemToObject(root, "rejections", rej_full);
    return root;
}

cJSON* ct_api_execute_import_files(ct_dataset_t *ds, const char *ev_path, const char *ent_path, const char *rel_path) {
    if (!ds || !ev_path || !ent_path || !rel_path) {
        cJSON *err = cJSON_CreateObject();
        cJSON_AddStringToObject(err, "status", "error");
        cJSON_AddStringToObject(err, "error", "Invalid dataset arguments.");
        cJSON_AddStringToObject(err, "message", "Invalid dataset arguments.");
        return err;
    }

    ct_dataset_reset(ds);
    ds->version++;
    ct_status_t st = ct_dataset_import_csvs(ds, ev_path, ent_path, rel_path);
    return ct_api_build_import_response(ds, st);
}

static bool ct_find_multipart_part(const char *body, size_t body_len, const char *key, const char **out_data, size_t *out_len) {
    if (!body || body_len == 0 || !key || !out_data || !out_len) return false;
    *out_data = NULL;
    *out_len = 0;

    char key_pattern1[64], key_pattern2[64];
    snprintf(key_pattern1, sizeof(key_pattern1), "name=\"%s\"", key);
    snprintf(key_pattern2, sizeof(key_pattern2), "filename=\"%s", key);

    const char *pos = body;
    const char *body_end = body + body_len;

    while (pos < body_end) {
        const char *found1 = strstr(pos, key_pattern1);
        const char *found2 = strstr(pos, key_pattern2);
        const char *found = NULL;
        if (found1 && found2) found = (found1 < found2) ? found1 : found2;
        else if (found1) found = found1;
        else found = found2;

        if (!found || found >= body_end) break;

        const char *head_end = strstr(found, "\r\n\r\n");
        size_t delim_len = 4;
        if (!head_end || head_end >= body_end) {
            head_end = strstr(found, "\n\n");
            delim_len = 2;
        }
        if (!head_end || head_end >= body_end) break;

        const char *data_start = head_end + delim_len;
        if (data_start >= body_end) break;

        const char *data_end = body_end;
        for (const char *p = data_start; p < body_end - 3; p++) {
            if ((p[0] == '\r' && p[1] == '\n' && p[2] == '-' && p[3] == '-') ||
                (p[0] == '\n' && p[1] == '-' && p[2] == '-')) {
                data_end = p;
                break;
            }
        }

        *out_data = data_start;
        *out_len = (size_t)(data_end - data_start);
        return true;
    }
    return false;
}

cJSON* ct_api_handle_import(ct_dataset_t *ds, const char *body, size_t body_len) {
    if (!body || body_len == 0) {
        cJSON *err = cJSON_CreateObject();
        cJSON_AddStringToObject(err, "status", "error");
        cJSON_AddStringToObject(err, "error", "No evidence files provided. Upload events.csv, entities.csv and relationships.csv to begin.");
        cJSON_AddStringToObject(err, "message", "No evidence files provided. Upload events.csv, entities.csv and relationships.csv to begin.");
        return err;
    }

    char ev_path[512], ent_path[512], rel_path[512];
    snprintf(ev_path, sizeof(ev_path), "%s/imported_events.csv", ds->data_dir);
    snprintf(ent_path, sizeof(ent_path), "%s/imported_entities.csv", ds->data_dir);
    snprintf(rel_path, sizeof(rel_path), "%s/imported_relationships.csv", ds->data_dir);

    /* Check if payload is JSON */
    if (body[0] == '{') {
        cJSON *json = cJSON_Parse(body);
        if (!json) {
            cJSON *err = cJSON_CreateObject();
            cJSON_AddStringToObject(err, "status", "error");
            cJSON_AddStringToObject(err, "error", "Invalid import payload. Expected JSON with events_csv, entities_csv, and relationships_csv.");
            cJSON_AddStringToObject(err, "message", "Invalid import payload. Expected JSON with events_csv, entities_csv, and relationships_csv.");
            return err;
        }

        cJSON *ev_item = cJSON_GetObjectItem(json, "events_csv");
        if (!ev_item) ev_item = cJSON_GetObjectItem(json, "events");
        cJSON *ent_item = cJSON_GetObjectItem(json, "entities_csv");
        if (!ent_item) ent_item = cJSON_GetObjectItem(json, "entities");
        cJSON *rel_item = cJSON_GetObjectItem(json, "relationships_csv");
        if (!rel_item) rel_item = cJSON_GetObjectItem(json, "relationships");

        if (!ev_item || !cJSON_IsString(ev_item) ||
            !ent_item || !cJSON_IsString(ent_item) ||
            !rel_item || !cJSON_IsString(rel_item)) {
            cJSON_Delete(json);
            cJSON *err = cJSON_CreateObject();
            cJSON_AddStringToObject(err, "status", "error");
            cJSON_AddStringToObject(err, "error", "Missing required CSV files. Please provide events_csv, entities_csv, and relationships_csv.");
            cJSON_AddStringToObject(err, "message", "Missing required CSV files. Please provide events_csv, entities_csv, and relationships_csv.");
            return err;
        }

        FILE *fev = fopen(ev_path, "wb");
        if (fev) { fputs(ev_item->valuestring, fev); fclose(fev); }
        FILE *fent = fopen(ent_path, "wb");
        if (fent) { fputs(ent_item->valuestring, fent); fclose(fent); }
        FILE *frel = fopen(rel_path, "wb");
        if (frel) { fputs(rel_item->valuestring, frel); fclose(frel); }

        cJSON_Delete(json);
        return ct_api_execute_import_files(ds, ev_path, ent_path, rel_path);
    }

    /* Otherwise parse as multipart/form-data */
    const char *ev_data = NULL, *ent_data = NULL, *rel_data = NULL;
    size_t ev_len = 0, ent_len = 0, rel_len = 0;

    bool has_ev = ct_find_multipart_part(body, body_len, "events", &ev_data, &ev_len);
    bool has_ent = ct_find_multipart_part(body, body_len, "entities", &ent_data, &ent_len);
    bool has_rel = ct_find_multipart_part(body, body_len, "relationships", &rel_data, &rel_len);

    if (!has_ev || !has_ent || !has_rel) {
        cJSON *err = cJSON_CreateObject();
        cJSON_AddStringToObject(err, "status", "error");
        cJSON_AddStringToObject(err, "error", "Missing required CSV files. Please upload events.csv, entities.csv, and relationships.csv.");
        cJSON_AddStringToObject(err, "message", "Missing required CSV files. Please upload events.csv, entities.csv, and relationships.csv.");
        return err;
    }

    FILE *fev = fopen(ev_path, "wb");
    if (fev) { fwrite(ev_data, 1, ev_len, fev); fclose(fev); }
    FILE *fent = fopen(ent_path, "wb");
    if (fent) { fwrite(ent_data, 1, ent_len, fent); fclose(fent); }
    FILE *frel = fopen(rel_path, "wb");
    if (frel) { fwrite(rel_data, 1, rel_len, frel); fclose(frel); }

    return ct_api_execute_import_files(ds, ev_path, ent_path, rel_path);
}

cJSON* ct_api_handle_get_events(ct_dataset_t *ds, const char *query_string) {
    char start_str[64] = {0}, end_str[64] = {0}, type_str[64] = {0}, ent_str[64] = {0}, sev_str[16] = {0};
    char limit_str[16] = {0}, offset_str[16] = {0};

    ct_get_query_param(query_string, "start", start_str, sizeof(start_str));
    ct_get_query_param(query_string, "end", end_str, sizeof(end_str));
    ct_get_query_param(query_string, "type", type_str, sizeof(type_str));
    ct_get_query_param(query_string, "entity_id", ent_str, sizeof(ent_str));
    if (ent_str[0] == '\0') {
        ct_get_query_param(query_string, "entity", ent_str, sizeof(ent_str));
    }
    ct_get_query_param(query_string, "severity", sev_str, sizeof(sev_str));
    if (sev_str[0] == '\0') {
        ct_get_query_param(query_string, "min_severity", sev_str, sizeof(sev_str));
    }
    ct_get_query_param(query_string, "limit", limit_str, sizeof(limit_str));
    ct_get_query_param(query_string, "offset", offset_str, sizeof(offset_str));

    ct_timeline_filter_t filter;
    memset(&filter, 0, sizeof(filter));

    if (start_str[0] != '\0') {
        int64_t ep = 0;
        if (ct_parse_timestamp_utc(start_str, &ep) == CT_OK) {
            filter.start_epoch = ep;
        } else {
            filter.start_epoch = atoll(start_str);
        }
    }

    if (end_str[0] != '\0') {
        int64_t ep = 0;
        if (ct_parse_timestamp_utc(end_str, &ep) == CT_OK) {
            filter.end_epoch = ep;
        } else {
            filter.end_epoch = atoll(end_str);
        }
    }

    if (type_str[0] != '\0') filter.event_type = type_str;
    if (ent_str[0] != '\0') filter.entity_id = ent_str;
    if (sev_str[0] != '\0') filter.min_severity = atoi(sev_str);

    filter.limit = (limit_str[0] != '\0') ? (size_t)atoi(limit_str) : 10000;
    filter.offset = (offset_str[0] != '\0') ? (size_t)atoi(offset_str) : 0;

    ct_event_t *results = NULL;
    size_t count = 0;
    size_t total_matched = 0;

    ct_timeline_filter(&ds->timeline, &filter, &results, &count, &total_matched);

    cJSON *root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "total", (double)total_matched);
    cJSON_AddNumberToObject(root, "count", (double)count);
    cJSON_AddNumberToObject(root, "limit", (double)filter.limit);
    cJSON_AddNumberToObject(root, "offset", (double)filter.offset);

    cJSON *events_arr = cJSON_CreateArray();
    for (size_t i = 0; i < count; i++) {
        cJSON *ev = cJSON_CreateObject();
        cJSON_AddStringToObject(ev, "event_id", results[i].event_id);
        cJSON_AddNumberToObject(ev, "timestamp_epoch", (double)results[i].timestamp_epoch);
        cJSON_AddStringToObject(ev, "timestamp", results[i].timestamp_str);
        cJSON_AddStringToObject(ev, "entity_id", results[i].entity_id);
        cJSON_AddStringToObject(ev, "event_type", results[i].event_type);
        cJSON_AddNumberToObject(ev, "severity", results[i].severity);
        cJSON_AddItemToArray(events_arr, ev);
    }
    cJSON_AddItemToObject(root, "events", events_arr);

    if (results) free(results);
    return root;
}

cJSON* ct_api_handle_get_event_by_id(ct_dataset_t *ds, const char *event_id) {
    if (!ds || !event_id) return NULL;

    void *val = NULL;
    if (ct_hashmap_get(&ds->events_map, event_id, &val) != CT_OK) {
        cJSON *err = cJSON_CreateObject();
        cJSON_AddStringToObject(err, "status", "error");
        cJSON_AddStringToObject(err, "message", "Event not found");
        return err;
    }

    const ct_event_t *ev = (const ct_event_t*)val;
    cJSON *root = cJSON_CreateObject();
    cJSON *obj = cJSON_CreateObject();
    cJSON_AddStringToObject(obj, "event_id", ev->event_id);
    cJSON_AddNumberToObject(obj, "timestamp_epoch", (double)ev->timestamp_epoch);
    cJSON_AddStringToObject(obj, "timestamp", ev->timestamp_str);
    cJSON_AddStringToObject(obj, "entity_id", ev->entity_id);
    cJSON_AddStringToObject(obj, "event_type", ev->event_type);
    cJSON_AddNumberToObject(obj, "severity", ev->severity);
    cJSON_AddItemToObject(root, "event", obj);

    return root;
}

cJSON* ct_api_handle_get_entity_by_id(ct_dataset_t *ds, const char *entity_id) {
    if (!ds || !entity_id) return NULL;

    void *val = NULL;
    if (ct_hashmap_get(&ds->entities_map, entity_id, &val) != CT_OK) {
        cJSON *err = cJSON_CreateObject();
        cJSON_AddStringToObject(err, "status", "error");
        cJSON_AddStringToObject(err, "error", "Entity not found");
        cJSON_AddStringToObject(err, "message", "Entity not found");
        return err;
    }

    const ct_entity_t *ent = (const ct_entity_t*)val;
    cJSON *root = cJSON_CreateObject();
    cJSON *obj = cJSON_CreateObject();
    cJSON_AddStringToObject(obj, "entity_id", ent->entity_id);
    cJSON_AddStringToObject(obj, "entity_type", ent->entity_type);
    cJSON_AddStringToObject(obj, "name", ent->name);
    cJSON_AddItemToObject(root, "entity", obj);

    /* Gather direct connections */
    cJSON *conn_arr = cJSON_CreateArray();
    cJSON *nodes_arr = cJSON_CreateArray();
    cJSON *edges_arr = cJSON_CreateArray();

    /* Add center node to nodes array */
    cJSON *center_node = cJSON_CreateObject();
    cJSON_AddStringToObject(center_node, "id", ent->entity_id);
    cJSON_AddStringToObject(center_node, "name", ent->name);
    cJSON_AddStringToObject(center_node, "label", ent->name);
    cJSON_AddStringToObject(center_node, "type", ent->entity_type);
    cJSON_AddItemToArray(nodes_arr, center_node);

    int32_t node_idx = ent->node_index;
    const ct_graph_node_t *node = ct_graph_get_node(&ds->graph, node_idx);
    int32_t e = node ? node->first_edge : -1;

    while (e >= 0) {
        const ct_graph_edge_t *edge = ct_graph_get_edge(&ds->graph, e);
        if (!edge) break;

        const ct_graph_node_t *target_node = ct_graph_get_node(&ds->graph, edge->target_node);
        if (target_node) {
            cJSON *c = cJSON_CreateObject();
            cJSON_AddStringToObject(c, "target_id", target_node->id);
            cJSON_AddStringToObject(c, "target_label", target_node->label);
            cJSON_AddStringToObject(c, "relationship", edge->label);
            cJSON_AddStringToObject(c, "record_id", edge->record_id);
            cJSON_AddBoolToObject(c, "is_inferred", edge->is_inferred);
            cJSON_AddItemToArray(conn_arr, c);

            cJSON *tn = cJSON_CreateObject();
            cJSON_AddStringToObject(tn, "id", target_node->id);
            cJSON_AddStringToObject(tn, "name", target_node->label);
            cJSON_AddStringToObject(tn, "label", target_node->label);
            cJSON_AddStringToObject(tn, "type", target_node->type);
            cJSON_AddItemToArray(nodes_arr, tn);

            cJSON *ed = cJSON_CreateObject();
            cJSON_AddStringToObject(ed, "record_id", edge->record_id);
            cJSON_AddStringToObject(ed, "source", ent->entity_id);
            cJSON_AddStringToObject(ed, "target", target_node->id);
            cJSON_AddStringToObject(ed, "relationship", edge->label);
            cJSON_AddStringToObject(ed, "label", edge->label);
            cJSON_AddItemToArray(edges_arr, ed);
        }
        e = edge->next_edge;
    }
    cJSON_AddItemToObject(root, "connections", conn_arr);
    cJSON_AddItemToObject(root, "nodes", nodes_arr);
    cJSON_AddItemToObject(root, "edges", edges_arr);

    return root;
}

cJSON* ct_api_handle_search(ct_dataset_t *ds, const char *prefix) {
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "prefix", prefix ? prefix : "");

    ct_suggestion_t *sugg = NULL;
    size_t count = 0;
    ct_trie_search_prefix(&ds->search_trie, prefix ? prefix : "", 15, &sugg, &count);

    cJSON *results_arr = cJSON_CreateArray();
    for (size_t i = 0; i < count; i++) {
        cJSON *item = cJSON_CreateObject();
        cJSON_AddStringToObject(item, "id", sugg[i].id);
        cJSON_AddStringToObject(item, "name", sugg[i].name);
        cJSON_AddStringToObject(item, "label", sugg[i].name);
        cJSON_AddStringToObject(item, "type", sugg[i].type);
        cJSON_AddItemToArray(results_arr, item);
    }
    cJSON_AddItemToObject(root, "results", results_arr);

    ct_suggestions_free(sugg);
    return root;
}

cJSON* ct_api_handle_graph(ct_dataset_t *ds, const char *query_string) {
    char entity_id[64] = {0}, depth_str[16] = {0}, mode_str[16] = {0};
    char include_iso_str[16] = {0}, all_str[16] = {0};
    ct_get_query_param(query_string, "entity_id", entity_id, sizeof(entity_id));
    if (entity_id[0] == '\0') {
        ct_get_query_param(query_string, "center", entity_id, sizeof(entity_id));
    }
    ct_get_query_param(query_string, "depth", depth_str, sizeof(depth_str));
    ct_get_query_param(query_string, "mode", mode_str, sizeof(mode_str));
    ct_get_query_param(query_string, "include_isolated", include_iso_str, sizeof(include_iso_str));
    ct_get_query_param(query_string, "all", all_str, sizeof(all_str));

    bool include_isolated = (strcmp(include_iso_str, "1") == 0 || strcmp(include_iso_str, "true") == 0);
    bool return_all = (strcmp(all_str, "1") == 0 || strcmp(all_str, "true") == 0 || strcmp(entity_id, "*") == 0 || strcmp(entity_id, "all") == 0);

    uint32_t depth = (depth_str[0] != '\0') ? (uint32_t)atoi(depth_str) : 2;
    if (depth < 1) depth = 1;
    if (depth > 5) depth = 5;

    bool use_dfs = (strcmp(mode_str, "dfs") == 0);

    /* Check LRU cache */
    char cache_key[128];
    snprintf(cache_key, sizeof(cache_key), "graph:%s:%u:%s:%d:%d", entity_id, depth, use_dfs ? "dfs" : "bfs", include_isolated, return_all);
    const char *cached_json = NULL;
    if (ct_lru_get(&ds->query_cache, cache_key, &cached_json) == CT_OK && cached_json) {
        cJSON *parsed = cJSON_Parse(cached_json);
        if (parsed) return parsed;
    }

    int32_t start_node = -1;
    if (entity_id[0] != '\0') {
        if (ct_graph_find_node(&ds->graph, entity_id, &start_node) != CT_OK) {
            cJSON *err = cJSON_CreateObject();
            cJSON_AddStringToObject(err, "status", "error");
            cJSON_AddStringToObject(err, "error", "Center entity or event not found");
            cJSON_AddStringToObject(err, "message", "Center entity or event not found");
            return err;
        }
    } else if (ct_graph_node_count(&ds->graph) > 0) {
        start_node = 0;
    } else {
        cJSON *empty = cJSON_CreateObject();
        cJSON_AddItemToObject(empty, "nodes", cJSON_CreateArray());
        cJSON_AddItemToObject(empty, "edges", cJSON_CreateArray());
        return empty;
    }

    int32_t *visited_nodes = NULL;
    int32_t *visited_dist = NULL;
    size_t visited_count = 0;

    if (!use_dfs) {
        ct_bfs_explore(&ds->graph, start_node, depth, &visited_nodes, &visited_dist, &visited_count);
    } else {
        ct_dfs_result_t dfs_res;
        ct_dfs_traverse(&ds->graph, start_node, depth, &dfs_res);
        visited_count = dfs_res.visited_count;
        visited_nodes = (int32_t*)malloc(visited_count * sizeof(int32_t));
        visited_dist = (int32_t*)calloc(visited_count, sizeof(int32_t));
        if (visited_nodes) {
            memcpy(visited_nodes, dfs_res.visited_order, visited_count * sizeof(int32_t));
        }
        ct_dfs_result_free(&dfs_res);
    }

    /* Build visited node index set for fast edge lookup */
    size_t total_g_nodes = ct_graph_node_count(&ds->graph);
    bool *in_subgraph = (bool*)calloc(total_g_nodes, sizeof(bool));
    for (size_t i = 0; i < visited_count; i++) {
        in_subgraph[visited_nodes[i]] = true;
    }

    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "center_entity", entity_id);
    cJSON_AddNumberToObject(root, "depth", depth);
    cJSON_AddStringToObject(root, "mode", use_dfs ? "dfs" : "bfs");

    cJSON *nodes_arr = cJSON_CreateArray();
    for (size_t i = 0; i < visited_count; i++) {
        int32_t u = visited_nodes[i];
        const ct_graph_node_t *node = ct_graph_get_node(&ds->graph, u);
        if (node) {
            cJSON *n_obj = cJSON_CreateObject();
            cJSON_AddStringToObject(n_obj, "id", node->id);
            cJSON_AddStringToObject(n_obj, "name", node->label);
            cJSON_AddStringToObject(n_obj, "label", node->label);
            cJSON_AddStringToObject(n_obj, "type", node->type);
            cJSON_AddStringToObject(n_obj, "kind", (node->kind == CT_NODE_EVENT) ? "event" : "entity");
            cJSON_AddNumberToObject(n_obj, "timestamp", (double)node->timestamp);
            cJSON_AddNumberToObject(n_obj, "severity", node->severity);
            cJSON_AddNumberToObject(n_obj, "distance", visited_dist ? visited_dist[i] : 0);
            cJSON_AddItemToArray(nodes_arr, n_obj);
        }
    }
    if (include_isolated || return_all) {
        for (size_t i = 0; i < total_g_nodes; i++) {
            if (!in_subgraph[i]) {
                const ct_graph_node_t *node = ct_graph_get_node(&ds->graph, (int32_t)i);
                if (node && (return_all || node->first_edge < 0)) {
                    cJSON *n_obj = cJSON_CreateObject();
                    cJSON_AddStringToObject(n_obj, "id", node->id);
                    cJSON_AddStringToObject(n_obj, "name", node->label);
                    cJSON_AddStringToObject(n_obj, "label", node->label);
                    cJSON_AddStringToObject(n_obj, "type", node->type);
                    cJSON_AddStringToObject(n_obj, "kind", (node->kind == CT_NODE_EVENT) ? "event" : "entity");
                    cJSON_AddNumberToObject(n_obj, "timestamp", (double)node->timestamp);
                    cJSON_AddNumberToObject(n_obj, "severity", node->severity);
                    cJSON_AddNumberToObject(n_obj, "distance", -1);
                    cJSON_AddItemToArray(nodes_arr, n_obj);
                    in_subgraph[i] = true;
                }
            }
        }
    }
    cJSON_AddItemToObject(root, "nodes", nodes_arr);

    /* Collect edges between visited nodes */
    cJSON *edges_arr = cJSON_CreateArray();
    for (size_t i = 0; i < total_g_nodes; i++) {
        if (!in_subgraph[i]) continue;
        int32_t u = (int32_t)i;
        const ct_graph_node_t *node_u = ct_graph_get_node(&ds->graph, u);
        int32_t e = node_u ? node_u->first_edge : -1;

        while (e >= 0) {
            const ct_graph_edge_t *edge = ct_graph_get_edge(&ds->graph, e);
            if (!edge) break;

            int32_t v = edge->target_node;
            if (v >= 0 && (size_t)v < total_g_nodes && in_subgraph[v] && u <= v) { /* Avoid double-adding undirected edges */
                const ct_graph_node_t *node_v = ct_graph_get_node(&ds->graph, v);
                if (node_v) {
                    cJSON *e_obj = cJSON_CreateObject();
                    cJSON_AddStringToObject(e_obj, "source", node_u->id);
                    cJSON_AddStringToObject(e_obj, "target", node_v->id);
                    cJSON_AddStringToObject(e_obj, "relationship", edge->label);
                    cJSON_AddStringToObject(e_obj, "label", edge->label);
                    cJSON_AddStringToObject(e_obj, "record_id", edge->record_id);
                    cJSON_AddNumberToObject(e_obj, "weight", edge->weight);
                    cJSON_AddBoolToObject(e_obj, "is_inferred", edge->is_inferred);
                    cJSON_AddItemToArray(edges_arr, e_obj);
                }
            }
            e = edge->next_edge;
        }
    }
    cJSON_AddItemToObject(root, "edges", edges_arr);

    free(in_subgraph);
    if (visited_nodes) free(visited_nodes);
    if (visited_dist) free(visited_dist);

    /* Cache response in LRU */
    char *serialized = cJSON_PrintUnformatted(root);
    if (serialized) {
        ct_lru_put(&ds->query_cache, cache_key, serialized);
        free(serialized);
    }

    return root;
}

cJSON* ct_api_handle_path(ct_dataset_t *ds, const char *query_string) {
    char from_id[64] = {0}, to_id[64] = {0}, mode_str[32] = {0};
    ct_get_query_param(query_string, "from", from_id, sizeof(from_id));
    ct_get_query_param(query_string, "to", to_id, sizeof(to_id));
    ct_get_query_param(query_string, "mode", mode_str, sizeof(mode_str));

    if (from_id[0] == '\0' || to_id[0] == '\0') {
        cJSON *err = cJSON_CreateObject();
        cJSON_AddStringToObject(err, "status", "error");
        cJSON_AddStringToObject(err, "error", "Missing required parameters: from and to");
        cJSON_AddStringToObject(err, "message", "Missing required parameters: from and to");
        return err;
    }

    bool use_time_gap = (strcmp(mode_str, "dijkstra") == 0 || strcmp(mode_str, "time") == 0 || strcmp(mode_str, "time_gap") == 0 || strcmp(mode_str, "lowest_time_gap") == 0 || strcmp(mode_str, "weighted") == 0);

    /* Check LRU cache */
    char cache_key[128];
    snprintf(cache_key, sizeof(cache_key), "path:%s:%s:%s", from_id, to_id, use_time_gap ? "time" : "hops");
    const char *cached = NULL;
    if (ct_lru_get(&ds->query_cache, cache_key, &cached) == CT_OK && cached) {
        cJSON *parsed = cJSON_Parse(cached);
        if (parsed) return parsed;
    }

    int32_t u_idx = -1, v_idx = -1;
    if (ct_graph_find_node(&ds->graph, from_id, &u_idx) != CT_OK ||
        ct_graph_find_node(&ds->graph, to_id, &v_idx) != CT_OK) {
        cJSON *root = cJSON_CreateObject();
        cJSON_AddBoolToObject(root, "found", false);
        cJSON_AddStringToObject(root, "from", from_id);
        cJSON_AddStringToObject(root, "to", to_id);
        cJSON_AddNumberToObject(root, "cost", 0);
        cJSON_AddItemToObject(root, "nodes", cJSON_CreateArray());
        cJSON_AddItemToObject(root, "edges", cJSON_CreateArray());
        cJSON_AddStringToObject(root, "message", "Endpoint node not found in evidence records");
        return root;
    }

    /* Constant-time connectivity pre-check via DSU */
    if (ds->dsu_initialized && !ct_dsu_connected(&ds->dsu, u_idx, v_idx)) {
        cJSON *root = cJSON_CreateObject();
        cJSON_AddBoolToObject(root, "found", false);
        cJSON_AddStringToObject(root, "from", from_id);
        cJSON_AddStringToObject(root, "to", to_id);
        cJSON_AddNumberToObject(root, "cost", 0);
        cJSON_AddItemToObject(root, "nodes", cJSON_CreateArray());
        cJSON_AddItemToObject(root, "edges", cJSON_CreateArray());
        cJSON_AddStringToObject(root, "message", "Entities are in disconnected evidence groups");
        return root;
    }

    ct_path_result_t path;
    ct_status_t st = use_time_gap ?
        ct_dijkstra_shortest_path(&ds->graph, u_idx, v_idx, &path) :
        ct_bfs_shortest_path(&ds->graph, u_idx, v_idx, &path);

    if (st != CT_OK) {
        cJSON *root = cJSON_CreateObject();
        cJSON_AddBoolToObject(root, "found", false);
        cJSON_AddStringToObject(root, "from", from_id);
        cJSON_AddStringToObject(root, "to", to_id);
        cJSON_AddNumberToObject(root, "cost", 0);
        cJSON_AddItemToObject(root, "nodes", cJSON_CreateArray());
        cJSON_AddItemToObject(root, "edges", cJSON_CreateArray());
        cJSON_AddStringToObject(root, "message", "No connecting pathway found");
        return root;
    }

    cJSON *root = cJSON_CreateObject();
    cJSON_AddBoolToObject(root, "found", true);
    cJSON_AddStringToObject(root, "from", from_id);
    cJSON_AddStringToObject(root, "to", to_id);
    cJSON_AddStringToObject(root, "mode", use_time_gap ? "lowest_time_gap" : "fewest_hops");
    cJSON_AddNumberToObject(root, "hops", path.hop_count);
    cJSON_AddNumberToObject(root, "cost", path.total_cost);
    cJSON_AddNumberToObject(root, "total_cost", path.total_cost);

    cJSON *nodes_arr = cJSON_CreateArray();
    cJSON *path_arr = cJSON_CreateArray();
    for (uint32_t i = 0; i <= path.hop_count; i++) {
        const ct_graph_node_t *n = ct_graph_get_node(&ds->graph, path.node_path[i]);
        if (n) {
            cJSON *item = cJSON_CreateObject();
            cJSON_AddStringToObject(item, "id", n->id);
            cJSON_AddStringToObject(item, "name", n->label);
            cJSON_AddStringToObject(item, "label", n->label);
            cJSON_AddStringToObject(item, "type", n->type);
            cJSON_AddStringToObject(item, "kind", (n->kind == CT_NODE_EVENT) ? "event" : "entity");
            cJSON_AddItemToArray(nodes_arr, item);

            cJSON *item_legacy = cJSON_CreateObject();
            cJSON_AddStringToObject(item_legacy, "id", n->id);
            cJSON_AddStringToObject(item_legacy, "label", n->label);
            cJSON_AddStringToObject(item_legacy, "type", n->type);
            cJSON_AddStringToObject(item_legacy, "kind", (n->kind == CT_NODE_EVENT) ? "event" : "entity");
            cJSON_AddItemToArray(path_arr, item_legacy);
        }
    }
    cJSON_AddItemToObject(root, "nodes", nodes_arr);
    cJSON_AddItemToObject(root, "path", path_arr);

    cJSON *edges_arr = cJSON_CreateArray();
    cJSON *records_arr = cJSON_CreateArray();
    for (uint32_t i = 0; i < path.hop_count; i++) {
        cJSON_AddItemToArray(records_arr, cJSON_CreateString(path.record_ids[i]));

        const ct_graph_node_t *src_n = ct_graph_get_node(&ds->graph, path.node_path[i]);
        const ct_graph_node_t *dst_n = ct_graph_get_node(&ds->graph, path.node_path[i+1]);
        const char *rel = "connected_to";
        if (src_n) {
            int32_t ce = src_n->first_edge;
            while (ce >= 0) {
                const ct_graph_edge_t *edge = ct_graph_get_edge(&ds->graph, ce);
                if (!edge) break;
                if (edge->target_node == path.node_path[i+1] && strcmp(edge->record_id, path.record_ids[i]) == 0) {
                    rel = edge->label;
                    break;
                }
                ce = edge->next_edge;
            }
        }

        cJSON *ed = cJSON_CreateObject();
        cJSON_AddStringToObject(ed, "record_id", path.record_ids[i]);
        cJSON_AddStringToObject(ed, "source", src_n ? src_n->id : "");
        cJSON_AddStringToObject(ed, "target", dst_n ? dst_n->id : "");
        cJSON_AddStringToObject(ed, "relationship", rel);
        cJSON_AddStringToObject(ed, "label", rel);
        cJSON_AddItemToArray(edges_arr, ed);
    }
    cJSON_AddItemToObject(root, "edges", edges_arr);
    cJSON_AddItemToObject(root, "supporting_records", records_arr);

    ct_path_result_free(&path);

    /* Cache response in LRU */
    char *serialized = cJSON_PrintUnformatted(root);
    if (serialized) {
        ct_lru_put(&ds->query_cache, cache_key, serialized);
        free(serialized);
    }

    return root;
}

cJSON* ct_api_handle_groups(ct_dataset_t *ds) {
    cJSON *root = cJSON_CreateObject();

    size_t num_nodes = ct_graph_node_count(&ds->graph);

    /* Collect groups using DSU roots */
    typedef struct {
        int32_t root_id;
        cJSON *members_arr;
        size_t size;
    } comp_group_t;

    comp_group_t *groups = (comp_group_t*)malloc(num_nodes * sizeof(comp_group_t));
    int32_t *root_to_group_idx = (int32_t*)malloc(num_nodes * sizeof(int32_t));
    for (size_t i = 0; i < num_nodes; i++) root_to_group_idx[i] = -1;

    size_t group_cnt = 0;
    if (ds->dsu_initialized) {
        for (size_t i = 0; i < num_nodes; i++) {
            int32_t r = ct_dsu_find(&ds->dsu, (int32_t)i);
            if (r >= 0 && (size_t)r < num_nodes) {
                int32_t g_idx = root_to_group_idx[r];
                if (g_idx == -1) {
                    g_idx = (int32_t)group_cnt++;
                    root_to_group_idx[r] = g_idx;
                    groups[g_idx].root_id = r;
                    groups[g_idx].members_arr = cJSON_CreateArray();
                    groups[g_idx].size = 0;
                }

                const ct_graph_node_t *node = ct_graph_get_node(&ds->graph, (int32_t)i);
                if (node) {
                    /* Add string ID so (g.members||[]).map(m=>link(m,'graph')) displays entity ID chip */
                    cJSON_AddItemToArray(groups[g_idx].members_arr, cJSON_CreateString(node->id));
                    groups[g_idx].size++;
                }
            }
        }
    }

    cJSON *groups_arr = cJSON_CreateArray();
    for (size_t g = 0; g < group_cnt; g++) {
        cJSON *g_obj = cJSON_CreateObject();
        const ct_graph_node_t *r_node = ct_graph_get_node(&ds->graph, groups[g].root_id);
        cJSON_AddNumberToObject(g_obj, "id", (double)(g + 1));
        cJSON_AddStringToObject(g_obj, "root_id", r_node ? r_node->id : "");
        cJSON_AddNumberToObject(g_obj, "size", groups[g].size);
        cJSON_AddItemToObject(g_obj, "members", groups[g].members_arr);
        cJSON_AddItemToArray(groups_arr, g_obj);
    }
    cJSON_AddItemToObject(root, "groups", groups_arr);
    cJSON_AddNumberToObject(root, "group_count", group_cnt);

    free(groups);
    free(root_to_group_idx);

    /* Articulation points & bridges via Tarjan */
    ct_tarjan_result_t tarjan_res;
    if (ct_tarjan_analyze(&ds->graph, &tarjan_res) == CT_OK) {
        cJSON *linking_arr = cJSON_CreateArray();
        cJSON *ap_arr = cJSON_CreateArray();
        for (size_t i = 0; i < tarjan_res.articulation_count; i++) {
            const ct_graph_node_t *n = ct_graph_get_node(&ds->graph, tarjan_res.articulation_nodes[i]);
            if (n) {
                cJSON *item = cJSON_CreateObject();
                cJSON_AddStringToObject(item, "id", n->id);
                cJSON_AddStringToObject(item, "name", n->label);
                cJSON_AddStringToObject(item, "label", n->label);
                cJSON_AddStringToObject(item, "reason", "Removing this entity would split the connected evidence group");
                cJSON_AddItemToArray(linking_arr, item);

                cJSON *item_legacy = cJSON_CreateObject();
                cJSON_AddStringToObject(item_legacy, "id", n->id);
                cJSON_AddStringToObject(item_legacy, "label", n->label);
                cJSON_AddStringToObject(item_legacy, "type", n->type);
                cJSON_AddStringToObject(item_legacy, "kind", (n->kind == CT_NODE_EVENT) ? "event" : "entity");
                cJSON_AddItemToArray(ap_arr, item_legacy);
            }
        }
        cJSON_AddItemToObject(root, "linking", linking_arr);
        cJSON_AddItemToObject(root, "linking_entities", ap_arr);

        cJSON *bridge_arr = cJSON_CreateArray();
        for (size_t i = 0; i < tarjan_res.bridge_count; i++) {
            const ct_graph_node_t *u = ct_graph_get_node(&ds->graph, tarjan_res.bridges[i].u);
            const ct_graph_node_t *v = ct_graph_get_node(&ds->graph, tarjan_res.bridges[i].v);
            cJSON *item = cJSON_CreateObject();
            cJSON_AddStringToObject(item, "source_id", u ? u->id : "");
            cJSON_AddStringToObject(item, "target_id", v ? v->id : "");
            cJSON_AddStringToObject(item, "record_id", tarjan_res.bridges[i].record_id);
            cJSON_AddItemToArray(bridge_arr, item);
        }
        cJSON_AddItemToObject(root, "bridges", bridge_arr);

        ct_tarjan_result_free(&tarjan_res);
    } else {
        cJSON_AddItemToObject(root, "linking", cJSON_CreateArray());
        cJSON_AddItemToObject(root, "linking_entities", cJSON_CreateArray());
        cJSON_AddItemToObject(root, "bridges", cJSON_CreateArray());
    }

    return root;
}

cJSON* ct_api_handle_timeline(ct_dataset_t *ds, const char *query_string) {
    return ct_api_handle_get_events(ds, query_string);
}

cJSON* ct_api_handle_bursts(ct_dataset_t *ds, const char *query_string) {
    char win_str[16] = {0}, thresh_str[16] = {0}, sev_str[16] = {0};
    ct_get_query_param(query_string, "window", win_str, sizeof(win_str));
    ct_get_query_param(query_string, "threshold", thresh_str, sizeof(thresh_str));
    ct_get_query_param(query_string, "severity", sev_str, sizeof(sev_str));

    int64_t raw_win = (win_str[0] != '\0') ? atoll(win_str) : 5;
    /* If window <= 60, interpret as minutes (from frontend app.js), else seconds */
    int64_t window_sec = (raw_win <= 60) ? (raw_win * 60) : raw_win;
    uint32_t threshold = (thresh_str[0] != '\0') ? (uint32_t)atoi(thresh_str) : 3;
    int32_t sev_thresh = (sev_str[0] != '\0') ? atoi(sev_str) : 0;

    ct_burst_window_t *bursts = NULL;
    size_t burst_count = 0;

    ct_timeline_detect_bursts(&ds->timeline, window_sec, threshold, sev_thresh, &bursts, &burst_count);

    cJSON *root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "window_seconds", (double)window_sec);
    cJSON_AddNumberToObject(root, "threshold", (double)threshold);
    cJSON_AddNumberToObject(root, "count", (double)burst_count);

    cJSON *bursts_arr = cJSON_CreateArray();
    for (size_t i = 0; i < burst_count; i++) {
        cJSON *b_obj = cJSON_CreateObject();
        cJSON_AddStringToObject(b_obj, "start", bursts[i].start_str);
        cJSON_AddStringToObject(b_obj, "end", bursts[i].end_str);
        cJSON_AddNumberToObject(b_obj, "count", (double)bursts[i].event_count);
        cJSON_AddNumberToObject(b_obj, "max_severity", bursts[i].max_severity);

        cJSON_AddNumberToObject(b_obj, "start_time", (double)bursts[i].start_time);
        cJSON_AddNumberToObject(b_obj, "end_time", (double)bursts[i].end_time);
        cJSON_AddStringToObject(b_obj, "start_str", bursts[i].start_str);
        cJSON_AddStringToObject(b_obj, "end_str", bursts[i].end_str);
        cJSON_AddNumberToObject(b_obj, "event_count", bursts[i].event_count);

        cJSON *ev_ids = cJSON_CreateArray();
        for (uint32_t k = 0; k < bursts[i].event_count; k++) {
            uint32_t ev_idx = bursts[i].event_indices[k];
            if (ev_idx < ds->timeline.count) {
                cJSON_AddItemToArray(ev_ids, cJSON_CreateString(ds->timeline.events[ev_idx].event_id));
            }
        }
        cJSON_AddItemToObject(b_obj, "event_ids", ev_ids);
        cJSON_AddItemToArray(bursts_arr, b_obj);
    }
    cJSON_AddItemToObject(root, "bursts", bursts_arr);

    ct_burst_windows_free(bursts, burst_count);
    return root;
}

cJSON* ct_api_handle_integrity_verify(ct_dataset_t *ds) {
    cJSON *root = cJSON_CreateObject();

    time_t now = time(NULL);
    char time_buf[64] = {0};
    struct tm *tm_info = gmtime(&now);
    if (tm_info) {
        strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S UTC", tm_info);
    } else {
        snprintf(time_buf, sizeof(time_buf), "UTC");
    }
    cJSON_AddStringToObject(root, "verified_at", time_buf);

    if (!ds->manifest_initialized) {
        cJSON_AddStringToObject(root, "status", "error");
        cJSON_AddStringToObject(root, "error", "Integrity baseline manifest not initialized. Please import evidence files first.");
        cJSON_AddStringToObject(root, "message", "Integrity baseline manifest not initialized. Please import evidence files first.");
        cJSON_AddBoolToObject(root, "manifest_match", false);
        cJSON_AddItemToObject(root, "results", cJSON_CreateArray());
        return root;
    }

    const char *paths[3] = {
        ds->events_file_path[0] ? ds->events_file_path : "data/events.csv",
        ds->entities_file_path[0] ? ds->entities_file_path : "data/entities.csv",
        ds->relationships_file_path[0] ? ds->relationships_file_path : "data/relationships.csv"
    };

    ct_integrity_report_t rep;
    ct_status_t st = ct_integrity_verify(&ds->manifest, paths, 3, &rep);
    if (st != CT_OK) {
        cJSON_AddStringToObject(root, "status", "error");
        cJSON_AddStringToObject(root, "error", "Integrity check failed during hash computation");
        cJSON_AddStringToObject(root, "message", "Integrity check failed during hash computation");
        cJSON_AddBoolToObject(root, "manifest_match", false);
        cJSON_AddItemToObject(root, "results", cJSON_CreateArray());
        return root;
    }

    cJSON_AddBoolToObject(root, "manifest_match", rep.manifest_root_matches);
    cJSON_AddBoolToObject(root, "verified", rep.all_files_match);
    cJSON_AddBoolToObject(root, "manifest_root_match", rep.manifest_root_matches);
    cJSON_AddStringToObject(root, "baseline_manifest_root", rep.baseline_root);
    cJSON_AddStringToObject(root, "current_manifest_root", rep.current_root);

    cJSON *results_arr = cJSON_CreateArray();
    cJSON *files_arr = cJSON_CreateArray();
    for (size_t i = 0; i < rep.file_count; i++) {
        cJSON *res_item = cJSON_CreateObject();
        cJSON_AddStringToObject(res_item, "file", rep.files[i].filename);
        cJSON_AddStringToObject(res_item, "baseline", rep.files[i].baseline_hash);
        cJSON_AddStringToObject(res_item, "current", rep.files[i].current_hash);
        cJSON_AddBoolToObject(res_item, "match", rep.files[i].is_match);
        cJSON_AddItemToArray(results_arr, res_item);

        cJSON *f = cJSON_CreateObject();
        cJSON_AddStringToObject(f, "filename", rep.files[i].filename);
        cJSON_AddStringToObject(f, "baseline_hash", rep.files[i].baseline_hash);
        cJSON_AddStringToObject(f, "current_hash", rep.files[i].current_hash);
        cJSON_AddBoolToObject(f, "is_match", rep.files[i].is_match);
        cJSON_AddItemToArray(files_arr, f);
    }
    cJSON_AddItemToObject(root, "results", results_arr);
    cJSON_AddItemToObject(root, "files", files_arr);

    ct_integrity_report_free(&rep);
    return root;
}

cJSON* ct_api_handle_create_report(ct_dataset_t *ds, const char *body) {
    char report_id[64];
    snprintf(report_id, sizeof(report_id), "RPT-%ld", (long)time(NULL));

    char title_str[128] = "Investigation report";
    cJSON *ev_ids_input = NULL;

    ct_report_request_t req;
    memset(&req, 0, sizeof(req));
    snprintf(req.report_id, sizeof(req.report_id), "%s", report_id);

    if (body && body[0] == '{') {
        cJSON *json = cJSON_Parse(body);
        if (json) {
            cJSON *title = cJSON_GetObjectItem(json, "title");
            if (title && title->valuestring && title->valuestring[0]) {
                snprintf(title_str, sizeof(title_str), "%s", title->valuestring);
            }
            cJSON *p_from = cJSON_GetObjectItem(json, "path_from");
            if (p_from && p_from->valuestring) {
                snprintf(req.path_from, sizeof(req.path_from), "%s", p_from->valuestring);
            }
            cJSON *p_to = cJSON_GetObjectItem(json, "path_to");
            if (p_to && p_to->valuestring) {
                snprintf(req.path_to, sizeof(req.path_to), "%s", p_to->valuestring);
            }
            cJSON *ev_arr = cJSON_GetObjectItem(json, "event_ids");
            if (ev_arr && cJSON_IsArray(ev_arr)) {
                ev_ids_input = cJSON_Duplicate(ev_arr, 1);
            }
            cJSON_Delete(json);
        }
    }
    snprintf(req.title, sizeof(req.title), "%s", title_str);

    time_t now = time(NULL);
    char time_str[64];
    struct tm *tm_info = gmtime(&now);
    if (tm_info) {
        strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S UTC", tm_info);
    } else {
        snprintf(time_str, sizeof(time_str), "UTC");
    }

    /* Build structured report JSON */
    cJSON *rep_json = cJSON_CreateObject();
    cJSON_AddStringToObject(rep_json, "id", report_id);
    cJSON_AddStringToObject(rep_json, "report_id", report_id);
    cJSON_AddStringToObject(rep_json, "title", title_str);
    cJSON_AddStringToObject(rep_json, "generated_at", time_str);

    cJSON *sections_arr = cJSON_CreateArray();

    /* Section 1: Executive Summary */
    cJSON *sec1 = cJSON_CreateObject();
    cJSON_AddStringToObject(sec1, "heading", "Executive Summary");
    cJSON *sec1_items = cJSON_CreateArray();

    size_t sel_cnt = ev_ids_input ? (size_t)cJSON_GetArraySize(ev_ids_input) : ds->timeline.count;
    char sum_buf[256];
    snprintf(sum_buf, sizeof(sum_buf), "Investigation dataset analyzed %lu chronological event record(s).", (unsigned long)sel_cnt);

    cJSON *item1 = cJSON_CreateObject();
    cJSON_AddStringToObject(item1, "text", sum_buf);
    cJSON *all_ids = cJSON_CreateArray();

    size_t sev3_cnt = 0;
    cJSON *sev3_ids = cJSON_CreateArray();

    if (ev_ids_input && cJSON_GetArraySize(ev_ids_input) > 0) {
        int n = cJSON_GetArraySize(ev_ids_input);
        for (int i = 0; i < n; i++) {
            cJSON *it = cJSON_GetArrayItem(ev_ids_input, i);
            if (it && it->valuestring) {
                cJSON_AddItemToArray(all_ids, cJSON_CreateString(it->valuestring));
                void *val = NULL;
                if (ct_hashmap_get(&ds->events_map, it->valuestring, &val) == CT_OK && val) {
                    const ct_event_t *ev = (const ct_event_t*)val;
                    if (ev->severity >= 3) {
                        sev3_cnt++;
                        cJSON_AddItemToArray(sev3_ids, cJSON_CreateString(ev->event_id));
                    }
                }
            }
        }
    } else {
        for (size_t i = 0; i < ds->timeline.count; i++) {
            cJSON_AddItemToArray(all_ids, cJSON_CreateString(ds->timeline.events[i].event_id));
            if (ds->timeline.events[i].severity >= 3) {
                sev3_cnt++;
                cJSON_AddItemToArray(sev3_ids, cJSON_CreateString(ds->timeline.events[i].event_id));
            }
        }
    }

    cJSON_AddItemToObject(item1, "record_ids", all_ids);
    cJSON_AddItemToArray(sec1_items, item1);

    if (sev3_cnt > 0) {
        cJSON *item2 = cJSON_CreateObject();
        char alert_buf[256];
        snprintf(alert_buf, sizeof(alert_buf), "Identified %lu high-severity (Level 3) security incident(s) requiring immediate attention.", (unsigned long)sev3_cnt);
        cJSON_AddStringToObject(item2, "text", alert_buf);
        cJSON_AddItemToObject(item2, "record_ids", sev3_ids);
        cJSON_AddItemToArray(sec1_items, item2);
    } else {
        cJSON_Delete(sev3_ids);
    }
    cJSON_AddItemToObject(sec1, "items", sec1_items);
    cJSON_AddItemToArray(sections_arr, sec1);

    /* Section 2: Chronological Sequence */
    cJSON *sec2 = cJSON_CreateObject();
    cJSON_AddStringToObject(sec2, "heading", "Chronological Activity Sequence");
    cJSON *sec2_items = cJSON_CreateArray();

    if (ev_ids_input && cJSON_GetArraySize(ev_ids_input) > 0) {
        int n = cJSON_GetArraySize(ev_ids_input);
        for (int i = 0; i < n; i++) {
            cJSON *it = cJSON_GetArrayItem(ev_ids_input, i);
            if (it && it->valuestring) {
                void *val = NULL;
                if (ct_hashmap_get(&ds->events_map, it->valuestring, &val) == CT_OK && val) {
                    const ct_event_t *ev = (const ct_event_t*)val;
                    char ev_buf[256];
                    snprintf(ev_buf, sizeof(ev_buf), "%s - %s observed on entity %s (Severity Level %d)",
                             ev->timestamp_str, ev->event_type, ev->entity_id, ev->severity);
                    cJSON *ev_item = cJSON_CreateObject();
                    cJSON_AddStringToObject(ev_item, "text", ev_buf);
                    cJSON *rec_arr = cJSON_CreateArray();
                    cJSON_AddItemToArray(rec_arr, cJSON_CreateString(ev->event_id));
                    cJSON_AddItemToObject(ev_item, "record_ids", rec_arr);
                    cJSON_AddItemToArray(sec2_items, ev_item);
                }
            }
        }
    } else {
        for (size_t i = 0; i < ds->timeline.count; i++) {
            const ct_event_t *ev = &ds->timeline.events[i];
            char ev_buf[256];
            snprintf(ev_buf, sizeof(ev_buf), "%s - %s observed on entity %s (Severity Level %d)",
                     ev->timestamp_str, ev->event_type, ev->entity_id, ev->severity);
            cJSON *ev_item = cJSON_CreateObject();
            cJSON_AddStringToObject(ev_item, "text", ev_buf);
            cJSON *rec_arr = cJSON_CreateArray();
            cJSON_AddItemToArray(rec_arr, cJSON_CreateString(ev->event_id));
            cJSON_AddItemToObject(ev_item, "record_ids", rec_arr);
            cJSON_AddItemToArray(sec2_items, ev_item);
        }
    }
    cJSON_AddItemToObject(sec2, "items", sec2_items);
    cJSON_AddItemToArray(sections_arr, sec2);

    /* Section 3: Entity Corroboration & Topology */
    cJSON *sec3 = cJSON_CreateObject();
    cJSON_AddStringToObject(sec3, "heading", "Involved Entities & Graph Topology");
    cJSON *sec3_items = cJSON_CreateArray();

    size_t ent_count = ct_hashmap_size(&ds->entities_map);
    char ent_sum[256];
    snprintf(ent_sum, sizeof(ent_sum), "Mapped %lu entities across connected infrastructure clusters.", (unsigned long)ent_count);
    cJSON *ent_item_node = cJSON_CreateObject();
    cJSON_AddStringToObject(ent_item_node, "text", ent_sum);
    cJSON *ent_recs = cJSON_CreateArray();
    size_t edge_total = ct_graph_edge_count(&ds->graph);
    for (size_t i = 0; i < edge_total; i++) {
        const ct_graph_edge_t *edge = ct_graph_get_edge(&ds->graph, (int32_t)i);
        if (edge) cJSON_AddItemToArray(ent_recs, cJSON_CreateString(edge->record_id));
        if (i >= 20) break;
    }
    cJSON_AddItemToObject(ent_item_node, "record_ids", ent_recs);
    cJSON_AddItemToArray(sec3_items, ent_item_node);
    cJSON_AddItemToObject(sec3, "items", sec3_items);
    cJSON_AddItemToArray(sections_arr, sec3);

    cJSON_AddItemToObject(rep_json, "sections", sections_arr);

    /* Save report JSON to reports/<id>.json */
    char json_filepath[512];
    snprintf(json_filepath, sizeof(json_filepath), "reports/%s.json", report_id);
    char *rep_str = cJSON_Print(rep_json);
    if (rep_str) {
        FILE *jf = fopen(json_filepath, "w");
        if (jf) {
            fputs(rep_str, jf);
            fclose(jf);
        }
        free(rep_str);
    }

    /* Also generate HTML version */
    char *html_out = NULL;
    size_t html_len = 0;
    ct_report_generate_html(ds, &req, &html_out, &html_len);
    char report_filepath[512];
    snprintf(report_filepath, sizeof(report_filepath), "reports/%s.html", report_id);
    FILE *rf = fopen(report_filepath, "w");
    if (rf) {
        if (html_out) fwrite(html_out, 1, html_len, rf);
        fclose(rf);
    }
    if (html_out) free(html_out);

    if (ev_ids_input) cJSON_Delete(ev_ids_input);
    cJSON_Delete(rep_json);

    /* Response to POST /reports: { id, report_id, status } */
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "id", report_id);
    cJSON_AddStringToObject(root, "report_id", report_id);
    cJSON_AddStringToObject(root, "status", "success");
    cJSON_AddStringToObject(root, "report_url", report_filepath);
    return root;
}

cJSON* ct_api_handle_get_report_by_id(ct_dataset_t *ds, const char *report_id) {
    (void)ds;
    if (!report_id || report_id[0] == '\0') {
        cJSON *err = cJSON_CreateObject();
        cJSON_AddStringToObject(err, "status", "error");
        cJSON_AddStringToObject(err, "error", "Missing report id");
        cJSON_AddStringToObject(err, "message", "Missing report id");
        return err;
    }

    char json_filepath[512];
    snprintf(json_filepath, sizeof(json_filepath), "reports/%s.json", report_id);

    FILE *f = fopen(json_filepath, "rb");
    if (!f) {
        cJSON *err = cJSON_CreateObject();
        cJSON_AddStringToObject(err, "status", "error");
        cJSON_AddStringToObject(err, "error", "Report not found");
        cJSON_AddStringToObject(err, "message", "Report not found");
        return err;
    }

    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);

    char *buf = (char*)malloc(sz + 1);
    if (!buf) {
        fclose(f);
        cJSON *err = cJSON_CreateObject();
        cJSON_AddStringToObject(err, "status", "error");
        cJSON_AddStringToObject(err, "error", "Out of memory reading report");
        return err;
    }

    fread(buf, 1, sz, f);
    buf[sz] = '\0';
    fclose(f);

    cJSON *parsed = cJSON_Parse(buf);
    free(buf);

    if (!parsed) {
        cJSON *err = cJSON_CreateObject();
        cJSON_AddStringToObject(err, "status", "error");
        cJSON_AddStringToObject(err, "error", "Malformed report data");
        return err;
    }

    return parsed;
}
