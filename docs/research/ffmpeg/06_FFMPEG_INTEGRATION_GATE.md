# FFmpeg integration gate

Date: 22 November 2026.

**The gate is closed.**

A configure pass does not open it. Owner approval and counsel approval are both still required. ADR-001 stays **ACCEPTED WITH CONDITIONS**. ADR-008 stays **OPEN — PENDING LEGAL COUNSEL**. Neither file was edited. This note does not mark ADR-008 accepted.

## What was shown

- The installed FFmpeg is the RPM Fusion 9.0.2 GPL build. At the start of the audit it had no development headers. At the close, `ffmpeg-devel-9.0.2-4.fc45` was installed on the machine. This qualification did not install it. Those headers belong to the GPL build. They remain rejected as the distribution configuration and as a link target.
- A separate pin is specified: FFmpeg 9.1, signed archive, Candidate A, LGPL version 2.1 or later.
- An out-of-tree configure of that pin exited 0. `config.h` records GPL off, nonfree off, version 3 off, shared on, static off, the five libraries on, and `libavfilter` off.
- No external GPL library, nonfree library, or version3 library is enabled.
- `make` was not run. Aether was not linked. No header was installed over the system FFmpeg.

## What is still required before the gate can be discussed again

All of these. One of them is not enough.

1. The owner approves the pin FFmpeg 9.1 and the exact configure command, including `--disable-x86asm` for as long as nasm is absent, or a new configure record after nasm is deliberately installed.
2. Counsel reviews `05_FFMPEG_LEGAL_REVIEW_PACKAGE.md` and the dependency matrix, and answers the open ADR-008 rows that this build actually touches. Engineering will not treat silence as acceptance.
3. ADR-008 moves only if the owner and counsel move it. This gate does not move it.
4. A later build uses one prefix, one header tree, and the libraries from that same build. It does not point `-I` at `/usr/include` and `-L` at `/usr/lib64`.
5. The Media Engine, when it exists, links only the five shared libraries. UI code does not include FFmpeg headers. `libavfilter` stays out until a new architecture decision.
6. The LGPL distribution items in `04_FFMPEG_DYNAMIC_LINKING_AND_DISTRIBUTION.md` are implemented for any binary that leaves the machine. They were not implemented here.
7. Slice 1E implementation and playback stay unstarted until a later task is actually authorised. This qualification does not authorise them.

## What would keep the gate closed even after a clean rebuild

- Linking the system `libavcodec.so.63`, either copy.
- Adding `--enable-gpl`, `--enable-nonfree`, or `--enable-version3` without a new owner decision and a new counsel review. Version 3 is not required by the current dependency set.
- Enabling libx264, libx265, libxvid, libfdk-aac, libfdk-xhe-aac, libmpeghdec, DeckLink, OpenSSL, OpenCORE, VisualOn, smbclient, or any other library marked EXCLUDE.
- Building `libavfilter` into the Media Engine link.
- Treating the `ffmpeg` CLI as the probe.
- Installing the pin onto `/usr` or replacing `/usr/bin/ffmpeg`.
- A configure exit code used as a substitute for the approvals above.

## Current decision

Do not integrate. Do not start Slice 1E. Do not build the pin until the owner asks for that build. The candidate configuration is documented so that request has a specific command to approve or reject.
