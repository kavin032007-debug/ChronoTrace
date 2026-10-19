#include "sort.h"
#include <stdlib.h>
#include <string.h>

int ct_event_comparator(const ct_event_t *a, const ct_event_t *b) {
    if (a->timestamp_epoch < b->timestamp_epoch) return -1;
    if (a->timestamp_epoch > b->timestamp_epoch) return 1;
    return strcmp(a->event_id, b->event_id);
}

static void ct_merge(ct_event_t *events, size_t left, size_t mid, size_t right, ct_event_t *scratch) {
    size_t i = left;
    size_t j = mid;
    size_t k = left;

    while (i < mid && j < right) {
        if (ct_event_comparator(&events[i], &events[j]) <= 0) {
            scratch[k++] = events[i++];
        } else {
            scratch[k++] = events[j++];
        }
    }

    while (i < mid) {
        scratch[k++] = events[i++];
    }
    while (j < right) {
        scratch[k++] = events[j++];
    }

    for (i = left; i < right; i++) {
        events[i] = scratch[i];
    }
}

static void ct_merge_sort_recursive(ct_event_t *events, size_t left, size_t right, ct_event_t *scratch) {
    if (right - left <= 1) return;

    size_t mid = left + (right - left) / 2;
    ct_merge_sort_recursive(events, left, mid, scratch);
    ct_merge_sort_recursive(events, mid, right, scratch);
    ct_merge(events, left, mid, right, scratch);
}

ct_status_t ct_stable_merge_sort_events(ct_event_t *events, size_t count) {
    if (!events && count > 0) return CT_ERR_INVARG;
    if (count <= 1) return CT_OK;

    ct_event_t *scratch = (ct_event_t*)malloc(count * sizeof(ct_event_t));
    if (!scratch) return CT_ERR_NOMEM;

    ct_merge_sort_recursive(events, 0, count, scratch);
    free(scratch);
    return CT_OK;
}
