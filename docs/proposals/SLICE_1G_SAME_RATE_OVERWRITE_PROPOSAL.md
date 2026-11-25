# Slice 1G same-rate overwrite proposal

Status: owner decisions recorded, then implemented. This document does not accept an ADR, does not change schema version 2, does not add a table, and does not change `kPrototypeVersion`. ADR-006 and ADR-007 stay **PROPOSED**. ADR-008 stays **OPEN — PENDING LEGAL COUNSEL**. Accepted ADRs were not edited.

Inspection date: 2026-11-22. Tree: `/home/editry/development/sise-editor`. `git status` failed: `fatal: not a git repository (or any parent up to mount point /)`, exit 128. No repository was created.

The owner decided every gate below on 2026-11-22. Those choices are closed. The implementation follows them and does not reopen them.

Displayed Mark Out is the last included frame. A mark pair becomes `[In, Out + 1)`. Marks stay session points, off `project.db`, and are not stored as ranges.

Source extent: until an editorial frame count exists, any non-negative half-open source span is allowed, as placement allows it now. A probe timestamp is not converted into frames.

Record extent: a record end past the current maximum record end on the destination track is refused. Schema stays version 2. No duration column is added. Later material is not moved.

Equal duration: the derived source duration and the derived record duration must be equal in integer frames. Unequal lengths are refused. There is no stretch.

Intersection: leftover pieces are kept. Their source frames stay on the same sequence frames. A removed head of `D` frames advances `source_start` by `D`. A removed tail retreats `source_end` by `D`. A piece wholly inside is deleted. A piece past both ends becomes two pieces. One new instance is the replacement. One transaction. One undo entry.

Empty intersection: the new instance is written and every existing row stays.

Destination track: the editor chooses one existing track. An empty choice fails. Overwrite does not create a track. It is its own command, not `placeInstance`.

## What was read

Accepted time and storage authority:

- `architecture/decisions/ADR-002-rational-time.md`
- `architecture/decisions/ADR-003-project-database.md`
- `include/aether/core/project/project_info.hpp` (`kPrototypeSchemaVersion` is 2, `kPrototypeVersion` is `slice-0a`)

Slice 1F, which is the marks implementation:

- `docs/proposals/SLICE_1F_SOURCE_RECORD_MARKS_PROPOSAL.md`
- `docs/validation/SLICE_1F_SOURCE_RECORD_MARKS.md`
- `src/app/project_session.cpp` (`storePoint`, `placeInstance`, `moveInstance`, `trimInstance`)
- `src/ui/main_window.cpp` (`formatSourceMarks`)
- `src/ui/timeline_workspace.cpp` (`formatRecordMarks`)
- `src/ui/placement_dialog.cpp`
- `src/core/sequence/sequence.cpp` (`isHalfOpenFrameSpan`, `validateInstancePlacement`, `identityRecordEnd`)
- `src/persistence/project_store.cpp` (schema version 2 tables, `moveInstance`, `trimInstance`)

Documents that describe overwrite and say they do not authorise it:

- `docs/EDITORIAL_OPERATIONS.md` — "Status: proposal, awaiting review. No implementation is authorized by this document."
- `docs/research/professional_nle/03_AETHER_PROFESSIONAL_EDITORIAL_MODEL.md` — "Status: research proposal. Nothing in this file is an accepted ADR."
- `docs/research/AETHER_PROFESSIONAL_EDITORIAL_WORKFLOW_SPECIFICATION.md` — "Status: research proposal. Nothing in this file accepts an ADR, changes a schema, or authorises implementation."
- `docs/TIMELINE_ENGINE_ARCHITECTURE.md` — overwrite is named, and the track-target default is listed as a confirmation item.
- `docs/19_DEVELOPMENT_ROADMAP.md` and `DEVELOPMENT_ROADMAP.md` — overwrite is **NOT STARTED** and **NOT AUTHORISED**. Displayed inclusive OUT is not settled.

`docs/research/professional_nle/` and `docs/research/ffmpeg/` were not edited. FFmpeg, `/opt/aether-ffmpeg-9.1`, RPATH, and licensing were not changed.

## How Source In, Source Out, Record In, and Record Out become ranges

They do not. Slice 1F stores four names as two pairs of frame indexes. It does not convert either pair into a half-open span.

`ProjectSession::storePoint` writes `markIn` or `markOut` as `std::optional<std::int64_t>`. A negative frame is rejected. When both points are present, Mark Out before Mark In is rejected with "Mark Out is before Mark In", and equal points are rejected with "equal Mark In and Mark Out are an incomplete range". A single point is stored. The function does not compute `end - start`, does not add one, and does not call `identityRecordEnd`.

The source readout in `formatSourceMarks` and the record readout in `formatRecordMarks` say, when both points exist: "Points only. Displayed inclusive OUT is not settled, so this pair is not a half-open edit range." A single point is "not an edit range."

`PlacementDialog` shows a complete source pair as an offer and then says the points "are not copied into Source in or Source out and they are not a half-open edit range." The half-open numbers that `placeInstance` stores are the integers the editor types into Source in, Source out, and Record in. Record out is `identityRecordEnd` of that typed source span. The typed fields stay empty when the offer is shown.

The Slice 1F proposal states the same boundary: a stored pair with `out > in` is two ordered points, "not converted to `[in, out)` or `[in, out + 1)`", and three-point derivation is not run. The Slice 1F validation repeats it: the pair "is not `[in, out)` and it is not `[in, out+1)`".

Source marks are keyed by representation id and are source-frame indexes. Record marks are keyed by sequence id and are sequence-frame indexes. The domains stay distinct. Marks are session maps. `project.db` has no mark table. Mark commands do not push undo.

## Owner decision: displayed Mark Out

The owner decided this on 2026-11-22, after the inspection above. ADR-002 was not edited. Its text still says a displayed out-point is **DEFERRED**. The decision recorded here is the owner's choice for how a command reads a Slice 1F mark pair. It is not an ADR amendment.

The displayed Mark Out is the last included frame. A mark pair becomes the half-open range `[In, Out + 1)`. Duration in integer frames is `(Out + 1) - In`. A stored pair 10 and 18 covers frames 10 through 18. That is 9 frames. The exclusive end is 19.

Marks stay session points. They stay off `project.db`. They are not stored as ranges. `storePoint` still writes two `int64` indexes. The `+ 1` is a reading when a later command derives a span. It is not a new column and not a new table. Source points stay source frames. Record points stay sequence frames.

Slice 1F's readout still says the stored pair is not a half-open edit range. That sentence describes the stored points. This decision does not rewrite that readout. The overwrite command applies `+ 1` when it reads the pair. `Out + 1` can overflow `int64`, and that command refuses the pair with "inclusive Mark Out does not fit".

This decision does not settle overlap, source extent, sequence duration, or destination track.

The alternatives that were open, and the one the owner chose:

1. **Exclusive stored end.** The range would be `[Mark In, Mark Out)`. A stored pair 10 and 18 would have duration 8 and cover frames 10 through 17. The owner did not choose this.

2. **Inclusive displayed last frame.** Chosen. The editor's Mark Out is the last frame included. The derived exclusive end is `Mark Out + 1`, so the range is `[Mark In, Mark Out + 1)`.

3. **Marks stay out of the command.** Not chosen. A later overwrite command may read the mark pair under the rule above. The points themselves remain points.

4. **Three-point derivation.** Not chosen. Three of four points determining the fourth, and a parked playhead for a missing side, stay unauthorised. Playback is not implemented. Slice 1F has no playhead mark.

## Decisions the owner closed

Each item below was checked against accepted ADRs and the code before the owner chose. The adopted line is the owner's choice. The other bullets stay as the alternatives that were not chosen.

### Source extent

`placeInstance` does not compare a source span with a media duration. `validateInstancePlacement` checks that the source span is a half-open range of non-negative `int64` frames. It does not check a frame count from the file.

Slice 1F's readout says "Not probed. No duration. No source rate." The placement dialog says no duration is known. Slice 1E's probe stores container and stream durations as timestamps and labels them as not editorial time. `src/media/ffmpeg_probe.cpp` records "stream duration is not an editorial frame count" and "Not a verified editorial duration." There is no accepted editorial frame count to test a source span against.

`docs/EDITORIAL_OPERATIONS.md` says the source span lies inside the known source duration when one is stored. The research model says the same check waits until a duration is known, and that import did not probe when that note was written. Those sentences do not name the integer a later probe must use, and they do not authorise a conversion from a timestamp into frames.

Adopted: refuse a source extent check until an editorial frame count exists, and allow any half-open non-negative source span, which is what `placeInstance` does now. A probe timestamp is not converted into frames.

Alternatives not chosen:

- Once an editorial frame count `N` is stored, require the source span to lie inside `[0, N)`.
- Convert a probed timestamp into frames and check against that. That conversion is not authorised. ADR-002 refuses silent subframe rounding, and the probe text says the timestamp is not an editorial frame count.

### Record range versus sequence duration

`sequences` has no duration column. The columns are `sequence_id`, `project_id`, `name`, `rate_numerator`, `rate_denominator`, `created_utc`, and `modified_utc`. A sequence end is not a stored field. `validateInstancePlacement` does not compare a record span with a maximum record end. `placeInstance` accepts any non-negative record start whose same-rate end fits in `int64` and does not overlap the destination track.

`docs/EDITORIAL_OPERATIONS.md` says overwrite leaves sequence duration unchanged when record out is at or before the current end, and grows it to record out when the replacement extends past the end. The research model says the same, "except where the span hangs past the current end." The Slice 1G task text says no sequence extension. Those three statements are not one accepted rule. The task text is this assignment, not an ADR. The other two documents authorise no implementation. The schema has no duration to grow.

Adopted: refuse a record end past the current maximum `record_end` on the destination track. Schema stays version 2. No duration column. Later material does not move.

Alternatives not chosen:

- Allow a record span past the current maximum `record_end`, write only the destination track, and add no duration column. Later material does not move. This is how an empty tail works for `placeInstance`.
- Refuse a record end past the maximum `record_end` on any track of the sequence.
- Store a sequence duration and set it to the record end when the new span extends past it. That is a schema change. Schema version stays 2 unless the owner accepts a new version.

### Equal integer-frame durations

For spans that are already half-open edit ranges, one duration relationship is implemented. `identityRecordEnd` requires both rates to be an accepted reduced pair and the same pair. The record end is `recordStart + (sourceEnd - sourceStart)`. A different pair fails with "source rate and sequence rate differ; no duration mapping is authorised." `placeInstance` rejects a caller-supplied record end that is not that value, with "record end is not the same-rate source span." A 24/1 source and a 25/1 sequence fail that function. No other duration mapping is in the accepted code. `pictureSourceOffset` selects one frame. It is not a length mapping.

`trimInstance` writes the four endpoints the caller passes after `validateInstancePlacement`. It does not call `identityRecordEnd`. Unequal source and record lengths can already be stored by trim. The Slice 1D notes record that split. It is existing trim behaviour. It is not an authorised mapping for a new command.

`docs/EDITORIAL_OPERATIONS.md` and the research notes say that at matching rates the durations are equal, and that four supplied points with different lengths fail. Those documents do not authorise overwrite. They do describe the same equality `identityRecordEnd` already enforces for `placeInstance`.

Adopted: the derived source duration and the derived record duration must be equal in integer frames. Unequal lengths are refused. There is no stretch. A mark pair's duration is `(Out + 1) - In` in its own domain. A rate mismatch stays the `identityRecordEnd` refusal. `trimInstance` may still store unequal lengths. That is trim, not overwrite.

### Partial and full intersections, and source offsets

`validateInstancePlacement` rejects a record overlap with "record ranges on one track must not overlap; transitions are not implemented." `placeInstance`, `moveInstance`, and `trimInstance` all use that check. An overlap writes nothing. There is no split.

`moveInstance` keeps the existing record length and the existing source span, and slides both record endpoints. `trimInstance` replaces all four endpoints with the caller's values. Neither function computes a surviving source offset from an intersection.

`docs/EDITORIAL_OPERATIONS.md` describes a split that the code does not perform. On a targeted track it splits items at the record in and the record out, retires material strictly inside `[record in, record out)`, and leaves a continuation whose source start advances by the covered length at matching rates. Its example: item B is record `[48, 144)` and source `[10, 106)`; the replacement is record `[48, 72)` and source `[8, 32)`; B's continuation is record `[72, 144)` and source `[34, 106)`. The covered record length is 24 frames, and `10 + 24 = 34`. A full cover retires the item. Items that begin at or after record out keep their ranges. The research model says a partially covered instance is split into the uncovered remainder or remainders, and that collision today is refused rather than split. Both documents are unauthorised. The gap note in `docs/research/AETHER_ARCHITECTURE_GAP_ANALYSIS.md` says same-track overlap stays illegal until an overwrite or transition model is an accepted change.

The source-offset formula in that example is one candidate, and only at equal rates: the surviving record piece keeps the source frames that sat on those same sequence frames, so a removed head of `D` frames advances `source_start` by `D` and leaves `source_end`, and a removed tail retreats `source_end` by `D` and leaves `source_start`. A piece wholly inside the record range is deleted. A piece that extends past both ends becomes two pieces, and the right piece's `source_start` advances by the frames from the old record start to the record out. New rows need new instance ids. The retired row's id is restored by undo. None of this is in the code, and none of it is an accepted ADR. `trimInstance` can change a source endpoint by a different amount than the record endpoint, so the code does not already enforce "source delta equals record delta."

Adopted: the continuation formula above. One new instance for the replacement. Surviving pieces keep the source frames that sat on the surviving sequence frames. One transaction. One undo entry.

Alternatives not chosen:

- Refuse every intersection. That is `placeInstance`. It is not overwrite.
- Delete every intersecting instance, including a partial intersection, and drop the uncovered source frames.
- Change only the record endpoints of a partial intersection and leave the source endpoints where they were. The resulting source and record lengths differ. `trimInstance` permits that shape. It is not a stated overwrite rule.

### When nothing intersects

The unauthorised replace section says a replacement into a gap writes the clip into the gap and leaves any uncovered remainder of the gap. Gaps are not rows in schema version 2. An empty record range on a track is the absence of an instance, not a gap item. `placeInstance` inserts one row in that case, provided the span is a legal same-rate half-open range.

Adopted: write the new instance and leave every existing row unchanged. One undo entry. Rows on other tracks stay.

Alternatives not chosen:

- Succeed and write nothing, because no material was replaced.
- Fail as an empty edit.

`docs/EDITORIAL_OPERATIONS.md` uses the first alternative for a gap. The lift section of the same document fails when the span covers no removable media. Those are different operations, and the document does not authorise either command. The owner chose the adopted line: write the new instance and leave every existing row unchanged.

### Destination track

`placeInstance` takes a track id. An empty id creates a track, and a failed insert deletes that track if it is still empty. The placement dialog lists tracks in stored order. There is no track-kind column. A timeline selection can pass one fixed track id. Tracks have no lock, no sync-lock, and no patch.

`docs/EDITORIAL_OPERATIONS.md` aims overwrite at unlocked targeted tracks, skips a locked track, fails if every aimed track is locked, and leaves untargeted tracks alone. With no targets lit, its insert section aims at every unlocked patched track. `docs/TIMELINE_ENGINE_ARCHITECTURE.md` lists that no-target default as a confirmation item. Locks, patches, and targets are not in the schema and are not session state in the Slice 1F code.

Adopted: the editor chooses one existing track, and an empty choice fails. Overwrite does not create a track. It is its own command, not `placeInstance`.

Alternatives not chosen:

- The track of the selected timeline instance.
- Every track in the sequence.
- The `placeInstance` rule: an empty track id creates a track.

## What this command does not change

`placeInstance` still refuses overlap. Marks stay stored points. Schema version stays 2. `kPrototypeVersion` stays `slice-0a`. Insert, splice, ripple, split, lift, extract, roll, slip, slide, and playback stay unimplemented. Slice 1H is not started.

A failed overwrite writes nothing and adds no undo. That is the failure rule ADR-003 already states for an editorial transaction.

## Implementation

The owner closed the gates. Overwrite is a separate command under the adopted lines above. `placeInstance` still refuses overlap. Marks stay stored points. The command derives `[In, Out + 1)` when it runs. A destination track with no instance has no maximum record end, so the record-end rule refuses that command and writes nothing.
