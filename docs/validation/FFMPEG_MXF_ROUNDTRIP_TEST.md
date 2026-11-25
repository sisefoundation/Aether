# FFmpeg MXF numbered-frame round trip

Status: one CLI session on 2026-11-20. This is not a library integration test, not a licence finding, and not a vendor conform.

Labels: **Passed**, **Failed**, **Partially passed**, **Not tested**, **Not supported**.

Fixture directory, created for this session and containing only synthetic files: `/home/editry/development/sise-editor/validation-fixtures/ffmpeg-mxf-numbered-2026-11-20/`. No user media was overwritten. Logs: `logs/run.log` (inventory and the first command attempt) and `logs/run-retry.log` (the commands that produced the files below).

## 1. Tools

| Tool | Identity |
| --- | --- |
| `/usr/bin/ffmpeg` | FFmpeg 9.0.2, gcc 16. Package `ffmpeg-9.0.2-3.fc45.x86_64` |
| `/usr/bin/ffprobe` | same version |
| `/usr/bin/mediainfo` | MediaInfoLib v26.05 |
| Libraries reported by `ffmpeg -version` | libavutil 61.1.102, libavcodec 63.1.102, libavformat 63.1.102, libavdevice 63.1.102, libavfilter 12.1.102, libswscale 10.1.102, libswresample 7.1.102 |
| Host | Linux fedora 7.2.15-300.fc45.x86_64 |
| Python used to write the sources | Python 3.15.0 |

`ffmpeg-devel` is not installed. These commands do not prove that AETHER can link `libavcodec`.

The configuration print includes `--enable-gpl`, `--enable-shared`, `--disable-static`, `--enable-libfdk-aac`, `--enable-decklink`, `--enable-libx264`, `--enable-libx265`. It does not include the token `--enable-nonfree`. The full print is in `logs/run.log`.

Muxers listed include `mxf` and `mxf_opatom`. That listing is not a result. Defaults from `ffmpeg -h`: `mxf` video `mpeg2video`, audio `pcm_s16le`; `mxf_opatom` video `dnxhd`, audio `pcm_s16le`, `-mxf_audio_edit_rate` default `25/1`. Every measured command names its own codec.

## 2. Synthetic sources

640×360 RGB PPM frames. Frame `n` has a white bar of width `8*(n+1)` pixels in rows 0–23, and one white pixel at `(n, 40)`.

| Source | Contents |
| --- | --- |
| `src/frame24_00.ppm` … `frame24_23.ppm` | 24 frames |
| `src/frame2997_00.ppm` … `frame2997_07.ppm` | 8 frames |
| `src/audio_24_stereo_48000.s24le` | 48000 stereo `s24le` samples, 288000 bytes. `sampleIndex(24)` at 24/1 and 48000 is `24*2000` |
| `src/audio_24_mono_48000.s24le` | 48000 mono samples, 144000 bytes |
| `src/audio_2997_stereo_12812.s24le` | 12812 stereo samples, 76872 bytes. `floor(8 * 48000 * 1001 / 30000) = 12812` |
| `src/audio_2997_stereo_12813.s24le` | 12813 stereo samples, 76878 bytes |

Contract order for the eight 30000/1001 frames, from `docs/TIMEBASE_AND_FRAME_ACCURACY.md`: 1601, 1602, 1601, 1602, 1602, 1601, 1602, 1601. Sum 12812.

## 3. Commands that did not measure a codec

The first attempt placed `-frames:v` before the second input, and used `-channels`, which this binary rejected (`Option channels not found`, exit 8). Exit 234 on the OP1a attempts was an option-placement error, not a muxer rejection. Those commands are in `logs/run.log`. They are not results. The OP-Atom picture command in that first log did complete, and it is repeated in the measurements below.

## 4. Commands that were measured

Working directory does not matter. Paths are absolute.

### 4.1 OP1a, 24/1 — exit 0

```bash
ffmpeg -hide_banner -y \
  -framerate 24 -i /home/editry/development/sise-editor/validation-fixtures/ffmpeg-mxf-numbered-2026-11-20/src/frame24_%02d.ppm \
  -f s24le -ar 48000 -ac 2 -i /home/editry/development/sise-editor/validation-fixtures/ffmpeg-mxf-numbered-2026-11-20/src/audio_24_stereo_48000.s24le \
  -map 0:v:0 -map 1:a:0 \
  -frames:v 24 \
  -c:v prores_ks -profile:v hq -pix_fmt yuv422p10le \
  -c:a pcm_s24le \
  -timecode 01:00:00:00 \
  -f mxf \
  /home/editry/development/sise-editor/validation-fixtures/ffmpeg-mxf-numbered-2026-11-20/out/op1a_prores422hq_pcm_s24le_stereo_24fps_24f.mxf
```

### 4.2 OP1a, 30000/1001, 12812 supplied samples — exit 0

```bash
ffmpeg -hide_banner -y \
  -framerate 30000/1001 -i /home/editry/development/sise-editor/validation-fixtures/ffmpeg-mxf-numbered-2026-11-20/src/frame2997_%02d.ppm \
  -f s24le -ar 48000 -ac 2 -i /home/editry/development/sise-editor/validation-fixtures/ffmpeg-mxf-numbered-2026-11-20/src/audio_2997_stereo_12812.s24le \
  -map 0:v:0 -map 1:a:0 \
  -frames:v 8 \
  -c:v prores_ks -profile:v hq -pix_fmt yuv422p10le \
  -c:a pcm_s24le \
  -timecode 01:00:00\;00 \
  -f mxf \
  /home/editry/development/sise-editor/validation-fixtures/ffmpeg-mxf-numbered-2026-11-20/out/op1a_prores422hq_pcm_s24le_stereo_30000_1001_8f_samples12812.mxf
```

### 4.3 OP1a, 30000/1001, 12813 supplied samples — exit 0

Same as 4.2 with `audio_2997_stereo_12813.s24le` and output `op1a_prores422hq_pcm_s24le_stereo_30000_1001_8f_samples12813.mxf`.

### 4.4 OP-Atom picture — exit 0

```bash
ffmpeg -hide_banner -y \
  -framerate 24 -i /home/editry/development/sise-editor/validation-fixtures/ffmpeg-mxf-numbered-2026-11-20/src/frame24_%02d.ppm \
  -frames:v 24 \
  -c:v prores_ks -profile:v hq -pix_fmt yuv422p10le \
  -timecode 01:00:00:00 \
  -f mxf_opatom \
  /home/editry/development/sise-editor/validation-fixtures/ffmpeg-mxf-numbered-2026-11-20/out/opatom_prores422hq_picture_24fps_24f.mxf
```

The successful run of this command is the one in `logs/run.log`. `-frames:v` is an output option there because there is no second input.

### 4.5 OP-Atom mono audio — exit 0

```bash
ffmpeg -hide_banner -y \
  -f s24le -ar 48000 -ac 1 -i /home/editry/development/sise-editor/validation-fixtures/ffmpeg-mxf-numbered-2026-11-20/src/audio_24_mono_48000.s24le \
  -c:a pcm_s24le \
  -mxf_audio_edit_rate 24/1 \
  -f mxf_opatom \
  /home/editry/development/sise-editor/validation-fixtures/ffmpeg-mxf-numbered-2026-11-20/out/opatom_pcm_s24le_mono_48000_edit24.mxf
```

No `-timecode` was set.

### 4.6 OP-Atom stereo audio — exit 234

Same as 4.5 with `-ac 2`, the stereo source, and output `opatom_pcm_s24le_stereo_48000_edit24.mxf`.

Stderr: `MXF OPAtom only supports single channel audio` and `Nothing was written into output file`. File size 0.

## 5. How the outputs were read

- Pattern label: the 16 bytes `06 0e 2b 34 04 01 01 01 0d 01 02 01 01 01 09 00` (OP1a) or `06 0e 2b 34 04 01 01 02 0d 01 02 01 10 03 00 00` (OP-Atom), searched in the first 65536 bytes. Both hits were at offset 84.
- `ffprobe -count_frames -show_entries` JSON in `logs/*.ffprobe.json`.
- Picture decode: `ffmpeg -i FILE -map 0:v:0 -fps_mode passthrough -f rawvideo -pix_fmt rgb24`. Frame size `640*360*3 = 691200`. Bar width is the leading run of pixels with R, G, and B each above 200 in row 0. The marker is the same test at row 40.
- Audio decode: `ffmpeg -i FILE -map 0:a:0 -c:a pcm_s24le -f s24le`. Sample count is byte size divided by `3 * channels`.
- Packet durations: `ffprobe -show_packets -show_entries packet=duration`.

`avg_frame_rate` was `0/0` on every video stream. The rate used below is `r_frame_rate` and `time_base`. `nb_frames` was null. `nb_read_frames` is from `-count_frames`. `channel_layout` was null on every audio stream. `nb_samples` was null; the sample count is `duration_ts` at time base `1/48000`, confirmed by the decoded byte size.

On every successful picture encode the log also said `Codec AVOption gop_timecode ... has not been used for any stream`. The container timecode below is from ffprobe format tags and from MediaInfo, not from that unused codec option.

## 6. Results

### 6.1 OP1a 24/1 — **Passed** for the checks in this section

File size 389177. Pattern UL at offset 84 is OP1a. ffprobe `company_name` FFmpeg, `product_name` OP1a Muxer, timecode `01:00:00:00`.

| Stream | Measured |
| --- | --- |
| Picture | prores, profile HQ, `yuv422p10le`, 640×360, `r_frame_rate` `24/1`, `time_base` `1/24`, `duration_ts` 24, `nb_read_frames` 24, 10-bit. Format duration string `1.000000` |
| Audio | `pcm_s24le`, 48000 Hz, 2 channels, `channel_layout` null, time base `1/48000`, `duration_ts` 48000, `nb_read_frames` 24, 24-bit stored in `s32` |
| Packets | 24 picture packets of duration 1. 24 audio packets of duration 2000. Sum 48000 |
| Decoded picture | 24 frames, remainder 0. Bar widths `8,16,24,…,192`. Marker column equals the frame index 0 through 23 |
| Decoded audio | 288000 bytes, 48000 samples per channel |

MediaInfo: OP-1a, Closed/Complete, ProRes 422 HQ, 24.000 FPS, 10 bits, progressive, timecode first `01:00:00:00` and last `01:00:00:23` on material and source packages, `Time code, stripped: Yes`. Audio: PCM, Little, `Frame (AES)`, codec id `0D01030102060300`, 48.0 kHz, 2 channels, 24 bits.

Sync against `sampleIndex`: 24 frames × 2000 = 48000. **Passed.**

Not claimed: wave-essence mapping, channel names, library integration, Resolve, Media Composer, Premiere.

### 6.2 OP1a 30000/1001, both supplied lengths — picture **Passed**, audio contract **Failed**, tuple **Partially passed**

Both files are 114233 bytes. They are not the same file: ffprobe shows different `material_package_umid` values. Both pattern ULs are OP1a.

Picture, both files: 8 decoded frames, bars `8,16,24,32,40,48,56,64`, markers 0 through 7, `r_frame_rate` `30000/1001`, `time_base` `1001/30000`, `duration_ts` 8, profile HQ, 640×360. The ffmpeg progress line prints `29.97 fps`. The rational from ffprobe is `30000/1001`.

Timecode tag `01:00:00;00`. MediaInfo first `01:00:00;00`, last `01:00:00;07`, rate `29.970 (30000/1001) FPS`, stripped Yes.

Audio, both files, including the file whose source was 12812 samples:

| Quantity | 12812 source | 12813 source |
| --- | --- | --- |
| `duration_ts` | 12813 | 12813 |
| Decoded samples per channel | 12813 (76878 bytes) | 12813 (76878 bytes) |
| Packet durations in order | 1602, 1601, 1602, 1601, 1602, 1602, 1601, 1602 | not separately listed; the histogram matched the same 5×1602 and 3×1601 |

Float strings on the 12813 file, all from ffprobe, none of them authoritative: format `0.266938`, picture `0.266933`, audio `0.266937`. MediaInfo duration `267 ms` and `1601.6 SPF`.

`sampleIndex(8) = 12812`. The muxer stored 12813 and a different per-frame split from the contract order. The 12812 source was not preserved. **Failed** against the editorial audio contract. Picture identity **Passed**. The tuple is **Partially passed**.

### 6.3 OP-Atom picture — **Passed** for picture checks, sync **Not tested**

File size 74297. Pattern UL at offset 84 is OP-Atom. Timecode `01:00:00:00`. `product_name` OPAtom Muxer. Picture: HQ, 640×360, `24/1`, `duration_ts` 24, 24 packets of duration 1. Decoded bars and markers match section 6.1. No audio stream. MediaInfo OP-Atom, Closed/Complete, wrapping `Frame`, timecode last `01:00:00:23`, stripped Yes.

### 6.4 OP-Atom mono — **Partially passed**

File size 151097. Pattern UL is OP-Atom. `product_name` OPAtom Muxer. Timecode tag `00:00:00:00` because the command did not set one. Audio: `pcm_s24le`, 48000 Hz, 1 channel, `channel_layout` null, `duration_ts` 48000. Decoded 144000 bytes, 48000 samples.

Packets: 25 packets, each duration 1920. `25*1920 = 48000`. 1920 samples is one 25/1 edit unit at 48000 (`48000/25`), not one 24/1 edit unit (2000). MediaInfo still reports timecode frame rate 24.000 FPS, first `00:00:00:00`, last `00:00:00:23`, wrapping `Frame (BWF)`, codec id `0D01030102060100`.

Total sample count **Passed**. Edit-unit grid versus a 24-frame picture **Failed**. There is no picture in this file, so interleaved sync was **Not tested**. The tuple is **Partially passed**.

### 6.5 OP-Atom stereo — **Not supported**

Exit 234. Output size 0. This build’s `mxf_opatom` muxer refused stereo PCM. The muxer’s presence in `-muxers` is not a counterexample.

## 7. What a pass here is not

- Not a link of `libavformat` into AETHER. **Not tested.**
- Not acceptance of `docs/NATIVE_MEDIA_ARCHITECTURE.md`.
- Not a DNxHR result. **Not tested** in this session.
- Not a statement that OP1a AES audio meets the wave target in `docs/MXF_CONTAINER_SPECIFICATION.md`.
- Not a relink, and not a verified conform.
