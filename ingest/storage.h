#ifndef CHRONOTRACE_STORAGE_H
#define CHRONOTRACE_STORAGE_H

#include "../include/models.h"
#include "../engine/core/arena.h"
#include "../engine/core/hashmap.h"
#include "../engine/core/vector.h"
#include "../engine/core/lru.h"
#include "../engine/graph/graph.h"
#include "../engine/graph/dsu.h"
#include "../engine/order/timeline.h"
#include "../engine/text/trie.h"
#include "../engine/integrity/integrity.h"
#include "validation.h"

typedef struct {
    ct_arena_t arena;
    ct_hashmap_t entities_map;   /* entity_id -> ct_entity_t* */
    ct_hashmap_t events_map;     /* event_id -> ct_event_t* */
    ct_vector_t entities_list;   /* array of ct_entity_t */
    ct_vector_t relationships;   /* array of ct_relationship_t */
    ct_vector_t rejections;      /* array of ct_rejection_t */
    ct_timeline_t timeline;      /* sorted events array */
    ct_graph_t graph;            /* entity + event relationship graph */
    ct_dsu_t dsu;                /* connected components DSU */
    bool dsu_initialized;
    ct_trie_t search_trie;       /* prefix search trie */
    ct_integrity_manifest_t manifest;
    bool manifest_initialized;
    ct_lru_t query_cache;        /* LRU query cache */
    uint64_t version;
    ct_import_summary_t summary;
    char data_dir[512];
    char events_file_path[512];
    char entities_file_path[512];
    char relationships_file_path[512];
} ct_dataset_t;

ct_status_t ct_dataset_init(ct_dataset_t *ds, const char *data_dir);
ct_status_t ct_dataset_import_csvs(ct_dataset_t *ds,
                                   const char *events_csv_path,
                                   const char *entities_csv_path,
                                   const char *relationships_csv_path);

/* Direct memory-based import for uploaded buffers or files */
ct_status_t ct_dataset_import_buffer(ct_dataset_t *ds,
                                     const char *events_csv_buf,
                                     const char *entities_csv_buf,
                                     const char *relationships_csv_buf);

void ct_dataset_reset(ct_dataset_t *ds);
void ct_dataset_free(ct_dataset_t *ds);

#endif /* CHRONOTRACE_STORAGE_H */
