# Slice 1B timeline workspace

**SLICE 1B COMPLETE — TIMELINE WORKSPACE IMPLEMENTED AND VALIDATED — PLAYBACK AND MEDIA PROCESSING NOT YET IMPLEMENTED.**

Validated on this Fedora machine only. Not production-ready. Not cross-platform. Not a playback engine.

Logs: `validation-fixtures/slice-1b/`. Slice 0A, 0B, and 1A logs were not overwritten. `build/`, `build-slice0b/`, and `build-slice1a/` were not deleted.

## Phase 1 — Repository Inspection

There is no git repository. `git status` returned `fatal: not a git repository`. There is no branch and no HEAD. `git init` was not run.

Slice 1A source matches its validation report on the points this slice uses:

- Schema version is 2. A new database creates the original eight tables plus `sequences`, `tracks`, and `timeline_instances`. A version-1 database migrates in one transaction. Another version is refused. An armed failure rolls back.
- A Sequence has a stable id, a name, and one reduced rate. A Track has a stable id, a parent Sequence, and an ordinal. There is no track-type column. An instance has its own id, a Master Clip id, that clip's Media ID, and separate half-open int64 source and record ranges.
- Same-track record ranges that share a frame are rejected. Abutting ranges are allowed.
- `ProjectSession::moveInstance` and `trimInstance` push an undo entry only after the store call succeeds. Close clears the undo stack. The committed rows remain.

Discrepancies with `docs/validation/SLICE_1A_EDITORIAL_SEQUENCE_FOUNDATION.md`, left as they were:

- `include/aether/core/timing/sample_boundary.hpp` still says ADR-002 is proposed. The ADR is accepted. The comment was not edited.
- `kPrototypeVersion` is still `slice-0a`.
- The ADR-006 owner record still says development is frozen at Slice 0A. It was not edited. ADR-006 stays **PROPOSED**.
- `docs/TIMELINE_UI_SPECIFICATION.md` describes playhead, thumbnails, waveforms, snapping, and ripple, roll, slip, and slide. The Slice 1A window did not implement that specification.

The proposal is `docs/proposals/SLICE_1B_TIMELINE_WORKSPACE_PROPOSAL.md`. No unapproved decision blocked the workspace. The ruler label is a display count. The drop-frame flag stays absent. The open 59.94 skipped-number table was not implemented.

## Slice 1B Proposal

The proposal's twelve sections are purpose, files inspected, verified Slice 1A facts, discrepancies, excluded scope, the decision gate, workspace behavior, time and drawing, selection and edits, undo, tests, and the implementation boundary. The decision gate says the workspace calls existing session commands and does not change the schema, an ADR, or a dependency.

## Phase 2 — Implementation Summary

The existing window keeps Create, Open, and Close, and the bin list. Under that it shows the timeline workspace: a Sequence list, a painted track lane, an instance list, a read-only inspector, integer move and trim fields, and Undo and Redo.

Tracks are listed in stored ordinal order, including a track with no instances. Clip blocks use the stored record range. The paint uses 4 pixels per frame and does not store that width. There is no playhead, thumbnail, snap, ripple, roll, slip, slide, or multi-clip edit.

Move calls `ProjectSession::moveInstance`. Trim calls `ProjectSession::trimInstance`. A token that is not a non-negative integer never reaches the session. A session rejection is shown in the rejection label and the stored ranges stay as they were.

## Architecture and Persistence

The widget includes `ProjectSession` and does not include the store. It does not issue SQL. Editorial checks remain in the sequence module. Infrastructure still owns the connection and the write. Schema version 2 is unchanged. There is no second database, no undo table, and no timecode column.

C++17 stays prototype-only. The link is still `SQLite3::SQLite3` and `/usr/lib64/libsqlite3.so`. FFmpeg and GStreamer are not linked. No package was installed.

Undo and redo buttons call the session. A failed parse and a failed command add no undo entry. Close drops the stack. Reopen shows the last committed range.

## Tests and Validation

Environment: Fedora Linux 45, kernel 7.2.15-300.fc45.x86_64, g++ 16.2.1 20261029 (Red Hat 16.2.1-3), CMake 4.3.0, Qt 6.11.2, SQLite 3.53.4, `WAYLAND_DISPLAY=wayland-0`, `DISPLAY=:0`, `XDG_SESSION_TYPE=wayland`. Log: `validation-fixtures/slice-1b/environment.log`.

Clean Debug configure and build in `build-slice1b/` both exited 0. The configure log has no `CMake Warning` line. The build log has no compiler `warning:` or `error:` line.

CTest, from `validation-fixtures/slice-1b/ctest.log`: `Total Tests: 1`. `ctest --output-on-failure` reports 1 passed out of 1. That is one CTest test.

Qt Test, from `validation-fixtures/slice-1b/qt-test.log`, run as the same executable directly:

| Class | Totals line |
| --- | --- |
| TimingTests | 7 passed, 0 failed |
| ProjectTests | 11 passed, 0 failed |
| SequenceTests | 10 passed, 0 failed |
| TimelineWorkspaceTests | 10 passed, 0 failed |

38 Qt Test cases, 0 failed. These 38 cases are inside the one CTest test. Slice 1A cases were not removed.

TimelineWorkspaceTests called workspace methods and, for the valid move, the invalid overlap move, and both trim cases, emitted the Apply button clicked signal. They did not use a screen coordinate and did not open a media file. Covered: selection and refresh, track order, record start and duration, inspector text, valid and invalid move, valid and invalid trim, integer labels and a rejected fraction, undo and redo, a failed command with an empty undo stack, close and reopen, an empty Sequence, and an empty Track.

Smoke, from `validation-fixtures/slice-1b/application-launch.log`. No control was clicked.

| Run | Result |
| --- | --- |
| `QT_QPA_PLATFORM=wayland --shutdown-now` | `AETHER start`, `AETHER stop`, exit 0 |
| Wayland without the shutdown flag | `AETHER start`. `wmctrl` did not list AETHER. Process killed, exit 143. A native Wayland window was not confirmed |
| `QT_QPA_PLATFORM=xcb --shutdown-now` | `AETHER start`, `AETHER stop`, exit 0 |
| XCB without the shutdown flag | `wmctrl` listed `0x01200007  0 fedora AETHER`. After `wmctrl -c`, `AETHER stop` and `xcb_window_close_exit=0` |

The XCB window is not a native Wayland result.

## Known Limitations

- Native Wayland window: not confirmed.
- Smoke did not click a button, a sequence row, or a clip block.
- Qt Test did not hit a painted coordinate. Block edges were not measured in pixels.
- Drop-frame numbering is not implemented. The 59.94 skipped-number table stays open. The ruler says the drop-frame flag is absent.
- No playhead, playback, thumbnails, waveforms, track types, snapping, ripple, roll, slip, slide, or overlap editing.
- Same-track overlap remains the Slice 1A rejection. It is not a finished timeline.
- `sample_boundary.hpp` still describes ADR-002 as proposed.
- The prototype version string still says `slice-0a`.
- C++17 stays prototype-only. Cross-platform validation was not run.

## Files Created or Modified

Created: `docs/proposals/SLICE_1B_TIMELINE_WORKSPACE_PROPOSAL.md`, `docs/validation/SLICE_1B_TIMELINE_WORKSPACE.md`, `include/aether/ui/frame_label.hpp`, `include/aether/ui/timeline_workspace.hpp`, `src/ui/timeline_workspace.cpp`, `tests/timeline_workspace_tests.hpp`, `tests/timeline_workspace_tests.cpp`, `validation-fixtures/slice-1b/`.

Modified: `include/aether/ui/main_window.hpp`, `src/ui/main_window.cpp`, `src/CMakeLists.txt`, `tests/CMakeLists.txt`, `tests/main.cpp`, `DEVELOPMENT_ROADMAP.md`, `docs/19_DEVELOPMENT_ROADMAP.md`.

Not modified: schema, ADRs, `docs/decisions/ADR-006_OWNER_DECISION_RECORD.md`, media identity, and the empty `docs/DEVELOPMENT_ROADMAP.md`.

## Roadmap Status

`DEVELOPMENT_ROADMAP.md` and `docs/19_DEVELOPMENT_ROADMAP.md` record Slice 1B complete on this machine. Later slices stay **NOT STARTED** and **NOT AUTHORISED**. `docs/DEVELOPMENT_ROADMAP.md` is still empty.

## Final Slice Status

| Check | Status |
| --- | --- |
| Sequence list, track order, and clip blocks from stored ranges | **PASSED** |
| Selection and inspector do not edit | **PASSED** |
| Valid and rejected move and trim | **PASSED** |
| Failed command adds no undo entry | **PASSED** |
| Close keeps the committed edit and drops undo | **PASSED** |
| Schema, ADR status, and dependencies unchanged | **PASSED** |
| CTest: 1 test, 1 passed | **PASSED** |
| Qt Test: 38 cases, 0 failed | **PASSED** |
| Wayland and XCB `--shutdown-now` | **PASSED** |
| Native Wayland window | **NOT CONFIRMED** |
| Pointer hit on a painted clip | **NOT TESTED** |
| Playback or media processing | **NOT IMPLEMENTED** |

**SLICE 1B COMPLETE — TIMELINE WORKSPACE IMPLEMENTED AND VALIDATED — PLAYBACK AND MEDIA PROCESSING NOT YET IMPLEMENTED.**

No further production slice is authorised by this prompt.
