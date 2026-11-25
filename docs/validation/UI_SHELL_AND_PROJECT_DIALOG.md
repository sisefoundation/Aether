# UI shell and project dialog

Date: 2026-11-21. Machine: Fedora, Qt 6.11.2, `QT_QPA_PLATFORM=xcb`.

This is a presentation of the commands already in the application. It is not a playback slice, not a media-processing slice, and not authorisation for a later slice. ADR status is unchanged. C++17 stays prototype-only. The link stays `SQLite3::SQLite3`. The window does not write `project.db` itself.

Executable: `/home/editry/development/sise-editor/build-slice-ui/src/aether`

Build directory: `build-slice-ui`. Earlier build directories were left in place.

The existing test binary in that directory passed: one CTest, and the Qt totals 7, 11, 10, 10, and 10.

## What the window does

The dark EDIT page shows the title, the six modes, the Project bins from the open project, the Media list and Import Media command, the timeline workspace, and the Clip inspector. All Media is a view over the stored bins. It is not a stored bin and it has no invented count. Root is the real bin created for this project. Tags say that none are stored.

SOURCE and RECORD show no picture. Their timecode is the sequence frame-0 label when a sequence is open. Transport buttons set the status to “Playback is not available.” They do not play.

The Clip section uses the fields already shown for a selected media reference. Transform, Colour, and Audio say “Not probed”. The shell does not invent a format, codec, resolution, or duration. The preview card stays hidden until a real media item is selected, and it does not draw a thumbnail.

The timeline keeps the existing move, trim, undo, and redo commands. The ruler uses the stored sequence rate. For the project created below, the selected sequence caption is “Sequence frame 0 label 00:00:00:00.” Tracks stay in stored order. No video or audio kind is stored, so none is drawn. There is no snapping, ripple, roll, overlap, filmstrip, or waveform.

TRIM, COLOUR, AUDIO, VFX, EXPORT, Effects, Transitions, and Titles switch to a page that says the mode or tab is not available. They do not draw finished controls.

## Project dialog

The dialog opens at startup when no project is open, and from File → New or Open Project. File → Close Project returns to it. Close drops session undo. It does not discard the last committed rows.

Create and Open both went through the existing session path.

- Create wrote `validation-fixtures/ui-shell-2026-11-21/live-parent/ShellCut/`, including one `project.db`. The dialog stored sequence `ShellCut` at `24/1`. Raster, pixel aspect, colour space, and stereoscopic settings were not written.
- Open used the native folder chooser, reopened that same directory, and left the sequence at `24/1`. The dialog showed the stored rate and did not change it.

There is no stored project catalog. The Existing projects page says so and opens a folder.

## Screenshots

- `validation-fixtures/ui-shell-2026-11-21/screenshots/project-dialog.png` — New Project, frame rate `24/1` visible.
- `validation-fixtures/ui-shell-2026-11-21/screenshots/editing-workspace.png` — EDIT workspace after the same project was opened.

## Not wired

Playback, decoding, FFmpeg, GStreamer, thumbnails, waveforms, snapping, ripple, roll, overlap, tags, effects, transitions, titles, and the non-EDIT modes. The viewers have no loaded picture. Technical metadata stays “Not probed” until a later authorised probe exists.
