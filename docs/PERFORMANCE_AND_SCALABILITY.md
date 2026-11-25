# Performance and scalability

Status: architecture preparation. No timing measurement was taken. Numbers in the existing performance documents are targets or arithmetic, not results. **CONFIRMED.**

Read:

- `docs/PERFORMANCE_REQUIREMENTS.md` — interactive budgets, the 172800-frame arithmetic example, job priority.
- `docs/TIMELINE_PERFORMANCE.md` — what a later large-fixture phase is asked to measure.
- `docs/AUDIO_PERFORMANCE.md` — callback constraints. No latency promise.
- `DEVELOPMENT_ROADMAP.md` phase 15 — measurements written down in that phase, not before.

This file does not add a frame-rate promise.

## 1. Contract

**PROPOSED.**

| Constraint | Rule |
| --- | --- |
| Reference workload | A two-hour feature timeline with picture, several audio tracks, and linked media or proxies. 172800 frames at 24/1 is arithmetic, not a benchmark |
| UI | Virtualise bins and the timeline. The visible window’s cost grows with the window, except operations defined to touch the whole tail |
| Threads | The audio callback does not take the database lock, does not allocate in the steady state, and does not read the disk. The UI thread does not wait on a proxy job to return an edit command |
| Jobs | Playback and scrub before visible thumbnails and waveforms, then ingest checksums, then full-file proxies |
| Cache | Deletable. Eviction never treats originals, managed media, or proxies as cache |
| Index | Interval lookup for “items on these tracks between frames A and B” |
| Non-targets | Real-time optical flow of 4K as a default. Unlimited plug-in channels. Network collaborative latency |

**OPEN.** The memory cap for decoded frames. The default device buffer inside 64–4096 samples. The sync tolerance. The reference machine. Phase 4 of `docs/19_DEVELOPMENT_ROADMAP.md` writes the tolerance down when that phase starts. Phase 7 writes the large-fixture numbers.

**PROPOSED** first performance fixture, from the requirements document, still not a result: 32 audio tracks and 8 picture tracks, 2000 picture cuts, scroll from start to end, and a ripple of the first cut with undo. Instrumentation of the audio callback, when it exists, expects zero allocations.

## 2. Data contract

A measurement record, when a run exists:

```text
machine_profile
fixture_id
action
samples
result            measured | not_run
```

Every action in the current documents has `result = not_run`. **CONFIRMED.**

## 3. Workflow

```mermaid
flowchart LR
  PLAY[Playback and scrub]
  VIS[Visible thumbs and waveforms]
  HASH[Ingest checksum]
  PROXY[Proxy and speculative cache]
  PLAY --> VIS --> HASH --> PROXY
```

A cancel is recognised at the next job boundary. The UI stays usable. A ripple that rewrites many rows stays off the audio thread and does not rebuild thumbnails as part of the command (`docs/PERFORMANCE_REQUIREMENTS.md`).

Failure: a missed budget, once a reference machine exists, is a bug against that table. Until the machine exists, the table is a specification. This preparation does not mark any budget met.

## 4. Failure cases

| Case | Result |
| --- | --- |
| Publishing a laptop timing as the product budget | Rejected. No laptop run was used |
| Full-file checksum on the UI thread during ingest | Rejected. The bin row can appear while the checksum is `pending` |
| Cache eviction deletes a proxy | Rejected |
| Timeline widget instantiates one widget per clip of a feature | Rejected as a design. The view asks for the visible range |

## 5. Acceptance criteria

- Foundation Prototype acceptance does not include a frame rate. It includes editorial commands that return without decoding.
- Phase 7 records measurement rows for the fixture it actually built, on a machine profile a person names, and leaves every unrun row as `not_run`.
- Scroll and ripple tests assert structure (only visible rows created, hash restored on undo) before they assert milliseconds.
- No sentence in a release note quotes a number this file does not have. It has no measured numbers.
