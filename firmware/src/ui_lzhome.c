/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments
 * Copyright (C) 2026 Juan Ignacio Antonucci (Juan Jiant) */
/* (0.8.5) HOME's lens (ui_lens.c): the panels of ECO and BEING, drawn as the approved mock-up (lente_sistema.html,
 * HOME) in the lens's panel (240 x 140 at y 84). Included by ui_graph.c after ui_being.c. The beings are JIANT's own
 * (ui_being.c: the same genomes, isotherms, nuclei, the same swelling, exciting and swimming with each track's sound,
 * graph_ecosys's maths); the mock-up gave the chrome around them.
 *   ECO    (0.8.8: as 0.8.4's HOME again, graph_ecosys) the six tracks' beings swimming free over the whole panel, each
 *          by itself (wandering, bouncing off the walls, a hit kicking it on, swelling with its sound), its number under
 *          it; the selected one larger, its number in the theme's colour (no ring, no grid, no names). The cytoplasm's
 *          granules drift behind them. (0.9.7, the approved variant A "Hoy con carita") each being today's (the spiky
 *          outline of its species, the same spikes as it sounds) in three isotherms instead of four, coloured by its own
 *          level (quiet cyan and blue .. loud red, orange, yellow), and where its nuclei were a small face: two eyes that
 *          shut on a hit, a mouth that opens with the level; muted, asleep (a dim outline, its eyes shut). MUTATE: when a
 *          mutation lands the being looks up, its number lights white and one ring in the accent opens (the critters and
 *          the ring of twelve dots, gone). MACROS: the macros' routes counted at the top left, LATCH / HOLD LFO at the
 *          right
 *   BEING  the track's being large in the middle, its four HOME knobs as organs at the corners (a gauge each, its name
 *          and value, a vein to the body), the sound's name under it; ROUTES: the being small, M1..M4 down the left
 *          with their routes drawn to their destinations ("T1 CUT+40")
 * Nothing in the audio path: the parts' peaks and the scope the UI already reads. */
#define ECO_N NTRK
static const int16_t ECO_PX[ECO_N] = {44, 120, 196, 64, 146, 206, 92, 172}, ECO_PY[ECO_N] = {34, 28, 40, 84, 78, 86, 58, 112};   /* (where they
                                                         * start, as 0.8.4's HOME; 7 and 8 (0.9.19) in the gaps) */
static struct {
    int32_t en[ECO_N], pen[ECO_N];                      /* each track's level now (0..256, slewed), the frame before */
    int32_t sx[ECO_N], sy[ECO_N], vx[ECO_N], vy[ECO_N];   /* the swim: where it is in the panel and its speed, Q8 px */
    int32_t r[ECO_N];                                   /* its radius now (Q4 px: swollen, pulsing) */
    uint32_t tm[ECO_N];                                 /* each being's time (it runs with its sound) */
    uint32_t f0;
    uint8_t init;
} eco;
/* (0.9.7, variant A) each being's face and colours, worked out once a drawn frame by eco_step for eco_being: its three
 * isotherms' colours by its level (c0 outer .. c2), its last hit's time (fm1_ms, low 16 bits: the eyes shut 0.1 s), its face's EF_*
 * (fl); init: the hits' times set. (Sized as the 0.9.6 critters were, in their place: the RAM's layout and so the sound
 * code's addressing as they were; EH_N the beings' room, ECO_N used) */
#define EH_N 36u
static struct {
    uint16_t c0[EH_N], c1[EH_N], c2[EH_N], ms[EH_N];
    uint8_t fl[EH_N];
    uint8_t init;
} eco_fc;
enum { EF_SHUT = 1, EF_SLEEP = 2, EF_UP = 4 };          /* (0.9.7) the face's: eyes shut (a hit), asleep, looking up */
static int eco_off(uint32_t c) { return trk[c].p[P_MUTE] || c >= trk_live; }   /* (muted, or asleep: TRACKS 4) */
static const genome_t *eco_genome(uint32_t c)
{
    uint32_t e = trk_eng(&trk[c]);
    return e == ENGI_DRUM ? &LANE_G_HOME : &GENOME[EGEN[e]];
}
/* track c's features: its genome, its HOME knobs, excited by its level k-fold (ui_being.c) */
static void eco_feat(uint32_t c, bfeat_t *f, int32_t k)
{
    uint8_t ids[4];
    uint32_t v, e = trk_eng(&trk[c]);
    for (v = 0; v < 4u; v++) ids[v] = (uint8_t)ENGINES[e]->knob[v];
    bg_feat(&trk[c], eco_genome(c), ids, f);
    bg_excite(f, eco.en[c], k);
}
/* (JIANT 0.5, as 0.8.4's HOME: graph_ecosys's life) the beings once a drawn frame (dt frames since the last): each
 * track's level (the part's peak, slewed: a hit at once, a quick fall), its time, its size (it pulses, and swells up to
 * ~2.2 x; the selected one larger), its swim over the whole panel (wandering, bouncing off the walls, a hit kicking it
 * on up to ~7 x). Still with ANIM OFF */
/* (0.8.9) the beings' water: the panel (COLORS 232 x 140, BOTTOM 240 x 148), HUD's HOME the whole screen; its floor, the
 * numbers' line under it (HUD: the numbers only in the middle, the floor near the screen's foot) */
static int32_t eco_h(void) { return hud_home() ? 240 : (int32_t)graph_h(); }
static int32_t eco_bot(void) { return eco_h() - (hud_home() ? 6 : 18); }
static uint32_t eco_step(void)
{
    uint32_t c, k, dt = ui.frame - eco.f0;
    int32_t bot = eco_bot();                            /* (the panel's floor, the number's line under it: 0.8.4's 104 of
                                                         * 122, the lens's 140 its 122) */
    if (!eco.init) {
        eco.init = 1;
        for (c = 0; c < ECO_N; c++) {
            eco.sx[c] = ECO_PX[c] << 8; eco.sy[c] = ECO_PY[c] << 8;
            eco.vx[c] = (c & 1u) ? 90 : -110; eco.vy[c] = (c & 2u) ? 70 : -60;
        }
    }
    if (dt > 8u) dt = 8u;
    if (ui_prefs & PREF_ANIM_OFF) dt = 0;
    eco.f0 = ui.frame;
    for (c = 0; c < ECO_N; c++) {
        track_t *t = &trk[c];
        int32_t v = t->peak * 256 / 7000, r = (c == song.sel ? 17 : 12) * 16, rad, sp;
        if (v > 256) v = 256;
        eco.pen[c] = eco.en[c];
        if (dt) {                                       /* (once a frame: a redraw in the same frame draws the same) */
            t->peak = 0;
            eco.en[c] = v > eco.en[c] ? v : eco.en[c] + (v - eco.en[c]) / 4;
            if (eco.en[c] - eco.pen[c] > 48)            /* (0.9.7) a hit: the face's eyes shut for 0.1 s */
                eco_fc.ms[c] = (uint16_t)fm1_ms;
        }
        if (!eco_fc.init || (uint16_t)((uint16_t)fm1_ms - eco_fc.ms[c]) > 1000u)   /* (none yet, or long ago: kept so, the
                                                         * 16 bits never wrapping round to a hit) */
            eco_fc.ms[c] = (uint16_t)(fm1_ms - 1000u);
        {   /* (0.9.7) its isotherms' colours: cyan / blue quiet .. red, orange, yellow loud; the inner ones warmer */
            int32_t h = 23 + eco.en[c] * 118 / 256, st = 38 + eco.en[c] * 13 / 256;
            eco_fc.c0[c] = vk_heat(h);
            eco_fc.c1[c] = vk_heat(h + st);
            eco_fc.c2[c] = vk_heat(h + 2 * st);
        }
        eco_fc.fl[c] = (uint8_t)((mut_flash[c] ? EF_UP : 0u) |
                                 ((uint16_t)((uint16_t)fm1_ms - eco_fc.ms[c]) < 100u && eco.en[c] > 77 ? EF_SHUT : 0u));
        if (eco.en[c] > 2) eco.tm[c] += dt * (uint32_t)(200 + eco.en[c] * 5);
        else eco.tm[c] += dt * 60u;                     /* (silent: it still breathes, slowly) */
        r = r * (256 + eco.en[c] * 5 / 4 + (bg_sin(eco.tm[c] / 3u) >> 11)) / 256;
        eco.r[c] = r;
        rad = r >> 4;
        sp = 256 + eco.en[c] * 6;
        for (k = 0; k < dt; k++) {                      /* swimming in the panel, bouncing off its walls, wandering */
            eco.vx[c] += bg_sin(ui.frame * 211u + c * 16000u + k * 977u) >> 12;
            eco.vy[c] += bg_sin(ui.frame * 157u + c * 23000u + k * 613u) >> 12;
            eco.vx[c] = clamp(eco.vx[c], -140, 140);
            eco.vy[c] = clamp(eco.vy[c], -110, 110);
            eco.sx[c] += eco.vx[c] * sp >> 8;
            eco.sy[c] += eco.vy[c] * sp >> 8;
            if (eco.sx[c] < (rad + 8) << 8) { eco.sx[c] = (rad + 8) << 8; eco.vx[c] = -eco.vx[c]; }
            if (eco.sx[c] > (232 - rad) << 8) { eco.sx[c] = (232 - rad) << 8; eco.vx[c] = -eco.vx[c]; }
            if (eco.sy[c] < (rad + 4) << 8) { eco.sy[c] = (rad + 4) << 8; eco.vy[c] = -eco.vy[c]; }
            if (eco.sy[c] > (bot - rad) << 8) { eco.sy[c] = (bot - rad) << 8; eco.vy[c] = -eco.vy[c]; }
        }
    }
    eco_fc.init = 1;
    return dt;
}
/* (0.9.7, variant A) a small ellipse (Q4: centre, radii at least 0.6 px) in ten lines */
static void eco_ell(int32_t cx, int32_t cy, int32_t rx, int32_t ry, uint16_t c)
{
    uint32_t i;
    int32_t px = 0, py = 0;
    rx = rx < 10 ? 10 : rx;
    ry = ry < 10 ? 10 : ry;
    for (i = 0; i <= 10u; i++) {
        uint32_t a = i * 65536u / 10u;
        int32_t x = cx + ((rx * og_cos(a)) >> 15), y = cy + ((ry * og_sin(a)) >> 15);
        if (i)
            og_line(px, py, x, y, c);
        px = x;
        py = y;
    }
}
/* .. the face (the mock-up's, the bichos'): at (cx, cy) Q4 of a being of radius r Q4, scaled k (Q8); its mouth open e (0..256),
 * its eyes shut (a hit) or asleep (muted), looking up (MUTATE) */
static void eco_face(int32_t cx, int32_t cy, int32_t r, int32_t k, int32_t e, uint32_t fl)
{
    int32_t rk = r * k >> 8, s = rk >= 19 * 16 ? 3 : 2, ex = rk * 77 >> 8, d;
    int32_t ey = cy - (rk * 31 >> 8), my, mo;
    uint16_t ec = (fl & EF_SLEEP) ? (T_DIM) : vk_heat(248);
    if (ex < 40) ex = 40;                               /* (2.5 px apart at least) */
    if (fl & EF_UP)                                     /* (MUTATE: it looks up) */
        ey -= rk * 31 >> 8 > 24 ? rk * 31 >> 8 : 24;
    for (d = -1; d <= 1; d += 2) {
        int32_t X = (cx + d * ex + 8) >> 4, Y = (ey + 8) >> 4;
        if (fl & (EF_SHUT | EF_SLEEP))
            cv_rect(X - (s + 1) / 2, Y, s + 1, 1, ec);
        else
            cv_rect(X - s / 2, Y - s / 2, s, s, ec);
    }
    my = ey + (rk * 92 >> 8 > 56 ? rk * 92 >> 8 : 56);
    mo = (e * 333 >> 8 > 256 ? 256 : e * 333 >> 8) * (rk * 97 >> 8) >> 8;   /* (its level x 1.3, at most the whole) */
    if (fl & EF_SLEEP)
        cv_rect(((cx + 8) >> 4) - 1, (my + 8) >> 4, 3, 1, ec);
    else if (fl & EF_UP)
        eco_ell(cx, my + 16, 22, 26, vk_heat(200));
    else if (mo > 21)
        eco_ell(cx, my + mo / 2 - 8, (rk * 26 >> 8) + (mo * 72 >> 8) > 24 ? (rk * 26 >> 8) + (mo * 72 >> 8) : 24, mo / 2, vk_heat(200));
    else {
        int32_t w = ((rk * 31 >> 8) + 8) >> 4;
        if (w < 1) w = 1;
        cv_rect(((cx + 8) >> 4) - w, (my + 8) >> 4, 2 * w + 1, 1, vk_heat(200));
    }
}
/* track c's being at (cx, cy) Q4, radius rq Q4, as HOME draws it (the most excited): (0.9.7, variant A) three isotherms by
 * its own level (outer: its species' spikes), its face; muted: asleep, a dim outline without spikes, its eyes shut. A
 * mutation just landed (mut_flash): it looks up, a ring in the accent opens round it */
static void eco_being(uint32_t c, int32_t cx, int32_t cy, int32_t rq)
{
    static const int16_t SC[3] = {256, 179, 113};
    const genome_t *g = eco_genome(c);
    bfeat_t f;
    int32_t en = eco.en[c], rr = rq + rq * en * 30 / 65536, i;
    uint32_t t = eco.tm[c] + c * 20000u;
    if (eco_off(c)) {
        bfeat_t a = {0, 0, 40, 3, 1, 0, 0, 0};
        uint16_t dc = T_DIM;
        bg_nv_in = 40;
        bg_outline(g, &a, cx, cy, rq, 0, eco.tm[c], dc);
        bg_nv_in = 0;
        eco_face(cx, cy, rq, 230, 0, EF_SLEEP);
        return;
    }
    uint16_t col[3] = {eco_fc.c0[c], eco_fc.c1[c], eco_fc.c2[c]};
    eco_feat(c, &f, 4);
    bg_nv_in = 40;                                      /* (the inner two: 40 vertices) */
    for (i = 0; i < 3; i++)                             /* (their colours by its level: eco_step) */
        bg_outline(g, &f, cx, cy, rr * SC[i] >> 8, !i, t, col[i]);
    bg_nv_in = 0;
    if (mut_flash[c]) {                                 /* MUTATE landed: one ring in the accent, opening and fading */
        int32_t q = 256 - (int32_t)mut_flash[c] * 256 / 12, rad = rr * (294 + q * 282 / 256) >> 8, px = cx + rad, py = cy;
        uint32_t k;
        for (k = 1; k <= 20u; k++) {
            uint32_t a = k * 65536u / 20u;
            int32_t x = cx + ((rad * og_cos(a)) >> 15), y = cy + ((rad * og_sin(a)) >> 15);
            og_line_a(px, py, x, y, T_ACCENT, 256 - q);
            px = x;
            py = y;
        }
    }
    eco_face(cx, cy, rr, 159, en, eco_fc.fl[c]);
}

/* (0.8.5) a gauge, as the mock-up's: an arc of 270 degrees (from 7:30, clockwise) round (cx, cy) px, radius r, w px
 * thick, the value f (0..1000) in vc over the track tc; antialiased lines (og_line), 24 a turn */
static void lz_arc(int32_t cx, int32_t cy, int32_t r, int32_t w, int32_t f, uint16_t tc, uint16_t vc)
{
    int32_t k, i;
    for (k = 0; k < w; k++) {
        int32_t rr = (r - k) * 16, px = 0, py = 0;
        for (i = 0; i <= 18; i++) {                     /* (18 segments over 270 degrees) */
            uint32_t a = 0x6000u + (uint32_t)i * 0xC000u / 18u;   /* (from 135 degrees, y down) */
            int32_t x = cx * 16 + (og_cos(a) * rr >> 15), y = cy * 16 + (og_sin(a) * rr >> 15);
            if (i)
                og_line(px, py, x, y, i * 1000 <= f * 18 ? vc : tc);
            px = x;
            py = y;
        }
    }
}

/* ------------------------------------------------------------------------------------------------ the panels --- */
static uint32_t routes_count(void)                      /* the MOD slots whose SRC is a macro, every track */
{
    uint32_t i, k, n = 0;
    for (i = 0; i < NTRK; i++)
        for (k = 0; k < 4u; k++) {
            const int16_t *p = &trk[i].p[P_M1SRC + 3u * k];
            n += p[0] >= MS_M1 && p[0] < MS_M1 + 4 && p[1] && p[2];
        }
    return n;
}
/* (GARDEN, Phase 6) ECO's standby (INSTRUCTIONS §6.1): a mushroom (still, its cap swelling with the level), a bird and a
 * frog wandering the ground (two sines each: they turn back, and face the way they go), two beetles crawling; all but
 * the mushroom hop on the beat. On the plants' clock (gd_pl: MOTION x the music), drawn into each strip by gd_poly */
static int32_t eco_wx(uint32_t i, uint32_t t)          /* walker i's x at time t (Q4 px) */
{
    static const int16_t C[4] = {150, 104, 34, 186}, A[4] = {44, 40, 12, 16}, B[4] = {16, 22, 6, 8};
    static const uint8_t F1[4] = {9, 7, 3, 4}, F2[4] = {23, 17, 7, 9};
    return C[i] * 16 + (A[i] * 16 * gd_s(t * F1[i] / 4u) >> 8) + (B[i] * 16 * gd_s(t * F2[i] / 4u + i * 9000u) >> 8);
}
static void eco_tri(int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t x2, int32_t y2, uint16_t c)
{
    int32_t pt[6] = {x0, y0, x1, y1, x2, y2};
    gd_poly(pt, 3, 1, c);
}
static void eco_dot(int32_t x, int32_t y, int32_t s, uint16_t c) { cv_rect((x >> 4) - gd.x0, y >> 4, s, s, c); }
static void eco_garden(void)
{
    int32_t e = gd_pl.e, k = 256 + e * 3 / 8, i;
    uint32_t t = gd_pl.t;
    {                                                   /* the mushroom: a pale stem, a burnt cap with spots */
        int32_t x = 72 * 16, y = gd_gy(72), r = 160 * k / 256, q[8] = {x - 40, y, x - 32, y - 160, x + 32, y - 160, x + 40, y};
        gd_poly(q, 4, 2, GD_SURF);
        gd_blob(x, y - 176, r, 9, GD_BURNT, 3, 256, 150);
        eco_dot(x - r / 2, y - 192, 2, GD_SURF);
        eco_dot(x + r / 3, y - 208, 2, GD_SURF);
        eco_dot(x + r * 2 / 3, y - 184, 2, GD_SURF);
    }
    for (i = 0; i < 4; i++) {
        int32_t x = eco_wx((uint32_t)i, t), d = eco_wx((uint32_t)i, t + 256u) >= x ? 1 : -1, y, s = k;
        y = gd_gy(x >> 4) - gd_hop(i == 1 ? 384u : i == 0 ? 256u : 128u) * 16;
        if (i == 0) {                                   /* the bird: a round body, its head, beak, wing and tail */
            gd_leaf(x - d * 80, y - 112, d > 0 ? 34000u : 64000u, 112 * s / 256, 48, GD_BURNT);
            gd_blob(x, y - 96, 96 * s / 256, 9, GD_ORANGE, 5, 300, 240);
            gd_blob(x + d * 96, y - 176, 64 * s / 256, 7, GD_ORANGE, 6, 256, 256);
            eco_tri(x + d * 144, y - 192, x + d * 208, y - 172, x + d * 144, y - 156, GD_INK);
            eco_dot(x + d * 104, y - 192, 2, GD_INK);
            gd_leaf(x - d * 16, y - 96, d > 0 ? 62000u : 36000u, 96 * s / 256, 40, GD_BLUE);
            eco_dot(x - 32, y - 32, 1, GD_INK);
            cv_rect(((x - 32) >> 4) - gd.x0, (y - 32) >> 4, 1, 2, GD_INK);
            cv_rect(((x + 32) >> 4) - gd.x0, (y - 32) >> 4, 1, 2, GD_INK);
        } else if (i == 1) {                            /* the frog: a squat body, two eyes on top, a wide mouth */
            gd_blob(x, y - 80, 112 * s / 256, 9, GD_BLUE, 7, 300, 200);
            gd_blob(x - 64, y - 160, 40, 6, GD_BLUE, 8, 256, 256);
            gd_blob(x + 64, y - 160, 40, 6, GD_BLUE, 9, 256, 256);
            eco_dot(x - 64 + d * 8, y - 168, 2, GD_INK);
            eco_dot(x + 64 + d * 8, y - 168, 2, GD_INK);
            cv_rect(((x + d * 16 - 48) >> 4) - gd.x0, (y - 96) >> 4, 6, 1, GD_INK);
            gd_leaf(x - d * 96, y - 16, d > 0 ? 40000u : 58000u, 80, 32, GD_BLUE);
        } else {                                        /* a beetle: a shell, its head, legs stepping */
            int32_t st = (int32_t)(t / 128u + (uint32_t)i) & 1, j;
            for (j = -1; j <= 1; j++) {
                cv_rect(((x + j * 40) >> 4) - gd.x0, (y - 24) >> 4, 1, 2 + (st ^ (j & 1)), GD_INK);
            }
            gd_blob(x, y - 64, 72 * s / 256, 8, GD_BURNT, (uint32_t)i + 11u, 300, 220);
            gd_blob(x + d * 72, y - 56, 32, 6, GD_INK, (uint32_t)i + 13u, 256, 256);
            cv_rect((x >> 4) - gd.x0, (y - 112) >> 4, 1, 4, GD_INK);
        }
    }
}
/* (GARDEN, Phase 6) ECO's X-Y view (INSTRUCTIONS §7.2; SELECT left on ECO, SELECT back): the whole screen dark, a
 * vectorscope's crosshair (the L and R diagonals, the M axis, the L-R / L+R line dashed), the last 512 sample pairs
 * (audio.c scope_buf, scope_r) as dots fading from old to new, the newest an orange head. L along the top-left
 * diagonal, R the top-right: x = 120 + A (R - L) / sqrt 2, y = 120 - A (L + R) / sqrt 2 */
static int eco_xy_on(void) { return ui.xy && ui.home && !gen_on() && home_level() == 0u && !home_set() && !ui.menu; }
static void xy_arrow(int32_t x, int32_t y, int32_t dx, int32_t dy, uint16_t c)   /* a head at x, y pointing dx, dy */
{
    cv_line(x, y, x - dx * 4 + dy * 3, y - dy * 4 - dx * 3, c);
    cv_line(x, y, x - dx * 4 - dy * 3, y - dy * 4 + dx * 3, c);
}
static void draw_xy(void)
{
    static uint32_t xy_ms;
    uint32_t pass, k, w = scope_w;
    uint16_t bg = T_TEXT, ax = ux_mix(T_TEXT, T_SURF, 45);
    if (!ui.force && fm1_ms - xy_ms < 66u)              /* (15 a second) */
        return;
    xy_ms = fm1_ms;
    ui.force = 0;
    for (pass = 0; pass < 2u; pass++) {
        int32_t y0 = (int32_t)pass * 120, x;
        cv_begin(240, 120, bg);
        cv_oy = -y0;
        cv_line(8, 8, 232, 232, ax);                    /* the crosshair: L, R, M, L-R / L+R */
        cv_line(232, 8, 8, 232, ax);
        cv_rect(120, 6, 1, 228, ax);
        for (x = 6; x < 234; x += 6)
            cv_rect(x, 120, 3, 1, ax);
        xy_arrow(8, 8, -1, -1, ax);
        xy_arrow(232, 8, 1, -1, ax);
        xy_arrow(8, 232, -1, 1, ax);
        xy_arrow(232, 232, 1, 1, ax);
        xy_arrow(120, 6, 0, -1, ax);
        xy_arrow(120, 233, 0, 1, ax);
        cv_text(18, 4, &AF_X, "L", ax);
        cv_text_r(222, 4, &AF_X, "R", ax, bg);
        cv_text(124, 4, &AF_X, "M", ax);
        cv_text(4, 108, &AF_X, "L-R", ax);
        cv_text_r(236, 108, &AF_X, "L+R", ax, bg);
        cv_text(22, 226, &AF_X, "X-Y \xB7 L / R", ax);
        for (k = 0; k < SCOPE_N; k++) {                 /* the trace, oldest first */
            uint32_t i = (w + k) & (SCOPE_N - 1u);
            int32_t L = scope_buf[i], R = scope_r[i], px, py, s = k + 1u == SCOPE_N ? 4 : 2;
            px = 120 + ((R - L) * 85 >> 14);
            py = 120 - ((L + R) * 85 >> 14);
            px = clamp(px, 2, 236);
            py = clamp(py, 2, 236);
            if (py + s <= y0 || py >= y0 + 120)
                continue;
            cv_rect(px - s / 2, py - s / 2, s, s, s == 4 ? GD_ORANGE : ux_mix(bg, GD_PBLUE, 15 + (int32_t)k * 85 / (int32_t)SCOPE_N));
        }
        cv_oy = 0;
        cv_blit(0, (uint32_t)y0);
    }
}
/* ECO: 0.8.4's HOME (graph_ecosys) in the lens's panel: the cytoplasm's granules drifting, the six beings swimming free
 * over the whole panel (0.9.7: variant A, eco_being), each with its number under it (the selected one larger, its number in
 * the theme's colour; a mutation landed, white). (0.8.9) Drawn into the canvas as the water's x0, y0 .. (eco_draw); SKIN HUD's HOME: the
 * water is the screen, every strip draws the part of it under it (eco_backdrop; a being off the strip skipped), the
 * numbers and the MACROS words in the middle strip only (words), clear of the floating ones */
static void eco_draw(int32_t x0, int32_t y0, int words)
{
    uint32_t c, k, still = (ui_prefs & PREF_ANIM_OFF) != 0u;
    int32_t bx[ECO_N], by[ECO_N], bot = eco_bot(), hud = hud_home();
    (void)eco_step();                                   /* (the swim, once a frame) */
    (void)bg_energy();                                  /* (the output's level kept as it was read: once a frame) */
    char b[16];
    cv_oy = -y0;
    (void)k; (void)c; (void)bx; (void)by; (void)bot; (void)still;
    eco_garden();                                       /* (GARDEN, Phase 6: the standby's creatures; the beings, their
                                                         * numbers and the cytoplasm retired) */
    if (words && home_set()) {                          /* MACROS: the routes, LATCH */
        int32_t y = hud ? (int32_t)graph_y() + 3 : 2;
        fmt_int(b, (int32_t)routes_count());
        str_cpy(b + str_len(b), " ROUTES", 8);
        cv_text(6, y, &AF_X, b, hud ? T_TEXT : T_DIM);
        cv_text_r(234, y, &AF_X, mac_latch ? "LATCH" : "HOLD LFO", mac_latch ? T_ACCENT : hud ? T_TEXT : T_DIM, T_SURF);
    }
    cv_oy = 0;
}
static void graph_eco(void) { eco_draw(0, 0, 1); }   /* (GARDEN: the GOOBERS look, ui_goobers.c, retired) */
static void eco_backdrop(int32_t x0, int32_t y0, int words)   /* (GARDEN: the beings in the garden, gd_scene) */
{
    gd_scene(x0, y0);
    eco_draw(x0, y0, words);
}
/* BEING: the track's being large, its HOME knobs as organs at the corners */
/* (GARDEN, Phase 6: the mockup's BEING, INSTRUCTIONS §7.1) each HOME knob a pod at a corner: "K1 PITCH" over it, a wobbly
 * body (K1 K3 orange, K2 K4 blue), a blue gauge and the value under it, a dotted vein to the creature */
static const int16_t ORG_X[4] = {34, 206, 34, 206}, ORG_Y[4] = {28, 28, 90, 90};
static void organ(uint32_t k, int32_t cx, int32_t cy)
{
    int16_t *vp;
    const param_desc_t *d = home_param(k, &vp);
    char v[12], nm[16] = {'K', (char)('1' + k), ' ', 0};
    const char *u;
    int32_t gx = ORG_X[k], gy = ORG_Y[k], hot = ui.hot_t && ui.hot_col == k, i, n, f;
    f = clamp(RATIO(d, enum_rank(d, *vp)), 0, 1000);
    n = (cx - gx < 0 ? gx - cx : cx - gx) / 5;
    for (i = 3; i < n; i++)                             /* the vein: a dot every 5 px, from the pod most of the way in */
        cv_rect(gx + (cx - gx) * i / (n + 4), gy + (cy - gy) * i / (n + 4), 1, 1, T_MID);
    gd_blob(gx * 16, gy * 16, 160 + (hot ? 24 : 0), 7, (k & 1u) ? GD_BLUE : GD_ORANGE, k + 3u, 256, 240);
    cv_rect(gx - 11, gy + 13, 22, 3, T_LINE);           /* the gauge */
    cv_rect(gx - 11, gy + 13, (22 * f + 500) / 1000, 3, GD_BLUE);
    param_format(d, *vp, v, &u);
    if (u && u[0] && str_len(v) + str_len(u) <= 5u)     /* ("901Hz": its unit when it fits) */
        str_cpy(v + str_len(v), u, sizeof v - str_len(v));
    GFX_HOOK_ALIGN(gx - 30, 0, gx + 30, 0, AL_H, "BEING organ value centred");
    cv_text_in(gx - 30, gy + 19, 60, &AF_X, v, T_TEXT, T_BG);
    str_cpy(nm + 3, d->label, sizeof nm - 3);
    GFX_HOOK_ALIGN(gx - 34, 0, gx + 34, 0, AL_H, "BEING organ name centred");
    cv_text_in(gx - 34, gy - 24, 68, &AF_X, nm, hot ? T_ACCENT : T_TEXT, T_BG);
}
static void graph_home_being(void)
{
    uint32_t c = song.sel % NTRK, k;
    int32_t cx = 120, cy = 68, bx = cx * 16, by = cy * 16;
    (void)eco_step();
    (void)bg_energy();
    char b[16];
    if (home_set()) {                                   /* ROUTES: the being small, the macros and their routes */
        uint32_t m, i, j, row = 0;
        int32_t ry[12];
        cx = 128;
        cy = 66;
        bx = cx * 16;
        by = cy * 16;
        eco_being(c, bx, by, (13 * 16) * (256 + eco.en[c] * 5 / 4 + (bg_sin(eco.tm[c] / 3u) >> 11)) / 256);
        for (m = 0; m < 4u; m++) {
            int32_t y = 16 + (int32_t)m * 34, n = 0;
            char mn[3] = {'M', (char)('1' + m), 0};
            uint16_t col = ui.hot_t && ui.hot_col == m ? T_ACCENT : T_THEME;
            for (i = 0; i < NTRK; i++)
                for (j = 0; j < 4u && row < 12u; j++) {
                    const int16_t *p = &trk[i].p[P_M1SRC + 3u * j];
                    char e[20] = {'T', (char)('1' + i), ' ', 0};
                    int32_t ty;
                    if (p[0] != (int16_t)(MS_M1 + m) || !p[1] || !p[2] || n >= 3)
                        continue;
                    ty = 6 + (int32_t)row * 11;
                    ry[row++] = ty;
                    n++;
                    str_cpy(e + 3, mod_dst_name(&trk[i], p[1]), sizeof e - 3);
                    str_cpy(e + str_len(e), p[2] > 0 ? "+" : "", sizeof e - str_len(e));
                    fmt_int(e + str_len(e), p[2] * 100 / 64);
                    og_quad_a(34 * 16, y * 16, 120 * 16, y * 16, 168 * 16, (ty + 4) * 16, heat_col(70 + macro_v[m] * 150 / 127),
                              14u, 200);
                    cv_clip_text(171, ty, &AF_X, e, T_MID, T_SURF, 66);
                }
            og_node(22 * 16, y * 16, 12, T_RAISE);
            lz_arc(22, y, 10, 2, macro_v[m] * 1000 / 127, T_LINE, col);
            GFX_HOOK_ALIGN(12, y - 5, 32, y + 5, AL_HV, "ROUTES macro name in its knob");
            cv_text_in(12, y - 4, 20, &AF_X, mn, T_TEXT, T_RAISE);
            if (!n)
                cv_text(40, y - 4 - AF_X_CAP_Y + 1, &AF_X, "NO ROUTE", T_DIM);
        }
        (void)ry;
        return;
    }
    gd.x0 = 0;                                          /* (the pods' bodies: gd_poly in the panel's own rows) */
    gd.y0 = 0;
    gd.y1 = (int32_t)cv_h;
    cv_halo = 1;                                        /* (their words outlined over the garden) */
    for (k = 0; k < 4u; k++)
        organ(k, cx, cy);
    {   /* the being: graph_being's size, its HOME knobs, the track's own level */
        const genome_t *g = eco_genome(c);
        bfeat_t f;
        eco_feat(c, &f, 2);
        if (vx_draw(&trk[c], cx - 73, cy - 56, 146, 112, eco_off(c) ? 0 : eco.en[c]))
            ;                                       /* (0.9.4 / 0.9.6: the engines' own creatures, ui_engvis.c) */
        else if (eco_off(c))
            bg_outline(g, &f, bx, by, (30 * 16), 1, eco.tm[c], T_DIM);
        else
            being_lines(g, &f, bx, by, (30 * 16) * (256 + eco.en[c] / 3) / 256, eco.en[c], eco.tm[c] + c * 20000u);
    }
    sound_name(&trk[c], b);
    GFX_HOOK_ALIGN(60, 0, 180, 0, AL_H, "BEING sound name centred");
    cv_text_in(60, 129, 120, &AF_X, b[0] ? b : trk_engp(&trk[c])->name, T_TEXT, T_SURF);
    cv_halo = 0;
}
static void graph_home_lz(void)
{
    if (home_level() == 1u)
        graph_home_being();
    else
        graph_eco();
}

