# Slice 1D timeline clip placement

Date: 2026-11-22. Machine: Fedora, Qt 6.11.2, X11 display `:0`. The window was launched with `QT_QPA_PLATFORM=xcb`.

The product name shown on the window is Aether. The repository, CMake targets, namespaces, and existing AETHER identifiers were not renamed. The closed window title is `Aether`. With a project open it is `Aether — {name}`. The mark from `resources/aether-logo-on-dark.png` is the window icon and the small mark in the existing title row. `resources/aether-logo-dark-wordmark.png` is compiled into `resources/aether.qrc` as the light-surface alternate and is not painted on the black workspace. Both files match the owner's originals byte for byte. No splash screen and no new panel were added.

This slice places one registered bin reference on a sequence through the existing workspace. It does not redesign the window. Playback, decoding, a probe, thumbnails, waveforms, relink, conform, ripple, roll, slip, slide, overlap, snapping, subframe edits, and speed changes stay out. ADR-006 stays **PROPOSED**. Schema version stays 2. `kPrototypeVersion` stays `slice-0a`. The link stays `SQLite3::SQLite3`. Widgets do not write SQL.

The proposal is `docs/proposals/SLICE_1D_TIMELINE_CLIP_PLACEMENT_PROPOSAL.md`. It was written before the placement source changes.

Executable: `/home/editry/development/sise-editor/build-slice1d/src/aether`

Launch: `QT_QPA_PLATFORM=xcb /home/editry/development/sise-editor/build-slice1d/src/aether`

The XCB window was left running on PlaceCut after the checks below. A separate process with `QT_QPA_PLATFORM=wayland` logged `AETHER start`. `wmctrl` did not list a window for it. No Wayland pass is claimed. That process was stopped. An older tmux session named `ui-wayland` was not used as this result.

## What is reused

`insertInstance` still stores a source range and a record range independently. `validateInstancePlacement` was not tightened and does not require equal lengths. `pictureSourceOffset` is a timing query. It is not the rule that sizes a new clip. Trim can still store unequal spans. An existing sequence still has no track until one is stored.

## Placement

The Media list can be dragged. Its context action is `Insert into Timeline`. Both call `TimelineWorkspace::beginPlacement`, which opens `PlacementDialog`, and the dialog calls `ProjectSession::placeInstance`. Insert prefills record frame 0. A drop prefills the frame from the existing pixel mapping: origin 48, 4 pixels per frame, `floor((x - origin) / pixelsPerFrame)`. A pointer before frame 0 is refused. The ruler and a point past the last row are refused. An empty timeline accepts a drop below the ruler and creates one track as part of that place.

The live drop was performed on the XCB window. `place.mov` was dragged from the Media list onto the empty timeline. The dialog opened with record in `2`, the file labelled `Not probed`, and the source rate still on `Choose a source rate`. Choosing `25/1` with source frames 0 and 8 showed `source rate and sequence rate differ; no duration mapping is authorised`. Place wrote no instance and no track. Choosing `24/1` with the same source frames showed `Record [2,10)` and Place stored the instance.

The first stored row was source `[0,8)` at `24/1` and record `[2,10)`. Media id `8d2fe67b-8287-4947-8fca-13c39875e082`. One track, ordinal 0. The block was selected and both inspectors showed those ranges.

`identityRecordEnd` accepts a source span only when both rates are accepted and the same reduced rational. The record span is then `[recordStart, recordStart + sourceLength)`. A different rate is refused. A record end that is not that span is refused by `placeInstance` before a track or a row is created, and no undo is added. `insertInstance` itself is unchanged.

## Edits, undo, and persistence

On the same window the block was clicked, moved to record start 20, then trimmed to source `[1,8)` and record `[20,27)`. Three Undo clicks removed the trim, the move, and the placement. The database then had 0 instances and 0 tracks. Three Redo clicks restored source `[1,8)`, record `[20,27)`, rate `24/1`, and the one track.

Close Project, then Open Project on `validation-fixtures/timeline-placement/live-parent/PlaceCut`, kept that row. Undo and Redo were not sensitive after reopen. Clicking the block showed source `[1,8)` and record `[20,27)`. `place.mov` is still 23 bytes. The fixture text is `aether-slice1d-fixture`.

ShellCut was not opened. Its sequence is still `ShellCut` at `24/1`, with 0 timeline instances, 0 media assets, and `project.db` of 102400 bytes.

## Screenshots

- `validation-fixtures/timeline-placement/screenshots/01-workspace-before-insert.png`
- `validation-fixtures/timeline-placement/screenshots/02-placement-dialog.png`
- `validation-fixtures/timeline-placement/screenshots/03-source-rate-refused.png`
- `validation-fixtures/timeline-placement/screenshots/04-first-insertion.png`
- `validation-fixtures/timeline-placement/screenshots/05-after-move-and-trim.png`
- `validation-fixtures/timeline-placement/screenshots/06-reopened.png`

## Build and tests

CTest in `build-slice1d`: 1/1 `slice0a` Passed, 1.32 sec. 100% tests passed, 0 tests failed out of 1.

Qt Test totals inside that one test, including init and cleanup:

- TimingTests: 7 passed, 0 failed, 0 skipped, 0 blacklisted
- ProjectTests: 11 passed, 0 failed, 0 skipped, 0 blacklisted
- SequenceTests: 10 passed, 0 failed, 0 skipped, 0 blacklisted
- TimelineWorkspaceTests: 10 passed, 0 failed, 0 skipped, 0 blacklisted
- MediaImportTests: 10 passed, 0 failed, 0 skipped, 0 blacklisted
- UiIntegrationTests: 9 passed, 0 failed, 0 skipped, 0 blacklisted
- TimelinePlacementTests: 6 passed, 0 failed, 0 skipped, 0 blacklisted

The first six counts are unchanged. TimelinePlacementTests is the new group: four cases plus init and cleanup.

## Limitations

Imported media stays `Not probed`. The dialog does not invent a duration from the file size, name, extension, or the sequence rate. The editor types a source rate and a source span. A rate that is not the sequence rate is refused. That is not a duration mapping and not a speed change. Same-track overlap stays rejected. There is no track type, insert edit, ripple, zoom control, or viewer picture. Transport still only says playback is not available.

**SLICE 1D COMPLETE — BIN-TO-TIMELINE CLIP PLACEMENT IMPLEMENTED — FIRST EDITORIAL ASSEMBLY WORKFLOW VALIDATED.**
