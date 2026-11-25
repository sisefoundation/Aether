# Undo durability evaluation

Status: evaluation that the owner has since accepted for the Foundation Prototype. ADR-009 is **ACCEPTED FOR FOUNDATION PROTOTYPE**. No undo stack was built and no crash was induced. Persistent undo remains future work.

Owners: `docs/15_UNDO_RECOVERY_AND_SAFETY.md`, `docs/RECOVERY_AND_BACKUP.md`, `docs/BIN_REVISION_AND_RECOVERY.md`, `docs/BIN_DATABASE_SCHEMA.md` (`OperationHistory`), `ARCHITECTURE.md` §5.4, `docs/PROJECT_DATABASE_SCHEMA.md`. Audit conflict C5.

## 1. Five mechanisms

They are different stores. Collapsing them into one “history” is how a restart either loses the cut or pretends an audit row is an undo.

| Mechanism | What it is | What it is not | Lifetime in the recommendation |
| --- | --- | --- | --- |
| Undo | The session stack of inverse editorial commands. Undo and redo restore composition hashes during one run. The UI does not keep a private copy of the cut | Not autosave, not a snapshot, not a journal, not a rewrite of camera bytes | This process. The Foundation Prototype requires this (capability 13) and does not require it after restart |
| Autosave | Flush of an already accepted database transaction, a refresh of the manifest hash, and a small note of the time. Interval in the recovery document is a recommendation (60 seconds and on idle) and is unmeasured. Autosave does not copy media | Not the undo stack. Not a second `autosave.sqlite` unless he explicitly keeps that earlier mechanism. Not a snapshot | The last successful transaction |
| Crash recovery | The open-time procedure when the lock is stale, the manifest disagrees, or integrity fails. It offers continue or a snapshot. It does not start playback and it does not pick a file silently | Not undo. Not a conform. Not deletion of originals or of cache | On the next open |
| Snapshot | A copy of `project.db`, the manifest, and settings, named with UTC time, under `BACKUP/`. Opened as a new copy until the person promotes it. Excludes originals unless a separate archive command was run | Not autosave. Not the live database. Not version history of the media files | The copies he keeps. Retention counts are **further research** |
| Journal | A log that an operation was committed: the autosave time note, and, if kept, `OperationHistory` (`operation_id`, bin, kind, `payload_json`, revision ids, time). `payload_json` names ids and previous field values. Clips stay in the typed tables | Not the session undo stack. Not sufficient for restart-undo unless the inverse record and its size bound are specified. Not a snapshot of the whole project | Append-only record of commits. Whether `OperationHistory` ships in the prototype is **further research** |

`docs/BIN_FILE_FORMAT_SPECIFICATION.md` revisions (revision id, parent id, member hashes) are portable bin checkpoints. They are closer to a snapshot of one bin than to session undo. They are not a fifth name for the same stack.

## 2. Recommendation

**Recommended for adoption** for the Foundation Prototype, not accepted:

- Session undo only. Capability 13 stands. Tests reopen the last commit. They do not require the pre-crash undo stack.
- The durable document is the last successful `project.db` transaction, cross-checked against the manifest.
- An interrupted autosave leaves that last commit openable (capability 15). The mechanism is the engine’s transaction, not a hand-built second copy of every clip in the UI process.
- Originals are outside undo. Undo never restores a cut by rewriting camera bytes.

**further research**, and not required to start the prototype:

- Restart-undo. A review that wants it has to specify the inverse record and a bound on its size before any code stores one. `OperationHistory.payload_json` is the column the bin schema offers. Using it as the undo stack without that bound is not recommended.
- Whether the early `autosave.sqlite` file remains as a second mechanism. This recommendation uses one database and the engine’s atomic commit. Keeping a second file is the alternative he can still choose.
- Snapshot interval and retention (the recovery document’s “last 20 or 14 days”).
- Decoder crashes during recovery. Isolation is open. Recovery must not auto-delete project files.

**not recommended:**

- Treating a journal row as proof that undo still works after a restart.
- Treating a snapshot restore as an undo command.
- Letting autosave replace a good database with a partial file.
- Putting float seconds into the undo record as the authority for a range. The composition hash covers rates, ranges, ids, and marker frames (`docs/FIRST_IMPLEMENTATION_MILESTONE.md`).

## 3. Why the split matches both existing texts

`ARCHITECTURE.md` §5.4 says undo and redo are session state and need not survive a restart. The later schema stores operation history so a later investigation can see gestures. Both sentences can be true when the stack and the journal are different. This recommendation is that reading. It does not close C5. Subhajit closes it by accepting the split or by requiring restart-undo.

## 4. What Subhajit is asked

Choose session-only undo for the prototype, or require undo after a restart and specify the inverse record first. The other four mechanisms stay in the product either way. They are not substitutes for the choice.
