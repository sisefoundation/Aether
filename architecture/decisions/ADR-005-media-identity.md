# ADR-005 — One Media ID and representations

- Status: **ACCEPTED**
- Date: 2026-11-20
- Deciders: project owner.
- Related: `docs/MEDIA_IDENTITY_SPECIFICATION.md`, `docs/MEDIA_IDENTITY_AND_METADATA.md`, `docs/MEDIA_REGISTRY_SCHEMA.md`, `docs/PROXY_AND_ORIGINAL_MEDIA_RELATIONSHIP.md`, `docs/MEDIA_CONSOLIDATION_SPECIFICATION.md`

## Context

The documents used asset id, Media ID, origin id, proxy media id, `proxy_id`, and location id. One of those sentences gave a proxy its own media id. The owner has closed that conflict. The editorial handle is not weakened.

Where an older sentence says “asset id”, it means this Media ID. Origin id, when a metadata document still stores one, names a recording grouping. It is not a second editorial source identity and it is not what a timeline instance mints.

## Decision

**ACCEPTED.** One stable Media ID per logical Media Asset.

A proxy is a representation of that asset. It is not a new editorial source identity. The same is true of a transcode, a consolidated copy, and any other validated version.

Representations are at least:

- `ORIGINAL`
- `PROXY`
- `TRANSCODE`
- `CONSOLIDATED`
- other roles only when a later validation row defines them

Each representation has its own Representation ID. Physical location is a separate record: a Location ID, a path, and checksum evidence. The path is not the Media ID and it is not the Representation ID.

### Rules

- The Media ID is stable across rename and move.
- Proxy generation does not change the Media ID. Proxy regeneration does not change the Media ID.
- Consolidation does not create a new logical asset. The consolidated file is a `CONSOLIDATED` representation of the existing Media ID. The previous representation stays in the historical record.
- Each representation is validated independently. A failed proxy does not invalidate the original’s Media ID. A failed original checksum does not mint a replacement Media ID.
- A missing original does not invalidate the editorial object. The original representation is offline. Another representation may be selected for playback. The cut still cites the same editorial object and the same Media ID.
- Relink must not silently replace the Media ID. Relink updates location evidence for the Media ID the editorial object already has. If the candidate file does not match, the previous Media ID and the previous location stay, and the result is a failure.
- A timeline instance references an editorial object. It resolves media through the Media Registry. It does not store a path as identity and it does not mint a Media ID.

Camera UMIDs, reel names, filenames, paths, and checksums are evidence. They are not the Media ID. A retired Media ID is not reissued. FFmpeg material-package UMIDs on the 2026-11-20 MXF fixtures are not Media IDs.

### Duplicate physical files

Two paths that are the same logical asset are two location records, or a second location of an existing representation. They do not mint a second Media ID. A byte-identical file is not automatically a new asset. If ingest cannot establish that it is the same logical asset, it is a candidate match for a person to confirm. It is not a silent replacement of a Media ID.

### Multiple proxies

Each proxy file has its own Representation ID with role `PROXY` and the same Media ID. More than one proxy may exist. Playback selection chooses a representation. It does not retarget the cut.

### Regenerated files

Regeneration does not change the Media ID. The new file receives a new Representation ID. The previous representation is kept as a historical row, marked superseded, with a revision so the earlier file can still be named. Historical rows are required where an edit, a relink, or a recovery record still points at that generation. A superseded representation is not a second logical asset.

### Split media

Picture and sound that are one logical asset share one Media ID. OP-Atom picture and the matching audio files are components of that asset, not new Media IDs. A representation may cite more than one physical file when those files are essence components. Linking split files that were ingested as unrelated assets is an explicit operation. It must not silently destroy or replace a Media ID.

### Audio-only and video-only

An audio-only asset has one Media ID and no picture essence. A video-only asset has one Media ID and no audio essence. The absent essence is a fact on the representation. It does not invalidate the Media ID or the editorial object.

## Consequences

- Audit conflicts C3 and C4 are closed. The glossary name in new text is Media ID. Older “asset id” sentences mean that id.
- `docs/BIN_ARCHITECTURE.md` no longer grants a proxy its own media id.
- Switching representation changes playback selection. It does not change instance id, source range, or record range.
- This decision does not claim that any representation on this machine is validated. The 30000/1001 sample-count failure is a property of a file, checked on ingest, not a reason to mint a second Media ID.

## Alternatives

- A second Media ID per proxy, with the cut following whichever file is online. **Not accepted.**
- An id derived from a path or a checksum. **Not accepted.**
- Silent relink onto a different Media ID when the original is missing. **Not accepted.**

## Confirmation

Accepted by the project owner on 2026-11-20. The stable Media ID is not weakened.
