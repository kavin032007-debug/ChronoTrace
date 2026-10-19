#ifndef CHRONOTRACE_SERVER_H
#define CHRONOTRACE_SERVER_H

#include "../ingest/storage.h"

typedef struct {
    int port;
    const char *web_root;
    ct_dataset_t *dataset;
} ct_server_config_t;

int ct_server_run(ct_server_config_t *config);

#endif /* CHRONOTRACE_SERVER_H */
