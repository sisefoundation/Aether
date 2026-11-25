# Slice 1A editorial sequence foundation

**PRODUCTION SLICE 1A COMPLETE — EDITORIAL SEQUENCE FOUNDATION IMPLEMENTED AND VALIDATED — PLAYBACK AND MEDIA PROCESSING NOT YET IMPLEMENTED — FUTURE SLICES REQUIRE SEPARATE AUTHORISATION.**

Validated on this Fedora machine only. Not production-ready. Not cross-platform. Not a playback engine.

The freeze was lifted only for this slice. Later slices stay **NOT STARTED** and **NOT AUTHORISED**. ADR-006 stays **PROPOSED**. ADR-007 stays **PROPOSED**. Refinements B and C were not implemented. No result below is inferred. Evidence is the logs under `validation-fixtures/slice-1a/`. Slice 0A and Slice 0B logs were not overwritten. `build/` and `build-slice0b/` were not deleted.

## 1. Objective

Persist a Sequence, its ordered Tracks, and Timeline instances of existing Master Clips. Store one reduced rational frame rate on the Sequence. Store a source range and a record range as distinct half-open int64 frame spans. Keep identity independent of display name and independent of a file path. This is not playback, decoding, mixing, or a visual timeline editor.

## 2. Authorisation scope

Authorised: the editorial Sequence foundation in the existing project database, session undo for these commands, a read-only inspection list on the existing window, and the tests and validation in this report.

Not authorised, and not done: another slice; `git init`, commit, or push; installing, removing, or upgrading packages; linking FFmpeg or GStreamer; playback, viewers, decoding, AETHER SOUND, mixing, proxies, thumbnails, waveforms, MXF, export, interchange, relink, conform, plugins, nested sequences, effects, transitions, speed, sample-accurate audio editing, or release packaging; accepting ADR-006 or ADR-007; editing the ADR-006 owner decision record or an accepted ADR; refinements B or C; promoting C++17; changing `SQLite3::SQLite3`; filling `docs/DEVELOPMENT_ROADMAP.md`.

## 3. Baseline

| Item | Record |
| --- | --- |
| Git | Not a repository. `git status` stopped with `fatal: not a git repository`. There is no `.git`. `git init` was not run |
| Branch | None |
| HEAD | None |
| Existing builds | `build/` is the Slice 0A tree. `build-slice0b/` is the Slice 0B tree. Both were left in place |
| New build | `build-slice1a/` |
| Unrelated work | No git repository, so there was no uncommitted branch to inspect. No unrelated tree was reset, stashed, or overwritten |

## 4. Doc-versus-source discrepancies

These were recorded from the files that were open. Docs were not assumed to match the code.

| Source | What it says | What the code did before this slice |
| --- | --- | --- |
| `include/aether/core/timing/sample_boundary.hpp` | The comment still says ADR-002 is **PROPOSED** | ADR-002 is **ACCEPTED — INITIAL EDITORIAL TIME AUTHORITY**. The floor function was left unchanged. The comment was not edited |
| `docs/SLICE_0A_TECHNICAL_DEBT.md` | The deprecated `SQLite::SQLite3` target is outstanding and Slice 0B has not started | Slice 0B already links `SQLite3::SQLite3`. That debt file was not edited |
| `docs/ENGINEERING_INTERFACES.md` and `docs/SYSTEM_ARCHITECTURE.md` | One sends EditCommand from the UI to Application. The other sends it from the UI to Editorial | The session remains the façade the window calls. This slice does not resolve that open routing note |
| `editorial_objects.kind` | The check allows only `master_clip` | Sequences, tracks, and instances are new tables. The kind check was not widened |
| Master clips and media assets | No frame-rate column | The source rate is stored on the instance with the source range. No media-rate column was added |
| Track type or role | Not present in the model | Omitted. No channel layout, bus, or Sound decision was added |
| `docs/SEQUENCE_SOURCE_MAPPING.md` | Describes a speed map on an instance | No speed column was added |
| Displayed out point | Inclusive versus exclusive stays deferred in the ADR-002 consequences | Internal ranges stay half-open `[start, end)` |
| Prototype strings | `kPrototypeVersion` remains `slice-0a` | The schema version in the manifest is the version tests check. The prototype string was not renamed |
| Journal mode | WAL is recommended and unmeasured in the storage note | The connection still uses DELETE. No WAL guarantee was added |

Source-time storage is settled by ADR-002 and `docs/decisions/ADR-002_IMPLEMENTATION_CONSEQUENCES.md`: the record range is int64 frames at the Sequence rate, the source range is int64 frames at the source rate, both are stored, and reopen reads both. It does not recompute one from the other and discard the stored one. That portion was not stopped.

## 5. Schema

Schema version moves from 1 to 2. The version-1 `CREATE` statements are unchanged. A new database runs those statements and then the three statements below, and inserts `schema_info.schema_version = 2`. Opening a version-1 database whose manifest version is also 1 runs the same three statements and the version update inside one transaction. Any other version is refused and the file is not modified. There is no second database, no undo table, no sample-time column, and no path column.

```sql
CREATE TABLE sequences (
  sequence_id TEXT PRIMARY KEY,
  project_id TEXT NOT NULL REFERENCES project_meta(project_id),
  name TEXT NOT NULL,
  rate_numerator INTEGER NOT NULL,
  rate_denominator INTEGER NOT NULL,
  created_utc TEXT NOT NULL,
  modified_utc TEXT NOT NULL,
  CHECK (rate_numerator > 0),
  CHECK (rate_denominator > 0)
);
CREATE TABLE tracks (
  track_id TEXT PRIMARY KEY,
  sequence_id TEXT NOT NULL REFERENCES sequences(sequence_id),
  ordinal INTEGER NOT NULL,
  created_utc TEXT NOT NULL,
  modified_utc TEXT NOT NULL,
  CHECK (ordinal >= 0),
  UNIQUE (sequence_id, ordinal)
);
CREATE TABLE timeline_instances (
  instance_id TEXT PRIMARY KEY,
  sequence_id TEXT NOT NULL REFERENCES sequences(sequence_id),
  track_id TEXT NOT NULL REFERENCES tracks(track_id),
  master_object_id TEXT NOT NULL REFERENCES master_clips(object_id),
  media_id TEXT NOT NULL REFERENCES media_assets(media_id),
  source_rate_numerator INTEGER NOT NULL,
  source_rate_denominator INTEGER NOT NULL,
  source_start INTEGER NOT NULL,
  source_end INTEGER NOT NULL,
  record_start INTEGER NOT NULL,
  record_end INTEGER NOT NULL,
  created_utc TEXT NOT NULL,
  modified_utc TEXT NOT NULL,
  CHECK (source_rate_numerator > 0),
  CHECK (source_rate_denominator > 0),
  CHECK (source_start >= 0),
  CHECK (source_end > source_start),
  CHECK (record_start >= 0),
  CHECK (record_end > record_start)
);
```

A fresh project has 11 tables. The original eight remain: `bin_membership`, `bins`, `editorial_objects`, `master_clips`, `media_assets`, `media_representations`, `project_meta`, `schema_info`. The added names, in the same alphabetical listing, are `sequences`, `timeline_instances`, and `tracks`.

Named rates and same-track non-overlap are enforced by the editorial check before the write. The SQL checks above are structural. They do not decide editorial meaning beyond the stored columns.

## 6. Implementation boundaries

Application (`ProjectSession`) coordinates the command and the session undo stack. Editorial rules live in `include/aether/core/sequence/sequence.hpp` and do not include SQLite. Media identity stays on the existing Media ID and Master Clip. An instance copies that Media ID. It does not change the record range from a path. Infrastructure (`ProjectStore`) owns the connection, the migration, and the physical write. The session does not issue unrestricted SQL. Processing jobs were not added. `SQLite3::SQLite3` remains the linked target. C++17 remains prototype-only.

The window lists sequences, tracks, and instances and labels the lists `Sequence inspection. Not playback.` There is no play control and no visual timeline editor. Buttons were not clicked in validation.

## 7. Commands

Supported through the session: create, read, and rename a Sequence; create Tracks and set their order; insert a Master Clip as an instance with explicit source and record ranges; move an instance by its record start while keeping the source range and the record duration; trim by storing both caller-supplied ranges; remove an instance. An invalid command returns an error and leaves the last committed rows in place. The overlap rejection test reads the earlier instance back unchanged.

Undo and redo for these commands stay in the session process. There is no undo database. Closing the project clears the undo stack. The last committed name is still there after reopen. Autosave is unchanged and still does not write the undo stack.

## 8. Time rules

Internal ranges are half-open `[start, end)`. Positions are int64 frames. There is no float-second argument, so a subframe is not rounded. Empty, negative, and reversed spans are rejected. Accepted reduced rates are 24/1, 25/1, 24000/1001, 30000/1001, and 60000/1001. An unreduced pair such as 48000/2002 is rejected.

The existing floor rule is unchanged. At 30000/1001 and 48 kHz the tests still report `[0, 8) = 12812`, `[8, 16) = 12813`, and `[0, 200000) = 320320000`. The MXF result of 12813 against 12812 stays a failed encoder test. Picture selection uses the accepted nearest-source rule: an exact tie stays on the earlier frame. The tested tie, sequence offset 480 at 24000/1001 against a 25/1 source, selects source frame 500. Audio phase remains the Sequence frame 0 floor. No speed, freeze, mixed-rate playback, or sample-accurate edit was implemented. Differing source and record frame counts are stored as the two explicit ranges. They are not a speed effect.

Same-track record ranges that share a frame are rejected. Abutting ranges such as `[0, 10)` and `[10, 20)` are accepted. That rejection is a limit so this slice does not invent compositing. It is not a complete timeline. Transitions, overlap policy, and compositing stay deferred.

## 9. Migration

| Case | Result |
| --- | --- |
| Fresh create | Schema version 2, 11 tables, Slice 0A project tests still pass |
| Version-1 project | A test seam drops the empty sequence tables and sets version 1, then reopen migrates. The bin, Media ID, and Master Clip remain. Sequence list is empty. Schema version becomes 2 |
| Unsupported version | The existing project test sets version 99. Open returns `MigrationRefused` and the version stays 99 |
| Armed migration failure | Open returns a database error, the file remains, the version stays 1, and a later open migrates without dropping the bin |
| Silent deletion | Not observed. A non-empty sequence schema refuses the downgrade seam. Failed migration rolls the transaction back |

The version-1 `CREATE` history was not rewritten.

## 10. Clean build

`build/` and `build-slice0b/` were not reused and were not removed.

```text
cmake -S /home/editry/development/sise-editor -B /home/editry/development/sise-editor/build-slice1a -DCMAKE_BUILD_TYPE=Debug
cmake --build /home/editry/development/sise-editor/build-slice1a --parallel
```

| Step | Result |
| --- | --- |
| Configure | Exit 0. Log: `validation-fixtures/slice-1a/cmake-configure.log`. It reports SQLite3 3.53.4, `Slice 0B links imported target SQLite3::SQLite3`, Qt6 6.11.2, prototype-only C++17, `Configuring done`, and `Generating done`. It contains no `CMake Warning` line |
| Build | Exit 0. Log: `validation-fixtures/slice-1a/build.log`. It ends with `Built target aether`. It contains no compiler `warning:` or `error:` line |

The link lines for `aether` and `aether_slice0a_tests` include `/usr/lib64/libsqlite3.so`. FFmpeg is not on those lines.

Environment log: `validation-fixtures/slice-1a/environment.log`. Fedora Linux 45, kernel 7.2.15-300.fc45.x86_64, g++ 16.2.1 20261029 (Red Hat 16.2.1-3), CMake 4.3.0, qt6-qtbase-devel 6.11.2-3.fc45, sqlite-devel 3.53.4-1.fc45, Qt6Widgets 6.11.2, sqlite3 3.53.4, `WAYLAND_DISPLAY=wayland-0`, `DISPLAY=:0`, `XDG_SESSION_TYPE=wayland`, `wmctrl` present. No package was installed or removed.

## 11. CTest

Log: `validation-fixtures/slice-1a/ctest.log`.

`ctest -N` lists `Test #1: slice0a` and `Total Tests: 1`. `ctest --output-on-failure` reports `1/1 Test #1: slice0a` passed, `100% tests passed, 0 tests failed out of 1`.

That is one CTest test. It is not 28 CTest tests.

## 12. Qt Test

Log: `validation-fixtures/slice-1a/qt-test.log`. The same executable was run directly.

| Class | Totals line |
| --- | --- |
| TimingTests | 7 passed, 0 failed, 0 skipped, 0 blacklisted |
| ProjectTests | 11 passed, 0 failed, 0 skipped, 0 blacklisted |
| SequenceTests | 10 passed, 0 failed, 0 skipped, 0 blacklisted |

28 Qt Test cases, 0 failed. Direct exit 0. These 28 cases are inside the one CTest test named `slice0a`. Slice 0A and Slice 0B cases were not removed. The project test now expects schema version 2 and 11 tables, and it still rejects an undo table and a journal table.

SequenceTests covers the model, insert/move/trim/remove and overlap rejection, reopen of both ranges, path and second-representation identity, two instances of one Master Clip, the picture tie and the published sample counts, undo across close/reopen, version-1 migration, and migration rollback.

## 13. Smoke test

Log: `validation-fixtures/slice-1a/application-launch.log`.

Buttons and menus were not used. The new lists were not clicked.

| Run | What the log shows |
| --- | --- |
| `QT_QPA_PLATFORM=wayland --shutdown-now` | `AETHER start`, `AETHER stop`, exit 0 |
| `QT_QPA_PLATFORM=wayland` without the shutdown flag | `AETHER start`. `wmctrl` did not list AETHER. The process was killed and the wait status was 143. A native Wayland window was not confirmed |
| `QT_QPA_PLATFORM=xcb --shutdown-now` | `AETHER start`, `AETHER stop`, exit 0 |
| `QT_QPA_PLATFORM=xcb` without the shutdown flag | `wmctrl` listed `0x01200007  0 fedora AETHER`. After `wmctrl -c`, the log shows `AETHER stop` and `xcb_window_close_exit=0` |

The XCB window is not a native Wayland result. Wayland and XCB differ: only the XCB probe listed a window.

## 14. Remaining debt

- Native Wayland window: not confirmed.
- Window controls and the new inspection lists: not clicked.
- C++17: still prototype-only.
- ADR-006 and ADR-007: still **PROPOSED**. Refinements A, B, and C were not accepted. B and C were not implemented.
- `sample_boundary.hpp` still describes ADR-002 as proposed.
- `docs/SLICE_0A_TECHNICAL_DEBT.md` still describes the Slice 0B target correction as not started.
- FFmpeg is not linked. The MXF 12813-against-12812 result stays a failed encoder test.
- Track type, channel layout, routing, and buses are absent.
- Power-loss protection and cross-platform validation were not run.
- The prototype version string still says `slice-0a`.

## 15. Deferred work

Same-track overlap rejection is a tested limit, not a finished timeline. Transitions, compositing, nesting, speed, freeze frames, mixed-rate playback, sample-accurate audio editing, and the displayed inclusive out point stay deferred. Playback and media processing are not implemented. Every later slice stays **NOT STARTED** and **NOT AUTHORISED**.

## 16. Final status

| Check | Status |
| --- | --- |
| Persistent Sequence, Track, and instance model | **PASSED** |
| Source and record ranges both stored and reread | **PASSED** |
| Identity independent of path and of a second representation | **PASSED** |
| Version-1 migration, refusal, and rollback | **PASSED** |
| Session undo, with the committed edit surviving close | **PASSED** |
| Published sample counts and the earlier-frame tie | **PASSED** |
| Clean Debug configure and build in `build-slice1a` | **PASSED** |
| CTest: 1 test, 1 passed | **PASSED** |
| Qt Test: 28 cases, 0 failed | **PASSED** |
| Startup and clean exit on Wayland and XCB `--shutdown-now` | **PASSED** |
| Native Wayland window | **NOT CONFIRMED** |
| XCB window close | **PASSED**. This is not a native Wayland result |
| Playback or media processing | **NOT IMPLEMENTED** |
| Production readiness or cross-platform validation | **NOT CLAIMED** |

**PRODUCTION SLICE 1A COMPLETE — EDITORIAL SEQUENCE FOUNDATION IMPLEMENTED AND VALIDATED — PLAYBACK AND MEDIA PROCESSING NOT YET IMPLEMENTED — FUTURE SLICES REQUIRE SEPARATE AUTHORISATION.**
