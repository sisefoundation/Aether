# External NLE relink and conform test

Status: **Not tested.** No conform was produced. No relink was performed in another application. Naming DaVinci Resolve, Avid Media Composer, or Adobe Premiere Pro in this file is not a result.

Date: 2026-11-20. Machine: the Fedora host used for `docs/validation/FFMPEG_MXF_ROUNDTRIP_TEST.md`.

## 1. What was checked for an installation

| Check | Result |
| --- | --- |
| `/opt/resolve`, `/opt/blackmagic`, `/opt/DaVinciResolve` | Absent |
| `rpm -qa` names matching DaVinci, Blackmagic, Media Composer | No such packages. `systemd-resolved` matches the substring “resolve” and is not DaVinci Resolve |
| Desktop files under `/usr/share/applications` for Resolve, Avid, Premiere, Composer | None |
| Flatpak list | No Resolve, Media Composer, or Premiere |
| Adobe RPM names present | `adobe-mappings-cmap`, `adobe-mappings-cmap-deprecated`, `adobe-source-code-pro-fonts`, `adobe-mappings-pdf`. These are fonts and PDF mappings. They are not Premiere Pro |

No application was launched. The synthetic MXF files were not imported anywhere.

## 2. Why the FFmpeg files are not a substitute

`docs/NATIVE_MEDIA_ARCHITECTURE.md` already says a file FFmpeg can open is not, on that evidence, a file another editorial application can relink. This session adds local measurements and does not change that sentence.

In particular, the OP1a files MediaInfo calls `Frame (AES)` are not the wave mapping the container specification targets, and the 30000/1001 files do not carry the editorial sample grid. Offering either file to a vendor and treating a successful open as a verified conform would contradict `docs/CONFORM_ENGINE_SPECIFICATION.md` and `docs/CONFORM_VALIDATION_AND_REPORTING.md`.

## 3. Procedure to run later

Do this on a machine where the application is actually installed, with a copy of the synthetic fixtures, never with camera originals. This review did not run it.

1. Record the application name, version, and build.
2. Copy, do not move, these files to a directory that test owns:
   - `validation-fixtures/ffmpeg-mxf-numbered-2026-11-20/out/op1a_prores422hq_pcm_s24le_stereo_24fps_24f.mxf`
   - `validation-fixtures/ffmpeg-mxf-numbered-2026-11-20/out/opatom_prores422hq_picture_24fps_24f.mxf`
   - `validation-fixtures/ffmpeg-mxf-numbered-2026-11-20/out/opatom_pcm_s24le_mono_48000_edit24.mxf`
3. In the NLE, import or relink as that application’s own operation. Write down the clicks and the on-screen messages.
4. Record, from the NLE’s own inspectors, not from ffprobe: operational pattern if shown, frame count, frame rate, timecode of the first frame, audio sample rate, channel count, and whether picture and sound stay together.
5. For the 24-frame OP1a file, step through frame 0 and frame 23 and say whether the white bar grows and whether the single white marker moves. A playhead that only shows a duration near one second is not a frame-identity pass.
6. Relink: change the path by copying the same bytes to a new name. Record whether the NLE keeps the cut or asks for a new conform. AETHER’s rule, which this test does not certify, is that relink updates a location and is not a conform state (`docs/MEDIA_RELINK_ENGINE.md`).
7. Conform: only if that NLE has an explicit conform or online command. Save its report. An export, a render, or a file that simply opens is not a verified conform (`docs/ONLINE_CONFORM_ARCHITECTURE.md`).
8. If the application refuses the file, the result is **Failed** or **Not supported** for that tuple, with the message copied verbatim. Do not retry with a different muxer and then report the first refusal as a pass.

OP-Atom stereo is not in the procedure. This build wrote no such file.

## 4. Result

**Not tested.** Reason: Resolve, Media Composer, and Premiere Pro are not installed on this machine. No interchange cell in `docs/EXCHANGE_COMPATIBILITY_MATRIX.md` changes because of this document.
