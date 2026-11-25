# AETHER Media Identity and Metadata

Status: proposal, awaiting review. No implementation is authorized by this document.

Owner decision, 2026-11-20 (ADR-005): the asset id in this document is the Media ID. One logical asset has one Media ID. A proxy, a transcode, and a consolidated file are representations with their own Representation IDs, not new editorial source identities. Origin id is not a substitute Media ID. The descriptor study below, including the 19 November FFmpeg notes where this file carries them, is kept.

The catalog identifies a take. The path is where a file happened to be. This document defines the identity Media Management stores, the descriptors a native MXF file carries, and the comparison used to relink. Clips continue to store an asset id and a source range, as [EDITORIAL_PHILOSOPHY.md](EDITORIAL_PHILOSOPHY.md) and [ARCHITECTURE.md](../ARCHITECTURE.md) already require. That asset id is the Media ID. Timecode remains a label, as [TIMEBASE_AND_FRAME_ACCURACY.md](TIMEBASE_AND_FRAME_ACCURACY.md) requires. Channel maps use the same idea as the audio documents: an ordered list stored with the asset, copied onto a clip when the clip is made.

The container fields are specified in [MXF_CONTAINER_SPECIFICATION.md](MXF_CONTAINER_SPECIFICATION.md). Which profiles exist is specified in [NATIVE_MEDIA_ARCHITECTURE.md](NATIVE_MEDIA_ARCHITECTURE.md).

## 1. Two identifiers

AETHER keeps two identifiers, and neither is derived from a path or from an MXF UMID.

| Identifier | Scope | Minted when | Survives |
| --- | --- | --- | --- |
| Asset id | One catalog asset. Clips, markers, and match-frame use this. | The first time the catalog accepts the take. | Relink, proxy generation, and a move of the file. |
| Origin id | The camera, scanner, or field recording the take came from. | The first time that recording enters any project on this machine. A transcode copies it. | A mezzanine, a proxy, and a project that receives a copy of the catalog record. |

The asset id is the handle Editorial already uses. The origin id is new. It answers a different question: two files can be one recording. A ProRes 422 HQ mezzanine of camera roll A001 and the camera file itself share an origin id and, when they are the working master and the preserved original of the same take, they share an asset id as two locations. A different recording gets a different origin id even when the file name looks similar.

FFmpeg 9.0.2 writes a material-package UMID and a product UID. On the file checked here the product UID was `adab4424-2f25-4dc7-92ff-29bd000c0002` and the company name was `FFmpeg`. Those values change their meaning when a different writer remuxes the same pictures. They are stored on the location as hints. They are not the asset id and they are not the origin id.

Both AETHER identifiers are random 128-bit values, the same kind of local identifier the architecture already uses for a project package. They are not an account and they are not a server key.

## 2. The asset record

One record per asset. Media Management is the only module that validates it.

| Field | Type | Meaning |
| --- | --- | --- |
| Asset id | 128-bit id | The editorial handle. |
| Origin id | 128-bit id | The recording. |
| Display name | Text | The name in the bin. Independent of the file name. |
| Reel | Text, optional | The reel the identity uses for relink. Empty when the source has none. |
| Camera roll | Text, optional | The camera roll, stored separately from the reel. A film roll and a lab reel can both exist. |
| Timecode label | Start frame-label, rate, drop-frame flag | The source timecode. A label, not a position in the model. |
| Source rate | `FrameRate` | The picture rate of the asset. |
| Source duration | Frame count, `int64` | `end - start` of the whole source, half-open, at the source rate. |
| Display rectangle | Width, height, both `int64` | The active picture. |
| Sample rate | Integer, or absent | 48000 on a native master that has sound. Absent when the take has no sound. |
| Channel count | Integer | 0 when the take has no sound. Otherwise the map length. |
| Channel map | Ordered list of channel entries | Section 4. |
| Color names | Primaries, transfer, matrix, range | Names for the OCIO setup. Absent when the file did not say. |
| Verification | Section 6 | The latest probe result. |

A still image and a sound-only file use the same record. A still has duration policy `hold` in the editorial model and a frame count of 1 in this record. Sound-only duration is the frame count implied by `sampleIndex` at the declared picture rate, or, when the operator has not declared a rate, the record stores the sample count and leaves the frame rate unset. A native master always has a picture rate.

## 3. The location record

Each file is a location on the asset. The architecture already has `master` and `proxy`. This proposal adds `original`. That role is decision 1 in the architecture document.

| Field | Type | Meaning |
| --- | --- | --- |
| Location id | 128-bit id | This file's row. |
| Role | `original`, `master`, or `proxy` | Which file this is. |
| Path | Local path | A hint. Relink updates it. |
| File size | Integer bytes | The size at the last successful read of the whole file. |
| File hash | SHA-256, optional | Section 5. |
| Online | Flag | The path opened at the last probe. |
| Container | Pattern name | `mxf-op1a`, `mxf-opatom`, or another container name from the probe. |
| Profile | Catalog profile name | `prores-422-hq`, `dnxhr-hqx`, and the other names in the architecture document. Empty when the codec is outside that list. |
| Stored rectangle | Width and height | The codec's stored size, including DNxHR padding. |
| Writer | Company, product, version | From the MXF identification set when present. Information, not a trust decision. |
| Package UMID | Bytes, optional | The material-package UMID, as a hint. |
| Essence coding | Label bytes, optional | The picture essence coding label. |
| Compression id | Integer, optional | The VC-3 compression id when the file is DNx. |
| Timecode in file | The label read from the file | Compared with the asset's label. |
| Probe time | Local timestamp | When this row was last filled. |

A proxy location additionally stores the id of the master location it was built from and the hash of that master at build time. Rebuilding the proxy after the master hash changes is a new location row. The old proxy remains until the operator deletes it, and playback prefers the row whose master hash still matches.

The original location, once accepted, is not replaced by a transcode. The transcode is a new master location. The original stays so the recording can be hashed and opened later.

## 4. Channel map

The map is an ordered list. Entry `i` is channel `i` in the interleaved sound element, or sound element `i` when the file is a set of mono OP-Atom files. Each entry stores:

| Field | Meaning |
| --- | --- |
| Index | Zero-based position in the file order. |
| Role | A name from the set the audio architecture already uses, or `unknown`. |
| Source name | The track name from the file or the operator, optional. |

The first named layouts are mono, stereo, and 5.1. Any other count from 1 through 32 is an adaptive map: the count is known and each role starts as `unknown` until the operator or a real label in the file sets it. A file that only stores a channel count, which is what FFmpeg 9.0.2 wrote for 1, 2, 6, and 8 channels, produces an adaptive map of that count. It does not become stereo or 5.1 because the count is 2 or 6.

The sample order in the file is the map order. Ingest does not reorder. A clip's channel map, set by `setChannelMap`, may pick and reorder from this source map. That selection lives on the clip. Relink does not rewrite clips.

A native master writes one interleaved element in map order. The target descriptor's channel count equals the map length. The names themselves live in this record, because the OP1a files measured here did not carry them.

## 5. Checksums and verification

The file hash is SHA-256 of every byte of the file, in order. It is computed when a location is verified, which means the file was read to the end. A hash of a partial read is not stored. The hash is not part of the asset id. Copying the file to a new path does not change the hash. Remuxing does.

Verification states:

| State | Meaning |
| --- | --- |
| `unchecked` | No completed read of this location. |
| `hash-match` | The stored hash equals a new read. |
| `hash-absent` | One side has no hash. Duration and size may still match. The location is not verified. |
| `hash-differs` | Both sides have a hash and they differ. The location stays offline for playback until the operator resolves it. |
| `unreadable` | The probe could not finish. The asset remains in the cut. |
| `profile-unsupported` | The file opened and the picture or sound is outside the native set. It can still be an original. |
| `descriptor-mismatch` | The descriptor and the bitstream disagree. The alpha case in the ProRes document is this state: FFmpeg decoded a matte and MediaInfo did not report alpha. |
| `duration-mismatch` | Picture frame count and audio sample count disagree with `sampleIndex`. |

An essence hash, meaning a hash of decoded pictures rather than of the file, is a reserved field with no algorithm in this proposal. Remuxes that preserve pictures and change the file hash are detected as `hash-differs` on the file, and the operator can confirm them. This proposal does not treat two different files as the same bytes.

## 6. Relink

Relink proposes a candidate path for a location. The previous path is tried first as a hint. A candidate that fails the comparison is not attached.

Comparison, in order:

1. The file opens far enough to read the header and the index.
2. Frame rate equals the asset source rate.
3. Frame count equals the source duration.
4. Display rectangle equals the asset display rectangle.
5. Sample rate and channel count equal the asset, or both sides have no sound.
6. Channel map equals the asset map. `unknown` matches `unknown` at the same index. A named role does not match `unknown`.
7. Reel and camera roll equal the asset when the asset has them. An empty candidate does not fill a stored reel.
8. Timecode label equals the asset label: start, rate, and drop-frame flag.
9. File size equals the stored size when both are known.
10. SHA-256 equals the stored hash when both are known. Missing hash is `hash-absent`, reported, and is not a match that counts as verified.

A profile difference is shown. It blocks attachment when the location role is `master` or `proxy` and the candidate's profile is not the stored profile. It does not block attachment of an `original` whose codec was always outside the native set.

The editorial document is unchanged by a successful relink. Clips keep their asset ids and source ranges.

## 7. Proxies

A proxy is generated from a verified master. It is the same asset. The generator copies origin id, reel, camera roll, timecode label, source rate, frame count, display rectangle, sample rate, and channel map. It writes a new file under the project's `proxies/` directory. The picture profile is `prores-422-proxy` or `dnxhr-lb`. The sound is 48 kHz 24-bit PCM in the master's map order. The proxy location stores the master location id and the master file hash.

A proxy whose frame count, rate, timecode label, or channel map differs from the master is rejected and not registered. Deleting the proxy file sets the location offline. The cut does not move.

Playback may choose the proxy. Export uses the master. That split is the architecture's existing rule.

## 8. Preservation

The original file is read for the hash and the probe, then left in place. AETHER does not open it for writing. A transcode writes a new path. The project package does not gain a copy of the original unless the operator asks for a copy, which is the architecture's existing import rule.

The identity that must still be true after a transcode is the origin id, the reel, the camera roll, the timecode label, the frame rate, the frame count, the display rectangle, and the channel map. The profile and the file hash of the new master are new. The original location keeps the old hash.

Source timecode is copied as a label. It is not recomputed from the transcode's encode clock. A transcode that starts its own timecode at zero fails the comparison in section 6 and is not attached as a location of that asset.

## 9. Fields the MXF file carries

For a native OP1a master the file itself carries the structural rows below. Everything else in sections 2 and 3 stays in the catalog even when the file also has a copy.

| Catalog field | MXF placement |
| --- | --- |
| Timecode label | Material-package timecode track, source-package timecode track, and the system-item timecode of each content package. |
| Source rate | Picture track edit rate and descriptor sample rate. |
| Source duration | Package duration and index duration, in edit units. |
| Display rectangle | Display width, display height, display offsets. |
| Stored rectangle | Stored width and stored height. |
| Profile | Picture essence coding, and the VC-3 compression id when the profile is DNx. |
| Component depth, subsampling, alpha | CDCI or RGBA descriptor, as the profile document requires. |
| Color names | Color primaries, transfer characteristic, coding equations. |
| Sample rate and bit depth | Wave audio descriptor on a target file. |
| Channel count | Generic sound descriptor channel count. |
| Reel | Lower-level source package name. |
| Display name | Material package name. |
| Writer | Identification set: company, product, version, product UID. |
| Channel map roles | Catalog only, unless a later mapping for channel labels is accepted. This proposal does not require that mapping. |
| Asset id and origin id | Catalog only. |
| File hash | Catalog only. |

FFmpeg 9.0.2 can write the timecode, the material package name, the file package name, and a reel package name. MediaInfo showed all three names on a test file. ffprobe showed the material package name and did not show the file package name or the reel name as separate tags. A round trip through ffprobe is not a round trip of the reel. The relink tests in [MEDIA_INTEROPERABILITY_TESTS.md](MEDIA_INTEROPERABILITY_TESTS.md) read the reel back with a tool that understands source packages, and they are not yet run outside MediaInfo.
