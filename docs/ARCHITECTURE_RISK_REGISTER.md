# Architecture risk register

Status: architecture preparation. Risks are classified. They are not closed. No mitigation below was executed.

Likelihood and impact are engineering judgement for review, not measurements. **OPEN** means the risk is current. **PROPOSED** means a mitigation is written in a spec and not built.

| ID | Risk | Evidence | Label | Mitigation already written | Residual |
| --- | --- | --- | --- | --- | --- |
| R-A | A prototype mints a second media identity inside the timeline | Two id vocabularies, audit C3–C4 | **OPEN** | ADR-005. Milestone capabilities 7 and 10 | Subhajit has not chosen the glossary |
| R-B | Float seconds become the stored cut | Early and later time documents forbid it. Nothing enforces it in code | **OPEN** | ADR-002. Milestone capability 1 | Canonical hash bytes are still unchosen |
| R-C | Relink or export is shown as a verified conform | Documents separate them. UI does not exist, so the risk is future copy | **OPEN** | ADR-007 | Easy to regress in a dialog |
| R-D | Original media is modified | Safety specs. No test | **OPEN** | `docs/SECURITY_AND_FILE_SAFETY.md`. Milestone capability 18 | Decoder isolation is unresolved |
| R-E | Stack treated as approved | `ARCHITECTURE.md` says approved. Licence audit has not started. CMake is empty | **OPEN** | ADR-001 status Proposed | A later editor may quote the sentence and link libraries |
| R-F | Qt, FFmpeg, ProRes, DNx, or AAF redistributed without review | `docs/16_SECURITY_AND_LICENSING.md` | **OPEN**. Legal | ADR-008. Writers hidden until both gates pass | Counsel has not written |
| R-G | FFmpeg 9.0.2 notes treated as a validation pass | Media docs call them a study to reconcile. This audit did not re-run them | **OPEN** | Capability query stays closed for those writers | The study itself was not repeated, so the warning may be stale or still true. Either way it is not a pass |
| R-H | Vendor compatibility implied by naming Resolve, Media Composer, Premiere, or Pro Tools | Every exchange doc says no test | **OPEN** | `docs/EXCHANGE_COMPATIBILITY_MATRIX.md` | Marketing language outside the specs |
| R-I | Two schemas generate one database | `PROJECT_DATABASE_SCHEMA.md` and `BIN_DATABASE_SCHEMA.md` | **OPEN** | `docs/PROJECT_STORAGE_AND_DATABASE.md` forbids generating both | Filename `project.db` versus `project.sqlite` is also open |
| R-J | Undo design thrash after the first save format | Session stack versus `OperationHistory` | **OPEN** | ADR-009. Milestone requires session undo only | Restart-undo could force a migration |
| R-K | Playback clock becomes the playhead | The architecture forbids it. FFmpeg is the core framework. GStreamer is not the playback dependency | **OPEN** | `docs/10_PLAYBACK_AND_RENDERING.md`. Oracle before interactive play | No playback oracle has been run |
| R-L | Audio sync tolerance invented early | Roadmap says write it down at the phase. Audio docs repeat that | **OPEN** | Phase 4 of the seven-phase roadmap | No number is available. Inventing one would be a false specification |
| R-M | In-process plugins | Early text treats them as trusted | **OPEN** | Milestone excludes plugins. Isolation unresolved | A later ABI might ship in-process by default |
| R-N | Empty source tree mistaken for a module layout | Directories exist and do not match `aether_*` names | **CONFIRMED** empty | `docs/ENGINEERING_INTERFACES.md` mapping. Directories preserved, not filled | Someone may drop code into `src/ui` and link Qt in the first week |
| R-O | Empty `LICENSE` ships | File exists and is empty | **CONFIRMED** | Release criteria in `docs/18_BUILD_AND_RELEASE.md` | No git history to notice the change |
| R-P | Large-timeline cost | Budgets are unmeasured. 172800 is arithmetic | **OPEN** | `docs/PERFORMANCE_AND_SCALABILITY.md` | No machine profile |
| R-Q | Colour tags filled with Rec.709 by default | Identity spec forbids the invention | **OPEN** | `docs/12_COLOUR_MANAGEMENT.md` | Probe code often defaults tags |
| R-R | `.abin` becomes a SQLite dump | Option A was judged and not chosen | **PROPOSED** rejection | ADR-004. Milestone capability 19 | ZIP JSON is more work than one file copy |
| R-S | Multi-writer on a film server | One writer is the proposal | **OPEN** | Lock file. PostgreSQL out of scope | Shared storage habits in cutting rooms |
| R-T | Specification drift | Two architectures, empty stubs beside full documents | **OPEN** | `docs/SYSTEM_INTEGRATION_AND_CONSISTENCY.md` | Another task may still add files. This preparation does not overwrite them |

## Review rule

A risk moves from **OPEN** only when a validation record or a counsel note exists, or when an ADR is accepted by a person. This register does not move them.

```mermaid
flowchart LR
  SPEC[Specification]
  REV[Subhajit review]
  TEST[Validation record]
  SPEC --> REV --> TEST
```
