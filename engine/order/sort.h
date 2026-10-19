#ifndef CHRONOTRACE_SORT_H
#define CHRONOTRACE_SORT_H

#include "../../include/models.h"

int ct_event_comparator(const ct_event_t *a, const ct_event_t *b);
ct_status_t ct_stable_merge_sort_events(ct_event_t *events, size_t count);

#endif /* CHRONOTRACE_SORT_H */
