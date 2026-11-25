# Slice 1J — Extract editing

## Repository inspection

The product is the existing Aether prototype in `/home/editry/development/sise-editor`. Owner decisions in `docs/proposals/SLICE_1J_EXTRACT_EDITING_PROPOSAL.md` were left as recorded. An implementation note at the end of that file points here. ADR-006 and ADR-007 stay **PROPOSED**. ADR-008 stays **OPEN — PENDING LEGAL COUNSEL**. Research notes, FFmpeg, `/opt/aether-ffmpeg-9.1`, RPATH, and licensing were not changed. `ShellCut`, `PlaceCut`, `MarksCut`, `ProbeCut`, `OverwriteCut`, `SplitCut`, and `LiftCut` were not edited.

`git status` is not a repository (`fatal: not a git repository`, exit 128). No repository was created. No commit and no push.

The build directory is `build-slice1j` only. Older `build-*` directories were not deleted or reused. The disposable project is `validation-fixtures/extract-editing/live-parent/ExtractCut`. EditRoom was not edited. Its window was left running.

## Implemented behaviour

Extract is one `ProjectSession::extract` command and one `UndoKind::Extract`. It is not a chain of lift, split, move, or trim. The session receives the sequence id, the track ids the editor named, and the half-open record interval `[r0, r1)`. `D` is `r1 - r0`.

`planExtract` classifies each row on those tracks before any write. A crossing row is cut only when `planSplit`'s gate passes: the same reduced rate, and the source duration equals the record duration. The source frame is `sourceStart + (frame - recordStart)`. `pictureSourceOffset` is not called. An uncut unequal or mixed-rate row may be deleted or shifted. Its source span stays on a shift.

The write is `ProjectStore::commitMultiTrackEdits`: every named track is validated, then one `mutate()` transaction deletes, updates, and inserts. A failure rolls that transaction back. The undo entry is pushed only after the write succeeds. A failed preflight returns before that call, so it writes nothing and adds no undo entry. Marks are not read or stored.

The session range is separate from the command arguments. `setExtractRange` stores `[r0, r1)` in the session only. It is not a `project.db` row. Project close clears it through `resetSessionState`. Changing the displayed sequence clears it from the timeline. A refused extract leaves the stored range. A successful extract clears it. Marks stay as they were.

## Range UI

The timeline has a checkable **Range** control (`extractRangeMode`), integer fields `extractRangeStart` and `extractRangeEnd`, and the readout `extractRangeReadout`. Range mode is off until the editor turns it on, so a ruler click still fills the record-mark field. With Range on, a canvas click uses `frameAtCanvasX` and does not select an instance or write a mark. The first click is the inclusive start. The next click is the exclusive end. The readout is `Start N inclusive. End N exclusive. D N.` A reversed pair is refused and does not replace a range already stored. The canvas paints a band between the two frames. That paint is not the stored visible end.

## Track selection

There is no extract checklist. Participation is the track name in `timelineTrackHeaders`. A left click on that name toggles the stable track id. The name stays the label `track N`. A selected name is filled. Enable, lock, and the V/A mark do not toggle participation. A clip click does not. A record-range click does not. Nothing is selected from the selected instance or from the overwrite combo. A sequence change clears the set. Extract sends only those ids, in header order. No selection, a duplicate id, or a track that is not on the sequence is refused before a write. Tracks that are not selected are not in the plan and are not written.

## Transformation

On each named track, half-open:

| Row | Result |
| --- | --- |
| `record_end <= r0`, including a row that ends on `r0` | Unchanged |
| `record_start >= r1`, including a row that starts on `r1` | Both record ends move by `-D`. Id, source span, media, master, and track stay |
| `r0 <= record_start` and `record_end <= r1` | Deleted |
| Starts before `r0` and ends inside, including a row that ends on `r1` | One piece ending at `r0`. Same id. Source end is `sourceStart + (r0 - recordStart)` |
| Starts on `r0` and ends after `r1` | One piece. Record start stays `r0`, so the id stays. Source start is `sourceStart + (r1 - recordStart)`. Record end is `r0 + (recordEnd - r1)` |
| Starts inside `(r0, r1)` and ends after `r1` | One piece on `[r0, r0 + (recordEnd - r1))`. New id. The original row is deleted |
| Starts before `r0` and ends after `r1` | Left piece keeps the id and ends at `r0`. Right piece starts at `r0` and is a new id. The source frames inside the interval are on neither piece |

A zero-length piece refuses the whole extract. No placeholder, gap object, or fake media is written.

## Empty interval

An interval that meets no instance on a named track is legal. Later rows on that track still move earlier by `D`. On ExtractCut, `[48, 52)` (`D` 4) on track 0 left `[8, 48)` in place and moved `[56, 72)` to `[52, 68)` with the same id and the same source `[0, 16)`. Track 1 and the unequal row on track 2 stayed. Visible end went from 72 to 68. Undo put `[56, 72)` back and the visible end returned to 72.

## Unequal rejection

A crossing unequal or mixed-rate row refuses the whole extract before any write. The message names the track id and the instance id and says the source span is not one frame per record frame. Every named track is planned first, so a later track cannot be left half-applied.

On ExtractCut, track 2 holds instance `59edf682-e660-405f-ba3b-30afebc07a11`, source `[0, 10)` at 24/1 and record `[10, 40)`. Range `[20, 30)` with only that track checked returned:

`Track 03495db9-c6a6-49c7-a13e-f4ac7a16d38b instance 59edf682-e660-405f-ba3b-30afebc07a11 crosses the extract interval and its source span is not one frame per record frame.`

The `project.db` SHA-256 stayed `39195173514e0b315cacf4fd863ea530e3043876d67dfe92e5b64970c14cc3b7`. The five instance rows stayed. The session range stayed `[20, 30)`. Undo and Redo in the timeline menu were grey. An uncut unequal row is a different case: the tests delete one that sits inside the interval and shift one that starts at `r1`, and the source span is unchanged on the shift.

## Visible end

Visible end is the maximum stored `record_end`, or none when the sequence has no instances. There is no duration column. The widget floor of 24 frames and a record mark that widens the canvas are not this end. The label is `visibleEnd`.

A successful extract whose participating track held the previous maximum shortens that maximum by `D` when no unlisted row ends later than `oldEnd - D`. ExtractCut went from 72 to 56 on `[24, 40)` (`D` 16). If an unlisted instance would extend past `oldEnd - D`, the command refuses, names that track and instance, and does not move or truncate it. A named row that is wholly before the interval can remain past `oldEnd - D`. An empty interval whose furthest row is wholly before the interval does not shorten the end. Exact reduction by `D` is not a universal success rule. The corrective section below names `candidateEnd` as the unselected overflow check and keeps the visible end as the furthest stored `record_end`.

## Identity

A whole instance moved by `-D` keeps its id. A fragment that still starts at the original record start keeps the id. A fragment that starts at the cut is minted with `makeStableId()`. Redo writes that stored id again and does not mint another. One retained fragment is never a zero-length row.

ExtractCut track 0, range `[24, 40)`:

| Id | After |
| --- | --- |
| `4e6cb513-99ba-446a-9e68-7119fe9ed141` | Kept. Record `[8, 24)`, source `[0, 16)` |
| `9c8f6dae-b32b-47ab-a381-1442e0730ccc` | Minted. Record `[24, 32)`, source `[32, 40)` |
| `ea74cb64-075a-4f38-99f5-59eaf242ad35` | Kept. Record `[40, 56)`, source `[0, 16)` |

Track 1 `64140cb0-5314-4a80-81c3-2b0fc7f9e1d4` stayed record `[0, 30)`. Undo removed the minted id and restored `[8, 48)` and `[56, 72)`. Redo put the same minted id back.

A minted id that already exists refuses the extract before the write. `debugArmNextMutationFailure` rolls the transaction back and adds no undo entry. The window does not call that seam.

## Files changed

- `include/aether/core/sequence/sequence.hpp`, `src/core/sequence/sequence.cpp` — `planExtract`
- `include/aether/persistence/project_store.hpp`, `src/persistence/project_store.cpp` — `commitMultiTrackEdits`
- `include/aether/app/project_session.hpp`, `src/app/project_session.cpp` — session range, `extract`, `UndoKind::Extract`
- `include/aether/ui/timeline_workspace.hpp`, `src/ui/timeline_workspace.cpp` — range, header-name participation, visible end, Extract
- `tests/extract_tests.hpp`, `tests/extract_tests.cpp`, `tests/CMakeLists.txt`, `tests/main.cpp`
- `docs/validation/SLICE_1J_EXTRACT_EDITING.md` and the implementation note on the proposal
- `validation-fixtures/extract-editing/` — ExtractCut, `extract-clip.mov`, `seed-ids.txt`, screenshots

The Edit room source and its tests were not reverted. Existing toolbar buttons, source and record tabs, track headers, and the navigator stayed.

## Build

```
cmake -S /home/editry/development/sise-editor -B /home/editry/development/sise-editor/build-slice1j -DCMAKE_BUILD_TYPE=Debug
cmake --build /home/editry/development/sise-editor/build-slice1j --target aether aether_slice0a_tests
```

FFmpeg stayed the CMake default `/opt/aether-ffmpeg-9.1`.

## CTest

```
Test project /home/editry/development/sise-editor/build-slice1j
    Start 1: slice0a
1/2 Test #1: slice0a ..........................   Passed    1.77 sec
    Start 2: aether_ffmpeg_resolution
2/2 Test #2: aether_ffmpeg_resolution .........   Passed    0.02 sec

100% tests passed, 0 tests failed out of 2

Total Test time (real) =   1.79 sec
```

`QT_QPA_PLATFORM=offscreen`.

## Qt groups and counts

Direct run of `build-slice1j/tests/aether_slice0a_tests`, same offscreen platform. `Totals` lines:

| Group | Totals |
| --- | --- |
| TimingTests | 7 passed, 0 failed, 0 skipped, 0 blacklisted, 48ms |
| ProjectTests | 11 passed, 0 failed, 0 skipped, 0 blacklisted, 35ms |
| SequenceTests | 10 passed, 0 failed, 0 skipped, 0 blacklisted, 26ms |
| TimelineWorkspaceTests | 11 passed, 0 failed, 0 skipped, 0 blacklisted, 105ms |
| MediaImportTests | 10 passed, 0 failed, 0 skipped, 0 blacklisted, 405ms |
| UiIntegrationTests | 9 passed, 0 failed, 0 skipped, 0 blacklisted, 365ms |
| TimelinePlacementTests | 6 passed, 0 failed, 0 skipped, 0 blacklisted, 156ms |
| MarkTests | 7 passed, 0 failed, 0 skipped, 0 blacklisted, 158ms |
| MediaProbeTests | 18 passed, 0 failed, 0 skipped, 0 blacklisted, 325ms |
| OverwriteTests | 10 passed, 0 failed, 0 skipped, 0 blacklisted, 72ms |
| SplitTests | 8 passed, 0 failed, 0 skipped, 0 blacklisted, 68ms |
| LiftTests | 8 passed, 0 failed, 0 skipped, 0 blacklisted, 76ms |
| ExtractTests | 11 passed, 0 failed, 0 skipped, 0 blacklisted, 114ms |

ExtractTests is the nine slots `rangeIsSessionOnly`, `namedTracksOnly`, `classification`, `emptyIntervalShiftsLaterRows`, `unequalUncutAndCrossing`, `visibleEnd`, `identityUndoRedoAndPersistence`, `collisionAndFailedWrite`, and `windowExtractsNamedTrack`, plus QTest's init and cleanup. Every earlier group still passed.

## Native UI

`QT_QPA_PLATFORM=xcb QT_QPA_PLATFORMTHEME=none QT_SCALE_FACTOR=0.58 GTK_IM_MODULE=gtk-im-context-simple QT_IM_MODULE=none` on `build-slice1j/src/aether`. tmux session `aether-extract`. The window left running is `0x01600007`, pid 38407, title `Aether — ExtractCut`.

The sequence was opened from the folder chooser. Range clicks on the canvas resolved through `frameAtCanvasX` to integer frames. The readout showed `Start 24 inclusive. End 40 exclusive. D 16.` Track 0 was checked. Track 1 and track 2 were not. Extract produced the fragments and the `D` shift above. Visible end read 56. The empty interval `[48, 52)` was extracted and undone before that cut. Undo and Redo are the buttons in the timeline menu. Undo restored the previous rows and visible end 72. Redo restored the extract, including the minted id. Close and reopen kept the extract. The database hash after reopen matched the hash taken while it was open: `39195173514e0b315cacf4fd863ea530e3043876d67dfe92e5b64970c14cc3b7`.

## Rejection test

Separate from the successful cut. After reopen, range `[20, 30)` and only track 2. The refusal is quoted above. The hash did not change. The timeline menu showed Undo and Redo grey. The range readout stayed `Start 20 inclusive. End 30 exclusive. D 10.` and also showed the refusal.

## XCB and Wayland

The session above was XCB, on the checklist build. Wayland was not run. No Wayland result is claimed.

A later XCB session used the rebuilt `build-slice1j/src/aether` after the checklist was removed. Same platform variables. The previous ExtractCut window was closed and this binary was started in tmux session `aether-extract`. The window left running is pid 42243, title `Aether — ExtractCut`. Clicking the track 0 name selected that track and left track 1 and track 2 plain. A click on the track 1 clip left the participant set as track 0. Extract of `[32, 36)` (`D` 4) moved only `ea74cb64-075a-4f38-99f5-59eaf242ad35` from record `[40, 56)` to `[36, 52)` and kept its id and source `[0, 16)`. Track 1 stayed `[0, 30)`. The unequal row on track 2 stayed `[10, 40)`. Visible end went from 56 to 52. The database SHA-256 became `12a730ada9de2c4afa250ae4cadb45f0708ca71abf9441bd932fdd4a65c2e0eb`.

The rejection was a separate click: only the track 2 name, range `[20, 30)`. The readout was:

`Track 03495db9-c6a6-49c7-a13e-f4ac7a16d38b instance 59edf682-e660-405f-ba3b-30afebc07a11 crosses the extract interval and its source span is not one frame per record frame.`

The SHA-256 stayed `12a730ada9de2c4afa250ae4cadb45f0708ca71abf9441bd932fdd4a65c2e0eb`. The five instance rows stayed, including the shifted track 0 row.

## Undo, redo, and persistence

One undo reverses every named track and, because visible end is the maximum stored `record_end`, the previous visible end. Redo writes the stored after-rows. Close and reopen clear the undo stack and keep the committed rows. Schema `schema_info` is version 2. The manifest `prototype_version` is `slice-0a`. The eleven tables are unchanged. `timeline_instances` has no duration column.

## Screenshots

| Path | What it shows |
| --- | --- |
| `validation-fixtures/extract-editing/screenshots/initial-timeline.png` | ExtractCut before the cut. Visible end 72 |
| `validation-fixtures/extract-editing/screenshots/selected-range.png` | `Start 24 inclusive. End 40 exclusive. D 16.` |
| `validation-fixtures/extract-editing/screenshots/selected-tracks.png` | Track 0 checked. Track 1 and track 2 unchecked |
| `validation-fixtures/extract-editing/screenshots/empty-interval.png` | `[48, 52)` extracted. Visible end 68 |
| `validation-fixtures/extract-editing/screenshots/successful-extract.png` | `[24, 40)` extracted. Visible end 56 |
| `validation-fixtures/extract-editing/screenshots/retained-fragments.png` | Same window as the successful extract: left fragment, new fragment, and the shifted row |
| `validation-fixtures/extract-editing/screenshots/unchanged-track.png` | Same window. Track 1 is still `[0, 30)` |
| `validation-fixtures/extract-editing/screenshots/undo.png` | Rows restored. Visible end 72 |
| `validation-fixtures/extract-editing/screenshots/redo.png` | Extract restored. Visible end 56 |
| `validation-fixtures/extract-editing/screenshots/reopen.png` | Extract still present after close and reopen |
| `validation-fixtures/extract-editing/screenshots/rejection.png` | Track 2 checked, range kept, refusal naming the track and the instance |
| `validation-fixtures/extract-editing/screenshots/header-track-selected.png` | After the checklist was removed. Track 0 name filled. Track 1 and track 2 not. Visible end 56 |

## Limitations

Undo does not put the session range back. Success clears it so the same interval is not extracted again by accident. The editor sets it again if they want another cut.

The visible-end guard refuses an unlisted row that would extend past `candidateEnd` (`oldEnd - D`). An unlisted row that ends exactly at `candidateEnd` is not overflow and is not trimmed. The guard does not force every success to land on exactly `oldEnd - D` when a named row wholly before the interval still ends later, or when the furthest row never met the interval.

Undo and Redo remain in the timeline menu. They were not added to the toolbar.

`successful-extract.png`, `retained-fragments.png`, and `unchanged-track.png` are one capture of the same window. That window shows the fragments, the shift, and the untouched track together.

Wayland was not run.

## Milestone

Slice 1J extract is implemented and was exercised on XCB: range, named track, empty interval, extract, visible end, undo, redo, close, reopen, and the crossing unequal refusal. The offscreen CTest run and the Qt totals above passed. That green run is not by itself the milestone. Wayland is untested. Slice 1K was not started.

## Corrective distinction

`candidateEnd` is the unselected overflow check. It is `oldVisibleEnd - D` when that value is not below zero. An unselected instance with `record_end` greater than `candidateEnd` rejects the whole extract before any write: no undo entry, the session range stays, and the current visible end stays. The message names that track and that instance. An unselected instance that ends exactly at `candidateEnd` is not overflow and is not trimmed. The stored visible end is the furthest `record_end` after the write. Exact reduction by `D` is not a universal success rule. A participating instance with `record_end <= r0` stays, including its id and source span, even when it remains the furthest endpoint.

The earlier ExtractCut results, hashes, and screenshots above were not replaced. Schema version stays 2. `kPrototypeVersion` stays `slice-0a`. No duration column was added. Lift, split, overwrite, and FFmpeg were not changed.

The correction build is `build-slice1j-correction`. `build-slice1j` was not overwritten. FFmpeg stayed `/opt/aether-ffmpeg-9.1`.

```
cmake -S /home/editry/development/sise-editor -B /home/editry/development/sise-editor/build-slice1j-correction -DCMAKE_BUILD_TYPE=Debug
cmake --build /home/editry/development/sise-editor/build-slice1j-correction --target aether aether_slice0a_tests
```

### CTest

`QT_QPA_PLATFORM=offscreen ctest --test-dir /home/editry/development/sise-editor/build-slice1j-correction --output-on-failure`

```
Test project /home/editry/development/sise-editor/build-slice1j-correction
    Start 1: slice0a
1/2 Test #1: slice0a ..........................   Passed    1.77 sec
    Start 2: aether_ffmpeg_resolution
2/2 Test #2: aether_ffmpeg_resolution .........   Passed    0.02 sec

100% tests passed, 0 tests failed out of 2

Total Test time (real) =   1.79 sec
```

### Qt groups

Direct run of `build-slice1j-correction/tests/aether_slice0a_tests`, same offscreen platform.

| Group | Totals |
| --- | --- |
| TimingTests | 7 passed, 0 failed, 0 skipped, 0 blacklisted, 50ms |
| ProjectTests | 11 passed, 0 failed, 0 skipped, 0 blacklisted, 33ms |
| SequenceTests | 10 passed, 0 failed, 0 skipped, 0 blacklisted, 26ms |
| TimelineWorkspaceTests | 11 passed, 0 failed, 0 skipped, 0 blacklisted, 100ms |
| MediaImportTests | 10 passed, 0 failed, 0 skipped, 0 blacklisted, 390ms |
| UiIntegrationTests | 9 passed, 0 failed, 0 skipped, 0 blacklisted, 350ms |
| TimelinePlacementTests | 6 passed, 0 failed, 0 skipped, 0 blacklisted, 149ms |
| MarkTests | 7 passed, 0 failed, 0 skipped, 0 blacklisted, 148ms |
| MediaProbeTests | 18 passed, 0 failed, 0 skipped, 0 blacklisted, 320ms |
| OverwriteTests | 10 passed, 0 failed, 0 skipped, 0 blacklisted, 71ms |
| SplitTests | 8 passed, 0 failed, 0 skipped, 0 blacklisted, 62ms |
| LiftTests | 8 passed, 0 failed, 0 skipped, 0 blacklisted, 72ms |
| ExtractTests | 12 passed, 0 failed, 0 skipped, 0 blacklisted, 139ms |

ExtractTests gained `candidateEndAndStoredEnd`. The earlier eleven slots still pass, including `visibleEnd`. The new slot covers an unselected row past `candidateEnd`, an unselected row ending exactly at `candidateEnd`, an unselected row ending before it, a participating row wholly before the interval whose derived visible end stays the furthest stored end, a shifted row that keeps its id and source span, an empty interval, a crossing unequal rejection, undo, redo, and close/reopen. A failed preflight writes nothing and pushes no undo entry. Undo and redo of a successful extract restore and reapply the stored rows. Reopen keeps the committed rows, clears the session range and the record mark, and leaves schema 2 and prototype `slice-0a`.

### Native XCB

`QT_QPA_PLATFORM=xcb QT_QPA_PLATFORMTHEME=none QT_SCALE_FACTOR=0.58` on `build-slice1j-correction/src/aether`. The disposable project is `validation-fixtures/extract-editing/correction-parent/endrule`. ExtractCut was not opened and its hash stayed `12a730ada9de2c4afa250ae4cadb45f0708ca71abf9441bd932fdd4a65c2e0eb`. The EndRule database stayed the seed hash `ec9202eeea77ae50fa83a589e5d43f27534908f9da6dafec83175541dbcff0b1` because no extract was committed. The window left running is pid 53928, title `Aether — EndRule`.

Range mode came up off. The Range control toggled on and back off. Visible end read 72. No track name was selected. Pointer clicks on the track names, the canvas, and the timeline Menu button did not change participation, the range fields, or open Undo. The compositor pointer does toggle the Range button when the click is aimed at that button. That was not enough to drive a success, an unselected overflow refusal, a wholly-before extract, an unequal refusal, undo, redo, close, or reopen.

XCB is the Qt platform used above. The desktop session is Wayland (`XDG_SESSION_TYPE=wayland`, Xwayland on `:0`). The Qt application was not run as a Wayland client. No Wayland result is claimed.

| Path | What it shows |
| --- | --- |
| `validation-fixtures/extract-editing/correction/screenshots/range-off-visible-end-72.png` | EndRule before any committed extract. Range off. Visible end 72. No success, refusal, undo, redo, or reopen capture was produced. |

### Native XCB, warp plus XTest

The click path was `XRaiseWindow`, `XSetInputFocus`, `XWarpPointer` to the X root, then `XTestFakeButtonEvent` button 1 press and release. `XTestFakeMotionEvent` was not used to move the pointer. `XSendEvent` was not used. While the button was down, `XQueryPointer` reported mask 256 (`Button1Mask`). The EndRule client origin on the root was `(0, 69)`. An `import -window 0x01600007` pixel `(ix, iy)` is root `(ix, iy + 69)`.

That path toggled Range. Root `(35, 759)` turned it on. Root `(20, 756)` turned it off. While Range was still on, a canvas click at root `(189, 789)`, aimed at frame 12, wrote `12` into `extractRangeStart` and the readout `Start 12 inclusive. End not set.` The end field stayed empty, so `setExtractRange` was not called. The start field was then cleared. The readout became `Start and end are integer sequence frames. End is exclusive.` With Range off, a ruler click at root `(198, 789)`, aimed at frame 16, left both extract fields empty. The record-mark field is not in the accessibility tree until the sequence viewer menu is open, and that menu did not open, so the mark value was not read.

A track-name click at root `(65, 811)` changed the track-header description from `No extract tracks` to `Extract tracks track 0`. Tracks 1 and 2 stayed unselected.

The same press did not open a tool-button menu. A sweep of the sequence viewer Menu button, image x 1358–1406 and y 68–82, produced no popup and no `sequence frames` field. A 1.5 second hold on the timeline Menu at root `(1882, 701)` did not show Undo or Redo and did not map a menu window. The seven editorial cases were not driven. No extract was committed. The EndRule hash stayed `ec9202eeea77ae50fa83a589e5d43f27534908f9da6dafec83175541dbcff0b1`. The ExtractCut hash stayed `12a730ada9de2c4afa250ae4cadb45f0708ca71abf9441bd932fdd4a65c2e0eb`. The window left running is still pid 53928, title `Aether — EndRule`. Track 0 remains selected. Range is off.

This pass was XCB (`QT_QPA_PLATFORM=xcb`). The desktop session is Wayland (`XDG_SESSION_TYPE=wayland`, Xwayland on `:0`). The Qt application was not a Wayland client. No Wayland result is claimed.

| Path | What it shows |
| --- | --- |
| `validation-fixtures/extract-editing/correction/screenshots/track-name-selected-menu-closed.png` | Range off. Visible end 72. Track 0 name selected. No menu open. No committed extract. |

### Native XCB, extract cases

Same window, pid 53928, same `XWarpPointer` then `XTestFakeButtonEvent` path. The extract command was not edited. ExtractCut was not opened. Its hash stayed `12a730ada9de2c4afa250ae4cadb45f0708ca71abf9441bd932fdd4a65c2e0eb`. There is no undo shortcut in the application: the source has no `QShortcut` and no `QKeySequence`. Ctrl+Z was not sent.

Track 0 was already selected. Root `(20, 756)` turned Range on. Canvas clicks at root `(272, 789)` and `(281, 789)` set start 48 and end 52. The readout was `Start 48 inclusive. End 52 exclusive. D 4.` The hash was still the seed `ec9202eeea77ae50fa83a589e5d43f27534908f9da6dafec83175541dbcff0b1`.

The Extract button, not the timeline menu, was clicked at root `(376, 701)`. The hash became `1f17cba180dca196f5560228155ed77e97e9662ba40ab5d6cdef1fa9b86fcbee`. `652d0676-bb41-44ad-9214-a825f72ec98a` moved from record `[56, 72)` to `[52, 68)` and kept source `[0, 16)`. `aa612a25-182f-4382-bda3-22caf0afb7cf` stayed `[0, 40)`. Unselected `83e11137-2a5e-4a5f-ab82-dbe90c361dad` stayed `[0, 30)`. Unselected `d67bdd8a-f04f-48b9-ae2a-296cdc6fa621` stayed record `[10, 40)` source `[0, 10)`. Visible end read 68, the furthest stored record end. The fields cleared. The readout was `Extract closed the interval. The record range was cleared.`

A hold on the timeline Menu at root `(1882, 701)` still did not open Undo or Redo. That tool button is the failed control. The committed extract was left in place. No undo was invented.

With only track 0 selected, canvas clicks set `[20, 52)`, readout `Start 20 inclusive. End 52 exclusive. D 32.` Extract was clicked again. The message was `Track 9eb9e698-aaad-4b10-970f-5b0d3534150e instance d67bdd8a-f04f-48b9-ae2a-296cdc6fa621 ends at 40, past candidate end 36. Visible end is 68 and D is 32. Candidate end checks unselected overflow. It is not the stored visible end. Extract does not move or truncate a track that was not named.` The hash stayed `1f17cba180dca196f5560228155ed77e97e9662ba40ab5d6cdef1fa9b86fcbee`. The rows stayed as after the success. The fields stayed `20` and `52`.

Track 0 was clicked off and track 2 was clicked on. The header read `Extract tracks track 2`. Canvas clicks set `[20, 30)`, readout `Start 20 inclusive. End 30 exclusive. D 10.` Extract was clicked. The message was `Track 9eb9e698-aaad-4b10-970f-5b0d3534150e instance d67bdd8a-f04f-48b9-ae2a-296cdc6fa621 crosses the extract interval and its source span is not one frame per record frame.` The hash stayed `1f17cba180dca196f5560228155ed77e97e9662ba40ab5d6cdef1fa9b86fcbee`. The rows did not change. The timeline menu still did not open, and there is no undo shortcut, so the undo stack was not visible. No claim is made that the refusal added no undo entry.

File, which is a menu-bar item, opened with the same click path. Close Project was clicked. The chooser was already in `endrule`, showing `project.manifest.json`, and Open was clicked. The window title returned to `Aether — EndRule`. The hash stayed `1f17cba180dca196f5560228155ed77e97e9662ba40ab5d6cdef1fa9b86fcbee`. The four rows above survived, including `652d0676-bb41-44ad-9214-a825f72ec98a` at `[52, 68)`. The start and end fields were empty, the readout was `No record range.`, and the header was `No extract tracks`. Visible end stayed 68. The Range button was still checked. The unequal refusal sentence was still painted in the rejection label.

This pass was XCB. The desktop session is Wayland. The Qt application was not a Wayland client. No Wayland result is claimed. The window left running is pid 53928.

| Path | What it shows |
| --- | --- |
| `validation-fixtures/extract-editing/correction/screenshots/range-on-48-52.png` | Range on. Start 48, end 52, D 4. Track 0 selected. Before Extract. |
| `validation-fixtures/extract-editing/correction/screenshots/success-track0-visible-end-68.png` | After Extract. Visible end 68. Range fields cleared. |
| `validation-fixtures/extract-editing/correction/screenshots/refusal-unselected-overflow.png` | Unselected overflow message naming track `9eb9e698-aaad-4b10-970f-5b0d3534150e` and instance `d67bdd8a-f04f-48b9-ae2a-296cdc6fa621`. |
| `validation-fixtures/extract-editing/correction/screenshots/refusal-unequal.png` | Crossing unequal message for that same track and instance. |
| `validation-fixtures/extract-editing/correction/screenshots/reopen-committed-range-cleared.png` | After close and reopen. Visible end 68. Readout `No record range.` |

## Native UI Completion and Lifecycle Correction

This section records the range, feedback, menu, and undo work. Earlier sections are unchanged. Extract transformation, track participation, visible-end rules, marks, schema, FFmpeg, lift, split, and overwrite were not edited. No Slice 1K. `ExtractCut`, `EndRule`, and `LaneStack` were not opened. No git repository was created. No commit and no push. `build-slice1j`, `build-slice1j-correction`, and earlier build directories were left in place. The new build directory is `build-slice1j-ui-completion`. The disposable project is `validation-fixtures/extract-editing/ui-completion-parent/ExtractUiCut`.

### Root causes

Range mode and the record range were different pieces of state. A change of displayed sequence cleared the start and end fields, the readout, the canvas overlay, and the session range, and left `extractRangeMode` checked and the canvas range mode on. After close and reopen the Range button was still checked, so a ruler click still wrote the extract range. `resetRangeInteraction` is the one transition for startup and for a change of displayed sequence, which covers project open, project close, and sequence change. It clears the awaiting-end flag, both fields, the readout to `No record range.`, the canvas overlay, the canvas range mode, the Range button under a signal blocker, and the session range through `ProjectSession::clearExtractRange`. A refused extract still keeps the fields, the mode, and the session range. A successful extract still clears the consumed fields, the overlay, and the session range, and does not uncheck Range. The mode and the range are not written to `project.db`, the manifest, or QSettings.

The refusal and success sentence is the `rejection_` label. The window stays alive across close and reopen, so the old sentence stayed painted. The same sequence-identity transition clears that label. A later extract replaces it through `explain`.

The timeline Menu is a real `QMenu` on a tool button. `QToolButton::InstantPopup` calls `QMenu::exec` from the mouse press. `QMenu` hides that press when the pointer is still outside the menu and has not moved past the drag threshold. A menu-bar menu survives because the menu forwards that press to the menu bar. A tool button is not given that exception, so the opening press dismisses the menu before a window stays mapped. `PanelMenuButton` keeps that same `QMenu`, does not emit `pressed` on the left click, and calls `menu->popup` from the finished click. Disabled actions stay disabled. The project, viewer, side, and timeline menus use this button. This is the same menu, not a substitute.

There was no Edit menu. Undo and Redo lived inside the timeline menu form, with no `QKeySequence` shortcut, and a finished extract did not refresh them. The menu bar now has Edit. `editUndo` and `editRedo` use `QKeySequence::Undo` and `QKeySequence::Redo` with `Qt::WindowShortcut`, and they call `TimelineWorkspace::undoEdit` and `redoEdit`, which call `ProjectSession` only. `historyChanged` comes from `syncUndo`. The actions are enabled from `canUndo` and `canRedo`, including after success, undo, redo, open, close, and sequence change. A text field keeps its own undo: the line edit accepts the shortcut override, and the window action does not take it.

### Files

`src/ui/timeline_workspace.cpp`, `include/aether/ui/timeline_workspace.hpp`, `src/ui/main_window.cpp`, `include/aether/ui/main_window.hpp`, `tests/ui_completion_tests.hpp`, `tests/ui_completion_tests.cpp`, `tests/CMakeLists.txt`, `tests/main.cpp`, and this file.

### Build and CTest

`QT_QPA_PLATFORM=offscreen ctest --output-on-failure` in `build-slice1j-ui-completion`:

```
Test project /home/editry/development/sise-editor/build-slice1j-ui-completion
    Start 1: slice0a
1/2 Test #1: slice0a ..........................   Passed    2.25 sec
    Start 2: aether_ffmpeg_resolution
2/2 Test #2: aether_ffmpeg_resolution .........   Passed    0.02 sec

100% tests passed, 0 tests failed out of 2

Total Test time (real) =   2.28 sec
```

Qt groups inside `slice0a`, from that run's `LastTest.log`: TimingTests 7 passed, 47ms. ProjectTests 11 passed, 34ms. SequenceTests 10 passed, 25ms. TimelineWorkspaceTests 12 passed, 115ms. MediaImportTests 10 passed, 451ms. UiIntegrationTests 9 passed, 406ms. TimelinePlacementTests 6 passed, 150ms. MarkTests 7 passed, 148ms. MediaProbeTests 18 passed, 320ms. OverwriteTests 10 passed, 73ms. SplitTests 8 passed, 65ms. LiftTests 8 passed, 72ms. ExtractTests 12 passed, 140ms. UiCompletionTests 5 passed, 176ms. ExtractTests was not weakened. UiCompletionTests covers the range and feedback lifecycle, the timeline menu opening again with Qt mouse events, and Edit-menu plus `QKeySequence::Undo` / `QKeySequence::Redo` undo and redo of an extract, including a text field keeping its own undo.

A ctest run without `QT_QPA_PLATFORM=offscreen` failed in MediaProbeTests with a Wayland protocol error while this live XCB window had a popup mapped. That run is not the result above. The offscreen run is the one that passed.

### Native XCB

Launch: `QT_QPA_PLATFORM=xcb QT_QPA_PLATFORMTHEME=none QT_SCALE_FACTOR=0.58` on `build-slice1j-ui-completion/src/aether`. The process is pid 68366, title `Aether — ExtractUiCut`, client `0x01200007`, client origin `(0, 69)`. The desktop session type is `wayland`. The application platform is xcb. Wayland was not tested. No Wayland result is claimed.

`XTestQueryExtension` reports XTest 2.2. `XTestFakeMotionEvent`, `XTestFakeButtonEvent`, and `XTestFakeKeyEvent` return success. `XTestFakeMotionEvent` leaves the pointer where `XQueryPointer` already had it. During a press, `XQueryPointer` reports button mask 0. An override-redirect window created under the pointer receives Expose and no ButtonPress. gnome-shell logs `[xwayland ei] Unhandled event EI_EVENT_SYNC (91)` when these requests are sent. `XWarpPointer` does move the pointer that `XQueryPointer` reports, and the Aether frame is the child at that point when the pointer and the window are in the same place.

Aimed clicks used `XRaiseWindow`, `XSetInputFocus`, `XWarpPointer`, `XTestFakeMotionEvent`, and `XTestFakeButtonEvent` button 1. A sweep across the Range control, local x 40 and y 630 through 754, left the Range check box unchecked. A sweep across the sequence list, local x 100 and y 410 through 474, left `Reel 24/1` unselected. The open sequence stayed `Other 24/1`.

| Case | Result |
| --- | --- |
| Open, range off, button unchecked | Observed on the running window. Range unchecked in AT-SPI. Readout `No record range.` Visible end `none` because Other has no instance. No refusal sentence. Screenshot `01-open-range-off.png`. |
| Ruler click sets a mark | Stopped. The same button path left Range and the sequence list unchanged, so the ruler was not claimed. |
| Range on | Stopped. The Range sweep left the button unchecked. |
| Close and reopen, mode off, range gone | Stopped. File was not opened by an aimed click, so the project was not closed. |
| Refusal, then reopen with the sentence gone | Stopped. No extract was issued. |
| Timeline Menu opens and an item runs | Stopped. The aimed point was local `(1882, 632)`, root `(1882, 701)`, the Menu button at the right of the timeline header. No menu mapped there. Screenshot `02-timeline-menu-not-open.png` shows that button closed. A different window, `0x1200038`, mapped at root `(30, 85)`, 194 by 49. Its pixels are the Edit menu: Undo `Ctrl+Z`, Redo `Ctrl+Shift+Z`. That is screenshot `03-edit-menu-undo-redo.png`. It is the menu-bar Edit menu, and it is not an aimed open of the timeline Menu. |
| Edit menu Undo and Redo of an extract | Stopped. No extract was committed, so Undo was not run. The Edit popup above is the menu, and its shortcuts are the platform text for `QKeySequence::Undo` and `QKeySequence::Redo`. |
| The same with the shortcuts | Stopped. `XTestFakeKeyEvent` left the keymap empty, so Ctrl+Z was not delivered. |
| Close and reopen with the committed extract | Stopped. ExtractUiCut was not changed by an extract. |

The window is still running: pid 68366, title `Aether — ExtractUiCut`. The Edit popup `0x1200038` was still mapped when this section was written.

| Path | What it shows |
| --- | --- |
| `validation-fixtures/extract-editing/ui-completion/screenshots/01-open-range-off.png` | ExtractUiCut open on Other. Range unchecked. `No record range.` Visible end none. No refusal sentence. |
| `validation-fixtures/extract-editing/ui-completion/screenshots/02-timeline-menu-not-open.png` | After the aimed timeline Menu click. The timeline Menu is closed. |
| `validation-fixtures/extract-editing/ui-completion/screenshots/03-edit-menu-undo-redo.png` | The popup that mapped at root `(30, 85)`. Undo and Redo, with Ctrl+Z and Ctrl+Shift+Z. |

SLICE 1J NATIVE UI COMPLETION PARTIALLY COMPLETE — REMAINING ISSUES DOCUMENTED.

### Resumed native pass on the running window

The same process was still pid 68366, title `Aether — ExtractUiCut`, client `0x01200007`, origin `(0, 69)`. The click path was the EndRule path only: `XRaiseWindow`, `XSetInputFocus`, `XWarpPointer` to root coordinates, `XTestFakeButtonEvent` press, then release. `XTestFakeMotionEvent` was not used. `XQueryPointer` was read after the warp, while the button was down, and after the release.

The first case was the ruler, with Range already unchecked. The aimed root point was `(280, 787)`, image `(280, 718)`, on the canvas ruler to the right of the red playhead. All three pointer reads were `ptr 280,787 child 0x0 mask 0`. The button mask stayed 0. After the click, Range was still unchecked, both range fields were still empty, the readout was still `No record range.`, visible end was still 40, and the playhead line was still `Playhead 00:00:00:00  Reel.` The database hash was the same before and after:

`2875069638694011bee1d031ef1dd6e9e168fc2a2f19b24be7255657e06ccbec`

The ruler is the control that did not change. The later cases were not clicked: Range on, track name, Extract, Edit Undo, Edit Redo, close and reopen, and the timeline Menu. No shortcut is claimed. Screenshot `validation-fixtures/extract-editing/ui-completion/screenshots/11-ruler-click-mask-0.png`. The window was left running.

SLICE 1J NATIVE UI COMPLETION PARTIALLY COMPLETE — REMAINING ISSUES DOCUMENTED.

## Final Native Interaction Verification

Recorded 2026-11-24 20:04 IST through 20:21 IST. No extract command, editorial rule, schema, mark, visible-end, FFmpeg, Split, Lift, or Overwrite code was changed. No application input code was changed. `11-ruler-click-mask-0.png` was not overwritten. ExtractCut, EndRule, LaneStack, ShellCut, PlaceCut, MarksCut, ProbeCut, OverwriteCut, SplitCut, LiftCut, and EditRoom were not opened.

### Process

At 2026-11-24 20:04:23 IST, pid 68366 was not running. No other `aether` process was running. The project log's last lines were `project closed project ExtractUiCut`. `project.db` was last written 2026-11-23 21:03:22 IST. Its hash was `b80f1a651913e0bc18dbb6a929d48719f5257cbf6ebf1acf5e453ebf6a0a26c6`, which differs from the earlier in-session hash `2875069638694011bee1d031ef1dd6e9e168fc2a2f19b24be7255657e06ccbec` because that close wrote the database. The same binary was relaunched at 2026-11-24 20:05:36 IST as pid 8902:

`QT_QPA_PLATFORM=xcb QT_QPA_PLATFORMTHEME=none QT_SCALE_FACTOR=0.58` on `build-slice1j-ui-completion/src/aether`, `DISPLAY=:0`. The desktop session type is `wayland`. The client is `0x01200007`, mapped, local `(0, 37)`, 1920 by 1011, inside frame `0xa00004` at `(0, 32)`, 1920 by 1048. The client origin on the root is `(0, 69)`. `wmctrl` reports `0,106 1920x1011` and the title `Aether — ExtractUiCut`. At 2026-11-24 20:21:08 IST the input focus was `0x01200007`.

The project folder was chosen through the existing Project dialog with AT-SPI widget actions. That was not an XTest click and it is not a validation click. After open, Range was unchecked, the readout was `No record range.`, visible end was `none`, and the playhead line was `Playhead 00:00:00:00  Other.` The hash was still `b80f1a651913e0bc18dbb6a929d48719f5257cbf6ebf1acf5e453ebf6a0a26c6`.

### XCB input diagnostic

`xdotool`, `xinput`, `xev`, and `xwininfo` are not installed. `wmctrl` and `xdpyinfo` are. `xdpyinfo` lists the XTEST extension. `XTestQueryExtension` reports version 2.2, event base 0, error base 0. Packages were not installed.

`xev` and `xinput` being absent, a passive XI raw-button monitor was used. It selects `XI_RawButtonPress` and `XI_RawButtonRelease` on the root and prints them. A pointer move is not counted as a click.

One `XTestFakeButtonEvent` press and one release were sent at root `(960, 575)`, inside the Aether frame. Both requests returned success. `XQueryPointer` while the button was requested down read `ptr 960,575 child 0xa00004 mask 0`. The monitor counted 0 raw presses and 0 raw releases. A separate `Atspi.generate_mouse_event` press and release also returned success, and the same monitor counted 0 and 0. Range stayed unchecked and the readout stayed `No record range.`

The X server generated no ButtonPress and no ButtonRelease. The application did not receive a button event. This is an input-injection and desktop failure. Application input code was not changed. No mouse-event logging was added.

The ruler failure has that cause. The earlier warp reached a root point, and this probe's pointer was over the Aether frame, but neither probe produced a button event. Range, the ruler, the timeline Menu, the Edit menu, Undo, Redo, and text-field undo were not driven by a delivered click. No live pass is claimed for them.

### Build and tests

No source change, so no new build directory. `build-slice1j`, `build-slice1j-correction`, and `build-slice1j-ui-completion` were not overwritten. `QT_QPA_PLATFORM=offscreen ctest --output-on-failure` in `build-slice1j-ui-completion` at 2026-11-24 20:21 IST:

```
1/2 Test #1: slice0a ..........................   Passed    3.10 sec
2/2 Test #2: aether_ffmpeg_resolution .........   Passed    0.03 sec
100% tests passed, 0 tests failed out of 2
Total Test time (real) =   3.13 sec
```

Qt groups in that run, all with 0 failed: TimingTests 7 passed, 65ms. ProjectTests 11 passed, 45ms. SequenceTests 10 passed, 37ms. TimelineWorkspaceTests 12 passed, 158ms. MediaImportTests 10 passed, 622ms. UiIntegrationTests 9 passed, 723ms. TimelinePlacementTests 6 passed, 303ms. MarkTests 7 passed, 183ms. MediaProbeTests 18 passed, 357ms. OverwriteTests 10 passed, 79ms. SplitTests 8 passed, 74ms. LiftTests 8 passed, 79ms. ExtractTests 12 passed, 139ms. UiCompletionTests 5 passed, 184ms.

Wayland was not tested. The application platform was xcb. The window left running is pid 8902, title `Aether — ExtractUiCut`.

| Path | What it shows |
| --- | --- |
| `validation-fixtures/extract-editing/ui-completion/screenshots/12-open-no-button-event.png` | ExtractUiCut after relaunch. Taken while the button monitor had counted no press and no release. |

SLICE 1J NATIVE INTERACTION BLOCKED — XCB EVENT DELIVERY FAILURE CONFIRMED.

## Desktop Input Recovery and Native Interaction Validation

Recorded 2026-11-24 20:33 IST through 20:39 IST. No extract command was edited. No source file was changed. No new build. `11-ruler-click-mask-0.png` and `12-open-no-button-event.png` were not overwritten. No new editorial screenshot was taken, because no press and release were observed. ExtractCut, EndRule, LaneStack, ShellCut, PlaceCut, MarksCut, ProbeCut, OverwriteCut, SplitCut, LiftCut, and EditRoom were not opened. ExtractUiCut was left open.

### What is running

pid 8902, started 20:05:36 IST, executable `build-slice1j-ui-completion/src/aether`, `QT_QPA_PLATFORM=xcb`, `QT_SCALE_FACTOR=0.58`, `DISPLAY=:0`. `xwininfo -id 0x01200007` at 20:39 IST: title `Aether — ExtractUiCut`, mapped, absolute upper-left `(0, 69)`, 1920 by 1011. Hash unchanged: `b80f1a651913e0bc18dbb6a929d48719f5257cbf6ebf1acf5e453ebf6a0a26c6`.

`DISPLAY=:0` is Xwayland, not Xorg. The process is `/usr/bin/Xwayland :0 -rootless ... -enable-ei-portal`. `xinput list` prints `WARNING: running xinput against an Xwayland server.` Devices are `xwayland-pointer:18` (id 6), `xwayland-relative-pointer:18` (id 7), `xwayland-pointer-gestures:18` (id 8), and `xwayland-keyboard:18` (id 9), plus the virtual core XTEST pointer and keyboard. No Xorg process is running.

Session: `XDG_SESSION_TYPE=wayland`, `XDG_CURRENT_DESKTOP=GNOME`, `XDG_SESSION_DESKTOP=gnome`, `WAYLAND_DISPLAY=wayland-0`. `loginctl` session 2, type wayland, leader `gdm-wayland-session`. Compositor is `/usr/bin/gnome-shell --mode=user` (pid 3669), which owns `org.gnome.Mutter.RemoteDesktop`.

### Packages

The owner installed four packages. Confirmed without sudo at 20:38:52 IST:

| Name | Installed | which | Version |
| --- | --- | --- | --- |
| xdotool | yes | `/usr/bin/xdotool` | 4.20260303.1-1.fc45 |
| xinput | yes | `/usr/bin/xinput` | 1.6.4-7.fc45 |
| xev | yes | `/usr/bin/xev` | 1.2.7-5.fc45 |
| xwininfo | yes | `/usr/bin/xwininfo` | 1.1.7-2.fc45 |

`dnf repoquery --cacheonly` had already shown those four names exist in the Fedora 45 repositories. `xorg-x11-apps` and `xorg-x11-utils` are not the providing names on this machine. Nothing further was installed.

`ydotool` and `dotool` are absent. `wmctrl`, `xdpyinfo`, and `gdbus` are present. Qt 6 platform plugins present: `/usr/lib64/qt6/plugins/platforms/libqwayland.so` and `libqxcb.so`. Build directories were not overwritten: `build-edit-room`, `build-slice0b`, `build-slice1a` through `build-slice1i`, `build-slice1j`, `build-slice1j-correction`, `build-slice1j-ui-completion`, `build-slice-ui`, `build-slice-ui-integration`.

### Path A, Wayland platform

The same ui-completion binary was launched at 20:37:18 IST as pid 12697 with `QT_QPA_PLATFORM=wayland QT_QPA_PLATFORMTHEME=none QT_SCALE_FACTOR=0.58`. Qt logged `Successfully loaded Qt platform plugin "wayland"`. `/proc/12697/maps` contains `libqwayland.so` and not `libqxcb.so`. The compositor sent `xdg_toplevel.configure` for a maximized surface `QSize(1920, 1048)` with `WindowMaximized|WindowActive`, and for the project dialog `QSize(327, 284)` with `WindowActive`. That is a native Wayland surface, visible and marked active. It is not an X11 window: `wmctrl` still lists only pid 8902.

No press and no release were observed on that surface. `xev` and `xinput` watch the Xwayland server, not a Wayland client. The disposable project `validation-fixtures/extract-editing/ui-completion-parent/ExtractNativeWayland` was copied from ExtractUiCut and renamed in its own manifest and `project_meta` only. Its hash is `d2c30b7359874d99d7ab76fbca78adef0fa4b62d330dae5236c677f4d7b96b1a`. The Wayland process is still on its project dialog. It was not given a pointer click. The default launch and the XCB implementation were not changed.

### Path B, remote desktop portal

`org.freedesktop.portal.RemoteDesktop` is installed. Its input methods are `NotifyPointerMotion`, `NotifyPointerButton`, and `NotifyKeyboardKeycode`. `org.gnome.Mutter.RemoteDesktop` is also on the session bus. The permission store table `remote-desktop` has no ids (`List` returned empty). No session is authorised. `Start` would open a permission dialog for remote-desktop control. The minimum request, which was not sent, would be `CreateSession`, then `SelectDevices` with pointer and keyboard only, then `Start`. Screen capture was not requested. Mutter's remote-desktop interface was not called, because that would be unrestricted compositor input without a stored permission. Path B stopped before any request.

### Path C, uinput

`/dev/uinput` is `crw------- root root`. The user groups are `editry wheel kvm video libvirt`. `ydotool` and `dotool` are not installed. No daemon was installed, no group was changed, no udev rule was written, and sudo was not used. A working device would require the owner to install an injector and grant this user access to `/dev/uinput`. That change was not made.

### Path D, sessions

`/usr/share/xsessions` is empty. There is no Xorg session file. `/usr/share/wayland-sessions` has `gnome.desktop` (`Name=GNOME`, `Exec=/usr/bin/gnome-session`) and `gnome-classic.desktop` (`Name=GNOME Classic`, `Exec=gnome-session --session gnome-classic`). The login-screen choices are those two Wayland sessions. The expected `XDG_SESSION_TYPE` is `wayland`, which is the current session. An XCB launch remains `QT_QPA_PLATFORM=xcb QT_QPA_PLATFORMTHEME=none QT_SCALE_FACTOR=0.58 ./build-slice1j-ui-completion/src/aether`. The session was not switched.

### Observed events

At 20:39 IST an `xev` window named `AetherObserve` was mapped at absolute `(55, 55)`, 270 by 247, window `0xa0004e`. `xinput test-xi2 6` watched `xwayland-pointer:18`. `xdotool windowactivate`, `mousemove --window`, and `click 1` were aimed only at that xev window. `xdotool click` exited 0. `xdotool getmouselocation` then reported `X=0 Y=87 WINDOW=884` (the root). `xinput` recorded 0 `EVENT` lines, 0 `ButtonPress`, and 0 `ButtonRelease`. `xev` recorded 0 `ButtonPress` and 0 `ButtonRelease`. It did record `FocusIn` and `FocusOut`. The xev window was then closed. Aether was not clicked.

A successful `xdotool` return is not a click. No press and no release were observed, so none is claimed. The application did not receive a button event. The ruler, Range button, timeline menu, Edit menu, Undo, Redo, and text-field undo were not driven. Classification: no supported desktop input path. The Xwayland limitation remains: XTEST and xdotool are accepted and do not emit a button event. The Wayland platform plugin loads a real surface, and no authorised injector can deliver a press to it.

Offscreen CTest was not rerun. Source is unchanged since the 20:21 IST run: slice0a passed in 3.10 sec, `aether_ffmpeg_resolution` passed in 0.03 sec, 2 of 2 passed, total 3.13 sec. Qt groups, 0 failed: TimingTests 7, ProjectTests 11, SequenceTests 10, TimelineWorkspaceTests 12, MediaImportTests 10, UiIntegrationTests 9, TimelinePlacementTests 6, MarkTests 7, MediaProbeTests 18, OverwriteTests 10, SplitTests 8, LiftTests 8, ExtractTests 12, UiCompletionTests 5.

Both Aether processes were left running: pid 8902 on ExtractUiCut, and pid 12697 on the Wayland platform.

SLICE 1J NATIVE INTERACTION BLOCKED — NO SUPPORTED DESKTOP INPUT PATH AVAILABLE.


## RemoteDesktop Portal Input Recovery and Native Interaction Validation

Checked at 21:06 IST, before any portal call: pid 8902 and pid 12697 were not running. Their project files were still present and were not opened by a replacement process. One XCB process was started later, pid 14581, at about 21:18 IST, from `build-slice1j-ui-completion/src/aether` with `QT_QPA_PLATFORM=xcb QT_QPA_PLATFORMTHEME=none QT_SCALE_FACTOR=0.58`. `/proc/14581/maps` contains `libqxcb.so`. This pass is that XCB client. The Wayland plugin was not under test, and the default launch was not changed.

`ExtractUiCut/project.db` stayed `b80f1a651913e0bc18dbb6a929d48719f5257cbf6ebf1acf5e453ebf6a0a26c6`. `ExtractNativeWayland/project.db` stayed `d2c30b7359874d99d7ab76fbca78adef0fa4b62d330dae5236c677f4d7b96b1a`. Neither project was opened or edited. The disposable project is `validation-fixtures/extract-editing/ui-completion-parent/ExtractPortalCut`. It was copied from ExtractUiCut, with `project.lock` left behind, and both the manifest name and `project_meta.name` set to ExtractPortalCut. Its hash after close and save is `1ced9f2ecb5bfdc2225c2dacb1df92901dbb76c1ec5d965e0e098a959328fbc3`.

### Consent

`AvailableDeviceTypes` was 7 and the interface version was 2. The request was `CreateSession`, `SelectDevices` with `types` 3 and `persist_mode` 0, then `Start`. No screen cast, clipboard, touchscreen, or restore token was requested. A returned request path was not treated as approval.

The first dialog was waiting at 21:08:55 IST. The owner approved it at 21:09:23 IST. The response code was 0 and `devices` was 3. That helper then aborted (`Fatal Python error: _PyThreadState_Attach`) before it delivered a pointer event, and the session died with the process. A second dialog was waiting at 21:16:21 IST. The owner approved it at 21:16:25 IST, again with response code 0 and `devices` 3. The dialogs were not clicked by the helper.

### EIS, and the harmless press

`ConnectToEIS` is on the installed portal interface and `libei.so.1.6.0` is installed, so the session used EIS only. `NotifyPointerMotion` and `NotifyPointerButton` were not called. The seat was `mutter default seat`. It offered an absolute pointer with buttons, region `0,0 1920x1080`, and a separate keyboard with an XKB keymap. `ei_device_pointer_motion_absolute` placed the pointer. Screen capture was not required and was not requested.

The pointer was moved to `520,427`. `xdotool getmouselocation` reported `X=520 Y=427` and the xev client `0x1200001`. `BTN_LEFT` is `0x110` in `linux/input-event-codes.h`. Press and release on that window produced:

```
ButtonPress event, serial 35, synthetic NO, window 0x1200001,
    state 0x0, button 1, same_screen YES
ButtonRelease event, serial 35, synthetic NO, window 0x1200001,
    state 0x100, button 1, same_screen YES
```

The release state `0x100` is the button that the press had held. The xev window was then closed.

### Aether, on ExtractPortalCut

The project dialog and the folder dialog were driven with the same absolute pointer. The folder path was typed through the EIS keyboard. Key codes came from the keymap the EIS server returned (`<AB01> = 52` is keysym `0x7a`, evdev 44, which is `KEY_Z`; `<LCTL> = 37` is evdev 29, `KEY_LEFTCTRL`; `<LFSH> = 50` is evdev 42, `KEY_LEFTSHIFT`; `<RTRN> = 36` is evdev 28, `KEY_ENTER`; `<AC01> = 38` is evdev 30, `KEY_A`). The opened window title was `Aether — ExtractPortalCut`.

On open, Other was current, Range was unchecked, the readout was `No record range.`, and visible end was none. A click on `Reel 24/1` selected Reel. Visible end became 40. Range stayed unchecked and the readout stayed `No record range.`

A ruler click with range off, at root `(171, 787)`, wrote `5` into the sequence-frame field in the viewer menu. The range fields stayed empty. `13-portal-mark-field-5.png`.

The Range button was then checked. A canvas click set start 26. A later click on the clip body set end 39. The readout became `Start 26 inclusive. End 39 exclusive. D 13.` The fields showed `26` and `39`, and the canvas painted the blue range span over the clip. `14-portal-range-overlay.png`.

The V1 name was clicked and drew the blue participant highlight. `16-portal-v1-selected.png`. The timeline Menu button, at root `(1882, 701)`, opened its own menu. The popup was `538x253` at `+1382+714` and contained the real edit form: `No instance selected.`, `track 0 start 0 duration 40`, and the move and trim fields. `15-portal-timeline-menu.png`. That was the header button, not a direct `QMenu::popup()` call.

Extract, with that range and V1, changed the database. The readout became `Extract closed the interval. The record range was cleared.` Range stayed checked. The fields were cleared. Visible end became 27. `timeline_instances` became `(0, 26)` and `(26, 27)`. ExtractUiCut's hash did not change.

The menu-bar Edit menu then showed Undo enabled and Redo disabled. Choosing Undo restored one instance, `(0, 40)`. Choosing Redo restored `(0, 26)` and `(26, 27)`. The menu popups are override-redirect, so `XQueryPointer` reported window 0, while the pointer coordinates were inside the menu rectangle that had just been measured.

With focus on the menu bar, portal Ctrl+Z restored `(0, 40)` and portal Ctrl+Shift+Z restored `(0, 26)` and `(26, 27)`. Those are `QKeySequence::Undo` and `QKeySequence::Redo` on this desktop. The same Ctrl+Z inside the range field typed `a` and then cleared that field, and the instances stayed `(0, 26)` and `(26, 27)`.

File, Close Project logged `project closed project ExtractPortalCut` and saved the database. Reopening the same folder logged `project opened project ExtractPortalCut`. Reel's visible end was 27. Range was unchecked. The readout was `No record range.`, not the extract sentence and not an older refusal. Edit, Undo and Edit, Redo were both disabled.

`import -window` took the new screenshots. It does not ask the screen-cast portal. Screenshots 11 and 12 were not overwritten.

Source was not edited. Offscreen CTest was not rerun. The last offscreen run remains 20:21 IST: slice0a passed in 3.10 sec, `aether_ffmpeg_resolution` passed in 0.03 sec, 2 of 2 passed, total 3.13 sec. Qt groups, 0 failed: TimingTests 7, ProjectTests 11, SequenceTests 10, TimelineWorkspaceTests 12, MediaImportTests 10, UiIntegrationTests 9, TimelinePlacementTests 6, MarkTests 7, MediaProbeTests 18, OverwriteTests 10, SplitTests 8, LiftTests 8, ExtractTests 12, UiCompletionTests 5.

Every button and key was released. At 21:50:28 IST the helper called `Session.Close` on `/org/freedesktop/portal/desktop/session/1_222/aether_session_14364` and exited. A later property read of that path returned `Object does not exist`. The earlier session path was already gone. `/tmp/aether-portal-input` was removed. Pid 14581 was left on the reopened ExtractPortalCut project. No second process shares its database.

SLICE 1J REMOTEDESKTOP INPUT VALIDATED — NATIVE RULER, MENUS, UNDO AND REDO COMPLETE.
