#include "timeline.h"
#include "sort.h"
#include "search.h"
#include <stdlib.h>
#include <string.h>

ct_status_t ct_timeline_init(ct_timeline_t *tl, size_t initial_capacity) {
    if (!tl) return CT_ERR_INVARG;
    if (initial_capacity == 0) initial_capacity = 64;

    tl->events = (ct_event_t*)malloc(initial_capacity * sizeof(ct_event_t));
    if (!tl->events) return CT_ERR_NOMEM;

    tl->count = 0;
    tl->capacity = initial_capacity;
    tl->is_sorted = true;
    return CT_OK;
}

ct_status_t ct_timeline_add(ct_timeline_t *tl, const ct_event_t *ev) {
    if (!tl || !ev) return CT_ERR_INVARG;

    if (tl->count >= tl->capacity) {
        size_t new_cap = tl->capacity * 2;
        ct_event_t *new_arr = (ct_event_t*)realloc(tl->events, new_cap * sizeof(ct_event_t));
        if (!new_arr) return CT_ERR_NOMEM;
        tl->events = new_arr;
        tl->capacity = new_cap;
    }

    tl->events[tl->count++] = *ev;
    tl->is_sorted = false;
    return CT_OK;
}

ct_status_t ct_timeline_sort(ct_timeline_t *tl) {
    if (!tl) return CT_ERR_INVARG;
    if (tl->is_sorted) return CT_OK;

    ct_status_t st = ct_stable_merge_sort_events(tl->events, tl->count);
    if (st == CT_OK) {
        tl->is_sorted = true;
    }
    return st;
}

ct_status_t ct_timeline_filter(const ct_timeline_t *tl, const ct_timeline_filter_t *filter,
                               ct_event_t **out_results, size_t *out_count, size_t *out_total_matched) {
    if (!tl || !out_results || !out_count) return CT_ERR_INVARG;
    *out_results = NULL;
    *out_count = 0;
    if (out_total_matched) *out_total_matched = 0;

    if (tl->count == 0) return CT_OK;

    size_t slice_start = 0;
    size_t slice_len = tl->count;

    if (tl->is_sorted && filter && (filter->start_epoch > 0 || filter->end_epoch > 0)) {
        ct_status_t st = ct_search_time_range(tl->events, tl->count,
                                             filter->start_epoch, filter->end_epoch,
                                             &slice_start, &slice_len);
        if (st != CT_OK) return st;
    }

    size_t cap = 32;
    ct_event_t *res = (ct_event_t*)malloc(cap * sizeof(ct_event_t));
    if (!res) return CT_ERR_NOMEM;

    size_t matched_count = 0;
    size_t collected_count = 0;
    size_t offset = filter ? filter->offset : 0;
    size_t limit = (filter && filter->limit > 0) ? filter->limit : (size_t)-1;

    for (size_t i = 0; i < slice_len; i++) {
        const ct_event_t *ev = &tl->events[slice_start + i];

        /* Apply non-time filters */
        if (filter) {
            if (!tl->is_sorted) {
                if (filter->start_epoch > 0 && ev->timestamp_epoch < filter->start_epoch) continue;
                if (filter->end_epoch > 0 && ev->timestamp_epoch > filter->end_epoch) continue;
            }
            if (filter->event_type && filter->event_type[0] != '\0') {
                if (strcmp(ev->event_type, filter->event_type) != 0) continue;
            }
            if (filter->entity_id && filter->entity_id[0] != '\0') {
                if (strcmp(ev->entity_id, filter->entity_id) != 0) continue;
            }
            if (filter->min_severity > 0 && ev->severity < filter->min_severity) {
                continue;
            }
        }

        matched_count++;

        if (matched_count > offset && collected_count < limit) {
            if (collected_count >= cap) {
                cap *= 2;
                ct_event_t *new_res = (ct_event_t*)realloc(res, cap * sizeof(ct_event_t));
                if (!new_res) {
                    free(res);
                    return CT_ERR_NOMEM;
                }
                res = new_res;
            }
            res[collected_count++] = *ev;
        }
    }

    if (out_total_matched) *out_total_matched = matched_count;
    *out_count = collected_count;
    *out_results = res;
    return CT_OK;
}

ct_status_t ct_timeline_detect_bursts(const ct_timeline_t *tl,
                                      int64_t window_seconds,
                                      uint32_t count_threshold,
                                      int32_t severity_threshold,
                                      ct_burst_window_t **out_bursts,
                                      size_t *out_burst_count) {
    if (!tl || !out_bursts || !out_burst_count) return CT_ERR_INVARG;
    *out_bursts = NULL;
    *out_burst_count = 0;

    if (tl->count == 0 || window_seconds <= 0) return CT_OK;
    if (count_threshold == 0 && severity_threshold <= 0) return CT_ERR_INVARG;

    ct_deque_t dq;
    ct_status_t st = ct_deque_init(&dq, 64);
    if (st != CT_OK) return st;

    size_t burst_cap = 8;
    ct_burst_window_t *bursts = (ct_burst_window_t*)malloc(burst_cap * sizeof(ct_burst_window_t));
    if (!bursts) {
        ct_deque_free(&dq);
        return CT_ERR_NOMEM;
    }
    size_t burst_cnt = 0;

    size_t left = 0;
    for (size_t right = 0; right < tl->count; right++) {
        /* Maintain monotonic deque for maximum severity (descending) */
        while (!ct_deque_is_empty(&dq)) {
            int32_t tail_idx = 0;
            ct_deque_peek_back(&dq, &tail_idx);
            if (tl->events[tail_idx].severity <= tl->events[right].severity) {
                ct_deque_pop_back(&dq, NULL);
            } else {
                break;
            }
        }
        ct_deque_push_back(&dq, (int32_t)right);

        /* Slide window left edge */
        while (left <= right && (tl->events[right].timestamp_epoch - tl->events[left].timestamp_epoch) > window_seconds) {
            int32_t head_idx = 0;
            ct_deque_peek_front(&dq, &head_idx);
            if ((size_t)head_idx == left) {
                ct_deque_pop_front(&dq, NULL);
            }
            left++;
        }

        uint32_t cur_count = (uint32_t)(right - left + 1);
        int32_t max_sev = 0;
        if (!ct_deque_is_empty(&dq)) {
            int32_t head_idx = 0;
            ct_deque_peek_front(&dq, &head_idx);
            max_sev = tl->events[head_idx].severity;
        }

        bool trigger = false;
        if (count_threshold > 0 && cur_count >= count_threshold) trigger = true;
        if (severity_threshold > 0 && max_sev >= severity_threshold) trigger = true;

        if (trigger) {
            /* Avoid duplicate overlapping window reporting with identical boundaries */
            if (burst_cnt > 0) {
                ct_burst_window_t *last = &bursts[burst_cnt - 1];
                if (last->start_time == tl->events[left].timestamp_epoch &&
                    last->end_time == tl->events[right].timestamp_epoch) {
                    continue;
                }
            }

            if (burst_cnt >= burst_cap) {
                burst_cap *= 2;
                ct_burst_window_t *new_b = (ct_burst_window_t*)realloc(bursts, burst_cap * sizeof(ct_burst_window_t));
                if (!new_b) {
                    ct_burst_windows_free(bursts, burst_cnt);
                    ct_deque_free(&dq);
                    return CT_ERR_NOMEM;
                }
                bursts = new_b;
            }

            ct_burst_window_t *b = &bursts[burst_cnt++];
            b->start_time = tl->events[left].timestamp_epoch;
            b->end_time = tl->events[right].timestamp_epoch;
            strncpy(b->start_str, tl->events[left].timestamp_str, CT_MAX_TIME_STR_LEN - 1);
            b->start_str[CT_MAX_TIME_STR_LEN - 1] = '\0';
            strncpy(b->end_str, tl->events[right].timestamp_str, CT_MAX_TIME_STR_LEN - 1);
            b->end_str[CT_MAX_TIME_STR_LEN - 1] = '\0';
            b->event_count = cur_count;
            b->max_severity = max_sev;

            b->event_indices = (uint32_t*)malloc(cur_count * sizeof(uint32_t));
            if (b->event_indices) {
                for (size_t k = 0; k < cur_count; k++) {
                    b->event_indices[k] = (uint32_t)(left + k);
                }
            }
        }
    }

    ct_deque_free(&dq);
    *out_bursts = bursts;
    *out_burst_count = burst_cnt;
    return CT_OK;
}

void ct_burst_windows_free(ct_burst_window_t *bursts, size_t count) {
    if (!bursts) return;
    for (size_t i = 0; i < count; i++) {
        if (bursts[i].event_indices) {
            free(bursts[i].event_indices);
        }
    }
    free(bursts);
}

void ct_timeline_free(ct_timeline_t *tl) {
    if (!tl) return;
    if (tl->events) {
        free(tl->events);
        tl->events = NULL;
    }
    tl->count = 0;
    tl->capacity = 0;
    tl->is_sorted = false;
}
