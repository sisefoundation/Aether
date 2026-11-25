# Aether architecture gap analysis

Inspection date: 2026-11-22, by reading the tree. No test binary was launched. No project was opened. The Slice 1D build directory was not configured or compiled by this research. One existing CTest log inside it was read after the fact.

## 1. What was actually in the tree

There is no git repository.

Executable reported by `docs/validation/NATIVE_INTERFACE_INTEGRATION.md`:

`/home/editry/development/sise-editor/build-slice-ui-integration/src/aether`

That file exists. Its mtime was 2026-11-22 00:22:36 +0530. It was not launched. The integration document says the window was launched with `QT_QPA_PLATFORM=xcb` and that Wayland was not confirmed.

A second binary exists at `/home/editry/development/sise-editor/build-slice1d/src/aether`, mtime 2026-11-22 00:51:33 +0530. It belongs to the Slice 1D work that was in progress. It was not launched and not rebuilt.

The first read, against sources timestamped 00:41 to 00:51, already contained drag-and-drop, "Insert into Timeline", `PlacementDialog`, `ProjectSession::placeInstance`, and `identityRecordEnd`. The integration validation document describes an older window with no insert control. Treat that document as the record of the 00:22 binary.

Slice 1D kept writing during this research. A second look, after 01:14, found:

- `src/ui/main_window.cpp` mtime 01:14:22. The constructor now calls `setWindowTitle("Aether")`, the shell label is `Aether`, and an open project sets the title to `Aether — ` plus the project name. The first read of that constructor still had `Open-Source Professional Video Editing`. A product-mark pixmap is loaded through `markForDarkSurface()`. This research did not review that graphic.
- `src/ui/main.cpp` sets the application name to `Aether`.
- `tests/timeline_placement_tests.cpp` mtime 01:13:45. The four slots named below were still the slots in the header at the first read. They were not re-counted after 01:13.
- `build-slice1d` was reconfigured around 01:13. This research did not configure or build it.

`build-slice1d/Testing/Temporary/LastTest.log` records a CTest run this research did not start. Start time Nov 22 01:14 IST. Test `slice0a`. Command `build-slice1d/tests/aether_slice0a_tests`. Result `Test Passed`. Qt totals in that log: TimingTests 7, ProjectTests 11, SequenceTests 10, TimelineWorkspaceTests 10, MediaImportTests 10, UiIntegrationTests 9, TimelinePlacementTests 6. Each total includes `initTestCase` and `cleanupTestCase`. `menuMixedRateOfflineAndBadTarget` passed and printed a Wayland warning: `Failed to create grabbing popup`. The 00:22 integration binary cannot contain the placement class. The 01:14 log is evidence of that one run in `build-slice1d`. It is not a run this research executed, and `main_window.cpp` was still being saved in the same minute.

Schema version constants in `include/aether/core/project/project_info.hpp`: `kSchemaVersionSlice0A = 1`, `kPrototypeSchemaVersion = 2`, `kPrototypeVersion = "slice-0a"`. The integration document says schema version stays 2. The placement path does not add a table. That matches the source that was read: `timeline_instances` is still the Slice 1A table.

### Test names and counts

`tests/CMakeLists.txt` builds one executable, `aether_slice0a_tests`, and registers one CTest test named `slice0a`.

`tests/main.cpp` runs seven Qt classes. Private slots in the headers, which are the test functions, and the Qt totals if init and cleanup are counted the way `docs/validation/SLICE_1C_MEDIA_IMPORT_AND_PROJECT_BIN.md` counted them:

| Class | Private slots in the header | Slots plus init and cleanup |
| --- | --- | --- |
| `TimingTests` | 5 | 7 |
| `ProjectTests` | 9 | 11 |
| `SequenceTests` | 8 | 10 |
| `TimelineWorkspaceTests` | 8 | 10 |
| `MediaImportTests` | 8 | 10 |
| `UiIntegrationTests` | 7 | 9 |
| `TimelinePlacementTests` | 4 | 6 |

The reported 7, 11, 10, 10, 10, 9 match the first six classes. `docs/validation/NATIVE_INTERFACE_INTEGRATION.md` records those six totals and one passing CTest, and it does not mention `TimelinePlacementTests`. That class is in `tests/main.cpp`. Its four slots, as first read, are `identityRecordEndAndPointerMapping`, `placeCommandPersistsAndRejectionLeavesTheCut`, `dropPlacesEditsUndoesAndReopens`, and `menuMixedRateOfflineAndBadTarget`. The 01:14 `build-slice1d` log, quoted in section 1, records those four plus init and cleanup as 6 passed. This research did not launch that executable.

## 2. Implemented behaviour, from the source

Project: `createProject`, `openProject`, `closeProject`, one `project.db`, manifest, lock that refuses a second open, schema 1 migrated to 2 inside one transaction.

Media: root bin on demand, import of an existing path as a master clip plus an `original` representation, stable ids if the recorded path is later edited through the path setter, exact-path duplicate refused, no byte-level merge. Technical metadata on the inspector is the string "Not probed".

Sequence: one rate per sequence, tracks with an ordinal and no type, timeline instances with source rate, source span, and record span. Overlap on one track is rejected. Abutting spans are legal.

Edits: `moveInstance` keeps record length. `trimInstance` writes four endpoints and does not require equal lengths. `removeInstance` deletes one row and does not shift others. `placeInstance` requires `identityRecordEnd` and then inserts. `insertInstance` does not require equal lengths. Undo is an in-memory vector of `UndoEntry`. Close drops it. Committed rows remain. `autosave` is a session method and is not a File menu item.

UI connected in the integration document, and still present in the source: project dialog, bins, import, sequence selection, clip-block selection, move, trim, undo, redo. SOURCE and RECORD panels show "No media loaded. No playback." Transport sets the status to "Playback is not available." TRIM, COLOUR, AUDIO, VFX, and EXPORT pages say they are not available. Tags say none are stored.

Placement UI in the source, newer than the integration document: the media list is draggable, the context menu says "Insert into Timeline", the canvas can start a placement, and `PlacementDialog` collects a source rate and a source span. The menu label says Insert. The command is `placeInstance`, which does not ripple and which refuses overlap. The label and the professional meaning of insert are different. That is a wording gap, not a behaviour this research changed.

## 3. Comparison matrix

Statuses mean:

- IMPLEMENTED AND VALIDATED: the recorded validation exercised the user path, and the source still has it.
- IMPLEMENTED, NOT FULLY VALIDATED: the code is there; the user path was not in the recorded validation, or this research could not re-check it because Slice 1D was in progress.
- PARTIALLY IMPLEMENTED: a piece exists and is not the professional operation.
- NOT IMPLEMENTED.
- DEFERRED: a written decision or roadmap already postpones it.
- REQUIRES ARCHITECTURAL DECISION: the current model cannot express it without a new rule.
- REQUIRES LEGAL REVIEW: a dependency or a code reuse would need counsel. ADR-008 is already open for that.

"Unknown" in a benchmark cell means the public document that was read does not say.

| Editorial area | Avid benchmark | Lightworks benchmark | Open-source implementation | Aether status | Gap |
| --- | --- | --- | --- | --- | --- |
| Project lifecycle | Bins and sequences in a project. Undo depth documented. Crash format not read | Project contains logs. Continuous auto-save claimed. Mechanism unknown | Kdenlive scene list; Shotcut MLT XML; Blender blend file (writer not opened) | IMPLEMENTED AND VALIDATED for create, open, close, reopen of committed rows | Autosave is not a command in the window. Persistent undo is deferred under ADR-009. No power-loss proof |
| Bins and master clips | Master clip, subclip, bin marks | Log versus file. Clip in many bins. Collections, tags, search | Kdenlive `ProjectItemModel` bin ids and zones | PARTIALLY IMPLEMENTED. Bins, membership, master clips, import. No subclip, no columns, no search | Assistant logging |
| Source/Record editing | Source and Record monitors, independent marks | Source and sequence viewers, independent playheads, mark and park | Kdenlive clip monitor zone; Shotcut source producer versus timeline | PARTIALLY IMPLEMENTED. Empty panels only | Marks, load, focus, timecode |
| Three-point editing | Fourth mark derived. Mixed rate uses Motion Adapter, formula not published as integer frames | Two cases written in the basic-editing article. Mixed rate unknown | Kdenlive zone insert uses inclusive in/out. Not Aether's rule | NOT IMPLEMENTED. `identityRecordEnd` can derive a record end when rates match | A command that uses it. No mixed-rate rule exists |
| Insert editing | Splice-in pushes later material | Insert, with black on tracks that stay in sync | Kdenlive `insertZone(overwrite=false)`; Shotcut `InsertCommand` / `insertClip` | NOT IMPLEMENTED. The menu word Insert calls `placeInstance`, which does not push | Ripple insert, and the menu word |
| Overwrite editing | Replaces, does not lengthen unless it extends the end | Replace keeps sequence length | Kdenlive overwrite zone; Shotcut `OverwriteCommand` | NOT IMPLEMENTED. Overlap is an error | A command that replaces a span |
| Track targeting | Track Selector | Track targets. Untargeted tracks trim black during a ripple | Kdenlive `useTargets`, audio and video targets | NOT IMPLEMENTED | Session targets. No track type to bind them to |
| Track patching | Source tracks to record tracks. Body of the selector chapter was not read | Source track enable chooses which components are edited | Kdenlive `A`/`V` mime prefix and audio streams | NOT IMPLEMENTED. REQUIRES ARCHITECTURAL DECISION | Components do not exist until probe |
| Timeline selection | Segment selection is a named chapter. Body not read | Click, shift, ctrl, link | Shotcut selection in QML; Blender sequencer select operators | IMPLEMENTED AND VALIDATED for one block click, in the integration document | Multi-select, link |
| Trim modes | Rollers, ripple, slip, and slide are named. Roller mathematics not read in this pass | Unjoin, ripple, move cut, slip, slide | Blender slip and ripple-trim operators; Shotcut trim-in and trim-out commands; Kdenlive `requestItemResize` | PARTIALLY IMPLEMENTED. One raw four-endpoint trim, clicked in the integration validation. It can store unequal lengths | The trim matrix in the workflow specification |
| Sync relationships | Sync lock versus track lock. Chapter body not read | Clip Link versus Track Sync versus lock | Shotcut groups; Kdenlive groups; Blender strip connections exist as operators, bodies not read | NOT IMPLEMENTED. REQUIRES ARCHITECTURAL DECISION | No link id, no sync set |
| Markers and locators | Coloured markers, comments, Find | Cue at the playhead, searched as metadata | Kdenlive guide model is referenced from the timeline menu. Not studied in depth | NOT IMPLEMENTED. Tags label says none are stored | A marker row |
| Keyboard editing | I, O, Ctrl+Z, Ctrl+R, and a user button map. Full map not read | Factory map, user-reassignable, documented 16 Oct 2026 | Each project uses its own `QAction` or keymap. Not copied | NOT IMPLEMENTED as a map. Undo and Redo buttons exist | Command ids and contexts, no borrowed defaults |
| Media relinking | Several relink chapters. Match rules not read | Offline is a search criterion. Algorithm unknown | Kdenlive document checker exists as a class name in includes. The relink body was not traced | PARTIALLY IMPLEMENTED as `setRepresentationPath`. Not a classified relink. ADR-007 PROPOSED | Do not call a path edit a relink |
| Proxy workflow | Not read in this pass | Viewer quality can drop to proxy. Generation not described | Shotcut `ProxyManager::generateIfNotExists` inside overwrite redo | PARTIALLY IMPLEMENTED. Role enum only. DEFERRED with the media engine | No files are made |
| Audio tracks | Real audio tracks and audio marks | `A1` targets, mute, stereo groups | Shotcut video versus audio track types | NOT IMPLEMENTED. REQUIRES ARCHITECTURAL DECISION. The model has no type | Do not invent a type in the UI |
| Multicam | Named in the guide TOC. Not read | Sync-group bin | Not traced | NOT IMPLEMENTED. DEFERRED | Group clips and a switcher |
| Project recovery | Undo list. Crash behaviour not read | Auto-save claimed. Format unknown | Kdenlive `backupLastSavedVersion` before scene save. Not fully traced | IMPLEMENTED AND VALIDATED for reopen of the last commit. IMPLEMENTED, NOT FULLY VALIDATED for crash recovery | Session undo dies on close |
| Media interchange | EDL comments are a marker use. Adapters not read | Not in the articles read | Not traced | NOT IMPLEMENTED. ADR-007 PROPOSED. DEFERRED | No vendor test exists |

## 4. Constraints that are not gaps to "fix" in passing

- Same-track overlap stays illegal until an overwrite or transition model is an accepted change. Transitions are not implemented. The validator says so.
- No track type until an owner decision. The window text is correct.
- No probe, no decode, no FFmpeg link. ADR-001 is accepted with conditions and the library link is still blocked. The 30000/1001 MXF 12813-against-12812 result remains a failed encoder test.
- ADR-006 proposed: widgets and the timeline do not open decoders. The placement dialog does not.
- Float seconds are not added as a convenience clock.
- C++17 is prototype-only, from the roadmap banners. This research did not re-litigate it.
- Apache-2.0 headers are on the Aether sources that were read. The roadmap still says Apache-2.0 is a proposal and ADR-008 is open pending counsel. Those headers are not a counsel opinion.

## 5. Debt already visible

- `insertInstance` allows a source length different from the record length. `placeInstance` refuses it. `trimInstance` allows it again. Three commands, two rules.
- The context menu says "Insert into Timeline" for a command that does not insert in the professional sense.
- Undo kind `InsertInstance` is also used by `placeInstance`. A later ripple insert needs a different kind or the undo records will not say what happened.
- `removeInstance` is the only delete. It is not lift and it is not extract.
- Session undo is a single struct with many optional fields. It will not scale to a ripple that touches every later row. That is a design limit, not a bug in the current single-row edits.
- `kPrototypeVersion` remains `slice-0a` after later slices. The post-1B consistency note already recorded this as a label. It is still true.
- Imported media has no source rate and no duration, so every placement asks the editor to type them. That is honest. It blocks Replace and any trim that must know the end of the file.

## 6. Dependencies among the missing pieces

1. Same-rate placement has to be finished and validated before more edit commands. Slice 1D was doing that during this inspection. Do not restart it.
2. Source and record marks can be session state before any decoder exists. Three-point insert and overwrite need those marks, targets, and the identity rule. They do not need playback.
3. Split is a local row operation under equal rates. It does not need playback. It is required before a partial lift.
4. Ripple insert, ripple trim, and extract need a sync-set decision. Without it they will desynchronise tracks and the tests will encode an accident.
5. Slip needs a known source duration or an explicit "unchecked" mode. Probe is the duration. Probe is behind the FFmpeg block and ADR-006.
6. Playback, proxies, and waveforms are a media engine. They consume a composition. They must not become the timeline. ADR-006 is the proposed boundary and is not accepted.
7. Relink and interchange wait on ADR-007 and on a real second representation. A path column update is not that work.
8. Keyboard bindings wait on command ids. Binding keys to buttons that refuse playback adds nothing.

A better order than "finishing tools next" is: finish placement, then marks and non-ripple overwrite, then split, then the sync decision, then ripple. Playback can start once a composition request can be built from rows that already exist, and only after the link block is lifted. That is later than the edit commands that are pure row math.

## 7. Risks

- Treating `placeInstance` as insert, and building ripple on top of a command that refuses overlap. The names will lie.
- Encoding a mixed-rate duration with `pictureSourceOffset` or with a one-second wall clock. Both contradict the refusal string already in `identityRecordEnd`.
- Adding a track-type column so the UI can say V1 and A1, before the owner decides. The current tests and the window text say the type is absent on purpose.
- Adopting MLT or Kdenlive code because the call chain is understood. The licences are GPL or LGPL, the time types differ, and ADR-008 is open.
- Extending the session undo struct until a multi-row command cannot be represented, then silently dropping the extra rows on undo.
- Validating a feature because a button or a unit test exists. Placement tests exist on disk and were not run here. The integration click-test did not include them.

## 8. Owner questions

These are decisions. This research does not answer them.

1. When rates differ, is the edit refused until a later ADR, or is there a mapping to write? The code refuses. ADR-002's nearest-frame rule is not that mapping.
2. For extract, which close rule is wanted: every sync-locked track moves by the full length (the Media Composer sentence that was read), or the close is limited by the black available on every track (the Lightworks sentence)?
3. Is the default sync set "all tracks", with lock as the opt-out?
4. Should `trimInstance` and `insertInstance` start refusing unequal source and record lengths, so one rule covers every write? Existing stored rows may already violate it.
5. Is a track type in scope, or do targets stay an ordered list of untyped tracks?
6. Does a subclip become a new editorial object with the same Media ID?
7. Where does the keyboard map live, if not in `project.db`?
8. Is ADR-006 accepted before any decoder is linked, or does a later slice propose a narrower boundary?

## 9. Legal review items

Not approved by this study:

- Any copy or translation of Kdenlive, Blender, Shotcut, or MLT source.
- Linking MLT (LGPL-2.1-or-later on the files read; modules not all opened).
- Linking FFmpeg beyond the existing ADR-001 conditions. `libavfilter` is not approved. The MXF sample-count failure is still a failure.
- GPL code from the reference applications inside an Apache-licensed prototype. ADR-008 is **OPEN — PENDING LEGAL COUNSEL**.
- Avid or Lightworks names, icons, or keyboard-card artwork used as Aether's own marks.

Reading those repositories to compare architecture is what this study did. It is not clearance to depend on them.
