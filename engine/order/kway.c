#include "kway.h"
#include "minheap.h"
#include "sort.h"
#include <stdlib.h>
#include <string.h>

typedef struct {
    size_t stream_idx;
    size_t event_idx;
    const ct_event_t *event;
} ct_kway_cursor_t;

static int kway_heap_cmp(const ct_heap_node_t *a, const ct_heap_node_t *b) {
    const ct_kway_cursor_t *ca = (const ct_kway_cursor_t*)a->user_data;
    const ct_kway_cursor_t *cb = (const ct_kway_cursor_t*)b->user_data;
    if (!ca || !cb) return 0;
    return ct_event_comparator(ca->event, cb->event);
}

ct_status_t ct_kway_merge_events(const ct_event_stream_t *streams, size_t num_streams,
                                 ct_event_t *out_merged, size_t out_capacity,
                                 size_t *out_count) {
    if (!out_merged || !out_count) return CT_ERR_INVARG;
    if (num_streams == 0) {
        *out_count = 0;
        return CT_OK;
    }

    /* Compute total count and check capacity */
    size_t total = 0;
    for (size_t i = 0; i < num_streams; i++) {
        total += streams[i].count;
    }
    if (out_capacity < total) return CT_ERR_CAPACITY;

    ct_kway_cursor_t *cursors = (ct_kway_cursor_t*)malloc(num_streams * sizeof(ct_kway_cursor_t));
    if (!cursors) return CT_ERR_NOMEM;

    ct_minheap_t heap;
    ct_status_t st = ct_minheap_init(&heap, num_streams + 1, 0, kway_heap_cmp);
    if (st != CT_OK) {
        free(cursors);
        return st;
    }

    for (size_t i = 0; i < num_streams; i++) {
        if (streams[i].count > 0 && streams[i].events) {
            cursors[i].stream_idx = i;
            cursors[i].event_idx = 0;
            cursors[i].event = &streams[i].events[0];
            ct_minheap_insert(&heap, (int32_t)i, (double)cursors[i].event->timestamp_epoch, &cursors[i]);
        }
    }

    size_t merged_idx = 0;
    while (!ct_minheap_is_empty(&heap)) {
        ct_heap_node_t min_node;
        ct_minheap_extract_min(&heap, &min_node);

        ct_kway_cursor_t *cur = (ct_kway_cursor_t*)min_node.user_data;
        out_merged[merged_idx++] = *(cur->event);

        cur->event_idx++;
        size_t s_idx = cur->stream_idx;
        if (cur->event_idx < streams[s_idx].count) {
            cur->event = &streams[s_idx].events[cur->event_idx];
            ct_minheap_insert(&heap, (int32_t)s_idx, (double)cur->event->timestamp_epoch, cur);
        }
    }

    *out_count = merged_idx;
    ct_minheap_free(&heap);
    free(cursors);
    return CT_OK;
}
