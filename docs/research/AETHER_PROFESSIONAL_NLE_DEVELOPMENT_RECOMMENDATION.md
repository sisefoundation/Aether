# Aether professional NLE development recommendation

Status: research recommendation. It does not authorise a slice, accept an ADR, or change code. Slice 1D was already implementing same-rate bin-to-timeline placement while this was written. This document does not tell anyone to restart that work.

## 1. What to do next, and why

Finish Slice 1D, then validate it, then build the next editorial command on the rule Slice 1D already uses.

Slice 1D, as proposed in `docs/proposals/SLICE_1D_TIMELINE_CLIP_PLACEMENT_PROPOSAL.md` and as present in the source on 2026-11-22, places one master clip on one track when the source rate and the sequence rate are the same reduced rational. The record length is the source length. Overlap is refused. Mixed rates are refused. One undo entry removes the new instance. The row is the existing `timeline_instances` row. That is the right first cut. A professional insert, which pushes later clips, is a larger change. It needs a sync rule the project does not have yet.

This recommendation depends on Slice 1D finishing. Placement was still being edited while these notes were written: `main_window.cpp` changed from the old window title to `Aether` at 01:14, and a product mark was added in that same edit. A `build-slice1d` CTest log from 01:14, which this research did not run, records `slice0a` passed, including `TimelinePlacementTests` at 6. That log is not a substitute for the Slice 1D author's own validation write-up, and it does not cover source changes saved in the same minute. The next slice should not start on top of that. Validation stays on a disposable project, not on `ShellCut`.

After that validation, the next engineering step is a source/record mark model and a same-rate overwrite that uses those marks. Reasons:

- The panels already exist and already say playback is absent. Marks do not need a decoder.
- `identityRecordEnd` already defines the only legal length. A three-point command can derive one missing endpoint from the table in `AETHER_PROFESSIONAL_EDITORIAL_WORKFLOW_SPECIFICATION.md` and then place or overwrite. It must refuse mixed rates with the same error the function already returns.
- Overwrite is the operation editors use when the timing of the cut must stay. Insert looks smaller in a menu and is larger in the model, because every later row moves.
- The context menu currently says Insert for a command that does not ripple. The next slice should not add a second meaning under that word. Either the label becomes Place, or a real insert waits until ripple exists. Relabelling is a product choice for the slice that owns the menu. It is not started here.

Do not start playback, probe, proxies, relink, or interchange next. Those are blocked by the FFmpeg link, by ADR-006 still being proposed, and by ADR-007 still being proposed. A picture on screen would not make the cut more truthful, and it would tempt the timeline to open files.

## 2. Preparation before that next slice

- Keep one duration rule. New commands call `identityRecordEnd` or refuse. They do not call `pictureSourceOffset` to size a clip.
- Decide the menu verb before adding another placement command.
- Keep widgets on `ProjectSession`. No SQL in the window. No decoder include in the timeline.
- New tests follow the existing pattern: disposable project, no `ShellCut`, no `IntegCut` unless that slice owns it, close and reopen, undo that leaves the previous commit when the command fails.
- Do not add a track-type column, a marker table, or a sync table in the same change as the first overwrite. Each is an owner decision from the gap analysis.
- Leave `insertInstance` and `trimInstance` able to store unequal lengths until the owner says otherwise. A silent tightening would change rows the current trim allows. The Slice 1D proposal already recorded that.

## 3. What not to reuse as code

Nothing from the reference checkouts is cleared for Aether.

Useful as structure only, already written up with SHAs in `AETHER_OPEN_SOURCE_NLE_CODE_STUDY.md`:

- Kdenlive's controller calls the model, and a failed insert does not push undo. Preview drags that mutate the model are not a pattern to copy.
- Blender's editor operators pass a frame and an overlap mode into sequencer functions, then tag the cache. The float scene FPS write in `add_movie_strip` is not a pattern to copy.
- Shotcut's `OverwriteCommand` versus `InsertCommand`, and `LiftCommand` versus `RemoveCommand`, show the operations as different undo classes. The MLT playlist they edit is not Aether's store.
- MLT's producer, playlist, tractor, and consumer are a processing graph. If a later decision uses a library as a consumer only, the editorial rows stay in `project.db`. That decision is not made. `mlt_position` is `int32_t` or `double` depending on `DOUBLE_MLT_POSITION`. Neither is the accepted clock.

Kdenlive files that were read are `GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL`. Blender files that were read are `GPL-2.0-or-later`. Shotcut's model file header says GPL version 3 or later. MLT framework files that were read are LGPL-2.1 or later. ADR-008 is open. A translation of those functions into Aether would be a copy for licence purposes and is out of scope.

## 4. External lessons that should affect the design

- Insert, overwrite, lift, extract, and split have different effects on later clips and on other tracks. One delete function cannot stand in for them. Aether's `removeInstance` deletes a row and leaves other record ranges. That is a hole, not a ripple.
- Lift is not extract. The Media Composer 2025.12 guide and the Lightworks basic-editing article both say so, and they do not say it the same way for the other tracks. Choose before implementing extract.
- Trim review can be session state. The undo entry is written when the trim is joined, not on every nudged frame. Lightworks documents that as unjoin and `Tab`. The idea is the proposal. The key is not.
- A keyboard map is data: command id, context, key. It is not a pile of `QShortcut` in the window. Defaults are an editor preference, stored outside the project, so a project remains portable.
- Bins hold logs. Files are representations. Aether already has that split and should not collapse it when probe arrives.
- Match Frame needs a mapping from a sequence frame to a source frame. Equal rates have one. Mixed rates do not, until an ADR says so.

## 5. How to validate the next work

For Slice 1D, when its author validates, the checks that match the proposal are: same-rate place, stored half-open source and record spans, mixed-rate refusal with no new row and no undo entry, overlap refusal, undo, redo, close, reopen. A disposable project. The existing integration document's ShellCut check was read-only and should stay that way.

For the following slice, validation is still Qt tests plus one deliberately small UI pass:

- Three-point cases from the workflow specification, including the negative derived endpoint that must refuse.
- The mixed-rate example sequence 24/1, source 25/1, source `[0, 25)`, record start 0, which must refuse and must not write record end 24.
- Overwrite replaces an occupied span and does not move a later instance.
- A failed overwrite leaves the previous instance and adds no undo entry.
- Reopen shows the overwrite.
- Playback buttons still report that playback is not available.
- No schema version change unless the owner accepted a table. Marks can live in the session.

Do not mark the slice validated because the unit test called the session. The integration slice was validated by clicking. The next one needs the same standard for the path it claims.

## 6. Risks of doing it in the wrong order

- Ripple before a sync decision writes a permanent desync into the only project database.
- Probe and playback before the edit commands exist will spend the media-engine work on an empty timeline.
- A track type added to make the UI look like a finished NLE will freeze a model the owner has not chosen.
- Copying a GPL timeline into the Apache tree, or linking MLT because Shotcut made it look convenient, is a licensing event. It is not a shortcut around ADR-002.
- Restarting placement under a new name while Slice 1D is landing will fork `placeInstance`.

## 7. Milestones after this research

These are not an approved roadmap. They restate the stage list in an order the architecture supports. Dependencies are technical, not calendar.

### Stage A — Editorial foundation

Already in progress and partly validated. Project, bin, import, sequence, track, instance, move, raw trim, remove, session undo, and same-rate placement. Exit: Slice 1D validated on the terms above, menu verb not pretending to ripple, unequal-length writes either kept on purpose or rejected by an owner decision.

### Stage B — Source/Record editing

Session marks, focused viewer, three-point derivation under the identity rule, same-rate overwrite, Match Frame for equal rates only. No decoder. Exit: the integer examples in the workflow specification pass, and mixed rates still refuse.

### Stage C — Professional timeline editing

Split, lift, then the sync decision, then ripple insert, ripple trim, roll, slip, and slide. Slip waits on a known source duration or stays limited to spans the editor types. Keyboard command ids land here, with an empty default map until the owner assigns keys. Exit: each operation has the ten properties listed in the workflow specification, one undo unit, and a reopen test.

### Stage D — Media engine and playback

Blocked by the FFmpeg link conditions and by ADR-006. A composition request built from instance rows. Processing does not write `project.db`. Playback buttons stop being refusals only when a frame index from the session is what is displayed. Proxy files, if any, are representations, not new Media IDs.

### Stage E — Professional media management

Probe fills source rate and duration. Relink, if ADR-007 is accepted, updates location rows only. Subclips, markers, bin columns, and search. Offline does not delete the cut.

### Stage F — Finishing and interchange

Colour, audio finishing, and exchange documents. ADR-007 says no adapter is a verified interchange until a test against that application is recorded. None is recorded. EDL and AAF stay out until a native project already reopens.

Stage D does not have to block all of Stage C. Split, lift, and same-rate overwrite do not need a decoder. Stage D should not start before Stage B, because a player with no marks is not the editing model the benchmark describes. Stage E's probe is what makes Replace and safe slip checkable. It can overlap the later part of Stage C. Stage F stays last.

## 8. What this phase did not do

No production source, test, CMake file, schema, ADR, roadmap, fixture, or user project was edited. Reference clones remain in `/tmp/aether-professional-nle-study/` at the SHAs in the code study. They are not part of Aether.
