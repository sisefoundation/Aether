# Slice 1C — media import and project bin

**SLICE 1C COMPLETE — MEDIA IMPORT AND PROJECT BIN IMPLEMENTED AND VALIDATED — PLAYBACK AND MEDIA PROCESSING NOT YET IMPLEMENTED.**

This authorises Slice 1C only. Later slices stay **NOT STARTED** and **NOT AUTHORISED**. There is no git repository. `git init` was not run. No package was installed. The C++ standard and the `SQLite3::SQLite3` link were not changed. ADR-006 was not marked accepted. Accepted ADR files were not edited.

## Repository Inspection

The tree at `/home/editry/development/sise-editor` is not a git repository. `build/`, `build-slice0b/`, `build-slice1a/`, and `build-slice1b/` were left in place. The new tree is `build-slice1c/`.

Slice 1A stores sequences, tracks, and timeline instances in schema version 2. Slice 1B paints those records and calls the existing move and trim commands. `ProjectSession` already created media assets, representations, and master clips. `media_assets.media_id`, `media_representations.representation_id`, and `media_representations.recorded_path` are separate columns. There is no unique constraint on `recorded_path`. Bins hold editorial objects through `bin_membership`. A bin is not a folder. `createProject` does not insert a bin; sequence tests still expect the bins they create themselves.

ADR-003 is one `project.db`, one transaction per command, and a manifest rewrite after commit. ADR-005 is one stable Media ID, a separate Representation ID, and a path that is neither of those IDs. The same logical asset must not mint a second Media ID once sameness is known. If sameness cannot be established, ADR-005 says the case is a candidate for a person, not a silent replacement. No code policy merged an exact path. This slice does not add a fingerprint or a merge.

The window already had Create, Open, Close, a bin list, and the timeline workspace. The inspector for media did not exist. `docs/DEVELOPMENT_ROADMAP.md` is still an empty file and was not filled.

## Product Architecture

Import is a reference registration. The source file is not copied, moved, or rewritten. The project still has one `project.db`. There is no bin database.

Schema version stays 2. Size, modification time, and online or offline are read from the file when the bin is listed. They are not identity columns. Technical metadata is not invented. The inspector says `Not probed` because nothing in this slice writes a probe into `metadata_json`.

A new import writes, in one transaction, a media asset, one original representation with status `registered`, a master clip, and membership in the Root bin. `registered` is the stored registration status. `online` and `offline` are the live file check. Taking a file away does not change the stored status and does not relink the path.

The window includes the session, not the store, and it has no SQL entry point.

## Project Bin Implementation

`ensureRootBin` finds a bin named Root or inserts one. `createProject` does not call it, so existing tests that count only their own bins stay valid. The window calls `ensureRootBin` when it refreshes an open project, then selects Root.

The asset list shows the file name and the live presence. An empty bin says `This bin has no media.` With no project open, the label says `No project is open.` Selecting a row fills the inspector with the file name, Media ID, Representation ID, object id, recorded path, size, UTC modification time, presence, registration status, and technical metadata.

## Media Import Workflow

The Import Media button calls `QFileDialog::getOpenFileNames`. The filter names the picture and sound extensions already used as `media_type` tokens: picture for `.mov`, `.mp4`, `.mxf`, `.avi`, `.mkv`, `.mpg`, `.mpeg`, and `.m4v`; sound for `.wav`, `.aif`, `.aiff`, `.mp3`, `.flac`, `.aac`, and `.m4a`. The extension check is case-folded. Any other regular file is an unknown type and is refused. A directory is invalid. A missing path is refused. A regular file that cannot be opened is unreadable. An empty path is invalid.

Every path is inspected before any write. One refusal refuses the whole request. The message says that no records were written. Drag-and-drop was not added, so it does not delay the picker.

The running XCB `aether` window was driven after the first write of this report. Import Media was invoked. The native file chooser opened with the title Import Media and the filter Media references. The fixture file `picture.mov` was chosen in that dialog. The earlier sentence that said the dialog was not opened is withdrawn.

## Media Identity and Persistence

A Media ID is a new stable id. It is not the path. The Representation ID is a different id. Changing the recorded path through the existing path command keeps the Media ID, the Representation ID, and the object id. Close and reopen return those same ids.

An exact recorded path, including a lexical form such as a path with `/../`, is reported as already registered. Nothing is merged and nothing is written for that request. Two different paths with the same byte size are not treated as the same asset, because this slice cannot establish that they are the same logical asset. Each gets its own Media ID. That is not a new identity rule and it is not a fingerprint match. A person is not asked to confirm a candidate.

A batch that fails inspection never starts the write. A batch that hits an existing path, or the armed database failure, uses one transaction and rolls back. After that failure the Root bin has no new clip. The next import can still register a file. An offline file remains listed with the same ids, the same path, registration `registered`, presence `offline`, and size `unavailable`. There is no relink control.

Sequences, tracks, and instances are untouched by the import statements. Their Slice 1A tests still pass. Version-1 and version-2 reads were not given a new migration.

## Live Product Checkpoints

The first compile of the new test failed because `ProjectStore` was only forward-declared. The test include was added. The rebuild finished with no compiler warning.

The Qt test window is not the acceptance run. It showed an empty bin, then a harness import of `visible.mov`. That did not open the file dialog.

The acceptance run is the `aether` process already running under `QT_QPA_PLATFORM=xcb`, window `0x01200007`, title AETHER. Create Project opened a Qt name dialog, then the native chooser titled Choose a parent folder. The folder `validation-fixtures/slice-1c/live-parent` was chosen. The project `LiveSlice` opened with Root selected and the label `This bin has no media.`

Import Media then opened the native chooser. Its breadcrumb was `editry / development / sise-editor / validation-fixtures / slice-1c / live-parent`, and the filter read Media references. The file `validation-fixtures/slice-1c/live-media/picture.mov` (13 bytes, the text `visible-bytes`) was activated in that chooser. The dialog closed. The bin showed `picture.mov online`. The inspector read file `picture.mov`, media `33606ead-06e5-487d-9850-f0416c9b37ba`, representation `6af8e9ea-4ed5-479e-990b-6406ebcc0629`, object `3dca4707-6cd5-4fa0-9dc1-29108e95dd8e`, the fixture path, size `13 bytes`, presence online, registration `registered`, and technical metadata `Not probed`. The project `MEDIA` directory stayed empty. The source file was not copied.

Close Project returned to No project. Open Project opened the native chooser again. `LiveSlice` was chosen. The same three ids came back, still online.

The fixture file was renamed to `picture.mov.offline-aside`. It was not deleted. Its size stayed 13 bytes. The user's `Documents/EDITORIAL/sise_4k.mp4` stayed 7293864 bytes and `Project.mov` stayed 1067242633 bytes. Close and Open Project again. The same ids remained. Presence was `offline`, size and mtime `unavailable`, registration still `registered`, technical metadata still `Not probed`.

Before that successful path, pointer clicks in the same chooser landed in `Documents/EDITORIAL` and, once, on a LaCie volume. Those clicks were not the accepted import. Two project folders created there, `edit_test` and `aether_test`, contained only a `LiveSlice` package from this run and were removed. One of those packages had registered `sise_4k.mp4` as a reference. That package was deleted. The mp4 was not modified. The accepted registration is only `picture.mov`.

## Build and Test Results

Environment: Fedora Linux 45, kernel 7.2.15-300.fc45.x86_64, g++ 16.2.1 20261029 (Red Hat 16.2.1-3), CMake 4.3.0, Qt 6.11.2, SQLite 3.53.4. User id 1000, so a mode `000` file is actually unreadable. `WAYLAND_DISPLAY=wayland-0`, `DISPLAY=:0`, `XDG_SESSION_TYPE=wayland`. Log: `validation-fixtures/slice-1c/environment.log`.

Configure: `validation-fixtures/slice-1c/cmake-configure.log`. Exit 0. No `CMake Warning` line. The log names `SQLite3::SQLite3`.

Build: `validation-fixtures/slice-1c/build.log`. The first attempt exited 2 on the incomplete type. The rebuild exited 0. `warning:` does not appear. The executable links `libsqlite3.so.0`, which resolves to `/usr/lib64/libsqlite3.so.3.53.4`. `ldd` shows no FFmpeg or GStreamer library.

CTest, from `validation-fixtures/slice-1c/ctest.log`: 1 discovered, 1 passed (`slice0a`). That is one CTest test.

Qt Test, from `validation-fixtures/slice-1c/qt-test.log`, the same executable run directly. The Totals lines include each class's `initTestCase` and `cleanupTestCase`, which is how Slice 1B counted 38:

| Class | Totals line |
| --- | --- |
| TimingTests | 7 passed |
| ProjectTests | 11 passed |
| SequenceTests | 10 passed |
| TimelineWorkspaceTests | 10 passed |
| MediaImportTests | 10 passed |

48 Qt Test cases, 0 failed. These 48 cases are inside the one CTest test. Earlier cases were not removed or weakened. MediaImportTests covers the Root bin and its empty list, stable ids across a path change, close and reopen, an exact-path refusal with no merge by size, missing, unknown, invalid, empty, and unreadable imports, a database failure that rolls the batch back, offline ids, and the window inspector.

The same executable was run again under `QT_QPA_PLATFORM=xcb` and `QT_QPA_PLATFORM=wayland` to save the grabs below. Both exits were 0.

## Actual UI Validation

Wayland and XCB are separate runs.

| Run | What happened |
| --- | --- |
| Qt test, `QT_QPA_PLATFORM=xcb` | The window was exposed. The grab shows project name Window, status Open, Root selected, `visible.mov online`, the inspector, and Import Media. The dialog was not clicked. |
| Qt test, `QT_QPA_PLATFORM=wayland` | The window was exposed. The grab shows the same layout with a different Media ID, because it was a second real import. `wmctrl` was not asked to list this test process. |
| `aether`, Wayland, `--shutdown-now` | `AETHER start`, `AETHER stop`, exit 0. |
| `aether`, Wayland, left up | `AETHER start`. `wmctrl -l` printed no AETHER line. The process was killed, exit 143, and did not log `AETHER stop`. A `wmctrl` Wayland window was not confirmed. |
| `aether`, XCB, `--shutdown-now` | `AETHER start`, `AETHER stop`, exit 0. |
| `aether`, XCB, left up and closed | `wmctrl` listed `0x01200007  0 fedora AETHER`. `wmctrl -c` closed it. `AETHER stop`, exit 0. |
| `aether`, XCB, still running, before this follow-up | `wmctrl` listed `0x01200007  0 fedora AETHER` with no project open. That capture did not open a dialog. |
| `aether`, XCB, this follow-up | The same process. Create Project, Import Media, and Open Project each opened the native chooser. Import Media was used to register `picture.mov`. Close and reopen kept the ids. After the fixture file was renamed aside, the inspector read offline. |

The XCB listing is not a native Wayland result. This follow-up did not open a Wayland dialog. The Wayland `aether` process from the first pass was still not listed by `wmctrl`.

## Screenshots and Launch Instructions

| Path | What it is |
| --- | --- |
| `validation-fixtures/slice-1c/screenshots/xcb-imported-inspector.png` | 720×812 grab of the Qt test window on XCB after a real import |
| `validation-fixtures/slice-1c/screenshots/wayland-imported-inspector.png` | 720×812 grab of the Qt test window on the Wayland platform after a separate real import |
| `validation-fixtures/slice-1c/screenshots/xcb-launched-window.png` | 720×641 ImageMagick capture of the running XCB `aether` window, with no project open, before the dialog was used |
| `validation-fixtures/slice-1c/screenshots/xcb-import-media-in-fixture.png` | The native Import Media chooser on the running XCB window, breadcrumb ending in `live-parent`, filter Media references |
| `validation-fixtures/slice-1c/screenshots/xcb-live-empty-bin.png` | The same window after Create Project: Root, `This bin has no media.` |
| `validation-fixtures/slice-1c/screenshots/xcb-live-imported-inspector.png` | After the dialog registered `picture.mov`: online, 13 bytes, Not probed |
| `validation-fixtures/slice-1c/screenshots/xcb-live-reopened-online.png` | After Close Project and Open Project, the same ids, still online |
| `validation-fixtures/slice-1c/screenshots/xcb-live-reopened-offline.png` | After the fixture file was renamed aside and the project was opened again: same ids, presence offline |

Executable: `/home/editry/development/sise-editor/build-slice1c/src/aether`

The process left running is in tmux session `aether-slice1c`:

```
QT_QPA_PLATFORM=xcb /home/editry/development/sise-editor/build-slice1c/src/aether
```

Logs for the launches are `validation-fixtures/slice-1c/application-launch-wayland-shutdown.log`, `application-launch-wayland.log`, `application-launch-wayland-wmctrl.log`, `application-launch-xcb-shutdown.log`, `application-launch-xcb.log`, `application-launch-xcb-wmctrl.log`, and `application-launch-running-wmctrl.log`.

## Known Limitations

The native Import Media chooser was opened on the running XCB window and `picture.mov` was registered through it. A Wayland file dialog was not opened. Drag-and-drop is absent. There is no relink, no probe, and no duration, rate, resolution, or codec. A disappeared file stays offline until something else changes the stored path; this slice does not change it.

Two different paths are two registrations even when the bytes match. ADR-005 would send a known-same asset to one Media ID, and an unconfirmed match to a person. This slice does not fingerprint and does not ask. It refuses only an exact recorded path.

Root is created when the window refreshes or when an import runs, not when the project is created. Registration status stays the stored word `registered`. Presence is a separate live check. Session undo is not pushed for an import. Close still drops undo and keeps committed rows.

Playback, decoding, thumbnails, waveforms, proxies, transcode, copy, consolidate, conform, interchange, ripple, roll, overlap, and persistent undo were not implemented. Slice 1D was not started.

## Files Modified

- `include/aether/core/media/import.hpp`
- `src/core/media/import.cpp`
- `include/aether/persistence/project_store.hpp`
- `src/persistence/project_store.cpp`
- `src/CMakeLists.txt`
- `include/aether/app/project_session.hpp`
- `src/app/project_session.cpp`
- `include/aether/ui/main_window.hpp`
- `src/ui/main_window.cpp`
- `tests/media_import_tests.hpp`
- `tests/media_import_tests.cpp`
- `tests/main.cpp`
- `tests/CMakeLists.txt`
- `docs/validation/SLICE_1C_MEDIA_IMPORT_AND_PROJECT_BIN.md`
- `DEVELOPMENT_ROADMAP.md`
- `docs/19_DEVELOPMENT_ROADMAP.md`

`docs/DEVELOPMENT_ROADMAP.md` was not filled. Accepted ADR files were not edited. ADR-006 stays **PROPOSED**.

## Roadmap Status

`DEVELOPMENT_ROADMAP.md` and `docs/19_DEVELOPMENT_ROADMAP.md` now record Slice 1C as complete on this Fedora machine. Slice 0A, Slice 0B, Slice 1A, and Slice 1B stay complete. Later slices stay **NOT STARTED** and **NOT AUTHORISED**. The proposed phase tables were not rewritten. The empty `docs/DEVELOPMENT_ROADMAP.md` stays empty.

## Final Slice Status

**SLICE 1C COMPLETE — MEDIA IMPORT AND PROJECT BIN IMPLEMENTED AND VALIDATED — PLAYBACK AND MEDIA PROCESSING NOT YET IMPLEMENTED.**
