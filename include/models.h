#ifndef CHRONOTRACE_MODELS_H
#define CHRONOTRACE_MODELS_H

#include "common.h"

/* Entity model: entity_id, entity_type, name */
typedef struct {
    char entity_id[CT_MAX_ID_LEN];
    char entity_type[CT_MAX_TYPE_LEN];
    char name[CT_MAX_NAME_LEN];
    int32_t node_index; /* Index in unified graph node pool */
} ct_entity_t;

/* Event model: event_id, timestamp, entity_id, event_type, severity */
typedef struct {
    char event_id[CT_MAX_ID_LEN];
    int64_t timestamp_epoch;
    char timestamp_str[CT_MAX_TIME_STR_LEN];
    char entity_id[CT_MAX_ID_LEN];
    char event_type[CT_MAX_TYPE_LEN];
    int32_t severity;
    int32_t node_index; /* Index in unified graph node pool */
} ct_event_t;

/* Relationship model: relationship_id, source_id, target_id, relationship */
typedef struct {
    char relationship_id[CT_MAX_ID_LEN];
    char source_id[CT_MAX_ID_LEN];
    char target_id[CT_MAX_ID_LEN];
    char relationship[CT_MAX_RELATION_LEN];
    double weight;       /* Time gap in seconds or 1.0 */
    bool is_inferred;    /* false for observed in relationships.csv, true if inferred */
} ct_relationship_t;

/* Rejected row record during import */
typedef struct {
    char source_file[CT_MAX_NAME_LEN];
    uint32_t line_number;
    char raw_line[512];
    char reason[CT_MAX_REASON_LEN];
} ct_rejection_t;

/* Activity burst detection window */
typedef struct {
    int64_t start_time;
    int64_t end_time;
    char start_str[CT_MAX_TIME_STR_LEN];
    char end_str[CT_MAX_TIME_STR_LEN];
    uint32_t event_count;
    int32_t max_severity;
    uint32_t *event_indices; /* Dynamic array of event indices in timeline */
} ct_burst_window_t;

/* Graph node kind */
typedef enum {
    CT_NODE_ENTITY = 0,
    CT_NODE_EVENT = 1
} ct_node_kind_t;

/* Graph node representation */
typedef struct {
    char id[CT_MAX_ID_LEN];
    char label[CT_MAX_NAME_LEN];
    char type[CT_MAX_TYPE_LEN];
    ct_node_kind_t kind;
    int64_t timestamp; /* Non-zero for events */
    int32_t severity;  /* Applicable for events */
    int32_t first_edge; /* Head index in edge pool (-1 if none) */
    uint32_t degree;
} ct_graph_node_t;

/* Graph edge representation */
typedef struct {
    int32_t target_node;   /* Target node index */
    int32_t next_edge;     /* Next edge index in node's linked list */
    char label[CT_MAX_RELATION_LEN];
    char record_id[CT_MAX_ID_LEN];
    double weight;
    bool is_inferred;
    bool is_directed;
} ct_graph_edge_t;

/* Path search result */
typedef struct {
    int32_t *node_path;     /* Array of node indices */
    char (*record_ids)[CT_MAX_ID_LEN]; /* Supporting edge record IDs */
    uint32_t hop_count;
    double total_cost;
} ct_path_result_t;

#endif /* CHRONOTRACE_MODELS_H */
