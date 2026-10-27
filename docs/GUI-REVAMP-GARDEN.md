# GARDEN: GUI revamp proposal (stems-10-fixed)

Goal: a complete visual change of the FM-1 screen (240 × 240) with **every control, page, function and
value kept as it is**. Look and motion change; input, parameters, storage and audio do not.

Vibe: a well-animated fuzzy garden or forest. Each engine is its own creature or plant, as they already
are. Colours: off-white beige as the base, with pastel blue, pastel orange and very dark brown as the
secondary colours.

Basis: `firmware/src/params.c` (PAGES, the families), `firmware/src/gfx.c` and `tools/gen_ui_palettes.py`
(palettes), `docs/MANUAL.md` (controls and skins), `docs/STEMS-10-BUILD.md` (memory and CPU limits).

---

## 0. Constraints the design is built around

| Constraint | Why it matters | Rule for the design |
| --- | --- | --- |
| **Flash is full**: 248 B free in the app slot | New art costs flash. The UI shares the image with the sound code. | Draw with primitives, not bitmaps. The palette and the new drawing code must pay for themselves: retire an old skin or ramp to free the bytes (see §9). |
| **RAM pool**: 15.6 KB spare | Sprites and frame buffers would take it. | No sprite sheets. Dither patterns come from a 4 × 4 Bayer table (16 bytes). |
| **CPU is shared with audio**: stems-10 is at the edge (glitch test) | Animation costs cycles on the core that also renders. | The garden animates in redrawn regions only, capped at about 15 fps. Dirty rectangles only. Nothing redraws when a page is idle. |
| **Controls are fixed**: 4 KNOB cards, SELECT / ALGORITHM / PRESETS encoders, the button rows, 27 keys (F3–G5), the layer buttons, the MENU | The user has muscle memory. | The GUI reads the same state and draws it differently. No change to `ui_input.c`, `ui_events.c` or the navigation rules. |
| **Existing lens system**: `PAGES[]` families and `ui.frame` signatures | Pages already redraw when their signature changes. | Hook the garden into that same path. A "live" page adds `ui.frame` to its signature, as today. |

---

## 1. Palette

Add a palette `GARDEN` to `tools/gen_ui_palettes.py` (the five slots are BG, SURF, TEXT, THEME, ACCENT). It
becomes the default, like JIANT is today.

| Slot | Role | Colour | Hex |
| --- | --- | --- | --- |
| BG | ground: off-white beige | warm off-white | `#F3ECDF` |
| SURF | raised beige: cards, the menu, mixer strips | sand | `#E6DAC4` |
| TEXT | very dark brown: labels, values' ink | espresso | `#2B1E16` |
| THEME | identity: values, curves, gauges, selection | **deep pastel blue** | `#3F6FA8` |
| ACCENT | the one active thing: cursor, playhead, sounding step | **pastel orange** | `#F5A66B` |

Derived by the firmware (no change needed): MID, DIM, LINE, SEL, TINT, RAISE, KEY.

Two colours that are not in the palette slots, used as fills by the drawing code:
- **pastel blue `#A9CBEA`**: the soft fill of a bed, a pond, a sky (THEME's tint, deliberately light).
- **burnt orange `#C8602A`**: an orange that carries text (ACCENT is too pale for small text on beige).

Why the blue is split: pastel blue on beige is about 1.7:1 contrast, too weak for numbers. The deep blue
`#3F6FA8` is about 4.3:1, which works for values. Pastel blue is kept for fills and areas. Pastel orange
on beige is also too weak for text, so the orange is a **fill with dark brown ink**: about 8.6:1.

Check: run `tools/gen_ui_palettes.py --report` before anything else. Every pairing in its CHECKS table must
pass its minimum.

GREY and MONO stay as they are (the fallbacks for the same pages).

---

## 2. Shared chrome (on every page)

| Element | Today | GARDEN |
| --- | --- | --- |
| Background | solid colour | a **meadow ground**: the lower third of the screen is grass, the upper part is sky. Built from a dithered horizon line. |
| Tabs (group words) | text tabs | **stepping stones** along a path. The active stone is orange, the others beige with a dot under each page. |
| KNOB cards (1–4) | boxes | four **seed pods** hanging from a branch. Each has its name small on top and its value under it, in deep blue. Turning a knob makes the pod swell and the seeds shed. The knob you turn is outlined in orange. |
| Breadcrumb | text | a **trail of footprints**; the current one is filled. |
| Header / track | number and engine name | the track's **flower** (T1–T6), its colour = the track's engine creature colour (§3). The engine name sits under it. |
| Layer held (MACRO, FX, SCL, …) | layer's own knobs | the page **blooms**: the meadow gets a soft pastel blue overlay for the layer, and the layer's own knob pods replace the normal ones. |

**Fuzz**: every surface edge uses a 4 × 4 Bayer dither at 1 px (a soft, furry edge). Surfaces are never
hard-edged. This gives the "fuzzy" look without bitmaps and costs only the dither table.

**Motion system** (one global clock, `ui.frame`):
- **Wind** drives everything that sways: grass, pods, leaves. Sway amplitude is a sine table entry (the
  table exists in `dsp.c`). The wind's strength is the sum of the track levels (louder = windier).
- **Breathing** of living things: small ±1 px pulse at a slow rate.
- **Pollen**: at most 16 pixels drifting over the screen. They spawn when a voice starts, so the picture
  shows the playing.
- The frame cap is 15 fps on the live pages. Pages that are not moving do not redraw.

---

## 3. Engine creatures (identity)

Each engine has one creature. It is the header flower on its track, the main picture on its EDIT pages, and
the colour of its track. The colours come from the palette (no new colours). The existing creatures in
`ui_being.c` and `ui_goobers.c` become these garden creatures; their shapes are redrawn, their logic kept.

| Engine | Creature | Moves with |
| --- | --- | --- |
| ANALOG | dandelion clock | seeds leave on each note; the ring of seeds = oscillator 2 |
| DIGITAL (FM4) / FM6 | fern with operator fronds | frond length = the operator level |
| PHASE | hummingbird on a stem | wings beat at the note's rate |
| LOFI | pixel-art mushroom (4-bit steps) | the cap steps through the chip levels |
| VOICE | songbird | beak opens with the formant; lyric words float up |
| WHEEL | sunflower (drawbars = petals) | petals open per drawbar value |
| NOISE | cloud of gnats | density = the noise level |
| DRUM (DRUM-X) | mushroom ring, 8 caps (BD SD CP CH OH TM RS CB) | each cap lifts on its hit |
| ANIMAL | frog on a log | jump on the call's onset |
| SERUM | serum with 8 branches (wavetable slots) | a branch per edit slot |
| CMOS | snail | the shell turns with the clock |
| PULSE | cricket | the chirp rate = the pulse rate |
| WHSTL | whistling kettle-plant | the steam = the pressure |
| WING | moth | wings flap at the wingbeat rate |
| GLOTT | toad (throat sac) | the sac inflates with the pulse |

Track order keeps the flower colours: T1 orange, T2 pastel blue, T3 burnt orange, T4 deep blue, T5 dark
brown, T6 sand. The same colour rule works in MONO (outline only).

---

## 4. Pages, one family at a time

Page names are the firmware's `PAGES[]` names. A page with the same name as another (FILTER, OPERATOR) is
drawn by its engine.

### 4.1 HOME (and ECO / BEING / GEN)

| Page | Scene | Kept | Motion |
| --- | --- | --- | --- |
| HOME | the whole meadow: four track flowers (T1–T4) with the engine creature beside each | tempo, song, section, 4 knob pods | each flower breathes with its track's voices |
| ECO | **standby**: the six tracks' creatures stroll the meadow on their own paths (mushroom, fern, songbird, snail, moth, frog), bobbing to the music; pollen drifts on each beat | the wander | live at all times; no button press needed (see `docs/gui-garden-mockup.html`) |
| BEING | one track's creature, large, its four knobs as organs at the corners | the four organ values | the creature's own motion (§3) |
| GEN | a path of six stones for the genre | the genre list | the stone lit = the genre |

### 4.2 ENV family

| Page | Scene | Kept | Motion |
| --- | --- | --- | --- |
| ENV | **a plant growing**: ATK = the stem rising, DEC = leaves unfolding, SUS = the flat plateau of leaves, REL = leaves falling | ATK DEC SUS REL | the growth follows the envelope, as the note plays |
| ENV DEST | four arrows from a seed to four targets (FLT, PIT, SHP, LOOP) | the four destinations | arrows pulse at the envelope's level |
| STAGE | the plant's detail: a stem with four nodes | STAGE, its ATK DEC SUS REL, LOOP | the active node glows |
| ENV › TRACKS / TRACKS DEST | six small plants in a row | each track's level | the plant height = the level; the selected one in orange |

### 4.3 LFO family

| Page | Scene | Kept | Motion |
| --- | --- | --- | --- |
| LFO | **wind over a grass field**: the wave shape sets the gust (SIN smooth, SAW sudden, SQR two-level, S&H steps, TRI triangle) | RATE, WAVE, MODE, FADE | the grass bends on the wave in real time |
| LFO DEST | a seed with four petals, each a target (PIT, FLT, SHP, AMP) | the four destinations | a petal's size = its depth |
| MOD | a **constellation of fireflies**: each route is a firefly moving from its source to its target | SRC, DST, AMT per slot | the firefly's brightness = the amount |
| MSEQ / MSEQ 2 | a row of 16 seed pods, a sprout per step | STEP, LEN, SLEW, DIV | the pod opens on the step; the sprout grows with the value |
| LFO 2 | a clock-dial in the grass (phase = where the sun is) | PHASE | the sun's arc |

### 4.4 FX family

The FX chain is a **stream** from the track (top) through the stones of each effect to the bus. Each effect
has its own creature, which is also how the user reads its state.

| Page | Scene | Kept | Motion |
| --- | --- | --- | --- |
| FX | a stream with four stones (DIST, CHOR, DLY, REV) | the four sends | the water's flow = the send level |
| MASTER | a **dam** with a gauge: clipper, drum bus, ducking | CLIP, PUNCH, DUCK, D.REL | the water level = the master's gain |
| DIST | a **thorned rose** (its thorns grow with the drive) | TYPE, DRIVE, TONE | thorns flicker on hit |
| AUTO PAN | a bird flying left and right | DEPTH, RATE, SYNC, WAVE | the bird's flight follows the pan |
| BUSES | six garden beds, each a bus | DRY MIX, ROOM SIZE, CHORUS DEPTH, SLICER DEPTH | each bed's fill = the bus level |
| DLY / DLY 2 / DLY 3 | an **echo in a hollow**: echoes are ripples on a pond that travel out and fade | TIME, FDBK, COLOR, MIX; WIDTH, PITCH, SPRY, MOD; TYPE (ANALOG / DIGITAL), PING | ripples at the delay time; the spread = the width |
| REVERB / REVERB 2 | **fog in a forest clearing**: the fog's size = the room, its colour = the tone | TYPE, SIZE, DAMP, PRE; MOD, RATE, FILT, WIDE | the fog drifts at the rate |
| CHORUS | fireflies in a choir | RATE, DEPTH, STEREO | fireflies blink at the rate |
| SLICER | a **bird that stutters**: the gate or stutter the wing's rate | MODE, PATTERN, RATE, DEPTH | the wing flaps on each slice |
| SL BUS / SL BUS 2 | a pond with four inlets (the slicer's inputs) | the six inputs, SLICER FX | the inlets fill with each track |
| SL PITCH | a row of stones with a pitch on each (the staircase of the slicer) | the step, LEN, PITCH SET (CHR / SCL) | the stone lit = the step |
| PUNCH | a **mushroom ring**, the punch-in lanes | LEN, RES, REC, CLEAR | the cap lifts on each punch |

### 4.5 SCL family

| Page | Scene | Kept | Motion |
| --- | --- | --- | --- |
| SCL | a **ring of 12 stones**, the scale's notes lit, the root in orange | ROOT, SCALE, QUANT, ALL TRACKS | lit stones follow the scale; the root pulses |
| CHORD | three flowers in a bunch (the chord) | CHORD, VOICING, STRUM | the flowers open one by one with the strum |

### 4.6 EDIT family (the engine's own page: the creature gets its full stage)

| Page | Scene | Kept | Motion |
| --- | --- | --- | --- |
| EDIT 1 / EDIT 2 | the engine's creature (§3), large | the eight engine values E0–E7 | creature motion, §3 |
| FILTER | a **leaf that filters light**: the cut is the leaf's opening | TYPE, CUT, RES, DRIVE | the shade moves with the cutoff |
| MORPH | a **flower changing**: the petals morph from wave A to B | MORPH, DRIFT | the petals slide |
| WORDS | a bird's song as words on a branch | LANG, LYRIC, POS | words float up on each syllable |
| RETRIG | a stone that knocks | RETRIG | knocks on the trigger |
| ROTOR | a **windmill** (WHEEL's rotor) | DEPTH, DUST | the blades turn with the rotor |
| KIT 3 / STEREO / X-MOD | the drum kit meadow: its sounds as flowers in a row, and a **panning wind** | IMAGE, the LFO on MORPH and RAND | the wind moves the flowers left and right |
| SOUND / SOUND 2 / SOUND 3 / SOUND 4 | one kit sound: the cap's flower (its colour = the lane) | the sound's values | the flower opens on its hit |
| LEVELS / LEVELS 2 | eight mushroom caps of different heights (the sounds' levels) | LN0–LN7 | each cap's height = its level |
| OPERATOR / OP ENV / OPERATOR 2 | **the FM6 fern**: each frond is an operator, the fern's shape is the algorithm | the operator's values | the operator's frond sways with its level |
| OP1–OP4 ENV | four plants, one per operator: the same growth as ENV | ATK DEC SUS REL | the plant grows with the envelope |
| OP LEVEL | four fronds side by side, each its level | LEVEL 1–4 | frond heights = the levels |
| VOICE / VOICE 2 | a **choir of birds** (POLY = many, MONO = one, LEG = a flying line, UNI = a flock) | VOICE, GLIDE, GLIDE MODE, PRIO; ALLOC, DETUNE | the flock's shape = the voice mode |
| FOCUS | a **magnifier on a leaf**: the one parameter the user is in | the focused parameter | the magnifier follows the focus |

### 4.7 GLO family

| Page | Scene | Kept | Motion |
| --- | --- | --- | --- |
| GLOBAL | a **sundial**: the sun's arc is the tempo; the arc's height = the swing | BPM, SWING, CLOCK, TUNE | the sun moves at the tempo |
| SYSTEM | a garden shed (the settings: tracks, load, route, info) | TRACKS, LOAD, ROUTE, INFO | the shed's door opens on a change |
| MANO (GLO > LIVE) | a board of keys as flowers that modify the six sequences | the live keys | flowers open on press |
| RIELES | six **rails** of vine, one per sequence | the six sequences | vines grow with each sequence |
| MIXER (GLO > MIX, the TRK family) | six garden beds, each a track: the bed's fill = its level, the pan is a little hill | LEVEL, PAN, REV, DJ FILTER | beds fill with the meter |
| SENDS | the same six beds: each bed's four sends as four stems | DIST, CHOR, DLY, REV per track | stems grow with the send |

### 4.8 SAVE family (projects, presets, arrangement)

| Page | Scene | Kept | Motion |
| --- | --- | --- | --- |
| PRESETS | a **seed catalogue**: rows of seed packets, one per factory preset | the browse knob, KNOB 1 | the packet lifts on the choice |
| USER | seed packets in your own colour (the user's presets) | SLOT, LOAD, ERASE, SAVE | the packet opens on load |
| PROJECT | 8 **seed packets** (songs) × 4 sections (A–D) as small plants | PROJECT 1–8, SECTION A–D, LOAD, SAVE | the plant in the chosen section grows on save |
| ARRANGE | a **path of stones**, one stone per song row | the song's rows | the stone the song is on is lit |
| TOOLS | a toolbox in the shed: CLEAR SEQ, INIT SOUND, CPU TEST | the tool actions | the tool's icon on action |

### 4.9 ARP family

| Page | Scene | Kept | Motion |
| --- | --- | --- | --- |
| ARP | a **pinwheel**: the arp's rate spins it, the mode is the wheel's pattern | MODE, RATE, OCT, GATE | spin = the arp's rate |
| ARP 2 | a **wind chime** (swing, probability, hold, order) | SWING, PROB, HOLD, ORDER | the chimes ring on each note |

### 4.10 SEQ family (the step grid)

The SEQ pages are where the garden becomes a **garden bed**: each step is a plot.

| Page | Scene | Kept | Motion |
| --- | --- | --- | --- |
| STEP | a plot with the step's notes as a small flower | step edits (KNOB 1–4 with the step held) | the flower opens when the step plays |
| PATTERN | the bed: 64 plots, the first 16 shown in a row | LEN, DIV, SWING, GATE | the playhead moves along the row |
| NOTE | a **piano roll as vines**: the note's pitch is the vine's height | FOCUS, PITCH, MOVE, MICRO | the vine grows on the note |
| STRETCH | the bed's plots stretched like taffy | STRETCH, REPEAT, NUDGE, ROTATE | plots spread as the knob turns |
| COMP / COMP 2 | a plot with two stems (the two components) | STEP TYPE, the two values; EVERY | stems move on the type |
| PLAYHEADS | three extra **bees** moving over the bed | the three playheads | bees fly at their own rates |
| TRACK | the track's bed: pitch, chance, human, rec quantise | PITCH, CHANCE, HUMAN, REC Q | the bed's colour = the chance |
| PHRASES | a **seed tray** of patterns (hidden page, as today) | the pattern loader | seeds lift on pick |
| SONG | the path of stones (as ARRANGE, with the play row) | the song's rows and sections | the stone currently playing is orange |
| CHANCE | dice on a leaf (hidden page) | CHANCE, RATCH | the dice roll on the step |
| AUTOMATION | a **vine on a trellis** that climbs with the locks | motion lanes | the vine grows with each lock |
| AUTO LIST | the same vine as a list of leaves | the locks and events | leaves tick over |
| MUTATE | a **seed mutating in a pot** | MSND, MPAT, MAMT, MMODE | the seed's colour shifts on mutate |

---

## 5. Overlays and special screens

| Screen | Scene |
| --- | --- |
| MENU | a **garden map** with the tabs as paths; its rows on the beige sand (SURF) |
| HARDWARE CALIBRATION | a sprout being measured; the key-by-key check as a row of stones |
| CRASH | keep the firmware's fixed red; a broken flower (the one place the garden stops) |
| Boot / power-on | a seed in the soil that sprouts into the HOME meadow |
| QR / sharing | a seed packet with the QR code (fixed black and white, as today) |

---

## 6. Controls: nothing moves

| Control | Keeps its job | Visual change only |
| --- | --- | --- |
| KNOB 1–4 | the four cards, the lens | pods (§2) |
| SELECT (encoder) | walks the section's pages | a footprint walks along the trail |
| ALGORITHM (encoder) | selects the track | the track's flower is selected |
| PRESETS (encoder) | presets / patterns / genre | the seed packets |
| HOME, LFO, ENV, SCL, ARP, SAVE, SEQ, EDIT, FX, GLO, LAYER, PLAY, OCT− / OCT+ | the same behaviour | a button label is a small garden icon (sun, leaf, wave, sprout, seed, bee, …); the icon is a 16 px primitive |
| 27 keys F3–G5 | play the track | keycaps are pebbles: white pebbles (white keys), dark pebbles (black keys), a ripple on press |

---

## 7. Motion rules (so the CPU holds)

1. A page draws only when its signature changes, as today. Live pages add `ui.frame` to it.
2. The garden redraws only its **dirty rectangle** (the sprout, the pod, the flower that moved), not the whole
   screen. The meadow ground is drawn once per page change and kept.
3. Frame cap 15 fps; a page with no motion and no change does not redraw.
4. Wind and breathing use the sine table in `dsp.c`. No float maths in the draw loop.
5. The pollen is at most 16 points.
6. The **ECO** setting (the existing low-load mode) drops the pollen and the breathing, keeping the wind.

Measure with `tools/glitch_watch.py` on the CPU TEST (the procedure in `docs/STEMS-10-BUILD.md`). Acceptance:
`lost` stays at 0, and the slowest render keeps the margin it has today.

---

## 8. Phases

| Phase | Scope | Gate |
| --- | --- | --- |
| 0 | `GARDEN` palette in `gen_ui_palettes.py`; the meadow ground, pods and the fuzz edge in the shared chrome | `--report` contrast passes; `tools/build.py` passes |
| 1 | HOME, BEING, ENV, LFO, FX pages (§4.1–4.4) | flash delta measured with `tools/size_fns.py`; glitch test passes |
| 2 | EDIT pages and the 15 engine creatures (§3, §4.6) | the creatures replace `ui_being.c` / `ui_goobers.c` drawings; their logic unchanged |
| 3 | SEQ, SAVE, GLO, ARP, SCL (§4.5, 4.7–4.10) | the SEQ grid with the playhead; the save and load dialogs unchanged in behaviour |
| 4 | Motion polish, the overlays (§5), the icons (§6) | CPU TEST clean on the 8-track build |

---

## 9. Size budget

Flash has 248 B free, so the revamp is paid for by removing code:

- Retire the **COLORS** skin (a legacy skin: its layout and 0.6 colours) and the heat ramp `heat_col` once GARDEN
  covers their pages. Candidate saving: measure first with `tools/size_fns.py`.
- The engine creatures replace the old drawings one for one. Keep the drawing count, not the sizes, in step.
- The Bayer table (16 B), the pollen (16 points) and the icons (16 px each, a few hundred bytes) are the new cost.

If a phase grows the flash, it does not merge until the removal is done.

---

## 10. Decisions (revision 2)

1. **GARDEN replaces every skin** (AIR, COLORS, HUD). It is the only look. The skin menu goes.
2. **COLORS is retired**, and so are its layout and the 0.6 colour set. Its code and the heat ramp `heat_col`
   come out with it, which pays for GARDEN's art. Measure the saving with `tools/size_fns.py` in Phase 1.
3. **The fuzz is dynamic and follows the music.** The rim's dither density, the blob jitter and the pollen count
   come from the output level of the track (the same envelope the meters already read). The rim is redrawn only
   where it changes. Still within the 15 fps cap and the dirty-rectangle rule (§7).
4. **ECO is a live standby**: the six tracks' creatures stroll the meadow (§4.1). They keep moving while the
   music plays and wander slowly when it is quiet. No buttons are needed to keep it alive.
5. **Fonts wait** until everything else is built. The fonts stay as they are until then.

Nothing in this document changes parameters, storage, the MIDI map, the audio code, or the controls.

Mockup: `docs/gui-garden-mockup.html` (nine screens at 240 × 240, the ECO standby, and a simulated music
signal: the level slider and tempo slider drive the fuzz and the wind).
