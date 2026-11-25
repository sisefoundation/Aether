# Slice 1E — FFmpeg 9.1 development-only dynamic integration

Date: 2026-11-22. Machine: Fedora, kernel `7.2.15-300.fc45.x86_64`. Display `1920x1080`. This record follows the blocked system-library preflight in `docs/validation/SLICE_1E_MEDIA_METADATA_AND_SOURCE_TIMEBASE.md`. That preflight is not withdrawn. The system FFmpeg 9.0.2 GPL libraries under `/usr` were not linked.

The owner authorised a local development-only dynamic link against exactly `/opt/aether-ffmpeg-9.1`. This is not counsel approval, not a distribution configuration, and not a package. ADR-008 stays **OPEN — PENDING LEGAL COUNSEL**. ADR-001 conditions, ADR-002, ADR-003, ADR-005, ADR-006 **PROPOSED**, and ADR-007 were not changed. `docs/research/` was not edited. No AppImage, installer, or bundled copy of the libraries was made. Nothing was copied into `/usr`. `/usr/bin/ffmpeg`, `/etc/ld.so.conf`, and the system libraries were not changed. No package was installed. `sudo` was not used.

`kPrototypeVersion` stays `slice-0a`. Schema stays version 2. SQLite stays `SQLite3::SQLite3`. The language standard stays C++17, prototype only. There is no git repository. `git init` was not run.

Playback, overwrite, insert, split, lift, and ripple were not implemented. No later slice was started.

## Prefix

| Item | Observed |
| --- | --- |
| Root | `/opt/aether-ffmpeg-9.1` |
| Version string | `9.1-aether-lgpl-candidate` |
| Licence printed by the prefix | LGPL version 2.1 or later |
| libavutil | 61.19.100, SONAME `libavutil.so.61` |
| libavcodec | 63.24.100, SONAME `libavcodec.so.63` |
| libavformat | 63.16.100, SONAME `libavformat.so.63` |
| libswscale | 10.5.100, SONAME `libswscale.so.10` |
| libswresample | 7.6.100, SONAME `libswresample.so.7` |

The prefix has no `libavfilter`, no static `.a` of these libraries, and no GStreamer. Configure of that prefix includes `--prefix=/opt/aether-ffmpeg-9.1`, `--disable-gpl`, `--disable-version3`, `--disable-nonfree`, and `--disable-avfilter`. CMake refuses any other root. Isolated `pkg-config` (`PKG_CONFIG_PATH` cleared, `PKG_CONFIG_LIBDIR` only the prefix) and a `try_run` of `cmake/ffmpeg_prefix_check.cpp` both passed at configure time. Headers and libraries come from that same prefix. A configure or runtime resolution that fell back to `/usr` would fail the build.

The earlier isolated link smoke, outside the product, is recorded in the agent store as `internal/ffmpeg-9.1-prefix-link-smoke.md`. It passed before this integration.

## CMake and runtime resolution

Build directory: `/home/editry/development/sise-editor/build-slice1e-ffmpeg91`. It was not wiped. `build`, `build-slice0b`, `build-slice1a`, `build-slice1b`, `build-slice1d`, `build-slice1f`, `build-slice-ui`, and `build-slice-ui-integration` were not overwritten.

Configure:

```
cmake -S /home/editry/development/sise-editor -B /home/editry/development/sise-editor/build-slice1e-ffmpeg91 -DAETHER_FFMPEG_ROOT=/opt/aether-ffmpeg-9.1 -DCMAKE_BUILD_TYPE=Debug
```

The configure message was: development-only FFmpeg `/opt/aether-ffmpeg-9.1` (`9.1-aether-lgpl-candidate`, LGPL 2.1 or later). ADR-008 remains open. GStreamer is not searched. Qt 6.11.2. SQLite 3.53.4.

`aether_ui` compile flags do not include `/opt/aether-ffmpeg-9.1/include`. FFmpeg headers are private to `src/media/ffmpeg_probe.cpp`. The executable RPATH is `/opt/aether-ffmpeg-9.1/lib` (`RPATH`, not `RUNPATH`). `INSTALL_RPATH` is empty. The five libraries are linked as the prefix `.so` files. `libavfilter` is not a `NEEDED` entry.

`ldd` of `/home/editry/development/sise-editor/build-slice1e-ffmpeg91/src/aether`:

```
libavformat.so.63 => /opt/aether-ffmpeg-9.1/lib/libavformat.so.63
libavcodec.so.63 => /opt/aether-ffmpeg-9.1/lib/libavcodec.so.63
libswscale.so.10 => /opt/aether-ffmpeg-9.1/lib/libswscale.so.10
libswresample.so.7 => /opt/aether-ffmpeg-9.1/lib/libswresample.so.7
libavutil.so.61 => /opt/aether-ffmpeg-9.1/lib/libavutil.so.61
```

No `libav*` or `libsw*` line resolves under `/usr` or `/lib64`. CTest `aether_ffmpeg_resolution` checks the same rule and passed.

## Media Engine

Public types live in `include/aether/media/probe.hpp`. They do not include an FFmpeg header. The only product source that includes FFmpeg headers is `src/media/ffmpeg_probe.cpp`. `src/media/probe_format.cpp` turns a report into text.

A probe reads one registered local file. Paths with `://`, relative paths, non-regular files, and unreadable files are refused before FFmpeg opens them. The open uses `protocol_whitelist=file`. After open, the protocol name must be `file`. `avformat_network_init` is not called. The format context and the option dictionary are released on every path, including failure.

The report can carry container name, long name, duration as integer ticks of a timebase, file size, mtime, completion, and errors. Video can carry index, codec name, codec id, width, height, pixel format, sample aspect ratio, a display aspect ratio only when one unambiguous sample aspect ratio and the pixel size multiply without overflow, timebase, average frame rate, nominal/base rate, codec-header rate, start, duration, and frame count only when `nb_frames` is greater than zero. Audio uses `AVChannelLayout` for layout and channel count, plus sample rate, sample format, timebase, start, and duration. Subtitle, data, and attachment streams are reported. An attached picture is reported as an attachment.

Missing numbers stay absent. `r_frame_rate` is not presented as a proven frame rate. `avg_frame_rate` is not presented as proof of a constant frame rate. Frame count is not inferred from duration. Provenance labels are: reported by stream, reported by container, calculated from reported exact values, estimated, unknown, conflicting, requires validation. `sourceRateVerified` is always false. The placement dialog still says the source rate is not probed and still requires an explicit rate. The bin line `technicalMetadata` stays the literal `Not probed`. Probe results are a session cache. They are not a `project.db` table and they do not auto-fill the editorial source rate. ADR-002 remains the editorial clock.

The UI uses one `QThread` worker (`ProbeController`). There was no existing job service to reuse. No third-party thread library was added. The window shows Not probed, then Probing, and stays able to accept a mark while a probe is in flight. A result is shown only for the representation and project epoch that requested it. A selection change does not display another representation's result. Reprobe bumps a generation so an older result is dropped. Closing the project clears the cache. Imported media starts Not probed.

## Marks

Source and record marks stay session points. They are not edit ranges. They are not written to `project.db`. Probing a representation does not change mark values or which representation or sequence they belong to. `MediaProbeTests::probeLeavesMarksAndEditorialRowsUntouched` places a clip, sets source marks 3 and 9 and record marks 1 and 4, probes, and checks that `project.db` bytes are unchanged, that `sqlite_master` has the same eleven tables and no table whose name contains `mark` or `probe`, and that after close and reopen the timeline instance remains while the marks are empty.

The disposable ProbeCut database after the live checkpoint has schema version 2 and these tables only: `bin_membership`, `bins`, `editorial_objects`, `master_clips`, `media_assets`, `media_representations`, `project_meta`, `schema_info`, `sequences`, `timeline_instances`, `tracks`. One timeline instance remains.

## Tests

CTest in `build-slice1e-ffmpeg91`, `QT_QPA_PLATFORM=offscreen`: 2 discovered, 2 passed.

| CTest | Result |
| --- | --- |
| `slice0a` | Passed, 1.18 s on the last full run |
| `aether_ffmpeg_resolution` | Passed |

Qt Test totals inside `slice0a`, each including init and cleanup:

| Group | Passed |
| --- | --- |
| TimingTests | 7 |
| ProjectTests | 11 |
| SequenceTests | 10 |
| TimelineWorkspaceTests | 10 |
| MediaImportTests | 10 |
| UiIntegrationTests | 9 |
| TimelinePlacementTests | 6 |
| MarkTests | 6 |
| MediaProbeTests | 18 |

MediaProbeTests has 16 slots plus init and cleanup. The slots cover the 9.1 prefix with no system 9.0.2, public headers, video rationals and an unverified 24/1, audio channel layout, video plus audio, multiple streams including a subtitle, an attachment, missing duration/start/frame count, conflicting rates, malformed/truncated/missing/unreadable files, a refused remote URL and a refused relative path, file-handle release, identity across move and offline, marks and `project.db` unchanged, async stale-result rejection, and reprobe plus project switch.

The offscreen plugin prints `This plugin does not support propagateSizeHints()`. That warning predates this slice. It is not a failure.

## Live XCB checkpoints

Disposable project only: `/home/editry/development/sise-editor/validation-fixtures/media-probe-9.1/disposable-project`. ShellCut, PlaceCut, and MarksCut were not opened. The MarksCut process was not signalled. `wmctrl -l` showed `Aether — ProbeCut` and did not show MarksCut.

Screenshots were taken with `import -window` of the live X11 window after the widget text for that step had already been checked. `QScreen::grabWindow` produced a black image on this display and was not used for the saved files.

| Shot | File | What is on screen |
| --- | --- | --- |
| A | `validation-fixtures/media-probe-9.1/screenshots/A-not-probed.png` | `video.mkv` selected. Technical probe: Not probed. No container or duration is claimed. |
| B | `validation-fixtures/media-probe-9.1/screenshots/B-probing-responsive.png` | Status Probing. The report says the window stays responsive. Source Mark In is 4, set while the probe was still running. |
| C | `validation-fixtures/media-probe-9.1/screenshots/C-video-metadata.png` | Probed. Timebase `1/1000` reported by stream. Average and nominal candidates `24/1`, neither treated as a verified source rate. Candidates do not conflict. Stream duration unknown. File size 28941 bytes. |
| D | `validation-fixtures/media-probe-9.1/screenshots/D-audio-metadata.png` | `audio.wav`. Audio stream, codec `pcm_s16le`, sample rate 48000, channel count 1, timebase `1/48000`, start unknown. |
| E | `validation-fixtures/media-probe-9.1/screenshots/E-multiple-streams.png` | `multi.mkv`. An audio stream (channel count 1, timebase `1/1000`) and subtitle stream 3, codec `subrip`. |
| F | `validation-fixtures/media-probe-9.1/screenshots/F-unknown-or-conflicting-timing.png` | `vfr.mp4`. Nominal candidate `30/1001` labelled not a proven frame rate. Frame rate candidates conflict. None is a verified source rate. Start 0 and duration `14863360` ticks of timebase `1/15360`, reported by the stream, and not turned into an editorial frame count. |
| G | `validation-fixtures/media-probe-9.1/screenshots/G-probe-failure.png` | `malformed.mov`. Status Failed. Container unknown. File size 17 bytes. FFmpeg's invalid-data error is shown. No invented streams. |
| H | `validation-fixtures/media-probe-9.1/screenshots/H-marks-bin-timeline-after-reopen.png` | After close and open of the same project. Source Mark In 11. Record Mark In 2 sequence frames. The timeline instance is `record [0, 8) duration 8 frames`, source `[0, 8)` at `24/1`. The probe panel is Not probed again, because the probe cache is session data. The bin still lists `video.mkv`. |

`wmctrl -lG` after the run reported the window at `25,131` with size `1860x1521`, which is taller than the 1080-line display. `import -window` wrote `1860x1011` images. The eight states above are visible in those images. The process was left running.

Launch that produced the shots, and that is still running in tmux session `aether-slice1e`:

```
QT_QPA_PLATFORM=xcb /home/editry/development/sise-editor/build-slice1e-ffmpeg91/src/aether \
  --checkpoint-project /home/editry/development/sise-editor/validation-fixtures/media-probe-9.1/disposable-project \
  --checkpoint-shots /home/editry/development/sise-editor/validation-fixtures/media-probe-9.1/screenshots \
  --import /home/editry/development/sise-editor/validation-fixtures/media-probe-9.1/media/video.mkv \
  --import /home/editry/development/sise-editor/validation-fixtures/media-probe-9.1/media/audio.wav \
  --import /home/editry/development/sise-editor/validation-fixtures/media-probe-9.1/media/multi.mkv \
  --import /home/editry/development/sise-editor/validation-fixtures/media-probe-9.1/media/vfr.mp4 \
  --import /home/editry/development/sise-editor/validation-fixtures/media-probe-9.1/media/malformed.mov
```

## Limitations

- Distribution and ADR-008 remain open. This link is a local development RPATH. It is not an installable, relocatable, or shippable configuration.
- Probe text is not editorial timing. A candidate rate is not a verified source rate. Placement still asks for an explicit rate.
- No picture is decoded for display. No thumbnail, waveform, proxy, transcode, or playback.
- The probe cache does not survive close. Reopen shows Not probed until the editor probes again.
- The inspector shows a long report in a scrolling box. The live shots scroll that box to the lines named above. Lines above the viewport remain in the widget.
- The live window's requested height exceeds this display, so the capture is the visible portion.
- System FFmpeg 9.0.2 remains installed and is still GPL. The product binary does not resolve it.

## Final status

**SLICE 1E COMPLETE — FFMPEG 9.1 DEVELOPMENT-ONLY DYNAMIC INTEGRATION AND MEDIA METADATA PROBING VALIDATED — DISTRIBUTION LEGAL REVIEW PENDING.**
