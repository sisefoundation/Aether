# Native interface integration map

Inspection date: 2026-11-22. This file was written from the source and the shell screenshots before the integration edits. It does not accept an ADR, does not change the schema, and does not authorise playback or a later slice.

Screenshots inspected:

- `validation-fixtures/ui-shell-2026-11-21/screenshots/project-dialog.png`
- `validation-fixtures/ui-shell-2026-11-21/screenshots/editing-workspace.png`

Source inspected: `src/ui/main.cpp`, `include/aether/ui/main_window.hpp`, `src/ui/main_window.cpp`, `include/aether/ui/project_dialog.hpp`, `src/ui/project_dialog.cpp`, `include/aether/ui/timeline_workspace.hpp`, `src/ui/timeline_workspace.cpp`, `include/aether/ui/frame_label.hpp`, `include/aether/app/project_session.hpp`.

There is no toolbar and no context menu in the designed window. None is added here.

Classes: 1 already functional, 2 needs application integration, 3 presentation-only, 4 not implemented and must stay unavailable, 5 ambiguous or blocked on an open architectural decision.

| UI component | User action | Existing application command/service | Current state | Class | Integration required |
|---|---|---|---|---|---|
| Project dialog, New Project, name and parent folder | Enter a name and a parent folder | `ProjectSession::createProject` | Create writes a real project. The button is wired. | 1 | Preserve and add a dialog test |
| Frame rate combo | Choose 24/1, 25/1, 24000/1001, 30000/1001, or 60000/1001 | `ProjectSession::createSequence` | The chosen reduced rational is stored on the new sequence. Raster, pixel aspect, colour, and stereo are not stored. | 1 | Preserve and test the stored rate |
| Browse | Choose a parent folder | `QFileDialog::getExistingDirectory`, then the same create path | Wired. The folder is not a project by itself. | 1 | Preserve |
| Open Project | Choose an existing project folder | `ProjectSession::openProject` | Wired. If a project is already open, it is closed before the new open. A failed open then leaves no project, so the window can look closed rather than mixed. | 2 | Keep the same open path. On failure, restore the project that was open. Test that a bad folder does not show another project's bins or clips |
| Stored sequence rate label | Read the open project's rate | `ProjectSession::listSequences` | Shown and not edited. | 1 | Preserve. Do not write a new rate for an existing project |
| Existing projects tab | Look for a catalog | None. No catalog is stored. | Honest empty text plus Open Project. | 3 | Preserve the empty statement |
| File → New or Open Project | Open the dialog | `MainWindow::showProjectDialog` | Wired. After the dialog, `refresh` reloads bins, sequences, and the timeline. | 1 | Preserve |
| File → Close Project | Close and return to the dialog | `ProjectSession::closeProject` | Wired. Session undo dies. Committed rows remain. | 1 | Test that bins, the sequence list, and the inspector clear |
| File → Import Media | Register files | `ProjectSession::importReferences` | The button opens a native file dialog and submits the chosen paths. Failure uses the session error. The list refreshes only after the call returns. | 1 | Test success, duplicate refusal, and a failed import that writes nothing. The live check must open the native dialog |
| Project bins | Choose All Media or a stored bin | `listBins`, `selectBin`, `listBinAssets` | All Media is a view, not a stored bin. Stored bins are listed by name with no invented count. | 1 | Preserve. Test that a second project does not keep the first project's rows |
| Media list | Select a registered reference | `listBinAssets` | Selection fills the Clip inspector with real ids, path, presence, registration, and "Not probed". | 1 | Preserve the exact empty-bin and inspector strings the current test checks |
| Import Media button | Same as the File command | `importReferences` | Same slot as the menu. | 1 | Same tests |
| Media preview card | Look at the selected reference | Selected `BinAssetFact` | Hidden until a real item is selected. Text only. No thumbnail. | 3 | Keep it hidden for a timeline selection, which has no thumbnail |
| Tags list | Browse tags | None | "No tags are stored." | 4 | Leave it |
| Effects, Transitions, Titles tabs | Switch tab | None | The page says the tab is not available. | 4 | Leave it |
| EDIT / TRIM / COLOUR / AUDIO / VFX / EXPORT | Switch mode | None beyond the page switch | EDIT is the real workspace. The others say the mode is not available. | 4 | Leave them unavailable |
| SOURCE and RECORD viewers | Look for a picture | None | "No media loaded. No playback." Timecode is the selected sequence's frame-0 label when one exists. | 4 | Do not decode. Point the label at the selected sequence rather than always the first |
| Transport Start, Back, Play, Forward, End | Press a transport button | None | The status line says "Playback is not available." Nothing moves. | 4 | Keep that refusal. Do not disable them into a different design, and do not simulate playback |
| Playhead label | Read the frame position | `formatFrameLabel` of frame 0 | A label, not a scrub control. | 4 | Keep frame 0. Do not advance it |
| Sequence list | Select a sequence | `listSequences`, `readSequence`, `listTracks`, `listInstances` | Selection reloads tracks and instances for that sequence only. | 2 | Keep the command. Update the rate label from the selected sequence. Clear the previous sequence's clip selection. Test that the other sequence's instances do not appear |
| Timeline canvas | Click a clip block | Selection is only through the instance list today | Blocks are drawn from persisted ranges. The selected flag is painted, but a mouse press does not hit-test a block. | 2 | Hit-test the existing blocks and call the existing `selectInstance`. Do not add snapping, ripple, roll, overlap, or waveforms |
| Instance list | Select an instance | `readInstance` | Wired. The timeline inspector shows the real ids and half-open ranges. | 1 | Also drive the designed Clip inspector from this selection |
| Clip inspector | Read the selection | Media facts, or instance facts when a block is selected | Shows the selected media reference. A selected instance updates only the small timeline inspector, not this panel. | 2 | When an instance is selected, show that instance's existing fields here. Transform, Colour, and Audio stay "Not probed" |
| Transform, Colour, Audio | Edit picture or sound | None | "Not probed". No command can save them. | 4 | Leave read-only |
| Move record start and Apply move | Move the selected instance | `ProjectSession::moveInstance` | Wired from the button. An invalid move is rejected and the committed range stays. | 1 | Test it after a real block click, including the persisted range and the inspector |
| Trim fields and Apply trim | Trim the selected instance | `ProjectSession::trimInstance` | Wired. Half-open integer frames only. | 1 | Same, through the button |
| Undo and Redo | Undo or redo the session command | `ProjectSession::undo` / `redo` | Wired. The buttons stay enabled when the session cannot undo or redo, so they look ready. | 2 | Enable each button only while `canUndo` or `canRedo` is true. Refresh the canvas and both inspectors after the command |
| Track rows and ruler | Read stored order and the frame label | `listTracks`, `formatFrameLabel` | Drawn in stored order. No video or audio kind is invented. | 3 | Preserve |
| Status line | Read the last action | Window text | Open, Closed, a session error, or the playback refusal. | 1 | Use it for import and project errors |
| Save | None. The designed menu has no Save item | `ProjectSession::autosave` exists and is not in this window | There is no control to connect. Commands already commit. | 5 | Do not add a Save button. Autosave stays unwired until a design and an ADR-006 decision say otherwise |
| Insert clip, new track, relink, conform | None in the layout | `createTrack` and `insertInstance` exist on the session and have no designed control | No button. | 5 | Do not add controls. A live clip is prepared through the session, then opened and edited in the window |
| Window close | Close the window | `closeProject` | Closes the session and does not reopen the dialog, so tests can close the window. | 1 | Preserve |

Playback, decoding, thumbnails, waveforms, and the non-EDIT modes stay class 4. ADR-006 stays PROPOSED. This map does not resolve it.
