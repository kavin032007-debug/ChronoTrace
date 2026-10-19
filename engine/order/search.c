#include "search.h"

int64_t ct_search_lower_bound(const ct_event_t *events, size_t count, int64_t target_epoch) {
    if (!events || count == 0) return 0;

    int64_t low = 0;
    int64_t high = (int64_t)count;

    while (low < high) {
        int64_t mid = low + (high - low) / 2;
        if (events[mid].timestamp_epoch < target_epoch) {
            low = mid + 1;
        } else {
            high = mid;
        }
    }
    return low;
}

int64_t ct_search_upper_bound(const ct_event_t *events, size_t count, int64_t target_epoch) {
    if (!events || count == 0) return 0;

    int64_t low = 0;
    int64_t high = (int64_t)count;

    while (low < high) {
        int64_t mid = low + (high - low) / 2;
        if (events[mid].timestamp_epoch <= target_epoch) {
            low = mid + 1;
        } else {
            high = mid;
        }
    }
    return low;
}

ct_status_t ct_search_time_range(const ct_event_t *events, size_t count,
                                 int64_t start_epoch, int64_t end_epoch,
                                 size_t *out_start_idx, size_t *out_count) {
    if (!events && count > 0) return CT_ERR_INVARG;
    if (!out_start_idx || !out_count) return CT_ERR_INVARG;

    if (count == 0 || (start_epoch > end_epoch && end_epoch > 0)) {
        *out_start_idx = 0;
        *out_count = 0;
        return CT_OK;
    }

    int64_t start_idx = (start_epoch <= 0) ? 0 : ct_search_lower_bound(events, count, start_epoch);
    int64_t end_idx = (end_epoch <= 0) ? (int64_t)count : ct_search_upper_bound(events, count, end_epoch);

    if (start_idx >= (int64_t)count || start_idx >= end_idx) {
        *out_start_idx = (size_t)start_idx;
        *out_count = 0;
        return CT_OK;
    }

    *out_start_idx = (size_t)start_idx;
    *out_count = (size_t)(end_idx - start_idx);
    return CT_OK;
}
