# AETHER offline and online interoperability test plan

Status: proposal for review. Every test in this document is **specified** and **not executed**. No fixture was generated. No external application was launched. Illustrative timecodes and frame indexes are the worked example from `SOURCE_TIMECODE_AND_FRAME_ACCURACY.md`, not observed output.

Phases 1–7 in `OFFLINE_EDITING_ARCHITECTURE.md` are a later implementation order. This plan does not start them. A test becomes executable only after the phase that provides its subject exists, and it still counts as not executed until a run is recorded.

## Verified facts

- The twelve integrity rules and the ten boundaries are specified in `OFFLINE_EDITING_ARCHITECTURE.md`.
- A prior local FFmpeg 9.0.2 study, not run for this plan, reported: OP1a ProRes and DNxHR profiles muxed; PCM only at 48 kHz 24-bit little-endian; channel layout not stored in MXF; a ProRes 4444 alpha descriptor incomplete; a 29.97 audio sample-count mismatch. Those items are reconcile inputs. They are not passes or failures of tests A–H.
- No Resolve, Media Composer, or Premiere version was available to this specification, and none is named as tested.

## Engineering recommendations

- Record execution in a log that names the build, the fixture id, and the result. Keep that log out of this document until a run exists, so the specification does not drift into a false results table.
- Prefer headless calls for relink, registry, and conform. Use the UI only for tests that exist to prove the UI does not block editing and does not decide matches.
- Treat export success and relink success as fixtures that must still fail a conform assertion when a critical check is open.

## Proposed behaviour

### Scope

Tests A–H cover identity, matching, proxies, offline reopen, relink, time and speed, conform and reporting, and interchange including external profiles. Camera ingest and consolidation are cases inside A, C, and E rather than extra letters.

Out of scope: codec bake-offs, colour, and any test that would modify a camera card.

### Requirements

Each test, when it is eventually run, records: fixture id, precondition, steps, expected records, integrity rules touched, and execution status. Until then the status cell is `not executed`.

Expected results below are specification oracles. They are what a future runner compares. They are not claims that the comparison happened.

### Data model

```text
test_case
  id                  A1, B3, ...
  letter              A | B | C | D | E | F | G | H
  title
  status              specified
  execution           not_executed
  phase_gate          1-7, the phase that must exist first
  integrity_rules     list of integers
  oracle              expected records

reconcile_note
  source              ffmpeg-9.0.2-prior-study
  execution           not_a_test_of_this_plan
  use                 input to C and F assertions about layout, alpha, and sample count
```

### Test A — Media ID persistence

Phase gate: 1, with camera cases after phase 3 and consolidation cases after phase 7.

| Id | Oracle |
| --- | --- |
| A1 | After a move, a rename, and a mount-point change, `media_id` is unchanged and was not minted again. |
| A2 | Proxy regeneration keeps `media_id` and issues a new `proxy_id` only when a new proxy record is required. The master clip id stays. |
| A3 | Project migration and a bin export/import round-trip return the same UUID. |
| A4 | A master clip with all representations offline still opens, still names `media_id`, and still has its instances. |
| A5 | A spanned fixture and a CinemaDNG-style stills sequence each receive one Media ID. Required or unclassified sidecars are listed on that recording. |
| A6 | A copy consolidation to a new volume keeps the Media ID. The camera original still exists. |
| A7 | Camera serial availability `absent` still allows a valid identity row. |

Rules: 3, 6, 11.

### Test B — Match classes

Phase gate: 4. Fingerprint cases wait until an algorithm id exists; until then B4’s oracle is “level 4 does not fire”.

| Id | Oracle |
| --- | --- |
| B1 | Manifest Media ID plus matching full checksum classifies `exact` and may auto-accept for critical policy after validation. |
| B2 | Unique full checksum against a completed registry checksum classifies `exact`. Two rows with that checksum classify `ambiguous`. |
| B3 | Unique trusted camera clip id plus agreeing recording metadata, without a checksum, classifies `strong` and does not auto-accept for critical conform. |
| B4 | With no algorithm id, fingerprint evidence does not raise the class. |
| B5 | Unique agreement of timecode, duration, reel, and size classifies `probable`. Duration alone does not classify `exact` or `strong`. |
| B6 | Filename and relative path alone classify `ambiguous` or stay support-only, and are not accepted. |
| B7 | A sidecar that names an existing Media ID with a disagreeing checksum classifies `mismatch` and is not accepted. |
| B8 | Two files with the same name and different checksums remain two Media IDs. |

Rules: 1, 2, 6, 7.

### Test C — Proxies and representation switch

Phase gate: 5.

| Id | Oracle |
| --- | --- |
| C1 | Presets `prores_proxy`, `dnxhr_lb`, `h264`, and `custom` are requestable as contracts. None is reported `validated` on the strength of the prior FFmpeg study. |
| C2 | A finished file fails validation when frame count, rate, timecode, audio sample count, channel map, sync, start timestamp, or `media_id` disagrees. Existence is not a pass. |
| C3 | A failed proxy is not selectable as a conform original and is not returned by playback resolve as a passed representation. |
| C4 | Switching original, proxy, editorial, and automatic, per clip and as a batch, leaves `master_clip_id`, source in, source out, timeline position, sequence duration, metadata, and `instance_id` unchanged. |
| C5 | Pause, resume, retry, and cancel do not mint a Media ID. Retry reuses the reserved `proxy_id` when no passed file exists. |
| C6 | A profile that cannot keep the source rate is refused before a write. |
| C7 | Reconcile input, not a result: an MXF proxy plan records layout as not stored in the container, alpha unverified for a 4444-style plan, and PCM claims limited to the studied 48 kHz 24-bit case until a new study says otherwise. |

Rules: 3, 4, 11, 12.

### Test D — Offline reopen

Phase gate: 5 for proxy-online cases; phase 1 for both-offline.

| Id | Oracle |
| --- | --- |
| D1 | Unmounting originals while a passed proxy remains yields `original_offline_proxy_online`. Editing, including trims that the timeline engine allows, still works. Source ranges change only if the user edits, not because the volume left. |
| D2 | Save and reopen with the original volume still absent restores the same state and the same Media IDs. |
| D3 | Both volumes absent yields `both_offline` or `relink_required`. The master clip remains valid. |
| D4 | Bin columns, the source viewer, and the timeline show the state. The indicators do not block an edit on an available representation. |
| D5 | Modes `offline_proxy`, `original`, and `automatic` change resolution of a path and do not rewrite relationships. |
| D6 | `validation_failed` and `incompatible` are distinct from `missing`. |

Rules: 4, 5.

### Test E — Relink engine

Phase gate: 4. Headless. No widget module loaded.

| Id | Oracle |
| --- | --- |
| E1 | Mount-point change, directory move, rename, other workstation, split volumes, consolidation volume, and a reorganised tree each produce a location update only after class and validation, same Media ID. |
| E2 | The eight procedure steps are observable in the log, and a validation failure leaves the previous relationship. |
| E3 | Scopes folder, one volume, and all listed volumes honour the scope. Unscanned mounts are reported as not scanned. |
| E4 | “All verified” commits only `exact` rows that passed validation. Ambiguous and unmatched remain listed. Manual accept of one `strong` row commits only that row, after validation. |
| E5 | Callers `open`, `bin_import`, `sequence_import`, `proxy_switch`, `conform`, `interchange`, and `batch` receive the same classes as `manual` for the same candidates. |
| E6 | Sequence record positions and speed maps are byte-identical across the commit, except for the log and location rows. |
| E7 | A filename-only candidate is never committed. |

Rules: 1, 2, 5, 6, 10.

### Test F — Time and speed maps

Phase gate: 2.

| Id | Oracle |
| --- | --- |
| F1 | Fixture stored as source start label `01:00:00:00`, source frame in `296`, exclusive source out `453`, record in `3216`, rate `24/1`, speed `1/1`. Display labels `01:00:12:08`, inclusive out `01:00:18:20`, record `00:02:14:00`. Export source timecode is the source label, not the record label. |
| F2 | Parse and format round-trip for `24000/1001`, `24/1`, `25/1`, `30000/1001` non-drop, `30000/1001` drop-frame, `30/1`, `50/1`, `60000/1001`, `60/1`. A dropped DF number is rejected. |
| F3 | Absent start timecode stays absent. A writer that substitutes `00:00:00:00` fails the fixture. |
| F4 | Mixed-rate evaluation uses the integer formula. A float-second field in the stored map fails the fixture. |
| F5 | Maps for normal, constant `1/2`, reverse, freeze, a two-segment ramp, and a three-keyframe remap all round-trip. The ramp export contains the segments. |
| F6 | A receiver capability of “constant only” yields a loss entry and an unchanged stored ramp. |
| F7 | Audio sample positions are integers. A 29.97 fixture whose sample count disagrees with the rational policy sets `sync_warning`. The prior FFmpeg mismatch is the reason this oracle exists. It is not the fixture’s measured count. |

Rules: 8, 12.

### Test G — Conform and the report

Phase gate: 6.

| Id | Oracle |
| --- | --- |
| G1 | A run on a pinned revision includes every instance once and compares duration, rate, track count and order, clip count and identity, Media ID, in and out, positions, durations, speed, transitions, sync, mapping, missing media, missing effects, unsupported effects, gaps, and overlaps. |
| G2 | An export event with no open checks still does not set the state by itself. The fixture “export succeeded, checksum pending” judges `incomplete` or `manual_review_required`, not `verified`. |
| G3 | A relink commit fixture is not `verified` until the conform comparisons pass. The report fields `export_is_sequence_conform` and `relink_is_sequence_conform` are false. |
| G4 | Ambiguous identity judges `manual_review_required`. A range contradiction judges `failed`. A missing original judges `incomplete`. |
| G5 | `verified` is rejected by `Report.judge` when any missing dependency or critical failure is present. |
| G6 | The writer emits JSON and HTML with the required fields and section ids. PDF is not produced. The illustrative sample in the reporting document is not copied into the results log as a facility outcome. |
| G7 | A proxy path supplied as an original is ignored, and a caller with no Media ID fails with `proxy_not_original`. |

Rules: 1, 8, 9, 11.

### Test H — Interchange and external applications

Phase gate: 7.

| Id | Oracle |
| --- | --- |
| H1 | AVX original block and playback block share `media_id`. The original file name is not the proxy file name. |
| H2 | AAF original locator is not the proxy name. The manifest holds Media ID, checksum, reel, roll, clip id, timecode, duration, rate, path, and relative path. |
| H3 | OTIO, FCP XML, and FCPXML packages either contain the source map or list a loss. FCP XML and FCPXML are distinct files when both are requested. |
| H4 | CMX 3600 output is marked limited, contains source timecode distinct from record timecode where the line allows, and is not the only file in the package. |
| H5 | Each path strategy can be named in a package, and `media_id` strategy still includes the standard source reference. |
| H6 | Resolve, Media Composer, and Premiere rows in the compatibility report have empty tested version and `execution: specified`. |
| H7 | A ramp the chosen format cannot store is listed. The native map is unchanged. |
| H8 | Unsupported fields appear in the manifest and the loss list. |

Rules: 8, 11, 12.

External execution remains `not executed` after H6 until someone records a build number and a result elsewhere. Updating this plan’s table from `not executed` to a result is a later edit with evidence, not part of the specification’s current status.

### APIs

The tests call the public operations already specified: `Identity.mint`, `Registry.commitRelationship`, `Match.classify`, `Relink.commit`, `Proxy.complete`, `Playback.select`, `Time.parseTimecode`, `Map.evaluate`, `Conform.run`, `Report.judge`, `Consolidate.commit`, `AVX.exportOffline`, `AAF.exportAudio`, and `Interchange.export`. A test harness is a caller. It is not a new matching policy.

### Workflow

When implementation later exists: build the fixture from stored integers and registry rows; call the operation headless; compare the oracle; write an execution log outside this specification; only then change a status. UI cases D4 and D5 are the exception that opens the indicators and performs an edit. They still use the same registry oracles.

### Validation

A test specification row is valid when it has an oracle that a machine can compare, an integrity rule, a phase gate, and `execution: not_executed` in this document. A row that says “looks correct” is not valid. A row that cites a vendor result is not valid inside this document.

### Errors

| Situation | Oracle |
| --- | --- |
| Phase not built | Test blocked, still not executed. Not a product failure. |
| Fixture uses float seconds as stored time | Fixture rejected. |
| Harness auto-accepts an ambiguous row to get a green bar | Harness defect. The expected result is that the row stays unaccepted. |
| External tool missing | H6 stays `not executed`. The absence is not a Resolve failure. |

### Recovery

Failed future runs keep the project fixture unchanged, because the operations under test either roll back relationships or do not edit the cut. The harness restores fixture files from a read-only copy. Camera-card fixtures, when they eventually exist, are mounted read-only.

### Performance

No performance test is defined with a number, because no measurement exists. A later addition may attach timing to E and G. It must publish the machine and the fixture size. This plan does not invent a frame budget.

### Tests

This document is the test list. It was not executed. Meta-check for reviewers: letters A through H each appear, each has an oracle table, and the FFmpeg study is labelled reconcile.

### Acceptance

- A through H are present with `not executed`.
- Integrity rules 1–12 each appear in at least one test.
- Export-versus-conform and relink-versus-conform are oracles in G.
- External versions are absent.
- Phases are gates, not work started by this plan.

## Traceability

| Rule | Tests |
| --- | --- |
| 1 No silent substitution | B, E, G |
| 2 No auto ambiguous relink | B, E |
| 3 No modification of originals | A, C |
| 4 Switch keeps source range | C, D |
| 5 Relink keeps sequence timing | D, E |
| 6 Filename is not identity | A, B, E |
| 7 Duration is not identity | B |
| 8 No silent discard | F, G, H |
| 9 No verified conform with unresolved dependencies | G |
| 10 No overwrite without validation | E |
| 11 Proxy is not the original | A, C, G, H |
| 12 No float time authority | C, F, H |

## Unresolved questions

- Where execution logs will live. Recommendation: a future `tests/` tree in the implementation repository, not in this specification folder, so results do not rewrite the oracles.
- Fixture media. Recommendation: synthetic files generated by a later suite, plus optional camera samples under a licence the project accepts. This plan does not include samples.
- Whether H6 should be split per application when the first build is tested. Recommendation: yes, as H6a, H6b, and H6c, leaving the parent row as the rollup. Not created now, because no run exists.
