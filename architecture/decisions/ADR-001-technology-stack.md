# ADR-001 — Technology stack

- Status: **ACCEPTED WITH CONDITIONS**
- Date: 2026-11-20
- Deciders: project owner. This acceptance is an architectural direction. It is not a production validation and it is not a legal clearance.
- Related: `docs/03_TECHNOLOGY_DECISIONS.md`, `docs/technology/FFMPEG_INTEGRATION_ARCHITECTURE.md`, `docs/technology/FFMPEG_DEVELOPMENT_CONFIGURATION.md`, `docs/technology/FFMPEG_BUILD_AND_DISTRIBUTION.md`, `docs/validation/FFMPEG_LIBRARY_INTEGRATION_VALIDATION.md`, `docs/NATIVE_MEDIA_ARCHITECTURE.md`

## Context

The early architecture named C++17 or newer, Qt 6, CMake, FFmpeg, GStreamer, OpenTimelineIO, SQLite, and OpenColorIO, and once called that list approved while the same file still awaited review. Nothing in that sentence was a recorded owner decision. `CMakeLists.txt` at the repository root is empty and remains the product scaffold.

On 2026-11-20 the owner accepted the direction below. Conditions are still open. A CLI session the same day is `docs/validation/FFMPEG_MXF_ROUNDTRIP_TEST.md`. CLI success is not library integration. The link attempt is `docs/validation/FFMPEG_LIBRARY_INTEGRATION_VALIDATION.md`.

## Decision

**ACCEPTED WITH CONDITIONS.** The approved direction is:

- C++ is the primary language for domain and infrastructure.
- Qt 6 is the desktop framework. UI types stay in the UI module.
- CMake is the build system. The empty root `CMakeLists.txt` is not turned into the application by this decision. The validation harness under `tools/validation/ffmpeg_link_smoke_test/` has its own CMake file and is not product code.
- SQLite is the project database engine. The file is `project.db` (ADR-003). PostgreSQL is out of scope.
- FFmpeg is the mandatory core media framework. It is not optional and it is not one candidate among equals.

FFmpeg is dynamically linked and encapsulated inside the AETHER Media Engine only. The approved link set is:

- `libavutil`
- `libavcodec`
- `libavformat`
- `libswscale`
- `libswresample`

Domain code does not include FFmpeg headers. FFmpeg types do not appear in Timeline, Bin, Project, Media Registry, or other editorial domain code. Those modules talk to media through AETHER interfaces. `AVFrame`, `AVPacket`, `AVRational`, and FFmpeg error codes stop at the Media Engine boundary.

`libavfilter` is **not approved**. It is not added. Reconsider it only when the Effects and Processing architecture requires it. That reconsideration is a new decision.

GStreamer is **not** a mandatory dependency. It is not a selected foundational media framework beside FFmpeg. It may be reconsidered later only for a specific justified use case. That reconsideration is a new decision.

The media architecture is not built around the FFmpeg CLI. The CLI is allowed for validation, diagnostics, and controlled standalone operations.

The tested Fedora FFmpeg 9.0.2 GPL build (`ffmpeg-9.0.2-3.fc45`), with extra codecs and no development headers, is **not** the approved distribution configuration. It has not shown that AETHER can link the libraries. The system install is not treated as redistributable. The build baseline to take to counsel and to a later pin is `docs/technology/FFMPEG_DEVELOPMENT_CONFIGURATION.md`. That document does not claim a legal approval.

### Engineering obligations of the media link

These are part of the accepted direction. They are not yet demonstrated:

- dynamic linking of the approved set only, inside the Media Engine
- version pinning and an ABI policy for that pin
- runtime discovery of the pinned libraries, distinct from “whatever the distro installed”
- development headers and a build configuration that can actually be linked
- codec and container capability discovery from AETHER’s own validation rows, not from a muxer name
- structured errors at the AETHER boundary
- explicit threading and resource ownership, and cancellation
- logging that does not leak FFmpeg types into domain code
- cross-platform availability as a later validation, not as a claim from this Fedora machine
- a software path when hardware acceleration is absent

OpenTimelineIO remains an adapter candidate, not the project database. OpenColorIO remains a colour candidate. Neither is adopted by this ADR. DeckLink stays off unless a later decision says otherwise.

## Conditions

Status stays **ACCEPTED WITH CONDITIONS** until each of these has evidence. None of them is closed by this ADR:

1. Build validation. A CMake build of the product must configure. The root file is still empty. The isolated harness failed configure on this machine because development files are absent.
2. Qt feasibility and SQLite feasibility against the project and UI contracts. Not measured here.
3. FFmpeg library integration validation. **BLOCKED** on this machine. See `docs/validation/FFMPEG_LIBRARY_INTEGRATION_VALIDATION.md`.
4. Version and ABI policy for the pinned FFmpeg build. The policy is stated in the development-configuration document. No pin has been built.
5. Cross-platform validation. This record is one Fedora machine.
6. Legal review of the actual dependency configuration. ADR-008 stays **OPEN**. Counsel has not written. Dynamic linking does not clear that review.

## Consequences

- Documents that list FFmpeg and GStreamer as equally selected foundational frameworks are superseded for that claim. GStreamer text that describes a playback graph remains an earlier design note, not a dependency.
- A writer is not offered because a library symbol or a CLI muxer exists. Capability rows still require a passing tuple on the pinned build.
- The 30000/1001 audio contract failure remains a media-writer failure. It does not change `sampleIndex` (`docs/validation/AUDIO_FRAME_SAMPLE_BOUNDARY_VALIDATION.md`).
- If counsel rejects a given FFmpeg build, or rejects Qt, the editorial model and Media IDs stay. The rejected library leaves the module that was allowed to link it.

## Alternatives

- GStreamer as a mandatory core framework. **Rejected** by the owner for that role.
- `libavfilter` in the core link. **Not approved.**
- Building the cut around the FFmpeg CLI. **Rejected.**
- Shipping or linking this machine’s Fedora FFmpeg as the AETHER engine. **Rejected** as the distribution configuration.
- PostgreSQL as the project database. Not in this decision.

## Confirmation

Accepted by the project owner on 2026-11-20 as an architectural direction **with the conditions above**. Not fully production-validated.
