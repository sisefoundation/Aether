# Aether open-source NLE code study

Inspection date: 2026-11-22. Checkouts live only under `/tmp/aether-professional-nle-study/`. They were not copied into the Aether tree, not built, and not installed. No dependency was added to Aether.

Shallow clones pin one commit. Parent history was not fetched. Line numbers are from those trees as checked out.

| Repository | Commit | Branch reported by clone | Commit time | How obtained |
| --- | --- | --- | --- | --- |
| `https://github.com/KDE/kdenlive.git` | `f5ad17c990589ffba60eb31a875f70ec338407d1` | `master` | 2026-11-21 18:40:36 +0100 | `git clone --depth 1` |
| `https://github.com/blender/blender.git` | `adc04d05b693a26fc6567357f4c456b60d0221a8` | default sparse checkout | 2026-11-21 15:38:32 +0100 | `git clone --filter=blob:none --sparse --depth 1`, then sparse paths listed below |
| `https://github.com/mltframework/shotcut.git` | `2eba56a72af3b7806876f2a2956cfff789db1468` | `master` | 2026-11-21 10:21:37 -0800 | `git clone --depth 1` |
| `https://github.com/mltframework/mlt.git` | `34088d6f47ae8896ab71101ddd72f600b3a18f8d` | `master` | 2026-11-20 16:36:02 -0800 | `git clone --depth 1` |

Blender sparse paths that were materialised: `source/blender/sequencer`, `source/blender/editors/space_sequencer`, `source/blender/makesdna`, `source/blender/makesrna/intern`, `tests/python`, `COPYING`. `source/blender/blenloader` was not checked out. Blend-file write of strips was therefore not traced. `doc/license/GPL-license.txt`, which `COPYING` points at, was not in the checkout.

Official layout note used for Blender: `https://developer.blender.org/docs/features/sequencer/`. It says core sequencer code is `source/blender/sequencer` and editor code is `source/blender/editors/space_sequencer`, and that DNA fields should not be the place editor code mutates strips ad hoc. That matches the split observed in this commit.

MLT framework overview used: `https://mltframework.org/docs/framework/`.

## 1. Kdenlive

### Licence notices actually read

- Repository `COPYING` is the GNU GPL version 3 text.
- `src/timeline2/model/timelinemodel.cpp` and `src/timeline2/view/qml/Timeline.qml` both carry `SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL`.
- Copyright lines on `timelinemodel.cpp` name Nicolas Carion, 2017. The QML file names Jean-Baptiste Mardelle, Nicolas Carion, Sashmita Raghav, Julius Künzel, and Abdias J.

The file SPDX is an OR expression. It is not recorded here as a single identifier. No Kdenlive source was copied.

### Objects

- Bin items: `ProjectItemModel` (`src/bin/projectitemmodel.cpp`). A dragged clip id is the bin id, optionally `A` or `V` prefixed, optionally `id/in/out`.
- Timeline: `TimelineModel` / `TimelineItemModel` (`src/timeline2/model/timelinemodel.cpp`, `timelineitemmodel.cpp`). Tracks are `TrackModel`. Clips on a track are clip models inserted by `TrackModel::requestClipInsertion`.
- UI: QML `Timeline.qml` calls into `TimelineController` (`src/timeline2/view/timelinecontroller.h`), which is exposed to QML. `TimelineWidget` (`src/timeline2/view/timelinewidget.cpp`) is the Qt widget around that QML root.
- Zones and lift/extract: `TimelineFunctions` (`src/timeline2/model/timelinefunctions.cpp`).
- Undo: `Fun` lambdas pushed with the `PUSH_UNDO` macro (`src/macros.hpp`) onto `FunctionalUndoCommand`, or `Core::pushUndo` (`src/core.cpp`).
- Project file: `KdenliveDoc::saveSceneList` (`src/doc/kdenlivedoc.cpp`) writes a QDom scene list. The producer walk inside `xmlSceneList` was not followed line by line.

Duration from a dropped zone is inclusive at this layer: `requestClipInsertion` sets `duration = out - in + 1` when the mime id contains two slashes (`timelinemodel.cpp` around the `binIdWithInOut.count` check). That is not Aether's half-open span.

### Call chain: bin drag to a committed insert

1. `ProjectItemModel::mimeData` builds `text/producerslist`. The comment in that function says a clip zone is encoded `2/10/200` for bin id, in, and out. Audio-only and video-only prefixes `A` and `V` are applied from `m_dragType`.
2. QML `DropArea` `clipDropArea` accepts key `text/producerslist` (`Timeline.qml`).
3. While the drag moves, `insertAndMaybeGroup` calls `root.timeline.insertClip(...)` with `logUndo` false, so the preview insert is not an undo step. `suggestClipMove` proposes a snapped frame. In insert/overwrite edit mode the preview is inserted at `fullDuration` and then fake-moved.
4. On drop, `processDrop` deletes the preview with `requestItemDeletion(cid, false)`, then calls `insertClip` again with `logUndo` true. If the controller is not in normal edit, it calls `insertClipZone` instead. A multi-clip drop in non-normal mode logs `multiple clips insert/overwrite not supported yet` and does not insert. That TODO was in the file.
5. `TimelineController::insertClip` (`timelinecontroller.cpp`) resolves track `-1` to the active track and position `-1` to `pCore->getMonitorPosition()`, then calls `TimelineModel::requestClipInsertion`.
6. The first `requestClipInsertion` overload builds undo and redo lambdas, optionally restricts tracks to those with `shouldReceiveTimelineOp`, and on success calls `PUSH_UNDO(undo, redo, i18n("Insert Clip"))`. On failure after `logUndo` it runs `undo()` so a partial track creation does not remain.
7. The second overload parses the bin id, rejects a missing bin clip, rejects dropping a sequence onto itself (`canBeDropped`), computes duration, and checks track locks and `isAvailable` for the span when `m_editMode == TimelineMode::NormalEdit`. The function continues into audio-stream fan-out and `TrackModel::requestClipInsertion`. The body is thousands of lines. This study followed it through the availability checks and did not re-read every audio-mirror branch.
8. `TrackModel::requestClipInsertion` (`trackmodel.cpp`) takes the `Fun` undo and redo pair. The lambda helper is `requestClipInsertion_lambda`.
9. Undo is one stack entry of those lambdas, not a second copy of the MLT graph stored by the command itself. Redo re-runs the stored lambda chain.
10. Persistence is the document scene list, not this undo stack. `KdenliveDoc::saveSceneList` refuses a null scene list, then writes UTF-8 XML and calls `backupLastSavedVersion` first. Which timeline fields the scene list includes was not fully traced.

Keyboard insert of the monitored zone is a different path, and it was traced:

`MainWindow::slotInsertClipInsert` → `TimelineController::insertZone(..., overwrite=false)` → `TimelineFunctions::insertZone` → `Core::pushUndo` with text "Insert zone".

`MainWindow::slotInsertClipOverwrite` is the same with `overwrite=true` and undo text "Overwrite zone".

`TimelineFunctions::liftZone` followed by `removeSpace` is the extract path. The undo text is "Lift zone" or "Extract zone" (`timelinefunctions.cpp`, the `liftOnly` branch). Lift and extract are separate operations in this code, matching the professional distinction.

### Trim

`TimelineController` calls `m_model->requestItemResize` while a trim is dragged (`timelinecontroller.cpp`, two call sites around the resize handlers). `TimelineModel::requestItemResize` takes `itemId`, a size reference, a `right` flag, `logUndo`, and undo/redo functors. Tests in `tests/trimmingtest.cpp` call `requestClipMove` and `TimelineFunctions::requestClipCut`. `tests/spacertest.cpp` calls `requestClipInsertion`. Those tests were not executed.

### What this architecture shows, without adopting it

The QML view asks a controller, and the controller asks the timeline model. The model returns false instead of inserting when the track is locked or the span is occupied in normal mode. Undo is a lambda pair pushed only after success. Preview drags pass `logUndo` false, and the real drop repeats the insert with undo on. That preview technique mutates the model during the drag and deletes it if the drag leaves. Aether does not need that pattern. The useful split is: the view does not write the project, and a refused edit does not become an undo entry.

Kdenlive's timeline is also an MLT-backed document. Editorial rules and the media graph are not separated the way ADR-006 proposes. ADR-006 remains proposed. This study does not adopt MLT inside Aether.

## 2. Blender Video Sequence Editor

### Licence notices actually read

- `COPYING` says Blender uses the GNU GPL and points at `doc/license/GPL-license.txt`. That second file was not in the sparse checkout, so its text was not read.
- `source/blender/sequencer/intern/strip_add.cc` SPDX: `GPL-2.0-or-later`. Copyright lines name NaN Holding BV (2001-2002), Blender Authors (2003-2009), and Peter Schlaile.
- `source/blender/editors/space_sequencer/sequencer_drag_drop.cc` SPDX: `GPL-2.0-or-later`, copyright 2022 Blender Authors.

No Blender source was copied.

### Split the developer document describes

Core: `source/blender/sequencer/` (`SEQ_add.hh`, `strip_add.cc`, `strip_edit.cc`, `strip_transform.cc`, cache and render). Editor: `source/blender/editors/space_sequencer/` (operators, drag-drop, drawing). Operators carry `OPTYPE_UNDO`. Blender's undo system then snapshots the operator. This study did not open the global undo implementation.

A strip's timeline placement fields used by the drop preview include `start`, `content_length_set`, `channel`, `speed_factor`, and `media_playback_rate` (`sequencer_drag_drop.cc`, the dummy strip in `update` overlap testing). Scene frame rate is `scene->r.frs_sec` / `scene->r.frs_sec_base` when a movie load syncs FPS (`strip_add.cc`). That is a float scene rate in the load path that was read. It is not a model Aether can adopt under ADR-002.

### Call chain: drop a movie onto the sequencer

1. `space_sequencer.cc` assigns `st->dropboxes = sequencer_dropboxes`.
2. `sequencer_dropboxes_add_to_lb` (`sequencer_drag_drop.cc`) registers drop boxes whose operator id is `SEQUENCER_OT_movie_strip_add` (and image, sound, and other types). Poll is `movie_drop_poll`. Copy callback is `sequencer_drop_copy`.
3. `sequencer_drop_copy` writes RNA `frame_start`, `channel`, and `overlap_mode` from the mouse-derived `g_drop_coords`. Shift, or `g_drop_coords.use_ripple`, sets `SEQ_OVERLAP_RIPPLE`. Otherwise the mode is `seq::tool_settings_overlap_mode_get(scene)`. Overlap itself is tested with `seq::transform_test_overlap` against a dummy `Strip`.
4. `SEQUENCER_OT_movie_strip_add` (`sequencer_add.cc`) has `invoke = sequencer_add_movie_strip_invoke`, `exec = sequencer_add_movie_strip_exec`, `poll = ED_operator_sequencer_active_editable`, and `flag = OPTYPE_REGISTER | OPTYPE_UNDO`.
5. Invoke detects drag-and-drop via `is_drag_and_drop` and then calls the exec directly. A non-drop invoke opens a file browser (`WM_event_add_fileselect`).
6. Exec calls `sequencer_add_generic_exec`, then `sequencer_add_movie_strips`, then `seq_build_proxy`, `DEG_relations_tag_update`, `DEG_id_tag_update(..., ID_RECALC_SEQUENCER_STRIPS)`, `sequencer_select_do_updates`, and `move_strips`.
7. `sequencer_add_movie_strips` was not read in full. It leads to `seq::add_movie_strip` (`strip_add.cc`, declared in `SEQ_add.hh`). That function absolutises the path, opens the movie with `openanim`, and can copy the file's FPS onto `scene->r.frs_sec` and `scene->r.frs_sec_base` when `SEQ_LOAD_MOVIE_SYNC_FPS` is set. If the file cannot be opened and `allow_invalid_file` is false, it returns null. The rest of strip-field initialisation after the FPS block was not read line by line.
8. Undo is the operator flag, not a hand-written command inside `add_movie_strip`. Redo is Blender's operator redo. The memfile implementation was not opened.
9. Persistence is the blend file. The writer was not in the sparse checkout, so the serialisation call chain stops at the DNA strip living on the scene. This limitation is explicit.

### Trim and other edits

`SEQUENCER_OT_slip` (`sequencer_edit.cc`) is a modal operator: `sequencer_slip_invoke`, `sequencer_slip_modal`, `sequencer_slip_exec`, flag `OPTYPE_BLOCKING | OPTYPE_REGISTER | OPTYPE_UNDO`. The modal applies `slip_strips_delta` and notifies `NC_SCENE | ND_SEQUENCER`. The description string is "Slip the contents of selected strips".

The same file also defines `SEQUENCER_OT_split`, `SEQUENCER_OT_delete`, `SEQUENCER_OT_ripple_delete`, `SEQUENCER_OT_ripple_trim`, `SEQUENCER_OT_gap_insert`, and `SEQUENCER_OT_gap_remove`. Their execute bodies were not each read. Their existence is enough to show that delete and ripple-delete are different operators.

Python tests present in the checkout, not run: `tests/python/sequencer_render_tests.py`, `sequencer_strip_naming.py`, `sequencer_load_meta_stack.py`, `sequencer_input_colorspace.py`, `sequencer_video_output_tests.py`, `tests/python/ui_simulate/test_sequencer.py`. No C++ sequencer unit test was opened.

### Lesson for Aether's layering

Editor code (drop, mouse, operator) passes a frame, a channel, and an overlap mode into sequencer functions. It does not itself invent the strip. Cache and dependency-graph tags are updated after the edit (`ID_RECALC_SEQUENCER_STRIPS`), not inside the widget. That is the separation Aether's UI / session / store split is already aiming at. Blender's scene frame rate is a float pair written onto the scene when a movie is added. Aether must not copy that. ADR-002 already names the legal rates and forbids float seconds as authority.

## 3. Shotcut

### Licence notices actually read

- Repository `COPYING` is the GNU GPL version 3 text.
- `src/models/multitrackmodel.cpp` header says version 3 or any later version, copyright 2013-2026 Meltytech, LLC. That header is GPL-3.0-or-later wording. The COPYING file is the GPL-3 text. Both are recorded. They are not collapsed into one SPDX guess beyond what the header says.
- `src/commands/timelinecommands.cpp` was read for command behaviour. Its header was not separately quoted. The file is in the same GPL-licensed tree. No Shotcut source was copied.

### Objects

- UI timeline is QML (`src/qml/views/timeline/timeline.qml`, `Timeline.js`) talking to `TimelineDock` (`src/docks/timelinedock.cpp`).
- The editorial/media object is an MLT tractor owned by `MultitrackModel` (`src/models/multitrackmodel.cpp`). Each track index maps to `mlt_index` on that tractor. A track producer is wrapped as `Mlt::Playlist`.
- Undo commands are `QUndoCommand` subclasses in `src/commands/timelinecommands.h`: `AppendCommand`, `InsertCommand`, `OverwriteCommand`, `LiftCommand`, `RemoveCommand`, `TrimClipInCommand`, `TrimClipOutCommand`, `SplitCommand`, `ReplaceCommand`, and track lock/mute/hide commands. `MAIN.undoStack()` is the stack.

### Call chain: drop onto the timeline

1. `timeline.qml` `DropArea.onDropped` accepts `application/vnd.mlt+xml` or URLs.
2. `Timeline.js` `acceptDrop` converts the drop x to a frame with `Math.round((dropTarget.x + tracksFlickable.contentX - headerWidth) / multitrack.scaleFactor)` and calls `timeline.handleDrop(timeline.currentTrack, position, xml)`.
3. `TimelineDock::handleDrop` (`timelinedock.cpp`) splits file URLs, opens them through `MLT.checkFile` and builds a playlist, or treats the payload as MLT XML. The URL branch was read through the file loop start. The later call that reaches overwrite was confirmed by `overwrite(...)` call sites in the same function (around the drop handling that calls `overwrite(trackIndex, position, xml, false)`).
4. `TimelineDock::overwrite` rejects a locked track (`isTrackLocked` → `warnTrackLocked` and return). It builds an `Mlt::Producer` from XML. For a Shotcut-marked tractor it loops source tracks and pushes one `Timeline::OverwriteCommand` per clip. The single-clip path pushes `new Timeline::OverwriteCommand(m_model, trackIndex, position, xmlToUse, seek)` (call around line 4767).
5. `OverwriteCommand::redo` (`timelinecommands.cpp`) constructs `Mlt::Producer` from `xml-string`, snapshots existing clip XML with `m_undoHelper.storeXmlForClip`, calls `m_undoHelper.recordBeforeState`, then `m_model.overwrite`. `OverwriteCommand::undo` calls `m_undoHelper.undoChanges()`.
6. `MultitrackModel::overwrite` wraps the tractor track as a playlist. If the position is past the end it inserts blank then `playlist.append(clip.parent(), in, out)`. Otherwise it finds the clip index at the position, `splitClip`s the head if the position is mid-clip, deletes covered playlist entries, and appends the new producer. The Qt model emits `beginInsertRows` / `beginRemoveRows` around those playlist mutations so the QML view updates.
7. Redo of the same `QUndoCommand` runs `redo()` again. The command text is "Overwrite onto track".
8. Project persistence is the MLT XML document Shotcut already uses as its project. The save function was not opened in this pass. The timeline state that undo restores is MLT XML stored on the command, which is a different object from Aether's row-level undo.

### Insert, lift, trim

`InsertCommand::redo` calls `MultitrackModel::insertClip(..., rippleAllTracks, ...)`. If `m_rippleAllTracks` is true, the undo helper is told `RestoreTracks` because the comment says the fine-grained diff does not track shifts on other tracks. `insertClip` (`multitrackmodel.cpp`) splits the destination clip when the position is mid-clip, then inserts. The ripple-all branch after that split was not read to the last line. The command exists, the model method exists, and the ripple flag changes undo strategy. That is what was verified.

`TimelineDock` pushes `TrimClipInCommand` and `TrimClipOutCommand` (around lines 2292 and 2310) and `SplitCommand` (around lines 1109 and 1143). `LiftCommand` and `RemoveCommand` are separate classes. Their redo bodies were not read. Shotcut's own names already refuse to treat lift and remove as one operation.

`src/tests/` in this checkout did not show a timeline overwrite test in the search that was run. Absence from that search is not proof none exists under another name. None was executed.

### Lesson

Shotcut's UI is Qt/QML and its cut is an MLT playlist. The undo command snapshots MLT XML and then calls the model. The view's `Math.round` pixel mapping is a display conversion. Aether's placement code already uses integer division for the pointer and does not use that round. Do not import Shotcut's pixel rule.

Track type is real in Shotcut (`VideoTrackType`, `AudioTrackType` on `InsertTrackCommand`). Aether's current model has no track type. Copying Shotcut's track enum would be an architectural decision, not a research finding.

## 4. MLT framework

### Licence notices actually read

- Repository `COPYING` is the GNU LGPL version 2.1 text.
- `src/framework/mlt_playlist.c` header: LGPL-2.1 or later, copyright 2003-2026 Meltytech, LLC.
- `src/modules/avformat/factory.c` header: the same LGPL-2.1-or-later wording.

The framework files read are LGPL-2.1-or-later. This study did not open every module. Other modules can differ. Linking MLT, or any module, is not approved by this document. ADR-008 remains the legal-counsel item already in the tree. No MLT source was copied and no MLT dependency was added.

### Model, from the files and the framework document

A producer is the source of frames. A playlist is an ordered list of producers, including blanks, on one timeline. A multitrack stacks playlists. A tractor is a producer that owns a multitrack and a field of transitions. `mlt_tractor_new` (`src/framework/mlt_tractor.c`) calls `mlt_producer_init`, `mlt_multitrack_init`, and `mlt_field_new`, and sets resource `<tractor>`. Filters and transitions are services. A consumer pulls frames out.

`mlt_position` (`src/framework/mlt_types.h`) is `double` if `DOUBLE_MLT_POSITION` is defined, otherwise `int32_t`. The default compile flag was not found in a CMake search of this checkout (`DOUBLE_MLT_POSITION` had no CMake hit). Either representation is incompatible with Aether's stored `int64` frame authority if it is used as the editorial clock. A 32-bit position overflows a long timeline. A double position is the float clock ADR-002 rejected.

In and out on a playlist entry are inclusive end points in the API that was read. `mlt_playlist_insert` (`mlt_playlist.c`) appends with `mlt_playlist_append_io(self, producer, in, out)` and then `mlt_playlist_move`s the new entry to `where`. It then `mlt_playlist_virtual_refresh`. `mlt_playlist_insert_at` takes a time `position` and a `mode` the comment describes as overwrite of a blank section. Only the blank-split start of that function was read.

`mlt_playlist_remove` closes the gap in the playlist array and seeks the playlist producer earlier by `where_info.frame_count` when the removed index is before the current clip. That is a playlist-length change, closer to extract than to lift. Lift in Shotcut is a higher-level command that leaves blank. The framework playlist does not have Aether's half-open record range.

Tests present, not run: `src/tests/test_playlist/test_playlist.cpp`, `src/tests/test_multitrack/test_multitrack.cpp`, `src/tests/test_tractor/test_tractor.cpp`, `src/tests/test_producer/test_producer.cpp`, `src/tests/test_frame/test_frame.cpp`.

### Trade-off, not a recommendation to adopt

MLT already implements multitrack composition, in/out trimming, blank, transitions, filters, and consumers. Shotcut and Kdenlive both use it as the timeline, which is why their undo and their project file are MLT XML. Aether's accepted direction is different: the project database is the editorial authority (ADR-003), the application is the proposed joiner (ADR-006, still proposed), and processing must not own the cut. Using MLT as a hidden timeline would put the cut in the graph. Using MLT only as a later playback consumer, fed by a composition the session built, is a possible boundary. It is not chosen here. It needs a licence review, an ADR-006 decision, and a time-conversion review, because MLT's position type is not Aether's `int64` frame.

## 5. Patterns worth keeping, and patterns that conflict

Worth keeping as ideas, not as code:

- UI event, then a single application command, then a model function that validates, then one undo record, then a view refresh from the model.
- Insert and overwrite as different functions (`TimelineFunctions::insertZone` overwrite flag; Shotcut `InsertCommand` versus `OverwriteCommand`; Blender overlap mode including ripple).
- Lift and extract as different functions (Kdenlive lift versus extract; Shotcut `LiftCommand` versus `RemoveCommand`; Blender delete versus ripple delete).
- A refused edit does not push undo (Kdenlive `requestClipInsertion` failure path; Shotcut locked-track return before `OverwriteCommand` is pushed).
- Editor code and edit logic in different directories (Blender's published split).

Conflicts with Aether's accepted rules:

- Inclusive in/out (`out - in + 1` in Kdenlive; MLT in/out) versus half-open ranges.
- Float or 32-bit positions (MLT; Blender scene FPS as a float pair) versus `int64` and reduced ratios.
- One project document that is the media graph (Kdenlive scene list, Shotcut MLT XML, Blender scene strips with linked movie paths) versus one `project.db` plus a proposed processing boundary.
- Track audio/video kinds in Shotcut and Kdenlive versus no track type in the current Aether model.

## 6. Call chains that were not finished

- Kdenlive: the full audio-mirror branch inside `TimelineModel::requestClipInsertion`, and every property `xmlSceneList` writes.
- Blender: `sequencer_add_movie_strips` after the call, the remainder of `add_movie_strip` after FPS sync, blend-file serialisation, and the global undo memfile.
- Shotcut: the URL half of `handleDrop` after the file loop starts, the ripple half of `insertClip` after the mid-clip split, `LiftCommand::redo`, and the project save function.
- MLT: `mlt_playlist_insert_at` after the blank split, and which modules are not LGPL.

Where a chain is marked unfinished above, it was not guessed.
