#ifndef CHRONOTRACE_KWAY_H
#define CHRONOTRACE_KWAY_H

#include "../../include/models.h"

typedef struct {
    const ct_event_t *events;
    size_t count;
} ct_event_stream_t;

ct_status_t ct_kway_merge_events(const ct_event_stream_t *streams, size_t num_streams,
                                 ct_event_t *out_merged, size_t out_capacity,
                                 size_t *out_count);

#endif /* CHRONOTRACE_KWAY_H */
