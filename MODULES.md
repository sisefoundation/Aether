# SISE Editor Modules

Status: proposal, awaiting review. This document defines modules, public interfaces, and dependencies. It does not create libraries, headers, or a build system.

Read [ARCHITECTURE.md](ARCHITECTURE.md) for the reasoning and the data flows. Read [DEVELOPMENT_ROADMAP.md](DEVELOPMENT_ROADMAP.md) for the order of later work.

The modules that must remain separate are Media Management, Editorial, Interchange, Project Storage, and Processing. They do not depend on each other. They meet in Contracts, and the Application joins them.

## 1. Rules

1. A module depends only on modules named in its row of the matrix.
2. A dependency is a compile-time or link-time use of a public header or library. Private headers stay inside the module.
3. The graph is acyclic. The foundation sits at the bottom. The UI sits at the top.
4. Third-party types do not appear in public headers. FFmpeg, GStreamer, and OpenColorIO are private to Processing. OpenTimelineIO is private to Interchange. SQLite is private to Project Storage. Qt 6 is private to the UI.
5. Cross-module calls return the kernel result type. Exceptions stay inside a module.
6. Domain mutation runs on the session thread. Processing threads receive immutable requests.
7. When a build exists, each module is its own link target. The targets follow this matrix. That build is a later phase, and it is not part of the current change.

Contracts holds passive data and interface definitions so the five domain modules can stay apart. The fields of a timeline document are specified by the editorial design and housed in Contracts. The fields of a media catalog document are specified by the media design and housed in Contracts. A change to either document is reviewed with the module that specifies it.

## 2. Dependency matrix

"Yes" means the row module may depend on the column module. A blank is forbidden.

| Module | Kernel | Platform | Contracts | Plugin SDK | Editorial | Media | Interchange | Project | Processing | Plugin Host | Application | UI |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Kernel | | | | | | | | | | | | |
| Platform | Yes | | | | | | | | | | | |
| Contracts | Yes | | | | | | | | | | | |
| Plugin SDK | Yes | | Yes | | | | | | | | | |
| Editorial | Yes | | Yes | | | | | | | | | |
| Media Management | Yes | Yes | Yes | | | | | | | | | |
| Interchange | Yes | | Yes | | | | | | | | | |
| Project Storage | Yes | Yes | | | | | | | | | | |
| Processing | Yes | Yes | Yes | | | | | | | | | |
| Plugin Host | Yes | Yes | Yes | Yes | | | | | | | | |
| Application | Yes | Yes | Yes | Yes | Yes | Yes | Yes | Yes | Yes | Yes | | |
| UI | | | | | | | | | | | Yes | |

Allowed edges, listed for review:

- Platform → Kernel
- Contracts → Kernel
- Plugin SDK → Kernel, Contracts
- Editorial → Kernel, Contracts
- Media Management → Kernel, Platform, Contracts
- Interchange → Kernel, Contracts
- Project Storage → Kernel, Platform
- Processing → Kernel, Platform, Contracts
- Plugin Host → Kernel, Platform, Contracts, Plugin SDK
- Application → Kernel, Platform, Contracts, Plugin SDK, Editorial, Media Management, Interchange, Project Storage, Processing, Plugin Host
- UI → Application

Forbidden edges that the design depends on:

- Editorial, Media Management, Interchange, Project Storage, and Processing do not depend on each other.
- No domain module depends on Application, Plugin Host, Plugin SDK, or UI.
- Project Storage does not depend on Contracts. Payloads are opaque bytes.
- The UI does not include domain headers. It talks to the Application.
- Kernel and Contracts do not perform I/O and do not depend on Platform.

A Tier A plugin may depend on the Plugin SDK only. It is not in the matrix because it is not a core module. A Tier B panel is source inside the UI and may depend only on what the UI may depend on, plus Qt.

There is no cycle in the allowed edges. Foundation modules point at Kernel. Domain modules point at foundation. Plugin Host points at foundation and the SDK. Application points at domain and host. UI points at Application.

## 3. Kernel

**Responsibility.** Names, time, and errors. Nothing about editing, files, or pixels.

**Owns.** `Ratio`, `FrameRate`, `Range`, opaque 128-bit ids, the result and error type, a content-hash value type, and byte buffers used as opaque payloads.

**Does not own.** Tracks, assets, documents, paths, threads, or logging policy. Callers format logs. The kernel does not open a log file.

**Public interface.**

| Operation | Behavior |
| --- | --- |
| Reduce and compare ratios | Exact integer comparison. |
| Convert a position between two rates | Integer conversion with an explicit `nearest` or `floor` policy. |
| Format and parse a timecode label | Drop-frame is a labeling rule for the relevant rates. The frame index is unchanged. |
| Construct an id | Random local id, or an id parsed from stored bytes. |
| Return success or a structured error | Errors carry a stable code and a human-readable message. |

**May depend on.** Nothing in this product.

## 4. Platform

**Responsibility.** Operating-system services for Linux, shaped so a second implementation can exist later.

**Owns.** UTF-8 path values, directory listing, atomic replace, lock files, monotonic time, and shared-library load and unload.

**Does not own.** Media paths as catalog entries, plugin policy, or audio device selection. Audio devices stay inside Processing's playback backend.

**Public interface.**

| Operation | Behavior |
| --- | --- |
| Read, write, and replace a file | Replace is atomic on the local volume. |
| List a directory | Returns names and file or directory kind. |
| Acquire and release a lock file | Exclusive for one local process. |
| Load a library and resolve a symbol | Used by Plugin Host. Domain modules do not call it. |
| Read a monotonic clock | For timeouts and measurements. |

**May depend on.** Kernel.

## 5. Contracts

**Responsibility.** Passive types that cross module boundaries, and the C++ interfaces those modules implement. No edit operations, no I/O, no validation that belongs to a domain.

**Owns.** The document shapes and the service interfaces listed below. Structural checks that are properties of the types themselves (positive denominator, non-empty id) live next to the types. Overlap rules, relink identity, and OTIO lossiness do not.

**Does not own.** The in-memory indexes, the command set, the SQLite schema, or any codec.

**Public types.**

| Type | Contents |
| --- | --- |
| `ProbeRecord` | Streams, rational duration, rate, resolution, sample rate, channel count, color-space hint, timecode label, reel. |
| `MediaRecord` | Asset id, display name, kind (`file`, `image_sequence`, `synthetic`, `sequence`), identity fields, locations. |
| `MediaLocation` | Role (`master`, `proxy`), local path, online flag, rate, color-space name. |
| `MediaCatalogDocument` | Version and the list of media records. |
| `TimelineDocument` | Version, sequence id, name, rate, start frame, tracks, items, markers, effect parameters, color decisions, nested-sequence references. |
| `CompositionSlice` | One sequence frame: ordered layers with asset id, source position, transition weight, evaluated parameters, color-decision id. |
| `RenderRequest` | One sequence frame: the same layers with locations instead of bare asset ids, plus the bake policy and the destination description for export. |
| `PresentedFrame` | Sequence frame, asset id, source frame, and whether the layer was a slate because the asset was offline. |
| `RoundTripReport` | Items preserved, items stored only in the SISE metadata namespace, and items dropped. |
| `ColorDecision` | Config-relative path, input space, working space, display, view, grade parameters, bake policy. |

**Public interfaces.**

| Interface | Implemented by | Used by |
| --- | --- | --- |
| `IMediaProbe` | Processing, or a Tier A probe | Media Management calls it when the application has attached one. |
| `IEffectProcessor` | Processing internals, or a Tier A effect | Processing, for preview and export. |
| `IExchangeAdapter` | Interchange's built-in OTIO adapter, or a Tier A adapter | Interchange. |
| `IExportTarget` | Processing's built-in encoder, or a Tier A target | Processing. |
| `IPlayback` | Processing's GStreamer backend | Application. |
| `ISessionObserver` | UI | Application notifies the observer. The application does not depend on the UI. |

Interface methods take and return contract types and kernel types. `IMediaProbe` turns a local path into a `ProbeRecord`. `IPlayback` accepts a queue of `RenderRequest` values and reports `PresentedFrame` values. `IExchangeAdapter` turns documents into bytes and bytes into documents, plus a `RoundTripReport`.

**May depend on.** Kernel.

## 6. Editorial

**Responsibility.** The frame-accurate timeline. This is the editorial model: sequences, tracks, clips, gaps, transitions, nests, markers, effect parameters, color decisions as data, commands, validation, and the resolver.

**Owns.** The in-memory model and its indexes, the command implementations, undo information returned to the application (the application owns the stack), and the mapping to and from `TimelineDocument`.

**Does not own.** File paths, probe results, pixels, OTIO, SQLite, or the playhead. The playhead remains session state in the Application.

**Public interface.**

| Operation | Behavior |
| --- | --- |
| `load(TimelineDocument)` | Validate and build the in-memory model. Reject documents that break editorial invariants, with a result that names the item. |
| `save()` | Return the normalized `TimelineDocument`. |
| `encode` / `decode` | Convert between a timeline document and the versioned bytes Project Storage will hold. Decode checks the document version and then calls `load`. |
| `apply(command)` | Mutate or leave the model unchanged. Commands: overwrite, insert, lift, extract, razor, trim, ripple, roll, slip, slide, marker edit, track enable, nest placement. |
| `query` | Snapshots of sequences, tracks, and items for the view, in frame numbers. |
| `resolve(sequence, range)` | `CompositionSlice` values for the range. Runs with offline assets. Does not consult Media Management. |
| `compositionHash(sequence, range)` | Stable hash so playback can detect a stale plan. |

**Invariants enforced here.** Record ranges on one track do not overlap except where a transition names the two neighboring items. Source ranges lie inside the known source duration when the document carries one. A transition's record range lies on the cut it names. A nested sequence reference names a sequence in the same project document set. Frame mapping uses the clip's stored conform policy.

**May depend on.** Kernel, Contracts.

## 7. Media Management

**Responsibility.** The independent catalog of logical assets, their identity, their local locations, and their online or offline state.

**Owns.** The in-memory catalog, relink, derivative records (proxy, thumbnail, waveform, frame index as named artifacts, not as decoded samples), and the mapping to and from `MediaCatalogDocument`.

**Does not own.** Which timeline uses an asset, how a trim works, decoding, or the project transaction.

**Public interface.**

| Operation | Behavior |
| --- | --- |
| `load` / `save` | Accept or produce a `MediaCatalogDocument`. |
| `encode` / `decode` | Convert between a catalog document and the versioned bytes Project Storage will hold. |
| `commitProbe(path, ProbeRecord)` | Create or update an asset and a master location. |
| `markOffline(asset)` | Keep the asset and clear the online flag. Timeline references remain valid because they use the asset id. |
| `relink(asset, candidate path, ProbeRecord)` | Accept the candidate when identity matches under the recorded rule. |
| `resolveLocation(asset, policy)` | Return a `MediaLocation` for `master` or `proxy`, including the offline case. |
| `recordDerivative(asset, role, path)` | Note a proxy or a cache artifact. Deleting the file is representable by a later offline mark. |
| `attachProbe(IMediaProbe)` | Receive the probe implementation from the Application. The module can run without one, and assets stay offline until a probe exists. |

**Identity rule.** Relink compares duration, rate, resolution, sample rate, reel, timecode start, file size, and content hash when present. The old path is tried first as a hint.

**May depend on.** Kernel, Platform, Contracts. The probe implementation is injected. This module does not link Processing.

## 8. Interchange

**Responsibility.** Translate editorial and catalog documents to and from external edit formats. OpenTimelineIO is the built-in adapter.

**Owns.** The OTIO mapping, the adapter registry, and the construction of a `RoundTripReport`. Mapping from OTIO rational time to kernel frame indexes happens here, once, using the kernel conversion.

**Does not own.** Validation of editorial overlap rules, creation of real files, or the decision to save a project. An imported document is a proposal until Editorial and Media Management accept it.

**Public interface.**

| Operation | Behavior |
| --- | --- |
| `exportOtio(timeline, catalog)` | Bytes plus a `RoundTripReport`. Features without an OTIO home are written under a SISE metadata namespace and listed in the report. |
| `importOtio(bytes)` | A timeline document, a catalog document of external references, and a report. Missing files are references, not dropped cuts. |
| `registerAdapter` | Add a Tier A adapter beside the built-in OTIO adapter. |

**Round-trip rule.** A document produced by `load`/`save` in Editorial, exported, then imported, then accepted by Editorial, exports to the same OTIO bytes for the features the report marked as preserved. The test that proves this is in the roadmap.

**May depend on.** Kernel, Contracts. OpenTimelineIO headers stay in private sources.

## 9. Project Storage

**Responsibility.** The local package: SQLite, transactions, autosave, the lock, and table migrations for the storage module's own schema.

**Owns.** `project.sqlite`, `autosave.sqlite`, the package layout, the generic secondary index, and the storage schema version.

**Does not own.** The meaning of a timeline blob or a catalog blob. It does not parse them and it does not link Contracts. The application passes bytes, a module key, an object id, and a document version.

**Public interface.**

| Operation | Behavior |
| --- | --- |
| `create(path)` / `open(path)` | Create or open a `.sise` directory. Refuse a second writer in another process. |
| `begin` / `put` / `commit` / `abort` | One transaction for a consistent set of blobs. |
| `read(module key, object id)` | Return the bytes and the document version. |
| `putIndex` / `findIndex` | Generic entries the application maintains, including path-to-asset lookup. |
| `autosave` | Write the recovery database without truncating the main database. |
| `migrate` | Move storage tables forward. Leave payload bytes for the owning module to migrate. |

**May depend on.** Kernel, Platform. SQLite stays in private sources.

## 10. Processing

**Responsibility.** Pixels, samples, and presentation. Probe, frame index, thumbnails, waveforms, proxy encode, final encode, color application, and interactive playback.

**Owns.** The FFmpeg use, the GStreamer use, the OpenColorIO use, the effect-processor registry, the frame cache, and the translation from a source frame to a seek timestamp via the frame index.

**Does not own.** The cut, the catalog commit, the project transaction, or the playhead. It reports the frame it presented. The application decides what to do with that report.

**Internal parts, one module, one public surface.**

| Part | Library | Role |
| --- | --- | --- |
| Probe and index | FFmpeg | `ProbeRecord`, keyframe and timestamp index for frame-accurate seek. |
| Stills, waveforms, proxies, export | FFmpeg | Non-realtime. Export consumes one `RenderRequest` per sequence frame. |
| Playback | GStreamer | Real-time graph. Seeks to timestamps the index computed. Reports `PresentedFrame`. |
| Color | OpenColorIO | Builds processors from the `ColorDecision` on the request. Shared by preview and export. |

A second playback backend that uses FFmpeg is not a module and is not part of the approved shape. The roadmap allows a review to add one behind `IPlayback` only if the GStreamer path fails the frame oracle.

**Public interface.**

| Operation | Behavior |
| --- | --- |
| `probe(path)` | Implement `IMediaProbe`. |
| `buildFrameIndex(asset path)` | Regenerable index. Media Management records the artifact; Processing computes it. |
| `renderStill`, `renderWaveform`, `encodeProxy` | Local files, progress, cancellation. |
| `export(requests, target, destination)` | Frame-stepped encode. Remove a partial file on cancel. |
| `startPlayback` / `submit` / `stop` | `IPlayback`. Immutable requests in, `PresentedFrame` out. |
| `registerEffect` | Attach an `IEffectProcessor`. Unknown effect ids pass through and are named in the result. |

**May depend on.** Kernel, Platform, Contracts.

## 11. Plugin SDK

**Responsibility.** The stable C ABI for Tier A plugins. This is a boundary, published when the plugin phase starts. It is specified now so the host and the contracts have a place to meet.

**Owns.** The version constant, the entry-point names, and the C structs that mirror the subset of contract types a plugin may see: probe results, per-frame effect parameters, buffer pointers, interchange byte spans, and export-target descriptions.

**Does not own.** C++ documents, Qt types, editorial commands, or a promise of C++ ABI stability.

**Public interface.**

| Entry | Behavior |
| --- | --- |
| ABI version function | Returns the major and minor the plugin was built for. The host loads only a matching major. |
| Init function | Receives a host function table and fills a plugin function table. |
| Capability functions | Probe, process one effect frame, translate interchange bytes, or describe an export target. Data in, data out. Errors are integer codes. |

The host in the Plugin Host module is the only core code that calls these functions. It adapts them to `IMediaProbe`, `IEffectProcessor`, `IExchangeAdapter`, and `IExportTarget`.

**May depend on.** Kernel, Contracts. The C header that plugins include does not include C++ contract headers. The adapter on the host side includes both.

## 12. Plugin Host

**Responsibility.** Discover local plugin files, load them through Platform, check the ABI version, and expose contract interfaces to the Application.

**Owns.** The load lifecycle and the conversion between C ABI calls and contract interfaces.

**Does not own.** The decision to attach a probe to the catalog or an effect to the renderer. The Application does that, which keeps the host free of Editorial, Media Management, Interchange, and Processing.

**Public interface.**

| Operation | Behavior |
| --- | --- |
| `scan(directory)` | List libraries that export the entry point. |
| `load(path)` | Refuse an ABI mismatch. Return the capability set as contract interfaces. |
| `unload` | Release the library when the application drops the interfaces. |

**May depend on.** Kernel, Platform, Contracts, Plugin SDK.

## 13. Application

**Responsibility.** Use cases and the seam. One session, the command and undo stack, policy, and the construction of `RenderRequest` values.

**Owns.** Session state: playhead, selection, zoom, undo and redo stacks, proxy-or-master policy, the open package path, and the order of module migrations. The exchange mapping that is pure field copying from documents into adapter calls. The task queue for probe, proxy, and export work, which posts results back to the session thread.

**Does not own.** Trim math, catalog identity rules, SQL, codec calls, or widgets. It calls the module that owns each of those.

**Public interface.** The UI is the caller.

| Operation | Behavior |
| --- | --- |
| `openProject` / `saveProject` / `autosave` | Read or write blobs through Project Storage. Load each blob through the owning module. |
| `importMedia` | Probe through Processing, commit through Media Management, then save. |
| `edit(command)` | Apply through Editorial, push undo, notify the observer. |
| `undo` / `redo` | Inverse commands through Editorial. |
| `seek` / `play` / `stop` | Resolve, locate, build `RenderRequest` values, submit to `IPlayback`. Update the playhead from `PresentedFrame`. |
| `export` | Resolve and locate, require masters unless the preset allows proxies, call Processing, surface the report. |
| `exportOtio` / `importOtio` | Documents in and out of Interchange. Accept imports through Editorial and Media Management before saving. |
| `setQualityPolicy` | Proxy or master for playback. Export policy is per request. |
| `attachPlugin` | Take interfaces from Plugin Host and register them with the owning module. |

**Request construction.** For each layer of a `CompositionSlice`, look up a `MediaLocation`. Copy source position, weights, parameters, and the color decision. Leave Processing with no reason to read a timeline.

**May depend on.** Every module except the UI.

## 14. UI

**Responsibility.** The Linux desktop shell: project open and save, media bin, timeline, program monitor, inspector, export dialog, and the interchange report.

**Owns.** Qt widgets, input gestures, and the translation of a gesture into an application command with a frame number. Painting of pictures that playback has already produced.

**Does not own.** The result of a trim, the contents of a project transaction, or a codec context. If the picture is unavailable, the monitor shows the offline or untagged state the application reported.

**Public interface.** None toward other modules. The UI implements `ISessionObserver` and calls the Application. Gestures that matter:

| Gesture | Command |
| --- | --- |
| Drop files on the bin | `importMedia` |
| Blade, trim, ripple, roll, slip, slide | the matching editorial command at a frame |
| Scrub or press play | `seek` or `play` |
| Export | `export` with a preset and a local path |
| Import or export OTIO | the matching interchange command, then show the report |

The shell uses Qt Widgets. Layout works at a desktop size and in a narrow window: the timeline and monitor remain reachable, and panels stack when the window is narrow. Those are UI requirements for the phase that builds the shell.

**May depend on.** Application, and Qt 6 as a private dependency.

## 15. Who calls whom during a save

A save is the clearest end-to-end check of the matrix.

1. UI calls `Application.saveProject`.
2. Application calls `Editorial.save` and `encode`, and `Media Management.save` and `encode`, and reads its own settings.
3. Application calls `Project Storage.begin`, `put` for each byte blob, and `commit`.
4. Editorial does not see SQLite. Project Storage does not see a track. Processing is not involved.

A playback seek checks the other seam:

1. UI calls `Application.seek`.
2. Application calls `Editorial.resolve` and `Media Management.resolveLocation`.
3. Application builds `RenderRequest` values and calls `Processing`.
4. Processing returns `PresentedFrame`. Application tells the observer.
5. Editorial and Processing never call each other.

## 16. Future split, if a module grows

The application is the largest orchestration surface. If exchange mapping or the task queue becomes hard to review, either one may become a module that sits where the Application sits in the matrix: it may depend on the domains it coordinates, and only the Application or the UI may depend on it. That split is a revision of this document. It is not a license to let domain modules depend on each other.

Contracts is the other place growth must stay boring. If the timeline document and the media document become too large to review together, they may split into two passive modules that still depend only on Kernel. Editorial would depend on the timeline types. Media Management would depend on the catalog types. Interchange and Processing would depend on the types they actually read. The forbidden edges in section 2 stay forbidden.
