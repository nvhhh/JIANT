# GARDEN: build checklist (stems-10-fixed)

Status: the changes below are written and read through, but **not compiled or tested**. There is no
C compiler on the machine they were written on. Build first, then work through the checks.

## What changed

| File | Change |
| --- | --- |
| `tools/gen_ui_palettes.py` | New palette **GARDEN**, appended (saved palette ids are append-only). Knob colours added. `--report` passes (exit 0). |
| `firmware/src/ui.c` | `ui_motion` = favourite byte 13 of engine 15 (0 = 100 %, the default). `MOTION_Q8` = 256 / 128 / 384 / 512 per draw. |
| `firmware/src/ui_draw.c` | `ui.frame++` becomes a step of `MOTION_Q8[ui_motion & 3]` per draw, with a 1/256 accumulator. At 100 % it is exactly one frame per draw, as before. |
| `firmware/src/menu_items.c` | New row **MOTION** (`MI_MOTION`, before `MI_PANEL`), on a new page **MOTION** in the SCREEN tab (`MP_COUNT` 9 to 10). Values 100 %, 50 %, 150 %, 200 %. Name, count, get, value name, put. |
| `firmware/src/ui_menu.c` | Help line for MOTION: "HOW FAST THE GARDEN MOVES". |
| `firmware/src/editor_menu.c` | MOTION added to the editor's row map, id **26** (the next free id). |

Notes on those changes:
- Rows after MOTION (PANEL, ABOUT) get new numbers. Nothing saves a row number (checked: only a runtime signature in `ui_menu.c`), so this is safe.
- `ui_motion` uses favourite byte 13. The hints use bytes 0..3 and bytes 4..12 are kept unused, so byte 13 looked free. **Check:** an existing settings file should open at 100 %. If it opens at another value, byte 13 was used before.
- Motion affects only the UI's frame clock (`ui.frame`): the beings, the ECO wander, the tulips' sway. Timers in milliseconds (holds, hints, message times) are unchanged.

## Not done (and why)

1. **Flash.** The app slot had 248 bytes free before these changes. The new code and table are probably 50 to 150 bytes, which may or may not fit. The build prints the size. If it is over the slot, retire the **COLORS** skin first (see `docs/GUI-REVAMP-GARDEN.md` §9), then **HUD**. That touches `gfx.c`, `ui_draw.c` and the settings migration, so it needs its own change.
2. **The GARDEN look.** The palette exists, but the screens are still the old layout. The mockup (`docs/gui-garden-mockup.html`) is not ported to C. Most of the look is drawing code for about 90 pages.
3. **Rows from the mockup not added.** RAIN has no firmware meaning (it is decoration in the mockup). GLOW is already the **LED GLOW** row. MUSIC LVL duplicates the master level. TEMPO and SWING already exist on the GLO page. Only MOTION was added. Say if you want others.
4. **Tests not updated.** `tests/ui_test.c` checks the page and tab layout through `MP_COUNT` and `MTAB_COUNT`, so it should still pass. The SELECT walk now has one more page. Add a test for MOTION (values, 100 % = one frame, 200 % = two).

## Build (in the Claude app or another machine)

1. Install the JieLi toolchain: `tools/get_toolchain.sh` (installs to `~/.jieli/toolchain`). On Windows the symlinks need Developer Mode or a Linux environment.
2. Clone the AC79 SDK as in `BUILDING.md` (`~/fw-AC79_AIoT_SDK`).
3. Install Pillow: `pip3 install Pillow`.
4. On Windows, run the build in WSL2 or Docker. On Linux x86-64 it runs natively.
5. Palette check: `python3 tools/gen_ui_palettes.py --report out.h` (exit 0).
6. Build: `./build.sh`. Note the app size against the slot.
7. Host tests (need a host C compiler): `tests/run_tests.sh`.

## After building: what to check

- **Palette:** MENU > SCREEN > COLOR lists GARDEN (last). Selecting it makes the screen beige and dark brown, with pastel blue and orange only where the palette sets them.
- **MOTION:** MENU > SCREEN > MOTION steps 100 %, 50 %, 150 %, 200 %. The card and help line show. At 200 % the beings and the ECO wander visibly run twice as fast; at 50 % half.
- **Persistence:** set MOTION to 200 %, leave the menu, power off and on. It should still read 200 %.
- **Old settings:** a settings file from before this change opens at 100 %.
- **Editor:** the desktop editor lists MOTION in the SCREEN tab (its id is 26).
- **Regression:** at 100 %, nothing should look different from before this change.
