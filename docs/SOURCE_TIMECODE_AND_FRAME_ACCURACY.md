# AETHER source timecode and frame accuracy

Status: source-timecode detail. Where this file previously chose a rounding menu, a project-wide policy, or a stored sample clock, ADR-002 (`architecture/decisions/ADR-002-rational-time.md`) is the authority, accepted 2026-11-21. This file does not authorise implementation. No timeline was executed. The worked example is illustrative and is not a measured conform. **SLICE 0A COMPLETE — DEVELOPMENT FROZEN PENDING NEXT AUTHORISATION.**

## Verified facts

- The timeline master spec stores a rational timebase `N/D` and integer positions, and it maps audio with integer arithmetic (`docs/TIMELINE_ENGINE_ARCHITECTURE.md`).
- That spec treats drop-frame as a label behaviour for 29.97 and 59.94. The model does not drop frames. The internal architecture note states the same distinction.
- The prior FFmpeg 9.0.2 study reported a 29.97 audio sample-count mismatch against picture duration. That is a media warning to reconcile. It is not a timecode reading taken for this document.
- No standard in this document was executed against a file.

## Engineering recommendations

- Keep source timecode, source frame index, and sequence position as three different numbers.
- Refuse a writer that stores the cut as floating-point seconds.
- When a container has no start timecode, store availability `absent`. Do not invent `00:00:00:00` and do not invent a zero first-frame timestamp.

## Proposed behaviour

### Scope

This document defines frame-accurate source time for offline and online work: rates, drop-frame labels, start timecode, the independence of source range and timeline position, and the integer audio map. Speed curves that consume this time are in `SEQUENCE_SOURCE_MAPPING.md`.

### Requirements

- Authority is a rational rate `N/D` in lowest terms, plus integer frame positions. Sample indexes are derived. They are not a stored clock. The named rates ADR-002 accepts are 24/1, 25/1, 24000/1001, 30000/1001, and 60000/1001.
- Display may format seconds. Those strings are not read back as the authority.
- Supported named rates, each stored as integers:

| Name | `N/D` | Drop-frame label |
| --- | --- | --- |
| 23.976 | `24000/1001` | No |
| 24 | `24/1` | No |
| 25 | `25/1` | No |
| 29.97 | `30000/1001` | No |
| 29.97 DF | `30000/1001` | Yes |
| 30 | `30/1` | No |
| 50 | `50/1` | No |
| 59.94 | `60000/1001` | Optional label, stored explicitly |
| 60 | `60/1` | No |
| Other | Any positive integers | Explicit flag |

- A sequence may contain instances whose source rates differ from the sequence rate and from each other.
- Source start timecode is the label of source frame 0 when the camera or container supplied it. Availability is stored.
- The first-frame media timestamp, when the container has one, is a rational `value/timescale` or an integer pair. It is not a binary float. It is not assumed to be zero.
- Source in and source out are positions in the source timebase. Record in and record out are positions in the sequence timebase. Export writes both. A writer that copies the record timecode into the source timecode field fails validation.
- Stored ranges are half-open `[in, out)` in integer frames. Whether a displayed out-point is inclusive or exclusive is **DEFERRED**. The earlier proposal that the displayed out is one frame before `source_out` is not decided. The phrase "architecture decision 7" in the previous text of this bullet was that deferred display question. It is not ADR-007.
- Drop-frame changes the displayed numbers at minute boundaries under the SMPTE counting rule. It does not remove frames from `duration_frames`.
- Picture duration is a frame count. An audio sample position is `sample_boundary` in ADR-002. It is not stored as a second clock and it is not stored as a float such as `1601.6`. A container's measured sample count may be compared later. A fractional-sample mapping is not an error.

### Data model

```text
timecode
  hours, minutes, seconds, frames   ints
  rate_n, rate_d                    ints
  drop_frame                        bool
  availability

source_time
  start_timecode                    timecode
  first_frame_ts_n, first_frame_ts_d
  first_frame_availability
  frame_count                       int

```

Picture selection and the audio boundary are ADR-002. This file does not keep a project-wide rounding field of `nearest`, `floor`, and `ceil`. Ceiling is rejected as the general picture or audio boundary policy.

Illustrative example, 24/1, not a measured cut. Sequence rate equals source rate. Speed is `1/1`.

| Quantity | Display | Stored |
| --- | --- | --- |
| Source start | `01:00:00:00` | Frame 0 of the file is labelled 01:00:00:00. Label frame index from midnight is `86400`. |
| Source in | `01:00:12:08` | Source frame `296` (`12*24+8`). |
| Source out, displayed form | `01:00:18:20` | Inclusive reading of label frame `452`, shown here only as the deferred display question. The stored out is the next row. |
| Source out, stored half-open | next frame | Source frame `453`. |
| Duration | 157 displayed frames | `453 - 296 = 157`. |
| Timeline position | `00:02:14:00` | Record in frame `3216` if the sequence start label is `00:00:00:00` (`2*60*24 + 14*24`). |

An export of this instance writes source in `01:00:12:08` and the stored source range. It does not write `00:02:14:00` as the source timecode. The numbers exist to lock that distinction. They are not the result of a program run.

Audio for a picture frame uses `sample_boundary` in ADR-002, anchored at sequence frame 0. The multiplication uses integers. A binary floating-point intermediate is not the stored result. There is no `round_policy` menu on the project.

A sync warning is only for a source offset or an operation the frame-only model cannot represent. The sentence previously in this paragraph treated any sample-count difference as that warning. That general rule is withdrawn. The MXF measurement itself is unchanged and lives in `docs/validation/AUDIO_FRAME_SAMPLE_BOUNDARY_VALIDATION.md`. This file does not rewrite that log. When this paragraph was first written, it had not itself opened that file.

### APIs

| Operation | Contract |
| --- | --- |
| `Time.parseTimecode(text, rate, drop_frame)` | Returns a frame offset from the rate’s midnight label, or an error if the text is not a timecode. Does not accept a decimal-second string as authority. |
| `Time.formatTimecode(frame, rate, drop_frame)` | Display only. |
| `Time.sourceFrame(media, timecode)` | Subtracts the stored start label. Errors if start timecode availability is not `present`, unless the caller passes an explicit user start marked `unverified`. |
| `Time.recordToSource(instance, record_frame)` | Uses the speed map in `SEQUENCE_SOURCE_MAPPING.md`. |
| `Time.frameToSample(frame, rate, sample_rate)` | `sample_boundary` in ADR-002. No rounding argument. |

### Workflow

Ingest stores start timecode availability from the probe. The timeline stores record positions in the sequence rate. Exchange writes both, plus the rationals and the drop-frame flag. Conform recomputes duration from stored integers and compares it with the report. A match of displayed strings is not the check. The check is integer equality after parsing with the stored rate and drop-frame flag.

Mixed-rate sequences select a source frame with ADR-002. Speed remains the open map in `SEQUENCE_SOURCE_MAPPING.md`. The previous `round_policy` expression is withdrawn.

```text
source = source_in + picture_selection(offset, speed, source_rate, sequence_rate)
```

`offset` is in sequence frames. Rates and speed are rationals. The specification adds that `speed` may be a map rather than a single rational (`SEQUENCE_SOURCE_MAPPING.md`). A single rational remains the `normal`, `constant`, `reverse`, and `freeze` cases.

### Validation

- Every persisted rate has positive `N` and `D`.
- Drop-frame true is refused for rates that are not `30000/1001` or `60000/1001`, until a review adds another rate. The flag is not inferred from the name “29.97”.
- Half-open `out > in` for a positive duration. A freeze stores a zero source advance and a positive record duration, which is the timeline engine’s rule, and the speed map says so.
- Re-parsing a formatted timecode returns the same frame.
- A DF timecode that lands on a skipped number (`00` or `01` at the start of a non-tenth minute, in the SMPTE display rule) is an invalid label, not a nearby frame chosen in silence.

### Errors

| Error | Result |
| --- | --- |
| Decimal seconds submitted as a stored position | Rejected |
| Missing start timecode treated as zero by a caller | Rejected unless the caller records a user-supplied start as `unverified` |
| DF flag missing | Availability `unknown`. Conform treats unknown DF on a `30000/1001` source as `manual_review_required`. |
| Sample count mismatch | `sync_warning`. Conform is not `verified`. |

### Recovery

Time fields are part of the editorial snapshot. Recovery restores the integers. It does not recompute positions from waveforms. If a document’s rate pair is corrupt, the sequence opens as `failed` for conform and the editor is told which row. Playback may still be refused for that instance. Neighbouring instances remain.

### Performance

Conversions are integer arithmetic on records already in memory. Timecode display is per visible item, not per file byte. No benchmark was run. Drop-frame formatting must be a pure function of frame index, rate, and flag so it can be tested without media.

### Tests

Test F, specified and not executed. Includes the illustrative 24 fps example as a fixture of stored integers, each named rate’s parse and format round-trip, a DF skipped-number rejection, a mixed-rate conversion, a non-zero start timecode, an absent start timecode, and the 29.97 sample comparison as a specified check. The FFmpeg note is a reconcile case, not a pass or a fail.

### Acceptance

- Float seconds are excluded from the authority path.
- Source position and timeline position are both specified, with the illustrative numbers showing they differ.
- DF is a label flag.
- Audio uses the derived integer boundary. The 29.97 encoder result stays in the validation report.

## Unresolved questions

- Displayed out-point, inclusive or exclusive. **DEFERRED.** Internal storage stays half-open. The old name "architecture decision 7" for this display question is not ADR-007.
- A stored sample clock at source rate, sequence rate, or one project tick. Closed for the initial model by ADR-002: samples are derived, not stored. The old name "architecture decision 8" was this question. It is not a licence to keep a second clock. A later subframe authorisation may consider sample positions. That step stays **OPEN**.
- 59.94 drop-frame display. The flag is stored. The exact skipped-number table for 59.94 must be confirmed against SMPTE practice in a later test. It was not confirmed here.
- Pull-up and pull-down. Recommendation: do not apply a pull-up unless a project preset stores it as an explicit rational. No silent 0.1 percent change.
