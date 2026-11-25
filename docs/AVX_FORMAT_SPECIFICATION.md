# AVX format specification

Status: proposal for review. AVX (AETHER Video Exchange) is the open, versioned picture exchange. It is not the native project database. No AVX file was exchanged with another application. No other vendor has implemented AVX. “Open” means the specification is implementable by others, not that a second implementation exists.

## Verified facts

- The brief requires AVX to preserve editorial intent, to be versioned, and to be implementable without AETHER’s private types.
- The brief requires an evaluation of JSON, XML, and binary, and a justification of one choice.
- Preservation states required: native editable, mapped equivalent, metadata only, rendered fallback with the original definition retained, and unsupported explicitly reported.
- Silent discard of editorial information is forbidden.
- Required coverage: layers, nests, mixed rates, speed ramps, reverse, freeze, transforms, crop, opacity, blend, transitions, effects, keyframes, handles, and media identity.
- No compatibility with Resolve, Media Composer, or Premiere is claimed through AVX. Those applications do not, on any evidence gathered here, read AVX.

## Engineering recommendations

- Choose UTF-8 JSON as the canonical AVX encoding for version 1.
- Store every rational as a string `"n/d"`, never as a JSON number.
- Store media by AETHER media id plus checksum and technical metadata. Paths are hints.
- Require a loss report array in every file, even when empty.
- Keep a single document schema version at the top. Unknown newer versions are refused by older readers, not partially imported in silence.

### Why JSON, not XML, not binary

| Encoding | Assessment |
| --- | --- |
| JSON | Text, widely implemented, diffable, enough for a tree of clips and keyframes. Weak at comments and at numeric precision. Precision is repaired by storing rationals as strings. Schema can be JSON Schema. A second implementer can parse it with a standard library. |
| XML | Also text and also implementable. Namespaces, entities, and mixed content add a second stack. FCP XML’s history shows verbose documents that still lose intent. XML is a credible alternative if review prefers a single stack with AAF-adjacent tooling. It is not the recommendation, because AVX does not need mixed content and JSON is the smaller contract. |
| Binary | Smaller and faster to parse. Opaque in an archive, hostile to review, and easy for a version skew to drop fields without a human noticing. Speed is not the binding constraint: AVX does not contain essence. Picture bytes stay in media files. |

Recommendation adopted as proposed behaviour: canonical AVX 1 is JSON. A binary profile is not part of version 1. An XML projection may be added later as a derived encoding of the same object model. It must not become a second feature set.

## Proposed AETHER behaviour

### File

- Extension: `.avx.json`
- Encoding: UTF-8, no BOM required
- One sequence per file in version 1. A project export may be a folder of AVX files plus a small index. The index is also JSON.
- Location when written by AETHER: `EXCHANGE/AVX/`

### Top-level object

```json
{
  "avx": "1.0.0",
  "kind": "sequence",
  "generator": { "name": "AETHER", "version": "0" },
  "sequence": {
    "id": "seq_01H",
    "name": "Reel 1",
    "timebase": "24/1",
    "raster": { "width": 1920, "height": 1080 },
    "pixel_aspect": "1/1",
    "audio_sample_rate": 48000
  },
  "media": [],
  "tracks": [],
  "markers": [],
  "loss_report": []
}
```

`generator` is informational. Readers must not branch behaviour on the generator name to accept undocumented fields. Unknown fields are preserved on round-trip if the reader is AETHER. A foreign reader lists unknown fields in its own loss report and must not delete them if it writes the file back. If it cannot preserve them, it writes `unsupported` entries and keeps the original file untouched unless the user chose overwrite.

### Media identity

```json
{
  "media_id": "med_01J",
  "checksum": { "algo": "sha256", "value": null, "state": "pending" },
  "original_name": "A001C001.mov",
  "frame_rate": "24000/1001",
  "raster": { "width": 3840, "height": 2160 },
  "duration_frames": 2400,
  "audio": { "sample_rate": 48000, "channels": 2, "layout": "unknown" },
  "path_hint": "/volumes/card/A001C001.mov"
}
```

Relink uses checksum and technical metadata. `path_hint` and `original_name` are not sufficient.

### Clip and layers

Tracks are ordered layers. Picture tracks stack. Audio tracks are included so a picture exchange does not strip sync sound. AAF remains the richer audio exchange. AVX still carries audio edit geometry so a picture round-trip does not destroy splits.

```json
{
  "clip_id": "clp_88",
  "media_id": "med_01J",
  "record_in": 86400,
  "record_out": 86520,
  "source_in": 100,
  "source_rate": "24/1",
  "speed": "1/1",
  "handles": { "head_frames": 100, "tail_frames": 40 },
  "transform": { "state": "native_editable", "position": [0, 0], "scale": ["1/1", "1/1"], "crop": [0, 0, 0, 0], "opacity": "1/1" },
  "blend": { "state": "native_editable", "mode": "normal" },
  "effects": [],
  "keyframes": []
}
```

Handles are the unused source frames at export time. They are informational for conform and also reconstruct slip room. Picture selection is ADR-002. A per-clip `rounding` string is not part of this example.

### Preservation states

Every effect, transition, transform, and nest carries `state`:

| State | Meaning |
| --- | --- |
| `native_editable` | The AVX structure is the edit. A capable reader can change it. |
| `mapped_equivalent` | The reader mapped AETHER’s effect to a named equivalent. Parameters are the mapped ones. `source_definition` holds the original parameter block. |
| `metadata_only` | The effect is described and must not be rasterised. Playback in a reader that does not implement it shows a bypass and the report. |
| `rendered_fallback` | A rendered media id is provided for viewing. `source_definition` still holds the original effect. The render does not replace the definition. |
| `unsupported` | The exporter could not represent the item. The item is still present as a placeholder with its original definition, and `loss_report` repeats it. |

Empty `loss_report` means the exporter claims every item is `native_editable` or `mapped_equivalent` with definitions retained. A reader that does not implement an effect still reports its own loss on import. The file’s report is the writer’s claim, not a promise about other software.

### Coverage rules

| Feature | AVX 1 representation |
| --- | --- |
| Layers | Ordered picture tracks |
| Nests | Clip with `nest.sequence` inline or referenced by id in a multi-file export. Cycle forbidden. |
| Mixed rates | `source_rate` and sequence `timebase` both stored |
| Speed ramps | Keyframes of rational speed |
| Reverse | Negative rational speed |
| Freeze | Speed `0/1` over a record range, source frame held |
| Transforms, crop, opacity, blend | Objects above. Absent means identity, and identity is `native_editable`, not “missing data”. |
| Transitions | Object between two clip ids, duration in sequence frames, state, definition |
| Effects and keyframes | Arrays. Time in sequence frames or source frames, declared per curve. Values that are rational are strings. |
| Handles | Integer frames as above |
| Media identity | Media array |

### Loss report entry

```json
{
  "severity": "unsupported",
  "object_id": "fx_12",
  "message": "Third-party blur has no AVX parameter map. Definition retained. No render attached."
}
```

### Import

The interchange layer produces a native document plus a report. The application commits it as a new sequence, or refuses if the version is newer. Import does not delete media. Unresolved media ids become offline clips with identity intact.

### Specified tests, not executed

- A speed of `-1/1` survives a write and a read as a negative rational, not as a float.
- An unknown effect is still in the file and in `loss_report`.
- A rendered fallback file still contains `source_definition`.
- A second implementation can parse version `1.0.0` with a stock JSON parser and ignore `generator`.

## Rationale

| Decision | Rationale |
| --- | --- |
| JSON with string rationals | Meets openness and avoids the binary-float failure the timeline specification forbids. |
| Loss report required | Otherwise “unsupported” is a log line the next facility never sees. |
| Render does not replace definition | The brief requires the original definition to be retained. A movie of an effect is not the effect. |
| Audio geometry included | A picture-only exchange that drops splits forces the editor to rebuild L-cuts. AAF still carries the richer mix. |
| One sequence per file in v1 | A smaller schema is more likely to be implemented by someone else. A folder index scales to a feature. |

## Unresolved questions

- Whether review will override JSON in favour of XML. The recommendation is JSON. It is not a ratified SISE standard.
- JSON Schema versus a prose schema as the normative text. Recommendation: prose in this specification is normative until a schema file is added. A schema file is a later artifact, not written here as code.
- How large a nest may be inlined before a reference is required. Recommendation: inline below a documented size, otherwise reference. The size is open.
- Digital signatures for exchange. Out of scope for version 1.
- Clip colours, locators, and bin structure. Markers are in. Bin structure is a project concern and is not required to interpret the sequence. Open whether a later minor version adds optional bins.
