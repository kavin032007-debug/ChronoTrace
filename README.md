# ChronoTrace

**Explainable Digital Incident Reconstruction & Evidence Correlation Platform**

[![Deploy to Render](https://render.com/images/deploy-to-render-button.svg)](https://render.com/deploy?repo=https://github.com/kavin032007-debug/ChronoTrace)

ChronoTrace is an investigation platform that correlates fragmented event logs across users, devices, files, and networks. It reconstructs incident timelines, uncovers relationships, detects bursts of anomalous activity, identifies critical connecting points between isolated evidence groups, and validates evidence integrity through cryptographic manifest verification.

---

## Key Capabilities

- **Structured Log Ingestion & Validation**: Ingests event logs, entity definitions, and relationship maps with rigorous validation. Malformed entries, timestamp inconsistencies, and referential errors are reported with exact line numbers and reasons.
- **Chronological Timeline Reconstruction**: Orders records with stable tie-breaking, provides rapid range queries and filtering by event type, associated entity, or severity levels.
- **Evidence Graph Exploration**: Interactive multi-step relationship explorer allowing investigators to focus on any entity or event, expanding one, two, or three connection steps.
- **Relationship Pathway Reconstruction**: Traces the shortest connection chain between any two evidence items by connection hops or by lowest time gap, detailing every intermediate entity and supporting record ID.
- **Cluster & Critical Connector Analysis**: Automatically groups interconnected entities and detects critical articulation nodes whose removal would fragment an evidence cluster.
- **Sliding-Window Burst Detection**: Pinpoints rapid bursts of high-volume or high-severity activity across user-defined time intervals.
- **Cryptographic Evidence Integrity Audit**: Calculates cryptographic SHA-256 hashes for all imported evidence files, constructs a Merkle tree manifest, and verifies file integrity against stored baselines.
- **Formal Audit Reporting**: Compiles chronological timelines, evidence pathways, connector analysis, and integrity results into a downloadable report.

---

## Getting Started

### 1. Build the Platform
Run the build command to generate the server application and test suite:

```bash
make all
```

To run the verification test suite:

```bash
make test
```

### 2. Start the Service
Launch the investigation service daemon:

```bash
./chronotrace.exe --port 8080 --data data --root frontend
```

By default, the server listens on `http://127.0.0.1:8080` (or specified port) and serves the interactive workspace.

### 3. Open the Workspace
Open your web browser and navigate to:
```
http://localhost:8080
```
or access the Executive Overview Dashboard at:
```
http://localhost:8080/dashboard.html
```

---

## Log Ingestion Schema

ChronoTrace standardizes evidence across three CSV files:

### `events.csv`
- `event_id`: Unique identifier (e.g. `E101`)
- `timestamp`: UTC timestamp in format `YYYY-MM-DD HH:MM:SS`
- `entity_id`: Associated entity reference (e.g. `U01`)
- `event_type`: Event classification (e.g. `login`, `file_modified`, `privilege_escalation`)
- `severity`: Numeric severity ranking (`1`, `2`, `3`)

### `entities.csv`
- `entity_id`: Unique identifier (e.g. `U01`, `D01`, `F01`)
- `entity_type`: Category (`user`, `device`, `file`, `network`)
- `name`: Human-readable label (e.g. `User A`, `Server Alpha`)

### `relationships.csv`
- `relationship_id`: Unique relationship record identifier (e.g. `R01`)
- `source_id`: Source entity or event ID
- `target_id`: Target entity or event ID
- `relationship`: Descriptive relationship label (e.g. `logged_into`, `accessed`, `connected_to`)

---

## REST API Reference

| Endpoint | Method | Description |
|---|---|---|
| `/api/import` | POST | Import and validate datasets from repository |
| `/api/events` | GET | Retrieve and filter chronological event records |
| `/api/events/:id` | GET | Retrieve single event details by identifier |
| `/api/entities/:id` | GET | Retrieve entity details and incident connections |
| `/api/search?prefix=...` | GET | Real-time prefix suggestions for entities and events |
| `/api/graph?entity_id=...&depth=...` | GET | Bounded neighborhood graph for visualization |
| `/api/path?from=...&to=...&mode=...` | GET | Trace evidence connection path between two nodes |
| `/api/groups` | GET | Analyze connected clusters, articulation points, and bridges |
| `/api/bursts?window=...&threshold=...` | GET | Detect rapid activity bursts across sliding time windows |
| `/api/integrity/verify` | POST | Verify cryptographic file hashes against baseline manifest |
| `/api/reports` | POST | Generate a formal investigation report |

---

## Investigation Methodology

ChronoTrace reconstructs recorded system activity, relationship pathways, and evidence file integrity. Findings represent observed correlation from verified log entries and do not assume speculative causality without independent corroboration.
