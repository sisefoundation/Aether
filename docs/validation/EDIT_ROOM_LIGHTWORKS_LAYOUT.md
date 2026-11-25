# Edit room layout

Date: 2026-11-23. Fedora, Qt 6.11.2, display `:0`, 1920×1080. Original Qt widgets. The Aether mark already in the repo is the one painted. No Lightworks logo, icons, or code.

Build directory: `/home/editry/development/sise-editor/build-edit-room`. Existing `build-*` directories were left in place. Schema stays `(1, 2)`. `prototype_version` stays `slice-0a`. No new `project.db` table and no duration column. ADR-008 was not edited and is not accepted.

## Launch

Disposable project, created through `ProjectSession` and then opened in the running window:

`/home/editry/development/sise-editor/validation-fixtures/edit-room-lightworks/live-parent/EditRoom`

```
GTK_IM_MODULE=gtk-im-context-simple QT_IM_MODULE=none QT_QPA_PLATFORM=xcb QT_QPA_PLATFORMTHEME=none QT_SCALE_FACTOR=0.58 /home/editry/development/sise-editor/build-edit-room/src/aether
```

`wmctrl` showed `0x01200007` at `0,106`, size `1920x1011`, title `Aether — EditRoom`. That process is still running in tmux session `aether-edit-room`.

The project holds sequence EditRoom at 24/1, two tracks, one reference `room-clip.mov` (25 bytes, not a picture), and two instances. Furthest stored `record_end` is 24. The manifest reads `schema_version` 2 and `prototype_version` `slice-0a`.

## What the window shows

Edit is the four panels. Effects reads "There is no effects engine." Audio reads "There are no sound tools. The sound build shape stays open." Export reads "There is no exporter." Each of those rooms sets the status to "This room is not available."

The clip row has no thumbnail. Settings names only the stored sequence rate and says raster, pixel aspect, colour, and stereoscopic values are not stored. Info shows the stored sequence rate and the selected reference. Sequence Cues reads "No sequence cues are stored." The Export tab reads "Export does nothing."

The viewer timecode is `00:00:00:00`, from frame 0 at 24/1. The ruler caption still says the drop-frame flag is absent. Transport buttons are drawn. A later X click on Play set the status to "Playback is not available." and left both timecode labels at `00:00:00:00`.

Insert and Ripple Delete are visible and disabled. Overwrite, Split, and Lift are the enabled timeline commands. The timeline menu's instance list still uses the existing row text (`track N start N duration N`). That is not a duration column and it is not stored as one.

## Live clicks

Room tabs were `XSendEvent` `ButtonPress` and `ButtonRelease` on `0x01200007`. After each one, AT-SPI reported that room checked. `XTestFakeMotionEvent` did not move the pointer. Panel menus and Play were an `XWarpPointer` onto the control followed by `XTest` button press and release.

Each panel menu opened as its own override-redirect window: Project Contents `210x173`, Sequence viewer `254x362`, Side panel `116x17` (the disabled "No panel commands" item), Timeline `404x224`.

One release while the Project Contents menu was open activated Probe. FFmpeg, on stderr, reported `moov atom not found` for the 25-byte `room-clip.mov`. The menu shows that failure, including "No verified source rate" and "Probe metadata is not editorial timing." The same `project.db` snapshot taken before any room click still matched after the rooms, the menus, Probe, and Play. `schema_info` stayed `(1, 2)`. No editorial row was written.

## Screenshots

`import -window` of the X11 window. The menu shot is the popup window, which is not part of the main window.

| Path | What it is |
| --- | --- |
| `/home/editry/development/sise-editor/validation-fixtures/edit-room-lightworks/screenshots/edit.png` | Edit room, project open, status Open |
| `/home/editry/development/sise-editor/validation-fixtures/edit-room-lightworks/screenshots/effects.png` | Effects empty state |
| `/home/editry/development/sise-editor/validation-fixtures/edit-room-lightworks/screenshots/audio.png` | Audio empty state |
| `/home/editry/development/sise-editor/validation-fixtures/edit-room-lightworks/screenshots/export.png` | Export empty state |
| `/home/editry/development/sise-editor/validation-fixtures/edit-room-lightworks/screenshots/project-menu.png` | Project Contents menu, including the real probe failure |

## Tests

`QT_QPA_PLATFORM=offscreen ctest --output-on-failure` in `build-edit-room`:

```
Test project /home/editry/development/sise-editor/build-edit-room
    Start 1: slice0a
1/2 Test #1: slice0a ..........................   Passed    2.61 sec
    Start 2: aether_ffmpeg_resolution
2/2 Test #2: aether_ffmpeg_resolution .........   Passed    0.02 sec

100% tests passed, 0 tests failed out of 2

Total Test time (real) =   2.64 sec
```

Qt totals from the same `aether_slice0a_tests` binary, run directly so the per-suite lines are visible. These 113 passes are Qt test functions, not 113 CTest tests.

```
Totals: 7 passed, 0 failed, 0 skipped, 0 blacklisted, 67ms
Totals: 11 passed, 0 failed, 0 skipped, 0 blacklisted, 44ms
Totals: 10 passed, 0 failed, 0 skipped, 0 blacklisted, 33ms
Totals: 10 passed, 0 failed, 0 skipped, 0 blacklisted, 113ms
Totals: 10 passed, 0 failed, 0 skipped, 0 blacklisted, 598ms
Totals: 9 passed, 0 failed, 0 skipped, 0 blacklisted, 662ms
Totals: 6 passed, 0 failed, 0 skipped, 0 blacklisted, 283ms
Totals: 6 passed, 0 failed, 0 skipped, 0 blacklisted, 212ms
Totals: 18 passed, 0 failed, 0 skipped, 0 blacklisted, 388ms
Totals: 10 passed, 0 failed, 0 skipped, 0 blacklisted, 72ms
Totals: 8 passed, 0 failed, 0 skipped, 0 blacklisted, 63ms
Totals: 8 passed, 0 failed, 0 skipped, 0 blacklisted, 72ms
```

Order: TimingTests, ProjectTests, SequenceTests, TimelineWorkspaceTests, MediaImportTests, UiIntegrationTests, TimelinePlacementTests, MarkTests, MediaProbeTests, OverwriteTests, SplitTests, LiftTests.

## Timeline headers and overview

Same tree, same `build-edit-room` binary, rebuilt after the track headers and the bottom strip were added. No schema, ADR, FFmpeg, or git change. EditRoom's instance rows were not edited: they are still record `[0, 24)` and `[8, 20)`.

Track headers sit to the left of the canvas. Each stored track shows an enable mark, a lock mark, the stored name `track N`, and a view mark. Even ordinals are drawn as V, odd ordinals as A. That mark is not a stored kind. Clicks on enable and lock do nothing. Clip blocks stay flat, with the file name and the stored record span. There is no waveform and no duration column. The red playhead is drawn at frame 0. Dragging it does not move it.

The strip under the timeline draws the whole sequence and a bracket for the range currently in view. Dragging that strip or the zoom slider changes only scroll and the on-screen scale. The default scale is still 4 pixels per frame.

After EditRoom was open, X button events hit enable, lock, the playhead, the overview strip, and the zoom slider. The `project.db` snapshot taken after open still matched. `schema_info` stayed `(1, 2)`.

| Path | What it is |
| --- | --- |
| `/home/editry/development/sise-editor/validation-fixtures/edit-room-lightworks/screenshots/timeline-navigator.png` | Edit room after the header and strip work, window `Aether — EditRoom` |
| `/home/editry/development/sise-editor/validation-fixtures/edit-room-lightworks/screenshots/timeline-strip.png` | Bottom of that window: tracks, playhead, and the overview strip |

`QT_QPA_PLATFORM=offscreen ctest --output-on-failure` in `build-edit-room` after this rebuild:

```
Test project /home/editry/development/sise-editor/build-edit-room
    Start 1: slice0a
1/2 Test #1: slice0a ..........................   Passed    1.53 sec
    Start 2: aether_ffmpeg_resolution
2/2 Test #2: aether_ffmpeg_resolution .........   Passed    0.02 sec

100% tests passed, 0 tests failed out of 2

Total Test time (real) =   1.55 sec
```

Qt totals from the same `aether_slice0a_tests` binary, run directly. TimelineWorkspaceTests is 11 because `viewControlsDoNotWriteTheCut` was added. These 114 passes are Qt test functions, not 114 CTest tests.

```
Totals: 7 passed, 0 failed, 0 skipped, 0 blacklisted, 65ms
Totals: 11 passed, 0 failed, 0 skipped, 0 blacklisted, 46ms
Totals: 10 passed, 0 failed, 0 skipped, 0 blacklisted, 34ms
Totals: 11 passed, 0 failed, 0 skipped, 0 blacklisted, 132ms
Totals: 10 passed, 0 failed, 0 skipped, 0 blacklisted, 632ms
Totals: 9 passed, 0 failed, 0 skipped, 0 blacklisted, 778ms
Totals: 6 passed, 0 failed, 0 skipped, 0 blacklisted, 199ms
Totals: 6 passed, 0 failed, 0 skipped, 0 blacklisted, 120ms
Totals: 18 passed, 0 failed, 0 skipped, 0 blacklisted, 336ms
Totals: 10 passed, 0 failed, 0 skipped, 0 blacklisted, 73ms
Totals: 8 passed, 0 failed, 0 skipped, 0 blacklisted, 66ms
Totals: 8 passed, 0 failed, 0 skipped, 0 blacklisted, 71ms
```

Order: TimingTests, ProjectTests, SequenceTests, TimelineWorkspaceTests, MediaImportTests, UiIntegrationTests, TimelinePlacementTests, MarkTests, MediaProbeTests, OverwriteTests, SplitTests, LiftTests.

## Source and record tabs

Same tree and the same `build-edit-room` binary, rebuilt for the viewer tabs, the larger type, and the shorter toolbar. No schema, ADR, FFmpeg, or git change. EditRoom was opened again. Its instance rows were not edited.

Source and record are two tabs on the one viewer. Source is on the left. The record tab is the open sequence. Double-clicking `room-clip.mov` loaded that name on the source tab and selected it. The source picture stayed black, with the line "Not probed. No source rate. No duration." Double-clicking that same clip again selected the record tab, `EditRoom  24/1`. Marks on each tab are the existing session points. They are not written to `project.db`.

The source tab and the record tab show Mark In and Mark Out. There is no play, shuttle, or step button. The timeline toolbar is Mark In, Mark Out, Overwrite, Split, and Lift. Insert and Ripple Delete are no longer on that bar. Enable and lock are still drawn and still do nothing. Overwrite, Split, and Lift are the same commands as before. The clip menu is the bin row's "Insert into Timeline" action. It was opened and not chosen.

The `project.db` snapshot taken after this open and before the double-clicks still matched after both double-clicks and after the menu was dismissed. SHA-256 `de65246061a1f3894d5e1fa17fac43700b980fc9ff83bdad532ea5963bace375`. Instance rows stayed record `[0, 24)` and `[8, 20)`. `schema_info` stayed `(1, 2)`. The window is still `0x01200007`, title `Aether — EditRoom`, in tmux session `aether-edit-room`.

| Path | What it is |
| --- | --- |
| `/home/editry/development/sise-editor/validation-fixtures/edit-room-lightworks/screenshots/source-tab.png` | Source tab after double-clicking `room-clip.mov`. Black viewer, Mark In, Mark Out |
| `/home/editry/development/sise-editor/validation-fixtures/edit-room-lightworks/screenshots/record-tab.png` | Record tab after double-clicking that same clip again |
| `/home/editry/development/sise-editor/validation-fixtures/edit-room-lightworks/screenshots/clip-menu.png` | Open clip menu, "Insert into Timeline", not activated |
| `/home/editry/development/sise-editor/validation-fixtures/edit-room-lightworks/screenshots/timeline-toolbar.png` | Timeline bar: Mark In, Mark Out, Overwrite, Split, Lift |

`ctest --output-on-failure` in `build-edit-room` after this rebuild:

```
Test project /home/editry/development/sise-editor/build-edit-room
    Start 1: slice0a
1/2 Test #1: slice0a ..........................   Passed    2.94 sec
    Start 2: aether_ffmpeg_resolution
2/2 Test #2: aether_ffmpeg_resolution .........   Passed    0.02 sec

100% tests passed, 0 tests failed out of 2

Total Test time (real) =   2.96 sec
```

Qt totals from the same `aether_slice0a_tests` binary, run directly so the per-suite lines are visible. MarkTests is 7 because `doubleClickLoadsSourceWithoutWriting` was added. These 115 passes are Qt test functions, not 115 CTest tests.

```
Totals: 7 passed, 0 failed, 0 skipped, 0 blacklisted, 49ms
Totals: 11 passed, 0 failed, 0 skipped, 0 blacklisted, 36ms
Totals: 10 passed, 0 failed, 0 skipped, 0 blacklisted, 25ms
Totals: 11 passed, 0 failed, 0 skipped, 0 blacklisted, 125ms
Totals: 10 passed, 0 failed, 0 skipped, 0 blacklisted, 413ms
Totals: 9 passed, 0 failed, 0 skipped, 0 blacklisted, 726ms
Totals: 6 passed, 0 failed, 0 skipped, 0 blacklisted, 334ms
Totals: 7 passed, 0 failed, 0 skipped, 0 blacklisted, 325ms
Totals: 18 passed, 0 failed, 0 skipped, 0 blacklisted, 423ms
Totals: 10 passed, 0 failed, 0 skipped, 0 blacklisted, 133ms
Totals: 8 passed, 0 failed, 0 skipped, 0 blacklisted, 112ms
Totals: 8 passed, 0 failed, 0 skipped, 0 blacklisted, 132ms
```

Order: TimingTests, ProjectTests, SequenceTests, TimelineWorkspaceTests, MediaImportTests, UiIntegrationTests, TimelinePlacementTests, MarkTests, MediaProbeTests, OverwriteTests, SplitTests, LiftTests.

## Not invented

No playback and no playhead motion. No waveforms, thumbnails, codecs, or durations treated as editorial timing. No video or audio track kind. No raster, pixel aspect, colour, or stereoscopic values. No effects engine, sound tools, or exporter. The timeline bar does not call `insertInstance`, and it has no ripple, extract, roll, slip, or slide command. The bin menu's "Insert into Timeline" still starts the existing place path. Probe results and marks stay out of `project.db`. No package was installed. FFmpeg, RPATH, licensing, and ADR files were not changed. No git commit.
