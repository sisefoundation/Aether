# Pre-build decision review

Status: earlier review. The owner’s later decisions are in `docs/PREBUILD_DECISION_PACKAGE.md`. This file does not start the editor, the UI, or the Foundation Prototype, and it does not grant approval.

Date of the original review: 2026-11-20. Project: `/home/editry/development/sise-editor`.

## What the owner has decided

ADR-001 is **ACCEPTED WITH CONDITIONS**, including the five-library FFmpeg link. GStreamer is not a mandatory dependency. ADR-003, ADR-005, and the prototype undo rule are accepted. ADR-008 stays open. The paragraph below that called C++, Qt, CMake, and SQLite proposals is superseded by that package.

## Documents inspected

Preparation set:

- `docs/AETHER_PREBUILD_READINESS_REPORT.md`
- `docs/ARCHITECTURE_AUDIT.md`
- `docs/SYSTEM_INTEGRATION_AND_CONSISTENCY.md`
- `docs/ENGINEERING_INTERFACES.md`
- `docs/FIRST_IMPLEMENTATION_MILESTONE.md`
- `docs/ARCHITECTURE_RISK_REGISTER.md`
- `architecture/decisions/ADR-001-technology-stack.md`
- `architecture/decisions/ADR-003-project-database.md`
- `architecture/decisions/ADR-005-media-identity.md`
- `architecture/decisions/ADR-008-licensing-and-originals.md`
- `architecture/decisions/ADR-009-undo-durability.md`

Owners this review follows, and does not rewrite:

| Subject | Documents |
| --- | --- |
| Bins are not folders | `docs/BIN_ARCHITECTURE.md`, `docs/BIN_DATA_MODEL.md`, `docs/BIN_DATABASE_SCHEMA.md`, `docs/BIN_FILE_FORMAT_SPECIFICATION.md` |
| Media registry and identity | `docs/MEDIA_IDENTITY_SPECIFICATION.md`, `docs/MEDIA_REGISTRY_SCHEMA.md`, `docs/MEDIA_IDENTITY_AND_METADATA.md`, `docs/MEDIA_IDENTITY_AND_BIN_RELATIONSHIP.md` |
| Relink distinct from conform | `docs/MEDIA_RELINK_ENGINE.md`, `docs/CONFORM_ENGINE_SPECIFICATION.md`, `docs/CONFORM_VALIDATION_AND_REPORTING.md`, `docs/ONLINE_CONFORM_ARCHITECTURE.md` |
| Timeline and rational time | `docs/TIMELINE_ENGINE_ARCHITECTURE.md`, `docs/TIMEBASE_AND_FRAME_ACCURACY.md`, `docs/05_TIMEBASE_AND_TIMING_ARCHITECTURE.md` |
| Audio derivation | `docs/AUDIO_TIMEBASE_AND_SYNC.md`, `docs/AUDIO_ENGINE_ARCHITECTURE.md` |
| MXF and the prior FFmpeg study | `docs/NATIVE_MEDIA_ARCHITECTURE.md`, `docs/MXF_CONTAINER_SPECIFICATION.md` |
| Exchange documents, not results | `docs/AVX_FORMAT_SPECIFICATION.md`, `docs/AAF_AUDIO_EXCHANGE_PROFILE.md`, `docs/AAF_MEDIA_REFERENCE_PROFILE.md`, `docs/EXCHANGE_COMPATIBILITY_MATRIX.md` |
| Playback | `docs/10_PLAYBACK_AND_RENDERING.md` |
| Recovery | `docs/RECOVERY_AND_BACKUP.md`, `docs/15_UNDO_RECOVERY_AND_SAFETY.md`, `docs/BIN_REVISION_AND_RECOVERY.md` |
| Licence posture | `docs/DEPENDENCY_AND_LICENCE_AUDIT.md`, `docs/16_SECURITY_AND_LICENSING.md` |

`docs/NATIVE_MEDIA_ARCHITECTURE.md` is the FFmpeg behaviour specification this review cross-references. The 19 November 2026 study inside it was not re-certified as a whole. This review ran a narrower numbered-frame measurement and cites that file for the profile target and the earlier warnings.

## Rules this review preserves

These stay proposals until Subhajit accepts them. This review does not promote them.

- A bin is an editorial object. A directory is a storage folder.
- Media ID is minted once. It is not a path, a filename, a checksum, or a timecode.
- Original bytes are never silently modified.
- Relink updates a location. Relink is not a conform.
- An export is a file and a report. It is not a verified conform.
- Stored time is a reduced rational and integer positions. Floating-point seconds are display text. They are not the authority for the cut, the undo stack, or a render request.

## Recommendations, not acceptances

| Topic | Recommendation | Label |
| --- | --- | --- |
| FFmpeg as the core media framework | Required, as the owner confirmed. GStreamer is not a candidate for that role | Confirmed requirement. Integration is **subject to prototype** |
| Libraries behind Processing | `libavutil`, `libavcodec`, `libavformat`, `libswscale`, `libswresample`, dynamically linked, types private to `aether_process` | **subject to prototype** |
| `libavfilter`, `libavdevice`, the `ffmpeg` CLI as the runtime | Filter graphs and device clocks stay out of the editorial model. The CLI is a test tool | **not recommended** as the core runtime |
| C++17 or newer, CMake, SQLite `project.db` | Leading implementation choices | **Recommended for adoption**. Not accepted |
| Qt 6 Widgets | Leading UI toolkit, after counsel on the modules actually linked | **subject to prototype** |
| One Media ID, representations by `proxy_id` and location id | Keep ADR-005’s working rule | **Recommended for adoption**. Not accepted |
| `project.db` plus manifest, one schema series | Retire opaque `project.sqlite` blobs as the canonical file when he agrees | **Recommended for adoption**. Not accepted |
| Session undo only for the Foundation Prototype | Undo, autosave, crash recovery, snapshots, and journals stay different stores | **Recommended for adoption** for the prototype. Restart-undo is **further research** |
| Project licence, codec patents, trademarks, Qt, FFmpeg redistribution | Counsel. This review selects nothing | **further research**. Legal |
| Shipping this machine’s Fedora FFmpeg | GPL build, extra codecs, no development headers | **not recommended** |
| Offering MXF writers because a muxer name exists | Capability is a tested tuple, not a muxer name | **not recommended** |

Detail is in `docs/technology/` and `docs/validation/`. The questions he has to answer are in `docs/PREBUILD_DECISION_PACKAGE.md`. The prototype word is in `docs/FOUNDATION_PROTOTYPE_READINESS_GATE.md`.

## What was run, and what was not

A synthetic numbered-frame round trip was run with `/usr/bin/ffmpeg` 9.0.2 on this machine. Files live only under `/home/editry/development/sise-editor/validation-fixtures/ffmpeg-mxf-numbered-2026-11-20/`. No user media was overwritten. The procedure and the measurements are `docs/validation/FFMPEG_MXF_ROUNDTRIP_TEST.md`.

CLI success is not library integration. `ffmpeg-devel` is not installed, `/usr/include/ffmpeg` is absent, and `src/` is empty. That gap is `docs/validation/FFMPEG_INTEGRATION_VALIDATION.md`, result **Not tested**.

DaVinci Resolve, Avid Media Composer, and Adobe Premiere Pro are not installed. The external relink and conform procedure is written and **Not tested**. No conform result was invented.

No database was created. No crash was induced. No package was installed. Nothing was committed.

## Readiness

**NOT READY.** The word is argued in `docs/FOUNDATION_PROTOTYPE_READINESS_GATE.md`. This review cannot grant approval.
