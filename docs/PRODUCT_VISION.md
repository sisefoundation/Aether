# AETHER product vision

Status: proposal for review. This document authorises no implementation.

AETHER is a professional, open-source, offline-first non-linear editor for feature films, television, documentaries, streaming originals, and commercials. It is an institutional initiative of the Society of Indian Screen Editors (SISE). It is not a consumer editor, a social-video tool, or a cloud service.

Guiding line: the editor should think about the cut, not about operating the software.

## Verified facts

- This document set was written without access to the Fedora project folder `/home/editry/development/sise-editor`. Nothing in that tree was read or confirmed.
- No application tests of DaVinci Resolve, Avid Media Composer, Adobe Premiere Pro, or Avid Pro Tools were run for this document. Compatibility with those applications is not claimed.
- A prior local study of FFmpeg 9.0.2, not re-run here, reported limits on MXF muxing. Those results are recorded in `NATIVE_MEDIA_ARCHITECTURE.md` and `DEPENDENCY_AND_LICENCE_AUDIT.md` as results to reconcile.
- Professional long-form editorial practice depends on stable media identity, rational time, non-destructive audio, and exchange that reports what it cannot carry. Those are requirements of the brief, not measurements from a finished AETHER build.

## Engineering recommendations

- Optimise every default for a cutting room: source and record monitors, a timeline, bins, and the keyboard. Do not optimise for first-run tutorials, templates marketplaces, or social export.
- Keep the product fully usable with no network, account, subscription, or activation. Optional later services, if any, must be off by default and must not gate file open, edit, or export.
- Treat original camera and sound media as irreplaceable. The application may create new files. It does not silently move, overwrite, rename, or delete originals.
- Design for Linux, Windows, and macOS. The first implementation target, when implementation is later approved, is Linux. Platform differences stay behind an infrastructure interface.
- Prefer a small, reviewable first prototype over a wide surface. The prototype scope is named in `DEVELOPMENT_ROADMAP.md`. It is not this vision.

## Proposed AETHER behaviour

AETHER is the editor a feature cutting room can run on a workstation that never phones home.

The people it is for:

- Picture editors cutting features, series, documentaries, and commercials.
- Assistant editors who ingest, organise, proxy, relink, and prepare turnovers.
- Sound editors and picture editors who need a serious multitrack timeline and a documented path into AAF, without pretending AETHER is a digital audio workstation.
- Departments and film schools that must be able to archive a project as files they control.

The people it is not for, in the first product: casual phone-video editors, live-switcher operators, and facilities that need a shared multi-user database on day one.

A session has no login. Creating a project writes a directory, a database, a manifest, and one sequence. Opening that directory on another machine, with the media or with media marked offline, is a complete open. There is no activation server to refuse the open.

The editor’s attention stays on the cut. The default workspace is the editorial layout in `UI_UX_DESIGN_SYSTEM.md`: bins, monitors, timeline, track controls, and a narrow meter column. Commands for insert, overwrite, lift, extract, trim modes, and navigation are first-class keyboard actions. Panels that are not part of the cut do not animate, badge, or upsell.

Media enters only by an explicit choice: link, copy, consolidate, or transcode. The project remembers identity separately from the path. If a volume disappears, the cut remains and the media is marked offline.

Picture exchange uses AVX, specified so another implementation can read it. Audio composition exchange uses AAF under an AETHER profile that states its limits. Other formats are adapters. None of them is the project.

Extensibility is a stable interface around the editorial model, media registry, and processing, not a permission for plugins to rewrite the database.

Institutional intent: SISE can publish the specification, accept review, and later accept implementation. This vision does not choose a licence. That choice is an unresolved legal decision in `DEPENDENCY_AND_LICENCE_AUDIT.md` because it changes who can ship codecs, plugins, and Qt.

## Rationale

| Decision | Rationale |
| --- | --- |
| Offline-first, no account | A cutting room and an archive must open ten years later without a vendor account. |
| Not a consumer editor | Feature workflows (reels, timecode, handles, relink, AAF, MXF) conflict with consumer defaults (auto cloud, silent transcode, destructive tidy). |
| Media-safe originals | Assistant editors lose days, and sometimes negatives of trust, when an app rewrites camera media. |
| Linux first, cross-platform design | The brief names Linux as the practical first platform and Windows and macOS as design targets. Encoding that split in the vision avoids a Linux-only data model. |
| AVX and AAF as exchange, not as the project | Round-trip through a foreign format drops intent. The native model stays the authority. |

## What “done” means for the product, later

A feature editor can create a project, link or copy camera and sound media, cut a two-hour timeline with rational time and undo, play picture and sample-accurate sound, relink after a drive change, and export an AVX picture exchange and an AAF audio exchange whose reports list anything not carried. They can do this with the network unplugged. Original files match the checksums taken at ingest.

That outcome is a product goal. It is not a claim that the goal has been built.

## Unresolved questions

- Which open-source licence SISE will use, and whether proprietary plugins are in scope.
- Whether the first public milestone is a picture-cut prototype or an assistant-editor ingest tool. The roadmap recommends a picture-cut prototype with link-only ingest.
- How much of finishing (colour, titles, mix delivery) belongs in AETHER rather than in neighbouring applications.
- Whether a SISE house keyboard map should resemble a particular commercial editor’s muscle memory. No commercial map is adopted here.
- Governance: who approves a change to AVX or to the project schema after the specification is accepted.
