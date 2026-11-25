# Contributing

AETHER (this repository, `sise-editor`) is in architecture preparation. The specifications are proposals. They are not an approval to build the editor.

## Current work

- Read `README.md` and `docs/ARCHITECTURE_AUDIT.md` before changing a contract.
- The document that already owns a subject is the one to edit. `docs/SYSTEM_INTEGRATION_AND_CONSISTENCY.md` lists owners. Do not add a second bin, timeline, audio, MXF, or conform specification.
- Label claims **CONFIRMED**, **PROPOSED**, or **OPEN**. Do not mark a proposal as confirmed, and do not mark an ADR accepted, unless a review from the maintainers says so.
- Do not claim that DaVinci Resolve, Avid Media Composer, Adobe Premiere Pro, or Avid Pro Tools can open a file. Compatibility cells stay “not executed” until a log names the application version and the fixture.
- Notes about FFmpeg 9.0.2 in the media documents are a local study to reconcile. Do not add them again as a new test result.
- Codec redistribution, Qt licensing, and AAF licensing are open legal questions. Do not decide them in a pull request.
- `LICENSE` is empty. Do not paste a licence into it as part of an unrelated change.

## What not to change yet

- Do not add application features, engines, or a filled-in build until the Foundation Prototype start condition in `docs/FIRST_IMPLEMENTATION_MILESTONE.md` is met.
- Do not vendor third-party source into `third_party/`.
- Do not silently modify, move, or delete original media in any tool or test.
- The cutlist project is a different repository. Do not mix it into this tree.

## Tests

`tests/` is empty. A future test belongs with a validation record, as `docs/17_TESTING_AND_VALIDATION.md` describes. A specified test is not a pass.

## Empty stubs

`docs/ARCHITECTURE.md`, `docs/DEVELOPMENT_ROADMAP.md`, `docs/MODULES.md`, `docs/MEDIA_PIPELINE.md`, and `docs/PROJECT_FORMAT.md` are empty on purpose relative to the full documents at the repository root and elsewhere under `docs/`. Do not overwrite a substantive specification. If a stub is still empty, prefer a pointer over a second copy of a long contract.
