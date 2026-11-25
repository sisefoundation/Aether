# ADR-003 — One project database and a manifest

- Status: **ACCEPTED**
- Date: 2026-11-20
- Deciders: project owner.
- Related: `docs/PROJECT_STORAGE_AND_DATABASE.md`, `docs/PROJECT_FOLDER_SPECIFICATION.md`, `docs/BIN_DATABASE_SCHEMA.md`, `docs/PROJECT_DATABASE_SCHEMA.md`, `docs/MEDIA_REGISTRY_SCHEMA.md`, `docs/RECOVERY_AND_BACKUP.md`, `architecture/decisions/ADR-004-abin-package.md`

## Context

Two package shapes were written. The early architecture used a directory ending in `.sise`, a file `project.sqlite`, and opaque per-module blobs. The later documents use `project.db`, a project manifest, and normalised tables. The owner has chosen the later shape. The early filename is not the authority.

## Decision

**ACCEPTED.** A project has one authoritative SQLite file, `project.db`, and one Project manifest.

`project.db` is the only authority for Project and editorial data. It holds settings, identity, the bin registry, bin membership, editorial objects, Media Registry references, sequence data, timeline data, dependencies, metadata, revision information, and the referential integrity among those rows.

The manifest describes the package. It carries the Project ID, name, format version, database filename, schema version, compatibility, and other package-level metadata. It is not a second editorial authority. If the manifest and the database disagree, recovery starts. Neither file silently wins (`docs/RECOVERY_AND_BACKUP.md`).

Bins are first-class editorial containers inside that one database. A bin is not an independent project database.

`.abin` is a separately specified portable interchange package. It is not an alternative authoritative database. ADR-004 and `docs/BIN_FILE_FORMAT_SPECIFICATION.md` specify the package bytes. The package carries Bin ID, origin Project ID, format and schema version, editorial objects, sequence references, media references, a dependency manifest, compatibility, and thumbnail or metadata references. Import resolves Media IDs that already exist in the destination project. Import does not duplicate a Media Asset because a bin was imported.

PostgreSQL is out of scope. IEEE floating-point columns are not the authority for time.

The operational rules below are the decision. `docs/PROJECT_STORAGE_AND_DATABASE.md` is the storage document that restates them for implementers and points at the schema documents. It does not keep a second package shape open.

### Transaction boundaries

One editorial command is one database transaction. A compound command is still one transaction. The transaction commits only when the command’s invariants hold, including referential integrity. A failure rolls the transaction back and leaves the previous commit. Session undo, when it applies, is a later transaction that restores the prior editorial rows. It is not a side effect inside the original commit, and it is not the commit itself (ADR-009).

### Locking

One writer holds the project lock. A second open while the lock is held is refused. The lock is not the undo stack and it is not a licence.

### Atomic writes

A database transaction commits or rolls back. The manifest is replaced by a write to a temporary file on the same volume, an fsync, and a rename, after the database commit that it describes. A crash between the commit and the manifest rename is a recovery case, not a second editorial document.

### Schema migrations

Schema version is an integer in `project.db` and is repeated in the manifest. A migration runs inside a transaction after a backup copy of the database and the manifest. A failed migration does not leave a half-migrated file as the current database.

### Backup and restore

A snapshot under `BACKUP/` is a copy of the database, the manifest, and settings. It does not include original camera or sound media unless a person runs an explicit archive command. Restore opens a snapshot as a project copy, or replaces the current database only after a confirmed swap. Restore does not delete originals. Restore is not undo.

### Corruption

If the file does not pass the engine integrity check, do not start playback and do not invent rows. Offer a snapshot that passes integrity. Do not choose silently.

### Crash recovery

Crash recovery reopens the last committed `project.db`. It does not require the session undo stack to be present. A completed editorial operation that committed stays in the database. Recovery is independent of undo (ADR-009).

### Project relocation

Moving or renaming the project directory does not mint a new Project ID and does not mint a Media ID. Locations stored as paths outside the project become offline until relink. Relink updates a location. It does not replace a Media ID (ADR-005).

### Bin package validation

Before any imported row is committed, the package is checked for format version, schema version, required identity fields, and internal consistency of its object, sequence, and media references. A failed check rolls the import back. Unknown members follow the format specification: retain and list, or roll back if retention is declined. A package that is not valid is not a partial bin in `project.db`.

### Referential integrity

Foreign keys are enabled on every connection. An editorial object that cites a Media ID requires that Media ID. A timeline instance cites an editorial object. A bin membership cites a bin and an object. Deleting a row that is still cited is refused. Offline media is a location state. It is not a deleted Media ID and it does not delete the editorial object.

## Consequences

- The early `Title.sise/project.sqlite` opaque-blob package is not the canonical file. Audit conflict C2 is closed for the filename and for which file is authoritative.
- Domain modules do not link SQLite. Infrastructure does.
- Two schema documents still exist (`docs/PROJECT_DATABASE_SCHEMA.md` and `docs/BIN_DATABASE_SCHEMA.md`). Implementation uses one migration series narrowed to the rows the prototype stores. It does not generate both documents into one file as two authorities. The later bin and registry tables are the working set. The earlier document is historical for overlapping columns.
- No `project.db` was created by this decision.

## Alternatives

- Opaque blobs in `project.sqlite`. **Not accepted.**
- One SQLite file per bin as the authority. **Not accepted.** Bins stay rows in `project.db`. `.abin` is interchange.
- A manifest that also stores editorial objects as a second source of truth. **Not accepted.**

## Confirmation

Accepted by the project owner on 2026-11-20.
