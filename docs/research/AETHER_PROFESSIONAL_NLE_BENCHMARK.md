# Aether professional NLE benchmark

Inspection date: 2026-11-22. This document records user-visible professional editing behaviour from public documentation. It is not an implementation, and it does not accept an ADR. Avid and Lightworks names are references to those products. They are not Aether branding.

Product behaviour below is what the cited document says an editor sees. It is not evidence of that product's internal data structures, database, or undo mechanism.

## 1. Product vision

Aether is being built as an open-source, offline-first, editor-centred picture editor for long-form film, documentary, television, and commercial work. The ambition is the operational depth associated with a facility picture-editor's daily tools: source and record, three-point edits, explicit insert versus overwrite, lift versus extract, trim that an editor can predict, track targeting and sync, bins an assistant can log, keyboard editing, and a project that reopens on the same frame.

The goal is those capabilities, not the appearance, trademarks, or graphics of another application.

Accepted constraints that this benchmark does not reopen:

- ADR-002 is **ACCEPTED — INITIAL EDITORIAL TIME AUTHORITY**. Positions are `int64` frames. Rates are reduced integer ratios. The named rates are 24/1, 25/1, 24000/1001, 30000/1001, and 60000/1001. Storage is half-open `[start, end)`. Float seconds are not the clock. Source range and sequence range are distinct. Picture selection for one sequence-frame offset is the nearest source frame, and an exact tie stays on the earlier frame. That rule is frame selection, not a duration or speed mapping. Displayed inclusive versus exclusive out-points stay deferred.
- ADR-003 is **ACCEPTED**. One `project.db` is the editorial authority.
- ADR-005 is **ACCEPTED**. Media ID, Representation ID, and path are three different things.
- ADR-006 and ADR-007 stay **PROPOSED**. This research does not mark them accepted.
- The current sequence model has no track type. Same-track record overlap is rejected. Imported media is not probed. Playback is not implemented.

## 2. Avid Media Composer — what was read

Primary source opened as text:

- *Avid Media Composer Editing Guide*, Version 2025.12, at `https://resources.avid.com/SupportFiles/attach/Media_Composer/Media_Composer_v2025.x_Editing_Guide.pdf`
- Documentation index: `https://kb.avid.com/pkb/articles/en_US/Knowledge/Media-Composer-Documentation` (page last updated 31 August 2026). The fetched index lists year sections and did not itself include the workflow prose. The guide PDF is the workflow source used below.

A second Avid product page was found and is **not** treated as Media Composer desktop behaviour: `https://docs.avid.com/acc/Content/ACC_Help/Editor_3pt_Editing.htm` is MediaCentral / ACC help. Its three-point table is recorded only as a different product. MediaCentral Cloud UX overwrite help (`https://mediacentral.avid.com/MCCUX/Content/MCUX_Users_Guide/Performing_an_Overwrite_Edit.htm`) is the same separation.

The 2025.12 guide is long. Chapters actually read for this benchmark: marking and subclips, markers, match frame, the three primary edit functions, mixed frame rates, and lift/extract. The table of contents also names segment mode, trim rollers, sync lock, keyboard settings, and relink. Those titles are cited where the body was not read. They are not described as if the body had been read.

### Source and record

The guide's editing chapter loads a clip into the Source monitor and marks the sequence in the Record monitor. IN and OUT can be marked in a monitor or, in Frame or Script view, on a clip still in the bin. Default US keyboard marks are I and O. Clearing IN, OUT, or both has separate controls. Alt/Option-drag moves a mark. If an insert has no record IN, the guide says the splice uses the current position indicator.

Mark Clip marks the material between the first aligned cuts on the selected tracks. Alt/Option-click Mark Clip ignores the current track selection and uses the nearest edit points.

### Master clips and subclips

A subclip is made from marked footage and stays linked to the master clip. The master clip references the media files. The guide says ordinary subclips do not block a later trim past the original marks. Subclips created in 24p or 25p projects are "hard" and cannot be trimmed past those edges. New subclips get a subclip icon and a numbered `.Sub` name. Batch Create Subclips can use entire clips or marks, chosen tracks, extra head and tail frames, and retained marks or markers.

### Markers

Markers are frame bookmarks with comments and one of sixteen colours from the Markers tab of the Command palette. They appear in the monitor, the position bar, and the Timeline. The guide's suggested uses include colour notes, visual sync alignment, music cues, trim return points, and EDL comments. Find can search marker keywords. Import and export of markers is named. The internal storage of a marker was not described.

### Three edit functions

The guide names three ways to add material: Insert (splice-in), Overwrite, and Replace.

- Insert / splice-in inserts marked source material and moves existing material later. The sequence gets longer. The documented procedure marks source IN and OUT, marks a sequence IN (or uses the position indicator), and clicks the yellow Splice-in button.
- Overwrite replaces a section and does not lengthen the sequence unless the new material extends past the current end. The documented procedure marks one source point, not both, and marks both record IN and OUT (or an OUT plus the position indicator as IN), then clicks the red Overwrite button.
- Replace keeps the IN and OUT of the sequence segment being replaced. The editor parks a sync frame in the source and a sync frame in the sequence and clicks the blue Replace Edit button. The system calculates source IN and OUT from those sync frames and the existing sequence segment. The guide says to check durations first, because an overlap edit can replace the wrong segment if the position indicator sits in the overlap. Sync Point editing is described as similar but ending at the nearest marks rather than always filling the sequence segment. The Replace button's default location is the Edit tab of the Command palette, not necessarily a monitor button.

The general rule, in the guide's words, is that most edits are three-point edits: three marks, either two in the source and one in the sequence or the reverse, and the fourth mark is determined automatically. Phantom marks and single-mark editing are separate settings. Single-mark editing is off until enabled under Composer Settings, Edit tab. With it on, the position indicator supplies the second mark, including an edit "on the fly" while playing.

This research did **not** find, in the pages read, a formula for unequal source and record durations, a rational-rate conversion, or an integer-frame tie rule. Mixed-rate behaviour is a different section, below.

### Lift and extract

These are not synonyms in the guide.

- Lift removes the marked material on selected tracks and leaves black or silence. Track or sequence duration stays the same. The removed material goes to the Clipboard.
- Extract removes the marked material and closes the gap, shortening the track or sequence. The guide says that if sync locks are on, all material on all tracks is extracted. The removed material also goes to the Clipboard.
- Copy to Clipboard duplicates the marked, track-selected material and leaves the sequence unchanged.

### Mixed frame rates

A project has a project frame rate. Clips of other rates are "mixed rate clips". The guide says they play at the project frame rate in Source and pop-up monitors and in sequences, and that audio stays synchronised. Media Composer applies a Motion Adapter effect automatically. The editor does not apply it from the Effect Palette. A Speed percentage is displayed as the value that makes the clip play at the project rate. Other Motion Adapter parameters are inactive until the effect is promoted to a Timewarp. Unrendered mixed-rate timeline clips show a green dot and the original frame rate appended to the name.

That is product behaviour. It is not a published integer mapping this research can adopt. Aether's accepted picture rule is nearest-frame selection for one offset, with the earlier frame on a tie. It is not a speed percentage, and it is not a duration rule. No source-to-record speed mapping is proposed here.

### Match frame

Match Frame loads the source clip for the frame in the monitor. Reverse Match Frame cues the sequence to a source frame, then to later matches on later clicks. The button is described as present by default in the second row under the Source monitor. Alt/Option and Alt+Ctrl / Option+Command change which media relative is loaded. The exact identity fields used to match were not specified in the pages read.

### What the guide names but this pass did not read in full

The 2025.12 table of contents includes, and this pass did not quote the body of:

- Segment modes (Lift/Overwrite and Extract/Splice-in). A 2019.6 what's-new note, a different document, describes red and yellow segment arrows. It is not used as the 2025 behaviour.
- Timeline trim: overwrite trim, ripple trim, dual-roller trim, sync-lock trim, J-K-L trim, trim review.
- Lock versus sync lock.
- Keyboard settings as user settings, and button mapping from the Command palette. The pages read do say Mark IN defaults to I and Mark OUT defaults to O on US keyboards, undo is Ctrl+Z, and redo is Ctrl+R. They also say undo can list up to 100 actions, and Composer Settings can limit undo to record events so source marks survive an undo of a sequence edit.
- Relink by resolution, to selected clips, for tape and file media, for consolidated clips, and for moved projects. The body of those procedures was not read. No claim is made here about match keys or about whether a relink rewrites a cut.

Find Bin is named in a monitor-button caption (Make Subclip, Find Bin, Add Marker). The procedure was not read.

## 3. Lightworks — what was read

Official knowledge-base articles, each last updated 16 October 2026:

- `https://lwks.com/lightworks-knowledge-base/basic-editing`
- `https://lwks.com/lightworks-knowledge-base/trimming`
- `https://lwks.com/lightworks-knowledge-base/selecting-and-moving-material`
- `https://lwks.com/lightworks-knowledge-base/viewers-and-playback`
- `https://lwks.com/lightworks-knowledge-base/bins-collections-and-searching`
- `https://lwks.com/lightworks-knowledge-base/keyboard-shortcut-tables`

These describe user-visible behaviour. They do not describe Lightworks' database, file format, or undo stack.

### Source and sequence

There is a source viewer and a sequence viewer. Headers use a blue icon for source and a red icon for sequence. The viewers are independent: marks in the source do not move the sequence playhead. Layouts can stack them, place them side by side, or keep several sources and sequences as tabs. A subclip button exists on the source viewer. Playback quality can drop to a proxy. That is a viewing control in the article, not a description of how proxies are generated.

### Marks

`I` marks in. `O` marks out. `P` clears marks. Mark and park means an in plus the playhead, with no out stored; the current position is the end. `Shift`+drag moves a marked region without changing its length. Source monitor buttons affect what is seen and heard, not what is edited.

### Three-point editing, as this article states it

The basic-editing article says three-point editing sets three of four in and out points and Lightworks works out the fourth. The two cases it writes down are:

- Source in and out, plus a timeline in: the clip lands at the timeline in with the marked length.
- Timeline in and out, plus a source in: the source fills the marked timeline span.

It does not, in that article, spell out a source-out plus record-out case or a source-out plus both record marks. Those are not invented here as Lightworks behaviour.

### Operations, and they are not interchangeable

| Operation | Documented consequence |
| --- | --- |
| Insert | Pushes existing material later and lengthens the sequence. Tracks not receiving the insert are padded with black so alignment holds, unless they are outside the sync group (locked, or the same exclusion as Sync off). |
| Replace | Also called overwrite in the same section. Lays material at the playhead and keeps the sequence length. |
| Remove | Takes the marked region out and leaves a gap. |
| Delete | Takes the marked region out and closes a gap, but only by the black every affected track can give up. If another track has picture or sound there, the close is limited or does not happen, so that track is not pulled out of step. A later "delete the remaining gap" can desynchronise tracks. The article states both the simple "closes the gap" wording and this black-limited rule. |
| Split | Cuts at the playhead and removes nothing. |

Clipboard cut is delete plus a copy. Remove to clipboard leaves a gap and keeps the material. Paste insert and paste replace do not consult track targets; they return to the tracks the copy came from. Lock is what keeps a track out of a paste. If every track is locked, the commands report that instead of doing nothing.

### Trim

Two gestures: drag an end, or unjoin then move then join. Unjoin curls the chosen end. `Tab` joins, and `Tab` again reopens the last trim. Keyboard unjoin, with the playhead over the clip:

| Key | Opens |
| --- | --- |
| Q | End of the previous clip |
| W | Start of the current clip |
| E | End of the current clip |
| R | Start of the next clip |
| U | Move cut (nearest cut) |
| Y | Slip |
| T | Slide |

`W` and `E` together are a slip. `Q` and `R` together are a slide. `Q`+`W` or `E`+`R` are a move cut. The same key again rejoins that point.

Move cut, slip, and slide do not change sequence length, so the Sync button does not change them. A start or end trim with Sync on ripples later material on the tracks that are in the sync group. Sync off leaves black instead of rippling (Drag In Black). A gap trim always ripples. A yellow line shows how far the trim can move, limited by the unjoined point with the least media. "Trim limit" appears when media runs out. `Alt`+`Space` previews with pre-roll and post-roll. `J`/`K`/`L` can move the trim if "Trim when playing" is on.

Track targets: a lit target trims that track's clip; an untargeted track trims black so it still moves with a ripple. No targets lit means the top clip under the playhead plus its audio. `Alt`+click a target limits the move to those tracks. A locked track never moves.

Safe trimming defaults to on and stops at the end of media. "Allow Trims To Overwrite Segments" defaults to off.

### Selection, link, and sync

Clip Link (`Ctrl/Cmd`+`M`) selects and moves linked picture and sound together. `Alt`+click inverts that for one click. Grouped tracks, such as a stereo pair, stay together even when Clip Link is off. Links are stored, not recomputed: one edit action links the clips it lays down; a cutaway laid over existing material does not join that group; a split divides the group. `Ctrl/Cmd`+`L` links or unlinks a selection.

Track Sync is a different control. It keeps tracks that are not the ones being edited in step when a trim, insert, or delete ripples.

Deactivate (`Alt`+`D`) hides or mutes one segment without removing it.

### Bins and assistant organisation

A clip is a log pointing at media. Deleting the log does not, by itself, delete the file, and deleting the file is a separate explicit step. Bins are manual containers. A clip can sit in more than one bin by reference. Collections are saved filters and do not own clips. Tags are metadata labels with OR, AND, and NOT. Searches can be active (live) or static (frozen). List columns are configurable per project and per user. Colour labels can be shown. Multicam uses a sync-group bin. Offline clips are a documented search criterion ("has or has no media, offline clips"). The article does not describe a relink algorithm.

Cue markers use `M` to add and `Shift`+`Alt`+`M` to remove the cue at the playhead.

### Keyboard

Defaults are a user-reassignable map. The article says the Keyboard Shortcuts panel can print the live map. Factory defaults are listed in the keyboard article cited above. This benchmark does not copy those keys into Aether.

### Project integrity, as far as the articles go

Basic editing says every edit is undoable and refers to continuous auto-save. It does not say how the save is stored, how many undo steps exist, or what survives a crash. Those mechanisms are unknown from this source.

## 4. Comparative analysis

| Concern | Media Composer 2025.12, pages read | Lightworks KB, 16 Oct 2026 | Implication for an editor-centred model |
| --- | --- | --- | --- |
| Two viewers | Source monitor and Record monitor | Source viewer and sequence viewer, independent playheads | Marks and playheads are per side. A sequence position is not a source position. |
| How much is marked | IN and OUT, or the position indicator as a phantom mark | IN and OUT, or mark-and-park (playhead stands in for the end) | A missing point must be defined. It must not be a silent speed change. |
| Add without moving later cuts | Overwrite | Replace / overwrite | Later record positions stay. |
| Add and push later cuts | Insert / splice-in | Insert, with black padding on tracks that stay in sync | Later record positions move by the inserted length on the sync set. |
| Replace one segment on a sync frame | Replace edit | Not described in the articles read | A distinct operation. Not overwrite. |
| Take out and leave a hole | Lift | Remove | Other record ranges stay. |
| Take out and close up | Extract, and sync lock can pull every track | Delete, limited by black on the other tracks | Closing a gap is a sync decision, not a synonym of delete. |
| Cut in two | Add edit is named in the TOC; body not read | Split | One instance becomes two. Nothing is removed. |
| Trim that keeps duration | Dual-roller is named; body not read here. 2019 note is a different document | Move cut | One side gains the frames the other loses. |
| Trim that changes duration | Ripple trim is named; body not read | Unjoined start or end with Sync on | Downstream record positions move, under a sync policy. |
| Slip / slide | Named in the TOC; body not read | Y and T; they do not ripple | Slip changes source in and out together. Slide moves the record span between neighbours. |
| Track choice | Track Selector panel. Extract with sync locks affects all tracks | Track targets, locks, Clip Link, Track Sync | Targeting, patching, lock, and sync are different controls. |
| Bins | Bins, master clips, subclips, markers, bin marks | Logs, bins, collections, tags, searches, cues | A bin membership is not the media file. |
| Undo | Up to 100 listed actions. Optional record-only undo | "Every edit is undoable." Depth and persistence not stated | One editorial command should be one undo unit. |
| Mixed rates | Motion Adapter plays at the project rate | Not stated in the articles read | Unknown for Lightworks. Not a licence to invent Aether's mapping. |

## 5. Capabilities that matter before finishing tools

For a first professional cutting room, the documents put these ahead of colour, effects, and interchange:

1. Independent source and record positions, marks, and timecode labels.
2. Insert and overwrite as different commands.
3. Lift and extract as different commands.
4. Split.
5. Trim that can ripple, roll, slip, and slide, with an explicit sync policy.
6. Track target, lock, and sync as separate from how the track is drawn.
7. Bins, clip logs distinct from files, markers, and a way to find a source from a timeline frame.
8. A keyboard map the editor can change, with context (source, record, timeline, trim).
9. A project that reopens committed cuts. Autosave is claimed by Lightworks and not specified. Media Composer undo depth is specified; its crash file format was not read.

Proxy, multicam, relink, and exchange are real facility needs. The Media Composer relink chapters were not read, so their match rules are unknown here. Lightworks can search offline clips and can delete a log without deleting a file. That is not a relink procedure.

## 6. Known and unknown

Known from the documents cited above: the operation table, the mark keys that those documents state, the Lightworks trim keys, the Lightworks bin model, and the Media Composer Motion Adapter description.

Unknown, and not filled in:

- Media Composer segment-mode hit regions, dual-roller mathematics, and play-loop trim, beyond the table-of-contents titles and the lift/extract pages.
- Media Composer relink match keys and whether relink rewrites sequence rows.
- Media Composer and Lightworks internal time representations. Neither document is evidence for or against integer frames.
- Lightworks behaviour when source and sequence frame rates differ.
- Lightworks four-point case list beyond the two cases written in the basic-editing article.
- Either product's on-disk project format, from these sources.
- Any Avid or Lightworks source code. None was sought, and none is assumed to be public.
