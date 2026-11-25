# ADR-006 — The application is the only joiner

- Status: **PROPOSED**
- Date: 2026-11-20
- Deciders: not decided.
- Related: `docs/ENGINEERING_INTERFACES.md`, `ARCHITECTURE.md` §2.2, `docs/SYSTEM_ARCHITECTURE.md`, `docs/SOFTWARE_MODULES.md`

## Context

If the timeline calls a decoder, a codec change changes trim code. If media management calls the editorial command API, a relink can move a cut. If a plugin receives the database path, a third party can bypass both rules.

The later layering also splits AETHER SOUND out of processing. The early architecture keeps interactive playback, including device audio, inside Processing. That split is part of this ADR and is not yet accepted.

## Decision

**PROPOSED.**

- Editorial emits a composition slice: sequence frame, layers, source positions, weights, effect parameters, Media IDs. No paths.
- Media management emits locations: Media ID, role, path or offline. No permission to delete, and no sequence structure.
- The application builds a picture request and an audio graph request from those documents.
- Processing and AETHER SOUND do not call editorial commands and do not write the project database.
- Interchange returns documents and a loss report. It does not update live rows in place.
- Whether sound is a separate link target (`aether_sound`) or a backend inside Processing is **OPEN** as a build shape. The data split (slice versus graph request) is the proposal either way.

## Consequences

- A unit test can resolve a cut with every asset offline and with no codec linked.
- A processing test can render a request built by the test, with no timeline object in the process.
- Forbidden edges in `docs/ENGINEERING_INTERFACES.md` are part of review, not a style preference.

## Alternatives

- Let editorial open files “just for the duration”. Not proposed.
- Let GStreamer be the authority for which frame is current. Not proposed. The presented frame is the frame index the request carried.

## Confirmation

None.
