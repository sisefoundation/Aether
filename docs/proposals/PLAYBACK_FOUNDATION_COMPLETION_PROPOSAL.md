# Playback foundation completion proposal

Status: proposal only. Implementation has not started. This file does not accept an ADR, does not change schema, and does not authorise a later slice.

Inspection date: 2026-11-24, after 23:45 IST. Tree: `/home/editry/development/sise-editor`. Read first: `docs/validation/SLICE_PLAYBACK.md`.

Every statement below carries one of these labels: **VERIFIED**, **OBSERVED**, **PROPOSED**, **OPEN**, or **UNSUPPORTED**.

**VERIFIED.** This document does not modify application source, CMake, schema, tests, ADRs, PlaybackCut, ExtractPortalCut, ExtractUiCut, `Normal_test.mxf`, or any validation screenshot. It does not implement audio, scrubbing, effects, or export. It does not start Slice 1K. It does not commit. It does not install packages. It does not mark ADR-006 accepted. It does not treat `25/1` as an approved source timebase.

## 1. Summary

**VERIFIED.** Picture playback for one local MXF already runs in `build-slice-playback/src/aether`. The source monitor shows a decoded picture. Place uses the existing placement path. Record Play and timeline Play share one timer and move the stored sequence playhead. Stop stops that timer.

**VERIFIED.** The source monitor uses a second timer. The three surfaces do not share one transport.

**VERIFIED.** The toolbar clock is written once as the label of sequence frame 0. The record label and the red playhead follow the session playhead.

**OPEN.** ADR-006 stays **PROPOSED**. Whether sound is `aether_sound` or a backend inside Processing stays **OPEN**. ADR-002 stays the editorial clock. A container field, including `r_frame_rate` `25/1` on `Normal_test.mxf`, is not an approved source timebase.

**PROPOSED.** Phases A through F in section 18 are recommendations. They are not accepted decisions.

## 2. Verified capability

**VERIFIED.** `docs/validation/SLICE_PLAYBACK.md` records an offscreen pass on 2026-11-24 at 22:19 IST. PlaybackTests passed 5, including a coloured picture 0, a different picture 1, source play and stop, placement of source `[0, 8)` at the editor-chosen rate `25/1`, a record picture, and a timeline play that moved the playhead and left it inside that span.

**OBSERVED.** The same note records the native window at 22:57 IST. Source picture 0 was a non-black orange frame. A later source play showed picture 445 with the word Playing, and Stop held picture 451. After placement the record monitor showed `00:00:00:00`. After the record transport ran it showed `00:00:00:07` and Stopped, on a different non-black frame. The red playhead moved from screen x 160 to screen x 176.

**VERIFIED.** Those native frames are launch shots `01` and `02`, then `03` through `07`, under `validation-fixtures/playback/screenshots/`. This proposal does not replace them.

**VERIFIED.** Audio does not play. Dragging the red line does not scrub. Effects, the audio room, and export are not on this path. ADR-008 stays open in the validation note.

## 3. Source files

**VERIFIED.** Ownership of the current path:

| Step | File | Owner |
| --- | --- | --- |
| Stored sequence, tracks, instances | `src/persistence/project_store.cpp`, `include/aether/core/sequence/sequence.hpp` | Editorial model and project storage |
| Half-open check and accepted rates | `src/core/sequence/sequence.cpp` | Editorial kernel |
| Place, list, undo | `src/app/project_session.cpp` | Application session |
| Playhead, coverage, toolbar clock, timeline Play | `src/ui/timeline_workspace.cpp` | UI |
| Source and record timers, labels, picture calls | `src/ui/main_window.cpp`, `include/aether/ui/main_window.hpp` | UI |
| Frame label | `include/aether/ui/frame_label.hpp` | UI display |
| Picture type with no decoder types | `include/aether/media/picture.hpp` | Media picture contract |
| Decode, scale, display interval | `src/media/ffmpeg_picture.cpp` | Media engine, the only picture decoder |
| Probe, including time base, not used by playback | `src/media/ffmpeg_probe.cpp`, `include/aether/media/probe.hpp` | Media engine |
| Nearest-frame query and sample floor | `src/core/timing/sample_boundary.cpp`, `include/aether/core/timing/sample_boundary.hpp` | Editorial timing helper |
| Link of the media library | `src/CMakeLists.txt` | Build. Not edited here |

**VERIFIED.** `pictureSourceOffset` is defined in `sample_boundary.cpp` and called from `tests/sequence_tests.cpp`. The playback path does not call it.

**VERIFIED.** `sampleBoundary` is called from `sampleCount` and from `tests/timing_tests.cpp`. The playback path does not call it.

## 4. Data flow

**VERIFIED.** Source open. The bin double-click reaches `MainWindow::loadSourcePicture`. That function asks the session for the original representation path, calls `PictureReader::open` and `pictureAt(0)`, and paints the RGB on the source view. The media index stays in the window. It is not written to `project.db`.

**VERIFIED.** Place. The source Place button calls `placeLoadedSource`, which calls `TimelineWorkspace::beginPlacement` and then `showRecordPicture`. Placement commits through `ProjectSession::placeInstance`, the same insert as `insertInstance`. It does not call overwrite.

**VERIFIED.** Record picture. `showRecordPicture` calls `TimelineWorkspace::pictureAtPlayhead`. Even-ordinal tracks are candidates. The highest ordinal whose half-open record span contains the playhead wins. The media index is `sourceStart + (playhead - recordStart)`. `PictureReader::pictureAt` decodes that index. Odd ordinals are skipped, so an audio lane does not supply the record picture.

**VERIFIED.** The application session does not open a decoder. The UI calls `PictureReader` directly. `aether_ui` links `aether_media` publicly, and `aether_media` links the approved FFmpeg libraries. Editorial sources do not include FFmpeg headers.

**PROPOSED.** ADR-006 would put the join in the application: editorial returns a slice, media returns a location, the application builds a picture request, and processing returns a presented frame. The running path does not do that. ADR-006 is not accepted, so this proposal does not treat the current UI call as a violation to be "fixed" by this file, and it does not treat the proposed join as already chosen.

## 5. Timing model

**VERIFIED.** Two timers exist. Both are `QTimer` objects parented to the main window, both `Qt::PreciseTimer`. `sourceTimer_` calls `advanceSource`. `recordTimer_` calls `advanceRecord`. `timelinePlayRequested` and the record Play button both call `playRecord`.

**VERIFIED.** Source play reads `reportedDisplayIntervalUs`, divides microseconds by 1000 with integer division, and starts `sourceTimer_` with at least 1 millisecond. Record play reads the stored sequence rate and starts `recordTimer_` at `(1000 * denominator) / numerator` milliseconds, with a floor of 1. For a stored `25/1` sequence that division is 40.

**VERIFIED.** `advanceSource` asks for `sourceIndex_ + 1` and, on a present picture, stores `picture.mediaIndex`. `advanceRecord` adds one to the session playhead when that next frame is still below `contentEndFrame()`. `contentEndFrame()` is the maximum stored `record_end` on the sequence.

**VERIFIED.** No playback function in `main_window.cpp` or `ffmpeg_picture.cpp` starts a `QThread`, a `QtConcurrent` task, or a `std::thread`. Decode, scale, seek, and the playhead update run on the thread that delivers the timer. That is the UI thread.

**UNSUPPORTED.** A claim that the process has no other threads. Qt may create threads of its own. Those threads are not the picture decoder.

## 6. Container versus editorial rate

**VERIFIED.** ADR-002, accepted 2026-11-21, is the editorial clock. Positions are `int64` frame indexes. Rates are reduced ratios. The named rates are `24/1`, `25/1`, `24000/1001`, `30000/1001`, and `60000/1001`. `isAcceptedFrameRate` accepts only those five, already reduced. Float seconds are not a stored clock. Timecode is a label. Internal ranges are half-open. A displayed inclusive out-point stays **DEFERRED**.

**VERIFIED.** `PictureReader::open` copies `avg_frame_rate` when both parts are positive, otherwise `r_frame_rate` when both parts are positive. `reportedDisplayIntervalUs` names that field in the note. The source label says the spacing is not an editorial time. The function does not write the field.

**OBSERVED.** `docs/validation/SLICE_PLAYBACK.md` says `Normal_test.mxf` has no `avg_frame_rate` and has `r_frame_rate` `25/1`. The native source label named `r_frame_rate`.

**VERIFIED.** `ffmpeg_picture.cpp` does not read `time_base` or a packet PTS. The picture index is a count of decoded pictures on the best video stream. `ffmpeg_probe.cpp` does read stream `time_base`, start, and duration. Probe results stay off `project.db`, as the validation note says.

**VERIFIED.** The PlaybackCut row stores source rate `25/1` and sequence rate `25/1` because those values were stored by placement and by sequence creation. `25/1` is one of the five accepted editorial rates. The container field `r_frame_rate` `25/1` is a separate fact. It is not the authority that approved the stored source rate. This proposal does not treat `25/1` as an approved source timebase.

**VERIFIED.** Record and timeline pacing use the stored sequence rate only. They do not read the container spacing.

Classification of the rate cases named by the inspection:

| Case | Class | Label |
| --- | --- | --- |
| The five ADR-002 rates as a stored sequence rate | Supported. The code accepts them and the record timer uses the stored pair | **VERIFIED** |
| The five ADR-002 rates as a stored source rate chosen in the place dialog | Supported as a stored editorial field. The dialog choice is not a measurement of the file | **VERIFIED** |
| `avg_frame_rate` as source-monitor spacing when both parts are positive | Supported only as a display interval. Not a stored rate | **VERIFIED** |
| `r_frame_rate` as source-monitor spacing when average rate is absent | Supported only as a display interval. This is the field the MXF note names | **VERIFIED** for the code, **OBSERVED** for this file |
| Either container field written as the instance source rate | Unsupported. The picture reader does not write the project | **VERIFIED** |
| Stream `time_base` as the editorial clock | Unsupported on the playback path. Probe can report it. Playback ignores it | **VERIFIED** |
| PTS as the picture index | Unsupported. The index is a decode count. A seek rewinds to timestamp 0 | **VERIFIED** |
| Identity index `sourceStart + (playhead - recordStart)` | The current record picture rule, including when the stored rates differ | **VERIFIED** |
| `pictureSourceOffset`, nearest timestamp, tie on the earlier frame | The accepted ADR-002 picture rule. Implemented and tested. Not called by playback | **VERIFIED** |
| Variable frame rate, or no positive average rate and no positive `r_frame_rate` | Source play refuses. Record play can still use the stored sequence rate | **VERIFIED** |
| `48/1` | Named as an ordinary value in `docs/05_TIMEBASE_AND_TIMING_ARCHITECTURE.md`. Rejected by `isAcceptedFrameRate`. Not an accepted stored rate | **UNSUPPORTED** as a stored rate |
| Drop-frame inferred because the rate is `30000/1001` | Unsupported. `formatFrameLabel` says a drop-frame flag is not stored, so it is not inferred. ADR-002 says the same | **VERIFIED** |
| Displayed inclusive out-point | Needs a decision. ADR-002 leaves it **DEFERRED** | **OPEN** |
| Using the container `25/1` as proof of the source timebase | Needs a decision before any such use. This file does not make that decision | **OPEN** |
| Speed-ramp resampling and mixed-rate conformance beyond the stored ranges | Named open in the timebase document and in system architecture item 19 | **OPEN** |

## 7. Clock discrepancy

**VERIFIED.** `TimelineWorkspace::showSelectedSequence` sets `timelineClock_` to `formatFrameLabel(0, sequence.rate)` after a successful sequence read. The ruler caption is the sentence "Sequence frame 0 label" plus that same label. Neither assignment reads `playheadFrame()`.

**VERIFIED.** `playheadChanged` is connected only to `showRecordPicture`. That function calls `updatePlayheadLabel`, which sets `recordTimecode_` and `playheadLabel_` from `formatFrameLabel(playhead, stored sequence rate)` and from Playing or Stopped. It does not set `timelineClock_`.

**VERIFIED.** The canvas paints the red line at `paintX(playheadFrame_)`. `setPlayheadFrame` updates that value and emits `playheadChanged`.

**OBSERVED.** The native record label moved to `00:00:00:07` while the toolbar clock stayed on `00:00:00:00`. That matches the two writers above.

**PROPOSED.** A later presentation change could set the toolbar clock from the same label the record monitor already shows. That would be a display change. It would not be a new editorial clock.

## 8. Monitor semantics

**VERIFIED.** The source monitor shows one media index and a sentence that names the container field used for spacing. Stopped and Playing are words on that sentence. The index is not a sequence frame.

**VERIFIED.** The record monitor shows the sequence playhead label, the sequence name, and Playing or Stopped. The picture is the decoded index from section 4, or a sentence when nothing covers the playhead, the file does not open, or the picture is absent. A black view with that sentence is not a picture, as the validation note says.

**VERIFIED.** Record Play calls `showViewerPage(1)` before the first record picture. Source play does not switch the page.

**VERIFIED.** `formatFrameLabel` for `25/1` prints `HH:MM:SS:FF` with the frame count modulo 25. Frame 0 is `00:00:00:00`. Frame 7 is `00:00:00:07`. Frame 3 at `24/1` is `00:00:00:03`, which the offscreen test already required.

## 9. Play, stop, and end

**VERIFIED.** Source Stop sets `sourcePlaying_` false and stops `sourceTimer_`. The current picture stays. A missing next picture also stops and shows the decoder note.

**VERIFIED.** Record Stop sets `recordPlaying_` false, stops `recordTimer_`, and refreshes the playhead label. If the status text is "Record playing", it becomes "Stopped".

**VERIFIED.** `advanceRecord` stops without moving when `playhead + 1 >= contentEndFrame()`. For a stored `record_end` of 8, frames 0 through 7 can be shown. Frame 8 is not shown. Play at a playhead already at or past that end sets the playhead to 0 first. Play at frame 7 starts the timer, and the next tick stops on frame 7 because the following frame is the end.

**OBSERVED.** The native window stopped on `00:00:00:07`. A further Play and Stop left that label. That matches the exclusive end.

**VERIFIED.** `contentEndFrame` considers every track, including odd ordinals. A later audio-only tail would extend the stop point. PlaybackCut has one instance, on ordinal 0, so the end used for that play is that row's `record_end`.

## 10. Audio gap

**VERIFIED.** `decodeNext` skips a packet whose stream is not the chosen video stream. It does not open an audio decoder, does not call `swr_convert`, and does not start a device. `libswresample` is included for the version assert only.

**VERIFIED.** There is no `aether_sound` target in `src/CMakeLists.txt`. The record picture path skips odd-ordinal tracks.

**OPEN.** `architecture/decisions/ADR-006-application-joiner.md` leaves the build shape open: sound as `aether_sound`, or sound as a backend inside Processing. `docs/ENGINEERING_INTERFACES.md` records the same open point. This proposal does not choose it.

**PROPOSED.** `docs/10_PLAYBACK_AND_RENDERING.md` describes an `AudioGraphRequest` from the application to AETHER SOUND. The running program does not build that request.

**OPEN.** Device-buffer bounds of 64 to 4096 samples are a recommendation in the audio documents. The default is unchosen. `docs/10_PLAYBACK_AND_RENDERING.md` says the Phase 11 sync tolerance is not invented here. This proposal invents neither a buffer size nor a sync tolerance.

## 11. Sample boundary

**VERIFIED.** The accepted floor is `sample_boundary(n) = floor(n * sample_rate * fps_denominator / fps_numerator)`, phase anchored at sequence frame 0. `sample_count` is the difference of two boundaries. A clip, an edit, a displayed timecode origin, and an export in-point do not reset the phase. There is no stored sample-time domain.

**VERIFIED.** The measured results stay as ADR-002 wrote them. At 30000/1001 and 48000 Hz, `[0, 8)` is 12812 samples and `[8, 16)` is 12813 samples. The 200000-frame walk is 320320000 samples. The MXF result of 12813 against 12812 for the origin span remains a failed encoder test. This proposal does not reclassify it.

**VERIFIED.** Playback does not call `sampleBoundary`. A future audio request, if one is authorised, would carry the sequence frame, the reduced sequence rate, and this span. That sentence is the accepted ADR-002 rule plus the ADR-006 owner record of the conceptual request. It is not an implementation in this tree's play path.

## 12. Clocks

**VERIFIED.** The clocks that exist today:

| Clock | Where it lives | Stored |
| --- | --- | --- |
| Sequence frame | `record_start`, `record_end`, session playhead | Ranges are stored. The playhead is session state |
| Source frame | `source_start`, `source_end` at the stored source rate | Stored |
| Source display index | `sourceIndex_` and `DisplayPicture::mediaIndex` | Not stored |
| Record picture index | Computed as `sourceStart + delta` | Not stored |
| Toolbar clock | Label of frame 0 | Not stored |
| Record label | `formatFrameLabel` of the playhead | Not stored |
| Container spacing | `avg_frame_rate` or `r_frame_rate` in the reader | Not stored |
| Sample index | `sampleBoundary` | Derived. Not stored |
| Device clock | Not opened by playback | Not the playhead |

**VERIFIED.** `docs/05_TIMEBASE_AND_TIMING_ARCHITECTURE.md` draws source frames, sequence frames, the session view, and a derived sample index as separate nodes. ADR-002 says the sequence rate must not destroy the source timing. The current record picture uses an identity offset and can ignore a difference between the two stored rates.

**PROPOSED.** When a later change maps a sequence frame to a source frame for presentation, the accepted rule is `pictureSourceOffset`: nearest valid timestamp, exact tie on the earlier frame. Identity remains exact when the two reduced rates are equal. This proposal does not switch the running path.

## 13. Scrubbing

**VERIFIED.** The canvas comment says the red playhead is the display playhead and dragging it is not scrubbing. A left press within two pixels of that line returns without calling `setPlayheadFrame`. The overview strip drag changes the horizontal scrollbar. Range mode reports a frame to the extract handler. Marks remain two optional points. The mark text in the UI says a displayed inclusive out is not settled, so the pair is not a half-open edit range.

**VERIFIED.** `docs/10_PLAYBACK_AND_RENDERING.md` says scrub uses the playback path at lower priority than play and does not start a transcode. The running canvas does not do that.

**PROPOSED.** Scrubbing, if later authorised, stays a presentation seek of the session playhead. It stays distinct from range selection and from marks. It does not write `record_start` or `record_end`.

## 14. Stale frames

**VERIFIED.** `PictureReader` keeps the last present picture. `pictureAt` returns it when the index matches. A lower index calls `rewind`, which seeks the format to timestamp 0, flushes the codec, and clears the cache. There is no seek to an arbitrary PTS.

**VERIFIED.** Because decode runs inside the timer slot on the UI thread, a slow picture delays the next timeout. The code has no queue of late frames and no drop counter.

**OPEN.** Late-frame policy is an owner question. This file does not close it. The alternatives, none of them chosen:

1. **PROPOSED** alternative. Hold the previous picture and let the sequence index wait until the decode returns. The displayed frame stays honest and the wall clock slips.
2. **PROPOSED** alternative. Advance the sequence index on the timer and present the newest finished picture, leaving a gap. `docs/10_PLAYBACK_AND_RENDERING.md` says a dropped preview updates the reported frame and does not rewrite the timeline.
3. **PROPOSED** alternative. Present the late picture and report both the requested sequence frame and the presented source frame. The architecture's `PresentedFrame` carries the sequence frame the request named.

**UNSUPPORTED.** A numeric slack, a frame of lateness, or a buffer depth. None is measured here, and none is invented.

## 15. FFmpeg boundary

**VERIFIED.** ADR-001 accepts FFmpeg as the mandatory core, dynamically linked, inside the media engine. The approved libraries are `libavutil`, `libavcodec`, `libavformat`, `libswscale`, and `libswresample`. `libavfilter` is not approved. GStreamer is not a mandatory dependency. The picture translation unit asserts `FFMPEG_VERSION` `9.1-aether-lgpl-candidate` and the matching library versions. The prefix used by the playback build is `/opt/aether-ffmpeg-9.1`.

**VERIFIED.** `include/aether/media/picture.hpp` has no decoder types. FFmpeg headers in the picture path are confined to `src/media/ffmpeg_picture.cpp` inside `extern "C"`. The public-header test skips that file and `ffmpeg_probe.cpp`.

**VERIFIED.** The picture reader refuses an empty path and a path containing `://`. It opens with `avformat_open_input` and does not write the file. RGB output is capped at 128 mebibytes. The scaler is `sws` to `AV_PIX_FMT_RGB24`.

**PROPOSED.** No new library, no bundling, and no `libavfilter`. Effects that need a filter graph wait for a separate decision, as ADR-001 already says.

## 16. Decisions

**VERIFIED.** Already accepted, and left accepted: ADR-001 with its conditions, ADR-002 as the editorial clock, ADR-003 one `project.db` and a manifest, ADR-005 one Media ID, ADR-009 session-only undo for the foundation prototype. Half-open stored ranges. `sample_boundary` from sequence frame 0. Picture selection by nearest timestamp with the earlier frame on a tie, as a rule, even though playback does not call it yet.

**OPEN.** ADR-006 remains **PROPOSED**. ADR-007 remains **PROPOSED**. Sound as `aether_sound` versus a Processing backend remains **OPEN**. The displayed inclusive out-point remains **DEFERRED**. Late-frame policy remains unchosen. Sync tolerance and the device-buffer default remain unchosen. ADR-008 remains open.

**UNSUPPORTED.** Marking ADR-006 accepted. Treating container `25/1` as an approved source timebase. A second editorial clock made of display indexes, PTS, or `time_base`.

## 17. Effects and export split

**VERIFIED.** The playback path does not build a filter graph, does not encode, and does not write an export file. `docs/10_PLAYBACK_AND_RENDERING.md` puts export on a non-realtime path and says a cancelled export leaves the project hash unchanged. ADR-007, still proposed, separates export from conform and from relink.

**PROPOSED.** Effects and export stay later proposals. They are not phases of picture transport. An effect that needs `libavfilter` needs its own ADR-001 reconsideration. An export is not a playback mode and is not a conform verdict.

## 18. Phases A through F

These phases are **PROPOSED** recommendations. They are not accepted decisions. They do not authorise edits.

**PROPOSED.** Phase A. Presentation only. Point the toolbar clock at the same playhead label the record monitor already shows. Leave the stored ranges, the sequence rate, and the frame-0 ruler sentence distinguishable from the live playhead.

**PROPOSED.** Phase B. Call structure. When ADR-006 is still proposed, keep any picture request as a recommendation: the UI asks the application, the application reads the editorial hit and the media location, and the media engine decodes. Do not add `aether_sound` in that phase.

**PROPOSED.** Phase C. Mapping. For a record picture, call `pictureSourceOffset` when a sequence frame must name a source frame, and keep the identity offset only where that function returns the same integer. Do not store the result. Do not change the placed spans.

**PROPOSED.** Phase D. Container evidence. Leave `avg_frame_rate` and `r_frame_rate` as named display fields. Do not copy either field into `source_rate_numerator`. A decision to trust a container rate is an owner decision and is outside this phase.

**PROPOSED.** Phase E. Audio request shape only. A request would carry the sequence frame, the reduced sequence rate, and `sample_boundary` from frame 0. No device, no buffer size, and no choice between `aether_sound` and a Processing backend.

**PROPOSED.** Phase F. Scrub, effects, and export stay out of A through E. Scrub, if ever authorised, is a playhead seek. Effects and export are separate proposals under sections 16 and 17.

## 19. Test matrix

**PROPOSED.** Tests to add only after a later authorisation. None of these were added or run for this proposal.

| Case | Expectation |
| --- | --- |
| Stored span `0` and `8` | Coverage includes 0 through 7 and excludes 8 |
| Record play from 0 at stored `25/1` | Playhead moves, stops on 7, label `00:00:00:07`, timer stopped |
| Play again at 7 | Playhead stays 7 |
| Toolbar clock after the move | Today: still the frame-0 label. After Phase A, if accepted: the live label |
| Source play | Index increases, Stop holds the index, the label names the container field and says it is not an editorial time |
| Container field absent | Source play refuses. The project row is unchanged |
| Rates differ | Today: identity index. After Phase C, if accepted: `pictureSourceOffset` |
| `sampleBoundary(0)` and `sampleBoundary(8)` at `30000/1001` and 48000 | 0 and 12812. Already tested. Playback still does not call it |
| MXF size and mtime | Unchanged by a picture read |
| Public headers | No FFmpeg include outside the two media translation units |

**VERIFIED.** The offscreen PlaybackTests already cover the MXF read, one source play, one placement, and one timeline play inside `[0, 8)`. This matrix does not replace that record.

## 20. Native validation

**VERIFIED.** On 2026-11-24 after 23:45 IST the workspace has no `.git` directory. `git status` reports that this directory is not a git repository.

**VERIFIED.** Running processes:

| Pid | Started | Command | Database |
| --- | --- | --- | --- |
| 14581 | 2026-11-24 21:18:38 | `build-slice1j-ui-completion/src/aether` | `validation-fixtures/extract-editing/ui-completion-parent/ExtractPortalCut/project.db` |
| 22702 | 2026-11-24 22:18:41 | `build-slice-playback/src/aether` | `validation-fixtures/playback/PlaybackCut/project.db`, and `Normal_test.mxf` twice |

**VERIFIED.** Pid 8865 is a tmux session named `aether-ui-completion`. It is not an `aether` process and holds no `project.db`.

**VERIFIED.** Hashes while both projects were still open, read only: PlaybackCut `2731236ee922c0b5be80a23d3fa6f928101e0cca4f659f4f30d30550947458ba`. ExtractPortalCut `1ced9f2ecb5bfdc2225c2dacb1df92901dbb76c1ec5d965e0e098a959328fbc3`. Both match the last known values. Neither project was closed or edited. ExtractUiCut was not opened.

**VERIFIED.** `validation-fixtures/ui-integration/media/Normal_test.mxf` size `2308308255`, mtime `1795273251`.

**VERIFIED.** The open PlaybackCut row, read with `mode=ro`, is `source_start` 0, `source_end` 8, `record_start` 0, `record_end` 8, source rate `25/1`, sequence name `PlaybackCut`, sequence rate `25/1`, representation role `original`, status `registered`, path the MXF above.

**VERIFIED.** Those integers are a half-open span before they are written as `[0, 8)`. `isHalfOpenFrameSpan` requires `start >= 0` and `end > start`. The store checks `source_end > source_start` and `record_end > record_start`. `pictureAtPlayhead` skips a row when `playhead < recordStart` or `playhead >= recordEnd`. Frame 8 is outside the span. Frames 0 through 7 are inside it. The visible-end label reading `Visible end 8` is that exclusive end, not an eighth presented frame.

## 21. Limitations

**VERIFIED.** Audio is silent. The red line does not scrub. The toolbar clock does not follow the playhead. The record picture uses an identity index. Mixed-rate nearest-frame selection is not on the play path. Decode blocks the UI thread. The UI calls the decoder. Container `r_frame_rate` paces only the source display timer. Probe time base and PTS are not the picture index. Effects and export are absent. ADR-006 is not the running join.

**OBSERVED.** Source play in the native pass was observed from picture 445, not from a Playing label on picture 0. Picture 0 was observed stopped, before that play. The validation note is the record of both.

## 22. Owner decisions

**OPEN.** These remain for the owner. This file does not decide them.

1. Whether the toolbar clock should show the live playhead label.
2. Which late-frame alternative in section 14, if any, is wanted.
3. Whether sound is a separate `aether_sound` target or a Processing backend.
4. Whether any container field may later be proposed as evidence of a source rate, and what measurement would be required. Container `25/1` is not that approval.
5. The displayed inclusive out-point, still deferred by ADR-002.
6. Sync tolerance and the device-buffer default. No number is proposed here.
7. Whether ADR-006's application join should become the picture path. The ADR stays **PROPOSED** until the owner accepts it.

## 23. Exclusions

**VERIFIED.** Not done by this proposal, and not authorised by it:

- No edit to application source, CMake, schema, tests, or ADRs.
- No edit to PlaybackCut, ExtractPortalCut, ExtractUiCut, `Normal_test.mxf`, or validation screenshots.
- No audio implementation, no scrubbing implementation, no effects, no export.
- No Slice 1K.
- No commit.
- No package install.
- No change of ADR-006 away from **PROPOSED**.
- No treatment of `25/1` as an approved source timebase.
- No new library, no `libavfilter`, no GStreamer, no bundling.
- No invented buffer size, sync tolerance, or late-frame rule.

**PROPOSED.** Phases A through F stay recommendations.
