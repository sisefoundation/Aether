# AETHER Timebase and Frame Accuracy

Status: detail of the time model. The project owner accepted the initial editorial time authority on 2026-11-21 in `architecture/decisions/ADR-002-rational-time.md`. This document does not authorise implementation. Worked cases for the acceptance are in `docs/decisions/ADR-002_IMPLEMENTATION_CONSEQUENCES.md` section 9. **SLICE 0A COMPLETE — DEVELOPMENT FROZEN PENDING NEXT AUTHORISATION.**

This document expands section 3 of [ARCHITECTURE.md](../ARCHITECTURE.md). It does not replace the locked decisions there. Integer rational time is the only stored time. Timecode is a label. Audio edits align to sequence frames. Subframe slip is outside the first model.

Read [EDITORIAL_PHILOSOPHY.md](EDITORIAL_PHILOSOPHY.md) for the three names used below: source time, sequence time, and timeline position. The kernel types `Ratio`, `FrameRate`, and `Range` are defined in [MODULES.md](../MODULES.md).

## 1. What is stored

A `Ratio` is a reduced pair of 64-bit integers. The denominator is greater than zero. Reduction uses exact integer arithmetic.

A `FrameRate` is a ratio in frames per second. These values are ordinary ratios, not special cases in the type:

| Name on screen | Ratio |
| --- | --- |
| 24 | 24/1 |
| 25 | 25/1 |
| 23.976 | 24000/1001 |
| 29.97 | 30000/1001 |
| 48 | 48/1 |
| 59.94 | 60000/1001 |

A position at a rate is an `int64` frame index. Frame 0 is the origin of that count. A stored position is greater than or equal to zero. A `Range` is a half-open interval `[start, end)` of frame indexes at one rate. A duration is `end - start` at that same rate. A range with `end < start` is rejected at the boundary of the type.

Floating-point seconds may be formatted for a person to read. They are not written into the document, the undo stack, the resolver, or a render request. Pixel coordinates are a view of frames. They are not written back as time.

## 2. Three clocks

The cut uses two stored clocks. The session adds a third, and the third is not a timebase of the film.

### 2.1 Source time

Source time is an integer frame at the source rate.

- A file asset's source rate is the rate recorded on the catalog from the probe.
- An image sequence's source rate is the rate declared for that asset. One asset is the whole sequence, with a frame range.
- A still stores one source frame. Every record frame in the clip selects that frame, because it is the only valid source frame.
- A nested sequence uses the inner sequence's rate as the source rate of the outer clip.

A clip stores a source range at that rate. The range lies inside the known source duration when the document carries one. The resolver's output for a sequence frame is a source position at the source rate, as `CompositionSlice` already requires.

### 2.2 Sequence time

Sequence time is an integer frame at the sequence rate. Each sequence has one rate. A project may hold sequences at different rates. There is no project-wide rate that overrides a sequence. A start timecode is a label. Sample phase is anchored at sequence frame 0 and is not reset by that label.

An item stores a record range at the sequence rate. On one track, record ranges do not overlap, except where a transition names the two neighbouring items and its own record range lies on that cut. Gaps occupy record ranges too, as specified in [TIMELINE_ENGINE_ARCHITECTURE.md](TIMELINE_ENGINE_ARCHITECTURE.md).

The sequence duration is the maximum track end, measured in sequence frames from the sequence start frame. A track that ends earlier contributes empty time up to the sequence duration only in the viewer, as the absence of a layer. It does not invent a stored tail on the shorter track.

### 2.3 Timeline position

Timeline position is how this session looks at sequence time.

| Field | Type | Meaning |
| --- | --- | --- |
| Playhead | sequence frame index | The frame the editor is parked on. |
| Scroll origin | sequence frame index, plus a pixel remainder | The frame at the left edge of the lane area, and the pixel shift inside the zoom ratio. |
| Scale | `Ratio` pixels per frame | How many pixels one sequence frame occupies. |
| Vertical origin | track index, plus a pixel remainder | Which track row is at the top of the lane area. |

The playhead is a sequence frame. It is not a source frame and it is not a pixel. Seeking sets it. Playback replaces it with the sequence frame Processing reported in `PresentedFrame`. A device clock is not the playhead.

Scroll and scale are session and presentation state. They change which frames are on screen. They do not change a record range. Hit-testing converts a pixel to a frame with the integer rule in section 6, and the command then carries that frame.

A saved project may store the last playhead as a convenience, as the architecture already allows. Interchange may ignore it. Zoom and scroll are not fields of the timeline document.

## 3. Mapping a sequence frame to a source frame

For a clip whose record range contains sequence frame `S`:

```
k = S - recordStart
```

`k` is the offset in sequence frames, and `0 <= k < recordEnd - recordStart`.

When the source rate equals the sequence rate, the timestamps match every frame, so selection is the identity:

```
sourceFrame = sourceStart + k
```

and `sourceStart <= sourceFrame < sourceEnd`.

When the rates differ, picture selection is the accepted rule in ADR-002. Let the sequence rate be `Ns/Ds` and the source rate be `Nr/Dr`, all positive integers, ratios already reduced.

```
numerator   = k * Nr * Ds
denominator = Dr * Ns
```

Integer division gives quotient `q` and remainder `r`. The source offset is `q + 1` when `2 * r > denominator`, and `q` otherwise. An exact tie, `2 * r = denominator`, selects `q`, the earlier frame. `sourceFrame = sourceStart + sourceOffset`, and the frame must lie in the stored source range. The product is computed in a width that can hold the product of three `int64` values, or the conversion fails. The conversion does not pass through binary floating point.

This is frame selection, not a duration-rounding rule. Ceiling is not a picture policy and is not an audio boundary policy. There is no project-wide sample-rounding menu. A still stores one source frame. The only valid frame in that range is that frame.

The source range and the sequence range are both stored. Reopen reads them. The sequence rate does not replace the source timing. The 2026-11-20 text of this section stored a per-clip choice of `floor`, `nearest`, and `hold`, and moved an exact tie toward the greater index. The owner acceptance replaces that menu.

### 3.1 Worked mapping

Sequence rate 24000/1001. Source rate 24/1. Record start 0, source start 0.

```
numerator   = k * 24 * 1001
denominator = 1 * 24000
sourceOffset = the accepted picture selection of k * 1001 / 1000
```

For `k = 500` the division is 500500 / 1000, quotient 500, remainder 500. Remainder 500 is exactly half of 1000, which is the tie. The accepted rule selects the earlier frame, 500. The 2026-11-20 text of this example selected 501. That direction is not the acceptance.

Five further cases, including 23.976 in 24, 25 in 23.976, 29.97 in 24, the 48 kHz boundary, and timecode `01:00:00:00`, are section 9 of `docs/decisions/ADR-002_IMPLEMENTATION_CONSEQUENCES.md`. They are not a second decision.

### 3.2 Identity at matching rates

At 24/1 sequence and 24/1 source, `sourceFrame = sourceStart + (S - recordStart)` for every frame in the record range. The export oracle in the roadmap checks this identity by frame number. This document adds the mixed-rate case above as the same kind of fixture: the expected source frame is written as an integer.

## 4. Audio sample relationships

The editorial cut is a sequence frame. An audio edit falls on a sequence-frame boundary. The first model has no subframe slip.

The sample index of a boundary is a function of the sequence, so that every clip agrees on where frame `F` starts in the output sample stream.

Let the sequence rate be `N/D` frames per second, and let the output sample rate `R` be a positive integer samples per second. For a sequence frame `F >= 0`:

```
sampleIndex(F) = floor(F * R * D / N)
```

The floor is integer division of the exact product. The function fails closed if the product does not fit the integer width. It is not computed in binary floating point.

The number of samples that belong to frame `F` is:

```
sampleIndex(F + 1) - sampleIndex(F)
```

The number of samples in a half-open span `[start, end)` is `sampleIndex(end) - sampleIndex(start)`, which is the same function. Boundaries are absolute from sequence frame 0. A clip, an edit, a timecode origin, and an export in-point do not reset the phase. A display of `01:00:00:00` does not reset it. At 30000/1001 and 48000, `[0, 8)` is 12812 samples and `[8, 16)` is 12813. The 200000-frame walk is 320320000. The 2026-11-20 measurement kept the origin value and did not adopt a rounding that would turn it into 12813 (`docs/validation/AUDIO_FRAME_SAMPLE_BOUNDARY_VALIDATION.md`). That MXF result remains a failed encoder test. A fractional sample in this floor is not an error.

That difference is constant when `R * D` is divisible by `N`. It alternates between two adjacent integers when it is not. The alternation is determined by the remainder and is the same on every reopen.

### 4.1 Rates that divide evenly

| Sequence rate | Sample rate | Samples in every frame |
| --- | --- | --- |
| 24/1 | 48000 | 2000 |
| 25/1 | 48000 | 1920 |
| 24000/1001 | 48000 | 2002 |
| 48/1 | 48000 | 1000 |

The 24000/1001 case is exact: `48000 * 1001 / 24000 = 2002`.

### 4.2 A rate that does not divide evenly

Sequence rate 30000/1001, sample rate 48000.

```
sampleIndex(F) = floor(F * 48000 * 1001 / 30000)
               = floor(F * 8008 / 5)
```

| Frame F | sampleIndex(F) | Samples in the frame |
| --- | --- | --- |
| 0 | 0 | 1601 |
| 1 | 1601 | 1602 |
| 2 | 3203 | 1601 |
| 3 | 4804 | 1602 |
| 4 | 6406 | 1602 |
| 5 | 8008 | 1601 |

`sampleIndex(5) = 8008`, so frames 0 through 4 cover exactly 8008 samples. The pattern 1601, 1602, 1601, 1602, 1602 repeats. That alternation is the entire sync rule for this pair of rates. A trim that moves a cut by one sequence frame moves the audio boundary from one of these sample indexes to the next. It does not move it by a fraction of a frame.

Source audio uses the same function at the source rate, to name the source sample a source frame boundary corresponds to. Picture selection is not an audio rounding policy. One sequence has one sample-boundary function. There is no stored sample-time clock. A sync warning is only for a source offset or an operation the frame-only model cannot represent.

Playback seeks picture and sound to the same rational time, then reports the presented sequence frame. The sample index above is that rational time, expressed as an integer sample. Monitoring latency is a Processing concern. It does not change `sampleIndex`.

## 5. Timecode is a label

A timecode label is a start value, a rate, and a drop-frame flag. Formatting and parsing live in the kernel. The frame index is the stored value.

Non-drop counting numbers every frame. At 24/1, with a label start of 00:00:00:00, sequence frame 48 formats as 00:00:02:00. Frame 48 is the value the command uses.

Drop-frame counting, used as a label for 30000/1001 and 60000/1001, skips numbers in the text at the start of each minute except every tenth minute. It skips two frame numbers per minute at 29.97, and the corresponding count at 59.94. The frame index still increases by one for each frame. A drop-frame label that reads 00:01:00;00 is not frame 1800. Kernel tests already required by the roadmap cover a label that changes the printed number and leaves the frame index unchanged. Editorial commands accept the frame index the parser returns, and they reject a label that does not parse.

Reel, timecode start, and file identity stay on the catalog. They are used for relink and for interchange references. They are not a second playhead.

Typed entry parses with the rate of the side the editor is addressing: sequence rate for the sequence readout, source rate for the source readout. The parser returns a frame index or a structured error. An unparsed string is never stored as a position.

## 6. From pixels back to frames

The timeline scale is a reduced ratio `P/Q`, in pixels per sequence frame, with `P > 0` and `Q > 0`.

The left edge of sequence frame `F` in the virtual pixel axis is:

```
pixelStart(F) = floor(F * P / Q)
```

using integer arithmetic. Frame `F` occupies the half-open pixel interval `[pixelStart(F), pixelStart(F + 1))`.

A pointer pixel `x` in that same axis, after adding the integer scroll origin, hits frame `F` when:

```
pixelStart(F) <= x < pixelStart(F + 1)
```

When several frames share one pixel (`P/Q < 1`), the hit is the first frame of that pixel. Snapping, stepping, and typed timecode still name a single frame. Frame-accurate pointer aim is obtained by zooming until a frame occupies at least one pixel, or by leaving the pointer and using the keyboard.

The inverse must be total for every frame index the sequence can hold. `pixelStart` is monotonic. Building the axis by repeatedly adding a binary floating-point stride is rejected by this specification, because the error accumulates across a feature-length sequence.

Vertical position is track rows and pixels. It is not time. A track height is an integer pixel count from the lane-height preset, possibly overridden per track by a presentation preference. That override is not a document field.

## 7. Marks, park, and off-by-one

A marked span is `[start, end)`. Duration in frames is `end - start`.

The In mark is drawn on frame `start`. The Out mark is drawn on the boundary `end`, which is the first frame outside the span. The highlight covers frames `start` through `end - 1` inclusive. The transport shows the integer duration.

Park uses the playhead frame as the missing end. If the playhead sits on frame 72 and In is frame 48, the span is `[48, 72)`, duration 24, and frame 72 is outside it. After an insert of that span, the playhead hint rests on the new end boundary, ready for the next splice.

Comparing two times uses integer comparison at the same rate. A source frame and a sequence frame are not compared until section 3 has mapped one into the other's rate.

## 8. What a command is allowed to add

A legal delta is an `int64` number of frames at a named rate. Applying it uses checked addition. Overflow, a negative stored position, a source range outside the known duration, or an overlap that is not a transition makes the command fail. The document stays as it was.

Sequence frames and source frames are never mixed in one addition. A roll moves a sequence-frame cut by a sequence delta, and it moves each affected source range by the source delta that section 3 assigns to that sequence delta. At matching rates the two deltas are equal. At mixed rates the source delta is the picture selection of that sequence delta. The stored source range remains the source timing.

## 9. Worked sequence

Sequence "Reel 1" at 24/1, start frame 0. Source asset WS at 24/1, duration 400, label start 01:00:00:00.

| Item | Record range | Source range | Source label of the first frame |
| --- | --- | --- | --- |
| Shot A | [0, 48) | [100, 148) | 01:00:04:04 |
| Shot B | [48, 144) | [10, 106) | 01:00:00:10 |

Frame 48 of the sequence is the first frame of Shot B. Its source frame is 10. The label 01:00:00:10 is display. The command that trims this cut carries sequence frame 48 and, after the mapping, source frame 10.

At 48000 samples per second, `sampleIndex(48) = 48 * 2000 = 96000`. A roll that moves the cut to sequence frame 52 moves the audio boundary to sample 104000. Both numbers are exact.

## 10. Invariants the fixtures must name

A later editorial phase can turn these into tests. They are specified here so the expected integers exist before any code does.

1. Matching rates: `sourceFrame = sourceStart + (S - recordStart)` for every `S` in the record range.
2. The mixed-rate tie in section 3.1 selects source frame 500, the earlier frame.
3. A still: every sequence frame in the record range selects the one stored source frame.
4. Drop-frame formatting of one index, plus the parse of that label back to the same index.
5. `sampleIndex` for 24/1, 24000/1001, and 30000/1001 at 48000, including the 1601/1602 alternation.
6. `pixelStart` across a span of at least 172800 frames at a scale below one pixel per frame, checked at the first frame, a middle frame, and the last frame, so a drift would fail.
7. A marked span `[48, 72)` has duration 24, and frame 72 is outside it.
8. A conversion whose product does not fit the integer width fails. A zero denominator fails at the ratio type, as the roadmap already requires.
