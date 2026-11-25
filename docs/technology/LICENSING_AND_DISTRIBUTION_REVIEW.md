# Licensing and distribution review

Status: facts, open questions, risks, and next actions. No licence is selected. ADR-008 stays **OPEN**. This file is not a legal opinion and it does not clear any codec, toolkit, or build for redistribution.

`LICENSE` at the repository root is empty. This review did not fill it in. No licence text was fetched. No third-party source was copied into `third_party/`.

## 1. Facts observed on this machine, 2026-11-20

These are configuration strings and package names. They are not a finding that the build is lawful or unlawful to ship.

| Fact | Observation |
| --- | --- |
| Binary | `/usr/bin/ffmpeg`, `/usr/bin/ffprobe`, `/usr/bin/mediainfo` |
| Version line | `ffmpeg version 9.0.2`, gcc 16 |
| Packages | `ffmpeg-9.0.2-3.fc45.x86_64`, `ffmpeg-libs-9.0.2-3.fc45.x86_64`. `ffmpeg-devel` is not installed |
| Libraries on disk | Shared objects only: `libavutil.so.61.1.102`, `libavcodec.so.63.1.102`, `libavformat.so.63.1.102`, `libavdevice.so.63.1.102`, `libavfilter.so.12` (soname present via the CLI version line 12.1.102), `libswscale.so.10.1.102`, `libswresample.so.7.1.102` |
| Link surface | No unversioned `libavcodec.so`, no `libavcodec.pc`, no `/usr/include/ffmpeg` |
| Flags present in `ffmpeg -buildconf` | `--enable-gpl`, `--enable-shared`, `--disable-static`, `--enable-version3`, `--enable-libfdk-aac`, `--enable-decklink`, `--enable-libx264`, `--enable-libx265`, plus many other `--enable-lib*` lines recorded in `validation-fixtures/ffmpeg-mxf-numbered-2026-11-20/logs/run.log` |
| Flag absent | The token `--enable-nonfree` does not appear in that configuration print. Absence of the token is not a conclusion about `libfdk-aac` or about Fedora’s packaging |
| Project licence | `LICENSE` is an empty file |

The same binary’s behaviour on synthetic MXF is `docs/validation/FFMPEG_MXF_ROUNDTRIP_TEST.md`. Behaviour is a separate gate from copyright, patents, and trademarks. Both gates are still open (`docs/DEPENDENCY_AND_LICENCE_AUDIT.md`).

## 2. Risks

| Risk | Why it is current |
| --- | --- |
| R-F in `docs/ARCHITECTURE_RISK_REGISTER.md` | Qt, FFmpeg, ProRes, DNx, or AAF could be described as available before counsel writes |
| Shipping this Fedora binary inside AETHER | The printed configuration is a GPL-enabled shared build with extra codecs and DeckLink. Describing it as an LGPL-only AETHER component would contradict the flags on the page |
| Encoder versus decoder | Playing a file the user already has is a different question from distributing an encoder. This review ran encoders. That run is not permission to ship them |
| Names in the UI | “ProRes” and “DNxHR” are vendor technologies. The UI may have to say VC-3 or “vendor codec”. This review uses the names because the specifications and the encoder help use them. Use here is not a trademark clearance |
| Patents | SMPTE documents name VC-3 and MXF patterns. Patent obligations are not settled by citing a standard, and not by a local mux |
| Empty `LICENSE` | A dependency’s licence is only meaningful next to AETHER’s licence, which is unchosen |
| Static link | This machine’s FFmpeg is `--disable-static --enable-shared`. Static linking of an LGPL library is a separate obligation. It was not evaluated because no AETHER binary exists |
| System headers missing | A later “it linked on Fedora” claim would require `ffmpeg-devel` or a private build. Neither was installed. This review must not be quoted as a link test |

## 3. Distribution posture recommended to counsel, not adopted

**not recommended** until counsel says otherwise:

- Redistributing `/usr/bin/ffmpeg` or `ffmpeg-libs` from this Fedora install as AETHER’s media engine.
- Enabling `--enable-gpl` or `--enable-nonfree` or `libfdk-aac` in a build that is described as LGPL-only.
- Turning patent-sensitive encoders on by default.
- Vendoring FFmpeg into `third_party/` before ADR-008 moves.
- Shipping the DeckLink SDK in the default build.

**subject to prototype**, and only after a written counsel note:

- A pinned FFmpeg source, built by the project, shared libraries, a flag set counsel has read, installed beside AETHER rather than taken from the distro package.
- Dynamic linking from `aether_process` only.
- A capability table that can hide an encoder even when the library contains it.

The flag sketch is in `docs/technology/FFMPEG_BUILD_AND_DISTRIBUTION.md`. It is a question for counsel, not a chosen build.

## 4. Next actions

1. Subhajit and counsel choose AETHER’s own licence (copyleft or permissive). The three families in `docs/DEPENDENCY_AND_LICENCE_AUDIT.md` stay open. This review prefers none.
2. Counsel reads the printed configuration of FFmpeg 9.0.2 on this machine and says whether any AETHER build may resemble it.
3. Counsel separates decode of files the user has from distribution of ProRes and VC-3 encoders, and says which names may appear in the UI.
4. Counsel reviews Qt 6 module list and static versus dynamic linking before any UI target is allowed to link Qt.
5. AAF code, if any, is named and read. “An AAF SDK” is not a licence.
6. No release note claims a codec is available on the strength of ADR-008 or of this file.
7. `LICENSE` stays empty until a person writes it. A preparation task does not.

## 5. Safety rule, still a product proposal

The application does not silently delete, overwrite, rename, or move original media. Proxies and transcodes are new files. The synthetic files in `validation-fixtures/` were created for this review. They are not originals. The safety rule is not a substitute for the legal decision.
