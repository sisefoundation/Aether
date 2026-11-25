# Slice 0A project core validation

**SLICE 0A — COMPLETE AND ACCEPTED ON THE TESTED FEDORA MACHINE.**

Not cross-platform. Not production-ready.

**SLICE 0A COMPLETE — DEVELOPMENT FROZEN PENDING NEXT AUTHORISATION.**

The owner accepted this slice on 2026-11-20. The next slice is not authorised. No result below is inferred. Evidence is the logs under `validation-fixtures/slice-0a-2026-11-20/` and `validation-fixtures/slice-0a-2026-11-20-build-verification/`. Those logs were not overwritten by this closeout.

C++17 stays prototype-only. ADR-002, ADR-006, and ADR-007 stay **PROPOSED**. The 30000/1001 MXF encoder failure in `docs/validation/FFMPEG_MXF_ROUNDTRIP_TEST.md` is unchanged.

## 1. Authorisation

| Check | Status |
| --- | --- |
| Slice 0A authorised and then accepted by the owner | **PASSED** |
| Next slice, or any further implementation | **OUT OF SCOPE** |

The slice is Project Core and Persistence Feasibility: one project package, SQLite, bins, Media IDs and representations, master clips, a pure timing function, session-only undo, a documented autosave, an interrupted-transaction path, a minimal Qt window, and Qt Test. Git was not initialised. `/home/editry/development/sise_cutlist` was not used.

## 2. Packages

| Check | Status |
| --- | --- |
| Install of the two named development packages and their DNF requirements, with nothing removed | **PASSED** |
| FFmpeg development files, a system upgrade, or a distro sync | **OUT OF SCOPE** |

Log: `validation-fixtures/slice-0a-2026-11-20-build-verification/package-install.log`.

The inspection and the install each list **14 packages** and **remove 0 B**. The install log ends with `Complete!`.

Direct packages:

| Package | Version in the log |
| --- | --- |
| `qt6-qtbase-devel` | 6.11.2-3.fc45 |
| `sqlite-devel` | 3.53.4-1.fc45 |

DNF labelled the other twelve as dependencies. They are requirements of those two packages. They are not independent AETHER runtime requirements:

| Package | Version in the log |
| --- | --- |
| `qt6-qtbase-ibase`, `qt6-qtbase-mysql`, `qt6-qtbase-odbc`, `qt6-qtbase-postgresql` | 6.11.2-3.fc45 |
| `qt6-rpm-macros` | 6.11.2-1.fc45 |
| `sqlite` | 3.53.4-1.fc45 |
| `libpq` | 18.4-4.fc45 |
| `libfbclient2` | 5.0.4.1812-2.fc45 |
| `unixODBC`, `odbcinst-generate` | 2.3.14-36.fc45 |
| `vulkan-headers` | 1.4.350.0-4.fc45 |
| `vulkan-loader-devel` | 1.4.350.0-2.fc45 |

The install log does not list `ffmpeg-devel` and does not record `dnf upgrade` or `distro-sync`. It does record that a pending offline transaction from `dnf5daemon-server` was invalidated. This closeout did not reschedule it.

The earlier configure log, `validation-fixtures/slice-0a-2026-11-20/cmake-configure.log`, shows configure stopping because `sqlite-devel` and `qt6-qtbase-devel` were missing, and states that FFmpeg is not searched.

## 3. Configure

| Check | Status |
| --- | --- |
| Debug configure after the packages existed | **PASSED** |
| Configure fails closed when those development packages are missing | **PASSED** (earlier log) |

Command recorded for the successful run: `cmake -S /home/editry/development/sise-editor -B /home/editry/development/sise-editor/build -DCMAKE_BUILD_TYPE=Debug`.

Log: `validation-fixtures/slice-0a-2026-11-20-build-verification/cmake-configure.log`.

The log reports SQLite3 3.53.4, Qt6 6.11.2, prototype-only C++17, `Configuring done`, and `Build files have been written to: /home/editry/development/sise-editor/build`. It contains **18** `CMake Warning` lines. Each one says the target `SQLite::SQLite3` is deprecated and the replacement is `SQLite3::SQLite3`. That warning did not stop configure. The CMake file was not changed to clear it. See `docs/SLICE_0A_TECHNICAL_DEBT.md`.

Machine, from `environment.log`: Fedora Linux 45 (Workstation Edition), kernel `7.2.15-300.fc45.x86_64`, g++ 16.2.1 20261029 (Red Hat 16.2.1-3), CMake 4.3.0. `pkg-config` reports Qt 6.11.2 and SQLite 3.53.4. clang is installed and was not the compiler used.

## 4. Build and link

| Check | Status |
| --- | --- |
| Debug build of `aether` | **PASSED** |
| FFmpeg on the product link line | **OUT OF SCOPE**. The configure logs do not search it |

Log: `validation-fixtures/slice-0a-2026-11-20-build-verification/build.log`.

The log links `libaether_timing.a`, `libaether_persistence.a`, `libaether_session.a`, `libaether_ui.a`, `aether_slice0a_tests`, and `aether`. It ends with `Built target aether`. It contains no compiler `warning:` or `error:` line.

Executable: `build/aether`. Test binary: `build/aether_slice0a_tests`.

CMake found Qt 6.11.2 and SQLite 3.53.4 and the build completed those link lines. The product targets that use them are the ones in `CMakeLists.txt`: persistence links SQLite, the window links Qt Widgets, and the test binary links Qt Test. This closeout did not relink the binary.

## 5. Language and decisions

| Check | Status |
| --- | --- |
| C++17 kept as a prototype-only choice | **PASSED** |
| ADR-002, ADR-006, and ADR-007 left **PROPOSED** | **PASSED** |
| A permanent C++ standard, or acceptance of those three ADRs | **OUT OF SCOPE** |

ADR-001 remains accepted with conditions and does not name a C++ standard. ADR-003, ADR-004, ADR-005, and ADR-009 were already accepted and were not reopened. ADR-008 stays open. No ADR file was edited for this slice.

## 6. Project package, schema, and transactions

| Check | Status |
| --- | --- |
| Create, manifest, directory layout, reopen, stable project id, bad manifest, missing database, bad schema, transactions, foreign keys, uniqueness, and the second open | **PASSED** |
| Power-loss protection | **NOT TESTED** |

Evidence is `ProjectTests` in `validation-fixtures/slice-0a-2026-11-20-build-verification/timing-tests.log`: `createManifestLayoutAndReopen`, `projectIdSurvivesRenameCloseAndReopen`, `badManifestMissingDatabaseAndBadSchema`, `transactionsForeignKeysAndUniqueness`, and `secondOpenIsRefusedAndLogsOmitPaths` passed.

The create case checks the prototype layout (`project.manifest.json`, `project.db`, empty `BINS/`, `MEDIA/`, `CACHE/`, `BACKUP/`, `LOGS/`), eight tables, a refused second create, and an empty `MEDIA/` directory. Those names do not amend `docs/PROJECT_FOLDER_SPECIFICATION.md`. The undo case checks that table names do not contain `undo`, `sequence`, or `journal`.

## 7. Bins, media identity, and master clips

| Check | Status |
| --- | --- |
| Bin membership, move, reference, duplicate, and selected bin cleared on reopen | **PASSED** |
| Media ID, representation path change, and master clip still related after reopen | **PASSED** |
| `.abin` import or export, relink, conform, or exchange | **OUT OF SCOPE** |

Evidence: `binsMembershipMoveReferenceAndDuplicate` and `mediaIdentityAndMasterClipReopen` passed. The source covers create, list, rename, select, remove membership, move, a second membership of the same object, and a duplicate with a new object id and the same Media ID. Representation roles in source are original, proxy, transcode, and consolidated. No media file is written. No `.abin` code is present.

## 8. Timing

| Check | Status |
| --- | --- |
| Integer floor boundary, including unequal adjacent spans, the 200000-frame walk, and negative and overflow failures | **PASSED** |
| FFmpeg 30000/1001 MXF writer | **OUT OF SCOPE**. The earlier failure is not reclassified |

The rule in source is `sample_boundary(n) = floor(n * sample_rate * fps_denominator / fps_numerator)` and `sample_count = sample_boundary(end) - sample_boundary(start)`, in integer arithmetic. Negative positions and overflow fail closed.

`TimingTests` passed: `publishedBoundaries`, `eightFrameSpansAreNotAllEqual`, `adjacentRepeatedAndNonZeroStarts`, `longWalkMatchesBoundary`, and `negativeAndOverflowFailClosed`.

The independent driver in `timing-tests.log` printed:

- `[0, 8)` at 30000/1001, 48000 Hz: **12812**
- `[8, 16)` at 30000/1001: **12813**
- `[0, 8)` at 30/1: 12800
- frame 200000 boundary and the 200000-frame walk sum at 30000/1001: **320320000**
- negative position: error
- `timing-driver-ok`, driver exit 0

Adjacent spans that differ are the expected floor result. The test source also requires that difference, and the long walk at the other four rates (400000000, 384000000, 400400000, 320000000). The earlier driver log `validation-fixtures/slice-0a-2026-11-20/timing-driver.log` contains only `timing-driver-ok`. The integers above are from the build-verification log and from the passed test source.

## 9. Undo, autosave, and interrupted transactions

| Check | Status |
| --- | --- |
| Session undo and redo, stack cleared on reopen, last commit kept | **PASSED** |
| Autosave leaves the undo stack, and both interruption paths keep the last commit or report an integrity error | **PASSED** |
| Persistent undo | **OUT OF SCOPE** |
| Power loss, snapshot restore | **NOT TESTED** |

Evidence: `undoRedoResetsAndStaysIndependentOfSave` and `autosaveAndInterruptedTransaction` passed. Undo is memory only. `BACKUP/` is created empty. There is no snapshot writer.

## 10. Launch and the window

| Check | Status |
| --- | --- |
| `QT_QPA_PLATFORM=xcb`: window title AETHER, clean close, process exit 0 | **PASSED** |
| Default Wayland window | **NOT TESTED**. It was not confirmed |
| Create, Open, Close, and the bin list used as a person would | **NOT TESTED** |
| Monitors, timeline, playback, mixer, colour, export | **OUT OF SCOPE** |

Log: `validation-fixtures/slice-0a-2026-11-20-build-verification/application-launch.log`.

The default launch logged `AETHER start`, `wmctrl` listed no window, and the process was sent SIGTERM (`process_exit=143`). That is not a confirmed window.

The XCB launch logged `AETHER start`. `wmctrl` reported `0x01400007  0 36009  fedora AETHER`. After `wmctrl -c`, the process logged `AETHER stop` and `xcb_process_exit=0`. The buttons were not clicked. This is not a native Wayland result and it is not a functional UI test.

Startup and shutdown lines are in that log. `secondOpenIsRefusedAndLogsOmitPaths` passed, including the check that the representation path used by that case is absent from the log.

## 11. Automated tests

| Check | Status |
| --- | --- |
| CTest discovers one test and that test passes | **PASSED** |
| Qt Test cases inside that one CTest test | **PASSED**. 18 cases, 0 failed |

Log: `validation-fixtures/slice-0a-2026-11-20-build-verification/ctest.log`.

`ctest -N` lists `Test #1: slice0a` and `Total Tests: 1`, exit 0. `ctest --output-on-failure` reports `1/1 Test #1: slice0a` passed, `0 tests failed out of 1`, exit 0.

That one CTest test is the executable `aether_slice0a_tests`. Inside it, Qt Test reported:

- TimingTests: 7 passed, 0 failed (five functional cases plus init and cleanup)
- ProjectTests: 11 passed, 0 failed (nine functional cases plus init and cleanup)

18 Qt Test cases. This is not 18 CTest tests. The same totals are in `timing-tests.log`. Qt Test is the prototype framework. The test binary links Qt Test; the domain libraries do not link Qt. That exception does not change `docs/ENGINEERING_INTERFACES.md`.

## 12. Exclusions

| Item | Status |
| --- | --- |
| FFmpeg linking, decoding, proxies, MXF, playback, rendering | **OUT OF SCOPE** |
| AVX, AAF, effects, colour, plugins, the full NLE UI | **OUT OF SCOPE** |
| Sequence and timeline schema while ADR-002 is proposed | **OUT OF SCOPE** |
| Relink, conform, and exchange while ADR-007 is proposed | **OUT OF SCOPE** |
| Application joiner while ADR-006 is proposed | **OUT OF SCOPE** |
| Persistent undo | **OUT OF SCOPE** |
| Codec, external NLE, or cross-platform claims | **OUT OF SCOPE** |
| Redistributable package. `LICENSE` is empty. ADR-008 is open | **OUT OF SCOPE** |

`src/core/sequence/` was not created. The FFmpeg link harness under `tools/validation/ffmpeg_link_smoke_test/` was not added to the product build and was not edited by this closeout.

## 13. Acceptance and freeze

| Check | Status |
| --- | --- |
| Acceptance criteria that this machine can execute | **PASSED** |
| Owner acceptance of Slice 0A | **PASSED** |
| Further development | **OUT OF SCOPE** until a later authorisation |

**SLICE 0A COMPLETE — DEVELOPMENT FROZEN PENDING NEXT AUTHORISATION.**

The recorded limits are in `docs/SLICE_0A_TECHNICAL_DEBT.md`. They are not a licence to implement them.
