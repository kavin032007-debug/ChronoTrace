#ifndef CHRONOTRACE_VALIDATION_H
#define CHRONOTRACE_VALIDATION_H

#include "../include/models.h"
#include "../engine/core/vector.h"

typedef struct {
    uint32_t total_rows_read;
    uint32_t valid_entities;
    uint32_t valid_events;
    uint32_t valid_relationships;
    uint32_t rejected_rows;
} ct_import_summary_t;

/* Validates identifier format (non-empty, alphanumeric / underscore / hyphen, length < 64) */
bool ct_validate_id_format(const char *id);

#endif /* CHRONOTRACE_VALIDATION_H */
