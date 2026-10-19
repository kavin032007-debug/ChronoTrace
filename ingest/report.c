#include "report.h"
#include "../engine/graph/bfs.h"
#include "../engine/graph/tarjan.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

ct_status_t ct_report_generate_html(const ct_dataset_t *ds, const ct_report_request_t *req,
                                    char **out_html, size_t *out_len) {
    if (!ds || !out_html || !out_len) return CT_ERR_INVARG;

    size_t buf_size = 128 * 1024;
    char *buf = (char*)malloc(buf_size);
    if (!buf) return CT_ERR_NOMEM;

    time_t now = time(NULL);
    char date_str[64];
    strftime(date_str, sizeof(date_str), "%Y-%m-%d %H:%M:%S UTC", gmtime(&now));

    int offset = 0;
    offset += snprintf(buf + offset, buf_size - offset,
        "<!DOCTYPE html>\n"
        "<html lang=\"en\">\n"
        "<head>\n"
        "<meta charset=\"UTF-8\">\n"
        "<title>Incident Reconstruction & Evidence Audit Report - ChronoTrace</title>\n"
        "<style>\n"
        "body { font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif; background: #0f172a; color: #f8fafc; margin: 0; padding: 40px; line-height: 1.6; }\n"
        ".container { max-width: 960px; margin: 0 auto; background: #1e293b; border-radius: 12px; border: 1px solid #334155; padding: 36px; box-shadow: 0 10px 25px rgba(0,0,0,0.5); }\n"
        "h1 { color: #38bdf8; font-size: 28px; margin-top: 0; border-bottom: 2px solid #334155; padding-bottom: 12px; }\n"
        "h2 { color: #94a3b8; font-size: 20px; margin-top: 28px; border-left: 4px solid #38bdf8; padding-left: 10px; }\n"
        ".meta { display: grid; grid-template-columns: repeat(3, 1fr); gap: 16px; margin: 20px 0; background: #0f172a; padding: 16px; border-radius: 8px; border: 1px solid #1e293b; }\n"
        ".meta-item { font-size: 13px; color: #94a3b8; }\n"
        ".meta-val { font-size: 16px; color: #f1f5f9; font-weight: 600; margin-top: 4px; }\n"
        "table { width: 100%%; border-collapse: collapse; margin-top: 14px; font-size: 14px; }\n"
        "th { background: #0f172a; color: #cbd5e1; text-align: left; padding: 10px 12px; border-bottom: 1px solid #334155; }\n"
        "td { padding: 10px 12px; border-bottom: 1px solid #334155; color: #e2e8f0; }\n"
        "tr:nth-child(even) { background: rgba(255, 255, 255, 0.02); }\n"
        ".badge { display: inline-block; padding: 2px 8px; border-radius: 4px; font-size: 11px; font-weight: 600; text-transform: uppercase; }\n"
        ".badge-sev1 { background: #064e3b; color: #6ee7b7; }\n"
        ".badge-sev2 { background: #78350f; color: #fcd34d; }\n"
        ".badge-sev3 { background: #7f1d1d; color: #fca5a5; }\n"
        ".badge-verified { background: #064e3b; color: #34d399; }\n"
        ".badge-unverified { background: #7f1d1d; color: #f87171; }\n"
        ".callout { background: rgba(56, 189, 248, 0.08); border-left: 4px solid #38bdf8; padding: 14px 18px; border-radius: 4px; margin: 18px 0; font-size: 13px; color: #94a3b8; }\n"
        "</style>\n"
        "</head>\n"
        "<body>\n"
        "<div class=\"container\">\n"
        "<h1>Digital Incident Reconstruction Report</h1>\n"
        "<div class=\"meta\">\n"
        "  <div class=\"meta-item\">REPORT IDENTIFIER<div class=\"meta-val\">%s</div></div>\n"
        "  <div class=\"meta-item\">GENERATION TIMESTAMP<div class=\"meta-val\">%s</div></div>\n"
        "  <div class=\"meta-item\">DATASET VERSION<div class=\"meta-val\">v%lu</div></div>\n"
        "  <div class=\"meta-item\">TOTAL EVENTS AUDITED<div class=\"meta-val\">%lu</div></div>\n"
        "  <div class=\"meta-item\">IDENTIFIED ENTITIES<div class=\"meta-val\">%lu</div></div>\n"
        "  <div class=\"meta-item\">ACTIVE RELATIONSHIPS<div class=\"meta-val\">%lu</div></div>\n"
        "</div>\n"
        "<div class=\"callout\">\n"
        "<strong>Investigation Disclaimer:</strong> This report reconstructs recorded system activity, relationship pathways, and evidence file integrity. Findings represent observed log correlation and do not establish speculative causality without independent forensic corroboration.\n"
        "</div>\n",
        (req && req->report_id[0] != '\0') ? req->report_id : "CR-2026-INCIDENT",
        date_str,
        (unsigned long)ds->version,
        (unsigned long)ds->timeline.count,
        (unsigned long)ds->entities_list.size,
        (unsigned long)ds->relationships.size
    );

    /* Section: Timeline of Key Events */
    offset += snprintf(buf + offset, buf_size - offset,
        "<h2>1. Chronological Timeline Reconstruction</h2>\n"
        "<table>\n"
        "<thead><tr><th>Event ID</th><th>Timestamp (UTC)</th><th>Associated Entity</th><th>Event Type</th><th>Severity</th></tr></thead>\n"
        "<tbody>\n"
    );

    size_t display_events = ds->timeline.count;
    if (req && req->event_count > 0) {
        for (size_t i = 0; i < req->event_count; i++) {
            void *v = NULL;
            if (ct_hashmap_get(&ds->events_map, req->selected_event_ids[i], &v) == CT_OK) {
                const ct_event_t *ev = (const ct_event_t*)v;
                const char *badge_cls = (ev->severity >= 3) ? "badge-sev3" : ((ev->severity == 2) ? "badge-sev2" : "badge-sev1");
                offset += snprintf(buf + offset, buf_size - offset,
                    "<tr><td><strong>%s</strong></td><td>%s</td><td>%s</td><td>%s</td><td><span class=\"badge %s\">Level %d</span></td></tr>\n",
                    ev->event_id, ev->timestamp_str, ev->entity_id, ev->event_type, badge_cls, ev->severity);
            }
        }
    } else {
        if (display_events > 25) display_events = 25;
        for (size_t i = 0; i < display_events; i++) {
            const ct_event_t *ev = &ds->timeline.events[i];
            const char *badge_cls = (ev->severity >= 3) ? "badge-sev3" : ((ev->severity == 2) ? "badge-sev2" : "badge-sev1");
            offset += snprintf(buf + offset, buf_size - offset,
                "<tr><td><strong>%s</strong></td><td>%s</td><td>%s</td><td>%s</td><td><span class=\"badge %s\">Level %d</span></td></tr>\n",
                ev->event_id, ev->timestamp_str, ev->entity_id, ev->event_type, badge_cls, ev->severity);
        }
    }

    offset += snprintf(buf + offset, buf_size - offset, "</tbody></table>\n");

    /* Section: Evidence Path (if requested) */
    if (req && req->path_from[0] != '\0' && req->path_to[0] != '\0') {
        int32_t u_idx = -1, v_idx = -1;
        if (ct_graph_find_node(&ds->graph, req->path_from, &u_idx) == CT_OK &&
            ct_graph_find_node(&ds->graph, req->path_to, &v_idx) == CT_OK) {
            ct_path_result_t path;
            if (ct_bfs_shortest_path(&ds->graph, u_idx, v_idx, &path) == CT_OK) {
                offset += snprintf(buf + offset, buf_size - offset,
                    "<h2>2. Reconstructed Evidence Pathway</h2>\n"
                    "<p>Path from <strong>%s</strong> to <strong>%s</strong> (%u hops):</p>\n"
                    "<table><thead><tr><th>Hop</th><th>Source Node</th><th>Relationship Record</th><th>Target Node</th></tr></thead><tbody>\n",
                    req->path_from, req->path_to, path.hop_count
                );
                for (uint32_t h = 0; h < path.hop_count; h++) {
                    const ct_graph_node_t *n1 = ct_graph_get_node(&ds->graph, path.node_path[h]);
                    const ct_graph_node_t *n2 = ct_graph_get_node(&ds->graph, path.node_path[h + 1]);
                    offset += snprintf(buf + offset, buf_size - offset,
                        "<tr><td>%u</td><td>%s (%s)</td><td><strong>%s</strong></td><td>%s (%s)</td></tr>\n",
                        h + 1, n1 ? n1->id : "?", n1 ? n1->label : "", path.record_ids[h], n2 ? n2->id : "?", n2 ? n2->label : "");
                }
                offset += snprintf(buf + offset, buf_size - offset, "</tbody></table>\n");
                ct_path_result_free(&path);
            }
        }
    }

    /* Section: Critical Linking Points (Tarjan Articulation Points & Bridges) */
    ct_tarjan_result_t tarjan_res;
    if (ct_tarjan_analyze(&ds->graph, &tarjan_res) == CT_OK) {
        offset += snprintf(buf + offset, buf_size - offset,
            "<h2>3. Critical Evidence Connectors & Isolated Groups</h2>\n"
            "<p>The following entities and relationships serve as single points of bridge connectivity between separate evidence groups:</p>\n"
            "<table><thead><tr><th>Type</th><th>Identifier</th><th>Role / Description</th></tr></thead><tbody>\n"
        );

        for (size_t i = 0; i < tarjan_res.articulation_count; i++) {
            const ct_graph_node_t *n = ct_graph_get_node(&ds->graph, tarjan_res.articulation_nodes[i]);
            if (n) {
                offset += snprintf(buf + offset, buf_size - offset,
                    "<tr><td><span class=\"badge badge-sev2\">Linking Entity</span></td><td><strong>%s</strong> (%s)</td><td>Critical articulation node connecting isolated evidence clusters</td></tr>\n",
                    n->id, n->label);
            }
        }

        for (size_t i = 0; i < tarjan_res.bridge_count; i++) {
            const ct_graph_node_t *u = ct_graph_get_node(&ds->graph, tarjan_res.bridges[i].u);
            const ct_graph_node_t *v = ct_graph_get_node(&ds->graph, tarjan_res.bridges[i].v);
            offset += snprintf(buf + offset, buf_size - offset,
                "<tr><td><span class=\"badge badge-sev3\">Bridge Relationship</span></td><td><strong>%s</strong></td><td>Connects %s with %s; removal isolates subgroups</td></tr>\n",
                tarjan_res.bridges[i].record_id, u ? u->id : "?", v ? v->id : "?");
        }

        if (tarjan_res.articulation_count == 0 && tarjan_res.bridge_count == 0) {
            offset += snprintf(buf + offset, buf_size - offset, "<tr><td colspan=\"3\">No single points of failure detected in the evidence graph.</td></tr>\n");
        }

        offset += snprintf(buf + offset, buf_size - offset, "</tbody></table>\n");
        ct_tarjan_result_free(&tarjan_res);
    }

    /* Section: Evidence Integrity Status */
    offset += snprintf(buf + offset, buf_size - offset,
        "<h2>4. Cryptographic Evidence Integrity Audit</h2>\n"
        "<p>Baseline Manifest Fingerprint (Merkle Root): <code>%s</code></p>\n",
        ds->manifest_initialized ? ds->manifest.baseline_manifest_hash : "Not initialized"
    );

    offset += snprintf(buf + offset, buf_size - offset,
        "</div>\n"
        "</body>\n"
        "</html>\n"
    );

    *out_html = buf;
    *out_len = offset;
    return CT_OK;
}

ct_status_t ct_report_generate_json(const ct_dataset_t *ds, const ct_report_request_t *req,
                                    char **out_json, size_t *out_len) {
    if (!ds || !out_json || !out_len) return CT_ERR_INVARG;

    size_t buf_size = 128 * 1024;
    char *buf = (char*)malloc(buf_size);
    if (!buf) return CT_ERR_NOMEM;

    time_t now = time(NULL);
    char date_str[64];
    strftime(date_str, sizeof(date_str), "%Y-%m-%dT%H:%M:%SZ", gmtime(&now));

    int offset = 0;
    offset += snprintf(buf + offset, buf_size - offset,
        "{\n"
        "  \"report_id\": \"%s\",\n"
        "  \"generated_at\": \"%s\",\n"
        "  \"dataset_version\": %lu,\n"
        "  \"metrics\": {\n"
        "    \"total_events\": %lu,\n"
        "    \"total_entities\": %lu,\n"
        "    \"total_relationships\": %lu,\n"
        "    \"rejected_rows\": %u\n"
        "  },\n"
        "  \"manifest_fingerprint\": \"%s\"\n"
        "}\n",
        (req && req->report_id[0] != '\0') ? req->report_id : "CR-2026-REPORT",
        date_str,
        (unsigned long)ds->version,
        (unsigned long)ds->timeline.count,
        (unsigned long)ds->entities_list.size,
        (unsigned long)ds->relationships.size,
        (unsigned int)ds->summary.rejected_rows,
        ds->manifest_initialized ? ds->manifest.baseline_manifest_hash : ""
    );

    *out_json = buf;
    *out_len = offset;
    return CT_OK;
}
