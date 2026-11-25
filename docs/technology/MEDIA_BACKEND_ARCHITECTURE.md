# Media backend architecture

Status: backend boundary for ADR-001. FFmpeg is the mandatory core framework, inside the Media Engine. The link is **ACCEPTED WITH CONDITIONS** and is not demonstrated (`docs/validation/FFMPEG_LIBRARY_INTEGRATION_VALIDATION.md`). GStreamer is not a second mandatory backend. The build baseline is `docs/technology/FFMPEG_DEVELOPMENT_CONFIGURATION.md`. Behaviour detail stays in `docs/technology/FFMPEG_INTEGRATION_ARCHITECTURE.md`.

Owners: `docs/ENGINEERING_INTERFACES.md`, `docs/10_PLAYBACK_AND_RENDERING.md`, `docs/SYSTEM_ARCHITECTURE.md`, `docs/NATIVE_MEDIA_ARCHITECTURE.md`, `docs/MEDIA_RELINK_ENGINE.md`, `docs/CONFORM_ENGINE_SPECIFICATION.md`.

## 1. Place in the process

Processing is a backend behind the application. It is not a module the timeline calls.

1. Editorial returns slices for frame F. Each slice carries a Media ID, an instance id, and integer ranges. It does not carry a path.
2. Media returns locations for those Media IDs, including which representation is selected for playback. Selection is not a new Media ID.
3. The application builds a picture request and an audio request in rationals and sample indexes.
4. Processing talks to FFmpeg in private and returns frames and samples, or an error.
5. The UI paints. It does not decode.

A test double stands in for Processing in the Foundation Prototype. The milestone forbids a linked FFmpeg on that prototype.

## 2. Two operations

| Operation | Reads | Writes | Authority it must not claim |
| --- | --- | --- | --- |
| Probe and decode | A location, read-only | A report and presented frames in memory | Not a Media ID, not a conform, not a change to the cut |
| Encode | A request that names a new path | A new file | Not an original. Not a verified conform. The report is an encode report |

Relink is neither of these. Relink is a media-management update of a location row. Conform is a later comparison with its own report (`docs/CONFORM_VALIDATION_AND_REPORTING.md`). An export that finishes in FFmpeg is not that report.

## 3. Clock

The sequence frame is the playhead. FFmpeg’s packet timestamps, `duration` floats, and any device clock are converted or discarded at the boundary. They do not write the playhead and they do not write the undo stack.

The 30000/1001 measurement is the concrete case. Three float durations on one file disagreed in the sixth decimal. The integer counts are what verification compares: 8 picture frames against `sampleIndex(8) = 12812`. This backend returned 12813. The editorial number stays 12812.

## 4. Capability, not a framework menu

The UI asks Processing what tuples are available. Processing answers from the validation table, not from `avcodec_find_encoder` and not from `ffmpeg -encoders`. An encoder that exists in the library and has no passing tuple is hidden. OP1a and OP-Atom are separate tuples. Stereo OP-Atom PCM is **Not supported** on the measured build and must not appear as a choice merely because `mxf_opatom` is linked.

`docs/NATIVE_MEDIA_ARCHITECTURE.md` remains the profile proposal a passing tuple would be compared with: OP1a as the working-file proposal, OP-Atom as optional export, 48 kHz 24-bit PCM, the six editorial rates, no `apl0` vendor id. This backend document does not accept that proposal. Where this FFmpeg diverges (AES-labelled PCM on OP1a, sample-count grid at 30000/1001, 25 packets of 1920 on an OP-Atom file whose timecode track says 24 fps), the file may be described as a local mezzanine only after a tuple says so. It is not offered as an interchange master.

## 5. Failure

| Case | Result |
| --- | --- |
| Location offline | Editorial still returns the instance and the Media ID. Processing is not asked to invent a path |
| Probe disagrees with the stored frame count, rate, or `sampleIndex` span | Verification failure on that representation. The Media ID does not change |
| Encode aimed at an original path | Refused |
| Codec tuple not in the table | Not offered. The command is an error, not a silent fallback to `mpeg2video` or `pcm_s16le`, which are this muxer’s defaults |
| Decoder fault | Must not delete the project and must not delete the original. Isolation is **further research** |

## 6. What remains out of this backend

Colour baking, effect rasterisation, AVX, AAF, OTIO, a conform verdict, and device output. Playback comparison against a non-realtime oracle is still a later phase (`docs/10_PLAYBACK_AND_RENDERING.md`). No oracle was run.
