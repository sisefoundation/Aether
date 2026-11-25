# AETHER AAF audio exchange profile

Status: proposal for review. This profile specifies what AETHER intends to put in an AAF for audio composition exchange. It does not claim that Pro Tools, Media Composer, or any other application imports that AAF. No AAF file was written or read. No published vendor compatibility matrix was executed.

AAF (Advanced Authoring Format) is the primary audio composition exchange. AVX remains the picture exchange. The native model remains the authority.

## Verified facts

- The brief names AAF as the primary audio composition exchange and asks for an AETHER profile covering multitrack layout, positions, source in and out, handles, fades, crossfades, clip gain, automation, channel layouts, track names, media references, and metadata.
- The brief requires a statement of what cannot be guaranteed across applications.
- The AAF specification is published by the AAF Association. A citation of that body is a pointer for later implementers. It is not evidence that a particular SDK was linked or that a file conformed.
- Channel layout and PCM limits from the prior FFmpeg 9.0.2 MXF study apply to an unvalidated MXF essence path. They also limit what an AAF can promise when the essence is that MXF. They do not by themselves define AAF composition semantics.

## Engineering recommendations

- Export composition structure and media references. Do not require essence to be embedded.
- Point media references at WAV/BWF copies or at the original audio when the user chose that, and include checksums.
- If the only essence is MXF that did not store channel layout, write the layout in the AAF if the profile can represent it, and also state in the report that the MXF itself may not carry layout. Do not claim both containers store it.
- Refuse to call an export “complete” when any required field was skipped. Write `EXCHANGE/AAF/` plus a sibling report.
- Keep picture in the AAF only as optional reference tracks for sync, marked so an audio workstation can ignore them. Do not make AAF the picture master.

## Proposed AETHER behaviour

### Profile identity

| Item | Value |
| --- | --- |
| Name | AETHER AAF Audio Exchange Profile |
| Version | 1 |
| Essence | External references by default. Embedded essence is off unless the user selects a delivery preset that copies WAV/BWF beside the AAF. |
| Time | Sample positions at the sequence audio rate, plus the picture timebase as metadata so frames can be reconstructed |
| Tracks | One AAF track per targeted audio track, names preserved |

### What version 1 tries to carry

| Editorial data | Intended AAF representation | If it cannot be written |
| --- | --- | --- |
| Multitrack | Ordered audio tracks | Fail the export. Do not flatten. |
| Record position | Start position in samples | Fail the export |
| Source in and out | Source clip origin and length | Fail the export |
| Handles | Extra source media beyond the cut, referenced by the longer source clip | Report `handles_not_in_file` if the referenced file was a trimmed consolidate without handles |
| Fades | Fade objects on clip edges | Report and keep a gain automation approximation only if labelled `mapped_equivalent` |
| Crossfades | Overlapping or transition fades between two clips | Same rule. Do not shorten clips silently. |
| Clip gain | Constant level on the source clip | Report if the chosen SDK cannot store it; do not bake unless the user chose “bake gain” |
| Automation | Volume automation as points in sample time | Unsupported curves listed. Points retained in the report even if not in the AAF. |
| Channel layout | A named layout or an explicit map | If unknown, write discrete channels and say `layout_unknown` |
| Track names | Track name and role (dialogue, music, and so on) | Role may be a comment if no standard slot exists. The report says so. |
| Media references | Locator, checksum, media id, original name | Export fails closed if a targeted clip has no locator and no copied essence |
| Metadata | Scene, take, reel, media id in comments or attributes | Attributes a target might strip are duplicated in the AETHER report |

Picture transforms, blends, and nests are not this profile. If included as reference, their state is `metadata_only` and the report says picture intent lives in AVX.

### Loss and bake

Bake is an explicit option: “render clip gain and fades into new WAV files”. Default is off. A bake produces new media ids. It does not replace originals. The AAF then points at the new files and the report says the original clip gain is also listed so the native project remains editable.

### What cannot be guaranteed across applications

These limits are product statements. They are not the result of an interoperability lab.

- A target may ignore unknown attributes, including AETHER media ids, even if the bytes are in the file.
- A target may reinterpret fade curves into its own shapes.
- A target may apply its own pull-up, pull-down, or 29.97 sample alignment and shift sync by a frame or by a fraction of a frame.
- A target may not understand AETHER track roles, nests, speed ramps, or reverse picture. Those are not guaranteed by this profile.
- A target may require OP-Atom MXF audio. AETHER will not claim that requirement is met until the OP-Atom writer is validated. Until then, a WAV/BWF delivery is the specified audio essence, and a facility that requires OP-Atom must be told the export is not that delivery.
- Channel layout stored only in the AAF will not survive a later relink that trusts MXF headers alone, if the MXF has no layout. The prior study is why this sentence exists.
- Plug-in and clip EQ beyond gain are not guaranteed. Version 1 omits them or marks them unsupported.
- Sample-accurate round-trip through another application and back is a desired test. It is not a result. The specified test is: export, import in the target, re-export or record positions, and compare sample numbers. That test has not been run.
- Pro Tools and Media Composer behaviour is unspecified here. Do not document a menu path or a success claim for them without a published source and a fixture.

### Report

```json
{
  "profile": "AETHER-AAF-AUDIO-1",
  "sequence_id": "seq_01H",
  "targeted_tracks": ["A1", "A2"],
  "essence": "external_wav",
  "items": [
    {
      "object_id": "clp_9",
      "state": "native_editable",
      "note": "Clip gain written as a constant level"
    }
  ],
  "not_guaranteed": [
    "Target sample alignment at 30000/1001",
    "Survival of AETHER media_id attributes",
    "MXF channel layout"
  ]
}
```

### Import

Import builds native audio clips from the profile’s fields. Unknown AAF objects become `unsupported` entries and are retained in an attachment on the sequence if they can be stored raw, or listed as not retained. The user sees the list before commit. Import does not delete existing sequences.

```mermaid
flowchart LR
  NAT[Native sequence] --> EXP[Profile exporter]
  EXP --> AAF[AAF plus report]
  AAF --> OTH[Other application unspecified]
  OTH --> BACK[Return path not guaranteed]
  BACK --> IMP[Importer builds a new native sequence]
```

## Rationale

| Decision | Rationale |
| --- | --- |
| External WAV/BWF by default | The MXF PCM and layout limits in the prior study make MXF essence a poor default promise. WAV is inspectable. |
| Fail closed on missing positions | A multitrack AAF with guessed offsets is worse than no file. |
| Bake off by default | Baking destroys the separation of clip gain and fades the audio philosophy requires. |
| Separate report file | AAF consumers are free to drop comments. A sibling report is AETHER’s record of intent. |
| No vendor success claims | The brief forbids uncited compatibility claims. None were earned. |

## Unresolved questions

- Which AAF SDK or library, and its licence. Open in the audit. A purpose-built writer for this narrow profile may be safer than a large SDK. Undecided.
- Whether picture reference tracks are in version 1. Recommendation: one optional picture track, `metadata_only`, off by default, so a dub stage can see sync if their tool allows it.
- Fade curve vocabulary: linear and equal-power only for version 1. Other curves are `unsupported` unless mapped. Open.
- Pull-up and pull-down flags for film and NTSC telecine. Store as metadata if the sequence has them. Do not invent a flag. How targets honour it is not guaranteed.
- Embedded BWF bext and iXML. Recommendation: copy through when present on export of a WAV. Not a conformance claim.
