# Slice 1H split editing proposal

Status: owner decision recorded, then implemented. The owner chose alternative 1 on 2026-11-23. That choice is closed. This document does not accept an ADR, does not change schema version 2, does not add a table, and does not change `kPrototypeVersion`. ADR-006 and ADR-007 stay **PROPOSED**. ADR-008 stays **OPEN — PENDING LEGAL COUNSEL**. Accepted ADRs were not edited. Overwrite behaviour was not changed. Slice 1I was not started.

Inspection date: 2026-11-22. Tree: `/home/editry/development/sise-editor`. `git status` failed: `fatal: not a git repository (or any parent up to mount point /)`, exit 128. No repository was created.

## 1. What split would be

Split is its own command. It is not a sequence of move, trim, or overwrite. It takes one selected instance and one integer sequence frame `f`. The record partition is `[r0, f)` and `[f, r1)`. It is valid only when `r0 < f < r1`. A split at the start or the end is rejected. That rejection writes nothing, adds no undo entry, and creates no zero-length instance.

Both results stay on the same track, with the same Media ID and the same master-clip association. The representation is not a column on `timeline_instances`. The instance keeps the media id copied from the master clip. There is no gap, overlap, ripple, downstream move, or sequence-length change. Other tracks stay. Marks stay session points, off `project.db`. One transaction. One undo entry. A failed split writes nothing and adds no undo. A committed split would survive close and reopen. Undo stays session-only, which is ADR-009. The window would call `ProjectSession` only. It would not run SQL, call FFmpeg, infer `f` from a mouse coordinate, or read a mark as the split position.

Those rules are the command shape. The source cut is the closed decision in section 3.

## 2. Source mapping gate

The source partition `sourceSplit = s0 + (f - r0)`, then `[s0, sourceSplit)` and `[sourceSplit, s1)`, is exact only when one source frame corresponds to one record frame. The live model does not guarantee that for the instances a split command can be asked to cut.

`timeline_instances` stores `source_start`, `source_end`, `record_start`, and `record_end` as separate integers. The checks require each span to be half-open. They do not require the lengths to be equal. There is no speed column. Schema version is 2. `kPrototypeVersion` is `slice-0a`.

`validateInstancePlacement` requires two accepted rates and two half-open spans, and it refuses a same-track record overlap. It does not compare the two lengths. It does not call `pictureSourceOffset`.

`placeInstance` does require the identity. `identityRecordEnd` accepts only the same reduced rate and sets the record length equal to the source length. A caller-supplied record end that is not that value fails with "record end is not the same-rate source span". A live call of `placeInstance` with source `[0, 8)` and record `[40, 47)` at 24/1 returned that message and wrote nothing.

`insertInstance` does not call `identityRecordEnd`. `SequenceTests::persistenceReopenKeepsBothRanges` stores source `[0, 25)` at 25/1 and record `[0, 24)` on a sequence at 24000/1001, closes, and reads both ranges back. That slot passed on the existing `build-slice1g/tests/aether_slice0a_tests` binary on 2026-11-22. `SequenceTests::operationsMoveTrimRemoveAndRejectOverlap` also passed. The same binary reported the two names as missing on the other test classes, so this was not a full-suite run and it is not a new count.

The same insert was repeated against `build-slice1g/src/libaether_session.a` in a disposable project that was removed afterwards:

- `insertInstance` of source `[0, 25)` at 25/1 and record `[0, 24)` on 24000/1001 succeeded.
- At record offset 12, `s0 + (f - r0)` is 12.
- `pictureSourceOffset(12, 24000/1001, 25/1)` returned 13. The timing error code was 0.
- Those are two different source frames for one sequence frame.

`trimInstance` also skips `identityRecordEnd`. It writes the four endpoints after `validateInstancePlacement`. The same disposable run trimmed a 24/1 instance from source `[0, 10)` and record `[0, 10)` to source `[0, 10)` and record `[0, 20)`. The trim succeeded. An interior record frame `f = 10` gives `sourceSplit = 10`, which is `sourceEnd`. The right source span would be empty. The record split is interior, and the identity formula still does not produce two non-empty source spans.

The timeline trim fields pass four independent integers to `trimInstance`. A stored unequal span is reachable from the window, not only from the older insert command.

`overwrite` is a different command and was not changed. Its replacement must have equal source and record durations. `retreatTail` and `advanceHead` then move a source endpoint by the record delta. That succeeds only when the leftover source span stays half-open. It is not a guarantee that every stored instance is one-to-one, and it is not a split rule. `pictureSourceOffset` selects one source frame. ADR-002 says that selection is not a duration rule. `identityRecordEnd` says a different rate has no authorised duration mapping.

So a selected instance can be one-to-one, unequal at the same rate, or unequal at two accepted rates. For the last two, `s0 + (f - r0)` is not the unique source frame that belongs to sequence frame `f`.

## 3. Alternatives

The owner chose alternative 1 on 2026-11-23. It is closed. The command refuses a split unless the stored source duration equals the stored record duration and the two rates are equal after reduction. Only then is `sourceSplit = s0 + (f - r0)`. `pictureSourceOffset` is not used. Unequal and mixed-rate instances fail, write nothing, and add no undo entry. No conversion was added for them.

1. **Accept only an instance that is already one-to-one.** Adopted. Require the source rate to equal the sequence rate after both ratios are reduced, and `sourceEnd - sourceStart` to equal `recordEnd - recordStart`. Then `sourceSplit = s0 + (f - r0)` lies strictly between `s0` and `s1` exactly when `r0 < f < r1`. Both pieces stay one-to-one. No source frame is duplicated or omitted. Every other stored shape is rejected, writes nothing, and adds no undo. This refuses rows the store can already hold. It does not define a mapping for them. `placeInstance` already uses this equality for new placement. `insertInstance` and `trimInstance` do not, and this command does not change them.

2. **Apply `sourceSplit = s0 + (f - r0)` to every instance.** On the stored 25/1 against 24000/1001 row, record offset 12 selects source index 12, and `pictureSourceOffset` selects 13. On the trimmed 24/1 row, record frame 10 selects source index 10, which is the source end, so the right piece is empty. Using the formula on those rows is a conversion the model does not define.

3. **Use `pictureSourceOffset` as the source cut.** That function returns one source frame for one sequence frame. It does not partition a duration. On the mixed-rate row it disagrees with the identity offset. On a same-rate unequal row the rates match, so the offset maps to itself and can land on or past `sourceEnd`. Adding it as a split rule would be a mixed-rate remap. This slice does not add one.

4. **Scale the offset by `sourceDuration / recordDuration`.** The integer remainder needs its own rule. That is a speed mapping. This slice does not add speed or remapping.

5. **Copy the whole source span onto both pieces, or leave the source endpoints where they were.** Both duplicate source frames, or they leave the source span attached to only one record piece. The continuity rule for this slice is no duplicate and no omitted source frame. This is not that rule.

## 4. Instance identity

The identity rule for a two-piece partition is already in the overwrite planner. The owner kept it. The piece that still starts at the original record start keeps the id. The piece that starts at the cut is minted with `makeStableId()`. Undo restores the original row. Redo restores both rows.

`planSameRateOverwrite`, when an instance starts before the replacement and ends after it:

- The left piece is `retreatTail`. It keeps `instanceId` and is written as an update.
- The right piece is `advanceHead`. Its `instanceId` is cleared. `ProjectSession::overwrite` mints the empty id with `makeStableId()`, which is UUID version 4 text.
- Undo calls `editToward(afterInstances, beforeInstances)`. An id that exists only in the after set is deleted. The original id is updated back to the single before row.
- Redo calls `editToward(beforeInstances, afterInstances)`. The original id is updated to the left span, and the right row is inserted with the same minted id.

That is the live convention for one instance becoming two pieces: the piece that still begins at the original record start keeps the id, and the piece that begins at the cut receives the new id. Undo restores the original instance. Redo restores the split, including the new id.

`docs/EDITORIAL_OPERATIONS.md` section 6 describes the same left-keeps-id rule for a razor, and its header says that document authorises no implementation. The code above is the convention. There is no second live rule. The source cut is closed in section 6. The identity rule is the one above.

## 5. What this decision does not change

The playhead label in `MainWindow::showRates` stays `formatFrameLabel(0, selected->rate)`. It is not the split position. The split frame is a separate integer field. Research notes, FFmpeg, `/opt/aether-ffmpeg-9.1`, RPATH, licensing, ShellCut, PlaceCut, MarksCut, ProbeCut, OverwriteCut, and earlier screenshots stay untouched. `docs/DEVELOPMENT_ROADMAP.md` stays empty.

## 6. Decision

Closed. A split runs only when the source duration equals the record duration and the rates match after reduction. The source cut is `sourceSplit = s0 + (f - r0)`. The two counter-examples in section 2 are refused. Alternatives 2 through 5 are not used.
