# FFmpeg reproducible build specification

Date: 22 November 2026. This is a specification and a configure-only record. It is not a build. `make` was not run. Nothing was installed.

Owner approval is required before any later integration. ADR-008 stays **OPEN**. A configure exit of 0 is not compliance and does not open the integration gate.

## Release comparison

Read on 22 November 2026 from `https://ffmpeg.org/download.html`. Archive URLs below were checked with HTTP HEAD or GET the same day. 9.0.2 was not assumed to be the pin.

| | FFmpeg 9.1 “Gauss” | FFmpeg 9.0.2 “Lei” |
| --- | --- | --- |
| Status on the download page | Latest stable. 9.1 release branch, cut from master on 2026-10-24 | Latest point on the 9.0 branch, cut from master on 2026-06-26 |
| Release date named by upstream | 2026-11-20 | 2026-09-18 |
| Archive | `https://ffmpeg.org/releases/ffmpeg-9.1.tar.xz` | `https://ffmpeg.org/releases/ffmpeg-9.0.2.tar.xz` |
| HTTP | 200, `content-length` 12,557,532, `last-modified` Fri, 20 Nov 2026 00:16:41 GMT | 200, `content-length` 12,040,788, `last-modified` Fri, 18 Sep 2026 02:30:12 GMT |
| Identity inside the 9.1 archive | `RELEASE` and `FF_VERSION` both contain `9.1`. There is no file named `VERSION`. `LICENSE.md` is the licence file | Not extracted. The system RPM is this version and is a GPL build |
| libavutil | 61.19.100 | 61.1.102 |
| libavcodec | 63.24.100 | 63.1.102 |
| libavformat | 63.16.100 | 63.1.102 |
| libswscale | 10.5.100 | 10.1.102 |
| libswresample | 7.6.100 | 7.1.102 |
| libavfilter (not approved) | 12.8.100 | 12.1.102 |
| libavdevice (not in the approved set) | 63.5.100 | 63.1.102 |
| Sonames of the approved libraries | `.so.61`, `.so.63`, `.so.63`, `.so.10`, `.so.7` | same majors |
| Signature method | OpenPGP signature `.asc`, key fingerprint `FCF9 86EA 15E6 E293 A564 4F10 B432 2F04 D676 58D8`. Optional post-quantum `.pq.asc` | Same published method. Not performed here, because 9.0.2 is not the pin |
| Notes | Two days old on this date. `libpostproc` is not in the 9.1 `LIBRARY_LIST`. Changelog adds features; majors did not change | This is the version of the rejected system RPM. Pinning the same version number would invite mixing those libraries with a new build |

Majors match, so the soname family matches. Minors do not. 9.1 headers must not be compiled against the installed 9.0.2 libraries, and the reverse is also forbidden. Same soname is not permission to mix a GPL system build with an LGPL pin.

FFmpeg’s download page encourages distributors to use a release and encourages individual users toward the development branch. This project pins a release. No unpinned git branch. The customary git tag form is `n9.1`. That tag was not checked out and its git signature was not verified. The pin is the signed release archive whose `RELEASE` file says `9.1`.

## Recommended pin

**FFmpeg 9.1, archive `ffmpeg-9.1.tar.xz`, identity `9.1`.**

Reasons: it is the current official stable on the date of this qualification; the approved library majors are unchanged from 9.0.2; the system 9.0.2 libraries are a different configuration and must not be reused; a named archive can be verified. The age of 9.1 is a reason for the owner to look at the next 9.1.x point release before integration. It is not a reason to float on git master.

This recommendation is not owner approval. No later point release is included by implication. A 9.1.1, if one appears, is a new pin.

## Signature result for 9.1

Downloaded to `/tmp/aether-ffmpeg-qualification/src/`.

- Archive SHA-256, computed locally after download: `1a4d394ae63d190a526cd052eb4415c156b5c5e2df37efab41a7de0b8fde2433`. Upstream does not publish a separate checksum file on the download page. This hash records the bytes that were verified. It is not an upstream checksum.
- Public key: `https://ffmpeg.org/ffmpeg-devel.asc`, imported only into `/tmp/aether-ffmpeg-qualification/gnupg`. The user keyring was not used.
- `gpg --verify ffmpeg-9.1.tar.xz.asc ffmpeg-9.1.tar.xz` exited 0. GnuPG reported a good signature from `FFmpeg release signing key <ffmpeg-devel@ffmpeg.org>`, RSA key `FCF986EA15E6E293A5644F10B4322F04D67658D8`, signature made Fri 20 Nov 2026 05:48:20 AM IST. That fingerprint matches the key published on `https://ffmpeg.org/download.html`.
- GnuPG also warned that the key is not certified with a trusted signature in that new keyring. The check is “this archive matches the key published by ffmpeg.org”. It is not a local owner trust decision.
- `ffmpeg-9.1.tar.xz.pq.asc` and the two upstream post-quantum certificates were downloaded. `sq` is not installed. The post-quantum signatures were not verified. `sq` was not installed.

## Licence mode chosen for the pin

Candidate A: **LGPL version 2.1 or later**.

`--enable-version3` is not in the command. The 9.1 `configure` source was read. Nothing enabled by this command is in `EXTERNAL_LIBRARY_VERSION3_LIST` or `EXTERNAL_LIBRARY_GPLV3_LIST`. Version 3 was not required by a dependency and was not deliberately selected. Candidate B (LGPLv3) was evaluated and not chosen. Detail: `02_FFMPEG_LICENCE_AND_DEPENDENCY_MATRIX.md`.

The command does not contain `--enable-gpl` or `--enable-nonfree`. It passes the disable forms so the generated `config.h` records them.

## `--disable-autodetect` on this release

`./configure --help` from the 9.1 source says:

```
--disable-autodetect     disable automatically detected external libraries [no]
```

The default is autodetect on. The 9.1 `configure` script, when autodetect is disabled, disables `EXTERNAL_AUTODETECT_LIBRARY_LIST` and `HWACCEL_AUTODETECT_LIBRARY_LIST`. Those lists include zlib, bzlib, lzma, libxcb, sdl2, and the hardware-acceleration probes (vaapi, vdpau, vulkan, nvenc, nvdec, and the rest of that list). Libraries whose help text says `[no]`, including libx264, stay off unless an explicit `--enable` is passed. Autodetect-off does not by itself disable libc iconv: the script still probes it unless `--disable-iconv` is passed. This command passes `--disable-iconv`.

`INSTALL.md` in the same archive says non-system dependencies such as libx264 and libvpx are disabled by default, and that `configure` may be launched from another directory. This run is out of tree.

## Exact configure command

This is the command that was run for the qualification result. Working directory: `/tmp/aether-ffmpeg-qualification/build`. The configure program was the absolute path below.

```
/tmp/aether-ffmpeg-qualification/src/ffmpeg-9.1/configure \
  --prefix=/tmp/aether-ffmpeg-qualification/prefix \
  --disable-static \
  --enable-shared \
  --disable-autodetect \
  --disable-gpl \
  --disable-version3 \
  --disable-nonfree \
  --disable-programs \
  --disable-doc \
  --disable-avdevice \
  --disable-avfilter \
  --disable-network \
  --disable-x86asm \
  --disable-encoders \
  --disable-muxers \
  --disable-hwaccels \
  --disable-filters \
  --disable-devices \
  --disable-unstable \
  --disable-stripping \
  --disable-iconv \
  --disable-avisynth \
  --disable-frei0r \
  --disable-libcdio \
  --disable-libdavs2 \
  --disable-libdvdnav \
  --disable-libdvdread \
  --disable-librubberband \
  --disable-libvidstab \
  --disable-libx264 \
  --disable-libx265 \
  --disable-libxavs \
  --disable-libxavs2 \
  --disable-libxvid \
  --disable-decklink \
  --disable-libfdk-aac \
  --disable-libfdk-xhe-aac \
  --disable-libmpeghdec \
  --disable-gmp \
  --disable-libaribb24 \
  --disable-libastcenc \
  --disable-liblensfun \
  --disable-libopencore-amrnb \
  --disable-libopencore-amrwb \
  --disable-libvo-amrwbenc \
  --disable-mbedtls \
  --disable-rkmpp \
  --disable-libsmbclient \
  --disable-openssl \
  --disable-gnutls \
  --disable-libtls \
  --disable-gcrypt \
  --extra-version=aether-lgpl-candidate
```

`--prefix` points at a directory under `/tmp` so the generated files do not name `/usr`. It is the qualification prefix. It is not a production install path. A production prefix is an owner decision and was not chosen. Do not run `make install` against this prefix as part of integration. Integration has not been approved.

`--disable-x86asm` is in the command because `nasm` is not installed. A first configure, without that flag, exited 1 and printed: `nasm not found or too old. Please install/update nasm or use --disable-x86asm for a build without hand-optimized assembly.` Nasm was not installed. Inline compiler SIMD flags can still show as enabled in the summary. Standalone assembly is off (`standalone assembly no` in the summary, `!HAVE_X86ASM=yes` in `config.mak`). Dropping `--disable-x86asm` later requires nasm to be present and requires a new configure record. It is not a licence change by itself, and it was not tested here.

## Classification of options

| Option | Class | Why it is in the command |
| --- | --- | --- |
| `--prefix=/tmp/aether-ffmpeg-qualification/prefix` | Required for this run | Keeps the generated prefix off `/usr`. Not a production path |
| `--disable-static` | Required | ADR-001 requires dynamic linking. Static is the 9.1 default |
| `--enable-shared` | Required | Shared libraries are off by default in 9.1 |
| `--disable-autodetect` | Required | Stops the host’s RPM libraries and hardware probes from entering the build |
| `--disable-gpl` | Licensing-sensitive | Keeps the build off GPL. Must not be replaced by `--enable-gpl` |
| `--disable-version3` | Licensing-sensitive | Candidate A. No enabled dependency requires version 3. See below |
| `--disable-nonfree` | Licensing-sensitive | A nonfree build is unredistributable and is forbidden in the candidate |
| `--disable-programs` | Required | `ffmpeg`, `ffprobe`, and `ffplay` are not the Media Engine. Libraries are enough |
| `--disable-doc` | Recommended | Documentation tools are not the libraries. `makeinfo` is absent and was not required |
| `--disable-avdevice` | Required | Not in the approved link set. Also the library that carries DeckLink |
| `--disable-avfilter` | Required | Not approved. GPL filters live here |
| `--disable-network` | Recommended | Approved use is local files. Avoids a TLS library decision |
| `--disable-x86asm` | Required on this machine | Nasm is absent and was not installed. Not a licence flag |
| `--disable-encoders` | Required | No encoder has a documented requirement. Future export is planned, not implemented |
| `--disable-muxers` | Required | Same reason. Probe and decode do not write a container |
| `--disable-hwaccels` | Recommended | ADR-001 requires a software path. Hardware is not part of this candidate |
| `--disable-filters` | Required | Consistent with libavfilter being absent |
| `--disable-devices` | Required | Consistent with libavdevice being absent |
| `--disable-unstable` | Recommended | Experimental components are not a documented requirement. The decoders checked after configure (h264, hevc, prores, dnxhd, aac, pcm, mxf, mov, wav) remained enabled |
| `--disable-stripping` | Recommended | Development configuration. Symbols stay available for a later debug build |
| `--disable-iconv` | Optional | Libc iconv is not required for timing metadata. Disabling it leaves the external-library list empty |
| `--disable-avisynth` through `--disable-gcrypt` | Licensing-sensitive | Explicit exclusions for GPL, nonfree, version3, and TLS libraries. Default-off is not a substitute for a line the owner can read |
| `--extra-version=aether-lgpl-candidate` | Recommended | Makes a later binary string distinguishable from the system 9.0.2 CLI |

`--enable-version3` is absent. It would be present only if a library in `EXTERNAL_LIBRARY_VERSION3_LIST` or `EXTERNAL_LIBRARY_GPLV3_LIST` were enabled, or if the owner deliberately selected LGPLv3. Neither is true. With GPL parts also off, `--enable-version3` would mean LGPLv3, not GPL. That fact was checked and was not a reason to add the flag.

Not passed, and forbidden in this candidate: `--enable-gpl`, `--enable-nonfree`, `--enable-version3`, `--enable-libx264`, `--enable-libx265`, `--enable-libxvid`, `--enable-libfdk-aac`, `--enable-libfdk-xhe-aac`, `--enable-openssl`, `--enable-libopencore-amrnb`, `--enable-libopencore-amrwb`, `--enable-libvo-amrwbenc`, `--enable-decklink`, `--enable-libavfilter` (the positive form is not the 9.1 flag; the library is disabled with `--disable-avfilter`).

Considered and left out:

| Option | Class | Disposition |
| --- | --- | --- |
| `--enable-zlib` | Optional | Would restore zlib-compressed MOV metadata. `zlib-ng-compat-2.3.3-6.fc45` is installed. Its RPM licence field is `Zlib`. The licence text was not in the package file list this pass opened. Left off. Not a licence failure of the candidate |
| `--enable-libsoxr` | Optional | Installed soxr 0.1.3 is LGPL-2.1-or-later in its `LICENCE` file. `libswresample` is the approved resampler. SoXR is not required |
| `--enable-libdav1d`, `--enable-libvpx`, and other permissive decoders | Optional | Native decoders for AV1, VP8, and VP9 are already in the LGPL build. No external decoder was required |
| `--disable-everything` plus a hand-picked decoder list | Not selected | ADR-001 does not adopt a closed codec list. Built-in LGPL decoders and demuxers stay. External codecs stay off |
| `--enable-rpath` | Excluded | The help text says to use rpath with care. A replaceable library is the LGPL model. Rpath was not enabled |

## What configure is allowed to produce

Shared targets, if a later approved `make` is run: `libavutil`, `libavcodec`, `libavformat`, `libswscale`, `libswresample`. Not `libavfilter`. Not `libavdevice`. Not the CLI programs. Not static archives. This qualification did not run that `make`, so those `.so` files do not exist yet. `config.mak` records the targets. That is the configure result, not a built library.
