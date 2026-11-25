# Media codec and container support

The integration rules and the approved library set are `docs/technology/FFMPEG_INTEGRATION_ARCHITECTURE.md`. The build baseline is `docs/technology/FFMPEG_DEVELOPMENT_CONFIGURATION.md`. A row below is a CLI measurement unless it says otherwise. The 30000/1001 audio contract remains **Failed** (`docs/validation/AUDIO_FRAME_SAMPLE_BOUNDARY_VALIDATION.md`). Library integration is **BLOCKED**. Nothing in this table is a legal approval or a vendor conform.

Status: evidence from one CLI session, plus a list of what was not run. No combination below is an AETHER capability. A muxer name is not a combination.

Date: 2026-11-20. Binary: `/usr/bin/ffmpeg` 9.0.2 (`ffmpeg-9.0.2-3.fc45.x86_64`), libavformat 63.1.102. Full measurements: `docs/validation/FFMPEG_MXF_ROUNDTRIP_TEST.md`. Profile targets, not re-certified here: `docs/NATIVE_MEDIA_ARCHITECTURE.md` and `docs/MXF_CONTAINER_SPECIFICATION.md`.

Labels: **Passed**, **Failed**, **Partially passed**, **Not tested**, **Not supported**. “Passed” means the numbered-frame checks named in the validation file for that one command. It does not mean a library link, a licence, or a vendor relink.

## 1. Names present in this CLI

`ffmpeg -muxers` listed, among others: `mov`, `mxf`, `mxf_d10`, `mxf_opatom`, `wav`. Listing `mxf` does not mean OP1a plus ProRes plus PCM works. Listing `mxf_opatom` does not mean stereo audio works. The stereo OP-Atom command **Failed** to write and is **Not supported** on this build.

`ffmpeg -encoders` listed `prores_ks`, `prores`, `prores_aw`, `prores_ks_vulkan`, `dnxhd`, `pcm_s24le`, `pcm_s24be`, `pcm_s16le`, `pcm_s32le`. This run used only `prores_ks` profile `hq` and `pcm_s24le`. The others are **Not tested** in this run.

Muxer defaults, from `ffmpeg -h muxer=`:

| Muxer | Default video | Default audio | Other default |
| --- | --- | --- | --- |
| `mxf` | `mpeg2video` | `pcm_s16le` | — |
| `mxf_opatom` | `dnxhd` | `pcm_s16le` | `-mxf_audio_edit_rate` default `25/1` |

Those defaults are not native profiles. Every command in this run named the codec.

## 2. Combinations this run actually measured

| Tuple | Label | What held | What did not |
| --- | --- | --- | --- |
| OP1a, ProRes 422 HQ, `yuv422p10le`, 640×360, 24/1, 24 frames, `pcm_s24le` stereo 48 kHz, timecode `01:00:00:00` | **Passed** for frame count, frame order, rate, timecode label, and 48000 audio samples in 24 packets of 2000 | Pattern UL `060e2b34.04010101.0d010201.01010900`. MediaInfo `OP-1a`, Closed/Complete | `channel_layout` null. MediaInfo audio wrapping `Frame (AES)`, codec id `0D01030102060300`, which is not the ST 382 wave element the container spec names as the target. `avg_frame_rate` `0/0`. `gop_timecode` was unused by the encoder; the timecode still appeared in container metadata. Not a library test. Not an NLE test |
| OP1a, same picture profile, 30000/1001, 8 frames, timecode `01:00:00;00`, supplied audio 12812 samples | **Partially passed** | 8 frames, order held, `r_frame_rate` `30000/1001`, drop-frame label kept, pattern UL is OP1a | Audio came back as 12813 samples. Packet durations `1602,1601,1602,1601,1602,1602,1601,1602`. Contract from `docs/TIMEBASE_AND_FRAME_ACCURACY.md` is 12812 samples in the order `1601,1602,1601,1602,1602,1601,1602,1601`. Sync contract **Failed** |
| OP1a, same picture, 30000/1001, 8 frames, supplied audio 12813 samples | **Partially passed** | Same picture result. Audio duration timestamp 12813, matching the supplied count | The per-frame split was not the contract split. Same packet pattern as the 12812 input. Supplying the muxer’s total does not make the edit-unit grid the editorial grid |
| OP-Atom picture only, ProRes 422 HQ, 24/1, 24 frames, timecode `01:00:00:00` | **Passed** for frame count, frame order, rate, timecode label, and pattern UL `060e2b34.04010102.0d010201.10030000` | MediaInfo `OP-Atom`, Closed/Complete, wrapping word `Frame`. No audio stream, so no sync claim | Not an Avid media database test. **Not tested** in Media Composer |
| OP-Atom mono `pcm_s24le`, 48000 samples, `-mxf_audio_edit_rate 24/1` | **Partially passed** | 48000 samples decoded. Pattern UL is OP-Atom. MediaInfo timecode rate 24 fps, first `00:00:00:00`, last `00:00:00:23`. Wrapping `Frame (BWF)`, codec id `0D01030102060100` | 25 packets of 1920 samples. 1920 is the 25/1 edit unit (`48000/25`). The essence grid does not match 24 picture frames of 2000. No timecode was passed on the command, so `00:00:00:00` is the muxer’s label, not a source label |
| OP-Atom stereo `pcm_s24le` | **Not supported** on this build | The process exited 234. The log line is `MXF OPAtom only supports single channel audio`. The output file is 0 bytes | A listed `mxf_opatom` muxer does not include this combination |

## 3. Not tested in this run

The 19 November 2026 study in `docs/NATIVE_MEDIA_ARCHITECTURE.md` reported a wider matrix (other ProRes profiles, DNxHR levels, other rasters, other rates, alpha, channel counts). This review did not repeat that matrix. Those rows stay **Not tested** for 2026-11-20. They are not promoted and they are not withdrawn. A reader must not add them to section 2.

Also **Not tested** here:

- `mxf_d10`, `mov`, `wav` as native masters
- `dnxhd` / DNxHR in either pattern
- `prores`, `prores_aw`, `prores_ks_vulkan`
- `pcm_s24be`, `pcm_s32le`, 96 kHz, 44.1 kHz (the November study reported rejections; this run did not re-issue those commands)
- 25/1, 24000/1001, 48/1, 60000/1001
- rasters 1920×1080, 1998×1080, 2048×858
- alpha
- multi-mono OP-Atom layouts (one file per channel)
- any file in Resolve, Media Composer, or Premiere Pro
- any call into `libavcodec` from AETHER code

## 4. Support rule

AETHER offers a writer only when the tuple has **Passed** inside AETHER’s own suite on a pinned build, and counsel has not forbidden the encoder. Section 2 is not that suite. The 24/1 OP1a picture-and-PCM count match is a CLI fact on synthetic 640×360 frames. The container spec’s wave-PCM target, the KAG size, the BER length, and vendor interchange stay where `docs/MXF_CONTAINER_SPECIFICATION.md` left them: specified, and not validated as an interchangeable master.

Camera originals stay as received. This table is about what a later writer may claim, not about what an editor may ingest read-only.
