# FFmpeg build and distribution

Status: earlier distribution notes. The build baseline is now `docs/technology/FFMPEG_DEVELOPMENT_CONFIGURATION.md`. This file does not approve a flag set. No package was installed. ADR-008 stays **OPEN**.

Companion: `docs/technology/LICENSING_AND_DISTRIBUTION_REVIEW.md` and `docs/technology/LEGAL_COUNSEL_QUESTIONS.md`.

## 1. What this machine has

| Item | Value |
| --- | --- |
| CLI | `/usr/bin/ffmpeg` 9.0.2, built with gcc 16 |
| RPM | `ffmpeg-9.0.2-3.fc45.x86_64`, `ffmpeg-libs-9.0.2-3.fc45.x86_64` |
| Development files | `ffmpeg-devel` is not installed. `/usr/include/ffmpeg` is absent. `pkg-config` does not see `libavcodec`. There is no unversioned `/usr/lib64/libavcodec.so` |
| Shared libraries that are present | `libavutil.so.61.1.102`, `libavcodec.so.63.1.102`, `libavformat.so.63.1.102`, `libswscale.so.10.1.102`, `libswresample.so.7.1.102`, and the device and filter sonames reported by `ffmpeg -version` |
| Linkage of the distro build | `--enable-shared --disable-static` |
| Licence-relevant flags printed by the binary | `--enable-gpl`, `--enable-version3`, `--enable-libfdk-aac`, `--enable-decklink`, `--enable-libx264`, `--enable-libx265`. The token `--enable-nonfree` is not in the print |
| Headers the package claims | `--incdir=/usr/include/ffmpeg` is in the configuration. The directory is not on this machine |

A program cannot be compiled against this install without installing headers or pointing at another build. This review did not install headers. Library integration is **Not tested**.

## 2. Recommendation

**not recommended:**

- Linking AETHER to the Fedora `ffmpeg-libs` package as the shippable engine.
- Treating “FFmpeg 9.0.2 is installed” as a pin. A pin is a source revision plus the exact flags plus the licence names on that revision.
- A static link. The obligations differ and were not reviewed.
- Enabling GPL-only or non-free components and describing the result as LGPL-only.

**Recommended for adoption** only as a question for counsel, **subject to prototype** after counsel answers:

- AETHER builds its own shared FFmpeg, pinned, and loads it from `aether_process`.
- The first flag sketch to put in front of counsel, not to configure now:
  - shared libraries, static disabled
  - no `--enable-gpl` unless the project licence and counsel allow a GPL media library
  - no `--enable-nonfree`
  - no `libfdk-aac`, no DeckLink, no `libx264`, no `libx265`, no other external codec that flips the licence or pulls a patent licence counsel has not read
  - the native `prores_ks` and `dnxhd` encoders left in or out as counsel decides in `docs/technology/LEGAL_COUNSEL_QUESTIONS.md` questions 9–13. Their presence in an LGPL FFmpeg source tree is a copyright fact and a patent question, and this file does not split that question
- Runtime is the libraries. The CLI may be built for tests. The application does not require the CLI on the user’s `PATH`.
- The capability table records the pin and the tuple id from `docs/technology/FFMPEG_INTEGRATION_ARCHITECTURE.md`. A distro upgrade that changes flags does not silently become the pin.

**further research:**

- Whether the pin is the 9.0.2 tag or a later tag. This machine measured 9.0.2 only. `docs/NATIVE_MEDIA_ARCHITECTURE.md` also measured 9.0.2, on 19 November 2026. The two sessions are not one certification.
- Windows and macOS. This review is one Fedora machine.
- Reproducible builds and the licence inventory that `docs/18_BUILD_AND_RELEASE.md` already requires before a release. No release exists.

## 3. What a later build record must contain

Name, source pin, every configure flag, licence texts for that pin, patent note, shared or static, the module allowed to link it (`aether_process`), platforms actually built, and the validation-suite id. `docs/03_TECHNOLOGY_DECISIONS.md` already says no such record exists. This review still does not create one that claims approval.

## 4. Distribution

Do not copy this machine’s `/usr/lib64/libavcodec.so.*` into an AETHER package. Do not vendor the FFmpeg sources into `third_party/` until ADR-008 moves. `third_party/` stays empty.
