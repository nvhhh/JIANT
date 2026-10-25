/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments
 * Copyright (C) 2026 Juan Ignacio Antonucci (Juan Jiant) */
/* (JIANT 0.9.x, GOOBERS) HOME's ECO in its other look, chosen by ui_eco_style (1; 0 keeps graph_ecosys). Included by
 * ui_lzhome.c at its end (after the ECO helpers it shares: eco_bot, eco_h, routes_count, home_set). Drawn with the
 * beings' own tools: og_line / og_line_a (antialiased line art, as ui_engvis.c's creatures), og_node, vx_col (the
 * creatures' thermal ramp: bg_col in every palette), the T_* tokens; no new colours.
 *   GOOBERS  one mushroom per awake track (TRACKS 4, 6 or 8): a cap with gills and spots, a stem with two eyes, two legs.
 *            They walk a forest floor at the pace of their track's level (silent: they rest). A faster one overtakes a
 *            slower one by hopping over it (a parabola; the pair swapped in the line), so louder means more swaps. The
 *            landing squashes it (a damped spring). Its body swells with its own level; a muted track sits, dim.
 *   SING     a note (the newest voice of the track, voice_t.age, read each frame as vx_age reads it: live or sequenced)
 *            rings out of its cap: two wobbly rings and a note; the note's pitch sets the height the note rises to
 *            (eased, so a new pitch glides; the goober keeps walking its path meanwhile).
 *   FOREST   twelve trees and grass on the floor. Band b (of the awake tracks; left = T1) is the trees in its place:
 *            their canopies grow leaves and glow with that track's level, their trunks sway with it. A note throws two
 *            fronts of light from its track's band, left and right through the canopy (timed in ms, ~1.2 s).
 *   SEAMLESS every position, speed, level, colour and size eases (gb_ease: an integer slew that always arrives): the
 *            walk's speed, the squash, the rise, the trees' light, the levels. The floor is a ring wider than the panel,
 *            its wrap off the panel. A track added or removed grows in or shrinks out. ANIM OFF eases the motion to rest.
 * State: gb (in the pool, .pool, as vxs in ui_engvis.c); nothing allocated per frame. */
#ifndef ui_eco_style
#define ui_eco_style 0                                  /* (fallback until the settings switch defines it: OG) */
#endif
#define GB_POOL __attribute__((section(".pool")))
#define GB_NT NTRK                                       /* goobers: one per track (4, 6 or 8 awake) */
#define GB_LO (16 << 8)                                  /* the floor: a corridor across the panel, Q8 px (the goobers turn at its ends) */
#define GB_HI (224 << 8)
#define GB_HOP 22                                        /* frames a hop takes */
#define GB_NTREE 12
#define GB_NG 36                                         /* grass tufts */
#define GB_NFR 8                                         /* light fronts: two per note, four notes at once */
#define GB_FRMS 1200                                     /* a front's life, ms (it runs 0.2 px a ms, both ways) */

static const int16_t GB_TX[GB_NTREE] = {6, 24, 41, 58, 73, 92, 110, 125, 144, 160, 180, 205};   /* (px, 0..240) */
static const uint8_t GB_TH[GB_NTREE] = {58, 66, 52, 70, 60, 56, 68, 54, 64, 72, 56, 62};       /* (px tall, lens panel) */

static struct {
    uint32_t f0, lastf, tm, init;                        /* the frame last stepped, the frame last drawn, the forest's time */
    int32_t mf;                                          /* motion 0..256: ANIM OFF eases it to rest */
    int32_t en[GB_NT];                                   /* each track's level (vx_level's slew, eased to 0 at rest) */
    int32_t grow[GB_NT];                                 /* present 0..256: a track added grows in, one removed shrinks out */
    int32_t x[GB_NT], v[GB_NT], spd[GB_NT];              /* the goober on the floor: position, base pace, signed speed (Q8 px a frame) */
    int8_t dir[GB_NT];                                   /* which way it walks: +1 right, -1 left */
    int32_t rise[GB_NT];                                 /* the note's height over the cap, eased (Q4 px) */
    int32_t sqa[GB_NT], sqv[GB_NT];                      /* the landing squash: a damped spring (Q8, 256 = the whole) */
    uint8_t hop[GB_NT], cool[GB_NT];                     /* a hop (frames in, 0 none), the cool-down after one */
    uint32_t age[GB_NT], nms[GB_NT];                     /* the newest voice seen (voice_t.age), when it came (fm1_ms) */
    int16_t nt[GB_NT];                                   /* .. its note */
    int16_t fx[GB_NFR];                                  /* light fronts: origin (px), direction, when (fm1_ms) */
    int8_t fd[GB_NFR];
    uint32_t fms[GB_NFR];
    uint8_t fn;                                          /* the next front pair */
    int16_t tl[GB_NTREE], tf[GB_NTREE];                  /* the trees: their band's level, the light through them (eased) */
    int16_t gl[GB_NG];                                   /* the grass: its band's level (eased) */
} gb GB_POOL;
static int32_t gb_x0;                                    /* the strip drawn: its x0 (SKIN HUD's strips; 0 on the lens) */
static uint32_t gb_fresh;                                /* 1: notes newer than the last look sing (not after a long while off) */

/* (GOOBERS on: ui_eco_style 1) */
int goobers_on(void) { return ui_eco_style == 1; }

static uint32_t gb_nlive(void) { return trk_live < GB_NT ? trk_live : GB_NT; }
static int32_t gb_nband(void) { uint32_t n = gb_nlive(); return n ? (int32_t)n : 1; }

/* an integer ease toward t by 1/div of the way each frame, always arriving (a step of at least 1) */
static int32_t gb_ease(int32_t cur, int32_t t, int32_t div)
{
    int32_t d = t - cur, s = d / div;
    if (!s && d) s = d > 0 ? 1 : -1;
    return cur + s;
}

/* track c's level, as ui_engvis's vx_level slews it (rises a third of the way a frame, falls slower) */
static int32_t gb_lvl(uint32_t c, int32_t en)
{
    int32_t v = trk[c].peak * 256 / 7000;
    v = clamp(v, 0, 256);
    return v > en ? v : en + (v - en - 2) / 3;
}

/* the newest voice of track c (read as vx_age reads it: the active voice with the largest age): a note new since the
 * last look sings and lights its band (fronts left and right from the band's middle); the ages only otherwise */
static void gb_notes(uint32_t c)
{
    uint32_t i, a = 0;
    int32_t n = 60;
    for (i = 0; i < NVOICE; i++)
        if (trk[c].v[i].active && trk[c].v[i].age > a) {
            a = trk[c].v[i].age;
            n = trk[c].v[i].note;
        }
    if (a > gb.age[c] && gb_fresh) {
        int32_t band = (2 * (int32_t)c + 1) * 240 / (2 * gb_nband());
        uint32_t f = (gb.fn++ & 3u) * 2u;
        gb.nms[c] = fm1_ms;
        gb.nt[c] = (int16_t)n;
        gb.fx[f] = gb.fx[f + 1u] = (int16_t)band;
        gb.fd[f] = -1;
        gb.fd[f + 1u] = 1;
        gb.fms[f] = gb.fms[f + 1u] = fm1_ms;
    }
    if (a > gb.age[c])
        gb.age[c] = a;
}

/* the walk, dt frames: the motion eases, each goober's signed pace follows its level (silent: it stands), turns at the
 * ends of the floor (its speed eases through rest), hops over a neighbour ahead of it (more often when louder), the
 * landing springs; a track's presence eases in and out */
static void gb_step(uint32_t dt, uint32_t still, uint32_t live)
{
    uint32_t k, c, j;
    for (k = 0; k < dt; k++) {
        gb.mf = gb_ease(gb.mf, still ? 0 : 256, 6);
        for (c = 0; c < GB_NT; c++) {
            int32_t tgt, ahead, m;
            gb.grow[c] = gb_ease(gb.grow[c], c < live ? 256 : 0, 6);
            if (!gb.grow[c]) {                           /* (not on the floor: nothing moves) */
                gb.hop[c] = 0;
                gb.spd[c] = 0;
                continue;
            }
            tgt = gb.dir[c] * (gb.v[c] * gb.en[c] * 3 / 512);   /* louder, faster: 1.5 x its pace at full level */
            tgt = tgt * gb.mf / 256;
            if (gb.hop[c]) tgt += gb.dir[c] * 160;       /* (the hop carries it over the other) */
            gb.spd[c] = gb_ease(gb.spd[c], tgt, 5);
            gb.x[c] += gb.spd[c];
            if (gb.x[c] < GB_LO) { gb.x[c] = GB_LO; gb.dir[c] = 1; }      /* a wall: it turns (the ease brings it round) */
            if (gb.x[c] > GB_HI) { gb.x[c] = GB_HI; gb.dir[c] = -1; }
            if (gb.hop[c] && ++gb.hop[c] > GB_HOP) {     /* lands: a squash, pushed by its impulse */
                gb.hop[c] = 0;
                gb.sqv[c] += 40;
                gb.cool[c] = (uint8_t)(8 + (256 - gb.en[c]) * 12 / 256);
            }
            gb.sqv[c] += (-gb.sqa[c] * 60 >> 8) - (gb.sqv[c] * 80 >> 8);
            gb.sqa[c] += gb.sqv[c];
            if (gb.cool[c]) gb.cool[c]--;
            else if (!gb.hop[c] && gb.en[c] > 0 && gb.mf > 64 && c < live) {
                m = gb.spd[c] < 0 ? -gb.spd[c] : gb.spd[c];
                for (j = 0; j < live; j++) {             /* one ahead, close (more when louder): hop over it when it's the
                                                          * other way, or slower; the same way and faster: it hops it */
                    int32_t mj;
                    if (j == c) continue;
                    ahead = gb.dir[c] > 0 ? gb.x[j] - gb.x[c] : gb.x[c] - gb.x[j];
                    mj = gb.spd[j] < 0 ? -gb.spd[j] : gb.spd[j];
                    if (ahead > -3 * 256 && ahead < (5 + gb.en[c] * 6 / 256) * 256 &&
                        (gb.dir[j] != gb.dir[c] || m > mj + 16)) {
                        gb.hop[c] = 1;
                        break;
                    }
                }
            }
        }
    }
    if (dt) gb.tm += dt * (120u + 4u * (uint32_t)(gb.en[0] > 0 ? gb.en[0] : 0));
}

/* a goober: its feet at (X px, gnd px) lifted by hq4 (Q4, the hop), its whole body k (Q8, the presence and its level),
 * its squash sqa (Q8), its legs lifted lfl / rfl (Q4); col its colour, still: dim, eyes shut */
static void gb_goober(int32_t X, int32_t gnd, int32_t hq4, int32_t k, int32_t sqa, int32_t lfl, int32_t rfl, uint16_t col, int still)
{
    int32_t cx = X * 16, gy = gnd * 16 - hq4, sx = 256 + sqa, sy = 256 - sqa;
    int32_t kx = k * sx >> 8, ky = k * sy >> 8, px = 0, py = 0, i;
    uint16_t ec = still ? (T_DIM) : vk_heat(248);
#define GB_GX(d) (cx + ((d) * kx >> 8))                  /* (d: Q4 px from the feet, scaled) */
#define GB_GY(d) (gy + ((d) * ky >> 8))
    /* legs: from the stem's foot to the ground, a stepping one lifted */
    og_line(GB_GX(-32), GB_GY(-16), GB_GX(-40), GB_GY(32 - lfl), col);
    og_line(GB_GX(-40), GB_GY(32 - lfl), GB_GX(-50), GB_GY(32 - lfl), col);
    og_line(GB_GX(32), GB_GY(-16), GB_GX(40), GB_GY(32 - rfl), col);
    og_line(GB_GX(40), GB_GY(32 - rfl), GB_GX(50), GB_GY(32 - rfl), col);
    /* stem: flaring a little at the foot */
    og_line(GB_GX(-40), GB_GY(0), GB_GX(-24), GB_GY(-112), col);
    og_line(GB_GX(40), GB_GY(0), GB_GX(24), GB_GY(-112), col);
    og_line(GB_GX(-40), GB_GY(0), GB_GX(40), GB_GY(0), col);
    /* cap: a dome of (128, 144) Q4 on the stem's top, its base flat */
    for (i = 0; i <= 6; i++) {
        uint32_t a = 0x8000u + (uint32_t)i * 0x8000u / 6u;
        int32_t x = GB_GX(128 * og_cos(a) >> 15), y = GB_GY(-112 + (144 * og_sin(a) >> 15));
        if (i) og_line(px, py, x, y, col);
        px = x;
        py = y;
    }
    og_line(GB_GX(128), GB_GY(-112), GB_GX(-128), GB_GY(-112), col);
    og_line(GB_GX(-77), GB_GY(-128), GB_GX(77), GB_GY(-128), ux_mix(T_SURF, col, 150));   /* gills */
    og_node(GB_GX(-51), GB_GY(-184), 1, col);                                              /* spots */
    og_node(GB_GX(38), GB_GY(-198), 1, col);
    /* eyes on the stem: two dots, or two short lines when it sits */
    if (still) {
        cv_rect(GB_GX(-24) >> 4, GB_GY(-56) >> 4, 2, 1, ec);
        cv_rect(GB_GX(8) >> 4, GB_GY(-56) >> 4, 2, 1, ec);
    } else {
        cv_rect(GB_GX(-24) >> 4, GB_GY(-56) >> 4, 1, 1, ec);
        cv_rect(GB_GX(24) >> 4, GB_GY(-56) >> 4, 1, 1, ec);
    }
#undef GB_GX
#undef GB_GY
}

/* the note of goober c: two wobbly rings from its cap and a note, rising to its pitch; dt_ms since it came (< 1 s) */
static void gb_sing(uint32_t c, int32_t X, int32_t gnd, uint32_t dt_ms)
{
    int32_t a = 256 - (int32_t)(dt_ms * 256u / 1000u), k, i;
    int32_t by = gnd * 16 - 24 * 16 - gb.rise[c], cx = X * 16;
    uint16_t col = vx_col(clamp(120 + (gb.nt[c] - 48) * 4, 0, 255));
    if (a <= 0) return;
    for (k = 0; k < 2; k++) {
        int32_t rad = 3 + (int32_t)(dt_ms * 10u / 1000u) + 5 * k, px = 0, py = 0, ak = a * (k ? 160 : 256) >> 8;
        for (i = 0; i <= 10; i++) {
            uint32_t ang = (uint32_t)i * 65536u / 10u;
            int32_t wob = bg_sin(gb.tm / 2u + k * 9000u + i * 6553u), r = rad * 16 + (wob * rad * 16 >> 17);
            int32_t x = cx + (r * og_cos(ang) >> 15), y = by + (r * og_sin(ang) >> 15);
            if (i) og_line_a(px, py, x, y, col, ak);
            px = x;
            py = y;
        }
    }
    {   /* the note: a head and its stem, rising a little as it fades */
        int32_t hx = cx + 160, hy = by - (int32_t)(dt_ms * 6u / 1000u) * 16;
        if (a > 96) og_node(hx, hy, 2, col);
        og_line_a(hx + 30, hy, hx + 30, hy - 96, col, a);
    }
}

/* a tree's canopy: an outer wobbling outline of radius r (Q4) at (cx, cy) Q4, an inner ring and its leaves as short
 * strokes whose strength is the level (alpha, so they grow in) */
static void gb_canopy(int32_t cx, int32_t cy, int32_t r, uint16_t col, int32_t tl, uint32_t tt)
{
    static const int8_t LX[8] = {-1, 0, 1, -1, 1, -1, 0, 1}, LY[8] = {-1, -1, -1, 0, 0, 1, 1, 1};
    int32_t i, j, px = 0, py = 0, rr;
    for (i = 0; i <= 11; i++) {
        uint32_t ang = (uint32_t)i * 65536u / 11u;
        rr = r + (r * bg_sin(tt + (uint32_t)i * 7281u) >> 18);            /* (a wobble of 1/8 of it) */
        {
            int32_t x = cx + (rr * og_cos(ang) >> 15), y = cy + (rr * og_sin(ang) >> 15) * 4 / 5;
            if (i) og_line(px, py, x, y, col);
            px = x;
            py = y;
        }
    }
    px = py = 0;
    for (i = 0; i <= 7; i++) {                           /* the inner ring: the crown's heart */
        uint32_t ang = (uint32_t)i * 65536u / 7u;
        rr = r * 11 / 20 + (r * bg_sin(tt * 2u + (uint32_t)i * 9000u) >> 19);
        {
            int32_t x = cx + (rr * og_cos(ang) >> 15), y = cy + (rr * og_sin(ang) >> 15) * 4 / 5;
            if (i) og_line_a(px, py, x, y, col, 170);
            px = x;
            py = y;
        }
    }
    for (j = 0; j < 8; j++) {                            /* leaves on the rim: their strength by the level, each its own */
        int32_t al = clamp((tl - j * 30) * 6, 0, 256), ex, ey;
        if (al > 0) {
            ex = cx + LX[j] * (r * 5 / 8);
            ey = cy + LY[j] * (r * 5 / 8) * 4 / 5;
            og_line_a(ex, ey, ex + LX[j] * 32, ey + LY[j] * 26, col, al);      /* (a 2 px tick out of the rim) */
        }
    }
}

/* the forest: the floor, the grass tufts, and the trees (tapered trunks, a fork of branches, the canopy), their levels and
 * light eased, swaying by mf (rest); drawn behind the goobers */
static void gb_forest(int32_t x0, int32_t gnd, int32_t sc, uint32_t live, int32_t mf)
{
    int32_t nb = gb_nband(), i;
    cv_rect(-x0, gnd + 2, 240, 1, ux_mix(T_SURF, T_LINE, 60));                /* the floor */
    for (i = 0; i < GB_NG; i++) {                        /* grass: tufts of three blades, leaning with their band's level */
        uint32_t h = (uint32_t)i * 2654435761u;
        int32_t gx = (int32_t)((h >> 8) % 240u), gh = 3 + (int32_t)((h >> 20) & 3u), b;
        uint16_t tone = ux_mix(T_SURF, T_LINE, 62 + gb.gl[i] * 24 / 256);
        for (b = -1; b <= 1; b++) {
            int32_t lean = (bg_sin(gb.tm / 2u + (uint32_t)(i * 3 + b + 1) * 3000u) * (2 + gb.gl[i] / 48) >> 15) * mf / 256;
            int32_t hb = gh + (b == 0 ? 2 : 0);
            og_line((gx - x0 + b * 2) * 16, (gnd + 2) * 16, (gx + lean + b * 2 - x0) * 16, (gnd + 2 - hb) * 16, tone);
        }
    }
    for (i = 0; i < GB_NTREE; i++) {                     /* trees */
        int32_t tx = GB_TX[i], lv = gb.tl[i], lit = gb.tf[i], th, bend, top, cr, h, cx, cy, bx, j;
        uint16_t cc, bc = ux_mix(T_SURF, T_MID, 70);
        uint32_t tt = gb.tm / 2u + (uint32_t)i * 4000u;
        th = GB_TH[i] * sc >> 8;
        bend = (bg_sin(tt) * (32 + lv * 48 / 256) >> 15) * mf / 256;      /* (Q4 px) */
        top = gnd - th;
        cr = (8 + lv * 4 / 256) * 16 + lit / 8;         /* (Q4: the canopy's radius, the light swells it up to 2 px) */
        h = clamp(23 + lv * 118 / 256 + lit * 90 / 256, 0, 255);
        cc = tx * nb / 240 < (int32_t)live ? vx_col(h) : T_DIM;
        cx = (tx - x0) * 16 + bend;
        cy = (top + 2) * 16;
        /* the trunk: tapered, its foot wider (Q4: 2 px at the ground, 0.75 px at the top), roots at the foot */
        og_line((tx - x0) * 16 - 32, gnd * 16, (tx - x0) * 16 + bend - 12, top * 16, bc);
        og_line((tx - x0) * 16 + 32, gnd * 16, (tx - x0) * 16 + bend + 12, top * 16, bc);
        og_line((tx - x0) * 16 - 40, (gnd + 2) * 16, (tx - x0) * 16 - 32, gnd * 16, bc);
        og_line((tx - x0) * 16 + 40, (gnd + 2) * 16, (tx - x0) * 16 + 32, gnd * 16, bc);
        /* a fork: two branches from the trunk's upper half up into the crown */
        bx = (tx - x0) * 16 + bend / 2;
        for (j = -1; j <= 1; j += 2) {
            int32_t fy = gnd * 16 + (top * 16 - gnd * 16) * 45 / 100;
            og_line(bx, fy, cx + j * cr * 5 / 8, cy + cr / 3, bc);
        }
        gb_canopy(cx, cy, cr, cc, lv, tt);
    }
}

/* the forest's targets for this frame: each tree's band (its level, the light of the fronts), each tuft's band */
static void gb_targets(uint32_t live, int32_t nb, uint32_t still)
{
    int32_t i, k;
    for (i = 0; i < GB_NTREE; i++) {
        int32_t band = GB_TX[i] * nb / 240, tgt = band < (int32_t)live && !still ? gb.en[band] : 0, lit = 0;
        if (!still)
            for (k = 0; k < GB_NFR; k++) {
                uint32_t age = fm1_ms - gb.fms[k];
                int32_t d, f;
                if (gb.fms[k] == 0u || age >= GB_FRMS) continue;
                f = gb.fx[k] + gb.fd[k] * (int32_t)(age / 5u);
                d = GB_TX[i] - f;
                if (d < 0) d = -d;
                if (d < 40) lit += (40 - d) * 6 * (int32_t)(GB_FRMS - age) / GB_FRMS;
            }
        gb.tl[i] = (int16_t)gb_ease(gb.tl[i], tgt, 4);
        gb.tf[i] = (int16_t)gb_ease(gb.tf[i], clamp(lit, 0, 255), 3);
    }
    for (i = 0; i < GB_NG; i++) {
        uint32_t h = (uint32_t)i * 2654435761u;
        int32_t gx = (int32_t)((h >> 8) % 240u), band = gx * nb / 240, tgt = band < (int32_t)live && !still ? gb.en[band] : 0;
        gb.gl[i] = (int16_t)gb_ease(gb.gl[i], tgt, 4);
    }
}

/* ECO's GOOBERS view in the panel as graph_eco draws it: x0 the strip's, y0 the panel's top (cv_oy); words: the labels */
static void goobers_draw(int32_t x0, int32_t y0, int words)
{
    uint32_t c, live = gb_nlive(), still = (ui_prefs & PREF_ANIM_OFF) != 0u, dt, fresh;
    int32_t hud = hud_home(), gnd = eco_bot() - 10, sc = clamp(eco_h() * 256 / 140, 128, 512), nb = gb_nband();
    char b[16];
    dt = ui.frame - gb.f0;
    if (dt > 8u) dt = 8u;
    gb.f0 = ui.frame;
    fresh = ui.frame - gb.lastf <= 30u;                  /* (off this view a while: it grows in again, notes not sung) */
    gb.lastf = ui.frame;
    if (!gb.init) {                                      /* (the first frame: the places and the paces) */
        for (c = 0; c < GB_NT; c++) {
            gb.x[c] = (GB_LO + (int32_t)c * ((GB_HI - GB_LO) / 7)) ;
            gb.dir[c] = (c & 1u) ? -1 : 1;
            gb.v[c] = 70 + (int32_t)((c * 5u) % 8u) * 16;
        }
        gb.init = 1;
    }
    if (!fresh) {
        gb.mf = 0;
        for (c = 0; c < GB_NT; c++) {
            gb.en[c] = 0;
            gb.grow[c] = 0;
            gb.rise[c] = 0;
            gb.hop[c] = 0;
            gb.spd[c] = 0;
        }
        for (c = 0; c < GB_NTREE; c++) gb.tl[c] = gb.tf[c] = 0;
        for (c = 0; c < GB_NG; c++) gb.gl[c] = 0;
    }
    gb_fresh = fresh && !still;
    for (c = 0; c < GB_NT; c++) {                        /* each track's level, eased once a frame (rest when ANIM OFF) */
        gb_notes(c);
        if (!dt) continue;                               /* (a redraw in the same frame draws the same) */
        if (c < live && !still) gb.en[c] = gb_lvl(c, gb.en[c]);
        else gb.en[c] = gb_ease(gb.en[c], 0, 6);
        if (gb.nms[c] && fm1_ms - gb.nms[c] < 1000u) gb.rise[c] = gb_ease(gb.rise[c], clamp((gb.nt[c] - 48) * 20 / 36, 0, 22) * 16, 4);
        else gb.rise[c] = gb_ease(gb.rise[c], 0, 4);
    }
    gb_step(dt, still, live);
    gb_x0 = x0;
    cv_oy = -y0;
    if (dt) gb_targets(live, nb, still);
    gb_forest(x0, gnd, sc, live, gb.mf);
    for (c = 0; c < GB_NT; c++) {
        int32_t g = gb.grow[c], X, lv = gb.en[c], hq4 = 0, lfl, rfl, k, ph;
        uint16_t col;
        int dim = trk[c].p[P_MUTE] != 0;
        if (!g) continue;
        X = (gb.x[c] >> 8) - x0;
        if (gb.hop[c]) {                                 /* the parabola: 14 px at its top, from and to the ground */
            int32_t u = (int32_t)(gb.hop[c] - 1) * 256 / (GB_HOP - 1);
            hq4 = 896 * u * (256 - u) >> 16;
        }
        ph = (int32_t)(((uint32_t)gb.x[c] * 9362u >> 8) & 0xFFFFu);  /* the step: the legs' phase (7 px a turn), smooth */
        {
            int32_t sl = bg_sin((uint32_t)ph), sr = bg_sin((uint32_t)ph + 32768u);
            lfl = (sl > 0 ? sl : 0) * 24 / 32768 * lv / 256;      /* (the stepping leg lifts, the other down) */
            rfl = (sr > 0 ? sr : 0) * 24 / 32768 * lv / 256;
        }
        k = g * (336 + lv * 32 / 256) / 256;             /* its body: present, and swelling with its level (cap ~20 px) */
        col = dim ? T_DIM : vx_col(clamp(23 + lv * 118 / 256, 0, 255));
        if (X < -16 || X > 256) continue;                /* (on the floor's corridor: always on the panel) */
        gb_goober(X, gnd, hq4, k, gb.sqa[c], lfl, rfl, col, still || dim);
        if (!still && gb.nms[c] && fm1_ms - gb.nms[c] < 1000u)
            gb_sing(c, X, gnd, fm1_ms - gb.nms[c]);
    }
    if (words) {                                         /* the track numbers under them (ECO's) and the MACROS words */
        for (c = 0; c < GB_NT; c++) {
            int32_t g = gb.grow[c], X = (gb.x[c] >> 8) - x0, ny = clamp(gnd + 6, 4, eco_bot());
            char num[2] = {(char)('1' + c), 0};
            uint16_t tone;
            if (!g) continue;
            if (hud && !(ny >= (int32_t)graph_y() + (home_set() ? 12 : 2) && ny + 14 <= (int32_t)(graph_y() + graph_h())))
                continue;
            tone = trk[c].p[P_MUTE] ? T_DIM : c == song.sel ? T_THEME : hud ? T_TEXT : T_MID;
            cv_text_in(X - 10, ny, 20, &AF_S, num, ux_mix(T_SURF, tone, g * 100 / 256), T_SURF);   /* (it fades in with it) */
        }
        if (home_set()) {
            int32_t y = hud ? (int32_t)graph_y() + 3 : 2;
            fmt_int(b, (int32_t)routes_count());
            str_cpy(b + str_len(b), " ROUTES", 8);
            cv_text(6, y, &AF_X, b, hud ? T_TEXT : T_DIM);
            cv_text_r(234, y, &AF_X, mac_latch ? "LATCH" : "HOLD LFO", mac_latch ? T_ACCENT : hud ? T_TEXT : T_DIM, T_SURF);
        }
    }
    cv_oy = 0;
}
