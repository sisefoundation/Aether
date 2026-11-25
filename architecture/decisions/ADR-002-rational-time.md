# ADR-002 — Rational time as the stored authority

- Status: **ACCEPTED — INITIAL EDITORIAL TIME AUTHORITY**
- Proposed: 2026-11-20
- Accepted: 2026-11-21
- Deciders: the project owner. `docs/decisions/ADR-002_TIME_AUTHORITY_DECISION_REVIEW.md` is the pre-decision review. That review did not grant this acceptance.
- Related: `docs/05_TIMEBASE_AND_TIMING_ARCHITECTURE.md`, `docs/TIMEBASE_AND_FRAME_ACCURACY.md`, `docs/AUDIO_TIMEBASE_AND_SYNC.md`, `docs/decisions/ADR-002_IMPLEMENTATION_CONSEQUENCES.md`

## Context

Feature cuts are reopened months later and must land on the same frame. Floating-point seconds drift. Drop-frame timecode is a counting label used on 29.97 and 59.94 displays. It does not remove frames from the model.

The 2026-11-20 proposal stored a per-clip mapping policy of `nearest`, `floor`, or `hold`, and its `nearest` moved an exact tie toward the greater frame index. The owner acceptance below replaces that menu for picture selection. The floor function for audio frame boundaries is kept. It is not a picture-selection policy.

## Decision

**ACCEPTED — INITIAL EDITORIAL TIME AUTHORITY.**

The project owner accepted this initial editorial time authority on 2026-11-21. Acceptance does not authorise a sequence schema, a timeline, playback, an audio engine, or media processing. No sequence table is added. Development stays frozen:

**SLICE 0A COMPLETE — DEVELOPMENT FROZEN PENDING NEXT AUTHORISATION.**

### Stored clock

Positions are `int64` frame indexes. Rates are reduced integer ratios. The named rates in this acceptance are 24/1, 25/1, 24000/1001, 30000/1001, and 60000/1001. Float seconds are not a stored clock. Timecode is a label. Arithmetic is checked: ratios are normalised, a product that does not fit the integer width fails, a negative stored position fails, and the same inputs convert to the same frame. This decision does not create a sequence or timeline schema.

### Mixed rate

One sequence has one sequence rate. Source media keeps its native timing. Each clip stores a source range and a sequence range. Conversion between them is explicit. The sequence rate must not destroy the source timing.

### Picture selection

The source frame presented for a sequence frame is the valid source frame whose presentation timestamp is nearest to that sequence frame. An exact tie selects the earlier source frame. This is frame selection, not a duration-rounding rule. Ceiling is rejected as the general picture or audio boundary policy.

### Audio for frame-boundary edits

```text
sample_boundary(n) = floor(n * sample_rate * fps_denominator / fps_numerator)
sample_count = sample_boundary(end) - sample_boundary(start)
```

Boundaries are absolute on the sequence. A segment is not rounded on its own. Phase is not reset at a clip, an edit, a timecode origin, or an export in-point. Phase is anchored at sequence frame 0. A display timecode of `01:00:00:00` does not reset phase. There is no stored sample-time domain. A fractional-sample mapping is not an error. A sync warning is raised only when a source offset or an operation cannot be represented in the frame-only model. Future sample positions may be considered when subframe audio is authorised.

### Frame-only scope

The initial model is frame-only. A subframe video position or a subframe audio position is refused or reported unsupported. It is never silently rounded onto a frame. This is a scope limit, not a permanent ban.

### Measured boundaries

These results stay exactly as measured. They are established behaviour of the floor rule. They are not re-decided here.

- 29.97, frames 0–8: 12812 samples at 48000 Hz
- 29.97, frames 8–16: 12813 samples at 48000 Hz
- The 200000-frame walk at 30000/1001 and 48000 Hz: 320320000

Different 8-frame counts are expected. The FFmpeg MXF result of 12813 samples against the contract 12812 remains a failed encoder test. That log is not rewritten.

### Ranges

Internal storage is half-open `[start, end)`. Whether a displayed out-point is inclusive or exclusive stays **DEFERRED**. This decision does not decide it.

## Consequences

Dependent specifications point here instead of restating this decision. The implementation consequences, including the five worked examples, are `docs/decisions/ADR-002_IMPLEMENTATION_CONSEQUENCES.md`. That file does not authorise implementation.

Undo records, render requests, and AVX JSON, when they exist, store integers. A test that compares frames compares integers. Slice 0A already implemented the floor span function and did not store a sequence. The prototype header still says this ADR is not accepted, because source files were not edited for this acceptance.

ADR-006 and ADR-007 are unchanged and remain **PROPOSED**.

## Alternatives

- Float seconds as the stored clock. Rejected.
- Ceiling as the general picture or audio boundary policy. Rejected.
- Rounding each audio segment from its own origin, or resetting sample phase at a clip, an edit, a timecode origin, or an export in-point. Rejected.
- A stored sample-time clock beside the frame clock. Rejected for this initial model.
- Silent rounding of a subframe position onto a frame. Rejected. Refusal or an unsupported report is the initial behaviour. A later authorisation may consider sample positions. That later step is **OPEN**. It is not part of this acceptance.

## Confirmation

The project owner accepted this initial editorial time authority on 2026-11-21. The decision review recommended a tie toward the later frame and treated a container sample mismatch as a sync warning. The owner did not adopt those two recommendations. An exact picture tie selects the earlier frame. A fractional-sample mapping is not an error.
