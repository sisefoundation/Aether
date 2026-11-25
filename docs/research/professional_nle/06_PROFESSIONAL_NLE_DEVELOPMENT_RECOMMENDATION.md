# Professional NLE development recommendation

Inspection date: Sunday 22 November 2026, Asia/Calcutta. This is a proposal. It does not accept ADR-006, ADR-007, or ADR-008. It does not change ADR-001, ADR-002, ADR-003, or ADR-005. It does not adopt MLT. It does not restart Slice 1D.

## Order

1. Leave Slice 1D as the validated same-rate placement baseline. Do not rebuild it.
2. Source and record marks, same rate only. The four three-point arrangements in the editorial model derive the missing mark with `identityRecordEnd`. A different rate still refuses. No speed percentage.
3. Overwrite on one track: equal lengths, split or shorten the covered instances, no shift of later instances. This is the first edit that is not "refuse on overlap".
4. Split, then lift of a marked range. Lift leaves the hole. It does not close it.
5. Stop there for ripple. Insert/splice and extract/delete wait on a sync decision: which other tracks move, and whether a locked track is skipped or blocks the command. Avid's sync lock and Lightworks's delete-by-available-black are different. The proposal does not pick one.
6. Roller trim, slip, and slide after the length-preserving contracts are written. They do not change sequence length, so they do not wait on the ripple rule. They do wait on a media-limit check, and that check waits on a known duration. Import does not probe.
7. Playback, probe, proxy, relink, and interchange stay behind ADR-006, ADR-007, and the FFmpeg library block. No player is slipped in beside step 2.

Bins and a keyboard map stay proposed. They are not the next code change.

## Contracts to write before code

- Destination tracks are an explicit list on the command. There is still no track-type column.
- Undo is one session command per edit, covering every row that changed. It still dies on close until a later decision.
- A failed validation writes no row and pushes no command.
- Mixed-rate placement stays the existing refusal until a conversion service exists. This recommendation does not design that service and does not map 25/1 onto 24/1.

## What not to do

- Do not treat the menu word Insert as splice-in.
- Do not link MLT, and do not copy GPL or LGPL sources into the tree.
- Do not store float seconds, inclusive zone lengths, or a scene FPS pair as the clock.
- Do not open ShellCut, PlaceCut, IntegCut, or any `project.db` as part of the next implementation until that work has its own brief.
- Do not mark ADR-006 or ADR-007 accepted in order to start a player.

## First useful milestone after this research

One sequence, one track, same rate, with marks: overwrite a range, undo it, redo it, close, reopen, and see the committed overwrite with no undo stack. Insert remains unimplemented until the sync decision is written down.
