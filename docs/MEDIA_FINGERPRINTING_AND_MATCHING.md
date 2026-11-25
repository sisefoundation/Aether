# AETHER media fingerprinting and matching

Status: proposal for review. Specification only. No file was fingerprinted. No checksum was computed. Classes below are policy, not observed scores.

## Verified facts

- The master relink note ranks checksum above filename and refuses to treat filename as sufficient (`docs/MEDIA_STORAGE_AND_RELINKING.md`).
- Identity requires an id that is not derived from the checksum, so a checksum change is a verification event (`docs/MEDIA_IDENTITY_AND_METADATA.md`).
- No fixture library was hashed for this document.

## Engineering recommendations

- Classify every candidate. Auto-accept a critical conform binding only for `exact`, or for a match a person has explicitly accepted that then passes validation.
- Let filename and relative path order a search the user scoped. Keep them from raising a class on their own.
- Store the evidence that produced the class so a later review can see why a row was accepted.

## Proposed behaviour

### Scope

Matching assigns a class to a candidate file or package relative to an expected Media ID or expected technical record. Fingerprints are one evidence type. The relink procedure that uses these classes is `MEDIA_RELINK_ENGINE.md`. Conform’s acceptance rule is `ONLINE_CONFORM_ARCHITECTURE.md`.

### Requirements

Match evidence is considered in this order. A lower level does not outvote a higher level’s contradiction.

| Level | Evidence | Role |
| --- | --- | --- |
| 1 | AETHER Media ID inside a valid project manifest or a valid AETHER sidecar | Strongest identity claim |
| 2 | Full-file checksum | Byte identity against a completed registry checksum |
| 3 | Trusted camera-native clip id plus recording metadata | Camera identity without a hash |
| 4 | Fingerprint plus technical metadata | Content and structure, algorithm-versioned |
| 5 | Timecode, duration, reel or camera, and file size together | Weak technical bundle |
| 6 | Filename and relative path | Support and sort only |

Trusted camera-native means the clip id was read from camera metadata or a required sidecar, with source `camera_native` or `sidecar`. A clip id parsed from a filename by an unconfirmed pattern is source `unverified` and is not level 3.

Classes:

| Class | Meaning |
| --- | --- |
| `exact` | Level 1 agrees and the full checksum matches, or level 2 matches a completed checksum and the match is unique |
| `strong` | Unique level 3 agreement, or level 1 with technical agreement while checksum is `pending` |
| `probable` | Unique level 4, or a unique level 5 bundle where every listed field agrees |
| `ambiguous` | More than one candidate, a partial bundle, or evidence that is only level 6 |
| `mismatch` | A claimed id, checksum, clip id, or rate contradicts the expected record |
| `missing` | No candidate in the search scope |

Policy:

- Critical conform and relationship replacement auto-accept `exact` only after the validation step in relink or conform passes.
- A person may accept one `strong` or `probable` candidate. That acceptance is stored on the decision row. Validation must then pass before the relationship changes. This is the only meaning of “explicitly validated”.
- `ambiguous` always waits for a person. A batch command cannot accept a set of ambiguous rows.
- `mismatch` is never accepted.
- `missing` leaves the previous relationship and the offline state.
- Filename alone cannot produce `exact`, `strong`, or `probable`.
- Duration alone cannot produce `exact` or `strong`.
- Two registry rows with the same completed checksum are `ambiguous`, not `exact`, until a person chooses. The specification expects this to be rare and still refuses to guess.

A valid manifest or sidecar is one the project can tie to a content hash it stored, or a sidecar whose embedded checksum matches both the file and a completed checksum already stored for that Media ID. A sidecar that names an existing Media ID and fails the checksum is `mismatch`. A brand-new ingest with no prior id may mint. It may not seize an existing id. This is unresolved decision 3, and the recommendation there is the proposed behaviour until review changes it.

### Data model

```json
{
  "decision_id": "00000000-0000-4000-8000-000000000010",
  "expected_media_id": "00000000-0000-4000-8000-000000000001",
  "candidate_path": "/volumes/raid/cam/A001C001.mov",
  "class": "probable",
  "evidence": {
    "levels_hit": [4, 5, 6],
    "levels_contradicted": [],
    "checksum_state": "pending",
    "filename_used_as": "sort_only"
  },
  "accepted": false,
  "actor": null
}
```

Illustrative. Not a match that was run.

Fingerprint record, stored when computed:

| Field | Content |
| --- | --- |
| `media_id` | Original |
| `algorithm_id` | Opaque string, versioned |
| `algorithm_version` | Integer |
| `value` | Opaque digest |
| `computed_utc` | Timestamp |
| `state` | `pending`, `complete`, `failed` |

Proposed algorithm content, not computed and not frozen: frame count, rational rate, raster, audio sample count, sample rate, channel count, start timecode, plus digests of a fixed set of picture indices and audio windows. The indices are unresolved decision 5. Until they are chosen, `algorithm_id` stays unset and level 4 cannot fire. Levels 1–3 and 5–6 still function.

### APIs

| Operation | Output |
| --- | --- |
| `Match.classify(expected, candidate, scope)` | One class plus evidence. No registry write. |
| `Match.classifySet(expected, candidates)` | One class per candidate, and `ambiguous` if two share the winning class. |
| `Match.explain(decision_id)` | The evidence a UI or a report can print. |
| `Fingerprint.store(media_id, record)` | Writes a completed fingerprint. Does not change class of past decisions. |

`Match.classify` has no UI parameter.

### Workflow

1. Caller supplies the expected registry row and one or more candidates already discovered.
2. The matcher reads metadata the caller extracted. It does not open a decoder library itself. Extraction is processing’s job and arrives as data, which keeps the registry and the matcher free of FFmpeg and GStreamer.
3. Classification applies the table above.
4. The caller (relink, ingest, or conform) applies policy and, on acceptance, validation, then the registry transaction.

### Validation

A class is valid when the evidence levels imply it. Examples of invalid results the API must not return:

- `exact` with checksum state `pending` or `mismatch`;
- `exact` or `strong` whose only agreeing evidence is filename;
- `probable` from duration alone;
- `exact` when two candidates share the checksum.

Reclassification after a checksum job completes is a new decision. The old decision remains in the log.

### Errors

| Error | Class |
| --- | --- |
| Candidate unreadable | `missing` for that candidate, with error text |
| Metadata contradiction on rate or clip id | `mismatch` |
| Manifest Media ID with checksum failure | `mismatch` |
| Algorithm version unknown | Level 4 ignored. Other levels still apply. |

### Recovery

Deleting a fingerprint record removes level 4 evidence. It does not change `media_id` and does not undo a relationship that was accepted on level 2. A relationship accepted on level 4 alone is `probable` and would not have been auto-committed for critical conform.

### Performance

Classification of metadata already in memory is a record comparison. Full-file checksums and fingerprint digests are jobs. The matcher must be able to return `strong` or `probable` while those jobs are `pending`, and must upgrade or contradict later. No timing was measured. Search scope limits (folder, one volume, all volumes) are the caller’s, specified in the relink engine, so a classify call does not walk a facility SAN by itself.

### Tests

Test B, specified and not executed. Required cases: each level; filename-only; duration-only; checksum collision; forged sidecar; unique camera clip id without a hash; pending checksum with a valid Media ID in the project manifest; two files with the same name and different sizes.

### Acceptance

- The six levels and six classes are defined with auto-accept rules.
- Critical conform cannot accept `ambiguous`, `probable`, or unvalidated `strong` by itself.
- Filename and duration have explicit ceilings.
- The fingerprint algorithm is honestly unfixed, and level 4 is specified to stay inert until it is fixed.

## Unresolved questions

- The fingerprint sample grid. Open.
- Whether a facility may enable auto-accept of a unique `strong` match for non-critical path restore. Architecture decision 4. Proposed behaviour: playback may show unverified; the relationship for conform may not change.
- Spanned clips: the checksum at level 2 is the set of part checksums in order, not a hash of a concatenated temporary file that would require rewriting media. Confirm at implementation. Recommendation: ordered part checksums plus the logical recording id.
