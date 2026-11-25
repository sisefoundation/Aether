# Timing and mixed-rate gap analysis

Inspection date: Sunday 22 November 2026, Asia/Calcutta. Read-only. No source file was modified.

## Authority that stays

ADR-002 is accepted. Positions are `int64` frame indices. Named rates are already reduced: 24/1, 25/1, 24000/1001, 30000/1001, 60000/1001. Spans are half-open. A source span and a record span are different domains. Float seconds are not the clock. Displayed inclusive OUT is deferred.

`identityRecordEnd` in `src/core/sequence/sequence.cpp` requires both rates to be accepted and the same reduced pair. The record end is `recordStart + (sourceEnd - sourceStart)`. A different pair fails with the message that no duration mapping is authorised. It does not call `pictureSourceOffset`.

`pictureSourceOffset` in `src/core/timing/sample_boundary.cpp` selects one source frame for one sequence frame. The code multiplies the sequence offset by the source numerator and the sequence denominator, divides by the source denominator times the sequence numerator, and keeps the lower frame when the remainder is an exact half. That is nearest-frame selection with the tie on the earlier frame. It is not a length mapping. This document does not turn it into one.

## Same-rate placement, validated

`docs/validation/SLICE_1D_TIMELINE_CLIP_PLACEMENT.md` records Slice 1D as complete on 22 November 2026. The executable is `build-slice1d/src/aether`. Placement goes through `placeInstance`. A 25/1 source on a 24/1 sequence was refused and wrote nothing. A 24/1 source `[0, 8)` at record `[2, 10)` was stored. Move, trim, three undos, three redos, and close/reopen of the committed row were exercised. Undo and redo were not sensitive after reopen. ShellCut was not opened.

That validation is same-rate placement. It is not mixed-rate editing, not probing, and not source/record editing.

Qt Test totals inside the one CTest `slice0a`, as that validation document reports them and as `build-slice1d/Testing/Temporary/LastTest.log` records them (22 November 2026, 01:40 IST, Qt 6.11.2): Timing 7, Project 11, Sequence 10, Timeline 10, Media 10, UiIntegration 9, TimelinePlacement 6. The headers declare 5, 9, 8, 8, 8, 7, and 4 cases. The log also passes the default `initTestCase` and `cleanupTestCase` for each class, which accounts for the extra two. This research pass did not re-run CTest.

## What each rate pair does today

| Sequence | Source | Source span | Record start | Result |
| --- | --- | --- | --- | --- |
| 24/1 | 24/1 | `[100, 148)` | `1000` | Record `[1000, 1048)` if the caller passes that end. |
| 25/1 | 25/1 | `[100, 148)` | `1000` | Record `[1000, 1048)` under the same rule. |
| 24000/1001 | 24000/1001 | `[100, 148)` | `1000` | Same identity. The drop-frame display of the numbers is not the stored index. |
| 30000/1001 | 30000/1001 | `[100, 148)` | `1000` | Same identity. |
| 24/1 | 25/1 | `[0, 25)` | `0` | Refused. A 24-frame record span is not computed. |
| 25/1 | 24/1 | `[0, 24)` | `0` | Refused. |
| 24000/1001 | 30000/1001 | any positive span | any | Refused. |
| 30000/1001 | 24000/1001 | any positive span | any | Refused. |

`placeInstance` checks the mapped end before it creates a track or a row. A refusal has no undo entry. An empty track id that then fails the insert deletes the track it had just created. That cleanup is in the session code. It is not a mixed-rate conversion.

## Audio floor counts

The published 48 kHz results stay:

- 30000/1001 frames `[0, 8)` → 12812 samples.
- 30000/1001 frames `[8, 16)` → 12813 samples.
- A 200000-frame walk at that rate and sample rate → 320320000 samples.

The rule is `floor` of `n * sample_rate * fps_den / fps_num` at the frame boundaries. Those two adjacent 8-frame spans are not equal. They are not a picture duration and they do not authorise a speed percentage. The FFmpeg MXF encoder test that emits 12813 samples against the 12812 contract remains failed.

## Gaps

| Topic | Status | Evidence |
| --- | --- | --- |
| Same-rate record end from a source span | IMPLEMENTED AND VALIDATED | `identityRecordEnd`; Slice 1D validation |
| Refusal of a different source rate | IMPLEMENTED AND VALIDATED | Slice 1D live 25/1 on 24/1; `placeInstance` check |
| Nearest source frame for one sequence frame | IMPLEMENTED AND VALIDATED as a selector only | `pictureSourceOffset` and the timing tests. Not a placement duration. |
| Audio sample boundaries at the published counts | IMPLEMENTED AND VALIDATED | Timing tests named by the timing work; counts above are the published contract |
| Inclusive zone length (`out - in + 1`) | Not an Aether rule | Kdenlive `timelinemodel.cpp` line 2121. Aether stays half-open. |
| Float scene FPS copied from a movie | Not an Aether rule | Blender `strip_add.cc` writes `frs_sec` / `frs_sec_base` under `SEQ_LOAD_MOVIE_SYNC_FPS`. |
| Mixed-rate duration | NOT IMPLEMENTED | Refused on purpose. Stays refused until a conversion service exists. No service is specified here. |
| Displayed inclusive OUT | DEFERRED | ADR-002 |
| Probe of imported media duration | NOT IMPLEMENTED | Import records "Not probed". ADR-006 is proposed. |

## Kdenlive, Blender, Shotcut, MLT

Kdenlive's zone duration includes both ends. Using those integers as Aether ends would add one frame. Blender's strip fields are integers, and the movie-add path can also write a float scene rate. Shotcut's insert and overwrite commands pass MLT positions into a tractor of playlists. MLT's `mlt_position` is `double` or `int32_t` depending on `DOUBLE_MLT_POSITION`. None of those representations replaces ADR-002.
