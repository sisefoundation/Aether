# Project database architecture evaluation

Status: evaluation that the owner has since accepted. ADR-003 is **ACCEPTED**. No database file was created by that acceptance. This file is not a second decision.

Owners: `docs/PROJECT_STORAGE_AND_DATABASE.md`, `docs/BIN_DATABASE_SCHEMA.md`, `docs/PROJECT_DATABASE_SCHEMA.md`, `docs/PROJECT_FOLDER_SPECIFICATION.md`, `docs/BIN_FILE_FORMAT_SPECIFICATION.md`. Audit conflict C2.

## 1. The two shapes on paper

| Shape | Where it is written | What it is |
| --- | --- | --- |
| `Title.sise/project.sqlite` plus opaque per-module blobs, and a separate `autosave.sqlite` | `ARCHITECTURE.md` §5 | Earlier package. Not withdrawn |
| `project.db`, `project_manifest.json`, normalised tables, folder tree | `docs/PROJECT_ARCHITECTURE.md`, `docs/BIN_DATABASE_SCHEMA.md`, `docs/PROJECT_FOLDER_SPECIFICATION.md` | Later working store |

Both name SQLite. PostgreSQL is described as out of scope. Neither schema was opened on this machine. **Confirmed.**

## 2. Recommendation

**Recommended for adoption**, not accepted:

- One SQLite file named `project.db` in the project directory is the working authority while a project is open.
- `PRAGMA foreign_keys = ON` on every connection.
- WAL for a database on a local disk. `project.db` is not placed on a camera card.
- Integer schema versions. A migration runs in a transaction after a backup copy. A failed migration does not leave a half-migrated file as the current database.
- `project_manifest.json` repeats project id, schema version, and a hash of the database. Disagreement starts recovery. It does not pick a winner silently (`docs/RECOVERY_AND_BACKUP.md`).
- One migration series. Do not generate SQL from both schema documents into the same file. The later bin and registry tables are the series to narrow for the Foundation Prototype: projects, bins, membership, objects, media, locations, sequences, tracks, instances, markers. Wider tables (effects, automation, the full metadata map) can wait.
- The earlier opaque-blob `project.sqlite` package stops being the canonical file if this recommendation is accepted. Until Subhajit says so, implementers do not guess a filename. Retiring it is his decision, not a withdrawal this file can make.
- `.abin` stays a versioned ZIP of public UTF-8 JSON (ADR-004). It is a portable checkpoint of one bin. It is not the working database and it is not a dump of the tables.

**not recommended:**

- PostgreSQL, or any multi-writer server, for the first project file.
- Two live schemas.
- Storing the cut only in an EDL, a cache, or a proxy.
- Placing the authoritative time in IEEE float columns. Rationals are two integers (`docs/05_TIMEBASE_AND_TIMING_ARCHITECTURE.md`).

**subject to prototype:**

- WAL behaviour on the actual project disk. The recommendation is unmeasured.
- Whether a bin checkpoint and the database commit are one transaction or two. `docs/PROJECT_STORAGE_AND_DATABASE.md` leaves that open. The milestone’s `.abin` test can be a direct serializer round trip without settling the project-commit question. Capability 19 may still be deferred by him, in writing, on ADR-004.

**further research:**

- The folder amendment that mentions `BINS/` and `LOGS/BIN_OPERATIONS/`. The folder specification does not list them. The bin id is still not read from the path.
- Snapshot retention counts. They are recommendations in the recovery document, not measurements.

## 3. What the database is not

- Not the Media ID. Identity is a column the registry owns.
- Not the session undo stack. See `docs/technology/UNDO_DURABILITY_EVALUATION.md`.
- Not a conform verdict and not a relink. Relink updates a location row.
- Not a licence to modify originals. Paths in location rows are hints.

## 4. Failure behaviour the recommendation keeps

| Case | Result |
| --- | --- |
| Manifest hash disagrees | Recovery. No silent continue |
| Newer schema than this build | Refuse to open |
| Migration fails | Restore the pre-migration copy |
| Second writer | Lock refuses it |
| Cache deleted | Composition hash unchanged |

## 5. What Subhajit is asked

Accept this shape, keep the opaque `project.sqlite` package, or name a third shape. Silence is not a choice of filename. The question is repeated in `docs/PREBUILD_DECISION_PACKAGE.md`.
