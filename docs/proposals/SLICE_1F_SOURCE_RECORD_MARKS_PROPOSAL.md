# Slice 1F source and record marks proposal

Status: proposal for the marks foundation only. It does not accept an ADR, does not change a schema, and does not authorise overwrite, split, lift, ripple, playback, probing, or FFmpeg.

Inspection date: 2026-11-22. Tree: `/home/editry/development/sise-editor`. There is no git repository. The professional NLE notes that were read are `docs/research/professional_nle/01_PROFESSIONAL_WORKFLOW_BENCHMARK.md`, `03_AETHER_PROFESSIONAL_EDITORIAL_MODEL.md`, `04_TIMING_AND_MIXED_RATE_GAP_ANALYSIS.md`, `05_AETHER_ARCHITECTURE_GAP_ANALYSIS.md`, `06_PROFESSIONAL_NLE_DEVELOPMENT_RECOMMENDATION.md`, and `RESEARCH_INDEX.md`. Those files are not rewritten here.

## What this slice is

An editor can park two frame points on the selected representation and two frame points on the selected sequence, see only the points that belong to the current selection, and clear them. A stored source pair may be shown in the existing placement dialog. The dialog still calls `placeInstance`. No new edit operation is added.

## Marks are points

ADR-002 stays authoritative. Internal edit ranges stay half-open `[start, end)`. A mark is a frame index in one domain, which is the vocabulary in `03_AETHER_PROFESSIONAL_EDITORIAL_MODEL.md`. Displayed inclusive OUT remains deferred there and in `04_TIMING_AND_MIXED_RATE_GAP_ANALYSIS.md`. This slice does not invent that convention.

A stored Mark In and Mark Out with `out > in` are two ordered points. They are not converted to `[in, out)` or `[in, out + 1)`. The shaded span between them is not drawn. Three-point derivation with `identityRecordEnd` is not run on the marks. Equal points are rejected as an incomplete range and are not stored. Mark Out before Mark In is rejected. A single stored point stays a point.

## Source marks

Source marks belong to one representation id, in that representation's native frame index. The window does not know a source rate: import still records technical metadata as `Not probed`, and this slice does not guess a rate, a duration, or a picture. The sequence rate is not copied onto the source marks.

The commands are Mark In, Mark Out, Clear In, Clear Out, and Clear Both. The number is typed in a field labelled `source frames`. Switching the bin selection shows the newly selected representation's points and does not show another representation's points. The typed field is cleared on that switch so a typed number is not carried across. The previous representation's points remain in the session until cleared or the project session ends.

Missing representation, a closed project, a negative frame, and a non-integer token are rejected. The previous points stay as they were. The window shows the reason.

## Record marks

Record marks belong to one sequence id, in sequence frames. The same five commands apply. Switching the sequence list shows that sequence's points and does not show the previous sequence's points. Those previous points remain stored for the session.

The typed field is labelled `sequence frames`. A left click on the timeline ruler fills that field through the existing `frameAtCanvasX` map: origin 48, 4 pixels per frame, `floor((x - origin) / pixelsPerFrame)`. That map is already covered by `TimelinePlacementTests`. The click does not store a mark and does not select a clip. Mark In and Mark Out store the integer from the field. The painter draws each stored integer with `timelinePaintX`, the same scale in the other direction. There is no zoom control. Scrolling the existing timeline view does not write the stored integers. Pixels are not the authority.

A click on a clip still selects that instance only. Mark lines are full-height strokes. Clip selection stays the existing block fill. No range is filled between the marks.

## Session ownership

The points live on `ProjectSession` in two maps, keyed by representation id and by sequence id. They are cleared in `resetSessionState`, so create, open, and close drop them. They are not written to `project.db`. Schema version stays 2. No new table. `kPrototypeVersion` stays `slice-0a`.

Mark commands do not push an undo entry and do not call `placeInstance`, `insertInstance`, `moveInstance`, `trimInstance`, or `removeInstance`. Undo and redo of a cut leave the marks untouched. A refused mark leaves the map and the cut untouched.

The research notes do not settle mark persistence. ADR-009 session undo is for editorial commands and is unchanged. Marks are not editorial rows.

## Placement

When the selected representation has both points and the out point is greater than the in point, `PlacementDialog` shows those integers as an offer. It does not copy them into Source in or Source out. The editor still chooses a source rate and types a half-open source span. `identityRecordEnd` and `placeInstance` are the same checks as Slice 1D. A different rate is still refused and writes nothing. A successful place stores the typed span, not the mark points.

## Keyboard

Existing menus and buttons have no `QKeySequence`. This slice does not add one and does not add a keymap. Every command is a button. The ruler click is a mouse position entry for the record field.

## Out of this slice

Overwrite, insert, split, lift, ripple, trim rollers, playback, transport that moves a playhead, decoding, probing, and a displayed inclusive out point. Later slices stay unauthorised.
