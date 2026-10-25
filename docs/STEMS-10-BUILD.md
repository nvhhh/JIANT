# stems-10 build: what changed and what it does

The stems-10 branch (eight tracks in the USB stems, 10 channels) did not fit the firmware's
memory: the image was 720 B over the flash slot and the RAM pool 256 B over. Three changes make
it build and pass every check in `tools/build.py`.

`./build.sh` builds it: on this branch `FELUCCA_HOTRAM` is off unless set (`FELUCCA_HOTRAM=1` puts the hot
code back in RAM, and the image no longer fits).

Result: app 577,104 B of the 584,336 B slot (7,232 B free); `.data+.bss` 94,484 B of 98,304 B;
pool spare 15,616 B (the guard asks for 8,192 B).

## 1. USB audio resampler table (`firmware/src/usb.c`)

- Before: the 44.1 -> 48 kHz table held all 160 phases in RAM (15 KiB of pool).
- Now: RAM holds phases 0..79. Phase 159 - p is phase p with its taps reversed, so the
  convolution reads the mirrored half backwards.
- Sound: none. The integer sums are the same, so the output is identical. The USB audio test
  (`tests/uac_test.c`) prints the same results as before, apart from its timing line.
- Pool: frees 7.5 KiB.
- CPU: about 5 ns more per 44.1 kHz frame on the host (25.7 -> 30.7 ns). The device's cost is
  not measured.
- Test change: `tests/uac_test.c` reads the mirrored half the same way.

## 2. COMB filter line (`firmware/src/voice.c`)

- Before: 1024 samples a part. The lowest pitch the COMB filter rings at is about 43 Hz.
- Now: 512 samples a part. The floor moves to about 86 Hz.
- Sound: on ANALOG's FILTER TYPE COMB only. A COMB patch's notes below about 86 Hz are
  clamped, so their resonance sits higher than the note asks for (an octave up at 43 Hz, two at
  22 Hz). Patches on other filter types are unchanged. Notes at 86 Hz and above are unchanged.
  Saved projects with low COMB notes sound different after the update.
- Pool: frees 8 KiB (eight tracks x 512 samples x 2 bytes).
- Regression renders: tracks-8 ran the same change and its 165 golden renders stayed the same.
  The golden set may not include low COMB notes, so this has not been checked for stems-10.

## 3. Core 0's hot audio code runs from flash (`FELUCCA_HOTRAM` off by default, `tools/build.py`)

- Before: the hottest audio functions (the mix and the parts' render) were copied to RAM at boot,
  and their copy sat in the flash image too. That copy is what put the image 648 B over.
- Now: they run from the XIP flash through its cache, like the rest of the code. The flash fits
  with 7,232 B free.
- `tools/build.py`: the reach check asked for at least one call from RAM code into the image, to
  prove it could read the disassembly. With `.hot_text` empty there is none, so the check now
  asks only when `.hot_text` holds code.
- Risk: this is the change that can cause audible glitches. Code in flash runs slower when the
  cache misses, and the UI drawing evicts audio code from the cache. If a render takes longer than
  a half buffer (2.9 ms), that half is late; past about two halves it is lost, and you hear a
  click. The host tests cannot see this (they have no flash cache). It has to be measured on the
  FM-1. See "Glitch test".

## Glitch test

What you need: the FM-1, a USB cable, `pip3 install pyserial`, and this build. For a comparison,
also the tracks-8 build (its hot code is in RAM).

### A. Measured load (USB AUDIO: CONSOLE)

1. Install the stems-10 build. MENU > USB > USB AUDIO: CONSOLE. MENU > TRACKS: 8.
2. Run `python3 tools/glitch_watch.py PORT` (macOS `/dev/cu.usbmodem…`, Linux `/dev/ttyACM0`).
   Each second it prints the load, the slowest render so far, and the late halves, lost halves
   (clicks), and shed voices in that second.
3. SAVE > TOOLS 6 (CPU TEST). This builds the heaviest song the firmware knows on all 8 tracks:
   160 BPM, 4-note chords on every step, POLY with GLIDE, FM6 and PHASE and ANALOG, a DRUM track
   with every lane on every step, DIST, CHORUS, the pitched DELAY and the CLOUD reverb.
4. Play it for at least 30 s. For the second half, keep the UI busy: switch screens, open the
   menu, turn knobs. This is the worst case for the flash cache.
5. Stop. Ctrl-C the watcher for the totals. MENU > ABOUT shows the CPU TEST result in %.
6. Harder: repeat with MENU > SYSTEM > CPU CLOCK: 320 MHZ, then CPU CORES: 1 CORE.
7. Undo: GLO + G4 brings the music back.

Reading it:

- `lost` above 0: an audible click for each one. This is the failure that matters.
- `late` above 0 with `lost` at 0: a render started late but was not dropped. You may hear
  nothing, but it is close to the edge.
- `shed` or `trimmed`: the load budget dropped voices. `trimmed` tails are under -50 dB and
  inaudible; `shed` voices can thin a chord.
- Slowest render near or over 100 % of the 2,902 us budget: no margin left.

Do the same steps on the tracks-8 build. The difference between the two runs is roughly what
moving the hot code to flash costs.

### B. Listening (USB AUDIO: STEMS)

The console is off in STEMS mode, so this one is by ear and by the on-device counter.

1. MENU > USB > USB AUDIO: STEMS. Record the 10 channels in a DAW at 48 kHz (this also runs the
   resampler change).
2. SAVE > TOOLS 6, play 30 s with the UI busy as above.
3. Listen to the recording for clicks and gaps, and read MENU > ABOUT > PROFILE: its LATE count
   and the worst block.
