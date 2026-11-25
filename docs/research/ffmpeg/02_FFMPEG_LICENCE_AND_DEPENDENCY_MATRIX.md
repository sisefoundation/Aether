# FFmpeg licence mode and external dependency matrix

Date: 22 November 2026. Sources for this file:

- `https://www.ffmpeg.org/legal.html`, read 22 November 2026. That page says it is not legal advice.
- `https://ffmpeg.org/doxygen/trunk/md_LICENSE.html`, read the same day, then compared with the release file below. The trunk page is not the pin.
- `LICENSE.md` inside the signed FFmpeg 9.1 archive. That is the licence file of the release under evaluation. The archive names it `LICENSE.md`, not `LICENSE`.
- `configure` and `./configure --help` from that same archive.
- Licence files on this machine for the libraries named in the assignment, where those packages are installed. Those files describe the installed versions. They are not enabled in the candidate.

This file does not clear a library by saying “FFmpeg is LGPL”. ADR-008 stays **OPEN**. Nothing here is a legal opinion.

## How the 9.1 licence file and configure script classify a build

From `LICENSE.md` of FFmpeg 9.1:

- Most of FFmpeg is LGPL version 2.1 or later. Some files are MIT, X11, or BSD. In combination, LGPL v2.1 or later applies.
- Optional parts are GPL version 2 or later. They are off unless `--enable-gpl` is passed. If they are used, the licence of FFmpeg becomes GPL v2 or later. The GPL parts named in the file are specific x86 optimisations, some build and test tools, and a list of libavfilter filters. `vf_lensfun.c` is GPL version 3 or later.
- `--enable-version3` upgrades the chosen (L)GPL to version 3. If GPL parts are also on, the text to read is `COPYING.GPLv3`. If they are off, it is `COPYING.LGPLv3`.
- Three libjpeg files in libavcodec (`jfdctfst.c`, `jfdctint_template.c`, `jrevdct.c`) carry an additional credit duty if only executables are distributed. `tests/reference.pnm` is Expat and is a test file, not a library.

The 9.1 `configure` script sets the summary string as follows: nonfree if `--enable-nonfree`; else GPL version 3 or later if version 3 and GPL; else LGPL version 3 or later if version 3 without GPL; else GPL version 2 or later if GPL; else **LGPL version 2.1 or later**.

`--enable-gpl` puts the build on GPL. `--enable-version3` is not itself GPL. `--enable-nonfree` makes the result unredistributable, in the words of `./configure --help`.

## Candidate A and Candidate B

Approved Aether use, from ADR-001 and the Slice 1E scope: probe, video metadata, audio metadata, picture decode, audio decode, scale, and resample, inside a future Media Engine. Future export is planned and is not implemented. `libavfilter` is not approved. The CLI is not the integration. No external codec has a documented requirement for those uses. Built-in LGPL decoders and demuxers are the decode and probe implementation. Encoders and muxers are disabled in the candidate so an export path is not configured early.

| | Candidate A | Candidate B |
| --- | --- | --- |
| Result if the flags match the 9.1 script | LGPL version 2.1 or later | LGPL version 3 or later |
| `--enable-gpl` | No | No |
| `--enable-nonfree` | No | No |
| `--enable-version3` | No, unless a verified dependency requires it | Only when a dependency requires it, or when deliberately selected |
| GPL-only externals | No | No |
| Linkage | Dynamic | Dynamic |

Checked against 9.1 `configure`, not against a shorter flag list. The version3 dependencies in this release are `gmp`, `libaribb24`, `libastcenc`, `liblensfun`, `libopencore_amrnb`, `libopencore_amrwb`, `libvo_amrwbenc`, `mbedtls`, `rkmpp`, and `libsmbclient` (smbclient also needs GPL, so the combination is GPLv3). None of those is required by the approved use. None is enabled. Version 3 is therefore not required. It was not deliberately selected.

**Candidate A is the candidate. Candidate B was not selected.**

The configure summary of the command that was run says `License: LGPL version 2.1 or later`. `config.h` has `CONFIG_GPL 0`, `CONFIG_NONFREE 0`, `CONFIG_VERSION3 0`, `CONFIG_GPLV3 0`, and `CONFIG_LGPLV3 0`.

## Use matrix

Future export is a planned row. It is not implemented and it is not enabled.

| Use | FFmpeg pieces | External library | In this candidate |
| --- | --- | --- | --- |
| Probe | libavformat, libavcodec, libavutil | none | Yes, as configure targets |
| Video metadata | same | none | Yes |
| Audio metadata | same | none | Yes |
| Picture decode | libavcodec, libavutil, built-in decoders | none | Yes. Encoders off |
| Audio decode | libavcodec, libavutil, built-in decoders | none | Yes |
| Scale | libswscale, libavutil | none. Not libzimg. Not libavfilter | Yes |
| Resample | libswresample, libavutil | none. Not libsoxr | Yes |
| Future export | would need encoders and muxers | none selected | No. `--disable-encoders` and `--disable-muxers` |

Built-in decoders confirmed still enabled after `--disable-unstable`: `h264`, `hevc`, `prores`, `dnxhd`, `aac`, `mp3`, `pcm_s24le`, `pcm_s16le`, `flac`, `vorbis`, `opus`, `dvvideo`, `mjpeg`, `vp9`, `av1`. Demuxers confirmed: `mov`, `mxf`, `wav`, `aiff`, `mp3`, `mpegts`, `avi`, `flac`, `matroska`. Encoder count in the summary: 0. Muxer count: 0. This does not answer any patent question about those decoders.

A functional limit of leaving zlib off: some MOV files store a zlib-compressed header. That case was not probed. zlib is Optional, not part of the candidate, and its licence text was not opened. See the autodetect table.

## Named libraries

Decision values are for this candidate only.

| Name | Version on this machine | Purpose | Licence, primary source | Required | Autodetect | Needs version3 | Needs gpl | Incompatible / nonfree | Notice | Decision |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| libx264 | `x264-libs-0.165-10.20250608gitb35605ac.fc45` | H.264 encoder | Installed `/usr/share/doc/x264/COPYING` is the GNU GPL version 2 text. RPM field: `GPL-2.0-or-later and Khronos`. 9.1 `LICENSE.md` lists libx264 under GPL version 2. `configure` puts it in `EXTERNAL_LIBRARY_GPL_LIST` | No. Native H.264 decoder exists. Export is not implemented | No. Default off | No | Yes | GPL external. Upstream legal checklist names libx264 as a library the program must not use for the LGPL path | Do not clear it because FFmpeg can be LGPL | EXCLUDE |
| libx265 | `x265-libs-4.3-1.fc45` | HEVC encoder | Installed `COPYING` is the GNU GPL version 2 text. RPM field: `GPLv2+ and BSD`. 9.1 `LICENSE.md` lists libx265 under GPL version 2. `configure` requires `--enable-gpl` | No. Native HEVC decoder exists | No | No | Yes | GPL external | The BSD portion claimed by the RPM was not a separate file in the COPYING that was opened | EXCLUDE |
| libfdk_aac | `fdk-aac-free-2.0.3-3.fc45`, soname `libfdk-aac.so.2` | AAC encode and decode | Installed `NOTICE` is “Software License for The Fraunhofer FDK AAC Codec Library for Android”. It is not the LGPL and not the GPL. It states no patent licence. 9.1 `LICENSE.md` says FDK AAC is incompatible with GPLv2 and GPLv3 and, to the best of upstream’s knowledge, compatible with the LGPL. The same page says enabling such libraries requires `--enable-nonfree`, which makes the binary unredistributable. 9.1 `configure` puts `libfdk_aac` in `EXTERNAL_LIBRARY_NONFREE_LIST` | No. Native AAC decoder exists | No | No | No | Yes. The candidate must not pass `--enable-nonfree` | The package name `fdk-aac-free` is not an LGPL finding | EXCLUDE |
| libfdk_xhe_aac | Not separately queried. Named by 9.1 configure | xHE-AAC encoder. New in the 9.1 changelog | Not given its own paragraph in `LICENSE.md`. `configure` puts it in `EXTERNAL_LIBRARY_NONFREE_LIST` with libfdk_aac | No | No | No | No | Yes, by the configure nonfree list | Same nonfree rule | EXCLUDE |
| OpenSSL | `openssl-libs-4.0.3-1.fc45`, soname `libssl.so.4` | TLS | Installed `LICENSE.txt` is Apache License 2.0. 9.1 `LICENSE.md` groups OpenSSL with FDK AAC: incompatible with GPLv2 and GPLv3, and to the best of upstream’s knowledge compatible with the LGPL, and tells the reader to pass `--enable-nonfree` for incompatible libraries. 9.1 `configure` does **not** put `openssl` in `EXTERNAL_LIBRARY_NONFREE_LIST`. For OpenSSL 3.0 or newer the script requires `--enable-version3` only when GPL is also on. With GPL off, that die does not fire | No. Local files do not need TLS | No | Not when GPL is off, according to the script. The `LICENSE.md` wording does not say that | No | Disagreement between `LICENSE.md` and `configure`. Unresolved as a licence classification | Left disabled. The disagreement is not a reason to enable it | EXCLUDE |
| OpenCORE AMR-NB and AMR-WB | `opencore-amr-0.1.6-11.fc45` | AMR decode and AMR-NB encode | Installed `LICENSE` is Apache License 2.0. `NOTICE` names PacketVideo, 2009. 9.1 `LICENSE.md`: OpenCORE is Apache-2.0, incompatible with LGPLv2.1 and GPLv2, and requires `--enable-version3`. `configure` lists both libraries in `EXTERNAL_LIBRARY_VERSION3_LIST` | No | No | Yes | No | Not GPL. Not compatible with Candidate A | Apache-2.0 is why version 3 would be required. Candidate A does not take that upgrade | EXCLUDE |
| VisualOn AMR-WB encoder | `vo-amrwbenc-0.1.3-25.fc45` | AMR-WB encode | Installed `COPYING` is Apache License 2.0. `NOTICE` says the files are copyright 2003-2010 VisualOn and released under Apache-2.0. Same `LICENSE.md` rule as OpenCORE. `configure` lists `libvo_amrwbenc` in the version3 list | No | No | Yes | No | Same as OpenCORE | Same | EXCLUDE |
| DeckLink | Headers were on the include path of the system RPM (`/usr/include/decklink` in that build’s flags). SDK version not re-queried | Capture and output device | Not named in the `LICENSE.md` incompatible paragraph. 9.1 `configure` puts `decklink` in `EXTERNAL_LIBRARY_NONFREE_LIST`. The installed 9.0.2 binary was built with `--enable-decklink` and without a printed `--enable-nonfree` | No. ADR-001 leaves DeckLink off | No | No | No | Yes, in the 9.1 script | Do not treat the 9.0.2 flag combination as a nonfree clearance | EXCLUDE |

## Libraries the 9.1 licence file puts on GPL version 2

`configure` requires `--enable-gpl` for each. All are EXCLUDE. None is autodetected (default off). None is required. Needs version3: no, except where noted.

| Name | Also in configure GPL list | Version seen here | Notes |
| --- | --- | --- | --- |
| avisynth | Yes | Not installed for this audit | Script demuxer |
| frei0r | Yes | Not opened | Video filter. Also requires libavfilter, which is off |
| libcdio | Yes | Not opened. The system ffmpeg links `libcdio_paranoia` and `libcdio_cdda` through libavdevice | CD grabbing. libavdevice is off |
| libdavs2 | Yes | Not opened | AVS2 decoder |
| librubberband | Yes | System libavfilter links `librubberband.so.3`. Package version not opened | Filter. libavfilter is off |
| libvidstab | Yes | System libavfilter links `libvidstab.so.1.2` | Filter |
| libx264 | Yes | See named table | |
| libx265 | Yes | See named table | |
| libxavs | Yes | Not opened | AVS encoder |
| libxavs2 | Yes | Not opened | AVS2 encoder |
| libxvid | Yes | System libavcodec needs `libxvidcore.so.4`. Package text not opened | MPEG-4 encoder. Native MPEG-4 encoder exists and is still disabled by `--disable-encoders` |

`configure` also puts **libdvdnav** and **libdvdread** in `EXTERNAL_LIBRARY_GPL_LIST`. The GPL version 2 bullet list in `LICENSE.md` does not name them. The script is the stricter source: both need `--enable-gpl`. The system libavformat needs both. They are not required. EXCLUDE. This difference between the licence file and the script is recorded and is not resolved by ignoring either one.

## Libraries that need version 3

From `LICENSE.md`, LGPL version 3: `gmp`, `libaribb24`, `liblensfun`. Apache-2.0, and therefore version 3 if combined with FFmpeg: mbedTLS, RK MPI, OpenCORE, VisualOn, libastcenc. `configure` matches that set and spells the OpenCORE and VisualOn libraries as `libopencore_amrnb`, `libopencore_amrwb`, and `libvo_amrwbenc`.

| Name | Version seen | Required | Decision |
| --- | --- | --- | --- |
| gmp | `gmp-6.3.0-6.fc45`. RPM field is a combination of LGPL-3.0-or-later, GPL-2.0-or-later, and GFDL. System libavformat needs `libgmp.so.10` | No | EXCLUDE |
| libaribb24 | RPM name `libaribb24` is not installed. System libavcodec still needs `libaribb24.so.0` | No | EXCLUDE |
| liblensfun | Not opened | No | EXCLUDE |
| libastcenc | Not opened. 9.1 changelog adds ASTC via this library | No | EXCLUDE |
| mbedtls | Not opened | No | EXCLUDE |
| rkmpp | Not opened. Rockchip. Not this host | No | EXCLUDE |
| OpenCORE, VisualOn | See named table | No | EXCLUDE |

Enabling any of these would select Candidate B, or would fail configure under Candidate A. They were not enabled.

## GPL version 3

`libsmbclient`. `LICENSE.md`: GPL version 3, so both `--enable-gpl` and `--enable-version3`. `configure`: `EXTERNAL_LIBRARY_GPLV3_LIST`. Installed package `libsmbclient-4.25.1-1.fc45`. System libavformat needs `libsmbclient.so.0`. Network and Samba are not an approved use. EXCLUDE. The result of enabling it would be GPLv3, not LGPLv3.

## Nonfree list in 9.1 configure

`decklink`, `libfdk_aac`, `libfdk_xhe_aac`, `libmpeghdec`. Also `HWACCEL_LIBRARY_NONFREE_LIST`: `cuda_nvcc`, `cuda_sdk`. All EXCLUDE. `libmpeghdec` is MPEG-H 3D Audio decoding. No documented requirement. Its own licence text was not opened. The script already refuses it without `--enable-nonfree`.

`openssl` is discussed in the named table because `LICENSE.md` treats it as incompatible and the script does not list it as nonfree. EXCLUDE.

## Other external libraries the 9.1 help text can enable

These are in `EXTERNAL_LIBRARY_LIST` and are not in the GPL, nonfree, or version3 lists. Default in the help text is `[no]` unless noted. None is required by the approved use. None is enabled. The decision is EXCLUDE.

Their own licence texts were not opened in this qualification, except libsoxr and the named libraries above. Absence from the GPL list is not a positive identification of the library’s licence. Enabling any of them later requires reading that library’s licence and a new configure record.

`cairo`, `chromaprint`, `gcrypt`, `gnutls`, `jni`, `ladspa`, `lcms2`, `libaom`, `libaribcaption`, `libass`, `libbluray`, `libbs2b`, `libcaca`, `libcodec2`, `libdav1d`, `libdc1394`, `libflite`, `libfontconfig`, `libfreetype`, `libfribidi`, `libharfbuzz`, `libgme`, `libgsm`, `libiec61883`, `libilbc`, `libjack`, `libjxl`, `libklvanc`, `libkvazaar`, `liblc3`, `liblcevc_dec`, `libmodplug`, `libmp3lame`, `libmysofa`, `liboapv`, `libonnxruntime`, `libopencv`, `libopencolorio`, `libopenh264`, `libopenjpeg`, `libopenmpt`, `libopenvino`, `libopus`, `libplacebo`, `libpulse`, `libqrencode`, `libquirc`, `librabbitmq`, `librav1e`, `librist`, `librsvg`, `librtmp`, `libshine`, `libsnappy`, `libsoxr`, `libspeex`, `libsrt`, `libssh`, `libsvtav1`, `libsvtjpegxs`, `libtensorflow`, `libtesseract`, `libtheora`, `libtls`, `libtorch`, `libtwolame`, `libuavs3d`, `libv4l2`, `libvmaf`, `libvorbis`, `libvpx`, `libvvenc`, `libwebp`, `libxevd`, `libxevdb`, `libxeve`, `libxeveb`, `libxml2`, `libzimg`, `libzmq`, `libzvbi`, `lv2`, `mediacodec`, `ohcodec`, `openal`, `openssl`, `pocketsphinx`, `vapoursynth`, `vulkan_static`, `whisper`.

Hardware libraries in `HWACCEL_LIBRARY_LIST` that are not in the nonfree hardware list: `libmfx`, `libvpl`, `mmal`, `opencl`. EXCLUDE. Not the software path.

Additional notes where this machine or the script says more:

| Name | Fact used | Decision |
| --- | --- | --- |
| gnutls | Installed `gnutls-3.8.13-3.fc45`. RPM field: `GPL-3.0-or-later AND LGPL-2.1-or-later`. System libavformat needs it. The dual field was not resolved by reading the GnuTLS licence text. Not required | EXCLUDE |
| libsoxr | Installed `soxr-0.1.3-22.fc45`. `/usr/share/licenses/soxr/LICENCE` grants LGPL-2.1-or-later. System libswresample needs it. The candidate uses libswresample’s own resampler | EXCLUDE |
| libzvbi | Not in the GPL list. `configure` dies unless the header reports version 0.2.28 or newer, or `--enable-gpl` is on. System libavcodec needs `libzvbi.so.0`. Version not read from the header | EXCLUDE |
| libaom | Installed `libaom-3.15.2-1.fc45`, RPM field `BSD-3-Clause`. Native AV1 decoder remained enabled. External library not required | EXCLUDE |
| libvpx | Installed `libvpx-1.17.0-1.fc45`, RPM field `BSD-3-Clause`. Native VP8 and VP9 decoders remained enabled | EXCLUDE |
| libass | Installed `libass-0.17.4-3.fc45`, RPM field `ISC`. Subtitle rendering is not the approved use | EXCLUDE |
| lame | Installed `lame-libs-4.0-1.fc45`, RPM field `LGPL-2.0-or-later AND LGPL-2.1-or-later`. MP3 encoding is an encoder. Encoders are off. Upstream’s LGPL checklist uses LAME as the example of an LGPL library compiled into FFmpeg, which then inherits the checklist. Not compiled in | EXCLUDE |
| libxml2 | Installed `libxml2-2.15.4-1.fc45`, RPM field `MIT AND ISC-Veillard AND W3C`. Needed by FFmpeg for DASH and IMF when enabled. Not required for the local probe set | EXCLUDE |

## Autodetect list, all disabled

`--disable-autodetect` turns these off. They are not enabled by name. Decision: EXCLUDE. Needs gpl / version3 / nonfree: no, on the strength of the 9.1 lists. Own licence texts were not opened except where stated.

`alsa`, `appkit`, `avfoundation`, `bzlib`, `coreimage`, `libcurl`, `libxcb`, `libxcb_shm`, `libxcb_shape`, `libxcb_xfixes`, `lzma`, `mediafoundation`, `metal`, `schannel`, `sdl2`, `securetransport`, `sndio`, `xlib`, `zlib`.

Hardware autodetect, also off: `amf`, `audiotoolbox`, `cuda`, `cuda_llvm`, `cuvid`, `d3d11va`, `d3d12va`, `dxva2`, `ffnvcodec`, `libdrm`, `nvdec`, `nvenc`, `vaapi`, `vdpau`, `videotoolbox`, `vulkan`, `v4l2_m2m`.

| Name | Version seen | Note |
| --- | --- | --- |
| zlib | `zlib-ng-compat-2.3.3-6.fc45` provides `libz.so.1`. RPM field `Zlib`. Licence text was not in the file list this pass opened | Optional later, for compressed MOV metadata. Not in the candidate |
| bzlib | `bzip2-libs-1.0.8-24.fc45`. RPM field `BSD-4-Clause` | EXCLUDE |
| lzma | `xz-libs-5.8.3-2.fc45`. RPM field `0BSD` | EXCLUDE |
| iconv | Libc. Explicitly disabled with `--disable-iconv` so it does not remain as the only external | EXCLUDE |

## What “INCLUDE” means in this candidate

The included pieces are the five FFmpeg libraries themselves, built from the 9.1 LGPL sources with no external library enabled: libavutil, libavcodec, libavformat, libswscale, libswresample. They are shared targets in `config.mak`. They were not compiled, because `make` was not run.

No external library is INCLUDE.

## IJG notice

`LICENSE.md` says the three libjpeg-derived files in libavcodec require an IJG credit in the documentation if only executables are distributed, and a note of any changes to those files. Those files are part of libavcodec even when GPL and external libraries are off. The credit is a distribution notice. It is not a reason to enable GPL. Whether the wording is satisfied is for counsel when a binary is actually distributed. No binary of this pin was produced.
