# Slice 0A technical debt

Status: recorded when the owner accepted Slice 0A on 2026-11-20. Nothing in this file is scheduled, and none of it was implemented by the closeout or by this status pass.

**SLICE 0A COMPLETE — DEVELOPMENT FROZEN PENDING NEXT AUTHORISATION.**

Accepted on the tested Fedora machine only. Not cross-platform, not production-ready, not a complete NLE, not codec, playback, interchange, or external-NLE validated, and not a power-loss proof. Slice 0B has not started.

The slice is complete on the tested Fedora machine. Evidence is `docs/validation/SLICE_0A_PROJECT_CORE_VALIDATION.md`. The items below remain outstanding and unimplemented.

## 1. Non-blocking debt

These items are true of the accepted slice. They did not fail the build or the tests.

| Item | What the record shows | What a later authorisation would still have to decide |
| --- | --- | --- |
| Deprecated CMake SQLite target | The Debug configure log contains 18 developer warnings. `SQLite::SQLite3` is deprecated. The replacement named by CMake is `SQLite3::SQLite3`. Configure and the build still completed. The executable links the SQLite CMake found | Change the target only under a later authorisation. Do not treat the warning as a failed build |
| C++17 | `CMAKE_CXX_STANDARD` is 17 and the configure log calls it prototype-only. ADR-001 does not set a standard | A permanent standard needs an accepted ADR. This slice does not set one |
| Native Wayland | The default launch did not appear in `wmctrl`. That process was terminated and exited 143 | Do not record a native Wayland pass from the XCB result |
| Window controls | The XCB window title was AETHER and the process exited 0 after close. Create, Open, Close, and the bin list were not clicked | A functional UI test is still undone |
| Transitive DNF packages | Twelve dependencies were installed with `qt6-qtbase-devel` 6.11.2 and `sqlite-devel` 3.53.4: the Qt ibase, mysql, odbc, and postgresql drivers, `libpq`, `libfbclient2`, `unixODBC` and `odbcinst-generate`, Vulkan headers and the loader development files, the `sqlite` command-line package, and `qt6-rpm-macros` | These are DNF requirements of the two development packages. They are not independent AETHER runtime requirements, and they are not a choice of project database |
| Qt Test on the test binary | The one CTest test links Qt Test. Domain libraries do not | The exception is prototype-specific. It does not change `docs/ENGINEERING_INTERFACES.md` |
| Prototype filenames | The tests pass with `project.manifest.json` and an empty `BINS/` | Those names do not amend `docs/PROJECT_FOLDER_SPECIFICATION.md` |
| Durability | Autosave and the interrupted-transaction case passed. `BACKUP/` is only an empty directory | That is not a power-loss proof and not a snapshot restore |

## 2. Future priorities, not authorised

The owner brief kept the following out of Slice 0A. They stay incomplete. Recording them here does not authorise the work.

1. Any next slice, including Slice 0B.
2. Accepting ADR-002, ADR-006, or ADR-007. They stay **PROPOSED**.
3. A sequence or timeline schema, and the timeline editor.
4. FFmpeg linking, decoding, proxies, MXF, playback, and rendering. The 30000/1001 eight-frame MXF failure stays a failure of that encoder check. Adjacent timing spans of 12812 and 12813 are the floor rule, not a reason to reclassify that failure.
5. AVX, AAF, effects, colour, and plugins.
6. Persistent undo. Session undo remains memory only, under the already accepted ADR-009.
7. Relink, conform, and exchange.
8. A full NLE UI, a clicked-window test, and a confirmed native Wayland window.
9. A redistributable package. ADR-008 stays open. `LICENSE` stays empty.
10. Cross-platform and production validation.

## 3. Outstanding and unimplemented

Confirmed still open. This pass does not resolve any of them.

| Item | State |
| --- | --- |
| Deprecated `SQLite::SQLite3` | Outstanding. CMake names `SQLite3::SQLite3` as the replacement. The 18 developer warnings remain. The target was not changed |
| Final C++ standard | Outstanding. C++17 stays prototype-only. No accepted ADR sets a permanent standard |
| Native Wayland | Outstanding. The default window was not confirmed |
| Untested controls | Outstanding. Create, Open, Close, and the bin list were not clicked |
| Recovery beyond the controlled test | Outstanding. Autosave and the interrupted-transaction case are not a power-loss proof. Snapshots and journals were not built |
| FFmpeg library integration | Outstanding. The link harness did not configure. `ffmpeg-devel` was not installed. `libavfilter` is not approved. GStreamer is not a dependency |
| Version and ABI policy | Outstanding. The policy is written. It has not been applied to a pin |
| 29.97 MXF discrepancy | Outstanding. The historical failure stays a failure: the file carried 12813 samples for eight frames against the contract's 12812 from frame 0. The timing library's 12813 for `[8, 16)` does not reclassify that failure |
| OP-Atom stereo | Outstanding. Not supported on the tested build. Slice 0A did not add it |
| External relink and conform | Outstanding. Not tested. ADR-007 stays **PROPOSED** |
| Cross-platform validation | Outstanding. One Fedora machine only |

## 4. What is not debt

- The floor boundary is doing what the tests require. `[0, 8)` at 30000/1001 is 12812. `[8, 16)` is 12813. The 200000-frame walk is 320320000. A later span differing from the origin span is expected.
- CTest discovered one test and that test passed. The 18 results are Qt Test cases inside it.
- The XCB launch confirmed the title AETHER and a clean exit. It does not need to be re-described as a Wayland pass.
