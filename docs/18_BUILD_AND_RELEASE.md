# 18 — Build and release

Status: architecture preparation. This document authorises no build files beyond the empty `CMakeLists.txt` already in the tree, and it does not edit that file.

## 1. Current build status

**CONFIRMED.**

| Item | State |
| --- | --- |
| `CMakeLists.txt` | Present at the repository root. Zero bytes. No `cmake_minimum_required`, no targets, no dependencies |
| Configure and compile | Not possible from this tree |
| `src/` | Empty directories. No translation units |
| `tests/` | Empty |
| `third_party/` | Empty. No vendored code |
| `plugins/` | Empty |
| Generator, compiler, or packaged Qt | Not selected, because nothing is linked |
| Installer, package, or version number | None |
| `LICENSE` | Empty file. No release can ship until a person chooses a licence and counsel reviews the combination |
| Git | This directory is not a git repository. There is no revision, tag, or remote recorded here |

CMake is the accepted build system (ADR-001). The root file is still empty. This document does not fill it. The validation harness has its own CMake file under `tools/validation/ffmpeg_link_smoke_test/` and is not the product. Its configure on this machine **Failed** because `ffmpeg-devel` is absent.

## 2. Contract for a later build

**PROPOSED** targets, matching `docs/ENGINEERING_INTERFACES.md`. Names are architectural.

| Target | Links | Does not link |
| --- | --- | --- |
| Kernel and editorial tests | C++ standard library, test harness | Qt, FFmpeg, SQLite, OpenColorIO, OpenTimelineIO. GStreamer is not a product dependency |
| Infrastructure database | SQLite. ADR-003 is accepted. Feasibility is still unmeasured | Qt, codecs |
| Media Engine | The approved FFmpeg shared libraries, after the link harness passes and counsel has reviewed the pin | `libavfilter` unless a later decision adds it. Editorial headers. GStreamer |
| UI | Qt 6, after the legal review | A second UI toolkit |
| Application | The domain targets | Nothing that creates a cycle |

Public headers do not include third-party headers (`MODULES.md`).

**OPEN.** Compiler (GCC or Clang are both open-source candidates on Linux). C++ standard library licence is part of ADR-008, not a reason to pick a compiler in this file.

**OPEN.** Linux packaging format. A note that names system libraries belongs to the later acceptance phase (`DEVELOPMENT_ROADMAP.md` phase 16). It is not written here as if packages existed.

## 3. Workflow: first configure, when it is allowed

1. Review accepts ADR-001 or replaces it.
2. A change adds the smallest `CMakeLists.txt` that builds the kernel and its tests. That change is implementation, not this preparation.
3. The configure log lists linked libraries. The Foundation Prototype’s log lists none of the UI or codec libraries.
4. Tests in `docs/17_TESTING_AND_VALIDATION.md` run from the build tree.

Failure: a configure that fetches proprietary SDKs, or that enables FFmpeg non-free components, stops. The capability stays off.

```mermaid
flowchart LR
  REV[Accepted review]
  CM[CMake kernel target]
  TEST[Headless tests]
  UI[Qt target later]
  REV --> CM --> TEST
  TEST --> UI
```

## 4. Release contract

**PROPOSED.** A release needs all of the following. None exist today.

- A non-empty `LICENSE` and a licence inventory whose rows match the linked libraries.
- A version identifier a person chose.
- A Linux build that passes the milestone then in force.
- Capability flags that hide unvalidated codecs and untested exchange targets.
- No phone-home default.
- A statement of what was not tested, including every vendor cell in `docs/EXCHANGE_COMPATIBILITY_MATRIX.md` that is still Not executed.

**OPEN.** Who may mark a release: the governance note in `docs/SYSTEM_ARCHITECTURE.md` unresolved 22. This file does not appoint a release manager.

## 5. Failure cases

| Case | Result |
| --- | --- |
| Empty CMake treated as a successful configure | False. There is no build |
| `third_party/` filled during preparation | Out of scope. The directory stays empty |
| Release notes say “works with Media Composer” | Rejected until an executed cell exists |
| Static link of an LGPL library without an audit note | Release blocked |

## 6. Acceptance criteria

- This preparation’s acceptance is the truthful status in §1, not a green build.
- The first implementation milestone’s build acceptance is in `docs/FIRST_IMPLEMENTATION_MILESTONE.md`.
- A reviewer who runs `cmake` today should expect failure or an empty project. That expectation is the current status. This preparation did not run `cmake`, because the file defines no project. **CONFIRMED** the file is empty.
