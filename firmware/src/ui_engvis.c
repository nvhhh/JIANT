/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments
 * Copyright (C) 2026 Juan Ignacio Antonucci (Juan Jiant) */
/* (GARDEN, Phase 5) the engines' creatures (docs/garden-handoff mockups/creatures.html): each engine's EDIT / BEING
 * picture and its HOME / ECO one, drawn by code on the garden's primitives (ui_garden.c: 2 px blocky polygons, leaves,
 * blobs), on the plants' clock (MOTION x the music's level). DRUM a kit of its eight pieces, each moving with its own
 * lane's hit only; VOICE a singing flower; ANIMAL a ladybird; LOFI an apple orbiting its dotted path; ANALOG an
 * orange being with a face; FM6 five fuzzy flowers; PHASE a black hole and its rings; WHEEL a barn wheel rolling; NOISE
 * a galaxy, its sun and planets; SERUM pixel bushes. Engines without their own (DIGITAL): ANIMAL's ladybird.
 * The pages: DRUM-X's KIT 2 / KIT 3 / SOUND the kit's eight pieces, each moving with its lane (vx_kit_page); FM6's
 * operator pages the fern, a frond pair an operator, its length the operator's level (vx_fm6_page, vx_fm6env).
 * (Retired with Phase 5: the 0.9.4 .. 0.9.12 creatures, the tract, the kit's beings, the colony's FM6, MOTION's
 * jellyfish.) */
enum { VK_KICK, VK_SNARE, VK_CLAP, VK_HATC, VK_HATO, VK_TOM, VK_RIM, VK_BELL };
/* (ECO's isotherms until Phase 6 replaces them) */
static uint16_t vk_heat(int32_t h) { return bg_col(clamp(h, 0, 255)); }
#define GD_DIRTDK 0xA42Du                                /* #A6876A */
#define GD_MID 0x6AC9u                                   /* #6B5A4C, the faint dotted path */

/* the mockup's 240 x 240 space about (120, 125) on the canvas: centre (Q4 canvas px) and scale (Q8) */
static struct { int32_t cx, cy, s; } cr;
static struct { uint8_t age[8], last; uint32_t hitm[8]; int32_t en[NTRK]; } vxs __attribute__((section(".pool")));
static int32_t CX(int32_t u) { return cr.cx + (u - 1920) * cr.s / 256; }   /* (u, v: the mockup's px x 16) */
static int32_t CY(int32_t v) { return cr.cy + (v - 2000) * cr.s / 256; }
static int32_t CR(int32_t r) { return r * cr.s / 256; }
static int32_t vx_s(uint32_t ph, int32_t a) { return gd_s(ph) * a / 256; }   /* sin (65536 a turn) x a */
static uint32_t vx_t(uint32_t k) { return gd_pl.t * k / 10u; }   /* the phase of sin(t k / 100), t in seconds */
static void cr_blob(int32_t u, int32_t v, int32_t r, uint32_t n, uint16_t c, uint32_t sd, int32_t sx, int32_t sy)
{
    gd_blob(CX(u), CY(v), CR(r), n, c, sd, sx, sy);
}
static void cr_leaf(int32_t u, int32_t v, uint32_t a, int32_t L, int32_t W, uint16_t c)
{
    gd_leaf(CX(u), CY(v), a, CR(L), CR(W), c);
}
/* a line w thick (all Q4, the mockup's) */
static void cr_line(int32_t u0, int32_t v0, int32_t u1, int32_t v1, int32_t w, uint16_t c)
{
    int32_t x0 = CX(u0), y0 = CY(v0), dx = CX(u1) - x0, dy = CY(v1) - y0, n = (dx < 0 ? -dx : dx) > (dy < 0 ? -dy : dy) ?
            (dx < 0 ? -dx : dx) : (dy < 0 ? -dy : dy), i, p = CR(w) >> 4;
    n = n / 16 + 1;
    p = p < 1 ? 1 : p;
    for (i = 0; i <= n; i++)
        cv_rect(((x0 + dx * i / n) >> 4) - p / 2, ((y0 + dy * i / n) >> 4) - p / 2, p, p, c);
}
static void cr_rect(int32_t u, int32_t v, int32_t w, int32_t h, uint16_t c)
{
    int32_t p = CR(w) >> 4, q = CR(h) >> 4;
    cv_rect(CX(u) >> 4, CY(v) >> 4, p < 1 ? 1 : p, q < 1 ? 1 : q, c);
}
/* an ellipse's arc a0 .. a1 (65536 a turn) about u, v, rx ry (Q4), turned by rot */
static void cr_arc(int32_t u, int32_t v, int32_t rx, int32_t ry, uint32_t rot, uint32_t a0, uint32_t a1, int32_t w, uint16_t c)
{
    int32_t i, px = 0, py = 0, cs = gd_s(rot + 16384u), sn = gd_s(rot);
    for (i = 0; i <= 12; i++) {
        uint32_t a = a0 + (a1 - a0) / 12u * (uint32_t)i;
        int32_t x = vx_s(a + 16384u, rx), y = vx_s(a, ry), X = u + (x * cs - y * sn) / 256, Y = v + (x * sn + y * cs) / 256;
        if (i)
            cr_line(px, py, X, Y, w, c);
        px = X;
        py = Y;
    }
}
static void cr_quad(const int32_t *q, uint16_t c)       /* 4 points (the mockup's Q4) */
{
    int32_t pt[8], i;
    for (i = 0; i < 4; i++)
        pt[2 * i] = CX(q[2 * i]), pt[2 * i + 1] = CY(q[2 * i + 1]);
    gd_poly(pt, 4, 2, c);
}
static int32_t vx_beat(void) { return (int32_t)((gd_pl.b >> 8) & 255u); }   /* the beat's fraction (Q8) */
/* (the owner on the device) a part's size, Q8: swelling on the pulse (gd_pulse: every den / num beats, off along; the
 * slow breath when stopped) by a0 + a1 x the level (both Q8 of the size) */
static int32_t vx_sw(uint32_t num, uint32_t den, uint32_t off, int32_t a0, int32_t a1, int32_t e)
{
    int32_t a = a0 + a1 * e / 256;
    if (gd_run)                                          /* (the owner: quiet music a quarter less, loud as it was) */
        a = a * (192 + clamp(e, 0, 256) / 4) / 256;
    return 256 + gd_pulse(num, den, off) * a / 256;
}
/* a stem of pixel blocks from v0 down to v1 at u, a little uneven (seed sd), w wide, its top swayed sw (all Q4) */
static void vx_stem(int32_t u, int32_t v0, int32_t v1, int32_t w, int32_t sw, uint32_t sd)
{
    int32_t v, i;
    for (v = v0, i = 0; v < v1; v += 64, i++) {
        int32_t h = (int32_t)gd_h(i / 2 + (int32_t)sd * 64, 31), x = u + ((h >> 13 & 3) == 3 ? w / 2 : (h >> 13 & 3) == 0 ? -w / 2 : 0) +
                                                                   sw * (v1 - v) / (v1 - v0 + 1), x0 = CX(x - w / 2) >> 4, y0 = CY(v) >> 4;
        cv_rect(x0, y0, (CX(x + w / 2 + ((h & 1) ? w / 2 : 0)) >> 4) - x0, (CY(v + 64) >> 4) - y0, GD_INK);   /* (each to the next's edge) */
    }
}
static int32_t vx_fall4(int32_t p) { p = 256 - p; p = p * p >> 8; return p * p >> 8; }   /* (1 - p)^4 */
/* each drum lane's level now (0..256), the kit's (a new hit: hitm, last) */
static void vx_lanes(const track_t *t, int32_t *e)
{
    drum_lane_t *K = drum_kit_of(t);
    int32_t i;
    for (i = 0; i < 8; i++) {
        int32_t a = K ? K[i].age : 255;
        e[i] = K && K[i].x.live && !(ui_prefs & PREF_ANIM_OFF) ? clamp(K[i].x.ea >> 22, 0, 256) : 0;
        if (K && a < vxs.age[i] && e[i]) {
            vxs.hitm[i] = fm1_ms;
            vxs.last = (uint8_t)i;
        }
        vxs.age[i] = (uint8_t)a;
    }
}

static void cr_shell(int32_t u, int32_t v, int32_t w, int32_t h, uint16_t c, int32_t hit)
{
    int32_t q[8] = {u - w / 2, v, u + w / 2, v, u + w / 2, v + h, u - w / 2, v + h};
    cr_quad(q, c);
    cr_blob(u, v, w / 2, 8, GD_SURF, 4, 256, 72 + hit / 10);
    cr_line(u - w / 2, v + h, u + w / 2, v + h, 24, GD_BURNT);
}
static void vx_piece(uint32_t l, int32_t e, int mute);
/* (the owner: every piece its own, each moving with its own lane's hit only) the kit's eight pieces in a 4 x 2 grid over
 * the creature's rectangle, each drawn by vx_piece about its slot's centre; the kit swelling on the beat */
static void vx_drum(const track_t *t)
{
    int32_t e[8], i, s0 = cr.s, cx0 = cr.cx, cy0 = cr.cy, sw = vx_sw(1u, 1u, 0u, 16, 0, 0);
    vx_lanes(t, e);
    for (i = 0; i < 8; i++) {
        int32_t u = 600 + (i & 3) * 880, v = 1400 + (i >> 2) * 1200;   /* (the slot's centre, the mockup's units) */
        cr.s = s0 * sw / 256;
        cr.cx = cx0 + (u - 1920) * s0 / 256;
        cr.cy = cy0 + (v - 2000) * s0 / 256;
        vx_piece((uint32_t)i, e[i], dx_lane_muted((uint32_t)i));
    }
    cr.s = s0;
    cr.cx = cx0;
    cr.cy = cy0;
}
static void vx_voice(int32_t e)
{
    int32_t i, gate = clamp((e - 13) * 5, 0, 256), y = 1600 - gd_pulse(1u, 1u, 0u) * 112 / 256;   /* (the head bobbing on the beat) */
    vx_stem(1920, y + 256, 2848, 96, vx_s(vx_t(90), 32), 0u);   /* the stem: pixel blocks, swaying at the head */
    cr_leaf(1920, 2400, 65536u - 8344u, 320, 96, GD_BLUE);
    for (i = 0; i < 7; i++)                              /* the petals, breathing in and out with the level, turning at the tempo */
        cr_leaf(1920, y, (uint32_t)i * 9362u + vx_t(40), 352 + e * 192 / 256, 160, i & 1 ? GD_ORANGE : GD_SURF);
    cr_blob(1920, y, 288, 9, GD_BURNT, 3, 256, 256);
    cr_rect(1792, y - 96, 48, 64, GD_INK);
    cr_rect(1984, y - 96, 48, 64, GD_INK);
    cr_blob(2080, y + 96, 64, 6, GD_INK, 4, 256, 64 + e * 230 / 256);   /* the mouth, open with the level */
    for (i = 0; gate && i < 6; i++) {                    /* the notes leaving it, only with music */
        uint32_t age = (vx_t(70) / 6u + (uint32_t)i * 10923u) & 65535u;   /* (0 .. 1 a turn) */
        int32_t x = 2160 + (int32_t)(age * (uint32_t)(800 + e * 640 / 256) >> 16), ny = y - vx_s(age * 5u / 6u + (uint32_t)i * 10430u, 96) -
                    (int32_t)(age * 224u >> 16);
        if ((int32_t)(age >> 8) < gate) {
            cr_rect(x, ny, 32 + e / 8, 32 + e / 8, GD_PBLUE);
            cr_rect(x + 32 + e / 8, ny - 64, 16, 64, GD_PBLUE);
        }
    }
}
/* a filled ellipse in 2 px blocks about u, v (the mockup's px x 16), rx ry (Q4): the creatures' clean round shapes */
static void cr_ell(int32_t u, int32_t v, int32_t rx, int32_t ry, uint16_t c)
{
    int32_t cx = CX(u) >> 4, cy = CY(v) >> 4, ax = CR(rx) >> 4, ay = CR(ry) >> 4, y;
    if (ax < 1 || ay < 1)
        return;
    for (y = -ay & ~1; y <= ay; y += 2) {
        int32_t yy = y + 1, q = (ay * ay - yy * yy) * ax * ax, w = 0;
        if (q <= 0)
            continue;
        while ((w + 1) * (w + 1) * ay * ay <= q)        /* (w = ax sqrt(1 - yy^2 / ay^2)) */
            w++;
        if (w > 0)
            cv_rect((cx - w) & ~1, cy + y, ((cx + w + 1) & ~1) - ((cx - w) & ~1), 2, c);
    }
}

#define SH_SD 8u                                         /* (the shell's wobble: the seed whose back is round) */
#define VX_RED 0xE187u                                   /* #E0303A ANIMAL's ladybird: its shell */
/* ANIMAL: a ladybird (the owner's picture): a round red shell with black spots, a black head with two white eyes,
 * stubby legs stepping, little antennae; it hops on the beat and swells with the level */
static void vx_beetle(int32_t e)
{
    int32_t s0 = gd_s((uint32_t)vx_beat() << 7), hop = s0 * s0 / 256 * (32 + 64 * e / 256) / 256, y = 2080 - hop, i;
    int32_t sw = vx_s(vx_t(400), 48), sz = vx_sw(1u, 2u, 0u, 12, 0, 0) + e / 8, rx = 640 * sz / 256, ry = 512 * sz / 256;
    for (i = 0; i < 3; i += 2) {                         /* the legs (two: the owner), stepping in turn */
        int32_t lx = 1696 + i * 352, s = i & 1 ? sw : -sw;
        cr_line(lx, y + 384, lx + s / 2, 2736, 96, GD_INK);
        cr_rect(lx + s / 2 - 48, 2704, 112, 48, GD_INK);
    }
    cr_line(1312, y - 160, 1152 + vx_s(vx_t(300), 40), y - 448, 24, GD_INK);   /* the antennae, a dot at each tip */
    cr_line(1408, y - 192, 1376 + vx_s(vx_t(300) + 16384u, 40), y - 496, 24, GD_INK);
    cr_ell(1152 + vx_s(vx_t(300), 40), y - 464, 40, 40, GD_INK);
    cr_ell(1376 + vx_s(vx_t(300) + 16384u, 40), y - 512, 40, 40, GD_INK);
    cr_blob(2080, y, rx + 40, 12, GD_INK, SH_SD, 256, 256 * (ry + 40) / (rx + 40));   /* the shell (wobbly, as the beetle's,
                                                         * its back round): outlined, flat red */
    cr_blob(2080, y, rx, 12, VX_RED, SH_SD, 256, 256 * ry / rx);
    cr_line(2080, y - ry + 96, 2080, y + ry - 96, 32, GD_INK);   /* the wings' seam */
    cr_blob(1792, y - 64, 112, 7, GD_INK, 21, 256, 256);   /* the spots: wobbly as the shell (round ones looked stamped on) */
    cr_blob(2336, y - 224, 96, 6, GD_INK, 22, 256, 256);
    cr_blob(2400, y + 160, 112, 7, GD_INK, 23, 256, 256);
    cr_blob(1888, y + 256, 80, 6, GD_INK, 24, 256, 256);
    cr_blob(1440, y + 64, 288, 8, GD_INK, 7, 256, 228);   /* the head (wobbly too), its eyes */
    cr_ell(1344, y + 16, 64, 72, GD_SURF);
    cr_ell(1504, y + 16, 64, 72, GD_SURF);
    cr_ell(1328, y + 32, 32, 40, GD_INK);
    cr_ell(1488, y + 32, 32, 40, GD_INK);
}
static void vx_lofi(int32_t e)
{
    uint32_t a = vx_t(40), i;                            /* (the owner: half p11's pace, at the tempo) */
    int32_t cx = 1920 + vx_s(a + 16384u, 896), cy = 1984 + vx_s(a, 448), sc = vx_sw(1u, 1u, 0u, 26, 38, e);   /* (swelling on the beat) */
    for (i = 0; i < 40u; i++)                            /* its path, dotted */
        if (i & 1u)
            cr_rect(1920 + vx_s(i * 1638u + 16384u, 896), 1984 + vx_s(i * 1638u, 448), 16, 16, GD_MID);
    cr_line(cx, cy - 480 * sc / 256, cx + 80, cy - 672 * sc / 256, 32, GD_INK);
    cr_leaf(cx + 80, cy - 640 * sc / 256, a + 65536u - 6258u, 448, 160, GD_BLUE);
    cr_blob(cx, cy, 480 * sc / 256, 10, GD_BURNT, 7, 256 + vx_s(a * 2u, 20), 256 - vx_s(a * 2u + 10430u, 31));
    cr_rect(cx - 288 * sc / 256, cy - 224 * sc / 256, 80, 64, GD_PBLUE);
}
static void vx_analog(int32_t e)
{
    int32_t wob = vx_s(vx_t(130), 48), j, px = 0, py = 0, r = 704 * vx_sw(1u, 1u, 0u, 20, 0, 0) / 256 + e * 160 / 256;   /* (the head
                                                         * swelling with the level, a breath on the beat) */
    cr_blob(1920, 2016, r, 9, GD_ORANGE, 4, 256, 256);
    cr_blob(2208, 1696, 256, 7, GD_ORANGE, 9, 256, 256);
    cr_blob(1696, 1888 + wob, 80, 6, GD_INK, 2, 256, 256);
    cr_blob(2112, 1824 + wob, 64, 6, GD_INK, 3, 256, 256);
    cr_arc(1952, 2112, 112 + e * 48 / 256, 112 + e * 48 / 256, 0, 2086u, 30190u, 24, GD_INK);   /* the smile */
    cr_blob(1536, 2112, 64, 6, GD_PBLUE, 5, 256, 256);
    cr_blob(2336, 2080, 48, 6, GD_PBLUE, 6, 256, 256);
    for (j = 0; j < 22; j++) {                           /* the swirl on top of the head (the owner), turning at the tempo */
        uint32_t a = (uint32_t)j * 3129u + vx_t(160);   /* (a turn every 8 beats) */
        int32_t rr = 192 + j * 22, X = 1920 + vx_s(a + 16384u, rr * 7 / 10), Y = 2016 - r + 96 + vx_s(a, rr / 2);
        if (j)
            cr_line(px, py, X, Y, 64, GD_BURNT);
        px = X;
        py = Y;
    }
    for (j = 0; j < 3; j++) {                            /* the dots round it */
        uint32_t a = vx_t((uint32_t)(110 - j * 30)) + (uint32_t)j * 21903u;
        int32_t rr = 1056 + j * 144;
        cr_rect(1920 + vx_s(a + 16384u, rr), 2016 + vx_s(a, rr * 55 / 100), 48, 48, j & 1 ? GD_BLUE : GD_BURNT);
    }
}
static void vx_fm6(int32_t e)
{
    int32_t i, k;
    for (i = 0; i < 5; i++) {
        int32_t x = 576 + i * 672, y = 2240 + vx_s(vx_t(200) + (uint32_t)i * 10430u, 80);
        cr_line(x, y + 128, x, 2848, 20, GD_INK);
        for (k = 0; k < 6; k++)
            cr_leaf(x, y, (uint32_t)k * 10923u + vx_t(30), 160 + e / 8, 64, i & 1 ? GD_PBLUE : GD_ORANGE);
        for (k = 0; k < 8; k++) {                        /* its fuzz */
            uint32_t a = (uint32_t)k * 8344u + (uint32_t)i * 10430u;
            int32_t r = 208 + vx_s(vx_t(500) + (uint32_t)k * 10430u, 32);
            cr_rect(x + vx_s(a + 16384u, r), y + vx_s(a, r), 32, 32, GD_PBLUE);
        }
        cr_blob(x, y, 64, 6, GD_BURNT, (uint32_t)i + 3u, 256, 256);
    }
}
static void vx_phase(int32_t e)
{
    int32_t i, h, hole = vx_sw(1u, 1u, 0u, 32, 64, e);   /* (the owner) the hole pulsing on the beat, each ring on its own,
                                                         * a ripple outward; their turns at the tempo */
    for (h = 0; h < 2; h++) {                            /* the rings' far halves, the hole, their near halves */
        if (h)
            cr_blob(1536 + 384, 1536, (448 + e * 48 / 256) * hole / 256, 12, GD_ORANGE, 2, 256, 256),
            cr_blob(1920, 1536, 352 * hole / 256, 12, GD_INK, 1, 256, 256);
        for (i = 0; i < 4; i++) {
            uint32_t rot = 2608u + (uint32_t)i * 1252u + (i & 1 ? vx_t((uint32_t)(25 + 20 * i)) : 0u - vx_t((uint32_t)(25 + 20 * i)));
            int32_t g = vx_sw(1u, 1u, 0u - (uint32_t)(i + 1) * 9000u, 16, 28, e);
            cr_arc(1920, 1536, (576 + i * 176) * g / 256, (224 + i * 64) * 141 / 256 * g / 256, rot,
                   h ? 0u : 32768u, h ? 32768u : 65536u, 36, i & 1 ? GD_ORANGE : GD_BLUE);
        }
    }
}
static void vx_wheel(int32_t e)
{
    uint32_t tr = gd_pl.t * 28u / 192u, rot = tr * 65536u / (16u * 50u) * 16u / 100u;   /* (travel Q4 px, a third of p11's
                                                         * (the owner), at the tempo; turns: travel / R) */
    int32_t cx = (int32_t)((tr + 640u) % 5120u) - 640, cy = 2112, k, px = 0, py = 0, rim = vx_sw(1u, 1u, 0u, 14, 24, e),
            hub = vx_sw(1u, 1u, 32768u, 40, 64, e);    /* (the rim on the beat, the hub between: each its own pulse) */
    for (k = 0; k <= 26; k++) {                          /* the rim, irregular, turning */
        uint32_t a = (uint32_t)k * 2521u;
        int32_t r = (800 + vx_s(4u * (a + rot), 40) + vx_s(7u * (a + rot) + 10430u, 24)) * rim / 256, X = cx + vx_s(a + 16384u, r), Y = cy + vx_s(a, r);
        if (k)
            cr_line(px, py, X, Y, 112, GD_BURNT);
        px = X;
        py = Y;
    }
    for (k = 0; k < 11; k++) {                           /* the spokes, tapered and uneven */
        uint32_t a = rot + (uint32_t)k * 5958u + (gd_h((uint32_t)k, 5) >> 11) - 16u;
        int32_t dx = gd_s(a + 16384u), dy = gd_s(a), tip = (704 - (int32_t)(gd_h((uint32_t)k, 9) >> 10)) * rim / 256, wb = 64 + (int32_t)(gd_h((uint32_t)k, 3) >> 12),
                wt = 32 + (int32_t)(gd_h((uint32_t)k, 7) >> 12);
        int32_t q[8] = {cx + (dx * 192 - dy * wb) / 256, cy + (dy * 192 + dx * wb) / 256, cx + (dx * tip - dy * wt) / 256,
                        cy + (dy * tip + dx * wt) / 256, cx + (dx * tip + dy * wt) / 256, cy + (dy * tip - dx * wt) / 256,
                        cx + (dx * 192 + dy * wb) / 256, cy + (dy * 192 - dx * wb) / 256};
        cr_quad(q, k & 1 ? GD_DIRTDK : GD_BURNT);
    }
    cr_blob(cx, cy, 240 * hub / 256, 10, GD_INK, 9, 256, 230);
    cr_blob(cx, cy, 176 * hub / 256, 10, GD_BURNT, 10, 256, 230);
    cr_blob(cx, cy, 56 * hub / 256, 6, GD_DIRTDK, 11, 256, 256);
}
static void vx_noise(int32_t e)
{
    int32_t i, sr = 176 + e * 48 / 256, pt[32];
    for (i = 0; i < 70; i++) {                           /* the galaxy's arm */
        uint32_t a = (uint32_t)i * 2712u + gd_pl.t * 25u / 30u;   /* (a third of p11's pace: the owner) */
        int32_t r = 96 + i * 10;
        cr_rect(1920 + vx_s(a + 16384u, r), 1952 + vx_s(a, r * 55 / 100), 32, 32, i % 3 ? GD_PBLUE : GD_BLUE);
    }
    for (i = 0; i < 16; i++) {                           /* the spiky sun */
        int32_t r = i & 1 ? sr * 7 / 10 : sr * 6 / 5;
        pt[2 * i] = CX(1920 + vx_s((uint32_t)i * 4096u + 16384u, r));
        pt[2 * i + 1] = CY(1952 + vx_s((uint32_t)i * 4096u, r));
    }
    gd_poly(pt, 16, 2, GD_ORANGE);
    cr_blob(1920, 1952, sr * 55 / 100, 7, GD_BURNT, 6, 256, 256);
    for (i = 0; i < 3; i++) {                            /* three planets, the middle one ringed */
        static const int16_t RP[3] = {800, 1184, 1536}, SZ[3] = {80, 112, 64};
        static const uint8_t SP[3] = {80, 50, 30}, PN[3] = {2, 1, 1}, PD[3] = {1, 1, 2};   /* (each its own pulse: 2, 1, 1/2 a beat) */
        uint32_t a = gd_pl.t * SP[i] / 30u + (uint32_t)i * 20860u;
        int32_t g = vx_sw(PN[i], PD[i], (uint32_t)i * 21845u, 40, 64, e);
        int32_t x = 1920 + vx_s(a + 16384u, RP[i]), y = 1952 + vx_s(a, RP[i] * 2 / 5);
        if (i == 1)
            cr_arc(x, y, 224 * g / 256, 56 * g / 256, 3129u, 0u, 65536u, 20, GD_BURNT);
        cr_blob(x, y, SZ[i] * g / 256, 7, i == 1 ? GD_BLUE : i ? 0xC50Fu : GD_BURNT, (uint32_t)i + 2u, 256, 256);
    }
}
static void vx_bush(int32_t bx, int32_t by, int32_t W0, int32_t H0, uint32_t sd, int32_t e)
{
    int32_t s = vx_sw(3u, 8u, sd * 9000u, 14, 36, e);    /* (the owner) swelling every 2 2/3 beats (25 % slower than a hop every 2) */
    int32_t W = W0 * s / 256, H = H0 * s / 256, sw = vx_s(vx_t(200) + sd * 10430u, 24) * e / 256, px, py, k, o;
    int32_t ol = 48 + (24 + 72 * e / 256) * gd_pulse(3u, 8u, sd * 9000u + 16384u) / 256;   /* (the owner) an outline round it,
                                                         * thicker on the pulse as the music is louder */
    for (o = 0; o < 2; o++)                              /* the outline, the leaves on it */
    for (py = by - H - (o ? 0 : (ol + 47) / 48 * 48); py < by; py += 48)   /* (on the leaves' grid) */
        for (px = bx - W / 2 - (o ? 0 : (ol + 47) / 48 * 48); px < bx + W / 2 + (o ? 0 : ol); px += 48) {
            int32_t u = (px - bx) * 512 / (W + (o ? 0 : 2 * ol)), v = (by - py) * 256 / (H + (o ? 0 : ol)),
                    n = (int32_t)(gd_h((uint32_t)(px + sd * 997u), py) >> 8);
            int32_t x0 = CX(px + sw) >> 4, y0 = CY(py) >> 4;
            uint16_t c;
            if (u * u + v * v * 230 / 256 > 65536 + (o ? 77 : 20) * (n - 128))   /* (the outline smoother than the leaves) */
                continue;
            if (!o) {
                cv_rect(x0, y0, (CX(px + sw + 48) >> 4) - x0, (CY(py + 48) >> 4) - y0, GD_INK);
                continue;
            }
            c = v > 184 ? GD_PBLUE : v < 77 ? 0x3290u : GD_BLUE;
            cv_rect(x0, y0, (CX(px + sw + 48) >> 4) - x0, (CY(py + 48) >> 4) - y0, n > 218 && v > 102 ? GD_PBLUE : c);   /* (each cell
                                                         * to the next one's edge: no seams between them) */
        }
    for (k = 0; k < 3; k++)                              /* a few berries */
        if (gd_h(sd, (uint32_t)k + 11u) & 32768u)
            cr_rect(bx + ((int32_t)(gd_h(sd * 7u, (uint32_t)k) >> 8) - 128) * W * 6 / 2560 + sw,
                    by - H * (77 + (int32_t)(gd_h(sd, (uint32_t)k * 3u) >> 9)) / 256, 48, 48, GD_ORANGE);
}
static void vx_serum(int32_t e)
{
    vx_bush(704, 2816, 896, 640, 1, e);
    vx_bush(1920, 2848, 1248, 896, 2, e);
    vx_bush(3136, 2816, 896, 640, 3, e);
    vx_bush(1344, 2912, 544, 352, 4, e);
    vx_bush(2496, 2912, 544, 352, 5, e);
}

/* -------------------------------------------------------------------------------------------------- MONK --- */
/* (0.9.20) MONK: a monk of six faces (ui_monk.h: tools/gen_monk.py from assets/ui-art/monk), drawn as they are, 112 px
 * (2 px an art pixel, the garden's blocks) in the middle of the rectangle, a smaller one (SOUND's) sampled down to fit.
 * Silent he rests; sounding he shows the vowel sung (VOWEL / VOWEL2 / TALK, or WORDS' phoneme): A ah, E I ee, O U
 * (rounded) oo, m n ng mm; a note held past 2 s the chant. Each note he sings mm-vowel-mm: his lips closed its first
 * 90 ms and as it lets go (the newest voice's release), so the mouth moves on every note. His red outline (a ring 2 px out and a fainter one 4 px)
 * pulses on the beat, as strong as the sound, and he lifts 2 px every other beat */
#include "ui_monk.h"
#define MK_RED 0xD8A1u
enum { MK_REST, MK_AH, MK_OO, MK_EE, MK_MM, MK_CHANT };
static struct { uint32_t age, ms; } mk_held[NTRK] __attribute__((section(".pool")));   /* the newest note, when it came */
static uint32_t vx_monk_face(const track_t *t, int32_t e)
{
    uint32_t k = (uint32_t)(t - trk) % NTRK, i, a = 0, st = 0, rnd;
    int32_t f2;
    for (i = 0; i < NVOICE; i++)
        if (t->v[i].active && t->v[i].age > a) { a = t->v[i].age; st = t->v[i].stage; }
    if (a > mk_held[k].age) mk_held[k].ms = fm1_ms;
    mk_held[k].age = a;
    if (!e)
        return MK_REST;
    if (!a || st >= 3u)                                  /* (the note letting go: the lips close) */
        return MK_MM;
    if (t->p[P_VLANG]) {                                 /* WORDS: the phoneme sounding */
        const sing_t *s = sing_view(t);
        uint32_t c = sing_phon(s);
        if (PHON[c].cls == PC_SIL) return MK_REST;
        if (PHON[c].cls == PC_NAS) return MK_MM;
        f2 = s->f[1];
        rnd = PHON[c].place & PL_RND;
    } else {                                             /* VOWEL -> VOWEL2 over TALK (as formant_render) */
        int32_t v0 = t->p[P_E0], v1 = t->p[P_E1], g = 0, x, ii, fr;
        for (i = 0; i < NVOICE; i++)
            if (t->v[i].active && (int32_t)(t->v[i].s[7] & FMT_TALK_MASK) > g) g = (int32_t)(t->v[i].s[7] & FMT_TALK_MASK);
        if (t->p[P_E2]) { g >>= 9; v0 += ((v1 - v0) * ((g * (65536 - g)) >> 15)) >> 15; }
        x = clamp(v0, 0, 127) * 4 * 256 / 127;
        ii = x >> 8 < 4 ? x >> 8 : 3;
        fr = x >> 8 < 4 ? x & 255 : 256;
        f2 = VOWEL_F[ii][1] + (((VOWEL_F[ii + 1][1] - VOWEL_F[ii][1]) * fr) >> 8);
        rnd = v0 > 80;
    }
    if (!t->p[P_VLANG] && fm1_ms - mk_held[k].ms < 90u) return MK_MM;   /* (the note's onset: m-) */
    if (fm1_ms - mk_held[k].ms > 2000u) return MK_CHANT;
    if (rnd) return MK_OO;
    return f2 > 1550 ? MK_EE : MK_AH;                    /* (front: F2 over ~1.55 kHz) */
}
/* a pixel of colour i (0 nothing, 1..13 MK_PAL; 14 15 the outline's rings, over what is there, ring / 256 strong) */
static void mk_px(int32_t x, int32_t y, uint32_t i, int32_t ring)
{
    uint16_t *p;
    y += cv_oy;
    if (!i || (uint32_t)x >= cv_w || (uint32_t)y >= cv_h || (i >= 14u && !ring))
        return;
    p = &cv_px[(uint32_t)y * cv_w + (uint32_t)x];
    *p = swap16(i < 14u ? MK_PAL[i] : ux_mix(swap16(*p), MK_RED, (i == 14u ? ring : ring * 45 / 100) * 100 / 256));
}
/* a face's skip: the pixels rest's (255s carry on); none left: never */
static uint32_t mk_skip(const uint8_t **d, const uint8_t *de)
{
    uint32_t k = 0;
    if (*d >= de)
        return 0xFFFFFFFFu;
    while (**d == 255u) { k += 255u; (*d)++; }
    return k + *(*d)++;
}
static void vx_monk(const track_t *t, int32_t x, int32_t y, int32_t w, int32_t h, int32_t e)
{
    uint32_t face = vx_monk_face(t, e), n = 0, c = 0, dn = 0, dc = 0, skip, sx, sy, tx;
    const uint8_t *b = MK_RLE, *d = MK_RLE + MK_OFF[face], *de = MK_RLE + MK_OFF[face + 1u];
    int32_t S = w < h ? w : h, ring = e ? e * (51 + gd_pulse(1u, 1u, 0u) * 205 / 256) / 256 : 0, x0, y0, ty = -1;
    uint8_t row[MK_W];
    S = S < MK_W ? S : MK_W;
    x0 = x + (w - S) / 2;
    y0 = y + (h - S) / 2 - (e && gd_run && ((gd_bt >> 16) & 1u) ? 2 : 0);
    skip = face ? mk_skip(&d, de) : 0xFFFFFFFFu;
    for (sy = 0; sy < MK_W; sy++) {
        for (sx = 0; sx < MK_W; sx++) {
            if (!n) {
                n = (*b >> 4) + 1u;
                c = *b++ & 15u;
            }
            n--;
            row[sx] = (uint8_t)c;
            if (!dn && !skip) {                          /* (the face's own pixels: a run) */
                dn = (*d >> 4) + 1u;
                dc = *d++ & 15u;
            }
            if (dn) {
                row[sx] = (uint8_t)dc;
                if (!--dn) skip = mk_skip(&d, de);
            } else
                skip--;
        }
        if ((int32_t)(sy * (uint32_t)S / MK_W) == ty)    /* (sampled down: this row's place drawn already) */
            continue;
        ty = (int32_t)(sy * (uint32_t)S / MK_W);
        for (tx = 0; tx < (uint32_t)S; tx++)
            mk_px(x0 + (int32_t)tx, y0 + ty, row[tx * MK_W / (uint32_t)S], ring);
    }
}

/* ------------------------------------------------------------------------------------------------ the glue --- */
/* the engine's creature in the rectangle x y w h (canvas px: EDIT's panel, HOME's middle), en_home: HOME's level for
 * the track (-1: its own, its part's peak slewed); 1 drawn */
static int vx_draw(const track_t *t, int32_t x, int32_t y, int32_t w, int32_t h, int32_t en_home)
{
    uint32_t k = (uint32_t)(t - trk) % NTRK;
    int32_t e, x0 = gd.x0, y0 = gd.y0, y1 = gd.y1;
    if (en_home >= 0)
        e = en_home;
    else {
        e = clamp(((track_t *)t)->peak * 256 / 7000, 0, 256);
        ((track_t *)t)->peak = 0;
        e = e > vxs.en[k] ? e : vxs.en[k] + (e - vxs.en[k] - 2) / 3;
    }
    e = (ui_prefs & PREF_ANIM_OFF) || e < 5 ? 0 : e;
    vxs.en[k] = e;
    cr.s = w * 256 / 220 < h * 256 / 150 ? w * 256 / 220 : h * 256 / 150;   /* (the creatures: ~220 x 150 of the mockup) */
    cr.cx = x * 16 + w * 8;
    cr.cy = y * 16 + h * 8 - (cr.s * 16 * 12 >> 8);
    gd.x0 = 0;
    gd.y0 = -cv_oy;
    gd.y1 = (int32_t)cv_h - cv_oy;
    switch (trk_eng(t)) {
    case ENGI_DRUM: vx_drum(t); break;
    case ENGI_VOICE: vx_voice(e); break;
    case ENGI_LOFI: vx_lofi(e); break;
    case ENGI_ANALOG: vx_analog(e); break;
    case ENGI_FM6: vx_fm6(e); break;
    case ENGI_PHASE: vx_phase(e); break;
    case ENGI_WHEEL: vx_wheel(e); break;
    case ENGI_NOISE: vx_noise(e); break;
    case ENGI_SERUM: vx_serum(e); break;
    case ENGI_MONK: vx_monk(t, x, y, w, h, e); break;
    default: vx_beetle(e); break;                        /* (ANIMAL's ladybird; DIGITAL its family's) */
    }
    gd.x0 = x0;
    gd.y0 = y0;
    gd.y1 = y1;
    return 1;
}
/* (GARDEN, the owner: the kit's pieces as its beings, the colony's blobs and the mushrooms retired) one lane's piece of
 * the BEING's kit (vx_drum) about the cell's centre, its level e: the kick, the snare, the clap's hands, the hi-hats
 * (shut, open), the tom, the rim, the bell; a muted lane's in GD_MID */
static void vx_piece(uint32_t l, int32_t e, int mute)
{
    int32_t i;
#define PC(c) (mute ? GD_MID : (c))
    switch (l) {
    case VK_KICK: {
        int32_t ky = 1728 - e * 128 / 256, q[8] = {1536, ky, 2304, ky, 2304, ky + 544, 1536, ky + 544};
        cr_quad(q, PC(GD_ORANGE));
        cr_blob(1920, ky + 272, 352 + e * 96 / 256, 9, PC(GD_SURF), 7, 230, 230);
        cr_blob(1920, ky + 272, 128 + e * 64 / 256, 7, PC(GD_BURNT), 8, 256, 256);
        break;
    }
    case VK_SNARE:
        cr_shell(1920 + vx_s((uint32_t)fm1_ms * 90u, e * 32 / 256), 1904 - e * 64 / 256, 640 + e / 3, 288, PC(GD_SURF), e);
        cr_line(1600, 1760, 1792, 1888 - e * 64 / 256, 32, PC(GD_INK));   /* its stick */
        break;
    case VK_CLAP:                                        /* two hands meeting on the hit */
        i = 224 - e * 160 / 256;
        cr_leaf(1920 - i, 2048, 12000u, 352, 176, PC(GD_ORANGE));
        cr_leaf(1920 + i, 2048, 20768u, 352, 176, PC(GD_BURNT));
        break;
    case VK_HATC:
    case VK_HATO: {
        int32_t up = l == VK_HATO ? 96 + e * 64 / 256 : 0, sh = l == VK_HATC ? e * 48 / 256 : 0;
        vx_stem(1920, 1872, 2336, 64, 0, l);
        cr_blob(1920, 1808 - up + sh, 256 + e / 4, 8, PC(GD_ORANGE), 1, 256, 77);
        cr_blob(1920, 1872, 256, 8, PC(GD_PBLUE), 2, 256, 77);
        break;
    }
    case VK_TOM:
        cr_shell(1920, 1856 - e * 64 / 256, 704 + e / 4, 352, PC(GD_BLUE), e);
        break;
    case VK_RIM:
        cr_shell(1920, 1952, 640, 224, PC(GD_BURNT), e);
        cr_line(1504, 1856 - e * 96 / 256, 2336, 2016, 32, PC(GD_INK));   /* the stick across the rim */
        break;
    default:                                             /* the bell: a cymbal swinging and ringing */
        vx_stem(1920, 1760, 2336, 64, 0, 3u);
        cr_blob(1920 + vx_s((uint32_t)fm1_ms * 60u, e * 64 / 256), 1712 - e * 96 / 256, 304 + e / 3, 8, PC(GD_PBLUE), 3, 256,
                77 + e / 8);
        break;
    }
#undef PC
}
/* DRUM-X's KIT 2 / KIT 3 (lane -1: MORPH's bar over it) and SOUND 1..4 (lane, lit): the eight pieces, 4 x 2, each moving
 * with its lane's hit, its name under it; the kit swelling on the beat */
static void vx_kit_page(const track_t *t, int32_t lane)
{
    char b[16];
    int32_t e[8], i, sw;
    uint32_t l;
    vx_lanes(t, e);
    gd.x0 = 0; gd.y0 = -cv_oy; gd.y1 = (int32_t)cv_h - cv_oy;
    sw = vx_sw(1u, 1u, 0u, 16, 0, 0);
    for (i = 0; i < 8; i++) {
        int32_t x = 33 + (i & 3) * 58, y = 48 + (i >> 2) * 50, mute = dx_lane_muted((uint32_t)i);
        cr.s = 165 * sw / 256;
        cr.cx = x * 16;
        cr.cy = (y - 2) * 16 + (2336 - 2000) * (165 - cr.s) / 256;   /* (swelling from its foot) */
        vx_piece((uint32_t)i, clamp(e[i] * 3 / 2, 0, 256), mute);   /* (the played piece reacts more: a hit reads at a glance) */
        cv_text_in(x - 18, y + 16, 36, &AF_X, drum_lane_abbr(t, (uint32_t)i),
                   i == lane || e[i] > 96 ? T_THEME : mute ? T_DIM : T_MID, T_SURF);   /* (lit while it sounds) */
        if (i == lane)
            cv_rect(x - 8, y + 25, 16, 2, T_THEME);
    }
    if (lane < 0) {                                      /* MORPH: A .. B (0.5.1: X-MOD live) */
        int32_t morph = clamp(t->p[P_E0] + dx_mot_v, 0, 127), mx = 30 + morph * 180 / 127;
        cv_rect(30, 10, 180, 1, T_LINE);
        cv_rect(30, 9, mx - 30, 3, GD_ORANGE);
        cv_text(12, 6 - AF_X_CAP_Y, &AF_X, "A", morph < 64 ? T_THEME : T_DIM);
        cv_text_r(226, 6 - AF_X_CAP_Y, &AF_X, "B", morph >= 64 ? T_THEME : T_DIM, T_SURF);
        return;
    }
    l = (uint32_t)lane & 7u;
    str_cpy(b, drum_lane_abbr(t, l), sizeof b);
    str_cpy(b + str_len(b), " \xB7 ", sizeof b - str_len(b));
    str_cpy(b + str_len(b), drum_lane_name(t, l), sizeof b - str_len(b));
    cv_text(8, 6 - AF_X_CAP_Y, &AF_X, "KIT", T_THEME);
    cv_text_r(232, 6 - AF_X_CAP_Y, &AF_X, b, T_THEME, T_BG);
}
/* FM6's OPERATOR / OPERATOR 2 / OP ENV: the fern, a frond pair an operator (OP 1 at the foot), its length the
 * operator's level (with its envelope while it sounds), the operator shown orange; the small tulip on top */
static void vx_fern(const track_t *t, uint32_t sel)
{
    uint32_t tr = (uint32_t)(t - trk) % NTRK;
    const uint8_t *p = fm6_eff[tr].ok ? fm6_eff[tr].p : fm6_patch[tr];
    int32_t k, e = vxs.en[tr];
    cr.cx = 120 * 16; cr.cy = 74 * 16 + 16 * 16; cr.s = 220;
    gd.x0 = 0; gd.y0 = -cv_oy; gd.y1 = (int32_t)cv_h - cv_oy;
    cr_line(1920, 2880, 1920 + vx_s(vx_t(100), 48), 1056, 32, GD_INK);
    for (k = 0; k < 6; k++) {
        const uint8_t *op = p + (5u - (uint32_t)k) * FP_OP;
        int32_t ol = op[FP_OL] % 100u, y = 2720 - k * 272, L = 96 + ol * (352 - k * 24) / 99, sw = vx_s(vx_t(200) + (uint32_t)k * 10430u, 1300 + e * 5);
        cr_leaf(1920, y, 49152u - 18000u - (uint32_t)sw, L, 64, (uint32_t)k == sel ? GD_ORANGE : GD_BLUE);
        cr_leaf(1920, y, 49152u + 16384u - 2000u + (uint32_t)sw, L * 9 / 10, 64, (uint32_t)k == sel ? GD_ORANGE : GD_BURNT);
    }
    cr_blob(1920, 1008, 96, 6, GD_ORANGE, 5, 256, 300);
}
static uint32_t fm6_alg_of(const track_t *t);
static void fop_freq(const uint8_t *op, char *s);
static const uint8_t *fop_bytes(uint32_t tr);
/* OPERATOR / OPERATOR 2: the fern, the operator picked orange; "ALG 05" over it on the left (" PAT": the pattern's), its
 * frequency on the right ("OP3 x1.50"), as the colony's pages had them */
static void vx_fm6_page(const track_t *t, uint32_t sel, int op2)
{
    uint32_t alg = fm6_alg_of(t), hot = ui.hot_t ? (uint32_t)cur_page()->id[ui.hot_col & 3u] : 0u;
    char b[16] = {'A', 'L', 'G', ' ', (char)('0' + (alg + 1u) / 10u), (char)('0' + (alg + 1u) % 10u), 0};
    char fq[16] = {'O', 'P', (char)('1' + sel), ' ', 0};
    (void)op2;
    vx_fern(t, sel);
    if (!t->p[P_E0])
        str_cpy(b + 6, " PAT", 5);
    cv_text(10, 4, &AF_S, b, hot == P_E0 || hot == P_E7 ? T_ACCENT : T_MID);
    fop_freq(fm6_patch[(t - trk) % NTRK] + (5u - sel) * FP_OP, fq + 4);
    cv_text_r(228, 4, &AF_S, fq, T_TEXT, T_BG);
}
/* OP ENV: the fern, the operator orange; "OP3" top left, the stage KNOB 3 / 4 edit with its R and L right */
static void vx_fm6env(const track_t *t)
{
    static const char *const STG[4] = {"ATK", "DEC", "SUS", "REL"};
    const uint8_t *op = fop_bytes((uint32_t)(t - trk));
    uint32_t sg = fop_stg & 3u;
    char b[24] = {'O', 'P', (char)('1' + fop_op % 6u), 0};
    vx_fern(t, fop_op % 6u);
    cv_text(10, 4, &AF_S, b, T_MID);
    str_cpy(b, STG[sg], sizeof b);
    str_cpy(b + str_len(b), "  R ", 5);
    fmt_int(b + str_len(b), op[FP_R1 + sg]);
    str_cpy(b + str_len(b), "  L ", 5);
    fmt_int(b + str_len(b), op[FP_L1 + sg]);
    cv_text_r(228, 4, &AF_S, b, T_TEXT, T_BG);
}
/* (0.9.12's MOTION jellyfish, retired with Phase 5: the MSEQ bars alone) */
static int vx_mseq(const track_t *t, int32_t x, int32_t y, int32_t w, int32_t h)
{
    (void)t; (void)x; (void)y; (void)w; (void)h;
    return 0;
}
