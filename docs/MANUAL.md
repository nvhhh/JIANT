# JIANT FM — User Manual

**Firmware v0.6.30 for the M-VAVE FM-1**

> *Versión en español: [MANUAL.es.md](MANUAL.es.md).*

This manual is for musicians who have just installed JIANT FM and want to know how to play it, build songs with it
and, above all, **save their work without losing anything**. It follows the firmware as it is in v0.6.30: every
button, knob and message is named exactly as the device shows it.

If you only read one chapter, read [12. Saving and loading](#12-saving-and-loading).

## Contents

0. [Quick start](#0-quick-start)
1. [The panel](#1-the-panel)
2. [The screen](#2-the-screen)
3. [Pages and layers](#3-pages-and-layers)
4. [Engines](#4-engines)
5. [DRUM-X](#5-drum-x)
6. [The sequencer](#6-the-sequencer)
7. [ARRANGE: songs, sections and scenes](#7-arrange-songs-sections-and-scenes)
8. [Dice and UNDO](#8-dice-and-undo)
9. [Effects and master](#9-effects-and-master)
10. [Modulation and macros](#10-modulation-and-macros)
11. [Performance](#11-performance)
12. [Saving and loading](#12-saving-and-loading)
13. [System: TRACKS, LOAD and the MENU](#13-system-tracks-load-and-the-menu)
14. [MIDI and USB](#14-midi-and-usb)
15. [Web tools: emulator, installer, editor, backup](#15-web-tools-emulator-installer-editor-backup)
16. [Troubleshooting and FAQ](#16-troubleshooting-and-faq)

---

## 0. Quick start

### Install

1. Open the **web installer** in Chrome or Edge: <https://juanjiant-bit.github.io/JIANT/webapp/installer/>
2. Connect the FM-1 **directly** to the computer with a USB data cable.
3. **Coming from another custom firmware (Felucca, baudgirl or any other)?** Return to the official V15 firmware
   first, with the installer's **Return to official V15 first** button (it also saves a complete backup). Writing
   JIANT straight over another custom firmware has left an FM-1 unable to start.
4. Press **Install**. It first checks the FM-1's firmware and writes only over official V15 or JIANT (anything else: it
sends you to Return to official V15 first). Don't unplug the cable while it writes.

JIANT has no user samples: if you come from Felucca, its user samples (USR1–3) are lost on install. Keep your WAV
files.

### Your first sound (1 minute)

1. Power on. The signature draws itself, then you land on **HOME**: six creatures, one per track. (0.9.9) The tracks
   start as **T1 DRUM-X** (the factory kit), **T2 VOICE** (SINGER EN: it sings words), **T3 ANIMAL** (CMOS BIRD),
   **T4 LOFI** (BYTEBEAT), **T5 ANALOG** (FAT BASS) and **T6 FM6** (TINE EP), each its preset's sound, dry.
2. Turn **ALGORITHM** to pick a track (T1–T6). With the default **TRACKS 4** it stops at T4: T5 and T6 appear only with TRACKS 6 (see
   [13](#13-system-tracks-load-and-the-menu)).
3. Turn **PRESETS** to browse sounds. Play the keys.
4. Turn **KNOB 1–4**: on HOME they are the four most useful controls of the track's engine.
5. Don't like it? Hold **EDIT** and turn **SELECT**: the sound is rolled into a new variation and you hear it at once.
   **OCT+ OK** keeps it, **OCT− CANCEL** brings the sound back.

### Your first song (5 minutes)

1. Hold **HOME** and turn **SELECT** (or on HOME tap **HOME** twice: ECO → BEING → **GEN**). The **GENERATOR** opens on
   **EXPLORE** (no genre, every rule broken): press **OCT+** as it is, or pick RANDOM or one of the 43 genres with KNOB 1
   or PRESETS first. A whole song idea appears: engines, sounds, parts, scale, tempo and effects. Press **PLAY**.
2. Not your thing? Press **OCT+** again: a new seed, a new piece. Too busy? Turn **SELECT** one detent right for GEN 2 and lower
   **DRUMS** or **ARP**, then OCT+: the same piece with that changed. **OCT−** goes back to ECO. To go back to what was there before: hold **GLO** and press **G4** (UNDO).
3. Like it? **Stop** (PLAY), then press **SAVE** (it opens USER, the user sounds) and press **SAVE** again: the
   **PROJECT** group's **SECTIONS**, the project's four sections. Turn **KNOB 4** to the right so its card says **OCT+**, then press
   **OCT+**. Type a name with the keys (or keep the one offered) and press **OCT+** again. The screen says **SAVED**.
4. Make a variation (change some steps, mute a track) and save it into section **B**: on SECTIONS turn **KNOB 1** to
   **B**, then KNOB 4 and OCT+ as before.
5. Turn **SELECT** one detent right: **ARRANGE** (the song's rows). Press **OCT+** to add a row, set its section with
   **KNOB 2** and its length with **KNOB 3** (bars). Add a row for A, one for B. Press **PLAY**: the song plays the
   rows in order.

That's the whole loop: **make music → save it into a section → chain sections in ARRANGE**.

---

## 1. The panel

The names below are the ones the firmware uses (the HARDWARE CALIBRATION screen asks for them by these names).

### Encoders and knobs

| Control | What it does |
| --- | --- |
| **SELECT** (encoder) | Alone (0.9.2): **walks the section's pages**, one page a detent, as one chain, stopping at its first and last page (no levels, no zoom; see [How navigation works](#how-navigation-works-six-rules-092)). In the MENU: its pages. With a button held: **HOME** + SELECT **GEN**, **EDIT / SEQ / LFO / FX** + SELECT that part's **dice**, **GLO** + SELECT the **tempo**, **ENV / SCL / ARP / SAVE** + SELECT walk that section's chain from wherever you are. |
| **ALGORITHM** (encoder) | Selects the track from any page: T1–T8 (T1–T4 with TRACKS 4, T1–T6 with TRACKS 6). The track is always ALGORITHM's (0.9.2: PRESETS no longer picks it on ENV, LFO, CHORD, ARP). In the MENU: the next / previous tab. With a layer held: its view, MAP ↔ KNOBS ↔ STATE. |
| **PRESETS** (encoder) | **Always presets** (0.9.2): the selected track's sounds (every engine's factory presets, then your user presets) on every page; on the **SEQ** pages the **patterns** (it opens the PHRASES loader with the next pattern picked: OCT+ loads it, OCT− back to the SEQ page); on **GEN** the GENRE. In the MENU and with a button held it does nothing. It never changes page, track, module or row. |
| **KNOB 1–4** | The four cards (columns) on screen. A page that picks something picks it with **KNOB 1** (the step, the bar, the slot, the stage, the operator, the kit's sound, the row). With a layer held, the layer's own knobs. Turning fast sweeps the whole range, slowly fine-tunes (MENU › SYSTEM › KNOB SPEEDUP turns this off). In the MENU: KNOB n the setting on card n. |
| **MASTER** | The output volume. |

### Buttons

| Button | Tap | Hold |
| --- | --- | --- |
| **HOME** | HOME (0.9.2, as every section's button): from another section the HOME page last shown; on HOME the first page of the next group, round: **ECO → BEING → GEN → ECO**. | About 0.7 s: opens the **MENU**. |
| **ENV** | The ENV section: from elsewhere the page last shown (at power-on ENV); inside, the next group: **TRACKS → ENV → STAGE**, round. | (no layer) |
| **LFO** | The LFO section: from elsewhere the page last shown (at power-on LFO); inside **MOD → LFO → MSEQ**, round. | **MACRO** layer: KNOB 1–4 are M1–M4. |
| **FX** | The FX section: from elsewhere the page last shown; inside, the next module: **TRACK → BUSES → DELAY → REVERB → CHORUS → SLICER → MASTER → PUNCH**, round. | **Punch-in FX** layer. |
| **SCL** | The SCL section (0.9.2: its own, ARP apart): **KEY ↔ CHORD**. | **SCL** layer: keys set the root. |
| **EDIT** | The EDIT section: from elsewhere the page last shown (one memory for the synths, one each for FM6 and DRUM); inside the next group: **SOUND → TONE → FOCUS** (FM6: SOUND → TONE → OPERATOR; DRUM: KIT → SOUND → LEVELS → TONE). Always EDIT (0.9.2: clearing a step is SEQ + G5, renaming SAVE + C5). A step held + EDIT still clears that step's locks. | **EDIT** layer: INIT, RECALL, DRUM-X sound mutes. |
| **GLO** | The GLO section: from elsewhere the page last shown (at power-on MIXER); inside **LIVE → MIX → GLOBAL → SYSTEM**, round (0.9.11: LIVE, the live sequences, first). | **GLO** layer: mutes, solos, levels, tap tempo, UNDO. |
| **SAVE** | The SAVE section: from elsewhere the page last shown, and USER or SECTIONS come with their **SAVE already picked** (SAVE, OCT+, OCT+ as before); inside **SOUND → PROJECT → LIBRARY**, round. | **SAVE** layer: play/store sections live; C5 NAME. |
| **ARP** | The ARP section (ARP · ARP 2, one group): from elsewhere the page last shown; inside, back to ARP. A chord held + ARP during step recording still arpeggiates it. | (no layer) |
| **SEQ** | The SEQ section: from elsewhere the page last shown (at power-on STEP, the piano roll); inside the next group: **PATTERN → TRACK → BAR → STEP → AUTO**, round. | **SEQ TOOLS** layer (on every SEQ page); G5 CLR STEP. |
| **PLAY** | Start / stop. | — |
| **REC** | Arms / disarms recording on the selected track. | **REC** layer: CLEAR, CLICK. |
| **OCT− / OCT+** | Keyboard octave (−3 to +3; both together: back to 0; on a DRUM track the kit stays on its keys: ratchets above, SHORT / GHOST / FLAM below, see [5](#5-drum-x)). On action pages and in dialogs: **OCT−** = back / NO, **OCT+** = do it / YES. | — |

### The keys

27 keys from **F3 to G5** (16 white, 11 black). They play the selected track. With a layer held they become that
layer's shortcuts; on the DRUM grid they are steps and lanes; on the NAME screen they type.

### Useful combinations

| Combination | Result |
| --- | --- |
| **GLO** held + **PLAY** | Restart from the top without stopping (internal clock). |
| **GLO** held + **SELECT** | Tempo, from any page. |
| **GLO** held + **G4** | **UNDO** the last big change (again: redo). |
| **SEQ** held + **REC** | **CLEAR ALL SEQUENCES?** — all six tracks' steps and automation, and the punch-in lane. |
| **REC** held + **FX / EDIT / ENV / LFO / SCL / ARP / GLO** | Clears that part's automation on the selected track (GLO: the level and pan automation of all tracks; FX: also the punch-in lane). No dialog. |
| **SAVE** held + **OCT+** / **OCT−** | Store the music into the section playing / recall that section as stored. |
| **HOME** held + **SELECT** | **GEN**, the song generator (HOME's last pages; see [8](#8-dice-and-undo)). |
| **EDIT / SEQ / LFO / FX** held + **SELECT** | The dice: the sound / the sequence / the LFO / the effects (see [8](#8-dice-and-undo)). |
| **ENV / SCL / ARP / SAVE** held + **SELECT** | Into that section and along its chain, as SELECT inside it. (0.9.2: a button held + **PRESETS** no longer walks pages.) |
| **SEQ** held + **G5** | **CLR STEP**: clears the cursor step and its locks (NOTE with a note in focus: that note; AUTO LIST: deletes the record). |
| **SAVE** held + **C5** | **NAME**: renames the slot on SAVE > USER, the section or project on SECTIONS / PROJECTS. |
| A step held (SEQ > STEP) + **KNOB 1–4** | A parameter lock on that step. |
| A step held (SEQ > STEP) + **SELECT** | (0.9.12) The page those knobs lock: walks the track's EDIT pages (KIT → KIT 2 → LEVELS …; EDIT 1 → EDIT 2 → FILTER …) without leaving STEP. |
| **OCT− + OCT+** held at power-on | HARDWARE CALIBRATION. |
| **OCT− + OCT+** held 5 s while running | Update mode (UBOOT), with a countdown from 2 s; let go to cancel. |

---

## 2. The screen

### The look: GARDEN (stems-10)

Every page stands in **the garden**, a pixel scene drawn by code: a sky from pale blue to cream, a spiky sun, clouds
drifting, rain (more drops the louder the music; they fall with the tempo), the horizon's dots, a windmill and a house,
a fence, the ground (soil in five tones, grass, blue tufts that bounce with the level). The scene **moves on the
music's clock**: slowly in silence (never frozen), up to twice as fast at full level, times **MENU › SCREEN › MOTION**
(100 %, 50 %, 150 %, 200 %); **ANIMATION OFF** stills it. On HOME the whole garden lives; behind the other pages it
stands still, and only the page's own plants and creatures move.

- **Header**: the breadcrumb at the left (**EDIT · OSC**), at the right REC, the lock, the track's pebble, the play
  state, the BPM (it rolls as it changes), the load meter and the battery.
- **Tabs**: the groups as pebbles along the top, the one you are on orange.
- **Panel**: a cream card with a burnt edge in the middle of the garden. ENV, LFO, FX and PATTERN have a scene instead
  (see below), BEING stands in the garden with no card.
- **Pods**: KNOB 1–4 are small acorns along the bottom: a burnt cap with the knob's name, the body with the value and
  its bar; the knob you turn has an orange body.
- **Footer**: the track and its sound at the left, the page's place in its chain at the right.

**MENU › LARGE** keeps its big cards (an accessibility option). COLOR, SKIN, STYLE and ECO STYLE are retired: GARDEN
is the one look (settings saved with other values load as GARDEN; nothing else in them changes).

**Page scenes.** **ENV**: one large tulip growing through the envelope of the voice playing. **LFO**: a row of six
tulips swaying with the LFO. **FX**: a stream of two wavy ribbons with an apple, an orange and cherries hopping on the
beat. **PATTERN**: sixteen dirt mounds in two rows, a tulip on each step that plays and a seed on each one that rests,
the playhead's mound orange. DRUM-X's **KIT** and **SOUND** pages: a ring of eight mushroom caps, each jumping on its
lane's hit. FM6's **OPERATOR** pages: a fern, a pair of fronds an operator, each frond as long as its operator's level.

**The letters**: names, tabs, the breadcrumb and the small captions are in a crisp 5 x 7 pixel face; values are in a
larger rounded face. **HOME** is the garden's own page: ECO (the standby), BEING (the track's creature) and GEN (see
[the sections](#the-sections-and-their-chains)). On the **MIXER**, asleep tracks are greyed and say OFF.

**Dialogs** ask before anything destructive: **OCT− NO**, **OCT+ YES**. (0.8.5, as the mock-up) They carry the
tabs **NOTICE · DICE · NAME**: a question is a card under a violet rule with its two buttons (YES in the accent); a
part's dice (EDIT, SEQ, LFO, FX + SELECT) shows **BEFORE** and **NOW** side by side (the sound's being then and now,
the steps, the LFO's or the effects' values) over a strip **KEEP DICED …? · SELECT: DICE AGAIN · OCT− CANCEL ·
OCT+ OK**; NAME has its cards CURSOR, CHAR, LEN and OCT+ SAVE / WRITE over the name, the keys' letters and the black
keys' functions. (0.9.5) A question's card covers only itself, its tabs and its breadcrumb: the page stays in sight
around it, still moving.

**Messages** (0.9.5): a message on a lens page shows as a small pill near the foot of the panel, over the page, which
keeps drawing and moving behind it (the header says it as well; not over HOME's beings or a layer's map); nothing goes
dark. The pill keeps clear of the page's own words: where the foot is written (AUTO LIST's rows, MUTATE's lines) it
sits in the nearest free rows, and on a page with no free rows it is left out and the header alone says it. A page's
**hint** (MUTATE's "OCT+ KEEP  GLO+A4 FREEZE", MOTION's and PUNCH's "REC + PLAY ...") comes only once you stay on the
page for about 0.8 s, never while SELECT walks past it, and each one at most twice in all (counted with the settings,
so not again after a reboot). SAVE › TOOLS › NEW PROJECT starts them over. Tapping a layer button says nothing (the
"HOLD [X] QUICK" hints are gone): hold it and its layer shows itself.

---

## 3. Pages and layers

[![The navigation map: every section, its groups and its pages in chain order, the six rules, the held layers and the combos](navigation-map.png)](navigation-map.svg)

*The whole synth on one page: each section's button, its groups (the tabs on the screen) and its pages in the order
SELECT walks them (the number in each dot is the page's n of the footer's n/N; orange, the page at power-on; dashed,
a page only some engines or LOADs have), with the six rules, the held layers and the combos. Click it for the zoomable
SVG ([en español](navigation-map.es.svg)). It is generated from the firmware's chain tables (`tools/gen_nav_map.py`).*

### How navigation works: six rules (0.9.2)

Since 0.9.2 every section is driven the same way (the design the owner approved, the mock-up "navegación"): learn it
once and every section is known. Until 0.9.1 each part was a lens of three levels with sets of knobs; the pages, their
knobs, their pictures and every function are the same, only the way between them changed.

1. **SELECT walks the pages.** Turning SELECT goes through all the pages of the section, in one chain, a page a
   detent, and stops at the first and the last. No levels, no zoom. FX: SENDS → DIST → AUTO PAN → BUSES → DLY →
   DLY 2 → DLY 3 → REVERB → … → PUNCH.
2. **The section's button: in, or on.** From another section it brings you back to the page you were last on there.
   Already inside, it jumps to the **first page of the next group** (and goes round). On FX › DLY 2, FX goes to
   REVERB; on EDIT › FILTER, EDIT goes to FOCUS.
3. **PRESETS loads presets.** Always: the track's sounds in every section, the **patterns** in SEQ (the PHRASES
   loader), the **genre** in GEN. It never changes page, track or module (in the MENU it does nothing).
4. **KNOB 1 picks.** When a page picks something (a step, a bar, a slot, a stage, an operator, the kit's sound, a
   row) KNOB 1 is the one that picks it. The track is always **ALGORITHM**'s. SEQ › STEP: KNOB 1 walks all 64 steps,
   bar after bar; the new **SEQ › BAR** page: KNOB 1 the bar.
5. **Hold = the layer.** Holding a button opens its layer of shortcuts, as before (past the HOLD time, MENU › SYSTEM › HOLD TIME,
   0.4 s by default: the keys and knobs change function while you hold it, a map shows what each key does; a key or a
   knob opens it at once; let go and you are back). Held + SELECT: **HOME** GEN, **EDIT / SEQ / LFO / FX** their dice,
   **GLO** the tempo, **ENV / SCL / ARP / SAVE** walk their chain. (Held + PRESETS no longer walks pages: rule 3.)
6. **The screen says where you are.** On top, the tabs are the section's **groups**, the one you are on lit and a dot
   for each of its pages. At the foot: **SECTION · GROUP · page** and **n/N**, the page's place in the chain:
   **FX · DELAY · DLY 2  6/16**. The same in AIR, COLORS and HUD, every palette.

- Each section remembers the page it was left on (EDIT one for the synths and one each for FM6 and DRUM); at power-on
  SEQ opens on STEP, ENV on ENV, LFO on LFO, the others on their first page.
- **SCL and ARP are two sections** (until 0.9.1 one lens): SCL is KEY · CHORD, ARP is ARP · ARP 2 (one group: an ARP
  tap inside goes back to ARP).
- **GEN is three HOME pages** (GEN 1–3, HOME's last group), no longer a screen of its own: the section buttons work
  there, OCT+ makes the piece, OCT− goes back to ECO. HOME + SELECT still opens it from anywhere.
- **SAVE from outside** comes back to the SAVE page you left; **USER** and **SECTIONS** come with their SAVE already
  picked, so SAVE, OCT+, OCT+ still stores the sound (or the section).
- **EDIT is always EDIT**: clearing a step went to the SEQ layer (**SEQ + G5**, CLR STEP; on AUTO LIST it deletes the
  record; TIME turned to REST clears a note too) and renaming to the SAVE layer (**SAVE + C5**, NAME).
- **The MENU** (hold HOME, 0.9.18): the same rules: SELECT walks its nine pages (SCREEN → AUDIO → MIDI → USB →
  SYSTEM), ALGORITHM jumps tabs, KNOB 1–4 the settings on the cards, HOME closes it.
- Some pages only appear when they apply: FM6's operator pages on an FM6 track, DRUM-X's pages on a DRUM track,
  RETRIG on LOFI BYTE/FLOAT, and with **LOAD ECO** the SLICER and CHORUS pages are hidden; the chain skips them.

### The sections and their chains

SELECT goes down each row from left to right; the button jumps to the next group's first page. Every page is the one
it was in 0.9.1, with its knobs and its picture (the ones 0.8.5's lenses drew).

| Section | Groups (the button) · pages (SELECT) | Pages |
| --- | --- | --- |
| **HOME** | **ECO**: ECO · MACROS — **BEING**: BEING · ROUTES — **GEN**: GEN 1 · GEN 2 · GEN 3 | 7 |
| **EDIT** (a synth) | **SOUND**: EDIT 1 · EDIT 2 — **TONE**: FILTER · MORPH · RETRIG · ROTOR · VOICE · VOICE 2 · WORDS (those the engine has) — **FOCUS**: FOCUS | up to 10 |
| **EDIT** (FM6) | **SOUND**: EDIT 1 · EDIT 2 — **TONE**: FILTER · VOICE · VOICE 2 — **OPERATOR**: OPERATOR · OP ENV · OPERATOR 2 | 8 |
| **EDIT** (DRUM) | **KIT**: KIT · KIT 2 · KIT 3 — **SOUND**: SOUND · SOUND 2 · SOUND 3 · SOUND 4 — **LEVELS**: LEVELS · LEVELS 2 — **TONE**: X-MOD · FILTER · STEREO · VOICE | 13 |
| **SEQ** | **PATTERN**: PATTERN · STRETCH · PLAYHEADS — **TRACK**: TRACK · MUTATE — **BAR**: BAR · STEP · NOTE — **STEP**: COMP · COMP 2 — **AUTO**: AUTOMATION · AUTO LIST | 12 |
| **FX** | **TRACK**: SENDS · DIST · AUTO PAN — **BUSES**: BUSES — **DELAY**: DLY · DLY 2 · DLY 3 — **REVERB**: REVERB · REVERB 2 — **CHORUS**: CHORUS — **SLICER**: SLICER · SL BUS · SL BUS 2 · SL BUS 3 · SL PITCH — **MASTER**: MASTER — **PUNCH**: PUNCH | 16 |
| **GLO** | **LIVE**: MANO · RIELES — **MIX**: MIXER · SENDS — **GLOBAL**: GLOBAL — **SYSTEM**: SYSTEM | 6 |
| **ENV** | **TRACKS**: TRACKS · TRACKS DEST — **ENV**: ENV · ENV DEST — **STAGE**: STAGE | 5 |
| **LFO** | **MOD**: MOD — **LFO**: LFO · LFO DEST — **MSEQ**: MSEQ · MSEQ 2 | 5 |
| **SCL** | **KEY**: SCL — **CHORD**: CHORD | 2 |
| **ARP** | **ARP**: ARP · ARP 2 | 2 |
| **SAVE** | **SOUND**: USER · PRESETS — **PROJECT**: SECTIONS · ARRANGE — **LIBRARY**: PROJECTS · TOOLS | 6 |

Where a page lived in 0.9.1: ENV's six-envelope views (the TRACKS level) are now pages of their own, **TRACKS** and
**TRACKS DEST**, with ENV's and ENV DEST's knobs; **NOTE** appears once (the BAR group; its big-step picture is
COMP's); FX's **SENDS** is the FX page; SAVE's PROJECT page is **SECTIONS** (the PROJECT level) and **PROJECTS** (the
LIBRARY level); the DRUM kit's EDIT 1 / EDIT 2 are **KIT** / **KIT 2**. The new **BAR** page is PATTERN's picture of
the bars with KNOB 1 the bar and KNOB 2 LEN.

- **ECO** (GARDEN): the standby garden, the whole screen: a mushroom, a bird and a frog wandering the ground (they
  turn back and face the way they go) and two beetles crawling; all but the mushroom hop on the beat, and all swell
  with the music's level. **MACROS**: KNOB 1–4 are **M1–M4** (the macros, as LFO held), the routes counted at the top
  left, LATCH at the right. **X-Y view**: on ECO turn **SELECT left**: the whole screen becomes a vectorscope (L along
  the top-left diagonal, R the top-right, M up, L-R / L+R across), the last moment of the output as fading blue dots
  and an orange head: a mono sound is a line up M, a wide one opens out, a sound in one side leans to its diagonal.
  **SELECT** either way goes back to ECO.
- **BEING**: the track's creature large in the garden (see [the creatures](#the-engines-creatures-garden)), its four
  HOME knobs as pods at the corners ("K1 CUT" over each, a blue gauge and the value under it, a dotted vein to the
  creature), the sound's name under it. **ROUTES**: M1–M4 down the left with their routes drawn to where they go
  (**T2 CUT+62**).
- **ENV** › **TRACKS** / **TRACKS DEST**: the six tracks' envelopes as tiles (the selected one tinted with its level now a dot; DEST:
  FILTER, PITCH and SHAPE as bars under each), the knobs the selected track's — **ALGORITHM** picks the
  track. **ENV**: the envelope large / ENV DEST's columns, as before. **STAGE**: KNOB 1 picks the stage (ATK DEC
  SUS REL), KNOB 2 sets it, KNOB 3 LOOP; the whole envelope with the stage shaded over the stage's own
  curve, large, and its time.
- **GLO** › **LIVE** (0.9.11): MANO, the keys as a board of live modifiers over the six sequences, and RIELES, the six
  sequences as rails with their playheads and ghosts ([11](#live-sequences-glo--live-0911)).
- **GLO** › **MIX**: the six tracks as strips (MIXER: LEVEL with the meter, PAN, REV; **SENDS**, new: each track's
  DIST, CHORUS, DLY and REV as four bars, the knobs the selected track's). **GLOBAL**: the bar of sixteenths, as before.
  **SYSTEM**: the tracks that play as chips, LOAD's three settings (the one set lit), MIDI's route, the CPU load over the
  last seconds as a line.
- **SCL · ARP** › **KEY**: the twelve notes round a circle (C on top), the scale's lit and joined, ROOT in the accent
  with the scale's name in the middle, QUANT and ALL TR at the right, the scale on a keyboard under it; KNOB 1 sets
  ROOT. **CHORD**: the chord each degree of the key plays now (I–VII, as a key would play it: one note while CHORD is
  OFF or the voice isn't POLY). **ARP** / **ARP 2**: "MODE · RATE · OCT" and the pattern it plays over a triad (GATE
  the cells' length, SWING the off-beats late, PROB some left out); OFF says so.
- **LFO · MOD** › **MOD**: the matrix as a net — the sources down the left (those a slot reads lit), a curve from each
  slot's source to its box **D1–D4** on the right (DEST, AMT as a bar), the slot KNOB 1 picked framed with a pulse
  running along its curve. **LFO**: the wave under "RATE · WAVE" / **LFO DEST**: the LFO small
  on the left, the four amounts as rows lit by what the LFO sends now. **MSEQ** / **MSEQ 2**: the
  sixteen steps under their caption; KNOB 1 the step.
- **EDIT** › **SOUND**: the engine's creature on the left (0.9.4 / 0.9.6: each engine its own, see below; alive with the sound, the page's name over it, the sound's
  under it), the scope in a well and all eight E parameters as chips on the right, the page's four lit. **TONE**:
  **FILTER** draws TYPE's response at CUT with its RES peak (LP, HP, BP, COMB), CUT a line, DRIVE / DIST a row of
  cells; **VOICE** / **VOICE 2** the voices one a row as full as they sound, VOICE and PRIO (VOICE 2: ALLOC), GLIDE's
  curve; the others keep their pictures. **FOCUS** (new): one E parameter large as a gauge — KNOB 1 picks it (E1–E8),
  KNOB 2 sets it, KNOB 3 the **source** of the MOD slot aimed at it (turned up from OFF a free slot takes it, its DEST
  that parameter; back to OFF frees it), KNOB 4 that slot's **AMT**; E1–E8 as tabs under it.
- **GEN 1–3** are the generator (as HOME held + SELECT, from any page): **SELECT** turns its page (left of GEN 1:
  ROUTES), **PRESETS** the GENRE, **OCT+** makes the piece, **OCT−** back to ECO, a **HOME** tap on to ECO.

### Every page and its four knobs

"—" means an empty card. An "action" card, picked (turn the knob right), is tinted violet with its name violet and says **OCT+**; press OCT+ to do it.

**ENV** (per track)

| Page | KNOB 1 | KNOB 2 | KNOB 3 | KNOB 4 |
| --- | --- | --- | --- | --- |
| ENV | ATK | DEC | SUS | REL |
| ENV DEST | FILTER | PITCH | SHAPE | LOOP |
| STAGE (0.8.5) | STAGE (ATK DEC SUS REL) | its value | LOOP | — |

**LFO** (per track)

| Page | KNOB 1 | KNOB 2 | KNOB 3 | KNOB 4 |
| --- | --- | --- | --- | --- |
| LFO | RATE | WAVE | MODE (TRIG / FREE) | FADE |
| LFO 2 (0.9.18) | PHASE (TRIG's; dimmed in FREE) | — | — | — |
| LFO DEST | PITCH | FILTER | SHAPE | AMP |
| MOD | SLOT (1–4) | SRC | DEST | AMT |
| MSEQ | STEP (1–16) | LEVEL | LEN | SLEW |
| MSEQ 2 | DIV | — | — | — |

**FX** (TRACK pages are per track; the rest are global)

| Page | KNOB 1 | KNOB 2 | KNOB 3 | KNOB 4 |
| --- | --- | --- | --- | --- |
| FX | DIST | CHORUS (send) | DLY (send) | REV (send) |
| DIST | DIST | TYPE | TONE | — |
| AUTO PAN | DEPTH | RATE | SYNC | WAVE |
| BUSES (0.8.5) | MIX (the delay's) | SIZE (the reverb's) | DEPTH (the chorus') | DEPTH (the SLICER's) |
| SLICER | MODE | PAT | RATE | DEPTH |
| SL BUS | T1 | T2 | T3 | T4 |
| SL BUS 2 | T5 | T6 | T7 | T8 |
| SL BUS 3 | FX | — | — | — |
| SL PITCH | STEP | PITCH | LEN | QUANT |
| DLY | TIME | FEEDBK | TONE | MIX |
| DLY 2 | WIDE | PITCH | SPRAY | MOD |
| DLY 3 | TYPE (ANALOG / DIGITAL) | PING (OFF / ON) | — | — |
| REVERB | TYPE | SIZE | DAMP (RESO: FOLLOW) | PRE (SHIMMER/CLOUD: AIR; RESO: PITCH) |
| REVERB 2 | MOD | RATE | FILT | WIDE |
| CHORUS | RATE | DEPTH | STEREO | — |
| MASTER | CLIP | PUNCH | DUCK | REL |

**SCL**

| Page | KNOB 1 | KNOB 2 | KNOB 3 | KNOB 4 |
| --- | --- | --- | --- | --- |
| SCL | ROOT | SCALE | QUANT | ALL TR (0.7.6; was TRANS, and SCL 2 held ALL TR alone) |

**EDIT** (per track; which pages appear depends on the engine)

| Page | KNOB 1 | KNOB 2 | KNOB 3 | KNOB 4 | Shown on |
| --- | --- | --- | --- | --- | --- |
| EDIT 1 | E1 | E2 | E3 | E4 | every engine (see [4](#4-engines)) |
| EDIT 2 | E5 | E6 | E7 | E8 | every engine |
| WORDS | LANG | LYRIC | POS | (OCT+ TYPE: the lyric) | VOICE (0.8.5, [words](#voice-words-and-lyrics-085)) |
| RETRIG | RTRG | — | — | — | LOFI with WAVE on BYTE or FLOAT |
| MORPH | MORPH | — | — | — | ANALOG (SAW..SIN), PHASE, LOFI (up to WRAM) |
| OPERATOR | OP | RATIO / FIXED | FINE | LEVEL | FM6 |
| OP ENV | OP | STAGE | RATE | LEVEL | FM6 |
| OPERATOR 2 | OP | MODE | DTUN | VEL | FM6 |
| FILTER | TYPE | CUT | RES | DRIVE (ANALOG) / DIST (others) | every engine |
| KIT 3 | BP | FEEDBK | HANDS | — | DRUM (the whole kit) |
| X-MOD | RATE | DEPTH | SHAPE | RAND | DRUM (the whole kit) |
| STEREO | IMAGE | — | — | — | DRUM (the whole kit) |
| SOUND | SOUND | SIDE | PITCH | PMOD | DRUM (one sound) |
| SOUND 2 | SOUND | DECAY | NOISE | COLOR | DRUM (one sound) |
| SOUND 3 | SOUND | MODE | PM.TYPE | DRIVE | DRUM (one sound) |
| SOUND 4 | SOUND | LAYER | FEEDBK | SEND | DRUM (one sound) |
| LEVELS | KICK | SNARE | CLAP | HATCL | DRUM |
| LEVELS 2 | HATOP | TOM | RIM | BELL | DRUM |
| VOICE | VOICE | GLIDE | MODE | PRIO | every engine |
| VOICE 2 (0.8.5) | ALLOC | DETUNE (UNI's spread) | — | — | every engine but DRUM |
| FOCUS (0.8.5) | PARAM (E1–E8) | its value | SRC (the MOD slot on it) | AMT | the synths (not FM6, DRUM) |

**GLO** (MANO and RIELES act on the tracks picked; MIXER is per track; GLOBAL and SYSTEM are global)

| Page | KNOB 1 | KNOB 2 | KNOB 3 | KNOB 4 |
| --- | --- | --- | --- | --- |
| MANO (0.9.11) | RATE | LOOP | THIN | DJ FILT (MIXER's; the keys: the live modifiers) |
| RIELES (0.9.11) | PISTAS (ALL, T1 … T6, SYN, DRM) | RATE | CORTE | DENS |
| MIXER | LEVEL | PAN | REV | DJ FILT (0.9.1: the master DJ FILTER; was MUTE, now on the GLO layer) |
| SENDS (0.8.5) | DIST | CHORUS | DLY | REV (the selected track's, as FX > SENDS) |
| GLOBAL | BPM | SWING | CLOCK | TUNE |
| SYSTEM | TRACKS | LOAD | ROUTE | CPU (%) |

**SAVE**

| Page | KNOB 1 | KNOB 2 | KNOB 3 | KNOB 4 |
| --- | --- | --- | --- | --- |
| PRESETS | No. (the sound) | ENG (the engine) | FAV (mark a favourite) | LIST (ALL / FAV / a sound type: BASS LEAD PAD KEYS PLUCK BELL VOX DRUM FX, 0.9.12) |
| USER | SLOT (U01–U32) | LOAD (action) | ERASE (action) | SAVE (action) |
| PROJECTS / SECTIONS (the PROJECT page) | PROJECT (1–8; on SECTIONS: SECT A–D) | SECT (A–D; on SECTIONS: the section's NAME) | LOAD (action) | SAVE (action) |
| ARRANGE | ROW ("2/6") | SECT (on END: STOP / x2–x8 / LOOP) | BARS | SCENE |
| TOOLS | any knob picks from the list: CLEAR SEQ, INIT SOUND, DELETE ROW, CLEAR SONG, NEW PROJECT, CPU TEST | | | |

**ARP** (per track)

| Page | KNOB 1 | KNOB 2 | KNOB 3 | KNOB 4 |
| --- | --- | --- | --- | --- |
| ARP | MODE | RATE | OCT | GATE |
| ARP 2 | SWING | PROB | HOLD | ORDER |

**SEQ** (per track)

The SEQ pages are one chain of five groups (0.9.2; see [6](#finding-your-way-the-seq-chain)): **SELECT** walks
them page by page, a **SEQ** tap jumps to the next group.

| Group (SEQ tap) | Page (SELECT) | KNOB 1 | KNOB 2 | KNOB 3 | KNOB 4 |
| --- | --- | --- | --- | --- | --- |
| PATTERN | PATTERN | LEN | DIV | SWING | GATE |
| PATTERN | STRETCH | STRETCH (/8 … /2, x1, x2 … x8) | REPEAT (OFF, x2 … x8, TO 64) | NUDGE | ROTATE (−32 … +32) |
| PATTERN | PLAYHEADS | HEAD | RATE | DIR | INTERVAL |
| TRACK | TRACK | PITCH | CHANCE | HUMAN | REC Q |
| TRACK | MUTATE | SOUND (every) | PATTERN (every) | AMOUNT | MODE |
| BAR | BAR (0.9.2) | BAR (the bar you edit) | LEN | — | — |
| BAR | STEP (melodic: piano roll) | STEP (all 64, bar after bar) | NOTE | TIME (REST ← TIE ← NOTE → 2 ST … 16 ST: the note's length in steps; REST clears the step) | FLAG (– / ACC / SLD / A+S) |
| BAR | STEP (DRUM: grid) | STEP | LANE | HIT | ACC |
| BAR | NOTE | FOCUS | PITCH | MOVE | MICRO (−32/64 … +31/64; 0.9.1) |
| STEP | COMP | STEP | TYPE | (its value; ARP: ORDER × RANGE; RATCH: x1–x8 × FLAT FADE RISE) | EVERY (ARP: RATE; RATCH: RATE and its pitch step) |
| STEP | COMP 2 | STEP | TYPE (the second component) | (its value) | EVERY |
| AUTO | AUTOMATION | PLAY (ON/OFF) | EVENT (count) | LOCK (count) | CLEAR (action) |
| AUTO | AUTO LIST | ROW | STEP | PARAM | VALUE |

PRESETS on any SEQ page loads a pattern (PHRASES); the bar is BAR's KNOB 1, the step STEP's (0.9.2; until 0.9.1
PRESETS did both).

### The layers (hold the button)

(0.8.5) A held layer is a lens too, as the mock-up: **turn ALGORITHM while you hold the button** to go **MAP ·
KNOBS · STATE** (it remembers the level; SELECT keeps its combos: GLO + SELECT the tempo, the dice). (0.8.9, in the
skins) **MAP** is the keyboard itself, F3 to G5: every key that does something is named upright on it (light on the
white keys, tinted on the black ones), the one held lit orange, the value now violet, a muted track pale, what cannot be
pressed now dim, and each white key's note under it; **KNOBS**: the four as gauges in their knobs' colours (JIANT: their values' heat), the layer's
key words under them (e.g. **PLAY RESTART · GLO DONE**); **STATE**: what is on, as chips (FX: the lock, the tracks, the
latch, the effects held; GLO: each track ON / MUTE, the drum groups, UNDO, MUTATE; LFO: the latch and each macro's
routes; REC, SAVE, SCL, EDIT, SEQ theirs). The header names the layer in words, **[GLO] SET**. The footer is the
breadcrumb, **FX · MAP**. The keys and knobs do exactly what they did.

**FX — punch-in effects** (details in [11](#11-performance))

| Keys | Effect |
| --- | --- |
| F3 G3 A3 | REPEAT 1/8, 1/16, 1/32 |
| B3 / C4 | GLITCH / TAPE STOP (0.9.15; 0.9.17 the knobs are theirs while held, TAPE STOP scratches) |
| D4 / E4 | OCT− / OCT+ |
| F4 | 1/2 TEMPO |
| G4 / A4 | DEC− / DEC+ |
| B4 / C5 | STUTTER 1/8 / 1/16 |
| D5 | ATK+ |
| E5 | ARP |
| F5 | RANDOM |
| G5 | erases the punch-in lane |
| Black keys 1–6 (F#3 G#3 A#3 C#4 D#4 F#4) | mute T1–T6 while held (T3–T8 while T7 or T8 is selected) |
| G#4 | FREEZE (0.9.17): the eighth after the press held as grains |
| A#4 | which tracks the note effects touch: ALL / SYN / DRM |
| C#5 | SHUFFLE (0.9.17): the last bar's 1/16s in another order (the FILTER sweep's key until 0.9.16) |
| D#5 / F#5 | the sweeps CRUSH / THROW (0.9.15): dry to wet over 2 bars while held, back in a beat |
| **Knobs** | the performance macros STEPS, TRANSPOSE, MUTATE, FILTER (0.9.15): up to the right; any knob turned back turns all four off. While GLITCH, TAPE STOP, FREEZE or SHUFFLE is down: its own four (0.9.17) |

**GLO — mix**

| Keys | Action |
| --- | --- |
| Black keys 1–6 | MUTE T1–T6, or T3–T8 while T7 or T8 is selected (latched: press again to unmute) |
| Black keys 7–10 (G#4 A#4 C#5 D#5) | DRUM-X group mutes: KICK, SNARE, HAT, PERC |
| F3–D4 | SOLO T1–T6 (T3–T8 while T7 or T8 is selected) while held |
| E4 | unmute everything (tracks and drum groups) |
| F4 | TAP tempo (from the third tap; internal clock only) |
| G4 | **UNDO** (again: redo) |
| PLAY | restart from the top |
| SELECT | tempo |
| **Knobs** | LEVEL T1–T4 (with T5 or T6 selected: T3–T6) |

**SCL** — any key sets the ROOT to its note name. Knobs: ROOT, SCALE, ROTATE, PITCH (the last two are SEQ > STRETCH's ROTATE and SEQ > TRACK's PITCH).

**EDIT** — F3 **INIT** (the track's sound back to its engine's own defaults (a plain INIT, not a preset), dry: every effect send at 0; FM6 the init
voice, OP1 alone at full level; on DRUM the factory DRUM-X kit too), G3 **RECALL** (the sound as the current section stored it; on DRUM with its kit). Both ask first and leave the
steps alone. On a DRUM track, black keys 1–8 mute each DRUM-X sound (BD SD CP CH OH TM RS CB). Knobs: EDIT 1's four.

**SEQ TOOLS** (on every SEQ page) — F3 CLEAR, G3 REVERSE, A3 / B3 SHIFT left / right,
C4 RANDOM, D4 COOK, E4 BEAT (DRUM), F4 CLR AUTO (the track's automation and locks; the steps stay). On a DRUM track the
lane tools: G4 CLR, A4 REV, B4 FILL, C5 RND on the lane picked with black keys 1–8. **G5 CLR STEP** (0.9.2; was
the EDIT tap): clears the cursor step and its locks, the cursor on to the next (on NOTE with a note in focus: that note
alone; on AUTO LIST, **DEL REC**: deletes the row's record). **REC** (while SEQ is held): CLEAR
ALL. Knobs: LEN, DIV, SWING, GATE. These tools have no undo.

**REC** — F3 **CLEAR** the selected track's steps and automation (no dialog; UNDO can bring it back), G3 **CLICK**
(OFF / REC / ON). KNOB 1: CLICK.

**SAVE — song layer** (details in [7](#7-arrange-songs-sections-and-scenes))

| Keys | Action |
| --- | --- |
| F3 G3 A3 B3 | play section A / B / C / D (next bar while playing; at once when stopped). Several in one hold: a quick chain. |
| C4 D4 E4 F4 | STORE the music into A / B / C / D |
| D5 | LOOP / SONG: what PLAY plays |
| E5 | SONG REC |
| G5 | go to the ARRANGE page |
| C5 | **NAME** (0.9.2; was the EDIT tap): rename the slot on USER, the section on SECTIONS, the project on PROJECTS (elsewhere dim) |
| OCT+ / OCT− | store into / recall the section playing |
| **KNOB 1** | the project (1–8); applied when you let go of SAVE, stopped only |

**LFO — macros**: KNOB 1–4 are M1–M4. Let go and they stay **latched** (the header says [LFO] LATCH, and KNOB 1–4
keep controlling the macros on every page) until you press LFO again. F3 rolls new routes, G3 clears them.
(LFO + SELECT rolls the track's LFO, with KEEP / CANCEL: see the dice.)

---

## 4. Engines

Nine engines, all synthesis, no samples. Any engine can go on any track. Pick them with **PRESETS** (one list over
every engine) or with KNOB 2 (ENG) on SAVE > PRESETS. Each track's four HOME knobs are listed too.

**The order** (0.9.9): PRESETS, KNOB 2 (ENG) and the web editor's lists show the engines as **DRUM-X, VOICE, ANIMAL,
LOFI, ANALOG, FM6, PHASE, WHEEL, NOISE** (the first six are the tracks at power-on, T1–T6). Only the order you see
changed: projects, user presets, favourites and backups name engines by their numbers, which stay, so everything saved
before loads as it was. PROGRAM CHANGE numbers the sounds in this new order (see [14](#14-midi-and-usb)).
SHRUB (0.9.10) comes last, after NOISE, so no number or place of the older engines moves.

**Sound types** (0.9.12, after Skiff): every factory sound has a type: **BASS, LEAD, PAD, KEYS, PLUCK, BELL, VOX, DRUM,
FX**. On SAVE › PRESETS, **KNOB 4 (LIST)** steps ALL → FAV → BASS → LEAD → … → FX: with a type, KNOB 1 and the PRESETS
knob browse only that type's sounds, **across every engine** (BASS: ANALOG's SUB BASS, FM6's FM BASS, PHASE's GROWL,
LOFI's TRI BASS, WHEEL's REED BASS …), and loading one keeps the steps as always. The type is kept with the settings;
your user presets show under ALL and FAV. The dice (GEN) uses the types too: a bass sound only on the bass.

**Smooth changes**: switching a track to another engine fades the old one out (about 12 ms) and what you are holding
(keys, or the notes the sequencer holds) goes on in the new engine, fading in; nothing is cut. Changing a list value
(WAVE, CHIP, a type) while notes sound makes a short dip (about 3 ms) instead of a click; the notes go on.

| Engine | EDIT 1 | EDIT 2 | HOME knobs |
| --- | --- | --- | --- |
| **ANALOG** | WAVE, DETUNE, MIX, NOISE | CUT, RES, DRIVE, INT | CUT, RES, ATK, REL |
| **FM6** | ALG, FB, MOD, RATIO | MEG, VMOD, DETUNE, SLOT | MOD, RATIO, MEG, SLOT |
| **PHASE** | WAVE, WAVE2, DCW, ENV | DETUNE, LINE, SUB, FB | DCW, ENV, DETUNE, REL |
| **LOFI** | CHIP, WAVE, DUTY, CRUSH | —, VIB, BEND, MASK | WAVE, DUTY, CRUSH, REL |
| **VOICE** | VOWEL, VOWEL2, TALK, SHIFT | BUZZ, BREATH, Q, RAND | VOWEL, BUZZ, BREATH, TALK |
| **WHEEL** | REG, SUB, BODY, TOP | PERC, CLICK, DRIVE, ROTOR | REG, PERC, DRIVE, ROTOR |
| **NOISE** | MODE, COLOR, FREQ, PEAK | TRK, DENSE, DRIFT, CRUSH | FREQ, PEAK, COLOR, REL |
| **DRUM** (DRUM-X) | MORPH, TUNE, TONE, DECAY | NOISE, FM ENV, FOLD, DRIVE | MORPH, TUNE, TONE, DECAY |
| **ANIMAL** (0.8.3) | MODE, PRESS, CHAOS, SIZE | SWEEP, RATE, GROUP, TEMP | PRESS, SWEEP, RATE, SIZE |

- **ANALOG**: two oscillators. WAVE: SAW, SQR, TRI, SIN, PWM, SYNC, RING, SAW3. **INT** sets oscillator 2's interval
  (±24 semitones). Its filter is per voice.
- **FM6**: a six-operator FM engine (Dexed's msfa), 32 algorithms drawn on screen, four voices per track. The EDIT 1/2
  knobs are macros over the patch. **SLOT**: the factory patches F1–F8, OWN (an edited or imported patch) and B1–B32
  (a 32-voice DX7 bank sent from the web editor; empty slots are skipped). A voice from the editor or a B slot plays
  as its file has it: the macros go back to none (0.8.1). **OPERATOR / OP ENV / OPERATOR 2** edit each
  of the six operators on the device (KNOB 1 picks the operator); editing makes the patch the track's own (SLOT OWN),
  saved with the project. A bank voice you pick is copied into the track, so projects keep it even if you send
  another bank later. (0.9.6) Their panel is the algorithm as a **colony of operators** (OP ENV: the envelope with the
  operator riding it), every number they showed kept.
- **PHASE**: phase distortion. WAVE / WAVE2: SAW, SQR, PLS, DSIN, SPLS, RSAW, RTRI, RTRP. LINE: MIX or RING.
  **FB** feeds the output back into the phase.
- **LOFI**: chip sounds. CHIP: 4BIT, 4B/2, 8BIT, 1BIT, STEP. WAVE: PLS, TRI, SAW, NOIS, WRAM, **BYTE**, **FLOAT**.
  - **BYTE** (bytebeat): KNOB 3 becomes **ALGO** (32 formulas), KNOB 4 **VAR** (mutates the formula); EDIT 2 becomes
    **MOTION, GRIT, BEND, LOOP**. MOTION speeds the formula's rhythm up or down (64 = as written), GRIT is a rate
    crusher, BEND folds time, LOOP repeats a tiny stretch (high LOOP turns the noise into a tuned note).
  - **FLOAT** (floatbeat): CHIP becomes the resolution (4BIT, 4B/2, FULL, 1BIT), KNOB 3 **ALGO** (32 formulas made of
    sines), KNOB 4 **VAR**; EDIT 2: **MOTION, FEEDBK, BEND, LOOP**. Hold a key: the formula plays in that key.
  - **RETRIG** page (BYTE / FLOAT): **RTRG ON** restarts the formula from the top on every note.
  - (0.9.11) **Level and edges.** LOFI's waves are full-scale and dense (a bytebeat is nearly a square), so at the same
    meter reading they sound louder and sharper than the other engines. LOFI now plays them lower, by wave (BYTE and
    FLOAT −6 dB, PLS SAW NOIS −4.5 dB, WRAM −3 dB, TRI −1.5 dB), and files their top a little: a fixed gentle low-pass
    (−3 dB at ~7.5 kHz) on the chip waves and FLOAT, and BYTE's own smoothing of its steps a bit lower. The FILTER
    page's CUT is unchanged (127 still opens it fully). Every LOFI sound gets it, user presets and **old projects too:
    they sound quieter and slightly softer than in 0.9.10** — raise the track's LEVEL (+6 dB = 12 steps) if a mix needs
    it back.
- **MORPH** (EDIT > MORPH, on ANALOG, PHASE and LOFI): the wave slides continuously toward the next ones (on ANALOG
  SAW > SQR > TRI > SIN, on PHASE toward its next shape, on LOFI toward the next wave up to WRAM). 0 = the wave as
  chosen. On ANALOG and LOFI the **SHAPE** modulation moves the morph too, so an LFO or envelope sweeps the wave. The
  page draws one cycle of the morphed wave and says which two waves it blends. KNOB 2 is **DRIFT** (0–100 %, 0 = off):
  each voice's pitch (up to about ±27 cents) and filter wander slowly, each in its own way, as old analog oscillators
  do. A chord comes alive; at high values it goes seasick.
- **VOICE**: a formant voice (after klattsch): vowels, talk, breath. Since 0.8.5 it also sings words and lyrics in
  Spanish, English and Japanese: see [VOICE: words and lyrics](#voice-words-and-lyrics-085).
- **WHEEL**: a drawbar organ: registrations (REG), percussion (PERC: OFF, 2ND, 3RD, 2SOFT, 3SOFT, 2SLOW, 3SLOW),
  key click, drive, ROTOR (OFF, SLOW, FAST). EDIT > **ROTOR** (WHEEL only): **DEPTH** (how much the rotary speaker
  swirls: 100 % as before, 0 % a still speaker) and **DUST** (0–100 %: an old organ's crackle and hiss while it plays).
- **NOISE**: coloured and metallic noise. MODE: ANLG, DUST, LFSR, META (DUST is only the crackle: at DENS 0 it still crackles softly). PEAK is its own resonance.
- **DRUM**: DRUM-X, see [5](#5-drum-x).
- **ANIMAL**: animal calls from physical models, see [ANIMAL](#animal-083) below.
- **SHRUB** (0.9.10): a wavetable synth with eight edit slots, see [SHRUB](#shrub-0910) below.

### ANIMAL (0.8.3)

Animal calls built the way animals make them: **a source that oscillates or strikes** (a bird's syrinx, a cricket's
file and scraper, a cicada's tymbal, a frog's larynx, a wing) **exciting one or two resonators** (the wing, the
abdomen, the vocal sac, the vocal tract), driven by a slow layer of **pressure, tension and rhythm**. **MODE** picks
the source; every mode reads the eight knobs its own way and the screen relabels them. The key sets the tension (the
pitch, the carrier), the velocity the pressure; **a note starts a call**, so a sequence of notes is a sequence of calls,
in tune with the rest of the music.

| MODE | EDIT 1 (KNOB 2..4) | EDIT 2 | What it models |
| --- | --- | --- | --- |
| **CMOS** | TOUCH, COUPLE, CAP | HYST, RATE, GROUP, FM | the 40106 / 4093 "bird machine" (below) |
| **PULSE** | CLICK, Q, SIZE | DETUNE, RATE, GROUP, TEMP | crickets, katydids, cicadas, frogs and toads |
| **WHSTL** | PRESS, CHAOS, SIZE | SWEEP, RATE, GROUP, TEMP | birds' whistles (and a bat, transposed) |
| **WING** | BUZZ, DRIFT, SIZE | DOPPLR, RATE, GROUP, TEMP | mosquito, bee, fly |
| **GLOTT** | MOUTH, ROUGH, SIZE | SWEEP, RATE, GROUP, AIR | mammals: wolf, dog, roar, purr |

**The call's rhythm** (every mode but CMOS): **RATE** is the syllable rate (0.05–40 a second); **GROUP** how they
group: **HOLD** (one syllable as long as the note: a coo, a howl, a drone), **TRILL** (no gaps) or *n*+*m* (*n*
syllables, then *m* silent ones: 3+9 is a field cricket's chirp of three); **TEMP** (°C, 22 as written) scales every
rate the way temperature does in insects and frogs (Dolbear's law: a cricket at 30 °C chirps about 2.4 times as fast
as at 14 °C). **SIZE** moves the whole animal up or down from the key, a semitone a step (64: as played, lower:
smaller and higher; GLOTT: the vocal tract's length instead, every formant together, the pitch left alone).
**SWEEP** is each syllable's pitch gesture: an exponential hook from below (+) or above (−) onto the note, up to two
octaves; above 16 syllables a second it narrows, as a bird's trill can only be that fast and that wide (Podos).

- **CMOS**: the owner's breadboard circuit, a 40106 / 4093 Schmitt-trigger chip, two big capacitors, three pots and a
  probe touched by hand ("a machine of bird, monkey and bug songs"). A fast RC oscillator (an octave above the key:
  C5 chirps at ~1.05 kHz, as in the video) charges and discharges its capacitor exponentially between two thresholds;
  its square wave is band-limited at the exact edge. A slow one gates it (the 4093's NAND) and its capacitor's voltage
  sweeps it (the 40106's FM): the exponential charge is the "hook" you hear at the start of each chirp. **RATE** the
  slow one (the chirps a second), **CAP** how long it charges against how long it discharges (short blips .. long
  sweeps), **HYST** its threshold window (the hook's depth; + rises onto the note, − falls onto it), **FM** 0 the gate
  alone (beeps), 20 gate and sweep (the bird), 40 the sweep alone (a siren), **TOUCH** the hand on the probe (faster,
  less regular), **COUPLE** a second fast oscillator coupled both ways (locking, then chaos: bugs), **GROUP** a third,
  very slow one that groups the chirps into phrases, speeding up as it goes (monkeys).
- **PULSE**: an escapement. Each tooth of a cricket's file (each rib of a cicada's tymbal, each pulse of a frog's
  larynx) is an impulse into a resonator tuned to the key (the wing, the abdomen, the vocal sac). **CLICK** 100 %:
  one pulse a cycle of the resonance, the clockwork cricket's pure tone; lower, one pulse every 2 .. 64 cycles: clicks
  ringing it (a cicada's buzz, a frog's croak; at 1/2 a treefrog's two peaks an octave apart). **Q**: broadband
  (a katydid's rasp) to pure (a cricket). **DETUNE**: the pulses off the resonance (beats, an "anomalous" song) and
  jittered.
- **WHSTL**: a syrinx as a **Hopf oscillator**: the tone is born when the air-sac **PRESS**ure crosses a threshold and
  grows with it (its level ~ the square root of how far past: below the threshold it is silent, whatever the
  envelope), so every syllable has a real onset; the pitch rises a little with the level. **CHAOS**: noise in the
  pressure and the pitch, and a second harmonic.
- **WING**: a band-limited pulse at the wingbeat with a slow random **DRIFT** of pitch; **BUZZ** narrows it (brighter);
  each syllable is a fly-by: louder and brighter as it comes, its pitch shifted by **DOPPLR** (higher coming, lower
  going). HOLD: a steady hover.
- **GLOTT**: a glottal pulse into two formants. **MOUTH** closed (oo) to open (aa), and darker to brighter; **ROUGH**
  in two halves of what mammals do: period doubling (a subharmonic, F0/2) up to 50 %, then a logistic map jittering each
  cycle (deterministic chaos: a roar); **AIR**: breath noise. A purr: play low (C1–C2), ROUGH halves the pitch.

**Presets** (24): CMOS BIRD, CMOS TRILL, CMOS MONKEY, CMOS BUG · CRICKET, TREE CRICKT, KATYDID, CICADA, TREEFROG, TOAD,
BULLFROG · CANARY, CARDINAL, DOVE, OWL, PEEPER, BAT · MOSQUITO, BEE, FLY · WOLF, DOG BARK, ROAR, PURR. They are set to
sound right around C5 (the frogs' and mammals' an octave or two lower: BULLFROG and OWL C4, WOLF G4, ROAR C3, PURR C2).

ANIMAL came last in the PRESETS list in 0.8.3, after DRUM, so every sound before it kept its place and its PROGRAM CHANGE
number; since 0.9.9 the list (and PROGRAM CHANGE) has it third (DRUM-X, VOICE, ANIMAL …). It costs no RAM (each voice's state fits the voice), about 7.6 KB of flash, and at most ~85 % of the heaviest
engine's CPU with eight voices (GLOTT; CMOS, PULSE, WHSTL and WING about half): all eight voices on every mode.

### SHRUB (0.9.10)

A wavetable synth. Its eight edit slots, in order: **WT**, **WARP**, **DETUNE**, **SUB**, **CUT**, **RES**, **DRIVE**,
**FILT**. FILT picks one of three filter types. **WT** (the wavetable position) and **CUT** (the filter's cutoff) are
the usual targets of the modulation matrix (MOD): an LFO on WT moves the sound through the wavetable, an LFO on CUT
sweeps the filter.

<!-- TODO(shrub): the slot meanings beyond their names and the three FILT types' names come from eng_shrub.c; fill in once it lands. -->

### VOICE: words and lyrics (0.8.5)

VOICE sings: syllables, words and whole lyrics, in **Spanish**, **English** and **Japanese**. Turn it on with
EDIT > **WORDS** > **LANG** (OFF, ES, EN, JA), or pick a word preset: **CANTOR ES**, **CANTORA ES**, **SINGER EN**,
**UTA JA**, **WORD CHOIR**, **SCAT EN**, **ROBOT EN**, **WHISPER JA**. LANG OFF is the VOICE you know (its vowels),
note for note as before.

**How it sings.** Every note sings the next syllable of the lyric. Hold the note and the vowel holds; a TIE holds it
on; at the release the syllable's last consonants sound (the *s* of "es", the *n* of "sun"). Legato notes glide from
syllable to syllable, and a syllable whose end was not sung yet gets it quickly before the next one. With several
voices every voice sings the same syllable: a choir. The lyric loops when it ends.

**How it sounds (0.9.8).** Like a singer: the hiss of *s*, *sh*, *f* comes in and goes softly (band-passed, ~6 kHz for
*s*, under the vowel's level), the formants glide from sound to sound without corners, a note's vowel carries into the
next syllable, held notes take a gentle vibrato after about 0.3 s (each voice of a choir its own), and the voice is
never perfectly still (a little jitter and shimmer). A glide between notes eases out of the first and into the second
(an S-curve, as long as GLIDE says); singing words, legato notes glide a little (~60 ms) even with GLIDE at 0.

**EDIT > WORDS** (VOICE tracks only): **LANG**, **LYRIC**, **POS**; **OCT+** (TYPE) opens the lyric's typing screen.

| LYRIC | sings |
| --- | --- |
| **TEXT** | your lyric (empty: W1) |
| **W1–W16** | the dice: phrases of real words from the language's bank (theme: THEME), each as long as the pattern's notes; a seed each, so W3 is always the same lyric |
| **S1–S8** | scat: pseudo-words of the language's syllables (*la, ta, ri* · *doo, bop, yeah* · *ra, shi, n*) |

With LANG on the EDIT pages read **POS THEME ARTIC SHIFT** | **BUZZ BREATH Q LOOP** (the screen relabels them):

| Knob | Does |
| --- | --- |
| **POS** | AUTO: each note the next syllable. 1–127: the next note sings that syllable of the lyric, whenever POS changes: lock it on a step to start a line there, or automate it |
| **THEME** | the dice's words (W): LOVE, NIGHT, SEA, SKY, ANY, ROAD, DREAM, FIRE; one detent a theme (0.9.17; was 16). It never touches the sound: a held note goes on as it was and the next words come from the new theme. Between the detents (automation, a mod, an older project) the top half of a theme's values blends in the next theme's words, a few to nearly all |
| **ARTIC** | the consonants: 100 % crisp (full closures, bursts, hiss) .. 0 % slurred (a mumble) |
| **SHIFT** | every formant up or down (−: a bigger voice, +: smaller, female, a child) |
| **BUZZ**, **BREATH**, **Q** | as for the vowels: brightness, air (127: a whisper), how sharp the formants are. (0.9.8) BUZZ's top (~112–127) also steadies the voice: no vibrato, no jitter, a machine (ROBOT EN) |
| **LOOP** | FREE: on through the lyric. 1, 2, 4, 8: back to its start every so many loops of the pattern (a chorus) |

**Writing a lyric.** On the device: EDIT > WORDS, **OCT+**. It is the NAME screen with room for 63 characters (the
field scrolls with the cursor): the keys type phone-style, **KNOB 3** throws the dice (a phrase of the generator in the
track's language and theme, to edit as you like), **OCT+** sets it (also while playing), **OCT−** cancels. From the
computer: the web editor's Sound tab shows a **Lyric** box on a VOICE track (LANG, LYRIC and the text; it turns
accents and ñ into what the device reads). Each track has its own lyric, saved with the project section.

**Text rules.**
- **Spanish** is read by rules, the way it is written: seseo (*z* and *ce ci* as *s*), *ll* and *y* as *y*, *rr* and
  an initial *r* trilled, *j* and *ge gi* as the jota, *ch*, *qu*, *gu*; diphthongs as one syllable (*bien*, *cuidado*).
  No accents needed; write **ñ as NY** (NINYO).
- **Japanese** in romaji (Hepburn or kunrei), **one note a mora**: *sakura* 3, *konnichiwa* 5 (ko-n-ni-chi-wa),
  *arigatou* 5 (a long *ou* sung o-o); a doubled consonant (*kitto*) is a short stop, not a note.
- **English** comes from a dictionary of ~2100 common words (from the CMU Pronouncing Dictionary) with their
  syllables; a word it lacks is read by rough letter rules, which work less well.
- A **-** on its own holds the vowel one more note (a melisma: *A - MOR* is a, a, mor). **_** joins two words into one
  (*MI_AMOR*: mia-mor, the Spanish *sinalefa*). Punctuation and digits only separate words.

**The screen.** A singing VOICE's EDIT pages show the language and the source, the syllables sung, the word in large,
its syllable's phonemes (the one sounding lit), your text from there on, and a vowel chart whose dot moves with the
formants (F1 down, F2 across: where the tongue is), with the voicing and noise levels.

**What to expect.** This is formant synthesis, like a vintage speech chip that sings: vowels, *s sh f*, *t k p*, *ch*,
*m n l* and the glides come through in context; *b d g*, *v z* and the English *r* are approximations, and
intelligibility drops at very short notes (a syllable's consonants take 40–100 ms before its vowel). Spanish and
Japanese read best (their spelling is regular); English depends on the dictionary.

**Cost.** No state per voice (the articulation is the track's); ~1 KB of RAM for the six lyrics and singers; ~42 KB
of flash (the English dictionary is most of it); about +9 % CPU over VOICE's vowels, so four voices singing still cost
well under ANALOG's eight.

### The engines' creatures (GARDEN)

Each engine has its creature on its EDIT pages (with SCOPE OFF the whole panel, centred) and on HOME › BEING, drawn in
2 px blocks in the garden's colours. They move on the music's clock (slow in silence, faster and pulsing with the
level and the beat):

| Engine | Creature | What moves |
| --- | --- | --- |
| DRUM-X | a drum kit: hi-hat, crash on its stand, two toms, snare, a kick with its logo | each piece jumps on its lane's hit, the hi-hat closes |
| VOICE | a singing flower with a face | its mouth opens with the level |
| ANIMAL | a beetle | its legs step, it hops on the beat |
| LOFI | an apple orbiting on a dotted path | its size and squash as it turns |
| ANALOG | an orange being with a face, a smile and blush | its dots orbit unevenly, it swells with the level |
| FM6 | a band of five small flowers | each bobs on its own phase |
| PHASE | a black hole: an orange ring, a black core, four tilted rings | each ring turns its own way; rings and core pulse |
| WHEEL | a barn wheel | it rolls along the ground, its spokes turning with the distance |
| NOISE | a spiral galaxy with a sun and planets | the arms turn, the planets orbit |
| SHRUB | pixel bushes with berries | they swell with the music only (still in silence) |

The other engines show the nearest family's creature. (Until stems-10 the engines drew thermal line pictures: VOICE's
vocal tract, ANIMAL's bird and mammal, LOFI's caterpillar and its DIP, DRUM-X's little beings and its colony; they are
retired with the old look.)

### FILTER (every engine)

EDIT > **FILTER**: **TYPE** LP, HP, BP or **COMB** (a comb tuned to the note you play; CUT moves it, RES is how much
it rings), **CUT**, **RES**, and KNOB 4: on ANALOG its own DRIVE, on every other engine the track's DIST. On ANALOG
the filter is per voice; on LOFI the filter page drives its per-voice filter; on the others it is a filter on the
track that follows LFO DEST / ENV DEST FILTER.

### VOICE (every engine)

EDIT > **VOICE**: **VOICE** POLY, MONO, LEG (legato), UNI (unison); **GLIDE**; **MODE** RATE or TIME (how glide is
measured); **PRIO** LAST, LOW, HIGH. The six tracks share a budget of 8 voices (**12 while the second core plays**,
0.9.12: MENU › SYSTEM › ABOUT says CORES 2); FM6 plays at most four per track, any track at most 8. (0.9.8) VOICE glides as a singer: an S-curve in
pitch over the same time (the other engines: a straight line, as before).

---

## 5. DRUM-X

DRUM-X is a synthesized kit of eight sounds, **BD SD CP CH OH TM RS CB**, generated in real time.

- (0.7.2) **Nine kits as the DRUM engine's presets**: DRUM-X (the factory kit), 909 CLUB, 808 BOOM, LOFI DUST,
  INDUSTRIAL, ACOUSTIC, DUB ROOTS, TECHNO RAW, BREAKS. Browsing one (PRESETS on HOME, the editor, a PROGRAM CHANGE)
  loads the whole kit into the section: the eight sounds' A and B sides, their LAYERs and SENDs, and the track's KIT
  knobs. Switching engines, INIT SOUND, a project load and GEN leave the section's kit as it is; DRUM-X brings the
  factory kit back. Edit any of them in EDIT > SOUND; the section saves the result.

- Every sound has two sides, **A** and **B**. **MORPH** (KIT, KNOB 1, also on HOME) moves the whole kit between them:
  the knob to play live.
- The pages go **the whole kit first, then one sound, then the volumes and the stereo** (0.6.58):
  - **KIT** (EDIT 1): MORPH, TUNE, TONE, DECAY. **KIT 2** (EDIT 2): NOISE, **FM ENV** (FM struck with each hit and gone
    in a few milliseconds: a beater, a click, a metal edge on the attack, the tail clean), **FOLD** (a wavefolder after
    the envelope: a bright attack, a clean tail), DRIVE.
  - **KIT 3**: **BP**, a band-pass on the metal sounds (the hats, the rim, the cowbell), OFF or 15 centres from about
    1.5 to 9 kHz; **FEEDBK**, every sound's oscillator fed back into itself (OFF, 1–15: warm, growl, grit, noise);
    **HANDS** (OFF, 1–15): more people clapping on the CLAP. Each extra hand strikes a few milliseconds after the first
    (3 to 45 ms), at its own moment and level, thrown again on every hit: never in time with the tempo. OFF is the clap
    as before; a few is a small group, 15 a crowd.
  - **X-MOD**: **RATE** (OFF, 4BAR … 1/32, in time), **DEPTH** (how far MORPH swings), **SHAPE** (SINE, TRI, SAW, RAMP,
    S&H), **RAND** (each hit gets its own MORPH and a little tune).
  - **FILTER**: the kit's.
  - **SOUND 1–4**, one sound each: KNOB 1 is always **SOUND** (which one; striking a key also picks it).
    - **SOUND**: SIDE (A / B), PITCH, PMOD (how far the pitch falls).
    - **SOUND 2**: DECAY, NOISE, COLOR, of the side picked on SOUND (KNOB 1 shows it: "SD B").
    - **SOUND 3**: **MODE** (wave SIN / FM / MTL / BEL, noise filter LP / BP / HP, "+" for snap), **PM.TYPE** (how the
      pitch moves: DECAY, LONG, NOISE, SINE), **DRIVE** (ON / OFF).
    - **SOUND 4**: **LAYER** (OFF, 1–15): a second oscillator at an out-of-tune interval, frequency modulated by the
      sound, more as it goes up: from a metal edge to a clang with no note, so the sound melts into the rhythm and stays
      out of the harmony (the factory kit has it on the snare, the hats and the cowbell); **FEEDBK** (OFF, 1–15): that
      sound's own (added to KIT 3's); **SEND**: that hit alone into one effect, OFF then CHO, DLY, REV with 16 amounts
      each, on one knob ("REV 75%"), on top of the track's own sends. The dice (EDIT + SELECT on a DRUM track, DICE ALL) set
      LAYER and FEEDBK boldly. Each sound with a LAYER costs a little CPU (about as FEEDBK).
  - **LEVELS / LEVELS 2**: the level of each sound (KICK, SNARE, CLAP, HATCL, HATOP, TOM, RIM, BELL).
  - **STEREO**: **IMAGE** (automatable) sweeps the kit through its placings in the stereo field. 0 (MONO): every sound
    in the middle; the first turns open it; from there each step is another placing (each sound moves on its own
    curve), so turn it until the image sits right. The kick stays near the middle. DRV, DIST and the FILTER work on the
    kit's sum, so with them the image narrows (the page draws the sounds grey).
  - KIT 3, X-MOD, LAYER, FEEDBK and SEND are saved with the section's kit.
- **Mutes**: GLO held + black keys 7–10 mute the groups KICK, SNARE, HAT, PERC; EDIT held + black keys 1–8 mute each
  sound.
- **EDIT held + SELECT** rolls a new kit around the factory one. **EDIT held + F3 (INIT)** brings the factory kit
  back.

### The kit in every octave (0.8.5)

The keys play the kit on the same keys at **every OCT** (C the kick, D the snare, F# the closed hat …); OCT now
changes **how** they play, not which sound. The screen says it when you press OCT on a DRUM track:

| OCT | What a key does | Recorded (REC) as the hit and its sound's mode |
|---|---|---|
| **+3** | **RATCHET x2**: the hit twice a step while held | **x4** (0.9.18: a sound's x4 spills two hits a step over two steps, as heard; was x2) |
| **+2** | **REPEAT**: the hit once a step while held, a roll at the step rate | a hit on every step held (**ON**) |
| **+1** | **REPEAT 1/2**: the hit every other step while held (0.9.17: every OCT half as fast again; 0.9.13 had already halved them) | a hit on every other step held (**ON**) |
| **0** | the kit as it always was | none (**ON**) |
| **−1** | **SHORT**: half the decay | **SHORT** |
| **−2** | **GHOST**: short, softer (~45 %), darker: a ghost note | **GHOST** |
| **−3** | **FLAM**: a soft grace hit, then the hit ~20 ms later (whatever the tempo) | **FLAM** |

- The ratchets are **in time**: playing, the hits fall on the step's RATCH grid (each step's start, then every step/n,
  swing included); stopped, every step/n of the track's DIV from the press. Letting go stops them.
- **Each sound has its own mode on each step** (0.8.5): a hi-hat roll recorded onto a step that has a kick ratchets
  the hi-hat only, a ghost snare leaves the kick as it is. The mode applies to that sound's hit and to its notes on the
  step (a low tom on TM); the last hit you record on a sound sets it.
- **Live recording keeps what you played**: the step you hit gets the hit and its sound's mode; hold a ratchet over
  several steps and every step it is held into is written too (and plays from the sequencer, so the next pass sounds
  exactly as you heard it). Let go before the middle of the last one and that step is put back, as with a held synth
  note.
- **See and edit it** on SEQ > STEP (the DRUM grid): KNOB 2 picks the sound, **KNOB 3 HIT** goes **--, ON, x2, x4, x8,
  SHORT, GHOST, FLAM** for it on the cursor step (left back to -- takes the hit and its mode away). A sound's x2 x4
  x8 spill over the steps after it as RATCH does (0.9.18: x2 a hit a step, x4 two, x8 four). The step's own RATCH
  (SEQ > COMP) repeats its other sounds: a sound with its own x2 / x4 / x8 or recorded ONCE is left out of it.
- **MIDI** (0.8.5): notes 35–81 still play the GM drum map exactly as before. The notes around it, which used to play
  their octave of 36–47 plain, are now these octaves, by pitch class: **23–34 SHORT, 11–22 GHOST, 0–10 FLAM, 82–93
  REPEAT 1/2, 94–105 REPEAT, 106–127 x2** (0.9.17; 84 the kick every other step, 96 the kick once a step, 108 the kick
  x2, 24 the kick SHORT, 114 the closed hat x2). The keys send those notes to
  MIDI OUT. **A song or MIDI setup that sent drum notes below 35 or above 81** (a kick on 24, say) will now hear these
  variations: move them into 35–81 (36–47 for the kit's own) to keep the old sound. Notes stored in the steps play as
  before, whatever their number.
- With the ARP on, the keys feed the arp plain (no ratchet, SHORT, GHOST or FLAM). The DRUM grid's lane keys are always the centre
  octave.

One kit per section, shared by every track on DRUM: while another track plays DRUM-X, browsing kit presets on a different track keeps that kit ("KIT OF T4 KEPT", 0.8.1); load kits on the drum track itself. The kit is saved with each section of a project (not with user presets — see [12](#12-saving-and-loading)).

---

## 6. The sequencer

Each track has its own **64-step** pattern with its own length (LEN), division (DIV), swing and gate, so patterns
of different lengths turn against each other.

### Finding your way: the SEQ chain

(0.9.2) The sequencer's eleven pages (0.9.18: the BAR page is gone) are one chain in five groups, as every section: **SELECT** walks them a page a
detent (no wrap), a **SEQ** tap jumps to the next group's first page (round), and the **tabs at the top** are the
groups, **PATT · TRACK · BAR · STEP · AUTO**, the one you are on lit with a dot for each of its pages; the footer says
**SEQ · BAR · STEP  6/11**. Until 0.9.1 SELECT zoomed three levels (PATTERN ↔ BAR ↔ STEP, the LENTE of 0.8.3) and the
tap went round each level's sets: the pages and their pictures are the same, only the way between them is new. The
pattern's bars (up to four, at LEN 64) are drawn as small cells: the bar you edit tinted violet with its number violet,
the number of the bar playing orange, a bar past LEN dark.

| Group | Pages | What the screen shows |
| --- | --- | --- |
| **PATTERN** | PATTERN · STRETCH · PLAYHEADS | (0.9.17) The roll, as every SEQ page. **PATTERN** draws the notes as long as **GATE** plays them and the odd steps late by **SWING**; steps past **LEN** are dark (in the strip, the bars past it). **PLAYHEADS** draws its extra heads over the roll as lines while they play, each as hot as its number (the head you edit 3 px wide). |
| **TRACK** | TRACK · MUTATE | **TRACK** outlines where **PITCH** moves the notes (in grey, over the notes as written). **MUTATE** shows a chip at the roll's foot: the bars to the next mutation (**NEXT n BARS**), **FROZEN**, or **MUTATE OFF**. |
| **BAR** | STEP · NOTE | STEP's KNOB 1 walks **all 64 steps**, crossing into the next bar by itself (0.9.18: a fast turn several at a time, KNOB SPEEDUP; the BAR page with its bar knob is gone, LEN is PATTERN's); NOTE's note in focus orange. |
| **STEP** | COMP · COMP 2 | (0.9.18) The step the component goes on framed violet with a tab at its foot (KNOB 1, at once); an orange bar over each step with the same component, a thin line over a step with another; a JUMP's step framed orange. |
| **AUTO** | AUTOMATION · AUTO LIST | A mark under each step with a record (a lock orange, automation white); on AUTO LIST the record picked has its step framed violet, the cards giving its row, step, parameter and value. |

**One picture for the whole SEQ** (0.9.17, the owner's "all the sequencer pages the same piano roll, each page doing
something different on top of it"): every page shows a strip of the pattern's bars, small, on top (the one you edit
tinted) and under it the bar's **piano roll** across the whole width: 30 semitones, the scale's rows tinted, the cursor
a frame round its column, the playhead an orange line, a step with a component a small mark on top, a parameter lock
a mark at the foot, a key held on the track a mark at the left of its row (a DRUM track: the **grid**, with its
keys). The page's own marks go over it (the table). Until 0.9.16 PATTERN's group drew the bars as big thumbnails with a
"PLAYHEADS" roll under them, COMP the step large with its chips, PLAYHEADS three lanes, MUTATE and AUTO LIST panels of
their own.

The roll has no keyboard strip nor the C rows' names since 0.8.5 (the mock-up's): the PITCH card says the note in
focus.

- **SEQ from another page** comes back to the page you left; at power-on STEP, the piano roll.
- **PRESETS** on a SEQ page loads a **pattern**: it opens the PHRASES loader with the next pattern picked (KNOB 1
  picks too), **OCT+** loads it into the track, **OCT−** goes back to the SEQ page you came from.
- **SEQ held** opens SEQ TOOLS on every page (**G5 CLR STEP**, new: clears the cursor step), **SEQ + SELECT** dices
  the sequence, **SEQ + REC** clears everything, a held step + a knob is a parameter lock (on STEP), step recording
  works on STEP and NOTE.
- CHANCE (a step's chance) is reached from code (the dice, the editor); it is drawn as the STEP group.

### Writing steps

- **SEQ > STEP** on a melodic track is a **piano roll**. KNOB 1 moves the step cursor, KNOB 2 transposes the step,
  KNOB 3 is **TIME**, KNOB 4 sets accent / slide. A step holds up to four notes.
- **A note's length** (0.8.4): KNOB 3 (TIME) is one range, REST ← TIE ← NOTE → 2 ST → … → 16 ST. On a note,
  turning right **lengthens** it: the steps after it become TIEs, one a detent (the card says **3 ST**, the roll draws
  the longer bar); a chord keeps all its notes. Turning left **shortens** it (its last TIEs become RESTs) down to
  NOTE, then the step itself becomes a TIE, then a REST, as before (a fast turn stops at NOTE first). It lengthens over
  RESTs and empty steps only: the **next note stops it** (the screen says NEXT NOTE; turn that step to TIE or clear it
  with SEQ + G5 to go on). At most 16 steps or LEN; near the end of the pattern it goes round to step 1, as the sequencer
  plays it. So any note, recorded or written, can be made longer or shorter afterwards with no REC. (On a DRUM track
  nothing changes: hits have no length.)
- **Step recording**: on STEP, while **stopped**, press **REC** (the screen says KEYS WRITE STEPS). Play a note or a
  chord: it goes into the cursor step, and when you let go the cursor moves on. Not armed, the keys just play.
  - **Length**: while the keys are held, turn **KNOB 1**: the note or chord lasts that many steps (1–16, ties after it).
    It is written when you let go, and the cursor jumps past it.
  - **Arpeggio**: while the keys are held, press **ARP**: the chord is written as an arpeggio into the roll. KNOB 1 is
    now its resolution (1/16, 1/8, 1/8., 1/4: a note every 1–4 steps), KNOB 2 the order (UP, DOWN, UP-DN, RND), KNOB 3
    the octaves (1–3). The screen shows the settings; letting go writes it, the cursor after it. KNOB 4 (and 2–3
    without ARP) still makes parameter locks on the step.
- **SEQ held + G5** (CLR STEP, 0.9.2; until 0.9.1 an EDIT tap) clears the cursor step and its locks and moves the
  cursor on; on NOTE with a note in focus it deletes that note alone. Turning TIME (KNOB 3) down to **REST** clears the
  step's notes too.
- On a **DRUM** track, STEP is a **grid**: white keys are the 16 steps of the page, black keys 1–8 pick the lane
  (and play it), black key 9 held is ACC (white keys then toggle accents), black keys 10 / 11 page down / up when
  LEN is over 16. Knobs: STEP, LANE, HIT, ACC.

### Live recording

Press **REC** anywhere (not on STEP while stopped): the track is armed and the transport starts. What you play is
recorded into the pattern, quantized to its steps, overdubbing. Knob moves on the selected track are recorded as
**automation**. MENU › AUDIO › CLICK has the **CLICK** (OFF / REC / ON), **CLICK LEVEL** and **COUNT-IN** (OFF, 1 BAR, 2 BARS).

### Pattern pages

- **COMP** (step components, OP-Z style): up to two per step (**COMP** the first, **COMP 2** the second, the same
  way; 0.6.82). KNOB 1 picks the step, KNOB 2 its **TYPE**, KNOB 3 its value, KNOB 4 **EVERY** (0.6.92; on COMP a
  RATCH's or an ARP's RATE sits there instead). The TYPEs by family (0.9.18): **OFF, CHANCE, RATCH, ARP, SEQ, WALK,
  PULSE, OCT, RAND, BEND, TONAL, JUMP, REV, TRIG, FILL, KILL** (COMP 2 has no CHANCE).
  **Where it goes** (0.9.18): the roll (a kit: the grid) frames the step KNOB 1 picks in violet, its whole column with
  a tab at its foot, following the knob at once; every step with the same component as the one you edit has an orange
  bar on top, a step with another component a thin line; a JUMP's TO STEP is framed orange as KNOB 3 turns.
  - **CHANCE** (0–100 %): the step's own chance (on COMP only).
  - **RATCH** (0.9.18: RATCH and ECHO in one): the step plays **x1–x8** hits. KNOB 3 the count and its shape:
    **FLAT** (every hit at the step's level), **FADE** (each softer: the old ECHO), **RISE** (each louder). KNOB 4 its
    **RATE**: **FIT** spreads the hits over **four steps** on a synth (two on a drum kit: x2 there is a hit a step), a
    note value sets how far apart they are (the card says it for the track's DIV), and a **pitch step** for the hits
    after the first: **+1 DEG** / **−1 DEG** along the scale, **+OCT** every other hit an octave up.
    The hits after the first **spill over the steps after it** (until 0.9.17 they fell inside the step): a hit that
    lands on a step with the same note of its own (a kit: the same sound) is left out, another note plays beside it on
    POLY; on MONO / LEGATO the next note cuts what is left; a new ratcheted step replaces it. The chance is rolled once
    for all of them, STOP ends them. Not repeated with the step: a sound with its own x2 / x4 / x8 on the grid (the
    sound's mode wins, never a ratchet of a ratchet), a hit recorded live into the step (ONCE, 0.9.17) and a note
    recorded live into a ratcheted POLY step (0.9.18): each plays once, as it was heard. SEQ > CHANCE's x1–x4 is RATCH
    FLAT; a project's ECHO is RATCH FADE (EVEN: FLAT). The live RATCH (GLO › LIVE) stays inside each step.
  - **ARP** (0.9.18): the step's notes (its chord; a kit: its sounds) **one at a time at RATE** (KNOB 4: **1/32, 1/16,
    1/8, 1/4**, then the same again with the loops it plays on, 1:2 …; the other loops play the chord as written), in
    KNOB 3's **ORDER** (UP, DOWN, UP-DN, RND) over 1–4 octaves (a single note: itself and that many octaves up), through
    the step and its TIEs and at least once round, spilling over the steps after it as RATCH's hits do; on MONO the next
    note cuts it. (Until 0.9.17 ARP NOTES played its notes inside the step at x1–x8 a step.) **ARP SCALE is gone**: a
    step that had it loads as a WALK of its lowest note (the same degrees, a move a lap).
  - **SEQ** walks the pattern's melody **one note a lap** (0.9.17): each time the pattern comes round to the step it
    plays the next note of the melody, in its ORDER and range, held over the TIEs after it like any note; its KNOB 4 is
    **LAP**, the note this pass plays (RND: a new one each lap), and PLAY from a stop starts again from the first note,
    as the other components' loops do.
  - **TRIG**: the step plays only on some loops of the pattern: 1:2, 2:2, 1:3, 2:3, 3:3, 1:4 … 4:4, 1:8, FIRST, !1ST
    (every loop but the first).
  - **RAND**: the note moves up or down the scale at random, up to RANGE degrees (1–7).
  - **OCT** (0.9.18, CLIMB's place): the step's notes move **an octave every so many loops** (KNOB 4: 1 … 10, 12, 16),
    over 1–4 octaves, KNOB 3 **UP / DOWN / UP-DN / RND**, then round again (WALK in octaves). The old OCT loads as what
    it played: +1 / −1 every time as the notes moved an octave, every other loop / ALT / RND as OCT with that motion.
  - **TONAL**: from this step to the end of the loop every note moves SHIFT degrees (−7 to +7) along the scale.
  - **BEND**: the note glides BEND semitones (−12 to +12) over the step.
  - **PULSE**: the note plays again on the empty steps after it, EVERY 1–4 steps, until the next note.
  - **JUMP**: after this step the pattern goes on from TO STEP.
  - **REV**: from this step the pattern plays backwards to step 1, then forwards again (ping-pong).
    JUMP and REV turn the pattern on the loops **EVERY** says (below): EVERY 1:2 jumps every other loop, FIRST only
    the first time (an intro), !FIRST every loop but the first (0.6.92; it was TIMES).
  - **FILL**: the step plays only while **FX** is held.
  - **KILL**: the step first cuts everything the track is sounding (long releases, held notes), then plays.
  - **TOUCH is gone** (0.9.18): a step that had it loads with its SHORT / GHOST / FLAM as the mode of each of its
    sounds that had none, the same sound (see [5](#5-drum-x)).
  - **WALK** (0.6.98, after the OP-Z's and OP-XY's evolving components): the step's notes move one degree of the scale
    **every so many loops** of the pattern (KNOB 4: 1 … 10, 12, 16 loops a move), over a **range** (3, 5, 7 or 12
    degrees), then round again. KNOB 3 the **MODE** and the range in one: **UP** (0, 1, 2 … range, 0 …), **DOWN**,
    **UP-DN** (there and back), **RND** (a place of the range each move, the same every time round). They stack with
    each other and with the rest: a WALK on COMP and an OCT on COMP 2 add up, a TONAL on top still applies, an ARP
    walks with its chord. Four notes with a WALK UP-DN 5 every 2 loops and an OCT UP 1 every 3 come back to themselves
    after 36 loops, different every time round. A drum kit ignores WALK.
  - **Two on one step**: both play (a RATCH with a TONAL: the hits moved; a RAND with an OCT: both moves). One that
    keeps the note silent wins: a **TRIG** or **FILL** in either place decides whether the step plays at all, before
    anything else. Of two that do the same job, the first wins (an ARP over a RATCH, two ARPs: COMP's).
  - **EVERY** (0.6.92, KNOB 4 on COMP 2, and on COMP for every type but ARP and RATCH): the loops of the pattern
    the step's components play on — **ALWAYS**; **1:2** (the 1st, 3rd, … loop), **2:2** (the 2nd, 4th, …); **1:3**,
    **2:3**, **3:3**; **1:4** … **4:4**; **1:8**; **FIRST** (the first loop only); **!FIRST** (every loop but the
    first); **SKIP** at the end of the knob (kept, but not played: an on/off switch). One setting a step, for both of
    its components, set from COMP 2's KNOB 4 whatever its TYPE (or COMP's, for the types without a knob of their own
    there; an ARP's KNOB 4 past 1/4). The step's own note plays on every loop (a **TRIG** component decides that). It
    took WHEN's place (that page is gone), with its A / B read as one.
  Projects keep them. **On a drum kit** (0.6.83) they work too: OCT (an octave a lap), RAND, TONAL tune the hits (RAND
  and TONAL in the track's scale), BEND bends each hit over the step, RATCH repeats the step's sounds, ARP plays them
  one after another at its RATE, SEQ the pattern's sounds one a lap.
- **A long chord slide** (0.6.83): the glide into a chord lasts as long as that chord, its TIEs included; GLIDE sets
  how much of it (127: all of it). A chord held over TIEs on slow steps (e.g. DIV 1/4 at 120 BPM: ten steps, 5 s)
  slides that slowly and smoothly; new notes fade in as they glide.
- **PATTERN** (the PATTERN group's first page): LEN (1–64), DIV, SWING, GATE — the pattern's length, its step's note value, its
  swing and the length of every note (a step's own length: STEP's TIE, or MICRO for its start).
- **Moving a note or a chord** that came in early or late: **NOTE**, KNOB 1 FOCUS on the step, KNOB 3 **MOVE** moves
  it sideways a step at a time (FOCUS ALL: the whole chord with its TIEs; one note focused: that note alone).
- **STRETCH** (0.6.85; the PATTERN group, after PATTERN): reshapes the whole sequence of the selected track **as the knob turns** (0.6.92;
  it used to wait for OCT+). Every turn works from the sequence as it was before the first one (its base), so x2 then
  x4 is x4, not x8, and back to the middle is back to where you started; the message says the LEN before and after
  (LEN 4 > 16); **UNDO** (GLO + G4) brings it back; editing the result makes it the new base. The other knob takes the
  result as its base (0.6.96): REPEAT x2 then STRETCH x2 stretches the repeats.
  - **KNOB 1 STRETCH**: **x2 … x8** spreads the steps that many times apart (LEN 4 x4: 16), a held note held all the
    way, short ones kept short; past 64 steps the end is cut. **/2 … /8** squeezes it: of each group of steps the first
    one that plays stays (LEN 32 /2: 16). **x1** in the middle: as it was. The automation and locks move with their
    steps.
  - **KNOB 2 REPEAT**: copies steps 1..LEN after themselves, **x2 … x8** times or **TO 64** (as many whole copies as
    fit), automation and locks too; **OFF** at the left: as it was. Four notes become a 64-step bed to record more notes
    over (live REC overdubs).
  - **KNOB 3 NUDGE**: the whole sequence one step right / left, round inside LEN (as TOOLS A3 / B3).
  - **KNOB 4 ROTATE** (−32 … +32; it was SHIFT's OFFSET): the sequence read from another step, without moving the
    steps themselves — a live, automatable rotation (also on the SCL layer's KNOB 3).
- **MICRO** (0.6.92, NOTE's KNOB 4; it was the TIMING page's OFFSET): the focused step's own place off the grid, in
  **64ths of a step** (0.9.1; it was sixteenths), from −32/64 (half a step early) to +31/64 (late): a 64th of a 1/16
  step is about 2 ms at 120 BPM. A 64th a detent (turned faster, more: knob acceleration); the card shows **+5/64**;
  0 puts it back on the grid. Early steps really sound early: the step before ends that much sooner and the grid
  is kept. The piano roll draws the notes where they play. A chord shares its step's offset; moving, stretching or
  repeating steps keeps it; saved with the project.
  **REC Q** (TRACK's KNOB 4) is the recording quantize that writes it: 100 % (the default) live recording snaps to the
  grid; lower, the step keeps that much of where you played it (0 %: all of it, to the nearest 64th), early or late;
  50 % half way to the grid.
  Projects from before 0.9.1 play exactly as they did (their sixteenths are 4/64 each). A project saved with 64ths
  still loads on older firmware, which plays each step at the sixteenth at or before its 64ths (up to 3/64 sooner).
- **TRACK** (0.6.92; it was SHIFT + HUMAN): the whole track's settings. **PITCH** (±24 semitones; not on drums).
  **CHANCE**: the whole track's chance, 100 % plays every step as written, lower thins it out (each step's own chance,
  from the dice or the editor, scaled). **HUMAN** (one amount, 0–100 %): the track plays like a person, each note a
  little different every time round, the steps staying as written — its velocity up or down at random (up to ±50 at
  100 %), its start a little late (up to 40 ms, never more than a third of the step, never early; a chord's notes stay
  together, a STRUM keeps its spacing), its length longer or shorter (up to ±40 %), and its envelope's DECAY and
  RELEASE a little longer or shorter (up to ±20). 0 % plays exactly as written (the default). Kept when you change the
  sound; automatable. A project saved with the old VEL / TIME / GATE loads with the smallest of the three as its HUMAN.
  **REC Q**: see MICRO. The old **C.LEN** (the components as a lane of their own length) is gone: components live on
  their steps, EVERY says how often they play.
- **AUTOMATION**: PLAY turns the track's automation on or off, EVENT and LOCK count the records, CLEAR erases them
  (OCT+, with a dialog). The automation is what you record by turning a knob while REC is on and the track plays (a
  value that moves along the pattern), and the parameter locks (a value a step, see below): this page shows how many
  of each the track holds and whether they play; the page's hint says so. AUTO LIST is where to see and edit each one.
- **AUTO LIST**: every automation event and lock as a list. KNOB 1 the row, 2 the step, 3 the parameter, 4 the value.
- **MUTATE**: the track changes by itself, in time with the clock, so a texture can evolve for minutes.
  **SOUND** and **PATTERN**: every OFF, 1, 2, 4, 8, 16 or 32 bars (the change lands on the bar's one). **AMOUNT**: how
  much (turning one on sets 50 %). The page (0.6.92) shows the selected track's next 32 bars as one strip, a mark on
  each bar a change lands on (orange: SOUND, violet: PATTERN), the playhead on it, and a line saying what each knob
  does. **MODE**: **ORBIT** varies around the anchor (the music as it was when MUTATE came
  on, when PLAY started, when you pressed **OCT+** here: KEEP, or when you last wrote or cleared steps), so it never
  loses itself, and a sequence you clear stays cleared; an empty pattern is never filled by MUTATE; **DRIFT** goes on from
  what it is now. SOUND changes the engine's values, the filter, its envelope, DEC, REL and the sends (never LEVEL, PAN
  or MUTE); PATTERN moves notes along the scale, flips accents, adds or takes a note off the beat (never on it); on
  DRUM it flips hits, never the kick. **OCT−** here (or **GLO + A4**, FREEZE) stops every mutation until pressed again.
  On HOME (0.9.7) the being a change lands on looks up, its number lights white and a ring in the accent opens round it.
  **Going back** (0.9.3): MUTATE keeps its anchor, so a track can return to it. Turn a track's SOUND and PATTERN both
  OFF after MUTATE has changed it and a NOTICE asks **BACK TO BEFORE MUTATE?**: **OCT+** (BACK) puts the track's sound
  and steps back as the anchor holds them, **OCT−** (KEEP) leaves it as it is (if nothing changed, nothing is asked).
  At any time **SEQ + D5** (**REVERT**, on every SEQ page) does the same for the selected track, and **GLO + B4**
  (**REVERT ALL**, next to FREEZE) for every track that plays. OCT+ and OCT− stay KEEP and FREEZE on this page, so
  going back is a layer key (rule 5). Only what MUTATE changes comes back: the engine's values, the filter, its
  envelope, DEC, REL, the sends and the steps (notes, accents, hits); LEVEL, PAN, MUTE, the pattern's settings (LEN,
  DIV …) and the other tracks stay (a track on another engine since: its steps only). The anchor is the point described
  above (MUTATE on, PLAY, KEEP, steps written), in DRIFT too: after a long DRIFT, REVERT goes all the way back to that
  point, not one change back; press KEEP to make the music now the point to return to. A revert is undone with
  **GLO + G4** (UNDO). After a project or section load, or NEW PROJECT, the anchor belongs to other music: REVERT says
  **NO ANCHOR** until MUTATE takes a new one.
- **PLAYHEADS** (the PATTERN group; it was FUGUE), after the Fugue Machine: three more **playheads** (heads 2, 3, 4) read the same steps
  as the track's sequencer. KNOB 1 **HEAD** picks one, KNOB 2 **RATE** (OFF, 1/4, 1/3, 1/2, 2/3, x1, 3/2, x2, x3, x4 of
  the track's), KNOB 3 **DIR** (FWD, REV, PING, RND), KNOB 4 **INTVL**, its interval in degrees of the scale (−14 to +14;
  a drum track keeps its lanes). A head plays each step's notes with the step's velocity, accent and chance and the
  track's GATE (no components, locks or ratchets: those are the sequencer's): canons, counterpoint, patterns phasing
  against themselves. The page draws the steps, the sequencer's playhead and each head where it is. Saved with the
  section; the heads start and stop with the transport.
- **AUTO LIST**: on **+ ADD LOCK**, OCT+ adds a lock; on a record, OCT+ switches it between LOCK and AUTO; **SEQ +
  G5** (DEL REC; until 0.9.1 EDIT) deletes it.

### Parameter locks

On **SEQ > STEP** (the BAR group), **hold a step** (a grid key, or the note being entered on the roll) and turn a knob: that step gets
its own value of the parameter that knob edits on the sound page you visited last (at power-on: HOME's four knobs).
The cards show those four while the step is held: a locked value bright, the sound's own DIM.

**Choosing the page while you hold (0.9.12).** Still holding the step, turn **SELECT**: the knobs move to the next of
the track's **EDIT pages**, the same chain SELECT walks on EDIT, without leaving STEP. The cards show that page's four
parameters and the footer says which page it is, **LOCK · KIT 2** with its place **2/7**. On a DRUM-X track the walk
is KIT (MRPH TUNE TONE DECY) → KIT 2 (NOIS FM FOLD DRV) → LEVELS → LEVELS 2 → FILTER → STEREO → VOICE; on a synth
EDIT 1 → EDIT 2 → FILTER → MORPH … → VOICE 2 (the pages that engine has). Pages whose values are not the track's
parameters are skipped: DRUM's SOUND 1–4, KIT 3 and X-MOD hold the kit's own values, and FOCUS and the FM6 operator
pages have no four track parameters on their knobs, so none of them can be locked. To lock DECAY and then FOLD on one drum step: hold the step, turn
SELECT to KIT, turn KNOB 4, turn SELECT one more to KIT 2, turn KNOB 3, let go. When you let go, the page stays the
locks' page (the next hold starts on it, and EDIT opens there). From a page outside the walk (ENV, LFO, FX, HOME's
four) the first detent goes to the walk's first page. With no step held, SELECT walks SEQ's pages as before. SELECT
had no use with a step held before 0.9.12: it left STEP.

**Locks on a DRUM-X track** belong to the step: every sound struck on that step plays with the locked values (KIT's are
the whole kit's). Black keys 1–8 still pick the lane: holding an empty step puts that lane's hit on it, and a step key
pressed on a hit and held for a lock keeps the hit. To give one sound its own value on a step, lock its level on LEVELS.

**Clearing (0.9.2 and later).** EDIT never clears a step any more. **SEQ held + G5** (CLR STEP) clears the cursor step,
its locks with it. **A step held + EDIT** clears only that step's locks; the notes or hits stay (the screen says
**LOCKS CLEARED**, or **NO LOCKS**). To change or delete one lock or one recorded automation event on a particular step,
use **SEQ › AUTO LIST**: each record is a row (KNOB 1 the row, 2 the step, 3 the parameter, 4 the value), OCT+ switches
it between LOCK and AUTO, and SEQ + G5 deletes it. **+ ADD LOCK** adds a lock on the cursor's step.

Automation events and locks share **64 records per section** (all tracks together); when they run out the screen says
**AUTOMATION FULL**.

### Scale

**ROOT, SCALE and QUANT are global**: one scale for the whole song. Whatever track you set them on (knob, SCL layer,
MIDI, a project load), every melodic track follows it at once; drum tracks have none. Each track keeps its own TRANS.

- SCALE: CHR, MAJ, MIN, DOR, MIX, PEN, MPEN, HARM, PHRY, LYD, LOC, MEL, BLUES, WHOLE, DIMHW, DIMWH.
- QUANT: OFF; SNAP (keys snap to the scale); WHITE (the white keys walk the scale, black keys are silent); SEQ (as SNAP,
  and the sequencer's notes snap too as they play).
- **SCL > ALL TR**, KNOB 4 (0.7.6; SCL 2 until then; it was called STRN) moves every sequence up or down the scale (±14 degrees), on every track at
  once. It is global and saved with the project: if your notes sound lower or higher than the piano roll shows, look
  here. When it is not 0 the piano roll says **ALL TR** and its value, and PLAY says it too. 0 = off.
- The RESO reverb and the SLICER's pitch steps follow the scale too.
- (0.8.3) **A section keeps its key**: ROOT and SCALE are stored with each section, so a song can change key from one
  section to the next (GEN's PARTS does it for pop genres: the last section a whole step up). When the song brings in a
  section, the keys, CHORD, ALL TR and the RESO reverb follow its key, also with a DRUM track selected (then the first
  melodic track's key counts; until 0.8.3 a DRUM track selected kept the key before).

### CHORD (0.6.81)

**SCL > CHORD** makes a key play a chord on the track (after the Omnichord and the Orchid):

- **CHORD**: OFF; in the scale (every chord in key, nothing to think): **TRIAD**, **7TH**, **ADD9**, **SUS2**,
  **SUS4**, built on the key's degree (a key outside the scale takes the scale note below; CHR: the major scale of
  ROOT); shapes on any key: **MAJ**, **MIN**, **7**, **MAJ7**, **MIN7**, **POWER**. In a mode of seven notes (MAJ MIN
  DOR MIX HARM PHRY LYD LOC MEL) the chord is the mode's own on that degree (every other note of the scale). (0.8.3) In
  the scales of five, six or eight notes (PEN, MPEN, BLUES, WHOLE, DIMHW, DIMWH) every other note made odd stacks (C
  minor pentatonic's "triad" on C was C F Bb), so each tone is now the interval the chord asks for where the scale has
  it, else the nearest one it offers: a major or minor third (none: the chord becomes a suspension), a perfect fifth
  (else diminished over a minor third, augmented over a major one), a minor or major seventh (else a sixth), the ninth.
  Every tone stays in the scale: a pentatonic key gives triads, sus4 / sus2 chords and sixths (C minor pentatonic: Cm,
  Eb, Fsus4, Eb/G, Bbsus4; 7TH on F: F7sus4), whole tone augmented chords, the diminished scales diminished and
  dominant sevenths.
- **VOICING**: CLOSE (root position); **LEAD** (each chord takes the inversion nearest the last one: voice leading, the
  Orchid's smoothness); OPEN (the second tone an octave up); +BASS (the root an octave down added).
- **STRUM**: 0 all at once; up from there, the notes come in one after the other, low to high (up to ~63 ms apart), as
  a harp.

Letting the key go ends every note of its chord (those still to strum never come). The chord records into the step,
live and in step recording alike (0.6.83: step recording took the key's note alone), and the ARP arpeggiates it. The
keys of every note sounding light up, the chord's whole shape too: a way to see and learn the chords. With **STRUM**
(0.6.84) a recorded chord goes whole into the one step where the key went down (its later notes no longer spill into
the next step), and the sequencer strums every chord step back the same way, low to high (within half the step's
gate). STRUM at 0: the steps' chords as before, all at once. A drum track, MONO, LEGATO and UNISON play the root alone. MIDI notes play as they come.

### ARP

ARP: MODE, RATE, OCT (1–4), GATE. ARP 2: SWING, PROB, HOLD, ORDER (NOTE / PLAY). Modes: OFF, UP, DN, UPDN, RND, ORD,
REPEAT, DNUP, UP+8, CONV, DIVG, PINKY, THUMB, WALK, CHORD, **TRNS** (the keys and incoming MIDI transpose the track's
sequence by their distance from C4, without playing notes). **Key latch**: with MODE OFF, **HOLD ON** keeps the notes
sounding after you let go; the next chord replaces them.

### Clearing

| To clear | Do |
| --- | --- |
| The selected track's steps and automation | **REC held + F3**, or SAVE > TOOLS > CLEAR SEQ, or SEQ TOOLS F3 |
| All six tracks' steps, automation and the punch-in lane | **SEQ held + REC**, then OCT+ |
| **Everything** (NEW PROJECT): every track its power-on sound (0.9.9: T1 DRUM-X, T2 VOICE SINGER EN, T3 ANIMAL CMOS BIRD, T4 LOFI BYTEBEAT, T5 ANALOG FAT BASS, T6 FM6 TINE EP, dry), no steps or automation, the mix, effects, tempo and SLICER at their defaults, the song empty, macros at 0, the factory DRUM-X kit. Saved projects, user presets, MENU, CLOCK, TUNE and MIDI stay | **SAVE > TOOLS > 5 NEW PROJECT** (until 0.9.17 also MENU > SYSTEM > INIT SYSTEM), then OCT+ (UNDO brings the music back) |
| Only one part's automation on the selected track | **REC held + FX / EDIT / ENV / LFO / SCL / ARP / GLO** |
| *(not a clear)* A heavy test song, to measure the CPU on the device and listen for clicks (every live track chords of four with two step components a step and long glides, the kit on every lane, every FX on, 160 BPM) | **SAVE > TOOLS > 6 CPU TEST**, OCT+, then PLAY (UNDO brings your music back) |
| The track's automation, keeping the steps | SEQ TOOLS **F4 CLR AUTO**, or SEQ > AUTOMATION > CLEAR |

A track clear says **T3 CLEARED** with **SEQ+REC: ALL TRACKS** under it as a reminder of the all-tracks combo.

---

## 7. ARRANGE: songs, sections and scenes

### The pieces

- A **project** is a song. There are **8 projects**.
- Each project has **4 sections, A–D**. A section is a full snapshot of the music: every track's sound, steps and
  automation, the DRUM-X kit and its mutes, the punch-in lane, the SLICER, the macros, plus the tempo and global effects.
- Each project has one **arrangement**: up to **16 rows**, each a section played for **1–64 bars**, optionally with a
  **scene**, and an **END** rule.

That's 32 sections in total (8 × 4).

### The ARRANGE page (SAVE > ARRANGE)

ARRANGE is the second page of SAVE's PROJECT group (0.8.5; 0.9.2 SECTIONS → ARRANGE with SELECT): on top the project's four sections large (stored: in their
colour, the one playing says **NOW**), then **ARRANGE**: the song's rows as chips (**A16**: the section and its bars, in
the section's colour; a scene: a white dot; the row you are on lit and framed), and under them the whole song as one
strip (its rows by their length, a white line where it plays) with its length and its END (**120 BARS · LOOP**).

| Knob | On a row | On + ADD | On END |
| --- | --- | --- | --- |
| KNOB 1 | choose the row | | |
| KNOB 2 | its section A–D; turned left past A: **DELETE SONG ROW?** | adds a row (same section as the row above, 1 bar) | **STOP**, **x2 … x8**, **LOOP** |
| OCT+ | adds a row after the last (its section and bars), and selects it (0.8.5) | | |
| KNOB 3 | its bars (1–64) | adds a row | |
| KNOB 4 | right: store a **scene** (SCENE STORED); left: clear it | | |

- **PLAY** on ARRANGE plays the song from the top; again: stop. (Until 0.8.4 OCT+ did too; now it adds a row.)
- When the song ends (or you stop it), the music you had before pressing PLAY comes back (SONG END).
- END: **STOP** plays the song once, **x2–x8** that many times, **LOOP** forever.
- If a row points to an empty section the song won't start: the screen says **PATTERN A EMPTY** (or B, C, D).
- While a song plays, step editing is blocked (**STOP TO EDIT**).
- To delete a row you can also use SAVE > TOOLS > DELETE ROW (the row ARRANGE has selected). To delete the whole
  arrangement: TOOLS > CLEAR SONG (**CLEAR SONG ORDER?**). Neither can be undone.
- The arrangement is saved automatically (about 2 seconds after a change, while stopped). See [12](#12-saving-and-loading).

### Scenes

A scene is stored per row and recalls, when that row starts: the track mutes (T1–T6), the DRUM-X group mutes, the
macros M1–M4 and the punch-in note effects being held. It stays in effect through the following rows that have none.
Scenes apply when a song plays, not in a quick chain. SONG REC also stores one per row as it begins.

### Playing sections live (SAVE layer)

Hold **SAVE**:

- **F3–B3**: play section A–D. While playing it comes in on the **next bar**; stopped, it loads at once.
- Tap **two or more** of F3–B3 in one hold: a **quick chain** — they play in turn, each for its longest pattern's
  length, round and round, starting on the next bar (stopped: at once).
- **C4–F4**: **store** the music now into A–D (works while playing; see [12](#12-saving-and-loading) for when it
  reaches flash).
- **OCT+**: store into the section playing (PLAY A SECTION FIRST if none). **OCT−**: recall it as stored.
- **D5**: **LOOP / SONG** — in SONG mode, PLAY from any page plays the arrangement.
- **E5**: **SONG REC** — the sections you play live (and their bars) are written into the arrangement's rows, with a
  scene each. It replaces the current rows.
- **G5**: jump to ARRANGE.
- **KNOB 1**: pick another project; it switches when you let go of SAVE (stopped only) and **loads its section A**.

---

## 8. Dice and UNDO

### GEN — the song generator (HOME + SELECT)

Hold **HOME** and turn **SELECT**, from any page, or on HOME walk the chain with **SELECT** (ECO · MACROS · BEING ·
ROUTES · **GEN 1 · GEN 2 · GEN 3**) or tap HOME to its next group (ECO → BEING → GEN). (0.9.2) The **GENERATOR** is
HOME's last group, three ordinary pages, no longer a screen of its own: **SELECT** turns the page, **KNOB 1–4** set the
four values shown (**PRESETS**: the GENRE, the generator's preset), **OCT+** makes the piece and **OCT−** goes back to
ECO (the piece stays; **UNDO**, GLO + G4, brings back what was there before); every section button works from there. The panel shows the five parts as rows (the ones the page's knobs weigh lit) with a preview of
their density and heat, the FORM's sections as coloured chips, the last piece and **OCT+ MAKE**. **PLAY** runs and stops
the transport without leaving the screen (0.6.97).

| Page | KNOB 1 | KNOB 2 | KNOB 3 | KNOB 4 |
|---|---|---|---|---|
| 1/3 | **GENRE**: **EXPLORE** (the screen always opens on it: no genre, every rule broken, below), RANDOM (half its throws are mutants of two genres, named A+B) or one of 43, in that order | **SEED** 0000–9999 | **ENERGY** 0–100 % | **FORM**: LOOP, PARTS, SONG |
| 2/3 | **DRUMS** | **BASS** | **CHORDS** | **ARP** — each OFF, 10–100 %, or **AUTO** (the genre's) |
| 3/3 | **LEAD** (as above) | **COMPLEX** 0–100 % | **SWING** AUTO or 0–100 % | **HUMAN** 0–100 % |

The piece is **deterministic**: the same settings make the same piece every time, so a seed you like can be dialled
back. **OCT+ with the settings unchanged throws a new seed** (another piece); turn a knob and press OCT+ and you get
the **same piece with that one thing changed** — every part draws its random numbers from its own stream, so BASS
turned leaves the drums and the lead as they were. The screen shows **LAST: HOUSE 0421** (the genre and the seed).

The genres: HOUSE, DEEP HOUSE, MICRO HOUSE, TECH HOUSE, ACID HOUSE, TECHNO, DETROIT, DUB TECHNO, HARD TECHNO,
HYPNOTIC, TRANCE, ELECTRO, EBM, ITALO, SYNTH POP, DARKWAVE, POST PUNK, TRIBAL PUNK, MINIMAL SYNTH, MAN MACHINE,
MOTORIK, INDIE, SHOEGAZE, POST ROCK, FUNK, DISCO, BOOM BAP, TRAP, UK GARAGE, DUBSTEP, BREAKS, DRUM & BASS, DUB,
STEPPERS, JAZZ, BOSSA, AFROBEAT, AMBIENT, NEO CLASSIC, IMPRESSION, SOUNDTRACK, IDM, BROKEN. Each one knows its tempo
range, its scales and chord progressions, its drum map and kit, its bass archetypes and register, how its chords, arp
and lead speak, which sound recipe each role takes, how human it plays and its space (reverb type, delay type and
time, how wet).

**EXPLORE** (0.8.6), the first choice and where the screen always opens (at power-on, after NEW PROJECT, every time
you open GEN), follows **no genre** and breaks their rules on purpose, so every seed is a very different piece
(**LAST: EXPLORE 0421**). **How far it goes is COMPLEX** (page 3, KNOB 2; 0.9.13): at **100** every rule below is
broken as in 0.8.6–0.9.12; at the default **50** the pieces are adventurous but musical; lower, plainer still (see
*COMPLEX on EXPLORE* below):
- **Tempo** anywhere from 56 to 200 BPM (COMPLEX 100; 80–165 at 50, 104–146 at 0); **any drum map** (the procedural IDM's too) and kit, its sounds pulled far
  from the kit's; any three bass archetypes against any kick rule; any chord play, voicing, arp and lead; a role may
  take another role's kind of sound (a plucked bass, a bell lead, an organ texture).
- **The key**: a mode of seven from the SCALE page, the exotic ones likeliest (phrygian, lydian, harmonic and melodic
  minor, **locrian**), the lead on a pentatonic, hexatonic or gapped set over it. **Harmony**: one to four chords on
  any degrees in any order (non-functional moves), any quality, now and then a borrowed, secondary or flat-side chord
  (the out-of-key notes), a family and colours thrown too; a PARTS song's last section may move anywhere (a semitone,
  a tritone ..), not just a whole step up.
- **Time**: a chord 8 to 48 steps, the loop 8 to 64 (odd metres: 3/4, 5/4, 7/8 ..); **each track its own LEN and
  DIV** (1/8, 1/32, 8T, 16T: polyrhythm), a track's own swing, drum hits off the grid.
- **Sound**: **any engine on any role**, ANIMAL and VOICE included (VOICE singing words in ES / EN / JA, a phrase or
  scat), FM6, ANIMAL and VOICE on two tracks at most; DRUM-X's octaves per sound on the steps (x2 x4 x8, SHORT, GHOST,
  FLAM); wide pans, glides, vibratos, distortion types.
- **Motion**: step components scattered (RAND, OCT, BEND, ECHO, PULSE, WALK, CLIMB, ARP, TONAL, JUMP, REV, some every
  other loop), **PLAYHEADS** on a track or two, **MUTATE** on a track or two; the delay's grain pitch and spray,
  ping-pong, any reverb.
- What it keeps: it always plays (two melodic parts at least, the genres' levels, the shared voice budget, the delay
  short of self-oscillation), the **SCALE page** and the **CHORD page** say the key and the chords it plays, and every
  note is in that key but the coloured chords' own tones and the approaches. The same seed is the same piece; the
  genres' pieces are exactly as before. After an EXPLORE piece a genre's piece sets DIV back to 1/16 and PLAYHEADS off.

**COMPLEX on EXPLORE** (0.9.13). Until 0.9.12 EXPLORE added 10–40 to COMPLEX and broke every rule on every seed,
whatever the knob said: most pieces came out as wild as it gets. Now the knob is how wild: each of EXPLORE's own
throws happens as often as COMPLEX says (at 50: half as often as at 100) — a track in a loop or a DIV of its own
(and on at most 1 + COMPLEX / 20 tracks: three at 50), odd chord lengths and loops, the exotic modes (below 100 the
commoner major, minor, dorian and mixolydian come up more), the borrowed and secondary colours, out-of-key chords,
step components, PLAYHEADS, MUTATE, the delay's grain; ANIMAL and VOICE on one track each under COMPLEX 61; the tempo
range narrows towards the middle. COMPLEX 100 is the full chaos again.

**The CPU budget** (0.9.13). An EXPLORE piece is made to fit the FM-1's CPU: the generator estimates what the piece
will cost to play (each part by its engine and how many voices it keeps sounding — its chords, how long its notes and
releases ring at the piece's tempo —, the reverb type, the delay's grain, the PLAYHEADS) and keeps it under **65 % of
the audio's time with two cores** (MENU › SYSTEM › ABOUT: CORES 2), **55 % with one** (with the margin of the estimate's error).
When a piece would cost more it gets lighter, a step at a time, until it fits: the heaviest engine swapped for LOFI or
ANALOG, CLOUD / SHIMMER played as ROOM / SPRING, the texture left out, then the arp, then the lead (one core: drums,
bass and chords at least); the delay's grain off, a PLAYHEAD less, a note less in each chord (two at least), the
longest releases shorter. With two cores most pieces are untouched; with one, most lose a part or two. The genres and
RANDOM are not changed by it.

**The sounds** (0.9.13, every genre and EXPLORE). Each sound starts with no modulation left over (the preset's LFO
routes and the previous track's are cleared), then gets its role's: the **filter moving** on almost every melodic part
(its envelope, deeper by role; on chords, arps and textures often a slow LFO on the filter, a cycle over one, two or
four bars at the genre's tempo), a **release** in the role's range (bass short but never a click, chords and textures
long, arps and plucks between), and a **vibrato only on a held lead** (now and then a held pad): soft (a fifth of a
semitone, a third at most), about 5 Hz, fading in. (Up to 0.9.12 the lead, track 5 on a 6-track piece, had a 1–2
semitone wobble at ~1–2 Hz half the time: a siren.) More pieces have a **chord progression**: a one-chord pedal only
now and then (EXPLORE: one in eight), the arp following the chords; ANIMAL comes up a quarter as often in EXPLORE.

What a piece is made of:

- **Drums** from a **map of priorities** (after Mutable Instruments' Grids): every step of every lane has a level, a
  hit where the level clears the density DRUMS sets, so ghosts go first as you turn it down and the kick's anchors
  last. Then the snare's ghost notes on the best positions (the sixteenth before 2 and 4 first), the hats' accent cell
  (house, funk, garage, machine), a small variation on the even bars (COMPLEX), the genre's **fill** on the phrase's
  last bar (a snare roll, a tom run, the kick dropped, a hat roll, a linear funk fill, Blue Monday's sixteenth kicks,
  a dub rim, a break before the one) and the snare laid back where the genre plays late (TIMING). ENERGY and the
  amounts decide how much plays; the kick is never dropped from the one.
- **The key** (0.8.3): a mode of seven notes for every genre (its list: major, minor, dorian, mixolydian, phrygian,
  lydian, harmonic and melodic minor), **written to the SCALE page** (ROOT, SCALE, and QUANT SNAP unless you chose
  another QUANT) on every track, so the keys you play afterwards, the CHORD page, ALL TR and the RESO reverb are in the
  piece's key. A genre that sings pentatonic or blues (deep house, funk, boom bap, D&B, afrobeat ..) gets the mode that
  holds it (dorian or aeolian; major for the major pentatonic) and its **lead** keeps to the pentatonic notes: the
  chords stay real chords and every note is still in the SCALE page's scale. The chord track's **CHORD page** is set
  to the piece's chords (TRIAD, 7TH, ADD9, SUS2 or SUS4, as its progression is coloured), VOICING LEAD, a strum where
  a guitar plays them; CHORD is off on the other tracks.
- **Harmony** (0.8.3): each genre has a family (modal loops; seventh-chord loops; functional pop; colour washes; film;
  jazz) and progressions written as degrees of the mode, each loop ending on the chord that leads back to its first
  (IV–I, bVII–i, V–i, bII–i ..): i bVII, i iv7, i bII (phrygian), I V vi IV, vi IV I V, I vi IV V, i bVI bIII bVII,
  the Andalusian i bVII bVI V, the Aeolian bVI bVII i, i7 bIIImaj7 bVII7 iv7, ii7 V7 Imaj7, Imaj7 vi7 ii7 V7, the
  bossa's Imaj7 II7 ii7 V7, I bVII IV I .. A mode that would turn one of its chords diminished (dorian's vi, lydian's
  #iv) never takes that progression: the genre's next one, else the mode's own loop. Then the cadences and colours the
  genre allows: the minor key's **dominant raised before its tonic** (the leading tone: trance, italo, synth pop,
  film, jazz), **jazz's turnaround** (ii–V in the last bar), a **secondary dominant** on the second half of the chord
  before its target (V/vi, V/ii, V/IV: indie, disco, jazz, film; with COMPLEX), a major key's **IV going minor** before
  I (mixture: synth pop, indie, shoegaze, film), the flat side borrowed in major (bVII, bVI: motorik, indie,
  soundtrack). Harmonic rhythm by genre: a chord every bar, every two or four bars, or one chord for the whole loop.
  Chords are triads, sevenths, ninths (never a b9), add9, sus2 / sus4 (a perfect fourth only), sixths, quartal stacks,
  power chords, recoloured by the genre (plain, rich, suspended, quartal, power, parallel).
- **Bass** from about fifty **archetypes written against the kick**: house offbeat and bounce, deep walking
  sixteenths, techno rolling sixteenths and octave bounce, Detroit, EBM, italo, psy, UK garage, Jamerson, around the
  kick, on the one, slap, Good Times, jazz walking and two-feel, boogie, country, ska, tumbao, bossa, rock eighths,
  punk sixteenths, McCartney, Peter Hook, motorik, angular post punk, indie, one drop, steppers, rockers, dub,
  boom bap, trap 808s, DnB, jungle, wobble, pedal, fifths, Suicide, Kraftwerk; and **acid** by the 303's rules
  (accents on 3–5 steps, slides on 2–3, an octave jump after a slide, the accents varied over the bars). **The root at
  every chord change** (a chord that comes in mid-bar gets its root there), the archetype's third, fifth and seventh
  the chord's own (a borrowed or raised chord's too), approaches by a semitone into the next root on a weak beat (they
  always resolve: a thinner bass never leaves one hanging), a pickup at the end of the fourth bar; the kick
  **doubled**, **answered** (the bass stays off the kick's steps), **interlocked** (and a note right after it) or
  sustained under it, as the genre does.
- **Chords voiced** (0.8.3): of each chord's inversions and spreads, the one nearest the last chord, voice by voice
  (common tones held, the least motion: about 4.6 semitones a change in all, against 5.5 before), in C3..G5, no second
  below G3 and no close third at the very bottom; close in the hand for stabs and comping, **spread** (drop 2) for the
  washes and film, **rootless** (3 5 7 9) where jazz and deep house have the bass on the root, moved in parallel for
  techno and impressionism; played held, swelling, stabbed on the offbeats, pulsed, dubbed through the delay, skanked,
  comped, pushed, as a drone, sparse, broken (UK garage) or trance-gated, each held to its own chord's end.
- **Arp**: an order over the voiced chord — up, up and down, down, Alberti, Glass's groups, a Reich pulse, a
  sequencer, a walk, cycles of 3 or 5 against the bar (polymetric), a trance gate, octaves — in sixteenths or
  eighths, with holes (ARP), in D3..D5; each chord's figure from its first note; only the chord's tones.
- **Lead** (0.8.3: composed, not walked): a one-bar **motif** — a rhythm cell (a long note and two short, a
  tresillo, a pickup into three, a dotted push ..) and a contour of steps (a third now and then, a leap turned back by a
  step, an arch, a fall, a rise or a wave) — developed over the four-bar **phrase** by a template: a **period**
  (antecedent closing open on the 2nd or the 5th, consequent closing on the tonic), a **sentence** (the motif, its
  sequence, a continuation climbing to the climax two thirds in, the cadence), **A A B A'**, **call and response** (the
  answer the call's contour turned), the techno **hook** A A A A', or the dub's with a bar of space; one operator on a
  varied bar (its last note left out, its end moved, a neighbour into its last note). The motif moves to each bar's
  chord the voice-led way (its first note the chord tone a step from the last note; the same chord: the motif as it
  was). The rules every lead keeps: **a chord tone on beats 1 and 3**, on 2 and 4 a chord tone or a passing note that
  moves by steps, passing and neighbour notes between, **mostly steps** (60 % of its moves), never more than an octave,
  a leap past a fourth followed by a step back, a range of about a tenth, the phrase **closing on a stable degree**
  (the tonic, the third or the fifth on a chord tone) and held, no minor ninth or tritone over the chord's root, no
  avoid note held (a fourth over a major third), a borrowed or raised chord's tones followed (the minor key's b7
  becomes the leading tone over its V). Or: a hypnotic **ostinato** on an euclid (its cell's notes on the chord at the
  beats), a machine's figure moved with the chord, **long notes** as a guide-tone line (each the chord tone a step or a
  third on, a short figure stepping into it now and then), a **tintinnabuli** line (stepwise, a chord tone on each long
  note, a passing note between). Never above A5 (B5 in a section lifted a step); high notes softer. Each lead, figure
  and drum pattern is the genre's own for a seed (the same seed in two genres: two tunes).
- **Sounds** by **recipe** a role: sub, acid, Reese, FM bass, rock bass, Hook's chorused bass, wow bass; pad,
  strings, choir, organ (WHEEL), electric piano, stab, dub chord, pluck, bell, lead, vox lead, chip, keys, drone,
  noise, texture, hoover, a shoegaze wall, a tremolo guitar. A recipe picks an engine, a factory sound whose name fits,
  moves its parameters a little, shapes the envelope, the filter (type, cutoff, resonance, the envelope on it), a slow
  LFO, drive, chorus, glide, the voice mode. The **kit** is one of seven (909, 808, click, industrial, soft, dub,
  breaks) written over the factory DRUM-X kit, the drum track's TONE, DECAY, NOISE and DRIVE with it.
- **Mix**: level, pan and sends by role and by how wet the genre is; **HUMAN** sets SEQ > TRACK's HUMAN on every track by
  the role (hats and leads more, the kick none) scaled by how human the genre is (a band more than a machine);
  SWING the genre's unless set; DUCK, CLIP and PUNCH on the master; the delay's TYPE, PING-PONG and TIME and the
  reverb's type the genre's.
- With **TRACKS 6** the sixth track is a **texture** (a drone, noise, a choir, an organ, strings) in long notes.
- **FORM**: **LOOP** writes one loop (2 or 4 bars). **PARTS** also stores sections **A** (the piece), **B** (a break:
  no drums, no bass) and **C** (a drop: the drums denser) and fills ARRANGE with A B A C (PLAY plays the song, as
  0.6.81's DICE ALL did). (0.8.3) The sections are one piece in one key: B starts the loop on its predominant (the
  same chords turned: IV–I–V–vi for I–V–vi–IV; a one-chord loop goes to its subdominant and back), the motif and the
  figures the same, moved onto those chords; in the pop genres (ITALO, SYNTH POP, INDIE, DISCO) **C is a whole step
  up**, a real key change stored in the section with its SCALE page (pop's last chorus). The modal loops (techno, house
  minimal, dub ..) keep their loop in every section. **SONG** writes a club form: **A** the drop, **B** the intro (drums and bass), **C** the build
  (the chords in, their filter opening over the loop, a roll at its end, the delay thrown on the last step) and **D**
  the break (no drums, no bass, the chords wetter; 0.8.3: on its predominant, but in the modal loops), in ARRANGE as
  B C A D C A with 16-bar rows (the drop 32). Ambient and post-rock genres get an arc instead (thin, in, apex: on its
  predominant). Sections of yours (stored by you, not by GEN) are never
  overwritten: then the piece stays one loop.

Every piece replaces every track's steps, sounds, kit, mix and automation. Mutes are left as they are. The roles that
play go on the first tracks (drums on the DRUM track), so with TRACKS 4 you hear the most important ones.

### Sound dice (EDIT + SELECT)

Hold **EDIT** and turn SELECT (from any page): the selected track loads a random factory preset of its engine and each engine
parameter moves up to a quarter of its range. On DRUM, both sides of every kit sound are rolled around the factory kit.
You hear it at once under **KEEP DICED SOUND?** (DRUM: **KEEP DICED KIT?**): turn SELECT again for another roll,
**OCT+ OK** keeps it, **OCT− CANCEL** brings back the sound you had before the first roll (only that track's sound:
the music plays on). After OK, **GLO held + G4** still undoes it.

### Sequence, LFO and FX dice (SEQ / LFO / FX + SELECT)

The same idea for the other parts of the selected track, each heard at once under its **KEEP DICED …?** dialog
(SELECT again: another roll, **OCT+ OK** keeps it, **OCT− CANCEL** puts back what was there, the rest plays on):

- **SEQ + SELECT** (0.9.17): a new sequence, the part GEN's generator would write for the track: on the song's
  harmony when GEN made the piece (its plan, genre and the track's role), so it fits the others; else in the track's
  own key, a genre made up for it. Drums on a DRUM track, else the role its steps sound like (low notes: a bass line,
  chords: a pad, else a lead or an arp). Never the same part twice in a row. Its sound, mix and automation stay.
- **LFO + SELECT**: the track's LFO: a rate in the middle of the range, any wave, MODE FREE or TRIG (half the time
  each; TRIG at any phase), a fade now and then, aimed
  mostly at the filter, a little at the shape, seldom at the pitch or the level.
- **FX + SELECT**: the track's drive (half the time none), its chorus, delay and reverb sends, an AUTO PAN one time in
  three, and the shared reverb's kind, size and damping and the delay's time and feedback (pitched one time in five).

### Macro dice

With the macros open (LFO held) or latched: **F3** rolls new macro routes on the tracks that are sounding;
G3 clears them. See [10](#10-modulation-and-macros).

### UNDO (GLO held + G4)

One step back over the **last big change**. Pressing it again goes forward (redo). The transport keeps playing.
G4 breathes on the GLO map when there is something to undo; with nothing, it says **NOTHING TO UNDO**.

**UNDO covers**: DICE ALL, a sound diced, a track cleared (REC F3, TOOLS CLEAR SEQ), CLEAR ALL SEQUENCES (SEQ + REC),
NEW PROJECT, INIT and RECALL (EDIT layer or TOOLS), clearing the automation (SEQ > AUTOMATION),
and a section loaded with **SAVE > SECTIONS > LOAD**. It restores the music as it was: sounds, steps, automation, the
kit, the punch-in lane, the macros, the tempo and global effects.

**UNDO does not cover**: section jumps, quick chains and RECALL from the SAVE layer, switching project with the SAVE
layer's KNOB 1, CLEAR SONG, deleting a row, SONG REC, anything saved (sections, user presets), user preset loads,
sounds picked with PRESETS, SEQ TOOLS actions, or ordinary knob turns. It is not available while a song plays
(**STOP TO EDIT**). It keeps only one step, in RAM: it's gone after power-off.

---

## 9. Effects and master

### FX's chain: TRACK · BUSES · DELAY · REVERB · CHORUS · SLICER · MASTER · PUNCH

(0.9.2) FX is one flat chain of 16 pages in eight groups, its tabs **TRK · BUS · DLY · REV · CHO · SLC · MST · PCH**:
**SELECT** walks every page (SENDS → DIST → AUTO PAN → BUSES → DLY → DLY 2 → DLY 3 → REVERB → REVERB 2 → CHORUS →
SLICER → SL BUS → SL BUS 2 → SL BUS 3 → SL PITCH → MASTER → PUNCH), an **FX** tap jumps to the next module (round), and there is
no module picker any more (until 0.9.1 SELECT zoomed TRACK ↔ BUSES ↔ EFFECT, PRESETS picked the module and the FX tap
its page). PRESETS loads the track's sounds, as everywhere. The pages, their knobs and their pictures are the same.

| Group | Pages | What the screen shows |
| --- | --- | --- |
| **TRACK** | SENDS (the FX page: DIST CHORUS DLY REV) · DIST · AUTO PAN | The track's set: its picture (the four sends as bars, DIST's curve with TYPE and TONE, AUTO PAN's swing with WAVE and RATE) and DIST's curve small under it. |
| **BUSES** | BUSES | The six modules as tiles, each its picture small: **DELAY** (its echoes), **REVERB** (its decay), **CHORUS** (three waves), **SLICER** (its 16 steps, the one playing framed), **MASTER** (the output now, the duck's curve), **PUNCH** (the lane); with LOAD ECO CHORUS and SLICER are dark ("ECO"). Knobs: **DLY MIX, REV SIZE, CHORUS DEPTH, SLICER DEPTH**. |
| **DELAY · REVERB · CHORUS · SLICER · MASTER · PUNCH** | DLY · DLY 2 · DLY 3 — REVERB · REVERB 2 — CHORUS — SLICER · SL BUS · SL BUS 2 · SL BUS 3 · SL PITCH — MASTER — PUNCH | The module large: its picture, "DELAY 2/3" (its page) in the title. With LOAD ECO the chain skips CHORUS and SLICER. |

- **FX from another page** comes back to the FX page you left.
- The footer says **FX · DELAY · DLY 2  6/16**; the header shows the track, **T1**, in orange.
- The pictures follow the values (a turn redraws them) and, while it plays or sounds, the sound (0.8.5, FX live): the
  echoes swell with the level and a pulse runs along them, the reverb's tail swells and a line sweeps it, the chorus
  waves run at RATE, the slicer frames the step playing, MASTER's meter follows the output, PUNCH's playhead walks the
  lane, DIST's level is a dot on its curve, AUTO PAN's dot swings. Stopped and silent they hold still (drawn by the
  screen alone; nothing of the audio's time).

### Per track

- **FX**: DIST amount and the three sends (CHORUS, DLY, REV).
- **DIST**: amount, **TYPE** (SOFT, HARD, FOLD, CRUSH, RECT, **AMP**), **TONE** (darker / brighter). **AMP** is a guitar
  amp: two valve stages that clip unevenly (low DIST clean, then crunch, then lead), the bias breathing with the level,
  and a speaker cabinet with its presence peak; TONE moves the cabinet's brightness.
- **AUTO PAN**: the track's pan swung left and right around its PAN. **DEPTH** (0 = off), **RATE**, **SYNC** (BEAT:
  RATE in divisions 4BAR … 1/32, locked to the bar; FREE: RATE in Hz), **WAVE** (SIN, TRI, SAW, SQR, S&H).
- **FILTER**: see [4](#4-engines).

### SLICER (one, on a bus)

- **SLICER**: **MODE** OFF, GATE, STUT; **PAT** 1–16; **RATE** 1/8 … 32T; **DEPTH**.
- **SL BUS / SL BUS 2 / SL BUS 3**: ON / OFF for what goes in: **T1–T8** and **FX** (the delay, reverb and chorus returns).
  Everything on chops the whole mix after the effects; with FX off the tails ring free.
- **SL PITCH**: the repeats follow a 16-step pitch sequence (±24 semitones). KNOB 1 the step, KNOB 2 its pitch, LEN,
  QUANT (CHR or SCL: the song's scale).
- Each section keeps its own SLICER settings. With **LOAD ECO** there is no SLICER (its tracks go straight to the mix).

### Delay

- **DLY**: **TIME** (synced divisions 4BAR … 1/32, or free times from 5 ms to 2.95 s), **FEEDBK**, **TONE** (below the
  middle darker, above thinner), **MIX**. FEEDBK behaves like an analog delay's (a tape, a PT2399; 0.6.86, 0.6.89):
  with **TONE in the middle (50 %)** each repeat is a copy of what went in; at **100 %** the echoes hold their level
  for a long time (they fade very slowly and are never cut off), and **past 100 %** they grow into themselves and
  hold, saturating softly instead of clipping (turn it back down to let them die). Up to 80 % it is as before.
  **TONE** is resonant: from the middle down a low-pass sweeping from ~8 kHz to ~250 Hz, from the middle up a low cut
  sweeping from ~70 Hz to ~3 kHz, both with a peak at the cutoff, so the repeats get darker or thinner and narrow to
  a band each pass (dub style), and at either end with FEEDBK high the loop self-oscillates at that band.
- **Long times** (0.8.4): every division plays its real length up to **2.97 s**, the longest (before, anything past
  1.49 s was cut to 1.49 s: 1/2 below 81 BPM, 1/1 below 162 BPM, 2BAR, 4BAR). To fit them the delay's line runs at
  three rates, chosen by TIME on its own:

  | TIME | Line | Top end of the repeats |
  | --- | --- | --- |
  | up to 0.74 s | full rate | full range, as before |
  | 0.74 s – 1.49 s | half rate | flat to ~9.5 kHz, -3 dB at ~10.4 kHz |
  | 1.49 s – 2.97 s | a quarter rate | flat to ~4.7 kHz, -3 dB at ~5.2 kHz |

  So the longer the time, the darker the tail, like a tape or bucket-brigade delay. Only the top end is lost:
  there is no grit or aliasing (anything folded back is more than 70 dB down) and no extra noise. The rate is
  applied once, so the repeats do not get darker with each pass (only TONE and FEEDBK do that). MOD, the GRAIN delay,
  PING, WIDE, TYPE and TONE work the same at every rate. On the two slower rates the first echo comes ~1 ms (half)
  or ~4 ms (quarter) late; the repeats stay exactly TIME apart. When TIME crosses 0.74 s or 1.49 s (knob, tempo,
  automation, a project or section load, an external clock) the delay fades out over 3 ms and starts again empty.
  That leaves a short gap: the echoes already in the line are dropped, but there is no click. A clock that wavers
  around a boundary does not restart it again and again. Long echoes come back at full level even when nothing else
  is playing.
- **DLY 2**: **WIDE** (the right echo later), and the **GRAIN** delay: **PITCH** (±12 semitones per repeat) and
  **SPRAY**. With LOAD LITE or ECO the GRAIN delay plays as a plain delay (the card says LITE / ECO). **MOD**
  (0.6.86): the tape's drift, slow WOW and fast FLUTTER on the repeats (0, the default: none, exact copies; ~20 a
  tape's; up to a seasick chorus); the repeats stay bright while they drift (0.6.89); saved with the project.
- **DLY 3** (0.6.90): **TYPE**: **ANALOG** (the default: the soft saturation, the resonant TONE, FEEDBK past 100 %
  self-oscillating) or **DIGITAL** (clean repeats, TONE a plain filter, FEEDBK 100 % at most: past it the repeats
  just hold, never grow). **PING**: **ON** sends the repeats left, right, left … across the stereo field, each one
  FEEDBK softer (TIME up to 1.49 s since 0.8.4, the right side at twice TIME; WIDE and the GRAIN delay out of it).
  Saved with the project.

### Reverb (five models)

**REVERB > TYPE** picks the model; **SIZE** is the decay on every model. KNOB 3 and 4 change meaning with the type:

| TYPE | What it is | KNOB 3 | KNOB 4 |
| --- | --- | --- | --- |
| **ROOM** | Classic room | DAMP | PRE (pre-delay, ms) |
| **SPRING** | Amp spring tank | DAMP | PRE |
| **SHIMR** (shimmer) | Rises an octave on every pass | DAMP | **AIR** (±24 st: darker and lower / airier and higher) |
| **RESO** | Four strings tuned to the song's scale | **FOLLOW** (OFF / T1–T8: the chord follows the lowest note that track plays) | **PITCH** (the chord 0–14 steps up the scale) |
| **CLOUD** | Granular wash; SIZE at max freezes it | DAMP | **AIR** |

**REVERB 2**: **MOD** (modulation depth), **RATE**, **FILT** (input tone: darker / brighter; 0.6.86 resonant, a band at the cutoff to carve the room's place), **WIDE**.

With **LOAD LITE**, SHIMMER and CLOUD play as ROOM and RESO as SPRING; with **LOAD ECO** every reverb plays as SPRING.
The settings stay; the TYPE card shows LITE or ECO under its value.

Changing the model (the knob, automation or motion, a project or song section, MIDI, the editor, or LOAD going LITE /
ECO and back) cuts the old model's tail with a short fade (under a millisecond, no click) and the new one starts
silent. Since 0.8.4 coming back to **CLOUD** starts an empty cloud, a frozen one too (it used to resume the cloud left
from before).

### Chorus

**CHORUS**: RATE, DEPTH, **STEREO** (0 as always, down to mono, up to about twice as wide). Hidden with LOAD ECO
(a track's chorus send card then says ECO).

### MASTER

- **CLIP**: saturation that adds character, not volume.
- **PUNCH**: fattens the DRUM tracks.
- **DUCK**: the kick pulls the rest down; **REL** is its release.

### DJ FILTER (0.9.1, GLO › MIXER KNOB 4)

One knob on the whole mix, just before CLIP and the leveler. **In the middle (OFF) it is out of the chain**: nothing
is processed and the sound is exactly as without it. Turned left it is a **low-pass** sweeping down (darker: from
~15 kHz a detent off the middle to ~70 Hz at the end); turned right a **high-pass** sweeping up (thinner: from ~30 Hz
to ~6 kHz). The sweep follows the ear (each detent the same musical step), with a mild resonance that grows a little
toward the ends. The card shows the cutoff and the side (**1.2k LP**, **400 HP**, **OFF**) and a small response
curve where the bar would be.
- It glides: the cutoff follows the knob in ~6 ms, no zipper; going into the chain, out of it and from one side to the
  other it crossfades (~6 ms), so crossing the middle never clicks. Turning the knob, it stops once in the middle.
- **Automation**: armed and playing, turning it records it on the selected track (SEQ › AUTOMATION, AUTO LIST shows
  **DJ FILT** rows; automation only, no lock); a stop or the loop's restart brings back where the knob was. The editor
  can set and record it too.
- Saved with the project (in the place a never-used global had: every older project loads with it OFF). Firmware
  before 0.9.1 loads a newer project without its DJ FILTER and its automation.
- Cost: nothing in the middle; on, about 200 instructions a sample on the FM-1 (the CPU TEST song +1.7 % on the host).

---

## 10. Modulation and macros

- **ENV DEST**: the envelope to FILTER, PITCH, SHAPE; **LOOP ON**: with the note held, the envelope goes back to the
  attack when it reaches sustain (an ADSR-shaped LFO).
- **LFO**: RATE, WAVE (SIN, TRI, SAW, SQR, S&H), MODE, FADE; **LFO 2**: PHASE. **LFO DEST**: PITCH, FILTER, SHAPE, AMP.
  - **RATE** is free in Hz; turn it left past the slowest and it goes **in time** with the tempo (4BAR, 2BAR, 1BAR, 1/2,
    1/4, 1/8D, 1/4T, 1/8, 1/8T, 1/16, 1/16T, 1/32), locked to the bar while the transport plays (in TRIG, PHASE is its
    offset), in either MODE: a note never moves it then.
  - **MODE TRIG**: a new phrase (a note with none held) restarts the LFO at **PHASE** (LFO 2) and fades it in again over
    **FADE**; a note played legato over another does not. **MODE FREE**: no note restarts anything: the LFO just runs,
    and FADE fades it in only once, when the project loads or when you turn MODE to FREE. In FREE, PHASE is dimmed:
    the phase TRIG had is kept for when you go back to TRIG. (Until 0.9.18 FREE was PHASE turned left past 0, and FREE
    still faded in again on every phrase; projects keep their mode: PHASE FREE opens as MODE FREE.)
- **MOD matrix**: 4 slots per track (KNOB 1 picks the slot).
  - Sources: OFF, LFO, ENV, VEL, KEY, RAND, MODW (MIDI CC1), AT (aftertouch), EXPR (CC11), **M1–M4**, **STEP** (MSEQ), **NOISE** (a smooth random wander, a new value about every 46 ms: a living wobble on
    the filter, the pan, a send).
  - Destinations: PITCH, CUT, SHP, AMP, PAN, DIST, CHO, DLY, REV, RATE (LFO rate), VIB, **E1–E8** (the engine's own
    EDIT knobs, shown by name), **CLIP** and **PNCH** (the master), and (0.7.0) the FX buses: **D.TIM D.FB D.TON D.MIX**
    (DELAY TIME, FDBK, TONE, MIX), **R.SIZ R.DMP** (REVERB SIZE, DAMP), **C.RAT C.DEP** (CHORUS RATE, DEPTH). AMT 64 at a
    full source = the parameter's whole range; every track's slots on the same bus add. A STEP source (the mod
    sequence) on D.TIM sequences the delay time.
- **MSEQ**: a 16-step modulation sequencer per track, like a CV sequencer. KNOB 1 picks the step, KNOB 2 sets its
  level, LEN and SLEW; DIV on MSEQ 2. Use it from MOD with SRC **STEP**.

### Macros M1–M4

- **Hold LFO**: KNOB 1–4 become M1–M4, and the map shows where each one goes ("NO ROUTE: MOD SRC" if nowhere).
- Let go: they stay **latched** — KNOB 1–4 keep moving the macros on any page — until you press LFO again.
- Every power-on starts with random routes (two per track) on tracks that have none. **F3** rolls new
  routes on the tracks that are sounding; **G3** clears them.
- Routes are ordinary MOD slots (SRC M1–M4): edit them by hand on LFO > MOD. Macro values are saved with each section.

---

## 11. Performance

### Live sequences: GLO › LIVE (0.9.11)

The six sequences side by side, and modifiers you play over them like MIDI effects on a master: divide, multiply,
shorten, reverse, thin, repeat. They never touch the steps: they change how the sequencer walks them, and a track
always comes back in phase. Two pages, the first group of GLO (**LIVE · MIX · GLOBAL · SYSTEM**): a GLO tap inside goes
round the groups, SELECT left of MIXER reaches them; at power-on GLO still opens the MIXER.

**RIELES** (rails): each track's loop across the screen at its own length, its playhead (orange) running at its own
speed, the **ghost** (violet dots: where the track would be if nothing were done to it) and the window of CORTE or LOOP
framed in violet (the steps outside it dim). The numbers at the left: ALGORITHM's track on the orange badge, the
tracks the knobs act on in violet with a bar.

- **KNOB 1 PISTAS**: which tracks the other three act on: **ALL**, then **T1 … T6** one by one, then **SYN** (the
  synths) and **DRM** (the drum tracks). The track you edit is still ALGORITHM's.
- **KNOB 2 RATE**: /4 /3 /2 2/3 · x1 · 3/2 x2 x3 x4 (2/3 dotted, 3/2 triplets). It goes in on the next beat.
- **KNOB 3 CORTE**: the first N steps only (32 … 1, OFF). On the next 1/16.
- **KNOB 4 DENS**: to the left THIN (90 … 10 % of the steps play; the same steps on the same loop every PLAY), in the
  middle everything, to the right RATCH (every step x2 … x4, never a part shorter than ~1.4 ms). On the next 1/16.

**MANO** (hand): the keys become a board; the rails small on top, the board under them (held: orange; latched: violet).

- **Black keys 1–6** (F#3 … F#4): hold them to pick the tracks (several at once); none held, or **A#4**: all of them.
- **White keys**, on while held, back in phase when let go: **F3** /2, **G3** x2, **A3** TR (3/2), **B3** DT (2/3),
  **C4 … F4** LOOP 8 4 2 1 (the N steps the playhead is in; the shortest held wins), **G4** REV, **A4** PING, **B4 C5**
  RATCH x2 x3, **D5** THIN 50 %, **E5 F5** OCT+ OCT− (the sequence's notes an octave up / down), **G5** RETURN. They go
  in on the next 1/16 (the rate keys too).
- **G#4 LATCH**: then a white key turns its modifier on and the next press off (for the tracks held at the time).
  **C#5 / D#5** GIRO: the playhead a step back / on (a rotation). **F#5** RND: the steps in any order (on / off).
- **KNOB 1 RATE, 2 LOOP, 3 THIN** on the tracks held (none: all), latched as any knob; **KNOB 4** the DJ FILTER, the
  same as MIXER's KNOB 4.
- On MANO the keys play no notes; on any other page they play again (a key still held keeps its modifier until it is
  let go).

**On both pages**: **OCT− tapped** is RETURN: the tracks picked lose every modifier and land on their ghost on the next
beat. **OCT− held** is BYPASS: the original while you hold it, every track; let go, the modifiers are back. **OCT+** is
COMMIT: it asks (WRITE LIVE CHANGES?) and **OCT+ again** writes what plays into the tracks: RATE into DIV when one fits
(x2 of 1/16 = 1/32, 3/2 = 16T, /2 = 1/8), else STRETCH (/2 /3 /4 slower, x3 x4 faster); CORTE into LEN (a LOOP's steps
moved to the start); GIRO into ROTATE; THIN into the track's CHANCE. REV, PING, RND, RATCH, OCT and a rate with no DIV
or STRETCH (2/3 of a 1/16) stay live. **UNDO: GLO + G4.**

**How the two pages combine**: they drive the same modifiers, one set per track. The knobs of both pages are one
latched layer: RATE is the same value on both; MANO's LOOP and RIELES' CORTE are the same window (LOOP counts from where
the playhead is, CORTE from step 1; the last one turned wins); THIN and DENS are the same density. MANO's keys sit on
top while held or latched: a rate key replaces the rate, a LOOP key shortens the window (the shortest wins), THIN takes
the thinner, RATCH the larger; REV, PING and OCT are the keys' alone. MANO is the hands-on face (momentary, several
tracks at once), RIELES the latched, visual one.

**Recording**: with REC armed, a note played while its track is modified lands where the ghost is (where the step is
in the written pattern), so recording over a modified track never scrambles it; the automation and the locks follow
the step that sounds.

**Never saved**: the modifiers are performance state, like the punch-in FX; STOP keeps them, NEW PROJECT clears them.
What you COMMIT is what stays. They cost nothing in the audio render, a few instructions a track in the sequencer.

### Punch-in FX (hold FX)

Effects that act while their key is held and go back exactly when you let go. **FX > PUNCH** (0.6.99) is their
sequencer: the lane that records them. **LEN** its length, 1 … 64 steps; **RES** the step, 1/8, 1/16 or 1/32 of a
beat (64 steps of 1/16 are four bars, of 1/32 two, of 1/8 eight); KNOB 3 says what it is doing (REC while recording,
PLAY while it plays, else how many steps hold an effect); **CLEAR** (KNOB 4, then OCT+) empties it. The page draws
the lane's cells in the effects' colours with the playhead on them, the last effect's name and the tracks it acts
on (ALL, SYN, DRM). To record: REC armed and playing (not in a song), hold FX and the effect's key; the steps it
passes get it; **G5** held erases where the playhead passes. Saved with the section.

- **On the audio**: REPEAT 1/8, 1/16, 1/32 (F3 G3 A3) and **the glitcher** (0.9.17): **GLITCH** (B3), **TAPE STOP**
  (C4), **FREEZE** (G#4), **SHUFFLE** (C#5). They start on the next 1/16 while playing (at once when stopped). Below
  about 81 BPM, REPEAT 1/8 is too long for its buffer and is shown dimmed. **While a glitcher key is down** (held, or
  pressed with FX LATCH) **the four knobs are its own** (the cards and the footer say so); let it go and they are the
  macros again. Its settings are not saved.
  - **GLITCH** (0.9.15; knobs 0.9.17: **DENS PITCH SIZE VARI**) cuts the master on the transport's sixteenths: each is
    played live, or the last slice repeated (SIZE: from a 1/16 down to a 1/256), each repeat PITCH semitones above or
    below the one before (a riser, or a fall), or the last sixteenth reversed, an octave up or down, gated, **SKIP**
    (tiny fragments from anywhere in the last 1/16, like a CD skipping) or **CRUSH**ed. DENS sets how many sixteenths
    are cut (the beats less often); VARI picks another pattern. Which slice does what is fixed by its place in a
    two-bar cycle of the transport, so the pattern grooves and comes back the same every time. Every slice fades in and
    out (no clicks). Below ~82 BPM only its gates, crushes and live slices play (the buffer holds 371 ms).
  - **TAPE STOP** (0.9.15; knobs 0.9.17: **SCRATCH TIME CURVE**): the whole mix slows to a stop over TIME (1/64 to a
    beat, at most what the buffer holds), the pitch falling with it; CURVE − drops at once, + holds then drops. Once
    stopped, the end of the music is kept and **KNOB 1 is your hand on the record**: turn it to scratch back and forth
    (the pitch with the speed, silent at rest). Let go of C4: the music at once, where it is.
  - **FREEZE** (0.9.17, G#4; **DENS PITCH SPRAY SIZE**): the eighth after the press is kept (you hear it live
    meanwhile), then played as grains: SIZE 1/128 to 1/8 long, 1 to 4 overlapping (DENS), PITCH semitones, slowly
    scanning the eighth and back (a stretch); SPRAY scatters where they read and moves some an octave, a fifth or a
    fourth.
  - **SHUFFLE** (0.9.17, C#5; **AMOUNT SIZE SPEED VARI**): the last bar's 1/16s (1/8s, 1/4s: SIZE) played back in
    another order: AMOUNT of them are replaced by another piece of the last bar (the beats, then the bar's first, only
    at higher amounts); SPEED plays the moved ones at half speed, double, or their first half twice; VARI picks another
    order. The bar is kept compressed in the same buffer (down to about 87 BPM a whole bar).
  REPEAT and the glitcher share one buffer (the SLICER's, borrowed: the SLICER plays live meanwhile); of those held,
  the last pressed plays, and letting it go returns to the one held before.
- **On the notes**: OCT− / OCT+ (D4 E4), 1/2 TEMPO (F4), DEC− (short, plucky) / DEC+ (long) (G4 A4), STUTTER 1/8 / 1/16
  (B4 C5), ATK+ (slow attacks, drums too) (D5), ARP (E5; on a DRUM track a new fill every press), RANDOM (F5). They come
  in on the next 1/16. **A#4** chooses which tracks they touch: ALL, SYN (synths) or DRM (drums); the header shows it.
- **The sweeps** (0.9.15, black keys **D#5 F#5**; until 0.9.14 the knobs FILTER CRUSH THROW and the keys LPF HPF):
  held, the effect goes from dry to wet over **2 bars at the tempo**, on an S curve (it leaves dry slowly, speeds up in
  the middle, settles into wet); let go, it goes back to dry over **a beat** on the same curve, so a release in time
  with the music sounds deliberate. They act at once (not on the 1/16) and stack with everything.
  - (The FILTER sweep on C#5 is gone in 0.9.17: KNOB 4's FILTER macro is the layer's one filter, and C#5 is SHUFFLE.)
  - **CRUSH** (D#5): fewer bits (4 to 11 taken off) and a lower rate (a sample held 1 to 8 times), and its mix rising.
  - **THROW** (F#5): the dry mix (and the SLICER's bus) into the delay and reverb sends, up to all of it: the echoes and
    the room build up, and stay ringing when you let go.
- **The knobs: PERFORMANCE MACROS** (0.9.15; until 0.9.14 FILTER CRUSH THROW DEPTH) on the whole project, never written
  into the patterns, never saved. Each is OFF at its left end and builds up turning **right**:
  - **KNOB 1 STEPS**: every sequence plays only part of itself, from where it is: **1/2, 1/4, 1/8, 1 STEP** of its
    length (three detents a stage), like a loop roll; it goes in on the next 1/16 (seq_live's LOOP, as GLO › LIVE).
  - **KNOB 2 TRANSPOSE**: every melodic sequence along the scale, **+2ND +3RD +4TH +5TH +6TH**, then **+OCT** and
    **-OCT** (three detents an interval); the next notes play it; DRUM tracks untouched. It adds to SCL's STRN.
  - **KNOB 3 MUTATE**: the music as it is becomes MUTATE's anchor and every pattern mutates around it by the amount
    (notes along the scale, accents, notes added off the beat or taken, drum hits), on the next beat and again every bar.
  - **KNOB 4 FILTER**: the master DJ FILTER's low-pass closing.
  - **Turn any of them back** (two detents counter-clockwise; one is ignored, so a nudge does nothing) and **all four
    are off at once**: full length, no transposition, every pattern exactly as before MUTATE, the filter open. The
    screen says **MACROS OFF** and the cards drop to OFF. Then each starts again from zero as you turn it right (the
    knobs are encoders: nothing jumps). STEPS lands the tracks back in phase on the next 1/16; MUTATE puts the steps
    back at once.
  - They **stay on when FX is let go** (the FX LED stays lit); they turn off with the cut, FX + OCT− (FX LATCH), the
    menu or a dialog. Not recorded as automation, not stored in scenes.
- **Black keys 1–6**: mute T1–T6 while held (not saved).

**The punch-in lane**: each section has a lane of 64 steps of 1/16 (4 bars). With **REC armed and playing** (not
during a song), the **note effects (D4–F5)**, **GLITCH**, **TAPE STOP**, **SHUFFLE** and **the sweeps** you hold are recorded into
it, one per step (the lowest held; REPEAT and the mutes are not recorded). Played back, a sweep held over a run of steps
ramps as if you held it. Hold **FX + G5** while playing to erase where the play head passes; stopped, FX + G5 erases
the whole lane. **REC held + FX** also clears it (along with the track's FX automation).

**Lock**: hold FX until its map opens and let go without touching anything: the FX layer stays open (**[FX]
LOCKED**), as the macros do with LFO; the keys play the effects (momentary: held on, let go off), the knobs are FX's
macros and OCT− is ALL OFF, with both hands free. Press **FX** again to unlock (latched effects you left on stay on). Held with keys or knobs, FX
works as before and closes when let go.

**FX LATCH** (MENU › SYSTEM › CONTROLS › FX LATCH, **OFF by default**; ON by default from 0.6.44 to 0.6.63): ON, a key pressed
with FX toggles its effect on and off, **FX + OCT−** turns everything off, the macros too (FX ALL OFF). Latched effects are not recorded into the lane.

### Mutes and solos

- **GLO held**: black keys 1–6 mute T1–T6 (latched, saved with the project and in scenes); F3–D4 solo while held;
  black keys 7–10 mute the DRUM-X groups; E4 unmutes everything.
- (0.9.1) MIXER's KNOB 4 is the DJ FILTER now: a track is muted from the GLO layer (above), as before; the MIXER still
  shows the MUTE badge on a muted track.
- **EDIT held** on a DRUM track: black keys 1–8 mute each DRUM-X sound.
- **FX held**: black keys 1–6 mute while held.

### Live layers and the song

The SAVE layer (section jumps on the bar, quick chains, store, recall, SONG REC) is described in
[7](#7-arrange-songs-sections-and-scenes). GLO + PLAY restarts from the top without stopping.

---

## 12. Saving and loading

This is the chapter to read carefully. JIANT keeps several kinds of data in different places, saved in different ways.

### The big picture

- **The music** is what's playing now, in RAM: all six tracks' sounds and steps, automation, effects, tempo.
- **Sections** (A–D of each of the 8 projects) are where you **save** music permanently, in flash.
- **The arrangement** (rows, END, scenes) belongs to the project and saves itself.
- **User presets** (32 slots) save single sounds.
- **Settings** (the MENU, TRACKS, LOAD) save themselves.
- **The autosave** is a safety copy of the music, so you find it again after power-off. **It is not a save into a
  section**, and it does not happen at power-off.

### What is saved where

| What | Where it lives | How it gets saved | Survives power-off? |
| --- | --- | --- | --- |
| The music now (sounds, steps, automation, tempo, effects, kit, macros, punch-in lane, SLICER) | RAM | Not saved by itself — store it into a section. The **autosave** keeps a copy. | Only via the autosave |
| A **section** (A–D of a project) | Flash | **SAVE > SECTIONS > SAVE** (stopped), or **SAVE layer C4–F4 / OCT+** | Yes (layer stores: once you stop) |
| The **arrangement** (rows, bars, END, scenes) of each project | Flash (song index) | Automatically, ~2 s after a change while stopped | Yes |
| **User presets** U01–U32 (one sound + a 16-step pattern) | Flash | **SAVE > USER > SAVE** (stopped) | Yes |
| **Settings** (MENU, TRACKS, LOAD, favourites) | Flash | Automatically (MENU: when it closes; while playing: after you stop) | Yes |
| The **autosave** | Flash, its own place | 10 s after the last change, stopped and idle, at most once a minute | Yes (restored at power-on if RESTORE LAST is ON) |
| The **FM6 voice bank** B1–B32 | Flash, its own place | Sent from the web editor | Yes (not in the backup: keep the .syx) |
| **UNDO** | RAM | — | No |

### How the autosave really works

- It writes a copy of the music **after 10 seconds without changes**, only when the transport is **stopped**, no key or
  button is held and nothing is sounding, and **at most once a minute**.
- It does **not** save when you switch off. If you change something and power off within those seconds, that last
  change is lost. To be safe: **stop, wait ~10 s**, or better, save into a section.
- At power-on the device starts at zero, as **NEW PROJECT** leaves it (0.9.9: T1 DRUM-X, T2 VOICE, T3 ANIMAL, T4 LOFI,
  T5 ANALOG, T6 FM6, each its preset's sound, dry; no
  steps). With **MENU › SYSTEM › RESTORE LAST ON** the autosave becomes the music instead (the screen says
  **RESTORED**). RESTORE LAST is **OFF** by default since 0.6.40: no autosave at all.
- After a crash, a hang or a failed start, the autosave is not restored (the music that was playing could be what
  crashed).

### SAVE's chain: SOUND · PROJECT · LIBRARY

(0.9.2) SAVE is a chain of six pages in three groups, as every section: **SELECT** walks USER → PRESETS → SECTIONS →
ARRANGE → PROJECTS → TOOLS, a **SAVE** tap jumps to the next group (round). The pages, their knobs, OCT+, the dialogs
and the NAME screen are the ones they always were (until 0.9.1 SELECT zoomed LIBRARY ↔ PROJECT ↔ SOUND and the tap
switched each level's two sets).

| Group | Pages | What the screen shows | Knobs |
| --- | --- | --- | --- |
| **SOUND** | USER · PRESETS | The track's being (its engine's creature, alive with its sound), the track and its sound under it; at the right the 32 user slots (a stored one: its engine's creature small and its name) or the factory sounds. | SLOT, LOAD, ERASE, SAVE / No., ENG, FAV, LIST |
| **PROJECT** | SECTIONS · ARRANGE | The project's four sections large, ARRANGE's rows as chips, the song as a strip (see [7](#7-arrange-songs-sections-and-scenes)). | SECT, NAME (shown; SAVE + C5 renames), LOAD, SAVE / ROW, SECT, BARS, SCENE (OCT+ adds a row) |
| **LIBRARY** | PROJECTS · TOOLS | The eight projects as tiles: the number, the name (its section A's), the four sections A–D as chips (stored: in the section's colour; empty: dark). The project open tinted and framed violet, its section picked framed white. / The six tools as a list. | PROJECT, SECT, LOAD, SAVE / the tool |

- **SAVE from another page** comes back to the SAVE page you left; **USER** and **SECTIONS** come with their SAVE
  already picked: SAVE, OCT+, OCT+ stores the sound (or the section), as before. Left on another SAVE page, go back
  to USER with SELECT (or a SAVE tap from the LIBRARY, round); reached from inside, SAVE is not picked for you
  (KNOB 4 to the right).
- The other projects' tiles are read from the flash a project a frame (the headers only: no loading, nothing the
  audio notices) and kept until a project is saved, loaded or renamed.
- The footer says **SAVE · LIBRARY · PROJECTS  5/6**. The OCT+ / OCT− hints are not drawn on these pages: the armed
  card says OCT+.

### To save a project section

1. **Stop** the transport (saving needs it stopped: otherwise **STOP TO SAVE**).
2. Press **SAVE**, then **SAVE** again to the next group (or turn **SELECT**): **SECTIONS** (the PROJECT group) or
   **PROJECTS** (the LIBRARY group).
3. On **PROJECTS**, **KNOB 1**: the project (1–8), its tile framed; **KNOB 2** its section. Turning KNOB 1
   does **not** change the music you're playing — it only chooses where to save. (It switches projects, so do this
   stopped.)
4. On **SECTIONS**, **KNOB 1**: the section A–D, drawn large with its name (or USED / EMPTY) and **NOW** on
   the one the music came from; KNOB 2's card shows its name.
5. **KNOB 4** to the right: SAVE is picked ([OCT+]). Press **OCT+**.
6. If the section already holds something: **OVERWRITE PROJECT A?** (with its name). OCT+ YES.
7. The **NAME** screen opens with a name ready. Edit it or not, then **OCT+** writes. **OCT−** cancels and writes
   nothing.
8. The screen says **SAVED**.

A section stores the music **including the tempo and global effects**. The **project's name** is its section A's name.

### To load a section

1. SAVE's PROJECTS: **KNOB 1** the project, **KNOB 2** the section (or SECTIONS: **KNOB 1** the section).
2. **KNOB 3** to the right (LOAD), press **OCT+**. The screen says **LOADED**.

LOAD **stops playback** and replaces the music: sounds, steps, automation, the kit, the punch-in lane, the macros,
and the **tempo, effects, MIDI routing and clock settings** stored with it. The arrangement stays. If you didn't mean
it, **GLO + G4** (UNDO) brings back what you had.

### Quick store while playing (SAVE layer)

Hold SAVE and press **C4–F4** to store the music into section A–D of the current project, or **OCT+** to store into
the section playing. This works **while playing**, without naming. The store is in **RAM at once and goes to flash
when you stop**. If you power off before stopping, it's lost — so stop before switching off.

### Copying

- **A section to another letter**: LOAD it, then SAVE it into the other letter.
- **A section to another project**: LOAD it, turn KNOB 1 to the other project (the music stays), SAVE into a letter.
- **"Save as" a new project**: with your music playing, stop, go to SAVE's LIBRARY, turn KNOB 1 to an empty project
  and SAVE into A (SAVE > PROJECTS).

### Switching projects

- On **SAVE > PROJECTS, KNOB 1** changes project but **keeps the music** (to store it there or load one of its sections).
- With the **SAVE layer's KNOB 1** (hold SAVE, turn, let go) the project changes **and its section A becomes the
  music** (if it has one). The previous music is replaced and **UNDO cannot bring it back** — save it first.
- Both need the transport stopped (**STOP TO CHANGE PROJECT** / **STOP TO CHANGE SONG**). Sections stored with the
  SAVE layer are written to flash before switching.

### Naming (the NAME screen)

Opens when you save a section or a user preset, and with **SAVE held + C5** (NAME, 0.9.2; until 0.9.1 an EDIT tap)
on SAVE > USER, SECTIONS or PROJECTS (renames the selected slot, section or project; only the name is written). Up to 12 characters, upper case.

| Control | Does |
| --- | --- |
| White keys | Letters phone-style: AB, CD, EF, GH, IJK, LM, NO, PQ, RS, TU, VW, XYZ, then 123, 456, 789, 0-. — tap again within 0.8 s for the next character |
| Black keys (by name) | **F#** cursor left, **G#** space, **A#** cursor right, **C#** delete, **D#** switch ABC / 123 (digits and symbols, one per key) |
| KNOB 1 | move the cursor |
| KNOB 2 | change the character at the cursor |
| OCT+ / OCT− | write / cancel |

An empty name: a project section gets none, a user preset an automatic one.

The same screen types a VOICE track's **lyric** (EDIT > WORDS, OCT+): up to 63 characters, the field scrolls with the
cursor, **KNOB 3** throws the dice, and OCT+ sets it even while playing (it is not a flash write).

### User presets (SAVE > USER)

32 slots for single sounds. **SAVE** comes back to this page with SAVE already picked (when you left SAVE on it, as at
power-on), so **SAVE, OCT+, OCT+** saves the selected track's sound with the offered name.

- **KNOB 1**: the slot (U01–U32). **KNOB 2** LOAD, **KNOB 3** ERASE (asks first), **KNOB 4** SAVE (asks before
  overwriting, then NAME). Turn the knob right to pick, **OCT+** to do it. Saving and erasing need the transport stopped.
- A user preset stores the **engine, its sound parameters and a 16-step pattern**; an FM6 sound keeps its patch.
- It does **not** store the **DRUM-X kit** (that lives in the project sections).
- Loading one loads the sound only.
- Your user presets also appear when browsing with PRESETS, after the factory sounds.

### Saving FAQ

**I switched off and my last changes are gone.** The autosave only writes after 10 s of no changes while stopped, at
most once a minute, never at power-off. Stop and wait a moment, or save into a section.

**It says STOP TO SAVE.** Saving into a section or a user preset writes flash, which pauses the audio for a moment, so
it only happens stopped. Press PLAY to stop and try again (the NAME screen stays open; PLAY there only stops).

**It says SAVED (NO FLASH).** The flash chip wasn't detected, so the save only went to RAM. It survives a reset, not a
power-off. Restart the device; if it keeps saying it, see [16](#16-troubleshooting-and-faq).

**It says SECTION 90% FULL / TOO FULL TO SAVE / SECTION FULL.** A section has a fixed size. Very busy music (six tracks
of chords on all 64 steps plus lots of automation) can outgrow it. Saving a section that doesn't fit says SECTION FULL
and leaves the stored one untouched. Shorten patterns, clear unused steps or automation.

**I stored with the SAVE layer while playing and lost it.** Layer stores reach flash when you stop. Stop before
switching off.

**Is my arrangement saved?** Yes, automatically, a couple of seconds after each change while stopped.

**Where's my drum kit in a user preset?** User presets don't store the DRUM-X kit; save the project section instead.

**Can I back everything up?** Yes: the web editor's complete backup includes all 8 projects with their sections,
arrangements and scenes, the user presets and the settings. See [15](#15-web-tools-emulator-installer-editor-backup).

---

## 13. System: TRACKS, LOAD and the MENU

### TRACKS (GLO > SYSTEM, KNOB 1 — also MENU > AUDIO > TRACKS)

- **4** (the default): T1–T4 play; **T5 and T6 are asleep**. They keep their sound, steps and mix and cost nothing.
  If a note is sent to an asleep track (a key, MIDI, its steps) or PLAY starts with steps on them, the screen says
  **T5-T6 ASLEEP** and **GLO>SYSTEM>TRACKS 6**. Asleep tracks are dim on the GLO map, greyed on HOME and the MIXER.
- **6**: all six tracks play. Watch the load meter.

### LOAD (GLO > SYSTEM, KNOB 2 — also MENU > AUDIO > LOAD (SYSTEM until 0.9.11))

How heavy the sound may get.

| LOAD | What changes while it sounds |
| --- | --- |
| **FULL** (default) | Everything as set. |
| **LITE** | SHIMMER and CLOUD reverbs play as ROOM, RESO as SPRING; the GRAIN delay plays plain; FM6 plays two voices. The screen draws lighter: the beings and graphs animate at about 20 frames a second with simpler outlines, and values change at once (no rolling digits). |
| **ECO** | LITE, plus every reverb plays as a light SPRING, **no SLICER** (its tracks go straight to the mix), **no chorus** (their pages are hidden), at most **4 voices** per synth track and **6 voices** between all tracks (8 otherwise, 12 with both cores; the drums count too). When the load climbs, nearly silent tails are let go sooner. The screen at about 14 frames a second, the beings simpler still. |

LOAD only changes how things are rendered: **projects keep their settings** and sound as saved on FULL (or on another
FM-1). The cards of swapped effects say LITE or ECO. LOAD frees **CPU**, not RAM: the firmware reserves all its memory
at start-up. The **CPU** card (KNOB 4 on SYSTEM) and the header's load meter show the audio load. TRACKS and LOAD are
device settings, not part of a project.

### ROUTE (GLO > SYSTEM, KNOB 3)

**CH1-8**: MIDI channels 1–8 play tracks 1–8, channels 9–16 are ignored. **SEL**: every channel plays the selected
track.

### GLOBAL (GLO > GLOBAL)

**BPM** (40–240), **SWING**, **CLOCK** (INT, USB, TRS: follow MIDI clock and start/stop from that input), **TUNE**
(±50).

### The MENU (hold HOME)

(0.9.18) The MENU is driven like every section: **nine pages of up to four settings** in five tabs, **SCREEN · AUDIO · MIDI
· USB · SYSTEM** (a dot a page under each tab). **SELECT** walks the pages (stopping at the first and the last),
**ALGORITHM** jumps to the next / previous tab's first page, **KNOB 1–4** change the settings on the cards, and
**OCT− / OCT+** step the setting you turned last (its row is framed; the LEDs breathe the way it can go). PRESETS does
nothing here. HOME closes it (the settings are saved then) and opens it again on the page you left. The header says
**MENU · AUDIO**, the footer **MENU · AUDIO · CLICK  5/9** (a page named as its tab is not said twice: **MENU · SCREEN 2**, **MENU · SYSTEM**).

Under the cards, a row for each setting: its knob's number, its full name, its value (in its heat colour, like the
card's) and a plain line on what it does (for some, on what the chosen value does). **CALIBRATION, ABOUT and LEARN
CLEAR** are actions: turn their knob to the right and the card says **OCT+** (armed); OCT+ does it; turning left lets go.
(Until 0.9.17 the MENU was one list of 30 rows, SELECT walked the rows and any knob changed the selected one; 0.9.18 kept
every setting, its values and its meaning: nothing changes in the saved settings or for the editor.)

| Tab › page | KNOB | Setting | Values |
| --- | --- | --- | --- |
| SCREEN › SCREEN | 1 | LARGE | OFF, ON (big labels and values) — (GARDEN) COLOR, SKIN and STYLE are retired: one look |
| SCREEN › SCREEN 2 | 1 | SCOPE | **OFF** (default), ON — (0.9.7) an engine's EDIT pages: OFF the creature on the whole panel, centred, the page's name and the sound's name small in its corners; ON the creature on the left, the SCOPE card and the chips as before. Settings saved before 0.9.7 load as OFF |
| | 2 | ANIMATION | ON, OFF (values snap, the garden still; the power-on signature appears whole) — was ANIM |
| | 3 | SCREEN SLEEP | **NEVER** (default), 5 MIN, 15 MIN, 30 MIN, 60 MIN — the screen dark after that long without a touch (was SCREEN OFF) |
| SCREEN › LEDS | 1 | LED GLOW | OFF, LOW, **HIGH** (default), INVERT — how the idle keys and buttons glow (was LEDS: OFF, DIM LO, DIM HI, INV); (0.7.3) the glow's LEDs take turns frame to frame, so it no longer hums through the speaker once the music stops |
| | 2 | SCALE LEDS | OFF, ON — the keys of the scale glow, the root brighter |
| SCREEN › MOTION | 1 | MOTION | **100%** (default), 50%, 150%, 200% — (GARDEN) how fast the garden and its creatures move |
| AUDIO › AUDIO | 1 | SPEAKER EQ | FLAT, LOWCUT, BASS+ (an EQ on the master for the small speaker; also reaches the outputs and USB) |
| | 2 | TRACKS | **4** (default), 6 — the same as GLO › SYSTEM KNOB 1 |
| | 3 | LOAD | ECO, LITE, **FULL** (default) — the same as GLO › SYSTEM KNOB 2 |
| | 4 | BPM LOCK | OFF, ON — DICE ALL keeps the tempo (the tempo is GLO + SELECT, GLO's F4 TAP or GLOBAL's BPM) |
| AUDIO › CLICK | 1 | CLICK | OFF, REC (while a track is armed and playing), ON |
| | 2 | CLICK LEVEL | LOW, MID, HIGH |
| | 3 | COUNT-IN | OFF, 1 BAR, 2 BARS |
| MIDI › MIDI | 1 | MIDI OUT | **KEYS**, CLOCK, ALL, OFF — what goes out over USB MIDI (see [14](#14-midi-and-usb)) |
| | 2 | LEARN | **OFF**, ON — (0.9.12) arms MIDI LEARN: a knob turned picks its parameter, the next CC is learned to it (see [MIDI LEARN](#midi-learn-0912)); not saved, OFF at power-on |
| | 3 | LEARN CLEAR | an action: the card shows how many CCs are learned (NONE, 3 CC …); KNOB 3 right arms it, OCT+ forgets them all |
| USB › USB | 1 | USB AUDIO | CONSOLE (the serial console with stereo USB audio, the default), STEREO (audio + MIDI only, class 0: helps macOS 13–15 see the USB audio input), STEMS (0.7.1, 10 channels from stems-10: tracks 1–8 and the FX return L R, see "Stems" below), IN+OUT (0.7.5: stereo audio both ways, see "USB audio in" below), STEMS+IN (0.8.0: the stems with the audio output too, 10 channels: 1–8 the tracks, as STEMS, 9 the host's audio as the mix took it, 10 as it came; no FX return, see "Stems with a voice" below) — was SYSTEM › USB; applied when the MENU closes |
| | 2 | USB LEVEL | MASTER (USB follows the MASTER knob), FIXED (USB at full level), −6 / −12 / −18 / −24 dB (USB fixed and attenuated: for a phone or a computer whose input clips at the full level) |
| SYSTEM › CONTROLS | 1 | HOLD TIME | 0.3 s, **0.4 s** (default), 0.5 s, 0.6 s — how long until a held button opens its layer (was HOLD) |
| | 2 | KNOB SPEEDUP | **ON** (default), OFF — fast turns take bigger steps (was KNOB ACCEL) |
| | 3 | FX LATCH | **OFF** (default since 0.6.64), ON |
| | 4 | CALIBRATION | an action: OCT+ opens HARDWARE CALIBRATION (teach each button and knob) |
| SYSTEM › SYSTEM | 1 | RESTORE LAST | **OFF** (default: no autosave, power-on at zero), ON (the last session back at power-on) |
| | 2 | CPU CORES | 1 CORE, **2 CORES** (default) — from the next start (see [Troubleshooting](#16-troubleshooting-and-faq)); was CPU |
| | 3 | CPU CLOCK | **360 MHZ** (default), 320 MHZ — from the next start (see [Troubleshooting](#16-troubleshooting-and-faq)) |
| | 4 | ABOUT | an action: OCT+ opens version, QR code, AUDIO LOAD (NOW, PEAK, LATE, SHED since power-on), CPU TEST, and credits (scroll with SELECT, OCT− back); there OCT+ opens PROFILE |

INIT SYSTEM (until 0.9.17 in the MENU) is **SAVE › TOOLS › NEW PROJECT**: the same dialog, the same UNDO.

If you hear clicks, open ABOUT and look at **LATE** (audio blocks that came too late) and **SHED** (voices the guard
faded): a photo of that screen helps when you report a problem.

**PROFILE (0.9.14): where the CPU goes.** On ABOUT, OCT+ opens PROFILE (SELECT scrolls, OCT− back to ABOUT). It
measures on the FM-1 itself from the first time it is opened until power-off. Every row is a share of the audio's time,
in % of a 725 µs block: first the mean and the worst block of **the last second playing**, then (grey) of **the last
second stopped** ("-" until there has been one). The rows: SEQUENCER; **T1–T6** (the track's engine; **C1** when it
ran mostly on the second core); FX BUSES and of them the REVERB (its type) and DELAY+CHORUS; CORE 0 WAITS (the first
core idle, waiting for the second); FX LAYER..CLIP; MASTER; USB+OUTPUT; CORE 1 BUSY; TIMER5 IN IT (the keyboard and USB
interrupt inside the render) and **WHOLE HALF** (the same as AUDIO LOAD); then TIMER5 ALL, IRQ LATE US (how late the
audio interrupt started at worst: the screen or a flash write held it), LATE HALVES, FX REST % (the time the effects
rested: stopped they cost nothing since 0.9.14) and UI FRAMES (a second). Below: the voices sounding and the cap,
the LOAD the sound plays, the cores and the CPU TEST. **To help tune JIANT on real
hardware, photograph it** (two or three screens) while your song plays, and again a few seconds after STOP.

**CPU TEST (0.9.14).** SAVE › TOOLS 6 CPU TEST builds the heaviest reference song (TRACKS 6); press PLAY, let it play
10 s or more and STOP: its mean load is kept with the settings (CPU TEST 98 % (2 CORES) on ABOUT and PROFILE). GEN's
EXPLORE sizes its pieces by it (the computer's estimate holds that song at 63 % with two cores); never measured, it
assumes the FM-1 is 1.6 times heavier than the estimate.

---

## 14. MIDI and USB

- **USB** is class-compliant MIDI and audio: the master output reaches the computer as a USB audio input.
- **MIDI IN** over USB and the **TRS** jack. **ROUTE** decides which track each channel plays (see
  [13](#13-system-tracks-load-and-the-menu)). Incoming notes on an armed track are recorded like the keys.
- **MIDI OUT** (over USB; MENU > MIDI > **MIDI OUT**, 0.6.72; in CONTROL until 0.9.11), each track on its channel (T1 = channel 1 … T6 =
  channel 6):
  - **KEYS** (the default): the keys send their notes, as before.
  - **CLOCK**: and the clock (24 a beat) with **START / STOP**, while the tempo is the FM-1's own (CLOCK INT): a drum
    machine or a DAW follows it. The clock runs while stopped too, so the follower knows the tempo before PLAY.
  - **ALL**: and every note the FM-1 plays: the sequencer, the arp, PLAYHEADS and the punch-in repeats, with the punch-ins'
    OCT− / OCT+ and RANDOM on them; each note gets its note-off, and STOP, a panic or a change of the setting end
    whatever is still on. Record the FM-1's music into a DAW as MIDI, or play other synths from it.
  - **OFF**: nothing.
- **PROGRAM CHANGE** (0.6.72): program *n* on the channel a track plays loads the PRESETS list's sound number *n* + 1
  (every engine's presets in their order, then the user presets) into that track; the selection stays where it was.
  The numbers follow the list exactly as PRESETS shows it: since 0.9.9 it starts with DRUM-X (program 0 is the
  DRUM-X kit), then VOICE, ANIMAL, LOFI, ANALOG, FM6, PHASE, WHEEL, NOISE. A MIDI setup made for 0.9.8 or older needs
  its program numbers set again. Programs reach the list's first 128 sounds; the ones after them (and the user
  presets) are picked on the device or from the editor.
- **Sections and projects** (0.6.80): **channel 16** is the song's: its PROGRAM CHANGE never loads a sound (whatever
  ROUTE). Programs **0–3** jump to section **A–D** of the project (playing: on the next bar, as the song layer's keys;
  stopped: at once; an empty one is said so); programs **16–23** load project **1–8** with its section A (stopped only,
  as SAVE > SECTIONS). A foot controller or a DAW can run a set.
- **Not possible**: MIDI OUT on the **TRS** jack (the FM-1's jack is an input only: no output line goes to it) and
  **Bluetooth MIDI** (the chip has a radio, but its Bluetooth software is JieLi's closed SDK, which JIANT does not use).
- **Clock**: GLO > GLOBAL > CLOCK **USB** or **TRS** follows that input's clock and Start / Continue / Stop. With an
  external clock, TAP and GLO + PLAY restart are disabled.
- **Controllers** (on the track the channel plays): CC5 glide, CC7 level, CC10 pan, CC71 resonance and CC74 cutoff (the
  engine's own, where it has them), CC72 release, CC73 attack, CC75 decay, CC91 reverb send, CC93 chorus send,
  CC94 delay send; **CC16–23** the engine's own eight values (EDIT 1 / EDIT 2's knobs, E1–E8, whatever the engine:
  CC16 is KNOB 1 of EDIT 1), **CC102 / CC103** the post filter's cutoff / resonance, **CC104** drive, **CC105** sustain
  (0.6.96). CC1 (mod wheel), CC11 (expression) and channel aftertouch are sources in the MOD matrix (MODW, EXPR,
  AT). Pitch bend, sustain (CC64), All Sound Off (CC120) and All Notes Off (CC123) work. A CC is recorded as automation
  like a knob move.
- USB MIDI and audio name: **"JIANT"** (0.9.12; 0.7.0–0.9.11 "JIANT FM"; "JIANT Update" while a package installs).
  In a DAW the MIDI port is "JIANT": Ableton Live and other Windows programs list "JIANT" (a second one "2- JIANT"),
  macOS the device "JIANT" and its port "JIANT", Linux "JIANT MIDI 1". Up to 0.9.11 Windows could keep showing
  "Felucca" (it remembered the first name it saw); from 0.9.12 it reads the name again. If macOS still shows an old
  name, open Audio MIDI Setup > MIDI Studio with the FM-1 unplugged, delete its old entry, and plug it in again. The
  web editor and the installer also accept a device still named "JIANT FM" or "Felucca".

### MIDI LEARN (0.9.12)

Any knob of a track's sound to any CC of your controller, on the device (from Felucca 1.5's MIDI LEARN):

1. **MENU › MIDI › LEARN ON** (hold HOME, ALGORITHM to the MIDI tab, KNOB 2 to the right), then HOME to close the
   menu.
2. **Turn a knob** on any page that edits a track's parameter (HOME, EDIT, ENV, LFO, FX, the MIXER, GLO's levels):
   the screen says **LEARN T2 CUT: SEND A CC** (or its CC if it already has one).
3. **Move the controller's knob** (send the CC): **CC21 > T2 CUT**. From then on that CC sets T2's CUT as the knob
   does (0–127 over its range, recorded as automation like a knob), on **any channel** and over **USB or the TRS jack**.
4. Turn the next knob, send the next CC … **LEARN OFF** when you are done (with LEARN on, knobs keep picking).

- One CC sets one parameter; a parameter has one CC: learning it again replaces the old one. **Ten** at most
  (**LEARN FULL**: clear one by learning its CC to another knob, or everything with LEARN CLEAR).
- **Never learned** (**CC7 RESERVED**, and they keep doing their own): CC0 / 32 (bank select), CC1 (mod wheel), CC6 /
  38 / 96–101 (data entry, RPN), CC11 (expression), CC64 (sustain), CC120–127 (channel modes) and every CC of the
  standard map above (CC5 7 10 16–23 71–75 91 93 94 102–105). So a learned CC and the map never meet.
- **Kept with the settings** (not in a project; a settings backup carries it); **MENU › MIDI › LEARN CLEAR** forgets
  them all. ROUTE still decides which channels are heard (CH1-8 ignores channels 9–16).
- Bluetooth MIDI does not exist on the FM-1 with JIANT (see above), so learning works on USB and TRS.

### Stems (0.7.1)

Two ways to get each track on its own, to post-process them one by one in a DAW:

- **Over USB, live: MENU › USB › USB AUDIO STEMS.** The device re-enumerates as a 10-channel USB audio input at 44.1 kHz,
  16 bit (set the DAW's input to the device, 10 channels): **channels 1–8 are the eight track stems, T1 to T8**, each mono
  with its LEVEL, DIST, SLICER routing and mutes in and its PAN left out (what it adds to the mix, L + R halved);
  **channels 9–10 are the FX return** in stereo (the DELAY, REVERB and CHORUS buses, the SLICER's when it is on). All
  before the master: no leveler, limiter, CLIP or MASTER knob on them. Every track is exported, so the sum of the ten
  channels is the mix of tracks 1–8 as the buses hear it. (Before this change STEMS was 8 channels: T1–T6 and the FX
  return; T7 and T8 were not in it. A DAW set up for the 8 must be set to 10 channels.) USB LEVEL
  at −6 dB or lower gives headroom. The serial console is off in this mode; USB-MIDI, the editor and the installer still
  work. The speaker and the headphones play the master as always.

  **0.9.8: clean stems.** Until 0.9.7 the 8-channel endpoint said 720 bytes (45 frames) while the FM-1 sends 46 frames
  whenever its clock gets ahead (often: it runs a little fast): the host dropped each such packet and paused, the FM-1's
  ring filled and whole blocks of 32 frames were lost, about a hundred times a second, in every channel at once: clicks,
  and a chopped, "separated by AI" sound in each stem (the stereo mix modes were not affected). Now 736 bytes; the ring
  also keeps a millisecond more in hand in STEMS and STEMS+IN, and a frame repeated after an underrun is made up spread
  over a packet instead of skipped. **Update the FM-1, and record again**: a take made
  with 0.9.7 cannot be mended afterwards. **0.9.10:** STEMS+IN (and STEMS, more rarely) still lost blocks, in bursts a
  few times a second in every channel (a rhythmic glitch), because the next USB packet was queued too late after the
  last one left; it is queued in time now. Takes recorded before cannot be repaired: record them again.
  A track through the **SLICER** now has its own sound as the SLICER let it
  through in its stem (its repeats in the first such track's stem, or with the SLICER's FX ON in the FX return), so the
  eight still add up to the mix of tracks 1–6 (until 0.9.7 that track was in its stem dry and again, sliced, in the FX return).
  What a stem is: exactly what the track adds to the mix in that take, so it nulls against the track rendered alone
  in the same take. The things that make one track depend on the others are in it, as in the mix: **DUCK** (the kick's
  pump on the other tracks), and the **shared voices** (8 among all tracks; **LOAD ECO**: 6, 4 a track, and with the
  load budget tight the tails already under −50 dB are trimmed): a note another track's note took the voice from ends
  early (a 6 ms fade). In the mix that is masked; soloed, a dense song can sound gated. For stems, record at **LOAD
  FULL** when the CPU allows it, and keep the song's polyphony within the budget.

### USB audio in (0.7.5, test version)

**MENU › USB › USB AUDIO IN+OUT** makes the FM-1 a USB audio output of the computer or the phone as well as an input:
the device re-enumerates (as with STEMS, the serial console is off), and the host now lists "JIANT" as a speaker,
stereo, 16 bit, 44.1 kHz. Whatever the host plays to it comes into the **last track** (T6, or T4 with TRACKS 4) and is
mixed like that track's sound:

- its **LEVEL** sets the input's level, its **PAN** works as a balance, its **CHORUS, DELAY and REVERB** sends take the
  input into the effects (from the left + right sum), **DUCK** ducks it, its **MUTE** and the mute layer silence it;
- the track's own engine still plays (an empty pattern leaves the input alone); its DIST and filter are not on the input;
- the recording side is the stereo master as with STEREO, with the input in it.

Choose the FM-1 as the output in the host's sound settings (or in the DAW / the app), set 44.1 kHz, and play. The
FM-1 follows the host's clock (it reads the audio a little faster or slower, ~3.6 ms behind); if the host stops
sending, the input fades out and comes back when it resumes. Some hosts make a newly connected USB audio device the
default output: that is why IN+OUT is a choice of its own, not on by default. Test version: tell us how it behaves
with your computer or phone.

### Stems with a voice (0.8.0, test version)

**MENU › USB › USB AUDIO STEMS+IN** is STEMS and IN+OUT together, for the phone app: the phone's microphone goes out to
the FM-1, comes into the **last track** as IN+OUT's input does (its LEVEL, PAN, MUTE and its CHORUS, DELAY and REVERB
sends: your voice through that track's effects), and **10 channels** come back: **T1–T8** (the eight track stems, as in STEMS), then **the
voice as the track mixed it** and **the voice as it came**. The voice has a channel of its own (not in the track's
stem), so the app can move it earlier by the round trip's latency and the voice lands in time with what you heard;
the second voice channel is the reference the app measures that latency against. STEMS+IN has no FX return: the
buses' reverb and delays are in the tracks' stems (as they are in the mix), so the voice's own reverb and delays
stay with the voice in its track's stem, and they land the latency late (~20–30 ms: in a reverb it sounds like pre-delay). The phone app reads and writes the FM-1 with its own USB driver (Android passes
only some of a USB device's channels to apps). Test version: the app is JIANT Probe for now (its test 7).

---

## 15. Web tools: emulator, installer, editor, backup

All three run in the browser and update themselves with every change to the firmware.

### Emulator — <https://juanjiant-bit.github.io/JIANT/emu/>

The same code as the FM-1, with sound, in the browser. Play it with the mouse or the computer keyboard: **Z–'** and
**Q–O** rows are the keys, **1** to **=** the buttons from FX to REC, **[ ]** OCT− / OCT+, **↑ ↓** SELECT, **← →**
ALGORITHM; **Shift-click** holds a button. Great for learning the layers without the device.

### Installer — <https://juanjiant-bit.github.io/JIANT/webapp/installer/>

Chrome or Edge, the FM-1 connected straight to the computer.

- **Install** JIANT (it checks the FM-1 first: official V15 or JIANT only).
- **Return to official V15**: give it M-VAVE's unmodified **FM-1.fwsc** (checked locally in the browser). It first
  saves a complete backup (current music, saved music, sound banks and settings; not user samples). Use this before
  installing JIANT if your FM-1 runs **another custom firmware** (Felucca, baudgirl…), and whenever you want to go back
  to stock. Official V15 cannot open JIANT music; to restore the backup later, reinstall JIANT and use the editor.

### Editor — <https://juanjiant-bit.github.io/JIANT/webapp/editor/>

Every parameter from the computer, over USB. Tabs: Sound, 6-OP FM, Sequencer, Tracks, Library, Projects, Settings.

- **6-OP FM**: edit FM6 patches, import .syx, and **Send to the device's voice bank (B1–B32)**.
- **Sound**, on a VOICE track (0.8.5): the **Lyric** box: LANG, LYRIC and the lyric's text, **Send** writes it to the
  track (accents and ñ converted).
- **Projects**: the project slots, and the **Complete backup**: **Save backup** and **Restore backup**.

### The complete backup

**Save backup** downloads one file with: the current music, **all 8 projects with their 4 sections each**, the
arrangements and scenes, the 32 user presets (with their FM6 patches) and the settings. **Restore backup** replaces
all of that on the device (keep USB connected). The **FM6 voice bank (B1–B32) is not included**: keep your .syx files.

Back up before updating, before returning to stock, and whenever you finish something you love.

---

## 16. Troubleshooting and FAQ

### The installer does not install: what to do

Installer: <https://juanjiant-bit.github.io/JIANT/webapp/installer/>. When something fails it shows a **What to do**
box with the steps for that error; if that is not enough, press **COPY REPORT** and paste it into your message (the
browser, the MIDI ports it saw, the FM-1's firmware, the step it stopped at).

- **"This browser has no WebMIDI"**: Chrome or Edge on a computer. **iPhone and iPad cannot** (no browser there has
  WebMIDI); Safari has none; Android Chrome with an OTG cable may work, a computer is safer.
- **MIDI not allowed / blocked**: the icon left of the address → Site settings → MIDI devices → Allow; reload.
  Brave: Shields off for the site.
- **FM-1 not found**: switch it on; a USB **data** cable (many only charge); straight into the computer, no hub. Then
  try **Is it in boot mode (WL80UBOOT)?**.
- **Another program is using its MIDI port** (Windows: one program per port): close the M-VAVE app, the DAW, MIDI-OX
  and other tabs with the JIANT editor; replug, reload, press Install.
- **An older official firmware (V14, V13…)**: Return to official V15 (no backup needed), then Install.
- **Felucca, Skiff or another custom firmware**: Return to official V15 first. It saves that firmware's backup first
  (Felucca's with its samples; Skiff's in its own format, for Skiff's editor) or, when it cannot make one, asks before
  going on without; then Install.
- **Where is FM-1.fwsc?** M-VAVE's download page (<https://www.m-vave.com/download>): FM-1, firmware V15, unzip it, the
  file FM-1.fwsc (699,956 bytes). We cannot host it: it is M-VAVE's. Picking the ZIP, another version or a JIANT
  package is told apart.
- **Stopped half way** (update mode not found, stopped, connection lost): do not switch it off; replug and press the
  same button: it resumes. Windows may take a minute to install the update mode's driver the first time.
- **Black screen**: connect it and press Install again; **WL80UBOOT** is the chip's boot mode (another cable, straight
  in; else [FM-1 Transporter](https://github.com/kurogedelic/FM-1-transporter)).
- **Page not found**: use the address above exactly; old links (`/JIANT/installer/`, `/JIANT/webapp/`, a link with
  `:1:1` stuck to it) now redirect by themselves.

### The firmware

**T5 or T6 is silent.** TRACKS is set to 4 (the default): T5 and T6 are asleep. GLO > SYSTEM > TRACKS **6** (or
MENU > AUDIO > TRACKS). The screen tells you when you play into an asleep track.

**After clearing a sequence, notes still played on all tracks.** This was fixed in v0.6.28: a clear now ends the
punch-in lane's repeats (STUTTER, ARP), a DRUM track with no steps gets no ARP fill, and a section jump or quick chain
waiting for the bar is called off. If something still plays after a clear, it may be the **punch-in lane**: hold
**REC and press FX** to erase it (or **SEQ + REC** to clear everything).

**Two cores (0.6.71) and MENU › SYSTEM › CPU CORES (0.9.12).** The second core renders some synth tracks; MENU › SYSTEM › ABOUT
says **CORES 2**, or **CORES 1** and why. Since 0.9.12 **MENU › SYSTEM › CPU CORES** chooses **2 CORES** (the default) or
**1 CORE**, kept with the settings, from the next start. If the FM-1 crashes or hangs while both cores work, or the
second core misreads, stops answering or faults, it **turns 2 CORES off for good** and says so (**2 CORES OFF AFTER A
CRASH** / **CORE 2 FAILED: 1 CORE**, then MENU › SYSTEM › CORES); ABOUT then reads **CORES 1 (AFTER A CRASH)**. Set
2 CORES again only if you want to test it. Hold **REC** while powering on for one core that session only.
With both cores running the tracks share **12 voices** instead of 8 (each track still at most 8); on one core (1 CORE
set, after a crash, REC held) it is 8, and a second core that stops brings the budget back to 8 from the next note.

**It crashes: a red screen "JIANT CRASH" (or "FELUCCA CRASH" before 0.7.0) with numbers.** Take a photo of it and
tell us the version (MENU › SYSTEM › ABOUT). How to read it:

- 0.9.12 on: the version, **CORE 0 OF 2** (the core that faulted, of how many were running), **CODE xx-nn** (the cause
  and the vector, below), then **VEC, PC, EMU, DBG, RETS**.
- Before 0.9.12 the same five numbers came unlabelled, in that order, top to bottom: VEC (1 = the CPU exception), PC
  (where it failed), EMU and DBG (the cause bits), RETS (who called the function that failed).
- The code: **IL** illegal instruction, **MA** misaligned access, **DZ** divide by zero, **ST** stack overflow, **PC**
  a jump outside the code, **WR** a write into a protected area, **MM** the memory unit, **BU** an access to no memory
  (a bad pointer), **WD** a hang (the watchdog reset it: no red screen), **??** none of those.
- The FM-1 restarts by itself after 4 s. The last crash stays in the settings, also after switching off: **MENU >
  ABOUT > LAST CRASH** shows it as one line (`IL01 2C 0.9.12 0201ABCD 0201ABCD`: code and vector, cores, version, PC,
  RETS). The editor's settings backup carries it too.
- For developers: `tools/crash_decode.py VEC PC EMU DBG RETS` names the function, against the ELF of the version that
  crashed.

**What to try when it crashes now and then (even idle).** In this order, a few hours each:
1. **MENU › SYSTEM › CPU CORES 1 CORE** (0.9.12 does it by itself after the first crash with both cores; before 0.9.12,
   hold REC at every power-on).
2. **MENU › SYSTEM › CPU CLOCK 320 MHZ**: the chip at JieLi's rated clock instead of the 360 MHz the FM-1's boot
   sets (switched at the next power-on; ABOUT shows **CPU 320 MHZ**). About 11 % less CPU: a heavy song may shed
   voices sooner (LOAD LITE helps). If the FM-1 refuses (its clock is set up differently), ABOUT keeps **CPU 360 MHZ**.
3. If it still crashes: the photo, the version and LAST CRASH, please.

**Clicks, stutters, the load meter full.** Keep TRACKS on 4, try **LOAD LITE** or **ECO**, use fewer voices (MONO,
fewer FM6 tracks), lighter reverbs (CLOUD and SHIMMER are the heaviest). Check MENU › SYSTEM › ABOUT › AUDIO LOAD.

**SELECT doesn't change the tempo.** Since 0.6.67 SELECT alone scrolls the pages; the tempo is **GLO held + SELECT**
(or GLO's F4 TAP, or GLO > GLOBAL's BPM knob).

**The knobs control M1–M4 instead of the page.** The macros are latched (header [LFO] LATCH). Press LFO.

**A song won't start: "PATTERN A EMPTY" / "ADD A SONG ROW".** A row points to an empty section, or there are no rows.
Save music into that section, or change the row.

**It says STOP TO EDIT.** A song (arrangement) or quick chain is playing; stop it to edit steps, use tools or UNDO.

**The keys feel inverted / a button does the wrong thing.** Run HARDWARE CALIBRATION: hold OCT− and OCT+ while powering
on (or MENU › SYSTEM › CONTROLS › CALIBRATION, KNOB 4 then OCT+) and teach each button and knob. A piano key cancels.

**USB audio input doesn't appear on macOS.** MENU › USB › USB AUDIO **STEREO** (or STEMS).

**A new USB mode doesn't show up on the phone or computer.** Since 0.9.10 the FM-1 leaves the bus for half a second
when MENU › USB › USB AUDIO changes (it came back after ~25 ms, and some phones and hubs did not notice it had gone, so
they kept the old mode); it appears about a second after the menu closes. Older firmware: switch again, or unplug.

**SAVED (NO FLASH).** The flash chip wasn't detected; saves stay in RAM only. Power-cycle the device. If it persists,
back up what you can with the editor and reinstall.

**The FM-1 stays black after an interrupted update.** Check whether the computer sees it as **WL80UBOOT** (a USB device
4C4A:8057): that's the chip's boot mode and the installer can write it again (try another cable). To enter update mode
deliberately from a running device, hold **OCT− + OCT+** for 5 seconds. If that isn't enough,
[FM-1 Transporter](https://github.com/kurogedelic/FM-1-transporter) reads and writes the flash with a Seeed XIAO RP2040.

**I want the factory firmware back.** Use the installer's **Return to official V15** section (it backs up first), or
M-VAVE's own updater.

**I came from baudgirl / Felucca. Can I install directly?** No: return to official V15 first with the installer, then
install JIANT. Going straight from another custom firmware has bricked an FM-1.

**Where did my Felucca samples go?** JIANT has no user samples; that flash now holds the songs.

---

If you resonate with my work, consider supporting me on Ko-fi (https://ko-fi.com/juanjiant) — in Argentina: Cafecito (https://cafecito.app/juanjiant).
