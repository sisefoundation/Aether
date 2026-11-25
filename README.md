# AETHER

AETHER is the open-source, offline-first nonlinear editor being specified for the Society of Indian Screen Editors. The repository is `sise-editor`. Early documents also call the same system SISE Editor. One product is intended. The public name is not finally chosen.

This tree is in early development. The first limited milestone, Slice 0A, is complete and accepted on the tested Fedora machine: a project package, SQLite persistence, session undo, a timing function, and a minimal Qt window. It is not production-ready, not cross-platform, and not the rest of the Foundation Prototype.

**SLICE 0A COMPLETE — DEVELOPMENT FROZEN PENDING NEXT AUTHORISATION.**

## Philosophy

The editor is for a feature cutting room on a local machine. The person thinks about the cut. The machine does not require an account, a sign-in, or a network.

A bin is an editorial object with its own id. A directory is a storage folder. Creating one does not create the other.

A Media ID is minted once for a recording and survives rename, move, and relink. A timeline instance cites that id. It does not invent a new one. Paths are location hints.

Time is a rational frame rate and an integer frame index. Timecode is a label. A seconds figure on screen is formatting. It is not the stored cut. Audio sample positions are derived from sequence frames. Subframe slip is outside the first model.

Original camera and sound files are not silently modified. Relink updates a location. Conform writes a verification report. Export writes a file and a report. Those three results are different. None of them is a claim that another application has opened the file.

The application joins an editorial decision to a media location and then to processing. Codec libraries, the database, and the UI toolkit stay behind those boundaries. The stack direction is accepted with conditions and is not production-validated. See `architecture/decisions/ADR-001-technology-stack.md`.

## Status

| Item | State |
| --- | --- |
| Specifications | Written. Labels in the documents are **CONFIRMED**, **PROPOSED**, and **OPEN**. Silence is not approval of the whole architecture |
| Slice 0A | **Complete and accepted** on the tested Fedora machine. Not production-ready. Development is frozen pending the next authorisation. See `docs/validation/SLICE_0A_PROJECT_CORE_VALIDATION.md` |
| Source | Slice 0A sources under `src/`, public headers under `include/aether/`. The Debug executable is `build/src/aether`. No codec, playback, interchange, or external NLE is implemented |
| Tests | CTest discovers 1 test. The last recorded run passed, with 18 Qt Test cases inside it and 0 failed. That is not 18 CTest tests. See `docs/validation/SLICE_0A_PROJECT_CORE_VALIDATION.md` |
| Licence | AETHER source is Apache License, Version 2.0. See `LICENSE` and `NOTICE`. Copyright 2026 Society of Indian Screen Editors Foundation. Architect: Subhajit Singha, SISE. Qt, FFmpeg, ProRes, DNx, AAF, and trademarks remain open in ADR-008 |
| Next feature slice | Not authorised. Slice 0A is complete. Development of the next feature slice has not started |
| Vendor compatibility | Not claimed and not tested for Resolve, Media Composer, Premiere Pro, or Pro Tools |
| Foundation Prototype | Specified in `docs/FIRST_IMPLEMENTATION_MILESTONE.md`. Slice 0A does not finish it and does not start the next slice |

The audit headline and the inventory of conflicts are in `docs/ARCHITECTURE_AUDIT.md`.

## Build

The tree follows the ASWF sample project’s top-level split: root `CMakeLists.txt`, `include/`, `src/`, and `tests/`. The map and the exceptions are `docs/ASWF_LAYOUT.md`. The language standard is prototype-only C++17. ADR-001 does not name a standard. FFmpeg is not searched. `tools/validation/ffmpeg_link_smoke_test/` is not added to this build. The source licence is `LICENSE`.

Configure, build, and test, from this directory:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure
./build/src/aether --shutdown-now
```

On this Fedora machine those commands completed after `sqlite-devel` 3.53.4 and `qt6-qtbase-devel` 6.11.2 were installed, together with the DNF dependencies of those two packages. CMake still stops with a message that names those packages when either development file is missing. Nothing is vendored in their place. The recorded result is `docs/validation/SLICE_0A_PROJECT_CORE_VALIDATION.md`. Development is frozen; these commands describe the accepted milestone, and they are not a start on the next slice.

`docs/18_BUILD_AND_RELEASE.md` still holds the release rules. This directory is not a git repository. There is no install rule and no redistributable package.

## Documentation index

Start with the audit, then the readiness report.

### Preparation set

| Document | Subject |
| --- | --- |
| [docs/ARCHITECTURE_AUDIT.md](docs/ARCHITECTURE_AUDIT.md) | What is written, missing, contradictory, and duplicated |
| [docs/AETHER_PREBUILD_READINESS_REPORT.md](docs/AETHER_PREBUILD_READINESS_REPORT.md) | Readiness sections A–I and the start recommendation |
| [docs/01_PRODUCT_REQUIREMENTS.md](docs/01_PRODUCT_REQUIREMENTS.md) | Requirements |
| [docs/03_TECHNOLOGY_DECISIONS.md](docs/03_TECHNOLOGY_DECISIONS.md) | Candidates. Not approvals |
| [architecture/decisions/](architecture/decisions/) | ADRs. Status is Proposed or Open. None is Accepted |
| [docs/04_CORE_DATA_MODEL.md](docs/04_CORE_DATA_MODEL.md) | Identity map |
| [docs/05_TIMEBASE_AND_TIMING_ARCHITECTURE.md](docs/05_TIMEBASE_AND_TIMING_ARCHITECTURE.md) | Rational time |
| [docs/PROJECT_STORAGE_AND_DATABASE.md](docs/PROJECT_STORAGE_AND_DATABASE.md) | `project.db` and `.abin` |
| [docs/10_PLAYBACK_AND_RENDERING.md](docs/10_PLAYBACK_AND_RENDERING.md) | Picture and monitoring seam |
| [docs/11_EFFECTS_AND_COMPOSITING.md](docs/11_EFFECTS_AND_COMPOSITING.md) | Effects as data |
| [docs/12_COLOUR_MANAGEMENT.md](docs/12_COLOUR_MANAGEMENT.md) | Tags and the open colour scope |
| [docs/14_PLUGIN_ARCHITECTURE.md](docs/14_PLUGIN_ARCHITECTURE.md) | Plugin tiers |
| [docs/15_UNDO_RECOVERY_AND_SAFETY.md](docs/15_UNDO_RECOVERY_AND_SAFETY.md) | Undo and recovery |
| [docs/EXCHANGE_COMPATIBILITY_MATRIX.md](docs/EXCHANGE_COMPATIBILITY_MATRIX.md) | Specified versus executed |
| [docs/16_SECURITY_AND_LICENSING.md](docs/16_SECURITY_AND_LICENSING.md) | File safety and legal flags |
| [docs/17_TESTING_AND_VALIDATION.md](docs/17_TESTING_AND_VALIDATION.md) | What counts as evidence |
| [docs/18_BUILD_AND_RELEASE.md](docs/18_BUILD_AND_RELEASE.md) | Empty build, release gates |
| [docs/19_DEVELOPMENT_ROADMAP.md](docs/19_DEVELOPMENT_ROADMAP.md) | Phases 1–7. No dates |
| [docs/SYSTEM_INTEGRATION_AND_CONSISTENCY.md](docs/SYSTEM_INTEGRATION_AND_CONSISTENCY.md) | How the set stays one product |
| [docs/ENGINEERING_INTERFACES.md](docs/ENGINEERING_INTERFACES.md) | Joins and forbidden edges |
| [docs/PERFORMANCE_AND_SCALABILITY.md](docs/PERFORMANCE_AND_SCALABILITY.md) | Budgets that are not measurements |
| [docs/FIRST_IMPLEMENTATION_MILESTONE.md](docs/FIRST_IMPLEMENTATION_MILESTONE.md) | Foundation Prototype |
| [docs/ARCHITECTURE_RISK_REGISTER.md](docs/ARCHITECTURE_RISK_REGISTER.md) | Risks |

### Existing contracts

These own their subjects. The preparation set points at them and does not replace them.

| Area | Start here |
| --- | --- |
| Early architecture, modules, phases 0–16 | [ARCHITECTURE.md](ARCHITECTURE.md), [MODULES.md](MODULES.md), [DEVELOPMENT_ROADMAP.md](DEVELOPMENT_ROADMAP.md) |
| Product and workstation | [docs/PRODUCT_VISION.md](docs/PRODUCT_VISION.md), [docs/EDITORIAL_PHILOSOPHY.md](docs/EDITORIAL_PHILOSOPHY.md), [docs/UI_ARCHITECTURE.md](docs/UI_ARCHITECTURE.md) |
| System layers | [docs/SYSTEM_ARCHITECTURE.md](docs/SYSTEM_ARCHITECTURE.md), [docs/SOFTWARE_MODULES.md](docs/SOFTWARE_MODULES.md) |
| Bins | [docs/BIN_ARCHITECTURE.md](docs/BIN_ARCHITECTURE.md) and the other `docs/BIN_*.md` files |
| Media identity, ingest, relink | [docs/MEDIA_IDENTITY_SPECIFICATION.md](docs/MEDIA_IDENTITY_SPECIFICATION.md), [docs/MEDIA_RELINK_ENGINE.md](docs/MEDIA_RELINK_ENGINE.md) |
| Timeline and trims | [docs/TIMELINE_ENGINE_ARCHITECTURE.md](docs/TIMELINE_ENGINE_ARCHITECTURE.md), [docs/EDITORIAL_OPERATIONS.md](docs/EDITORIAL_OPERATIONS.md), [docs/TRIM_ENGINE_SPECIFICATION.md](docs/TRIM_ENGINE_SPECIFICATION.md) |
| Time | [docs/TIMEBASE_AND_FRAME_ACCURACY.md](docs/TIMEBASE_AND_FRAME_ACCURACY.md), [docs/AUDIO_TIMEBASE_AND_SYNC.md](docs/AUDIO_TIMEBASE_AND_SYNC.md) |
| Sound | [docs/AUDIO_ENGINE_ARCHITECTURE.md](docs/AUDIO_ENGINE_ARCHITECTURE.md) |
| MXF and profiles | [docs/NATIVE_MEDIA_ARCHITECTURE.md](docs/NATIVE_MEDIA_ARCHITECTURE.md) |
| Offline, proxy, conform | [docs/OFFLINE_EDITING_ARCHITECTURE.md](docs/OFFLINE_EDITING_ARCHITECTURE.md), [docs/CONFORM_ENGINE_SPECIFICATION.md](docs/CONFORM_ENGINE_SPECIFICATION.md) |
| Exchange | [docs/INTERCHANGE_ADAPTER_ARCHITECTURE.md](docs/INTERCHANGE_ADAPTER_ARCHITECTURE.md), [docs/AVX_FORMAT_SPECIFICATION.md](docs/AVX_FORMAT_SPECIFICATION.md), [docs/AAF_AUDIO_EXCHANGE_PROFILE.md](docs/AAF_AUDIO_EXCHANGE_PROFILE.md) |
| Project folder and schemas | [docs/PROJECT_FOLDER_SPECIFICATION.md](docs/PROJECT_FOLDER_SPECIFICATION.md), [docs/BIN_DATABASE_SCHEMA.md](docs/BIN_DATABASE_SCHEMA.md), [docs/PROJECT_DATABASE_SCHEMA.md](docs/PROJECT_DATABASE_SCHEMA.md) |
| Safety, recovery, licences | [docs/SECURITY_AND_FILE_SAFETY.md](docs/SECURITY_AND_FILE_SAFETY.md), [docs/RECOVERY_AND_BACKUP.md](docs/RECOVERY_AND_BACKUP.md), [docs/DEPENDENCY_AND_LICENCE_AUDIT.md](docs/DEPENDENCY_AND_LICENCE_AUDIT.md) |

`docs/ARCHITECTURE.md`, `docs/DEVELOPMENT_ROADMAP.md`, `docs/MODULES.md`, `docs/MEDIA_PIPELINE.md`, and `docs/PROJECT_FORMAT.md` are empty stubs. The text for the first three subjects lives at the repository root.

## Known limits

- Slice 0A is a limited project-core milestone on Fedora. It is not a production editor. There is no codec, no playback, no interchange, and no claim that an external NLE has opened a file.
- The owner has accepted an architectural direction: C++, Qt 6, CMake, SQLite, and FFmpeg as the mandatory core media framework (ADR-001, **ACCEPTED WITH CONDITIONS**). GStreamer is not a mandatory dependency. The stack is not production-validated. FFmpeg is not linked by Slice 0A. Library integration on this machine remains blocked. Licensing is open, pending counsel.
- A prior local study of FFmpeg 9.0.2 is recorded in the media documents as behaviour to reconcile. It was not re-run for this index, and it is not a validation pass or a compatibility test.
- MXF, ProRes, and DNxHR / VC-3 profiles are specified and not validated. OP-Atom is not an available writer.
- One `project.db` plus a manifest is the accepted store. One Media ID per logical asset is accepted. Session-only undo is accepted for the Foundation Prototype. `.abin` is interchange only.
- Performance figures in the specifications are targets or arithmetic, not measurements.
- Slice 0A is complete and accepted on the tested Fedora machine. It is not the Foundation Prototype, not a production validation, and not a cross-platform claim. The 30000/1001 MXF encoder failure is unchanged. Debt and the unauthorised next work are listed in `docs/SLICE_0A_TECHNICAL_DEBT.md`.
- **SLICE 0A COMPLETE — DEVELOPMENT FROZEN PENDING NEXT AUTHORISATION.**

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md). Slice 0A is frozen. Do not add application features until the next slice is authorised.
