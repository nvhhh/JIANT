# GARDEN: release notes (stems-10-newUI)

The FM-1's screen is rebuilt to the GARDEN mockups (`docs/garden-handoff/`). Sound, controls, saved settings and
projects are unchanged.

## New

- **One look: the garden.** Every page stands in a pixel scene (sky, sun, clouds, rain, windmill, house, fence, soil,
  tufts). The page's panel is a cream card with a burnt edge; tabs are pebbles; KNOB 1–4 are acorn pods along the
  bottom; the header is a breadcrumb at the left and the track, transport, BPM, load and battery at the right.
- **MENU › SCREEN › MOTION** (100 %, 50 %, 150 %, 200 %): how fast the garden moves. The scene runs on the music's
  clock: slow in silence (never frozen), up to twice as fast at full level. ANIMATION OFF stills it.
- **Page scenes**: ENV (a tulip growing through the envelope), LFO (six tulips swaying), FX (a stream with fruit
  hopping on the beat), PATTERN (sixteen mounds, a tulip on each step that plays, the playhead orange), DRUM-X KIT /
  SOUND (a ring of mushroom caps, each jumping on its hit), FM6 OPERATOR (a fern, a frond pair an operator).
- **Engine creatures** on the EDIT pages and HOME › BEING: a drum kit, a singing flower, a beetle, an orbiting apple,
  an orange being, five small flowers, a black hole, a barn wheel, a galaxy, pixel shrubs.
- **ECO** is a standby garden: a mushroom, a bird, a frog and two beetles.
- **X-Y view**: on ECO turn SELECT left; SELECT back. A vectorscope of the output (L, R, M, L-R / L+R).
- **BEING**: the creature in the garden, its four knobs as pods at the corners.
- **MENU**: rows on a card, the last turned on a blue band with an orange pebble, a value bar under each.
- **MIXER** as garden beds: a mound and a tulip per track (as tall as its LEVEL, its cup open with the meter; the
  selected track's mound orange), with the track's number, its sound's name, LEVEL in dB, PAN and the REV send kept over
  each bed (the owner's choice over the bare mockup). SENDS keeps its columns.
- **ECO** has HOME's three tulips: the selected track and the next two, each as tall as its LEVEL, open with its meter.

## Retired

- MENU rows **COLOR**, **SKIN**, **STYLE** and **ECO STYLE** (their editor ids stay reserved; old settings load).
- The AIR / COLORS / HUD skins, the FLAT / LINE styles, the palettes other than GARDEN, the heat colours, the plates.
- The old engine pictures (VOICE's vocal tract, ANIMAL's bird and mammal, LOFI's caterpillar and DIP, DRUM-X's little
  beings and colony, FM6's colony, the cell, worms, swarm and snail), ECO's swimming beings and GOOBERS.
- **LARGE** is kept (an accessibility option).

## After the device review (p12)

- **Motion follows the music**: the tempo sets how fast things move (the mockups' own seconds at 120 BPM; the old
  pace ran 18 to 120 times faster and strobed), the beat makes them pulse, the volume sets how much (quiet a quarter
  less). With the music off everything stands still, breathing slowly.
- HOME: the ground's waves rise on the beat; the rain falls at the tempo, as much of it as the volume.
- The creatures as the owner asked: the ladybird (two legs), ANALOG's swirl on its head, VOICE's pixel stem and bobbing
  head, DRUM-X's kit pulsing with each drum moving its own way and pixel stands, PHASE's hole and rings pulsing apart,
  WHEEL and NOISE at a third of the pace, SHRUB solid (no seams) with an outline, LOFI's apple at half the pace.
- FX: SENDS' stream slower and still when the music is; CHORUS's lines pulse; AUTO PAN's dot is the part's real pan
  (DEPTH 0 says OFF).
- The knob pods' values: no fading while the knobs rest, and a 13 px face (AF_V, 15 % smaller than M).
- MENU > SCREEN on two pages: SCREEN (SCOPE LARGE ANIMATION MOTION) and LIGHTS (SCREEN SLEEP LED GLOW SCALE LEDS).
- **COMB at full quality again**: each pair of parts shares a 1024-sample line (down to ~43 Hz); a part has all of
  it unless its neighbour is on COMB too (then half, ~86 Hz, as stems-10). No more RAM.

## Not done yet

- The MENU's dimmed garden behind the card, and knob pods with arcs (the acorn pods are used everywhere).
- Engines not in the handoff (DIGITAL, CMOS, PULSE, WHSTL, WING, GLOTT) use the nearest family's creature.
- **The CPU TEST on the device** (`docs/STEMS-10-BUILD.md`, `tools/glitch_watch.py`) with the UI busy. Host
  measurements: the audio ISR's loop cost is unchanged (the X-Y view's right-channel tap runs only while it is open);
  HOME's frame costs less than before; animated pages about +25 % UI instructions.

## Size

565,756 B of the 584,336 B slot (18,580 B free); RAM .data+.bss 95,332 of 98,304 B; pool spare 11,008 B.
