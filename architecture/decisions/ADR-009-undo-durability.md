# ADR-009 — Undo for the Foundation Prototype

- Status: **ACCEPTED FOR FOUNDATION PROTOTYPE**
- Date: 2026-11-20
- Deciders: project owner.
- Related: `docs/15_UNDO_RECOVERY_AND_SAFETY.md`, `docs/technology/UNDO_DURABILITY_EVALUATION.md`, `docs/RECOVERY_AND_BACKUP.md`, `docs/FIRST_IMPLEMENTATION_MILESTONE.md`

## Context

The early architecture treated undo as session state. Later schema sketches stored operation history in the database and could be read as if that history were the undo stack. Those mechanisms are different. The owner has accepted a scope limit for the Foundation Prototype and has kept the other mechanisms independent.

## Decision

**ACCEPTED FOR FOUNDATION PROTOTYPE.** Undo and redo for the Foundation Prototype are session-only. The undo history need not survive close and reopen for this milestone.

These are independent. They must not be conflated:

| Mechanism | What it is in this decision |
| --- | --- |
| Undo / redo | Inverse commands for the running process. Restores the editorial model inside the session. |
| Autosave | The policy that flushes a committed project transaction and the manifest. Not the undo stack. |
| Crash recovery | Reopen of the last committed `project.db`. Not the undo stack. |
| Snapshots | Copies under `BACKUP/`. Not the undo stack. |
| Operation journals | A record that a commit happened, including `OperationHistory` if a later schema keeps it as a log. Not the undo stack and not a snapshot. |

Session-only undo is not session-only project persistence. A completed editorial operation must remain safe in project storage under the persistence and recovery rules in ADR-003. Closing the application may drop the undo stack. It must not drop the last committed edit.

The prototype requires:

- command-based undo and redo
- transaction boundaries, so a command commits or leaves the database unchanged
- compound operations as one undo step
- safe undo of the supported timeline operations
- no corruption of database references
- autosave independent of undo
- crash recovery independent of undo
- a project that reopens safely onto the last commit

Persistent undo is future work. It is not required for this milestone.

### When to reconsider

Reconsider persistent undo before any release that promises undo after relaunch, and not before the prototype has a working session stack and a written inverse record with a size bound. Until that review, do not store undo as rows that crash recovery treats as the cut.

## Consequences

- Audit conflict C5 is closed for the prototype. Restart-undo remains future work, not an open fork of the prototype.
- Tests reopen the last commit. They do not require the pre-crash undo stack.
- A snapshot restore, an autosave, or a journal row is not an undo command.

## Alternatives

- Full undo history in `project.db` for the prototype. **Not required.** Not accepted for this milestone.
- Treating autosave or crash recovery as the undo stack. **Not accepted.**

## Confirmation

Accepted by the project owner on 2026-11-20 for the Foundation Prototype only. The scope limit is intentional.
