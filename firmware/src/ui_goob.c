/* ECO STYLE GOOBERS (MENU > SCREEN 2 > ECO STYLE; favorites.factory[15][14]: 0 OG, the beings swimming free as before;
 * 1 GOOBERS): one mushroom per track (4, 6 or 8: TRACKS) walking a forest along the floor. They skip over one another:
 * now and then two swap places in a hop each over the other. A hit on a track makes its goober sing out the note it
 * plays (the name over its cap, the mouth open), and its tree behind it grows and sways with the sound: sounds played,
 * the sequence and live notes alike, as the level reads them. Included by ui_lzhome.c (eco_draw): the same panel,
 * the same numbers and MACROS line as OG. Static state only (no pool), the sizes of ECO_N. */
#define GOOB_HOP_MS 600u                            /* a hop over the neighbour */
#define GOOB_SWAP_MS 1400u                          /* a swap starts this often (MENU > ANIM OFF: none) */
#define GOOB_SING_MS 700u                           /* the note's name shows this long after a hit */
#define GOOB_SING_GAP 250u                          /* and a fast run of hits sings again this often */
static struct {
    uint8_t slot[ECO_N], from[ECO_N], note[ECO_N], n;   /* the slot each goober stands in, the one its hop starts from */
    uint32_t hop[ECO_N], sing[ECO_N], swap, seed;       /* when its hop began, when it last sang, when the last swap */
} gb;
static int32_t gb_x0;                               /* the strip's left edge (eco_draw's x0) */
static const char *const GB_NOTE[12] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
static uint32_t gb_rnd(void) { gb.seed ^= gb.seed << 13; gb.seed ^= gb.seed >> 17; gb.seed ^= gb.seed << 5; return gb.seed; }
static uint32_t gb_hash(uint32_t k) { k = (k ^ 61u) * 2654435761u; return k >> 13; }
static int32_t gb_sqrt(int32_t v) { int32_t r = 0; while ((r + 1) * (r + 1) <= v) r++; return r; }
/* goober c sings the note of its newest voice (the smallest age; kept when none sounds) */
static void gb_note(uint32_t c)
{
    uint32_t i, a = 0xFFFFFFFFu;
    for (i = 0; i < NVOICE; i++)
        if (trk[c].v[i].active && trk[c].v[i].age < a) {
            a = trk[c].v[i].age;
            gb.note[c] = trk[c].v[i].note;
        }
}
/* the slots along the floor: n of them, the panel's 224 px shared out, the first 8 px in */
static int32_t gb_sp(void) { return 224 / (int32_t)gb.n; }
static int32_t gb_slot_x(uint32_t s) { return 8 + gb_sp() / 2 + (int32_t)s * gb_sp(); }
static int gb_hopping(uint32_t c) { return fm1_ms - gb.hop[c] < GOOB_HOP_MS; }
static void gb_reset(uint32_t n)                    /* the tracks changed (or the first frame): each in its own slot */
{
    uint32_t c;
    gb.n = (uint8_t)n;
    gb.seed = 0x6F0B5u;
    gb.swap = fm1_ms - GOOB_SWAP_MS;
    for (c = 0; c < n; c++) {
        gb.slot[c] = gb.from[c] = (uint8_t)c;
        gb.hop[c] = fm1_ms - GOOB_HOP_MS;
        gb.sing[c] = fm1_ms - GOOB_SING_MS;
        gb.note[c] = 60;
    }
}
/* two goobers swap: one at random hops to another slot, and the goober there hops back over it */
static void gb_swap(void)
{
    uint32_t n = gb.n, c = gb_rnd() % n, p, s = (gb.slot[c] + 1u + gb_rnd() % (n - 1u)) % n, a = gb.slot[c];
    for (p = 0; p < n && gb.slot[p] != s; p++)
        ;
    if (gb_hopping(c) || gb_hopping(p))
        return;
    gb.from[c] = (uint8_t)a;
    gb.slot[c] = (uint8_t)s;
    gb.from[p] = (uint8_t)s;
    gb.slot[p] = (uint8_t)a;
    gb.hop[c] = gb.hop[p] = fm1_ms;
    gb.swap = fm1_ms;
}
/* goober c's feet now: x (panel px) on its hop from its old slot to its new, and the height of the hop (px) */
static void gb_at(uint32_t c, int32_t *x, int32_t *lift)
{
    uint32_t dt = fm1_ms - gb.hop[c];
    int32_t q = dt < GOOB_HOP_MS ? (int32_t)(dt * 256u / GOOB_HOP_MS) : 256, a = gb_slot_x(gb.from[c]), b = gb_slot_x(gb.slot[c]);
    *x = a + ((b - a) * q >> 8);
    *lift = (q * (256 - q) * (gb_sp() / 2)) >> 14;  /* a parabola: its top half a slot over the neighbour */
}
/* a filled ellipse of radius r px at (lx, cy) in the strip's local x (cv_oy: the panel's y); top: its upper half only */
static void gb_dome(int32_t lx, int32_t cy, int32_t r, uint16_t c, int top)
{
    int32_t dy;
    for (dy = -r; dy <= (top ? 0 : r); dy++) {
        int32_t hw = gb_sqrt(r * r - dy * dy);
        cv_rect(lx - hw, cy + dy, 2 * hw + 1, 1, c);
    }
}
/* the forest behind the goobers: a tree at each slot (track k's, growing and swaying with its sound), and six small
 * ones between, static; their canopies lit by the level, the trunks drawn from the floor */
static void gb_forest(uint32_t n, int32_t feet, int still)
{
    uint32_t k;
    for (k = 0; k < 6u; k++) {                      /* the far trees: static, dim */
        int32_t x = 10 + (int32_t)k * 42 + (int32_t)(gb_hash(k + 16u) % 20u);
        gb_dome(x - gb_x0, feet - 16 - (int32_t)(gb_hash(k + 40u) % 10u), 7, ux_mix(T_SURF, T_LINE, 60), 0);
    }
    for (k = 0; k < n; k++) {                       /* the near trees: this track's level */
        int32_t en = eco.en[k], lx = gb_slot_x(k) + (int32_t)(gb_hash(k) % 9u) - 4 - gb_x0;
        int32_t h = 24 + en * 20 / 256, cr = 6 + en * 4 / 256, top = feet - h;
        int32_t sway = still ? 0 : bg_sin(ui.frame * 37u + k * 9000u) >> 13;
        cv_rect(lx - 1, top + cr, 2, feet - top - cr, T_LINE);
        gb_dome(lx + sway, top, cr, ux_mix(T_SURF, T_THEME, 40 + en * 60 / 256), 0);
    }
}
/* goober c: its mushroom at slot x (panel px), its feet at feet; r its cap; the stem's face, the legs while it walks, a
 * hop lifts it; a hit shuts its eyes; singing (sing) the mouth opens and the note's name shows over its cap */
static void gb_one(uint32_t c, int32_t feet, int words, int hud, int still)
{
    int32_t x, lift, r = gb_sp() / 3, hs, ws, cy, lx, fy, ph;
    uint16_t stem = T_TEXT, dark = T_SURF;
    int singing = fm1_ms - gb.sing[c] < GOOB_SING_MS;
    gb_at(c, &x, &lift);
    if (r > 12) r = 12;
    hs = r;
    ws = r * 2 / 3 < 3 ? 3 : r * 2 / 3;
    fy = feet - lift;
    cy = fy - hs;
    lx = x - gb_x0;
    ph = still || lift ? 0 : bg_sin(eco.tm[c] / 4u) > 0;
    if (!lift && !eco_off(c)) {                     /* legs: one lifts and drops as it walks */
        cv_rect(lx - 2, fy, 1, 2 - ph, stem);
        cv_rect(lx + 1, fy, 1, 1 + ph, stem);
    }
    cv_rect(lx - ws / 2, cy, ws, hs, stem);
    gb_dome(lx, cy, r, eco_off(c) ? T_DIM : eco_fc.c0[c], 1);
    cv_rect(lx - r / 2, cy - r / 2, 2, 2, T_TEXT);  /* spots */
    cv_rect(lx + r / 3, cy - r / 3, 2, 1, T_TEXT);
    cv_rect(lx - 2, cy + 2, 1, (eco_fc.fl[c] & EF_SHUT) ? 1 : 2, dark);   /* eyes: shut on a hit */
    cv_rect(lx + 1, cy + 2, 1, (eco_fc.fl[c] & EF_SHUT) ? 1 : 2, dark);
    if (singing)                                    /* the mouth open, and the note above the cap */
        cv_rect(lx - 1, cy + 4, 2, 2, dark);
    if (singing && !hud && lx >= 20 && lx + 20 <= (int32_t)cv_w) {   /* (the name over the cap; HUD: no room under its knobs, the mouth alone) */
        char nm[6];
        str_cpy(nm, GB_NOTE[gb.note[c] % 12u], 4);
        fmt_int(nm + str_len(nm), (int32_t)(gb.note[c] / 12u) - 1);
        cv_text_in(lx - 20, cy - r - 9, 40, &AF_X, nm, T_ACCENT, T_SURF);
    }
    if (words && (!hud || feet + 6 <= (int32_t)(graph_y() + graph_h()))) {   /* its number under it, as OG */
        char num[2] = {(char)('1' + c), 0};
        cv_text_in(lx - 30, eco_bot(), 60, &AF_S, num, mut_flash[c] ? vk_heat(255) : c == song.sel ? T_THEME : eco_off(c) ? T_DIM :
                   hud ? T_TEXT : T_MID, T_SURF);
    }
}
static void goob_draw(int32_t x0, int32_t y0, int words)
{
    uint32_t c, n = trk_live, still = (ui_prefs & PREF_ANIM_OFF) != 0u, pass;
    int32_t feet = eco_bot() - 6, hud = hud_home();
    gb_x0 = x0;
    if (n != gb.n)
        gb_reset(n);
    if (!still && fm1_ms - gb.swap >= GOOB_SWAP_MS)
        gb_swap();
    cv_oy = -y0;
    gb_forest(n, feet, still);
    for (c = 0; c < n; c++)                         /* a hit: the note sung (once per GOOB_SING_GAP) */
        if ((eco_fc.fl[c] & EF_SHUT) && fm1_ms - gb.sing[c] >= GOOB_SING_GAP) {
            gb.sing[c] = fm1_ms;
            gb_note(c);
        }
    for (pass = 0; pass < 2u; pass++)               /* the walkers first, the hopping ones over them */
        for (c = 0; c < n; c++)
            if (gb_hopping(c) == (int)pass)
                gb_one(c, feet, words, hud, still);
    if (words && home_set())
        eco_macros(hud);
    cv_oy = 0;
}
