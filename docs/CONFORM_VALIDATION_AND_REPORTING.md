# AETHER conform validation and reporting

Status: proposal for review. Specification only. The JSON and HTML below are schemas and an illustrative example. They are not the output of a conform run. No PDF renderer is specified.

## Verified facts

- The engine states are `verified`, `verified_with_warnings`, `incomplete`, `failed`, and `manual_review_required` (`ONLINE_CONFORM_ARCHITECTURE.md`).
- A successful export is not a verified conform. A successful relink is not a verified sequence conform.
- HTML can be printed and archived without a separate renderer. PDF can be produced later by a tool this document does not choose.

## Engineering recommendations

- Write JSON as the machine report and HTML as the human report, from the same run record, so the two cannot disagree.
- Put the state in the first screen of the HTML and in a top-level JSON field.
- Include enough identity data for an assistant to see expected versus found without opening the application.

## Proposed behaviour

### Scope

This document defines validation gates that turn engine discrepancies into a state, and the report that stores that outcome. The engine’s walk is `CONFORM_ENGINE_SPECIFICATION.md`.

### Requirements

- Every run produces one JSON document and one HTML document, or the run is `failed` and neither document is presented as a verdict.
- PDF is a later rendering of the HTML. Until a renderer exists, the readable form is HTML. Implementations must not claim a PDF they did not write.
- Required fields: project id, sequence id, revision, export format and version, source application, target application, identity results, source-range and timing validation, effects, missing dependencies, warnings, failures, timestamp, application version.
- Export format and version record the package under discussion, or `none` when the run is an internal conform with no exchange file.
- Source application is AETHER and its version when AETHER produced the sequence. Target application is `aether` for an internal online conform, or the named external profile when the run checks a package aimed at Resolve, Media Composer, or Premiere. Naming a target does not mean that target was executed.
- Sample numbers in the example are illustrative.
- The HTML is a single file with the project’s CSS either embedded or omitted. It does not require a network fetch to be readable.
- The JSON uses strings for rationals, `"n/d"`, never a JSON number for a rate or a speed.

### Data model

```json
{
  "report": "1.0.0",
  "illustrative": true,
  "project_id": "00000000-0000-4000-8000-0000000000aa",
  "sequence_id": "00000000-0000-4000-8000-0000000000bb",
  "revision": "12",
  "export": { "format": "none", "version": null },
  "source_app": { "name": "AETHER", "version": "0" },
  "target_app": { "name": "aether", "version": "0", "tested": false },
  "state": "manual_review_required",
  "timestamp_utc": "2026-11-20T00:00:00Z",
  "app_version": "0",
  "identity_results": [
    {
      "instance_id": "00000000-0000-4000-8000-0000000000c1",
      "media_id": "00000000-0000-4000-8000-000000000001",
      "class": "strong",
      "accepted": false,
      "note": "Checksum pending. Not auto-accepted."
    }
  ],
  "source_range_and_timing": [
    {
      "instance_id": "00000000-0000-4000-8000-0000000000c1",
      "source_rate": "24/1",
      "sequence_rate": "24/1",
      "source_in_frame": 296,
      "source_out_frame_exclusive": 453,
      "source_in_timecode_display": "01:00:12:08",
      "source_out_timecode_inclusive_display": "01:00:18:20",
      "record_in_frame": 3216,
      "record_in_timecode_display": "00:02:14:00",
      "speed": "1/1",
      "range_result": "pass",
      "timing_result": "pass",
      "comment": "Illustrative. Record timecode is not the source timecode."
    }
  ],
  "effects": [],
  "missing_dependencies": [
    { "media_id": "00000000-0000-4000-8000-000000000001", "role": "original", "state": "checksum_pending" }
  ],
  "warnings": [],
  "failures": [],
  "relink_event_id": "00000000-0000-4000-8000-0000000000d1",
  "relink_is_sequence_conform": false,
  "export_event_id": null,
  "export_is_sequence_conform": false
}
```

The two boolean fields exist so a reader of the JSON cannot treat a populated `relink_event_id` as approval.

State rules:

| State | When |
| --- | --- |
| `verified` | Every instance has a critical pass, and warning list is empty |
| `verified_with_warnings` | Every critical check passed, and every warning is on the policy allow-list |
| `incomplete` | A required input or original is not available, and nothing has contradicted the cut |
| `failed` | A comparison contradicted the cut, the report could not be written, or identity mismatched |
| `manual_review_required` | A person must accept a match, an effect, or an ambiguous structure, and no contradiction has failed the run |

If both a failure and an ambiguity exist, the state is `failed`. The ambiguity remains in the body.

HTML structure, normative section ids:

| Section id | Content |
| --- | --- |
| `state` | State, project, sequence, revision, timestamp, app version |
| `identity` | Identity results table |
| `timing` | Source-range and timing table, with a column that labels display timecode as display |
| `effects` | Support per effect |
| `missing` | Missing dependencies |
| `warnings` | Warnings |
| `failures` | Failures |
| `events` | Export and relink event ids, each labelled as not a conform verdict |
| `policy` | Allow-list and match policy id |

The HTML repeats the illustrative banner when `illustrative` is true. Production reports set `illustrative` false. This example document keeps it true.

### APIs

| Operation | Output |
| --- | --- |
| `Report.build(run)` | JSON object and HTML string from the same items |
| `Report.write(run, directory)` | Writes `conform-<sequence_id>-<revision>.json` and `.html`. Refuses to write `.pdf`. |
| `Report.judge(items, policy)` | Returns the state. Pure. |

`Report.write` is called only by `publish` in the engine. A caller that writes HTML without JSON, or the reverse, is non-conforming.

### Workflow

1. The engine finishes comparisons.
2. `Report.judge` sets the state.
3. `Report.build` creates both bodies.
4. The writer stores both, then stores the hashes on the run.
5. The UI opens the HTML from disk. It does not recompute the state locally.
6. A later PDF tool, if one is chosen, reads the HTML or the JSON. It is not part of phase 6’s minimum exit.

### Validation

A report is valid when:

- required fields are present;
- `state` is one of the five;
- `verified` has empty `failures`, empty `missing_dependencies`, and empty `warnings`;
- every timing row includes both a source timecode display and a record timecode display when start timecode is `present`;
- rationals match `"n/d"`;
- `export_is_sequence_conform` is false and `relink_is_sequence_conform` is false;
- the HTML contains the section ids;
- the JSON `state` equals the HTML state text.

### Errors

| Error | What the user sees |
| --- | --- |
| HTML write fails after JSON write | Run `failed`, reason `report_incomplete`. The JSON is retained and marked incomplete. It is not a verdict. |
| JSON parse failure on read | “Report unreadable”. No state is inferred from the HTML alone. |
| Allow-list names an effect the body marks unsupported, and critical identity failed | State `failed`, not warnings. |
| Caller requests PDF | Error `pdf_not_available`. HTML remains the readable form. |

### Recovery

Reports are derived artifacts. Deleting them does not change the sequence. The next conform run writes a new pair. Backup may include the report directory. Restore of an old report does not move the sequence revision backward.

### Performance

Building the report is linear in discrepancies. HTML generation must not embed essence or thumbnails in version 1, so a feature-length report stays a document. No size was measured. A later optional contact sheet is out of scope.

### Tests

Test G includes a schema check of the illustrative JSON shape, a check that `illustrative` examples are not stored as facility results, a check that both event booleans are false, and a check that a `verified` fixture with a missing dependency is rejected by `Report.judge`. Specified, not executed. The numbers `296`, `453`, and `3216` are the illustrative 24 fps example from the time document. They are not evidence a conform ran.

### Acceptance

- JSON and HTML are both specified, and PDF is deferred without a pretend renderer.
- Required fields are present in the schema.
- The example is marked illustrative.
- Judge rules make `verified` incompatible with unresolved dependencies.

## Unresolved questions

- Localisation of the HTML. Recommendation: English section headings in version 1, with machine state tokens left untranslated so tests can find them.
- Whether the JSON schema is published as a separate schema file later. Recommendation: yes, generated from this document during phase 6, not before.
- Signature on the report. Recommendation: store a SHA-256 of the JSON beside it. A cryptographic signature with a facility key is a later security decision, already open in the master set.
