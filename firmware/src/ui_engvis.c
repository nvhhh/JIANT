/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments
 * Copyright (C) 2026 Juan Ignacio Antonucci (Juan Jiant) */
/* (GARDEN, Phase 5) the engines' creatures (docs/garden-handoff mockups/creatures.html): each engine's EDIT / BEING
 * picture and its HOME / ECO one, drawn by code on the garden's primitives (ui_garden.c: 2 px blocky polygons, leaves,
 * blobs), on the plants' clock (MOTION x the music's level). DRUM a kit (hi-hat, crash, two toms, snare, kick; each
 * piece moves with its lane); VOICE a singing flower; ANIMAL a beetle; LOFI an apple orbiting its dotted path; ANALOG an
 * orange being with a face; FM6 five fuzzy flowers; PHASE a black hole and its rings; WHEEL a barn wheel rolling; NOISE
 * a galaxy, its sun and planets; SHRUB pixel bushes. Engines without their own (DIGITAL): ANIMAL's beetle.
 * The pages: DRUM-X's KIT / SOUND a ring of eight mushroom caps, each jumping on its lane's hit (vx_kit_page); FM6's
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
static void vx_drum(const track_t *t)
{
    int32_t e[8], hh, tm, ky, i;
    vx_lanes(t, e);
    hh = e[VK_HATC] > e[VK_HATO] ? e[VK_HATC] : e[VK_HATO];
    tm = e[VK_TOM];
    cr_line(640, 1600, 640, 2848, 24, GD_INK);           /* the hi-hat, closing on its hits */
    cr_blob(640, 1536 + hh / 4, 240, 8, GD_ORANGE, 1, 256, 77);
    cr_blob(640, 1664 - hh / 8, 240, 8, GD_PBLUE, 2, 256, 77);
    cr_line(3264, 1184, 3264, 2816, 24, GD_INK);         /* the crash, shaking with the bell */
    cr_blob(3264, 1120 - e[VK_BELL] / 8, 272, 8, GD_PBLUE, 3, 256, 77);
    cr_shell(1440, 1888 + tm / 8, 640, 320, GD_BLUE, tm);   /* the toms, the snare */
    cr_shell(2400, 1888 + tm / 8, 640, 320, GD_BURNT, tm);
    i = e[VK_SNARE] > e[VK_CLAP] ? e[VK_SNARE] : e[VK_CLAP];
    cr_shell(1056, 2400 + i / 8, 480, 224, GD_SURF, i);
    ky = 2272 - e[VK_KICK] * 48 / 256;                   /* the kick, jumping */
    {
        int32_t q[8] = {1536, ky, 2304, ky, 2304, ky + 544, 1536, ky + 544};
        cr_quad(q, GD_ORANGE);
    }
    cr_blob(1920, ky + 272, 352, 9, GD_SURF, 7, 230, 230);
    cr_blob(1920, ky + 272, 128, 7, GD_BURNT, 8, 256, 256);
}
static void vx_voice(int32_t e)
{
    int32_t i, gate = clamp((e - 13) * 5, 0, 256);
    for (i = 0; i < 7; i++)
        cr_leaf(1920, 1600, (uint32_t)i * 9362u + vx_t(20), 416 + e * 48 / 256, 160, i & 1 ? GD_ORANGE : GD_SURF);
    cr_line(1920, 1952, 1920, 2848, 32, GD_INK);
    cr_leaf(1920, 2400, 65536u - 8344u, 320, 96, GD_BLUE);
    cr_blob(1920, 1600, 288, 9, GD_BURNT, 3, 256, 256);
    cr_rect(1792, 1504, 48, 64, GD_INK);
    cr_rect(1984, 1504, 48, 64, GD_INK);
    cr_blob(2080, 1696, 64, 6, GD_INK, 4, 256, 64 + e * 230 / 256);   /* the mouth, open with the level */
    for (i = 0; gate && i < 6; i++) {                    /* the notes leaving it, only with music */
        uint32_t age = (vx_t(70) / 6u + (uint32_t)i * 10923u) & 65535u;   /* (0 .. 1 a turn) */
        int32_t x = 2160 + (int32_t)(age * (uint32_t)(800 + e * 640 / 256) >> 16), y = 1600 - vx_s(age * 5u / 6u + (uint32_t)i * 10430u, 96) -
                    (int32_t)(age * 224u >> 16);
        if ((int32_t)(age >> 8) < gate) {
            cr_rect(x, y, 32 + e / 8, 32 + e / 8, GD_PBLUE);
            cr_rect(x + 32 + e / 8, y - 64, 16, 64, GD_PBLUE);
        }
    }
}
static void vx_beetle(int32_t e)
{
    int32_t s0 = gd_s((uint32_t)vx_beat() << 7), hop = s0 * s0 / 256 * (32 + 64 * e / 256) / 256, y = 2336 - hop, i;
    int32_t sw = vx_s(vx_t(900), 64);
    for (i = -1; i <= 1; i++) {                          /* the legs, stepping */
        int32_t s = i & 1 ? sw : -sw;
        cr_line(1920 + i * 256, y + 192, 1920 + i * 256 + s, 2816, 24, GD_INK);
        cr_line(1920 + i * 256, y - 192, 1920 + i * 256 - s, 2816 - 2 * hop, 24, GD_INK);
    }
    cr_line(1280, y - 416, 1024, y - 672 - vx_s(vx_t(600), 48), 20, GD_INK);   /* the antennae */
    cr_line(1280, y - 416, 1088, y - 640 + vx_s(vx_t(600) + 16384u, 48), 20, GD_INK);
    cr_blob(1920, y, 576, 10, GD_BURNT, 5, 346, 256);
    cr_line(1248, y, 2592, y, 24, GD_SURF);
    cr_blob(1696, y - 192, 80, 6, GD_INK, 2, 256, 256);
    cr_blob(2144, y + 192, 64, 6, GD_INK, 4, 256, 256);
    cr_blob(1152, y, 176, 8, GD_INK, 7, 256, 256);
    cr_rect(1632, y - 256, 32, 32, GD_PBLUE);
}
static void vx_lofi(int32_t e)
{
    uint32_t a = vx_t(80), i;
    int32_t cx = 1920 + vx_s(a + 16384u, 896), cy = 1984 + vx_s(a, 448), sc = 256 + vx_s(a, 26);
    for (i = 0; i < 40u; i++)                            /* its path, dotted */
        if (i & 1u)
            cr_rect(1920 + vx_s(i * 1638u + 16384u, 896), 1984 + vx_s(i * 1638u, 448), 16, 16, GD_MID);
    cr_line(cx, cy - 480 * sc / 256, cx + 80, cy - 672 * sc / 256, 32, GD_INK);
    cr_leaf(cx + 80, cy - 640 * sc / 256, a + 65536u - 6258u, 448, 160, GD_BLUE);
    cr_blob(cx, cy, 480 * sc / 256 + e / 8, 10, GD_BURNT, 7, 256 + vx_s(a * 2u, 20), 256 - vx_s(a * 2u + 10430u, 31));
    cr_rect(cx - 288, cy - 224, 80, 64, GD_PBLUE);
}
static void vx_analog(int32_t e)
{
    int32_t wob = vx_s(vx_t(130), 48), j, px = 0, py = 0;
    cr_blob(1920, 2016, 704 + e / 4, 9, GD_ORANGE, 4, 256, 256);
    cr_blob(2208, 1696, 256, 7, GD_ORANGE, 9, 256, 256);
    for (j = 0; j < 22; j++) {                           /* the swirl */
        uint32_t a = (uint32_t)j * 3129u + vx_t(60);
        int32_t r = 160 + j * 14, X = 1792 + vx_s(a + 16384u, r * 7 / 10), Y = 2176 + vx_s(a, r / 2);
        if (j)
            cr_line(px, py, X, Y, 24, GD_SURF);
        px = X;
        py = Y;
    }
    cr_blob(1696, 1888 + wob, 80, 6, GD_INK, 2, 256, 256);
    cr_blob(2112, 1824 + wob, 64, 6, GD_INK, 3, 256, 256);
    cr_arc(1952, 2112, 112 + e * 48 / 256, 112 + e * 48 / 256, 0, 2086u, 30190u, 24, GD_INK);   /* the smile */
    cr_blob(1536, 2112, 64, 6, GD_PBLUE, 5, 256, 256);
    cr_blob(2336, 2080, 48, 6, GD_PBLUE, 6, 256, 256);
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
    int32_t p = (256 - vx_beat()) * (256 - vx_beat()) >> 8, grow = 256 + e * 13 / 256 + e * p / 3200, i, h;
    for (h = 0; h < 2; h++) {                            /* the rings' far halves, the hole, their near halves */
        if (h)
            cr_blob(1536 + 384, 1536, 448 + e * 48 / 256 + e * p / 2048, 12, GD_ORANGE, 2, 256, 256),
            cr_blob(1920, 1536, 352 + e * p / 2048, 12, GD_INK, 1, 256, 256);
        for (i = 0; i < 4; i++) {
            uint32_t rot = 2608u + (uint32_t)i * 1252u + (i & 1 ? vx_t((uint32_t)(25 + 20 * i)) : 0u - vx_t((uint32_t)(25 + 20 * i)));
            cr_arc(1920, 1536, (576 + i * 176) * grow / 256, (224 + i * 64) * 141 / 256 * (256 + e * p / 2560) / 256, rot,
                   h ? 0u : 32768u, h ? 32768u : 65536u, 36, i & 1 ? GD_ORANGE : GD_BLUE);
        }
    }
}
static void vx_wheel(int32_t e)
{
    uint32_t tr = gd_pl.t * 28u / 64u, rot = tr * 65536u / (16u * 50u) * 16u / 100u;   /* (travel Q4 px; turns: travel / R) */
    int32_t cx = (int32_t)((tr + 640u) % 5120u) - 640, cy = 2112, k, px = 0, py = 0;
    (void)e;
    for (k = 0; k <= 26; k++) {                          /* the rim, irregular, turning */
        uint32_t a = (uint32_t)k * 2521u;
        int32_t r = 800 + vx_s(4u * (a + rot), 40) + vx_s(7u * (a + rot) + 10430u, 24), X = cx + vx_s(a + 16384u, r), Y = cy + vx_s(a, r);
        if (k)
            cr_line(px, py, X, Y, 112, GD_BURNT);
        px = X;
        py = Y;
    }
    for (k = 0; k < 11; k++) {                           /* the spokes, tapered and uneven */
        uint32_t a = rot + (uint32_t)k * 5958u + (gd_h((uint32_t)k, 5) >> 11) - 16u;
        int32_t dx = gd_s(a + 16384u), dy = gd_s(a), tip = 704 - (int32_t)(gd_h((uint32_t)k, 9) >> 10), wb = 64 + (int32_t)(gd_h((uint32_t)k, 3) >> 12),
                wt = 32 + (int32_t)(gd_h((uint32_t)k, 7) >> 12);
        int32_t q[8] = {cx + (dx * 192 - dy * wb) / 256, cy + (dy * 192 + dx * wb) / 256, cx + (dx * tip - dy * wt) / 256,
                        cy + (dy * tip + dx * wt) / 256, cx + (dx * tip + dy * wt) / 256, cy + (dy * tip - dx * wt) / 256,
                        cx + (dx * 192 + dy * wb) / 256, cy + (dy * 192 - dx * wb) / 256};
        cr_quad(q, k & 1 ? GD_DIRTDK : GD_BURNT);
    }
    cr_blob(cx, cy, 240, 10, GD_INK, 9, 256, 230);
    cr_blob(cx, cy, 176, 10, GD_BURNT, 10, 256, 230);
    cr_blob(cx, cy, 56, 6, GD_DIRTDK, 11, 256, 256);
}
static void vx_noise(int32_t e)
{
    int32_t i, sr = 176 + e * 48 / 256, pt[32];
    for (i = 0; i < 70; i++) {                           /* the galaxy's arm */
        uint32_t a = (uint32_t)i * 2712u + vx_t(25);
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
        static const uint8_t SP[3] = {80, 50, 30};
        uint32_t a = vx_t(SP[i]) + (uint32_t)i * 20860u;
        int32_t x = 1920 + vx_s(a + 16384u, RP[i]), y = 1952 + vx_s(a, RP[i] * 2 / 5);
        if (i == 1)
            cr_arc(x, y, 224, 56, 3129u, 0u, 65536u, 20, GD_BURNT);
        cr_blob(x, y, SZ[i], 7, i == 1 ? GD_BLUE : i ? 0xC50Fu : GD_BURNT, (uint32_t)i + 2u, 256, 256);
    }
}
static void vx_bush(int32_t bx, int32_t by, int32_t W0, int32_t H0, uint32_t sd, int32_t e)
{
    int32_t p = (256 - vx_beat()) * (256 - vx_beat()) >> 8, s = 256 + e * 36 / 256 * (102 + p * 154 / 256) / 256;
    int32_t W = W0 * s / 256, H = H0 * s / 256, sw = vx_s(vx_t(200) + sd * 10430u, 24) * e / 256, px, py, k;
    for (py = by - H; py < by; py += 48)
        for (px = bx - W / 2; px < bx + W / 2; px += 48) {
            int32_t u = (px - bx) * 512 / W, v = (by - py) * 256 / H, n = (int32_t)(gd_h((uint32_t)(px + sd * 997u), py) >> 8);
            uint16_t c;
            if (u * u + v * v * 230 / 256 > 65536 + 77 * (n - 128))
                continue;
            c = v > 184 ? GD_PBLUE : v < 77 ? 0x3290u : GD_BLUE;
            cr_rect(px + sw, py, 48, 48, n > 218 && v > 102 ? GD_PBLUE : c);
        }
    for (k = 0; k < 3; k++)                              /* a few berries */
        if (gd_h(sd, (uint32_t)k + 11u) & 32768u)
            cr_rect(bx + ((int32_t)(gd_h(sd * 7u, (uint32_t)k) >> 8) - 128) * W * 6 / 2560 + sw,
                    by - H * (77 + (int32_t)(gd_h(sd, (uint32_t)k * 3u) >> 9)) / 256, 48, 48, GD_ORANGE);
}
static void vx_shrub(int32_t e)
{
    vx_bush(704, 2816, 896, 640, 1, e);
    vx_bush(1920, 2848, 1248, 896, 2, e);
    vx_bush(3136, 2816, 896, 640, 3, e);
    vx_bush(1344, 2912, 544, 352, 4, e);
    vx_bush(2496, 2912, 544, 352, 5, e);
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
    case ENGI_SHRUB: vx_shrub(e); break;
    default: vx_beetle(e); break;                        /* (ANIMAL; DIGITAL its family's) */
    }
    gd.x0 = x0;
    gd.y0 = y0;
    gd.y1 = y1;
    return 1;
}
/* DRUM-X's KIT (lane -1) and SOUND 1..4 (lane): a ring of eight mushroom caps on stems, each jumping on its lane's hit,
 * the sound edited blue, a muted lane's cap pale; KIT over it on the left, the sound (struck last / edited) on the right */
static void vx_kit_page(const track_t *t, int32_t lane)
{
    char b[16];
    int32_t e[8], i;
    uint32_t l;
    vx_lanes(t, e);
    cr.cx = 120 * 16; cr.cy = 74 * 16; cr.s = 256;
    gd.x0 = 0; gd.y0 = -cv_oy; gd.y1 = (int32_t)cv_h - cv_oy;
    for (i = 0; i < 8; i++) {
        uint32_t a = (uint32_t)i * 8192u + 49152u;
        int32_t hit = e[i], x = 1920 + vx_s(a + 16384u, 992), y = 2000 + vx_s(a, 560) - hit * 96 / 256;
        cr_line(x, y + 160, x, y + 352, 32, GD_INK);
        cr_blob(x, y, 112 + hit * 48 / 256, 7, i == lane ? GD_BLUE : dx_lane_muted((uint32_t)i) ? GD_SURF : i & 1 ? GD_ORANGE : GD_BURNT,
                (uint32_t)i, 333, 205);
    }
    l = lane < 0 ? vxs.last & 7u : (uint32_t)lane & 7u;
    str_cpy(b, drum_lane_abbr(t, l), sizeof b);
    str_cpy(b + str_len(b), " \xB7 ", sizeof b - str_len(b));
    str_cpy(b + str_len(b), drum_lane_name(t, l), sizeof b - str_len(b));
    cv_text(8, 6 - AF_X_CAP_Y, &AF_X, "KIT", T_THEME);
    cv_text_r(232, 6 - AF_X_CAP_Y, &AF_X, b, lane < 0 ? T_TEXT : T_THEME, T_BG);
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
static void vx_fm6_page(const track_t *t, uint32_t sel, int op2)
{
    (void)op2;
    vx_fern(t, sel);
}
static void vx_fm6env(const track_t *t) { vx_fern(t, fop_op % 6u); }
/* (0.9.12's MOTION jellyfish, retired with Phase 5: the MSEQ bars alone) */
static int vx_mseq(const track_t *t, int32_t x, int32_t y, int32_t w, int32_t h)
{
    (void)t; (void)x; (void)y; (void)w; (void)h;
    return 0;
}
