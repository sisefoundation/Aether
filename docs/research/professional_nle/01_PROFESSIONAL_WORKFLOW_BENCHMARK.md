# Professional workflow benchmark

Inspection date: Sunday 22 November 2026, Asia/Calcutta. Product behaviour only. These documents are not evidence of either product's database, undo stack, or time representation. Avid and Lightworks names are references, not Aether branding. MediaCentral pages are not used as Media Composer behaviour.

## What was opened

Media Composer 2026 index, last updated 10 November 2026:

`https://kb.avid.com/pkb/articles/en_US/Knowledge/Media-Composer-2026-Documentation`

That page lists *Media Composer v2026.x Editing Guide*. The file read is:

`https://resources.avid.com/SupportFiles/attach/Media_Composer/2026/Media_Composer_v2026.x_Editing_Guide.pdf`

The extract's title block says *Avid Media Composer Editing Guide Version 2026.8*, created 31 August 2026. Sections whose body was read are named below. A chapter that is only a table-of-contents title is marked unknown.

Lightworks, each article last updated 16 October 2026:

- `https://lwks.com/lightworks-knowledge-base/basic-editing`
- `https://lwks.com/lightworks-knowledge-base/trimming`
- `https://lwks.com/lightworks-knowledge-base/sequences-and-timelines`

## Media Composer 2026.8 — read

### Source and record

Editing loads a clip into the Source monitor and a sequence into the Record monitor. Marks can be set in a monitor. The first-edit procedure selects tracks in the Track Selector, then uses Splice-in. The Record monitor then shows the end of the last frame of the new edit. Playback of a sequence in either monitor is limited to tracks currently monitored in the Track Selector. The monitoring chapter's own procedure was not located in the extract, so how a monitor button differs from a selected track is **unknown** beyond that sentence.

### Marks

Default US keys: Mark IN is I, Mark OUT is O. Audio Mark IN and Audio Mark OUT are separate Command-palette buttons and can mark picture and sound apart. They appear in the Timeline and in the position bar.

Three-point editing, in the guide's words: three marks, two in the source and one in the sequence, or the reverse, and the fourth mark is determined automatically. Phantom marks, a Composer setting, draw the calculated points. With one source IN and no other marks, the guide says the position indicator is the sequence IN and both OUT points are calculated from the length of the source clip. The word "four-point" was not found in the sections read. What happens when all four marks are already set, and they disagree, is **unknown**.

### Splice-in, overwrite, replace, sync point

- Insert / splice-in inserts marked source material and moves existing material later. The sequence gets longer. Procedure: source IN and OUT, sequence IN (or the position indicator if no IN), yellow Splice-in button.
- Overwrite replaces a section and does not lengthen the sequence unless the new material goes past the current end. Procedure as written: one source mark, not both, and both record IN and OUT (or record OUT plus the position indicator as IN). Red Overwrite button.
- Replace keeps the IN and OUT of the sequence segment. The editor parks a sync frame in the source and a sync frame in the sequence. The blue Replace Edit button is on the Edit tab of the Command palette by default. The guide says replace always fills the existing segment. Sync Point editing is similar but ends at the nearest marks. Sync Point needs a sync frame on each side and a duration from marks. It is enabled in Composer Settings or Composer > Sync Point Overwrite, then the Overwrite button is used. An orange mark on that button means it is active. Marks may sit on one monitor or be split across monitors.

### Lift and extract

Lift removes marked material on selected tracks and leaves black or silence. Duration stays the same. Extract removes it and closes the gap. Both put the material on the Clipboard. If sync locks are on, the extract sentence in this edition says all material on all tracks is extracted. That sentence is in the lift/extract section. The full track-selector chapter was not read, so the interaction of an unselected track with that sentence is only as good as that one sentence.

### Segment mode

The guide names two segment tools: Lift/Overwrite and Extract/Splice-in. A yellow arrow is called Segment insert in the sync-break fix. Disabling those tools is mentioned as a way to click a segment. The hit regions, and whether red and yellow still mean overwrite and splice, are **unknown** in this 2026 extract. A 2019 what's-new note is a different document and is not used here.

### Tracks

Read: the Track Selector chooses which source and record tracks an edit uses. A talking-head example selects V1 and A2 and deselects A1. Sync Lock keeps several tracks together. Inserting into one sync-locked track inserts filler in the other. Sync lock can include any combination of video and audio tracks. It applies to a whole track. While trimming, sync lock applies only to single-roller trims, because dual-roller trims do not break sync. A dual-roller trim maintains duration. A single-roller trim can break sync. The step-by-step patching procedure (TOC page 631) was **not** found in the extract. The step-by-step trim-mode entry, play-loop review, and multi-track trim roller selection were **not** found as their own sections. Those remain **unknown**.

### Match frame, find bin, markers, subclips

Match Frame uses the Track Selector. Several enabled tracks match the highest selected track, descending V1, A1, A2. Right-click a track number and choose Match Frame Track to match one track. Find Bin, with the Source monitor active, highlights the loaded clip. With the Record monitor active, it highlights the sequence. Alt/Option-click Find Bin on a sequence frame highlights the clip under the position indicator. Right-click a segment and choose Find Bin. Match Frame twice, then Find Bin, walks from a subclip in the sequence to the master clip in the bin.

A subclip stays linked to its master clip. The master references the media. Ordinary subclips can be trimmed past their original marks. Subclips made in 24p or 25p projects are hard: the trim cannot pass those edges. New subclips get a subclip icon and a numbered `.Sub` name.

Sixteen marker colours exist on the Markers tab. Markers are frame bookmarks with comments. The storage format is not described.

### Keyboard and undo

Keyboard settings and toolbar mappings are user settings. Bin layouts are project settings. Changes to the Keyboard palette are saved in the Keyboard settings. Undo and redo list up to 100 actions. Undo is Ctrl+Z. Redo is Ctrl+R. The full factory map was not read.

### Long-form and audio

Multiple sequences, sequence templates, and tail leader for sync are named. Audio tracks are real tracks in the selector (A1, A2). Audio marks are separate from picture marks. How many audio tracks a sequence may hold is **unknown** from the pages read. Relink procedures were not opened. Mixed-rate Motion Adapter behaviour was not re-read in this 2026 pass; it is not restated here as 2026 fact.

## Lightworks — read

### Viewers and three-point

Source viewer and sequence viewer. Source monitor buttons change what is seen and heard, not what is edited. Track enable on the source chooses which of its tracks take part. Track enable on the sequence chooses where they land. Three-point, as written, is two cases only:

- Source in and out, plus a timeline in: the clip lands at that in with the marked length.
- Timeline in and out, plus a source in: the source fills that timeline span.

A source-out plus record-out case is not written. Mixed frame rates are not written. Both are **unknown**.

### Operations

| Name | Documented result |
| --- | --- |
| Insert | Pushes later material and lengthens the sequence. Tracks sitting out the insert are padded with black so they keep their alignment. Tracks taken out of the sync group stay where they are, described as the same as every untargeted track with Sync off. |
| Replace | Also called overwrite in the same section. Lays material at the playhead. Sequence length stays. |
| Remove | Takes the marked region out and leaves black or silence. |
| Delete | Closes the gap by the black available across all tracks. Where another track has less black, the close is only that shared amount. Where a track has no black, nothing closes. The page says this form of delete does not knock a track out of sync. Selecting the remaining gap and deleting it can pull tracks out of step. |
| Split | A cut at the playhead. Nothing is removed. |

Clipboard cut is delete plus a copy. Remove to clipboard leaves a gap. Paste returns to the tracks the copy came from. Track targets do not steer a paste. Lock does. If every track is locked, the command says so.

### Drag and drop zones

A bin clip can be dragged onto the timeline. The viewer shows the frame under the drop. A Drop Zone is the strip above the video tracks or below the audio tracks. A drop there creates as many tracks as the material needs, keeps source grouping (a stereo pair stays a pair), and one undo reverses it. An inserting drop makes room on every track. Lock keeps a track out. Snapping aligns to cuts and the playhead.

### Tracks

Video tracks are `V1`, `V2`, higher numbers above. Audio tracks are `A1`, `A2`, mixed together. The article says up to 24 video and 32 audio, plus effects tracks. A new sequence is Auto until the first clip sets a compatible format. The first clip sets the output format. That is a product sentence, not a rational-rate rule for Aether.

With no track target lit, edits use every track, and a trim key trims the top clip under the playhead plus its audio. A click narrows the target. A locked track cannot be targeted. Lock refuses edit, trim, and cut, and the track is not pulled along for sync. Monitor hides video, including on export, or mutes audio. Solo is a separate state. The amber Track Sync strip shows which tracks will move with a ripple. Slip, slide, and move cut do not ripple, so those strips stay dark.

### Trim

Unjoin, move, join. `Tab` joins. `Tab` again reopens the last trim. Keys, playhead over the clip: Q previous out, W in, E out, R next in, U move cut, Y slip, T slide. `W`+`E` is slip. `Q`+`R` is slide. `Q`+`W` or `E`+`R` is move cut. Move cut, slip, and slide do not change sequence length.

Sync on ripples later material. Sync off leaves black (Drag In Black). A gap trim always ripples. Nudge is left/right arrow, one frame, or Shift plus arrow, ten frames. `Alt+.` and `Alt+,` take a typed frame count. A yellow line shows remaining source. Trim limit appears when media runs out. Safe trimming defaults to on. Allow Trims To Overwrite Segments defaults to off. Clip Link defaults to on, so linked picture and sound unjoin together.

### What Lightworks does not document here

Internal time format, project-file crash format, and mixed-rate mathematics. Continuous auto-save is claimed. The mechanism is unknown.

## Comparison that matters for Aether

Insert and overwrite are different. Lift/remove and extract/delete are different. A dual-roller, move cut, slip, and slide do not change sequence length. A single-roller or an unjoined end can ripple, and the other tracks move only under an explicit sync or lock rule. A menu label is not the operation. None of these pages authorise a speed change when the source rate and the sequence rate differ.
