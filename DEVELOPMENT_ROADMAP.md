# SISE Editor Development Roadmap

Status: previously proposed stages, kept below. Slice 0A is a later and narrower milestone. It is not a phase in this list.

**SLICE 1I COMPLETE — LIFT EDITING IMPLEMENTED AND VALIDATED.**

**SLICE 1H COMPLETE — SPLIT EDITING IMPLEMENTED AND VALIDATED.**

**SLICE 1G COMPLETE — SAME-RATE OVERWRITE IMPLEMENTED AND VALIDATED.**

**SLICE 1F COMPLETE — SOURCE AND RECORD MARKS IMPLEMENTED AND VALIDATED.**

**SLICE 1E COMPLETE — FFMPEG 9.1 DEVELOPMENT-ONLY DYNAMIC INTEGRATION AND MEDIA METADATA PROBING VALIDATED — DISTRIBUTION LEGAL REVIEW PENDING.**

Slice 0A, Slice 0B, Slice 1A, Slice 1B, Slice 1C, and Slice 1D remain complete. Slice 1F is complete on this Fedora machine only. Slice 1E is complete as a development-only dynamic link to `/opt/aether-ffmpeg-9.1` and a local media probe. ADR-008 stays **OPEN — PENDING LEGAL COUNSEL**. That link is not distribution approval. The evidence is `docs/validation/SLICE_1E_FFMPEG_9_1_DEVELOPMENT_INTEGRATION.md`. The earlier system-library block is kept in `docs/validation/SLICE_1E_MEDIA_METADATA_AND_SOURCE_TIMEBASE.md`. Not cross-platform and not production-ready. Marks stay session points. The overwrite command reads a mark pair as `[In, Out + 1)`. The stored points and the readout are unchanged, and ADR-002 was not edited. Same-rate overwrite is complete on this Fedora machine only. The evidence is `docs/validation/SLICE_1G_SAME_RATE_OVERWRITE.md`. Split is complete on this Fedora machine only. The evidence is `docs/validation/SLICE_1H_SPLIT_EDITING.md`. A split is refused unless the stored source duration equals the stored record duration and the rates match after reduction. Lift is complete on this Fedora machine only. The evidence is `docs/validation/SLICE_1I_LIFT_EDITING.md`. A lift removes one selected instance and leaves that record interval empty. Extract, ripple, and playback are **NOT STARTED**. Slice 1J is **NOT STARTED**.

The editing shell and project dialog are done as a presentation of the existing commands. The note is `docs/validation/UI_SHELL_AND_PROJECT_DIALOG.md`. Playback is not implemented. Later slices stay **NOT AUTHORISED**.

Native interface integration is done. The same shell now drives the existing project, bin, import, sequence, and timeline commands. The note is `docs/validation/NATIVE_INTERFACE_INTEGRATION.md`. Playback is not implemented. Later slices stay **NOT STARTED** and **NOT AUTHORISED**.

Slice 1D places a registered bin reference on a sequence through that same workspace. The note is `docs/validation/SLICE_1D_TIMELINE_CLIP_PLACEMENT.md`. The window title uses the product name Aether. Playback was not started.

The architecture in [ARCHITECTURE.md](ARCHITECTURE.md) and the module boundaries in [MODULES.md](MODULES.md) are the input to Phase 0. No module, library, scaffold, or build system is created until that review approves them. Approval means the decisions in section 11 of the architecture document are accepted or the documents have been revised and accepted. Silence is not approval. That paragraph is the roadmap as proposed before Slice 0A. Slice 0A, recorded above, does not approve these phases. `docs/19_DEVELOPMENT_ROADMAP.md` carries the same Slice 1D status. Later slices stay unauthorised. Its seven phases are a later proposed compression, not a second completed plan.

## Completed Milestones

### Slice 1E — FFmpeg 9.1 development probe

Complete on this Fedora machine only, as a development-only dynamic link. The source of truth is `docs/validation/SLICE_1E_FFMPEG_9_1_DEVELOPMENT_INTEGRATION.md`. The executable is `build-slice1e-ffmpeg91/src/aether`. `ldd` resolves `libavformat.so.63`, `libavcodec.so.63`, `libswscale.so.10`, `libswresample.so.7`, and `libavutil.so.61` only under `/opt/aether-ffmpeg-9.1/lib`. A registered local representation can be probed without freezing the window. Unknown and conflicting timing stay visible. The probe does not write marks or a new table into `project.db`, and it does not set the placement source rate. ADR-008 stays **OPEN**. Playback was not started.

### Slice 1F — source and record marks

Complete on this Fedora machine only. The source of truth is `docs/validation/SLICE_1F_SOURCE_RECORD_MARKS.md`. Source marks follow the selected representation in source frames. Record marks follow the selected sequence in sequence frames. A pair is two ordered points, not a half-open edit range. Marks stay in the session and off `project.db` and off the cut undo stack. Schema version stays 2. No new table. Slice 1F did not implement overwrite.

### Slice 1G — same-rate overwrite

Complete on this Fedora machine only. The source of truth is `docs/validation/SLICE_1G_SAME_RATE_OVERWRITE.md`. The command reads session marks as `[In, Out + 1)`, requires equal integer-frame durations at the same reduced rate, and refuses a record end past the destination track's current maximum. Leftover pieces keep the source frames that sat on the surviving sequence frames. One transaction, one undo entry. The build is `build-slice1g`. Schema version stays 2. `kPrototypeVersion` stays `slice-0a`. Slice 1G did not start insert, split, lift, ripple, playback, or Slice 1H.

### Slice 1H — split editing

Complete on this Fedora machine only. The source of truth is `docs/validation/SLICE_1H_SPLIT_EDITING.md`. One selected instance and one typed sequence frame. The record partition is `[r0, f)` and `[f, r1)`, valid only when `r0 < f < r1`. The source cut is `s0 + (f - r0)` only when the source duration equals the record duration and the rates match after reduction. The piece that still starts at the original record start keeps the id. The piece that starts at the cut is minted with `makeStableId()`. One transaction, one undo entry. The build is `build-slice1h`. Schema version stays 2. `kPrototypeVersion` stays `slice-0a`. Insert, lift, ripple, playback, and Slice 1I were not started.

### Slice 1I — lift editing

Complete on this Fedora machine only. The source of truth is `docs/validation/SLICE_1I_LIFT_EDITING.md`. One selected timeline instance. The command deletes that row and leaves the record interval `[r0, r1)` empty. No gap item is written. Other instances, other tracks, and the sequence rate stay. One transaction, one undo entry. Undo restores the original id. The build is `build-slice1i`. Schema version stays 2. `kPrototypeVersion` stays `slice-0a`. Extract, ripple, playback, and Slice 1J were not started.

### Slice 1D — timeline clip placement

Complete on this Fedora machine only. The source of truth is `docs/validation/SLICE_1D_TIMELINE_CLIP_PLACEMENT.md`. A registered reference can be dragged onto a track, or inserted by the same command. An unprobed file needs an explicit source rate and source span. A different rate is refused. Playback is not implemented.

### Slice 1C — media import and project bin

Complete on this Fedora machine only. The source of truth is `docs/validation/SLICE_1C_MEDIA_IMPORT_AND_PROJECT_BIN.md`. This is not playback, not a probe, and not production-ready.

Scope: the window has a Root bin, an asset list, an empty state, selection, and an inspector. Import Media opens the native file dialog and registers existing files as references. The file is not copied. A Media ID stays stable when the recorded path changes. An exact recorded path that is already registered is reported and writes nothing. A failed batch writes nothing. An offline file stays in the bin with the same IDs. Schema version stays 2. No second database.

- There is no git repository, branch, or HEAD. `git init` was not run.
- Configure and the Debug build in `build-slice1c/` **PASSED** after one include fix, both exit 0. The configure log has no `CMake Warning` line. The successful build log has no compiler warning line. `build/`, `build-slice0b/`, `build-slice1a/`, and `build-slice1b/` were left in place. The link remains `SQLite3::SQLite3` and `/usr/lib64/libsqlite3.so.3.53.4`.
- CTest: 1 discovered, 1 passed. Qt Test inside that one test: TimingTests 7, ProjectTests 11, SequenceTests 10, TimelineWorkspaceTests 10, and MediaImportTests 10, 48 cases, 0 failed. That is not 48 CTest tests.
- On the running XCB window, Import Media opened the native file chooser and registered `validation-fixtures/slice-1c/live-media/picture.mov`. Close and Open Project kept the same Media ID, Representation ID, and object id. Renaming that fixture file aside, without deleting user media, left the row offline with those ids. A Wayland file dialog was not opened.
- `QT_QPA_PLATFORM=wayland --shutdown-now` and `QT_QPA_PLATFORM=xcb --shutdown-now` each logged start and stop and exited 0. The XCB probe listed `0x01200007  0 fedora AETHER` and closed with exit 0. `wmctrl` did not list a Wayland window. The XCB result is not a native Wayland result.
- ADR-006 and ADR-007 stay **PROPOSED**. ADR-002 stays **ACCEPTED — INITIAL EDITORIAL TIME AUTHORITY**. Refinements A, B, and C were not accepted. C++17 stays prototype-only. FFmpeg is not linked.

### Slice 1B — timeline workspace

Complete on this Fedora machine only. The source of truth is `docs/validation/SLICE_1B_TIMELINE_WORKSPACE.md`. This is not a playback engine, not production-ready, and not cross-platform.

Scope: the existing window shows a Sequence list, Tracks in stored order, and clip blocks from persisted half-open record ranges. Selection and the inspector do not edit. Move and trim call the Slice 1A session commands. A rejected command leaves the cut unchanged. Session undo covers a successful edit and does not survive close. The ruler's timecode is a label. Pixels are used only while painting. No schema change.

- There is no git repository, branch, or HEAD. `git init` was not run.
- Configure and the Debug build in `build-slice1b/` **PASSED**, both exit 0. The configure log has no `CMake Warning` line. The build log has no compiler warning line. `build/`, `build-slice0b/`, and `build-slice1a/` were left in place. The link remains `SQLite3::SQLite3` and `/usr/lib64/libsqlite3.so`.
- CTest: 1 discovered, 1 passed. Qt Test inside that one test: TimingTests 7, ProjectTests 11, SequenceTests 10, and TimelineWorkspaceTests 10, 38 cases, 0 failed. That is not 38 CTest tests.
- Apply move and Apply trim were activated in Qt Test through the button clicked signal. Painted clip coordinates were not used. The smoke process did not click a control.
- `QT_QPA_PLATFORM=wayland --shutdown-now` and `QT_QPA_PLATFORM=xcb --shutdown-now` each logged start and stop and exited 0. The XCB probe listed a window titled AETHER and closed with exit 0. `wmctrl` did not list a Wayland window. The XCB result is not a native Wayland result.
- ADR-006 and ADR-007 stay **PROPOSED**. ADR-002 stays **ACCEPTED — INITIAL EDITORIAL TIME AUTHORITY**. Refinements A, B, and C were not accepted. C++17 stays prototype-only. FFmpeg is not linked.

### Slice 1A — editorial sequence foundation

Complete on this Fedora machine only. The source of truth is `docs/validation/SLICE_1A_EDITORIAL_SEQUENCE_FOUNDATION.md`. This is not a playback engine, not production-ready, and not cross-platform.

Scope: one Sequence with a stable ID, a display name, and one reduced frame rate; ordered Tracks; Timeline instances of an existing Master Clip with a distinct instance ID, the Master Clip's Media ID, and separate half-open source and record ranges. Schema version 2 adds `sequences`, `tracks`, and `timeline_instances` to the one `project.db`. A version-1 project migrates in one transaction. Session undo covers these commands and does not survive close. The last committed edit does. The window can list sequences, tracks, and instances. That list is inspection, not playback.

- There is no git repository, branch, or HEAD. `git init` was not run.
- Configure and the Debug build in `build-slice1a/` **PASSED**, both exit 0. The configure log has no `CMake Warning` line. The build log has no compiler warning line. `build/` and `build-slice0b/` were left in place. The link remains `SQLite3::SQLite3` and `/usr/lib64/libsqlite3.so`.
- CTest: 1 discovered, 1 passed. Qt Test inside that one test: TimingTests 7, ProjectTests 11, and SequenceTests 10, 28 cases, 0 failed. That is not 28 CTest tests.
- `QT_QPA_PLATFORM=wayland --shutdown-now` and `QT_QPA_PLATFORM=xcb --shutdown-now` each logged start and stop and exited 0. The XCB probe listed a window titled AETHER and closed with exit 0. `wmctrl` did not list a Wayland window. Buttons were not clicked. The XCB result is not a native Wayland result.
- ADR-006 and ADR-007 stay **PROPOSED**. ADR-002 stays **ACCEPTED — INITIAL EDITORIAL TIME AUTHORITY**. The published counts 12812, 12813, and 320320000 still pass. The MXF 12813-against-12812 result stays a failed encoder test. Refinements A, B, and C were not accepted. C++17 stays prototype-only. FFmpeg is not linked.

### Slice 0B — build-system hardening and foundation validation

Complete on this Fedora machine only. The source of truth is `docs/validation/SLICE_0B_BUILD_SYSTEM_HARDENING.md`. This is not production-ready and not cross-platform.

Scope: the product link uses the imported target `SQLite3::SQLite3`, which CMake 4.3's `FindSQLite3` exports. The deprecated name `SQLite::SQLite3` is not linked and was not aliased. The link library remains `/usr/lib64/libsqlite3.so`. No package was installed. The Slice 0A `build/` directory was left in place. The new tree is `build-slice0b/`.

- There is no git repository, branch, or HEAD. `git init` was not run.
- Configure and the Debug build **PASSED**, both exit 0. The configure log has no `CMake Warning` line. The build log has no compiler warning line.
- CTest: 1 discovered, 1 passed. Qt Test inside that one test: TimingTests 7 and ProjectTests 11, 18 cases, 0 failed. That is not 18 CTest tests.
- `QT_QPA_PLATFORM=wayland --shutdown-now` and `QT_QPA_PLATFORM=xcb --shutdown-now` each logged start and stop and exited 0. The XCB probe listed a window titled AETHER and closed with exit 0. `wmctrl` did not list a Wayland window. Buttons were not clicked. The XCB result is not a native Wayland result.
- ADR-006 and ADR-007 stay **PROPOSED**. ADR-002 stays **ACCEPTED — INITIAL EDITORIAL TIME AUTHORITY**. Refinements A, B, and C were not accepted. C++17 stays prototype-only. No schema, persistence, FFmpeg, playback, or audio change.

### Slice 0A — project core and persistence feasibility

This remains a completed implementation milestone. It is not Phase 1 of this roadmap. Phase 1 forbids a Qt or SQLite link. Slice 0A links both. The source of truth is `docs/validation/SLICE_0A_PROJECT_CORE_VALIDATION.md`.

Scope that was built and accepted on the tested Fedora machine: one project package (`project.db` plus a manifest), bins, one Media ID with separate representation IDs, master clips, a pure integer timing function, session-only undo, a documented autosave, an interrupted-transaction path, a minimal Qt window, and Qt Test. No sequence schema. No FFmpeg link.

- Fedora. `qt6-qtbase-devel` 6.11.2. `sqlite-devel` 3.53.4. 14 packages installed, 0 removed. The transitive packages are DNF requirements, not AETHER runtime dependencies. No FFmpeg package. No system upgrade.
- CMake Debug **PASSED**. Build **PASSED**, exit 0. Executable `build/aether`. Qt and SQLite linked. 18 developer warnings for deprecated `SQLite::SQLite3`. The replacement named by CMake is `SQLite3::SQLite3`. Debt, not a failure, and not fixed.
- Default Wayland: window not confirmed. `QT_QPA_PLATFORM=xcb`: window titled AETHER, close exit 0. Buttons not clicked.
- CTest: 1 discovered, 1 passed. Inside it, TimingTests 7 and ProjectTests 11: 18 Qt Test cases, 0 failed. Not 18 CTest tests.
- Timing uses the floor rule. `[0, 8)` at 30000/1001 is 12812. `[8, 16)` is 12813. The 200000-frame walk is 320320000. Negative positions and overflow fail closed. Adjacent spans that differ are expected. The earlier FFmpeg MXF failure is unchanged.

## Current Status

**SLICE 1I COMPLETE — LIFT EDITING IMPLEMENTED AND VALIDATED.**

**SLICE 1H COMPLETE — SPLIT EDITING IMPLEMENTED AND VALIDATED.**

**SLICE 1G COMPLETE — SAME-RATE OVERWRITE IMPLEMENTED AND VALIDATED.**

**SLICE 1F COMPLETE — SOURCE AND RECORD MARKS IMPLEMENTED AND VALIDATED.**

Accepted as a completed slice on this Fedora machine only. Playback is not implemented. Lift is complete on this Fedora machine only. Extract, ripple, and playback are not started. Slice 1J is not started. C++17 stays prototype-only. ADR-001 stays **ACCEPTED WITH CONDITIONS**. ADR-002 stays **ACCEPTED — INITIAL EDITORIAL TIME AUTHORITY**. ADR-003, ADR-004, and ADR-005 stay **ACCEPTED**. ADR-006 and ADR-007 stay **PROPOSED**. ADR-009 stays **ACCEPTED FOR FOUNDATION PROTOTYPE**. ADR-008 stays **OPEN — PENDING LEGAL COUNSEL**. Apache-2.0 stays a proposal. `libavfilter` is not approved. GStreamer is not a dependency. Refinements A, B, and C are not accepted.

## Future Milestones — Not Yet Authorised

The phases below stay the previously proposed stages. None of them is started by Slice 0A, Slice 0B, Slice 1A, Slice 1B, Slice 1C, Slice 1D, Slice 1F, Slice 1G, Slice 1H, or Slice 1I. Slice 1J is **NOT STARTED**. Extract, ripple, and playback were not started. Lift is the Slice 1I command and does not start these phases. No dates. For every row, implementation is **NOT STARTED** and authorisation is **NOT GRANTED**.

| Proposed stage | Prerequisites | ADR-002, ADR-006, ADR-007 | Blockers |
| --- | --- | --- | --- |
| Phase 0 — Architecture review | The architecture and module documents named above | None of the three is accepted by finishing this review. They stay **PROPOSED** unless the owner accepts them separately | This roadmap's Phase 0 exit was not adopted as a completed milestone. Slice 0A does not close it |
| Phase 1 — Kernel and the build skeleton | Phase 0 | A permanent rational-time model depends on ADR-002, which is **PROPOSED**. Slice 0A's timing function does not accept it | Slice 0A is not this phase. This phase's own text forbids Qt and SQLite |
| Phase 2 — Contracts | Phase 1 | No implementation of the three proposed ADRs | Not authorised |
| Phase 3 — Editorial model | Phases 0–2 | Sequence time depends on ADR-002, which is **ACCEPTED — INITIAL EDITORIAL TIME AUTHORITY** | Slice 1A stores a sequence foundation. It does not close this phase. Playback, nesting, transitions, and effects were not built |
| Phase 4 — Project storage | Earlier phases in this list | Does not depend on accepting ADR-002, ADR-006, or ADR-007. ADR-003 is already **ACCEPTED** | The roadmap phase was not authorised. Slice 0A's project package does not close this phase's exit |
| Phase 5 — Media catalog with a stand-in probe | Phase 4, and ADR-005, which is already **ACCEPTED** | Relink behaviour would depend on ADR-007, which is **PROPOSED** | FFmpeg is not linked. No probe was built |
| Phase 6 — Headless session | Phases 1–5 | A joiner across picture, sound, and exchange would depend on ADR-006, which is **PROPOSED**. The Slice 0A session is not that joiner | Not authorised |
| Phase 7 — FFmpeg probe and frame index | The Media Engine boundary in ADR-001 | Wiring that probe through the application joiner would depend on ADR-006 | FFmpeg library integration is **BLOCKED**. `ffmpeg-devel` was not installed. `libavfilter` is not approved. Version and ABI policy has no pin |
| Phase 8 — Export oracle | Phase 7 | ADR-006 if the oracle is joined to the editorial model | The 29.97 MXF discrepancy remains a failure. No export writer was built |
| Phase 9 — Qt shell | A reviewed shell, not the Slice 0A window | ADR-006 if the shell joins processing and exchange | The Slice 0A window was not a functional UI test. Buttons were not clicked. Native Wayland was not confirmed |
| Phase 10 — Playback against the oracle | Phases 7–9. The GStreamer wording in the phase body is an earlier proposal, not a selected framework | ADR-002 for frame-accurate `sampleIndex`, ADR-006 for the playback join | FFmpeg is not linked. GStreamer is not a dependency. Playback was not built |
| Phase 11 — Audio and sync | Phase 10 and the floor timing rule | ADR-002 for the persisted time model, ADR-006 for the sound join | Not authorised. The timing function is not a playback engine |
| Phase 12 — Color | A separate acceptance of any colour library | None of the three proposed ADRs is accepted by naming a colour library | OpenColorIO is not accepted. Not authorised |
| Phase 13 — OTIO round trip | A native project that already reopens, and ADR-007 | ADR-007 is **PROPOSED** | Interchange was not built. External NLE behaviour was not tested |
| Phase 14 — Plugin ABI | A reviewed ABI decision | ADR-006 if plugins enter the joiner | Not authorised |
| Phase 15 — Large projects and proxies | Phase 7 and a passed codec tuple | ADR-007 if conform is implied by a proxy | FFmpeg and proxy generation were not started. The 29.97 MXF failure still blocks a 29.97 writer |
| Phase 16 — Linux acceptance and portability audit | The earlier exits, on more than one machine | None of the three is closed by a portability note | Cross-platform validation was not run. One Fedora machine only |

## 1. How phases work

Each phase after approval is one reviewable slice.

- It has one purpose and a written exit test.
- It adds the smallest code that satisfies that test.
- It follows the dependency matrix. A phase does not add a convenience edge between domain modules.
- It leaves the tree buildable on Linux and leaves the previous exit tests passing.
- It does not start the next phase's feature. Playback does not appear in the export-oracle phase. The plugin ABI does not appear in the first editorial phase.
- It carries no calendar estimate. Size is the amount of surface that must be reviewed, and each phase below is one surface.

A phase ends in a review of its diff and its exit test. The next phase waits for that review.

The accepted direction is C++, Qt 6, CMake, FFmpeg as the mandatory core media framework, and SQLite (ADR-001, **ACCEPTED WITH CONDITIONS**). GStreamer is not a mandatory dependency. Phase 10 below is an earlier playback plan and is not a selected framework. OpenTimelineIO and OpenColorIO remain candidates. A phase may use a test library as an optional open-source development dependency, named in that phase's review, and not linked into the editor. `libavfilter` is not in scope. If a phase believes it needs a new runtime dependency, it stops and revises this roadmap before any library is added.

CMake appears in Phase 1, because that is the first phase that compiles. The current document-only change does not add it.

## 2. Phase 0 — Architecture review

**Purpose.** Accept or revise the architecture before any code exists.

**Work.** Read the three documents. Check the ten requirements against the seam (`CompositionSlice`, `MediaLocation`, `RenderRequest`), the offline package, the plugin tiers, and the matrix. Confirm the locked decisions or send them back for a revision.

**Exit.** A recorded approval, or a new revision of these documents that replaces the open questions. Implementation stays stopped until the approval exists.

**Out of this phase.** Repositories beyond the documents, package manifests, and sample media.

## 3. Phase 1 — Kernel and the build skeleton

**Purpose.** Establish rational time and a Linux build that can run tests, and nothing else.

**Work.** Kernel types and conversions. A minimal CMake build for the kernel target and its tests. No Qt, FFmpeg, GStreamer, SQLite, OTIO, or OCIO link.

**Exit.** Reviewed tests for rate conversion at 24, 25, 24000/1001, 30000/1001, and 60000/1001, including a drop-frame label that changes the printed number and leaves the frame index unchanged. A negative test for a zero denominator.

**Depends on.** Phase 0 approval.

## 4. Phase 2 — Contracts

**Purpose.** Freeze the passive documents and interfaces enough for Editorial to be written against them.

**Work.** Contract types and interface definitions from [MODULES.md](MODULES.md). No implementations, no I/O.

**Exit.** A review that the public types contain no third-party headers and no functions that trim, relink, decode, or open a database. Editorial, Media Management, Interchange, and Processing are still absent as libraries.

## 5. Phase 3 — Editorial model

**Purpose.** Prove frame-accurate editing in memory.

**Work.** In-memory model, commands, validation, resolver, and composition hash. Fixtures are constructed in tests, not read from disk.

**Exit.** A reviewed fixture set, with expected frame numbers written in the test, covering overwrite, insert, lift, extract, razor, trim, ripple, roll, slip, slide, a transition centered on a cut, a still with `hold`, a mixed-rate clip under both `nearest` and `floor`, and a nested sequence placed on an outer sequence. A failed invariant leaves the model unchanged.

**Risk addressed.** Frame accuracy of the model, before any codec is involved.

## 6. Phase 4 — Project storage

**Purpose.** Persist opaque documents locally and recover from a failed autosave.

**Work.** Package directory, SQLite module, transactions, lock file, autosave database, storage-schema version. Editorial documents are serialized by Editorial and stored as bytes.

**Exit.** A headless test writes a timeline document, reopens it, and gets the same composition hash. A test that interrupts autosave still opens `project.sqlite`. A second open in another process is refused. No media files.

## 7. Phase 5 — Media catalog with a stand-in probe

**Purpose.** Prove media management is independent of decoding and of the timeline.

**Work.** Catalog, offline state, relink, derivative records, and a test double for `IMediaProbe`.

**Exit.** Relink fixtures: matching identity links, a mismatched rate stays offline, a timeline document from Phase 3 still resolves when every asset is offline. Media tests do not link Editorial or Processing.

## 8. Phase 6 — Headless session

**Purpose.** Join the modules the way the architecture describes, without a window.

**Work.** Application operations for open, edit, undo, save, and import-with-stand-in-probe. `ISessionObserver` implemented by the test.

**Exit.** A script opens a package, places a clip, ripple-trims it, undoes the trim, saves, and reopens to the same hash. A review of the link line shows the UI target does not exist yet and domain libraries still do not link each other.

## 9. Phase 7 — FFmpeg probe and frame index

**Purpose.** Put FFmpeg behind `IMediaProbe` and build a frame index.

**Work.** Processing's probe and index parts only. Short local fixtures generated in the test: one intra-frame clip and one long-GOP clip, using an encoder the LGPL FFmpeg build already provides, or a numbered image sequence where that is simpler.

**Exit.** The index frame count matches the known count. For the long-GOP fixture, the index names the keyframe that must be decoded to reach a chosen frame. No playback graph and no export UI.

**Risk addressed.** Frame accuracy of the file side, which is the input the playback seek will later trust.

## 10. Phase 8 — Export oracle

**Purpose.** Render a timeline to numbered images through the FFmpeg path and check source-frame identity.

**Work.** Application builds `RenderRequest` values from the resolver and from media locations. Processing writes one image per sequence frame. Color is a raw passthrough. This phase does not link GStreamer or Qt.

**Exit.** For a fixture with a cut, a transition, and a mixed-rate clip, image N carries the source frame the fixture names. The test reads identity from the resolver expectation, and a pixel spot-check confirms the image is that source frame. Cancel removes a partial output and leaves the package hash unchanged.

**Risk addressed.** Frame accuracy of the path that does not depend on a realtime clock. This oracle is the reference for playback.

## 11. Phase 9 — Qt shell

**Purpose.** Give the headless session a Linux UI for bins, the timeline, and a monitor that can show a still from the oracle path.

**Work.** UI module, Qt Widgets, open and save, import, timeline gestures for the Phase 3 commands, and a monitor that displays a rendered still. Layout for a desktop window and a narrow window.

**Exit.** A reviewer can open a package, trim by a frame, undo, save, and reopen, on Wayland or X11 as available. The domain libraries still do not link Qt. Playback transport may be absent or inert.

## 12. Phase 10 — Playback against the oracle

GStreamer is not the accepted playback framework. The useful requirement is unchanged: interactive playback must present the same frames the oracle encoded, through the Media Engine, and a mismatch stops the roadmap.

**Purpose.** Show that interactive playback presents the same frames the oracle encoded.

**Work.** Processing's GStreamer backend behind `IPlayback`. Seek timestamps come from the Phase 7 index. An appsink (or equivalent in-process sink) captures `PresentedFrame` and pixels so the test does not require a window. The UI transport is wired after the headless comparison passes.

**Exit.** Seek and a short play of the Phase 8 fixture report the same sequence frame, asset id, and source frame as the oracle. A mismatch stops the roadmap until it is fixed or a review authorizes an FFmpeg implementation of `IPlayback` inside Processing. That authorization is a revision note, not a new module.

**Risk addressed.** GStreamer versus FFmpeg roles, and frame accuracy under playback.

## 13. Phase 11 — Audio and sync

**Purpose.** Define and measure audio alignment.

**Work.** Audio in the editorial mapping, audio in the export oracle, and audio in the GStreamer graph. The editorial quantum remains the sequence frame.

**Exit.** A local fixture with a visible and audible mark at a known frame. Export places the audio sample at the integer position the stored policy names. Playback of the same fixture stays within a tolerance written down at the start of the phase. The tolerance is a review decision of that phase, recorded next to the test, and it is not invented here. Seek flushes picture and sound to the same rational time before the presented frame is reported.

**Risk addressed.** A/V sync.

## 14. Phase 12 — Color

**Purpose.** Apply OpenColorIO on both the oracle path and the playback path from the same color decision.

**Work.** Copy a chosen config into the package. Processing builds the processor. Preview uses display and view. Export bakes only when the request says so.

**Exit.** A fixture with a known config produces the same transformed sample through export and through the playback sink, within the tolerance of the processor's CPU path. Removing the embedded config yields the documented raw view and an untagged flag, with no network attempt. A project saved on one directory opens on another directory on the same machine with the embedded config.

**Risk addressed.** Color management, and offline portability of the color decision.

## 15. Phase 13 — OTIO round trip

**Purpose.** Prove the interchange engine on the oracle fixture.

**Work.** Built-in OTIO adapter inside Interchange. Import and export commands in the Application. The report shown in the UI.

**Exit.** Export, import, accept, export again: the second OTIO file matches the first for every item the report marked preserved. A feature placed only in the SISE metadata namespace is listed in the report and survives a SISE-to-SISE cycle. A file with a missing media path imports as an offline asset and the cut remains. Interchange tests do not link Processing, and Processing tests do not link Interchange.

**Risk addressed.** OTIO round trip.

## 16. Phase 14 — Plugin ABI

**Purpose.** Load one effect across the C ABI and show that an ABI mismatch is refused.

**Work.** Plugin SDK header, Plugin Host, one in-tree sample effect built as a separate shared library against that header. The effect is open source and lives in the project. It is a sample, not a new runtime dependency.

**Exit.** The sample effect runs in the export oracle and in playback. A copy of the plugin built with a different major version does not load, and the clip falls back to passthrough with the missing effect named in the result. A review of the SDK header finds no C++ standard-library types and no Qt types in the C API.

**Risk addressed.** Plugin ABI stability. The residual risk is recorded in section 20: in-process plugins and a C++ Tier B that is allowed to break.

## 17. Phase 15 — Large projects and proxies

**Purpose.** Measure behavior when the timeline and the catalog are large, and select proxies through the playback quality policy on the same timeline.

**Work.** Interval lookup in Editorial, lazy catalog queries, proxy derivatives through the existing encode path, and the application's quality policy. A local fixture generator builds a project with many clips. The fixture count is chosen in the phase so the test stays reviewable.

**Exit.** Resolve and save times are measured and written into the phase review. Playback of the fixture through proxies presents the source frames that correspond to the proxy index, and export of a short range still uses masters by default. Deleting `cache/` and reopening changes no composition hash.

**Risk addressed.** Large-project performance. The phase reports numbers. It does not promise a frame rate in advance.

## 18. Phase 16 — Linux acceptance and portability audit

**Purpose.** Accept the Linux editor as a whole and write down what a port would have to replace.

**Work.** Wayland and X11 smoke of the shell, including a narrow window. A packaging note that names the system libraries and the LGPL FFmpeg requirement. An audit of the source for OS calls outside Platform, third-party includes outside the owning module, and domain links to Qt.

**Exit.** The audit is clean, or the exceptions are fixed before the phase is accepted. A short portability note lists Platform operations that a second OS must implement. No second OS is built in this phase.

**Risk addressed.** Linux-first portability: the design is shown to be portable because the OS surface is listed, and the product is shown to be Linux-first because that is the system that passed.

## 19. After the first plan

The following are real products and are out of the sixteen phases. Each one starts by revising the architecture if it changes a boundary.

- A second operating system, using the Phase 16 note.
- Subframe audio slip.
- Process isolation for plugins.
- Further interchange adapters (EDL, AAF, other XML) as Tier A adapters.
- Collaboration by any means other than copying a package or an OTIO file.
- A network render farm.

None of these is required to satisfy the ten architectural requirements.

## 20. Technical risks

### 20.1 Frame accuracy

**Why it is hard.** Long-GOP codecs decode in groups. Mixed frame rates do not land on integers unless the rounding is defined. A realtime sink will happily present a nearby frame.

**Where the architecture answers.** Positions are integer frames. Conform policy is stored on the clip. Export steps frames through FFmpeg. Playback seeks with a frame index and reports the frame it presented.

**Where the roadmap proves it.** Phase 3 for the model, Phase 7 for the index, Phase 8 for the oracle, Phase 10 for playback against the oracle.

**Residual risk.** Some files will have broken timestamps. The probe records that condition, and those assets do not silently enter the oracle. Handling them is a later, explicit repair tool, not a hidden guess in the resolver.

### 20.2 A/V sync

**Why it is hard.** Audio devices and video presentation do not share a clock. Sample rates and frame rates meet at ratios such as 24000/1001 against 48000.

**Where the architecture answers.** The sequence frame is the cut. Sample positions are integer conversions with a stored policy. Monitoring follows the audio clock and chooses the video frame that contains it. Seek flushes both to one rational time. The playhead displays the presented frame, not a raw clock.

**Where the roadmap proves it.** Phase 11, with a tolerance fixed at the start of that phase.

**Residual risk.** Device latency on a particular Linux audio graph can exceed the tolerance. The mitigation is the sink choice inside Processing and a measured delay, not a change to the editorial model.

### 20.3 Color management

**Why it is hard.** Preview and export drift apart when they use different processors, and a missing config tempts a program to fetch one.

**Where the architecture answers.** One `ColorDecision` is copied into the `RenderRequest`. OpenColorIO runs in Processing for both paths. The config is embedded in the package. A missing config uses a raw view and an untagged flag.

**Where the roadmap proves it.** Phase 12.

**Residual risk.** GPU and CPU OCIO paths can differ by small amounts. The acceptance comparison uses the CPU path for both sides first. A GPU preview is acceptable later only if a review records the new tolerance.

### 20.4 Large project performance

**Why it is hard.** Long-form timelines contain many items. Saving a full document on every mouse move, or resolving the whole film for a single-frame scrub, will not stay interactive.

**Where the architecture answers.** The in-memory model is the working set. SQLite writes happen on save and autosave. The resolver accepts a range. Caches are regenerable. Proxies are a playback policy on the same asset id.

**Where the roadmap proves it.** Phase 15 measures a generated large fixture. Phase 4 already keeps autosave off the main database file.

**Residual risk.** The first measurements may force a change to indexes or to snapshot granularity. That change stays inside Editorial and Project Storage and must not create a new dependency edge. If it would, the architecture is revised first.

### 20.5 GStreamer and FFmpeg roles

**Why it is hard.** Two media frameworks invite two sources of truth. GStreamer seeks are not frame-accurate for every codec. FFmpeg is a poor fit for device clocks.

**Where the architecture answers.** FFmpeg probes, indexes, and exports. GStreamer presents. Both consume `RenderRequest`. The index, not the GStreamer clock, chooses the source frame. The playhead follows `PresentedFrame`.

**Where the roadmap proves it.** Phase 7, Phase 8, and Phase 10. Phase 10 has an explicit stop if the comparison fails, and a single optional backend behind the same interface.

**Residual risk.** A format neither backend seeks correctly. It stays unsupported until the index can name its frames, rather than being approximately scrubbed.

### 20.6 OTIO round trip

**Why it is hard.** OTIO does not represent every grade, nest, and effect parameter. Silent loss is worse than a loud report.

**Where the architecture answers.** Interchange is a separate module. It returns a round-trip report. Unrepresentable data is also stored in a SISE metadata namespace. Editorial, not Interchange, accepts a document into the live model. Missing media becomes an offline asset.

**Where the roadmap proves it.** Phase 13, including idempotent re-export.

**Residual risk.** Foreign tools will drop the metadata namespace. The report is the mitigation the editor can guarantee. Perfect fidelity through a tool that does not know SISE Editor is not a goal.

### 20.7 Plugin ABI stability

**Why it is hard.** A C++ boundary breaks when compilers or standard-library versions differ. An effect that can see the editorial model will be abused and will freeze the model.

**Where the architecture answers.** Tier A is a C ABI with a major version check, data in and data out, no Qt, and no pointer to the timeline. Tier B is in-tree and is allowed to break. The host adapts plugins to contracts. Domain modules do not load libraries.

**Where the roadmap proves it.** Phase 14.

**Residual risk.** In-process plugins can still crash the editor. Isolation is listed after the first plan. The ABI can stay stable while a plugin remains unsafe. That is an accepted limit of the first design, not a hidden extra guarantee.

### 20.8 Linux-first portability

**Why it is hard.** A Linux-only call in Editorial or a Qt type in a document header becomes a rewrite when a second OS appears. Case sensitivity, path encoding, and audio sinks differ across systems.

**Where the architecture answers.** Platform owns OS calls. Its first implementation is Linux. Public headers stay free of third-party types. Qt is private to the UI. Paths are UTF-8. Domain tests need no display. The playback sink is inside Processing.

**Where the roadmap proves it.** Every phase runs on Linux. Phase 16 audits the boundaries and writes the port list. No port is scheduled inside this plan.

**Residual risk.** GStreamer sink behavior and Qt's windowing differ even between Wayland and X11. Phase 9 and Phase 16 treat both as Linux acceptance targets. Differences that force a domain change stop for an architecture revision.

## 21. Approval checklist

Review can approve the plan when all of the following are true:

- The ten requirements in the architecture document each map to a module boundary.
- Editorial, Media Management, Interchange, Project Storage, and Processing have no edges between them in the matrix.
- The application is the only builder of `RenderRequest`.
- Phase 0 is the current phase, and Phases 1 through 16 are still unstarted.
- The risk section names frame accuracy, A/V sync, color management, large-project performance, GStreamer versus FFmpeg, OTIO round trip, plugin ABI stability, and Linux-first portability, and each one has a phase that proves the mitigation.

When that approval is recorded, Phase 1 may start. Until then, the repository holds these documents and no implementation.
