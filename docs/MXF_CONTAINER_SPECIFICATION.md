# AETHER MXF Container Specification

Status: proposal, awaiting review. No implementation is authorized by this document.

This document specifies the MXF file AETHER writes and the MXF file AETHER accepts as a native master. The choice of profile and the identity record are in [NATIVE_MEDIA_ARCHITECTURE.md](NATIVE_MEDIA_ARCHITECTURE.md), [PRORES_PROFILE_SPECIFICATION.md](PRORES_PROFILE_SPECIFICATION.md), [DNXHR_PROFILE_SPECIFICATION.md](DNXHR_PROFILE_SPECIFICATION.md), and [MEDIA_IDENTITY_AND_METADATA.md](MEDIA_IDENTITY_AND_METADATA.md).

The measurements below are from FFmpeg 9.0.2 (`/usr/bin/ffmpeg`, libavformat 63.1.102) and MediaInfoLib 26.05 on 19 November 2026, plus the FFmpeg 9.0.2 muxer source at tag `n9.0.2`. A file this muxer can open is a local fact. It is not, by itself, an interchangeable master.

## 1. Pattern

The default container is Operational Pattern 1a, SMPTE ST 378. One material package, one top-level file package, one internal essence container. Picture and sound of a take are interleaved in that container. A lower-level source package may carry the reel name. The file has no run-in. RDD 44:2022 section 5.7 requires the absence of a run-in for ProRes, and ST 390 requires a zero-length run-in for OP-Atom. AETHER uses the same rule for every native file.

OP-Atom, SMPTE ST 390, is optional output. Each file holds one essence track. A picture export is one file. Each sound channel is its own file, because the only OP-Atom audio this FFmpeg accepted is a single channel. The header is closed and complete, and the index is in the footer. OP-Atom is a poor recording format while the file is still open, which ST 390 states in its own introduction. AETHER does not record a growing take as OP-Atom.

The `mxf_d10` muxer is a different pattern. It is limited by this FFmpeg to MPEG-2 at 30, 40, or 50 Mbit/s and to one audio track. It is not a native profile.

## 2. File structure AETHER requires

The structural rules come from SMPTE ST 377-1:2019, which RDD 44 cites, and from ST 379-2:2010 for the Generic Container.

| Item | Requirement |
| --- | --- |
| Header partition | Closed and complete when the write finishes. A file still being written may be open. AETHER does not present an open file as a finished master. |
| Body | Frame-wrapped content packages. One package is one picture frame and the audio samples that belong to that frame. |
| Footer | Present on a finished file, closed and complete, repeating the index. |
| Random index pack | Present. RDD 44 section 5.4 requires it for ProRes. AETHER requires it for every native file. |
| KLV fill and KAG | Legal under ST 377-1. The target KAG size is 1, which RDD 44 section 5.6 recommends. |
| Index | One entry per edit unit for variable-size picture, so a frame can be entered without reading the essence before it. Constant-size picture may use a constant edit-unit byte count. |
| Preface | Operational pattern 1a. Essence-container batch lists the picture container and, when sound is present, the sound container. |
| Packages | One material package and one top-level file package of equal duration. A source package carries the reel when a reel is known. |
| Timecode | A material-package timecode track and a source-package timecode track. The label is the source timecode. Drop-frame is a flag on that label. It does not remove frames from the index. |
| Descriptive metadata | Reel, camera roll, and channel names that the MXF file cannot carry structurally are stored in the catalog. User comments are optional and are not the identity. |

Edit rate and sample rate of the picture track are the frame rate, one frame per edit unit, as RDD 44 section 5.5 requires for ProRes. The same edit unit is the picture frame for DNxHR. The six native rates are 24/1, 25/1, 24000/1001, 30000/1001, 48/1, and 60000/1001.

## 3. Picture mapping

### 3.1 ProRes

RDD 44:2022 maps each ProRes frame as one frame-wrapped picture element. The element key it specifies is `060E2B34.01020101.0D010301.15cc17nn`: byte 14 is the count of picture elements in the item, byte 15 is the frame-wrapped ProRes element type, byte 16 is the element number. The essence-container label is `060E2B34.0401010D.0D010301.021C0100`. The picture-essence-coding label is one of the six labels in [PRORES_PROFILE_SPECIFICATION.md](PRORES_PROFILE_SPECIFICATION.md).

The KLV length RDD 44 section 6.2.2 requires is five bytes. The picture descriptor is a CDCI descriptor for component pictures, which is the native case, or an RGBA descriptor when a file actually carries an alpha plane that the descriptor admits. Frame layout is `full_frame` (0) for progressive media. Native masters are progressive. Interlaced and progressive-segmented-frame values exist in RDD 44 and are not native masters.

Color items follow the source. RDD 44 Table 9 gives registered labels for BT.601, BT.709, BT.2020, BT.2100 PQ, BT.2100 HLG, and P3 D65 PQ. AETHER writes the label that matches the catalog's color names and leaves the image in the working space. The monitoring view is not baked, which is the export rule already in the architecture.

### 3.2 DNxHR

DNxHR is VC-3, mapped by SMPTE ST 2019-4 as frame-wrapped or clip-wrapped coding units. Native files are frame-wrapped. One content package holds one progressive compressed frame. The compression id in the 640-byte VC-3 header is the profile id. The values this encoder writes are in [DNXHR_PROFILE_SPECIFICATION.md](DNXHR_PROFILE_SPECIFICATION.md).

The active picture is the display rectangle. Where the codec pads the stored rectangle to a multiple of 16, both rectangles are stored in the identity. The display rectangle is the picture.

## 4. Sound mapping

The target is SMPTE ST 382 wave audio essence: little-endian PCM, the sample packing used by Broadcast Wave, frame-wrapped so that each content package holds the samples of one picture frame.

| Property | Native value |
| --- | --- |
| Sampling rate | 48000/1 |
| Bits per sample | 24 on a new master. 16 is accepted on ingest and is not the default write. |
| Channel count | The source count, from 1 through 32. |
| Channel order | The catalog channel map. The file's interleave follows that map. |
| Edit-unit length | The samples whose indexes fall in `[sampleIndex(F), sampleIndex(F+1))` at 48000 and the picture rate. |
| Locked audio | Locked to the picture edit rate. |
| Element | One wave sound element for the interleaved channels. |

At 24/1 that edit unit is 2000 samples. At 25/1 it is 1920. At 24000/1001 it is 2002. At 48/1 it is 1000. At 30000/1001 and 60000/1001 the count alternates. The native contract is the integer `sampleIndex` function. A writer that uses a different rounding fails verification.

ST 382 also maps AES3. AES3 essence, in that standard, is AES3 subframe data. AETHER's native element is the wave element. An ingested file that carries real AES3 subframes is a source the catalog can describe. It is not rewritten in place.

OP-Atom audio, when that export is used, is one mono wave file per channel. Its timecode edit rate is the picture rate of the take. This FFmpeg's default for that option is 25/1, which is wrong for a 24-frame film unless the caller sets it.

## 5. Descriptor set

Every native profile carries these values. Profile-specific rows are in the ProRes and DNxHR documents.

| Descriptor item | Picture | Sound |
| --- | --- | --- |
| Essence container | The profile's container label | The wave container label on a target file |
| Sample rate / edit rate | The frame rate, reduced | 48000/1, and the picture edit rate for the track |
| Duration | Frame count of the take | The same frame count, in edit units |
| Stored rectangle | Codec stored size | — |
| Display rectangle | Active picture, origin and size | — |
| Frame layout | Progressive, full frame | — |
| Aspect ratio | The catalog's display ratio, as a rational | — |
| Picture essence coding | The profile label | — |
| Component depth | 10 for ProRes and for DNxHR HQX and 444. 8 for DNxHR LB, SQ, and HQ. | — |
| Subsampling | 2×1 for 4:2:2 profiles, 1×1 for 4:4:4 profiles | — |
| Alpha sample depth | Absent on native masters | — |
| Color primaries, transfer, coding equations | The source labels, when known | — |
| Audio sampling rate | — | 48000/1 |
| Channel count | — | The map's length |
| Bits per sample | — | 24, or 16 on a legacy ingest |
| Block align and average bytes per second | — | Count times bytes per sample, and that quantity times 48000, for the wave descriptor |
| Timecode start | The source label, on both packages | The same label |

Signal standard for a ProRes file should be 0, which is the value RDD 44 Table 6 asks for. Video line map should be 0 on that same table. This probe did not read those two integers back out of a file.

## 6. What FFmpeg 9.0.2 actually writes

The `mxf` muxer writes the OP1a label above. The `mxf_opatom` muxer writes the OP-Atom label `060e2b34.04010102.0d010201.10030000`. Both were found at byte 84 of the short files, which is the operational-pattern field of the header partition pack.

Observed on the short OP1a files:

| Item | Observed |
| --- | --- |
| MXF version in the partition pack | 1.3. MediaInfo reported the same. |
| KAG size | 512 (`0x00000200` in the partition pack). |
| ProRes picture key, one picture element | `060e2b34010201010d01030115011700` |
| ProRes BER length | 4 bytes. A 422 HQ frame had value length 315210. |
| ProRes container and coding | MediaInfo codec id `0D010301021C0100-0401020203060400` for 422 HQ, which matches RDD 44's container label and the 422 HQ coding label. |
| DNxHR picture key | `060e2b34010201010d01030115010c00` |
| DNxHR HQX coding | MediaInfo `0D01030102110100-0401020271250000`. Compression id 1271 at offset `0x28` of the coding unit. |
| OP1a audio key | `060e2b34010201010d01030116010300` |
| OP1a audio payload | Little-endian PCM. A stereo 24-bit frame at 24 fps is 12000 bytes, which is 2000 samples. MediaInfo: `Frame (AES)`. |
| OP-Atom DNxHR element | One key ending `15010600`, 9-byte BER, value length 7340032 for eight HQX frames. MediaInfo wrapping word: `Frame`. |
| OP-Atom PCM element | Key ending `16010100`, MediaInfo `Frame (BWF)`, codec id `0D01030102060100`. |
| Timecode | Material package, source package, and system item. Drop-frame semicolon preserved at 30000/1001. MediaInfo `Time code, stripped: Yes`. |
| Channel layout | Count only. ffprobe `channel_layout` empty. |
| Identification | `company_name=FFmpeg`, `product_name=OP1a Muxer`, product UID `adab4424-2f25-4dc7-92ff-29bd000c0002`. |
| Package names | `material_package_name` visible to ffprobe. `reel_name` visible to MediaInfo as part of a combined package name. |

The muxer accepts only `pcm_s16le` and `pcm_s24le`, and only 48000 Hz. That limit is an explicit error in `mxfenc.c`, and it was reproduced. The same source maps those two codecs to the AES3 container index for OP1a, and to the wave container index for OP-Atom audio.

ProRes and DNxHR both muxed at 1920×1080, 1998×1080, and 2048×858. The DNxHR descriptor stored the padded rectangle and the display rectangle. Details are in the DNxHR document.

`prores_ks` with vendor `apl0` caused MediaInfo to report `Writing library: Apple`. The bitstream vendor field of that file was the four characters `apl0`. A file written without that override carried `Lavc`. AETHER does not set `apl0`.

## 7. Ingest rules

Processing probes. Media Management stores the probe as data.

- The operational pattern is read from the partition pack. OP1a and OP-Atom are known patterns. Any other pattern is an ordinary ingested file, not a native master.
- The picture essence coding and, for VC-3, the compression id select the profile. A mismatch between the coding label and the compression id fails verification.
- Display rectangle, stored rectangle, frame rate, frame count, and timecode label are stored on the asset.
- Channel count is stored from the descriptor. Channel names are stored only when the file actually carries them, or when the operator assigns them. A count of 2 does not become a stereo map by assumption.
- Duration is the frame count at the file's rate. A sound edit-unit total that disagrees with `sampleIndex` over that frame count fails verification of a native master. The 30000/1001 file written by this FFmpeg is an example of that disagreement: eight picture frames, 12813 audio samples, picture span 12812.8 sample periods.
- The file's UMID and the writer identification are location fields. They do not become the asset id.

## 8. Write rules for a finished master

A finished master uses the target in sections 2 through 5. A local mezzanine written by FFmpeg 9.0.2 may differ in the ways section 6 records. The catalog stores which of the two the file is. Export of an interchange master uses the target. Playback may use the local mezzanine this FFmpeg can decode.

The writer sets the picture codec and profile explicitly. The muxer's default video codec is `mpeg2video`, and its default audio codec is `pcm_s16le`. Neither default is a native master.

The writer sets the timecode label, the material package name, the file package name, and the reel name from the identity record. It sets a real creation time. It does not invent a reel from the file name.
