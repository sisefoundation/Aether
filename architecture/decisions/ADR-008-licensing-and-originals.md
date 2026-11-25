# ADR-008 — Originals, project licence, and third-party legal review

- Status: **OPEN — PENDING LEGAL COUNSEL**
- Date: 2026-11-20. Source-licence direction added 2026-11-21. This ADR is not accepted.
- Deciders: the project owner, with counsel for every legal row. Engineering does not close this ADR and does not make legal determinations.
- Related: `docs/technology/LEGAL_COUNSEL_QUESTIONS.md`, `docs/technology/LICENSING_AND_DISTRIBUTION_REVIEW.md`, `docs/technology/FFMPEG_DEVELOPMENT_CONFIGURATION.md`, `docs/16_SECURITY_AND_LICENSING.md`, `LICENSE`, `NOTICE`

## Context

On 2026-11-20, `LICENSE` was empty and Apache-2.0 was only a proposal. That record stays. It was true on that date.

On 21 November 2026 the owner directed Apache License, Version 2.0, as the licence for AETHER’s own source. The copyright holder is the Society of Indian Screen Editors Foundation. The work is AETHER. The architect is Subhajit Singha, SISE. `LICENSE` now contains the Apache-2.0 terms, and the appendix boilerplate is applied. `NOTICE` carries the same attribution.

That direction is the project source licence. It does not accept this ADR. Counsel questions for Qt, FFmpeg, codecs, AAF, and trademarks remain open. They are not accepted.

The safety rule about original media is a product rule. It is not a substitute for the legal decision. Dynamic linking does not automatically clear Qt, FFmpeg, patent, trademark, or packaging obligations.

## Decision

This ADR is **not accepted**. Status stays **OPEN — PENDING LEGAL COUNSEL**.

The owner’s 21 November 2026 direction covers AETHER source only: Apache License, Version 2.0, copyright 2026 Society of Indian Screen Editors Foundation. It does not choose an answer for Qt distribution, the FFmpeg build’s LGPL or GPL status, ProRes patents, DNx patents, AAF implementation licensing, or trademarks.

**Product rule, not a legal clearance:** the application does not silently delete, overwrite, rename, or move original media. Proxies and transcodes are new files. A confirmed destructive command names the path and the Media ID.

## Still open

Counsel has not answered these. Engineering does not answer them:

- AETHER source licence. The owner directed Apache-2.0 for AETHER source on 21 November 2026. Counsel has not accepted the remaining rows, and this line is not closed by that direction
- contributor IP policy
- Developer Certificate of Origin and any extra contributor documents
- Qt distribution obligations
- FFmpeg build, LGPL or GPL, for the actual configuration AETHER intends to link
- ProRes patents
- DNx patents
- AAF implementation licensing
- third-party obligations
- trademarks and asset ownership
- binary packaging

Also still open, and folded into the list above rather than decided here: which Qt 6 modules, static versus dynamic Qt, whether the words ProRes and DNxHR may appear in the UI, DeckLink redistribution, and a UI font licence.

## Intended dependency direction for counsel

This is the configuration counsel is asked to review. It is not a claim that the configuration is legally approved.

- Link dynamically, inside the AETHER Media Engine only: `libavutil`, `libavcodec`, `libavformat`, `libswscale`, `libswresample`.
- Do not link `libavfilter` unless a later architecture decision approves it. `libavdevice` is not in the approved set.
- Do not redistribute the Fedora `ffmpeg-9.0.2-3.fc45` GPL build. That build’s configuration print includes `--enable-gpl`, `--enable-libfdk-aac`, `--enable-decklink`, `--enable-libx264`, and `--enable-libx265`. The token `--enable-nonfree` does not appear in the print. Absence of the token is not a legal conclusion. Development headers for that build are not installed. The system install has not been shown to be linkable by AETHER and is not the distribution configuration.
- The build to review is a separate, pinned, shared FFmpeg, with headers, static libraries disabled, GPL components excluded unless separately approved, and non-free components excluded unless counsel approves them. Enabled and disabled components and the licence configuration of that pin must be recorded. No such pin has been built. `docs/technology/FFMPEG_DEVELOPMENT_CONFIGURATION.md` is the baseline description.

## Facts recorded 2026-11-20

Observations, not a clearance. Detail: `docs/technology/LICENSING_AND_DISTRIBUTION_REVIEW.md` and `docs/validation/FFMPEG_LIBRARY_INTEGRATION_VALIDATION.md`.

- `LICENSE` is still empty.
- `/usr/bin/ffmpeg` is FFmpeg 9.0.2, package `ffmpeg-9.0.2-3.fc45.x86_64`, shared libraries `ffmpeg-libs-9.0.2-3.fc45.x86_64`. `ffmpeg-devel` is not installed. `/usr/include/ffmpeg` is absent. `pkg-config` does not see the approved libraries. The link harness did not configure.
- A numbered-frame CLI session wrote synthetic MXF under `validation-fixtures/ffmpeg-mxf-numbered-2026-11-20/`. A later sample-boundary session wrote more synthetic files under `validation-fixtures/audio-sample-boundary-2026-11-20/`. Neither session is permission to ship an encoder.
- No third-party source was copied into `third_party/`.

The empty-`LICENSE` observation is the 2026-11-20 fact. On 21 November 2026 the owner directed the Apache-2.0 text now in `LICENSE`. That later fact does not change the FFmpeg observations above.

## Risks

- Shipping or linking this machine’s FFmpeg and describing it as an LGPL-only component. The printed flags do not support that description.
- Treating dynamic linking, a CLI round trip, or a failed link configure as a licence pass.
- Treating the 21 November 2026 Apache-2.0 source direction as a clearance of Qt, FFmpeg, ProRes, DNx, AAF, or trademarks. It is not.

## Next actions

1. The owner sends `docs/technology/LEGAL_COUNSEL_QUESTIONS.md` with the development-configuration baseline. Engineering does not answer it.
2. No AETHER package copies `/usr/lib64/libavcodec.so.*` or vendors FFmpeg into `third_party/` before this ADR moves.
3. No release document says a codec is available on the strength of this ADR.

## Consequences

- No release document may say a codec is available on the strength of this ADR.
- `LICENSE` and `NOTICE` record the owner’s source-licence direction. They do not close this ADR.
- `third_party/` stays empty until counsel and the owner move the third-party rows.
- Status stays **OPEN — PENDING LEGAL COUNSEL**.

## Alternatives

On 2026-11-20, project-licence families, including Apache-2.0, were unadopted. On 21 November 2026 the owner directed Apache-2.0 for AETHER source. Other licence families were not selected. This ADR does not treat that direction as counsel’s acceptance of the open rows.

## Confirmation

None. Left open on purpose.
