# Media identity model evaluation

Status: evaluation that the owner has since accepted. ADR-005 is **ACCEPTED**. Conflicts C3 and C4 are closed. This file is not a second decision. The conflict section below is the history of the choice.

Owners: `docs/MEDIA_IDENTITY_SPECIFICATION.md`, `docs/MEDIA_REGISTRY_SCHEMA.md`, `docs/MEDIA_IDENTITY_AND_METADATA.md`, `docs/MEDIA_IDENTITY_AND_BIN_RELATIONSHIP.md`, `docs/PROXY_AND_ORIGINAL_MEDIA_RELATIONSHIP.md`, `docs/BIN_ARCHITECTURE.md`, `docs/04_CORE_DATA_MODEL.md`.

## 1. The conflict

The documents use asset id, Media ID, origin id, proxy media id, `proxy_id`, and location id.

`docs/BIN_ARCHITECTURE.md` says a proxy hangs off the original Media ID and, in an identity table, that a proxy receives its own media id. `docs/PROXY_AND_ORIGINAL_MEDIA_RELATIONSHIP.md` tightens the proxy to a `proxy_id` that references the original Media ID. Timeline instances must not mint a new identity when they are placed, copied, slipped, or relinked.

## 2. Recommendation

**Recommended for adoption**, not accepted. One editorial Media ID, with representations:

| Row | Identity it carries | What it references |
| --- | --- | --- |
| Logical recording | `media_id`, a UUID minted when the recording is first accepted | Nothing else is the editorial handle |
| Master clip | Its own object id | That `media_id` |
| Timeline instance | Its own instance id and the source object id | The same `media_id`. Placement does not mint one |
| Location | Its own location id, plus a role (`original`, `master`, `proxy` as the location documents propose) | The `media_id`. The path is a hint |
| Proxy or transcode | Its own `proxy_id` or equivalent row id | The original `media_id`. It is a representation |

Rules that go with that shape:

- Earlier documents that say “asset id” mean this same handle until the glossary is edited. New text says Media ID. Do not add both columns.
- An origin id, if the metadata document’s recording identifier is kept, names a recording. It is not the timeline’s foreign key and it is not minted by the timeline.
- Camera UMIDs, reel names, paths, filenames, checksums, and FFmpeg package UMIDs are evidence. The MXF files written in this review carry FFmpeg company and product names and generated UMIDs (`docs/validation/FFMPEG_MXF_ROUNDTRIP_TEST.md`). Those values are not a Media ID.
- A retired Media ID is not reissued.
- A checksum mismatch is a verification failure. It does not mint a replacement Media ID.
- Switching proxy and original changes a playback selection. It does not change instance id, source range, or record range.
- Relink updates a location for an existing Media ID. It does not edit the cut and it is not a conform state.
- A bin membership is a relationship between a bin id and an object id. Creating a directory does not create a bin.

**not recommended:**

- A second Media ID for every proxy, with the cut following whichever file is online. That makes the timeline the owner of identity.
- Deriving the id from a hash of the path. A rename would mint a new take.
- Storing a filesystem path, a proxy path, or a checksum as the instance’s identity.

**further research:**

- Whether the metadata document’s origin id is kept beside Media ID, once the glossary is edited. It must not become a second editorial handle.
- The third location role `original`, which `docs/NATIVE_MEDIA_ARCHITECTURE.md` recommends and `ARCHITECTURE.md` §5.3 does not yet list. This review does not accept that role. Originals stay unmodified either way.

## 3. What a representation is allowed to differ in

A proxy may differ in codec, raster, and file path. The proxy relationship document requires the same frame count, rate, timecode label, and channel map as the master. A proxy that drops or reorders channels is rejected under that proposal. This review does not relax it.

A mezzanine that FFmpeg can open is a location and a technical description. It is not a new recording. The 2026-11-20 round trip shows why: the same eight picture frames were stored with an audio sample count the editorial `sampleIndex` function does not use. That is a property of a representation, checked on ingest. It is not a reason to mint a second Media ID, and it is not a conform.

## 4. What Subhajit is asked

Accept one Media ID with representation rows, or write the other shape. Until then the bin architecture sentence and the proxy document still disagree, and implementers do not guess.
