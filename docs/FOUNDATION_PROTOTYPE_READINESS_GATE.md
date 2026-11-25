# Foundation Prototype readiness gate

Status: a gate, not an approval. This review does not grant final approval and does not start the prototype. The owner does that.

The prototype validates architecture. It is not a complete NLE. Scope: `docs/FIRST_IMPLEMENTATION_MILESTONE.md`.

## 1. Owner-approved decisions

| Decision | State |
| --- | --- |
| Technology direction | **ACCEPTED WITH CONDITIONS** (ADR-001). Not fully production-validated |
| Project database | **ACCEPTED** (ADR-003). One `project.db` plus a manifest. `.abin` is interchange |
| Media identity | **ACCEPTED** (ADR-005). One Media ID. Representations are not new source identities |
| Undo for the prototype | **ACCEPTED FOR FOUNDATION PROTOTYPE** (ADR-009). Session-only. Other recovery mechanisms stay separate |
| Licensing | **OPEN — PENDING LEGAL COUNSEL** (ADR-008). Not accepted |

## 2. Conditions still to validate

- Product CMake build. Root `CMakeLists.txt` is empty.
- Qt feasibility and SQLite feasibility.
- FFmpeg version pin, ABI policy, and a controlled build. The baseline is written. The pin is not built.
- Cross-platform validation. Not run.
- Legal review of the actual dependency configuration.

## 3. Tests

| Item | State |
| --- | --- |
| 24 fps OP1a ProRes 422 HQ + stereo PCM, tested configuration | **Passed** (`docs/validation/FFMPEG_MXF_ROUNDTRIP_TEST.md`) |
| 29.97 audio sample contract | **Failed**. Eight frames, 12813 samples in the file, 12812 in the contract. Reproduced. The expectation was not changed (`docs/validation/AUDIO_FRAME_SAMPLE_BOUNDARY_VALIDATION.md`) |
| OP-Atom picture-only, tested configuration | **Passed** |
| OP-Atom stereo | **Not supported** by the installed build |
| External NLE relink | **Not tested**. Not required before internal architecture tests. It remains an interoperability blocker for any claim of professional conform readiness |
| FFmpeg development-library integration | **Not demonstrated**. Headers are absent. Harness configure **BLOCKED** (`docs/validation/FFMPEG_LIBRARY_INTEGRATION_VALIDATION.md`) |
| Resolve, Media Composer, Premiere | **Not tested**. Not claimed |

Full MXF OP-Atom stereo is not a prerequisite for all prototype work. It is not validated.

## 4. Legal blockers

Counsel has not written. Open items are listed in `docs/technology/LEGAL_COUNSEL_QUESTIONS.md`. No codec is distributable on the strength of this gate. The Fedora FFmpeg GPL build is not the shippable engine. Qt distribution obligations are open, which blocks a redistributable UI package and does not by itself forbid a later local feasibility check the owner authorises.

## 5. Prototype acceptance

The prototype is accepted as a scope description only. It is not accepted as done. No capability has `result = passed` in product code. `src/` has no application. This gate does not mark the stack production-validated.

## 6. What can proceed, and what stays blocked

The owner can authorise isolated work that does not pretend the blocked rows are done. This document does not start that work.

Can proceed when the owner authorises it, because the editorial decisions those pieces need are accepted:

- C++ / Qt 6 / CMake application foundation, as a feasibility check of the Qt condition, without a redistributable package
- one `project.db` and a manifest, including create, save, close, and reopen, as the SQLite feasibility check
- multiple bins in that one project
- stable Media IDs and representation records
- Master Clip creation, a Sequence, and a basic Timeline model
- save and reopen of those editorial relationships
- location changes and relink that do not replace a Media ID
- integer sample and frame timing checked against `sampleIndex`, without calling the 29.97 muxer a pass
- session-only command undo and redo
- autosave and basic recovery, independent of undo

Stays blocked:

- Linking or shipping FFmpeg until the harness compiles and runs against a reviewed pin
- Media inspection and proxy generation that require that link, and any proxy codec that is not a passed tuple
- Offering a 29.97 MXF writer while the eight-frame contract fails
- OP-Atom stereo on this build
- External NLE relink as a conform claim
- `libavfilter`
- AVX import and export until the AVX schema and its acceptance criteria are explicitly judged stable enough. This gate does not make that judgement
- Filling `LICENSE` or claiming a cleared Qt or FFmpeg distribution
- Any statement that the stack is fully production-validated

## 7. Gate word

**CONDITIONALLY READY**

The decisions that previously blocked the editorial model are recorded. Conditions, one failed audio contract, a blocked library link, and an open legal review remain. The owner has not been asked to reopen the accepted decisions. The owner is asked to authorise any isolated start, and to send the counsel questions. This gate does not authorise the start.

## 8. Post-Slice-0A status — 2026-11-20

**SLICE 0A COMPLETE — DEVELOPMENT FROZEN PENDING NEXT AUTHORISATION.**

Accepted on the tested Fedora machine only. Not cross-platform, not production-ready, not a complete NLE, not codec, playback, interchange, or external-NLE validated, and not a power-loss proof. Slice 0B has not started.

Sections 1–7 are the original pre-build assessment and are kept. Where they say the root `CMakeLists.txt` is empty, that `src/` has no application, or that Qt and SQLite feasibility were not measured, that was true when this gate was written. Slice 0A is a later, narrower measurement. It does not complete the Foundation Prototype and it does not authorise the next milestone. The source of truth for the slice is `docs/validation/SLICE_0A_PROJECT_CORE_VALIDATION.md`.

### Completed

Only what that validation report shows, on the tested Fedora machine:

- Slice 0A, Project Core and Persistence Feasibility, was authorised and then accepted.
- `qt6-qtbase-devel` 6.11.2 and `sqlite-devel` 3.53.4 were installed with their DNF requirements. 14 packages installed, 0 removed. Those other packages are not AETHER runtime dependencies. No FFmpeg package was installed. The system was not upgraded.
- The earlier configure failed closed while those two development packages were missing. After they were present, CMake Debug configure **PASSED** and the build **PASSED**, exit 0. The executable is `build/aether`. Qt and SQLite are linked. The configure log contains 18 developer warnings that `SQLite::SQLite3` is deprecated and that `SQLite3::SQLite3` is the replacement. That warning is debt. It is not a failure and it is not fixed.
- C++17 remains the prototype-only standard recorded by the build. ADR-002, ADR-006, and ADR-007 were left **PROPOSED**.
- Qt Test passed for the project package, the eight-table schema, transactions, bins, Media IDs and representations, master clips, session-only undo, autosave, and the interrupted-transaction path. `MEDIA/` stayed empty. No media file is created.
- Timing passed on the floor rule. `[0, 8)` at 30000/1001 and 48000 Hz is 12812. `[8, 16)` is 12813. The 200000-frame walk is 320320000. Negative positions and overflow fail closed. Adjacent spans that differ are expected.
- CTest discovered 1 test and that test passed. Inside it, TimingTests reported 7 and ProjectTests reported 11: 18 Qt Test cases, 0 failed. That is not 18 CTest tests.
- With `QT_QPA_PLATFORM=xcb`, the window was titled AETHER and the process exited 0 after close.

### Still proposed or unresolved

- ADR-002, ADR-006, and ADR-007 stay **PROPOSED**. They were not accepted.
- ADR-008 stays **OPEN — PENDING LEGAL COUNSEL**. Apache-2.0 stays a proposal.
- ADR-001 stays **ACCEPTED WITH CONDITIONS**. Still open: a real FFmpeg development build, Qt and SQLite beyond this Slice 0A check, library integration, a version and ABI policy applied to a pin, another platform, and legal review. The approved later link set remains `libavutil`, `libavcodec`, `libavformat`, `libswscale`, and `libswresample`, encapsulated in the Media Engine. `libavfilter` is not approved. GStreamer is not a dependency.
- The default Wayland window was not confirmed. The window buttons were not clicked.
- Recovery beyond the controlled autosave and interrupted-transaction test is not a power-loss proof.
- FFmpeg library integration remains **BLOCKED**. The 29.97 MXF discrepancy remains a failure. OP-Atom stereo remains unsupported on the tested build. External relink and conform remain untested. Cross-platform validation was not run.
- A sequence and timeline were not built. The next slice is not authorised.

### Current readiness

The Foundation Prototype is not complete. The pre-build word in section 7 stays **CONDITIONALLY READY** for the editorial decisions recorded there. It is not a statement that the prototype has been built, and it does not authorise more work.

Current readiness after Slice 0A: one project-core slice is accepted on the tested Fedora machine. The prototype as specified in `docs/FIRST_IMPLEMENTATION_MILESTONE.md` is not done. Development is frozen. The next milestone is unauthorised.
