# Slice 0A implementation plan — project core and persistence feasibility

Status: **SLICE 0A COMPLETE — DEVELOPMENT FROZEN PENDING NEXT AUTHORISATION.** The owner accepted the slice on 2026-11-20. Evidenced tasks are marked in section 18. Excluded work stays incomplete. This is not authorisation for the rest of the Foundation Prototype, and it does not accept ADR-002, ADR-006, or ADR-007. C++17 stays prototype-only.

The slice is Project Core and Persistence Feasibility. It is a local feasibility check of the Qt and SQLite conditions in ADR-001, plus the accepted editorial rules those conditions need. It is not a redistributable package. ADR-008 stays open.

## 1. Repository status

Inspected `/home/editry/development/sise-editor` before any Slice 0A source was added.

| Item | State at inspection |
| --- | --- |
| Git | Not a repository. This slice does not run `git init` |
| Root `CMakeLists.txt` | Empty. It is the product scaffold and this slice fills it |
| `src/` | Empty directories only. No application |
| `tests/` | Empty |
| `LICENSE` | Empty. Left empty |
| `third_party/` | Empty. Nothing is vendored |
| `tools/validation/ffmpeg_link_smoke_test/` | Isolated harness. Left untouched. The product CMake file does not add it |
| FFmpeg 29.97 MXF report | Not rewritten. The encoder failure stays open |

`/home/editry/development/sise_cutlist` is not read or modified.

Accepted and binding for this slice:

- C++, Qt 6, CMake, SQLite. FFmpeg is a future mandatory media framework and is not linked or called.
- One authoritative `project.db` plus a manifest. The manifest is package metadata only.
- Bins are editorial containers. No `.abin` import or export.
- One stable Media ID per logical asset. Original, proxy, transcode, and consolidated are representations with their own Representation IDs. No media files are generated.
- Session-only undo. Autosave and recovery are separate. Undo history does not survive close and reopen. A committed command does not need the undo stack to stay saved.

## 2. Machine and dependencies

Measured on this Fedora machine. The first pass installed nothing, and configure failed closed while the development files were absent. A later authorised install of `qt6-qtbase-devel` and `sqlite-devel` completed, including the required dependencies recorded in the validation file. Sudo succeeded for that transaction. The versions below are the state after that install.

| Item | Measured |
| --- | --- |
| OS | Fedora Linux 45 (Workstation Edition), kernel `7.2.15-300.fc45.x86_64` |
| Compiler | `g++` (GCC) 16.2.1 20261029 (Red Hat 16.2.1-3). `clang` 23.1.4 is installed and is not the compiler this slice uses |
| CMake | 4.3.0 |
| Qt 6 runtime | `qt6-qtbase-6.11.2-3.fc45` and `qt6-qtbase-gui-6.11.2-3.fc45` |
| Qt 6 development files | `qt6-qtbase-devel-6.11.2-3.fc45`. `Qt6Config.cmake` is present |
| Qt Test | Provided by `qt6-qtbase-devel`. The test binary reports QtTest 6.11.2 |
| SQLite runtime | `sqlite-libs-3.53.4-1.fc45` provides `/usr/lib64/libsqlite3.so.3.53.4` |
| SQLite development files | `sqlite-devel-3.53.4-1.fc45`. `/usr/include/sqlite3.h` is present. The `sqlite` command-line package was installed with it |
| Other test frameworks | Not searched as substitutes. Catch2 and GoogleTest are not adopted |

The runtime libraries are not sufficient. CMake must fail with a message that names `qt6-qtbase-devel` and `sqlite-devel` when those development files are missing. This slice does not vendor Qt or SQLite, does not switch UI toolkits, and does not switch databases.

The first product configure stopped, as required, while the development files were missing. After they were installed, configure, the Debug build, `ctest`, and the `QT_QPA_PLATFORM=xcb` launch completed. The commands and exits are in `docs/validation/SLICE_0A_PROJECT_CORE_VALIDATION.md`. CMake also warns that the target name `SQLite::SQLite3` is deprecated in favour of `SQLite3::SQLite3`. That warning did not fail the build.

## 3. C++ standard

ADR-001 accepts C++ as the language. It does not name a standard. No later accepted ADR names one.

**Prototype-only choice:** C++17. `CMAKE_CXX_STANDARD` is 17. This is not a permanent standard. A review can move it when an ADR actually sets one. The code does not use C++20 library features (`std::format`, `std::span`, designated initializers).

## 4. Source layout

New code uses these directories. Pre-existing empty directories (`src/engines/`, `src/ui/timeline/`, `src/ui/viewer/`, `src/database/`, and the rest of the old scaffold) stay empty. This slice does not fill them and does not delete them.

```text
CMakeLists.txt                          product scaffold for this slice
src/core/timing/                        sample_boundary only
src/core/project/                       result type, id mint, project record
src/core/bin/                           bin and membership records
src/core/media/                         asset, representation, master clip records
src/persistence/                        SQLite, manifest, log, schema
src/app/                                project session, command undo
src/ui/                                 Qt window and process entry
tests/                                  Qt Test sources
docs/validation/SLICE_0A_IMPLEMENTATION_PLAN.md
docs/validation/SLICE_0A_PROJECT_CORE_VALIDATION.md
```

There is no `src/core/sequence/`. There is no FFmpeg wrapper, no proxy encoder, no MXF reader, no playback module, and no undo table.

## 5. Modules and dependencies

Link direction. A box does not include another box's third-party headers.

| Target | May include | Links | Must not link |
| --- | --- | --- | --- |
| `aether_timing` | C++ standard library | nothing | Qt, SQLite, FFmpeg |
| `aether_persistence` | timing is not required; model headers; `sqlite3.h` | `SQLite::SQLite3` | Qt, FFmpeg |
| `aether_session` | model headers, persistence, timing is not required | `aether_persistence` | Qt, FFmpeg |
| `aether_ui` | session public header, Qt Widgets | `aether_session`, `Qt6::Widgets` | SQLite headers, FFmpeg |
| `aether` | `main.cpp` | `aether_ui` | FFmpeg |
| `aether_slice0a_tests` | session, persistence, timing, Qt Test | `Qt6::Test`, `aether_session`, `aether_timing` | `aether_ui`, FFmpeg |

`aether_session` is a prototype façade so the window can call services. It is not an acceptance of ADR-006. It does not build picture requests, audio graphs, or exchange documents.

Domain headers do not include `sqlite3.h`, Qt headers, or FFmpeg headers. The UI does not open the database and does not own the schema.

`tools/validation/ffmpeg_link_smoke_test/` is not a subdirectory of the product build.

### Prototype exception recorded against the interface note

`docs/ENGINEERING_INTERFACES.md` says the Foundation Prototype test targets link neither Qt nor a codec. This slice's test executable links Qt Test, because the owner allowed Qt Test when Qt is the chosen UI framework and forbade substituting another framework. The timing and session libraries still do not link Qt. The test executable does not link a codec. This exception is prototype-specific. It does not edit the interface document.

## 6. Unresolved ADRs

| ADR | Status | Effect on this slice |
| --- | --- | --- |
| ADR-001 | Accepted with conditions | Language, Qt 6, CMake, and SQLite are in scope. FFmpeg stays unlinked. The build is a feasibility check, not production validation. The C++ standard is not set by this ADR |
| ADR-002 | **PROPOSED.** Not approved | Blocks a permanent sequence and timeline schema. See section 8 |
| ADR-003 | Accepted | One `project.db`, manifest, foreign keys, transactions, lock, no silent recreate of a bad database |
| ADR-004 | Accepted as interchange | No `.abin` reader or writer |
| ADR-005 | Accepted | One Media ID, separate Representation IDs |
| ADR-006 | **PROPOSED.** Not approved | No processing joiner and no sound library. The session façade is not this ADR |
| ADR-007 | **PROPOSED.** Not approved | No relink engine, no conform report, no exchange. A representation path edit is a column update, not a relink result and not a conform |
| ADR-008 | Open, pending counsel | No licence text, no redistributable package, no install rules |
| ADR-009 | Accepted for the Foundation Prototype | Session-only undo |

No ADR is moved to accepted by this work.

## 7. Project package

Prototype layout under a directory the caller creates as the project root. This is not `docs/PROJECT_FOLDER_SPECIFICATION.md` and does not amend it. The folder specification's `project_manifest.json` name and its larger tree stay as written. `BINS/` remains the open folder-spec question in `docs/PROJECT_STORAGE_AND_DATABASE.md`. This slice creates an empty `BINS/` directory because the owner named it in the prototype layout. The directory is not a bin and is not searched for editorial state.

```text
PROJECT/
  project.manifest.json
  project.db
  project.lock
  BINS/
  MEDIA/
  CACHE/
  BACKUP/
  LOGS/
```

`project.lock` is the one-writer lock (`flock`, non-blocking exclusive). It is not the undo stack and it is not a licence. A second open is refused. The kernel drops the lock if the process dies.

### Manifest

UTF-8 JSON, one flat object. Package metadata only. Keys:

| Key | Role |
| --- | --- |
| `project_id` | Stable id. Rename of the directory, close, and reopen do not mint a new one |
| `name` | Project display name |
| `format_version` | Integer 1 for this prototype package |
| `database_filename` | `project.db` |
| `schema_version` | Integer 1. Must match `schema_info` in the database |
| `created` | UTC timestamp from creation. Rewrites keep it |
| `modified` | UTC timestamp of the last committed project row the manifest writer saw |
| `prototype_version` | `slice-0a` |
| `app_version` | `0.0.0-slice-0a` |

The writer escapes strings. The reader accepts only this flat object, requires every key, and rejects a second value that would smuggle editorial rows into the file. Unknown keys are rejected so the manifest cannot grow a shadow database by accident.

Replacement is a temp file in the same directory, `fsync`, then `rename`. The database transaction commits before that replacement.

### Open rules

1. Parse the manifest. A bad file is an error. The database is not created to replace it.
2. Require `project.db` to exist already. A missing file is an error. It is not created. SQLite is opened with read/write and without the create flag.
3. Take the lock. If it is held, refuse.
4. `PRAGMA foreign_keys = ON` on that connection, then read the pragma back and refuse if it is off.
5. `PRAGMA integrity_check` must be a single `ok`.
6. `PRAGMA foreign_key_check` must return no rows.
7. Schema version in the database must match the manifest. The migration hook is one function. It allows only version 1. Any other version returns an error and does not write. There is no migration framework and no second version.
8. Project id in the database must match the manifest. A mismatch is an error. Neither file is deleted and neither is rewritten to force a winner.
9. `database_filename` must be `project.db`. `format_version` must be 1.

A lag between `modified` in the manifest and `modified` in `project_meta` is the crash window ADR-003 already names (commit succeeded, manifest rename did not). Open logs it and does not rewrite the manifest. The database remains the editorial authority. The next explicit command or `autosave()` rewrites the manifest. That is not a silent merge of two editorial documents. The manifest holds no bins, clips, or sequences.

`name` is treated the same way: the open project uses the database row, logs a manifest stamp difference, and does not rewrite until the next explicit save. Identity and schema mismatches still refuse.

Failed create: the directory is removed only when this call created it and has not returned success. Open never deletes a database, including one that fails integrity or schema checks.

## 8. Schema

One migration series, schema version 1, prototype tables only. The SQL is not generated from `docs/PROJECT_DATABASE_SCHEMA.md` and `docs/BIN_DATABASE_SCHEMA.md`. Those documents still overlap. Using both would be risk R-I.

Tables:

| Table | Holds |
| --- | --- |
| `schema_info` | Singleton schema version |
| `project_meta` | Project id, name, created, modified |
| `bins` | Bin id, project id, name, timestamps |
| `editorial_objects` | Object id, project id, kind `master_clip` only, name, opaque metadata text, timestamps |
| `bin_membership` | Bin id, object id, created. Primary key is the pair |
| `media_assets` | Media ID, project id, type string, opaque metadata, timestamps |
| `media_representations` | Representation ID, Media ID, role, recorded path, status, timestamps |
| `master_clips` | Object id, Media ID |

Roles are `original`, `proxy`, `transcode`, `consolidated`.

Journal mode is DELETE, set at create. WAL stays the unmeasured recommendation in the storage document. `synchronous=FULL` is set on each connection. That is SQLite's ordinary full sync for this journal mode. It is not a power-loss certificate.

There is no undo table, no operation journal, and no snapshot writer. `BACKUP/` is an empty directory.

### Sequence and timeline — not stored

ADR-002 is **PROPOSED**. It is the decision that would make rational rates and integer frame indexes the stored authority. `docs/04_CORE_DATA_MODEL.md` lists sequence, track, and timeline-instance fields as **PROPOSED**. Putting those tables in `project.db` would publish a sequence design while that ADR is unapproved.

This slice does not create those tables and does not invent stand-in columns "for now" that a later reader could mistake for the timeline engine.

The independently safe part is the pure timing function in section 9. It stores nothing. It is marked prototype-only until ADR-002 is accepted. ADR-006 and ADR-007 do not add a sequence schema; they are also unapproved, so this slice does not implement a joiner, a relink engine, or exchange.

## 9. Timing

`sample_boundary(n) = floor(n * sample_rate * fps_denominator / fps_numerator)` for `n >= 0`.

`sample_count(start, end) = sample_boundary(end) - sample_boundary(start)` for `end >= start`, both non-negative.

Exact integer arithmetic. No `double`, no `float`. The product uses a wide unsigned multiply that fails closed on overflow. A negative position is an error. The timebase documents reject a negative stored position because floor and toward-zero disagree. A zero or negative rate is an error. An inverted span is an error.

The function is the span form already stated in `docs/TIMEBASE_AND_FRAME_ACCURACY.md` and checked in `docs/validation/AUDIO_FRAME_SAMPLE_BOUNDARY_VALIDATION.md`. At 48000 Hz and 30000/1001, frames `[0, 8)` are 12812 samples. A later eight-frame span may be 12813. The tests assert that difference. They do not stamp 12812 onto every eight-frame span.

This library does not call FFmpeg. The MXF writer result of 12813 samples for `[0, 8)` stays a failed encoder check in the existing report.

## 10. Bins, media, master clips

Services, all in one transaction when they change rows:

| Operation | What changes | What does not change |
| --- | --- | --- |
| Create bin | New Bin ID | Project ID |
| Rename bin | Name and bin `modified` | Bin ID |
| Logical select | In-memory selected Bin ID | Database |
| Add membership | One membership row | Object id, Media ID |
| Remove membership | That membership row | Media asset, representations, the editorial object |
| Move | Membership leaves one bin and enters another | Object id, Media ID |
| Reference | A second membership of the same object id | Object id, Media ID. No `.abin`, no share folder |
| Duplicate master clip | New object id, same Media ID, copied name and metadata, one membership | Media ID, representation files (none exist) |

Removing the last membership leaves the object in the project. This slice has no command that deletes a Media Asset. Complex sharing (portable packages, collaboration, share folders) is not implemented.

A master clip cites one Media ID. A representation path update changes that representation's path and its `modified` time. It does not change the Media ID or the Representation ID. The file at the path is not required to exist and is not created.

## 11. Undo

In-memory stack on the session. Inverse commands:

- bin rename
- membership add (including reference, which is a membership add)
- basic metadata (object name and the opaque metadata string)

Each inverse runs as its own later transaction. Redo is the forward transaction. A failed inverse leaves the stack and the database as they were.

Create, move, duplicate, and remove-membership commit, and they are not undo steps in this slice. The stack is cleared on close. Reopen starts empty. There is no undo table.

Closing after a rename, without undo, keeps the new name. The committed row does not consult the stack.

## 12. Autosave and interrupted transactions

Autosave is `ProjectSession::autosave()`. It fsyncs the committed database file and rewrites the manifest from the committed project row. It does not read or clear the undo stack. It does not copy `MEDIA/`. It does not write `BACKUP/`. It is not crash recovery and it is not a snapshot.

An interrupted transaction is an uncommitted `BEGIN` plus an insert, then either a connection close without `COMMIT` or a child process `_exit` while the transaction is open. The next open runs the integrity check. The expected result is the last commit, with the uncommitted insert absent, or an explicit error that leaves the file in place. This does not kill unrelated processes. It does not claim the machine survives a power cut.

## 13. UI

One Qt window. Title `AETHER`. It shows the project name and a status line, and it offers Create, Open, and Close. The bin list is read-only. Choosing a row calls logical select. The window calls `ProjectSession` only.

No monitors, timeline, playback, mixer, colour, or export. `--shutdown-now` constructs the window, logs startup, and quits. The confirmed launch on this machine used `QT_QPA_PLATFORM=xcb`: `wmctrl` reported the title AETHER, the window was closed, and the process exited 0. The default Wayland window was not confirmed. The buttons were not clicked.

## 14. Logging

Categories: `startup`, `shutdown`, `project`, `database`, `transaction`, `schema`, `manifest`, `autosave`, `recovery`.

Startup and shutdown go to stderr. While a project is open, the same lines also go to `LOGS/aether.log`. Messages name the operation, the project display name, and ids. They do not include representation paths. A failure to open the log file does not fail the project open.

## 15. Test strategy

Framework: **Qt Test**, prototype-specific, because Qt 6 is the accepted UI framework and `qt6-qtbase-devel` is the package that provides Qt Test. No other framework is added. On this machine those tests ran after the development packages were installed. The result is in the validation file: one `ctest` entry passed, TimingTests reported 7 passed, and ProjectTests reported 11 passed.

Intended cases, in `tests/`:

| Area | Cases |
| --- | --- |
| Timing | 24000/1001, 24/1, 25/1, 30000/1001, 30/1 at 48000 Hz. Zero and non-zero starts. Eight-frame spans. Adjacent and repeated spans. Long positions including frame 107892 and frame 200000. A 200000-frame walk whose sum equals the boundary. `[0, 8)` at 30000/1001 is 12812 and `[8, 16)` is 12813. Negative positions error. Overflow errors. No floating-point type in the timing translation unit |
| Project | Create, manifest keys, directory layout, open, close, reopen. Project id stable across directory rename, close, and reopen. Bad manifest. Missing database is not recreated. Bad schema version is left in place. Manifest and database project-id mismatch refuses |
| Transactions | Commit, rollback on a foreign-key failure, integrity check, foreign-key check, unique ids, second open refused |
| Bins | Create, list, rename, select, add, list objects, remove membership, move, reference, duplicate |
| Media | Asset plus original and proxy, same Media ID, proxy path change, close, reopen, ids unchanged |
| Master clip | Bin, clip, asset, and representation still related after reopen |
| Undo | Undo and redo for rename, membership add, and metadata. Stack empty after reopen. Last commit present when the stack was discarded |
| Autosave | Create, change, autosave, undo still available, close, reopen, change present, stack empty |
| Interruption | Abandoned transaction on an isolated project. Child `_exit` mid-transaction. Reopen, integrity, last commit or a clear error. Database file still present |
| Schema shape | The eight tables above, and no sequence table, undo table, or journal table |

An extra `g++` compilation of the timing translation unit is allowed as a side check because that file has no Qt and no SQLite include. It is not the Qt Test run and it is not recorded as the suite passing. The product bar is `ctest`.

## 16. Acceptance criteria

The slice is acceptable as a feasibility attempt only if all of the following are true in the validation record:

- The plan and the validation file exist and do not mark an unrun test as passed.
- Root CMake configures only when Qt 6 and SQLite development files exist, and otherwise stops with a message that names the missing Fedora packages.
- FFmpeg is not on the link line. The smoke-test harness is unchanged.
- No ADR in the proposed set is edited into accepted.
- No sequence schema is presented as the timeline engine.
- The timing function matches the published integer contract, including a 12813-sample span that is not the origin span.
- Session undo is absent from the database.
- The validation record states this machine's configure, compile, test, and launch results from commands that were actually run.

On this machine those bars were met after the authorised install. The acceptance record is `docs/validation/SLICE_0A_PROJECT_CORE_VALIDATION.md`: Debug configure completed, the build completed, CTest discovered one test and it passed, and an XCB window titled AETHER closed with process exit 0. The owner accepted that result. Meeting it does not authorise the next slice.

## 17. Limitations

- Not the Foundation Prototype as a whole. Items in `docs/FIRST_IMPLEMENTATION_MILESTONE.md` that need FFmpeg, inspection, proxy generation, AVX, or a permanent timeline stay out.
- Not production-validated. One Fedora machine. No cross-platform claim.
- Not a power-loss proof, not a snapshot restore, and not persistent undo.
- Not relink, conform, or exchange.
- Not `.abin`.
- The 30000/1001 MXF encoder failure is untouched.
- Qt distribution and the project licence stay with ADR-008.
- `BINS/` and `project.manifest.json` are prototype names. They do not revise the folder specification.
- C++17 is prototype-only.
- The empty pre-existing source directories are not a module map.
- Development is frozen. The next slice is not authorised.

## 18. Closeout against this plan

**SLICE 0A COMPLETE — DEVELOPMENT FROZEN PENDING NEXT AUTHORISATION.**

The owner accepted the slice. Statuses use the validation record. A task is complete only where that record has a pass. Excluded work is not complete.

| Plan area | Closeout |
| --- | --- |
| Root CMake, Qt 6, and SQLite, including the closed failure when the development packages are missing | **Complete** on this Fedora machine. Configure and the build completed. The deprecated `SQLite::SQLite3` target is debt, not a failure, and the CMake file was not changed |
| C++17 | **Complete** as a prototype-only choice. A permanent standard is **incomplete** and was not set |
| Project package, manifest, eight-table schema, transactions, lock | **Complete** in the Qt Test cases that passed |
| Bins, Media ID, representations, master clips | **Complete** in the Qt Test cases that passed. No media file is generated |
| Timing function and the published integers, including 12812, 12813, and 320320000 | **Complete**. Adjacent spans that differ are expected |
| Session undo, autosave, interrupted transaction | **Complete** for the session-only behaviour the tests cover. Power-loss protection is **incomplete** and was not tested |
| Minimal window | **Complete** for a built executable whose XCB window title is AETHER and which closed with exit 0. Native Wayland and clicked controls are **incomplete** |
| Logging without representation paths | **Complete** for the launch lines and the passed log case |
| CTest and Qt Test | **Complete**. CTest: 1 discovered, 1 passed. Qt Test inside it: 18 cases, 0 failed |
| FFmpeg, proxies, MXF, playback, rendering | **Incomplete.** Out of scope. The earlier MXF failure stays a failure |
| Sequence and timeline schema | **Incomplete.** ADR-002 stays **PROPOSED** |
| ADR-006 joiner and ADR-007 relink, conform, and exchange | **Incomplete.** Both stay **PROPOSED** |
| Persistent undo, `.abin`, plugins, effects, colour, AVX, AAF, full NLE UI | **Incomplete.** Out of scope |
| Redistributable package and a filled `LICENSE` | **Incomplete.** ADR-008 stays open |
| Next slice | **Incomplete.** Not authorised |
