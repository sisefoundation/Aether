# AETHER Avid media compatibility

Status: proposal for review. This document does not claim that Avid Media Composer will import, link, or play any file. No Media Composer test was run. Compatibility is a later test against published Avid and SMPTE behaviour, and it is not executed.

## Verified facts

- The brief requires two folder systems. AETHER’s project tree and Avid’s media tree are different. Avid delivery is an export, not AETHER’s internal layout.
- The brief requires that only validated OP-Atom MXF be written into `Avid MediaFiles/MXF/<numbered folder>/` at a volume root, and only as a deliberate export.
- The brief forbids placing OP1a, AAF, or arbitrary files in that Avid folder, forbids overwriting existing Avid media without confirmation, and forbids editing Avid’s database files.
- A prior local study of FFmpeg 9.0.2, not run for this document, reported that OP-Atom was optional only in that build. This specification does not treat FFmpeg as a proven OP-Atom writer.
- SMPTE ST 390 defines OP-Atom MXF. SMPTE ST 378 defines OP1a. Those are published standards. Citing them does not mean a file AETHER might write conforms.

## Engineering recommendations

- Ship the Avid writer as unavailable until QA records a conformance pass on the actual bytes: operational pattern, one essence per file, index, and the folder location.
- Do not infer conformance from the `.mxf` extension.
- Keep AAF exports out of `Avid MediaFiles`. AAF belongs in `EXCHANGE/AAF/` or in a user-chosen delivery folder that is not the MXF media directory.
- Leave `msmFMID.pmr`, `msmMMOB.mdb`, and any later Avid database filenames untouched. Media Composer manages them. AETHER does not rewrite them to “refresh” a bin.

## Proposed AETHER behaviour

### Two trees

```text
AETHER project (authority)          Avid delivery (export only)
ProjectRoot/                        VolumeRoot/
  project.db                          Avid MediaFiles/
  MEDIA/                                MXF/
    MANAGED/                            1/
    PROXIES/                            2/
  EXCHANGE/AAF/                         ...
```

Daily editorial files stay in the AETHER tree. An assistant may run “Export Avid media” and choose a volume. AETHER writes only files that passed OP-Atom validation into the next numbered folder it is allowed to use.

### Write rules

| Rule | Behaviour |
| --- | --- |
| Pattern | OP-Atom only, after validation |
| Destination | `VolumeRoot/Avid MediaFiles/MXF/N/` where `N` is a numbered folder |
| Forbidden payloads | OP1a MXF, AAF, AVX, WAV as a stand-in, proxies, project databases, any file that failed validation |
| Existing files | Never overwrite. If the destination name exists, pick a new name or stop and ask. Default: stop and ask. |
| Avid databases | Not created, edited, deleted, or “repaired” |
| Confirmation | The export dialog states the volume, the folder, the file count, and that Media Composer compatibility is not guaranteed until a named test pass exists |

If the processing backend cannot write OP-Atom, the command is disabled and the dialog says why. The prior study is sufficient reason to keep the command disabled in any build that has not re-validated a writer. Disabling is the specified behaviour, not a test result.

### What AETHER will not do

- Store working media only in `Avid MediaFiles` and treat that as the project.
- Convert an internal OP1a mezzanine by renaming it into an Avid folder.
- Delete Avid media as part of project cleanup.
- Claim that a bin, a master clip, or a sequence will appear in Media Composer because an MXF file exists.

### Picture and sound

OP-Atom practice separates picture and sound into different files. The export plan, when a writer exists, emits one essence per file and records the map in an AETHER delivery report. Audio inside that MXF is limited to profiles the writer has validated. The prior study’s PCM constraints (48 kHz, 24-bit little-endian) and missing channel layout are inputs to that profile. They are not a certificate.

### Delivery report

Every export writes a report under `LOGS/` and a copy beside the AAF if an AAF was requested:

- source media ids
- output paths
- validation result
- known gaps (layout not in MXF, alpha unverified, sample-count warning)
- statement that Media Composer was not tested, until a test record exists

### Specified tests, not executed

- A file with an `.mxf` extension and an OP1a pattern is refused for the Avid folder.
- A destination that already contains a file of the chosen name does not overwrite.
- Export does not change mtime or contents of pre-existing Avid database files. Measurement method: hash those files before and after, on a fixture volume, when a test environment exists.
- A build without a validated OP-Atom writer exposes a disabled command, not a best-effort mux.

## Rationale

| Decision | Rationale |
| --- | --- |
| Separate export | Avid’s folder is a managed media database with its own rules. Using it as AETHER storage couples every save to those rules and risks other projects on the volume. |
| Validation gate | The prior FFmpeg note says OP-Atom was optional. Writing unvalidated MXF into an Avid folder is how cutting rooms corrupt media databases. |
| No database edits | Those files are Avid’s index. An outside rewrite can hide or duplicate media without a specification AETHER owns. |
| No overwrite | Avid media file names can collide across projects. Overwrite destroys another show’s essence. |

## Unresolved questions

- Which numbered folder to choose when several exist. Recommendation: the highest existing number the user confirms, or `1` if none exist, and never create a folder above `1` without showing the path.
- Whether a later validated writer is FFmpeg, a dedicated MXF library, or a refusal to support Avid media until one exists. Open. Licence and patent review applies (`DEPENDENCY_AND_LICENCE_AUDIT.md`).
- Tape name, project name, and physical source metadata that Media Composer displays. Specify after a published Avid media-metadata reference is chosen. Do not invent a private tag and call it compatible.
- OP-Atom audio channel splitting rules for more than the validated PCM profile. Blocked with the writer.
