# AETHER ProRes Profile Specification

Status: proposal, awaiting review. No implementation is authorized by this document.

ProRes is an intra-frame picture coding. SMPTE RDD 44:2022 maps it into MXF. The bitstream syntax is SMPTE RDD 36, which was not obtained for this document. Profile names, chroma, alpha, and the informative rates below are from RDD 44:2022 Table 2, which was read. Sample depth and four-character codes are from Apple's ProRes white paper, checked against the codes this FFmpeg wrote. The container around the picture is [MXF_CONTAINER_SPECIFICATION.md](MXF_CONTAINER_SPECIFICATION.md). The choice of which profile is a master is [NATIVE_MEDIA_ARCHITECTURE.md](NATIVE_MEDIA_ARCHITECTURE.md).

Measurements are from FFmpeg 9.0.2 `prores_ks` and the `mxf` muxer on 19 November 2026.

## 1. Profiles

RDD 44 Table 2. The rates are informative, at 24 fps, and they move with the picture. Alpha has no nominal rate in that table. Apple's white paper quotes different targets at 1920×1080 and 29.97 fps: about 45 Mbit/s for Proxy, 102 for LT, 147 for 422, and 220 for 422 HQ. AETHER treats both sets as targets, not as a file-size check.

| Profile | Chroma | Alpha in RDD 44 | Informative rate, 1920×1080, 24 fps | Informative rate, 3840×2160, 24 fps |
| --- | --- | --- | --- | --- |
| 422 Proxy | 4:2:2 | None | 36 Mbit/s | 145 Mbit/s |
| 422 LT | 4:2:2 | None | 82 Mbit/s | 329 Mbit/s |
| 422 | 4:2:2 | None | 117 Mbit/s | 472 Mbit/s |
| 422 HQ | 4:2:2 | None | 176 Mbit/s | 707 Mbit/s |
| 4444 | 4:4:4 | None, or 8 or 16 | 264 Mbit/s without alpha | 1.06 Gbit/s without alpha |
| 4444 XQ | 4:4:4 | None, or 8 or 16 | 396 Mbit/s without alpha | 1.59 Gbit/s without alpha |

Apple's white paper says the 422 family accepts image sources up to 10 bits, and that 4444 and 4444 XQ accept image sources up to 12 bits, with a lossless alpha of up to 16 bits. RDD 44 section 6.1 says the same thing about alpha: 8 or 16 bits, optional, on the 4:4:4 profiles only.

## 2. MXF labels

RDD 44 Table 5. These are the picture-essence-coding labels. The frame-wrapped container label for every profile is `060E2B34.0401010D.0D010301.021C0100`.

| Profile | Picture essence coding |
| --- | --- |
| 422 Proxy | `060E2B34.0401010D.04010202.03060100` |
| 422 LT | `060E2B34.0401010D.04010202.03060200` |
| 422 | `060E2B34.0401010D.04010202.03060300` |
| 422 HQ | `060E2B34.0401010D.04010202.03060400` |
| 4444 | `060E2B34.0401010D.04010202.03060500` |
| 4444 XQ | `060E2B34.0401010D.04010202.03060600` |

The frame-wrapped picture element key is `060E2B34.01020101.0D010301.15cc17nn`. The KLV length is five bytes. The descriptor is CDCI for the native masters. Horizontal subsampling is 2 for the 422 family and 1 for 4444 and 4444 XQ. Vertical subsampling is 1. Alpha sample depth is omitted when the frame is opaque, which RDD 44 Table 7 allows, and is 8 or 16 when alpha is present. Coding equations are required even if an RGBA descriptor is used.

Stored width and height follow the bitstream's horizontal and vertical size. For progressive media the stored height is the frame height. Sampled width and height are absent or equal to the stored size. Sample offsets are absent or zero. Display size is the active picture. Frame layout is full frame. Signal standard should be 0. Video line map should be 0.

## 3. Descriptor values AETHER writes

| Item | 422 Proxy, LT, 422, 422 HQ | 4444 and 4444 XQ, native |
| --- | --- | --- |
| Wrapping | Frame, one picture per element | Frame, one picture per element |
| Descriptor | CDCI | CDCI |
| Component depth | 10 | 10 |
| Horizontal subsampling | 2 | 1 |
| Vertical subsampling | 1 | 1 |
| Alpha sample depth | Absent | Absent |
| Frame layout | Full frame, progressive | Full frame, progressive |
| Picture essence coding | The row in section 2 | The row in section 2 |
| Color | Source labels from RDD 44 Table 9 when the source matches one of those rows | The same |
| Vendor id | The encoder's own id. Not `apl0`. | The same |

4444 XQ is a readable high-end profile. The native high-end choice is opaque 4444. XQ stays the camera or facility file when that is what arrived.

## 4. What this FFmpeg encodes

`ffmpeg -encoders` lists `prores`, `prores_aw`, `prores_ks`, and `prores_ks_vulkan`.

`prores_ks` help:

| Option | Values |
| --- | --- |
| Pixel formats | `yuv422p10le`, `yuv444p10le`, `yuva444p10le` |
| `-profile` | `-1` auto, `0` proxy, `1` lt, `2` standard, `3` hq, `4` 4444, `5` 4444xq |
| `-vendor` | Four-character string, default `Lavc` |
| `-alpha_bits` | 0 to 16, default 16 |
| `-quant_mat` | auto, proxy, lt, standard, hq, default |

`prores` and `prores_aw` help lists the same three pixel formats, a vendor default of `fmpg`, and no profile option. Both wrote ProRes 422, four-character code `apcn`, when muxed. They are not the native encoder. `prores_ks_vulkan` was listed and was not run.

An 8-bit `yuv422p` input to `prores_ks` profile `hq` did not encode as 8-bit ProRes. FFmpeg logged `Incompatible pixel format 'yuv422p' for codec 'prores_ks', auto-selecting format 'yuv422p10le'` and wrote a 10-bit HQ file. There is no 8-bit ProRes profile in this encoder.

### 4.1 Files that completed

Each file is OP1a, eight frames, 1920×1080, 24/1, plus stereo `pcm_s24le` at 48 kHz, unless noted. FFmpeg decoded each of the HQ, 4444, and alpha files with an empty error log. The other rows were muxed and reported by the encoder log and, where noted, by ffprobe or MediaInfo.

| Profile argument | Pixel format | Four-character code | Read-back |
| --- | --- | --- | --- |
| `proxy` | `yuv422p10le` | `apco` | ffprobe profile `Proxy`, 10 bits. MediaInfo `422 Proxy` on the proxy files used for audio tests. |
| `lt` | `yuv422p10le` | `apcs` | Encoder log profile `LT`. |
| `standard` | `yuv422p10le` | `apcn` | Encoder log profile `Standard`. |
| `hq` | `yuv422p10le` | `apch` | ffprobe `HQ`, `yuv422p10le`, 10 bits. MediaInfo `422 HQ`, 4:2:2, 10 bits, progressive. Frame header identifier `icpf`, vendor `apl0` because this particular file set `-vendor apl0`. |
| `4444` | `yuv444p10le` | `ap4h` | ffprobe pixel format `yuv444p12le`, `bits_per_raw_sample` 12. MediaInfo bit depth 10, chroma 4:4:4. |
| `4444xq` | `yuv444p10le` | `ap4x` | Encoder log `XQ`. ffprobe `yuv444p12le`, 12 bits. |
| `4444` | `yuva444p10le`, `alpha_bits` 16 | `ap4h` | ffprobe `yuva444p12le`. MediaInfo reported 4:4:4 and 10 bits and did not mention alpha. |

The 10-bit versus 12-bit pair is both true and easy to confuse. The encoder's input pixel format is 10-bit. MediaInfo's bit depth follows the MXF descriptor, which this muxer fills from that 10-bit format. FFmpeg's ProRes decoder reports a 12-bit buffer for the 4444 family. The file is not evidence of a 12-bit source.

The picture element key on the HQ file was `060e2b34010201010d01030115011700`. The length field was 4 bytes. RDD 44 asks for 5. The container and coding identifiers MediaInfo printed for HQ match section 2.

1920×1080, 1998×1080, and 2048×858 all muxed as HQ. ffprobe reported those exact sizes. No padded stored rectangle was observed on the ProRes side of that comparison. The DNxHR side of the same rasters is in the DNxHR document.

Color. Files encoded with BT.709 primaries, transfer, and matrix were reported by MediaInfo with matrix BT.709. A 4444 alpha file encoded without those flags had no matrix line in MediaInfo. The native writer sets the labels from the catalog. It does not leave them unspecified when the catalog knows them.

## 5. Native rows

| Catalog profile | `prores_ks` profile | Pixel format | Coding label byte 15 |
| --- | --- | --- | --- |
| `prores-422-hq` | `hq` | `yuv422p10le` | `0x04` |
| `prores-422-proxy` | `proxy` | `yuv422p10le` | `0x01` |
| `prores-4444` | `4444` | `yuv444p10le` | `0x05` |

LT (`0x02`), 422 (`0x03`), and 4444 XQ (`0x06`) are accepted on ingest when the coding label, the four-character code, and the chroma agree. They are not the profiles AETHER picks for a new master or a new proxy.

Alpha is a property of an ingested 4444 or 4444 XQ file when the descriptor's alpha sample depth is 8 or 16 and a decoder returns a matte. This FFmpeg can put a matte in the bitstream and can decode it. It does not write alpha sample depth. That file is not a native master. Decision 5 in the architecture document records the review.

## 6. Checks on ingest

Verification of a ProRes file compares:

- Frame header identifier `icpf` and the four-character code, against the profile.
- Picture essence coding label, against the same profile.
- CDCI subsampling, against 4:2:2 or 4:4:4.
- Component depth in the descriptor. Native masters read 10. A descriptor depth that disagrees with the profile fails.
- Alpha sample depth. Present only with a real matte. Absent on native masters.
- Display size and frame count, against the identity.
- Vendor id. Stored as information. `apl0` does not mean the file was written by Apple. This probe produced that MediaInfo reading by setting the vendor string.

A file that fails the label comparison stays in the catalog as an ingested source. It is not promoted to a native master.
