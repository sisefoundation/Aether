# Next development slice proposal

**SLICE 0A COMPLETE — DEVELOPMENT FROZEN PENDING NEXT AUTHORISATION.**

Accepted on the tested Fedora machine only. Not cross-platform, not production-ready, not a complete NLE, not codec, playback, interchange, or external-NLE validated, and not a power-loss proof. Slice 0B has not started.

This document is a proposal. It does not authorise implementation. It did not accept ADR-002, ADR-006, or ADR-007. A recommendation is not an approval.

On 2026-11-21 the project owner accepted ADR-002. That acceptance is the ADR, not this proposal. ADR-006 and ADR-007 stay **PROPOSED**. The body below is the pre-decision proposal and is not rewritten.

`docs/DOCUMENTATION_RECONCILIATION_SLICE_0A.md` is not in the tree. One later fact is recorded here and nowhere else in this pass: after the validation report, the sources were arranged as in `docs/ASWF_LAYOUT.md`, and the Debug executable path named there is `build/src/aether`. The acceptance evidence remains `docs/validation/SLICE_0A_PROJECT_CORE_VALIDATION.md`, which names `build/aether` for the run it recorded. This planning pass did not rebuild or retest.

## 1. Purpose

Propose one narrow next slice, after comparing five candidates, and separate that engineering recommendation from the owner decisions it still needs. No dates.

## 2. Documents and source inspected

Read: `README.md`, `DEVELOPMENT_ROADMAP.md`, `docs/19_DEVELOPMENT_ROADMAP.md`, `docs/FOUNDATION_PROTOTYPE_READINESS_GATE.md`, `docs/PREBUILD_DECISION_PACKAGE.md`, `docs/PREBUILD_DECISION_CHANGELOG.md`, `docs/SLICE_0A_TECHNICAL_DEBT.md`, `docs/validation/SLICE_0A_PROJECT_CORE_VALIDATION.md`, `docs/validation/SLICE_0A_IMPLEMENTATION_PLAN.md`, and ADR-001 through ADR-009. ADR-002, ADR-006, and ADR-007 were read from those files.

The source skim, not a new test, shows Slice 0A built:

- `include/aether/core/timing/` and `src/core/timing/`: `sampleBoundary` and `sampleCount` only. The header says ADR-002 is not accepted.
- `include/aether/core/project/`, `core/bin/`, `core/media/`: ids, errors, bin records, Media ID, representation roles, master clip records.
- `src/persistence/`: SQLite, manifest, log. `project_store.cpp` creates eight tables: `schema_info`, `project_meta`, `bins`, `editorial_objects`, `bin_membership`, `media_assets`, `media_representations`, `master_clips`. No sequence table. No FFmpeg include in `src/` or `include/`.
- `include/aether/app/` and `src/app/`: a session façade. Its header says ADR-006 is not accepted.
- `include/aether/ui/` and `src/ui/`: one Qt window.
- `tests/`: Qt Test for timing and the project session.

No media file writer, no decoder, no timeline, and no relink or conform program is in that tree.

## 3. Established Slice 0A facts

These are the measured facts. Nothing in this proposal adds a capability.

- Fedora. `qt6-qtbase-devel` 6.11.2 and `sqlite-devel` 3.53.4. 14 packages installed, 0 removed. The other packages are DNF requirements, not AETHER runtime dependencies. No FFmpeg package. No system upgrade.
- CMake Debug passed. The build passed, exit 0. Qt and SQLite are linked. Eighteen developer warnings name deprecated `SQLite::SQLite3` and the replacement `SQLite3::SQLite3`. That warning is debt. It is not fixed.
- C++17 is prototype-only. ADR-001 does not set a standard.
- Default Wayland: the window was not confirmed. `QT_QPA_PLATFORM=xcb`: the window was titled AETHER and the process exited 0 on close. Create, Open, Close, and the bin list were not clicked.
- CTest: 1 discovered, 1 passed. Inside it, TimingTests 7 and ProjectTests 11: 18 Qt Test cases, 0 failed. That is not 18 CTest tests.
- Timing uses the floor rule. `[0, 8)` at 30000/1001 and 48000 Hz is 12812. `[8, 16)` is 12813. The 200000-frame walk is 320320000. Negative positions and overflow fail closed. Adjacent spans that differ are expected.
- FFmpeg development headers are missing. The link harness did not configure. `libavfilter` is not approved. GStreamer is not a dependency.
- The 29.97 MXF discrepancy remains a failure: eight frames in the file carried 12813 samples against the contract's 12812 from frame 0. The library result 12813 for `[8, 16)` does not reclassify that failure. OP-Atom stereo remains unsupported on the tested build. External relink and conform remain untested.
- The slice is not a power-loss proof. `BACKUP/` is an empty directory in the create path. Undo does not survive close.

## 4. What stays frozen

Slice 0B has not started. This proposal does not start it. The Foundation Prototype in `docs/FIRST_IMPLEMENTATION_MILESTONE.md` is not complete. Both roadmaps still mark every later phase **NOT STARTED** and **NOT GRANTED**.

ADR-001 is **ACCEPTED WITH CONDITIONS**. ADR-003, ADR-004, and ADR-005 are **ACCEPTED**. ADR-009 is **ACCEPTED FOR FOUNDATION PROTOTYPE**. ADR-008 is **OPEN — PENDING LEGAL COUNSEL**. ADR-002, ADR-006, and ADR-007 stay **PROPOSED**.

The gate, the decision package, both roadmaps, and the debt list still say in places that Apache-2.0 is only a proposal, and some of those pages still say the root `CMakeLists.txt` is empty or that the executable is `build/aether`. `README.md`, `LICENSE`, `NOTICE`, and the 21 November 2026 section of ADR-008 record a later source-licence direction and the layout move. This proposal does not edit those files and does not treat the source licence as a clearance of Qt, FFmpeg, codecs, AAF, or trademarks.

## 5. ADR-002 — rational time as the stored authority

Status stays **PROPOSED**. This review is not acceptance.

| Review | Finding |
| --- | --- |
| Problem | A cut reopened later must land on the same frame. Float seconds drift. Drop-frame timecode is a display count, not the stored position. |
| Existing spec | The ADR proposes reduced integer ratios, `int64` frame indexes, and audio sample positions only as `sampleIndex` derived from those frames. Subframe slip is outside the first model. Timecode is a label. Seconds on screen are not written back. Mixed-rate mapping is data on the clip: `nearest`, `floor`, or `hold`. The time documents already state the rule and still await review. Slice 0A implemented the floor span function and did not store it. |
| Alternatives | Float seconds as the authority. The ADR says every time document rejects that, and it is not proposed. Subframe audio slip in the first model. The ADR leaves that **OPEN** as later work, not part of this decision. |
| Missing information | No owner confirmation. The mixed-rate policy is named and not exercised by Slice 0A, because there is no sequence. Whether a sequence start frame is a second sample origin is already recorded as not adopted; it is not a new gap. |
| Dependents | Any sequence or timeline table, persisted undo of timeline operations, AVX JSON time fields, render requests, and the Foundation Prototype's sequence of instances. ADR-003 allows the one database to hold sequence and timeline data. It does not choose the time representation. |
| Risk of a wrong decision | Storing float seconds, or treating the muxer's 12813-at-frame-8 result as the contract, would contradict the floor rule and the unchanged MXF failure. Accepting subframe slip inside the first model would widen the schema before the first model exists. Leaving the ADR proposed and still creating sequence tables would invent the decision in code. |
| Small experiment | Not required before a decision. Slice 0A already ran the integer function, including 12812, 12813, and 320320000. Another experiment would not answer the owner question. |
| Owner approval | Required. Repetition in the specs is not acceptance. The ADR says confirmation is none. |
| Recommendation | Engineering recommends that the owner answer the questions in section 16 before any sequence schema is written. The recommendation is to accept the proposed integer rule and to keep subframe slip out of the first model. That sentence is not an approval. Until the owner answers, the status stays **PROPOSED** and no sequence table should be added. |
| Roadmap consequences | If the owner accepts it, a later authorised slice could store integer frames and derived sample positions. If the owner rejects it or leaves it proposed, Phases that need a sequence stay **NOT STARTED**. Acceptance would not authorise playback, FFmpeg, or a timeline editor. |

## 6. ADR-006 — the application is the only joiner

Status stays **PROPOSED**. This review is not acceptance.

| Review | Finding |
| --- | --- |
| Problem | If the timeline calls a decoder, a codec change changes trim code. If media management calls the editorial command API, a relink can move a cut. If a plugin receives the database path, it bypasses both rules. |
| Existing spec | Editorial would emit a composition slice with Media IDs and no paths. Media management would emit locations and would not delete or carry sequence structure. The application would build a picture request and an audio graph request. Processing and AETHER SOUND would not call editorial commands and would not write `project.db`. Interchange would return documents and a loss report, not update live rows in place. The build shape of sound, `aether_sound` versus a backend inside Processing, is **OPEN** inside the ADR. The data split is the proposal either way. The early architecture's placement of device audio inside Processing is part of this ADR and is not accepted. |
| Alternatives | Editorial opens files for the duration of a command. Not proposed. GStreamer is the authority for which frame is current. Not proposed. The presented frame would be the frame index on the request. |
| Missing information | No owner confirmation. The sound build shape is explicitly open. Slice 0A's `ProjectSession` is a window façade. It is not this joiner. |
| Dependents | Playback, an export oracle, audio sync, a Qt shell that joins processing, and any effect path that enters the joiner. `docs/ENGINEERING_INTERFACES.md` treats the forbidden edges as review rules. |
| Risk of a wrong decision | A timeline that links FFmpeg, or a relink that edits the cut, collapses the boundaries ADR-001 and ADR-005 already drew. Choosing GStreamer as the clock would contradict ADR-001. Deciding `aether_sound` now would answer a question the ADR itself leaves open. |
| Small experiment | Not required to decide the data split. The ADR already describes a later unit test that resolves a cut with every asset offline and no codec linked, and a processing test with no timeline object. Those tests belong to an authorised implementation, not to this decision. |
| Owner approval | Required. The ADR's confirmation is none. |
| Recommendation | Engineering recommends leaving the status **PROPOSED** until the owner answers section 16. If the owner later accepts the data split, the sound build shape should stay **OPEN** rather than be implied. Accepting the data split would not authorise playback or a codec link. |
| Roadmap consequences | Root phases 6 through 11 and the seven-phase playback, effects, and exchange stages stay blocked for implementation. Slice 0A does not become the joiner by a later sentence. |

## 7. ADR-007 — relink, conform, and exchange are different results

Status stays **PROPOSED**. This review is not acceptance.

| Review | Finding |
| --- | --- |
| Problem | A cutting room uses relink, online, conform, and export for different events. Collapsing them makes a path update look like a verified picture match, and makes a rendered file look like the database. |
| Existing spec | Relink may change location rows for a known Media ID after classification and validation. It does not change instance ids, record ranges, or Media IDs, and its result is not a conform state. Conform reads a pinned sequence revision and the registry, compares structure, identity, time, and effects, and writes a report. `verified` is allowed only when the critical checks passed. Export does not set that state. Exchange writes a translation plus a loss report. The native project stays the source of truth. No adapter is a verified interchange with another vendor until a test against that application is recorded. None is recorded. The conform and relink specifications already separate the words. |
| Alternatives | A successful export preset counts as conform. Not proposed. Filename match counts as relink. Not proposed. `docs/MEDIA_STORAGE_AND_RELINKING.md` says filename-only match is insufficient. |
| Missing information | No owner confirmation. No conform report has been generated by a program in this repository. External NLE relink was not tested. There is no sequence revision to pin, because ADR-002 is proposed and no sequence table exists. |
| Dependents | Location and relink work, conform reports, AVX, AAF, OTIO, FCP XML, FCPXML, and EDL. ADR-005 already says relink must not replace a Media ID. ADR-004 already says `.abin` is interchange, not a second database. Those accepted rules do not accept this ADR. |
| Risk of a wrong decision | A path column update recorded as `verified`, or an EDL treated as the only save of the cut, would undo the distinction. A prior FFmpeg mux is not a conform and is not a vendor test. The ADR says that. |
| Small experiment | Not required to keep the three results distinct on paper. A program that writes `verified` would need a later fixture test, a sequence model, and this ADR accepted. That experiment is not the next slice. |
| Owner approval | Required. |
| Recommendation | Engineering recommends that the owner keep the three results distinct if the ADR is later accepted, and that no code write a conform state or an exchange file until then. The recommendation is not an approval. Status stays **PROPOSED**. |
| Roadmap consequences | Relink, conform, and exchange phases stay **NOT STARTED**. Accepting the distinction would still not authorise an external NLE claim. |

## 8. Candidate A — architecture-decision resolution

Resolve ADR-002, ADR-006, and ADR-007 on paper. No schema, no FFmpeg, no new editorial command.

This is the only candidate that answers the decisions the next editorial code would otherwise invent. It does not, by itself, build the Foundation Prototype. Doing all three in one sitting is wider than one narrow slice. ADR-002 is the one that blocks the next stored model. ADR-006 and ADR-007 block later joins and later file results. They can be answered separately.

## 9. Candidate B — project-core hardening

Exercise or tidy what Slice 0A already built: the unclicked window controls, a recorded Wayland result if a later run can see one, the deprecated SQLite target name, and the gap between autosave and a power-loss claim.

This adds no timeline, no FFmpeg, and no new editorial behaviour if the scope is held to the existing commands. The risk is that a green retest gets described as production validation, a native Wayland pass, or a power-loss proof. The SQLite warning did not fail the build. Renaming the target is optional maintenance, not a milestone. Hardening does not unblock a sequence.

## 10. Candidate C — FFmpeg library feasibility

ADR-001 already names the later link set: `libavutil`, `libavcodec`, `libavformat`, `libswscale`, and `libswresample`, inside a Media Engine, dynamically linked. Domain code must not see FFmpeg types. `libavfilter` is not approved. GStreamer is not a dependency. The Fedora FFmpeg 9.0.2 GPL build is not the distribution pin. Development headers are missing. The harness configure is **BLOCKED**. No pin exists. The version and ABI policy is written and not applied.

This proposal does not install, build, or link FFmpeg.

Approval that would be required before any such work, each on its own:

- An owner authorisation to install development files or to build a separate pinned FFmpeg, naming which one. Installing `ffmpeg-devel` for the Fedora package would not adopt that package as the pin.
- A recorded pin: version, shared libraries, headers, GPL and non-free components excluded unless counsel has approved them. ADR-008 has not closed that legal row.
- A decision that the harness stays outside the product `CMakeLists.txt` until that pin configures, links, and runs.
- An explicit statement that a successful link does not reclassify the 29.97 MXF discrepancy, does not make OP-Atom stereo supported, and does not accept ADR-006.

Without those approvals, candidate C stays blocked. A link would also not create a sequence or a relink result.

## 11. Candidate D — application-foundation debt

Classified only. Nothing here is a fix, and this proposal does not authorise one.

| Class | Item | Why that class |
| --- | --- | --- |
| Critical | Do not add a sequence or timeline schema while ADR-002 is **PROPOSED** | The time authority would be invented in code |
| Critical | Do not call the XCB window a native Wayland pass or a functional UI test | The default Wayland window was not confirmed. The buttons were not clicked |
| Critical | Do not call autosave or the interrupted-transaction test a power-loss proof | `BACKUP/` has no snapshot writer |
| Critical | Do not reclassify the 29.97 MXF failure because `[8, 16)` is 12813 | Those are different measurements |
| Important | Window controls untested | The session tests passed. The clicked window did not |
| Important | Native Wayland unconfirmed | One launch path was observed. The session default was not |
| Important | FFmpeg headers missing, library integration blocked, version and ABI policy unapplied | ADR-001 conditions 3 and 4 |
| Important | OP-Atom stereo unsupported, external relink and conform untested | Open media and interchange claims |
| Deferrable | Rename `SQLite::SQLite3` to `SQLite3::SQLite3` | The Debug build passed with the warning |
| Deferrable | `project.manifest.json` and empty `BINS/` | Prototype names. They do not amend the folder specification |
| Deferrable | Transitive DNF packages and the Qt Test link on the test binary | Already recorded. They are not a new database and not a codec |
| Deferrable | Choosing a permanent C++ standard | C++17 stays prototype-only until an ADR sets one. Switching it is not a defect fix |

## 12. Candidate E — media-model and ingest preparation

What can be designed without FFmpeg, because ADR-005 is accepted and Slice 0A already stores it: one Media ID, separate Representation IDs, roles original, proxy, transcode, and consolidated, a path that is not the identity, and no media file created. A paper ingest state can say "candidate match" versus "confirmed same asset" without opening a file. Offline can stay a location state.

What needs FFmpeg, or at least a probe this tree does not have: duration, codec, container, a sample-count check of a real file, proxy generation, MXF, OP-Atom stereo, and any playback selection that decodes. Those stay out until candidate C is authorised and the MXF discrepancy is still treated as a failure.

Relink that writes a location, conform that writes `verified`, and exchange that writes a file need ADR-007. Designing those writers now would start the behaviour this proposal excludes.

## 13. Comparison

No candidate is chosen by the order of the list.

| Candidate | Unblocks | Requires new product code | Main block |
| --- | --- | --- | --- |
| A. Decision resolution | The decision a sequence schema would otherwise invent | No, if it stays on paper | Owner answers. All three at once is wider than one question |
| B. Project-core hardening | Test gaps on the existing window and an optional CMake rename | Only if a later authorisation says so | Does not answer ADR-002. Easy to over-claim |
| C. FFmpeg feasibility | ADR-001 conditions 3 and 4, and only after a pin | Yes, a harness at minimum | Headers missing, legal review open, MXF failure unchanged |
| D. Debt classification | A priority list | No | This section is the classification. It fixes nothing |
| E. Media-model preparation | A clearer ingest boundary | No, if it stays a design note | File ingest and relink are not ready |

A, D, and the design-only part of E can be written without code. B and C are implementation the moment they change the tree or the machine. C is the one that must not start from this document.

## 14. Recommended slice

Engineering recommends one narrow slice. The recommendation is not a selection by the owner and is not authorisation.

| Field | Proposal |
| --- | --- |
| Id | `P-ADR-002` |
| Name | Rational-time decision review |
| Objective | Put ADR-002 in front of the owner as a yes, a no, or a decision to leave it **PROPOSED**, using the Slice 0A timing evidence and the existing time documents. Do not write a schema. |
| Prerequisites | Slice 0A accepted. The floor results 12812, 12813, and 320320000 stand. The MXF discrepancy stays a failure. ADR-003 and ADR-005 stay accepted and are not reopened. |
| Scope | A written owner answer to the ADR-002 questions in section 16. If the answer is to leave it proposed, record that and stop. |
| Exclusions | No Slice 0B. No sequence or timeline table. No playback. No FFmpeg install, build, or link. No change to timing arithmetic. No media-identity change. No database-semantic change. No relink, conform, or exchange. No acceptance of ADR-006 or ADR-007 by implication. No C++ standard change. No clicked-UI or Wayland claim. No power-loss claim. |
| Modules | None. No new module and no edit to `src/`, `include/`, `tests/`, or CMake under this proposal. |
| Dependencies | The published floor rule and ADR-002's own text. Not FFmpeg. Not Qt beyond what Slice 0A already built. |
| Tests | No new test. The existing TimingTests and the independent driver remain the evidence. They are not re-run by this proposal. |
| Acceptance | The owner has answered the ADR-002 questions in section 16 in writing. The ADR file is changed only in a later pass the owner authorises. Until that pass, the status in the file stays **PROPOSED**. |
| Risks | An answer that accepts float seconds, or that treats the muxer failure as the contract, would contradict the measured floor rule. An answer that is read as permission to add sequence tables in the same breath would skip a separate implementation authorisation. |
| Exit conditions | Either a recorded owner decision on ADR-002 alone, or a recorded decision to leave it **PROPOSED**. The next feature slice is still unauthorised either way. Development stays frozen until a separate authorisation. |

Why this slice, and not the others: the next editorial model in both roadmaps needs a stored time authority, and Slice 0A deliberately did not invent one. ADR-006 and ADR-007 block later work and do not have to be decided to stop a sequence schema from appearing. Hardening and a CMake rename do not answer that. FFmpeg feasibility is blocked by missing headers, an open legal review, and the MXF failure, and this proposal forbids starting it. Ingest of real files needs that link. A paper review of one ADR is the narrow step that does not pretend the prototype has moved.

## 15. Engineering recommendations and owner decisions

Engineering recommendations, which the owner may reject:

- Do not implement a sequence, a joiner, a relink, or an FFmpeg link under this proposal.
- Prefer an owner answer on ADR-002 before any of candidates B, C, or E becomes code.
- If ADR-002 is accepted later, keep subframe slip out of the first model and keep the floor rule, including a later span of 12813 that is not the origin span.
- If ADR-006 is discussed later, leave the sound build shape **OPEN**.
- If ADR-007 is discussed later, do not treat export, a path update, or a previous mux as conform.
- Do not describe Slice 0A as more than the validation report shows.

Owner decisions, which this document does not make:

- Whether ADR-002, ADR-006, or ADR-007 changes status.
- Whether any implementation, package install, or FFmpeg pin is authorised.
- Whether C++17 becomes a permanent standard.
- Whether counsel's open rows are answered.

## 16. Approval questions

Each question is independent. Answering one does not answer another. None is approved by this file.

1. Does the owner accept ADR-002's proposed rule: reduced integer ratios, `int64` frame indexes, audio samples only derived by `sampleIndex`, timecode as a label, and seconds not written back as the position?
2. If question 1 is yes, does mixed-rate mapping stay data on the clip, limited to `nearest`, `floor`, and `hold`?
3. Does subframe audio slip stay out of the first model?
4. Does the owner accept ADR-006's data split while the choice between `aether_sound` and a Processing backend stays **OPEN**?
5. Does the owner accept ADR-007's separation of relink, conform, and exchange, with no vendor test claimed?
6. Does the owner authorise any install, build, or link of FFmpeg, including `ffmpeg-devel`?
7. Does the owner authorise a separate pinned FFmpeg, distinct from the Fedora 9.0.2 GPL build, for a later feasibility check?
8. Does the owner authorise a non-feature retest of the existing Create, Open, Close, and bin-list controls?
9. Does the owner require a confirmed native Wayland window before any further UI work?
10. Does the owner authorise renaming the CMake target `SQLite::SQLite3` to `SQLite3::SQLite3`?
11. Does the owner authorise any implementation slice after this proposal?

The engineering recommendation on questions 6 and 11, for this document, is no. That recommendation is not the owner's answer.

## 17. Close

`P-ADR-002` is the recommended next slice. It is a decision review. It is not Slice 0B, not the Foundation Prototype, and not permission to edit the ADR, the schema, or the build. Candidates B, C, D, and E are compared above and are not started. ADR-002, ADR-006, and ADR-007 remain **PROPOSED**.

PROPOSED — NOT AUTHORISED FOR IMPLEMENTATION.
