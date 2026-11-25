# Aether professional editorial workflow specification

Status: research proposal. Nothing in this file accepts an ADR, changes a schema, or authorises implementation.

Accepted decisions are named as accepted. Everything else is proposed behaviour for a later owner decision. In particular, ADR-006 and ADR-007 stay **PROPOSED**. No source-to-record speed mapping is defined.

Inspection of the tree was on 2026-11-22. `placeInstance` and the placement dialog were already in the source. They are described as what the code does, not as this proposal.

## 1. Domains

Two clocks stay distinct, as ADR-002 requires.

- A source position is an `int64` frame in the source rate. The source rate is a reduced rational, and the only rates the current code accepts are 24/1, 25/1, 24000/1001, 30000/1001, and 60000/1001.
- A sequence position is an `int64` frame in that sequence's rate. One sequence has one rate.
- A stored span is half-open `[start, end)`. The displayed out-point, inclusive or exclusive, is still deferred by ADR-002. This proposal stores half-open spans and does not decide the label.
- A path is not an identity. A Media ID is not a Representation ID. A timeline instance cites a master-clip object id and stores the media id. It does not store a path. That is ADR-005, already implemented on `timeline_instances`.

`pictureSourceOffset` answers which source frame is nearest a sequence-frame offset, with an exact tie on the earlier frame. It is not a rule for how long a marked source span is on the sequence. `identityRecordEnd` is the only duration rule in the code: the rates must be the same reduced pair, and the record length equals the source length. A different pair fails with "source rate and sequence rate differ; no duration mapping is authorised."

## 2. Source and record — proposed model

The window already has two empty panels labelled SOURCE and RECORD, and a playhead label that says playback is not available. This proposal does not implement viewers.

Proposed session state, separate from `project.db` until an owner decides otherwise:

| State | Domain | Notes |
| --- | --- | --- |
| Loaded source | Master-clip object id | The bin selection is not yet a loaded source. Loading is explicit. |
| Source position | Source frames | Independent of the sequence playhead. |
| Source mark in, source mark out | Source frames, half-open, either may be unset | Marks do not change the master clip. |
| Record position | Sequence frames | One per open sequence in the session. |
| Record mark in, record mark out | Sequence frames, half-open, either may be unset | Marks are not timeline instances. |
| Focused viewer | Source or record | Keyboard marks apply to the focused side. |

Identity:

- A master clip is the bin object. Many timeline instances may cite it.
- A timeline instance has its own instance id, its own source span, and its own record span.
- Match Frame, proposed: from a selected instance and a sequence frame inside its record span, compute the source frame only when a mapping exists. Today a mapping exists only when the rates are equal, in which case `sourceFrame = sourceStart + (sequenceFrame - recordStart)`, and that frame must be inside `[sourceStart, sourceEnd)`. Mixed rates have no authorised mapping, so Match Frame fails closed. Find Bin, proposed: select the bin membership of that master clip. Neither command exists.

Playheads do not move each other. Closing the project drops session marks with the rest of the session undo stack. Committed instances remain. That matches current close behaviour and is not a new persistence design.

## 3. Three-point and four-point mathematics

Only the identity rule is legal. Both rates are one of the five accepted rates, the pairs are equal, and one source frame occupies one sequence frame. No speed, no pull-up, no Motion Adapter.

Let `Ss`, `Se` be the source span and `Rs`, `Re` the record span. Lengths are `Se - Ss` and `Re - Rs`. Both spans must be half-open and non-negative. A derived endpoint that would be negative or would overflow `int64` is refused.

### Equal rates — the four three-point cases

Sequence and source are both 24/1.

Start from source `[100, 148)` (48 frames) and record in `1000`.

| Given | Derived | Result |
| --- | --- | --- |
| `Ss=100`, `Se=148`, `Rs=1000` | `Re = Rs + (Se - Ss) = 1048` | Record `[1000, 1048)`. This is what `identityRecordEnd(24/1, 24/1, 100, 148, 1000)` returns. |
| `Ss=100`, `Rs=1000`, `Re=1048` | `Se = Ss + (Re - Rs) = 148` | Source `[100, 148)`. |
| `Se=148`, `Rs=1000`, `Re=1048` | `Ss = Se - (Re - Rs) = 100` | Source `[100, 148)`. Legal because `Ss >= 0` and `Ss < Se`. |
| `Ss=100`, `Se=148`, `Re=1048` | `Rs = Re - (Se - Ss) = 1000` | Record `[1000, 1048)`. Legal because `Rs >= 0`. |

A derived source in that would be negative is refused. Example: source out `10`, record `[0, 48)`. `Ss = 10 - 48 = -38`. Refuse. Do not clamp.

`placeInstance` does not derive a missing point. The caller must pass a record end that already equals `identityRecordEnd`. The proposal is that a later three-point command may derive one missing endpoint by the table above, and then call the same placement rule. That command does not exist.

### Four points

All four numbers are supplied.

- Lengths equal, rates equal: the fourth point is redundant. The edit is the same as the three-point result. Example: source `[100, 148)`, record `[1000, 1048)`.
- Lengths differ, rates equal: this would be a speed change. Example: source `[100, 148)` (48 frames) and record `[1000, 1024)` (24 frames). Refuse. Do not invent a 200 percent play speed.
- Either rate not accepted, or the pairs differ: refuse. Do not call `pictureSourceOffset` to invent a length.

### Mixed rates — refused examples

Sequence 24/1, source 25/1, source `[0, 25)`, record start `0`.

One wall-clock second is 25 source frames and 24 sequence frames. A wall-clock rule would write record `[0, 24)`. That rule is not authorised. `identityRecordEnd` already returns failure. A three-point command must return the same failure and must not write.

Sequence 30000/1001, source 24000/1001, source `[0, 48)`, record start `0`. Same refusal. The published audio sample counts (12812 and 12813) are properties of the floor boundary function. They are not a picture duration mapping.

### Invalid ranges and overlap

- `[10, 10)` and `[10, 9)` fail `isHalfOpenFrameSpan`.
- A negative endpoint fails.
- Same-track record overlap fails `validateInstancePlacement`. Abutting `[0, 10)` and `[10, 20)` does not overlap.
- Overwrite, proposed below, is the operation that is allowed to replace an occupied record range. Ordinary placement is not overwrite. Today the store rejects the overlap instead.

### What the products do, and what Aether will not copy blindly

Media Composer 2025.12 says the fourth mark is determined automatically, and its Motion Adapter plays mixed-rate clips at the project rate. Lightworks' basic-editing article writes two of the four cases (source in/out plus timeline in, and timeline in/out plus source in) and does not state a mixed-rate rule. Neither document is an integer mapping Aether may store. The table in this section is the identity rule already in `identityRecordEnd`, extended only to which endpoint is the unknown. It is still a proposal for the three-point command.

## 4. Operations

Current code, so these rows are not proposals:

- `placeInstance` requires the identity record end, inserts one row, and pushes one `InsertInstance` undo. An empty track id creates a track first and deletes that track if the insert fails. The name `InsertInstance` is the undo kind for adding a row. It does not ripple later instances.
- `insertInstance` stores the caller's four endpoints after `validateInstancePlacement`. It does not require equal lengths. It does not ripple.
- `moveInstance` keeps the record length and rejects overlap. It does not move neighbours.
- `trimInstance` rewrites the four endpoints of one row and rejects overlap. It does not move neighbours. It does not require the identity length. A trim can therefore store a source length different from the record length. `placeInstance` cannot. That split is existing behaviour, recorded in the Slice 1D proposal, and is not changed here.
- `removeInstance` deletes the row. Other rows keep their record ranges. A hole remains unless the deleted instance was the only material at the end. That is closer to lift than to extract. The function is not named lift in the code.

Proposed operations. Each is one undo unit and one database transaction (ADR-003). A refusal writes nothing and pushes no undo.

### Insert

1. Intention: put the marked source on the targeted tracks at the record in, and push later material later.
2. Inputs: source id, source span, record in, target tracks, sync set.
3. Ranges: record span length equals source span length under the identity rule. Mixed rates refuse.
4. Targets: only targeted tracks receive media. Aether has no track type, so "targeted" cannot yet mean picture versus sound. See section 6.
5. Downstream: every instance on a track in the sync set with `recordStart >= Rs` has both record endpoints increased by the insert length. Instances that straddle `Rs` are split first. Split does not exist yet.
6. Other tracks: a track in the sync set that receives nothing still gains an empty span of the same length (the professional "black pad"), so later material on that track moves too. A locked track does not move. Lock does not exist yet.
7. Sync: without a sync set, an insert on one track desynchronises the others. The proposal is that the default sync set is every track, and a lock removes a track from it. That default is not an accepted ADR.
8. Undo: one unit restores every moved endpoint and deletes the new instances and any splits.
9. Validation: identity length, half-open, no negative, targets exist, locks respected, overflow checks.
10. Persistence: the new rows and the shifted endpoints commit together.

This is not `placeInstance`. `placeInstance` refuses occupied frames instead of pushing them.

### Overwrite

1. Intention: replace the record span with the source span. Later material beyond `Re` stays.
2. Inputs: source span, record span, targets.
3. Ranges: lengths equal under the identity rule. The record span may be occupied.
4. Targets: only targeted tracks lose material.
5. Downstream beyond the span: unchanged.
6. Other tracks: unchanged. No black pad.
7. Sync: picture can be replaced while sound stays, which is a deliberate sync break if the editor targeted one side only. Linked selection is a separate proposal.
8. Undo: restore every deleted or shortened instance and remove the new ones.
9. Validation: the same identity and span checks. Overlap is expected and is not an error.
10. Persistence: one transaction.

### Replace

1. Intention: keep an existing instance's record span and fill it from a source sync frame.
2. Inputs: source position `P`, sequence position `Q` inside an existing instance, the instance's record span `[Rs, Re)`.
3. Ranges, rates equal only: source length is `Re - Rs`. Source in is `P - (Q - Rs)`. Source out is source in plus that length. If the source in would be negative or the source out would pass the known source duration, refuse. Source duration is unknown today because media is not probed, so Replace cannot be validated against a file until a duration exists. Do not invent a duration from the file size.
4. Targets: the instance's track, plus linked instances if link is on.
5. Downstream: unchanged. This is why Replace is not Insert.
6. Other tracks: only linked instances.
7. Sync: the sync frame is the point that stays put. That is the Media Composer definition read in the 2025.12 guide. It is proposed, not implemented.
8. Undo: the previous source span of those instances.
9. Validation: `Q` inside the record span, derived source span half-open and non-negative, rates equal.
10. Persistence: update source endpoints only. Record endpoints stay.

### Lift (Remove)

1. Intention: take the marked record range off the targeted tracks and leave the hole.
2. Inputs: record `[Rs, Re)`, targets.
3. Ranges: instances wholly inside are deleted. Instances that straddle are trimmed to the outside pieces. A split is required for a straddle. Split is not implemented.
4. Targets: only those tracks.
5. Downstream: record positions unchanged.
6. Other tracks: unchanged.
7. Sync: a one-track lift desynchronises that track from the others. That is the point of lift.
8. Undo: restore the deleted and trimmed rows.
9. Validation: the range is half-open. An empty mark is not a lift of the whole sequence.
10. Persistence: deletes and trims in one transaction.

Current `removeInstance` deletes one whole instance and does not trim a partial overlap with a mark. It is not this operation.

### Extract (Delete)

1. Intention: lift, then close the hole by moving later material earlier by `Re - Rs`.
2. Inputs: the same mark, plus the sync set.
3. Ranges: as lift, then every later record endpoint on the sync set decreases by the length.
4. Targets: the tracks that lose media.
5. Downstream: moves earlier.
6. Other tracks: if they are in the sync set, they move even when they had no media in the mark. Lightworks' documented delete will only close by the black every track can give. That rule is a candidate. Media Composer's extract with sync locks pulls all tracks. These are different. Aether has not chosen. The proposal is to pick one in an owner decision before coding extract. This file does not pick silently. It does say extract must not be implemented as `removeInstance`.
7. Sync: the chosen close rule is the sync policy.
8. Undo: one unit, including every shifted endpoint.
9. Validation: underflow (a record start that would go negative) refuses the whole command.
10. Persistence: one transaction.

### Split

1. Intention: cut one instance into two at a sequence frame `F` with `recordStart < F < recordEnd`. Nothing is removed. The sequence does not get longer or shorter.
2. Inputs: instance id, `F`.
3. Ranges, rates equal: left record `[recordStart, F)`, right record `[F, recordEnd)`. Source splits by the same frame count: left source `[sourceStart, sourceStart + (F - recordStart))`, right source the rest. Mixed rates refuse, because the source frame at `F` has no authorised mapping.
4. Targets: the one instance, or the linked group if the editor split the group.
5. Downstream: unchanged.
6. Other tracks: unchanged unless linked.
7. Sync: a split of picture without sound is how an editor starts an L-cut. Link defaults belong in section 6.
8. Undo: delete the two rows and restore the one.
9. Validation: `F` strictly inside the record span. Rates equal.
10. Persistence: one transaction. Two new ids. The old id is not reused.

### Move

1. Intention: change record start, keep record length and source span.
2. Inputs: instance id, new record start.
3. Ranges: length preserved. This is what `moveInstance` does now.
4. Targets: the instance's track. A move onto another track is not implemented and is not proposed until tracks have a role or an explicit "any track" decision.
5. Downstream: unchanged. Current move does not ripple. A ripple move is Insert of the same material plus Extract of the old span, and it needs the extract decision above.
6. Other tracks: unchanged, unless a future linked move says otherwise.
7. Sync: a one-track move breaks sync with other tracks. That is current behaviour because nothing else moves.
8. Undo: the previous record start and end. Already implemented for the single-row move.
9. Validation: non-negative, no overflow, no same-track overlap. Already implemented.
10. Persistence: one update. Already implemented.

### Trim

See section 5. The current `trimInstance` is a raw four-endpoint write, not a roller.

## 5. Trim matrix — proposed

All rows assume equal rates until a mapping exists. Mixed rates refuse. A trim that needs media past a known source duration refuses. Source duration is not known for imported media today, so a lengthening trim cannot be checked against the file. The current code does not check it either. The proposal is to keep refusing negative and empty spans, and to add a source-duration check only when a probed duration exists. Do not invent one.

| Trim | Boundaries that change | Sequence length | Adjacent clip | Downstream | Sync | Validation | Undo |
| --- | --- | --- | --- | --- | --- | --- | --- |
| Single-roller, no ripple | One record end and the matching source end, same frame count | Changes if this instance is the end and no other track is longer | Unchanged. Overlap with the neighbour refuses | Unchanged | Can break | Half-open, no overlap, identity delta | Previous four endpoints of this row |
| Ripple trim | The same roller, plus every later record endpoint on the sync set, shifted by the delta | Changes by the delta | Unchanged except the roller | Moves | Holds for the sync set. A locked track stays | The no-ripple checks, plus no negative after the shift | This row and every shifted row |
| Roll (dual-roller) | Out of A and in of B by the same delta, opposite directions. Record length of A+B is constant | Unchanged | The neighbour's opposite end | Unchanged | Holds if both sides and their links roll together | Both spans stay half-open. No overlap is created because the shared frame moves | Both instances' endpoints |
| Slip | Source in and source out move by the same delta. Record span stays | Unchanged | Unchanged | Unchanged | Holds | New source span half-open. If duration is known, it must lie inside the source | The two source endpoints |
| Slide | This record span moves. The previous clip's record out and the next clip's record in absorb the delta. This source span stays | Unchanged | Both neighbours change one end | Unchanged beyond the next clip | Holds if the whole linked group slides | Neighbour spans stay half-open. No overlap | Three instances |
| Asymmetric | Two or more rollers with deltas that are not equal and opposite | May change | May change | May move | May break | Each delta still identity-mapped. The composition of the deltas must be explicit in the command, not a side effect | Every touched row |
| Play-loop review | None until the editor joins | Unchanged while previewing | Unchanged | Unchanged | Unchanged | The preview is session state | No undo entry until join. Join pushes one trim unit |

Keyboard trim amounts are deltas on the open rollers, in sequence frames. They are not a second timebase.

Current `trimInstance` can change all four endpoints at once, including to unequal lengths. The proposal is that the professional trims above never write an unequal length. The raw four-endpoint command can stay as a test and maintenance entry, or a later slice can stop accepting unequal lengths. That is an owner decision, because changing it would reject rows the current trim already allows. This research does not change the code.

## 6. Tracks, targets, patching, sync

What exists: `tracks` has `track_id`, `sequence_id`, `ordinal`, and timestamps. No kind, no lock, no mute, no target, no patch. The window says a video or audio kind is not stored. `orderTrack` changes ordinal and has undo.

Editorial role and presentation stay separate:

- Role, which this model does not have: picture, sound, or undecided. Adding a type is an architectural decision. This proposal does not add one.
- Presentation: order, height, colour. Ordinal is already the order. Height and colour are not data.
- Target: which destination tracks receive the next source/record edit. Session state, not a column, until persistence is required.
- Patch: which source component lands on which target. Meaningless until a source has components. Probing is not implemented, and ADR-006 is proposed. Patch is deferred with the media engine.
- Enable: a track that is disabled is skipped by playback. Playback does not exist. Do not store a mute that nothing reads, unless an owner asks for the column ahead of playback.
- Lock: a locked track refuses insert, overwrite, lift, extract, move, and trim. Proposed column or session flag. Not in the schema today.
- Sync lock: membership of the set that ripple, insert, and extract move together. Proposed. Not in the schema.
- Link: instances created by one edit share a link id and select together. Proposed. Not in the schema. Different from sync lock. Lightworks documents the same split (Clip Link versus Track Sync). Media Composer documents track selection versus sync lock. The proposal copies the distinction, not the graphics.

Reordering tracks changes `ordinal` only. It does not change instance ids or record ranges. That is what `orderTrack` does now.

## 7. Bins and the assistant

What exists: bins, membership, master clips, media assets, representations with roles `original`, `proxy`, `transcode`, `consolidated`, a recorded path, and a status. Import registers a reference. It does not probe. It does not copy the file. Exact path is refused as a second registration. Two different paths are two assets even when the bytes match, because sameness is not known. ADR-005 says a known-same file is one Media ID, and an unconfirmed match waits for a person. The code does not fingerprint. Offline is a live filesystem fact on the inspector, not a stored placement permission. Slice 1D's placement path refuses an offline asset. The store itself does not.

Essential for an assistant, in the order the professional documents imply, and not as a claim that Aether has them:

1. Stable clip identity separate from path. Implemented (ADR-005).
2. Bins and membership, including the same clip in more than one bin. `addMembership` and `referenceObject` exist. The window does not expose a second membership.
3. Master clip versus a marked subclip. Not implemented. A subclip would be a new editorial object citing the same Media ID, with its own source marks. It must not mint a Media ID.
4. Markers on a master clip and on a sequence frame. Not implemented. The Tags list says none are stored.
5. Columns, sort, and filter. Not implemented. The media list is a name and a presence flag.
6. Offline versus online as a status of a representation, not of the cut. Presence is computed. Relink is not implemented. ADR-007, proposed, says relink may change location rows and must not change instance ids, record ranges, or Media IDs.
7. Proxy as another representation of the same Media ID. The role enum exists. No proxy file is created. Switching representation must not change the cut. That is ADR-005.
8. Search across bins. Not implemented.

Consolidation, multicam groups, and logging databases are later. They are not required to place a same-rate clip.

## 8. Keyboard architecture — proposed

No Aether shortcut is assigned here. Media Composer and Lightworks defaults are evidence of which actions exist, not a map to copy. An editor-specific map is the point.

Proposed shape:

- A command id, stable, not a key: `mark-in`, `mark-out`, `clear-marks`, `step-back`, `step-forward`, `insert`, `overwrite`, `lift`, `extract`, `split`, `undo`, `redo`, `trim-nudge`, `target-track`.
- A context: `source`, `record`, `timeline`, `trim`, `bin`. The same key can mean different commands in different contexts. Example of the pattern, not a binding: a nudge key moves the playhead in `timeline` and moves the open roller in `trim`.
- A map the user can change, stored as user settings, not in `project.db`. Project portability should not depend on one editor's keys. Media Composer documents keyboard settings as user settings. That separation is the proposal.
- Commands call `ProjectSession`. Widgets do not write SQL. That is already the window's rule.
- A command that is not implemented is unbound and reported, not silently ignored.
- Undo and redo stay one session step. They already exist as buttons.

Context rules:

- `mark-in` and `mark-out` write the focused viewer's marks.
- `insert` and `overwrite` read source marks, record marks, and targets, and run section 4. They do nothing useful until those inputs exist.
- `trim-nudge` is legal only while a trim is open, and its amount is an integer frame delta.
- Track targeting has its own commands. It is not implied by which viewer is focused.

## 9. Integrity

Implemented and already validated in earlier slices, not re-run by this research: one `project.db`, one transaction per command, manifest rewrite after commit, a second open refused, session undo independent of the committed rows, undo discarded on close, committed rows kept on reopen.

Not implemented: persistent undo, a documented autosave interval, a power-loss proof, multiple editors in one project, interchange. `ProjectSession::autosave` exists and is not a window command. The roadmap already says the Slice 0A autosave test is not a power-loss proof. This proposal does not claim a new recovery design.

Determinism: the same `int64` inputs and the same rows produce the same rows. No float in the edit math. A failed command leaves the previous commit. That is the current store's pattern and the proposal for every operation in section 4.

## 10. Proposal versus decision

| Topic | Status |
| --- | --- |
| Integer frames, reduced rates, half-open spans, distinct domains | Accepted, ADR-002 |
| Identity duration when rates match, refusal when they differ | In the code as `identityRecordEnd`. Not a new ADR |
| `pictureSourceOffset` as a duration or speed rule | Not accepted. Not proposed |
| Three-point derivation of one missing endpoint | Proposed |
| Insert, overwrite, replace, lift, extract, split, ripple, roll, slip, slide | Proposed. Not in the code, except the non-ripple place, move, raw trim, and whole-row remove |
| Extract's cross-track close rule | Not chosen |
| Track type, patch, lock, sync, link | Not chosen. Type is not invented here |
| Subclips, markers, keyboard map | Proposed |
| Playback, probe, relink, interchange | Not in this proposal. ADR-006 and ADR-007 stay proposed |
