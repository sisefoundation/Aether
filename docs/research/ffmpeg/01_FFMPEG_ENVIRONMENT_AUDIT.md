# FFmpeg environment audit

Date of this re-check: 22 November 2026. Machine: Linux `7.2.15-300.fc45.x86_64`. Project root: `/home/editry/development/sise-editor`. The tree is not a git repository.

Inspection only. No package was installed, removed, or upgraded. `sudo` was not used. `make` and `make install` were not run. `/usr/bin/ffmpeg` was not replaced. No library was copied into a system path.

This file records the installed FFmpeg. It is not the candidate pin. The pin is FFmpeg 9.1, documented in `03_FFMPEG_REPRODUCIBLE_BUILD_SPECIFICATION.md`.

## What was re-checked

Commands used: `command -v`, `ffmpeg -version`, `ffmpeg -buildconf`, `rpm -qf`, `rpm -qi`, `rpm -q`, `rpm -ql`, `pkg-config`, `ldconfig -p`, `ldd`, `readelf -d`, `sha256sum`, `find`.

## The installed package changed during this audit

The first inventory in this qualification, before 18:39 IST, recorded:

| Item | First inventory |
| --- | --- |
| Packages | `ffmpeg-9.0.2-3.fc45`, `ffmpeg-libs-9.0.2-3.fc45`, `libavcodec-freeworld-9.0.2-3.fc45`, `libavdevice-9.0.2-3.fc45` |
| `ffmpeg` install time | Wed 04 Nov 2026 11:19:14 PM IST |
| Build time | Thu 29 Oct 2026 |
| `ffmpeg-devel` | Not installed |
| `/usr/include/ffmpeg` | Absent |
| `pkg-config` for `libavcodec` and the other approved modules | Missing |
| Unversioned `libavcodec.so` and the other approved linker names | Absent |
| `libavcodec.so.63.1.102` size | 14,926,120 bytes |
| SHA-256 `/usr/lib64/libavcodec.so.63.1.102` | `55b1eddbea911f7890c71814be5f2c4f200b44bf4048c1a3b00644fb87d74435` |
| SHA-256 `/usr/lib64/ffmpeg/libavcodec.so.63.1.102` | `7858393cbb24b23463095cc5c9ac4ae33ec3c81d26ffddcc8e87acf1f34c51e7` |

A closing check the same evening found a different RPM release already installed. This qualification did not run `dnf`, `rpm`, or any installer. The new install time is the evidence that the replacement happened on the machine during the audit:

| Item | Closing check |
| --- | --- |
| Packages | `ffmpeg-9.0.2-4.fc45`, `ffmpeg-libs-9.0.2-4.fc45`, `libavcodec-freeworld-9.0.2-4.fc45`, `libavdevice-9.0.2-4.fc45`, and `ffmpeg-devel-9.0.2-4.fc45` |
| Install time of those packages | Sun 22 Nov 2026 06:39:50 PM IST |
| Build time | Wed 11 Nov 2026 09:59:16 PM IST |
| Build host | `buildvm-03.online.rpmfusion.net` |
| RPM `License` field, including `ffmpeg-devel` | `GPLv3+` |
| `/usr/include/ffmpeg` | Present. `avcodec.h` is owned by `ffmpeg-devel-9.0.2-4.fc45` |
| `pkg-config` | Resolves `libavutil` 61.1.102, `libavcodec` 63.1.102, `libavformat` 63.1.102, `libswscale` 10.1.102, `libswresample` 7.1.102, and also `libavfilter` 12.1.102 and `libavdevice` 63.1.102 |
| Unversioned linker names | Present, owned by `ffmpeg-devel`, for example `/usr/lib64/libavcodec.so` → `libavcodec.so.63.1.102` |
| `libavcodec.so.63.1.102` size | 14,927,200 bytes, file mtime 2026-11-03 05:30 |
| SHA-256 `/usr/lib64/libavcodec.so.63.1.102` | `22ea3be0e48440b6b9d53bf975f64541ab752aefa216febcfe18f72ddda4d63a` |
| SHA-256 `/usr/lib64/ffmpeg/libavcodec.so.63.1.102` | `bfa11fe082aad920f28d79175551506cf4e4280e905a9b3265416579dcf147fe` |
| SHA-256 `/usr/bin/ffmpeg` | `3ab9693a4c2ec509236a2d9c22cb39bc66a5af87250a05ad985c9b49ab217790` |

What did not change between those two observations: the version string is still FFmpeg 9.0.2; the printed library versions are still 61.1.102, 63.1.102, 63.1.102, 10.1.102, 7.1.102, with libavfilter 12.1.102 and libavdevice 63.1.102; the vendor is still RPM Fusion; the licence-relevant configure tokens are still `--enable-gpl`, `--enable-version3` four times, `--enable-avfilter`, `--enable-libx264`, `--enable-libx265`, `--enable-libfdk-aac`, and `--enable-decklink`; `--enable-nonfree` is still absent. Both libavcodec copies still directly need `libx264.so.165`, `libx265.so.217`, `libxvidcore.so.4`, `libfdk-aac.so.2`, and the OpenCORE and VisualOn AMR libraries.

The new headers and pkg-config modules describe this GPL build. They are not an LGPL development install. They are 9.0.2 headers, so they must not be compiled against the FFmpeg 9.1 candidate. `pkg-config` also advertises `libavfilter`, which ADR-001 does not approve. The unversioned `libavcodec.so` points at the `ffmpeg-libs` copy, while the runtime soname can still prefer `/usr/lib64/ffmpeg/libavcodec.so.63` through `ffmpeg-lib64.conf`. A link made with these `.pc` files is a link to the GPL build.

`ffmpeg-free` and `ffmpeg-free-devel` were not installed at the closing check.

## Differences from the previous machine record

The previous record is `docs/validation/SLICE_1E_MEDIA_METADATA_AND_SOURCE_TIMEBASE.md`, written earlier on 22 November 2026. The gate facts in that record still hold. This re-check did not assume they were unchanged.

Still true, and matched on this pass:

- CLI path `/usr/bin/ffmpeg`. `command -v` also finds `/usr/bin/ffprobe` and `/usr/bin/ffplay`.
- Version string: FFmpeg 9.0.2, copyright 2000-2026, built with GCC 16.
- Packager and vendor: RPM Fusion. URL `https://ffmpeg.org/`. Build host `buildvm-03.online.rpmfusion.net`.
- RPM `License` field `GPLv3+` on `ffmpeg`, `ffmpeg-libs`, `libavcodec-freeworld`, `libavdevice`, and, after the mid-audit replacement, `ffmpeg-devel`.
- The configuration line printed by `ffmpeg -version` still matches the licence-relevant tokens quoted in the Slice 1E record: `--enable-gpl`, `--enable-version3` four times, `--enable-avfilter`, `--enable-shared`, `--disable-static`, `--enable-libx264`, `--enable-libx265`, `--enable-libfdk-aac`, and `--enable-decklink`.
- The token `--enable-nonfree` is absent from that line. Absence is not a clearance.
- `pkg-config` is still 2.5.1. `PKG_CONFIG_PATH` and `PKG_CONFIG_LIBDIR` were unset at the first inventory. Search path: `/usr/lib64/pkgconfig:/usr/share/pkgconfig`. At that first inventory every `libav*` and `libsw*` module was missing. At the closing check, after `ffmpeg-devel-9.0.2-4.fc45` appeared, those modules resolve. They resolve to the GPL 9.0.2 build. See the section above.
- `libpostproc` has no pkg-config module. FFmpeg 9.1 does not build `libpostproc`.
- Runtime sonames are present: `libavutil.so.61`, `libavcodec.so.63`, `libavformat.so.63`, `libswscale.so.10`, `libswresample.so.7`, `libavfilter.so.12`, `libavdevice.so.63`.
- Library versions printed by the CLI: libavutil 61.1.102, libavcodec 63.1.102, libavformat 63.1.102, libswscale 10.1.102, libswresample 7.1.102, libavfilter 12.1.102, libavdevice 63.1.102.
- No second FFmpeg install under `/usr/local` or the project tree. `/usr/local/bin` and `/usr/local/lib` are empty. `/usr/local/lib64` contains only an empty `bpf` directory.

New observations from this pass. These do not change the Slice 1E gate result. They are written because the machine was re-checked:

1. Upstream’s current stable, read from `https://ffmpeg.org/download.html` on 22 November 2026, is FFmpeg 9.1 “Gauss”, released 2026-11-20. The installed RPM stayed on 9.0.2. Its release number changed from 3 to 4 during the audit, as recorded above. This qualification did not upgrade it. 9.0.2 is not the pin.
2. `/lib64` and `/usr/lib64` are the same directory (`readlink -f` of both is `/usr/lib64`).
3. Two `libavcodec.so.63.1.102` files exist, owned by `ffmpeg-libs` and `libavcodec-freeworld`. On release 3 they were the same size (14,926,120 bytes) and different hashes, recorded in the table above. On release 4 they are again the same size (14,927,200 bytes) and different hashes, also in that table. The pair was not identical at either observation.
4. `/etc/ld.so.conf.d/ffmpeg-lib64.conf` contains `/usr/lib64/ffmpeg` and is owned by `libavcodec-freeworld`. `ldd /usr/bin/ffmpeg` resolves `libavcodec.so.63` to `/usr/lib64/ffmpeg/libavcodec.so.63`. The other FFmpeg sonames resolve under `/lib64`.
5. `readelf -d` on both libavcodec copies shows the same direct `NEEDED` set, including `libx264.so.165`, `libx265.so.217`, `libxvidcore.so.4`, `libfdk-aac.so.2`, `libopencore-amrnb.so.0`, `libopencore-amrwb.so.0`, and `libvo-amrwbenc.so.0`. The `ffmpeg-libs` copy is not an LGPL-only libavcodec.
6. `libavformat.so.63` directly needs `libgnutls.so.30`, `libgmp.so.10`, `libsmbclient.so.0`, `libdvdnav.so.4`, and `libdvdread.so.8`. It does not directly need `libssl`. A transitive `ldd` walk can still show `libssl.so.4`. The printed configure line has `--enable-gnutls` and does not have `--enable-openssl`.
7. `libswresample.so.7` directly needs `libsoxr.so.0`. `libavutil.so.61` directly needs `libva`, `libvdpau`, `libgcrypt`, `libdrm`, `libvpl`, and `libOpenCL`. `libavfilter.so.12` is installed and directly needs `libavcodec`. `libavdevice.so.63` is a separate RPM and directly needs `libavfilter`.
8. `fdk-aac-free-2.0.3-3.fc45` provides `libfdk-aac.so.2`. The package name is not an LGPL finding. Its `NOTICE` is the Fraunhofer FDK AAC Codec Library for Android licence. `openssl-libs-4.0.3-1.fc45` is installed and its `LICENSE.txt` is Apache License 2.0. The system FFmpeg configure line does not enable OpenSSL.
9. `nasm` and `yasm` are not installed. `sq` is not installed. `gpg` 2.4.9, `curl`, `xz`, `sha256sum`, `gcc` 16.2.1, and GNU Make 4.4.1 are installed. Make was not run.
10. The `ffmpeg` RPM also ships `/usr/bin/qt-faststart`. The release-3 package observed first was installed Wed 04 Nov 2026 and built Thu 29 Oct 2026. The release-4 package observed at the close was installed Sun 22 Nov 2026 06:39 PM IST and built Wed 11 Nov 2026.

`libavc1394`, `libavif`, and `libavahi` appear in `ldconfig -p` beside the FFmpeg sonames. They are not FFmpeg libraries.

## Runtime packages

| Package | Version | Licence field | Role |
| --- | --- | --- | --- |
| `ffmpeg` | 9.0.2-4.fc45 at the close; 9.0.2-3.fc45 at the first inventory | GPLv3+ | `/usr/bin/ffmpeg`, `ffprobe`, `ffplay`, `qt-faststart` |
| `ffmpeg-libs` | 9.0.2-4.fc45 at the close | GPLv3+ | sonames under `/usr/lib64`, including one of the two libavcodec copies |
| `libavcodec-freeworld` | 9.0.2-4.fc45 at the close | GPLv3+ | `/usr/lib64/ffmpeg/libavcodec.so.63` and the ld.so snippet |
| `libavdevice` | 9.0.2-4.fc45 at the close | GPLv3+ | `libavdevice.so.63`. Not in the ADR-001 link set |
| `ffmpeg-devel` | 9.0.2-4.fc45, present only at the close | GPLv3+ | headers, pkg-config files, and unversioned `.so` symlinks for the same GPL build |

RPM signature key id on these packages: `e06f8ecdd651ff2e`. That is the RPM Fusion package signature, not the upstream FFmpeg release key.

## Development files

At the first inventory the shared objects were runtime libraries only. At the close, `ffmpeg-devel-9.0.2-4.fc45` provides headers and pkg-config files for that same GPL configuration. A program can now be compiled against them. That does not make the install the approved development configuration. The headers belong to a build with `--enable-gpl`, `--enable-version3`, libx264, libx265, libfdk-aac, DeckLink, and libavfilter. ADR-001 already rejects this install as the distribution configuration. The new devel package does not change that rejection. The CLI is still not the Media Engine.

## Conflicting installs

One vendor, two libavcodec binaries, one soname. The dynamic linker prefers the freeworld path for `libavcodec.so.63`. No `/usr/local` FFmpeg and no project-tree FFmpeg. After 18:39 IST there is a header-complete install, and it is the GPL build. There is still no second, LGPL-only install on this machine.

## Gate fact for the installed build

The installed build was compiled with `--enable-gpl` and with GPL external libraries, including libx264. Under the distinction on `https://www.ffmpeg.org/legal.html`, GPL applies to that build. The RPM licence field is `GPLv3+`, which matches a build that also passes `--enable-version3`. `--enable-nonfree` is not printed. In the FFmpeg 9.1 source evaluated separately, DeckLink is in the nonfree list; this 9.0.2 binary enables DeckLink without printing `--enable-nonfree`. That combination is not treated as a nonfree clearance and is not treated as an LGPL build.

ADR-001 stays **ACCEPTED WITH CONDITIONS**. ADR-008 stays **OPEN — PENDING LEGAL COUNSEL**. This audit does not change either file.
