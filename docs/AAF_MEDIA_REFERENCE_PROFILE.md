# AETHER AAF media reference profile

Status: proposal for review. Specification only. No AAF file was written or read. No consumer was executed. The AAF Association publishes the format. Citing that body is not a conformance test.

## Verified facts

- AAF under the AETHER audio profile is the primary audio composition exchange. Picture stays in AVX. The native model stays the authority (`docs/AAF_AUDIO_EXCHANGE_PROFILE.md`, `docs/INTERCHANGE_ADAPTER_ARCHITECTURE.md`).
- That audio profile already says essence is external by default, and that an MXF which did not store channel layout must not be described as if it had.
- The prior FFmpeg 9.0.2 study’s PCM limits (48 kHz, 24-bit, little-endian), missing MXF channel layout, and 29.97 sample-count mismatch constrain any MXF essence an AAF might point at. They do not define AAF composition semantics, and they were not re-tested here.
- Consumers differ. This profile does not assume a file that AETHER writes will round-trip in every AAF tool.

## Engineering recommendations

- Put the composition and the standard source references in the AAF, and put the full identity in the AETHER manifest beside it.
- Point the camera-original reference at the original filename, reel, timecode, and path strategy. Put a proxy, if included, in a separate labelled reference.
- Keep unsupported mix or picture fields in the manifest and in the loss report.

## Proposed behaviour

### Scope

This profile defines how AETHER refers to media from an AAF so an audio workstation, or a picture tool that imports AAF, can find sound and the accompanying picture references. Clip gain, fades, automation, and track layout remain specified in `docs/AAF_AUDIO_EXCHANGE_PROFILE.md`. This document does not reopen them except where a media reference would lose them.

### Requirements

- The AAF contains external essence references by default. Embedded essence stays off unless a delivery preset copies WAV or BWF beside the AAF and says so in the report.
- For each source, the AAF carries, when the chosen AAF structures can hold them: file name, path or relative path according to the path strategy, reel or tape name, source timecode, duration, sample rate or frame rate, and channel count.
- The same export writes an AETHER manifest entry with: filename, reel, roll, clip id, timecode, duration, rate, path, relative path, checksum, and Media ID. The Media ID is for AETHER and for any future integration that has been tested. No such integration is claimed.
- Fields the AAF structure cannot hold stay in the manifest and are listed in the compatibility report as `metadata_only` or `unsupported`. They are not dropped without that entry (integrity rule 8).
- The original reference does not use the proxy filename. A second reference may name the proxy, with a role the manifest marks `playback`, when the user asked to include playback media.
- Source in and out are positions at the source rate, and record positions are positions at the sequence rate. The profile writes both. It does not write the timeline position as if it were the source timecode.
- Speed maps that AAF cannot represent for a given source are reported. The AAF is not given a shortened constant substitute unless the user chose a fallback that the report names. The native map stays.
- Do not claim a consumer matches this profile. The report’s target is a name, and `tested` is false until test H is executed against a named build.
- MXF essence referenced by the AAF, if any, follows the unvalidated constraints: do not promise PCM beyond the studied 48 kHz 24-bit little-endian case, do not claim channel layout is inside the MXF, and do not treat a 29.97 duration as matched without the integer sample comparison. WAV or BWF remains the conservative audio essence, as in the native media specification.

### Data model

Logical reference, independent of a particular AAF SDK’s class names. A later implementation maps these fields onto published AAF objects. This document does not invent unpublished AAF class identifiers or copy a vendor’s SDK headers.

| Logical field | Intended placement | If the writer cannot place it |
| --- | --- | --- |
| Original file name | Essence locator file name | Fail the item. Do not substitute the proxy name. |
| Relative or absolute path | Locator, strategy recorded in the manifest | Record the strategy actually written |
| Reel or roll | Tape or reel mob attribute when the writer has a published place; always in the manifest | Manifest plus report |
| Camera clip id | Manifest. AAF only if a published user comment or attribute is used and labelled `aether_clip_id` | Manifest |
| Media ID | Manifest, and an AAF comment labelled `aether_media_id` | Manifest at minimum. The comment is informational. |
| Start timecode and drop-frame | Timecode slot | Fail the audio export if picture-sync timecode was `present` and could not be written |
| Source in, source out, record in | Source clip origin, length, and timeline position in edit units the profile declares | Fail the export |
| Edit rate | Rational recorded as the AAF edit rate the writer supports, plus the exact `n/d` in the manifest | If the AAF rate cannot represent `24000/1001` exactly, fail rather than write `23.98` as a float |
| Checksum | Manifest | Report `checksum_omitted_from_aaf` |
| Channel layout | AAF when the profile can represent it; manifest always | State when the essence file itself does not store layout |
| Proxy locator | Separate locator, manifest role `playback` | Omit proxy. Do not overwrite the original locator. |

Path strategies match the AVX profile: `absolute`, `relative`, `volume_relative`, `roll_and_clip_id`, `media_id`, `consolidated`. `media_id` strategy still writes filename, reel, and timecode, because the receiving application does not understand the UUID.

### APIs

| Operation | Contract |
| --- | --- |
| `AAF.exportAudio(sequence_id, strategy)` | Writes the AAF, the manifest, and the compatibility report. |
| `AAF.referenceOriginal(media_id)` | Fills the logical reference from the registry original role. |
| `AAF.import(document, manifest)` | Proposes a native audio layout. Match and relink run as separate steps. Conform is separate. |

The importer prefers the manifest for Media ID and checksum, and the AAF for the composition. If they disagree on duration or track count, the import stops for review. It does not pick one in silence.

### Workflow

1. The user exports audio exchange for a sequence.
2. The writer builds locators from original sound Media IDs and original picture references needed for sync.
3. The manifest receives every identity field, including those the AAF locator held.
4. The compatibility report lists effects, ramps, and layout items the AAF could not carry.
5. A receiving user relinks with filename, reel, timecode, and path. An AETHER user can relink with the manifest’s Media ID and checksum.
6. Nobody marks the sequence verified because the AAF write returned success.

### Validation

- Every audio instance in the snapshot has a reference or a loss entry.
- No original locator’s file name equals a known proxy file name for that Media ID.
- Edit rate in the manifest is the rational. A float edit rate in the AAF, if a writer was forced toward one, is a failure, not a warning, for 23.976, 29.97, and 59.94.
- Channel layout claims match the container capability flag.
- `tested` on the target application is false in any file this specification’s unimplemented writer would produce, until an execution record exists.

### Errors

| Error | Export |
| --- | --- |
| Source timecode present and not writable | Item failed, export not marked complete |
| Only a proxy online and the user asked for original locators | Locator omitted, manifest says original offline, export not described as camera-original complete |
| Manifest write fails | AAF not marked complete, even if the AAF bytes were written |
| Consumer report imported without a manifest | Allowed as a foreign AAF. Media IDs are not invented. Matching uses levels 2–6 only. |

### Recovery

Re-export overwrites only the exchange directory the user named, after confirmation if files exist. The project is unchanged on failure. A half-written AAF is deleted or marked incomplete. Originals are untouched.

### Performance

The writer walks audio instances and registry rows. It does not copy WAV files unless the delivery preset said to. No AAF size was measured.

### Tests

Test H, specified and not executed. Cases: original locator file name, manifest Media ID, rational rate in the manifest, proxy excluded from the original locator, missing layout reported for MXF, and `tested: false`. A round-trip in a named external build is listed in the test plan as not executed.

### Acceptance

- The profile states what the AAF carries and what the manifest carries.
- It refuses float rates and proxy filenames in the original slot.
- It does not claim consumer agreement.
- PCM and layout limits from the prior study are reconcile constraints, not new results.

## Unresolved questions

- The exact published AAF property used for a reel name. Recommendation: the implementer maps the logical reel field to the published tape name on a source mob and records the property id they used in the compatibility report. This specification does not freeze a private property.
- OP-Atom audio as an AAF locator target. Recommendation: out of scope until `docs/AVID_MEDIA_COMPATIBILITY.md` has a validated writer. WAV/BWF locators are the profile default.
- Whether picture tracks are included as sync-only. The audio profile allows optional picture references. This profile includes them only when the user asks, and they use the same original-versus-playback split.
