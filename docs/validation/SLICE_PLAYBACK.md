# Slice Playback

Picture playback for one local MXF. Extract, marks, split, lift, overwrite, the schema, and the visible-end rule are unchanged. ADR-008 stays open. Probe results are not written to `project.db`. A display picture is not an editorial time.

## The picture

The file used, read only, is:

`validation-fixtures/ui-integration/media/Normal_test.mxf`

Size `2308308255` bytes. mtime `1795273251`. `file(1)` reports a Material Exchange Format container. A same-sized copy sits under `validation-fixtures/ui-integration/live-parent/IntegCut/MEDIA/`. It was not opened. `Normal_Test.mxf` is a different, larger file and was not opened.

The approved libraries at `/opt/aether-ffmpeg-9.1` decode it. There is no `ffmpeg` binary in that prefix, and the system `ffmpeg` was not called. The video stream is ProRes, `1920x1080`, `yuv422p10le`. `avg_frame_rate` is absent. `r_frame_rate` is `25/1`. That field paces the source monitor only. The label says it is not an editorial time. The sequence rate stays the rate stored on the sequence.

One decoded picture and the next one both have non-zero samples, and they are not the same picture. The file size, mtime, and the media-folder entry count were the same after the decode. Nothing was transcoded and the MXF was not overwritten.

## What the running code does

The decoder is `src/media/ffmpeg_picture.cpp`. UI, editorial, and persistence headers do not include FFmpeg headers. The public picture type is RGB bytes plus a media picture index and an error string.

Opening a clip in the source monitor decodes picture 0 into that monitor. Source Play advances that index on the container's reported interval. Source Stop stops the timer and leaves the current picture.

Place on the source monitor calls the existing `beginPlacement` path. It does not add a placement command and it does not call overwrite. The record monitor then shows the decoded picture for the instance under the playhead. The picture index is the stored source offset plus the sequence frames into that instance. The topmost video lane wins when more than one covers the frame. An empty playhead stays empty and says why. A black monitor with that sentence is not a picture.

Record Play and timeline Play are the same transport. The red playhead moves one stored sequence frame at a time. The record label uses `formatFrameLabel` at the stored sequence rate. Stop stops the timer. Dragging the red line still does not scrub. Playback stops at the stored record end.

## Offscreen proof

Build directory: `build-slice-playback`. Executable: `build-slice-playback/src/aether`.

`QT_QPA_PLATFORM=offscreen` on 24 November 2026, 22:19 IST. Every existing group passed, including ExtractTests (12) and UiCompletionTests (5). PlaybackTests passed 5:

- `mxfPictureIsReadableAndUnchanged` decoded picture 0 at `1920x1080` with colour, decoded a different picture 1, and left the MXF unchanged.
- `sourceRecordAndTimelinePlaybackUseTheMxf` opened that MXF in the source monitor, played and stopped with the picture index no longer 0, placed source `[0,8)` at `25/1` through the place dialog, showed a record picture at the playhead, then played and stopped the timeline with the playhead moved and still inside that span.
- `playheadLabelFollowsTheStoredFrame` showed frame 3 of Reel at `24/1` as `00:00:00:03`, and Play with no timeline picture stayed stopped.

## Native window

Pid `14581` is still `build-slice1j-ui-completion/src/aether` on ExtractPortalCut. Its `project.db` hash is still `1ced9f2ecb5bfdc2225c2dacb1df92901dbb76c1ec5d965e0e098a959328fbc3`. That process was not killed. The playback process does not have that database open.

Pid `22702` is `build-slice-playback/src/aether`, launched at 22:18 IST with `QT_QPA_PLATFORM=xcb`, `QT_QPA_PLATFORMTHEME=none`, and `QT_SCALE_FACTOR=0.58`. The window is `0x1600007`, titled `Aether — PlaybackCut`.

`01-playback-window-at-launch.png` is the empty launch. The record monitor says `No timeline picture`. `02-project-dialog-at-launch.png` is the empty New Project page. Those two frames are not playback.

The project was created in that window. Name `PlaybackCut`, sequence rate `25/1`, folder `validation-fixtures/playback/PlaybackCut`. While the project was still open, `project.db` hashed to `2731236ee922c0b5be80a23d3fa6f928101e0cca4f659f4f30d30550947458ba`. The `MEDIA` directory is empty. The representation path is the original MXF and its status is `registered`. The instance is source `[0,8)` at `25/1`, record `[0,8)`, on track ordinal 0. The visible-end label reads `Visible end 8`.

The MXF was still `2308308255` bytes, mtime `1795273251`, after the native decode. `Normal_Test.mxf` was not imported.

## Native pictures and playback

Checked at 22:57 IST through one temporary RemoteDesktop session. Device mask `3`, `persist_mode` `0`, EIS only. The consent dialog was not clicked. The session was closed at 22:57:51 IST and `/tmp/aether-portal-input` was removed. XTEST was not used.

`03-source-picture.png` is the source monitor on picture 0. The picture is the orange frame with the UTKARSH mark, not a black frame. A sample pixel is `(89, 28, 1)`. The label says picture 0, the container `r_frame_rate`, and that this is not an editorial time. It says Stopped.

`04-source-playing.png` is a later source play. The label says `Picture 445` and `Playing`. Sample pixels are `(146, 156, 133)` and `(213, 218, 212)`.

`05-source-stopped.png` is after Stop. The label says `Picture 451` and `Stopped`. A second look, after the other checks, still said picture 451 and Stopped.

`06-placed-record-frame-0.png` is the timeline after Place. The record monitor shows the same orange frame as source picture 0, labelled `00:00:00:00` and `Playhead 00:00:00:00 PlaybackCut. Stopped.` The red playhead stands at screen x `160`.

`07-record-playhead-07.png` is the same monitor after the transport ran. The label is `00:00:00:07` and `Playhead 00:00:00:07 PlaybackCut. Stopped.` The picture is a different, pale frame. A sample pixel is `(223, 246, 245)`. The red playhead stands at screen x `176`. The 16 screen pixels are 7 sequence frames at 4 logical pixels per frame and the `0.58` scale. Play and Stop pressed again at that frame left the label on `00:00:00:07` and Stopped. The next sequence frame would be the stored record end, so the transport does not move past it.

The toolbar clock still shows `00:00:00:00`. That text is the frame-0 label written when the sequence is shown. The live playhead is the record label and the red line.

## What does not play

Audio does not play. The red playhead cannot be dragged. Effects, the audio room, and export stay unavailable. The source interval is the container `r_frame_rate`, not a verified source rate. The toolbar clock does not follow the playhead. ADR-008 stays open.

## Status

SLICE PLAYBACK NATIVE SOURCE, RECORD, AND TIMELINE PLAYBACK CHECKED
