# Slice 1G — Same-rate overwrite

## Repository inspection

The product is the existing Aether prototype in `/home/editry/development/sise-editor`. The owner decisions are recorded in `docs/proposals/SLICE_1G_SAME_RATE_OVERWRITE_PROPOSAL.md`. This slice implemented that command. Accepted ADRs were not edited. ADR-002 still says a displayed out-point is deferred. ADR-006 and ADR-007 stay **PROPOSED**. ADR-008 stays **OPEN — PENDING LEGAL COUNSEL**. Research under `docs/research/professional_nle/` and `docs/research/ffmpeg/` was not edited. FFmpeg, `/opt/aether-ffmpeg-9.1`, RPATH, and licensing were not changed.

`git status` failed: `fatal: not a git repository (or any parent up to mount point /)`, exit 128. No repository was created. No package was installed. `docs/DEVELOPMENT_ROADMAP.md` is still a zero-byte stub and was not filled.

The build directory is `build-slice1g` only. `ShellCut`, `PlaceCut`, `MarksCut`, `ProbeCut`, and the earlier build directories were not modified. The running ProbeCut process was left running.

## What the command does

Overwrite is its own `ProjectSession` command. It is not `placeInstance`. The editor chooses one existing track. An empty choice fails with "choose a track". The command does not create a track.

Marks stay session points. They stay off `project.db`. They are not stored as ranges. The readout still says a stored pair is not a half-open edit range. When overwrite runs, it reads the four points and derives `[In, Out + 1)`. Duration is `(Out + 1) - In`. `Out + 1` that does not fit in `int64` fails with "inclusive Mark Out does not fit" and writes nothing.

Until an editorial frame count exists, any non-negative half-open source span is allowed. A probe timestamp is not converted into frames. A record end past the current maximum `record_end` on the destination track is refused. A track with no instance has no maximum, so that overwrite is refused. Schema version stays 2. No duration column was added. Later material is not moved. The sequence duration is not a stored field and is not extended.

The derived source duration and the derived record duration must be equal in integer frames. Unequal lengths fail with "source duration and record duration differ". A different reduced rate fails with the existing `identityRecordEnd` message, "source rate and sequence rate differ; no duration mapping is authorised." There is no stretch. `24/1` with `24/1` is allowed. `24/1` with `25/1` is refused.

Leftover pieces stay. Their source frames stay on the same sequence frames. A removed head of `D` frames advances `source_start` by `D`. A removed tail retreats `source_end` by `D`. A piece wholly inside the record span is deleted. A piece past both ends becomes two pieces. One new instance is the replacement. The write is one transaction and one undo entry. An empty intersection writes the new instance and leaves every existing row unchanged. Other tracks are not read or written. A failed validation writes nothing and adds no undo. Undo history is cleared when the project is closed or opened. The committed rows survive close and reopen.

The interface is a track combo, a source-rate combo, and an Overwrite button on the timeline workspace. The button calls `ProjectSession::overwrite`. The widgets do not run SQL, call FFmpeg, or decide the ranges. No shortcut was added. The marks model was not changed. Equal stored marks are still rejected by Slice 1F, so a one-frame span cannot be expressed through a mark pair. A zero-length span is refused by the half-open check.

## Tests

Configure:

`cmake -S /home/editry/development/sise-editor -B /home/editry/development/sise-editor/build-slice1g -DCMAKE_BUILD_TYPE=Debug`

The Qt binary `build-slice1g/tests/aether_slice0a_tests` reported these groups, each with 0 failed:

| Group | Passed |
| --- | --- |
| TimingTests | 7 |
| ProjectTests | 11 |
| SequenceTests | 10 |
| TimelineWorkspaceTests | 10 |
| MediaImportTests | 10 |
| UiIntegrationTests | 9 |
| TimelinePlacementTests | 6 |
| MarkTests | 6 |
| MediaProbeTests | 18 |
| OverwriteTests | 10 |

That is 97 passed and 0 failed. OverwriteTests covers the inclusive mark reading, missing and reversed marks, a zero-length span, unequal durations, `24/1` against `25/1`, a record end past the current maximum, an empty track choice, a missing track, a missing asset, a bad representation, a track with no record end, a full cover, a removed head, a removed tail, an interior split, retained source offsets, the other track, one undo, undo, redo, close and reopen, a failed write that adds no undo, and one window click through the session.

CTest in `build-slice1g` then reported:

`100% tests passed, 0 tests failed out of 2`

`Total Test time (real) = 2.01 sec`

The two CTest names are `slice0a` and `aether_ffmpeg_resolution`. `slice0a` is the Qt binary above. These are not 97 CTest tests.

## Live XCB window

The successful launch was:

`QT_QPA_PLATFORM=xcb QT_QPA_PLATFORMTHEME=none QT_SCALE_FACTOR=0.58 /home/editry/development/sise-editor/build-slice1g/src/aether`

`QT_SCALE_FACTOR=0.58` was required. At `0.72` the client window was 922×1187 on a 1920×1080 display, and the overwrite row sat below the work area. At `0.58` the client was 742×956 and the row was on screen. `QT_QPA_PLATFORMTHEME=none` was set so Qt would not be asked for a native theme. The folder chooser was still the GTK dialog.

`XTestFakeMotionEvent` did not move the pointer. `XQueryPointer` stayed where it was. Button events that changed the Aether widgets were `XSendEvent` `ButtonPress` and `ButtonRelease` delivered to the XCB window. The GTK folder chooser ignored those events. Opening the project folder used the accessibility activate action on that chooser. The mark, track, rate, overwrite, undo, redo, inspect, close, and reopen actions were X button events on the XCB window.

The disposable project is `validation-fixtures/same-rate-overwrite/live-parent/OverwriteCut`. It was seeded with sequence Reel at 24/1, track 0 holding record `[0, 48)` source `[0, 48)`, and track 1 holding record `[0, 48)` source `[5, 53)`. Both source rates are 24/1.

In the window the source marks were set to 10 and 17, and the record marks were set to 16 and 23. The readouts still said the pairs are points and are not a half-open edit range. The track combo was set to track 0 and the rate combo to 24/1. Overwrite produced:

- track 0, record `[0, 16)`, source `[0, 16)`
- track 0, record `[16, 24)`, source `[10, 18)`
- track 0, record `[24, 48)`, source `[24, 48)`
- track 1, record `[0, 48)`, source `[5, 53)`, unchanged

The selected replacement showed source 10 and 18, record 16 and 24. Undo returned track 0 to one row, start 0 duration 48, and left track 1 at duration 48. Redo restored the three track 0 rows. Close and reopen kept those four rows. The reopened readout said no record marks were stored. Undo after reopen did not change the list.

After the second close, `schema_info` is `(1, 2)`. `project.manifest.json` has `schema_version` 2 and `prototype_version` `slice-0a`. `sequences` has no duration column. There is no mark table. The instance rows are the four spans above, each at source rate 24/1.

Screenshots are in `validation-fixtures/same-rate-overwrite/screenshots/`. The overwrite, undo, redo, reopen, and inspect frames are `08-after-overwrite.png`, `09-after-undo.png`, `10-after-redo.png`, `11-reopened.png`, and `12-reopen-inspect.png`.

A separate `QT_QPA_PLATFORM=wayland` launch with `--shutdown-now` exited 0. `wmctrl -lG` after that launch listed `Aether — ProbeCut` and no new Aether window. No Wayland window is claimed.

## What stays unimplemented

Insert, splice, ripple, split, lift, extract, roll, slip, slide, mixed-rate overwrite, and playback were not started. Slice 1H was not started. `placeInstance` still refuses overlap. `trimInstance` can still store unequal lengths. That is trim, not overwrite.
