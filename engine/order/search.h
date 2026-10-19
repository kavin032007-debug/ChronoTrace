#ifndef CHRONOTRACE_SEARCH_H
#define CHRONOTRACE_SEARCH_H

#include "../../include/models.h"

int64_t ct_search_lower_bound(const ct_event_t *events, size_t count, int64_t target_epoch);
int64_t ct_search_upper_bound(const ct_event_t *events, size_t count, int64_t target_epoch);

ct_status_t ct_search_time_range(const ct_event_t *events, size_t count,
                                 int64_t start_epoch, int64_t end_epoch,
                                 size_t *out_start_idx, size_t *out_count);

#endif /* CHRONOTRACE_SEARCH_H */
