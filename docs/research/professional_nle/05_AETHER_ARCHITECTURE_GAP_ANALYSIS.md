# Aether architecture gap analysis

Inspection date: Sunday 22 November 2026, Asia/Calcutta. Statuses are only the seven names in the brief. A row cites a file or a validation report. Professional insert is not implemented. Playback is not implemented.

Accepted and left accepted: ADR-002, ADR-003, ADR-005. ADR-001 keeps FFmpeg as the framework direction. The library link is still blocked. ADR-006 and ADR-007 stay proposed. ADR-008 stays open. This file does not accept any of them and does not adopt MLT.

## Editorial operations

| Capability | Status | Evidence |
| --- | --- | --- |
| Same-rate place of one instance, overlap refused, wrong rate refused | IMPLEMENTED AND VALIDATED | `docs/validation/SLICE_1D_TIMELINE_CLIP_PLACEMENT.md`; `ProjectSession::placeInstance` |
| Menu label "Insert into Timeline" | IMPLEMENTED AND VALIDATED as a label for placement | `src/ui/main_window.cpp`. It is not splice-in. |
| Professional insert / splice (later material moves) | NOT IMPLEMENTED | `placeInstance` does not shift other rows |
| Overwrite that splits covered instances | NOT IMPLEMENTED | Overlap is refused |
| Replace by sync frame, record span kept | NOT IMPLEMENTED | No command |
| Lift of one whole instance (hole, no shift) | PARTIALLY IMPLEMENTED | `removeInstance` deletes one row. No marked sub-range, no clipboard |
| Extract / delete that closes the gap | NOT IMPLEMENTED | No shift on remove |
| Split at a record frame | NOT IMPLEMENTED | No command |
| Single-roller, dual-roller, slip, slide | NOT IMPLEMENTED | `trimInstance` changes spans without roller rules. Slice 1D trimmed one instance's source and record together. That is not a roller mode. |
| Three-point marks in source and record monitors | NOT IMPLEMENTED | One sequence view. No source monitor. |
| Track targeting, patching, lock, monitor, sync lock | NOT IMPLEMENTED | `tracks` has id, sequence id, and ordinal |
| Bins as a professional browser | PARTIALLY IMPLEMENTED | Tables exist. No validated bin workflow. |
| Keyboard map | NOT IMPLEMENTED | No stored map |
| Session undo of placement, move, trim, remove | IMPLEMENTED AND VALIDATED inside one session | Slice 1D three undos and three redos |
| Undo after close | NOT IMPLEMENTED | Slice 1D: after reopen, undo and redo were not sensitive. ADR-009 for the foundation prototype is session-only. |
| Playback | NOT IMPLEMENTED | Window text "No media loaded. No playback." |
| Probe, proxy, relink, interchange | DEFERRED | Behind ADR-006, ADR-007, and the FFmpeg link block |
| Mixed-rate conversion | NOT IMPLEMENTED | Refused. See the timing note. |
| MXF audio packet that matches the 12812-sample contract | NOT IMPLEMENTED | The encoder test remains failed |

## Data model

| Capability | Status | Evidence |
| --- | --- | --- |
| One `project.db` | IMPLEMENTED AND VALIDATED | ADR-003; project tests |
| Schema version 2 | IMPLEMENTED AND VALIDATED | `kPrototypeSchemaVersion = 2`. `kPrototypeVersion` is still `slice-0a`. |
| Media ID distinct from Representation ID and path | IMPLEMENTED AND VALIDATED | ADR-005 |
| Timeline instance stores source rate and both half-open spans | IMPLEMENTED AND VALIDATED | `timeline_instances`; Slice 1D row survived reopen |
| Same-track overlap rejected | IMPLEMENTED AND VALIDATED | Placement and move |
| Track type, lock, mute, target, patch | NOT IMPLEMENTED | No columns |
| Filler object for a lift hole | NOT IMPLEMENTED | A hole is the absence of a row |
| Fingerprint / known-same media | DEFERRED | ADR-005: unconfirmed match waits for a person |

## What the reference programs do that Aether does not store

Avid's Track Selector, sync lock, and Clipboard, and Lightworks's track targets, lock, monitor, and amber sync strip, are product behaviour. Mapping them onto Aether columns is an architecture decision, not a transcription. Patching in particular needs a picture/sound component model that ADR-002 and the current schema do not contain. That row stays **REQUIRES ARCHITECTURAL DECISION**.

Ripple (insert and extract) changes other tracks only under a sync rule. Avid and Lightworks state different rules. Choosing one is **REQUIRES ARCHITECTURAL DECISION**. Until that choice, the operations are not scheduled as code.

Copying Kdenlive, Blender, Shotcut, or MLT implementations into this tree is **REQUIRES LEGAL REVIEW** under ADR-008, and is out of scope regardless. The study cites paths and licences. It does not vendor those trees.

## Validation boundary

Slice 1D is complete. It is not reopened by this research. The executable used as evidence is `/home/editry/development/sise-editor/build-slice1d/src/aether`. This pass did not launch it and did not re-run CTest. The totals above are the validation document's totals, checked against the test slot lists.
