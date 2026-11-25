# AETHER security and file safety

Status: proposal for review. No penetration test and no file-safety test was run.

AETHER’s safety problem is not an account system. It is irreversible damage to picture, sound, and project files.

## Verified facts

- The product has no mandatory cloud, account, subscription, or activation.
- Originals must not be silently moved, overwritten, renamed, or deleted.
- Proxies must not replace originals.
- Unvalidated media-record deletion is forbidden.
- Third-party folders, including Avid media databases, are not edited without an explicit export path, and Avid database files are not edited at all (`AVID_MEDIA_COMPATIBILITY.md`).
- Destructive actions need confirmation and a recovery path.

## Engineering recommendations

- Treat paths from media files, AVX, AAF, XML, OTIO, and EDL as untrusted. Resolve them inside the path policy before any write.
- Refuse writes whose destination is the same file as a source original unless the user is performing a named export into a different path.
- Prefer atomic rename for new files. Do not truncate an existing master in order to “update” it.
- Run decoders on data that may be hostile. Isolation is unresolved. Until isolation exists, a decoder crash must not be allowed to auto-delete project files during crash recovery.
- Do not phone home. If a later build adds update checks, they are off by default and are not this specification’s present behaviour.

## Proposed AETHER behaviour

### Prohibited without an explicit confirmed command

| Action | Rule |
| --- | --- |
| Delete, overwrite, rename, or move an original | Forbidden silently. A confirmed command may move or delete only a copy AETHER made, or a user-selected file that is not the last master, and only after stating the path. |
| Replace an original’s bytes with a proxy or a transcode | Forbidden |
| Delete a media record that clips still reference, or whose checksum was never validated, as a purge | Forbidden |
| Write into `Avid MediaFiles` except the validated OP-Atom export | Forbidden |
| Edit another application’s database or project | Forbidden |
| Follow a path hint from an exchange file and overwrite that path | Forbidden |

### Confirmation

A destructive dialog names the path, the media id, the sequence usage count, and whether a snapshot exists. Confirm is not the default button. The recovery path is the snapshot or the quarantine folder, and the dialog says which.

### Path policy

Writes are allowed only under storage roles the user configured, plus a temporary directory the platform module owns. Exchange import may read outside the project. It may not write there. A path that escapes a role via `..` or a symlink is refused until the user adds the resolved location as a role.

Camera cards are read-only in the path policy. Ingest can link or copy from them. It cannot format them, delete clips from them, or write proxies back onto them.

### Quarantine

Failed copies and failed transcodes move to a quarantine directory under the cache or temp role. Quarantine is not a master location. The user may delete quarantine in one confirmed command. That command does not scan originals.

### Process and crash

Crash recovery follows `RECOVERY_AND_BACKUP.md`. A crash handler does not “clean” media folders. It appends a log and leaves files for the next open.

### Network

The default application opens no socket. Optional later device control (DeckLink is local hardware, not a service account) and optional later update checks are separate, default off, and unapproved. Projects do not embed web credentials.

### Logs and privacy

Ingest logs contain paths, which may include personal names in folder titles. Logs stay on disk in the project. They are not uploaded. There is no telemetry account because there is no account.

### Specified tests, not executed

- Import an AVX file whose `path_hint` points at an existing original. AETHER does not open it for write.
- Proxy job destination equals the master path. The job is refused before launch.
- Delete command on a camera-card path is unavailable.
- Avid database file hashes are unchanged after a refused export and after a successful export to a different numbered folder, when those tests can be run.

## Rationale

| Decision | Rationale |
| --- | --- |
| Camera cards read-only | The most expensive silent write is onto the only copy of a shoot. |
| Untrusted exchange paths | An EDL or XML from outside is not a list of files AETHER may overwrite. |
| Confirm is not the default button | Enter-to-dismiss must not delete media. |
| No network in the default | The vision forbids a mandatory service. An idle socket is how that promise erodes. |
| Quarantine instead of leaving partial masters | Partial files get cut if they look finished. Quarantine keeps them out of the registry. |

## Unresolved questions

- Sandbox technology for decoders on Linux, Windows, and macOS. Open. Strongly recommended before untrusted camera files from third parties are a support burden. Not a prototype blocker if the prototype only opens files the user already has.
- Code signing and installation trust. Packaging is later. No claim is made.
- Project encryption for shared disks. Open. Default is the filesystem’s own permissions.
- Whether “archive and then delete originals” will ever exist. Recommendation: no in-application deletion of originals in the first releases. The user can delete in the file manager. Revisit only with a design that still cannot target a camera card.
- Maliciously large rational numbers or JSON in AVX. Readers must bound sizes and refuse. Exact bounds are open.
