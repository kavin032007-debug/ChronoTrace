#ifndef CHRONOTRACE_TIMELINE_H
#define CHRONOTRACE_TIMELINE_H

#include "../../include/models.h"
#include "../core/deque.h"

typedef struct {
    ct_event_t *events;
    size_t count;
    size_t capacity;
    bool is_sorted;
} ct_timeline_t;

ct_status_t ct_timeline_init(ct_timeline_t *tl, size_t initial_capacity);
ct_status_t ct_timeline_add(ct_timeline_t *tl, const ct_event_t *ev);
ct_status_t ct_timeline_sort(ct_timeline_t *tl);

/* Filter options */
typedef struct {
    int64_t start_epoch;
    int64_t end_epoch;
    const char *event_type;
    const char *entity_id;
    int32_t min_severity;
    size_t limit;
    size_t offset;
} ct_timeline_filter_t;

/* Returns a vector/array of matching events. User frees out_results. */
ct_status_t ct_timeline_filter(const ct_timeline_t *tl, const ct_timeline_filter_t *filter,
                               ct_event_t **out_results, size_t *out_count, size_t *out_total_matched);

/* Sliding window burst detection using monotonic deque */
ct_status_t ct_timeline_detect_bursts(const ct_timeline_t *tl,
                                      int64_t window_seconds,
                                      uint32_t count_threshold,
                                      int32_t severity_threshold,
                                      ct_burst_window_t **out_bursts,
                                      size_t *out_burst_count);

void ct_burst_windows_free(ct_burst_window_t *bursts, size_t count);
void ct_timeline_free(ct_timeline_t *tl);

#endif /* CHRONOTRACE_TIMELINE_H */
