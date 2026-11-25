# Slice 1I — Lift editing

## Repository inspection

The product is the existing Aether prototype in `/home/editry/development/sise-editor`. The owner decision is recorded in `docs/proposals/SLICE_1I_LIFT_EDITING_PROPOSAL.md`. The timeline selection is one instance. There is no range selection, so lift was not stopped for a second selection authority. This slice implemented that command. Accepted ADRs were not edited. ADR-006 and ADR-007 stay **PROPOSED**. ADR-008 stays **OPEN — PENDING LEGAL COUNSEL**. Research under `docs/research/professional_nle/` and `docs/research/ffmpeg/` was not edited. FFmpeg, `/opt/aether-ffmpeg-9.1`, RPATH, and licensing were not changed. Overwrite and split behaviour were not changed.

`git status` failed: `fatal: not a git repository (or any parent up to mount point /)`, exit 128. No repository was created. No commit, no push, and no package was installed. `docs/DEVELOPMENT_ROADMAP.md` is still a zero-byte stub and was not filled.

The build directory is `build-slice1i` only. `ShellCut`, `PlaceCut`, `MarksCut`, `ProbeCut`, `OverwriteCut`, `SplitCut`, the earlier validation screenshots, and the earlier build directories were not modified.

## What the command does

Lift is its own `ProjectSession` command and its own `UndoKind::Lift`. It is not split, delete, overwrite, move, or trim. The editor selects one timeline instance. The Lift button has no shortcut. An empty selection explains "no instance is selected" and does not call the session. Marks are not read, modified, persisted, cleared, or consumed. A mark pair is not the lift range.

The command reads that instance and deletes its `timeline_instances` row inside the existing `mutate()` transaction. The record interval `[r0, r1)` is then empty because no row covers it. No Gap entity, placeholder clip, black-media asset, or synthetic instance is written. Downstream and preceding instances stay where they are. Other tracks stay. The sequence row is not written, so the stored name and rate stay. There is no duration column. No floating-point seconds are used.

The undo entry is pushed only after that delete succeeds. A failed read or a failed delete returns before `redo_` is cleared and before the undo push, so a failure writes nothing and adds no undo. Undo calls `reinsertInstance` with the captured instance, which keeps the original id, track, media, master, source rate, and both ranges. `created_utc` and `modified_utc` are stamped again by that existing insert path. Redo deletes that same id. Undo history is cleared when the project is closed or opened. The committed delete survives close and reopen. Schema version stays 2. `kPrototypeVersion` stays `slice-0a`. No table was added.

## Tests

Configure:

`cmake -S /home/editry/development/sise-editor -B /home/editry/development/sise-editor/build-slice1i -DCMAKE_BUILD_TYPE=Debug`

The Qt binary `build-slice1i/tests/aether_slice0a_tests` reported these groups, each with 0 failed:

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
| LiftTests | 8 |

That is 113 passed and 0 failed. Each total includes that group's init and cleanup. LiftTests has six slots: selection refusal, first last only middle and adjacent, a post-split empty interval, one undo with redo and reopen, a failed write that adds no undo, and one window click through the session. SplitTests stayed at 8. OverwriteTests stayed at 10. MediaProbeTests stayed at 18.

CTest in `build-slice1i` then reported:

`100% tests passed, 0 tests failed out of 2`

`Total Test time (real) = 3.30 sec`

The two CTest names are `slice0a` and `aether_ffmpeg_resolution`. `slice0a` is the Qt binary above and passed in 3.27 sec. `aether_ffmpeg_resolution` passed in 0.02 sec. These are not 113 CTest tests.

## Live XCB window

The successful launch was:

`GTK_IM_MODULE=gtk-im-context-simple QT_IM_MODULE=none QT_QPA_PLATFORM=xcb QT_QPA_PLATFORMTHEME=none QT_SCALE_FACTOR=0.58 /home/editry/development/sise-editor/build-slice1i/src/aether`

`QT_SCALE_FACTOR=0.58` was kept. The client started at 742×996 on a 1920×1080 display. `wmctrl` listed that window as `Aether — LiftCut`. `QT_QPA_PLATFORMTHEME=none` was set so Qt would not be asked for a native theme. The input-module variables were set so the GTK location field would not go through ibus. The folder chooser was still the GTK dialog.

Button events that changed the Aether widgets were `XSendEvent` `ButtonPress` and `ButtonRelease` delivered to the XCB window. That selected the instance list row, clicked Lift, Undo, and Redo, opened File, and later chose Existing projects and Open Project. Close Project was an `XSendEvent` click on the menu popup window. The GTK chooser ignored folder-row clicks. Its location entry was revealed with the chooser's `show_location` accessibility action, focused with an `XTest` click, and filled with `XTestFakeKeyEvent`. Key autorepeat was off during that typing because a stuck `e` otherwise filled the field, and it was turned back on afterwards. Return left the LiftCut path in the entry. A second Return, with that entry focused, opened the project. The window title became `Aether — LiftCut`.

The disposable project is `validation-fixtures/lift-editing/live-parent/LiftCut`. It was seeded with sequence Reel at 24/1, id `1807753c-2c53-45f4-a24e-81b36dec6ac4`. Track 0, id `8a888e78-173b-4167-bd6a-0eb4e993b977`, held three instances. Track 1, id `d649ff60-8bdf-4889-a807-3ae70064c541`, held one. Every source rate was 24/1. The media id was `b3603f05-42d0-4260-aefc-061f285a1450`. The master was `c63775fc-37cc-42a3-84f4-2d5589716196`.

The selected row was track 0 record `[24, 48)`, source `[0, 24)`, id `0364b345-bd8f-4a30-ad43-a607ce8bfcf1`. Lift removed that row and left the interval empty. No replacement row was written. The other rows stayed:

- track 0, record `[0, 24)`, source `[0, 24)`, id `0c169990-e99f-4e1d-8f3e-a8afe93460dc`
- track 0, record `[60, 72)`, source `[0, 12)`, id `df359aa0-cf49-4199-b719-224dfedbf025`
- track 1, record `[0, 48)`, source `[0, 48)`, id `184d3dbd-4f5c-477b-9908-0805c0183a90`

The sequence name stayed Reel at 24/1. The furthest stored record end stayed 72. The readout said no record marks were stored. The selection cleared.

Undo restored id `0364b345-bd8f-4a30-ad43-a607ce8bfcf1` on the same track with record `[24, 48)` and source `[0, 24)`. Redo deleted that same id. Close Project and opening LiftCut again left those three rows. Undo after reopen did not put the lifted id back.

`schema_info` is `(1, 2)`. `project.manifest.json` has `schema_version` 2 and `prototype_version` `slice-0a`. The eleven tables are unchanged. There is no mark table and no duration column. The three instance rows above are stored at source rate 24/1.

Screenshots are in `validation-fixtures/lift-editing/screenshots/`. The open, selected, lifted, undo, redo, and reopened frames are `01-open.png`, `02-selected.png`, `03-lifted.png`, `04-undo.png`, `05-redo.png`, and `06-reopened.png`.

A separate `QT_QPA_PLATFORM=wayland QT_QPA_PLATFORMTHEME=none QT_SCALE_FACTOR=0.58 ./build-slice1i/src/aether --shutdown-now` exited 0 and logged start and stop. `wmctrl -lG` before, during, and after that launch listed `Aether — LiftCut` and no new window. No Wayland window is claimed.

## What stays unimplemented

Extract, ripple, insert, splice, roll, slip, slide, multi-track lift, mark-based range lift, patching, sync lock, gap closing, mixed rate, speed, and playback were not started. Slice 1J was not started. Overwrite and split behaviour are unchanged.
