# ASWF layout

**SLICE 0A COMPLETE — DEVELOPMENT FROZEN PENDING NEXT AUTHORISATION.**

Accepted on the tested Fedora machine only. Not cross-platform, not production-ready, not a complete NLE, not codec, playback, interchange, or external-NLE validated, and not a power-loss proof. Slice 0B has not started.

The layout compared here is the `main` tree of [AcademySoftwareFoundation/aswf-sample-project](https://github.com/AcademySoftwareFoundation/aswf-sample-project) at commit `485ecf3dbb2bf86a0a18495ed0add24083343b59`, fetched on 21 November 2026. Timing rules, media-identity rules, and database semantics were not changed. C++17 stays prototype-only.

## Directories matched

| ASWF sample | AETHER |
| --- | --- |
| Root `CMakeLists.txt` | Root `CMakeLists.txt`. It finds Qt 6 and SQLite, sets prototype-only C++17, then adds `include`, `src`, and `tests` |
| `include/` | `include/aether/`. Public headers |
| `include/CMakeLists.txt` | Present. No generated version header |
| `src/` | `src/`. Translation units only |
| `src/CMakeLists.txt` | Libraries `aether_timing`, `aether_persistence`, `aether_session`, `aether_ui`, and the `aether` executable |
| `tests/` | `tests/`. Qt Test executable `aether_slice0a_tests` |
| `docs/` | `docs/`. Architecture and validation documents stay here |
| `LICENSE` | Apache License, Version 2.0, with the appendix applied |

The fetched tree also contains `.github/` (issue templates, a pull-request template, and two workflows), `CTestConfig.cmake`, `CTestScript.cmake`, and governance files (`GOVERNANCE.md`, `CODE_OF_CONDUCT.md`, `CODEOWNERS`, `azure-pipelines.yml`). It does not contain a `cmake/` module directory, and it does not contain `NOTICE`.

## Path map

| Old path | New path |
| --- | --- |
| `src/core/timing/sample_boundary.hpp` | `include/aether/core/timing/sample_boundary.hpp` |
| `src/core/project/ids.hpp` | `include/aether/core/project/ids.hpp` |
| `src/core/project/project_info.hpp` | `include/aether/core/project/project_info.hpp` |
| `src/core/project/result.hpp` | `include/aether/core/project/result.hpp` |
| `src/core/bin/bin.hpp` | `include/aether/core/bin/bin.hpp` |
| `src/core/media/media.hpp` | `include/aether/core/media/media.hpp` |
| `src/persistence/logger.hpp` | `include/aether/persistence/logger.hpp` |
| `src/persistence/manifest.hpp` | `include/aether/persistence/manifest.hpp` |
| `src/persistence/project_store.hpp` | `include/aether/persistence/project_store.hpp` |
| `src/app/project_session.hpp` | `include/aether/app/project_session.hpp` |
| `src/ui/main_window.hpp` | `include/aether/ui/main_window.hpp` |
| `src/core/timing/sample_boundary.cpp` | unchanged |
| `src/core/project/ids.cpp` | unchanged |
| `src/persistence/*.cpp` | unchanged |
| `src/app/project_session.cpp` | unchanged |
| `src/ui/main.cpp`, `src/ui/main_window.cpp` | unchanged |
| `tests/*.cpp`, `tests/*.hpp` | unchanged |
| Root product targets in `CMakeLists.txt` | Split across `CMakeLists.txt`, `include/CMakeLists.txt`, `src/CMakeLists.txt`, and `tests/CMakeLists.txt` |
| `build/aether` | `build/src/aether`, because the executable is added from `src/` |
| `build/aether_slice0a_tests` | `build/tests/aether_slice0a_tests` |
| Quoted includes `core/`, `persistence/`, `app/`, `ui/` | `aether/core/`, `aether/persistence/`, `aether/app/`, `aether/ui/` |

## Exceptions

These sample conventions were not copied. The AETHER decision stays.

| Sample convention | AETHER decision that stays |
| --- | --- |
| Hello target compiles as C++14 for the VFX Reference Platform | C++17, prototype-only. ADR-001 sets no permanent standard |
| Tests are registered from `src/CMakeLists.txt` | The Qt Test executable stays in `tests/`. The sample’s own `tests/` directory is otherwise only a README |
| Generated `include/hello.h` from `hello.h.in` | No generated version header. Slice 0A does not add one |
| `.github` workflows, issue templates, and Azure Pipelines | Not added. No CI was authorised, and this is not an ASWF-hosted project |
| `cmake/` modules | The fetched tree has none. None were invented. `include(CTest)` is the sample’s own test hook |
| No `architecture/` directory | `architecture/decisions/` stays. Accepted and proposed ADRs are not relocated |
| No validation-log directory | `validation-fixtures/` stays at the repository root. The validation reports cite those paths. The logs were not deleted and were not rewritten |
| No `tools/` directory | `tools/validation/ffmpeg_link_smoke_test/` stays isolated and is not added to the product build |
| `SQLite::SQLite3` is not a sample concern | The deprecated CMake target name is unchanged. The warning is still debt, not a fixed build |
| FFmpeg is not in the sample | FFmpeg is still not searched or linked. `libavfilter` is not approved |

`NOTICE` is an Apache-2.0 attribution file. The sample tree does not have one. AETHER adds it because the owner required the appendix attribution.
