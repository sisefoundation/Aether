# Slice 0B build-system hardening

**SLICE 0B COMPLETE — BUILD SYSTEM HARDENED — FOUNDATION VALIDATED — FUTURE DEVELOPMENT REQUIRES SEPARATE AUTHORISATION.**

Validated on this Fedora machine only. Not production-ready. Not cross-platform.

The freeze was lifted only for this slice. Slice 0C is **NOT STARTED** and **NOT AUTHORISED**. ADR-006 stays **PROPOSED**. ADR-007 stays **PROPOSED**. Refinements A, B, and C stay unaccepted. No result below is inferred. Evidence is the logs under `validation-fixtures/slice-0b-2026-11-21/`. The Slice 0A logs were not overwritten. The existing `build/` directory was not deleted.

## 1. Objective

Replace the deprecated CMake target `SQLite::SQLite3` with `SQLite3::SQLite3` if, and only if, this CMake and this SQLite package actually export that target. Preserve the link to the same SQLite library. Re-run the foundation configure, build, CTest, Qt Test, and a startup smoke test.

## 2. Authorisation scope

Authorised: build-system hardening and foundation validation.

Not authorised, and not done: accepting ADR-006 or ADR-007; treating refinements A, B, or C as accepted; a sequence schema; a timeline; playback; an audio engine; FFmpeg linking; relink; conform; plugins; a new public interface; a C++ standard change; a Qt major-version change; a SQLite schema change; a project-database format change; a persistence-logic change; package installation or removal; a Fedora upgrade; `git init`, commit, or push; filling `docs/DEVELOPMENT_ROADMAP.md`; editing an ADR or `docs/decisions/ADR-006_OWNER_DECISION_RECORD.md`.

## 3. Baseline

| Item | Record |
| --- | --- |
| Git | Not a repository. `git status` stopped with `fatal: not a git repository`. Slice 0A recorded the same fact and did not run `git init`. This slice did not either |
| Branch | None |
| HEAD | None |
| Existing build | `build/` is the Slice 0A CMake tree (`CMakeCache.txt` names that directory). It was left in place |
| New build | `build-slice0b/` |

## 4. Environment

Log: `validation-fixtures/slice-0b-2026-11-21/environment.log`.

| Item | Version |
| --- | --- |
| OS | Fedora Linux 45 (Workstation Edition) |
| Kernel | 7.2.15-300.fc45.x86_64 |
| Compiler | g++ 16.2.1 20261029 (Red Hat 16.2.1-3). This was the compiler CMake selected |
| CMake | 4.3.0 |
| `qt6-qtbase-devel` | 6.11.2-3.fc45 |
| `sqlite-devel` | 3.53.4-1.fc45 |
| pkg-config Qt6Widgets | 6.11.2 |
| pkg-config sqlite3 | 3.53.4 |
| Session | `XDG_SESSION_TYPE=wayland`, `WAYLAND_DISPLAY=wayland-0`, `DISPLAY=:0` |

`cmake`, `gcc-c++`, `qt6-qtbase-devel`, and `sqlite-devel` were already installed. No package was installed or removed. `ffmpeg-devel` was not installed.

## 5. CMake and dependency findings

`find_package(SQLite3)` uses CMake's module `/usr/share/cmake/Modules/FindSQLite3.cmake`. There is no `SQLite3Config.cmake` from the SQLite package. The module, for CMake 4.3, creates two imported targets when SQLite is found:

| Target | What this module does |
| --- | --- |
| `SQLite3::SQLite3` | Added in CMake 4.3. `IMPORTED_LOCATION` is the library CMake found |
| `SQLite::SQLite3` | Marked deprecated, with the message to use `SQLite3::SQLite3` |

A throwaway probe, not part of the product build, reported `SQLite3::SQLite3` present, `IMPORTED_LOCATION` `/usr/lib64/libsqlite3.so`, version 3.53.4, CMake 4.3.0. The deprecated target was also present. No compatibility alias was added.

Qt 6 Widgets and Qt 6 Test were found by the product configure: Qt 6.11.2. FFmpeg was not searched.

## 6. SQLite target correction

`src/CMakeLists.txt` linked `aether_persistence` to `SQLite::SQLite3`. That is the name CMake 4.3 deprecates. It now links `SQLite3::SQLite3`.

`CMakeLists.txt` fails configure if `find_package(SQLite3)` succeeds but does not export `SQLite3::SQLite3`. It does not alias the old name and does not fall back to it. `cmake_minimum_required` stays 3.24. C++17 stays prototype-only.

The product link lines in `build-slice0b` use `/usr/lib64/libsqlite3.so` for both `aether` and `aether_slice0a_tests`. That is the same library path the probe reported for `SQLite3::SQLite3`.

## 7. Files changed

| File | Change |
| --- | --- |
| `CMakeLists.txt` | Require the imported target `SQLite3::SQLite3` and record that the build links it |
| `src/CMakeLists.txt` | Link `aether_persistence` to `SQLite3::SQLite3` |
| `docs/validation/SLICE_0B_BUILD_SYSTEM_HARDENING.md` | This report |
| `DEVELOPMENT_ROADMAP.md` | Slice 0B status |
| `docs/19_DEVELOPMENT_ROADMAP.md` | Slice 0B status |
| `validation-fixtures/slice-0b-2026-11-21/` | New logs. Slice 0A logs were not rewritten |

`docs/DEVELOPMENT_ROADMAP.md` was not filled. No C++ source, header, test, schema, or persistence file was edited.

## 8. Clean build

The Slice 0A directory `build/` was not reused and was not removed.

```text
cmake -S /home/editry/development/sise-editor -B /home/editry/development/sise-editor/build-slice0b -DCMAKE_BUILD_TYPE=Debug
cmake --build /home/editry/development/sise-editor/build-slice0b --parallel
```

| Step | Result |
| --- | --- |
| Configure | Exit 0. Log: `validation-fixtures/slice-0b-2026-11-21/cmake-configure.log`. It reports SQLite3 3.53.4, `Slice 0B links imported target SQLite3::SQLite3`, Qt6 6.11.2, prototype-only C++17, `Configuring done`, and `Generating done`. It contains no `CMake Warning` line |
| Build | Exit 0. Log: `validation-fixtures/slice-0b-2026-11-21/build.log`. It ends with `Built target aether`. It contains no compiler `warning:` or `error:` line |

Executable: `build-slice0b/src/aether`. Test binary: `build-slice0b/tests/aether_slice0a_tests`.

## 9. CTest

Log: `validation-fixtures/slice-0b-2026-11-21/ctest.log`.

`ctest -N` lists `Test #1: slice0a` and `Total Tests: 1`, exit 0. `ctest --output-on-failure` reports `1/1 Test #1: slice0a` passed, `100% tests passed, 0 tests failed out of 1`, exit 0.

That is one CTest test. It is not 18 CTest tests.

## 10. Qt Test

Log: `validation-fixtures/slice-0b-2026-11-21/qt-test.log`.

The same executable was run directly, not only through CTest. Qt Test reported:

| Class | Totals line |
| --- | --- |
| TimingTests | 7 passed, 0 failed, 0 skipped, 0 blacklisted |
| ProjectTests | 11 passed, 0 failed, 0 skipped, 0 blacklisted |

18 Qt Test cases, 0 failed. Direct exit 0. These 18 cases are inside the one CTest test named `slice0a`.

## 11. Smoke test

Log: `validation-fixtures/slice-0b-2026-11-21/application-launch.log`.

Buttons and menus were not used.

| Run | What the log shows |
| --- | --- |
| `QT_QPA_PLATFORM=wayland --shutdown-now` | `AETHER start`, `AETHER stop`, exit 0. Qt started and the process exited cleanly. This run does not show a window |
| `QT_QPA_PLATFORM=wayland` without the shutdown flag | `AETHER start`. `wmctrl -l` did not list AETHER. The process was killed and the wait status was 143. A native Wayland window was not confirmed |
| `QT_QPA_PLATFORM=xcb --shutdown-now` | `AETHER start`, `AETHER stop`, exit 0 |
| `QT_QPA_PLATFORM=xcb` without the shutdown flag | `wmctrl` listed `0x01200007  0 fedora AETHER`. After `wmctrl -c`, the log shows `AETHER stop` and `xcb_window_close_exit=0` |

The XCB window is not a native Wayland result. Wayland and XCB differ: only the XCB probe listed a window.

## 12. Remaining warnings

The Slice 0B configure log has no `CMake Warning` line. The 18 developer warnings from the Slice 0A configure log are historical. They were not rewritten. The Slice 0B build log has no compiler warning line.

`docs/SLICE_0A_TECHNICAL_DEBT.md` still describes the deprecated target as outstanding. That file was not edited.

## 13. Remaining debt

- Native Wayland window: not confirmed. `wmctrl` did not list it.
- Window controls: not clicked.
- C++17: still prototype-only. No standard was chosen.
- ADR-006 and ADR-007: still **PROPOSED**. Refinements A, B, and C were not accepted.
- FFmpeg: not linked. The MXF result of 12813 samples against 12812 for the tested eight frames from frame 0 stays a failed encoder test.
- Power-loss protection: not tested.
- Cross-platform validation: not run.

## 14. Deferred work

Slice 0C and every later phase stay **NOT STARTED** and **NOT AUTHORISED**. Sequence schema, timeline, playback, audio implementation, FFmpeg linking, relink, conform, plugins, and new public interfaces were not started.

## 15. Final status

| Check | Status |
| --- | --- |
| `SQLite3::SQLite3` is a real export of this CMake module and was linked | **PASSED** |
| Clean Debug configure and build in `build-slice0b` | **PASSED** |
| CTest: 1 test, 1 passed | **PASSED** |
| Qt Test: 18 cases, 0 failed | **PASSED** |
| Startup, Qt init, and clean exit | **PASSED** on both platform settings for `--shutdown-now`, and on the XCB window close |
| Native Wayland window | **NOT CONFIRMED** |
| Production readiness or cross-platform validation | **NOT CLAIMED** |

**SLICE 0B COMPLETE — BUILD SYSTEM HARDENED — FOUNDATION VALIDATED — FUTURE DEVELOPMENT REQUIRES SEPARATE AUTHORISATION.**
