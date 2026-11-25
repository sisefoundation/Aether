# AETHER DNxHR Profile Specification

Status: proposal, awaiting review. No implementation is authorized by this document.

DNxHR is the resolution-independent family of Avid DNx picture coding. The coding is VC-3. SMPTE ST 2019-1 specifies the picture coding. SMPTE ST 2019-4 maps coding units into the MXF Generic Container, frame-wrapped or clip-wrapped. A 2016 text of the mapping was read for scope. A DOI record lists a 2026 revision of ST 2019-4, and Avid's public DNx 4 notes describe ST 2019-1:2026 as a wider codec: the names DNxHD and DNxHR fold into "Avid DNx", bit depth is no longer tied to the level in that marketing, RGB and alpha are described on the levels, and bit rate can vary. Those 2026 texts were not obtained. This specification is the five levels FFmpeg 9.0.2 encodes, which are the levels AETHER can write today.

The container is [MXF_CONTAINER_SPECIFICATION.md](MXF_CONTAINER_SPECIFICATION.md). Which level is a master or a proxy is [NATIVE_MEDIA_ARCHITECTURE.md](NATIVE_MEDIA_ARCHITECTURE.md).

## 1. Levels AETHER names

These names are the `dnxhd` encoder's profile names. The compression id is the 32-bit value at offset `0x28` in the VC-3 coding unit. Each id below was read from a file this encoder wrote.

| Catalog name | Encoder profile | Compression id | Pixel format the encoder accepted | Bits | Sampling |
| --- | --- | --- | --- | --- | --- |
| DNxHR LB | `dnxhr_lb` | 1274 | `yuv422p` | 8 | 4:2:2 |
| DNxHR SQ | `dnxhr_sq` | 1273 | `yuv422p` | 8 | 4:2:2 |
| DNxHR HQ | `dnxhr_hq` | 1272 | `yuv422p` | 8 | 4:2:2 |
| DNxHR HQX | `dnxhr_hqx` | 1271 | `yuv422p10le` | 10 | 4:2:2 |
| DNxHR 444 | `dnxhr_444` | 1270 | `yuv444p10le` or `gbrp10le` | 10 | 4:4:4, or RGB |

`yuv422p10le` into LB, SQ, or HQ was rejected: `pixel format is incompatible with DNxHR LB/SQ/HQ profile`. `yuv422p` into HQX was rejected and wrote no file. There is no alpha option on this encoder. There is no 12-bit or 16-bit pixel format on this encoder. Custom bit rates are not an encoder option in `ffmpeg -h encoder=dnxhd`.

Avid's public notes distinguish HQ and HQX at the same nominal rate by the quantizer range: HQX keeps more precision above 8 bits, HQ is the 8-bit high-quality level. That distinction matches the split this encoder enforces. AETHER keeps the two names. HQX is the alternative working master. HQ is a readable 8-bit ingest. LB is the DNx proxy. SQ is a readable ingest, not a proxy AETHER generates. 444 is the high-end DNx master.

## 2. MXF mapping this muxer writes

Native wrapping is frame wrapping: one VC-3 compressed frame in each content package. On OP1a files the picture element key ended in `15010c00`. MediaInfo reported `Frame` and `VC-3`.

The picture-essence-coding labels in FFmpeg 9.0.2's muxer table, selected by the compression id, are:

| Compression id | Label in the muxer table | MediaInfo on the file |
| --- | --- | --- |
| 1270 | `060E2B34.0401010A.04010202.71240000` | `RI@444`, codec id ending `0401020271240000`, on both the `yuv444p10le` file and the `gbrp10le` file |
| 1271 | `060E2B34.0401010A.04010202.71250000` | `RI@HQX`, codec id ending `0401020271250000` |
| 1272 | `060E2B34.0401010A.04010202.71260000` | Not printed in the MediaInfo extract kept for HQ. The compression id in the file is 1272, and the muxer selects this row from that id. |
| 1273 | `060E2B34.0401010A.04010202.71270000` | Same situation as HQ. Compression id 1273. |
| 1274 | `060E2B34.0401010A.04010202.71280000` | Same situation as HQ. Compression id 1274. |

The container label on the HQX and 444 files, as printed by MediaInfo, is `0D01030102110100` in the short form MediaInfo uses, which corresponds to the muxer's container label `060E2B34.0401010A.0D010301.02110100`.

For compression id 1270 the muxer switches the picture descriptor key to the RGBA descriptor. The pixel layout it writes is R, G, B at the component depth, with no alpha code. Component depth follows the pixel format: 10 for `yuv444p10le` and `gbrp10le`. The `gbrp10le` file was reported with matrix `Identity`. The `yuv444p10le` file, encoded with BT.709 flags, was reported with matrix BT.709. MediaInfo did not print a chroma-subsampling line on either 444 file. ffprobe reported `yuv444p10le` and `gbrp10le`.

HQX stays on the CDCI descriptor: horizontal subsampling 2, vertical subsampling 1, component depth 10. LB, SQ, and HQ are CDCI, component depth 8, horizontal subsampling 2.

OP-Atom DNxHR HQX used a different element key, ending `15010600`. FFmpeg's source comment says it writes `0x06` rather than the clip-wrap value in ST 2019-4:2009, and that it does this for Avid systems. That comment is the muxer's stated intent. Avid was not run. MediaInfo reported the same file as wrapping mode `Frame` and as operational pattern OP-Atom. The byte and the MediaInfo word disagree. The interchange test records both and does not pick a winner in advance.

## 3. Rectangles

VC-3 coding units are macroblock-aligned. The muxer rounds stored width and stored height up to a multiple of 16 for compression ids 1270 and above. The display and sampled rectangles keep the requested size.

Observed in the header of the short files:

| Requested size | Stored integers present | Display integers present | ffprobe and MediaInfo size |
| --- | --- | --- | --- |
| 1920×1080 | 1920 and 1080, already aligned | 1920×1080 | 1920×1080 |
| 1998×1080 | 2000×1088 | 1998×1080 | 1998×1080 |
| 2048×858 | width 2048, stored height 864 | height 858 | 2048×858 |

The identity stores both rectangles. The picture the editor shows is the display rectangle. A verification failure is a display rectangle that does not match the catalog, or a stored rectangle that is not the display rectangle rounded up to a multiple of 16.

ProRes of the same 1998×1080 and 2048×858 frames did not show this padding. The rule is specific to DNxHR.

## 4. Descriptor values AETHER writes

| Item | LB | SQ | HQ | HQX | 444 |
| --- | --- | --- | --- | --- | --- |
| Wrapping on a native master | Frame | Frame | Frame | Frame | Frame |
| Compression id | 1274 | 1273 | 1272 | 1271 | 1270 |
| Descriptor | CDCI | CDCI | CDCI | CDCI | CDCI for component 4:4:4. The RGB ingest records an RGB layout and identity coding equations. |
| Component depth | 8 | 8 | 8 | 10 | 10 |
| Horizontal subsampling | 2 | 2 | 2 | 2 | 1 |
| Vertical subsampling | 1 | 1 | 1 | 1 | 1 |
| Alpha | Absent | Absent | Absent | Absent | Absent |
| Frame layout | Progressive | Progressive | Progressive | Progressive | Progressive |
| Stored rectangle | Display, rounded up to a multiple of 16 | The same | The same | The same | The same |
| Display rectangle | Active picture | Active picture | Active picture | Active picture | Active picture |
| Color | Source labels | Source labels | Source labels | Source labels | Source labels, or identity matrix for `gbrp10le` |

Constant frame size is a property of these profiles. The eight-frame 1920×1080 HQX OP1a file had a picture-element value length of 917504 bytes on every inspected frame. The index may therefore be a constant edit-unit byte count. A size that changes from frame to frame fails verification for these levels.

## 5. Native rows

| Catalog profile | Level | Pixel format | Role |
| --- | --- | --- | --- |
| `dnxhr-hqx` | HQX, 1271 | `yuv422p10le` | Alternative working master |
| `dnxhr-lb` | LB, 1274 | `yuv422p` | Proxy |
| `dnxhr-444` | 444, 1270 | `yuv444p10le`, or `gbrp10le` when the source is RGB | High-end master |

SQ and HQ are ingested and can remain the master when the file already is that profile and the rest of the identity matches. AETHER does not generate them.

Interlaced DNxHD compression ids are a different family. The muxer table includes them, with fixed rasters and an interlaced flag. They are not DNxHR and they are not native masters. A file that carries one of those ids is an ingested source with its own rate and raster.

## 6. Checks on ingest

- The compression id at offset `0x28` matches the coding label.
- Component depth and subsampling match the level.
- The display rectangle matches the catalog. The stored rectangle is the padded one when the active size is not a multiple of 16.
- Frame size is constant across the file.
- The frame count equals the audio edit-unit count on a native master.
- Alpha is absent. A future file that claims alpha under the 2026 codec is ingested as an original and is not a native master until a writer and a reader on the approved stack both handle it. This FFmpeg is not that writer.
