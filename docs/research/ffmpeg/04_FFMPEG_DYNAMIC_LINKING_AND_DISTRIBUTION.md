# FFmpeg dynamic linking and distribution

Date: 22 November 2026. This is a note for counsel and for a later packaging task. It does not implement packaging. It does not link Aether to FFmpeg. ADR-008 stays **OPEN**. Upstream’s checklist on `https://www.ffmpeg.org/legal.html` says it is not the only way to comply and that the page is not legal advice. The items below are that checklist, applied to the candidate as engineering facts. They are not marked done.

## Model

ADR-001 requires dynamic linking of `libavutil`, `libavcodec`, `libavformat`, `libswscale`, and `libswresample`, inside the Media Engine only. The UI does not call FFmpeg. Domain code does not include FFmpeg headers. The CLI is not the application’s media path.

The candidate configure selects those five as shared libraries and disables static libraries. `--enable-rpath` was not passed. A user-replaceable library is the point of the LGPL dynamic-link path described by upstream. An rpath that pins a private copy so the user cannot substitute a rebuilt library would need its own review. It is not part of this candidate.

The system copies under `/usr/lib64` are not the libraries this note describes. They are a GPL build. They are not vendored and they are not the pin.

No `.so`, `.dll`, or `.dylib` of the pin was produced. `make` was not run.

## Linux

- Ship the shared objects with their sonames: `libavutil.so.61`, `libavcodec.so.63`, `libavformat.so.63`, `libswscale.so.10`, `libswresample.so.7`, from a build of this pin only.
- Do not mix 9.1 headers with the installed 9.0.2 libraries. The minors differ, and the installed libraries are GPL.
- Do not copy `/usr/lib64/libavcodec.so.63` or `/usr/lib64/ffmpeg/libavcodec.so.63` into an Aether package. Those two files are different binaries, and both directly need GPL and nonfree-classified libraries.
- The dynamic linker must resolve the pin, not “whatever `ldconfig` finds first”. On this machine the first `libavcodec.so.63` is the RPM Fusion freeworld file, because `/etc/ld.so.conf.d/ffmpeg-lib64.conf` inserts `/usr/lib64/ffmpeg`.
- Replacement: a user who has the corresponding source and these build instructions can rebuild the shared objects and point the dynamic linker at the rebuild (runpath chosen at packaging time, `LD_LIBRARY_PATH` in a private tree, or a packaged substitution). The mechanism is not implemented here. Counsel still has to accept that the chosen mechanism is enough.
- Do not strip the soname down to an unversioned private name that hides the library. Upstream allows a prefix or suffix (`libavcodec-aether.so.63` is the kind of example the checklist gives) and forbids an obfuscated rename.

## Windows

Not built and not configured here. The same source archive and the same licence flags are the starting point for a later Windows configure. Upstream’s checklist says dynamic linking on Windows means linking to DLLs, not absorbing the libraries into one static EXE.

- Distribute `avcodec-61` is the wrong major. The 9.1 majors are libavutil 61, libavcodec 63, libavformat 63, libswscale 10, libswresample 7. The DLL names follow those majors.
- Do not rename `avcodec.dll` to an unrelated stem. A prefix or suffix is the form the checklist allows.
- Import libraries (`.lib`) that only exist to link the DLL are part of dynamic linking. A static `.a` or a fully static EXE is not the candidate.
- The user must be able to replace the DLLs with a rebuild from the corresponding source. How an installer lays that out is a packaging task and was not done.
- `--disable-x86asm` was required on this Linux host because `nasm` is absent. A Windows build has its own assembler question. It is not answered by this Linux configure.

## macOS

Not built and not configured here. The candidate is dynamic `.dylib` libraries for the same five majors, not a static archive inside the application binary.

- Install names and `@rpath` have to leave a route for the user to substitute a rebuilt dylib. That route was not designed here.
- Hardening, notarisation, and library validation can conflict with replacement if they are applied without a documented relink path. That conflict is a packaging question for counsel and the owner. It is not decided.
- System FFmpeg from Homebrew or the OS, if any, is not this pin. This machine was not a macOS host.

## Three situations

These are the distinctions counsel asked to keep apart. Engineering states what happened. It does not decide the legal consequence.

### Private development

This qualification did not link Aether, did not compile the pin, and did not copy libraries into the project. A later private build of the pin, after the owner and counsel approve the gate, is still not a distribution. Whether unpublished private use creates an LGPL duty is a question for counsel. Upstream’s patent mini-FAQ separates a private user from a commercial product and does not treat that separation as a patent clearance. ADR-008 is not accepted by a private experiment.

### Source distribution

If Aether source is distributed without FFmpeg binaries, the FFmpeg source-correspondence duties attach when the FFmpeg binaries are conveyed, and also if the FFmpeg source itself is conveyed. The owner has directed Apache-2.0 for Aether’s own source. That direction does not relicense FFmpeg and does not erase the LGPL. The FFmpeg sources to ship, when a binary of this pin is shipped, are the 9.1 archive that matches the binary, plus any local patch as a diff, plus the configure line. No patch was made. The archive used is the signed `ffmpeg-9.1.tar.xz`.

### Binary distribution

This is the case the upstream checklist is written for. A binary that includes the five shared libraries has to be able to meet that checklist. Distributing Aether as open source does not remove those items. Apache-2.0 for Aether’s own code and LGPL for FFmpeg are two licences. Whether they can sit together in one product is **Requires legal counsel**. This note does not say they can.

## Checklist to give counsel

Status in the last column is the state after this qualification. “Not done” means the item is a distribution duty and no package exists yet. It is not a finding that the candidate configure violates the item.

| # | Upstream item | How it applies to this candidate | State |
| --- | --- | --- | --- |
| 1 | Compile without `--enable-gpl` and without `--enable-nonfree` | The verified command passes `--disable-gpl` and `--disable-nonfree`. `CONFIG_GPL` and `CONFIG_NONFREE` are 0. The summary licence is LGPL version 2.1 or later | Technically verified for the configure result. Not a legal approval |
| 2 | Dynamic linking | `--enable-shared`, `--disable-static`. No binary was linked | Specified. Not yet built |
| 3 | Distribute the FFmpeg source whether or not it was modified | No modification. The source to correspond is the signed 9.1 archive | Not done. No distribution |
| 4 | The source must correspond exactly to the library binaries | Cannot be checked until a binary exists. The configure line is recorded so a later binary can be matched | Not done |
| 5 | `git diff > changes.diff` if there are changes | No changes. A later patch must be a diff against this archive, not against git master | Not applicable yet |
| 6 | Explain the configure line, in a text file with the source | The command is in `03_FFMPEG_REPRODUCIBLE_BUILD_SPECIFICATION.md` and in generated `config.h` as `FFMPEG_CONFIGURATION` | Recorded. Not yet shipped with a source tarball |
| 7 | Distribute that source as a tarball or zip | The upstream xz archive is the artefact. It was not republished | Not done |
| 8 | Host that source on the same web server as the binary | No download server and no binary | Not done |
| 9 | Download-page notice: “This software uses code of FFmpeg licensed under the LGPLv2.1 and its source can be downloaded here” | The candidate is LGPLv2.1-or-later, so this is the sentence upstream prints, not the version-3 sentence | Not done |
| 10 | About box: “This software uses libraries from the FFmpeg project under the LGPLv2.1” | UI was not modified | Not done |
| 11 | Mention FFmpeg under the LGPLv2.1 in the EULA | No EULA was written | Not done. Requires legal counsel |
| 12 | If the EULA claims ownership of the code, say that FFmpeg is not owned and where the owners are found | Not written | Requires legal counsel |
| 13 | No prohibition of reverse engineering in the EULA | Not written | Requires legal counsel |
| 14 | Same text in every translation of the EULA | No translation | Requires legal counsel |
| 15 | Spell FFmpeg with two capitals F and a lowercase “mpeg” | Used that way in these notes | A writing rule, not a clearance |
| 16 | Do not give the libraries an obfuscated name. A prefix or suffix is allowed | No rename was made | Holds for the future package |
| 17 | Repeat the checklist for any LGPL library compiled into FFmpeg. Upstream’s example is LAME | No external library is enabled. LAME is not compiled in. libsoxr is LGPL-2.1-or-later and is also not compiled in | Technically verified for this configure: the external list is empty |
| 18 | The program must not use GPL libraries. Upstream names libx264 | libx264, libx265, libxvid, and the rest of the GPL list are disabled. `CONFIG_LIBX264` is off | Technically verified for this configure |

Also for counsel, from `LICENSE.md`, and not in the numbered checklist: the IJG credit for the three libjpeg-derived files if executables are distributed. See the dependency matrix.

## What this note refuses

- Treating the RPM Fusion 9.0.2 libraries as the LGPL binaries.
- Treating a successful configure as a shipped, compliant package.
- Treating Apache-2.0 on Aether’s own source as a waiver of the LGPL.
- Adding `libavfilter`, a GPL external, or `--enable-nonfree` and still using the LGPLv2.1 sentences in the checklist.
