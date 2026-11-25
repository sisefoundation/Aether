# Pre-build decision changelog

Date: 2026-11-20. Records substantive documentation changes after the owner’s decisions. It does not invent an approval, a test result, or a legal conclusion.

Labels used below: a **confirmed requirement** was already true before this pass; an **accepted decision** is an owner decision recorded here; a **conditional decision** is accepted and not yet validated; an **unresolved matter** stays open.

## Decisions recorded

| Matter | Kind | Where |
| --- | --- | --- |
| C++, Qt 6, CMake, SQLite, FFmpeg as the mandatory core media framework, five libraries dynamically linked inside the Media Engine | Conditional decision. Not production-validated | ADR-001 |
| `libavfilter` excluded. GStreamer not a mandatory dependency. Fedora FFmpeg 9.0.2 GPL build is not the distribution configuration | Accepted decision | ADR-001, `docs/technology/FFMPEG_DEVELOPMENT_CONFIGURATION.md` |
| One `project.db` plus a manifest. `.abin` is interchange only | Accepted decision | ADR-003, ADR-004 |
| One Media ID. Representations are not new source identities | Accepted decision | ADR-005 |
| Session-only undo for the Foundation Prototype. Autosave, crash recovery, snapshots, and journals stay separate | Accepted decision, scoped | ADR-009 |
| Licensing, including Apache-2.0 as a proposal only | Unresolved. Pending counsel | ADR-008, `docs/technology/LEGAL_COUNSEL_QUESTIONS.md` |
| 12812 samples for eight frames at 30000/1001 and 48 kHz from frame 0 | Confirmed requirement. Not changed | `docs/validation/AUDIO_FRAME_SAMPLE_BOUNDARY_VALIDATION.md` |

## Tests this pass actually ran

| Test | Result | Evidence |
| --- | --- | --- |
| Integer `sampleIndex` / `sample_boundary` across 24, 25, 24000/1001, 30000/1001, 30 at 48 kHz, including adjacent spans and a 200000-frame walk | Passed for the contract. Origin `[0, 8)` at 30000/1001 is 12812 | `validation-fixtures/audio-sample-boundary-2026-11-20/logs/summary.txt` |
| FFmpeg CLI MXF at those rates | Integer rates and 24000/1001 passed. 30000/1001 eight-frame audio failed: 12813 against 12812, ProRes and MPEG-2 | Same directory, and the earlier failure left untouched in `docs/validation/FFMPEG_MXF_ROUNDTRIP_TEST.md` |
| 40-frame and 600-frame 30000/1001 totals | Partially passed. Totals match the floor function. The packet phase does not. Error against the floor grid stayed within one sample and did not grow past that on these files | Same logs |
| Audio-only WAV of 12812 samples, video-only eight-frame MXF | Passed for those narrower claims | Same logs |
| Negative frame index | Not supported | Timebase rule. Not extended |
| CMake configure of `tools/validation/ffmpeg_link_smoke_test` | Blocked. Packages `libavutil`, `libavcodec`, `libavformat`, `libswscale`, `libswresample` not found. `ffmpeg-devel` is not installed | `validation-fixtures/ffmpeg-library-inspect-2026-11-20/cmake-configure.log` |
| External NLE relink | Not tested | Unchanged |

No sample was padded or trimmed to force a pass. Ceil and nearest-half-up both yield 12813 at frame 8 and were rejected because they would hide the failure. A constant 1602 samples per frame drifts by 400 samples over 1000 frames and was rejected.

## Files created

- `architecture` decisions were revised in place, not created.
- `/home/editry/development/sise-editor/docs/validation/AUDIO_FRAME_SAMPLE_BOUNDARY_VALIDATION.md` — boundary measurement and the recommendation to keep `sampleIndex`.
- `/home/editry/development/sise-editor/docs/technology/FFMPEG_DEVELOPMENT_CONFIGURATION.md` — build baseline. Not a legal approval.
- `/home/editry/development/sise-editor/docs/validation/FFMPEG_LIBRARY_INTEGRATION_VALIDATION.md` — link attempt, blocked.
- `/home/editry/development/sise-editor/docs/PREBUILD_DECISION_CHANGELOG.md` — this file.
- `/home/editry/development/sise-editor/tools/validation/ffmpeg_link_smoke_test/CMakeLists.txt` and `main.c` — isolated harness. Did not compile.
- `/home/editry/development/sise-editor/validation-fixtures/audio-sample-boundary-2026-11-20/` — synthetic PCM, MXF, WAV, and logs.
- `/home/editry/development/sise-editor/validation-fixtures/ffmpeg-library-inspect-2026-11-20/` — environment, codec listings, configure log.

The root `CMakeLists.txt` and the empty stubs `docs/ARCHITECTURE.md`, `docs/DEVELOPMENT_ROADMAP.md`, `docs/MODULES.md`, `docs/MEDIA_PIPELINE.md`, and `docs/PROJECT_FORMAT.md` were left empty. Git was not initialised. No package was installed.

## Files changed, and why

ADRs:

- `architecture/decisions/ADR-001-technology-stack.md` — status set to accepted with conditions. Libraries, dynamic link, `libavfilter` exclusion, GStreamer exclusion, Fedora build rejected as the pin.
- `architecture/decisions/ADR-003-project-database.md` — one database and a manifest accepted, with the operational rules the owner required.
- `architecture/decisions/ADR-004-abin-package.md` — interchange role accepted so it cannot be read as a second database.
- `architecture/decisions/ADR-005-media-identity.md` — one Media ID and representation rules accepted.
- `architecture/decisions/ADR-008-licensing-and-originals.md` — kept open. Intended build direction added for counsel. No legal determination.
- `architecture/decisions/ADR-009-undo-durability.md` — accepted for the prototype only, with the reconsideration point.

Decision and gate documents:

- `docs/PREBUILD_DECISION_PACKAGE.md` — the five items are no longer awaiting an owner answer, except licensing.
- `docs/FOUNDATION_PROTOTYPE_READINESS_GATE.md` — **CONDITIONALLY READY**. Isolated work versus blocked work.
- `docs/FIRST_IMPLEMENTATION_MILESTONE.md` — seventeen-item architecture prototype, not a complete NLE.
- `docs/AETHER_PREBUILD_READINESS_REPORT.md` — the five items are no longer listed as unanswered.
- `docs/ARCHITECTURE_AUDIT.md` — C1–C5 and C9 closed to the extent of the decisions. Failures and the blocked link stay visible.
- `docs/PREBUILD_DECISION_REVIEW.md` — pointed at the later package so it does not deny the acceptance.

Technology and validation cross-references, not second copies of the integration spec:

- `docs/technology/FFMPEG_INTEGRATION_ARCHITECTURE.md` — remains the integration authority. Status and the eight-frame rule updated. `libavfilter` marked not approved.
- `docs/technology/FFMPEG_BUILD_AND_DISTRIBUTION.md` — points at the new baseline.
- `docs/technology/MEDIA_BACKEND_ARCHITECTURE.md` — points at the accepted framework and the blocked link.
- `docs/technology/MEDIA_CODEC_AND_CONTAINER_SUPPORT.md` — failure and blocked link kept in view.
- `docs/technology/LEGAL_COUNSEL_QUESTIONS.md` — open list and the intended pin, without answering them.
- `docs/technology/TECHNOLOGY_STACK_EVALUATION.md`, `MEDIA_IDENTITY_MODEL_EVALUATION.md`, `PROJECT_DATABASE_ARCHITECTURE_EVALUATION.md`, `UNDO_DURABILITY_EVALUATION.md` — marked as evaluations the owner has since answered, except where they still describe history.
- `docs/03_TECHNOLOGY_DECISIONS.md`, `docs/10_PLAYBACK_AND_RENDERING.md`, `docs/18_BUILD_AND_RELEASE.md`, `docs/SYSTEM_ARCHITECTURE.md`, `docs/DEPENDENCY_AND_LICENCE_AUDIT.md`, `docs/validation/ARCHITECTURE_CONTRACT_REVIEW.md`.

Storage, identity, undo, and the eight local documents that were not to be overwritten wholesale:

- `docs/PROJECT_STORAGE_AND_DATABASE.md` — `project.db` is the authority. Column overlap remains one future migration, not a second package.
- `docs/15_UNDO_RECOVERY_AND_SAFETY.md`, `docs/04_CORE_DATA_MODEL.md`.
- `docs/BIN_ARCHITECTURE.md`, `docs/MEDIA_IDENTITY_SPECIFICATION.md`, `docs/MEDIA_IDENTITY_AND_METADATA.md`, `docs/MEDIA_IDENTITY_AND_BIN_RELATIONSHIP.md`, `docs/PROXY_AND_TRANSCODE_SYSTEM.md`, `docs/PROXY_AND_ORIGINAL_MEDIA_RELATIONSHIP.md` — removed the rule that a proxy receives its own media id. Descriptor and relationship text kept.
- `docs/NATIVE_MEDIA_ARCHITECTURE.md` — 19 November study kept. Failure not reclassified.
- `docs/AUDIO_EDITING_PHILOSOPHY.md`, `docs/AUDIO_ENGINE_ARCHITECTURE.md`, `docs/EDITORIAL_PHILOSOPHY.md`, `docs/TIMELINE_ENGINE_ARCHITECTURE.md`, `docs/TRIM_ENGINE_SPECIFICATION.md` — contradictions annotated. Useful engine and trim text kept.
- `DEVELOPMENT_ROADMAP.md` — stack sentence and Phase 10 no longer select GStreamer. Phase list kept.
- `docs/TIMEBASE_AND_FRAME_ACCURACY.md`, `docs/AUDIO_TIMEBASE_AND_SYNC.md`, `docs/05_TIMEBASE_AND_TIMING_ARCHITECTURE.md` — span form stated. The formula and 12812 were not replaced.
- `ARCHITECTURE.md` — stack sentence and the locked FFmpeg/GStreamer split superseded. The rest of the architecture remains.
- `docs/19_DEVELOPMENT_ROADMAP.md`, `docs/ARCHITECTURE_RISK_REGISTER.md`, `README.md`.

## Conflicts and how they were resolved

- FFmpeg and GStreamer as equal foundational frameworks. Resolved by ADR-001. Older GStreamer playback prose was marked as an earlier proposal, not deleted where it still describes clocks and request shapes.
- `project.sqlite` versus `project.db`. Resolved by ADR-003.
- Proxy media id versus one Media ID. Resolved by ADR-005 and by correcting the sentences that granted a second media id.
- Undo history versus session undo. Resolved for the prototype by ADR-009. The other four mechanisms stay named and separate.
- 12812 versus the muxer’s 12813. Not resolved by changing the contract. The floor span rule already allows a later eight-frame span to be 12813 without changing `[0, 8)`. Alternatives that yield 12813 at frame 8 were rejected. No owner conflict forced a change.
- Sequence start frame as a second sample origin. Not adopted. `sampleIndex` uses the stored frame index. Recorded as a remaining note, not a silent change.
- Negative indexes. Left unsupported because floor and toward-zero disagree and the timebase rejects a negative position.

## What this changelog does not do

It does not mark the stack production-validated. It does not claim Resolve, Media Composer, or Premiere compatibility. It does not claim the link harness ran. It does not adopt a licence.

## Milestone — Slice 0A accepted, 2026-11-20

This entry is later than the documentation pass above. It does not revise those rows. The sentence in "Files created" that no package was installed, and that the root `CMakeLists.txt` was left empty, is that documentation pass. It stays true for that event. It is not deleted and it is not rewritten.

**SLICE 0A COMPLETE — DEVELOPMENT FROZEN PENDING NEXT AUTHORISATION.**

Accepted on the tested Fedora machine only. Not cross-platform, not production-ready, not a complete NLE, not codec, playback, interchange, or external-NLE validated, and not a power-loss proof. Slice 0B has not started.

Slice 0A then happened as a separate sequence. Both the no-install row above and the install row below stay true because they are different events.

1. First attempt. The product configure found `qt6-qtbase-devel` and `sqlite-devel` missing and stopped. No package was installed on that attempt. Log: `validation-fixtures/slice-0a-2026-11-20/cmake-configure.log`.
2. The owner approved installing `qt6-qtbase-devel` and `sqlite-devel` only.
3. 14 packages were installed and 0 were removed. The direct packages are `qt6-qtbase-devel` 6.11.2 and `sqlite-devel` 3.53.4. The other twelve are DNF requirements of those two, not AETHER runtime dependencies. No FFmpeg package. No system upgrade. Log: `validation-fixtures/slice-0a-2026-11-20-build-verification/package-install.log`.
4. The Debug configure and the build completed, exit 0. CTest discovered 1 test and that test passed. TimingTests reported 7 and ProjectTests reported 11: 18 Qt Test cases, 0 failed. That is not 18 CTest tests. The executable is `build/aether`. Qt and SQLite are linked. Eighteen developer warnings name deprecated `SQLite::SQLite3` and the replacement `SQLite3::SQLite3`. That warning is not fixed.
5. The owner accepted Slice 0A on the tested Fedora machine.
6. Development is frozen pending the next authorisation. Slice 0B has not started.

The owner authorised Slice 0A, Project Core and Persistence Feasibility, and has accepted it as complete on the tested Fedora machine.

- Dependencies installed: 14 packages, 0 removed. Direct packages were `qt6-qtbase-devel` 6.11.2 and `sqlite-devel` 3.53.4. The other twelve are DNF requirements of those two, not independent AETHER runtime requirements. No FFmpeg package was installed. The system was not upgraded.
- Debug CMake configure completed. The build completed. The executable is `build/aether`. Qt and SQLite were the libraries CMake found and the build linked. Eighteen CMake developer warnings name the deprecated `SQLite::SQLite3` target and the replacement `SQLite3::SQLite3`. That warning is non-blocking debt. CMake was not changed to clear it.
- C++17 remains prototype-only. ADR-002, ADR-006, and ADR-007 stay **PROPOSED**.
- Default Wayland: the window was not confirmed. `QT_QPA_PLATFORM=xcb`: the window title was AETHER and the process exited 0 after close. The buttons were not clicked.
- CTest discovered 1 test and that test passed. Qt Test inside that executable reported TimingTests 7 and ProjectTests 11: 18 cases, 0 failed. That is not 18 CTest tests.
- Timing passed on the floor boundary rule. `[0, 8)` at 30000/1001 is 12812, `[8, 16)` is 12813, and the 200000-frame walk is 320320000. Negative positions and overflow fail closed. Adjacent spans that differ are expected. The earlier FFmpeg MXF failure was not reclassified.
- The owner accepted the slice. The next slice is not authorised.

**SLICE 0A COMPLETE — DEVELOPMENT FROZEN PENDING NEXT AUTHORISATION.**

Evidence: `docs/validation/SLICE_0A_PROJECT_CORE_VALIDATION.md`, `docs/validation/SLICE_0A_IMPLEMENTATION_PLAN.md`, and `docs/SLICE_0A_TECHNICAL_DEBT.md`. Logs: `validation-fixtures/slice-0a-2026-11-20-build-verification/`. The earlier fixture directory was not overwritten.

On 21 November 2026 the tree was arranged to the current ASWF sample-project layout (`include/aether` headers, `src` implementation, `tests` kept separate) and the owner directed Apache License 2.0 for AETHER source, copyright 2026 Society of Indian Screen Editors Foundation, architect Subhajit Singha, SISE (`LICENSE`, `NOTICE`, ADR-008). Counsel questions for Qt, FFmpeg, codecs, AAF, and trademarks stay open. ADR-008 is not accepted. C++17 stays prototype-only. The next feature slice is not authorised. `docs/ASWF_LAYOUT.md` is the path map.

## Decision — ADR-002 accepted, 2026-11-21

This entry is later than the Slice 0A rows above. Those rows said ADR-002 stayed **PROPOSED**. That was true on 2026-11-20. It is not deleted and it is not rewritten.

On 2026-11-21 the project owner accepted ADR-002 as **ACCEPTED — INITIAL EDITORIAL TIME AUTHORITY**. The decision is `architecture/decisions/ADR-002-rational-time.md`. The pre-decision review is `docs/decisions/ADR-002_TIME_AUTHORITY_DECISION_REVIEW.md`. That review did not grant the acceptance. Consequences and the five worked examples are `docs/decisions/ADR-002_IMPLEMENTATION_CONSEQUENCES.md`.

Accepted: `int64` frames, reduced rates 24/1, 25/1, 24000/1001, 30000/1001, and 60000/1001, no float clock, timecode as a label, checked arithmetic, hybrid ranges, picture selection by nearest source timestamp with an exact tie on the earlier frame, audio `sample_boundary` as absolute floor from sequence frame 0, no stored sample clock, frame-only scope with subframe positions refused or reported. The floor results 12812, 12813, and 320320000 stay. The MXF 12813-against-12812 encoder failure stays a failure. Its log was not rewritten. The displayed out-point stays **DEFERRED**.

No sequence table was added. No source, test, CMake, or package file was changed. ADR-006 and ADR-007 were not changed and stay **PROPOSED**. Timeline, playback, an audio engine, and media processing are not authorised.

**SLICE 0A COMPLETE — DEVELOPMENT FROZEN PENDING NEXT AUTHORISATION.**
