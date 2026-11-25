# Audio frame and sample boundary validation

Status: measured on 2026-11-20 with the installed FFmpeg CLI and with integer arithmetic. This is not a library integration test, not a licence finding, and not a vendor conform.

The 30000/1001 failure is not reclassified. The editorial expectation for eight frames from frame 0 stays **12812** samples at 48 kHz. It was not changed to 12813. No sample was padded or trimmed to force a pass.

Labels: **Passed**, **Failed**, **Partially passed**, **Not tested**, **Not supported**.

Evidence directory, synthetic files only: `/home/editry/development/sise-editor/validation-fixtures/audio-sample-boundary-2026-11-20/`. The earlier MXF round trip was not edited: `/home/editry/development/sise-editor/docs/validation/FFMPEG_MXF_ROUNDTRIP_TEST.md` and `/home/editry/development/sise-editor/validation-fixtures/ffmpeg-mxf-numbered-2026-11-20/`. Summary of this session: `validation-fixtures/audio-sample-boundary-2026-11-20/logs/summary.txt`. Runner: `validation-fixtures/audio-sample-boundary-2026-11-20/run_boundary_tests.py`.

## 1. Original failure

Kept, and reproduced.

From `docs/validation/FFMPEG_MXF_ROUNDTRIP_TEST.md`, not rewritten by this session:

- FFmpeg 9.0.2
- MXF OP1a, ProRes 422 HQ, 24-bit stereo PCM, 24 numbered frames, timecode `01:00:00:00`, 48 kHz
- 24 fps **Passed**
- At 30000/1001, eight frames produced **12813** samples against the editorial expectation **12812**. **Failed**

This session reproduced that audio result with a new synthetic file, `out/prores_30000_1001_8f_supplied12812.mxf`. Picture `duration_ts` was 8. Audio `duration_ts` was 12813. Packet durations were `1602, 1601, 1602, 1601, 1602, 1602, 1601, 1602`. The source PCM contained 12812 samples. The muxer did not keep them.

## 2. The arithmetic

At 48 kHz and 30000/1001:

```
8 * 48000 * 1001 / 30000 = 12812.8
```

That value is not an integer. The established rule does not round 12812.8 up to 12813.

`docs/TIMEBASE_AND_FRAME_ACCURACY.md` and `docs/AUDIO_TIMEBASE_AND_SYNC.md` already define, for a frame index `F >= 0`:

```
sampleIndex(F) = floor(F * R * D / N)
```

with exact integer division. At this rate that is `floor(F * 8008 / 5)`.

The published samples in the first eight frames are `1601, 1602, 1601, 1602, 1602, 1601, 1602, 1601`. The sum is 12812. `sampleIndex(8) = 12812`.

## 3. Existing rule and the candidate

The candidate that was evaluated, and not auto-approved in advance, is:

```
sample_boundary(n) = floor(n * sample_rate * fps_denominator / fps_numerator)
segment count = sample_boundary(end) - sample_boundary(start)
```

For `n >= 0` this is the same function as `sampleIndex`. A half-open span `[start, end)` already had to contain `sampleIndex(end) - sampleIndex(start)` samples, because the per-frame counts telescope. The candidate does not replace the rule. It states the span form that the per-frame rule already implies.

Checked by integer arithmetic in the runner, not by floating point:

| Check | Result |
| --- | --- |
| `sample_boundary(8)` at 30000/1001, 48000 | 12812. **Passed** against the existing contract |
| First eight frame sizes | `1601, 1602, 1601, 1602, 1602, 1601, 1602, 1601`. **Passed** |
| Walk of per-frame counts from 0 through 200000, five rates | Equal to `sample_boundary(200000)` on every rate. **Passed**. No accumulated drift |
| Adjacent eight-frame segments tile `[0, 64)` | Sum of segment counts equals `sample_boundary(64)` on every rate. **Passed** |
| Several edit boundaries `[0, 8) + [8, 24) + [24, 25) + [25, 40)` | Sum equals `sample_boundary(40)` on every rate. **Passed** |

At 30000/1001 an eight-frame span is not always 12812 samples. That is the same formula, not a new expectation for the origin span:

| Span | Samples | Absolute start |
| --- | --- | --- |
| `[0, 8)` | 12812 | 0 |
| `[8, 16)` | 12813 | 12812 |
| `[16, 24)` | 12813 | 25625 |
| `[24, 32)` | 12813 | 38438 |
| `[32, 40)` | 12813 | 51251 |
| `[40, 48)` | 12812 | 64064 |
| `[100, 108)` | 12812 | 160160 |
| `[1000, 1008)` | 12812 | 1601600 |

Forcing every eight-frame span to 12812 would miss `sample_boundary(16)` by one sample and `sample_boundary(40)` by four. That would accumulate. It was not adopted. The origin span stays 12812.

Rates where `R * D` is divisible by `N` do not alternate. Eight frames is 16000 at 24/1, 15360 at 25/1, 16016 at 24000/1001, and 12800 at 30/1, from any non-negative start that was checked (`0`, `8`, `100`, `1000`).

### Rounding alternatives

Evaluated at 30000/1001 and 48000. None of these was adopted.

| Alternative | Value at frame 8 | Drift | Verdict |
| --- | --- | --- | --- |
| Floor, the existing rule | 12812 | Error against the exact rational stays in `[0, 1)` and does not grow. Walk to frame 200000 matched | **Kept** |
| Ceil of the same product | 12813 | Bounded, but it changes the recorded expectation | **Rejected.** Would hide the failure |
| Nearest, half up | 12813 | Bounded. Frame 5 and frame 10 are exact (8008 and 16016). Frame 8 is not the contract | **Rejected.** Would hide the failure |
| Constant 1602 samples every frame | 12816 over eight frames | Over 1000 frames, 1602000 against floor 1601600, drift `+400` | **Rejected.** Unbounded drift |

### Negative positions

**Not supported.** The timebase document rejects a negative stored position. `sampleIndex` is specified for `F >= 0`. An illustration, not a contract result: the product for frame `-1` is `-48048000`. Python floor division yields `-1602`. Toward-zero truncation yields `-1601`. Those are not interchangeable. The formula was not extended.

### Sequence origin

`sampleIndex` takes the stored frame index. It does not subtract a sequence start frame. A non-zero start was tested as a non-zero index (`100`, `1000`), not as a second sample origin. At 30000/1001, `[100, 108)` is 12812 because frame 100 has the same phase as frame 0. `[8, 16)` is 12813. One function still names the sample at which frame `F` starts, which is what `docs/AUDIO_TIMEBASE_AND_SYNC.md` requires.

No established rule had to be broken to keep 12812 for `[0, 8)`. Owner review is not required to keep that number. A later owner note could still say that a sequence start frame resets sample phase. That reading is not what the formula says, and it was not adopted.

## 4. Method

Host: Linux fedora 7.2.15-300.fc45.x86_64, Fedora Linux 45. `/usr/bin/ffmpeg` and `/usr/bin/ffprobe` are FFmpeg 9.0.2. Python 3.15.0 ran the integer checks and wrote silent stereo `s24le` PCM. No package was installed. No production media was overwritten.

Picture duration was read from ffprobe `duration_ts` on the video stream. `nb_frames` was absent. Audio duration was `duration_ts` with time base `1/48000`, which matched the sum of audio packet durations.

### Commands

The runner wrote one log per command under `logs/`. The origin failure was:

```bash
ffmpeg -hide_banner -y \
  -f lavfi -i color=c=black:s=32x32:r=30000/1001,format=yuv422p \
  -f s24le -ar 48000 -ac 2 \
  -i /home/editry/development/sise-editor/validation-fixtures/audio-sample-boundary-2026-11-20/src/prores_30000_1001_8f_supplied12812.s24le \
  -map 0:v:0 -map 1:a:0 -frames:v 8 \
  -c:v prores_ks -profile:v hq -pix_fmt yuv422p10le \
  -c:a pcm_s24le -f mxf \
  /home/editry/development/sise-editor/validation-fixtures/audio-sample-boundary-2026-11-20/out/prores_30000_1001_8f_supplied12812.mxf
```

Exit 0. The other MXF files used the same shape. Integer rates passed `-r` as `24`, `25`, or `30`. The 23.976 files used `24000/1001`. MPEG-2 files used `-c:v mpeg2video -b:v 80k -pix_fmt yuv422p`. The 40-frame and 600-frame files used that MPEG-2 command at `30000/1001` with PCM of length `sample_boundary(frames)`. Probe:

```bash
ffprobe -hide_banner -v error -show_format -show_streams -of json <file>
ffprobe -hide_banner -v error -select_streams a \
  -show_entries packet=pts,duration,size,flags -of json <file>
```

Audio-only control:

```bash
ffmpeg -hide_banner -y \
  -f s24le -ar 48000 -ac 2 \
  -i /home/editry/development/sise-editor/validation-fixtures/audio-sample-boundary-2026-11-20/src/audio_only_12812.s24le \
  -c:a pcm_s24le \
  /home/editry/development/sise-editor/validation-fixtures/audio-sample-boundary-2026-11-20/out/audio_only_12812.wav
```

Video-only control: eight frames of 32×32 MPEG-2 MXF at `30000/1001` with `-an`.

## 5. Results

| Test | Result |
| --- | --- |
| Contract arithmetic, `sample_boundary(8) = 12812` at 30000/1001 | **Passed** |
| Contract pattern of the first eight frames | **Passed** |
| Contract continuity, five rates, 200000 frames, adjacent segments, multiple edit boundaries | **Passed** |
| 24/1, 8 frames, MPEG-2 MXF, 48 kHz stereo PCM. Video `duration_ts` 8. Audio packets eight times 2000. Audio `duration_ts` 16000 | **Passed** |
| 25/1, 8 frames. Audio `duration_ts` 15360, packets of 1920 | **Passed** |
| 24000/1001, 8 frames. Audio `duration_ts` 16016, packets of 2002 | **Passed** |
| 30/1, 8 frames. Audio `duration_ts` 12800, packets of 1600 | **Passed** |
| 24/1, 40 frames. Video `duration_ts` 40. Audio `duration_ts` 80000 | **Passed** |
| 24000/1001, 40 frames. Video `duration_ts` 40. Audio `duration_ts` 80080 | **Passed** |
| 30000/1001, 8 frames, ProRes 422 HQ, 12812 samples supplied | **Failed.** Video `duration_ts` 8. Audio `duration_ts` 12813. Packets `1602, 1601, 1602, 1601, 1602, 1602, 1601, 1602` |
| Same picture rate and length, MPEG-2, 12812 samples supplied | **Failed.** Same audio count and the same packet pattern |
| Same ProRes command with 12813 samples supplied | Audio `duration_ts` 12813. That matches the supplied file and still does not match `sample_boundary(8) = 12812`. **Failed** against the contract |
| 30000/1001, 40 frames, 64064 samples supplied | **Partially passed.** Video `duration_ts` 40. Audio total `duration_ts` 64064, which equals `sample_boundary(40)`. The packet grid does not. Segment `[0, 8)` was 12813 against 12812. Absolute positions differ by one sample until a later boundary. Maximum absolute error against the floor grid across the 40 packets was 1 |
| 30000/1001, 600 frames, 960960 samples supplied | **Partially passed** on the same terms. Video `duration_ts` 600. Audio total 960960, equal to `sample_boundary(600)`. Maximum absolute error against the floor grid was 1. The error did not grow past one sample on this file. The origin eight-frame pattern was still the failing pattern |
| A/V duration on the integer-rate files | **Passed.** Video `duration_ts` matched the requested frame count and audio matched `sample_boundary` |
| A/V duration on 30000/1001 eight-frame files | **Partially passed.** Picture duration matched. Audio did not |
| Audio-only WAV of 12812 stereo samples | **Passed.** `duration_ts` 12812. No video stream. A picture grid was not applied |
| Video-only eight-frame MXF | **Passed** for “no audio stream”. Video `duration_ts` 8. This does not validate a 29.97 audio contract, because there is no audio |
| Negative frame index | **Not supported** |
| External NLE placement of these files | **Not tested** |
| Library decode of these files | **Not tested.** No development headers. See `docs/validation/FFMPEG_LIBRARY_INTEGRATION_VALIDATION.md` |

FFmpeg’s 30000/1001 phase is a one-sample shift of the editorial pattern, not an unbounded accumulator on the files that were long enough to measure (40 and 600 frames). Totals match the floor function when the frame count makes the rational exact (40 and 600 are multiples of 5). The origin span of eight frames is still 12813 in the file and 12812 in the contract. A bounded phase error is not a pass.

## 6. Recommended policy

Keep `sampleIndex`. State the span as `sampleIndex(end) - sampleIndex(start)` for `0 <= start <= end`. Keep 12812 for `[0, 8)` at 30000/1001 and 48 kHz, and keep the published per-frame pattern.

Do not require every eight-frame span on the timeline to be 12812. Do not pad or trim one sample so a file matches. A file whose audio sample count for a picture span disagrees with `sampleIndex(end) - sampleIndex(start)` fails verification. The installed FFmpeg 9.0.2 MXF muxer fails that check for an eight-frame 30000/1001 origin. Writers that depend on this muxer’s edit-unit grid stay hidden for that tuple.

This recommendation does not change the contract. It refuses the alternatives that would.

## 7. Remaining questions

- The owner is not asked to change 12812. No conflict with another established editorial rule forced that change.
- Whether a sequence start frame should reset sample phase is not what the written function does. It was not changed. A later note can say otherwise. Until then, implementations use the stored frame index.
- Negative positions stay unsupported until a definition chooses floor or toward-zero and the timebase document allows a negative index. It does not.
- Hardware or another FFmpeg build may split the same span differently. This session measured one Fedora CLI. It does not certify a pin.
- 60000/1001 was not muxed in this session. The same floor function covers it. That rate is **Not tested** on the CLI here.
