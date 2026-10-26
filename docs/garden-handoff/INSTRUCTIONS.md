# GARDEN UI: implementation instructions for Claude Code

This is the complete brief for replacing the FM-1 firmware's current UI with the **GARDEN** look. The look was
designed and approved over a long review with the owner. Every decision in it is recorded here (§3). Read this
whole file before changing anything, then work phase by phase (§9). Each phase ends with a build, the host tests,
and a short report to the owner.

Repository: `JIANT` (FM-1 firmware), branch **`stems-10-fixed`**. Everything here is relative to the repo root.

## 0. What is in this folder

| Path | What |
| --- | --- |
| `INSTRUCTIONS.md` | this brief (the spec) |
| `PROMPT.md` | the short prompt that starts a session on this brief |
| `mockups/pages.html` | the page screens: HOME, ECO (click it: the X-Y view), ENV, LFO, FX, DRUM-X, FM6, SEQ, MIXER |
| `mockups/creatures.html` | the 11 engine creatures |
| `mockups/menus.html` | the BEING page, the ECO X-Y view, the MENU overlay |
| `images/pages/`, `images/creatures/`, `images/menus/` | each screen at 480 x 480 (the 240 x 240 screen, 2x, nearest neighbour): `_a_music` and `_b_music` two moments with music at 70 %, `_c_silent` music at 0 |
| `images/sheets/` | the same frames as whole sheets |
| `refs/` | the owner's reference images (third-party, not committed; see §13) |

The mockups are **the source of truth for the look and the motion**: their drawing code is short and commented.
Open them in a browser (they animate; the sliders set the music level and tempo). For a fixed frame:
`mockups/creatures.html?shot&t=2.5&lvl=70` draws one frame in a grid of 240 px tiles. They are JavaScript
canvas drawings with floating point, not firmware: port the *result*, in the firmware's own style (§8).

## 1. The firmware in brief

- **Screen:** 240 x 240 ST7789 on SPI, RGB565. **No full framebuffer:** `gfx.c` draws into off-screen strips and
  blits each with one DMA transfer. Nothing can be read back after drawing, so a "draw then pixelate" pass (the
  mockups do that) is impossible: draw blocky shapes directly on a 2 px or 3 px grid.
- **One translation unit:** `firmware/src/felucca.c` `#include`s every `.c` file. A macro or `static` in `ui.c`
  is visible to `menu_items.c`, `ui_graph.c`, `ui_draw.c`, `ui_menu.c`, `ui_input.c`, `ui_layer.c` (included in
  that order: see `felucca.c` lines 88-95).
- **Drawing primitives** (`gfx.c`, prefix `cv_`): `cv_rect`, `cv_rrect`, `cv_line`, `cv_line_t`, `cv_pset`,
  `cv_rule`, `cv_text*`, `cv_blit*`, `cv_keycap`, `cv_frame`, `cv_bits`, `cv_alpha`. Colours are theme tokens
  (`T_TEXT`, `T_THEME`, `T_ACCENT`, `T_MID`, `T_DIM`, `T_LINE`, `T_SEL`, `T_TINT`, `T_RAISE`, `T_KEY`, ...)
  derived by `palette_set()` from a palette's five stored colours.
- **Organic drawing already exists:** `ui_organic.c` (sub-pixel lines, Bezier curves, petals, rings, nodes; integer
  only, with `dsp.c`'s sine table). Reuse it before writing new primitives.
- **Pages:** `params.c` `PAGES[]` (about 80 pages in 11 families: HOME, ENV, LFO, FX, SCL, EDIT, GLO, SAVE,
  ARP, SEQ, TRK). Drawing per page type: `ui_graph.c` (`GR_*`), `ui_engvis.c` (engine visuals), `ui_being.c`
  (BEING), `ui_goobers.c` (ECO goobers), `ui_lzhome.c` (HOME/ECO), `ui_lzpage.c`, `ui_lzedit.c`, `ui_lzfx.c`,
  `ui_lzsave.c`, `ui_draw.c` (chrome, tabs, cards, the frame `ui_draw()`), `ui_menu.c` (the MENU),
  `ui_layer.c` (quick layers), `ui_live.c` (GLO > LIVE).
- **Animation clock:** `ui.frame`, advanced in `ui_draw()` (`ui_draw.c`). Live pages add it to their signature
  so they redraw; static pages redraw only when their signature changes.
- **Audio signals the UI can read:** `audio.c` `scope_buf[SCOPE_N]` (a ring of the **left** output samples,
  write index `scope_w`); `ui_graph.c` has per-track meters (`meter[NTRK]`). There is no right-channel ring yet:
  the X-Y view (§7.2) needs one.
- **Settings:** stored in `favorites.factory[15][n]` (engine 15's spare favourite bytes) and the bits of
  `ui_prefs` (`favorites.factory[15][30]`, all 8 bits used) and `ui_rec_prefs`. Migration in
  `settings_persist.c` `settings_import()`. Byte map today: 0..3 hints, 4..12 old layer counts (kept unused),
  **13 MOTION (new, §2)**, 14 ECO STYLE, 16 `cpt_res`, 18 LIST type, 19 SCOPE, 20 SKIN, 21 USB OUT, 22 USB LEVEL,
  23 JIANT palette mark, 24..31 others. A saved byte must keep its meaning for old settings: **append-only**.
- **Palettes:** `tools/gen_ui_palettes.py` makes `build/gen/ui_palettes.h`. Saved palette ids are tagged indices:
  **append-only**. `--report` checks WCAG contrast of every token pairing and fails on a miss.
- **The editor** (`editor_menu.c` `ED_MENU`): each menu row has a permanent id, **append-only** (next free: 27).
- **Tests:** `tests/run_tests.sh` (host, POSIX: it uses `fork`). `tests/ui_test.c` (UI logic against stubs),
  `tests/ui_render.c` (every screen in every palette: layout lint, alignment, PNGs in `build/ui_new`),
  `tests/editor_test.c`, `tests/settings_test.c`. Read the comment block at the top of `run_tests.sh`.

## 2. State of the branch when this brief was written

Committed: `docs/GUI-REVAMP-GARDEN.md` (the original page-by-page proposal: still the reference for which scene
each of the ~80 pages gets, §4 there), `docs/gui-garden-mockup.html`, and the **GARDEN palette** in
`tools/gen_ui_palettes.py` (appended; `--report` passes).

**Uncommitted in the working tree** (written and read through, never compiled; check them first in Phase 0):

- `firmware/src/ui.c`: `#define ui_motion (favorites.factory[15][13])` and `MOTION_Q8[4] = {256, 128, 384, 512}`.
- `firmware/src/ui_draw.c`: `ui.frame++` replaced by a 1/256 accumulator stepping `MOTION_Q8[ui_motion & 3]`.
- `firmware/src/menu_items.c`: row `MI_MOTION` (before `MI_PANEL`), page `MOTION` in the SCREEN tab
  (`MP_COUNT` 10), values 100 % / 50 % / 150 % / 200 %, get / name / put.
- `firmware/src/ui_menu.c`: help line "HOW FAST THE GARDEN MOVES".
- `firmware/src/editor_menu.c`: `{MI_MOTION, 26}`.
- `docs/GARDEN-BUILD-CHECKLIST.md`: notes for that change.

If the working tree on your machine does not have these (they were not committed), redo them from this list.

## 3. Decisions (the owner's, all final unless they say otherwise)

1. **GARDEN replaces the whole old UI.** No skin choice, no style choice, no palette choice: one look. Retire
   AIR, COLORS and HUD skins, FLAT / LINE style, the old palettes as choices, the heat ramp (`heat_col`), the
   plates / `og_ill` drawings, the ECO STYLE OG / GOOBERS choice, the SCOPE options that belong to the old look.
   Keep saved settings loading without errors (map retired values to GARDEN's).
2. **Every function and every control stays.** Knobs, encoders (SELECT, ALGORITHM, PRESETS), buttons, the 27
   keys, the layers, the navigation rules: unchanged. Only how things are drawn changes. Do not change
   `ui_input.c` / `ui_events.c` behaviour except where a retired option is read.
3. **Colours:** off-white beige base; pastel blue, pastel orange and very dark brown as the secondary colours.
   Text and values must pass the contrast check, so the palette's THEME / ACCENT are the darkened versions (§4).
4. **Fonts:** unchanged for now. The owner will revisit fonts after everything else.
5. **Fuzz / motion reacts to the music.** All motion speed follows the music level: about 0.2x of normal in
   silence, 2x at a full level (`rate = 0.2 + 1.8 x level`), times the user's MOTION setting. Sizes and pulses
   follow the level and the beat. Silence = slow, never frozen.
6. **ECO is a live standby** (creatures stroll), and **SELECT on ECO opens an X-Y vectorscope** (replaces the old
   phase / sweep view). Clicking (SELECT again) returns.
7. **MENU:** the new overlay look (§7.3) with the firmware's real rows, plus the new MOTION row. The mockup's
   placeholder rows (RAIN, GLOW, MUSIC LVL, TEMPO, SWING) are **not** to be added: RAIN is decoration, GLOW is the
   existing LED GLOW, TEMPO / SWING already exist on GLO, MUSIC LVL duplicates the master.
8. **Pixel art:** the backdrop pieces (clouds, soil, grass, house, windmill, shrubs) are blocky (3 px); creatures
   and plants are slightly pixelated (2 px); text stays as the fonts draw it.

## 4. Palette and art colours

The palette (`tools/gen_ui_palettes.py`, entry `GARDEN`, already appended; contrast passes):

| Token | Hex | Use |
| --- | --- | --- |
| BG | `#F3ECDF` | off-white beige ground of every screen |
| SURF | `#E6DAC4` | raised beige: panels, cards, cloud undersides |
| TEXT | `#2B1E16` | very dark brown: text, outlines, stems, eyes |
| THEME | `#1E4682` | deep blue: values, selected text (darkened from pastel for contrast) |
| ACCENT | `#963E14` | burnt orange: the one active thing (darkened from pastel for contrast) |
| knobs | `#1E4682 #963E14 #643214 #285890` | KNOB 1..4 value colours |

Art colours (fixed, used by the scene drawing only, never for text; define them as RGB565 constants next to the
scene code, like `UI_QR_LIGHT`):

| Name | Hex | Use |
| --- | --- | --- |
| `pblue` | `#A9CBEA` | sky top, rain, horizon dots, grass light, creature accents |
| `blue` | `#3F6FA8` | grass tufts, leaves, bush body, rings, sails |
| `orange` | `#F5A66B` | sun, flowers, the cursor pebble, the active pod |
| `burnt` | `#C8602A` | roofs, timber, caps, fruit, outlines of wood |
| `mid` | `#6B5A4C` | faint dotted paths |
| `dirt-lt` / `dirt` / `dirt-dk` | `#D9C2A6` / `#C2A07E` / `#A6876A` | soil, plus two blends between them = five soil tones |

## 5. The scene (every page's backdrop)

See `images/pages/*` and `mockups/pages.html` (`sky`, `clouds`, `rain`, `horizon`, `house`, `windmill`, `fence`,
`ground`, `acorn`, `chrome`). Draw order, back to front:

1. **Sky:** vertical gradient from BG at the horizon (y 186) to `pblue` at the top, exponent 1.4. A spiky orange
   sun top right (16-point star, radius 12 + 2 x level, the long points pulsing with the music), burnt core.
2. **Horizon band:** sparse `pblue` 2 px dots on a 3 px grid from y 150 to 186, an ordered-dither (Bayer 4x4)
   density rising toward the ground and with the level. It separates the sky from the ground.
3. **Clouds:** 4 cartoon clouds on the 3 px grid: a rounded base plus 2-4 round top puffs, off-white tops, a
   SURF underside whose top edge is ragged (1D value noise), never one straight line. Each cloud has its own random
   size, puff layout, height and drift speed. No outline. They pulse a little with the level.
4. **Rain:** slanted 1 px streaks in `blue` / `pblue`. Up to 130 drops; the music level sets how many show (23 of
   130 in silence, all at full level); the tempo sets the fall speed (120 BPM = the reference speed).
5. **House** (right, about x 196): irregular cottage on the ground line: wobbly stone footing, lopsided body
   with a leaning top and a shaded right side, crooked timber beams, a steep asymmetric roof with an off-centre
   peak and moss blobs on its lower edge, an off-centre chimney, two different windows (one with a flower box),
   an off-centre arched orange door. Burnt outlines. **Drawn blocky (3 px).** Always visible.
6. **Windmill** (left edge, x 20): tapering tower with stone courses and a shaded side, a window, a balcony rail,
   an orange door, a two-tone shingle cap, a swinging weather vane, and **big sails** (lengths 38 / 46, alternate
   cream / `pblue` with ladder rungs) that may run off the left edge. Sails turn a quarter turn per two beats
   (tempo); hub size and colour (burnt to orange) follow the level. **Drawn blocky (3 px).**
7. **Fence** (x 74-126): short picket fence on the ground line, shown 25 s of every 50 s.
8. **Ground:** the top edge `groundY(x) = 182 + 1.5 sin(...) + 5 x level x sin(...)` heaves with the music.
   **Soil:** 3 px pixels, two octaves of smooth value noise on an **integer hash** (a sine-based hash streaks
   diagonally and reads wet/shiny: do not use one), low contrast (`0.5 + 0.55 x (noise - 0.5)`), a 0.015 checker
   nudge, deeper rows slightly darker, quantised to the five soil tones. Matte, never shiny.
   **Grass:** one 3 px row of `blue`-ish patches along the edge (skip where noise < 0.25; a second row only where
   noise > 0.8). **Tufts:** every 18 px a stepped 3 px clump; about half (picked by hash) stand on a `dirt-dk`
   stem with a blue head; their height bounces with the level.
9. **Pods** (the four KNOB cards): small acorns along the bottom (y 226): a burnt cap and a body, the active
   knob's body orange, the others mid-soil (not cream: cream read as shiny).
10. **Label:** the page name top left in TEXT.

Motion rules for the scene: wind, sway, clouds, sails and creatures all run on the music-scaled clock (§3.5).

## 6. Pages and creatures

### 6.1 Page scenes (from `mockups/pages.html`; the full page list is `docs/GUI-REVAMP-GARDEN.md` §4)

| Page | Scene | What moves |
| --- | --- | --- |
| HOME | three tulips (one per track shown) + two floating flowers with faces (moods: happy, wow) | tulips grow, flowers bob |
| ECO | standby: a mushroom (still), a bird and a frog wander (two sines, turn back, face the way they move), two beetles crawl | all hop on the beat, pulse with the level |
| ENV | one large tulip growing through its envelope | growth = the envelope |
| LFO | a row of tulips swaying in the wind | sway = the LFO |
| FX | a stream (two wavy ribbons) with an apple, orange and cherry bobbing on the beat; beetles on the bank | ripples speed with level |
| DRUM-X | a ring of 8 mushroom caps on stems | each cap jumps on its lane's hit |
| FM6 | a fern whose stem grows from short, with asymmetric leaves (each side its own length), a small tulip on top | frond length = operator level |
| SEQ | 16 scalloped dirt mounds (2 rows of 8; no boxes): a tulip on each ON step, a seed dot on OFF, the playhead mound orange | the playhead walks at the tempo |
| MIXER | six mounds, a tulip in each whose height is the track level | heights = meters |

**Plants:** the tulip (stem, leaves whose two sides differ in length and angle, a three-petal cup that opens as it
grows; it grows one leaf at a time over 16 beats, then restarts). Free-standing flowers have faces and moods.
No shape is a plain circle: use wobbly polygons (`blob`), leaves (two Beziers), petals, teardrops.

### 6.2 Engine creatures (from `mockups/creatures.html`, `images/creatures/*`)

Each engine's EDIT / BEING picture and its HOME / ECO identity. All slightly pixelated (2 px), all on the shared
scene, all on the music-scaled clock.

| Engine | Creature | Behaviour |
| --- | --- | --- |
| DRUM | a drum kit like JIANT's: hi-hat (closes on the beat), crash on a stand, two toms, snare, kick with a front logo | toms / snare bounce, kick jumps on the beat; no stick |
| VOICE | a singing flower with a face | mouth opens with the level; notes leave the mouth only while music plays, farther and faster when louder |
| ANIMAL | a beetle | legs step, body hops on the beat |
| LOFI | an apple orbiting on a faint dotted path | size and squash change as it turns |
| ANALOG | a whimsical irregular orange being with a face, a smile, blush | off-centre swirl, uneven orbiting dots |
| FM6 | a band of five small fuzzy flowers | each bobs on its own phase |
| PHASE | a black hole: orange photon ring, black core, four tilted rings in 3D | far half of each ring behind the core, near half in front; each ring turns at its own speed and direction; rings and core pulse with beat and level; clipped above the ground |
| WHEEL | a barn wheel (`refs/wheel-barn-cart.png`): thick irregular wooden rim, 11 tapered uneven spokes, iron hub | rolls along the ground; rim and spokes turn with the distance covered (rotation = travel / radius) |
| NOISE | a tilted spiral galaxy with a spiky sun and three planets (one ringed) | arms turn; planets orbit at their own speeds |
| SHRUB | pixel bushes (`refs/shrub-pixel.png`): 3 px blocks, light top, dark base, a few berries | swell only with the music level (still in silence) |
| USER | a face inside an irregular halo that does not follow the face | eyes blink, mouth opens with the level, brows lift on each beat |

Engines not listed (DIGITAL, CMOS, PULSE, WHSTL, WING, GLOTT): ask the owner; until then reuse the closest
creature (CMOS / PULSE: ANIMAL's beetle family; WHSTL / WING / GLOTT: VOICE's flower family).

## 7. Menus and special views (`mockups/menus.html`, `images/menus/*`)

### 7.1 BEING
The track's creature large in the middle of the scene; the four HOME knobs as pods at the corners (label above,
a blue gauge and the value below, a dotted vein to the creature). Breadcrumb "HOME / BEING".

### 7.2 ECO X-Y view (SELECT on ECO)
Dark TEXT-brown background, no grid. A vectorscope crosshair in SURF at about 45 %: diagonals from the centre to
the corners with arrowheads, a vertical M axis with arrowheads, a dashed horizontal L-R / L+R line, small labels
L, R, M, L-R, L+R. The trace: left and right samples plotted against each other, L along the top-left diagonal,
R along the top-right (`x = 120 + A(R - L)/sqrt2`, `y = 120 - A(L + R)/sqrt2`), the last second as fading
`pblue` 2 px dots, the newest as an orange 4 px head. Needs a right-channel ring next to `scope_buf` in
`audio.c` (or interleave both into one ring): budget the RAM (§10).

### 7.3 MENU overlay
Over the garden, dimmed (TEXT at about 45 %): pebble tabs along the top (the active tab an orange pebble), a
rounded BG panel with a burnt 1.5 px edge, the breadcrumb ("MENU / SCREEN"), and the page's **four rows, row n
on KNOB n** as the firmware has them (`menu_items.c` `MPAGE`): the knob number in MID, the name in TEXT, the value
in THEME right-aligned, a 2 px value bar under each; the row whose knob was turned last on a `pblue` band with an
orange pebble at its left and an orange bar. Four knob pods along the bottom (orange / blue bodies, an arc that
fills with **that row's** value, a pointer, "K1".."K4"). The rows, pages, tabs and behaviour are `menu_items.c`'s
as they are, minus the retired COLOR, SKIN, STYLE (and ECO STYLE, LARGE if retired: §9 Phase 1) rows, plus
MOTION. An action row (CALIBRATION, ABOUT, LEARN CLEAR) shows "OCT+" as its value, as today.

## 8. How to port the mockups to firmware

- **Integer math.** Follow the repo: Q8 / Q16 fixed point, `dsp.c`'s sine table, no float in the draw loop.
- **Hashes:** use an integer hash (the mockups' `vnoise` hash: `imul` by 374761393 / 668265263, xorshift,
  `imul` 1274126177). Precompute the soil's shade per 3 px cell into a small table if it is cheaper than
  recomputing (the soil is static apart from its top edge).
- **Pixel art without a framebuffer:** draw on the block grid directly: `cv_rect` 3x3 / 2x2 blocks, polygons
  rasterised per block (test the block's centre). The 2 px creature look = fill each 2 px block whose centre is
  inside the shape.
- **Strips:** `gfx.c` renders in strips; per-pixel noise, per-block polygons and the trails all must work strip
  by strip. Look at how `ui_organic.c` and `ui_engvis.c` draw across strips before writing new loops.
- **Redraw:** only redraw what changed (signatures), frame cap about 15 fps for the animated scene. Static parts
  (soil below the top edge, house, windmill tower) can be drawn once per page and kept unless their region is
  covered by something moving.
- **The motion clock:** effective step per draw = `MOTION_Q8[ui_motion] x (0.2 + 1.8 x level)` (Q8); `level`
  from the output envelope (derive from `scope_buf` peaks or the master meter, smoothed). Beat phase from the
  sequencer / clock position (find what `ui_graph.c` already uses for the playhead).
- **Reuse:** `ui_organic.c` for curves and petals; the existing creature code paths (`ui_being.c`,
  `ui_goobers.c`, `ui_engvis.c`) are where engine pictures live today: replace their drawing, keep their hooks.

## 9. Phases (each ends with: build, size against the slot, host tests, a report to the owner)

**Phase 0: environment and baseline.**
- Linux x86-64 (or WSL2 / Docker on Windows / macOS). The JieLi toolchain is a Linux binary
  (`tools/get_toolchain.sh`; on Windows tar cannot make its symlinks). Clone the AC79 SDK (`BUILDING.md`).
  Pillow **with libraqm** (the font generator `tools/aa_raster.py` uses OpenType features; without raqm it fails).
  A POSIX C compiler for the host tests (`tests/run_tests.sh` uses `fork`).
- Build the tree as it is (with §2's uncommitted MOTION change). Record the app size and free bytes against the
  584,336 B slot (it was 248 B free before MOTION), `.data+.bss`, the pool spare. Fix MOTION if it fails.
- Run `tests/run_tests.sh`. Record the baseline (which pass, which fail before you touch anything).

**Phase 1: retire the old UI options (frees flash; nothing drawn new yet).**
- Make GARDEN the only palette: the COLOR row goes; `palette_from_stored` maps every stored id to GARDEN;
  keep the palette table only if the tests or the editor need the names (prefer removing the old entries'
  data but keep the tag scheme so old settings load). `settings_jiant_once` sets GARDEN instead of JIANT.
- Remove the SKIN row and the AIR / COLORS / HUD code paths (`gfx.c`, `ui_draw.c`, `ui.c`, `menu_items.c`,
  `settings_persist.c`): one layout remains. Keep `favorites.factory[15][20]` readable (ignored).
- Remove the STYLE row (FLAT / LINE) and its paths; LARGE: ask the owner (it is an accessibility option).
- Remove `heat_col` and its uses (ui_graph.c 20 uses, ui_tech.c, ui_lz*.c, ui_being.c, ...), the plates
  (`ui_organic.c` `og_ill`, `gen_ui_shapes.py` plates) if nothing else uses them.
- The editor: ids of removed rows stay reserved (never reuse an id); MENU_DESC must not list removed rows.
- Update the tests that check those rows (`ui_test.c`, `ui_render.c` "every palette", `editor_test.c`,
  `settings_test.c`): GREY gray / MONO neutral checks go; GARDEN's contrast stays checked by the generator.
- Gate: builds, smaller than before, all remaining tests pass, old settings load (settings_test).

**Phase 2: the scene engine.** A `ui_garden.c` (include it in `felucca.c` next to the other UI files): sky,
sun, horizon dots, clouds, rain, ground (edge, soil, grass, tufts), house, windmill, fence, pods; the music level
and beat inputs; the motion clock. One page (HOME) uses it. Gate: HOME matches `images/pages/HOME_*`, the frame
budget holds (the CPU TEST in `docs/STEMS-10-BUILD.md` with `tools/glitch_watch.py`: zero lost halves).

**Phase 3: chrome.** Tabs as pebbles, cards as acorn pods, breadcrumb, values, the knob highlight, the help line,
on every page. Gate: `ui_render` layout lint and alignment pass.

**Phase 4: page scenes by family** (§6.1 and `docs/GUI-REVAMP-GARDEN.md` §4): HOME / ECO, ENV, LFO, FX, SCL,
EDIT, GLO / MIXER, SAVE, ARP, SEQ. Each family: build, lint, size, report with `ui_render` PNGs.

**Phase 5: creatures** (§6.2), engine by engine, replacing the old engine visuals and the beings / goobers.

**Phase 6: ECO standby, the X-Y view (with the right-channel ring), BEING, the MENU overlay (§7).**

**Phase 7: motion and polish.** Music-scaled clock everywhere, silence slow, MOTION multiplier, the CPU TEST
with the UI busy, `docs/MANUAL.md` updated (the retired rows, MOTION, the new look), the release notes.

## 10. Hard limits

- **Flash:** the app slot is 584,336 B; at the start it was within a few hundred bytes. Every phase must report
  the size. If a phase grows the image past the slot, retire more old drawing code in the same phase. No bitmaps
  for art: everything is drawn by code (a few small tables are fine).
- **RAM:** `.data+.bss` 94,484 of 98,304 B; pool spare about 15.6 KB (the guard asks for 8 KB).
- **CPU:** stems-10 is near the edge (`docs/STEMS-10-BUILD.md`). The UI runs in the main loop beside the audio.
  The glitch test (`tools/glitch_watch.py`, CPU TEST) must show **zero lost halves** with the UI busy.
- **Hot audio code stays in RAM** (`FELUCCA_HOTRAM`): never move audio code to flash to make room.
- **Append-only:** palette ids, editor ids, settings bytes and their meanings.

## 11. Acceptance (the owner checks these on the device)

- Every page shows the garden scene; HOME, ECO, ENV, LFO, FX, DRUM-X, FM6, SEQ, MIXER match the images.
- Each engine shows its creature (§6.2) and it reacts as described.
- ECO is a live standby; SELECT opens the X-Y view and draws stereo shapes; SELECT returns.
- MENU looks like `images/menus/MENU_*`, with the real rows; COLOR, SKIN, STYLE are gone; MOTION works.
- In silence everything moves slowly; with music it moves faster (up to 2x) and pulses on the beat.
- All controls do exactly what they did before. Old settings and projects load.
- Builds within the flash slot; host tests pass; the CPU TEST shows no lost halves.

## 12. Working rules

- Work on `stems-10-fixed` or a branch from it (e.g. `garden-ui`). Commit at the end of each phase with a clear
  message; do not push or open a pull request unless the owner asks.
- Match the repo's code style: dense comments that say what a thing does and why, plain English, the version tag
  in comments (`(GARDEN)`), `static` everything, integer math.
- Never weaken a test to make it pass; if a test checks something retired, remove that check and say so.
- Report after each phase: what changed, size before / after, tests, what you could not do, what you need decided.

## 13. References (`refs/`, not committed)

The owner's examples, for intent only (do not copy them):
`fm1-device-colour-series.png` (the device and the colour idea), `cloud-shape-cartoon.png` and
`cloud-pixel-sky.png` (clouds), `ground-shading-*.png` (soil shading in pixel art), `house-windmill-pixel.png`
(house and windmill style), `phase-black-hole.png` (PHASE), `wheel-barn-cart.png` (WHEEL), `shrub-pixel.png`
(SHRUB), `xy-crosshair-vectorscope.png` and `xy-trace-tangle.png` (the X-Y view), `old-ui-eco-phase.png` and
`old-ui-menus.png` (the old UI being replaced).
