# ADR-002 implementation consequences

Status: record of what the accepted decision requires of later work. This file does not authorise implementation. It does not add a sequence table, a timeline, playback, an audio engine, or media processing.

**ACCEPTED — INITIAL EDITORIAL TIME AUTHORITY** is the status of `architecture/decisions/ADR-002-rational-time.md`, accepted by the project owner on 2026-11-21. The pre-decision review did not grant that acceptance.

**SLICE 0A COMPLETE — DEVELOPMENT FROZEN PENDING NEXT AUTHORISATION.**

## 1. Data contract

Later storage, when a separate authorisation allows it, would hold:

| Record | Fields | Not a field |
| --- | --- | --- |
| Rate | Reduced `int64` numerator and denominator, denominator greater than zero | A float seconds value |
| Named rates in the acceptance | 24/1, 25/1, 24000/1001, 30000/1001, 60000/1001 | A second clock beside the ratio |
| Frame index | `int64`, greater than or equal to zero, at one named rate | A sample index stored as the cut |
| Range | Half-open `[start, end)` at one rate | An inclusive displayed out, which stays **DEFERRED** |
| Clip | Source range at the source rate, and sequence range at the sequence rate | A per-clip menu of floor, nearest, and ceiling |
| Timecode | Label: start, rate, drop-frame flag, availability | The phase origin of the sample grid |
| Picture selection | Nearest valid source-frame presentation timestamp. Exact tie: the earlier frame | Ceiling. A duration-rounding rule |
| Audio span | `sample_boundary(end) - sample_boundary(start)` from sequence frame 0 | A sample count stored as authority |

Normalisation reduces the rate by an exact integer gcd. Overflow fails closed. A negative stored position fails. The same inputs always select the same frame and the same sample boundary. Slice 0A already fails closed on overflow and on a negative position for the audio boundary. That behaviour is not a sequence schema.

The prototype comment in `include/aether/core/timing/sample_boundary.hpp` still says ADR-002 is not accepted. Source was not edited for this decision. The comment is stale relative to this acceptance. The function itself is the accepted floor rule.

`docs/PROJECT_DATABASE_SCHEMA.md` and `docs/BIN_DATABASE_SCHEMA.md` were not edited. A proposed `duration_samples` column, a keyframe domain of `sample`, and a required `rounding` column are not the accepted clock. They do not become tables by this decision. The accepted rule overrides those proposals until a schema change is separately authorised.

## 2. Conversion boundaries

Picture conversion and audio conversion are different operations.

Picture. For sequence offset `k` inside the clip's sequence range, with sequence rate `Ns/Ds` and source rate `Nr/Dr`:

```text
numerator   = k * Nr * Ds
denominator = Dr * Ns
```

Integer division gives quotient `q` and remainder `r`. The selected source offset is `q + 1` when `2 * r > denominator`, and `q` otherwise. An exact tie, `2 * r = denominator`, stays on `q`, the earlier frame. The frame must be a valid frame inside the stored source range. The product is computed in a width that can hold it, or the conversion fails. It does not pass through binary floating point.

Audio. For sequence frame `n >= 0` and positive integer sample rate `R`:

```text
sample_boundary(n) = floor(n * R * D / N)
```

The count of `[start, end)` is the difference of the two boundaries. Phase uses `n` as the sequence frame index from sequence frame 0. A clip that starts at frame 8 does not start a new grid at sample 0.

A typed millisecond string, where `docs/AUDIO_TIMEBASE_AND_SYNC.md` parses one, becomes a frame count before it is stored. That parse is not picture selection and is not `sample_boundary`. The stored result is a frame count. The string is not a clock.

Ceiling is not the general rule for either boundary.

## 3. Source and sequence

Sequence time is the `int64` record range at the one sequence rate. Source time is the `int64` source range at the source rate. Both are stored. Reopen reads them. It does not recompute one from the other and then discard the stored one.

Timeline position, the playhead, scroll, and zoom, remains session state. It is not a third stored clock of the film.

A still has one valid source frame. Nearest selection inside that source range is that frame. That is the source range, not a ceiling policy.

## 4. Audio

There is no stored sample-time domain. Sample indexes are derived. Fades that the audio document evaluates inside a frame stay inside the frame. Their stored duration is a frame count.

A fractional remainder in `sample_boundary` is the floor rule doing its job. It is not an error and it is not a sync warning.

A sync warning is only for a source offset or an operation that the frame-only model cannot represent. A subframe slip is that kind of operation: it is refused or reported unsupported. It is not rounded onto a neighbouring frame and saved.

The MXF encoder result, 12813 samples in the file against 12812 for frames 0–8 at 30000/1001, stays a failed encoder test in `docs/validation/AUDIO_FRAME_SAMPLE_BOUNDARY_VALIDATION.md`. This decision does not rewrite that log and does not turn the failure into a sync warning on the editorial model. `[8, 16) = 12813` is the floor rule for a later span. It does not clear the origin failure.

## 5. Mixed rate

Hybrid means both ranges survive. The sequence rate is the rate of the record range. It is not written back over the source rate or the source range. Conversion runs when a sequence frame must name a source frame, and the result is the picture-selection rule in section 2. Two clips do not carry two audio rounding policies. One sequence has one `sample_boundary`.

## 6. Persistence

Slice 0A persistence is unchanged: one `project.db`, a manifest, eight tables, no sequence table, no timecode column, no sample column. The manifest is package metadata. A bin is not a timeline.

When persistence of a sequence is later authorised, the integers in section 1 are the contract. Until then, writing those columns would be an implementation this file does not allow.

## 7. Timeline

`docs/TIMELINE_ENGINE_ARCHITECTURE.md` and `docs/EDITORIAL_OPERATIONS.md` keep their operation descriptions. Commands carry frames. The source frame of a mixed-rate cut is the picture-selection rule, not a private widget formula. Item 9 of the timeline review list is the accepted floor function. The other items on that list are not accepted by ADR-002.

No timeline engine is started.

## 8. Interchange

Exchange documents were not given a new writer. What they can keep is unchanged in kind: integers and reduced ratios where the format has them, timecode as a label, and a loss report where the format cannot hold a frame-only refusal. CMX 3600 still cannot hold every rational speed. AVX still stores rationals as strings. No vendor test is claimed. ADR-007 is not accepted and is not modified.

A subframe value that a foreign file contains is reported unsupported on import into this initial model. It is not silently rounded.

## 9. Worked examples

These use the accepted rules. They are not a new test run. Picture offsets assume source start 0 and a source range that contains the selected frame. Audio uses 48000 Hz.

### 9.1 23.976 in 24

Sequence 24/1. Source 24000/1001. `k * 1000 / 1001`.

| Sequence offset k | Quotient | Remainder | Selected source frame |
| --- | --- | --- | --- |
| 0 | 0 | 0 | 0 |
| 1 | 0 | 1000 of 1001 | 1 |
| 24 | 23 | 977 of 1001 | 24 |

`2 * 1000 > 1001`, so frame 1 is nearer than frame 0. `2 * 977 > 1001`, so frame 24 is nearer than frame 23. No integer `k` lands on an exact half for this pair, because that equation has no integer solution. The stored source range is still the source's own 24000/1001 range. The sequence rate does not replace it.

### 9.2 25 in 23.976

Sequence 24000/1001. Source 25/1. `k * 25025 / 24000`.

| Sequence offset k | Exact quotient | Selection |
| --- | --- | --- |
| 1 | 1 + 1025/24000 | 1, because `2 * 1025 < 24000` |
| 480 | 500.5 | 500, the earlier frame |

Offset 480 is an exact tie between source frames 500 and 501. The earlier frame is 500. The 2026-11-20 preparation text moved an exact tie to the later frame. That tie direction is not the acceptance.

### 9.3 29.97 in 24

Sequence 24/1. Source 30000/1001. `k * 1250 / 1001`.

| Sequence offset k | Exact value | Selected source frame |
| --- | --- | --- |
| 1 | 1 + 249/1001 | 1 |
| 24 | 29 + 971/1001 | 30 |

One sequence second is 24 frames at 24/1 and selects source frame 30. The real source position for that second is 30000/1001, which is nearer to 30 than to 29. This selection does not change the audio count of those 24 sequence frames. Audio still uses `sample_boundary` on the sequence rate.

### 9.4 48 kHz boundary

Sequence 30000/1001. Sample rate 48000. Phase anchored at sequence frame 0.

| Boundary or span | Samples |
| --- | --- |
| `sample_boundary(8)` | 12812 |
| `[0, 8)` | 12812 |
| `[8, 16)` | 12813 |
| `sample_boundary(200000)` | 320320000 |

A clip, an edit, or an export in-point at frame 8 starts at sample 12812. It does not start a new eight-frame origin of 12812. Rounding every eight-frame segment as if it began at frame 0 would miss `sample_boundary(16)` by one sample. That per-segment rule is rejected.

### 9.5 Timecode 01:00:00:00

Sequence frame 0 may be labelled `01:00:00:00`. The label is display and identification. `sample_boundary` is still computed from frame index 0, so the boundary at the labelled start is 0. At 30000/1001 the next eight sequence frames still contain 12812 samples.

The same label on a 24/1 source means source frame 0, not sequence frame 0, and not sample phase. Parsed against midnight at 24/1, the text sits 86400 label frames from midnight. That midnight count is not the sequence sample origin. An export in-point does not move the origin either.

## 10. Deferred and open

| Question | State |
| --- | --- |
| Displayed out-point inclusive or exclusive | **DEFERRED.** Internal ranges stay half-open. |
| Subframe audio or video as a stored position | **OPEN** for a later authorisation. Out of the initial model. Not a permanent ban. |
| 59.94 drop-frame skipped-number table | **OPEN.** The flag is a label. The table was not confirmed. |
| Pull-up and pull-down | **OPEN.** No silent 0.1 percent change. |
| Speed-ramp resampling beyond frame selection | **OPEN.** |
| Which additional rate names beyond the five accepted names | Not a second decision. 30/1, 48/1, 50/1, and 60/1 remain named in the time documents as the same kind of ratio. |
| ADR-006 and ADR-007 | **PROPOSED.** Untouched. |
| Sequence tables, timeline, playback, audio engine, media processing | Not authorised. |

## 11. Future validation

A later authorised test would compare integers already stated here: the three picture examples, the tie at source frame 500, the floor results 12812, 12813, and 320320000, a negative position failing, and an overflow failing. It would not call the MXF origin span a pass. No such new test is run by this document. The Slice 0A timing evidence stays in `docs/validation/SLICE_0A_PROJECT_CORE_VALIDATION.md`.

## 12. What this file is not

This file is not an implementation slice. It does not authorise code, schema, packages, or a player. The freeze stands.

**SLICE 0A COMPLETE — DEVELOPMENT FROZEN PENDING NEXT AUTHORISATION.**
