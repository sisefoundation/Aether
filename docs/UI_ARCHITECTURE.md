# AETHER UI Architecture

Status: proposal, awaiting review. No implementation is authorized by this document.

AETHER is the product name of this editor. [ARCHITECTURE.md](ARCHITECTURE.md), [MODULES.md](MODULES.md), and [DEVELOPMENT_ROADMAP.md](DEVELOPMENT_ROADMAP.md) refer to the same system as SISE Editor. This document uses AETHER.

AETHER is a professional nonlinear editor for feature-film editorial, assistant editorial, and post. The people at the machine are feature-film editors, assistant editors, and post professionals. The workstation is a dense Linux desktop for long-form picture and sound, in the sense of the architecture: cinema, television, documentaries, and long-form film, offline, with no account and no network.

This document specifies the interface architecture and the application-shell foundations. It does not add a module, a widget, a palette file taken from another project, or a build target.

## 1. Scope of this stage

This stage is the written workstation: regions, panels, shell boundaries, workspace rules, keyboard access, and colour and density. The shell those rules describe is an empty, honest frame. Panels are shaped so later engines can drive them. They do not imitate those engines.

In this stage the shell, when it is later built from this document, may draw layout, labels, inactive controls, and empty states. It stops there.

The roadmap's later Qt shell (Phase 9) is the phase that connects real editorial commands. That phase is not this stage. This document is the layout that phase must follow. It does not start that phase, and it does not change the dependency matrix.

## 2. Workflow references

AETHER has its own layout, its own type, its own panel names, and its own key model. The notes below are workflow references for what a feature-film editor expects to be able to do. They are not a claim that another product's interface, source, or pictures are being reproduced.

### 2.1 Editorial engine and timeline

The editorial workflow AETHER is built around is timeline-centred, in the tradition associated with Lightworks.

- The timeline is the centre of the session. Viewers and bins serve the cut.
- Source editing and sequence editing are both first-class. A clip is reviewed in the source viewer. The sequence is reviewed in the record viewer. The timeline is the sequence.
- Trimming is frame-accurate. Record ranges, source ranges, and the playhead are integer frames, as in the architecture's time model. Timecode on screen is a label.
- Clip manipulation covers the editorial command set the Editorial module owns: overwrite, insert, lift, extract, razor, trim, ripple, roll, slip, slide, markers, track enable, and nest placement. The shell will eventually send those as commands. It does not perform them itself.
- Editing is keyboard-driven. The pointer is available. It is not the only path.
- Track management is visible and per-track: identity, targeting, patching, enable, lock, and sync lock.
- Editorial operations are expected to stay responsive. The shell paints snapshots. It does not run trim math on the UI thread, and it does not decode in order to move a cut.
- Sequence organisation lives in the project: more than one sequence, nests as data, and a way to find a sequence without hunting through a single untitled timeline.

### 2.2 Media management and metadata

The catalog workflow AETHER is built around is structured project media, in the tradition associated with Avid.

- A project contains bins. Bins organise master clips, subclips, and sequences.
- A master clip is the catalog identity of source media. A subclip is a named source range of that identity. The shell will show both as clip rows. Identity rules stay in Media Management.
- Source media identity is distinct from a path. Relink matches duration, rate, resolution, sample rate, reel, timecode start, file size, and content hash when present. The path is a hint.
- Reel and tape metadata, source timecode, and clip metadata columns are visible in the bin and in the inspector.
- Offline media remains a clip. The picture well reports offline. The cut is not removed.
- Proxy and original are two locations of one asset (`proxy` and `master`). The bin can show which relationship the session is using. Choosing the policy is an application concern. The shell does not encode a proxy.
- Project organisation is a tree of bins and sequences inside one local project, not a feed of recent files.

### 2.3 Post interchange

The interchange workflow AETHER is built to reach is professional conform, in the tradition associated with DaVinci Resolve: AAF, XML, EDL, and OpenTimelineIO; conform and reconform; timeline handles (head and tail media beyond the cut); relink during conform; preservation of source and timeline metadata; and round-trip validation.

These interchange features are not implemented. The shell must not show them as working. No menu, panel, sample file, progress state, or report in this stage may imply that an AAF, XML, EDL, or OTIO exchange has succeeded, failed, or been previewed. OpenTimelineIO is the built-in format named in the architecture. AAF, EDL, and other XML are later adapter work named in the roadmap. None of them exists in this stage. A future report surface is named in the module map so the shell has a place for it. That surface is absent until the Interchange module exists.

## 3. Where the shell sits

The UI is the presentation layer. It depends on the Application and on Qt 6, which stays private to the UI. It does not include domain headers. Qt Widgets is the toolkit. QML is not part of the design.

The shell issues commands and paints snapshots. The Application is the only seam between panels and the engines. A panel never calls Editorial, Media Management, Interchange, Project Storage, or Processing.

| The shell does | The engines do |
| --- | --- |
| Turn a gesture into a command that carries a frame number | Compute the trim, the relink, the resolve, the transaction, or the picture |
| Draw the snapshot the Application returns | Own tracks, assets, documents, and pixels |
| Show the presented sequence frame the Application reports | Decide which source frame that sequence frame is |
| Show offline, untagged, and missing-effect states that the Application already reported | Probe, decode, relink, and write the package |

Playhead, selection, zoom, and quality policy are session state in the Application. Markers, record ranges, and sequence In and Out points are document state. Workspace arrangement is neither. It is presentation state owned by the UI module on the local machine. Interchange does not carry it. Project Storage does not interpret it.

Tier B panels, if they are added in a later phase, are source inside the UI and may depend only on what the UI may depend on. This stage defines no plugin panel and no second toolkit.

## 4. Workspace layout

The window is one top-level Qt Widgets shell. A menu bar and a thin workspace bar sit at the top. There is no ribbon, no home page, and no card dashboard. The work surface below is three columns in the default arrangement.

### 4.1 Regions

| Region | Role |
| --- | --- |
| Chrome | Menu bar and a single-line workspace bar: project name, workspace name, and a quiet status line. |
| Left column | Full height. Project and Bin Browser above. Metadata and Properties Inspector below. |
| Centre column | The editorial surface. Viewers, then the transport bar, then the timeline. |
| Viewer band | Source Viewer on the left, Record/Sequence Viewer on the right, equal width. |
| Transport band | Timecode and Playback Controls, one bar under both viewers, spanning the centre column. |
| Timeline band | Track Control Area as a rail on the left edge. Timeline Workspace fills the rest. Track rows in the rail and lanes in the timeline share one vertical alignment. |
| Right column | Audio Monitoring Area, a narrow full-height meter column. |

The timeline band is the tallest part of the centre column. The viewer band is shorter than the timeline. The left column is wide enough for a metadata list and narrow enough that the timeline remains the dominant surface.

### 4.2 Default arrangement

The shipped arrangement is named **Editorial**.

- Left column, about a quarter of the window width, full height. The Project and Bin Browser takes about the top three fifths. The Metadata and Properties Inspector takes the remainder. A horizontal splitter divides them.
- Centre column. The viewer band occupies about a third of the centre height. Source Viewer and Record/Sequence Viewer sit side by side at equal width.
- Directly under the viewers, the Timecode and Playback Controls bar spans the centre column. It is outside both viewers so source timecode and sequence timecode stay visible together.
- The timeline band fills every remaining pixel of the centre column. The Track Control Area is a rail of fixed modest width on the left of that band. The Timeline Workspace occupies the rest.
- The Audio Monitoring Area is a full-height column on the far right, wide enough for a dB scale and a small number of meter pairs, and no wider.

```
+------------------------------------------------------------------+
| Chrome: menu, workspace name, project name                       |
+------------------+---------------------------+-------------------+
| Project and Bin  | Source    | Record        | Audio             |
| Browser          | Viewer    | Viewer        | Monitoring        |
|                  +-----------+---------------+                   |
|                  | Timecode and Playback     |                   |
|                  +------+--------------------+                   |
|                  |Track | Timeline           |                   |
| Metadata and     |rail  | Workspace          |                   |
| Properties       |      |                    |                   |
+------------------+------+--------------------+-------------------+
```

Approximate targets, so a later shell can be reviewed against one picture: left column 26% of width; viewer band 34% of centre height; transport bar a fixed two-line strip; track rail wide enough for a short track name plus target, patch, enable, lock, and sync lock; meter column a fixed narrow strip. Splitters remember their positions inside the workspace. The targets are proportions, not a theme.

### 4.3 Narrow window

The same eight panels remain reachable when the window is narrow. The timeline and the record viewer stay on screen.

- The meter column leaves the right edge and sits as a short strip under the transport bar.
- The left column collapses to a tab on the editorial surface. Bin and inspector remain one switch away. They are not removed.
- The two viewers stack. The record viewer stays above the source viewer.
- The track rail stays attached to the timeline.

A second display may hold an undocked panel. An undocked panel is a plain window with the same title bar, the same density, and the same contents. It is not a floating card.

## 5. Panel responsibilities

Each panel has one job. Shared facts (the open sequence, the selected clip, the playhead frame) come from one presentation snapshot inside the UI. Panels do not query each other for editorial results.

### 5.1 Project and Bin Browser

The browser is the project. It shows the open project, its bins, and its sequences as a tree, and the contents of the selected bin beside or beneath that tree.

Bin contents have two views. List view is the default. Thumbnail view is the alternative. Both are arrangements of the same rows.

List view provides:

- metadata columns
- sort by a column
- filter and search
- clip identity on each row
- grouping within a bin

Columns reserved for the catalog, shown as headers in this stage: Name, Type, Reel, Source start, Source end, Duration, Frame rate, Resolution, Tracks, Online, Quality. Type distinguishes master clip, subclip, and sequence. Quality distinguishes master and proxy locations of one asset. Online distinguishes an online location from an offline asset. Search matches the visible identity fields. Grouping organises rows inside a bin. Organisation of bins themselves is the tree: bins may contain bins.

Thumbnail view shows a fixed-size well per clip, the clip name, and a single identity line (reel and source start). It uses the same selection, search, filter, and grouping as the list. It does not replace the columns. The user can return to the list without changing the bin.

The browser is also where sequences are found. Opening a sequence is a future command to the Application. In this stage the tree has its labels and no rows.

### 5.2 Source Viewer

The source viewer is the picture and the status of the source side. It shows the current source clip, source timecode, playback status, and a compact transport. The picture well is a plain rectangle. Timecode sits in the well as tabular figures. Playback status is a word (Stopped, and later Playing) plus the offline or untagged flag when the Application has reported one.

The source transport is limited to review: step and play for the source. Marking a source In and Out, when that exists, is a source-side mark shown in this viewer and echoed on the transport bar. The source viewer does not show the sequence timeline.

### 5.3 Record/Sequence Viewer

The record viewer is the picture and the status of the sequence. It shows the frame at the sequence playhead, sequence timecode, playback status, and a compact transport. It is a separate surface from the source viewer. The two wells are never merged into one preview.

The record viewer shows the frame the Application says was presented. A clock reading is not the displayed frame. When the Application reports an offline layer or an untagged picture, the well shows that state in words inside the well. It does not invent a slate image in this stage.

### 5.4 Timeline Workspace

The timeline is the sequence. It is multi-track. Video tracks and audio tracks are distinguishable at a glance by lane treatment (section 8) and by a V or A mark on the matching track-rail row. Video tracks occupy the upper lanes. Audio tracks occupy the lower lanes. A thin gap separates the two groups.

The workspace provides places, as chrome, for:

- clip selection
- the playhead
- horizontal and vertical zoom
- sequence In and Out
- sequence timecode along the ruler
- source timecode where a clip is selected, as a readout rather than a second ruler
- track targeting, aligned with the rail

The ruler is sequence time in frames, labelled with sequence timecode. The playhead is a single vertical line across every lane. In and Out are a marked span on the ruler. Zoom changes the frame-to-pixel scale of the view. It does not change the sequence.

Clips, when a later snapshot contains them, are rectangles on lanes with a name and a source-timecode hint. Transitions, gaps, and nests are distinct shapes in that later snapshot. This stage draws empty lanes and the ruler. It does not draw sample clips.

### 5.5 Track Control Area

The rail is one row per track, locked vertically to the timeline lanes. Each row shows the track name and provisions for:

- track targeting
- track patching (which source track feeds this record track)
- enable and disable
- lock
- sync lock

Video rows and audio rows are grouped the same way as the lanes. A header on the rail carries the trim-mode control: a compact readout of the trim type (trim, ripple, roll, slip, slide) and whether trim mode is engaged. The readout is chrome. Engaging a trim mode is a future editing command.

Patching, locking, enable, sync lock, targeting, and trim mode are visual provisions. They are not simulated operations. They do not move clips, mute audio devices, or change a document.

### 5.6 Metadata and Properties Inspector

The inspector is the column view of whatever is selected. A bin selection shows catalog fields: display name, master or subclip, reel or tape, source timecode, duration, rate, resolution, online or offline, master and proxy locations, and the identity fields used for relink. A timeline selection shows editorial fields the snapshot already contains: record range, source range, track, conform policy, and effect names as data. A sequence selection shows sequence name, rate, start frame, and track list.

The inspector displays. Writes, when they exist, become application commands. The inspector does not parse a file and does not validate a timeline.

### 5.7 Audio Monitoring Area

The monitoring area is meters and a short status line. It shows playback level for the monitored output, with a dB scale, peak hold as a mark, and a channel count taken from the session when one exists. It identifies the monitor source as the sequence.

It is not a mixing console. It has no plugin slots, no per-clip faders, and no device picker beyond a status label the Application can later fill. Levels, when they exist, are numbers Processing reported. This stage draws the scale at silence and does not animate it.

### 5.8 Timecode and Playback Controls

The transport bar is the console for the session. It shows, as tabular readouts:

- sequence timecode
- source timecode
- sequence In, Out, and duration
- playback status

It shows transport controls: step back, play, stop, step forward, mark In, mark Out, clear marks, and go to In or Out. It repeats the trim-mode readout so the mode is visible while the track rail is scrolled. Viewer transports remain for the picture they belong to. The bar is the place where source and sequence time sit side by side.

Typed timecode entry is a field on this bar. Interpreting that field as a seek is a future application command. In this stage the field is visible and inactive.

## 6. Visual shell and future engine

The shell is allowed to look like a workstation that is ready for engines. It is not allowed to look like the engines have already run.

| Surface | Visual shell | Future engine |
| --- | --- | --- |
| Project and Bin Browser | Tree labels, list and thumbnail chrome, column headers, search, filter, sort, group control, empty bin | Media Management catalog; sequence list from Editorial; open project path from Project Storage; all of it joined by the Application |
| Source Viewer | Empty well, source timecode label, status "No source", inactive transport | Source frame from the session; picture from Processing; source range from Editorial |
| Record/Sequence Viewer | Empty well, sequence timecode label, status "No sequence", inactive transport | Presented frame from the Application; picture from Processing; sequence resolve from Editorial |
| Timeline Workspace | Empty video and audio lanes, ruler, static playhead at the origin, In/Out marks unset, zoom chrome | Editorial snapshot of tracks and items; playhead, selection, and zoom from the Application |
| Track Control Area | Inactive target, patch, enable, lock, sync lock, and trim-mode readout on empty tracks that exist only as lane chrome | Track enable and related mutations through Editorial commands; patch and target as session state through the Application |
| Metadata and Properties Inspector | Labelled rows with empty values | Catalog fields from Media Management; item and sequence fields from Editorial |
| Audio Monitoring Area | Silent meter scale, no motion | Levels and device status from Processing, delivered by the Application |
| Timecode and Playback Controls | Readouts at an empty timecode, status Stopped, inactive transport and mark buttons | Seek, play, and stop through the Application; marks as document state through Editorial; displayed timecode formatted from integer frames |
| Interchange report | Not present | Interchange round-trip report, shown only after a real exchange command exists |
| Menu commands that would edit, import, export, or exchange | Named and inactive | The matching Application operations |

Inactive means visibly inactive: a control that has no engine is disabled, or it is drawn as a provision and refuses input. Activating it does not insert a clip, move a playhead, light a meter, open a file, or post a success message.

Empty bins, empty lanes, and empty wells are the correct picture of this stage. Sample media, a demo sequence, a fake conform, and a bouncing meter are out of bounds for the shell foundations.

## 7. Mapping panels to future modules

The six domain and presentation modules named here are the ones in [MODULES.md](MODULES.md). The Application is listed because it is the only legal caller between the UI and those modules. The shell does not gain a dependency on a domain module by being mapped to it.

| Panel or surface | UI | Application | Editorial | Media Management | Interchange | Project Storage | Processing |
| --- | --- | --- | --- | --- | --- | --- | --- |
| Project and Bin Browser | Widget, views, columns, search chrome | Open, import, and bin commands; catalog and sequence snapshots | Sequence list and sequence identity | Bins, master clips, subclips, identity, offline, proxy and master | — | Which package is open; save does not redraw the tree by itself | Thumbnail wells later use stills the application already requested. The browser does not decode |
| Source Viewer | Well, timecode, status, source transport | Source playhead and source marks as session commands | Source range of the loaded clip | Clip identity shown beside the picture | — | — | The picture, when a request exists |
| Record/Sequence Viewer | Well, timecode, status, record transport | Sequence playhead from the presented frame | The resolved sequence frame | Offline identity for a layer the snapshot names | — | — | The presented picture |
| Timeline Workspace | Lanes, ruler, playhead drawing, zoom, selection drawing | Playhead, selection, zoom | Tracks, items, transitions, nests, markers, In and Out as document state | Asset names on clips, by id, from the snapshot the application assembled | — | — | Waveforms later, as derived pictures. The timeline does not build them |
| Track Control Area | Rail rows and inactive provisions | Patch and target session state | Track enable and the other track commands | — | — | — | — |
| Metadata and Properties Inspector | Rows and labels | The command if a field is edited later | Record range, source range, conform policy, effect names | Reel, source timecode, locations, online flag, proxy and master | — | — | Probe fields already stored on the catalog. The inspector does not probe |
| Audio Monitoring Area | Meter drawing | Forwards the reported levels | — | — | — | — | Meter values and output status |
| Timecode and Playback Controls | Readouts and buttons | Seek, play, stop, typed-frame commands | In and Out, once marks are document edits | — | — | — | Playback submission. The bar does not hold a clock that overrides the presented frame |
| Interchange report (future, not in this stage) | A plain report view, absent now | Export and import commands | Accepts a timeline document | Accepts catalog references, including offline | Parses and writes the exchange and the round-trip report | Stores only after the application commits accepted documents | — |
| Workspace arrangement | Owns it | — | — | — | Does not read it | Does not store it as a timeline or catalog blob | — |

A dash means that module has no responsibility for that surface.

Interchange is mapped to a future report, not to a default panel. Project Storage is mapped to the open package and to save, which appear as the project name and a clean or dirty mark in the chrome, both inactive until a package exists. Processing is mapped to picture, meters, and later thumbnails and waveforms. None of those mappings is a license for the shell to call the module.

## 8. UI module boundaries

The UI module is one link target. Its internal parts are source boundaries, not extra modules and not extra dependencies.

| Part | Owns | Does not own |
| --- | --- | --- |
| Shell | The main window, docks, the menu bar, the narrow-window policy, focus order | Any domain type |
| Workspaces | Named arrangements, local persistence of presentation state | The project package |
| Presentation snapshot | A UI-side copy of the last application snapshot, distributed to panels | The decision that produced the snapshot |
| Panels | Drawing and gestures for the eight surfaces | Trim results, relink, SQL, codecs, OTIO |
| Observer | The single `ISessionObserver`, implemented by the shell when the application is connected | A second path by which a panel listens to a domain module |

Rules:

- Panels talk to the shell's presentation snapshot. They do not include Editorial, Media Management, Interchange, Project Storage, Processing, or Contracts headers.
- Gestures become Application calls in one place, the shell, so a panel cannot grow its own command dialect.
- Qt types stay inside the UI. A snapshot structure that crosses from the Application is made of plain values the Application already exposes. This stage does not invent a parallel document format in the widgets.
- Painting a picture uses only a frame the Application has handed over. If that frame is absent, the well shows the reported empty, offline, or untagged state.
- Domain mutation stays on the session thread, as in the module rules. The UI thread sends a command and paints. It does not decode a file and it does not compute a trim.

This stage does not connect the observer. The boundary is drawn so the connection is a later wiring, not a redesign.

## 9. How workspaces are configured

A workspace is a named arrangement of the eight panels: which region each panel occupies, splitter sizes, visibility, whether a panel is undocked onto another screen, the bin's view (list or thumbnail), the bin's visible columns, and the timeline's lane-height preset (compact or medium). The lane-height preset is a view preference. It does not change the sequence.

- **Editorial** is the default defined in section 4.2. It is always available. Reset restores it.
- Further named workspaces may be added by the user. They rearrange the same panels. They do not introduce a different product surface, a consumer home, or a working interchange desk.
- The workspace bar shows the active name and can switch, save, and reset. Those actions change presentation state only.
- Persistence is local to the machine and local to the UI module. It is not a field of the timeline document, not a catalog record, and not an interchange item. A project copied to another machine does not depend on a workspace file.
- Hiding a panel does not destroy its state. The record viewer and the timeline can be hidden only when the operator does so deliberately. The narrow-window policy does not hide them.
- Panels dock into the regions in section 4.1 or undock as plain windows. They do not become free-floating rounded cards.

Configuration in this stage is specified, not stored. There is no settings file to write until a shell exists, and a shell is not part of this document.

## 10. Keyboard-access principle

The workstation is operable from the keyboard. The pointer may aim at a frame. Every panel, every transport control, every bin view command, and every track-rail provision has a keyboard path that reaches the same control.

- Focus is a visible hairline on the focused panel and the focused control. Focus order in the Editorial workspace is: Bin Browser, Source Viewer, Record Viewer, Timecode and Playback Controls, Track Control Area, Timeline Workspace, Inspector, Audio Monitoring.
- The workspace bar and the menus sit on the same key path as any other desktop menu. A command that exists as a button also exists as a menu item.
- Tab and the focus cycle move between panels. Inside a panel, arrow keys move between rows, tracks, and controls. The bin list, the track rail, and the timeline are lists of rows, not pictures that only a pointer can address.
- Typed timecode is the keyboard path onto a frame. It lives on the transport bar. It stays inactive until seek exists.
- A later key map may bind single keys and modified keys to application commands, including transport and trim. That map is AETHER's. This stage does not publish a live map, and it does not advertise keys as already editing.
- No operation is available only by hovering, and no operation is concealed inside an animation.

The principle is part of the shell foundation. A control added later is added with a focus stop in the same order.

## 11. Colour and density

The visual language is a restrained feature-film workstation. The surface is dense, aligned, and quiet. Type is functional. Hierarchy comes from region size and from a few weights of the same type, not from decoration.

The colour identity is the established AETHER identity based on the SISE workstation palette: dark neutral grounds, light text, and one restrained accent. Numeric tokens are taken from that palette when a shell is built. They are specified here as roles. They are not produced by importing another project's source, stylesheets, or assets.

### 11.1 Colour roles

| Role | Use |
| --- | --- |
| Ground | The window canvas. A dark neutral, short of pure black. |
| Panel | Panel interiors, a small step from the ground so regions separate without a heavy frame. |
| Chrome | Title bars, the menu bar, the transport bar. One step lighter than the panel, still matte. |
| Hairline | One-pixel borders and splitters. |
| Primary text | Names, timecode, column values. |
| Secondary text | Units, empty-state lines, disabled provisions. |
| Accent | One hue, used as a hairline or a short mark: playhead, In and Out, keyboard focus, targeted track, current selection. The accent is not a large fill and not a banner. |
| Picture lanes | Video lanes and audio lanes differ by a small lightness step or a desaturated shift so picture and sound separate at a glance. Both stay inside the neutral range. |
| Warning | A single sparse hue for a reported offline, untagged, or missing-effect state, applied to the status word. It is not a card behind the panel. |

Selection is an outline or a low wash using the accent. Disabled provisions drop to secondary text. Master and proxy are column values, not themes. There are no decorative gradients, no glass, no shadow stacks, and no second accent for ornament.

### 11.2 Density

- UI text is compact. Timecode and frame counts use tabular figures, slightly larger than labels, so a timecode can be read without opening a panel.
- Bin rows are tight list rows. Thumbnail wells are a uniform small grid, with the name under the well, not a caption card.
- Default timeline lanes are short enough that a feature-length stack of video and audio tracks fits the timeline band. The compact lane-height preset is the default. Medium is available from the workspace.
- Padding inside panels is a few pixels. Panel titles are a single short label on a short bar.
- Icons are small and monochrome, used where a word will not fit. Transport glyphs sit at the same size as the label type. There is no oversized iconography.
- Corners are square, or at most a two-pixel radius on a control. Buttons are rectangular.
- Alignment follows one grid. The track rail and the timeline lanes share row boundaries. Viewer timecode and the transport readouts share a baseline. Column headers in the bin align with the inspector labels where the same field appears in both.
- Motion is reserved for a later playhead that is tracking a reported frame. This stage has no transition, no animated meter, and no animated reveal of panels.

The result is high information density with a clear order: timeline first by area, viewers second, bin and inspector as text, meters as a narrow instrument.

## 12. Not in this stage

The following are outside this stage. They remain future work under the architecture and the roadmap. The shell must not imitate them.

- Any implementation: Qt widgets, CMake, module libraries, plugins, and sample media.
- Simulated or real editorial operations: overwrite, insert, lift, extract, razor, trim, ripple, roll, slip, slide, markers, track enable, nesting, undo, and redo.
- A playhead that moves, a zoom that reveals clips, a selection of clips, or a trim mode that changes a sequence. The provisions are drawn inactive.
- Media import, probe, relink, offline simulation, proxy generation, and any picture in a viewer or a thumbnail well.
- Playback, decode, waveforms, meters that move, device audio, and export.
- Project creation, open, save, autosave, and any `.sise` package.
- AAF, XML, EDL, and OpenTimelineIO import or export. Conform, reconform, timeline handles as a working conform feature, relink during conform, and round-trip validation. These are not implemented and must not be shown as working. No sample exchange, no sample report, and no menu item that claims to run one.
- Colour grading, OpenColorIO views, and an export dialog that encodes.
- A live keyboard map that performs edits.
- Populated demo bins, a demo sequence, or decorative animation.
- Importing code, stylesheets, icons, or layout from any other project. The SISE palette informs colour roles only.

What this stage does contain is this document: the Editorial arrangement, the eight panels, the shell-versus-engine line, the module map, workspace configuration, the keyboard principle, and the colour and density rules.

## 13. What a later connection looks like

When a review authorizes the shell, the first UI slice builds the Editorial workspace as empty and inactive, matching sections 4 through 11. A later slice connects `ISessionObserver` and the Application commands. The panels stay where they are. The snapshot gains rows, lanes, and a presented frame. Interchange appears as the report surface in section 7 only after a real exchange command exists, and the report lists what the Interchange module actually returned.

Until that review, AETHER's workstation exists as this specification.
