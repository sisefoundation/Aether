# Pre-build decision package

Status: owner decisions recorded on 2026-11-20. This package does not grant implementation approval and does not start the Foundation Prototype. The readiness word in the original gate is **CONDITIONALLY READY** (`docs/FOUNDATION_PROTOTYPE_READINESS_GATE.md`).

**SLICE 0A COMPLETE — DEVELOPMENT FROZEN PENDING NEXT AUTHORISATION.**

Accepted on the tested Fedora machine only. Not cross-platform, not production-ready, not a complete NLE, not codec, playback, interchange, or external-NLE validated, and not a power-loss proof. Slice 0B has not started. No next slice is authorised.

## Decision classification

The five owner matters are no longer all awaiting an owner. They are not all the same kind of decision.

| Matter | Class |
| --- | --- |
| ADR-001, technology | **Accepted with conditions.** C++, Qt 6, CMake, SQLite, and FFmpeg as the later mandatory media framework. Not production-validated |
| ADR-003, project database | **Accepted.** One `project.db` plus a manifest |
| ADR-004, `.abin` | **Accepted.** Interchange, not a second database |
| ADR-005, media identity | **Accepted.** One Media ID. Representations have their own IDs |
| ADR-009, undo | **Accepted for the Foundation Prototype.** Session-only undo. Autosave, crash recovery, snapshots, and journals stay separate. Persistent undo stays deferred |
| ADR-008, licensing | **Open legal.** Pending counsel. Not accepted. Apache-2.0 stays a proposal |
| ADR-002, rational time | **Proposed.** Not accepted |
| ADR-006, application joiner | **Proposed.** Not accepted |
| ADR-007, relink, conform, and exchange | **Proposed.** Not accepted |
| Validation conditions | **Open.** A real FFmpeg development build, library integration, a version and ABI pin, Qt and SQLite beyond the Slice 0A check, another platform, and legal review |
| `libavfilter` | **Not approved** |
| GStreamer | **Not a dependency** |

Sections 1–6 below are the decision record. They are not rewritten into a claim that every condition has been measured.

## 1. Technology — accepted architectural direction, with validation conditions

ADR-001 is **ACCEPTED WITH CONDITIONS**. It is not fully production-validated.

Accepted direction: C++ as the primary language, Qt 6 as the desktop framework, CMake, SQLite for `project.db`, and FFmpeg as the mandatory core media framework. The Media Engine dynamically links `libavutil`, `libavcodec`, `libavformat`, `libswscale`, and `libswresample` only. Domain code does not see FFmpeg types. `libavfilter` is not approved. GStreamer is not a mandatory dependency.

The Fedora FFmpeg 9.0.2 GPL build on this machine is not the distribution configuration. Library integration is **BLOCKED** (`docs/validation/FFMPEG_LIBRARY_INTEGRATION_VALIDATION.md`). The build baseline is `docs/technology/FFMPEG_DEVELOPMENT_CONFIGURATION.md`.

Outstanding technical conditions:

1. Product build validation. The root `CMakeLists.txt` is still empty.
2. Qt feasibility and SQLite feasibility. Not measured.
3. FFmpeg library integration. **BLOCKED** until `ffmpeg-devel` headers exist and the harness configures, links, and runs. Installing that package would not adopt the Fedora build as the pin.
4. Version and ABI policy applied to a real pin. The policy is written. The pin is not built.
5. Cross-platform validation. One Fedora machine only.
6. Legal review of the actual dependency configuration. Still open (section 5).

Items 1 and 2 are the package as recorded before Slice 0A. They stay here because they were true on that date. What the later slice measured is in "Post-Slice-0A Development Status". That section does not close items 3–6.

## 2. Project database — accepted

ADR-003 is **ACCEPTED**. One `project.db` is the only editorial authority. A Project manifest describes the package and is not a second editorial authority. Bins are containers in that database. `.abin` is interchange (ADR-004), and import does not duplicate a Media Asset.

Outstanding technical work, not an open decision: one migration series, and a measured SQLite open with foreign keys, the lock, and a crash between commit and manifest rename. No database file was created by this package.

## 3. Media identity — accepted

ADR-005 is **ACCEPTED**. One stable Media ID per logical asset. Proxies and other files are representations with their own Representation IDs. Location is separate. Relink does not silently replace a Media ID. The Media ID is not weakened.

## 4. Undo — accepted for the Foundation Prototype

ADR-009 is **ACCEPTED FOR FOUNDATION PROTOTYPE**. Session-only undo and redo. Autosave, crash recovery, snapshots, and operation journals stay independent. A committed edit survives close. Persistent undo is future work, reconsidered before any release that promises undo after relaunch.

## 5. Licensing — open, pending counsel

ADR-008 is **OPEN — PENDING LEGAL COUNSEL**. It was not accepted. Apache-2.0 is not adopted. Dynamic linking does not clear the open list in `docs/technology/LEGAL_COUNSEL_QUESTIONS.md`.

Still open: AETHER source licence, contributor IP policy, DCO and extra contributor documents, Qt distribution obligations, the FFmpeg build’s LGPL or GPL status, ProRes patents, DNx patents, AAF implementation licensing, third-party obligations, trademarks and asset ownership, and binary packaging.

## 6. What this package does not approve

- Starting the Foundation Prototype as a whole. Isolated work that can proceed is listed in the readiness gate. This package does not write that code.
- A production-validated stack.
- A 29.97 audio pass. The contract failure stands (`docs/validation/AUDIO_FRAME_SAMPLE_BOUNDARY_VALIDATION.md`).
- OP-Atom stereo on this build. **Not supported.**
- Resolve, Media Composer, or Premiere compatibility. External relink is **Not tested**.
- `libavfilter`.
- A filled `LICENSE`.

## Post-Slice-0A Development Status

**SLICE 0A COMPLETE — DEVELOPMENT FROZEN PENDING NEXT AUTHORISATION.**

Accepted on the tested Fedora machine only. Not cross-platform, not production-ready, not a complete NLE, not codec, playback, interchange, or external-NLE validated, and not a power-loss proof. Slice 0B has not started. No next slice is authorised.

The evidence is `docs/validation/SLICE_0A_PROJECT_CORE_VALIDATION.md`, not a new test run.

On Fedora, `qt6-qtbase-devel` 6.11.2 and `sqlite-devel` 3.53.4 were installed with their DNF requirements. 14 packages were installed and 0 were removed. The transitive packages are DNF requirements, not AETHER runtime dependencies. No FFmpeg package was installed. The system was not upgraded.

CMake Debug configure **PASSED**. The build **PASSED**, exit 0. The executable is `build/aether`. Qt and SQLite are linked. The configure log contains 18 developer warnings for the deprecated `SQLite::SQLite3` target. The replacement named by CMake is `SQLite3::SQLite3`. The warning is debt, not a failure, and it is not fixed. C++17 stays prototype-only.

The default Wayland window was not confirmed. With `QT_QPA_PLATFORM=xcb`, the window was titled AETHER and the process exited 0 on close. The buttons were not clicked.

CTest discovered 1 test and that test passed. Inside that executable, TimingTests reported 7 and ProjectTests reported 11: 18 Qt Test cases, 0 failed. That is not 18 CTest tests.

Timing uses the floor rule. `[0, 8)` at 30000/1001 is 12812. `[8, 16)` is 12813. The 200000-frame walk is 320320000. Negative positions and overflow fail closed. Adjacent spans that differ are expected. This does not reclassify the earlier FFmpeg 30000/1001 MXF failure.

The slice covers one project package, bins, Media IDs and representations, master clips, the timing function, session-only undo, a documented autosave, an interrupted-transaction path, and a minimal window. It does not cover a sequence, FFmpeg linking, playback, interchange, an external NLE, or power-loss protection.

ADR-002, ADR-006, and ADR-007 stay **PROPOSED**. ADR-008 stays **OPEN — PENDING LEGAL COUNSEL**. The Foundation Prototype is not complete.
