# FFmpeg integration validation

Status: **Not tested.** The CLI session in `docs/validation/FFMPEG_MXF_ROUNDTRIP_TEST.md` is a different kind of evidence. A command that exits 0 does not show that `aether_process` can link, probe, or decode.

## 1. Why this is not tested

Observed on 2026-11-20:

- `src/` contains no files. There is no AETHER translation unit that includes an FFmpeg header.
- `CMakeLists.txt` is empty. There is no link line.
- Package `ffmpeg-devel` is not installed.
- `/usr/include/ffmpeg` does not exist.
- `pkg-config` does not find `libavcodec`, `libavformat`, `libavutil`, `libswscale`, or `libswresample`.
- `/usr/lib64/libavcodec.so` (the unversioned development symlink) does not exist. The versioned shared object `libavcodec.so.63.1.102` does exist. That is the CLI’s runtime, not a development link.

No package was installed to close that gap. Installing headers would have been a system change this review is not allowed to make, and it still would not have been an AETHER integration test.

## 2. What the CLI session may be used for

It may be cited as the behaviour of `/usr/bin/ffmpeg` 9.0.2 on the synthetic files named in the round-trip document. The 24/1 count match and the 30000/1001 sample-count failure are properties of that binary. They are inputs to the capability rule in `docs/technology/FFMPEG_INTEGRATION_ARCHITECTURE.md`. They do not fill a capability row.

## 3. Procedure for a later integration test

Run this only after implementation is allowed, a pin exists, and counsel has not forbidden the link. This review did not run it.

1. Build the pinned shared libraries from `docs/technology/FFMPEG_BUILD_AND_DISTRIBUTION.md`, not the Fedora package, unless counsel has accepted that package in writing.
2. Link a test target only from the processing module. The link line must not appear on editorial or bin tests (`docs/18_BUILD_AND_RELEASE.md`, `docs/FIRST_IMPLEMENTATION_MILESTONE.md`).
3. Call `libavformat` and `libavcodec` through the private wrapper. Do not shell out to `ffmpeg`.
4. Open `validation-fixtures/ffmpeg-mxf-numbered-2026-11-20/out/op1a_prores422hq_pcm_s24le_stereo_24fps_24f.mxf` read-only. Report frame count 24, rate 24/1, audio samples 48000, and the Media ID must not be taken from the file’s UMID.
5. Open the 30000/1001 file whose source was 12812 samples. The wrapper must report 8 frames and 12813 samples, and the verification step must **fail** the file against `sampleIndex(8) = 12812`. A wrapper that “fixes” the count without recording the failure is a bad test.
6. Confirm public headers of editorial, media, and UI do not include `libavcodec/avcodec.h` or any other FFmpeg header.
7. Record the pin, the flags, and the tuple id beside the result. A pass on step 4 does not enable an encoder and does not pass step 5.

Until that record exists, integration stays **Not tested**.
