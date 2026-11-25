# Slice 1F — Source and Record Marks

## Repository and Research Inspection

The product is the existing Aether prototype in `/home/editry/development/sise-editor`. This slice continued the implementation already on disk. The proposal was not rewritten. It is `docs/proposals/SLICE_1F_SOURCE_RECORD_MARKS_PROPOSAL.md`.

Research under `docs/research/professional_nle/` and `docs/research/ffmpeg/` was not edited. The FFmpeg qualification report and the blocked Slice 1E preflight were not edited. ADR statuses were not changed. ADR-002 stays the half-open editorial-time authority. Schema version stays 2. No new table was added. `kPrototypeVersion` stays `slice-0a`. There is no git repository. No package was installed. FFmpeg was not linked, compiled, or installed.

`docs/DEVELOPMENT_ROADMAP.md` is still an empty stub and was not filled.

## Source Mark Model

Source marks belong to the selected representation. The commands are Mark In, Mark Out, Clear In, Clear Out, and Clear Both. The typed position is an integer labelled source frames. The readout says the file is not probed, that there is no source rate, and that there is no duration. A source rate is not guessed, and the sequence rate is not copied onto the source marks.

Switching the bin selection hides the previous representation's points and clears the typed field. Those points remain stored for that representation for the rest of the session and reappear when it is selected again.

## Record Mark Model

Record marks belong to the selected sequence. The same five commands use integers labelled sequence frames. The readout says the sequence rate is not a source rate. Switching sequence hides the previous sequence's points. They remain stored for that sequence and reappear when it is selected again.

A left click on the ruler fills the record frame field from the existing pixel-to-frame map. It does not store a mark and it does not select a clip. Mark In and Mark Out store the integer in the field. The painter draws those stored integers. Zoom and scroll do not change the stored frames. There is no zoom control.

## State Ownership and Persistence

Marks are session maps on `ProjectSession`, keyed by representation id and sequence id. They are cleared when the project is created, opened, or closed. They are not written to `project.db`. They are not undo entries. A mark command does not move or trim a timeline instance.

The disposable project used for the live window is `validation-fixtures/source-record-marks/live-parent/MarksCut`. After the live move, trim, undo, and redo, `schema_info` is still `(1, 2)`. The tables are the existing schema tables. There is no mark table. One timeline instance remains.

## Timing and Range Semantics

Internal editorial ranges stay half-open, as ADR-002 requires. A stored mark pair with Mark Out after Mark In is two ordered points. It is not `[in, out)` and it is not `[in, out+1)`. No shaded span is drawn. Three-point derivation is not run. Displayed inclusive OUT is not settled, so the interface does not invent an inclusive displayed Mark Out and does not turn the pair into an edit range.

Equal points are rejected as an incomplete range. Mark Out before Mark In is rejected. A negative or non-integer field is rejected. A missing representation or sequence is rejected. A rejection leaves the previous points unchanged and shows the reason.

## Native UI Implementation

Source controls sit in the source viewer and are labelled source frames. Record controls sit in the timeline workspace and are labelled sequence frames. Record strokes are a full-height green IN and a gold OUT. They are distinct from the clip block. An unpaintable frame stays stored and is shown as an integer without a pixel.

Mouse buttons are the commands. No keyboard shortcut was added. The running window for the live checks used `QT_SCALE_FACTOR=0.72` so the move, trim, undo, and redo row fit on the 1920×1080 display. Without that scale the same binary's lower row sits below the work area.

## Placement Integration

A valid source pair is offered in the existing placement dialog. The offer names the points, says displayed inclusive OUT is not settled, and does not copy the points into Source in or Source out. The source rate is still chosen in the dialog. Commit still uses the existing `placeInstance` path. No new edit operation was added. In the live window the dialog was opened and the offer was read. Place was not clicked, so the live session did not create a second instance.

## Undo and Redo Behaviour

Mark changes are not pushed onto the cut undo stack. Move and trim of an existing instance still use the existing session undo. On the disposable project the live window moved the seeded instance, trimmed it, undid the trim, and redid the trim. The database after those clicks was:

- after move: record `[20, 28)`, source `[0, 8)`
- after trim: record `[20, 27)`, source `[1, 8)`
- after undo: record `[20, 28)`, source `[0, 8)`
- after redo: record `[20, 27)`, source `[1, 8)`

The source marks on `a.mov` were still 10 and 18 when the placement dialog was opened after those record-mark edits. Mark edits were not the undo entries.

## Live Product Checkpoints

The native window was `Aether — MarksCut` on `QT_QPA_PLATFORM=xcb`. Clicks were XTest button events at coordinates measured from that window. Each result below was read back from the window or from `project.db`.

- Set source marks on `a.mov`: Mark In 10 source frames, Mark Out 18 source frames. The readout says the pair is not a half-open edit range.
- Clear Both on that representation: Mark In none, Mark Out none, and no source marks stored.
- The same pair was set again. Switching to `b.mov` showed a different representation id, Mark In none, and Mark Out none. Reselecting `a.mov` showed 10 and 18 again.
- On sequence Picture, record marks were stored at 5 and 40 sequence frames. The canvas showed a green IN stroke and a gold OUT stroke beside the existing clip. A measured green pixel for frame 5 was at window x 251.
- A ruler click replaced the field value and left the stored marks at 5 and 40. The field read 2, which is the frame under that click.
- Record Mark Out was then stored at 2000 so the canvas was wider than the view. A press-drag on the horizontal scrollbar moved the view. The green IN stroke left the visible canvas. The readout stayed Mark In 5 and Mark Out 2000.
- Clear Both on Picture stored no record marks. Marks 6 and 40 were stored again. Switching to Sound showed Mark In none and Mark Out none for sequence `7e371b89-5571-4a88-b582-71a68a8d043c`. Returning to Picture showed 6 and 40 again.
- A right-click on the media row opened Insert into Timeline. The placement dialog offered source marks Mark In 10, Mark Out 18, with Source in and Source out empty. Place was not clicked.
- The existing instance row was clicked. Apply move with record start 20, Apply trim with source 1 and 8 and record 20 and 27, Undo, and Redo produced the database rows in the undo section.

There is no zoom control. The scroll drag is the timeline scroll checkpoint. No click is claimed beyond this list.

A separate process, `QT_QPA_PLATFORM=wayland` with `--shutdown-now`, logged start and stop and exited 0. `wmctrl -lG` did not list a Wayland window. The only listed window was the XCB window `Aether — MarksCut`. A Wayland window is not claimed.

The XCB process was left running.

## Build and Test Results

The executable is the existing Debug binary `/home/editry/development/sise-editor/build-slice1f/src/aether`. `cmake --build build-slice1f --target aether` reported every target already built and printed no compiler warning line. This turn did not reconfigure and did not clean the tree. Earlier build directories were left in place.

CTest in `build-slice1f`: 1 test discovered, 1 passed, 0 failed. The test is `slice0a`. It passed in 2.41 seconds.

The same binary's Qt Test run exited 0. Totals, in order, with no `QWARN` line and no failure line:

- TimingTests: 7 passed, 0 failed, 0 skipped, 0 blacklisted
- ProjectTests: 11 passed, 0 failed, 0 skipped, 0 blacklisted
- SequenceTests: 10 passed, 0 failed, 0 skipped, 0 blacklisted
- TimelineWorkspaceTests: 10 passed, 0 failed, 0 skipped, 0 blacklisted
- MediaImportTests: 10 passed, 0 failed, 0 skipped, 0 blacklisted
- UiIntegrationTests: 9 passed, 0 failed, 0 skipped, 0 blacklisted
- TimelinePlacementTests: 6 passed, 0 failed, 0 skipped, 0 blacklisted
- MarkTests: 6 passed, 0 failed, 0 skipped, 0 blacklisted

That is one CTest test, not 69 CTest tests.

## Actual UI Validation

The live XCB window received the mark actions. Source set and clear, record set and clear, ruler fill, scrollbar drag, asset switch, sequence switch, the placement offer, move, trim, undo, and redo were performed with pointer events on that window. The placement offer was not submitted. The instance edits were checked in `project.db` immediately after the clicks.

The window was scaled to 0.72. The unscaled window on this 1920×1080 display does not show the move, trim, undo, and redo row. The scale does not change the stored frames or the commands.

## Screenshots and Launch Instructions

Executable: `/home/editry/development/sise-editor/build-slice1f/src/aether`

Launch used for the live window:

```
QT_SCALE_FACTOR=0.72 QT_QPA_PLATFORM=xcb /home/editry/development/sise-editor/build-slice1f/src/aether
```

The same binary without the scale factor is:

```
QT_QPA_PLATFORM=xcb /home/editry/development/sise-editor/build-slice1f/src/aether
```

Screenshots, all from the XCB window:

- `/home/editry/development/sise-editor/validation-fixtures/source-record-marks/screenshots/01-source-marks-set.png`
- `/home/editry/development/sise-editor/validation-fixtures/source-record-marks/screenshots/02-source-marks-cleared.png`
- `/home/editry/development/sise-editor/validation-fixtures/source-record-marks/screenshots/03-asset-switched.png`
- `/home/editry/development/sise-editor/validation-fixtures/source-record-marks/screenshots/04-asset-restored.png`
- `/home/editry/development/sise-editor/validation-fixtures/source-record-marks/screenshots/05-record-marks-set.png`
- `/home/editry/development/sise-editor/validation-fixtures/source-record-marks/screenshots/06-ruler-fills-field.png`
- `/home/editry/development/sise-editor/validation-fixtures/source-record-marks/screenshots/07-record-marks-scrolled.png`
- `/home/editry/development/sise-editor/validation-fixtures/source-record-marks/screenshots/08-record-marks-cleared.png`
- `/home/editry/development/sise-editor/validation-fixtures/source-record-marks/screenshots/09-sequence-switched.png`
- `/home/editry/development/sise-editor/validation-fixtures/source-record-marks/screenshots/10-picture-marks-restored.png`
- `/home/editry/development/sise-editor/validation-fixtures/source-record-marks/screenshots/11-placement-offer.png`
- `/home/editry/development/sise-editor/validation-fixtures/source-record-marks/screenshots/12-after-move.png`
- `/home/editry/development/sise-editor/validation-fixtures/source-record-marks/screenshots/13-after-trim.png`
- `/home/editry/development/sise-editor/validation-fixtures/source-record-marks/screenshots/14-after-undo.png`
- `/home/editry/development/sise-editor/validation-fixtures/source-record-marks/screenshots/15-after-redo.png`

## Known Limitations

Displayed inclusive OUT is not settled. Marks stay points. A complete pair is not presented as a half-open edit range and is not copied into the placement source span. There is no zoom control. Marks are not persisted and are not editorial undo. The file is not probed, and no source rate is invented. No keyboard map was added. The Wayland process did not appear in `wmctrl`. The live edit row was reached by scaling the same binary to fit this display. Overwrite, insert, split, lift, ripple, and playback were not started.

## Files Modified

The mark implementation was already in the session, the source viewer, the timeline workspace, the placement dialog, and `tests/mark_tests.cpp`. This close-out edited:

- `docs/validation/SLICE_1F_SOURCE_RECORD_MARKS.md`
- `DEVELOPMENT_ROADMAP.md`
- `docs/19_DEVELOPMENT_ROADMAP.md`

The proposal was not rewritten. Research documents, the FFmpeg qualification report, and the Slice 1E preflight were not edited. `docs/DEVELOPMENT_ROADMAP.md` was not filled.

## Roadmap Status

`DEVELOPMENT_ROADMAP.md` and `docs/19_DEVELOPMENT_ROADMAP.md` record Slice 1F complete on this Fedora machine. Slice 0A through Slice 1D stay complete. Overwrite, insert, split, lift, ripple, and playback stay **NOT STARTED** and **NOT AUTHORISED**. Later roadmap phases stay unauthorised. ADR statuses are unchanged.

## Final Slice Status

SLICE 1F COMPLETE — SOURCE AND RECORD MARKS IMPLEMENTED AND VALIDATED — READY FOR SAME-RATE OVERWRITE DESIGN.
