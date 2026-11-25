# Slice 1E media technical metadata and source timebase

Date: 2026-11-22. Machine: Fedora, kernel `7.2.15-300.fc45.x86_64`. Preflight ran before any Slice 1E source edit.

The product name on the existing window remains Aether. `kPrototypeVersion` stays `slice-0a`. Schema stays version 2. ADR-001 stays **ACCEPTED WITH CONDITIONS**. ADR-002, ADR-003, and ADR-005 stay accepted. ADR-006 and ADR-007 stay **PROPOSED**. ADR-008 stays **OPEN — PENDING LEGAL COUNSEL**. No ADR file was edited. `docs/research/` was not read for a lock and was not edited.

Playback is not implemented.

## Repository Inspection

Project root: `/home/editry/development/sise-editor`. The tree is not a git repository. No `git init`, commit, or push was performed.

Slice 1D is the last completed editorial slice. Its record is `/home/editry/development/sise-editor/docs/validation/SLICE_1D_TIMELINE_CLIP_PLACEMENT.md`. Its executable is `/home/editry/development/sise-editor/build-slice1d/src/aether`. That build directory was left as it was. `build-slice1e` was not created.

Existing Qt Test groups, counted from the slot declarations in `tests/*_tests.hpp` (each published total includes init and cleanup):

| Group | Cases in source | Slice 1D reported total |
| --- | --- | --- |
| TimingTests | 5 | 7 |
| ProjectTests | 9 | 11 |
| SequenceTests | 8 | 10 |
| TimelineWorkspaceTests | 8 | 10 |
| MediaImportTests | 8 | 10 |
| UiIntegrationTests | 7 | 9 |
| TimelinePlacementTests | 4 | 6 |

Those headers were not modified. No probe test was added. Slice 1E did not run CTest, so the pass counts above are the Slice 1D record plus an unchanged source count, not a new run.

Imported media in the current UI is still labelled `Not probed` (`docs/validation/UI_SHELL_AND_PROJECT_DIALOG.md`, Slice 1D). No representation timing rule and no mixed-rate refusal was changed.

`docs/DEVELOPMENT_ROADMAP.md` is empty and was left empty. `/home/editry/development/sise-editor/DEVELOPMENT_ROADMAP.md` and `/home/editry/development/sise-editor/docs/19_DEVELOPMENT_ROADMAP.md` were not edited. ShellCut, PlaceCut, IntegCut, and existing `project.db` files were not opened for writing. `~/development/sise_cutlist` and Documents/EDITORIAL media were not used.

## FFmpeg Environment and Licensing Preflight

Inspection only. No package was installed. No FFmpeg source was downloaded or built. No library was linked. No file was probed with the CLI or with a library call.

### Runtime binary and packages

| Item | Observed |
| --- | --- |
| CLI | `/usr/bin/ffmpeg` |
| Version | FFmpeg 9.0.2, Copyright 2000-2026, built with GCC 16 |
| CLI package | `ffmpeg-9.0.2-3.fc45.x86_64` |
| Library package | `ffmpeg-libs-9.0.2-3.fc45.x86_64` |
| Companion package | `libavcodec-freeworld-9.0.2-3.fc45.x86_64` |
| Packager / vendor | RPM Fusion |
| RPM `License` field | `GPLv3+` on `ffmpeg`, `ffmpeg-libs`, and `libavcodec-freeworld` |
| pkg-config | `/usr/bin/pkg-config` 2.5.1. `PKG_CONFIG_PATH` empty. Search path `/usr/lib64/pkgconfig:/usr/share/pkgconfig` |

Runtime versions printed by `ffmpeg -version`:

| Library | Version | Soname present | Approved link set in ADR-001 |
| --- | --- | --- | --- |
| libavutil | 61.1.102 | `/lib64/libavutil.so.61` → `libavutil.so.61.1.102`, owned by `ffmpeg-libs` | Yes |
| libavcodec | 63.1.102 | `/lib64/libavcodec.so.63` → `libavcodec.so.63.1.102`, and `/usr/lib64/ffmpeg/libavcodec.so.63` | Yes |
| libavformat | 63.1.102 | `/lib64/libavformat.so.63` → `libavformat.so.63.1.102` | Yes |
| libswscale | 10.1.102 | `/lib64/libswscale.so.10` → `libswscale.so.10.1.102` | Yes |
| libswresample | 7.1.102 | `/lib64/libswresample.so.7` → `libswresample.so.7.1.102` | Yes |
| libavfilter | 12.1.102 | `/lib64/libavfilter.so.12` → `libavfilter.so.12.1.102` | No. Not approved |
| libavdevice | 63.1.102 | printed by the CLI | No. Not in the approved set |

The package name `libavcodec-freeworld` is not an LGPL determination. The RPM licence field on that package is `GPLv3+`, and the binary that uses these libraries was configured with `--enable-gpl`.

### Development files

| Check | Result |
| --- | --- |
| `pkg-config --exists` for `libavutil`, `libavcodec`, `libavformat`, `libswscale`, `libswresample`, `libavfilter` | Each module is missing. No version, cflags, or libs line |
| `libav*.pc` / `libsw*.pc` under `/usr` | None found |
| `avutil.h`, `avcodec.h`, `avformat.h`, `swscale.h`, `swresample.h` under `/usr/include` and `/usr/local/include` | None found |
| `/usr/include/ffmpeg` | Absent |
| Unversioned linker names `libavcodec.so`, `libavformat.so`, `libavutil.so`, `libswscale.so`, `libswresample.so` in `/usr/lib64` and `/lib64` | Absent |
| `rpm -q ffmpeg-devel` | `package ffmpeg-devel is not installed` |
| `rpm -q ffmpeg-free-devel` | `package ffmpeg-free-devel is not installed` |
| `rpm -q libavformat-devel` | `package libavformat-devel is not installed` |

The shared objects are runtime libraries. They are not a compilable development install. The `ffmpeg` binary alone is not a linkable Media Engine.

### Configure flags that decide the gate

Full configuration line printed by `/usr/bin/ffmpeg -version` on 2026-11-22:

```
--prefix=/usr --bindir=/usr/bin --datadir=/usr/share/ffmpeg --docdir=/usr/share/doc/ffmpeg --incdir=/usr/include/ffmpeg --libdir=/usr/lib64 --mandir=/usr/share/man --arch=x86_64 --optflags='-O2 -flto=auto -ffat-lto-objects -fexceptions -g -grecord-gcc-switches -pipe -Wall -Wno-complain-wrong-lang -Werror=format-security -Wp,-U_FORTIFY_SOURCE,-D_FORTIFY_SOURCE=3 -Wp,-D_GLIBCXX_ASSERTIONS -specs=/usr/lib/rpm/redhat/redhat-hardened-cc1 -fstack-protector-strong -specs=/usr/lib/rpm/redhat/redhat-annobin-cc1 -m64 -march=x86-64 -mtune=generic -fasynchronous-unwind-tables -fstack-clash-protection -fcf-protection -mtls-dialect=gnu2 -fno-omit-frame-pointer -mno-omit-leaf-frame-pointer' --extra-ldflags='-Wl,-z,relro -Wl,--as-needed -Wl,-z,pack-relative-relocs -Wl,-z,now -specs=/usr/lib/rpm/redhat/redhat-hardened-ld -specs=/usr/lib/rpm/redhat/redhat-hardened-ld-errors -specs=/usr/lib/rpm/redhat/redhat-annobin-cc1 -Wl,--build-id=sha1 -specs=/usr/lib/rpm/redhat/redhat-package-notes ' --extra-cflags=' -I/usr/include/rav1e -I/usr/include/decklink' --extra-cxxflags=-I/usr/include/decklink --enable-libopencore-amrnb --enable-libopencore-amrwb --enable-libvo-amrwbenc --enable-version3 --enable-bzlib --enable-chromaprint --enable-decklink --enable-fontconfig --enable-frei0r --enable-gcrypt --enable-gmp --enable-version3 --enable-gnutls --enable-ladspa --enable-lcms2 --enable-libaom --enable-libaribb24 --enable-libaribcaption --enable-libdav1d --enable-libass --enable-libbluray --enable-libbs2b --enable-libcaca --enable-libcodec2 --enable-cuda --enable-cuda-llvm --enable-cuvid --enable-libcdio --enable-libdc1394 --enable-libiec61883 --enable-libdrm --enable-libdvdnav --enable-libdvdread --enable-libxeveb --enable-libxevdb --enable-libfdk-aac --enable-libflite --enable-libjack --enable-libjxl --enable-libklvanc --enable-libfreetype --enable-libfribidi --enable-libgme --enable-libgsm --enable-libharfbuzz --enable-libilbc --enable-liblc3 --enable-libmp3lame --enable-libmysofa --enable-nvenc --enable-nvdec --enable-openal --enable-liboapv --enable-opencl --enable-opengl --enable-libopenh264 --enable-libopenjpeg --enable-libopenmpt --enable-libopus --enable-libpulse --enable-libplacebo --enable-librabbitmq --enable-librist --enable-librsvg --enable-librav1e --enable-librubberband --enable-libqrencode --enable-libsmbclient --enable-version3 --enable-libsnappy --enable-libsoxr --enable-libspeex --enable-libsrt --enable-libssh --enable-libsvtav1 --enable-libtesseract --enable-libtheora --enable-libtwolame --enable-libvorbis --enable-libv4l2 --enable-libvidstab --enable-libvmaf --enable-version3 --enable-vapoursynth --enable-libvpx --enable-libvvenc --enable-vulkan --enable-libwebp --enable-libx264 --enable-libx265 --enable-libxvid --enable-libxml2 --enable-libzimg --enable-libzmq --enable-libzvbi --enable-lv2 --enable-avfilter --enable-libmodplug --enable-pthreads --disable-static --enable-shared --enable-gpl --disable-debug --disable-stripping --shlibdir=/usr/lib64 --enable-lto --enable-libvpl --enable-runtime-cpudetect
```

| Token | Present in that line |
| --- | --- |
| `--enable-gpl` | Yes |
| `--enable-version3` | Yes, four times |
| `--enable-nonfree` | No. Absence of the token is not a clearance |
| `--enable-avfilter` | Yes. `libavfilter` is built and installed |
| `--enable-shared` / `--disable-static` | Yes / yes |
| `--enable-libx264` | Yes |
| `--enable-libx265` | Yes |
| `--enable-libfdk-aac` | Yes |
| `--enable-decklink` | Yes |

### Public licence distinction

Read on 2026-11-22 from `https://www.ffmpeg.org/legal.html`. That page says it is not legal advice. The distinction it states:

- FFmpeg is licensed under the GNU Lesser General Public License (LGPL) version 2.1 or later.
- FFmpeg incorporates optional parts and optimizations covered by the GNU General Public License (GPL) version 2 or later. If those parts get used, the GPL applies to all of FFmpeg.
- The LGPL compliance checklist begins: compile FFmpeg without `--enable-gpl` and without `--enable-nonfree`.
- The same checklist says the program must not use GPL libraries, and names `libx264`.

This machine’s build was compiled with `--enable-gpl` and with `--enable-libx264`. Under that public distinction, GPL applies to this build. ADR-008 is **OPEN — PENDING LEGAL COUNSEL**. ADR-001 already records that `ffmpeg-9.0.2-3.fc45` is not the approved distribution configuration. `docs/technology/FFMPEG_DEVELOPMENT_CONFIGURATION.md` requires a separate pinned shared build with GPL components excluded unless separately approved, and with development headers. No such pin exists on this machine.

### Licence gate result

**FAIL. No integration.**

Two independent stops, either of which is enough:

1. Development requirements are not satisfied. Headers, pkg-config modules, unversioned linker names, and `ffmpeg-devel` are absent. A Media Engine cannot be compiled against this install.
2. Licensing requirements are not satisfied. The installed libraries come from a GPL-enabled configuration (`--enable-gpl`, `--enable-version3`, `--enable-avfilter`, `--enable-libx264`, `--enable-libx265`, `--enable-libfdk-aac`). The RPM licence field is `GPLv3+`. ADR-008 is open. A GPL or non-free configuration is not linked while that ADR is open. `--enable-nonfree` is not printed; that does not make the GPL build acceptable.

No second, header-complete, LGPL-only FFmpeg install was found. The gate does not assume LGPL from a package name.

## Probe Architecture

Not implemented. The preflight failed, so no Media Engine boundary, no probe type, and no FFmpeg include were added. UI code was not given a path to `AVFormatContext`, `AVStream`, or codec structs, because no probe call exists. The application still does not call FFmpeg. The `ffmpeg` CLI was not used as an application probe and was not used to invent metadata for this slice.

## Media Metadata Model

Not implemented. No framework-independent probe result type was added. No container name, duration, file size, mtime, stream list, codec, pixel format, sample rate, channel layout, timebase, or provenance label is produced by new code. Unknown was not stored as zero. No metadata was fabricated.

## Source Timebase and Frame-Rate Interpretation

Not implemented. No integer rational was read from a file. No average frame rate, r_frame_rate, or container duration was turned into an editorial frame count. No Verified Frame Rate was labelled. No variable-frame-rate classification was made. Place-cut timing rules and the mixed-rate refusal are unchanged. ADR-002 was not edited.

## Media Representation Integration

Not implemented. No probe was attached to a Media ID or a Representation ID. No registered path was opened. No Media ID or Representation ID was changed. No media file was modified. No `project.db` table was added. No second database was added. Probe results were not given editorial authority, because there are no probe results.

`validation-fixtures/media-probe/` was not created. No disposable project was written.

## Native UI Integration

Not implemented. The placement dialog was not given candidate rates. Saved ranges were not changed. The inspector still shows the existing `Not probed` state until a later authorised probe exists. Online and offline behaviour was not changed. No probing state, success panel, or failure panel was added. The window title contract stays Aether. The designed layout was not edited.

## Asynchronous Execution

Not implemented. No job, `QThread`, or other Qt mechanism was added for a probe. No request identity, stale-result guard, or project-switch guard was added, because no probe request exists. No FFmpeg context or file handle was opened by this slice.

## Live Product Checkpoints

No Slice 1E window was launched. Checkpoints that were not shown:

- not probed
- probing
- success on a real file
- ambiguous timing
- a real failure

No pixels were invented. `validation-fixtures/media-probe/screenshots/` does not exist. Wayland was not started. `wmctrl` was not asked to list a Slice 1E window. The Slice 1D executable was not relaunched for this slice. A real file was not probed in an XCB window.

## Build and Test Results

`build-slice1e` was not configured and was not built. There is no Slice 1E executable. Compiler warnings for this slice: none, because nothing was compiled.

CTest for Slice 1E: not run.

Existing groups were left in source. Probe cases for video, audio, both, multi-stream, unknown rate, conflict, unknown duration, unknown frame count, missing timestamps, malformed, truncated, missing, inaccessible, identity, stale result, and project switch were not added.

The last recorded CTest, from Slice 1D in `build-slice1d`, was 1/1 `slice0a` Passed. That run is not a Slice 1E result. Its Qt totals were TimingTests 7, ProjectTests 11, SequenceTests 10, TimelineWorkspaceTests 10, MediaImportTests 10, UiIntegrationTests 9, TimelinePlacementTests 6. This slice did not repeat that run.

## Actual UI Validation

No GUI click was performed for Slice 1E. No dialog field was observed. No claim is made that a probe panel, a candidate rate, or a failure string was shown.

## Screenshots and Launch Instructions

Executable path: none for Slice 1E.

Launch command: none. The required form, had the gate passed, would have been `QT_QPA_PLATFORM=xcb` with the `build-slice1e` executable. That executable was not produced.

Screenshot paths: none.

The previous slice executable remains `/home/editry/development/sise-editor/build-slice1d/src/aether`. It was not started by this slice and it does not contain a probe.

## Known Limitations

The machine can run `/usr/bin/ffmpeg`. It cannot compile an application against the approved libraries, and the libraries that are installed are a GPL-enabled RPM Fusion build. Slice 1E does not remove that block and does not replace it with a CLI probe.

Still true, and unchanged by this slice:

- Imported media stays `Not probed`.
- The placement dialog does not select a source rate from a file.
- Playback, picture decode, thumbnails, waveforms, proxies, transcode, relink, conform, insert/overwrite editing beyond Slice 1D, and viewers stay out.
- `libavfilter` stays unapproved.
- ADR-008 stays open. This document is an engineering stop. It is not a legal opinion.

## Files Modified

Created:

- `/home/editry/development/sise-editor/docs/validation/SLICE_1E_MEDIA_METADATA_AND_SOURCE_TIMEBASE.md`

No source file, CMake file, test, resource, ADR, roadmap, existing build directory, or user project was modified. `docs/research/` was not modified.

## Roadmap Status

`/home/editry/development/sise-editor/DEVELOPMENT_ROADMAP.md` and `/home/editry/development/sise-editor/docs/19_DEVELOPMENT_ROADMAP.md` were not updated. This slice is blocked before implementation, so it is not a completed slice and not a partial implementation. The empty stub `/home/editry/development/sise-editor/docs/DEVELOPMENT_ROADMAP.md` was not filled. Playback is not marked started. Later slices stay **NOT STARTED** and **NOT AUTHORISED**.

## Final Slice Status

Library versions: libavutil 61.1.102, libavcodec 63.1.102, libavformat 63.1.102, libswscale 10.1.102, libswresample 7.1.102. Also present and not approved: libavfilter 12.1.102, libavdevice 63.1.102. Configure flags that fail the gate: `--enable-gpl`, `--enable-version3`, `--enable-avfilter`, with `--enable-libx264`, `--enable-libx265`, `--enable-libfdk-aac`, and `--enable-decklink`. `--enable-nonfree` is absent from the print.

Licence gate result: **FAIL**.

Qt group counts: unchanged in source; Slice 1E CTest was not run. Last recorded totals remain 7, 11, 10, 10, 10, 9, 6 inside one CTest from Slice 1D.

A real file was not probed in an XCB window.

**SLICE 1E BLOCKED — FFMPEG DEVELOPMENT OR LICENSING REQUIREMENTS NOT SATISFIED — NO UNSAFE INTEGRATION PERFORMED.**

## Continuation — 2026-11-22, development-only FFmpeg 9.1 prefix

The blocked preflight above is unchanged. It records the system FFmpeg 9.0.2 GPL build under `/usr`. That build was not linked.

A later owner authorisation allowed a development-only dynamic link to the separate prefix `/opt/aether-ffmpeg-9.1` (`9.1-aether-lgpl-candidate`, LGPL version 2.1 or later). That link is not distribution approval. ADR-008 stays **OPEN — PENDING LEGAL COUNSEL**. The implementation record, `ldd` lines, tests, and the live XCB screenshots are in `docs/validation/SLICE_1E_FFMPEG_9_1_DEVELOPMENT_INTEGRATION.md`.

**SLICE 1E COMPLETE — FFMPEG 9.1 DEVELOPMENT-ONLY DYNAMIC INTEGRATION AND MEDIA METADATA PROBING VALIDATED — DISTRIBUTION LEGAL REVIEW PENDING.**
