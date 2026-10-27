# Build guide: SERUM branch (`shrub-serum`, the remote branch name is unchanged)

This guide is for building the SERUM firmware (the wavetable engine, the two new creatures and the two new scopes) into a `.fwsc` you can install on the FM-1. Read `BUILDING.md` first for the general toolchain notes; this file lists only what is specific to this branch and the order to run things in.

## 0. What changed on this branch

| Area | Files |
| --- | --- |
| SERUM engine (new, engine 15) | `firmware/src/eng_serum.c`, `firmware/src/serum_tables.c` (generated), `firmware/src/core.h`, `firmware/src/engines.c` |
| Generator for the wave tables | `tools/gen_serum_waves.py` |
| Creatures (visualizers) | `firmware/src/ui_engvis.c` (`vx_serum`, `vx_mseq`, `vx_knob`) |
| Scopes (BUBBLES, ORBIT) | `firmware/src/ui_graph.c` (`scope_at`, `scope_bubbles`, `scope_orbit`) |
| Menu and settings | `firmware/src/menu_items.c`, `firmware/src/settings_persist.c` |
| Other engine-count wiring | `favorites.c`, `gen.c`, `ui_being.c`, `midi_control.c` |
| Tests | `tests/serum_test.c` (new), `tests/ui_test.c`, `tests/ui_render.c` |
| Docs | `docs/MANUAL.md` (SERUM section) |

The branch is based on `origin/claude/intelligent-rubin-3oz7z6` (the usbloop / LCD 20 MHz lineage, commit `007789b`). Nothing on it is committed yet.

## 1. Check out the branch

```
git switch shrub-serum
git status          # expect the changes listed above, nothing else
```

## 2. Install the toolchain (once per machine)

Follow `BUILDING.md`, sections "Prerequisites (macOS)" and "Build". In short:

```
python3 -m pip install Pillow fonttools      # or a venv if pip says PEP 668
tools/get_toolchain.sh                        # -> ~/.jieli/toolchain
git clone --depth 1 --branch AC79NN_SDK_V1.2.1_2023-12-13 \
    https://gitee.com/Jieli-Tech/fw-AC79_AIoT_SDK.git ~/fw-AC79_AIoT_SDK
```

The JieLi toolchain is Linux x86-64 only. On macOS, `build.sh` runs each tool in a `linux/amd64` Docker container, so Docker Desktop must be running. Keep the repo under `/Users` so Docker can see it.

## 3. Host tests first (no toolchain needed)

Run these before a device build. They use gcc only.

```
python3 tools/gen_serum_waves.py                      # rewrites firmware/src/serum_tables.c (deterministic; git diff should be empty)
```

Generated headers for the UI tests come from the `tools/*.py` generators (`build/gen`). If `build/gen` is empty, run `./build.sh` once (step 4) or regenerate with the Pillow venv.

```
# SERUM DSP: presets bounded, WT and SUB audible, filters stable at RES 127
gcc -O2 -w -Ifirmware/src -Ibuild/gen -o /tmp/serum_test tests/serum_test.c -lm
/tmp/serum_test                                       # expect: "all ok (0 failed)"

# UI: menu, scopes, creatures, settings persistence
gcc -O1 -w -Ifirmware/src -Ibuild/gen -Itests -o /tmp/ui_test tests/ui_test.c -lm
/tmp/ui_test; echo "exit=$?"                          # expect: exit=0, no FAIL lines

# UI render harness (the PNG screens): compile with the same flags as ui_test; the
# harness's own usage line (head -40 tests/ui_render.c) gives its arguments and output folder.
gcc -O1 -w -Ifirmware/src -Ibuild/gen -Itests -o /tmp/ui_render tests/ui_render.c -lm
```

If any of these fail, stop and fix before step 4.

Known and expected:
- `tests/regress` exits 1 with 22 CPU entries over budget on a plain host (the same on the untouched HEAD tree). This is a host timing artifact, not a regression from this branch.

## 4. Build the firmware

```
./build.sh                    # defaults: FELUCCA_FLASH=1, FELUCCA_OTA=1, FELUCCA_CDC=1, FELUCCA_UAC=1
```

Outputs in `build/`:
- `felucca.bin`: the app
- `loader/ota.bin`: the update loader
- `felucca.fwsc`: the installable package

Optional, as in `BUILDING.md`: `LCD_BAUD=<hz>` for the 20 MHz panel test build, the `FELUCCA_*` flags in its table, `./build.sh --release X.Y` for a release.

Then check that the build reports, and note the flash and RAM numbers it prints. The app region is about 0x8EA90 bytes (`firmware/app.ld`). The build fails with "RAM region overflow" if RAM is exceeded. Record the flash-used figure in your report; it has not been measured for this branch yet.

## 5. Update the goldens (only after the engine is final)

The new engine has no golden renders yet. When the sound and the UI are settled, regenerate them and review the diff before committing. The update switches are documented at the top of `tests/run_tests.sh` (search it for `UPDATE`); use the ones it names, then:

```
git diff tests/golden.txt tests/cpu_baseline.txt
```

Do not update goldens for the existing 157 renders unless the diff shows the change was intended. They should not change on this branch.

## 6. Flash and check on the device

1. Make a backup of the current firmware (the installer keeps one; see `BUILDING.md`, "Install").
2. Install `build/felucca.fwsc` with the installer in `tools/fm1_install.py` (or the web installer under `web/`).
3. On the device: MENU > DISPLAY > SCOPE should show OFF / ON / BUBBLES / ORBIT. Press a SERUM preset (PRESETS, engine SERUM) and check the creature draws on the EDIT page.
4. Check MENU > ANIM OFF rests the creatures.

## 7. Known gaps (report these, do not fix silently)

- The web editor (`web/editor.html`) has a hardcoded 9-engine list and does not know engine 15. It may misbehave on SERUM until updated.
- SERUM is last in the engine display order (after NOISE) to keep existing PROGRAM CHANGE numbers. Moving it changes those numbers.
- Favorites row 4 is reused for SERUM. Stale bits from an old SAMPLE favorite would show as SERUM favorites.
- The jellyfish (`vx_mseq`) is called at the end of `graph_mseq`. Its bobbing and knob hop have only been seen as a static frame.
- Knob-flourish detection (`vx_knob`) can fire on preset load or track switch.
- The orbit scope draws about four times its delay in segments per frame. Frame cost on the device is not measured.
- CPU cost of SERUM (two wavetable reads per sample with DETUNE) is not compared against ANALOG.
- Table data is 4 KB (8 frames x 256 x int16), not the 2 KB originally asked for.
- The tree in `vx_serum` is sparse at low WT.

## 8. Commit (only when you are ready)

Nothing is committed. When the device check passes:

```
git add firmware/src tests tools docs
git commit -m "SERUM: wavetable engine, two creatures, two scopes"
```

Follow the repo's commit conventions, and add the attribution lines the session asks for.
