# Aether professional editorial model

Status: research proposal. Nothing in this file is an accepted ADR. Inspection date: Sunday 22 November 2026, Asia/Calcutta.

Current code is the boundary. `placeInstance` (`src/app/project_session.cpp`) calls `identityRecordEnd` (`src/core/sequence/sequence.cpp`) and refuses a source rate that is not the sequence rate. The menu label "Insert into Timeline" (`src/ui/main_window.cpp`) is that placement. It is not splice-in. `removeInstance` deletes one row and does not shift later rows. `trimInstance` still accepts a source span and a record span whose lengths differ. `validateInstancePlacement` does not compare those lengths. Session undo dies when the window closes. Committed rows remain.

## Shared vocabulary

A source span and a record span are both half-open frame ranges: `[start, end)`. Duration in frames is `end - start`. A mark is a frame index in one domain. An IN mark is a start. An OUT mark, when the editor shows an inclusive last frame, is the frame after that last frame once it is stored. Displayed inclusive OUT remains deferred (ADR-002). No float second is a position.

Tracks today are `track_id`, `sequence_id`, and `ordinal`. There is no track type. Picture and sound are not distinguished by a column. A proposal must not invent that column here.

## Three-point arrangements

The missing fourth mark is computed. The record length must equal the source length. That is the same-rate rule already in `identityRecordEnd`: `recordEnd = recordStart + (sourceEnd - sourceStart)`, and only when both rates are the same accepted reduced pair.

| Marks present | Derived mark | At 24/1, 25/1, 24000/1001, or 30000/1001, same rate both sides |
| --- | --- | --- |
| Source IN, source OUT, record IN | Record OUT | Source `[100, 148)`, record IN `1000` → record `[1000, 1048)`. |
| Source IN, record IN, record OUT | Source OUT | Record `[1000, 1048)`, source IN `100` → source `[100, 148)`. |
| Source OUT, record IN, record OUT | Source IN | Record `[1000, 1048)`, source OUT `148` → source `[100, 148)`. |
| Source IN, source OUT, record OUT | Record IN | Source `[100, 148)`, record OUT `1048` → record `[1000, 1048)`. |

A derived source IN below zero is refused. A source span that does not sit inside the known media range is refused once a duration is known. Import today does not probe, so "known media range" is not yet a fact the application can check. Two marks on one side and none on the other are not an edit. All four marks set, with lengths that differ, are a speed change and are refused. `pictureSourceOffset` is nearest-timestamp picture selection with the tie on the earlier frame (`src/core/timing/sample_boundary.cpp`). It is not a duration mapping and is not used to fill a missing mark.

## Operation contracts

Each row is a proposed contract. Undo and redo are session commands until a persistence decision says otherwise. A command that fails validation writes nothing and pushes nothing.

### Insert / splice

| Field | Contract |
| --- | --- |
| Intention | Place the source span at the record IN and move later material later. |
| Inputs | Master object, source rate, source span, record IN, destination track or tracks. |
| Preconditions | Same rate. Half-open source span with positive length. Record IN ≥ 0. Destination track exists and is not locked. |
| Source effect | None. The source span is a reference. |
| Record effect | New instance `[recordIn, recordIn + sourceLength)`. Every later instance on each affected track has its record span shifted by `sourceLength`. |
| Tracks | Only the destination tracks. Other tracks move only when a sync rule says so. That rule is not chosen in this document. |
| Duration | Sequence content end on an affected track grows by `sourceLength`. |
| Downstream | Later instances on affected tracks move. Instances on unaffected tracks stay. |
| Sync | Open. See the sync classification below. |
| Validation | Refuse overlap that the shift does not clear. Refuse a different rate. |
| Undo / redo | One command restores every shifted row and deletes the new row. |
| Persistence | The new row and the shifted rows are the database. The command itself is not. |
| Status | **NOT IMPLEMENTED.** `placeInstance` does not shift. |

### Overwrite

| Field | Contract |
| --- | --- |
| Intention | Cover the record span with the source span. Later material does not move. |
| Inputs | Source span and record span of equal length, or three marks that derive that equality. |
| Preconditions | Same rate. Equal lengths. Destination not locked. |
| Source effect | None. |
| Record effect | The covered record range is replaced. A partially covered instance is split into the uncovered remainder or remainders. Material past the sequence end extends that track only. |
| Tracks | Destination tracks only. |
| Duration | Unchanged, except where the span hangs past the current end. |
| Downstream | No shift. |
| Sync | No ripple, so sync locks do not insert filler. |
| Validation | Same-rate equal length. No write on refusal. |
| Undo / redo | Restore the previous rows of the destination tracks. |
| Persistence | The resulting rows. |
| Status | **NOT IMPLEMENTED.** Same-rate placement can land in an empty region. It is not this contract, because a collision is refused rather than split. |

### Replace

| Field | Contract |
| --- | --- |
| Intention | Keep an existing record span. Fill it from a source sync frame aligned to a record sync frame. |
| Inputs | Existing instance identity, source sync frame, record sync frame inside that instance. |
| Preconditions | Same rate. The derived source span lies inside the known source. The record span is unchanged. |
| Source effect | None. |
| Record effect | The instance's source span changes. Its record span does not. Sequence length does not. |
| Tracks | The instance's track. |
| Downstream | None. |
| Sync | None. |
| Validation | Derived source IN ≥ 0 and, once known, inside the media. |
| Undo / redo | Restore the previous source span. |
| Persistence | The updated source columns. |
| Status | **NOT IMPLEMENTED.** |

### Lift / remove

| Field | Contract |
| --- | --- |
| Intention | Take material out and leave the time empty. |
| Inputs | A record span, or an instance, on one or more tracks. |
| Preconditions | The span is inside the sequence. Tracks are not locked. |
| Record effect | Covered instances are deleted or split. The hole stays. Later instances do not move. |
| Duration | Unchanged. |
| Downstream | None. |
| Sync | No close-up. |
| Undo / redo | Restore the removed rows in place. |
| Persistence | The hole is the absence of rows, not a filler row. Aether has no filler object today. |
| Status | **PARTIALLY IMPLEMENTED.** `removeInstance` deletes one whole instance and leaves later instances where they are. It does not lift a marked sub-range, and it does not put the material on a clipboard. |

### Extract / delete

| Field | Contract |
| --- | --- |
| Intention | Take material out and close the time. |
| Inputs | A record span on one or more tracks. |
| Preconditions | Same as lift, plus an explicit sync decision for tracks that were not marked. |
| Record effect | Covered material goes. Later material on each closed track shifts earlier by the removed length. |
| Duration | Those tracks get shorter. |
| Downstream | Shift. |
| Sync | Avid's read sentence pulls every track when sync lock is on. Lightworks closes only by black every track can give. Those are different. Aether must not pick one silently. |
| Undo / redo | Restore rows and reverse the shift. |
| Status | **NOT IMPLEMENTED.** |

### Split

| Field | Contract |
| --- | --- |
| Intention | Cut an instance at a record frame. Nothing is removed. |
| Inputs | Instance, record frame strictly inside its record span. |
| Preconditions | The frame is inside `[recordStart, recordEnd)`. |
| Record effect | Two instances. The first ends at the frame. The second starts there. Source frames divide by the same count, because the rate is the same. |
| Duration | Unchanged. |
| Downstream | None. |
| Undo / redo | Join the two rows back into one. |
| Status | **NOT IMPLEMENTED.** |

## Trim matrix

Not implemented as roller modes. `trimInstance` exists and can change both spans. This matrix is the target, not the code.

| Operation | Sequence length | Source span | Record span | Later material |
| --- | --- | --- | --- | --- |
| Single-roller, one side | Changes | Changes on that side | Changes on that side | Ripple only under the sync decision |
| Dual-roller / move cut | Unchanged | One side gives what the other takes | The cut moves; the sum of the two record lengths stays | Does not ripple |
| Slip | Unchanged | Moves, length stays | Unchanged | Does not ripple |
| Slide | Unchanged | Unchanged on the middle clip | Middle record span moves; neighbours change length and keep the total | Does not ripple |

A slip or a slide that would need source frames outside the media is refused. Trim limit is that refusal.

## Where each control lives

| Control | Class | Why |
| --- | --- | --- |
| Track targeting for the next edit | Session | It chooses the next command's tracks. It is not a property of a committed instance. |
| Track patching | **REQUIRES ARCHITECTURAL DECISION** | There is no picture/sound component to patch. Do not add a patch table before that model exists. |
| Lock | Sequence data, once specified | A lock that disappears on reopen would not protect the cut. Not in the schema today. |
| Monitor / mute | Session or preference | It changes what a future player would emit. Playback is not implemented. It must not change instance rows. |
| Sync lock | Sequence data, once the sync decision is made | It changes how insert and extract treat other tracks. Until that decision, ripple stays unbuilt. |
| Bins | Proposed, not accepted | Tables exist (`bins`, `bin_membership`). A professional bin browser is not a validated workflow. |
| Keyboard map | Proposed, not accepted | No map is stored. Factory keys are not specified. |

## What stays refused

A source rate that differs from the sequence rate is refused until a conversion service exists. That service is not designed here. Examples that must keep failing:

- Sequence 24/1, source 25/1, source `[0, 25)`, record start `0`.
- Sequence 25/1, source 24/1, source `[0, 24)`, record start `0`.
- Sequence 24000/1001, source 30000/1001, any positive source span.
- Sequence 30000/1001, source 24000/1001, any positive source span.

Same pair on both sides is the identity case in the table above, including 24000/1001 and 30000/1001.

Audio sample counts at 48 kHz stay the published floor results: frames `[0, 8)` at 30000/1001 are 12812 samples; frames `[8, 16)` are 12813 samples. Those counts are not a picture duration. The MXF test that writes 12813 samples where the contract expects 12812 remains failed.
