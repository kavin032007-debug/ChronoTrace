#include "server.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[]) {
    int port = 8080;
    const char *data_dir = "data";
    const char *web_root = "frontend";

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--port") == 0 && i + 1 < argc) {
            port = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--data") == 0 && i + 1 < argc) {
            data_dir = argv[++i];
        } else if (strcmp(argv[i], "--root") == 0 && i + 1 < argc) {
            web_root = argv[++i];
        }
    }

    printf("====================================================\n");
    printf("   ChronoTrace - Explainable Incident Reconstruction\n");
    printf("====================================================\n");

    ct_dataset_t ds;
    ct_status_t st = ct_dataset_init(&ds, data_dir);
    if (st != CT_OK) {
        fprintf(stderr, "[Error] Failed to initialize dataset storage: code %d\n", st);
        return 1;
    }

    printf("[ChronoTrace] Server initialized with clean, empty dataset state.\n");
    printf("[ChronoTrace] Ready for dataset import via interface or API.\n");

    ct_server_config_t config;
    config.port = port;
    config.web_root = web_root;
    config.dataset = &ds;

    int ret = ct_server_run(&config);

    ct_dataset_free(&ds);
    return ret;
}
