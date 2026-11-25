# AETHER Timeline UI Specification

Status: proposal, awaiting review. No implementation is authorized by this document.

This document specifies the timeline surface of AETHER. The workstation around it — viewers, bins, transport, colour roles, density, and keyboard principle — is [UI_ARCHITECTURE.md](../UI_ARCHITECTURE.md). This document does not move those panels and does not authorise a widget, a stylesheet, or a build.

The surface is drawn by the timeline rendering engine from a snapshot. Gestures are interpreted by the timeline interaction engine. Neither engine computes a trim. Both are specified in [TIMELINE_ENGINE_ARCHITECTURE.md](TIMELINE_ENGINE_ARCHITECTURE.md).

## 1. Identity

The timeline is a dense, matte, square-cornered film surface. It uses the colour roles already named for AETHER: ground, panel, chrome, hairline, primary text, secondary text, one accent, a small lightness step between picture lanes and sound lanes, and one warning hue for a reported offline, untagged, missing-effect, or drift state.

The accent is a hairline or a short mark: playhead, marks, focus, targeted track, selection, open edit point. It is not a large fill. Selection is an outline or a low wash in that accent. Disabled provisions drop to secondary text. There is no gradient, no glass, no shadow stack, and no second accent for ornament.

Type is compact and functional. Timecode and frame counts use tabular figures. Lane rows share their boundaries with the track rail. Padding is a few pixels. Icons are small and monochrome, and only where a word will not fit. Motion is limited to the playhead following a frame the Application reported, and to a preview boundary following a delta the trim engine returned.

The layout, the type, the trim terminals, and the sync marks are AETHER's. Published pictures of other editors are not a source for shapes, hues, or control chrome.

## 2. Where the timeline sits

In the Editorial workspace the timeline band fills the centre column under the transport bar. The track rail is the left edge of that band. The timeline workspace is the remainder. Rows in the rail and lanes in the workspace share one vertical alignment. The transport bar already shows sequence timecode, source timecode, In, Out, duration, and transport. The timeline does not invent a second pair of clocks. It repeats sequence time on the ruler and source time on the selected clip.

In a narrow window the rail stays attached to the lanes. Horizontal zoom and scroll keep working. Thumbnails yield first when a lane becomes too short to hold them. Labels truncate. Waveforms remain while the lane can show a one-pixel envelope. The dual timecode stays on the transport bar.

## 3. Regions of the workspace

From top to bottom:

| Region | Role |
| --- | --- |
| Sequence overview | A thin map of the whole sequence. The visible window is an accent rectangle. Marks and the playhead are hairlines. |
| Ruler | Sequence timecode, marks, marker ticks, the limit span during a preview. |
| Video lanes | One lane per video track, top to bottom in track order. |
| Group gap | A hairline separation between picture and sound. |
| Audio lanes | One lane per audio track. |
| Horizontal bar | Scroll of sequence time, aligned with the lanes, not with the rail. |

The overview is a view of record ranges, not a second sequence. Clicking it sets the scroll origin so the clicked frame is centred, or sets the playhead when the modifier for playhead-seek is held. Dragging the visible rectangle scrolls. It does not zoom by itself. Zoom stays on the scale control and the keyboard actions.

The ruler's labels are sequence timecode, formatted from integer frames at the sequence rate, including drop-frame as a label. Label spacing follows the scale so labels do not collide. The ruler also shows frame indexes when the scale is at or above one pixel per frame and the editor has asked for indexes. That choice is presentation state.

## 4. Track rail

One row per track, the height of its lane. The row shows a short name, a V or A mark, and these provisions, each a keyboard stop:

| Provision | State | Appearance |
| --- | --- | --- |
| Target | Session | Accent hairline on the row when this track is aimed. Unlit when no targets are set, which means the default aim in the philosophy. |
| Patch | Session | A compact source-track label, primary text when patched, secondary when empty. |
| Enable | Document | Primary when the track contributes. Secondary, with the lane dimmed, when disabled. |
| Lock | Document | A small lock mark. The row's name drops to secondary. A locked lane ignores edit gestures. |
| Sync lock | Document | A small sync mark. In sync, the mark is accent. Drift at the playhead shows the signed frame count in the warning hue. |

The rail header carries the trim-kind readout already required by the workstation specification: trim, ripple, roll, slip, or slide, and whether a preview is open. The readout is repeated on the transport bar when the rail is scrolled away.

Video rows sit above the group gap. Audio rows sit below it. Compact lane height is the default. Medium is a workspace preference. A per-track height override is presentation state. None of these change the sequence.

A disabled track stays visible. Hiding a track from the lane stack is a presentation choice and does not disable it. The command that disables a track is separate, and it is inactive until the Editorial command exists. This specification defines the control. It does not run it.

## 5. Lanes and clips

A clip is a rectangle in a lane. Its left edge is `pixelStart(recordStart)` and its right edge is `pixelStart(recordEnd)`, using the integer pixel rule in the timebase document. A gap is a rectangle with no fill beyond the lane ground, a hairline boundary, and no label. A transition is a shorter rectangle centred on its cut, drawn in the lane with a diagonal hairline so it reads as a join. A nest uses the same rectangle as a clip, with a nested-sequence mark in the label row.

Video clips, when the lane is tall enough and a still has arrived:

- A strip of thumbnails along the top of the rectangle, each still the source frame the resolver names for that slot.
- The clip name and the source-timecode hint in the label row, primary text, truncated from the end.
- Offline and untagged states as words in the warning hue, in the label row. The rectangle does not invent a picture.

Audio clips:

- The name in the label row.
- A waveform envelope filling the rest of the lane, painted from the min/max pyramid the Application has delivered for the visible source range.
- Silence for a gap. A pending waveform is an empty envelope, not a blocking spinner.

The selected clip uses the accent outline. A link group selected together outlines every member. Drift on a member adds the warning hue to that member's sync mark, which is a short chain mark at the clip's label. An unlinked clip has no chain mark.

Thumbnails and waveforms are paintings of cache artifacts. Their absence does not change the rectangle's edges. Those edges are the document.

## 6. Playhead, marks, and dual timecode

The playhead is one vertical accent line from the ruler through every lane, at `pixelStart` of its sequence frame. It is the session playhead. During playback it moves to the frame the Application reported. The renderer does not interpolate a second position between frames.

Sequence In and Out are document marks. The ruler carries an accent bracket at In and a bracket at the Out boundary. The lanes wash the half-open span `[In, Out)` with a low accent. The frame at Out is outside the wash. Duration on the transport is `Out − In` in frames, also shown as timecode.

Source In and Out appear on the source viewer and on the transport, not as a second ruler on the sequence. When a clip is selected, the transport's source timecode is the source frame under the sequence playhead, mapped by the clip's policy. The clip label shows the source timecode of its record start.

During an open trim the transport adds the proposed delta in frames, the resulting sequence timecode of the edit point, and the resulting source timecode of the opened edge. Those readouts come from the preview snapshot.

## 7. Trim terminals and edit points

A terminal is a square cap, a few pixels wide, on the head or the tail of an item. It is drawn in secondary text at rest. Under the pointer it becomes a hairline preview of the edit point that a click would open: one cap for a head or a tail, two caps facing each other for a roll at a cut, both caps of one item for a slip, the outer caps of the neighbours for a slide. The preview is a drawing. It does not open the trim.

An open edit point replaces those caps with accent caps. A ripple also draws a thin accent line across the sync-locked lanes at that frame, so the editor can see which tracks will pad or close. A held track draws that line in the warning hue and shows the delta it would record. The limit span is a secondary hairline on the ruler, from the furthest legal frame on one side to the furthest legal frame on the other. The limiting item's name sits in the status line.

The pointer aims at a terminal when it is within a fixed inset of a few pixels from an edge, measured in view pixels. Inside the item, the pointer aims at the body: select, and drag only when the gesture is a move that a later command defines. A clip narrower than the inset is still reachable from the keyboard. The keyboard opens the head, the tail, the previous tail, the next head, the cut, the slip, or the slide of the item under the playhead on the focused track.

Join and abandon are actions. Join commits the preview. Abandon drops it. Neither depends on the pointer.

## 8. Snapping, zoom, and scrolling

Snap targets are cut frames, the playhead frame, In, Out, and marker frames. The snapped result is a frame index. A one-pixel accent guide shows the target during the gesture. Snapping is a session toggle. A modifier inverts it for one gesture. The key binding is later. The action is required.

Zoom changes the rational pixels-per-frame about the playhead: the frame under the playhead stays under the playhead. The keyboard uses a detent ladder. The pointer may set any rational between the minimum and the maximum. The minimum shows a full feature-length sequence in the lane width. The maximum gives a frame many pixels. Hit-testing follows the timebase rule, including the rule that a shared pixel hits the first frame in it.

Horizontal scroll changes the scroll origin in frames and the pixel remainder. Vertical scroll changes the first visible track and the pixel remainder inside that row. The rail scrolls vertically with the lanes and does not scroll horizontally. Scrolling does not change the playhead. A separate action scrolls so the playhead is centred.

The sequence overview and the scroll bar both show where the window sits in the whole duration. Dragging either is a scroll. Throwing the scroll does not skip the index: the renderer asks for the new visible range and paints what has arrived.

## 9. Markers and references on the surface

A sequence marker is a tick on the ruler and a short vertical hairline through the lanes, in secondary text, with the accent when it is the selected marker. A ranged marker is a bracket on the ruler. The text is a tooltip and a row in the inspector, not a card on the lane.

A clip-reference pin on a marker draws a small chain mark on that tick. Activating it runs match frame for the stored asset and source frame, once that command exists.

Source cues that fall inside a clip's source range draw as secondary ticks on the clip's top edge. They are properties of the asset. They move with slip, because the source window moved, and they stay on the same source frame.

Match frame and the usage search are commands available from the selected clip and from the playhead. Their results open the source viewer or a list in the bin column. The timeline does not draw a second results surface.

## 10. Keyboard actions the timeline must reach

Bindings are a later key map, owned by the shell. Every action below has a binding in that map, and every one of them has a menu item. No action exists only as a drag.

| Group | Actions |
| --- | --- |
| Playhead | Step one frame, step ten frames, step a typed count, go to start, go to end, go to In, go to Out, type a sequence timecode, type a source timecode. |
| Events | Previous cut, next cut, previous marker, next marker. During a preview, previous event and next event move the proposed boundary. |
| Marks | Mark In, mark Out, clear marks, park-swap with the playhead, mark the item under the playhead. |
| Operations | Insert, replace, remove, delete, split, and the five trim kinds. Join and abandon. |
| Aim | Focus next and previous track, target the focused track, clear targets, toggle lock, toggle sync lock, toggle enable, isolate for the next command. |
| View | Zoom in, zoom out, zoom to fit the sequence, zoom to fit the marked span, centre the playhead, toggle snapping, toggle the overview. |

Typed counts and typed timecode parse to integers or show the parser's error in the status line. They do not change the document until the action that uses the number is issued.

## 11. States the painter must tell apart

| State | Drawing |
| --- | --- |
| Empty sequence | Ruler at the origin, no items, playhead at the start frame, status "No sequence" until a sequence is open. |
| Item, media pending | Rectangle and label. Thumbnail or waveform slot empty. |
| Item, offline | Label in the warning hue, word Offline. Edges still from the record range. |
| Selected | Accent outline. |
| Link group | Chain mark. Shared outline when any member is selected. |
| Drift | Signed frame count on the rail, warning hue on the clip's chain mark. |
| Locked track | Lock mark, secondary name, gestures do not aim edits at it. |
| Open preview | Accent terminals, limit span, sync lines, delta on the transport. The committed edges remain underneath in secondary hairlines so the editor can see the change. |
| Focus | Accent hairline on the focused panel and the focused rail row, as the workstation specification requires. |

An inactive shell, the stage [UI_ARCHITECTURE.md](../UI_ARCHITECTURE.md) calls the visual shell, draws the empty lanes, the ruler, the static playhead, and the inactive provisions. It does not draw sample clips, and it does not animate. This specification is the picture those controls grow into when a review connects the engines. It is not that connection.

## 12. Comfort over a long session

The default density fits a feature stack of picture and dialogue tracks in the timeline band without hiding the record viewer. Compact lanes are the default for that reason. The editor can raise one track to medium or taller while reading a waveform, without raising the rest.

Hit targets for rail controls stay large enough to aim after many hours, even when the lane is compact: the control is a square of the row height, and the word is the name beside it. Trim terminals stay at a fixed pixel size when the clip is wide, and the keyboard path covers the case where the clip is narrower than a terminal.

The status line carries one message: the limiting item during a preview, the parser error, or the last command's sync report in a single clause. It does not stack toasts. Colour is quiet enough that the warning hue still means something when a take is offline or a track has drifted.

## 13. What this surface refuses to do

The timeline widget does not resolve a frame, does not map a rate, does not validate a link, and does not write a snapshot of its own. It sends frames and receives snapshots. If the snapshot is late, it keeps the last committed drawing and shows the playhead where the session says it is. It does not guess the result of a trim in order to feel responsive. The preview snapshot is the responsive path, and it is an Editorial result.
