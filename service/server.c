#include "server.h"
#include "api.h"
#include "../third_party/mongoose.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static ct_server_config_t *s_server_config = NULL;

static void send_json_reply(struct mg_connection *c, int status, cJSON *json) {
    if (!json) {
        mg_http_reply(c, 500, "Content-Type: application/json\r\nAccess-Control-Allow-Origin: *\r\n", "{\"status\":\"error\",\"error\":\"Internal server error\",\"message\":\"Internal server error\"}\n");
        return;
    }
    int http_status = status;
    cJSON *st_item = cJSON_GetObjectItem(json, "status");
    if (st_item && st_item->valuestring && strcmp(st_item->valuestring, "error") == 0) {
        if (http_status == 200) {
            cJSON *msg_item = cJSON_GetObjectItem(json, "message");
            if (msg_item && msg_item->valuestring && strstr(msg_item->valuestring, "not found")) {
                http_status = 404;
            } else {
                http_status = 400;
            }
        }
    }
    char *str = cJSON_PrintUnformatted(json);
    mg_http_reply(c, http_status, "Content-Type: application/json\r\nAccess-Control-Allow-Origin: *\r\n", "%s\n", str ? str : "{}");
    if (str) free(str);
    cJSON_Delete(json);
}

static void ev_handler(struct mg_connection *c, int ev, void *ev_data) {
    if (ev == MG_EV_HTTP_MSG) {
        struct mg_http_message *hm = (struct mg_http_message *)ev_data;
        ct_dataset_t *ds = s_server_config ? s_server_config->dataset : NULL;

        /* Handle CORS pre-flight */
        if (mg_strcmp(hm->method, mg_str("OPTIONS")) == 0) {
            mg_http_reply(c, 200,
                "Access-Control-Allow-Origin: *\r\n"
                "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"
                "Access-Control-Allow-Headers: Content-Type\r\n",
                "");
            return;
        }

        /* Extract body string if present */
        char *body_str = NULL;
        if (hm->body.len > 0) {
            body_str = (char*)malloc(hm->body.len + 1);
            if (body_str) {
                memcpy(body_str, hm->body.buf, hm->body.len);
                body_str[hm->body.len] = '\0';
            }
        }

        /* Extract query string if present */
        char *qs_str = NULL;
        if (hm->query.len > 0) {
            qs_str = (char*)malloc(hm->query.len + 1);
            if (qs_str) {
                memcpy(qs_str, hm->query.buf, hm->query.len);
                qs_str[hm->query.len] = '\0';
            }
        }

        /* API Routing */
        if (mg_match(hm->uri, mg_str("/api/import"), NULL)) {
            send_json_reply(c, 200, ct_api_handle_import(ds, hm->body.buf, hm->body.len));
        } else if (mg_match(hm->uri, mg_str("/api/events/*"), NULL)) {
            char ev_id[64] = {0};
            size_t prefix_len = strlen("/api/events/");
            if (hm->uri.len > prefix_len) {
                size_t id_len = hm->uri.len - prefix_len;
                if (id_len >= sizeof(ev_id)) id_len = sizeof(ev_id) - 1;
                memcpy(ev_id, hm->uri.buf + prefix_len, id_len);
                ev_id[id_len] = '\0';
            }
            send_json_reply(c, 200, ct_api_handle_get_event_by_id(ds, ev_id));
        } else if (mg_match(hm->uri, mg_str("/api/events"), NULL)) {
            send_json_reply(c, 200, ct_api_handle_get_events(ds, qs_str ? qs_str : ""));
        } else if (mg_match(hm->uri, mg_str("/api/entities/*"), NULL)) {
            char ent_id[64] = {0};
            size_t prefix_len = strlen("/api/entities/");
            if (hm->uri.len > prefix_len) {
                size_t id_len = hm->uri.len - prefix_len;
                if (id_len >= sizeof(ent_id)) id_len = sizeof(ent_id) - 1;
                memcpy(ent_id, hm->uri.buf + prefix_len, id_len);
                ent_id[id_len] = '\0';
            }
            send_json_reply(c, 200, ct_api_handle_get_entity_by_id(ds, ent_id));
        } else if (mg_match(hm->uri, mg_str("/api/search"), NULL)) {
            char prefix[64] = {0};
            ct_get_query_param(qs_str, "prefix", prefix, sizeof(prefix));
            send_json_reply(c, 200, ct_api_handle_search(ds, prefix));
        } else if (mg_match(hm->uri, mg_str("/api/graph"), NULL)) {
            send_json_reply(c, 200, ct_api_handle_graph(ds, qs_str ? qs_str : ""));
        } else if (mg_match(hm->uri, mg_str("/api/path"), NULL)) {
            send_json_reply(c, 200, ct_api_handle_path(ds, qs_str ? qs_str : ""));
        } else if (mg_match(hm->uri, mg_str("/api/groups"), NULL)) {
            send_json_reply(c, 200, ct_api_handle_groups(ds));
        } else if (mg_match(hm->uri, mg_str("/api/timeline"), NULL)) {
            send_json_reply(c, 200, ct_api_handle_timeline(ds, qs_str ? qs_str : ""));
        } else if (mg_match(hm->uri, mg_str("/api/bursts"), NULL)) {
            send_json_reply(c, 200, ct_api_handle_bursts(ds, qs_str ? qs_str : ""));
        } else if (mg_match(hm->uri, mg_str("/api/integrity/verify"), NULL)) {
            send_json_reply(c, 200, ct_api_handle_integrity_verify(ds));
        } else if (mg_match(hm->uri, mg_str("/api/reports/*"), NULL)) {
            char rpt_id[128] = {0};
            size_t prefix_len = strlen("/api/reports/");
            if (hm->uri.len > prefix_len) {
                size_t id_len = hm->uri.len - prefix_len;
                if (id_len >= sizeof(rpt_id)) id_len = sizeof(rpt_id) - 1;
                memcpy(rpt_id, hm->uri.buf + prefix_len, id_len);
                rpt_id[id_len] = '\0';
            }
            send_json_reply(c, 200, ct_api_handle_get_report_by_id(ds, rpt_id));
        } else if (mg_match(hm->uri, mg_str("/api/reports"), NULL)) {
            send_json_reply(c, 200, ct_api_handle_create_report(ds, body_str));
        } else if (mg_match(hm->uri, mg_str("/api/reset"), NULL) || mg_match(hm->uri, mg_str("/api/clear"), NULL)) {
            ct_dataset_reset(ds);
            ds->version++;
            cJSON *res = cJSON_CreateObject();
            cJSON_AddStringToObject(res, "status", "success");
            cJSON_AddStringToObject(res, "message", "Dataset cleared. Ready for evidence import.");
            send_json_reply(c, 200, res);
        } else if (mg_match(hm->uri, mg_str("/landing"), NULL)) {
            struct mg_http_serve_opts opts;
            memset(&opts, 0, sizeof(opts));
            opts.root_dir = s_server_config->web_root;
            mg_http_serve_file(c, hm, "frontend/landing.html", &opts);
        } else {
            /* Serve static web assets */
            struct mg_http_serve_opts opts;
            memset(&opts, 0, sizeof(opts));
            opts.root_dir = s_server_config->web_root;
            opts.page404 = NULL;
            mg_http_serve_dir(c, hm, &opts);
        }

        if (body_str) free(body_str);
        if (qs_str) free(qs_str);
    }
}

int ct_server_run(ct_server_config_t *config) {
    if (!config) return 1;
    s_server_config = config;

    struct mg_mgr mgr;
    mg_mgr_init(&mgr);

    char addr[64];
    snprintf(addr, sizeof(addr), "http://0.0.0.0:%d", config->port);

    printf("[ChronoTrace] Starting service on %s (Web root: %s)...\n", addr, config->web_root);
    struct mg_connection *c = mg_http_listen(&mgr, addr, ev_handler, NULL);
    if (!c) {
        fprintf(stderr, "[ChronoTrace] Failed to listen on %s\n", addr);
        mg_mgr_free(&mgr);
        return 1;
    }

    printf("[ChronoTrace] Service is ready and listening for investigation requests.\n");

    for (;;) {
        mg_mgr_poll(&mgr, 500);
    }

    mg_mgr_free(&mgr);
    return 0;
}
