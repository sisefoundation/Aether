# AETHER architecture audit

Status: architecture preparation. This audit authorises no implementation, no dependency, and no compatibility claim.

Labels used here and in the documents this audit introduces:

- **CONFIRMED** — observed in `/home/editry/development/sise-editor` on this pass, or a negative fact the existing documents already record (nothing was run, nothing was linked, no vendor file was exchanged).
- **PROPOSED** — a design written in the existing set, or a reconciliation restated by this preparation. A proposal is not an approval. This audit does not promote one.
- **OPEN** — contradictory, unmeasured, or waiting on a decision, including every item that needs legal review.

## Headline

The repository is a large specification set and an empty source scaffold. There is no editor and no product build. On 2026-11-20 the owner accepted the technology direction with conditions, the project database, the Media ID rule, and session-only undo for the Foundation Prototype. Licensing stays open. The 29.97 audio contract is a recorded failure. FFmpeg library integration is blocked. The stack is not production-validated. The Foundation Prototype is not started by this audit. The gate word is **CONDITIONALLY READY** in `docs/FOUNDATION_PROTOTYPE_READINESS_GATE.md`.

## 1. How this audit was made

**CONFIRMED.** The tree at `/home/editry/development/sise-editor` was listed and the markdown set was read, including the root architecture trio, the empty stubs under `docs/`, and the later AETHER specifications for bins, timeline, audio, native media, offline/online, and exchange.

**CONFIRMED.** This pass did not compile anything, did not run FFmpeg, did not open a database, did not hash a camera file, and did not import a package into DaVinci Resolve, Avid Media Composer, Adobe Premiere Pro, or Avid Pro Tools. Prior notes about FFmpeg 9.0.2 are a local study already written in the media documents. They are reconciled below as engineering warnings. They are not a new test and they are not a validation pass.

**CONFIRMED.** The directory is not a git repository. `LICENSE` is an empty file. No licence was chosen by filling it.

## 2. Repository inventory

### 2.1 What is on disk

| Path | State | Label |
| --- | --- | --- |
| `README.md` | Empty file at the start of this preparation | **CONFIRMED** |
| `LICENSE` | Empty file. Not rewritten | **CONFIRMED** |
| `CMakeLists.txt` | Empty file. No targets, no languages, no dependencies | **CONFIRMED** |
| `ARCHITECTURE.md`, `MODULES.md`, `DEVELOPMENT_ROADMAP.md` | Substantive early proposal, status “awaiting review” | **PROPOSED** |
| `docs/*.md` substantive set | About seventy specifications. Each says proposal or “not an audit”. None records an executed AETHER test | **PROPOSED** |
| `docs/ARCHITECTURE.md`, `docs/DEVELOPMENT_ROADMAP.md`, `docs/MODULES.md`, `docs/MEDIA_PIPELINE.md`, `docs/PROJECT_FORMAT.md` | Zero-byte stubs beside the real documents | **CONFIRMED** empty. Left untouched |
| `src/**`, `tests/`, `plugins/`, `resources/`, `third_party/` | Directories only. No source, no fixture, no vendored library | **CONFIRMED** scaffold. Preserved |
| Application behaviour | Absent | **CONFIRMED** |

The empty `src/` layout (`application`, `core`, `database`, `engines`, `services`, `ui`) does not match the module names in `docs/SOFTWARE_MODULES.md` (`aether_editorial`, `aether_media`, `aether_sound`, and the rest). It is a directory sketch. It is not an implementation and it is not the dependency matrix.

### 2.2 Documentation that is substantively written

These clusters are the contracts. New numbered documents point at them and do not restate their tables.

| Cluster | Authority documents | What they already decide, as proposals |
| --- | --- | --- |
| Product | `docs/PRODUCT_VISION.md`, `docs/EDITORIAL_PHILOSOPHY.md`, `docs/UI_ARCHITECTURE.md`, `docs/UI_UX_DESIGN_SYSTEM.md` | Offline-first feature editor for SISE. AETHER is the product name. SISE Editor is the name used by the early trio. |
| Early architecture | `ARCHITECTURE.md`, `MODULES.md`, `DEVELOPMENT_ROADMAP.md` | Layers, `CompositionSlice` / `MediaLocation` / `RenderRequest`, phases 0–16. Stack sentence says “approved” while the status line says the proposal is awaiting review. |
| System and modules | `docs/SYSTEM_ARCHITECTURE.md`, `docs/SOFTWARE_MODULES.md`, `docs/PROJECT_ARCHITECTURE.md` | Seven layers. Candidates are not approvals. Application is the joiner. |
| Bins | `docs/BIN_ARCHITECTURE.md` and the twelve companions (`BIN_DATA_MODEL.md`, `BIN_DATABASE_SCHEMA.md`, `BIN_FILE_FORMAT_SPECIFICATION.md`, lifecycle, portability, import/export, dependency graph, revision, UI, test plan, media relationship, sequence integration) | A bin is an editorial object. A directory is a storage folder. `.abin` is a versioned ZIP of public UTF-8 JSON. SQLite is the working project database. |
| Media identity | `docs/MEDIA_IDENTITY_SPECIFICATION.md`, `docs/MEDIA_IDENTITY_AND_METADATA.md`, `docs/MEDIA_IDENTITY_AND_BIN_RELATIONSHIP.md`, `docs/MEDIA_REGISTRY_SCHEMA.md`, `docs/MEDIA_FINGERPRINTING_AND_MATCHING.md`, `docs/MEDIA_INGESTION_SPECIFICATION.md`, `docs/CAMERA_MEDIA_INGEST.md`, `docs/MEDIA_MANAGEMENT_PHILOSOPHY.md`, `docs/MEDIA_STORAGE_AND_RELINKING.md`, `docs/MEDIA_RELINK_ENGINE.md` | Media ID survives path changes. Relink updates locations. |
| Timeline | `docs/TIMELINE_ENGINE_ARCHITECTURE.md`, `docs/EDITORIAL_OPERATIONS.md`, `docs/TRIM_ENGINE_SPECIFICATION.md`, `docs/TIMEBASE_AND_FRAME_ACCURACY.md`, `docs/TIMELINE_UI_SPECIFICATION.md`, `docs/TIMELINE_PERFORMANCE.md`, `docs/SEQUENCE_SOURCE_MAPPING.md`, `docs/SOURCE_TIMECODE_AND_FRAME_ACCURACY.md`, `docs/SEQUENCE_AND_BIN_INTEGRATION.md` | Integer frames, half-open ranges, instance id distinct from Media ID. |
| Audio | `docs/AUDIO_ENGINE_ARCHITECTURE.md`, `docs/AUDIO_EDITING_PHILOSOPHY.md`, `docs/AUDIO_TIMEBASE_AND_SYNC.md`, `docs/AUDIO_PROCESSING_GRAPH.md`, `docs/AUDIO_AUTOMATION_SPECIFICATION.md`, `docs/AUDIO_INTERACTION_MODEL.md`, `docs/AUDIO_TIMELINE_UI_SPECIFICATION.md`, `docs/AUDIO_PERFORMANCE.md` | Non-destructive sound. `sampleIndex` is derived. Subframe slip is outside the first model. |
| Picture files | `docs/NATIVE_MEDIA_ARCHITECTURE.md`, `docs/MXF_CONTAINER_SPECIFICATION.md`, `docs/PRORES_PROFILE_SPECIFICATION.md`, `docs/DNXHR_PROFILE_SPECIFICATION.md`, `docs/MEDIA_INTEROPERABILITY_TESTS.md`, `docs/AVID_MEDIA_COMPATIBILITY.md` | Profiles are specified. Availability waits on a validation suite that has not been run. |
| Offline / online | `docs/OFFLINE_EDITING_ARCHITECTURE.md`, `docs/ONLINE_CONFORM_ARCHITECTURE.md`, `docs/CONFORM_ENGINE_SPECIFICATION.md`, `docs/CONFORM_VALIDATION_AND_REPORTING.md`, `docs/PROXY_AND_ORIGINAL_MEDIA_RELATIONSHIP.md`, `docs/PROXY_AND_TRANSCODE_SYSTEM.md`, `docs/MEDIA_CONSOLIDATION_SPECIFICATION.md`, `docs/OFFLINE_ONLINE_INTEROPERABILITY_TEST_PLAN.md` | Relink is not a conform verdict. Export is not a verified conform. |
| Exchange | `docs/INTERCHANGE_ADAPTER_ARCHITECTURE.md`, `docs/AVX_FORMAT_SPECIFICATION.md`, `docs/AVX_OFFLINE_ONLINE_EXCHANGE.md`, `docs/AAF_AUDIO_EXCHANGE_PROFILE.md`, `docs/AAF_MEDIA_REFERENCE_PROFILE.md`, `docs/EXTERNAL_APPLICATION_COMPATIBILITY.md` | AVX and the AAF profile are AETHER documents. No second implementation and no vendor import was executed. |
| Project, safety, recovery | `docs/PROJECT_FOLDER_SPECIFICATION.md`, `docs/PROJECT_DATABASE_SCHEMA.md`, `docs/SECURITY_AND_FILE_SAFETY.md`, `docs/RECOVERY_AND_BACKUP.md`, `docs/DEPENDENCY_AND_LICENCE_AUDIT.md`, `docs/PERFORMANCE_REQUIREMENTS.md` | Folder tree, an earlier logical schema, safety rules, recovery proposal, an audit checklist that has not been performed. |

### 2.3 What is partial

- Two architectures describe one product and were not merged.
- Two logical database schemas describe one `project.db` (`docs/PROJECT_DATABASE_SCHEMA.md` and `docs/BIN_DATABASE_SCHEMA.md`).
- Identity is named asset id, media id, origin id, proxy id, and location id in different documents. The editorial handle is stable in every one of them. The column that owns it is not unified.
- Identity names were asset id, media id, origin id, and proxy id. ADR-005 closes that: one Media ID, and a Representation ID for a proxy. The bin architecture identity row was corrected. Schema column names in older tables still need one migration series. That is implementation, not an open identity decision.
- The early roadmap runs Phase 0 through Phase 16 and stops before code. The later documents add bins, conform, and AVX without renumbering that roadmap. `docs/19_DEVELOPMENT_ROADMAP.md` restates the work as seven phases. It does not retire the earlier file.
- Colour, effects, plugins, playback, and a real build are specified at the boundary and are not present as code.
- `docs/DEPENDENCY_AND_LICENCE_AUDIT.md` is explicitly not an audit.

### 2.4 What is missing

- A recorded review that accepts or revises section 11 of `ARCHITECTURE.md` and the unresolved list in `docs/SYSTEM_ARCHITECTURE.md`.
- A project licence in `LICENSE`.
- A CMake build that configures, and any test binary.
- A single glossary that maps asset id to Media ID with one surviving key.
- A merged schema migration. Neither schema was executed.
- Validation evidence for MXF, ProRes, DNxHR / VC-3, OP-Atom, AAF, OTIO, FCP XML, FCPXML, and EDL.
- Legal review of Qt, FFmpeg build flags, codec patents and trademarks, and any AAF implementation route.
- The numbered preparation documents listed in section 6 of this audit. They are added by this preparation where they were absent. They do not replace the clusters above.

### 2.5 What is duplicated

Duplication is preserved. The empty stubs were not filled, because another task may own them, and filling a zero-byte `docs/ARCHITECTURE.md` would hide the fact that the real text lives at the repository root.

| Topic | Copies | What to read |
| --- | --- | --- |
| Architecture narrative | `ARCHITECTURE.md` and `docs/SYSTEM_ARCHITECTURE.md` | Both. They disagree on approval language and on whether audio and interchange are separate layers. |
| Modules | `MODULES.md` and `docs/SOFTWARE_MODULES.md` | Both. `docs/ENGINEERING_INTERFACES.md` maps them. |
| Roadmap | `DEVELOPMENT_ROADMAP.md`, empty `docs/DEVELOPMENT_ROADMAP.md`, and the new seven-phase file | The root file is the earlier phase list. The new file is the preparation roadmap. The empty stub stays empty. |
| Project database | `docs/PROJECT_DATABASE_SCHEMA.md`, `docs/BIN_DATABASE_SCHEMA.md`, `docs/MEDIA_REGISTRY_SCHEMA.md` | Bin schema and registry are the later working-store proposals. The earlier schema is not deleted. |
| Time | `ARCHITECTURE.md` section 3, `docs/TIMEBASE_AND_FRAME_ACCURACY.md`, `docs/AUDIO_TIMEBASE_AND_SYNC.md`, `docs/SOURCE_TIMECODE_AND_FRAME_ACCURACY.md` | The timebase document is the picture and editorial expansion. Audio derives samples from it. |
| Media files | Root processing story versus `docs/NATIVE_MEDIA_ARCHITECTURE.md` | Native media is a profile proposal on top of a candidate backend, not a completed writer. |

## 3. Contradictions

These are not resolved by this audit. Each row names the documents and the label of the conflict.

| # | Conflict | Documents | Label |
| --- | --- | --- | --- |
| C1 | Early text called a mixed stack including GStreamer “approved” while another file called every name a candidate | `ARCHITECTURE.md` opening; `docs/SYSTEM_ARCHITECTURE.md` | **CLOSED** for the direction. ADR-001 is **ACCEPTED WITH CONDITIONS**: C++, Qt 6, CMake, SQLite, FFmpeg mandatory, GStreamer not mandatory. Not production-validated. Licensing stays open in ADR-008 |
| C2 | Package is `Title.sise/project.sqlite` with opaque blobs versus `project.db` plus a manifest | `ARCHITECTURE.md` §5; `docs/PROJECT_STORAGE_AND_DATABASE.md` | **CLOSED.** ADR-003 accepts `project.db` plus a manifest. `.abin` is interchange. Folder-layout details that the folder spec does not list remain a documentation tidy-up, not a second database |
| C3 | Editorial handle called asset id, with a separate origin id, versus Media ID | `docs/MEDIA_IDENTITY_AND_METADATA.md`; `docs/04_CORE_DATA_MODEL.md` | **CLOSED.** ADR-005. Asset id means Media ID. Origin id is not a second editorial identity |
| C4 | Proxy is another media id versus a representation that cites the original Media ID | `docs/BIN_ARCHITECTURE.md`; `docs/PROXY_AND_ORIGINAL_MEDIA_RELATIONSHIP.md` | **CLOSED.** ADR-005. The bin architecture identity row was corrected. A proxy has a Representation ID, not a second Media ID |
| C5 | Undo is session state versus undo rows stored for crash recovery | `ARCHITECTURE.md` §5.4; `docs/BIN_DATABASE_SCHEMA.md`; `docs/RECOVERY_AND_BACKUP.md` | **CLOSED** for the prototype. ADR-009: session-only undo. Journals, autosave, snapshots, and crash recovery stay separate. Persistent undo is future |
| C6 | Sound is a mapping inside Editorial plus a GStreamer path inside Processing versus a separate AETHER SOUND module | `ARCHITECTURE.md` §3.3 and §4; `docs/SYSTEM_ARCHITECTURE.md` layer 5; `docs/AUDIO_ENGINE_ARCHITECTURE.md` | **PROPOSED** separate module in the later set. The early file is not withdrawn. |
| C7 | OpenTimelineIO is the built-in professional interchange versus AVX for picture and an AAF profile for audio, with OTIO as one adapter | `ARCHITECTURE.md` §1.5 and §9.5; `docs/INTERCHANGE_ADAPTER_ARCHITECTURE.md` | **OPEN** which adapter is first after the native model exists. None is the live database. |
| C8 | Bin design states that the assignment fixes C++ and Qt 6 for that subsystem, while the system architecture still lists both as unresolved and the licence audit has not cleared Qt | `docs/BIN_ARCHITECTURE.md` verified facts; `docs/SYSTEM_ARCHITECTURE.md` unresolved 1–2; `docs/DEPENDENCY_AND_LICENCE_AUDIT.md` | **OPEN**. A subsystem assignment does not clear a licence. |
| C9 | Native-media text says Processing uses FFmpeg, while an earlier system-architecture list kept GStreamer as an equal candidate | `docs/NATIVE_MEDIA_ARCHITECTURE.md`; `docs/SYSTEM_ARCHITECTURE.md` | **CLOSED** for the framework choice. FFmpeg is mandatory. Profiles stay unvalidated where the suite has not passed. The 30000/1001 audio contract is **Failed**. Library integration is **BLOCKED** |
| C10 | `docs/SOFTWARE_MODULES.md` says no source tree was available. This tree now has empty directories | That document’s verified facts, versus the directories listed in §2.1 | **CONFIRMED** the directories exist and are empty. The module document’s historical statement is stale. The directories were not filled. |
| C11 | `docs/PRODUCT_VISION.md` says this Fedora folder was not read. This audit read it | Same | **CONFIRMED** the folder has now been read. The vision text was not edited to hide that history. |
| C12 | Early model stores colour decisions for OpenColorIO and copies a config into the package. The system architecture leaves OpenColorIO adoption open and recommends tags plus a display hook for a first prototype | `ARCHITECTURE.md` §5.3 and §11.9; `docs/SYSTEM_ARCHITECTURE.md` unresolved 12 | **OPEN** how much colour is in the first code. |

Items that look like contradictions and are not:

- Relink and conform are specified as different operations on purpose. See `docs/MEDIA_RELINK_ENGINE.md` and `docs/CONFORM_ENGINE_SPECIFICATION.md`.
- A directory and a bin are specified as different things on purpose. See `docs/BIN_ARCHITECTURE.md` requirement 1.
- Drop-frame timecode changes printed numbers and does not skip frame indexes. The timebase document and the early architecture agree. That agreement is still **PROPOSED**, not a measured implementation.

## 4. Invariants the preparation keeps

The numbered documents agree with the contracts below. Agreement means the new text does not invent a second rule. It does not mean Subhajit has approved the rule.

| Invariant | Label | Where it is already written |
| --- | --- | --- |
| A bin is an editorial object with a bin id and membership. A directory is a storage folder. Creating one does not create the other. | **PROPOSED** | `docs/BIN_ARCHITECTURE.md` |
| Media ID is minted once and survives rename, move, relink, proxy regeneration, and project copy. It is not a path, a filename, a checksum, or a timecode. | **PROPOSED** | `docs/MEDIA_IDENTITY_SPECIFICATION.md` |
| A timeline instance cites an object id and a Media ID. It does not mint media identity. | **PROPOSED** | `docs/BIN_ARCHITECTURE.md`, `docs/SEQUENCE_AND_BIN_INTEGRATION.md`, `docs/TIMELINE_ENGINE_ARCHITECTURE.md` |
| `.abin` is a versioned ZIP of public UTF-8 JSON members (format version 1, ZIP method 0). It is not a dump of the project tables and it is not a SQLite file. | **PROPOSED** | `docs/BIN_FILE_FORMAT_SPECIFICATION.md` |
| SQLite is the working project database (`project.db`), with a manifest cross-check. PostgreSQL is out of scope. | **PROPOSED** | `docs/BIN_DATABASE_SCHEMA.md`, `docs/PROJECT_DATABASE_SCHEMA.md`, `docs/SYSTEM_ARCHITECTURE.md` unresolved 7 |
| Relink updates location rows for a known Media ID. It does not edit the cut and it is not a conform state. | **PROPOSED** | `docs/MEDIA_RELINK_ENGINE.md`, `docs/CONFORM_ENGINE_SPECIFICATION.md` |
| An export writes a file and a report. That report is not a verified conform. | **PROPOSED** | `docs/ONLINE_CONFORM_ARCHITECTURE.md`, `docs/CONFORM_VALIDATION_AND_REPORTING.md` |
| Stored time is a reduced rational and integer positions. Floating-point seconds are a display formatting. They are not the authority for the cut, the undo stack, or a render request. | **PROPOSED** | `ARCHITECTURE.md` §3, `docs/TIMEBASE_AND_FRAME_ACCURACY.md` |
| Original camera and sound bytes are never silently modified, moved, renamed, or deleted. | **PROPOSED** | `docs/SECURITY_AND_FILE_SAFETY.md`, `docs/MEDIA_IDENTITY_SPECIFICATION.md` |
| The application is the only joiner of an editorial slice and a media location. Domain modules do not call each other to help. | **PROPOSED** | `ARCHITECTURE.md` §2.2, `docs/SYSTEM_ARCHITECTURE.md` |
| No network, account, or activation is required to open and cut. | **PROPOSED** | `docs/PRODUCT_VISION.md` |
| Vendor compatibility is untested. Naming Resolve, Media Composer, Premiere Pro, or Pro Tools is not a result. | **CONFIRMED** absent | Every exchange and media document that states no application was tested |

## 5. FFmpeg 9.0.2 study, reconciled as warnings

**CONFIRMED** as a citation, not as a result of this audit. The media and licence documents already record a prior local study of FFmpeg 9.0.2. This preparation did not re-run it. The study is not an AETHER ingest, not an interoperability test, and not a licence finding.

The notes those documents tell a later validation suite to reconcile:

- OP1a ProRes and DNxHR profiles muxed in that study.
- PCM was reported only at 48 kHz, 24-bit, little-endian.
- Channel layout was not stored in MXF.
- The ProRes 4444 alpha descriptor was incomplete.
- A 29.97 audio sample-count mismatch was reported.
- OP-Atom was optional only in that build.

**PROPOSED** consequence, already written and kept: a capability query may not offer those writers as available until AETHER’s own suite passes on a pinned backend. Until then the profiles stay specified and `not_validated`. The backend for that suite is FFmpeg (ADR-001). The 2026-11-20 CLI did re-run part of the study: 24 fps OP1a **Passed**, 30000/1001 audio **Failed**, OP-Atom stereo **Not supported**, library link **BLOCKED**. Those results are not deleted from the media documents.

## 6. Documents this preparation adds

Added only where the path was missing. `README.md` was an empty stub and is updated. `LICENSE` is not rewritten. Empty stubs under `docs/` that this task does not own are not filled.

| Document | Role |
| --- | --- |
| `docs/ARCHITECTURE_AUDIT.md` | This inventory |
| `docs/01_PRODUCT_REQUIREMENTS.md` | Requirements traced to existing specs |
| `docs/03_TECHNOLOGY_DECISIONS.md` | Candidates, with legal items flagged |
| `architecture/decisions/ADR-001-technology-stack.md` and further ADRs | ADR-001 **ACCEPTED WITH CONDITIONS**. ADR-003 and ADR-005 **ACCEPTED**. ADR-004 accepted as interchange. ADR-009 **ACCEPTED FOR FOUNDATION PROTOTYPE**. ADR-008 **OPEN**. Not every ADR in the directory was part of this decision |
| `docs/04_CORE_DATA_MODEL.md` | Identity map. Points at the bin and registry specs |
| `docs/05_TIMEBASE_AND_TIMING_ARCHITECTURE.md` | Points at the timebase and audio timebase specs |
| `docs/10_PLAYBACK_AND_RENDERING.md` | Picture and monitoring seam |
| `docs/11_EFFECTS_AND_COMPOSITING.md` | Effects as data |
| `docs/12_COLOUR_MANAGEMENT.md` | Tags now, display transform later |
| `docs/14_PLUGIN_ARCHITECTURE.md` | Points at the early plugin tiers and keeps them unapproved |
| `docs/15_UNDO_RECOVERY_AND_SAFETY.md` | Points at recovery and bin revision |
| `docs/PROJECT_STORAGE_AND_DATABASE.md` | Points at both schemas and the `.abin` split |
| `docs/EXCHANGE_COMPATIBILITY_MATRIX.md` | Specified versus executed |
| `docs/16_SECURITY_AND_LICENSING.md` | Safety plus the legal flags |
| `docs/17_TESTING_AND_VALIDATION.md` | What would count as evidence |
| `docs/18_BUILD_AND_RELEASE.md` | Empty CMake, no release |
| `docs/19_DEVELOPMENT_ROADMAP.md` | Phases 1–7 |
| `docs/SYSTEM_INTEGRATION_AND_CONSISTENCY.md` | How the set must stay one product |
| `docs/ENGINEERING_INTERFACES.md` | Joins and forbidden edges |
| `docs/PERFORMANCE_AND_SCALABILITY.md` | Points at budgets that are not measurements |
| `docs/FIRST_IMPLEMENTATION_MILESTONE.md` | Foundation Prototype |
| `docs/ARCHITECTURE_RISK_REGISTER.md` | Risks |
| `docs/AETHER_PREBUILD_READINESS_REPORT.md` | Sections A–I and the recommendation |
| `CONTRIBUTING.md` | Short contribution pointer |
| `README.md` | Overview, index, status, limits |

Numbers 02, 06, 07, 08, 09, and 13 are intentionally absent. Those subjects already have substantive specifications (vision and editorial philosophy, timeline and trim, audio, media and bins, interchange). Creating a second copy would duplicate them.

## 7. Decisions that need Subhajit

A decision is needed where the documents disagree or where a licence is involved. Silence is not an approval, which `DEVELOPMENT_ROADMAP.md` already says for Phase 0.

1. Whether C++ (17 or newer) and Qt 6 Widgets are the implementation choices, after the Qt licence review.
2. Whether the canonical project is the later `project.db` plus manifest and folder tree, with `.abin` checkpoints, and the earlier `Title.sise/project.sqlite` opaque-blob package is retired.
3. The single glossary for Media ID, asset id, origin id, `proxy_id`, and location id (conflicts C3 and C4).
4. Whether undo history must survive a restart (conflict C5).
5. Whether AETHER SOUND is a separate module in the first build (conflict C6).
6. Which interchange adapter, if any, follows the native model first (conflict C7). None may become the database.
7. The project licence, and whether patent-encumbered encoders or an AAF SDK may be distributed. Counsel. Not an engineering choice.
8. Whether the Foundation Prototype’s acceptance includes a `.abin` round-trip or only the in-database bin and sequence. The milestone document includes the round-trip as **PROPOSED** acceptance. He can narrow it.
9. How much colour is in the first code (conflict C12). The milestone excludes colour baking.

## 8. What this audit refuses to say

- It does not say the stack is approved.
- It does not say a test passed.
- It does not say a file will open in Resolve, Media Composer, Premiere Pro, or Pro Tools.
- It does not say FFmpeg 9.0.2 is the backend AETHER will ship.
- It does not start the NLE.

The readiness answer is in `docs/AETHER_PREBUILD_READINESS_REPORT.md`. The Foundation Prototype may not start until the review named in the headline.
