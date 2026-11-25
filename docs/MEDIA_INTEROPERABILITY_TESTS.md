# AETHER Media Interoperability Tests

Status: proposal, awaiting review. No implementation is authorized by this document.

These tests decide whether a native master can move between AETHER and the applications that cut feature films. A file that opens in FFmpeg is not, by that fact, a file Resolve, Media Composer, or Premiere can relink.

This document separates two kinds of result.

- **Executed.** The step was run on this machine on 19 November 2026. The tools were FFmpeg 9.0.2 at `/usr/bin/ffmpeg` and `/usr/bin/ffprobe` (libavformat 63.1.102, built with `--enable-gpl`) and MediaInfoLib 26.05. The pictures were `testsrc2`. The sound was `anullsrc`. The files were eight frames unless a row says otherwise. They are not camera negative, not a scan, and not a picture-quality test.
- **Specified, not executed.** The step is required before the profile is treated as interchangeable. It was not run. DaVinci Resolve, Avid Media Composer, and Adobe Premiere Pro were not launched. No result is claimed for them.

The profiles and the identity rules under test are in [NATIVE_MEDIA_ARCHITECTURE.md](NATIVE_MEDIA_ARCHITECTURE.md), [MXF_CONTAINER_SPECIFICATION.md](MXF_CONTAINER_SPECIFICATION.md), [PRORES_PROFILE_SPECIFICATION.md](PRORES_PROFILE_SPECIFICATION.md), [DNXHR_PROFILE_SPECIFICATION.md](DNXHR_PROFILE_SPECIFICATION.md), and [MEDIA_IDENTITY_AND_METADATA.md](MEDIA_IDENTITY_AND_METADATA.md).

## 1. Local results already executed

### 1.1 Picture and container

| Id | Status | Procedure and result |
| --- | --- | --- |
| L-OP1A | Executed | The `mxf` muxer wrote files whose partition pack contains the OP1a label `060e2b34.04010101.0d010201.01010900`. MediaInfo reported `OP-1a` and `Closed / Complete`. ffprobe reported the same label as `operational_pattern_ul`. |
| L-PRORES | Executed | `prores_ks` into OP1a with stereo `pcm_s24le` at 48 kHz and 1920×1080 at 24/1 produced Proxy `apco`, LT `apcs`, 422 `apcn`, HQ `apch`, 4444 `ap4h`, and 4444 XQ `ap4x`. HQ, 4444, and 4444-with-alpha were decoded by FFmpeg with an empty error log. |
| L-DNX | Executed | `dnxhd` into OP1a produced LB CID 1274, SQ 1273, HQ 1272, HQX 1271, and 444 CID 1270 for both `yuv444p10le` and `gbrp10le`. HQX was decoded by FFmpeg with an empty error log. MediaInfo reported `VC-3` and `RI@HQX` or `RI@444` on the files it was asked to describe. |
| L-RASTER | Executed | ProRes 422 HQ and DNxHR HQX both muxed at 1998×1080 and at 2048×858. ffprobe reported the requested size. The DNxHR descriptor also contains the padded stored size, 2000×1088 and height 864. |
| L-ALPHA | Executed | ProRes 4444 from `yuva444p10le` decoded in ffprobe as `yuva444p12le`. MediaInfo 26.05 did not report an alpha channel. The descriptor write in FFmpeg 9.0.2 has no alpha-sample-depth tag. |
| L-DEPTH | Executed | MediaInfo reported 10 bits for ProRes 4444. ffprobe reported `bits_per_raw_sample` 12 and pixel format `yuv444p12le`. The encoder input was `yuv444p10le`. |
| L-KLV | Executed | ProRes HQ picture KLV length was 4 bytes, value length 315210. RDD 44:2022 section 6.2.2 specifies five bytes. Partition KAG size was 512. |
| L-VENDOR | Executed | `-vendor apl0` produced MediaInfo `Writing library: Apple` and bitstream vendor `apl0`. A ProRes OP-Atom file without that flag carried vendor `Lavc`. |

### 1.2 Sound, time, and names

| Id | Status | Procedure and result |
| --- | --- | --- |
| L-PCM | Executed | OP1a accepted `pcm_s16le` and `pcm_s24le` at 48000 Hz. It rejected `pcm_s24be`, `pcm_s32le`, 44100 Hz, and 96000 Hz with the muxer errors recorded in the architecture document. |
| L-AES | Executed | OP1a audio element key `060e2b34010201010d01030116010300`. MediaInfo: `PCM`, `Little`, `Frame (AES)`. Stereo 24-bit at 24/1 is 12000 bytes per frame, 2000 samples. |
| L-LAYOUT | Executed | One element of 1, 2, 6, and 8 channels muxed. Eight mono elements in one OP1a file muxed and decoded. ffprobe `channel_layout` was empty on every file. MediaInfo reported the count and no channel positions in the extract. |
| L-RATE | Executed | 24/1, 24000/1001, 25/1, 48/1, and 30000/1001 muxed without `-strict unofficial`. 18/1 muxed only with that flag. At 24000/1001 MediaInfo reported 2002 samples per frame. At 24/1, 2000. |
| L-2997 | Executed | Eight frames at 30000/1001. Video `duration_ts` 8. Audio `duration_ts` 12813 samples at 48000. MediaInfo `1601.6 SPF`. The picture span is 12812.8 sample periods. |
| L-TC | Executed | `-timecode 01:00:00:00` at 24/1 and at 24000/1001 read back on the material package, the source package, and the system item. `-timecode 01:00:00;00` at 30000/1001 read back with the semicolon. MediaInfo also reported `Time code, stripped: Yes`. |
| L-NAME | Executed | `material_package_name`, `file_package_name`, and `reel_name` were set. MediaInfo package name included all three. ffprobe showed `material_package_name` and a `comment_reel` tag, and did not show `file_package_name` or `reel_name` as their own tags. Identification read back as FFmpeg's OP1a muxer. |
| L-OPATOM | Executed | Picture-only OP-Atom succeeded for DNxHR HQX and ProRes 422 HQ, and FFmpeg decoded the DNxHR file. Mono `pcm_s24le` OP-Atom succeeded and was reported as `Frame (BWF)`. With no audio edit rate, the timecode was 25 fps. With `-mxf_audio_edit_rate 24/1`, it was 24 fps. Stereo OP-Atom was rejected: `MXF OPAtom only supports single channel audio`. |
| L-D10-NOT-RUN | Specified, not executed | The `mxf_d10` muxer is present and was not given a native-profile test. Its help and source restrict it to MPEG-2 at fixed rates. It is not a candidate master. |

## 2. Tests still to run

Each row states the required observation. The status is specified, not executed. A pass is that observation and no others invented by the application. A fail is a wrong duration, a wrong timecode, a reordered or dropped channel, a padded DNxHR edge shown as picture, a missed alpha when the file is the alpha specimen, or a relink that attaches a different recording.

The specimen set, when these tests are run, is program-length enough to cross a body partition: at least 30 seconds, at 24/1 and at 24000/1001, plus one 30000/1001 specimen for the sample-count rule. The eight-frame files already measured are the local evidence only.

### 2.1 DaVinci Resolve

| Id | Subject | Required observation | Status |
| --- | --- | --- | --- |
| R-V-HQ | OP1a ProRes 422 HQ, 24-bit PCM, timecode 01:00:00:00 | Reports OP1a, ProRes 422 HQ, the display size, 24/1 or 24000/1001 as written, duration equal to the frame count, timecode 01:00:00:00. | Specified, not executed |
| R-V-HQX | OP1a DNxHR HQX | Reports DNxHR HQX or VC-3 HQX, 10-bit 4:2:2, the display size rather than the padded stored size on the 1998×1080 and 2048×858 specimens. | Specified, not executed |
| R-V-444 | OP1a ProRes 4444 and DNxHR 444, opaque | Reports 4:4:4 and the profile. No invented alpha. | Specified, not executed |
| R-A | Sound | Channel count matches. Playback assignment matches the catalog map for a file AETHER wrote, where the map was stored at write time. A file with only a count is shown as unlabeled channels. | Specified, not executed |
| R-TC | Timecode and drop-frame | 24/1 and 24000/1001 start at 01:00:00:00. The 30000/1001 specimen keeps the semicolon. Duration matches the frame count. | Specified, not executed |
| R-META | Reel and package name | The reel entered as `reel_name` is visible, or the test records that Resolve does not show it. The result is evidence for the catalog, not a silent pass. | Specified, not executed |
| R-RELINK | Relink after the file is moved | The clip stays on the same frames. Match uses duration, rate, reel, timecode, and hash when Resolve is given the hash by the operator. A second file with the same name and a different duration does not attach. | Specified, not executed |
| R-PROXY | Proxy of the same asset | Proxy and master share frame count, rate, and timecode. Switching does not slip the cut. | Specified, not executed |

### 2.2 Avid Media Composer

| Id | Subject | Required observation | Status |
| --- | --- | --- | --- |
| A-V-HQX | OP1a DNxHR HQX | Picture, duration, timecode, and display rectangle as in R-V-HQX. | Specified, not executed |
| A-ATOM-V | OP-Atom DNxHR HQX | The picture atom opens. The test records whether Media Composer treats the `15010600` element key as clip-wrapped. MediaInfo called the local file frame-wrapped. | Specified, not executed |
| A-ATOM-A | One mono OP-Atom `pcm_s24le` file per channel, edit rate set to the picture rate | Channels come back in map order. A 24-frame film is not interpreted as 25 fps timecode. | Specified, not executed |
| A-ATOM-ST | Stereo OP-Atom | Not a specimen this FFmpeg can write. The test is blocked until a writer exists. Status remains specified, not executed, and also blocked. | Specified, not executed |
| A-OP1A-PCM | OP1a with AES-labeled little-endian PCM | The test records whether Media Composer plays the sound, rejects the file, or plays the wrong bit depth. No expected pass is assumed. | Specified, not executed |
| A-TC | Timecode | Same required observation as R-TC. | Specified, not executed |
| A-RELINK | Relink by source timecode and reel | A moved master relinks. A file with the same reel and a different timecode start does not. | Specified, not executed |

### 2.3 Adobe Premiere Pro

| Id | Subject | Required observation | Status |
| --- | --- | --- | --- |
| P-V-HQ | OP1a ProRes 422 HQ | Picture, size, rate, duration, timecode as in R-V-HQ. | Specified, not executed |
| P-V-PRX | OP1a ProRes 422 Proxy | Reported as Proxy, not as HQ. Frame count matches the master specimen. | Specified, not executed |
| P-V-444 | Opaque ProRes 4444 and the alpha specimen | The opaque file has no matte. The alpha specimen either shows the matte or the test records that Premiere ignored it. The local MediaInfo reading is not used as Premiere's result. | Specified, not executed |
| P-A | 24-bit and 16-bit PCM, and a 6-channel file | Count and bit depth match. Channel names match only if Premiere had a label to read. | Specified, not executed |
| P-TC | Timecode, including drop-frame | Same required observation as R-TC. | Specified, not executed |
| P-RELINK | Relink after a path change | The cut's frames stay. Offline remains offline until the identity matches. | Specified, not executed |

### 2.4 FFmpeg, ffprobe, and MediaInfo on program-length files

The short-file rows in section 1 were executed. These longer rows were not.

| Id | Subject | Required observation | Status |
| --- | --- | --- | --- |
| F-LONG | At least 30 seconds, OP1a, ProRes 422 HQ and DNxHR HQX, 24/1 and 24000/1001 | Duration, sample count, and timecode still agree after more than 250 edit units, which is this muxer's body-partition interval. Index seek to the first, middle, and last frame decodes that frame. | Specified, not executed |
| F-SAMPLE | 30000/1001, program length | Audio sample count equals `sampleIndex(frameCount) - sampleIndex(0)` at 48000. The eight-frame file already failed that equality. | Specified, not executed |
| F-WAVE | A file written to the ST 382 wave target, when a writer exists | ffprobe reports `pcm_s24le` at 48000, and MediaInfo reports wave wrapping rather than `Frame (AES)`. | Specified, not executed |
| F-HASH | SHA-256 | Two reads of an unchanged file match. One flipped byte in a copy does not. The catalog stores `hash-differs` for the copy. | Specified, not executed |
| F-ID | Package names and reel | A second probe, not only MediaInfo's combined package-name line, returns the reel as its own value. | Specified, not executed |

## 3. Identity tests

These are catalog tests. They do not require Resolve, Media Composer, or Premiere. They were not executed, because the catalog is not implemented. The procedure is the comparison in [MEDIA_IDENTITY_AND_METADATA.md](MEDIA_IDENTITY_AND_METADATA.md).

| Id | Specimen | Required result | Status |
| --- | --- | --- | --- |
| I-RELINK | Master moved to a new directory. Hash, duration, rate, rectangle, reel, roll, timecode, and map unchanged. | The location path updates. The asset id is unchanged. Clips keep their source ranges. | Specified, not executed |
| I-WRONG | A file with the same display name and a different frame count. | Not attached. The cut stays offline for that asset. | Specified, not executed |
| I-HASH | Same size and duration, different SHA-256. | `hash-differs`. Not verified. | Specified, not executed |
| I-NOHASH | Matching metadata, hash not yet computed. | `hash-absent`. The operator can see that the link is unverified. | Specified, not executed |
| I-MAP | A six-channel file relinked onto an asset whose map is named 5.1. | Refused, because `unknown` does not match a named role. | Specified, not executed |
| I-PROXY | Proxy generated from a verified master, then the master is remuxed so its hash changes. | The proxy row no longer matches the master hash. Playback does not treat it as current. The frame count of a current proxy equals the master. | Specified, not executed |
| I-ORIGIN | A transcode written as a new master of an existing original. | Origin id, reel, roll, timecode label, rate, frame count, display rectangle, and channel map are unchanged. File hash and profile are the transcode's. The original file's bytes are unchanged. | Specified, not executed |
| I-TCZERO | A transcode whose timecode starts at 00:00:00:00 while the asset label is 01:00:00:00. | Not attached to that asset. | Specified, not executed |
| I-PAD | DNxHR HQX at 2048×858. | Display rectangle 2048×858. Stored height 864. A viewer that shows 864 lines as picture fails the check. | Specified, not executed |
| I-PRESERVE | Hash the original, then run the transcode. | The original path is not opened for writing. The hash after the transcode equals the hash before it. | Specified, not executed |

## 4. How a row becomes executed

A row moves from specified to executed only with the application version, the specimen file's hash, and the observed timecode, duration, channel count, and rectangle. A screenshot of a bin is not enough. The cut is parked on a known source frame and the reported timecode and frame count are written down. Until that record exists, the row stays specified, not executed.
