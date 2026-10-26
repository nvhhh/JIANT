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

## Not done yet

- The MENU's dimmed garden behind the card, and knob pods with arcs (the acorn pods are used everywhere).
- Engines not in the handoff (DIGITAL, CMOS, PULSE, WHSTL, WING, GLOTT) use the nearest family's creature.
- **The CPU TEST on the device** (`docs/STEMS-10-BUILD.md`, `tools/glitch_watch.py`) with the UI busy. Host
  measurements: the audio ISR's loop cost is unchanged (the X-Y view's right-channel tap runs only while it is open);
  HOME's frame costs less than before; animated pages about +25 % UI instructions.

## Size

555,600 B of the 584,336 B slot (28,736 B free); RAM .data+.bss 95,524 of 98,304 B; pool spare 11,008 B.
