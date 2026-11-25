# Native interface integration

Date: 2026-11-22. Machine: Fedora, Qt 6.11.2. The window was launched with `QT_QPA_PLATFORM=xcb`.

This connects the existing dark workspace to the existing `ProjectSession` commands. It does not redesign the window, does not add a database, and does not implement playback. ADR-006 stays **PROPOSED**. Schema version stays 2. `kPrototypeVersion` stays `slice-0a`. The link stays `SQLite3::SQLite3`. Widgets do not write SQL.

The inspection map is `docs/proposals/NATIVE_INTERFACE_INTEGRATION_MAP.md`. It was written before these edits.

Executable: `/home/editry/development/sise-editor/build-slice-ui-integration/src/aether`

Launch: `QT_QPA_PLATFORM=xcb /home/editry/development/sise-editor/build-slice-ui-integration/src/aether`

The XCB window was left running on this machine after the checks below. A second process with `QT_QPA_PLATFORM=wayland` was started. `wmctrl -l` continued to list only the existing XCB window `0x01400007`. No Wayland window was confirmed. That process was stopped.

## What is connected

Project Create and Open still go through `ProjectSession`. A failed create or open restores the project that was already open, so the bins and timeline do not mix two projects. Close clears the visible bins, sequence list, inspector, and edit fields, then returns to the dialog. Session undo still dies on close. Committed rows remain.

The bin list and media list read persisted bins and references. Import goes through `importReferences` after the native file dialog returns. A failure is shown only after that call, and the list is refreshed from the database. The original file is not modified.

The sequence list selects one sequence. The rate label and the record timecode follow that sequence. Tracks and instances from another sequence are not shown. Clicking a painted clip block selects that instance. The Clip inspector and the timeline inspector both show the persisted ids and half-open ranges. Transform, Colour, and Audio stay "Not probed".

Apply move and Apply trim call `moveInstance` and `trimInstance`. A rejected edit leaves the committed range and does not add an undo entry. Undo and Redo call the session and are enabled only while `canUndo` or `canRedo` is true. After a successful edit they refresh the canvas and both inspectors.

There is no insert or new-track control in the designed window, so none was added. The live clip used below was placed with `ProjectSession` and then opened, clicked, and moved in the window.

## What stays unavailable

TRIM, COLOUR, AUDIO, VFX, EXPORT, Effects, Transitions, and Titles still say they are not available. Tags say none are stored. SOURCE and RECORD still say "No media loaded. No playback." Transport buttons still set the status to "Playback is not available." The playhead stays the frame-0 label. There is no Save item; commands commit themselves, and `autosave` was not given a new button. No thumbnails, waveforms, snapping, ripple, roll, overlap, or subframe edit.

## Small visible changes

The layout, colours, and panel arrangement are the shell from `docs/validation/UI_SHELL_AND_PROJECT_DIALOG.md`. Three text behaviours changed because the commands required them:

- The rate label names the selected sequence, then lists every stored rate.
- Undo and Redo are grey when the session cannot undo or redo.
- The move and trim fields clear when no instance is selected, including after close and reopen.

## Build and tests

Debug configure of `build-slice-ui-integration` completed. No CMake warning was printed. The build completed. No compiler warning was printed. Earlier build directories were left in place.

CTest: 1 test, 1 passed (`slice0a`).

Qt totals, in order:

- TimingTests: 7 passed
- ProjectTests: 11 passed
- SequenceTests: 10 passed
- TimelineWorkspaceTests: 10 passed
- MediaImportTests: 10 passed
- UiIntegrationTests: 9 passed

The new 9 are 7 integration cases plus Qt's init and cleanup. They click Create, drive open and failed open, switch projects, import a bad file and a real file, click a sequence row, click a clip block, move, reject an overlap, trim, undo, redo, close, and reopen. `ShellCut` is not used by the tests.

## Live checks

`ShellCut` was opened through the native folder chooser and closed with no edit. It still has one sequence, `ShellCut` at `24/1`, no timeline instances, no media assets, and one `project.db` of 102400 bytes.

`IntegCut` was created from the dialog at the visible rate `24/1` into `validation-fixtures/ui-integration/live-parent/IntegCut/`. The rate menu was opened. Choosing `25/1` from accessibility did not change the combo, so the live project was stored at the visible `24/1`. The automated test does set that combo.

`note.mov` (11 bytes, under `validation-fixtures/ui-integration/media/`) was chosen in the native Import Media dialog. The bin then showed `note.mov  online`. The file is still 11 bytes. A session helper then added one track and one instance at record `[0,8)`. The window was reopened, the block was clicked, and Apply move set the committed range to `[12,20)`. Close and open restored that range, the media row, and `24/1`.

Screenshots, all under `validation-fixtures/ui-integration/screenshots/`:

- `01-project-dialog.png`
- `02-shellcut-reopened.png`
- `03-new-project-rate.png`
- `04-integcut-workspace.png`
- `05-imported-note.png`
- `06-clip-selected.png`
- `07-clip-moved.png`
- `08-reopened-after-move.png`
- `09-reopened-selection.png`

## Open question

There is still no designed control for creating a track or placing an instance. Those session commands exist. Adding a button would be a new control, so it was not done. ADR-006 remains unresolved and was not used to invent that control.
