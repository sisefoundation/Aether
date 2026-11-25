# FFmpeg development configuration

Status: build baseline for the direction accepted in `architecture/decisions/ADR-001-technology-stack.md`. No pin has been built. No flag set is legally approved. ADR-008 stays **OPEN — PENDING LEGAL COUNSEL**.

This file is the build baseline. The integration behaviour stays in `docs/technology/FFMPEG_INTEGRATION_ARCHITECTURE.md`. Distribution notes that predate this baseline stay in `docs/technology/FFMPEG_BUILD_AND_DISTRIBUTION.md` and point here. The link attempt is `docs/validation/FFMPEG_LIBRARY_INTEGRATION_VALIDATION.md`.

## 1. What is not the pin

The Fedora install measured on 2026-11-20 is an exhibit. It is not the approved distribution configuration and it is not redistributable as AETHER’s engine.

| Item | Observed |
| --- | --- |
| OS | Fedora Linux 45, kernel 7.2.15-300.fc45.x86_64 |
| CLI | `/usr/bin/ffmpeg` 9.0.2, built with GCC 16 |
| Packages | `ffmpeg-9.0.2-3.fc45.x86_64`, `ffmpeg-libs-9.0.2-3.fc45.x86_64` |
| Development package | `ffmpeg-devel` is not installed |
| Headers | `/usr/include/ffmpeg` is absent |
| pkg-config | 2.5.1 is installed. Modules `libavutil`, `libavcodec`, `libavformat`, `libswscale`, `libswresample` are not found |
| Unversioned linker names | `/usr/lib64/libavcodec.so` and the matching names for the other approved libraries are absent. Versioned sonames are present |
| Configure flags that matter for counsel | `--enable-gpl`, `--enable-shared`, `--disable-static`, `--enable-version3`, `--enable-libfdk-aac`, `--enable-decklink`, `--enable-libx264`, `--enable-libx265`, plus the rest of the print in `validation-fixtures/ffmpeg-library-inspect-2026-11-20/environment.txt`. The token `--enable-nonfree` is not in that print. That absence is not a legal conclusion |
| Runtime versions reported by the CLI | libavutil 61.1.102, libavcodec 63.1.102, libavformat 63.1.102, libswscale 10.1.102, libswresample 7.1.102. libavfilter 12.1.102 and libavdevice 63.1.102 are also in the CLI and are not in the approved link set |

A program cannot be compiled against this install. The smoke test’s CMake configure failed for that reason. CLI operation does not imply headers.

## 2. Approved link set

Dynamic libraries only, loaded by the AETHER Media Engine and by no editorial module:

- `libavutil`
- `libavcodec`
- `libavformat`
- `libswscale`
- `libswresample`

Not in the set: `libavfilter`, `libavdevice`, and the `ffmpeg` CLI as a runtime dependency of the application. `libavfilter` is reconsidered only when the Effects and Processing architecture requires it. The CLI remains a validation and diagnostic tool.

## 3. Controlled build to evaluate

This is the configuration a later build record must be able to name. It is not a claim that the configuration is legally approved, and it is not an instruction to configure it on this machine during a documentation task.

- A pinned FFmpeg source revision, recorded by version and commit, not by “whatever the distro installed”.
- Shared libraries. Static archives are not the accepted link.
- Development headers installed with that pin, and pkg-config files that resolve the five libraries and no others for the Media Engine target.
- GPL components excluded unless a separate approval says otherwise.
- Non-free components excluded unless counsel approves them.
- External libraries that change the licence or pull a patent licence counsel has not read — including `libfdk-aac`, DeckLink, `libx264`, and `libx265` — excluded unless that approval names them.
- `prores_ks` and `dnxhd` left in or out only after the patent questions in `docs/technology/LEGAL_COUNSEL_QUESTIONS.md`. Their presence in an upstream tree is not permission to ship them.
- The licence configuration of the pin written down next to the flag list: which `--enable` and `--disable` tokens were used, and which licence texts were in that source revision.
- A reproducible validation: the same pin, the same flags, and the harness in `tools/validation/ffmpeg_link_smoke_test/` plus the editorial sample contract in `docs/validation/AUDIO_FRAME_SAMPLE_BOUNDARY_VALIDATION.md`.
- Software decode and encode paths that do not require hardware acceleration. Hardware methods may be detected. Their absence must still leave a software path. The CLI on this machine lists vdpau, cuda, vaapi, qsv, drm, opencl, vulkan, and amf. Listing them is not an AETHER capability.

Version and ABI policy: the pin is the sonames and the pkg-config versions of that build. A distro upgrade does not become the pin. The Media Engine discovers the pinned libraries. It does not accept a newer soname as compatible without a new validation record. Public AETHER headers do not include FFmpeg headers, so an FFmpeg ABI change cannot leak into Timeline, Bin, Project, or the Media Registry.

Runtime discovery, structured errors, threading, ownership, cancellation, and logging are specified as obligations in ADR-001 and in `docs/technology/FFMPEG_INTEGRATION_ARCHITECTURE.md`. They are unimplemented.

## 4. Capability discovery

A muxer name, an encoder name, or a library version is not a capability row. Rows are tuples that AETHER’s own suite has passed on the pin, as `docs/technology/FFMPEG_INTEGRATION_ARCHITECTURE.md` already requires. The Fedora CLI inventory is evidence of what that binary can name. It is filed at `validation-fixtures/ffmpeg-library-inspect-2026-11-20/` (`muxers.txt`, `demuxers.txt`, `decoders.txt`, `encoders.txt`, `hwaccels.txt`). It does not fill a capability row.

Components confirmed present in that CLI listing, and not thereby approved:

- Muxers `mxf`, `mxf_d10`, `mxf_opatom`
- Encoders `prores`, `prores_aw`, `prores_ks`, `prores_ks_vulkan`, `dnxhd`, `pcm_s24le`, `mpeg2video`, `mjpeg`, `aac`, `libfdk_aac`, `libx264`, `libx265`
- Decoders `prores`, `pcm_s24le`, `dnxhd`, and the H.264 and HEVC entries including hardware wrappers

OP-Atom stereo PCM remains **Not supported** as a measured writer on this build (`docs/validation/FFMPEG_MXF_ROUNDTRIP_TEST.md`). The 30000/1001 eight-frame audio contract remains **Failed**.

## 5. Cross-platform

This baseline was written on one Fedora machine. Windows and macOS builds are not claimed. A later record names each platform that was actually configured, linked, and measured.
