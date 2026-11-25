# AETHER UI and UX design system

Status: proposal for review. No interface was built. The layout below is the proposed default. It is kept from the brief. No change is proposed.

Guiding line: the editor should think about the cut, not about operating the software.

## Verified facts

- Required surfaces: source monitor, record/program monitor, timeline, media browser, track controls, audio controls, metadata, effects, and project navigation.
- Required qualities: configurable workspaces, keyboard-first commands, custom shortcuts, docking, multi-monitor, presets, high-DPI, dark and light themes, accessible scaling.
- The brief asks for a restrained professional feature-film look, without consumer chrome and without decorative motion.
- The brief’s default editorial layout is retained: left column with bins over the inspector; centre with source and record, a timecode bar, and the timeline with a track-control rail; narrow meters on the right.

## Engineering recommendations

- Drive every edit from the command catalogue. Buttons call commands. They do not contain a second copy of trim logic.
- Persist workspaces, shortcuts, and theme as preferences outside the project, with an optional project override for a shared cutting room.
- Use density and type, not animation, to show focus. A trim mode may change a cursor and a one-line mode label. It does not bounce the timeline.
- Target the layout at 1920×1080 and at a 13–14 inch laptop width, and at a second monitor. Specify both. Measurements of a real theme were not taken.

## Proposed AETHER behaviour

### Default editorial layout

```text
+------------------+---------------------------+------+
| Bins             | Source      | Record      | Meters|
|                  |             |             |       |
|                  +---------------------------+       |
|                  | Timecode and transport    |       |
+------------------+---------------------------+       |
| Inspector        | Track rail | Timeline     |       |
|                  |            |              |       |
+------------------+---------------------------+------+
```

| Region | Contents |
| --- | --- |
| Left, upper | Project navigation and bins. Virtual bins. No file move on rename. |
| Left, lower | Inspector: metadata of the selection, or effect parameters when an effect is selected. |
| Centre, upper | Source monitor and record/program monitor, equal weight. |
| Centre, middle | Timecode bar: source, record, duration, mode (insert, overwrite, ripple, roll, slip, slide), and transport. |
| Centre, lower | Track-control rail (target, lock, sync, mute, solo, arm) fixed to the timeline. Timeline tracks scroll horizontally with the rail. |
| Right | Narrow meter column for the monitor bus. Solo and mute state is also on the rail so a meter is not the only clue. |

The meters stay visible in the default preset because a hidden solo is an editorial fault (`AUDIO_EDITING_PHILOSOPHY.md`). The column is narrow so the timeline keeps the width.

### Other surfaces

Effects occupy the inspector or a dock the user opens. They do not replace the timeline. Project settings and preferences are dialogs or a docked panel, not a separate consumer home screen. The project browser is the bin column and an open dialog for other projects.

### Workspaces

Named presets: Editorial (default), Audio (taller track rail, same monitors), Colour tagging (inspector emphasises colour metadata; it is not a grading suite), and Layout only. Users may save further presets. Docking allows a panel to move to another monitor. The timeline and record monitor can be torn off. Closing the last timeline window does not close the project without a save check.

### Keyboard

Every command in the editorial and playback catalogues is bindable. Conflicts are shown. A shortcut set is a file the user can export. The factory set is an AETHER set documented by command name. It is not described as matching Media Composer, Premiere Pro, or Final Cut. Users who want those muscle memories build or import a map. Copyrighted shortcut lists are not copied into this specification.

Focus is visible. The key that trims the playhead goes to the timeline when the timeline has focus, and to the source monitor when the source has focus. The mode line states which.

### Visual language

- Dark theme default, light theme available. Both use the same layout.
- Type is a single utilitarian family at sizes that scale with the OS accessibility setting and an in-app scale control.
- High-DPI: layout in device-independent units. Monitors and waveforms render at device pixels.
- Colour: a small set for track kinds, selection, offline, and sync warning. Offline is a clear mark on the clip, not only a colour, so it survives colour-vision differences.
- No gradients used as decoration, no animated logos, no startup video, no notification badges for features.

### States the UI must show

| State | Presentation |
| --- | --- |
| Empty project | Bins empty, one sequence open, a single line that says to link or copy media. No illustration carousel. |
| Loading project | Named project and a progress line if migration or validation runs. |
| Error on open | Recovery wording from `RECOVERY_AND_BACKUP.md`. The timeline is not shown as if the data were fine. |
| Media offline | Clip label and inspector path. Edit commands still available. |
| Job running | A jobs list with pause, cancel, and retry. Not a modal that traps the cut, unless the job is project creation. |
| Ingest verifying | Status text on the media row. |

### Multi-monitor

A second display defaults to the record monitor at full raster, with timecode. The editing display keeps source, bins, and timeline. The user can swap. Audio meters remain on one display only so two meter drawings do not drift. The chosen display is a preference.

### Accessibility

Scaling at least at 100, 125, 150, and 200 percent is a requirement to design for. Contrast in both themes must keep text and the offline mark distinguishable. Keyboard reach covers commands without a required drag. Drag remains available. Screen-reader labels are required on transport, mode, and dialogs. Full screen-reader coverage of the timeline canvas is an open problem and is not claimed as solved.

### Specified checks, not executed

- At a narrow laptop width, bins and inspector stack or scroll. The record monitor does not collapse to zero.
- At a wide desktop, the default layout matches the regions above.
- A solo engaged is visible on the track rail without opening a menu.
- Changing theme does not move panels.
- A command can be invoked from the keyboard with the pointer outside the window.

## Rationale

| Decision | Rationale |
| --- | --- |
| Keep the brief’s layout | It already places bins, both monitors, timecode, track controls, timeline, and meters where a feature editor looks. Changing it would need a reason. There is none. |
| Inspector under bins | Selection metadata is used constantly and should not cover the picture. |
| Narrow meters | Audio faults must be visible. A wide mixer would turn the default into a DAW. |
| No decorative motion | Motion that is not the cut steals attention and dates quickly. Transport motion is the playhead and the meters. |
| Factory shortcut set is AETHER’s | Claiming another product’s map without a licence and a test would be both a legal and a truth problem. |

## Unresolved questions

- Exact factory key table. Open. It should be written as a table of command ids before implementation, and reviewed by editors. Not invented here as a fake standard.
- Timeline font and tabular figures for timecode. Recommendation: a tabular figure face for timecode, a text face for labels. Specific family is a packaging and licence choice.
- Whether the meter column hides automatically below a width threshold. Recommendation: collapse to a peak-light strip, do not remove solo indication on the rail.
- Touch input. Out of scope.
- Colour-managed UI for picture in the monitors (OCIO). The monitors must be able to show a tagged frame. The UI chrome is sRGB. The wiring is unresolved with the colour decision in the system architecture.
