# AETHER dependency and licence audit

Status: not an audit. This is the list the audit must complete before any dependency is approved. No licence text was fetched and no binary was inspected for this document. Nothing here clears a library for redistribution.

ADR-001 has since accepted a direction: FFmpeg is the mandatory core media framework and GStreamer is not a mandatory dependency. That acceptance does not clear a licence. A library may be redistributed only after the checks below are recorded against the pinned version counsel has read. ADR-008 stays open.

## Verified facts

- No dependency is linked in this specification workspace.
- This checklist did not itself run FFmpeg. Later sessions did. Their results are engineering records, not licence findings: `docs/validation/FFMPEG_MXF_ROUNDTRIP_TEST.md` and `docs/validation/AUDIO_FRAME_SAMPLE_BOUNDARY_VALIDATION.md`. The 29.97 audio contract **Failed**. Library integration is **BLOCKED**. The Fedora GPL build is not an approved distribution configuration.
- No statement in this file is a legal opinion. SISE needs counsel for the project licence and for codec patents.

## Engineering recommendations

- Pin versions. Record the exact source, the licence names on that tag, and whether the build flags change the licence (FFmpeg and GStreamer are the obvious cases).
- Prefer dynamic linking where a weak copyleft requires it, and document the obligation. Do not assume LGPL is “free to ship statically”.
- Keep each candidate behind the module boundary in `SOFTWARE_MODULES.md` so a rejected library can be removed without rewriting the cut.
- Do not enable a patent-encumbered encoder by default until counsel and SISE say so. Playback of a file the user already has is a different question from redistributing an encoder. Both need review.
- Treat trademarks as separate from copyright. “ProRes” and “DNxHR” may be usable only as descriptive compatibility notes. The UI should be ready to say “VC-3” or “vendor codec” if counsel requires it.

## Proposed AETHER behaviour

Until a row is marked approved by a later review, the application specification does not require that library. The prototype scope in `DEVELOPMENT_ROADMAP.md` may name at most one processing candidate and SQLite as the database candidate, still behind interfaces, and still subject to this audit. If the audit rejects a candidate, the prototype waits or switches candidates. It does not copy the code anyway.

### Check each candidate

For every row, the audit must record: version, copyright holders, licence text URL as of the pin, patent notes, API stability, Linux, Windows, and macOS support, runtime cost relevant to the performance table, redistribution duties, and whether AETHER’s chosen project licence can be combined with it.

| Candidate | Role | What is commonly published, to be confirmed on the pin | Audit status |
| --- | --- | --- | --- |
| C++ language and a C++ standard library | Implementation language | The language is an ISO standard. Compiler runtimes (`libstdc++`, libc++, MSVC) have their own licences. | Not started. Language is a candidate, not an approval. |
| Qt 6 | UI, docking, high-DPI, cross-platform widgets | Qt is widely published under LGPL-3.0 and a commercial licence. Some add-on modules are GPL-only. Obligations depend on the modules and on static versus dynamic linking. | Not started. Leading UI candidate because it matches the UX requirements. Not cleared. |
| FFmpeg | Decode, encode, containers | FFmpeg is widely published under LGPL-2.1 or later, becoming GPL if GPL-only components are enabled. A non-free build flag exists and is incompatible with the GPL builds. Patents on codecs are separate from that copyright licence. | Not started. Prior 9.0.2 study is a behaviour note, not a licence pass, and was not re-run. |
| GStreamer | Alternative media framework | Core is widely published under LGPL. Plugin sets (base, good, ugly, bad, libav) differ and include GPL and patent-sensitive plugins. | Not started. |
| OpenTimelineIO | OTIO adapter only | The project is widely published under Apache-2.0. Confirm on the pin. | Not started. Must not become the project database. |
| SQLite | Local project database | SQLite is published with a public-domain dedication. Confirm the amalgamation used. | Not started. Recommended engine. Not linked. |
| PostgreSQL | Shared server, only if a real shared service appears | PostgreSQL is published under a permissive licence similar to BSD. It is not the default and is out of scope until needed. | Not started. Not recommended now. |
| OpenColorIO | Display colour transforms | The project is widely published under BSD-3-Clause. Confirm on the pin. | Not started. Optional for a display hook. |
| DeckLink SDK | Optional monitoring hardware | Blackmagic’s SDK is proprietary. Redistribution and API terms are whatever the current SDK agreement says. | Not started. Default off. |
| AAF SDK or another AAF writer | Audio exchange | Several codebases exist. Their licences are not interchangeable. A narrow writer might be written by SISE and then itself needs a licence. | Not started. |
| MXF, ProRes, DNxHR / VC-3, PCM | Profiles, not libraries | OP1a is SMPTE ST 378. OP-Atom is SMPTE ST 390. VC-3 is associated with SMPTE ST 2019. ProRes and DNx names are vendor technologies. PCM in WAV/BWF is the conservative audio path. | Not validated. Prior FFmpeg notes apply as behaviour to reconcile. |

### Project licence, unresolved

AETHER’s own licence is not chosen. It has to be chosen before dependencies are approved, because the combination is the real question.

| Option | Effect |
| --- | --- |
| Strong copyleft | Editorial improvements stay open. Combining with some codecs and with a proprietary DeckLink SDK or a commercial Qt licence needs explicit review. |
| Weak copyleft | Core stays open. Plug-ins may be closed. LGPL dependencies are a closer fit. Patent grants must still be read. |
| Permissive | Easiest combination with commercial tools. Weakest obligation for a facility to return cutting-room improvements. |

This specification does not pick a row. SISE counsel and the editors’ society pick it.

### Redistribution rules the architecture will honour once an audit exists

- Ship a licence inventory with the application.
- Do not statically link an LGPL library unless the audit says the chosen method complies.
- Do not turn on FFmpeg `--enable-nonfree` or GPL-only codecs in a binary that is presented as LGPL-only.
- Do not ship the DeckLink SDK inside the default build.
- Do not describe a codec as available unless both the legal status and the validation suite allow it.

### Prior FFmpeg study, licence-adjacent only

The behaviour limits (PCM profile, layout, alpha, 29.97 samples, optional OP-Atom) can cause AETHER to misrepresent media if a build enables those muxers early. That is a safety and truth issue as well as an engineering issue. It does not, by itself, say whether the muxers are legal to ship. Both gates apply: behaviour validation and licence validation.

## Rationale

| Decision | Rationale |
| --- | --- |
| No approval in this document | The brief says to verify licences, APIs, platforms, performance, and redistribution. This environment did not do that work. Writing “approved” would be false. |
| Record commonly published licence names as leads | Reviewers need a starting point. Each lead is marked unconfirmed so it cannot be quoted as clearance. |
| Project licence first | A candidate’s licence is only meaningful next to AETHER’s licence. |
| Codecs are not just software copyright | ProRes and DNx-family patents and trademarks have blocked careless redistributions before. The audit has to say so without pretending a case was briefed. |

## Unresolved questions

- AETHER’s licence.
- Counsel on patent risk for distributing ProRes or VC-3 encoders versus decoding only.
- Whether Qt’s LGPL build is acceptable for SISE’s first Linux packages, and which Qt modules are actually needed (widgets and perhaps multimedia; multimedia may pull another media stack and should be avoided if processing is separate).
- Which AAF code, if any, has a licence SISE can accept.
- Whether GStreamer ugly/bad plugins are forbidden entirely.
- Export compliance and cryptography. SQLite and HTTPS update checks, if ever added, need a second look. Default builds have no network.
- Font licence for the UI typeface. Unchosen.
