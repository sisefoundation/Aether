# Professional NLE research index

Inspection date: Sunday 22 November 2026, Asia/Calcutta. Research only. No production source, schema, test, ADR, or project database was edited.

| File | Subject |
| --- | --- |
| `01_PROFESSIONAL_WORKFLOW_BENCHMARK.md` | Media Composer 2026.8 Editing Guide sections that were read, and the three Lightworks articles |
| `02_OPEN_SOURCE_SOURCE_CODE_STUDY.md` | Kdenlive, Blender VSE, Shotcut, MLT at the pinned commits |
| `03_AETHER_PROFESSIONAL_EDITORIAL_MODEL.md` | Proposed contracts. Not an accepted ADR. |
| `04_TIMING_AND_MIXED_RATE_GAP_ANALYSIS.md` | Same-rate identity, mixed-rate refusal, audio floor counts |
| `05_AETHER_ARCHITECTURE_GAP_ANALYSIS.md` | Gap statuses against the current tree and Slice 1D |
| `06_PROFESSIONAL_NLE_DEVELOPMENT_RECOMMENDATION.md` | Proposed next order. Does not restart Slice 1D. |

The five files already in `docs/research/` were not edited.

## Pinned checkouts

Directory: `/tmp/aether-professional-nle-study/`. Each is a detached `HEAD`. The fetch was `git fetch --depth 1 origin <sha>`. No branch name was attached. The earlier clones were absent and were fetched again into that directory.

| Tree | SHA | Commit timestamp |
| --- | --- | --- |
| kdenlive | `f5ad17c990589ffba60eb31a875f70ec338407d1` | 2026-11-21 18:40:36 +0100 |
| blender | `adc04d05b693a26fc6567357f4c456b60d0221a8` | 2026-11-21 15:38:32 +0100 |
| shotcut | `2eba56a72af3b7806876f2a2956cfff789db1468` | 2026-11-21 10:21:37 -0800 |
| mlt | `34088d6f47ae8896ab71101ddd72f600b3a18f8d` | 2026-11-20 16:36:02 -0800 |

Blender is a sparse checkout: `source/blender/sequencer`, `source/blender/editors/space_sequencer`, `source/blender/makesdna`, and `COPYING`.

## What was not read

- Media Composer: the step-by-step bodies for patching, track monitoring, lock versus sync lock, entering trim mode, roller selection, multi-track trim, and segment-tool hit regions. The table of contents names those chapters. The extract did not yield their procedures. They are unknown. Relink and the full keyboard map were not read. Mixed-rate Motion Adapter text was not re-read in this 2026 pass.
- MediaCentral was not used.
- Lightworks: mixed-rate behaviour, internal time format, and the auto-save mechanism.
- Kdenlive: the audio-mirror tail of `requestClipInsertion` and the rest of `insertClip` after the zone-length line.
- Blender: `blenloader`, `doc/license/GPL-license.txt`, and therefore blend-file persistence of a strip. `COPYING` points at the unread GPL file.
- Shotcut and MLT: only the insert, overwrite, tractor, playlist, and position paths named in document 02.
- This pass did not launch `build-slice1d/src/aether` and did not re-run CTest. Slice 1D totals are taken from `docs/validation/SLICE_1D_TIMELINE_CLIP_PLACEMENT.md` and checked against the test slot declarations.

## Status

PROFESSIONAL NLE RESEARCH PARTIALLY COMPLETE — VERIFIED FINDINGS AND REMAINING RESEARCH GAPS DOCUMENTED — NO PRODUCTION CODE CHANGED.
