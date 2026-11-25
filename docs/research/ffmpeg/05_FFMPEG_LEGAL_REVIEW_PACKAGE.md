# FFmpeg legal review package

Date: 22 November 2026. Audience: the project owner and legal counsel. This package does not answer a legal question by choosing the comfortable reading. ADR-008 stays **OPEN — PENDING LEGAL COUNSEL**. It is not marked accepted. The owner’s 21 November 2026 Apache-2.0 direction for Aether’s own source is recorded in that ADR and is not re-decided here.

Status words used below mean only this:

- **Technically verified** — a command, file, or configure result on this machine shows the fact.
- **Documented by upstream licence** — the statement is in FFmpeg’s published legal page or in `LICENSE.md` of the 9.1 archive. That is not advice from this project’s counsel.
- **Requires legal counsel** — the engineering record is not allowed to close it.
- **Requires project-owner decision** — the owner has to accept or reject a choice. This qualification does not do that.
- **Unresolved** — the sources disagree, or the text was not obtained, and no status above would be honest.

## Questions

The open rows are the ones ADR-008 lists as still open. There are eleven. Each is classified. FFmpeg-specific follow-ups that sit under those rows are included so counsel can see what this configure did and did not decide.

### 1. Aether source licence

**Requires project-owner decision. Requires legal counsel.**

ADR-008 records that on 21 November 2026 the owner directed Apache License 2.0 for Aether’s own source, with the Society of Indian Screen Editors Foundation as copyright holder. The ADR says that direction does not accept ADR-008. This qualification does not confirm the text of `LICENSE`, does not edit it, and does not treat Apache-2.0 as a clearance of FFmpeg, Qt, patents, or trademarks.

### 2. Contributor IP policy

**Requires legal counsel. Requires project-owner decision.**

No contributor term was reviewed. The FFmpeg configure result does not create one.

### 3. Developer Certificate of Origin and any extra contributor documents

**Requires legal counsel. Requires project-owner decision.**

Not reviewed. The FFmpeg 9.1 archive was downloaded and configured. It was not modified and it was not committed. The project tree was not turned into a git repository.

### 4. Qt distribution obligations

**Requires legal counsel.**

Qt was not part of this configure. Which Qt modules, and static or dynamic Qt, stay open. This package does not narrow them.

### 5. FFmpeg build, LGPL or GPL, for the configuration Aether intends to link

Split, because one answer would hide the distinction.

- The installed RPM Fusion build is GPL. **Technically verified.** `/usr/bin/ffmpeg` is 9.0.2, package `ffmpeg-9.0.2-3.fc45`, RPM `License` field `GPLv3+`, configuration includes `--enable-gpl` and `--enable-version3`, and the libavcodec binaries directly need `libx264`. **Documented by upstream licence:** if GPL parts are used, GPL applies to all of FFmpeg. What obligations follow from that installed combination remains **Requires legal counsel**. Engineering does not redistribute it. ADR-001 already rejects it as the distribution configuration.
- The candidate pin, FFmpeg 9.1 with the recorded configure line, reports `License: LGPL version 2.1 or later`. **Technically verified** in `config.h`: `FFMPEG_LICENSE` is that string, `CONFIG_GPL` is 0, `CONFIG_NONFREE` is 0, `CONFIG_VERSION3` is 0, and no GPL-only external is enabled. **Documented by upstream licence:** that is the script’s result when GPL, version 3, and nonfree are all off. Whether counsel accepts this configuration for Aether is **Requires legal counsel**. Whether the owner adopts the pin is **Requires project-owner decision**.
- `--enable-version3` alone is not GPL. **Documented by upstream licence** and by the 9.1 `configure` script: without GPL parts the summary is LGPL version 3 or later; with GPL parts it is GPL version 3 or later. The candidate does not pass the flag. **Technically verified** that no enabled dependency is in the version3 or GPLv3 lists. Candidate B was not selected.
- `--enable-nonfree` is absent from the system binary’s printed configuration. **Technically verified.** **Unresolved** as a legal conclusion: absence of the token is not a clearance. In the 9.1 script, DeckLink, libfdk-aac, libfdk-xhe-aac, and libmpeghdec require that flag. The system 9.0.2 build enables DeckLink and libfdk-aac without printing it. **Requires legal counsel** if anyone proposes to explain that 9.0.2 combination. The candidate does not include those libraries.
- Linking the five approved libraries rather than `libavfilter` or a GPL filter. **Technically verified** that the candidate disables `libavfilter` and `libavdevice` and enables the five. **Documented by upstream licence** that the GPL filters named in `LICENSE.md` are off unless `--enable-gpl` is passed, and this candidate does not pass it. Whether the five-library link changes any remaining duty is **Requires legal counsel**.
- The `ffmpeg` CLI as a separate work. **Technically verified** that the candidate disables programs, so a later build of this configure would not produce the CLI. The installed CLI remains the system GPL binary. Whether an unshipped CLI is a separate work is **Requires legal counsel**. Aether is not to be built around the CLI. That is the ADR-001 direction, not a licence ruling.

### 6. ProRes patents

**Requires legal counsel. Unresolved.**

**Technically verified:** the candidate enables the built-in `prores` decoder and disables every encoder, including any ProRes encoder. **Documented by upstream licence:** the patent mini-FAQ says the FFmpeg authors are not able to say that patented algorithms are absent, and it points commercial products at patent holders. Decoding a file the user already has, versus shipping an encoder, is the counsel question already written in `docs/technology/LEGAL_COUNSEL_QUESTIONS.md`. This configure does not answer it. Disabling encoders reduces what the binary would contain. It is not a patent licence.

### 7. DNx patents

**Requires legal counsel. Unresolved.**

**Technically verified:** the built-in `dnxhd` decoder is enabled and encoders are disabled. The same upstream patent mini-FAQ applies. SMPTE citations in the product documents are not a patent licence. This package does not decide VC-3 or DNxHR.

### 8. AAF implementation licensing

**Requires legal counsel.**

No AAF code was added or reviewed in this qualification.

### 9. Third-party obligations

**Technically verified** for the candidate’s external set: the configure summary lists no external libraries. The GPL, nonfree, and version3 libraries reviewed in the dependency matrix are EXCLUDE.

**Requires legal counsel** for the duties that remain if a binary of the five libraries is later shipped: FFmpeg’s own LGPL, the IJG credit for the three libjpeg-derived files, and any patent duty on built-in decoders.

**Unresolved**, and excluded rather than waved through:

- OpenSSL. The installed 4.0.3 `LICENSE.txt` is Apache-2.0. `LICENSE.md` groups OpenSSL with libraries that need `--enable-nonfree`. The 9.1 script does not put OpenSSL on the nonfree list and only demands `--enable-version3` when GPL is also enabled. Those sources disagree. The candidate leaves OpenSSL off. The disagreement stays unresolved.
- Libraries in the long external list whose own `COPYING` was not opened. They are excluded. Their licences are unresolved if someone later wants to enable one. Exclusion is the candidate decision, not an identification of each licence.
- zlib. RPM field of `zlib-ng-compat` is `Zlib`. The licence text was not opened. Left off. Optional for compressed MOV metadata, not part of the candidate.

### 10. Trademarks and asset ownership

**Documented by upstream licence:** the legal page says FFmpeg is a trademark of Fabrice Bellard, and the checklist tells distributors not to misspell it.

**Requires legal counsel** for every other trademark row ADR-008 leaves open, including whether the words ProRes, DNxHR, and DNxHD may appear in the UI. This qualification does not add those words and does not approve them.

**Requires project-owner decision** for Aether’s own name and assets. Not reviewed here.

### 11. Binary packaging

**Requires legal counsel. Requires project-owner decision.**

**Documented by upstream licence:** the LGPL checklist (source correspondence, build instructions, notices, about box, download page, user ability to replace or relink, no GPL libraries, and the same checklist for any LGPL library compiled into FFmpeg). The dynamic-link note maps each item and marks them not done, because no package exists.

**Technically verified:** the candidate is shared, not static; it links no external library; it does not enable rpath; Linux, Windows, and macOS replacement mechanisms are described and not implemented.

Open source does not erase those duties. A private tree, a source tarball of Aether, and a binary that carries the five libraries are different cases. They are separated in `04_FFMPEG_DYNAMIC_LINKING_AND_DISTRIBUTION.md`. None of them is cleared here.

## What counsel is not being told

- That configure exit 0 means the licence is acceptable.
- That the system package is LGPL because `--enable-nonfree` was not printed.
- That dynamic linking, by itself, satisfies the LGPL.
- That Apache-2.0 on Aether’s source relicenses FFmpeg.
- That disabling encoders answers the ProRes or DNx patent questions.
- That this package accepts ADR-008.
