# Slice 1J extract editing proposal

Status: design only. Implementation has not started. On 2026-11-23 the owner chose track scope C, a legal extract of an interval that intersects nothing on the named tracks, deletion or shifting of an uncut unequal or mixed-rate instance, the visible end as the maximum stored `record_end` described as shortening by `D`, and retention of the instance id on an uncut shift. Those choices are recorded in section 1. They do not authorise a command, a test, or a control. On 2026-11-23 the owner approved Dedicated Timeline Range Selection for how `r0` and `r1` are supplied. The approved model is session-only `[r0, r1)`, `r0 < r1`, integer sequence frames, independent of marks, clip selection, and named tracks. Implementation of Extract has not started. This document does not accept an ADR, does not change schema version 2, does not add a table or a column, and does not change `kPrototypeVersion`. ADR-006 and ADR-007 stay **PROPOSED**. ADR-008 stays **OPEN — PENDING LEGAL COUNSEL**. Accepted ADRs were not edited.

Inspection date: 2026-11-23. Tree: `/home/editry/development/sise-editor`. `git status` failed: `fatal: not a git repository (or any parent up to mount point /)`, exit 128. No repository was created. No commit, no push, and no package was installed.

Four labels are used below.

- **Verified** is behaviour read from the current source, or from the Slice 1G, 1H, and 1I proposals and validation reports.
- **Proposed** is the extract command this document describes. It is not code.
- **Decided** is an owner choice recorded here. It binds only the item it names.
- **Open** is a decision only the owner can close. How `r0` and `r1` are supplied was closed on 2026-11-23 by the decision block.

## Decision: How are Extract's Record interval boundaries supplied?

**Decision.** How are Extract's Record interval boundaries supplied?

**Recommendation.** Dedicated Timeline Range Selection.

**Status.** **OWNER APPROVED** on 2026-11-23. The approved model is session-only `[r0, r1)`, `r0 < r1`, integer sequence frames, independent of marks, clip selection, and named tracks.

The owner approved this model on 2026-11-23. Implementation of Extract has not started. Numeric `r0` and `r1` entry alone, mark-based selection, and an interval derived from selected instances remain alternatives the owner has not rejected.

### Recommended model

**Proposed.** The interval is a dedicated range on the active Sequence Timeline. Its canonical form is `[r0, r1)` with `r0 < r1`. Both boundaries are integer sequence frames under ADR-002. Section 2 already admits the interval only when `isHalfOpenFrameSpan(r0, r1)` is true, so `r0 >= 0` and `r1 > r0`. ADR-002 is not edited. Its deferred displayed out-point stays deferred. The Start and End labels below belong to this proposed control.

The range is independent of selected instances, source marks, record marks, source ranges, and named-track participation. It may cover multiple instances, empty regions, and boundaries that are not clip edges.

### Interaction

**Proposed only.** No control is added.

The editor uses a dedicated Timeline Range Selection mode and sets two exact frame boundaries. The UI shows Start inclusive and End exclusive. Either boundary accepts integer numeric adjustment. The editor reviews the interval before Extract. A mouse coordinate may resolve to an integer sequence frame. That coordinate is never the stored authority. There is no float-second field. Clip selection is not the interval source. A mark is not the interval source.

**Verified.** `frameAtCanvasX` maps a canvas x to an `int64` by integer division, `floor((x - origin) / pixelsPerFrame)`. Its comment says there is no snap and no float. A ruler click writes that integer into the record-mark frame field and does not store the pixel. The paint constants are commented as not stored. That path fills mark entry. It is not an extract interval.

### Session

**Proposed.** The range belongs to the active Sequence. It is session state only. It is not written to `project.db`. Storing it or clearing it does not modify marks. It is cleared when the Sequence changes. It is cleared when the Project closes. It is kept after a rejected Extract. It is cleared after a successful Extract. No new schema is added. Schema stays 2.

**Verified session boundary.** `ProjectSession` has no range field and no active-sequence field. Its session members are the undo stack, the redo stack, `sourceMarks_`, `recordMarks_`, and `selectedBinId_`. The mark calls are commented as session context only, not a `project.db` row and not an undo entry. `src/persistence/project_store.cpp` creates the eleven tables in section 3. It creates no mark table and no range table. `kPrototypeSchemaVersion` is 2.

`closeProject` calls `resetSessionState` before `store_->close()`. Successful `createProject` and successful `openProject` call the same reset. The reset clears both stacks, both mark maps, and `selectedBinId_`. That is the close boundary for the session state that exists. The range is not in that function, so a clear on project close is a proposed rule for a field the code does not have.

The timeline shows one sequence. `sequences_` is single selection, and that id is widget state. Changing it runs `showSelectedSequence`, which reloads the instance list and reads `recordMarks` for the newly selected id. A change of displayed sequence id clears the record-mark frame field and its reason text in the widget. The mark maps stay. Undo stays. A mark stored under one sequence id remains until project create, open, or close. A clear of the range when the Sequence changes is proposed. Record marks and undo remain across that change.

**Unverified.** Extract is not implemented. The code has no range to keep after a rejected Extract, no range to clear after a successful Extract, and no range write that could show marks left unchanged. Those three rules stay proposed.

### Three independent selections

**Proposed.** Record Range, Named Tracks, and ordinary Timeline Instance selection are three selections. None is inferred from another.

1. **Record Range.** The proposed `[r0, r1)` on the active Sequence Timeline.
2. **Named Tracks.** The explicit list from the owner decision of 2026-11-23. Extract, when later authorised, closes only the tracks the editor names for that operation. The program does not fill the list from the selected instance or from every track. Unlisted tracks stay. No sync group, link, or patch is added.
3. **Timeline Instance selection.** The existing single selected instance. Lift and split read that id. The id does not supply `r0` or `r1`, and it does not fill the track list.

Selecting an instance leaves the range and the named-track list unchanged. Setting the range leaves the instance selection and the named-track list unchanged. Naming tracks leaves the range and the instance selection unchanged.

### Comparison

This comparison is the recommendation. The owner has not rejected numeric entry, mark-based selection, or derivation from selected instances.

| | Dedicated Timeline Range Selection | Numeric `r0` and `r1` alone | Mark-based selection | Interval from selected instances |
| --- | --- | --- | --- | --- |
| Standing | Recommended. **OWNER APPROVED** on 2026-11-23. | An alternative. | An alternative. Section 1 records that record marks and source marks stay excluded. | An alternative. Section 1 records that one selected timeline instance is not sufficient authority for the interval. |
| What the editor sets | A range on the active Sequence Timeline | Two integers, with no range shown on the timeline | Source marks or record marks | The record span of the selected instance |
| Clip edges | A boundary can sit inside a clip or on empty frames | A typed frame can sit inside a clip or on empty frames | A mark is a point. The `MarkPoints` comment says the two points are not a half-open edit range | The span is that instance's `record_start` and `record_end` |
| Empty regions and several instances | One range may cover empty frames and several instances, and it may cut inside one instance | The same two integers can name that span | Overwrite is the caller that derives a span from marks. The pair is still two points | The current list is one instance. Its edges are the whole clip. An empty interval contains no selected instance |
| Separate from the other two selections | The range does not read instance selection, marks, source ranges, or the named-track list | Two typed frames can stay separate from all three. They give the editor no timeline range to review | The interval would come from marks | The interval would come from instance selection |

**Why a range that is not tied to clip edges fits Extract.** Extract removes `[r0, r1)` on the named tracks and closes that interval by `D = r1 - r0`. Section 4 classifies each instance against those two frames. The cut lands at `r0` or `r1`, including a frame inside a clip and a frame that meets no clip. The decision of 2026-11-23 says an interval that intersects no instance on the named tracks is a legal extract, and later material on those named tracks still moves earlier by `D`. A span taken from clip edges cannot name that empty interval, and it cannot place a boundary inside a clip while leaving the rest of the clip in place. Numeric entry can name the same two frames. It remains an alternative the owner has not rejected. It does not place a range on the Sequence Timeline for review before Extract. Marks remain two optional points in one domain, and the mark type says they are not a half-open edit range.

Implementation of Extract has not started.

## 1. Owner decisions

The choices below are recorded. None of them authorises implementation. How `r0` and `r1` are supplied was approved on 2026-11-23. The decision block records that approval.

1. **Track scope. Decided on 2026-11-23.** The owner chose C. Extract, when later authorised, closes only the tracks the editor names for that operation. The program does not fill the list from the selected instance or from every track. Unlisted tracks stay. No sync group, link, or patch is added. A and B were not chosen. Section 6 keeps their analysis.
2. **Interval authority. Owner approved on 2026-11-23.** The owner approved Dedicated Timeline Range Selection. The approved model is session-only `[r0, r1)`, `r0 < r1`, integer sequence frames, independent of marks, clip selection, and named tracks. Record marks and source marks stay excluded. One selected timeline instance is not sufficient authority for the interval, and it is not authority for every track.
3. **Empty interval. Decided on 2026-11-23.** An interval that intersects no instance on the named tracks is a legal extract. Later material on those named tracks still moves earlier by `D`. No row is deleted. Unlisted tracks stay.
4. **Uncut unequal and mixed-rate rows. Decided on 2026-11-23.** An uncut unequal or mixed-rate instance may be deleted or shifted. Its source span is left unchanged when it is shifted, and it is removed with the row when it is deleted. A crossing unequal or mixed-rate instance remains a blocker and writes nothing.
5. **Visible end. Decided on 2026-11-23.** The visible end is the maximum stored `record_end`. A successful extract is described as shortening that end by `D`. No duration column is added. Schema stays 2. That description is not a universal success rule. `candidateEnd` is the unselected overflow check. The stored visible end remains the furthest `record_end`. Section 8 states the distinction.
6. **Identity of an uncut shift. Decided on 2026-11-23.** An uncut instance shifted earlier by `D` keeps its instance id. Only `record_start` and `record_end` change, each by `-D`. The source span, source rate, track, media id, and master id stay.

## 2. What extract is

**Proposed.** Extract is one command. When later authorised, it removes the half-open record interval `[r0, r1)` on each track the editor names for that operation and closes that interval by moving later material earlier by `D = r1 - r0`. The program does not fill that list from the selected instance or from every track. Unlisted tracks stay. `r0` and `r1` are non-negative `int64` sequence frames. `D` is a positive integer frame count in the sequence rate. The interval is legal only when `isHalfOpenFrameSpan(r0, r1)` is true, so `r0 >= 0` and `r1 > r0`.

On a closed track the command does all of the following in one transaction:

- material wholly inside `[r0, r1)` is deleted
- a piece that crosses either boundary is cut with the mapping in section 5, then any surviving piece that began at or after `r1` has both record endpoints reduced by `D`
- a piece wholly before the interval stays on its record span
- a piece wholly after the interval keeps its source span and its record length, and both record endpoints move earlier by `D`

**Verified.** Lift, split, overwrite, and move are different commands, and each one already has its own undo kind.

- `ProjectSession::lift` reads one instance and deletes that one `timeline_instances` row. The record interval `[recordStart, recordEnd)` stays empty. Preceding rows, following rows, and other tracks keep their record spans. `UndoKind::Lift` restores that one row. Slice 1I validated this on a disposable project: lifting track 0 `[24, 48)` left track 0 `[60, 72)` at `[60, 72)`.
- `ProjectSession::split` cuts one instance at one interior record frame and removes nothing. Later rows stay. `planSplit` rejects a cut at the start or the end.
- `ProjectSession::overwrite` replaces one existing track over a mark-derived span of equal source and record duration. Later material beyond that span stays. The replacement must not extend past that track's current maximum `record_end`.
- `ProjectStore::moveInstance` changes one row's `record_start` and `record_end` by the same record length, leaves the source span, and refuses a same-track overlap. It is `UndoKind::MoveInstance`.

**Proposed.** A lift of one row followed by a move of another row is two transactions and two undo entries. That sequence leaves a hole when lift is the only delete, and move refuses to land on a row that still occupies the destination. Extract's close, its treatment of a crossing row, and its single undo entry are the command. The arguments are a sequence id, an explicit list of track ids, and the two frames `r0` and `r1`. There is no mark argument. There is no instance id that expands to every track. The decision block records the owner-approved source of those two frames. The command, when later authorised, still receives the two frames. Implementation has not started.

A failure during classification, during the same-rate gate, or during the write adds no undo entry. The database is unchanged when the failure happens before `mutate`, and a failure inside `mutate` rolls back. **Verified.** `mutate` begins a transaction, rolls back when the body fails, and on success updates `project_meta.modified_utc`. **Proposed.** Extract uses one such transaction for every closed track. The `sequences` row is not rewritten. Its name and rate stay.

## 3. Verified model

### Schema and duration

**Verified.** `kPrototypeSchemaVersion` is 2 and `kPrototypeVersion` is `slice-0a` (`include/aether/core/project/project_info.hpp`). The eleven tables are `schema_info`, `project_meta`, `bins`, `editorial_objects`, `bin_membership`, `media_assets`, `media_representations`, `master_clips`, `sequences`, `tracks`, and `timeline_instances`. Slice 1G, 1H, and 1I each recorded `schema_info` as `(1, 2)` and the same eleven tables.

`sequences` is `sequence_id`, `project_id`, `name`, `rate_numerator`, `rate_denominator`, `created_utc`, `modified_utc`. `SequenceRecord` has those fields and no duration. There is no mark table. `timeline_instances` stores `source_start`, `source_end`, `record_start`, and `record_end` as separate integers. Each span must be half-open. The checks do not require the two lengths to be equal. There is no speed column, no link column, and no filler table.

`tracks` is `track_id`, `sequence_id`, and `ordinal`, with `UNIQUE (sequence_id, ordinal)`. There is no track type, lock, target, patch, monitor, or sync-lock column. `docs/research/professional_nle/05_AETHER_ARCHITECTURE_GAP_ANALYSIS.md` records the same absence. That file's operation table was written on 22 November 2026, before slices 1G, 1H, and 1I. Overwrite, split, and lift now exist in the code. Extract does not: no extract function is declared.

`docs/EDITORIAL_OPERATIONS.md` uses a reel of duration 240, gap items, links, and sync locks. Its header says that document authorises no implementation. Those objects are not rows in schema version 2.

### Empty regions

**Verified.** An empty record range on a track is the absence of an instance. `placeInstance` inserts a row into that absence. Lift deletes one row and leaves the absence. `record_end > record_start` rejects a zero-length row. Overlap on one track is refused with "record ranges on one track must not overlap; transitions are not implemented." Abutting spans do not overlap: `recordRangesOverlap` is `startA < endB && startB < endA`.

### Selection, marks, and undo

**Verified.** `TimelineWorkspace::instances_` is `QAbstractItemView::SingleSelection`. `selectedInstanceId()` is the current list item's instance id, or empty. A canvas click selects that same id. There is no marquee and no second selected id. `sequences_` is also single selection. The overwrite control is a combo of one destination track for `overwrite` only.

`MarkPoints` is two optional frame points in one domain. The header says it is not a half-open edit range. The points live in `sourceMarks_` and `recordMarks_` on the session. `exclusiveEndFromInclusiveMark` derives `[markIn, markOut + 1)` for a caller that asks. `overwrite` is the caller that asks. `lift` does not read the maps. Close and open call `resetSessionState`, which clears both mark maps and both undo stacks.

Undo is session-only. ADR-009 for the foundation prototype is that session limit. A committed lift, split, or overwrite remains after close and reopen. Undo after reopen does not reverse it. `autosave` does not touch the undo stack. `editToward` deletes an id present only in the current set, updates an id present in both, and inserts an id present only in the target. Split and overwrite store `beforeInstances` and `afterInstances` and undo through that function. A minted id is reused on redo. It is not minted again.

### Source mapping already in the code

**Verified.** `planSplit` cuts only when the source duration equals the record duration and `sameReducedRate` says the source rate and the sequence rate match. The cut is `sourceSplit = sourceStart + (frame - recordStart)`. The message on refusal is "source span is not one frame per record frame". A split at either endpoint is rejected. The left piece keeps the instance id because its record start is unchanged. The right piece's id is cleared and `makeStableId()` mints it.

`placeInstance` calls `identityRecordEnd` and refuses a different rate with "source rate and sequence rate differ; no duration mapping is authorised". It also refuses a record end that is not the same-rate source length. `insertInstance` and `trimInstance` do not call `identityRecordEnd`. The Slice 1H proposal records a stored row with source `[0, 25)` at 25/1 and record `[0, 24)` on a 24000/1001 sequence, and a trim that stored source `[0, 10)` against record `[0, 20)` at 24/1. Those rows are reachable. `pictureSourceOffset` returns one source frame for one sequence frame. The 1H inspection recorded it returning 13 where `s0 + (f - r0)` was 12 on that mixed-rate row. It is not a duration mapping.

`retreatTail` and `advanceHead` move a source endpoint by a record delta and succeed when the leftover source span stays half-open. Overwrite uses them after it has required equal durations on the replacement. They are not the split gate. Extract does not adopt them for a crossing row.

## 4. How a closed track meets the interval

**Proposed.** Classification uses the existing half-open overlap test. For an instance `[a, b)` and an interval `[r0, r1)` the classes are a partition.

| Class | Condition | Result on a closed track |
| --- | --- | --- |
| Wholly before | `b <= r0` | The row stays. Record span and source span stay. This includes a row that touches `r0` from the left. |
| Wholly after | `a >= r1` | Source span and record length stay. Both record endpoints decrease by `D`. This includes a row that touches `r1` from the right. |
| Wholly inside | `a >= r0` and `b <= r1` | The row is deleted. An exact cover `[r0, r1)` is this class. |
| Spanning | `a < r0` and `b > r1` | Two positive pieces. The left keeps the original id. The right is minted. |
| Tail crossing | `a < r0` and `r0 < b <= r1` | One piece, ending at `r0`. It keeps the original id. A cut at `b == r1` does not mint an empty right piece. |
| Head crossing | `r0 <= a < r1` and `b > r1` | One piece. The original start was inside the interval, or on `r0`. Section 5 states which id it keeps. A cut at `a == r0` does not mint an empty left piece. |

A row that shares no frame with `[r0, r1)` is wholly before or wholly after. `recordRangesOverlap` is false for both, including the two touches.

**Proposed examples.** Sequence rate 24/1. Each source span below is one-to-one, so section 5 authorises the cut. `D = 10` for `[20, 30)`.

| Situation | Before | After |
| --- | --- | --- |
| Wholly before | record `[0, 10)`, source `[100, 110)` | unchanged |
| Touches `r0` | record `[0, 20)`, source `[100, 120)` | unchanged |
| Wholly inside | record `[22, 28)`, source `[100, 106)` | deleted |
| Exact cover | record `[20, 30)`, source `[100, 110)` | deleted |
| Tail crossing | record `[10, 26)`, source `[100, 116)` | record `[10, 20)`, source `[100, 110)`, same id |
| Tail ends at `r1` | record `[10, 30)`, source `[100, 120)` | record `[10, 20)`, source `[100, 110)`, same id |
| Head starts at `r0` | record `[20, 40)`, source `[100, 120)` | record `[20, 30)`, source `[110, 120)`, same id |
| Head starts inside | record `[24, 40)`, source `[100, 116)` | record `[20, 30)`, source `[106, 116)`, new id; original id deleted |
| Spanning | record `[10, 40)`, source `[100, 130)` | left `[10, 20)` / `[100, 110)` keeps id; right `[20, 30)` / `[120, 130)` is minted |
| Wholly after | record `[40, 50)`, source `[100, 110)` | record `[30, 40)`, source `[100, 110)` |
| Touches `r1` | record `[30, 40)`, source `[100, 110)` | record `[20, 30)`, source `[100, 110)` |

The head-inside source cut is `100 + (30 - 24) = 106`. The spanning cuts are `100 + (20 - 10) = 110` and `100 + (30 - 10) = 120`. Source `[110, 120)` sat on the removed record `[20, 30)` and is on neither piece. The wholly-after row and the row that touches `r1` are uncut shifts. Each keeps its instance id.

**Gap.** **Decided.** An interval that intersects no instance on the named tracks is a legal extract. Instances `[0, 10)` and `[40, 50)` with interval `[20, 30)` share no frames with the interval and share no frames with each other. The first stays. The second moves to `[30, 40)` and keeps its id. The empty frames inside `[20, 30)` are not a row before the command and are not a row after it. The empty frames that were between `30` and `40` move earlier with the later instance. If the same interval intersects nothing on the named tracks, the command still succeeds: named-track rows at or after `r1` move by `D`, and unlisted tracks stay.

**Adjacent.** **Proposed.** Instances `[0, 20)` and `[30, 40)` against `[20, 30)` become `[0, 20)` and `[20, 30)`. They abut. Abutting is legal. Instances `[0, 10)`, `[10, 20)`, and `[20, 30)` against interval `[10, 20)` become `[0, 10)` and `[10, 20)`: the middle row is deleted and the last row moves by `D = 10`.

**Domain overlap.** **Verified.** Two instances on one track that satisfy `recordRangesOverlap` are already refused by `validateInstancePlacement` on insert, move, trim, and `commitInstanceEdits`. **Proposed.** Extract does not classify a track that already contains such a pair. The command refuses and writes nothing. A plan that would leave two positive pieces overlapping also refuses. A legal classification does not produce that pair: every retained end that stays before the interval is at most `r0`, and every moved start is at least `r0`.

`commitInstanceEdits` accepts one `trackId`. An update whose track id differs is rejected. Two calls are two transactions. **Proposed.** A command that closes more than one track validates every closed track first and then writes all of them in one `mutate`. The tables stay the eleven tables above.

## 5. Source mapping and the crossing blocker

**Proposed.** A cut uses `sourceSplit = sourceStart + (frame - recordStart)` only when the instance already passes the gate inside `planSplit`: the source rate and the sequence rate match after reduction, and `sourceEnd - sourceStart == recordEnd - recordStart`. The frames used as `frame` are `r0` for a left edge and `r1` for a right edge. Both resulting source spans must be half-open. No source frame is duplicated and none of the removed record frames keep a source frame on a survivor.

That formula is the split formula. It is exact when one source frame corresponds to one record frame. On the spanning example it removes source `[110, 120)` with record `[20, 30)` and leaves the two survivors one-to-one.

**Blocker.** **Proposed.** If a crossing instance fails the gate, the whole extract writes nothing and adds no undo entry. This covers a tail crossing, a head crossing, and a span.

- Unequal, same rate. Source `[0, 10)`, record `[0, 20)`, interval `[5, 12)`. `s0 + (f - r0)` at record frame 12 is 12, which is past `sourceEnd`. The command refuses. It does not scale the offset and it does not call `retreatTail`.
- Mixed rate. Sequence 24/1, source 25/1, source `[0, 25)`, record `[0, 24)`, interval `[12, 16)`. The rates differ after reduction. The command refuses. `pictureSourceOffset` is not called. `identityRecordEnd` already says a different rate has no authorised duration mapping. This proposal adds none.

**Decided.** An uncut row does not need a source frame chosen. An unequal or mixed-rate instance wholly inside the interval is deleted with its row. One wholly after the interval is shifted by `-D` and keeps its source span and its instance id. One wholly before the interval stays, source span included. A crossing unequal or mixed-rate instance remains the blocker above and writes nothing. No conversion is added for it.

## 6. Three sync policies

The owner chose C on 2026-11-23. A and B stay in this section as the alternatives that were not chosen. How `r0` and `r1` are supplied was approved on 2026-11-23 as Dedicated Timeline Range Selection. Numeric entry, marks, and selected instances are compared in the decision block. The owner has not rejected those other methods.

**Verified.** Nothing in the session stores a set of tracks that move together. Patching, linked selection, and sync groups are not in the schema and are not session fields. The research states two external rules and does not pick one. Media Composer, as read in `docs/research/professional_nle/01_PROFESSIONAL_WORKFLOW_BENCHMARK.md`, says that when sync locks are on, extract takes all material on all tracks. The track-selector chapter was not read, so an unselected track's interaction with that sentence is only that sentence. Lightworks delete, as read there, closes only by the black every track can give, and closes nothing when a track has no black. `docs/research/professional_nle/03_AETHER_PROFESSIONAL_EDITORIAL_MODEL.md` and `06_PROFESSIONAL_NLE_DEVELOPMENT_RECOMMENDATION.md` say those are different and that Aether must not pick one silently. This proposal does not adopt either sentence as the command. The Lightworks shared-black limit is not option A, B, or C.

A closed track follows section 4. An open track is unchanged: every instance keeps its id, its source span, and its record span. Empty tracks have no instance rows. Closing or opening an empty track writes no instance row and creates no filler.

The sync offset below is a record-frame count. Take a sequence frame `F` that held material on two tracks before the command.

- `F < r0`: both tracks still have that frame at `F`. Offset 0.
- `r0 <= F < r1`: a closed track no longer has frame `F`. An open track still has it. The extracted material remains only on the open track.
- `F >= r1`: a closed track holds that material at `F - D`. An open track holds it at `F`. The open track is `D` frames later than the closed track.

### A. Selected track only

Not chosen. The analysis below is the record of the alternative.

**Proposed.** The editor names one track id that already belongs to the sequence. That track is closed. Every other track of the sequence stays, including empty tracks and tracks that hold instances inside `[r0, r1)`.

The current selection can display the track id of one instance. That display is not the naming step. The overwrite combo is not read. Naming an empty track, or an interval that intersects no instance on the named tracks, changes no row on that empty track and is a legal extract. Later rows on a named track still move by `D`.

| Question | Answer |
| --- | --- |
| Removed | On the named track: wholly inside rows are deleted; crossing rows lose the source and record frames that sat in `[r0, r1)`, subject to the section 5 blocker. |
| Tracks that move | The named track. Later instances on that track, and the surviving tail of a head crossing or a span, move earlier by `D`. |
| Stays | Every other track, at the same record positions. Wholly-before rows on the named track. Marks. The sequence name and rate. |
| Undo restores | The named track's deleted ids, previous endpoints, and minted ids, in one undo entry. Other tracks have nothing to restore. |
| Editor must confirm | The one track id, `r0`, `r1`, and `D`. The confirmation states that other tracks stay, that material at `F >= r1` on those tracks remains at `F`, and that material which occupied `[r0, r1)` on those tracks remains. |

**Sync offset.** `D` frames for every shared moment at `F >= r1`. The interval's own material remains on the open tracks, so the break is both the missing `D` frames on the closed track and the later offset of everything that followed.

Worked case. Tracks V and A each hold `[40, 50)` and `[20, 30)`. The editor names V. Interval `[20, 30)`. After the command, V's later clip is `[30, 40)` and V's `[20, 30)` is gone. A's `[20, 30)` and `[40, 50)` are still there. The old frame 40 is frame 30 on V and frame 40 on A.

### B. Every track

Not chosen. The analysis below is the record of the alternative.

**Proposed.** Every track returned by `listTracks` for that sequence is closed. The editor's instance selection does not limit the set. A track the editor did not touch is still closed.

| Question | Answer |
| --- | --- |
| Removed | On every track, including tracks with no selected instance: a wholly inside instance is deleted; a crossing instance is cut. A clip that starts inside `[r0, r1)` loses its head or the whole row. A clip that ends inside `[r0, r1)` loses its tail or the whole row. A clip that spans the interval loses the middle. |
| Tracks that move | Every track that has an instance with `record_start >= r1`, and every surviving piece whose record start moves to `r0`. |
| Stays | Wholly-before instances on every track. Empty tracks stay empty: no row is deleted, no row is moved, and no filler is inserted. Marks. The sequence name and rate. Track ids and ordinals. |
| Undo restores | Every deleted id, every previous endpoint, and every minted id, on every track, in one undo entry. |
| Editor must confirm | The full track list, by id and ordinal, before the write. The confirmation names each instance that will be deleted or cut, including instances on tracks the editor did not select, and it names each empty track as a track with no row to change. |

**Sync offset.** Between tracks of this sequence, the shift is the same `D`, so moments that were at `F >= r1` stay aligned with each other at `F - D`. Moments wholly before `r0` stay aligned at `F`. The interval is removed on every track, so one track does not keep picture or sound that another track lost. An empty track has no instance to align. It does not gain black filler to represent the close.

What B destroys on a track the editor did not touch is real stored material: any instance that intersects `[r0, r1)`, wholly or partly. An empty track has nothing to destroy. A later clip on an untouched track still moves earlier by `D`.

### C. Explicit tracks named for the operation

**Owner decision, 2026-11-23.** Extract, when later authorised, closes only the tracks the editor names for that operation. The program does not fill the list from the selected instance or from every track. Unlisted tracks stay. No sync group, link, or patch is added.

**Proposed.** The command receives the list of track ids the editor names for that operation. Each id must already belong to the sequence. The list is unique and non-empty. A duplicate, an empty list, or an id from another sequence refuses and writes nothing. Tracks in the list are closed. Tracks absent from the list stay. The offset between a listed track and an unlisted track is the offset described under A.

The selected instance does not fill the list. Selecting one instance, or selecting none, leaves the list empty until the editor adds ids. The overwrite combo is not the list. A list of one track has the same effect on other tracks as A. A list that names every track has the same effect as B. The program builds neither list.

**Verified.** A sync group is not a stored object. There is no group id, no membership table, and no session flag. Linked selection is not implemented. Patching is not implemented. This decision does not add them. A persisted sync group would be a later schema decision. It is not part of this choice.

| Question | Answer |
| --- | --- |
| Removed | Section 4, applied to each listed track. Unlisted tracks lose nothing. |
| Tracks that move | The listed tracks, and only those. |
| Stays | Unlisted tracks. Wholly-before rows on listed tracks. Empty listed tracks stay empty. Marks. Sequence name and rate. |
| Undo restores | The before and after rows of every listed track, in one undo entry. Unlisted tracks are absent from that entry. |
| Editor must confirm | The exact id list, `r0`, `r1`, and `D`. The confirmation states which tracks close and which stay, and that no sync group, link, or patch was read. |

## 7. Comparison

| | A. One named track | B. Every track | C. Explicit list |
| --- | --- | --- | --- |
| Removed | Intersecting material on that track | Intersecting material on every track, including tracks with no selection | Intersecting material on the listed tracks |
| Tracks that move | The named track | Every track of the sequence | The listed tracks |
| Stays | All other tracks, in place | Wholly-before rows; empty tracks stay empty | Unlisted tracks, in place |
| Empty track | Unchanged, because it is not the named track | Unchanged as rows; it is still part of the confirmed set | Unchanged as rows; listed or unlisted only changes the confirmation |
| Clip starting or ending inside the interval on a track the editor did not touch | Stays | Cut or deleted | Stays when its track is unlisted; cut or deleted when its track is listed |
| Sync offset after `r1` | `D` frames against every open track | 0 between tracks of this sequence | `D` frames between a listed track and an unlisted track |
| Undo | One entry, one track | One entry, every track | One entry, the listed tracks |
| Editor confirms | One track, and that the others stay | Every track, including destruction the selection did not name | The exact list, which the selection does not fill |

### Owner decision

**Owner decision, 2026-11-23.** The owner chose C. Extract, when later authorised, closes only the tracks the editor names for that operation. The program does not fill the list from the selected instance or from every track. Unlisted tracks stay. No sync group, link, or patch is added.

This decision covers track scope. The later choices in section 1 cover an empty interval, an uncut unequal or mixed-rate row, the visible end, and the id of an uncut shift. How `r0` and `r1` are supplied was approved on 2026-11-23 in the decision block. That approval is not one of these choices. None of these choices authorises implementation.

The comparison records A and B as the alternatives that were not chosen. Selection names one instance. A track row has an id, a sequence id, and an ordinal. The program does not turn that instance into the track list, and it does not turn every track into the track list.

## 8. Sequence duration

**Verified.** Duration is not stored. `sequences` has no duration column. `SequenceRecord` has none. Lift does not write the sequence row. The furthest stored record end changes only when the deleted row was the unique instance that reached it. The Slice 1I live project still had a row ending at 72 after a lift of `[24, 48)`, so the furthest stored end stayed 72.

The canvas floor is separate. `TimelineCanvas::relayout` starts its paint end at 24, then raises it to the maximum block `record_end`, and can raise it again to `mark + 1` for a record mark that paints. That 24 is a widget minimum. A record mark can widen the widget. Neither value is a sequence duration, and neither is the extract interval.

Overwrite's "current maximum record end" is the maximum `record_end` of the one destination track. It is a guard on that command. It is not a sequence duration.

**Decided.** The visible end is the maximum stored `record_end`. A successful extract is described as shortening that end by `D`. No duration column is added. Schema stays 2. The command, when later authorised, writes no duration.

That description is not a universal success rule. `candidateEnd`, `oldVisibleEnd - D`, is the overflow check for a track the editor did not name. An unselected instance that ends exactly at `candidateEnd` is not overflow. The stored visible end stays the furthest `record_end` after the write. A participating instance with `record_end <= r0` stays unchanged, including its id and source span, even when it remains the furthest endpoint. The command does not move or trim that instance to force `oldVisibleEnd - D`.

The paint floor of 24 can remain after every instance is gone. That floor is a widget minimum. It is not the visible end. A record mark can widen the widget. That mark is not the interval, and it is not `r0` or `r1`.

## 9. Identity, failure, and undo

**Proposed.** Identity follows the decisions split and lift already made.

- A surviving piece whose record start is still the instance's original record start keeps the original id. The tail-crossing survivor and the left piece of a span are this case. The head-crossing survivor that started at `r0` is also this case, because after the close its record start is still `r0`.
- A surviving piece whose record start is a different frame is minted. The right piece of a span, and the head-crossing survivor whose original start was strictly inside `(r0, r1)`, are this case. The original row is deleted when no surviving piece keeps it. Undo restores that original id and removes the minted id.
- A wholly inside row is a delete. Undo restores the original id, track, media id, master id, source rate, and both spans. **Verified.** Lift's restore goes through `reinsertInstance`, which keeps a non-empty id and stamps `created_utc` and `modified_utc` again. **Proposed.** A full removal uses that restore. A kept id uses the update path, which changes the endpoints and `modified_utc` and leaves `created_utc`.
- Redo applies the stored after-rows, including minted ids. It does not mint a second time.

**Decided.** An uncut instance shifted earlier by `D` keeps its instance id. It is not a new piece and it is not a delete. Only `record_start` and `record_end` change, each by `-D`. The source span, source rate, track, media id, and master id stay. This includes an unequal or mixed-rate row that the interval does not cut.

**Proposed.** The undo entry is one new kind, distinct from `Lift`, `RemoveInstance`, `MoveInstance`, `Split`, and `Overwrite`. It stores the before-rows and after-rows of every closed track. Undo and redo each run as one transaction. A failed undo leaves the stack in place, which is what `ProjectSession::undo` does today when `apply` fails. Close and reopen clear the stacks and keep the committed rows. Marks are not read, written, cleared, or stored in the entry.

A crossing blocker, an illegal overlap, an unknown track, an empty track list, an interval that is not a positive half-open range, or a failed write leaves every track as it was. An interval that intersects no instance on the named tracks is not that failure.

## 10. UI

**Proposed.** This section specifies a future control. It adds none. There is no mock-up. The existing Lift button, the split frame field, and the overwrite track combo stay as they are.

The control collects the explicit track-id list the editor names. How `r0` and `r1` are supplied was approved on 2026-11-23 as Dedicated Timeline Range Selection. This section does not add the mode. Marks stay excluded. One selected instance is not sufficient. A single selected instance may be shown, including its track id, as a candidate the editor can add to the track list. The control does not call the session with the selected instance id alone, and it does not read the mark readout as the interval.

The window would call `ProjectSession` only. It would not run SQL and it would not call FFmpeg. No shortcut is specified here.

## 11. What this proposal does not change

Schema version stays 2. The eleven tables stay. `kPrototypeVersion` stays `slice-0a`. No duration column is added. No mark table, sync-group table, link column, patch table, lock column, or target column is added. ADR-006 and ADR-007 stay **PROPOSED**. ADR-008 stays **OPEN — PENDING LEGAL COUNSEL**. ADR-002, ADR-003, and ADR-009 stay as accepted. Overwrite, split, lift, place, move, trim, and `removeInstance` stay. Marks stay off `project.db`. Research notes, FFmpeg, `/opt/aether-ffmpeg-9.1`, RPATH, licensing, ShellCut, PlaceCut, MarksCut, ProbeCut, OverwriteCut, SplitCut, LiftCut, and earlier screenshots stay untouched. `docs/DEVELOPMENT_ROADMAP.md` stays empty. No application source, test, build file, roadmap, ADR, or research doc is edited by this proposal.

## 12. Exclusions

Insert, splice, ripple trim, roll, slip, slide, multi-track lift, a clipboard, filler, speed, mixed-rate conversion, playback, persistent marks, and a new database are not part of this command. The Lightworks shared-black close and the Media Composer sync-lock sentence are not adopted. Implementation of extract is not started.

SLICE 1J DESIGN PROPOSAL COMPLETE — IMPLEMENTATION NOT STARTED — OWNER APPROVAL REQUIRED.

SLICE 1J INTERVAL-SELECTION DECISION OWNER APPROVED — EXTRACT IMPLEMENTATION NOT STARTED.

## Implementation note

The decisions above were not rewritten. The command is now in `ProjectSession::extract`. What was run, including the XCB session on ExtractCut, is in `docs/validation/SLICE_1J_EXTRACT_EDITING.md`. Track participation is a click on the track name in the timeline header. The separate checklist was removed. The command still receives the stable track ids the editor selected. ADR-006 and ADR-008 were not marked accepted. Schema version stays 2. `kPrototypeVersion` stays `slice-0a`. No new table and no duration column were added.

A later correction keeps that same command. `candidateEnd` is the unselected overflow check. The visible end is the furthest stored `record_end`. Exact reduction by `D` is not required of every success. The correction build is `build-slice1j-correction`. ExtractCut and its screenshots were left in place.
