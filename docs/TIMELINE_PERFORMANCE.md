# AETHER Timeline Performance

Status: proposal, awaiting review. No implementation is authorized by this document.

This document states the performance constraints for a feature-length sequence. It does not promise a frame rate. Phase 15 of [DEVELOPMENT_ROADMAP.md](../DEVELOPMENT_ROADMAP.md) is where measurements are written down. The constraints here are what those measurements must be able to exercise. No code, cache format, or index library is introduced by this document.

The engines are placed in [TIMELINE_ENGINE_ARCHITECTURE.md](TIMELINE_ENGINE_ARCHITECTURE.md). Painting rules are in [TIMELINE_UI_SPECIFICATION.md](TIMELINE_UI_SPECIFICATION.md).

## 1. The sequence this has to survive

A feature-length sequence at 24/1 is 172800 frames for two hours, and longer for a film with a slower rate or a longer cut. A documentary stringout or an assistant's selects sequence can hold tens of thousands of items across many tracks. A music bed or an ambience stem can be one item that spans the whole duration. A short cutaway can be a handful of frames.

The interactive actions are: scroll, zoom, next cut, previous cut, hit-test a terminal, preview a one-frame nudge, join a trim, and paint the window. Save and autosave write the document on their own cadence. They are not part of the nudge.

The working set is the in-memory model. The package on disk is written by save and autosave. A gesture does not serialise the film. A gesture does not resolve every frame of the film. A gesture does not decode.

## 2. Virtualised painting

The timeline is one custom-painted widget. Clips are not widgets and are not scene-graph nodes allocated per item.

For each paint the renderer knows:

- the visible sequence range, expanded by a small prefetch of frames on either side
- the visible track rows, similarly expanded by a row or two
- the scale as a rational pixels-per-frame
- the snapshot generation it is painting

It asks the presentation snapshot for items that overlap that range on those tracks. The query returns items that start before the window and end inside or after it, so a two-hour music bed is present as one rectangle, not as a walk of the whole track.

Work to paint a frame of the UI is proportional to the number of visible items and visible tracks, plus the cost of filling thumbnail and waveform slots that already have pixels. It is not proportional to the sequence duration or to the total item count.

Overview painting uses a coarse summary: per track, the runs of occupied versus gap time inside the overview's pixel columns. That summary is cached on the snapshot generation and rebuilt when the generation changes. It is not rebuilt on scroll.

## 3. Visible-region queries

Editorial exposes `overlap(track, range)` and the cut-navigation queries named in the engine architecture. The performance requirements on those queries:

| Query | Bound |
| --- | --- |
| `overlap` for one track and one window | Logarithmic in the number of items on that track, plus linear in the number of hits. |
| Next or previous cut from a frame, one track | Logarithmic in the number of items on that track. |
| Next or previous cut across a set of tracks | Logarithmic per track in that set. |
| `syncDelta(track, frame)` | Logarithmic in the number of anchors on that track. |
| `usages(asset id)` | Logarithmic in the number of items in the project, plus linear in the number of hits. |

A structure that only sorts start frames is not enough, because an item that starts before the window must still be found. An interval index, or a start index plus a structure that accounts for long items, meets the bound. The choice inside Editorial is free if the bounds hold. The index is rebuilt or updated as part of the command that mutated the model, before `apply` returns, so a paint that follows a join never walks a stale order.

The resolver stays ranged. A scrub resolves the frame under the playhead, or a short prefetch the playback path already uses. It does not resolve 172800 frames to move the playhead by one.

## 4. Commands stay off the paint path

Mutation runs on the session thread, as the module rules require. The UI thread sends a command and paints. Preview of a nudge reads the model and builds a snapshot of the affected tracks' tails. It does not copy unrelated sequences in the project if a structural share is possible, and it does not need to: correctness is the requirement, and an implementation may copy the whole in-memory sequence provided the nudge stays interactive on the Phase 15 fixture. If a full copy is too slow, the fix is inside Editorial. It is not a cache of trim results in the widget.

A preview does not decode, does not touch SQLite, and does not build a `RenderRequest`. Join does not either. The composition hash changes on join, and playback drops its plan. That invalidation is a hash comparison, not a walk of the media.

Undo applies one inverse. It is the same class of work as the command it undoes.

## 5. Waveforms

Processing builds waveforms. The timeline never reads a media file to draw one.

The artifact is a pyramid of min and max sample pairs per channel, stored under the project's `cache/` as a regenerable derivative. The cache key includes the asset id, the channel count, the sample rate, and a content hash when the catalog has one. Deleting the cache changes no composition hash and no edit point.

Generation is asynchronous. The Application requests the spans whose slots are in the prefetch window, at the pyramid level the current scale needs. A request that is still queued when the slot leaves that window is cancelled. A running request may finish and land in the cache; the renderer simply does not ask for it again until the slot returns. The UI thread does not wait.

The first paint of a span uses whatever level is already in memory: a coarser level is acceptable for one paint, and the finer level replaces it when it arrives. The envelope is drawn from integer bins. It is not a reason to resample the editorial cut.

The stride of the pyramid and the prefetch length are open numbers, listed for review in the engine architecture. They are chosen so a two-hour stereo bed does not enter memory as raw samples.

## 6. Thumbnails

Processing extracts stills. The Application asks for the source frame a visible slot needs, which is the frame `resolve` names for that sequence frame, or the slot's left edge when the slot covers many frames. Requests are immutable, cancellable, and bounded by the visible slots plus the prefetch.

Stills are cached by asset id, source frame, and a hash of the colour configuration, as the architecture already says for decoded frames and thumbnails. A second zoom level may reuse a still that is already close to the slot's source frame. The renderer does not block the cut on a missing still. The label and the rectangle are enough to edit.

Thumbnail extraction for a scroll is scheduled after the snapshot paint, at a lower priority than a still the record viewer is waiting on for the playhead. Scrolling through a reel must not stall the playhead still behind a queue of off-screen extractions.

## 7. Navigation of a large sequence

Next cut and previous cut use the cut index. They do not scan items linearly from the playhead to the event. Typing a timecode parses to a frame and sets the playhead in constant time. Centring that frame is a scroll-origin assignment.

The sequence overview is the way across a long duration with the pointer. Its hit-test maps a pixel of the overview to a frame with the same integer rule as the lanes, using the overview's own pixels-per-frame, which is `duration / overviewWidth` reduced to a ratio. Dragging the overview updates the scroll origin and requests a new visible window. It does not build a picture of every frame.

Zoom about the playhead recomputes `pixelStart` for the visible range only. The renderer does not materialise a pixel coordinate for every frame in the film. `pixelStart(F)` is evaluated for the frames that bound visible items.

Snap collects candidate frames from the visible window plus the playhead, the marks, and the nearest cuts from the index. It does not gather every marker in the film on each mouse move. Markers outside the prefetch are reached by next-marker and previous-marker, which are index queries.

## 8. What stays responsive, and what is allowed to take longer

| Action | Constraint |
| --- | --- |
| Paint the current window | Proportional to visible items and ready pictures. |
| Hit-test a pixel to a frame and an item | Logarithmic in items on the visible tracks. |
| Nudge an open trim by one frame | Preview returns without disk and without decode. |
| Next cut, previous cut, next marker | Index query, without scanning the reel. |
| Join, insert, delete | One session-thread command. The tail shift is proportional to the number of items that actually move, not to the frame count. |
| Save | Writes the document. It is not on the mouse path. Autosave uses the recovery database the architecture already specifies. |
| First waveform of a new asset | Asynchronous. The lane is editable while it runs. |
| Resolve and present the playhead frame | The existing playback seam. It is not the timeline's paint. |

A command that shifts a large tail — delete near the head of a stringout with tens of thousands of later items — may cost more than a nudge. That cost belongs on the session thread as one transaction. The paint that follows still draws only the window. If Phase 15 shows that tail updates miss the interactive bar the review sets for that fixture, the index and the snapshot granularity change inside Editorial and the Application. They do not gain an edge to Processing, and they do not move trim math into the widget.

## 9. Memory

The in-memory model holds items, not pixels and not samples. Thumbnails and waveform levels in memory are bounded by the prefetch window plus a small cache of recently seen slots. Scrolling a long way drops slots that left the window. The cache on disk may keep them. The process does not retain a decoded frame per sequence frame.

A preview snapshot shares structure with the committed model or copies the affected tracks. It does not retain a history of every nudge. Abandoning the preview drops it. Undo retains inverses of committed commands only, and the Application may cap the session stack. A cap drops the oldest inverse. It does not drop the document.

## 10. Failure under load

A cancelled waveform or still is a normal result. The painter leaves the slot empty or coarse. A failed probe leaves the asset offline and the clip rectangle in place.

If the session thread is inside `apply`, further gestures queue. They do not run a second mutation beside the first. Paint may show the last committed snapshot until apply returns. The open preview's secondary hairlines stay only while that preview is still the current one.

None of these paths take a network. None of them treat a missing cache as a missing cut.

## 11. What Phase 15 is asked to measure

When a review reaches Phase 15, the fixture is a local project large enough to stand in for a feature: a sequence on the order of a feature duration, with a mix of long items and many short items, several video and audio tracks, and a catalog that can resolve offline. The exact counts are chosen in that phase so the fixture stays reviewable, as the roadmap already says.

The measurements to record are: time to paint a scroll step, time to preview a one-frame ripple, time to join a ripple near the head of the sequence, time to jump to the next cut, time to resolve one playhead frame, and the memory held by the timeline cache after scrolling the length of the sequence. Deleting `cache/` and reopening changes no composition hash. Those numbers are the phase's result. This document does not invent them in advance.
