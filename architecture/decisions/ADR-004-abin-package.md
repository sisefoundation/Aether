# ADR-004 — `.abin` is a versioned ZIP of public JSON

- Status: **ACCEPTED** as an interchange package, not as a database. The ZIP member layout in this ADR remains the byte specification. It was not reopened.
- Date: 2026-11-20
- Deciders: project owner, for the interchange role (ADR-003). The member layout stays the specification already written here and in `docs/BIN_FILE_FORMAT_SPECIFICATION.md`.
- Related: `docs/BIN_FILE_FORMAT_SPECIFICATION.md`, `docs/BIN_ARCHITECTURE.md`, `docs/BIN_PORTABILITY_AND_SHARING.md`

## Context

A bin has to move between projects when the source project is absent. A SQLite file used as that package invites a dump of whatever tables were open and invites two writers on a shared volume. The bin architecture judged that option and rejected it for the package.

## Decision

**ACCEPTED** as interchange. Format version 1 of `.abin` is a ZIP archive, method 0 (store), of UTF-8 JSON members with a published list. `abin.json` carries `format = aether-abin`, `format_version = 1`, and `schema_version = 1`. The package also carries Bin ID, origin Project ID, editorial objects, sequence references, media references, a dependency manifest, compatibility, and thumbnail or metadata references, as the format specification details them. Paths inside `media_refs.json` are hints. Object ids and Media IDs are the identity. Unknown members and unknown object types are retained and listed, or the import transaction rolls back if the person declines retention. The previous good file remains until a partial write is verified and renamed.

SQLite `project.db` is the only editorial authority (ADR-003). Opening a bin for editing loads the package into that database. Import resolves Media IDs that already exist and does not duplicate a Media Asset because a bin was imported. The package is not a second project database. The package file can still move without the origin project.

## Consequences

- Tests can round-trip JSON without linking Qt or a codec.
- A writer that omits an object because the type is new or old fails the format’s own rule.
- Checksums of members are SHA-256 over uncompressed member bytes. The algorithm name is stored.

## Alternatives

- SQLite as the `.abin`. **Not accepted.** The package is not an authoritative database.
- A binary container. Not proposed.

## Confirmation

The interchange role was accepted by the project owner on 2026-11-20. No `.abin` sample exists. Acceptance of the role is not a test result.
