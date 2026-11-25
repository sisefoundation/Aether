# Slice 1I lift editing proposal

Status: inspection closed the selection question, then this slice implemented the command. Timeline selection already names one instance. There is no range selection to reconcile. This document does not accept an ADR, does not change schema version 2, does not add a table, and does not change `kPrototypeVersion`. ADR-006 and ADR-007 stay **PROPOSED**. ADR-008 stays **OPEN — PENDING LEGAL COUNSEL**. Accepted ADRs were not edited. Overwrite and split were not changed. Slice 1J was not started.

Inspection date: 2026-11-23. Tree: `/home/editry/development/sise-editor`. `git status` failed: `fatal: not a git repository (or any parent up to mount point /)`, exit 128. No repository was created.

## 1. What lift is

Lift is its own command. It is not split, extract, overwrite, move, or trim, and it is not the existing `removeInstance` undo kind. It takes one Timeline Instance id. That id is the selected row. The command deletes that `timeline_instances` row and leaves the record interval `[recordStart, recordEnd)` with no row on that track.

Nothing else moves. The preceding instance stays on its record range. The following instance stays on its record range. Other tracks stay. The sequence row is not written, so its name and frame rate stay. There is no duration column on `sequences`. The furthest stored record end changes only when the deleted row was the unique instance that reached it, because the empty interval is the absence of that row. A fixture that still has another instance reaching the old end keeps that end. No floating-point seconds are read or written.

Marks stay session points, off `project.db`. Lift does not read them, write them, clear them, or use them as the interval. The interval is the selected instance's stored record span.

One database transaction. One undo entry of kind Lift. A failure writes nothing and adds no undo entry. A committed lift stays gone after close and reopen. Undo is session-only, which is ADR-009. Close and open clear the undo stack. The window calls `ProjectSession` only. It does not run SQL, call FFmpeg, or validate the span itself.

## 2. Selection

The stop condition was: if the timeline selection cannot name one unambiguous instance, or an existing range selection would change that meaning, stop before implementing.

`TimelineWorkspace::instances_` is `QAbstractItemView::SingleSelection`. `selectedInstanceId()` returns the current item's instance id, or empty when there is no current item. A click on a canvas block calls `selectInstance`, which sets that same list row. The canvas paints a block as selected only when its id equals `selectedInstanceId()`. There is one authority. There is no marquee, no multi-select, and no second selected id.

Record marks are two optional frame points on the session. The readout says a stored pair is not a half-open edit range. Overwrite reads those points for its own command. That read is not a timeline selection, and lift does not perform it. Using the marks as the lift interval would be a second authority. This command does not.

An empty selection is a refusal, not an ambiguity. The button explains "no instance is selected" and does not call the session. An unknown id fails in `readInstance` with "instance was not found" before any write.

The selection can name one instance. No range selection changes that meaning. Implementation proceeds.

## 3. Empty region

Schema version 2 stores a Timeline Instance as one `timeline_instances` row with a half-open record span. There is no gap table, no placeholder clip, and no black-media asset. `docs/proposals/SLICE_1G_SAME_RATE_OVERWRITE_PROPOSAL.md` already records the live rule: an empty record range on a track is the absence of an instance, not a gap item. `placeInstance` inserts a row into that absence. Lift removes one row and leaves the absence. `EDITORIAL_OPERATIONS.md` describes a gap item and a marked span. That document authorises no implementation, and this command does not follow it.

The deleted span is empty when `listInstances` on that track has no row whose record span intersects `[r0, r1)`, and the row count is one smaller. No zero-length row is inserted. The schema check `record_end > record_start` already rejects one.

## 4. Identity

The row is deleted. `ProjectSession::lift` reads the instance, then calls `ProjectStore::deleteInstance`. That delete is one `mutate` transaction: begin, one `DELETE FROM timeline_instances WHERE instance_id = ?`, touch the project modified time, commit. The undo entry is pushed only after that commit. It stores the `TimelineInstance` captured before the delete.

Undo calls `reinsertInstance` with that captured value. `reinsertInstance` is `insertInstance`. A non-empty `instanceId` is kept, so the original id returns. The stored track id, media id, master id, source rate, source span, and record span are written back. `insertInstance` stamps `created_utc` and `modified_utc` at insert time. That stamp is the existing restore path used by `removeInstance`. This command does not add a second insert path. The editorial identity that returns is the original id, track, and ranges.

Redo calls `deleteInstance` on that same id. No new id is minted.

A failed read or a failed delete returns before `redo_` is cleared and before the undo entry is pushed. `debugArmNextMutationFailure` rolls the delete back with "write failed and was rolled back" and leaves the row.

`removeInstance` stays a separate command with undo kind RemoveInstance. Existing tests that call it are unchanged.

## 5. What this decision does not change

Schema version stays 2. The eleven tables stay. `kPrototypeVersion` stays `slice-0a`. ADR-006 and ADR-007 stay **PROPOSED**. ADR-008 stays **OPEN — PENDING LEGAL COUNSEL**. ADR-002, ADR-003, and ADR-009 stay as accepted. Overwrite and split stay. Marks stay off `project.db`. No shortcut is added. The Lift button is `liftApply`. Research notes, FFmpeg, `/opt/aether-ffmpeg-9.1`, RPATH, licensing, ShellCut, PlaceCut, MarksCut, ProbeCut, OverwriteCut, SplitCut, and earlier screenshots stay untouched. `docs/DEVELOPMENT_ROADMAP.md` stays empty.

## 6. Exclusions

Extract, ripple, insert, splice, overwrite redesign, split redesign, roll, slip, multi-track lift, mark-based range lift, patching, sync lock, gap closing, mixed rate, speed, playback, a new FFmpeg, persistent marks, a new database, and Slice 1J are not in this command.
