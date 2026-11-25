# Slice 1H — Split editing

## Repository inspection

The product is the existing Aether prototype in `/home/editry/development/sise-editor`. The owner decision is recorded in `docs/proposals/SLICE_1H_SPLIT_EDITING_PROPOSAL.md`. The owner chose the equal-duration, same-reduced-rate mapping on 2026-11-23. That choice is closed. This slice implemented that command. Accepted ADRs were not edited. ADR-006 and ADR-007 stay **PROPOSED**. ADR-008 stays **OPEN — PENDING LEGAL COUNSEL**. Research under `docs/research/professional_nle/` and `docs/research/ffmpeg/` was not edited. FFmpeg, `/opt/aether-ffmpeg-9.1`, RPATH, and licensing were not changed. Overwrite behaviour was not changed.

`git status` failed: `fatal: not a git repository (or any parent up to mount point /)`, exit 128. No repository was created. No package was installed. `docs/DEVELOPMENT_ROADMAP.md` is still a zero-byte stub and was not filled.

The build directory is `build-slice1h` only. `ShellCut`, `PlaceCut`, `MarksCut`, `ProbeCut`, `OverwriteCut`, and the earlier build directories were not modified.

## What the command does

Split is its own `ProjectSession` command. It is not a sequence of move, trim, or overwrite. The editor selects one instance and types one integer sequence frame. The frame is not taken from a mouse coordinate and not taken from a mark. The playhead label stays at frame 0. The split field shows the typed integer beside the field.

The record partition is `[r0, f)` and `[f, r1)`. It is valid only when `r0 < f < r1`. A split at the start or the end fails with "a split at the start or end is rejected". A frame outside the record span fails with "split frame is not inside the record span". Neither failure writes a row or an undo entry.

The source cut is `sourceSplit = s0 + (f - r0)` only when the stored source duration equals the stored record duration and the two rates are equal after reduction. `pictureSourceOffset` is not used. An unequal span or a mixed rate fails with "source span is not one frame per record frame", writes nothing, and adds no undo entry. No conversion was added for those instances.

The piece that still starts at the original record start keeps the instance id. The piece that starts at the cut is minted with `makeStableId()`. Both pieces stay on the same track, with the same Media ID and the same master object. Representation is that association. There is no representation column. There is no gap, overlap, ripple, downstream move, or sequence length change. Other tracks and marks stay as they were. Marks stay session points and off `project.db`. The write is one transaction and one undo entry of kind Split. Undo restores the original row. Redo restores both rows. Undo history is cleared when the project is closed or opened. The committed rows survive close and reopen. Schema version stays 2. `kPrototypeVersion` stays `slice-0a`. No table was added.

## Tests

Configure:

`cmake -S /home/editry/development/sise-editor -B /home/editry/development/sise-editor/build-slice1h -DCMAKE_BUILD_TYPE=Debug`

The Qt binary `build-slice1h/tests/aether_slice0a_tests` reported these groups, each with 0 failed:

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
| SplitTests | 8 |

That is 105 passed and 0 failed. Each total includes that group's init and cleanup. SplitTests has six slots: boundary and mapping refusal, interior continuity and identity, other tracks and downstream and marks, one undo with redo and reopen, a failed write that adds no undo, and one window click through the session. OverwriteTests stayed at 10. MediaProbeTests stayed at 18.

CTest in `build-slice1h` then reported:

`100% tests passed, 0 tests failed out of 2`

`Total Test time (real) = 2.31 sec`

The two CTest names are `slice0a` and `aether_ffmpeg_resolution`. `slice0a` is the Qt binary above. These are not 105 CTest tests.

## Live XCB window

The successful launch was:

`QT_QPA_PLATFORM=xcb QT_QPA_PLATFORMTHEME=none QT_SCALE_FACTOR=0.58 /home/editry/development/sise-editor/build-slice1h/src/aether`

`QT_SCALE_FACTOR=0.58` was kept. The client was 742×976. `wmctrl` moved that window up so the split row stayed inside the 1920×1080 display. `QT_QPA_PLATFORMTHEME=none` was set so Qt would not be asked for a native theme. The folder chooser was still the GTK dialog.

Button events that changed the Aether widgets were `XSendEvent` `ButtonPress` and `ButtonRelease` delivered to the XCB window. The sequence frame field received that click, then `XTestFakeKeyEvent` typed `24`. The field and the readout both showed 24. The GTK folder chooser ignored `XSendEvent` and an `XTest` click on its Open button. Opening the project folder used the accessibility activate action on that chooser once the chooser was inside the SplitCut directory. The instance selection, Split, Undo, Redo, File menu, and Close Project actions were X button events. Close Project was an X button event on the menu popup window.

The disposable project is `validation-fixtures/split-editing/live-parent/SplitCut`. It was seeded with sequence Reel at 24/1. Track 0 held record `[0, 48)` source `[10, 58)` as instance `1ce4d38b-dd23-43f7-ae89-ad9f65ef64fe`, and record `[60, 72)` source `[0, 12)` as `d802f7e4-19e7-4b20-bdd0-5261b5ceb93b`. Track 1 held record `[0, 48)` source `[5, 53)` as `04f2a710-dcca-420e-ab8f-b7ea045924c6`. Every source rate was 24/1. The media id was `c10c1ffc-abdc-4fdc-99c9-935f3aa4fa93`.

The selected row was the instance that starts at record 0. The typed frame was 24. Split produced:

- track 0, record `[0, 24)`, source `[10, 34)`, id `1ce4d38b-dd23-43f7-ae89-ad9f65ef64fe`
- track 0, record `[24, 48)`, source `[34, 58)`, id `034be483-2dc7-451e-a6c7-2e13f40e9783`
- track 0, record `[60, 72)`, source `[0, 12)`, unchanged
- track 1, record `[0, 48)`, source `[5, 53)`, unchanged

The new id starts at the cut. The original id still starts at record 0. Both pieces share the media id and the master `0cab0fc3-8324-4cbd-9ca0-76ce3cb5f455`. The sequence name stayed Reel at 24/1. The furthest record end stayed 72. The readout said no record marks were stored.

Undo returned track 0 to the original row, start 0 duration 48, and removed the new id. The later row and track 1 stayed. Redo restored the same new id and the two record spans `[0, 24)` and `[24, 48)`. Close and reopen kept those four rows. Undo after reopen did not change the list.

`schema_info` is `(1, 2)`. `project.manifest.json` has `schema_version` 2 and `prototype_version` `slice-0a`. The eleven tables are unchanged. There is no mark table and no duration column. The four instance rows above are stored at source rate 24/1.

Screenshots are in `validation-fixtures/split-editing/screenshots/`. The split, undo, redo, and reopen frames are `04-after-split.png`, `05-after-undo.png`, `06-after-redo.png`, and `07-reopened.png`.

A separate `QT_QPA_PLATFORM=wayland QT_QPA_PLATFORMTHEME=none QT_SCALE_FACTOR=0.58 ./build-slice1h/src/aether --shutdown-now` exited 0 and logged start and stop. `wmctrl -lG` before and after that launch listed `Aether — SplitCut` and no new window. No Wayland window is claimed.

## What stays unimplemented

Insert, lift, extract, ripple, roll, slip, slide, mixed-rate split, and playback were not started. Slice 1I was not started. Overwrite behaviour is unchanged. `trimInstance` can still store unequal lengths. That is trim, not split.
