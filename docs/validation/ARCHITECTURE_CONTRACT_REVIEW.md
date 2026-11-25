# Architecture contract review

Status: review against the documents already in force. No contract is accepted by being restated here. No ADR is marked accepted.

Date: 2026-11-20. The new measurement is `docs/validation/FFMPEG_MXF_ROUNDTRIP_TEST.md`. The FFmpeg behaviour spec remains `docs/NATIVE_MEDIA_ARCHITECTURE.md`.

## 1. Contracts this review keeps as proposals

| Contract | Label after this review | Why it still holds |
| --- | --- | --- |
| A bin is an object. A directory is a folder | **Recommended for adoption**, not accepted | Nothing in the media session touches bins. `docs/BIN_ARCHITECTURE.md` stays the owner. The FFmpeg product name inside an MXF file is not a bin |
| One Media ID, representations underneath | **Recommended for adoption**, not accepted | UMIDs and paths in the synthetic files are evidence. ADR-005 is unchanged in status. See `docs/technology/MEDIA_IDENTITY_MODEL_EVALUATION.md` |
| Originals are not modified | **Recommended for adoption**, not accepted | The session wrote only under `validation-fixtures/`. No ingest of camera media |
| Relink is not conform. Export is not a verified conform | **Recommended for adoption**, not accepted | No NLE was run (`docs/validation/EXTERNAL_NLE_RELINK_AND_CONFORM_TEST.md`). A CLI open is neither operation |
| Rational time. Float seconds are not authority | **Recommended for adoption**, and the new measurement is evidence for the rule | On one 30000/1001 file, ffprobe printed format `0.266938`, picture `0.266933`, and audio `0.266937`. The integers were 8 frames and 12813 samples. `docs/TIMEBASE_AND_FRAME_ACCURACY.md` is the authority. The floats are not |
| `sampleIndex(F) = floor(F * 48000 * rate_d / rate_n)` | **Recommended for adoption**, not changed by the muxer | Eight frames at 30000/1001 span 12812 samples in the documented order. This FFmpeg stored 12813 in a different order. The contract stands. The writer fails verification |
| Application is the only joiner. Editorial does not link FFmpeg | **Recommended for adoption**, **subject to prototype** | `docs/technology/FFMPEG_INTEGRATION_ARCHITECTURE.md`. The prototype still links neither Qt nor FFmpeg |
| FFmpeg is the core media framework | Confirmed by the owner | GStreamer is **not recommended** for that role. The confirmation is not an acceptance of ADR-001 |
| GStreamer as core framework | **not recommended** | Owner decision, recorded on ADR-001 |

## 2. Where a document and this binary disagree

The disagreement is a verification failure of the binary against the document, not an edit to the document.

| Document | Document rule | This binary, this session |
| --- | --- | --- |
| `docs/TIMEBASE_AND_FRAME_ACCURACY.md`, `docs/AUDIO_TIMEBASE_AND_SYNC.md` | 8 frames at 30000/1001 and 48000 cover 12812 samples, order 1601, 1602, 1601, 1602, 1602, 1601, 1602, 1601 | OP1a stored 12813 samples, order 1602, 1601, 1602, 1601, 1602, 1602, 1601, 1602, even when the source had 12812 samples |
| `docs/MXF_CONTAINER_SPECIFICATION.md` | Native sound element is ST 382 wave PCM, frame-wrapped to the picture edit unit | MediaInfo reported OP1a audio as `Frame (AES)`, codec id `0D01030102060300`. OP-Atom mono was `Frame (BWF)`. The November study said the same split. This session confirms it for these files only |
| `docs/MXF_CONTAINER_SPECIFICATION.md` | OP-Atom audio edit rate is the picture rate, not the muxer default 25/1 | `-mxf_audio_edit_rate 24/1` set the timecode track to 24 fps and left 25 packets of 1920 samples |
| `docs/NATIVE_MEDIA_ARCHITECTURE.md` | A generic file this FFmpeg opens is not an interchangeable master | Unchanged. Vendor test **Not tested** |
| `docs/SYSTEM_ARCHITECTURE.md` | FFmpeg and GStreamer were both candidates when this review was written | Superseded. The system-architecture file now records ADR-001. GStreamer is not a mandatory dependency |

The 19 November study’s other rows (DNxHR, alpha, other rates, other rasters) were **Not tested** again. They stay warnings to reconcile, not new passes and not new failures.

## 3. Contracts this review does not close

- Canonical project file (`project.db` versus `project.sqlite`). Recommendation only.
- Restart-undo versus session undo. The owner accepted session-only undo for the Foundation Prototype. ADR-009 is **ACCEPTED FOR FOUNDATION PROTOTYPE**. Persistent undo remains future work.
- Qt, the project licence, patents, trademarks. ADR-008 stays **OPEN**.
- Whether sound is its own library. GStreamer is no longer the reason to fold it into Processing. The module question is still C6.
- Capability 19 (`.abin` round trip) included or deferred.
- Colour scope for the first code.

## 4. Result

The cutting-room rules survive the new FFmpeg facts. The binary does not get to rewrite `sampleIndex`. No contract moved from proposal to accepted.
