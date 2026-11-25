# Slice 1D timeline clip placement proposal

Status: proposal written from the source before the Slice 1D edits. It does not accept an ADR, does not change a schema, and does not authorise playback or a later slice.

Inspection date: 2026-11-22. Tree: `/home/editry/development/sise-editor`. There is no git repository.

## 1. Existing Timeline Instance creation

`ProjectSession::insertInstance` already exists. It fills a `TimelineInstance` and calls `ProjectStore::insertInstance`. The store mints an instance id when one is empty, checks that the master clip exists, copies that clip's media id, checks that the track belongs to the sequence, then calls `validateInstancePlacement`. A success is one `INSERT` into `timeline_instances` and one session undo entry of kind `InsertInstance`. Undo deletes that instance. Redo calls `reinsertInstance`. A refused insert does not push an undo entry.

`createTrack` also exists and has its own undo. A new project sequence has no track. The designed window has no track-creating control.

Nothing in the window calls `insertInstance`. The integration tests placed the live clip through a session helper. That is the gap this slice closes.

## 2. Source and record range semantics

`TimelineInstance` stores, separately:

- `sourceRate` as a reduced rational
- `sourceStart`, `sourceEnd` as `int64`, half-open, in that source rate's frames
- `recordStart`, `recordEnd` as `int64`, half-open, in the sequence's frames

`validateInstancePlacement` requires both rates to be one of the five accepted rates, both spans to be half-open, and no same-track record overlap. It does not compare the two lengths. It does not call `pictureSourceOffset`. Trim already uses that looser check, so this slice does not tighten the store. Tightening it would change trim.

`pictureSourceOffset` maps one sequence-frame offset to a source-frame offset by nearest timestamp, with an exact tie staying on the earlier frame. It is a timing query. It is not the rule `insertInstance` uses, and this slice does not adopt it as a clip-duration rule.

Imported media stores technical metadata as "Not probed". There is no source rate, source duration, codec, or resolution on the asset.

## 3. How a bin asset becomes an instance

The media list item's object id is the master clip id. `insertInstance` takes that id and stores the master's media id. The representation id is not a column on `timeline_instances`. The instance keeps the media id. Offline is a live filesystem fact, not a stored placement permission. This slice refuses an offline asset in the placement path and does not relink.

## 4. Destination track

A drop on an existing row uses that row's stored track id. Rows stay in stored order. No video or audio kind is invented.

A drop on the empty "No tracks" area, or Insert into Timeline when the sequence has no track, asks the dialog to create one track with the existing `createTrack` command and then place on it. That creation is its own undo. The insertion is a second undo. The first undo removes the instance. A later undo removes the empty track. If the insert fails, the placement path undoes the track it just created, so a refused place leaves no new track and no undo entry.

## 5. Record start

The canvas already maps a frame `n` to `x = 48 + n * 4`. The inverse, used only as a pointer reading, is:

`frame = floor((x - 48) / pixelsPerFrame)`

with `pixelsPerFrame` of 4 on the visible canvas. `x < 48` is before frame 0 and is refused. An exact boundary is the frame that starts there. A position between boundaries stays in the frame that contains it, by integer division, with no float and no snap. There is no zoom control. Tests also call the same function at 8 pixels per frame so a second scale is deterministic. That is not a new on-screen zoom.

Insert into Timeline, which has no pointer, prefills record start 0 and shows it before commit. The dialog shows the frame either way, and the editor can correct it as a non-negative integer sequence frame.

## 6. Source range

The editor types source start and source end. The labels say they are source frames at the rate chosen in the dialog, not sequence frames, and not a probed duration. The rate combo lists only 24/1, 25/1, 24000/1001, 30000/1001, and 60000/1001. Nothing is preselected. The file does not supply a rate.

## 7. How the two timings relate

`identityRecordEnd` is the placement rule, and it does not write:

- Both rates must already be accepted.
- They must be the same reduced rational. A different pair is refused with "source rate and sequence rate differ; no duration mapping is authorised."
- The source span must be half-open.
- The record span is `[recordStart, recordStart + sourceLength)`. One source frame occupies one sequence frame because the rates are the same pair. That is not a speed change.
- Overflow is refused.

A longer or shorter record span is not offered. That would be a speed change, and this slice does not add one.

`placeInstance` applies this rule before it creates a track or a row. A record end that is not that span is refused, and the refusal adds no undo entry. `insertInstance` is unchanged, so an existing unequal span can still be stored by that older command and by trim.

## 8. Overlap and invalid placement

The store's existing overlap refusal is unchanged. Abutting spans still do not overlap. A refused insert leaves the committed rows and adds no undo entry. A drop on the ruler, before frame 0, or with no sequence selected does not call the store.

## 9. Undo and redo

Success pushes the existing `InsertInstance` entry. Undo deletes the instance and redo reinserts it. The timeline and both inspectors refresh through the existing selection path. Close still drops session undo. The committed row remains.

## 10. Persistence and reopening

The row is the same `timeline_instances` insert. Reopening the project reads it back. One `project.db`. No new table and no schema change.

## 11. UI interaction

The Media list, which already shows the selected bin, becomes draggable. It gains one context action, "Insert into Timeline". Both call `beginPlacement`. During a drag the canvas draws a vertical line at the computed frame. It does not draw a clip block before commit.

The dialog names the file, the sequence, the track or the pending new track, the record in, the source rate, the source in, and the source out. When the rates match it states the record out and that one source frame occupies one sequence frame. When they do not, Place does not call the store. An offline asset is refused in that same dialog. Transform, Colour, Audio, viewers, and transport stay as they are.

## 12. Test plan

A new Qt group covers `identityRecordEnd`, the pointer mapping at the origin, a boundary, a mid-frame position, a position before frame 0, and a second pixel scale. UI tests open a disposable project, import a tiny file, drop or start Insert, commit an explicit source span at the sequence's rate, read the stored ranges, reject a different rate and an overlap, undo and redo, move and trim, close, and reopen. `ShellCut` is not opened.

## 13. Files likely to change

`include/aether/core/sequence/sequence.hpp`, `src/core/sequence/sequence.cpp`, `include/aether/ui/frame_label.hpp`, `include/aether/ui/timeline_workspace.hpp`, `src/ui/timeline_workspace.cpp`, `src/ui/main_window.cpp`, a new placement dialog, `src/CMakeLists.txt`, a new test, `tests/main.cpp`, `tests/CMakeLists.txt`, the two roadmaps, and `docs/validation/SLICE_1D_TIMELINE_CLIP_PLACEMENT.md`.

## 14. Exclusions

Playback, decoding, probing, thumbnails, waveforms, relink, conform, ripple, roll, slip, slide, overlap, snapping, subframe edits, speed ramps, track types, and a schema change. `pictureSourceOffset` is not used to size a clip.

## 15. Blockers

None for same-rate explicit placement. Mixed-rate placement stays refused because the instance command does not define a duration mapping, and this slice will not invent one. Unprobed media has no default source range.
