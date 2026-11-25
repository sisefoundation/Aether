# Open-source source-code study

Inspection date: Sunday 22 November 2026, Asia/Calcutta. Read-only checkouts under `/tmp/aether-professional-nle-study/`. Each tree is a detached `HEAD` at the pinned commit. `git fetch --depth 1 origin <sha>` did not attach a branch name. No upstream source was copied into the Aether tree. No function body is reproduced here.

| Project | SHA | Commit date on that object | Licence seen on the files cited |
| --- | --- | --- | --- |
| Kdenlive | `f5ad17c990589ffba60eb31a875f70ec338407d1` | 2026-11-21 18:40:36 +0100 | SPDX `GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL` on `timelinemodel.cpp`, `Timeline.qml`, `timelinecontroller.cpp`, `timelinefunctions.cpp`, and `macros.hpp` |
| Blender | `adc04d05b693a26fc6567357f4c456b60d0221a8` | 2026-11-21 15:38:32 +0100 | SPDX `GPL-2.0-or-later` on the sequencer files cited. Sparse checkout. |
| Shotcut | `2eba56a72af3b7806876f2a2956cfff789db1468` | 2026-11-21 10:21:37 -0800 | GPL-3.0 or later, Meltytech, on `timelinecommands.cpp`, `multitrackmodel.cpp`, `timelinedock.cpp`, and `Track.qml` |
| MLT | `34088d6f47ae8896ab71101ddd72f600b3a18f8d` | 2026-11-20 16:36:02 -0800 | LGPL-2.1 or later, Meltytech, on `mlt_tractor.c`, `mlt_playlist.c`, `mlt_producer.c`, `mlt_types.h`, and `modules/avformat/factory.c` |

Aether stays on its own licence track. ADR-008 is open. This study does not recommend adopting MLT, and it does not copy GPL or LGPL code.

## Kdenlive

Call chain that was re-read at this SHA:

1. `src/timeline2/view/qml/Timeline.qml` line 832 calls `timeline.insertClip(activeTrack, frame, clipBeingDroppedData, false, true, false)`.
2. `TimelineController::insertClip` is `src/timeline2/view/timelinecontroller.cpp` line 526. It calls `m_model->requestClipInsertion` at line 535.
3. `TimelineModel::requestClipInsertion` has two overloads in `src/timeline2/model/timelinemodel.cpp`, at lines 2017 and 2060. The first, on success and when `logUndo` is set, calls `PUSH_UNDO` at line 2050. On failure with `logUndo` it calls `undo()` and does not push. `PUSH_UNDO` is `src/macros.hpp` line 50.
4. At line 2121, inside the second overload, a bin id that carries an in and an out sets `duration = out - in + 1`. That range is inclusive. Aether spans are half-open `[start, end)`. The same numbers do not mean the same duration.
5. `TimelineFunctions::extractZone` is `timelinefunctions.cpp` line 827. Its undo label is "Lift zone" or "Extract zone" according to `liftOnly`. `insertZone` is at lines 861 and 876. `liftZone` is at line 961.

`TimelineController::ripple` at line 516 returns false. That is the body of that method at this commit. It is not a statement about every ripple path in the application.

What was not walked at this SHA: the body of `liftZone` after the opening cut, the body of `insertZone` after the locked-track loop starts, and `clipAudioStreamInfo` (called at line 2041). Those tails are **not fully traced**.

## Blender VSE

Sparse checkout only: `source/blender/sequencer`, `source/blender/editors/space_sequencer`, `source/blender/makesdna`, and `COPYING`. `source/blender/blenloader` and `doc/license/GPL-license.txt` were not checked out. `COPYING` points at that unread GPL file. Blend-file persistence of a strip was **not** read.

Editor and core are different directories. The editor registers operators. The core allocates strips.

Re-read at this SHA:

- `sequencer_ops.cc` line 119 calls `WM_operatortype_append(SEQUENCER_OT_movie_strip_add)`.
- `sequencer_add.cc` lines 1526–1542 define `SEQUENCER_OT_movie_strip_add`, idname `SEQUENCER_OT_movie_strip_add`, flags `OPTYPE_REGISTER | OPTYPE_UNDO`. Its exec is `sequencer_add_movie_strip_exec` at line 1439, which reaches `seq::add_movie_strip` at line 1334.
- `sequencer_drag_drop.cc` lines 922–923 register a drop box whose operator id is `SEQUENCER_OT_movie_strip_add`. Line 1038 registers the same id for the preview drop boxes. There is no `SEQUENCER_OT_drop_clips` in this file.
- `strip_add.cc` `add_movie_strip` begins at line 392. Around lines 450–460 it divides `fps_num` by `fps_denom` into a float `video_fps`. When `SEQ_LOAD_MOVIE_SYNC_FPS` is set it writes `scene->r.frs_sec` and `scene->r.frs_sec_base`. That is a scene frame-rate write, not an integer frame clock. Aether does not copy it.
- `sequencer_edit.cc` line 1276 defines `SEQUENCER_OT_slip`. The idname is set at line 1280.

`Strip` in `DNA_sequence_types.h` stores `start`, `startofs`, `endofs`, and `len` as integers. The DNA comment on `len` is the number of frames the strip covers, and the comment on `endofs` is an offset from the strip start. The inclusive or half-open reading of those four integers was not re-derived from the C that consumes them. Do not treat the field names as Aether's half-open rule.

## Shotcut

One insert and one overwrite, both `QUndoCommand` subclasses in `src/commands/timelinecommands.cpp`:

- `InsertCommand::redo` begins at line 253. It records undo state, builds an `Mlt::Producer` from stored XML, then at line 288 calls `m_model.insertClip(...)`.
- `OverwriteCommand::redo` begins at line 383. It snapshots the clips the overwrite will replace, then at line 421 calls `m_model.overwrite(...)`.
- The dock pushes them: `timelinedock.cpp` line 4452 and line 4532 push `InsertCommand`; line 4687 and line 4767 push `OverwriteCommand`.
- `MultitrackModel::insertClip` is at `multitrackmodel.cpp` line 1733. `OverwriteCommand` calls `MultitrackModel::overwrite` at line 1636, which returns `QString` and takes an `Mlt::Producer`. It does not call `overwriteClip`. `overwriteClip` is defined at line 1540 and has no other caller in the `.cpp` and `.h` files searched.

The model is an MLT tractor of playlists. `src/qml/views/timeline/Track.qml` line 163 sets a frame with `Math.round(clip.x / timeScale)`. That rounding is a view mapping. It is not a time authority and is not a pattern for Aether.

## MLT

- `mlt_position` in `src/framework/mlt_types.h` lines 249–257 is `double` when `DOUBLE_MLT_POSITION` is set, otherwise `int32_t`.
- `mlt_tractor_new` is `mlt_tractor.c` line 85. It allocates a tractor, initialises the parent producer, and creates a multitrack and a field.
- `mlt_playlist_insert` is `mlt_playlist.c` line 813. It appends with in and out, then moves the new entry to `where`. `mlt_playlist_append` at line 741 passes `get_playtime(producer) - 1` as the out point. `mlt_producer_get_playtime` at `mlt_producer.c` line 578 returns `out - in + 1`. Those end points are inclusive. Aether's half-open end is a different convention.
- `modules/avformat/factory.c` is the FFmpeg producer/consumer module. It is LGPL-2.1 or later. It is not an Aether dependency and not a reason to link MLT.

MLT is a reference for names (tractor, playlist, producer) only. The recommendation does not adopt it.

## What this study does not claim

It does not claim these projects use integer frames, reduced rational rates, or one SQLite file. It does not claim their undo survives a crash. It does not authorise copying any of the cited files.
