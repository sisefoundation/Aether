# First implementation milestone — Foundation Prototype

Status: scope recorded from the owner’s decisions on 2026-11-20. This milestone authorises no code. It does not start the prototype. The gate is `docs/FOUNDATION_PROTOTYPE_READINESS_GATE.md` and the word there is **CONDITIONALLY READY**. The owner authorises any start.

The prototype validates architecture. It is not a complete NLE.

`src/` and `tests/` contain no application. The root `CMakeLists.txt` is empty and stays the product scaffold. The file under `tools/validation/ffmpeg_link_smoke_test/` is a link harness, not this prototype.

## 1. Scope

Each item is a validation target. None has been implemented.

| # | Scope | What has to be true |
| --- | --- | --- |
| 1 | C++ / Qt 6 / CMake application foundation | A product target configures with CMake, links Qt only in the UI module, and does not put FFmpeg or SQLite types in editorial headers. This is the Qt feasibility condition in ADR-001. It is not a redistributable package while ADR-008 is open |
| 2 | One `project.db` and a manifest | ADR-003. The manifest is package metadata. It is not a second editorial authority |
| 3 | Create, save, close, reopen | The last commit reopens. A failed create does not publish a project id |
| 4 | Multiple bins in one project | Bins are rows in that database, not separate project files |
| 5 | Stable Media IDs | ADR-005. Rename and move do not mint an id |
| 6 | Representation records | `ORIGINAL`, `PROXY`, `TRANSCODE`, `CONSOLIDATED` as roles. Each has a Representation ID. Location is separate |
| 7 | FFmpeg behind an isolated Media Engine boundary | Approved libraries only, and only after `docs/validation/FFMPEG_LIBRARY_INTEGRATION_VALIDATION.md` is no longer **BLOCKED**. Domain code does not include FFmpeg headers. Until the harness runs, this item stays blocked and the rest of the editorial scope does not wait on it |
| 8 | Media inspection | Probe results are data on a representation. A UMID is not a Media ID. Blocked with item 7 |
| 9 | Master Clip creation | One master clip cites one Media ID |
| 10 | Basic proxy registration or generation | Registration of a representation can proceed without a new codec. Generation waits on a validated codec tuple. The 30000/1001 eight-frame writer is not that tuple |
| 11 | A Sequence and a basic Timeline model | Integer frames, half-open ranges, picture and audio tracks. `sampleIndex` is the audio boundary |
| 12 | Save and reopen editorial relationships | Composition identity survives reopen. Paths are not the identity |
| 13 | Media location changes and relinking | A successful relink writes a location only. It does not replace the Media ID |
| 14 | Basic timeline sample and frame timing | Integer tests of `sampleIndex`, including a non-zero span at 30000/1001. The installed muxer’s 12813-sample file remains a **Failed** writer check, not a change to 12812 |
| 15 | Session-only command undo and redo | ADR-009. Compound timeline edits undo as one step and do not corrupt database references |
| 16 | Autosave and basic recovery, independent of undo | A committed edit remains after close. An interrupted autosave leaves the last commit openable. The undo stack is not required after a crash |
| 17 | Basic AVX export and import | Only if the AVX schema and its acceptance criteria are stable enough to implement without inventing a second model. This milestone does not judge them stable. Until that judgement is written, item 17 is not in the first slice |

Earlier twenty-capability notes that still apply sit inside these rows: rational rates including 30000/1001, rejection of a zero denominator, drop-frame as a label, one writer lock, offline media that still returns the Media ID, and honest failure. They are not a second milestone.

## 2. Not prerequisites

- Full MXF OP-Atom stereo. It is **Not supported** on the installed build. It is not validated. Other prototype items do not wait for it.
- External NLE relink. **Not tested.** Internal architecture tests do not wait for it. It remains an interoperability blocker for any claim of professional conform readiness.
- A passing 29.97 MXF audio writer. The contract stays 12812 for `[0, 8)`. The measured file stays failed.
- Persistent undo.
- `libavfilter`.
- GStreamer as a dependency.
- A filled `LICENSE`.

## 3. Exclusions

The prototype does not include, and their absence is not a failure of this milestone:

- A complete NLE: multicam, advanced trimming, colour grading, effects rasterisation, or a plugin host.
- A claim that DaVinci Resolve, Avid Media Composer, or Adobe Premiere Pro can relink the prototype’s media.
- Shipping the Fedora FFmpeg 9.0.2 GPL build.
- Restart-undo.
- Network, accounts, and phone-home.
- Vendoring third-party source into `third_party/`.

## 4. Acceptance

The milestone is met only when the items that were actually in scope have validation records under `docs/17_TESTING_AND_VALIDATION.md`, on a build whose link line matches ADR-001, and when the conditions in ADR-001 that those items use have evidence. Meeting it is not a release, not a conform, and not a compatibility claim.

Do not start the blocked rows in section 1 by folding them into an editorial test. Do not mark the stack production-validated because a headless project file reopens.

## 5. Relationship to the gate

Isolated editorial work in items 1 through 6 and 9 through 16 can be authorised by the owner while items 7, 8, and codec generation stay blocked. Item 1 still has to respect the open Qt distribution question: a local feasibility target is not a package. The gate word remains **CONDITIONALLY READY**.
