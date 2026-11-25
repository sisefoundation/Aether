# Questions for legal counsel

Status: unanswered. **OPEN — PENDING LEGAL COUNSEL.** Engineering does not answer these. ADR-008 is not accepted. Apache-2.0 is a proposal only. It is not adopted. Dynamic linking does not, by itself, clear the obligations below.

Send this list with `docs/technology/LICENSING_AND_DISTRIBUTION_REVIEW.md`, `docs/technology/FFMPEG_DEVELOPMENT_CONFIGURATION.md`, and the configuration print in `/home/editry/development/sise-editor/validation-fixtures/ffmpeg-mxf-numbered-2026-11-20/logs/run.log`. Do not send it as a statement that a build is already cleared.

## Intended dependency direction, for review and not as a clearance

The owner has accepted an architectural direction. Counsel is asked to review this direction. Nothing in it is a legal determination.

- AETHER’s own licence is unset. Apache-2.0 may be discussed as a proposal. It is not the licence of the tree. `LICENSE` is empty.
- The media engine, when a pin exists, dynamically links only `libavutil`, `libavcodec`, `libavformat`, `libswscale`, and `libswresample`, inside the AETHER Media Engine.
- `libavfilter` is not in that set.
- The Fedora FFmpeg 9.0.2 GPL build on this machine (`ffmpeg-9.0.2-3.fc45`, `--enable-gpl`, `libfdk-aac`, DeckLink, `libx264`, `libx265`, no `--enable-nonfree` token in the print, no development headers) is not the build AETHER intends to redistribute. It is an exhibit.
- The build to review is a pinned shared build with headers, GPL components excluded unless separately approved, and non-free components excluded unless counsel approves them. That pin has not been built. `docs/technology/FFMPEG_DEVELOPMENT_CONFIGURATION.md` records the baseline.

## A. AETHER’s own licence, contributors, and packaging

1. Which licence will AETHER itself use: a strong copyleft, a weak copyleft, or a permissive licence? Apache-2.0 is only a proposal. The dependency audit names effects and does not pick a row.
2. Who is the copyright holder on the text and, later, the code? What contributor IP policy applies, and is a Developer Certificate of Origin enough, or are extra contributor documents required?
3. May an empty `LICENSE` file ship with a private prototype that is not distributed? The file is empty today. This question does not authorise filling it.
3a. What trademark and asset-ownership terms apply to the AETHER name, the documentation, and any later icons or fonts?
3b. What binary-packaging obligations follow from the licence chosen in question 1 once Qt and the FFmpeg pin are actually linked?

## B. FFmpeg copyright and build flags

4. The Fedora binary reports FFmpeg 9.0.2 with `--enable-gpl`, `--enable-shared`, `--disable-static`, `--enable-version3`, `--enable-libfdk-aac`, `--enable-decklink`, `--enable-libx264`, and `--enable-libx265`. The printed configuration does not contain the token `--enable-nonfree`. What obligations follow from that combination?
5. May AETHER ship a dynamically linked FFmpeg that is GPL, next to the project licence chosen in question 1?
6. What flag set would counsel accept for a project-built shared library if the goal is a weak-copyleft application: in particular `--disable-gpl`, no `--enable-nonfree`, no `libfdk-aac`, no DeckLink, no `libx264`, no `libx265`?
7. Does linking `libavcodec`, `libavformat`, `libavutil`, `libswscale`, and `libswresample` change the answer relative to linking `libavfilter`, `libavdevice`, or a GPL filter plugin?
8. Is the `ffmpeg` command-line tool a separate work from those libraries for redistribution purposes if AETHER never ships the CLI and only links the libraries?

## C. Patents and codec distribution

9. For Apple ProRes: may AETHER distribute an encoder, a decoder, both, or neither? Is decoding a file the user already has a different answer from shipping `prores_ks`?
10. For VC-3 / the profiles this FFmpeg calls DNxHR: the same question, for the `dnxhd` encoder.
11. Do SMPTE ST 378, ST 390, ST 2019, RDD 44, or ST 382 convey a patent licence AETHER can rely on? The specifications cite them as technical targets. Citation is not a licence.
12. PCM at 48 kHz, 24-bit, little-endian: any distribution issue counsel wants separated from the picture codecs?
13. If an encoder must stay out, is a build that contains the encoder symbol but hides it in the application still a distribution of that encoder?

## D. Trademarks and UI words

14. May the words ProRes, DNxHR, and DNxHD appear in the user interface, in documentation, and in error text?
15. If not, is “VC-3”, “vendor codec”, or another phrase the required wording?
16. FFmpeg’s own muxer writes `company_name=FFmpeg`. May AETHER leave that identification set as-is, or must a project writer replace it before a file leaves the machine? `docs/NATIVE_MEDIA_ARCHITECTURE.md` already recommends not writing the ProRes vendor id `apl0`.

## E. Qt and other components

17. Which Qt 6 modules may be linked, under LGPL, and under a commercial Qt licence? QML is not in the current proposal. Qt Multimedia should be called out, because it can pull another media stack beside FFmpeg.
18. Dynamic versus static linking of Qt for a first Linux package.
19. OpenColorIO (commonly BSD-3-Clause, unconfirmed on a pin) and OpenTimelineIO (commonly Apache-2.0, unconfirmed on a pin): any conflict with the licence chosen in question 1?
20. SQLite’s public-domain dedication: confirm it on the amalgamation actually vendored, if one ever is.
21. AAF: which codebase, if any, has a licence SISE can accept? A writer written by the project needs a licence of its own.
22. DeckLink SDK redistribution. The proposals leave it off. Is “off” enough, or is a click-through still required to compile a disabled stub?
23. Font licence for a UI typeface. Unchosen. The Adobe font packages on this Fedora machine are not Adobe Premiere and are not a UI selection.

## F. What counsel is not asked to decide

- Whether a bin is a folder. It is not.
- Whether float seconds are the cut. They are not.
- Whether a local FFmpeg open is a conform. It is not.
- Whether this review’s CLI round trip passed a vendor. It did not run one.
- Whether 12812 or 12813 is the editorial sample count. That is an engineering contract. It is not a licence question.

## G. Items the owner left open on 2026-11-20

These names are the open list. The numbered questions above are the same list in more detail. None is answered.

- AETHER source licence
- contributor IP policy
- DCO and extra contributor documents
- Qt distribution obligations
- FFmpeg build LGPL or GPL, for the intended pin rather than for the Fedora exhibit alone
- ProRes patents
- DNx patents
- AAF implementation licensing
- third-party obligations
- trademarks and asset ownership
- binary packaging
