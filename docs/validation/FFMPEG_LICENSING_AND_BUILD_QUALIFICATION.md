# FFmpeg licensing and build qualification

Date: 22 November 2026. Machine: Linux `7.2.15-300.fc45.x86_64`. Project: `/home/editry/development/sise-editor`.

Qualification only. FFmpeg was not integrated. Aether production source, CMake, tests, UI, schema, ADRs, roadmaps, builds, and projects were not modified. No package was installed, removed, or upgraded. `sudo` was not used. `make` and `make install` were not run. `/usr/bin/ffmpeg` was not replaced. `docs/research/professional_nle/` and the five existing files in `docs/research/` were not modified. Git was not initialised, and nothing was committed or pushed. Slice 1E implementation and playback were not started.

ADR-001 stays **ACCEPTED WITH CONDITIONS**. ADR-008 stays **OPEN — PENDING LEGAL COUNSEL**. This document does not accept ADR-008 and does not decide the legal questions.

## Existing Environment Audit

Re-checked on this date. The Slice 1E record was not assumed to be current.

`/usr/bin/ffmpeg` is still FFmpeg 9.0.2 from RPM Fusion. The RPM `License` field is `GPLv3+`. The printed configuration still contains `--enable-gpl`, `--enable-version3` four times, `--enable-avfilter`, `--enable-libx264`, `--enable-libx265`, `--enable-libfdk-aac`, and `--enable-decklink`. `--enable-nonfree` is still absent. That absence is not a clearance. `libavfilter.so.12` is still installed. The runtime libraries still directly need libx264, libx265, libfdk-aac, OpenCORE, and VisualOn.

The package release changed during the audit. This qualification did not install or upgrade anything. The first inventory saw `ffmpeg-9.0.2-3.fc45` with no `ffmpeg-devel`, no `/usr/include/ffmpeg`, and no pkg-config modules. A closing check found `ffmpeg-9.0.2-4.fc45` and `ffmpeg-devel-9.0.2-4.fc45` installed at 18:39 IST the same day, with headers and pkg-config modules for versions 61.1.102, 63.1.102, 63.1.102, 10.1.102, and 7.1.102. Those files describe the GPL build, and pkg-config also advertises `libavfilter`. They are not the 9.1 candidate and they do not open the gate. The two libavcodec binaries were replaced as well: the closing SHA-256 values differ from the hashes taken on the release-3 files. Detail is in the environment audit.

Differences found on the re-check, written in full in `/home/editry/development/sise-editor/docs/research/ffmpeg/01_FFMPEG_ENVIRONMENT_AUDIT.md`:

- Upstream’s current stable is FFmpeg 9.1, released 2026-11-20. The installed RPM was not upgraded.
- Two `libavcodec.so.63.1.102` files exist. On both the release-3 and release-4 observations they had the same size as each other and different SHA-256 values. The dynamic linker bound `/usr/bin/ffmpeg` to the freeworld copy because `/etc/ld.so.conf.d/ffmpeg-lib64.conf` adds `/usr/lib64/ffmpeg`.
- Both libavcodec copies directly need `libx264`, `libx265`, `libxvidcore`, `libfdk-aac`, OpenCORE AMR, and VisualOn AMR-WB. The `ffmpeg-libs` copy is not an LGPL-only library.
- `libavformat` directly needs GnuTLS, GMP, smbclient, libdvdnav, and libdvdread. It does not directly need OpenSSL. `libswresample` directly needs libsoxr. `libavutil` directly needs VA-API, VDPAU, libgcrypt, libdrm, libvpl, and OpenCL.
- `nasm`, `yasm`, and `sq` are not installed. No second FFmpeg lives under `/usr/local` or the project tree.

The installed build remains a failed gate for integration. GPL applies to it under the distinction on `https://www.ffmpeg.org/legal.html`.

## FFmpeg Release Selection

9.0.2 was not assumed to be the pin. Compared on 22 November 2026 with the download page.

| | Pin candidate | System / previous point |
| --- | --- | --- |
| Release | FFmpeg 9.1 “Gauss” | FFmpeg 9.0.2 “Lei” |
| Date | 2026-11-20 | 2026-09-18 |
| Archive | `https://ffmpeg.org/releases/ffmpeg-9.1.tar.xz` | `https://ffmpeg.org/releases/ffmpeg-9.0.2.tar.xz` |
| Identity | `RELEASE` and `FF_VERSION` in the archive say `9.1` | Not extracted. The RPM is this version |
| Approved library versions | 61.19.100, 63.24.100, 63.16.100, 10.5.100, 7.6.100 | 61.1.102, 63.1.102, 63.1.102, 10.1.102, 7.1.102 |
| Sonames | `.so.61`, `.so.63`, `.so.63`, `.so.10`, `.so.7` | same majors |
| Signature | OpenPGP `.asc`, key `FCF986EA15E6E293A5644F10B4322F04D67658D8`. Post-quantum `.pq.asc` also published | Not checked. Not the pin |

**Recommended pin: FFmpeg 9.1, the signed xz archive, not a git branch.** The customary tag name `n9.1` was not checked out. Majors match 9.0.2 and minors do not, so headers and libraries must come from the same build. The system libraries must not be mixed with 9.1 headers. The owner has to approve the pin before integration. A later 9.1.x point release is a new pin, not an automatic update.

9.1 was two days old on this date. That is a reason for the owner to look again before a build is integrated. It is not a reason to follow git master.

## Licence Mode Analysis

Read `https://www.ffmpeg.org/legal.html`, `https://ffmpeg.org/doxygen/trunk/md_LICENSE.html`, `https://ffmpeg.org/doxygen/trunk/md_INSTALL.html`, `https://ffmpeg.org/download.html`, and `LICENSE.md` inside the 9.1 archive. The archive file is `LICENSE.md`. The trunk page matches it on the points used here. The release file is the one that governs the pin.

- FFmpeg without the optional GPL parts is LGPL version 2.1 or later.
- `--enable-gpl` applies GPL to the whole build.
- `--enable-version3` is not itself GPL. Without GPL parts the 9.1 script reports LGPL version 3 or later. With GPL parts it reports GPL version 3 or later.
- `--enable-nonfree` produces an unredistributable build and is forbidden in the candidate.

Candidate A is LGPL version 2.1 or later: no GPL, no version 3, no nonfree, no GPL-only external, dynamic. Candidate B is LGPLv3, only if version 3 is required by a dependency or deliberately selected.

The 9.1 `configure` lists were read. The version3 and GPLv3 dependencies are GMP, ARIB B24, liblensfun, libastcenc, OpenCORE AMR, VisualOn AMR-WB, mbedTLS, RK MPI, and smbclient. None of them is required for probe, metadata, picture decode, audio decode, scale, or resample. None is enabled. Version 3 is not required and was not deliberately selected.

**Candidate A is the candidate. Candidate B was not selected.** The configure summary says `License: LGPL version 2.1 or later`.

## External Dependency Audit

Full rows: `/home/editry/development/sise-editor/docs/research/ffmpeg/02_FFMPEG_LICENCE_AND_DEPENDENCY_MATRIX.md`. No external library was cleared by the sentence “FFmpeg is LGPL”.

| Library | Required by the approved use | Decision |
| --- | --- | --- |
| libavutil, libavcodec, libavformat, libswscale, libswresample | Yes. These are the FFmpeg libraries, not extra codecs | INCLUDE, as shared targets of this pin only |
| libavfilter | No. Not approved | EXCLUDE |
| libx264, libx265, libxvid, and the rest of the 9.1 GPL list | No | EXCLUDE. They need `--enable-gpl` |
| libdvdnav, libdvdread | No | EXCLUDE. The script puts them on the GPL list. `LICENSE.md` does not name them. The script is followed |
| libfdk_aac, libfdk_xhe_aac, libmpeghdec, DeckLink | No | EXCLUDE. The 9.1 script marks them nonfree |
| OpenSSL | No | EXCLUDE. `LICENSE.md` and the script disagree. Left off. Unresolved if someone later proposes it |
| OpenCORE, VisualOn | No | EXCLUDE. Apache-2.0. They need `--enable-version3`. Installed licence files are Apache-2.0 |
| smbclient | No | EXCLUDE. GPLv3. Needs both `--enable-gpl` and `--enable-version3` |
| Every other external the 9.1 help text can enable, including hardware | No | EXCLUDE. Own licence text not opened unless the matrix says it was |

Scale is libswscale, not a filter and not libzimg. Resample is libswresample, not libsoxr. SoXR’s own `LICENCE` is LGPL-2.1-or-later and it is still excluded, because it is not required. Future export is planned and is not in the candidate: encoders and muxers are disabled. Built-in decoders for H.264, HEVC, ProRes, DNxHD, AAC, PCM, and the `mov`, `mxf`, and `wav` demuxers stayed enabled. That is not a patent clearance.

zlib is off. Some MOV files use a zlib-compressed header. That limit is recorded. zlib was not enabled, because its licence text was not opened.

## Candidate Configure Specification

The command that was verified:

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

The command does not contain `--enable-gpl` or `--enable-nonfree`. `--enable-version3` is absent because no enabled dependency requires it and Candidate B was not selected. `--disable-autodetect` is a real 9.1 option. Its help text is `disable automatically detected external libraries`, default on. `--disable-x86asm` is present because `nasm` is not installed and was not installed. The prefix is under `/tmp` so the generated files do not name `/usr`. It is not a production prefix.

Every option is classified in `/home/editry/development/sise-editor/docs/research/ffmpeg/03_FFMPEG_REPRODUCIBLE_BUILD_SPECIFICATION.md`.

## Configure-Only Qualification

Configure was run. `make` was not.

| Attempt | Result |
| --- | --- |
| Same flags without `--disable-x86asm` | Exit 1. Message: `nasm not found or too old. Please install/update nasm or use --disable-x86asm for a build without hand-optimized assembly.` Nasm was not installed |
| With `--disable-x86asm`, before encoders and muxers were disabled | Exit 0. Superseded. It still selected LGPL 2.1 or later, and it enabled encoders. It is not the candidate |
| The command in the previous section | Exit 0 |

Out of tree, directory `/tmp/aether-ffmpeg-qualification/build`, source `/tmp/aether-ffmpeg-qualification/src/ffmpeg-9.1`. Stderr of the successful run was empty. `ffbuild/config.log` contains the usual feature-probe failures (`windows.h`, big-endian tests, and similar). Those are how configure detects the host. They are not licence warnings.

Generated `config.h` records:

- `FFMPEG_LICENSE` = `LGPL version 2.1 or later`
- `CONFIG_GPL 0`, `CONFIG_NONFREE 0`, `CONFIG_VERSION3 0`, `CONFIG_GPLV3 0`, `CONFIG_LGPLV3 0`
- `CONFIG_AVUTIL 1`, `CONFIG_AVCODEC 1`, `CONFIG_AVFORMAT 1`, `CONFIG_SWSCALE 1`, `CONFIG_SWRESAMPLE 1`
- `CONFIG_AVFILTER 0`, `CONFIG_AVDEVICE 0`
- Shared yes, static no, network no, strip no, standalone assembly no
- External library list empty
- Programs empty
- Encoders 0, muxers 0, demuxers 359, decoders 482
- Hardware-acceleration list empty

`config.mak` confirms the same five libraries as enabled shared targets and `!CONFIG_LIBX264`, `!CONFIG_LIBFDK_AAC`, `!CONFIG_DECKLINK`, and `!CONFIG_OPENSSL`. Pkg-config files are not produced by configure. They were not generated. The `.so` files were not produced, because `make` was not run.

Exit 0 is not compliance. The licence class matches Candidate A only because the flags and the summary were checked, not because configure returned success.

Signature of the archive that was configured: GnuPG good signature from `FFmpeg release signing key <ffmpeg-devel@ffmpeg.org>`, fingerprint `FCF9 86EA 15E6 E293 A564 4F10 B432 2F04 D676 58D8`, which matches the download page. The key was imported into `/tmp/aether-ffmpeg-qualification/gnupg` only. GnuPG warned that this new keyring does not certify the key with a trusted signature. Local SHA-256 of the archive: `1a4d394ae63d190a526cd052eb4415c156b5c5e2df37efab41a7de0b8fde2433`. The post-quantum signature was downloaded and not verified, because `sq` is not installed and was not installed.

## Dynamic Linking Model

Specified, not implemented. Detail: `/home/editry/development/sise-editor/docs/research/ffmpeg/04_FFMPEG_DYNAMIC_LINKING_AND_DISTRIBUTION.md`.

Linux: five shared libraries, sonames from the 9.1 majors, resolved from the pin and not from the RPM Fusion freeworld path. Windows: DLLs, not a static EXE, no obfuscated rename. macOS: dylibs with a real substitution path. None of those packages was built. `--enable-rpath` was not used.

## Distribution Obligations

The upstream LGPL checklist is the list for counsel. Nothing on it is marked done, because no binary of the pin was distributed. The configure line is recorded so a later binary can be matched to this source. No external LGPL library is compiled into the candidate, so the “repeat the checklist for LAME” item has an empty set. No GPL library is enabled. The IJG credit for the three libjpeg-derived files in libavcodec still applies if executables of libavcodec are later distributed.

Private development, source distribution, and binary distribution are different. Aether being open source does not erase the LGPL duties on a binary that carries the libraries. Apache-2.0 for Aether’s own source does not relicense FFmpeg. Whether those two licences can be shipped together is for counsel.

## Legal Review Questions

Full wording and the status of each open ADR-008 row: `/home/editry/development/sise-editor/docs/research/ffmpeg/05_FFMPEG_LEGAL_REVIEW_PACKAGE.md`.

| Question | Status |
| --- | --- |
| Aether source licence | Requires project-owner decision. Requires legal counsel. Not closed by this work |
| Contributor IP policy | Requires legal counsel. Requires project-owner decision |
| DCO and extra contributor documents | Requires legal counsel. Requires project-owner decision |
| Qt distribution | Requires legal counsel |
| FFmpeg LGPL or GPL for the intended pin | Candidate configure is technically verified as LGPL 2.1 or later. Acceptance is Requires legal counsel and Requires project-owner decision. The system build is technically verified as GPL |
| ProRes patents | Requires legal counsel. Unresolved. Decoder on, encoders off, is not a patent answer |
| DNx patents | Requires legal counsel. Unresolved. Same |
| AAF | Requires legal counsel |
| Third-party obligations | External set of the candidate is technically verified empty. Remaining LGPL and patent duties Require legal counsel. OpenSSL’s classification is Unresolved and the library is excluded |
| Trademarks | FFmpeg spelling and the Bellard trademark are documented by upstream. UI words such as ProRes Require legal counsel |
| Binary packaging | Requires legal counsel. Requires project-owner decision. Checklist documented, not implemented |

## Integration Gate

**Closed.** Owner approval and counsel approval are both outstanding. A configure exit of 0 does not open the gate. Conditions and the list of changes that would keep it closed are in `/home/editry/development/sise-editor/docs/research/ffmpeg/06_FFMPEG_INTEGRATION_GATE.md`.

Slice 1E stays unstarted. Playback stays unstarted. `libavfilter` stays unapproved. GStreamer stays unselected. The CLI stays off the application path.

## Documents Created

- `/home/editry/development/sise-editor/docs/research/ffmpeg/01_FFMPEG_ENVIRONMENT_AUDIT.md`
- `/home/editry/development/sise-editor/docs/research/ffmpeg/02_FFMPEG_LICENCE_AND_DEPENDENCY_MATRIX.md`
- `/home/editry/development/sise-editor/docs/research/ffmpeg/03_FFMPEG_REPRODUCIBLE_BUILD_SPECIFICATION.md`
- `/home/editry/development/sise-editor/docs/research/ffmpeg/04_FFMPEG_DYNAMIC_LINKING_AND_DISTRIBUTION.md`
- `/home/editry/development/sise-editor/docs/research/ffmpeg/05_FFMPEG_LEGAL_REVIEW_PACKAGE.md`
- `/home/editry/development/sise-editor/docs/research/ffmpeg/06_FFMPEG_INTEGRATION_GATE.md`
- `/home/editry/development/sise-editor/docs/validation/FFMPEG_LICENSING_AND_BUILD_QUALIFICATION.md`

No other project document was created or edited.

## Files and System State Changed

Created the seven documents above, and the directory `/home/editry/development/sise-editor/docs/research/ffmpeg/`.

Created `/tmp/aether-ffmpeg-qualification/` and its contents: the 9.1 archive, its OpenPGP signature, the post-quantum signature and certificates, an isolated GnuPG homedir, the extracted source, the out-of-tree build directory (`config.h`, `config_components.h`, `ffbuild/config.mak`, `ffbuild/config.log`), and notes of the configure command and output. The GnuPG key was imported only into that homedir.

Not changed:

- No package install, removal, or upgrade by this qualification. Nasm and `sq` stayed absent. During the audit the machine itself replaced `ffmpeg-9.0.2-3.fc45` with `ffmpeg-9.0.2-4.fc45` and added `ffmpeg-devel`. That replacement was not a command run here. It does not make the system libraries the candidate.
- No `sudo`. No `make`. No `make install`.
- `/usr/bin/ffmpeg` still reports 9.0.2.
- No library copied into `/usr`, `/usr/local`, or the project tree.
- No Aether source, CMake, test, UI, schema, ADR, roadmap, build directory, or project database.
- No edit under `docs/research/professional_nle/` and no edit of the five pre-existing `docs/research/*.md` files.
- No `git init`, commit, or push. The project is still not a git repository.
- ADR-001 and ADR-008 were not edited.

`/tmp/aether-ffmpeg-qualification/` can disappear on reboot. The facts needed to repeat the judgement are in the seven documents. The archive can be downloaded again from the URL above and checked with the same key.

## Final Status

**FFMPEG QUALIFICATION COMPLETE — LGPL-COMPATIBLE CANDIDATE CONFIGURATION DOCUMENTED — CONFIGURE-ONLY VALIDATION PASSED — LEGAL AND OWNER APPROVAL PENDING — NO AETHER PRODUCTION CHANGES.**

The verified configure exited 0. The summary licence is LGPL version 2.1 or later. GPL, nonfree, and version 3 are off. No GPL-only external is enabled. The five approved libraries are shared targets. `libavfilter` is absent. Standalone x86 assembly is off because nasm is not installed. Encoders and muxers are off because export is not in this candidate. None of that is owner approval, counsel approval, or permission to link Aether.
