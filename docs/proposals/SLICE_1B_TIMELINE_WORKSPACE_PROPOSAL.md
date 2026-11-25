# Slice 1B timeline workspace proposal

Status: proposal for the authorised Slice 1B workspace. This file does not accept an ADR, does not change a schema, and does not authorise a later slice.

The inspection below is of the tree at `/home/editry/development/sise-editor` after Slice 1A. There is no git repository, so there is no branch and no HEAD.

## 1. Purpose

Put a first timeline workspace on the commands Slice 1A already commits. The workspace lists Sequences, selects one, draws Tracks in stored order, and draws clip blocks from persisted half-open record ranges. Selection and the inspector do not edit. Move and trim go through the existing session commands. A rejected command leaves the cut unchanged and says why. Undo stays in the session.

## 2. Files inspected

Source: `include/aether/core/sequence/sequence.hpp`, `src/core/sequence/sequence.cpp`, `include/aether/core/timing/sample_boundary.hpp`, `include/aether/app/project_session.hpp`, `src/app/project_session.cpp`, `include/aether/persistence/project_store.hpp`, `src/persistence/project_store.cpp`, `include/aether/core/project/project_info.hpp`, `include/aether/ui/main_window.hpp`, `src/ui/main_window.cpp`, `src/CMakeLists.txt`, `CMakeLists.txt`, `tests/sequence_tests.cpp`, `tests/project_tests.cpp`, `tests/main.cpp`.

Records: `docs/validation/SLICE_1A_EDITORIAL_SEQUENCE_FOUNDATION.md`, `DEVELOPMENT_ROADMAP.md`, `docs/19_DEVELOPMENT_ROADMAP.md`, `architecture/decisions/ADR-002-rational-time.md`, `docs/decisions/ADR-002_IMPLEMENTATION_CONSEQUENCES.md`, `docs/decisions/ADR-006_OWNER_DECISION_RECORD.md`. The owner record was read and is not edited. ADR-003, ADR-005, and ADR-009 remain the accepted persistence, identity, and session-undo rules already used by Slice 1A.

`docs/TIMELINE_UI_SPECIFICATION.md` is a proposal. It describes transport, playhead, thumbnails, waveforms, track types, snapping, ripple, roll, slip, and slide. Those are not this slice.

## 3. Slice 1A facts verified in source

| Fact | Where it is true |
| --- | --- |
| Schema version 2 | `kPrototypeSchemaVersion` is 2. `kSchemaVersionSlice0A` is 1. New databases create the original eight tables and then `sequences`, `tracks`, and `timeline_instances` |
| Version-1 migration | `migrateSequenceSchema` runs the three new statements and sets version 2 in one transaction. The armed failure path rolls back. Another version is refused |
| Sequence | Stable `sequence_id`, display `name`, one `rate_numerator` / `rate_denominator`. Rename does not mint a new id |
| Track | Stable `track_id`, parent sequence, `ordinal`. No type or role column. Empty tracks are allowed because instances are a separate table |
| Instance | Stable `instance_id`, parent sequence and track, `master_object_id`, `media_id` copied from the Master Clip. Source range and record range are separate int64 columns. No path column |
| Half-open ranges | `source_end > source_start` and `record_end > record_start`, with starts at least 0. `isHalfOpenFrameSpan` matches that |
| Overlap | `validateInstancePlacement` rejects record ranges on one track that share a frame. Abutting ranges are allowed. The message says transitions are not implemented |
| Committed edit | `moveInstance` and `trimInstance` write only after validation. Close clears the session undo vectors in `resetSessionState`. Reopen reads the rows |
| Session undo | Entries are pushed only after the store call returns success. There is no undo table |

`tests/sequence_tests.cpp` still covers the model, overlap rejection, reopen of both ranges, identity, the picture tie, the counts 12812, 12813, and 320320000, undo across close, version-1 migration, and migration rollback. `tests/project_tests.cpp` expects 11 tables and schema version 2, and still rejects an undo table and a journal table.

## 4. Discrepancies with the Slice 1A validation report

| Report | Tree |
| --- | --- |
| The window is inspection, not playback, and buttons were not clicked | `main_window.cpp` matches that. The lists are read-only. No move or trim control exists yet |
| `sample_boundary.hpp` still says ADR-002 is proposed | The comment is still there. The ADR file status is accepted. This proposal does not edit that comment or the ADR |
| Prototype version string remains `slice-0a` | `kPrototypeVersion` is still `"slice-0a"`. Schema version 2 is the value the manifest carries |
| Drop-frame skipped-number table for 59.94 is open in the ADR-002 consequences | No timecode column exists. The report correctly says timecode is a label. A stored drop-frame flag was not added |
| The ADR-006 owner record still says development is frozen at Slice 0A | Later slices were authorised by separate prompts. The record was not updated, and this slice does not update it. ADR-006 stays **PROPOSED** |
| `docs/TIMELINE_UI_SPECIFICATION.md` describes a full editorial surface | The running window does not implement that specification |

No discrepancy requires a schema change or an ADR edit before a workspace can call the existing commands.

## 5. Scope that stays out

Playback, viewers, decoding, thumbnails, waveforms, audio, effects, transitions, proxies, relink, conform, interchange, `.abin` changes, track types, a new overlap policy, persistent undo, autosave changes, crash recovery, ripple, roll, slip, slide, magnetic trim, multi-clip edits, snapping, and new dependencies. FFmpeg and GStreamer stay unlinked. `SQLite3::SQLite3` stays the link. C++17 stays prototype-only. Slice 1C is not started.

The existing same-track overlap rejection stays as Slice 1A left it. This workspace does not add a second rule and does not allow an overlap.

## 6. Decision gate

Implementation can proceed. The workspace uses existing session methods. It does not need a new architectural decision, an ADR status change, a data-model change, a second database, or a new package.

The ruler's timecode is a display count of the integer frame index at the Sequence rate. Frame 0 is Sequence frame 0. The drop-frame flag is absent, so the count numbers every frame and does not use a drop-frame table. The text is not written to `project.db`. The open 59.94 skipped-number table stays open. Pixels are used only while painting. They are not stored and they do not snap.

## 7. Workspace behavior

The existing project window gains one workspace under the bin list.

- A Sequence list shows the stored name and rate. Choosing a row selects that Sequence. Selection is widget state.
- Tracks of that Sequence are shown in stored `ordinal` order, including a track with no instances.
- Each instance is a block whose edges are the stored record range. The block has no thumbnail and no decoded picture.
- An instance list selects one instance. The inspector shows identity, track, both ranges, and the integer duration `record_end - record_start`. The inspector does not write.
- Move asks for one non-negative integer record start and calls `ProjectSession::moveInstance`.
- Trim asks for four integer frame edges and calls `ProjectSession::trimInstance`.
- The window does not include the store header and does not issue SQL.

## 8. Time, ruler, and drawing

The ruler labels integer frames. For an accepted rate whose denominator is 1, the label second holds that many frames. For 24000/1001, 30000/1001, and 60000/1001 the label second holds 24, 30, and 60 frames. That is integer division of the frame index. It is not a float second and it is not a drop-frame numbering. The caption says the drop-frame flag is absent.

Drawing uses a fixed integer pixel width per frame so a block's left edge and width are integer products of the record range. A block that does not fit in a 32-bit paint coordinate is omitted from the paint. Its stored range is still shown in the inspector. No zoom, scroll origin, or snap target is stored.

## 9. Selection, inspector, move, and trim

Selection changes no row. Move preserves the source range and the record duration because that is what `moveInstance` already does. Trim replaces both stored ranges with the four integers the editor submitted. The workspace does not derive one range from the other.

A value that is not a non-negative integer frame is rejected in the widget. The session is not called. A value the session rejects, including a same-track overlap or an empty span, is shown as that command's message. The following read of the instance still shows the previous ranges.

## 10. Undo and persistence

Successful move and trim already push one session undo entry and clear redo. The workspace does not push its own entry. A parse failure and a failed command push nothing. Undo and redo buttons call `ProjectSession::undo` and `redo`, then reload from the session. Closing the project still drops the undo stack and keeps the last committed rows. Reopen shows those rows and `canUndo()` is false.

No migration, no new table, and no undo database.

## 11. Tests

New Qt Test cases on the workspace, without screen coordinates and without media files:

- selection and refresh
- track order, including an empty track
- instance record start and duration
- inspector text for the selection
- a valid move and a move the session rejects
- a valid trim and a trim the session rejects
- integer frame labels, and rejection of a fractional token
- undo and redo of a successful edit
- a failed command leaves `canUndo()` false when the stack was empty
- close and reopen keep the committed range and drop undo
- an empty Sequence and an empty Track
- the existing TimingTests, ProjectTests, and SequenceTests stay in the same executable

CTest count and Qt Test case count are reported separately. The build directory is `build-slice1b/`. `build/`, `build-slice0b/`, and `build-slice1a/` stay. Logs go under `validation-fixtures/slice-1b/`.

## 12. Implementation boundary

Phase 2 may add the workspace widget, the integer label helper, tests, the Slice 1B validation report, and, only after the acceptance checks pass, status text in `DEVELOPMENT_ROADMAP.md` and `docs/19_DEVELOPMENT_ROADMAP.md`.

Phase 2 may not edit an ADR, the ADR-006 owner record, the schema, the media tables, or `docs/DEVELOPMENT_ROADMAP.md`. It may not link FFmpeg or GStreamer, add a package, or start Slice 1C.
