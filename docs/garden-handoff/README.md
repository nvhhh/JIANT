# GARDEN handoff

Everything needed to implement the GARDEN UI with Claude Code.

1. Start here: `PROMPT.md` (paste it into Claude Code at the repository root).
2. The spec: `INSTRUCTIONS.md`.
3. The look: `images/` (per screen, 480 x 480, three moments each), `images/sheets/` (whole sheets).
4. The motion and exact drawing: `mockups/*.html` (open in a browser; `?shot&t=2.5&lvl=70` draws one frame).
5. The owner's references: `refs/` (third-party images, ignored by git: copy the folder by hand if you move
   this handoff to another machine).

## Image index

| Folder | Files |
| --- | --- |
| `images/pages/` | HOME, ECO, ENV, LFO, FX, DRUM-X, FM6, SEQ, MIXER |
| `images/creatures/` | DRUM, VOICE, ANIMAL, LOFI, ANALOG, FM6, PHASE, WHEEL, NOISE, SERUM, USER |
| `images/menus/` | BEING, ECO_XY (the SELECT view on ECO), MENU (the overlay) |

Each name has three files: `_a_music` (t = 1.2 s, level 70 %), `_b_music` (t = 3.4 s, level 70 %),
`_c_silent` (t = 2.0 s, level 0 %).

To regenerate the images (needs Chrome and Python with Pillow), screenshot each mockup with
`chrome --headless=new --window-size=960,720 --screenshot=out.png "file:///.../mockups/pages.html?shot&t=1.2&lvl=70"`
(menus: 960 x 240), then crop 240 px tiles in reading order (4 columns) and scale them 2x, nearest neighbour.
