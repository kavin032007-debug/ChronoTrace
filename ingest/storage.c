#include "storage.h"
#include "csv_parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

static bool ct_str_equals_ci(const char *a, const char *b) {
    if (!a || !b) return false;
    while (*a && *b) {
        if (tolower((unsigned char)*a) != tolower((unsigned char)*b)) return false;
        a++;
        b++;
    }
    return *a == '\0' && *b == '\0';
}

ct_status_t ct_dataset_init(ct_dataset_t *ds, const char *data_dir) {
    if (!ds) return CT_ERR_INVARG;
    memset(ds, 0, sizeof(ct_dataset_t));

    ct_status_t st = ct_arena_init(&ds->arena, 128 * 1024);
    if (st != CT_OK) return st;

    st = ct_hashmap_init(&ds->entities_map, 256);
    if (st != CT_OK) return st;

    st = ct_hashmap_init(&ds->events_map, 1024);
    if (st != CT_OK) return st;

    st = ct_vector_init(&ds->entities_list, sizeof(ct_entity_t), 128);
    if (st != CT_OK) return st;

    st = ct_vector_init(&ds->relationships, sizeof(ct_relationship_t), 256);
    if (st != CT_OK) return st;

    st = ct_vector_init(&ds->rejections, sizeof(ct_rejection_t), 64);
    if (st != CT_OK) return st;

    st = ct_timeline_init(&ds->timeline, 1024);
    if (st != CT_OK) return st;

    st = ct_graph_init(&ds->graph, 512, 1024);
    if (st != CT_OK) return st;

    st = ct_trie_init(&ds->search_trie);
    if (st != CT_OK) return st;

    st = ct_lru_init(&ds->query_cache, 128);
    if (st != CT_OK) return st;

    ds->version = 1;
    ds->dsu_initialized = false;
    ds->manifest_initialized = false;

    if (data_dir) {
        strncpy(ds->data_dir, data_dir, sizeof(ds->data_dir) - 1);
    } else {
        strcpy(ds->data_dir, "data");
    }

    return CT_OK;
}

static void ct_record_rejection(ct_dataset_t *ds, const char *file, uint32_t line_no,
                                const char *raw_line, const char *reason) {
    ct_rejection_t rej;
    memset(&rej, 0, sizeof(rej));
    strncpy(rej.source_file, file ? file : "unknown", CT_MAX_NAME_LEN - 1);
    rej.line_number = line_no;
    strncpy(rej.raw_line, raw_line ? raw_line : "", sizeof(rej.raw_line) - 1);
    strncpy(rej.reason, reason ? reason : "Unknown validation error", CT_MAX_REASON_LEN - 1);
    ct_vector_push(&ds->rejections, &rej);
    ds->summary.rejected_rows++;
}

static ct_status_t ct_parse_entities_file(ct_dataset_t *ds, FILE *f, const char *filename) {
    char line_buf[1024];
    uint32_t line_no = 0;
    bool is_header = true;

    while (fgets(line_buf, sizeof(line_buf), f)) {
        line_no++;
        ds->summary.total_rows_read++;

        char line_copy[1024];
        strncpy(line_copy, line_buf, sizeof(line_copy) - 1);
        line_copy[sizeof(line_copy) - 1] = '\0';
        ct_str_trim(line_copy);

        if (line_copy[0] == '\0') continue;

        ct_csv_row_t row;
        if (ct_csv_parse_line(line_buf, &row) != CT_OK || row.col_count < 3) {
            ct_record_rejection(ds, filename, line_no, line_copy, "Malformed CSV row or missing columns (expected entity_id, entity_type, name)");
            continue;
        }

        if (is_header) {
            is_header = false;
            if (ct_str_equals_ci(row.fields[0], "entity_id")) {
                continue;
            }
        }

        const char *id = row.fields[0];
        const char *type = row.fields[1];
        const char *name = row.fields[2];

        if (!ct_validate_id_format(id)) {
            ct_record_rejection(ds, filename, line_no, line_copy, "Invalid entity_id format");
            continue;
        }

        if (ct_hashmap_contains(&ds->entities_map, id)) {
            ct_record_rejection(ds, filename, line_no, line_copy, "Duplicate entity_id detected");
            continue;
        }

        ct_entity_t *ent = (ct_entity_t*)ct_arena_alloc(&ds->arena, sizeof(ct_entity_t));
        if (!ent) return CT_ERR_NOMEM;

        strncpy(ent->entity_id, id, CT_MAX_ID_LEN - 1);
        strncpy(ent->entity_type, type, CT_MAX_TYPE_LEN - 1);
        strncpy(ent->name, name, CT_MAX_NAME_LEN - 1);

        int32_t node_idx = -1;
        ct_graph_add_node(&ds->graph, id, name, type, CT_NODE_ENTITY, 0, 0, &node_idx);
        ent->node_index = node_idx;

        ct_hashmap_put(&ds->entities_map, ent->entity_id, ent, false);
        ct_vector_push(&ds->entities_list, ent);

        ct_trie_insert(&ds->search_trie, ent->entity_id, ent->entity_id, ent->name, ent->entity_type, node_idx);
        if (ent->name[0] != '\0') {
            ct_trie_insert(&ds->search_trie, ent->name, ent->entity_id, ent->name, ent->entity_type, node_idx);
        }

        ds->summary.valid_entities++;
    }
    return CT_OK;
}

static ct_status_t ct_parse_events_file(ct_dataset_t *ds, FILE *f, const char *filename) {
    char line_buf[1024];
    uint32_t line_no = 0;
    bool is_header = true;

    while (fgets(line_buf, sizeof(line_buf), f)) {
        line_no++;
        ds->summary.total_rows_read++;

        char line_copy[1024];
        strncpy(line_copy, line_buf, sizeof(line_copy) - 1);
        line_copy[sizeof(line_copy) - 1] = '\0';
        ct_str_trim(line_copy);

        if (line_copy[0] == '\0') continue;

        ct_csv_row_t row;
        if (ct_csv_parse_line(line_buf, &row) != CT_OK || row.col_count < 5) {
            ct_record_rejection(ds, filename, line_no, line_copy, "Malformed CSV row or missing columns (expected event_id, timestamp, entity_id, event_type, severity)");
            continue;
        }

        if (is_header) {
            is_header = false;
            if (ct_str_equals_ci(row.fields[0], "event_id")) {
                continue;
            }
        }

        const char *ev_id = row.fields[0];
        const char *time_str = row.fields[1];
        const char *ent_id = row.fields[2];
        const char *ev_type = row.fields[3];
        const char *sev_str = row.fields[4];

        if (!ct_validate_id_format(ev_id)) {
            ct_record_rejection(ds, filename, line_no, line_copy, "Invalid event_id format");
            continue;
        }

        int64_t epoch = 0;
        if (ct_parse_timestamp_utc(time_str, &epoch) != CT_OK) {
            ct_record_rejection(ds, filename, line_no, line_copy, "Invalid timestamp format (expected YYYY-MM-DD HH:MM:SS in UTC)");
            continue;
        }

        int severity = atoi(sev_str);
        if (severity < 0) {
            ct_record_rejection(ds, filename, line_no, line_copy, "Invalid severity (must be non-negative integer)");
            continue;
        }

        if (ct_hashmap_contains(&ds->events_map, ev_id)) {
            ct_record_rejection(ds, filename, line_no, line_copy, "Duplicate event_id detected");
            continue;
        }

        ct_event_t *ev = (ct_event_t*)ct_arena_alloc(&ds->arena, sizeof(ct_event_t));
        if (!ev) return CT_ERR_NOMEM;

        strncpy(ev->event_id, ev_id, CT_MAX_ID_LEN - 1);
        ev->timestamp_epoch = epoch;
        strncpy(ev->timestamp_str, time_str, CT_MAX_TIME_STR_LEN - 1);
        strncpy(ev->entity_id, ent_id, CT_MAX_ID_LEN - 1);
        strncpy(ev->event_type, ev_type, CT_MAX_TYPE_LEN - 1);
        ev->severity = severity;

        /* Add event node to graph */
        int32_t ev_node_idx = -1;
        ct_graph_add_node(&ds->graph, ev_id, ev_type, ev_type, CT_NODE_EVENT, epoch, severity, &ev_node_idx);
        ev->node_index = ev_node_idx;

        /* Connect event to associated entity if entity exists */
        int32_t ent_node_idx = -1;
        if (ct_graph_find_node(&ds->graph, ent_id, &ent_node_idx) == CT_OK) {
            ct_graph_add_edge(&ds->graph, ent_node_idx, ev_node_idx, "associated_with", ev_id, 1.0, false, false);
        } else {
            /* Associated entity was not declared in entities.csv */
            ct_record_rejection(ds, filename, line_no, line_copy, "Event references unknown entity_id");
            continue;
        }

        ct_hashmap_put(&ds->events_map, ev->event_id, ev, false);
        ct_timeline_add(&ds->timeline, ev);

        ct_trie_insert(&ds->search_trie, ev->event_id, ev->event_id, ev->event_type, "event", ev_node_idx);
        if (ev->event_type[0] != '\0') {
            ct_trie_insert(&ds->search_trie, ev->event_type, ev->event_id, ev->event_type, "event", ev_node_idx);
        }

        ds->summary.valid_events++;
    }
    return CT_OK;
}

static ct_status_t ct_parse_relationships_file(ct_dataset_t *ds, FILE *f, const char *filename) {
    char line_buf[1024];
    uint32_t line_no = 0;
    bool is_header = true;

    while (fgets(line_buf, sizeof(line_buf), f)) {
        line_no++;
        ds->summary.total_rows_read++;

        char line_copy[1024];
        strncpy(line_copy, line_buf, sizeof(line_copy) - 1);
        line_copy[sizeof(line_copy) - 1] = '\0';
        ct_str_trim(line_copy);

        if (line_copy[0] == '\0') continue;

        ct_csv_row_t row;
        if (ct_csv_parse_line(line_buf, &row) != CT_OK || row.col_count < 4) {
            ct_record_rejection(ds, filename, line_no, line_copy, "Malformed CSV row or missing columns (expected relationship_id, source_id, target_id, relationship)");
            continue;
        }

        if (is_header) {
            is_header = false;
            if (ct_str_equals_ci(row.fields[0], "relationship_id")) {
                continue;
            }
        }

        const char *rel_id = row.fields[0];
        const char *src_id = row.fields[1];
        const char *tgt_id = row.fields[2];
        const char *rel_label = row.fields[3];

        if (!ct_validate_id_format(rel_id)) {
            ct_record_rejection(ds, filename, line_no, line_copy, "Invalid relationship_id format");
            continue;
        }

        int32_t src_idx = -1;
        int32_t tgt_idx = -1;
        if (ct_graph_find_node(&ds->graph, src_id, &src_idx) != CT_OK) {
            ct_record_rejection(ds, filename, line_no, line_copy, "Relationship references nonexistent source_id");
            continue;
        }
        if (ct_graph_find_node(&ds->graph, tgt_id, &tgt_idx) != CT_OK) {
            ct_record_rejection(ds, filename, line_no, line_copy, "Relationship references nonexistent target_id");
            continue;
        }

        /* Compute weight: if both endpoints are events with timestamps, time gap in seconds; else 1.0 */
        const ct_graph_node_t *s_node = ct_graph_get_node(&ds->graph, src_idx);
        const ct_graph_node_t *t_node = ct_graph_get_node(&ds->graph, tgt_idx);
        double weight = 1.0;
        if (s_node && t_node && s_node->kind == CT_NODE_EVENT && t_node->kind == CT_NODE_EVENT) {
            int64_t diff = s_node->timestamp - t_node->timestamp;
            if (diff < 0) diff = -diff;
            weight = (double)diff;
        }

        ct_relationship_t rel;
        memset(&rel, 0, sizeof(rel));
        strncpy(rel.relationship_id, rel_id, CT_MAX_ID_LEN - 1);
        strncpy(rel.source_id, src_id, CT_MAX_ID_LEN - 1);
        strncpy(rel.target_id, tgt_id, CT_MAX_ID_LEN - 1);
        strncpy(rel.relationship, rel_label, CT_MAX_RELATION_LEN - 1);
        rel.weight = weight;
        rel.is_inferred = false;

        ct_graph_add_edge(&ds->graph, src_idx, tgt_idx, rel_label, rel_id, weight, false, false);
        ct_vector_push(&ds->relationships, &rel);

        ds->summary.valid_relationships++;
    }
    return CT_OK;
}

ct_status_t ct_dataset_import_csvs(ct_dataset_t *ds,
                                   const char *events_csv_path,
                                   const char *entities_csv_path,
                                   const char *relationships_csv_path) {
    if (!ds || !events_csv_path || !entities_csv_path || !relationships_csv_path) {
        return CT_ERR_INVARG;
    }

    strncpy(ds->events_file_path, events_csv_path, sizeof(ds->events_file_path) - 1);
    ds->events_file_path[sizeof(ds->events_file_path) - 1] = '\0';
    strncpy(ds->entities_file_path, entities_csv_path, sizeof(ds->entities_file_path) - 1);
    ds->entities_file_path[sizeof(ds->entities_file_path) - 1] = '\0';
    strncpy(ds->relationships_file_path, relationships_csv_path, sizeof(ds->relationships_file_path) - 1);
    ds->relationships_file_path[sizeof(ds->relationships_file_path) - 1] = '\0';

    ct_dataset_reset(ds);
    ds->version++;
    ct_lru_set_version(&ds->query_cache, ds->version);

    FILE *f_ent = fopen(entities_csv_path, "r");
    if (!f_ent) return CT_ERR_IO;
    ct_status_t st = ct_parse_entities_file(ds, f_ent, "entities.csv");
    fclose(f_ent);
    if (st != CT_OK) return st;

    FILE *f_ev = fopen(events_csv_path, "r");
    if (!f_ev) return CT_ERR_IO;
    st = ct_parse_events_file(ds, f_ev, "events.csv");
    fclose(f_ev);
    if (st != CT_OK) return st;

    FILE *f_rel = fopen(relationships_csv_path, "r");
    if (!f_rel) return CT_ERR_IO;
    st = ct_parse_relationships_file(ds, f_rel, "relationships.csv");
    fclose(f_rel);
    if (st != CT_OK) return st;

    /* Sort timeline */
    ct_timeline_sort(&ds->timeline);

    /* Build DSU over graph */
    size_t num_nodes = ct_graph_node_count(&ds->graph);
    if (ds->dsu_initialized) {
        ct_dsu_free(&ds->dsu);
    }
    st = ct_dsu_init(&ds->dsu, num_nodes);
    if (st == CT_OK) {
        ds->dsu_initialized = true;
        size_t num_edges = ct_graph_edge_count(&ds->graph);
        for (size_t i = 0; i < num_edges; i++) {
            const ct_graph_edge_t *edge = ct_graph_get_edge(&ds->graph, (int32_t)i);
            if (edge) {
                /* Find source from node's edge list */
                for (size_t u = 0; u < num_nodes; u++) {
                    const ct_graph_node_t *nu = ct_graph_get_node(&ds->graph, (int32_t)u);
                    int32_t e = nu ? nu->first_edge : -1;
                    while (e >= 0) {
                        if (e == (int32_t)i) {
                            ct_dsu_union(&ds->dsu, (int32_t)u, edge->target_node);
                            break;
                        }
                        const ct_graph_edge_t *next_e = ct_graph_get_edge(&ds->graph, e);
                        e = next_e ? next_e->next_edge : -1;
                    }
                }
            }
        }
    }

    /* Initialize baseline integrity manifest */
    const char *manifest_names[3] = { "events.csv", "entities.csv", "relationships.csv" };
    const char *manifest_paths[3] = { events_csv_path, entities_csv_path, relationships_csv_path };

    if (ds->manifest_initialized) {
        ct_integrity_manifest_free(&ds->manifest);
    }
    st = ct_integrity_init_manifest(&ds->manifest, manifest_names, manifest_paths, 3);
    if (st == CT_OK) {
        ds->manifest_initialized = true;
    }

    return CT_OK;
}

void ct_dataset_reset(ct_dataset_t *ds) {
    if (!ds) return;
    ct_arena_reset(&ds->arena);
    ct_hashmap_clear(&ds->entities_map, NULL);
    ct_hashmap_clear(&ds->events_map, NULL);
    ct_vector_clear(&ds->entities_list);
    ct_vector_clear(&ds->relationships);
    ct_vector_clear(&ds->rejections);
    ct_timeline_free(&ds->timeline);
    ct_timeline_init(&ds->timeline, 1024);
    ct_graph_clear(&ds->graph);
    ct_trie_clear(&ds->search_trie);
    if (ds->dsu_initialized) {
        ct_dsu_free(&ds->dsu);
        ds->dsu_initialized = false;
    }
    if (ds->manifest_initialized) {
        ct_integrity_manifest_free(&ds->manifest);
        ds->manifest_initialized = false;
    }
    ct_lru_invalidate(&ds->query_cache);
    memset(&ds->summary, 0, sizeof(ct_import_summary_t));
}

void ct_dataset_free(ct_dataset_t *ds) {
    if (!ds) return;
    ct_dataset_reset(ds);
    ct_hashmap_free(&ds->entities_map, NULL);
    ct_hashmap_free(&ds->events_map, NULL);
    ct_vector_free(&ds->entities_list);
    ct_vector_free(&ds->relationships);
    ct_vector_free(&ds->rejections);
    ct_timeline_free(&ds->timeline);
    ct_graph_free(&ds->graph);
    ct_trie_free(&ds->search_trie);
    ct_lru_free(&ds->query_cache);
    ct_arena_free(&ds->arena);
}
