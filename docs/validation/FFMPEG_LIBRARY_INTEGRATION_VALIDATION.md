# FFmpeg library integration validation

Status: **BLOCKED**. The CLI runs. The libraries were not linked. CLI success is not library integration. This file is separate from `docs/validation/FFMPEG_MXF_ROUNDTRIP_TEST.md` and from `docs/validation/AUDIO_FRAME_SAMPLE_BOUNDARY_VALIDATION.md`.

No package was installed. No editor and no UI were built. The product `CMakeLists.txt` at the repository root was not modified.

## 1. Host

| Item | Value |
| --- | --- |
| OS | Fedora Linux 45 (Workstation Edition), kernel `7.2.15-300.fc45.x86_64` |
| gcc | GCC 16.2.1 20261029 (Red Hat 16.2.1-3) |
| g++ | same |
| clang | 23.1.4 (Fedora 23.1.4-1.fc45). Not used to compile the harness |
| CMake | 4.3.0 |
| pkg-config | 2.5.1 |

Recorded in `/home/editry/development/sise-editor/validation-fixtures/ffmpeg-library-inspect-2026-11-20/environment.txt`.

## 2. CLI and runtime libraries

`/usr/bin/ffmpeg` is FFmpeg 9.0.2, packages `ffmpeg-9.0.2-3.fc45.x86_64` and `ffmpeg-libs-9.0.2-3.fc45.x86_64`.

Runtime versions printed by `ffmpeg -version`:

| Library | Version | In the approved link set |
| --- | --- | --- |
| libavutil | 61.1.102 | Yes |
| libavcodec | 63.1.102 | Yes |
| libavformat | 63.1.102 | Yes |
| libswscale | 10.1.102 | Yes |
| libswresample | 7.1.102 | Yes |
| libavfilter | 12.1.102 | No |
| libavdevice | 63.1.102 | No |

Versioned sonames are on the library path, including `/usr/lib64/libavcodec.so.63.1.102` and the matching files for the other libraries above. Unversioned development symlinks (`libavcodec.so` and the rest) are absent.

The configure line includes `--enable-shared`, `--disable-static`, `--enable-gpl`, `--enable-libfdk-aac`, `--enable-decklink`, `--enable-libx264`, and `--enable-libx265`. Full text: `validation-fixtures/ffmpeg-library-inspect-2026-11-20/buildconf.txt` and `environment.txt`. This build is not the approved distribution configuration (`docs/technology/FFMPEG_DEVELOPMENT_CONFIGURATION.md`).

## 3. Headers and pkg-config

`rpm -q ffmpeg-devel` reports that the package is not installed.

Absent:

- `/usr/include/ffmpeg`
- `/usr/include/libavcodec`, `/usr/include/libavutil`, `/usr/include/libavformat`, `/usr/include/libswscale`, `/usr/include/libswresample`
- `/usr/lib64/pkgconfig/libav*.pc` and `libsw*.pc`

`pkg-config --exists` fails for `libavutil`, `libavcodec`, `libavformat`, `libswscale`, `libswresample`, `libavfilter`, and `libavdevice`. The failure text is in `environment.txt`.

The missing development package on this Fedora install is `ffmpeg-devel`, matching the installed `ffmpeg-9.0.2-3.fc45` line. It was not installed. Installing it would not by itself make the Fedora GPL build the approved distribution configuration.

## 4. Codec inventory from the CLI

Full listings, not a capability table:

- `validation-fixtures/ffmpeg-library-inspect-2026-11-20/muxers.txt`
- `validation-fixtures/ffmpeg-library-inspect-2026-11-20/demuxers.txt`
- `validation-fixtures/ffmpeg-library-inspect-2026-11-20/decoders.txt`
- `validation-fixtures/ffmpeg-library-inspect-2026-11-20/encoders.txt`
- `validation-fixtures/ffmpeg-library-inspect-2026-11-20/hwaccels.txt`
- `validation-fixtures/ffmpeg-library-inspect-2026-11-20/formats.txt`

Hardware acceleration methods printed by `ffmpeg -hwaccels`: vdpau, cuda, vaapi, qsv, drm, opencl, vulkan, amf. None was used. None is an AETHER capability.

Muxers named in the listing include `mxf`, `mxf_d10`, and `mxf_opatom`. Encoders named include `prores_ks`, `dnxhd`, `pcm_s24le`, `mpeg2video`, `libx264`, `libx265`, and `libfdk_aac`. Naming them is not a passing tuple.

## 5. Harness

Source, and not product code:

- `/home/editry/development/sise-editor/tools/validation/ffmpeg_link_smoke_test/CMakeLists.txt`
- `/home/editry/development/sise-editor/tools/validation/ffmpeg_link_smoke_test/main.c`

The CMake file searches only `libavutil`, `libavcodec`, `libavformat`, `libswscale`, and `libswresample`. It does not search `libavfilter` or `libavdevice`. It refuses a static archive if the pkg-config link line contains one. The C file, when it can compile, prints the five runtime versions, writes a synthetic NUT if no path is given, opens the file, inspects streams, decodes one video frame when a video stream exists, decodes audio when an audio stream exists, exercises `libswscale` and `libswresample`, frees the allocations, and returns non-zero on any failed step.

Configure command, build tree outside the source directory:

```bash
cmake -S /home/editry/development/sise-editor/tools/validation/ffmpeg_link_smoke_test \
  -B /tmp/aether-ffmpeg-link-smoke-build
```

Result: exit 1. CMake reported that the required packages were not found: `libavutil`, `libavcodec`, `libavformat`, `libswscale`, `libswresample`. Log: `validation-fixtures/ffmpeg-library-inspect-2026-11-20/cmake-configure.log`.

The harness was not compiled. It was not linked. It did not open a file. There is no successful link result to report.

## 6. Verdict

**BLOCKED.** Development headers and pkg-config modules for the approved libraries are missing. The exact missing package name on this system is `ffmpeg-devel`.

Library integration is not demonstrated. The MXF CLI results, including the 24 fps pass and the 29.97 audio failure, remain CLI results.
