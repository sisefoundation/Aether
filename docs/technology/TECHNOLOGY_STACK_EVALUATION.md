# Technology stack evaluation

Status: evaluation that led to ADR-001. The ADR is now **ACCEPTED WITH CONDITIONS**. This file is not a second decision and it does not approve redistribution. Where a row below still says “recommended”, read the ADR.

Date: 2026-11-20. Decision record: `architecture/decisions/ADR-001-technology-stack.md`. Licence record: `architecture/decisions/ADR-008-licensing-and-originals.md`.

Labels used here: **Recommended for adoption**, **subject to prototype**, **further research**, **not recommended**. “Confirmed” is used only for the owner’s FFmpeg requirement. Confirmed is not a synonym for accepted.

## 1. How to read the early “approved stack” sentence

`ARCHITECTURE.md` names C++17, Qt 6, CMake, FFmpeg, GStreamer, OpenTimelineIO, SQLite, and OpenColorIO and once calls that list approved. The same file is a proposal awaiting review. `docs/SYSTEM_ARCHITECTURE.md` and `docs/DEPENDENCY_AND_LICENCE_AUDIT.md` treat the names as candidates. Audit conflict C1 stays open for every row except the two the owner has now closed: FFmpeg is required, and GStreamer is not a core-framework candidate.

`CMakeLists.txt` is empty. `src/` has no files. Nothing is linked.

## 2. Evaluation

| Piece | Role in the documents | This review | Label |
| --- | --- | --- | --- |
| FFmpeg as the core media framework | Processing probes and writes. The behaviour spec is `docs/NATIVE_MEDIA_ARCHITECTURE.md` | Owner confirmed, 2026-11-20. Which libraries, the link shape, the build, and which codec tuples are valid remain open | Requirement confirmed. Shape **subject to prototype** |
| GStreamer | Early alternative or combined playback graph. Conflict C6 folds interactive audio into a GStreamer path | Not selected. Not a candidate for decode, encode, containers, or the playback clock | **not recommended** for the core framework |
| C++17 or newer | Domain and infrastructure language. Public headers carry no third-party types | Matches the module rules in `docs/ENGINEERING_INTERFACES.md`. No other language is proposed | **Recommended for adoption**. Not accepted |
| CMake | Build system. The root file is empty | Introduce it only when implementation is allowed. The empty file is not a build | **Recommended for adoption**. Not accepted |
| Qt 6 Widgets | UI toolkit, confined to UI. QML is not in the proposal | Leading candidate. LGPL-or-commercial review of the modules actually linked has not been done | **subject to prototype** |
| SQLite | Working project database | Leading engine for `project.db` only. Not the `.abin` package. Not measured. See the database evaluation | **Recommended for adoption**. Not accepted |
| PostgreSQL | Shared server | Existing documents put it out of scope | **not recommended** |
| OpenColorIO | Display and bake | First-code scope is open in `docs/12_COLOUR_MANAGEMENT.md`. The milestone excludes baking | **further research** |
| OpenTimelineIO | One interchange adapter | Must not become the project database. Which adapter is first is audit C7 | **further research** |
| DeckLink SDK | Optional monitoring | Proprietary. Default remains off. That default is not a clearance | **not recommended** in the default build |
| AAF SDK | Audio exchange | Several codebases, licences not interchangeable. A project-written writer would still need a licence | **further research** |
| AVX | Picture exchange document | `docs/AVX_FORMAT_SPECIFICATION.md` is a specification. No vendor test was run | **further research** |

## 3. What “FFmpeg is required” does not decide

- It does not accept ADR-001.
- It does not choose C++, Qt, CMake, or SQLite.
- It does not choose `libavfilter` or `libavdevice`.
- It does not make the `ffmpeg` command-line program the runtime. The CLI on this machine is the measurement tool in `docs/validation/FFMPEG_MXF_ROUNDTRIP_TEST.md`.
- It does not clear a GPL build, `--enable-nonfree`, `libfdk-aac`, or a patent-bearing encoder.
- It does not validate a codec, a container pattern, or a vendor relink.
- It does not authorise the Foundation Prototype. That prototype is specified not to link FFmpeg (`docs/FIRST_IMPLEMENTATION_MILESTONE.md`).

The integration recommendation is `docs/technology/FFMPEG_INTEGRATION_ARCHITECTURE.md`. The build recommendation is `docs/technology/FFMPEG_BUILD_AND_DISTRIBUTION.md`.

## 4. Module boundary

**Recommended for adoption**, still **subject to prototype** until the link line exists:

- Editorial, bins, and the timeline link neither Qt nor FFmpeg (`docs/ENGINEERING_INTERFACES.md`).
- `aether_process` is the only module that may include FFmpeg headers, after a build is allowed.
- `aether_ui` is the only module that may include Qt headers, after the Qt review.
- `aether_db` is the only module that may include SQLite.
- The application is the only joiner (ADR-006, still **PROPOSED**).

A rejected library has to leave without changing trim mathematics or Media IDs. That rule is why the editorial model keeps rationals of its own and does not store `AVRational` or floating-point seconds.

## 5. Sound

AETHER SOUND as its own library remains audit conflict C6. The owner’s decision removes the GStreamer path as the core framework. It does not, by itself, decide whether sound is a separate link target. `docs/AUDIO_ENGINE_ARCHITECTURE.md` stays the owner. Interactive audio does not take its clock from a media framework.

## 6. Acceptance criteria this evaluation does not meet

- ADR-001 is still **PROPOSED**.
- No licence row in `docs/DEPENDENCY_AND_LICENCE_AUDIT.md` has moved from “not started” to a counsel note.
- No CMake target exists whose link line can be inspected.
