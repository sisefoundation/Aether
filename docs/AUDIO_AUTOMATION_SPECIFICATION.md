# AETHER SOUND — Automation Specification

Status: proposal, awaiting review. No implementation is authorized by this document.

This document specifies fades, level nodes, and automation passes. The philosophy of the three layers is [AUDIO_EDITING_PHILOSOPHY.md](AUDIO_EDITING_PHILOSOPHY.md). Placement in the engines is [AUDIO_ENGINE_ARCHITECTURE.md](AUDIO_ENGINE_ARCHITECTURE.md). How a gesture becomes one of these commands is [AUDIO_INTERACTION_MODEL.md](AUDIO_INTERACTION_MODEL.md).

Stored positions are sequence frames. Stored levels are millibels. Processing evaluates the curve at each output sample. The view evaluates it at each visible frame from the snapshot. Both evaluations use the formulas below. Neither stores a floating-point time.

## 1. Units

| Quantity | Stored as | Display |
| --- | --- | --- |
| Gain, fader, node level | `int32` millibels. 0 is unity. 100 millibels is 1.00 dB. | `millibels / 100` with a sign and `dB`. |
| Silence floor | −9600 millibels, exactly. A command that would store a lower value stores −9600. | `−96.0 dB`, and Processing uses linear amplitude 0. |
| Maximum boost | +2400 millibels. A command above that fails unchanged. | `+24.0 dB`. |
| Pan or balance | `int16` from −1000 to +1000. 0 is centre. | A signed number, or L/R words when the absolute value is 1000. |
| Fade duration | Sequence frames, `int64`, zero or greater. Zero clears the fade. | Frames, and timecode when the transport already shows duration. |
| Node frame | Sequence frame index inside the owning range. | The ruler's timecode. |

Processing converts millibels `m` to a linear amplitude by `10^(m / 2000)` when `m > −9600`, and by 0 when `m = −9600`. That conversion is the amplitude law `20 log10`. It runs in Processing at mix time and in the Application when it prepares a frame-resolution envelope for the painter. The document stores `m`.

Pan converts in Processing only. For a stereo balance, let `a = (pan + 1000) / 2000 * π / 2`. The left amplitude is `cos(a)` and the right amplitude is `sin(a)`. Centre is equal power on both channels. A mono pan uses the same pair of amplitudes into a stereo track or a stereo bus. Layouts wider than stereo use a documented pairwise law: the stored pan moves the balance between the left group and the right group of that layout, and the centre channel of a 5.1 layout stays at unity until a later command set gives it its own node lane. That limit is a review item.

## 2. What each layer automates

| Lane | Static value | Nodes | Recorded by a pass |
| --- | --- | --- | --- |
| Clip level | Clip gain | Yes, on the clip's record range | Drawn and typed. A pass does not write clip nodes in the first model. |
| Clip pan | Clip pan | Yes, on the clip's record range | Drawn and typed. |
| Track level | Track fader | Yes, on the sequence, from the start frame through the sequence duration | Yes, when the mode is Write, Touch, or Latch. |
| Track pan | Track pan | Yes, same span as track level | Yes, same modes. |
| Bus level | Bus fader | Yes | Yes. |
| Master level | Master gain | Yes | Yes. |

A lane with no nodes means the static value holds for the whole span. Nodes do not erase the static value. The heard level is the static value plus the node value, in millibels. A flat line the editor has never touched is an empty node list, not a stored node at every frame. The first node the editor adds is inserted at the interpolated value, which is 0 millibels of automation plus the static value on the line the painter shows. The stored node stores the automation component. The painter adds the static value when it places the line.

Parameter automation inside an effect uses the same node record, keyed by effect slot and parameter id. It is drawn in the inspector for that slot, and it may be overlaid on the lane only when that parameter is the selected overlay. The first model requires the data. It requires the lane overlay for clip level, clip pan, track level, and track pan.

## 3. Node record

A node is:

| Field | Meaning |
| --- | --- |
| id | Opaque id. Stable across a move. Removed by a delete. Restored by undo. |
| frame | Sequence frame. |
| value | Millibels for a level lane, or a pan integer for a pan lane. |
| toNext | `linear` or `hold`. The segment from this node to the next uses this law. |

One node per frame per lane. A command that places a second node on an occupied frame replaces the value and `toNext`, keeps the existing id if the command is a move onto that frame, and drops the displaced node. The inverse restores both.

Order is by frame. The segment before the first node holds the first node's value. The segment after the last node holds the last node's value. `hold` keeps this node's value until the next node's frame. `linear` is specified in section 5.

Clip nodes outside the clip's record range are rejected. Track, bus, and master nodes outside `[start, start + duration)` are rejected. A node on a gap is legal on a track lane, because the track lane covers the sequence, and illegal on a clip lane, because a gap has no clip.

## 4. Fade and crossfade curves

A fade-in or fade-out stores a duration in frames and a curve id: `linear`, `equalPower`, or `exponential`. A crossfade is an audio transition. It stores one curve id and a record range lying on the cut, the same invariant transitions already have. The outgoing clip follows the fade-out law. The incoming clip follows the fade-in law. One id governs both sides.

Let the fade occupy output samples `[s0, s1)`, with `s1 > s0`, where `s0` and `s1` are `sampleIndex` of the fade's sequence-frame edges. At output sample `s` in that half-open span:

```
u = (s - s0) / (s1 - s0)
```

`u` is a rational. The fade amplitude is:

| Curve | Fade in | Fade out |
| --- | --- | --- |
| `linear` | `u` | `1 − u` |
| `equalPower` | `sin(π/2 · u)` | `cos(π/2 · u)` |
| `exponential` | `u²` | `(1 − u)²` |

Equal Power is the constant-power pair: the squares of the two crossfade amplitudes sum to 1. Exponential is AETHER's slow-start law, a square on the linear parameter. It is fully specified here so an implementation has nothing to infer from another product. A fade of zero frames is stored as an absent fade. A fade longer than the clip fails unchanged. A crossfade whose range would leave either clip without the source handles the transition needs fails unchanged, which is the transition rule the timeline engine already recommends.

Fades multiply amplitude after the clip gain and after clip level automation. They are not nodes. A node inside a fade is legal. The fade still multiplies.

## 5. Evaluating a level lane at a sample

For a linear segment from node `(Fa, va)` to node `(Fb, vb)`, with `Fb > Fa`, at output sample `s` where `sampleIndex(Fa) ≤ s < sampleIndex(Fb)`:

```
span = sampleIndex(Fb) - sampleIndex(Fa)
m = va + (vb - va) * (s - sampleIndex(Fa)) / span
```

The division above was written to match a timebase tie toward the greater index. ADR-002 does not use that tie. Picture selection and `sample_boundary` are the accepted rules. This gain interpolation is not either of those rules. Its greater-index step stays **OPEN** and does not define the editorial clock. `hold` uses `m = va` on the whole span.

The heard millibel value is the static value plus `m`, then clamped to the silence floor and the maximum boost at the end of the sum. Clamping is evaluation. It does not write back into the node.

The specified value exists at every sample. A device buffer may interpolate between the values at its first and last sample if every sample in that buffer stays within 1 millibel of the formula. That bound is the only permitted shortcut.

Pan lanes interpolate the integer pan with the same nearest rule and then apply the pan law. They do not interpolate in decibels.

## 6. Automation modes

The mode is session state on the track, the bus, or the master. The default is Read. It is not a field of the timeline document. A saved project reopens in Read.

| Mode | While playing | While the editor moves the control |
| --- | --- | --- |
| Off | The static fader is used. Nodes are kept and ignored. | The static fader changes. Nodes do not. |
| Read | Nodes are followed. | The gesture is refused, and the status line says the lane is in Read. The editor switches to Write, Touch, or Latch to record, or edits nodes while stopped. |
| Write | The pass replaces nodes from the pass start through the playhead. | Every playhead advance during the pass writes or replaces the node at the current frame when the value has changed by at least 10 millibels, or by 10 pan units. |
| Touch | Nodes are followed until the control is held. | While held, write as in Write. On release, ramp from the released value back to the previously stored envelope across the samples of one sequence frame, then stop writing. |
| Latch | Nodes are followed until the control is held. | While held, write as in Write. After release, keep writing the last value until the pass stops. |

A pass starts when playback starts in Write, or when the control is first held in Touch or Latch during playback. A pass stops when playback stops, when the mode changes, or when a structural command arrives. The committed pass is one command, `setAutomationNodes`, covering the half-open frame span that was written, with the inverse equal to the nodes that span held before the pass. Samples are not undo steps.

Thinning is the 10-millibel deadband above, plus a final node at the stop frame. The document does not store one node per sample.

## 7. Copy and paste

The session clipboard can hold a run of nodes, copied from one lane, stored as frame offsets from the first copied frame together with values and `toNext`. Paste targets one lane and a destination frame, which is the playhead unless the command names another frame.

- Each node's frame is the destination frame plus its offset.
- A node that lands outside the lane's legal span is dropped.
- A node that lands on an occupied frame replaces that frame.
- If every node would be dropped, the command fails and the clipboard is kept.
- One undo step restores the replaced span.

Paste does not scale time to the destination clip's length. A second command can do that later. The first paste is a frame-faithful copy, which is what a repeated footstep fade or a repeated dialogue dip needs.

Copy of nodes does not copy the underlying media. Copy of a clip remains the editorial clipboard the timeline engine already describes, and that clipboard includes the clip's gain, fades, nodes, map, and effect parameters so a pasted clip sounds like the one that was copied.

## 8. Multi-node edit

A selection is a set of node ids on one lane. Move-vertical adds one millibel delta, or one pan delta, to every selected node. Move-horizontal adds one frame delta to every selected node. The horizontal move fails unchanged if any selected node would leave the span or land on a node that is not in the selection. The editor can deselect the blocker and try again. Delete removes the set in one step.

Selection does not cross lanes. Two clips nudged together use `adjustClipGain` on their static gains, which is a different command and does not require node selection.

## 9. Worked evaluation

Sequence at 24/1. Mix rate 48000. `sampleIndex(F) = F * 2000`.

A clip with static gain −300 millibels (−3.0 dB) has two level nodes: frame 10 at 0, and frame 20 at −600, with `linear`. At the first sample of frame 15, `s = 30000`, `span = 20000`, and the node component is −300 millibels. The heard value is −600 millibels, −6.0 dB, until a fade multiplies.

A fade-in of 2 frames on that clip occupies samples `[sampleIndex(recordStart), sampleIndex(recordStart + 2))`, 4000 samples. At the sample halfway through, `u = 1/2`. Linear amplitude is 1/2. Equal Power amplitude is `sin(π/4)`. Exponential amplitude is 1/4. The order in [AUDIO_PROCESSING_GRAPH.md](AUDIO_PROCESSING_GRAPH.md) applies the automated clip gain first and multiplies the fade amplitude after that. The fade does not add a second millibel term.

## 10. Invariants

- A node frame is an integer sequence frame. Undo restores values and ids.
- An empty node list means "static value only", on every reopen.
- Curve ids outside the three named ids are rejected on load of a document this version understands, and named in the error.
- Evaluation does not write the document.
- Read mode never writes nodes as a side effect of playback.
