/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments
 * Portions Copyright (C) 2026 Juan Ignacio Antonucci (Juan Jiant) */
/* GARDEN (docs/garden-handoff/INSTRUCTIONS.md §5; mockups/pages.html chrome()): the scene a page stands in, drawn by
 * code into the strip being drawn (gd_scene: x0, y0 its place on the screen), back to front: the sky (BG at the
 * horizon to pblue at the top), the spiky sun, the horizon's dots, four clouds, the rain, the house and the windmill
 * (3 px blocks: each block the colour its centre pixel would have), the fence, the ground (soil in five tones, grass,
 * tufts). Integer only, every strip on its own (no framebuffer). The motion clock (gd_tick, every draw): the scene's
 * time runs at MENU > MOTION x (0.2 + 1.8 x the music's level), the beat from the tempo on that clock; ANIM OFF: still.
 * (Phase 2: HOME's backdrop, ui_lzhome.c eco_backdrop.) */

/* the art colours (fixed, never text; RGB565) */
#define GD_PBLUE 0xAE5Du                               /* #A9CBEA sky top, rain, horizon dots */
#define GD_BLUE 0x3B75u                                /* #3F6FA8 tufts, sails' frames, rain */
#define GD_ORANGE 0xF52Du                              /* #F5A66B the sun, the door */
#define GD_BURNT 0xCB05u                               /* #C8602A roofs, timber, the sun's core */
#define GD_INK 0x28E2u                                 /* #2B1E16 */
#define GD_SURF 0xE6D8u                                /* #E6DAC4 cloud undersides, walls */
enum { GC_PBLUE, GC_BLUE, GC_ORANGE, GC_BURNT, GC_INK, GC_SURF, GC_S1, GC_S2, GC_S3, GC_SHADE, GC_ROOF2, GC_CAP2,
       GC_TOWER, GC_GMID, GC_GLIGHT, GC_DIRTLT, GC_DIRTDK };
static const uint16_t GD_C[] = {GD_PBLUE, GD_BLUE, GD_ORANGE, GD_BURNT, GD_INK, GD_SURF,
                                0xCD92u, 0xC50Fu, 0xB4AEu,   /* the soil's middle three (dirt-lt..dirt, dirt, dirt..dirt-dk) */
                                0xDE55u, 0xDBC8u, 0xDBE8u, 0xDE35u,   /* the house's shade, the roof's lit side, the cap's, the tower's */
                                0x74F9u, 0x95DBu,            /* grass: mid, light */
                                0xDE14u, 0xA42Du};           /* dirt-lt, dirt-dk */
static const uint8_t GD_SOIL[5] = {GC_DIRTLT, GC_S1, GC_S2, GC_S3, GC_DIRTDK};

static struct {
    uint32_t t, ms, b;                                 /* the scene's time (1/1024 s), fm1_ms then, the beat (Q16) */
    int32_t e, x0, y0, y1;                             /* the music's level 0..256; the strip: x0, rows y0 .. y1 - 1 */
    uint16_t fq;                                       /* the clock's fraction */
} gd;

static uint8_t gd_plain;                               /* (host tests: a strip on BG, to measure its words) */
static int32_t gd_edged;
static struct { uint32_t t, b; int32_t e; } gd_pl;      /* (Phase 4) the plants' clock: their time (1/1024 s), beats (Q16), the level */                               /* (gd_edges: the panel's edges kept for that strip) */
static int32_t bg_energy(void);                        /* (ui_being.c: the output's level, 0..256, slewed) */
/* the motion clock: once a draw (ui_draw.c ui_draw) */
static void gd_tick(void)
{
    uint32_t dt = fm1_ms - gd.ms, d;
    gd.ms = fm1_ms;
    if (!(ui_prefs & PREF_ANIM_OFF) && dt <= 250u) {   /* (Phase 4) the plants' clock: always, at MOTION's rate */
        if (dt)
            gd_pl.e = bg_energy();
        d = dt * 1024u * (51u + 461u * (uint32_t)gd_pl.e / 256u) / 1000u * MOTION_Q8[ui_motion & 3u] >> 8;
        gd_pl.t += d;
        gd_pl.b += d * (uint32_t)song.g[G_BPM] * 16u / 15u;
    }
    if (!hud_home())                                    /* (Phase 3) still behind the other pages: their strips agree */
        return;
    gd_edged = 0;
    if (dt > 250u || (ui_prefs & PREF_ANIM_OFF))
        dt = 0;
    if (dt)                                             /* (the level with the clock: a redraw at the same time, the same) */
        gd.e = bg_energy();
    d = gd.fq + (dt * 1024u * (51u + 461u * (uint32_t)gd.e / 256u) / 1000u * MOTION_Q8[ui_motion & 3u] >> 8);
    gd.fq = (uint16_t)(d & 255u);
    gd.t += d >> 8;
    gd.b += (d >> 8) * (uint32_t)song.g[G_BPM] * 16u / 15u;   /* (beats: bpm / 60 a second, 1024 ticks a second) */
}
/* an integer hash (the mockups' vnoise): 0..65535 */
static uint32_t gd_h(int32_t a, int32_t b)
{
    uint32_t s = (uint32_t)a * 374761393u + (uint32_t)b * 668265263u;
    s = (s ^ (s >> 13)) * 1274126177u;
    return (s ^ (s >> 16)) >> 16;
}
/* smooth value noise at x, y (Q8 lattice units): 0..255 */
static int32_t gd_vn(int32_t x, int32_t y)
{
    int32_t xi = x >> 8, yi = y >> 8, xf = x & 255, yf = y & 255;
    int32_t u = xf * xf * (768 - 2 * xf) >> 16, v = yf * yf * (768 - 2 * yf) >> 16;
    int32_t a = (int32_t)gd_h(xi, yi), b = (int32_t)gd_h(xi + 1, yi), c = (int32_t)gd_h(xi, yi + 1);
    int32_t top = a + ((b - a) * u >> 8), bot = c + (((int32_t)gd_h(xi + 1, yi + 1) - c) * u >> 8);
    return (top + ((bot - top) * v >> 8)) >> 8;
}
static int32_t gd_s(uint32_t ph) { return sine_i(ph << 16) >> 7; }   /* ph: 65536 a turn -> Q8 */
/* the ground's top edge at x (Q4 px): it heaves with the music */
static int32_t gd_gy(int32_t x)
{
    return 2912 + (24 * gd_s((uint32_t)x * 522u + gd.t * 12u) >> 8) +
           (80 * gd.e >> 8) * gd_s((uint32_t)x * 417u - gd.t * 41u) / 256;
}
/* the sky's colour at row y: BG at the horizon (186) to pblue at the top, (1 - y / 186) ^ 1.4 */
static uint16_t gd_sky(int32_t y)
{
    int32_t u = y >= 186 ? 0 : (186 - y) * 256 / 186;
    return ux_mix(T_BG, GD_PBLUE, ((u * u >> 8) * 2 / 5 + u * 3 / 5) * 100 >> 8);
}
static int32_t gd_fl(int32_t v, int32_t p) { return v >= 0 ? v / p * p : -((p - 1 - v) / p) * p; }
/* a polygon (Q4 screen points) filled on a P px grid: each block whose centre is inside */
static void gd_poly(const int32_t *p, uint32_t n, int32_t P, uint16_t c)
{
    int32_t x0 = 99999, x1 = -99999, y0 = 99999, y1 = -99999, bx, by;
    uint32_t i, j;
    for (i = 0; i < n; i++) {
        if (p[2 * i] < x0) x0 = p[2 * i];
        if (p[2 * i] > x1) x1 = p[2 * i];
        if (p[2 * i + 1] < y0) y0 = p[2 * i + 1];
        if (p[2 * i + 1] > y1) y1 = p[2 * i + 1];
    }
    for (by = gd_fl(y0 >> 4, P); by <= y1 >> 4; by += P) {
        int32_t Y = by * 16 + P * 8;
        if (by + P <= gd.y0 || by >= gd.y1)
            continue;
        for (bx = gd_fl(x0 >> 4, P); bx <= x1 >> 4; bx += P) {
            int32_t X = bx * 16 + P * 8, in = 0;
            for (i = 0, j = n - 1u; i < n; j = i++)
                if ((p[2 * i + 1] > Y) != (p[2 * j + 1] > Y) &&
                    X < (p[2 * j] - p[2 * i]) * (Y - p[2 * i + 1]) / (p[2 * j + 1] - p[2 * i + 1]) + p[2 * i])
                    in ^= 1;
            if (in)
                cv_rect(bx - gd.x0, by, P, P, c);
        }
    }
}

/* the house and the windmill: tables of shapes in px from their foot (a polygon's points, a rectangle, a 1.2 px line, a
 * closed outline, a disc of radius r / 4), sampled at each 3 px block's centre, the last shape that covers it wins */
enum { GP_POLY, GP_LOOP, GP_RECT, GP_SEG, GP_DISC };
typedef struct { uint8_t k, c; int8_t a, b, w, h; } gd_prim_t;   /* POLY / LOOP: a the first point, b how many */
static const int8_t GD_PT[] = {
    -23, 0, -23, -4, -14, -6, 0, -5, 12, -7, 23, -4, 23, 0,                       /* 0: the house's footing */
    -21, -4, -22, -17, -15, -19, -9, -18, -2, -22, 6, -20, 14, -21, 22, -15, 21, -4,   /* 7: its body */
    8, -21, 14, -21, 22, -15, 21, -4, 10, -4,                                    /* 16: the body's shaded side */
    -25, -16, -14, -30, -4, -26, 7, -38, 25, -17,                                /* 21: the roof */
    7, -38, 25, -17, 17, -18, 8, -26,                                            /* 26: its lit side */
    -8, 0, -5, -60, 5, -60, 8, 0,                                                /* 30: the windmill's tower */
    -7, -59, 0, -68, 0, -59,                                                     /* 34: the cap */
    0, -68, 7, -59, 0, -59,                                                      /* 37: its lit side */
    -9, -1, -4, -2, -4, 1, 0, -1, 4, -3, 4, 0};                                  /* 40: the vane (from its pivot) */
static const gd_prim_t GD_HOUSE[] = {
    {GP_POLY, GC_S3, 0, 7, 0, 0}, {GP_POLY, GC_SURF, 7, 9, 0, 0}, {GP_POLY, GC_SHADE, 16, 5, 0, 0},
    {GP_LOOP, GC_BURNT, 7, 9, 0, 0}, {GP_SEG, GC_BURNT, -21, -11, 21, -13}, {GP_SEG, GC_BURNT, -4, -21, -2, -4},
    {GP_POLY, GC_BURNT, 21, 5, 0, 0}, {GP_POLY, GC_ROOF2, 26, 4, 0, 0}, {GP_LOOP, GC_BURNT, 21, 5, 0, 0},
    {GP_DISC, GC_GMID, -24, -17, 9, 0}, {GP_DISC, GC_GMID, -20, -16, 9, 0}, {GP_DISC, GC_GMID, -16, -17, 9, 0},
    {GP_DISC, GC_GMID, -12, -16, 9, 0}, {GP_DISC, GC_GMID, -8, -17, 9, 0}, {GP_DISC, GC_GMID, -4, -16, 9, 0},
    {GP_DISC, GC_GMID, 0, -17, 9, 0}, {GP_DISC, GC_GMID, 4, -16, 9, 0}, {GP_DISC, GC_GMID, 8, -17, 9, 0},
    {GP_DISC, GC_GMID, 12, -16, 9, 0}, {GP_DISC, GC_GMID, 16, -17, 9, 0}, {GP_DISC, GC_GMID, 20, -16, 9, 0},
    {GP_DISC, GC_GMID, 24, -17, 9, 0},                                           /* (moss on the roof's lower edge) */
    {GP_RECT, GC_BURNT, 9, -38, 4, 9},                                           /* the chimney */
    {GP_RECT, GC_INK, -17, -17, 8, 11}, {GP_RECT, GC_PBLUE, -16, -16, 6, 9}, {GP_RECT, GC_INK, -13, -16, 1, 9},
    {GP_RECT, GC_INK, 7, -26, 7, 7}, {GP_RECT, GC_PBLUE, 8, -25, 5, 5}, {GP_RECT, GC_INK, 10, -25, 1, 5},
    {GP_RECT, GC_BURNT, -17, -7, 8, 2}, {GP_DISC, GC_ORANGE, -15, -8, 6, 0}, {GP_DISC, GC_BLUE, -12, -8, 6, 0},
    {GP_RECT, GC_INK, 1, -13, 9, 8}, {GP_RECT, GC_INK, 2, -14, 7, 1},           /* the door, arched */
    {GP_RECT, GC_ORANGE, 2, -12, 7, 7}, {GP_RECT, GC_ORANGE, 3, -13, 5, 1}};
static const gd_prim_t GD_MILL[] = {
    {GP_POLY, GC_SURF, 30, 4, 0, 0},
    {GP_RECT, GC_S2, -8, -5, 16, 1}, {GP_RECT, GC_S2, -6, -11, 14, 1}, {GP_RECT, GC_S2, -8, -17, 16, 1},
    {GP_RECT, GC_S2, -6, -23, 14, 1}, {GP_RECT, GC_S2, -8, -29, 16, 1}, {GP_RECT, GC_S2, -6, -35, 14, 1},
    {GP_RECT, GC_S2, -8, -41, 16, 1}, {GP_RECT, GC_S2, -6, -47, 14, 1}, {GP_RECT, GC_S2, -8, -53, 16, 1},
    {GP_RECT, GC_TOWER, 2, -60, 6, 60}, {GP_LOOP, GC_BURNT, 30, 4, 0, 0},
    {GP_RECT, GC_INK, -2, -40, 5, 7}, {GP_RECT, GC_PBLUE, -1, -39, 3, 5},       /* a window */
    {GP_RECT, GC_BURNT, -9, -55, 18, 2}, {GP_RECT, GC_BURNT, -8, -58, 1, 3}, {GP_RECT, GC_BURNT, -5, -58, 1, 3},
    {GP_RECT, GC_BURNT, -2, -58, 1, 3}, {GP_RECT, GC_BURNT, 1, -58, 1, 3}, {GP_RECT, GC_BURNT, 4, -58, 1, 3},
    {GP_RECT, GC_BURNT, 7, -58, 1, 3},                                           /* the balcony and its rail */
    {GP_RECT, GC_ORANGE, -2, -9, 4, 9}, {GP_POLY, GC_BURNT, 34, 3, 0, 0}, {GP_POLY, GC_CAP2, 37, 3, 0, 0}};
/* X, Y (Q4) on the segment a .. b drawn as a line about 2 px wide (the blocks' outlines) */
static int gd_near(int32_t X, int32_t Y, int32_t ax, int32_t ay, int32_t bx, int32_t by)
{
    int32_t dx = bx - ax, dy = by - ay, l = dx * dx + dy * dy, t = l ? ((X - ax) * dx + (Y - ay) * dy) * 256 / l : 0, px, py;
    t = clamp(t, 0, 256);
    px = ax + dx * t / 256 - X;
    py = ay + dy * t / 256 - Y;
    return px * px + py * py <= 324;                   /* (1.1 px, so that a 3 px block's centre finds it) */
}
static int gd_in_poly(int32_t X, int32_t Y, const int8_t *p, uint32_t n, int loop)
{
    uint32_t i, j;
    int in = 0;
    for (i = 0, j = n - 1u; i < n; j = i++) {
        int32_t xi = p[2 * i] * 16, yi = p[2 * i + 1] * 16, xj = p[2 * j] * 16, yj = p[2 * j + 1] * 16;
        if (loop) {
            if (gd_near(X, Y, xi, yi, xj, yj))
                return 1;
        } else if ((yi > Y) != (yj > Y) && X < (xj - xi) * (Y - yi) / (yj - yi) + xi)
            in ^= 1;
    }
    return in;
}
/* the colour (GD_C index) of shapes s[0 .. n - 1] at X, Y (Q4 from the foot), -1 none */
static int32_t gd_prims(const gd_prim_t *s, uint32_t n, int32_t X, int32_t Y)
{
    while (n--) {
        const gd_prim_t *q = &s[n];
        int hit;
        if (q->k <= GP_LOOP)
            hit = gd_in_poly(X, Y, GD_PT + 2 * q->a, q->b, q->k == GP_LOOP);
        else if (q->k == GP_RECT)
            hit = X >= q->a * 16 && X < (q->a + q->w) * 16 && Y >= q->b * 16 && Y < (q->b + q->h) * 16;
        else if (q->k == GP_SEG)
            hit = gd_near(X, Y, q->a * 16, q->b * 16, q->w * 16, q->h * 16);
        else
            hit = (X - q->a * 16) * (X - q->a * 16) + (Y - q->b * 16) * (Y - q->b * 16) <= q->w * q->w * 16;
        if (hit)
            return q->c;
    }
    return -1;
}
/* the windmill's moving parts at X, Y (Q4 from its foot): the hub, the sails, the vane; -1 none, -2 the hub */
static int32_t gd_sails(int32_t X, int32_t Y)
{
    int32_t k, hx = X, hy = Y + 60 * 16, r = 64 + 32 * gd.e / 256;
    uint32_t rot = gd.b >> 3;                          /* (a quarter turn every two beats) */
    if (hx * hx + hy * hy <= r * r)
        return -2;
    if (hx * hx + hy * hy > 840 * 840)                 /* (past the longest sail and the vane) */
        return -1;
    for (k = 3; k >= 0; k--) {
        uint32_t a = rot + (uint32_t)k * 16384u;
        int32_t c = gd_s(a + 16384u), s = gd_s(a), len = ((k & 1) ? 38 * 16 : 46 * 16) + 48 * gd.e / 256;
        int32_t u = (hx * c + hy * s) / 256, v = (hy * c - hx * s) / 256;   /* (into the sail: along it, across it) */
        if (u < 0 || u > len + 16 || v < -88 || v > 88)
            continue;
        if (u < 24 || u > len - 8 || v < -40 || v > 40 || ((u + 24) % 112 < 48 && u > 88))
            return (k & 1) ? GC_BLUE : GC_BURNT;       /* (its frame and its rungs, every 7 px) */
        return (k & 1) ? GC_PBLUE : GC_SURF;
    }
    if (X > -176 && X < 80 && Y > -76 * 16 && Y < -60 * 16) {   /* the vane: a tail that swings with the wind (+-0.5 rad) */
        uint32_t va = (uint32_t)(5215 * gd_s(gd.t * 7u) / 256);
        int32_t c = gd_s(va + 16384u), s = gd_s(va), vy = Y + 68 * 16, u = (X * c + vy * s) / 256, v = (vy * c - X * s) / 256;
        if (gd_in_poly(u, v, GD_PT + 80, 3, 0) || gd_in_poly(u, v, GD_PT + 86, 3, 0))
            return GC_INK;
    }
    return -1;
}
/* the house (mill 0) or the windmill (1) standing at fx, its foot on row fr (px, on the 3 px grid, so the blocks keep
 * their colours as it heaves): its blocks, columns x0b .. (ncol of them), rows fr + 3 j for j jlo .. 0. What stands
 * still is worked out once (gd_map, .pool), the sails, the hub and the vane every time */
static int8_t gd_map[2][39 * 24] __attribute__((section(".pool")));
static uint8_t gd_mapped;
static void gd_object(int mill, int32_t fx, int32_t fr, int32_t x0b, int32_t ncol, int32_t jlo)
{
    int32_t i, j;
    if (!(gd_mapped & (1u << mill))) {
        gd_mapped |= (uint8_t)(1u << mill);
        for (j = jlo; j <= 0; j++)
            for (i = 0; i < ncol; i++) {
                int32_t X = (x0b + 3 * i + 1 - fx) * 16 + 8, Y = (3 * j + 1) * 16 + 8;
                gd_map[mill][(j - jlo) * ncol + i] = (int8_t)(1 + (mill ? gd_prims(GD_MILL, NELEM(GD_MILL), X, Y) :
                                                                   gd_prims(GD_HOUSE, NELEM(GD_HOUSE), X, Y)));
            }
    }
    for (j = jlo; j <= 0; j++) {
        int32_t by = fr + 3 * j;
        if (by + 3 <= gd.y0 || by >= gd.y1)
            continue;
        for (i = 0; i < ncol; i++) {
            int32_t bx = x0b + 3 * i, c = mill ? gd_sails((bx + 1 - fx) * 16 + 8, (3 * j + 1) * 16 + 8) : -1;
            if (c == -2) {
                cv_rect(bx - gd.x0, by, 3, 3, ux_mix(GD_BURNT, GD_ORANGE, gd.e * 100 / 256));
                continue;
            }
            if (c < 0)
                c = gd_map[mill][(j - jlo) * ncol + i] - 1;
            if (c >= 0)
                cv_rect(bx - gd.x0, by, 3, 3, GD_C[c]);
        }
    }
}

/* (Phase 3) a pebble in the canvas being drawn (its own coordinates): x, y, w x h, a little uneven (seed k), filled c */
static void gd_pebble(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t k, uint16_t c)
{
    static const int8_t P[16] = {0, 6, 3, 0, 9, 1, 15, 0, 16, 9, 13, 16, 6, 15, 1, 16};   /* (sixteenths of w, h) */
    int32_t pt[16], i;
    for (i = 0; i < 8; i++) {
        int32_t j = (int32_t)(gd_h((int32_t)k, i) & 3u) - 1;   /* (-1 .. 2 px: no two pebbles alike) */
        pt[2 * i] = (x * 16 + P[2 * i] * w) + (i & 1 ? j * 8 : 0);
        pt[2 * i + 1] = (y * 16 + P[2 * i + 1] * h) + (i & 1 ? 0 : j * 8);
    }
    gd.x0 = 0; gd.y0 = 0; gd.y1 = (int32_t)cv_h;
    gd_poly(pt, 8, 1, c);
}
/* (Phase 3) a knob's pod: an acorn the card's size at x (its cap rows 0..10 burnt, under the label; its body mid-soil, the
 * knob turned last orange, under the value and the gauge). What lies under the value */
static uint16_t gd_pod(int32_t x, int32_t h, int hot)
{
    uint16_t pc = hot ? GD_ORANGE : 0xDE14u;            /* (dirt-lt: the soil's lightest, matte) */
    cv_rrect(x + 3, 5, CARD_W - 6, h - 5, 12, GD_BURNT, 0xC50Fu);
    cv_rrect(x + 4, 6, CARD_W - 8, h - 7, 11, pc, GD_BURNT);
    cv_rrect(x, 0, CARD_W, 11, 5, GD_BURNT, 0xC50Fu);   /* (rows 0..10: the value's strip, from row 11, the body's) */
    cv_rect(x + 3, 9, CARD_W - 6, 2, GD_BURNT);         /* (the cap's lip: square under, round over) */
    return pc;
}
/* a panel's burnt edge (1 px, its corners round, r 7) on the canvas's BG: the ring only, not the whole card again */
static void gd_ring(int32_t x, int32_t y, int32_t w, int32_t h)
{
    uint32_t i;
    for (i = 0; i < 4u; i++) {
        int32_t cx = i & 1u ? x + w - 14 : x, cy = i & 2u ? y + h - 14 : y;
        cv_rrect(cx, cy, 14, 14, 7, GD_BURNT, GD_PBLUE);
        cv_rrect(cx + 1, cy + 1, 12, 12, 6, T_BG, GD_BURNT);
        cv_rect(i & 1u ? cx : cx + 7, cy, 7, 14, T_BG);   /* (the circle's halves inside the card: its corner's quarter left) */
        cv_rect(cx, i & 2u ? cy : cy + 7, 14, 7, T_BG);
    }
    cv_rect(x + 7, y, w - 14, 1, GD_BURNT);
    cv_rect(x + 7, y + h - 1, w - 14, 1, GD_BURNT);
    cv_rect(x, y + 7, 1, h - 14, GD_BURNT);
    cv_rect(x + w - 1, y + 7, 1, h - 14, GD_BURNT);
}
/* (Phase 3) the garden round a page's panel (the graph strip at gy, gh high: 3 px each side, a row over and under it, its
 * corners): still on the pages, so worked out once with the frame (keep) and laid into each redraw of the panel (paste).
 * gd_edged: kept for that strip (0: none, the scene whole under the panel) */
#define GD_EDGE_N (3u * 2u * 150u + 234u * 2u + 4u * 49u)
static uint16_t gd_edge[GD_EDGE_N] __attribute__((section(".pool")));
static void gd_scene(int32_t x0, int32_t y0);
static void gd_edges(int32_t gy, int32_t gh, int paste)
{
    const int32_t r[8][4] = {{0, 0, 3, gh}, {237, 0, 3, gh}, {3, 0, 234, 1}, {3, gh - 1, 234, 1},
                             {3, 1, 7, 7}, {230, 1, 7, 7}, {3, gh - 8, 7, 7}, {230, gh - 8, 7, 7}};
    uint32_t i, k, n = 0;
    for (i = 0; i < 8u; i++) {
        uint32_t w = (uint32_t)r[i][2], m = w * (uint32_t)r[i][3];
        if (!paste) {
            cv_begin(w, (uint32_t)r[i][3], T_BG);
            gd_scene(r[i][0], gy + r[i][1]);
        }
        for (k = 0; k < m && n < GD_EDGE_N; k++, n++)
            if (paste)
                cv_px[((uint32_t)r[i][1] + k / w) * 240u + (uint32_t)r[i][0] + k % w] = gd_edge[n];
            else
                gd_edge[n] = cv_px[k];
    }
    gd_edged = paste ? gd_edged : gy * 256 + gh;
}
/* ---- (Phase 4) the page scenes: plants on the garden in the panel's place (mockups/pages.html), drawn 2 px blocky.
 * ENV a tulip growing through the envelope; LFO a row of tulips the LFO sways; FX the stream, its fruit hopping on the
 * beat, its flow the sends; SEQ > PATTERN the bed: a mound a step, a tulip on a note, a seed on a rest, the playhead's
 * mound orange. The sky stays still behind them (the strips round it agree); the plants run on their own clock */
enum { GS_NONE, GS_ENV, GS_LFO, GS_FX, GS_SEQ };
static const uint8_t GS_TOP[5] = {0, 84, 110, 104, 84};   /* the rows the plants can reach: redrawn each frame */
static uint32_t gd_page(void)
{
    const page_t *pg = cur_page();
    if (!gd_back() || ui.layer || ui.menu || large_kind() != LK_OFF || ui.home)
        return GS_NONE;
    return pg->graph == GR_ADSR && pg->id[0] == P_ATK ? GS_ENV : pg->graph == GR_LFO ? GS_LFO : pg->graph == GR_FX ? GS_FX :
           pg->graph == GR_STEPS ? GS_SEQ : GS_NONE;
}
/* a leaf at x, y (Q4), pointing at a (65536 a turn), L long and W wide (px, Q4 / 16) */
static void gd_leaf(int32_t x, int32_t y, uint32_t a, int32_t L, int32_t W, uint16_t c)
{
    static const int8_t LF[12] = {0, 0, 5, -16, 12, -14, 16, 0, 12, 14, 5, 16};   /* (sixteenths of L, W) */
    int32_t pt[12], i, cs = gd_s(a + 16384u), sn = gd_s(a);
    if (L < 32)
        return;
    for (i = 0; i < 6; i++) {
        int32_t u = LF[2 * i] * L / 16, v = LF[2 * i + 1] * W / 16;
        pt[2 * i] = x + (u * cs - v * sn) / 256;
        pt[2 * i + 1] = y + (u * sn + v * cs) / 256;
    }
    gd_poly(pt, 6, 2, c);
}
/* a wobbly round thing at x, y (px), r (Q4), n points, stretched sx, sy (Q8) */
static void gd_blob(int32_t x, int32_t y, int32_t r, uint32_t n, uint16_t c, uint32_t sd, int32_t sx, int32_t sy)
{
    int32_t pt[20], i;
    for (i = 0; i < (int32_t)n; i++) {
        uint32_t a = (uint32_t)i * 65536u / n;
        int32_t k = 256 + (36 * gd_s(a * 3u + sd * 4096u) >> 8);
        pt[2 * i] = x * 16 + (r * k >> 8) * sx / 256 * gd_s(a + 16384u) / 256;
        pt[2 * i + 1] = y * 16 + (r * k >> 8) * sy / 256 * gd_s(a) / 256;
    }
    gd_poly(pt, n, 2, c);
}
/* a tulip from its foot x, by (px): stem h px, its tip bent sw (Q4), nl leaves (each side its own length, by seed sd),
 * the cup open 0..256, all at scale sc (Q8) */
static void gd_tulip(int32_t x, int32_t by, int32_t h, int32_t sw, int32_t nl, int32_t open, uint32_t sd, int32_t sc, uint16_t col)
{
    int32_t X0 = x * 16, Y0 = by * 16, CX = X0 + sw, CY = Y0 - h * 8, X1 = X0 + sw * 3 / 10, Y1 = Y0 - h * 16, s, X, Y, i, w, top;
    int32_t pt[10];
    for (s = 0; s <= 256; s += 384 / (h + 2) + 1) {    /* the stem: a quadratic curve, 2 px */
        int32_t u = 256 - s;
        X = (u * u * X0 + 2 * u * s * CX + s * s * X1) >> 16;
        Y = (u * u * Y0 + 2 * u * s * CY + s * s * Y1) >> 16;
        cv_rect((X >> 4) - gd.x0, Y >> 4, 2, 2, GD_INK);
    }
    for (i = 0; i < nl; i++) {                          /* the leaves, up the stem, blue left and burnt right */
        int32_t u, sv = 77 + i * 56, lL = (5 + (int32_t)(gd_h(sd * 5u + (uint32_t)i * 3u, 1) >> 13)) * sc / 16,
                lR = (5 + (int32_t)(gd_h(sd * 7u + (uint32_t)i * 5u, 2) >> 13)) * sc / 16;
        uint32_t wv = (uint32_t)(sw * 40);
        u = 256 - sv;
        X = (u * u * X0 + 2 * u * sv * CX + sv * sv * X1) >> 16;
        Y = (u * u * Y0 + 2 * u * sv * CY + sv * sv * Y1) >> 16;
        gd_leaf(X, Y, 49152u - 9387u - wv, lL, 4 * sc / 16, GD_BLUE);
        gd_leaf(X, Y, 49152u + 6258u - wv, lR, 4 * sc / 16, GD_BURNT);
    }
    w = 4 * sc * (128 + open / 2) / 256 / 16;           /* the cup (px x 16) */
    top = Y1 - 6 * sc * (128 + open / 2) / 256 / 16;
    pt[0] = X1 - w; pt[1] = Y1;
    pt[2] = X1 - w * 6 / 5; pt[3] = top;
    pt[4] = X1; pt[5] = top - w * 3 / 4;
    pt[6] = X1 + w * 6 / 5; pt[7] = top;
    pt[8] = X1 + w; pt[9] = Y1;
    gd_poly(pt, 5, 2, col);
    pt[0] = X1 - w * 2 / 5; pt[2] = X1; pt[3] = top + 16; pt[4] = X1 + w * 2 / 5; pt[5] = Y1;
    gd_poly(pt, 3, 2, GD_ORANGE);
}
/* a scalloped dirt mound: a bed with a bumpy top, no box (px) */
static void gd_mound(int32_t x, int32_t y, int32_t w, uint16_t c)
{
    int32_t pt[30], i, h = w * 141;                     /* (Q8: 0.55 w) */
    for (i = 0; i <= 12; i++) {
        uint32_t a = 32768u - (uint32_t)i * 32768u / 12u;
        pt[2 * i] = x * 16 + w * gd_s(a + 16384u) / 16;
        pt[2 * i + 1] = y * 16 - (h * gd_s(a) >> 12) * (256 + (36 * gd_s((uint32_t)i * 23986u) >> 8)) / 256;
    }
    pt[26] = x * 16 + w * 16; pt[27] = y * 16;
    pt[28] = x * 16 - w * 16; pt[29] = y * 16;
    gd_poly(pt, 15, 2, c);
}
static int32_t gd_hop(uint32_t k)                       /* a hop on the beat, k Q8: |sin(beat pi)|^2 (1.5 + 7 e) k (px) */
{
    int32_t s = gd_s(gd_pl.b >> 1);
    return s * s / 256 * (384 + 7 * gd_pl.e) / 256 * (int32_t)k / 65536;
}
static void gd_plants(uint32_t sc)
{
    const track_t *t = TSEL;
    int32_t i, e = gd_pl.e;
    if (sc == GS_ENV) {                                 /* the tulip the envelope grows: rising on the attack, its leaves
                                                         * on the decay, held on the sustain, drooping on the release */
        int32_t lvl = -1, st = 0, h;
        for (i = 0; i < NVOICE; i++)
            if (t->v[i].stage && (int32_t)(((int64_t)t->v[i].env * 1000) >> 24) > lvl)
                lvl = (int32_t)(((int64_t)t->v[i].env * 1000) >> 24), st = t->v[i].stage;
        if (lvl < 0)                                    /* (resting: a sprout as high as the sustain) */
            lvl = 150 + t->p[P_SUS] * 250 / 127, st = 0;
        h = (16 + lvl * 40 / 1000) * 3 / 2;
        gd_tulip(120, gd_gy(120) / 16 + 1, h, (gd_s(gd_pl.t * 20u) * (16 + e / 8)) >> 8, st == 1 ? 1 : st == 2 ? 3 : 4,
                 lvl * 256 / 1000, 1, st >= 4 ? 300 : 384, GD_BURNT);
    } else if (sc == GS_LFO) {                          /* the wind: the LFO's value bends every tulip */
        int32_t w = t->lfo_val * 160 >> 15;
        for (i = 0; i < 6; i++) {
            int32_t x = 20 + i * 40;
            uint32_t ph = (gd_pl.b / 16u + (uint32_t)i * 8978u) & 65535u;   /* (16 beats a life, each its own start) */
            gd_tulip(x, gd_gy(x) / 16 + 1, 16 + (int32_t)(ph * 40u >> 16), w + (gd_s(gd_pl.t * 20u + (uint32_t)i * 9000u) * 8 >> 8),
                     1 + (int32_t)(ph >> 14), (int32_t)(ph >> 8), (uint32_t)x, 230, i & 1 ? GD_BLUE : GD_BURNT);
        }
    } else if (sc == GS_FX) {                           /* the stream: its ripples run with the sends and the level */
        int32_t amp = 16 + (t->p[P_DIST] + t->p[P_CHOR] + t->p[P_DLY] + t->p[P_REV]) / 8 + e / 5, r, x;
        for (r = 0; r < 2; r++)
            for (x = 0; x < 240; x += 2)
                cv_rect(x - gd.x0, 146 + r * 8 + (gd_s((uint32_t)x * 626u - gd_pl.t * (uint32_t)(30 + 10 * r)) * amp >> 12), 2, 2,
                        r ? GD_BLUE : GD_PBLUE);
        gd_blob(50, 140 - gd_hop(102), 128, 7, GD_BURNT, 1, 256, 282);   /* the apple, */
        gd_leaf(52 * 16, (133 - gd_hop(102)) * 16, 49152u - 6000u, 128, 48, GD_BLUE);
        gd_blob(130, 140 - gd_hop(141), 128, 9, GD_ORANGE, 3, 256, 256);  /* the orange, */
        gd_blob(195, 143 - gd_hop(179), 67, 5, GD_BURNT, 5, 256, 256);   /* the cherries */
        gd_blob(205, 145 - gd_hop(179), 67, 5, GD_BURNT, 6, 256, 256);
    } else if (sc == GS_SEQ) {                          /* the bed: the bank shown, 2 rows of 8 */
        uint32_t ph = song.playing ? seq_src(t, t->seq_idx) : 0xFFFFu;
        for (i = 0; i < 16; i++) {
            uint32_t si = ui.bank * 16u + (uint32_t)i;
            int32_t x = 18 + (i & 7) * 29, y = 136 + (i >> 3) * 36, hot = si == ph;
            const step_t *st = &seq_steps(t)[si];
            if (si >= (uint32_t)t->p[P_SLEN])
                continue;
            gd_mound(x, y + 8, 11, hot ? GD_ORANGE : 0xDE14u);
            if (step_on(st))
                gd_tulip(x, y + 6, hot ? 22 : 14, (gd_s(gd_pl.t * 20u + (uint32_t)i * 5000u) * 6) >> 8, 1, hot ? 256 : 96, (uint32_t)i,
                         hot ? 180 : 120, hot ? GD_BLUE : GD_BURNT);
            else
                cv_rect(x - 1 - gd.x0, y + 4, 2, 2, GD_SURF);
        }
    }
}
/* the soil's cell at x, y (3 px; y from 171): two octaves of noise, low contrast, a checker's nudge (Q8; without the
 * depth's darkening). Static: kept once worked out (.pool, 1.8 KB) */
#define GD_SOIL_Y 171
static uint8_t gd_soil_c[80 * 23] __attribute__((section(".pool")));
static int32_t gd_soil(int32_t x, int32_t y)
{
    uint8_t *c = y >= GD_SOIL_Y && y < 240 ? &gd_soil_c[(y - GD_SOIL_Y) / 3 * 80 + x / 3] : 0;
    int32_t n;
    if (c && *c)
        return *c - 1;
    n = 128 + 141 * ((gd_vn(x * 128 / 5, y * 128 / 5) * 4 + gd_vn(x * 256 / 5 + 1792, y * 256 / 5 + 768)) / 5 - 128) / 256;
    n = n * 4 / 5 + (((x + y) / 3 & 1) ? 4 : -4);
    if (c)
        *c = (uint8_t)(clamp(n, 0, 254) + 1);
    return n;
}
/* the scene into the canvas being drawn (cv_begin), which lies at x0, y0 on the screen */
static void gd_scene(int32_t x0, int32_t y0)
{
    static const uint8_t B4[16] = {0, 8, 2, 10, 12, 4, 14, 6, 3, 11, 1, 9, 15, 7, 13, 5};   /* (Bayer 4 x 4) */
    int32_t x, y, i, e = gd.e, pt[32];
    uint32_t t = gd.t;
    gd.x0 = x0;
    gd.y0 = y0;
    gd.y1 = y0 + (int32_t)cv_h;
    cv_oy = -y0;
    for (y = gd.y0 & ~1; y < gd.y1 && y < 192; y += 2)   /* 1. the sky */
        cv_rect(0, y, (int32_t)cv_w, 2, gd_sky(y));
    {                                                  /* the sun: a 16-point star, its long points pulsing */
        int32_t sr = 192 + 32 * e / 256;
        for (i = 0; i < 16; i++) {
            uint32_t a = (uint32_t)i * 4096u + t * 2u;
            int32_t r = (i & 1) ? sr * 7 / 10 : sr * 5 / 4 + (48 * e >> 8) * gd_s(t * 83u + (uint32_t)i * 10430u) / 256;
            pt[2 * i] = 3136 + r * gd_s(a + 16384u) / 256;
            pt[2 * i + 1] = 544 + r * gd_s(a) / 256;
        }
        gd_poly(pt, 16, 2, GD_ORANGE);
        for (i = 0; i < 8; i++) {                      /* its core */
            uint32_t a = (uint32_t)i * 8192u + t;
            int32_t r = sr * 6 / 10 + (int32_t)(gd_h(i, 4) & 15u) - 8;
            pt[2 * i] = 3136 + r * gd_s(a + 16384u) / 256;
            pt[2 * i + 1] = 544 + r * gd_s(a) / 256;
        }
        gd_poly(pt, 8, 2, GD_BURNT);
    }
    for (y = 150; y < 186; y += 3)                     /* 2. the horizon's dots, denser toward the ground */
        if (y + 2 > gd.y0 && y < gd.y1)
            for (x = 0; x < 240; x += 3)
                if ((int32_t)B4[(y / 3 & 3) * 4 + (x / 3 & 3)] * 16 * 36 < (y - 150) * (128 + 77 * e / 256))
                    cv_rect(x - x0, y, 2, 2, GD_PBLUE);
    for (i = 0; i < 4; i++) {                          /* 3. the clouds: a base and 2..4 round puffs on the 3 px grid */
        uint32_t sd = (uint32_t)i * 31u + 1u;
        int32_t sc = 179 + (int32_t)(gd_h(i, 9) * 154u >> 16);   /* (0.7 .. 1.3, Q8) */
        int32_t cx = (int32_t)((t * (48u + (gd_h(i, 3) * 112u >> 16)) / 1024u + (gd_h(i, 7) * 320u >> 16) * 16u) % 5440u) - 800;
        int32_t cy = 288 + (int32_t)(gd_h(i, 5) * 800u >> 16);   /* (Q4: x -50 .. 290, y 18 .. 68) */
        int32_t w = (640 + (int32_t)(gd_h(sd, 0) * 480u >> 16)) * sc >> 8, h = (224 + (int32_t)(gd_h(sd, 1) * 96u >> 16)) * sc >> 8;
        int32_t nb = 2 + (int32_t)(gd_h(sd, 2) * 3u >> 16), bxy[12], j, bx, by;
        if (cy - h * 2 > gd.y1 * 16 || cy + h < gd.y0 * 16)
            continue;
        for (j = 0; j < nb; j++) {
            int32_t f = j * 256 / (nb - 1) - 128;      /* (-0.5 .. 0.5, Q8) */
            bxy[3 * j] = cx + f * w * 7 / 2560 + ((int32_t)(gd_h(sd + 3u * (uint32_t)j, 6) >> 8) - 128) * w / 3200;
            bxy[3 * j + 1] = cy - h / 5 - (128 - (f < 0 ? -f : f)) * h * 3 / 1280;
            bxy[3 * j + 2] = (h * 45 / 100 + (int32_t)(gd_h(sd + 5u * (uint32_t)j, 8) * 35u >> 16) * h / 100) * (256 + 8 * e / 256) >> 8;
        }
        for (by = gd_fl((cy - h * 6 / 5) >> 4, 3); by * 16 <= cy + h / 2; by += 3) {
            int32_t Y = (by + 1) * 16 + 8, edge;
            if (by + 3 <= gd.y0 || by >= gd.y1)
                continue;
            for (bx = gd_fl((cx - w / 2) >> 4, 3); bx * 16 <= cx + w / 2 + 48; bx += 3) {
                int32_t X = (bx + 1) * 16 + 8, in = X >= cx - w / 2 && X <= cx + w / 2 && Y >= cy - h / 2 && Y <= cy + h / 2;
                for (j = 0; j < nb && !in; j++)
                    in = (X - bxy[3 * j]) * (X - bxy[3 * j]) + (Y - bxy[3 * j + 1]) * (Y - bxy[3 * j + 1]) <=
                         bxy[3 * j + 2] * bxy[3 * j + 2];
                if (!in)
                    continue;
                edge = cy + h / 20 + (gd_vn(bx * 256 / 9, (int32_t)sd * 95) - 128) * h * 11 / 2560;
                cv_rect(bx - x0, by, 3, 3, Y > edge ? GD_SURF : T_BG);
            }
        }
    }
    if (gd.y0 < 192) {                                 /* 4. the rain: the level sets how many (23 of 130 .. all), the tempo how fast */
        uint32_t thr = 11598u + 53938u * (uint32_t)e / 256u;
        for (i = 0; i < 130; i++) {
            uint32_t r = gd_h(i, 13);
            int32_t len, ry, rx;
            if (gd_h(i, 17) >= thr)
                continue;
            len = 4 + (int32_t)(gd_h(i, 7) >> 14);
            ry = (int32_t)(((gd_h(i, 5) * 186u >> 8) + (gd.b >> 7) * (70u + (gd_h(i, 3) * 60u >> 16)) / 4u)   /* (the beats: a tempo change moves no drop) */ % (186u * 256u) >> 8);
            rx = (int32_t)(gd_h(i, 2) * 250u >> 16) - 5 - ry * 12 / 100;
            if (ry + len < gd.y0 || ry >= gd.y1)
                continue;
            cv_line(rx - x0, ry, rx - len / 4 - x0, ry + len, ux_mix(gd_sky(ry), (gd_h(i, 11) & 1u) ? GD_BLUE : GD_PBLUE,
                                                                    45 + 55 * e / 256 * (int32_t)(r >> 8) / 256));
        }
    }
    if (gd.y1 > 132 && gd.y0 < 195)                     /* 5. the house (3 px blocks) */
        gd_object(0, 196, gd_fl((gd_gy(196) >> 4) + 2, 3), 168, 19, -14);
    if (gd.y1 > 72 && gd.y0 < 195)                      /* 6. the windmill, its sails running off the left edge */
        gd_object(1, 20, gd_fl((gd_gy(20) >> 4) + 2, 3), 0, 24, -38);
    if (t / 1024u % 50u > 25u)                          /* 7. the fence, 25 s of every 50 */
        for (x = 74; x < 126; x++) {
            y = (gd_gy(x) >> 4) + 1;
            cv_rect(x - x0, y - 5, 1, 1, GD_BURNT);
            if (!((x - 74) & 7))
                cv_rect(x - x0, y - 7, 2, 7, GD_BURNT);
        }
    if (gd.y1 < 160)
        goto done;
    for (x = 0; x < 240; x += 3) {                     /* 8. the ground: soil in five tones, matte */
        int32_t gy = gd_gy(x), top = gd_fl(gy >> 4, 3), r;
        for (y = top > gd_fl(gd.y0, 3) ? top : gd_fl(gd.y0, 3); y < gd.y1; y += 3) {
            int32_t d = y * 16 - gy, v = gd_soil(x, y);
            v += d <= 0 ? 0 : d >= 640 ? 51 : d * 51 / 640;
            cv_rect(x - x0, y, 3, 3, GD_C[GD_SOIL[clamp(v * 5 >> 8, 0, 4)]]);
        }
        r = gd_vn(x * 64, 2304);                       /* the grass: one row of patches, a second here and there */
        if (r >= 64) {
            cv_rect(x - x0, top, 3, 3, GD_C[r < 154 ? GC_GMID : GC_GLIGHT]);
            if (r > 205)
                cv_rect(x - x0, top + 3, 3, 3, GD_C[GC_GMID]);
        }
    }
    for (x = 6; x < 240; x += 18) {                    /* the tufts, bouncing with the level */
        int32_t top = gd_fl(gd_gy(x) >> 4, 3), s = gd_s(t * 61u + (uint32_t)x * 3129u), hg;
        hg = 1 + (2 * e * (s < 0 ? -s : s) / 256 + 128) / 256;
        if (gd_h(x, 23) & 1u) {                        /* a dirt-dk stem with a blue head */
            cv_rect(x + 3 - x0, top - 3 * (hg + 2), 3, 3 * (hg + 2), GD_C[GC_DIRTDK]);
            cv_rect(x - x0, top - 3 * (hg + 2), 9, 3, GD_BLUE);
            cv_rect(x + 3 - x0, top - 3 * (hg + 3), 3, 3, GD_BLUE);
        } else {
            cv_rect(x - x0, top - 3 * hg, 3, 3 * hg, GD_BLUE);
            cv_rect(x + 3 - x0, top - 3 * (hg + 1), 3, 3 * (hg + 1), GD_BLUE);
            cv_rect(x + 6 - x0, top - 3 * hg, 3, 3 * hg, GD_BLUE);
        }
    }
done:
    cv_oy = 0;
}
