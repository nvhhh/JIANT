/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments
 * Copyright (C) 2026 Juan Ignacio Antonucci (Juan Jiant) */
/* (0.8.5) FX's lens (ui.c fx_*): the panels of its three levels, drawn as the approved HTML mock-up (FX, the proposal
 * "LENTE"), in the lens's panel (240 x 140 at y 84; the mock-up's coordinates less 84). Included by ui_graph.c.
 *   TRACK   the track's set (SENDS, DIST, AUTO PAN): its place as dots and its name at the top, its picture (the sends
 *           as four bars, DIST's curve, AUTO PAN's swing), the DIST curve small under it
 *   BUSES   the six modules as tiles (3 x 2, 76 x 68: DELAY REVERB CHORUS SLICER MASTER PUNCH), each its name and its
 *           picture small; the one PRESETS picked TINT with a THEME frame; one LOAD ECO took away: dark, "ECO"
 *   EFFECT  the module picked large: its pages as dots, "DELAY . DLY 2" at the right, its picture
 * The pictures (fxg_*, any size): DELAY the echoes as bars, MIX high, each FDBK lower, TIME apart (PING-PONG: L / R),
 * REVERB its decay filled (its TYPE: SPRING rippled, CLOUD wavy), CHORUS three sines DEPTH deep, STEREO apart, SLICER its
 * 16 steps (the pattern's open ones lit, the step playing framed), MASTER the output now as a bar over CLIP, the
 * DUCK's curve, PUNCH the lane's steps, SL PITCH its 16 pitches from the middle, the sends four bars, DIST its transfer
 * curve in a box with TYPE and TONE, AUTO PAN the swing's span with its dot; heat colours (JIANT's thermal ramp; another
 * palette: its theme toward its accent).
 * (0.8.5, FX live) While it sounds (playing, or the output above silence: bg_energy) they move with it: the echoes
 * swell with the level and a pulse runs along them, the tail swells and a line sweeps it, the chorus lines run at
 * RATE, the slicer's step playing is framed, MASTER's meter follows the output, PUNCH's playhead walks the lane, DIST's
 * level is a dot on its curve, AUTO PAN's dot swings at RATE. Drawn on the panel's redraws only (every other frame
 * while it sounds: graph_signature); nothing of the audio path */
#define FXG_H(v) heat_col((int32_t)(v))                 /* (0..256) */
/* (GARDEN, the owner on the device) the pictures' motion at half the frames' pace: ui.frame >> 1 */
static int32_t fxg_en(void) { return bg_energy() > 2 ? 32 + bg_energy() : 0; }   /* 0: still (GARDEN: the sound, not the transport: silence is still) */
static void fxg_label(int32_t x, int32_t y, const char *s, uint16_t c, int right)
{
    if (right)                                          /* (AF_X 3 rows down: S's capitals' middle) */
        cv_text_r(x, y + 3, &AF_X, s, c, cv_bg);
    else
        cv_text(x, y + 3, &AF_X, s, c);
}
static void fxg_delay(int32_t x, int32_t y, int32_t w, int32_t h)
{
    char b[24];
    const char *u;
    int32_t i, a = 1000, mix = song.g[G_DMIX], fb = song.g[G_DFDBK], en = fxg_en();
    int32_t tr = RATIO(&GP[G_DTIME], enum_rank(&GP[G_DTIME], song.g[G_DTIME])), step = w * (500 + tr) / 10000;
    int32_t pulse = en && step > 0 ? (int32_t)((ui.frame >> 1) * 2u % (uint32_t)(9 * step)) : -99;   /* (the echo running) */
    cv_rect(x, y + h - 1, w, 1, T_LINE);
    for (i = 0; i < 9 && a > 8; i++) {
        int32_t px = x + 6 + i * step, bh = (h - 14) * 9 / 10 * a / 1000 * (mix * 7 / 10 + 38) / 127;
        int hit = pulse >= i * step && pulse < (i + 1) * step;
        if (px + 4 > x + w)
            break;
        if (en)
            bh = bh * (176 + clamp(en, 0, 288) * 80 / 288) / 256;
        if (bh > 0)
            cv_rect(px, y + h - 1 - bh, 4, bh, hit ? FXG_H(240) : FXG_H(64 + a * 179 / 1000));
        if (song.g[G_DPING] && w > 120) {
            b[0] = i & 1 ? 'R' : 'L';
            b[1] = 0;
            fxg_label(px - 1, y + 9, b, T_DIM, 0);
        }
        a = a * (fb > 127 ? 127 : fb) / 127;
    }
    if (pulse >= 0 && x + 6 + pulse + 2 <= x + w)     /* (the pulse itself, on the line) */
        cv_rect(x + 6 + pulse, y + h - 3, 3, 2, T_TEXT);
    param_format(&GP[G_DTIME], song.g[G_DTIME], b, &u);
    if (w > 120) {
        str_cpy(b + str_len(b), " \xB7 ", 4);
        str_cpy(b + str_len(b), song.g[G_DTYPE] ? "DIGITAL" : "ANALOG", 9);
        fxg_label(x + w, y, song.g[G_DCOLOR] < 58 ? "DARKER" : song.g[G_DCOLOR] > 70 ? "BRIGHTER" : "EXACT COPY", T_DIM, 1);
    }
    fxg_label(x, y, b, T_MID, 0);
}
static void fxg_reverb(int32_t x, int32_t y, int32_t w, int32_t h)
{
    int32_t i, sz = song.g[G_RSIZE], ty = song.g[G_RTYPE], e = 1000, k = 1000 - (5000 - sz * 4300 / 127) / (w ? w : 1);
    int32_t top = y + 12, hh = h - 12, en = fxg_en(), sweep = en && w > 0 ? (int32_t)((ui.frame >> 1) * 3u % (uint32_t)w) : -1;
    char b[16];
    for (i = 0; i < w; i++) {                        /* the decay, a column each: its height the envelope */
        int32_t env = i < w / 25 + 1 ? e * i / (w / 25 + 1) : e, n = 1000, ch;
        if (ty == 4) n = 500 + (bg_sin((uint32_t)(i * 65536 / (w ? w : 1) * 6)) >> 6) * 500 / 512;
        else if (ty == 1) n = 700 + (bg_sin((uint32_t)(i * 65536 / (w ? w : 1) * 14)) >> 6) * 300 / 512;
        ch = hh * 9 / 10 * env / 1000 * n / 1000;
        if (en)                                         /* (the tail swells with the output) */
            ch = ch * (168 + clamp(en, 0, 288) * 88 / 288) / 256;
        if (ch > 0)
            cv_rect(x + i, top + hh - ch, 1, ch, i == sweep ? T_TEXT : FXG_H(218 - i * 180 / (w ? w : 1)));
        e = e * k / 1000;
    }
    fxg_label(x, y, N_RTYPE[clamp(ty, 0, 4)], T_MID, 0);
    if (w > 120) {
        str_cpy(b, "PRE ", sizeof b);
        fmt_int(b + 4, song.g[G_RPRE]);
        str_cpy(b + str_len(b), " MS", 4);
        fxg_label(x + w, y, b, T_DIM, 1);
    }
}
static void fxg_chorus(int32_t x, int32_t y, int32_t w, int32_t h)
{
    int32_t k, i, dep = song.g[G_CDEPTH], st = song.g[G_CSTER], mid = y + 8 + (h - 8) / 2;
    uint32_t ph = fxg_en() ? (ui.frame >> 1) * (uint32_t)(song.g[G_CRATE] + 8) * 48u : 0u;   /* (running at RATE while it sounds) */
    char b[16];
    for (k = 0; k < 3; k++) {
        int32_t py = 0;
        for (i = 0; i <= w; i++) {
            int32_t amp = (h - 8) * 35 / 100 * dep / 127 * (10 + 2 * k) / 10 * (128 + st) / 128 *
                          (200 + gd_pulse(1u, 1u, (uint32_t)k * 12000u) * 112 / 256) / 256;   /* (the owner: each line pulsing on the beat) */
            int32_t yy = mid + (bg_sin((uint32_t)(i * 65536 / (w ? w : 1) * 2) + (uint32_t)k * 21000u + ph) * amp >> 15);
            yy = clamp(yy, y + 9, y + h - 1);
            if (i)
                cv_line(x + i - 1, py, x + i, yy, FXG_H(77 + k * 64));
            py = yy;
        }
    }
    str_cpy(b, "STEREO ", sizeof b);
    b[7] = st > 0 ? '+' : 0;
    fmt_int(b + 7 + (st > 0), st);
    fxg_label(x, y, b, T_MID, 0);
}
static void fxg_slicer(int32_t x, int32_t y, int32_t w, int32_t h)
{
    uint32_t i, pat = sl_pattern(), mode = (uint32_t)song.g[G_SLMODE], play = mode && song.playing ? sl.idx % 16u : 99u;
    int32_t cw = w / 16, x0 = x + (w - 16 * cw) / 2, lw;
    char b[32];
    const char *u;
    for (i = 0; i < 16u; i++) {
        int on = ((pat >> i) & 1u) && mode;
        int32_t cx = x0 + (int32_t)i * cw;
        cv_rect(cx + 1, y + 12, cw - 2, h - 18, on ? FXG_H(102 + song.g[G_SLDEP] * 128 / 127) : T_RAISE);
        if (i == play)
            cv_frame(cx, y + 11, cw, h - 16, T_TEXT);
    }
    str_cpy(b, mode == SL_OFF ? "OFF" : mode == SL_STUT ? "STUT" : "GATE", sizeof b);
    str_cpy(b + str_len(b), " ", 2);
    if (w > 120) str_cpy(b + str_len(b), "\xB7 PAT ", 8);
    fmt_int(b + str_len(b), song.g[G_SLPAT]);
    if (w > 120) {
        char r[8];
        param_format(&GP[G_SLRATE], song.g[G_SLRATE], r, &u);
        str_cpy(b + str_len(b), " \xB7 ", 4);
        str_cpy(b + str_len(b), r, 8);
    }
    lw = text_w(&AF_X, b);
    fxg_label(x, y, b, T_MID, 0);
    if (w > 120) {                                   /* the tracks in it, FX */
        uint32_t n = 0;
        for (i = 0; i < NTRK && n + 4u < sizeof b; i++)
            if (song.g[G_SLT1 + i]) { b[n++] = 'T'; b[n++] = (char)('1' + i); b[n++] = ' '; }
        b[n] = 0;
        if (!n) str_cpy(b, "NONE ", sizeof b);
        if (song.g[G_SLFX]) str_cpy(b + str_len(b), "+FX", 4);
        {   /* (at the right, clear of the mode's words: ellipsised when it must) */
            int32_t rw = text_w(&AF_X, b), l = x + lw + 14, rx = x + w - rw < l ? l : x + w - rw;
            cv_free_text(rx, y + 3, &AF_X, b, T_DIM, cv_bg, x + w - rx);
        }
    }
}
static void fxg_master(int32_t x, int32_t y, int32_t w, int32_t h)
{
    int32_t en = bg_energy(), lvl = en > 2 ? 128 + en / 2 : 26, i, py = 0, duck = song.g[G_DUCK];
    int32_t rel = song.g[G_DREL], cy = y + 30, ch = h - 32;
    char b[24];
    cv_rect(x, y + 2, w, 9, T_RAISE);
    cv_rect(x, y + 2, w * lvl / 256 > 1 ? w * lvl / 256 : 1, 9, FXG_H(lvl));
    str_cpy(b, "CLIP ", sizeof b);
    fmt_int(b + 5, song.g[G_CLIP]);
    if (w > 120) {
        str_cpy(b + str_len(b), " \xB7 PUNCH ", 10);
        fmt_int(b + str_len(b), song.g[G_PUNCH]);
        fxg_label(x + w, y + 14, "DUCK", T_DIM, 1);
    }
    fxg_label(x, y + 14, b, T_MID, 0);
    {   /* the duck's gain over a beat: down by DUCK, back over REL (e^-u k, a factor a column) */
        int32_t kk = clamp(8000 - rel * 50, 1600, 8000), e = 1000;
        for (i = 0; i <= w; i++) {
            int32_t d = 1000 - duck * e / 127, yy = cy + ch - d * ch / 1000;
            if (i)
                cv_line(x + i - 1, py, x + i, yy, FXG_H(154));
            py = yy;
            e = e * (1000 - kk / (w ? w : 1)) / 1000;
        }
    }
}
static void fxg_punch(int32_t x, int32_t y, int32_t w, int32_t h)
{
    static const uint8_t RES[3] = {8, 16, 32};
    int32_t steps = song.g[G_PFXLEN] * RES[clamp(song.g[G_PFXRES], 0, 2)] / 8, i, cw;
    char b[32];
    if (steps > 64) steps = 64;
    if (steps < 1) steps = 1;
    cw = w / steps;
    if (cw < 1) cw = 1;
    for (i = 0; i < steps && i * cw < w; i++) {
        int on = pfx_lane_at((uint32_t)i) != 0u, at = song.playing && (uint32_t)i == pfx_lane_pos();   /* (the playhead) */
        if (on || at)                                   /* (0.8.9, the skins' mock-up: the open steps as lines, an empty one a
                                                         * tick at the foot) */
            cv_rect(x + i * cw, y + 14, cw > 2 ? cw - 1 : 1, h - 20, at ? T_TEXT : FXG_H(179));
        else
            cv_rect(x + i * cw, y + h - 7, cw > 2 ? cw - 1 : 1, 1, T_LINE);
    }
    str_cpy(b, "LANE ", sizeof b);
    fmt_int(b + 5, song.g[G_PFXLEN]);
    str_cpy(b + str_len(b), " ST", 4);
    if (w > 120) {
        str_cpy(b + str_len(b), " \xB7 ", 4);
        str_cpy(b + str_len(b), N_PFXRES[clamp(song.g[G_PFXRES], 0, 2)], 6);
        if (pfx_rec_ok()) str_cpy(b + str_len(b), " \xB7 REC", 7);
    }
    fxg_label(x, y, b, T_MID, 0);
}
static void fxg_slpitch(int32_t x, int32_t y, int32_t w, int32_t h)
{
    int32_t i, cw = w / 16, mid = y + 10 + (h - 10) / 2, len = clamp(song.g[G_SLPLEN], 1, 16);
    cv_rect(x, mid, w, 1, T_LINE);
    for (i = 0; i < 16; i++) {
        int32_t v = song.g[G_SLP0 + i], bh = (v < 0 ? -v : v) * ((h - 10) / 2 - 4) / 24 + 2;
        uint16_t c = i == (int32_t)sl_ui_step ? T_THEME : i < len ? FXG_H(128 + v * 5) : T_RAISE;
        cv_rect(x + i * cw + 1, v >= 0 ? mid - bh : mid + 1, cw - 2, bh, c);
    }
    fxg_label(x, y, song.g[G_SLPQ] ? "SL PITCH \xB7 SCL" : "SL PITCH \xB7 CHR", T_MID, 0);
}
static void fxg_sends(const track_t *t, int32_t x, int32_t y, int32_t w, int32_t h)
{
    static const char *const N[4] = {"DIST", "CHOR", "DLY", "REV"};
    int32_t i, bw = w / 4 - 6;
    for (i = 0; i < 4; i++) {
        int32_t v = t->p[P_DIST + i], bx = x + i * (w / 4), fh = (h - 20) * v / 127;
        cv_rect(bx, y + 10, bw, h - 20, T_RAISE);
        if (fh > 0)
            cv_rect(bx, y + h - 10 - fh, bw, fh, FXG_H(51 + v * 205 / 127));
        GFX_HOOK_ALIGN(bx, 0, bx + bw, 0, AL_H, "FX send name centred under its bar");
        cv_text_in(bx, y + h - 6, bw, &AF_X, N[i], T_DIM, T_SURF);
    }
}
static void fxg_dist(const track_t *t, int32_t x, int32_t y, int32_t w, int32_t h)
{
    int32_t d = t->p[P_DIST], g = 4096 + d * d * 2, i, py = 0, s = h, cy = y + s / 2;
    uint32_t type = (uint32_t)t->p[P_DTYPE] > DT_AMP ? DT_SOFT : (uint32_t)t->p[P_DTYPE];
    char b[16];
    (void)w;
    cv_frame(x, y, s, s, T_LINE);
    for (i = 0; i < s; i++) {
        int32_t xx = (i - s / 2) * 131072 / (s / 2), yy = cy - tk_dist_y(type, xx, d ? g : 4096, d) * (s / 2 - 1) * 9 / 10 / 32767;
        yy = clamp(yy, y + 1, y + s - 2);
        if (i)
            cv_line_t(x + i - 1, py, x + i, yy, FXG_H(102 + d), 2);
        py = yy;
    }
    if (fxg_en() && s > 20) {                           /* the level now, a dot on the curve */
        int32_t xx = clamp(bg_energy(), 0, 256) * 131072 / 256, px = x + s / 2 + xx * (s / 2) / 131072;
        int32_t yy = clamp(cy - tk_dist_y(type, xx, d ? g : 4096, d) * (s / 2 - 1) * 9 / 10 / 32767, y + 3, y + s - 4);
        cv_rect(clamp(px, x + 3, x + s - 4) - 1, yy - 1, 3, 3, T_TEXT);
    }
    fxg_label(x + s + 8, y + 1, N_DTYPE[type], T_MID, 0);
    str_cpy(b, "TONE ", sizeof b);
    fmt_int(b + 5, t->p[P_DTONE]);
    fxg_label(x + s + 8, y + 15, b, T_DIM, 0);
}
static void fxg_pan(const track_t *t, int32_t x, int32_t y, int32_t w, int32_t h)
{
    int32_t dep = t->p[P_APDEP], mid = y + 10 + (h - 10) / 2, cx = x + w / 2, sw = dep * (w / 2 - 12) / 127;
    char b[24];
    const char *u;
    cv_rect(cx, y + 10, 1, h - 10, T_LINE);
    fxg_label(x, mid - 6 - AF_X_CAP_Y, "L", T_DIM, 0);
    fxg_label(x + w, mid - 6 - AF_X_CAP_Y, "R", T_DIM, 1);
    if (sw > 0)
        cv_rect(cx - sw, mid, 2 * sw + 1, 1, T_MID);
    {   /* the dot: where AUTO PAN has the part now (the owner: it seemed not to work; fx.c autopan's own phase and
         * wave, so in time with the tempo and still when the part is), the middle with DEPTH 0 (off) */
        uint32_t ti = (uint32_t)(t - trk) % NTRK, ph = ap_ph[ti], wv = (uint32_t)clamp(t->p[P_APWAVE], 0, 4);
        int32_t v = wv == 1u ? (ph < 0x80000000u ? (int32_t)(ph >> 15) - 32768 : 98303 - (int32_t)(ph >> 15)) :
                    wv == 2u ? (int32_t)(ph >> 16) - 32768 : wv == 3u ? (ph < 0x80000000u ? 32767 : -32767) :
                    wv == 4u ? ap_sh[ti] : osc_sine(ph);
        cv_rrect(cx + (v * sw >> 15) - 4, mid - 4, 9, 9, 4, FXG_H(205), T_SURF);
        if (!dep)
            fxg_label(cx + 8, mid + 4, "DEPTH 0: OFF", T_DIM, 0);
    }
    {   /* WAVE . RATE as AUTO PAN's cards say them (its RATE by SYNC: page_desc) */
        int16_t *vp;
        const param_desc_t *dw = page_desc(cur_page(), 3, &vp), *dr;
        char r[10];
        param_format(dw ? dw : track_desc(t, P_APWAVE), t->p[P_APWAVE], b, &u);
        str_cpy(b + str_len(b), " \xB7 ", 4);
        dr = page_desc(cur_page(), 1, &vp);
        param_format(dr ? dr : track_desc(t, P_APRATE), dr && vp ? *vp : t->p[P_APRATE], r, &u);
        str_cpy(b + str_len(b), r, 10);
    }
    fxg_label(x + 12, y, b, T_MID, 0);
}
/* module m's picture */
static void fxg_mod(uint32_t m, int32_t x, int32_t y, int32_t w, int32_t h)
{
    switch (m) {
    case 0: fxg_delay(x, y, w, h); break;
    case 1: fxg_reverb(x, y, w, h); break;
    case 2: fxg_chorus(x, y, w, h); break;
    case 3: if (cur_page()->graph == GR_SLPIT && w > 120) fxg_slpitch(x, y, w, h); else fxg_slicer(x, y, w, h); break;
    case 4: fxg_master(x, y, w, h); break;
    default: fxg_punch(x, y, w, h); break;
    }
}
/* the set's place: n dots (5 x 3, 7 apart) at the top left, k lit; its name at the right */
static void fx_dots(uint32_t n, uint32_t k, const char *a, const char *b)
{
    char s[32];
    uint32_t i;
    for (i = 0; i < n; i++)
        cv_rect(6 + (int32_t)i * 7, 6, 5, 3, i == k ? T_THEME : T_DIM);
    str_cpy(s, a, sizeof s);
    str_cpy(s + str_len(s), " \xB7 ", 4);
    str_cpy(s + str_len(s), b, 16);
    GFX_HOOK_ALIGN(0, 3, 0, 12, AL_V | AL_PASS, "FX lens set name");
    cv_text_r(234, 3, &AF_X, s, T_DIM, T_SURF);
}
static void graph_fx_lz(const track_t *t, const page_t *pg)
{
    uint32_t lv = fx_level();
    if (lv == FXL_TRACK) {
        int32_t k = fx_trk_k(ui.page);
        char tn[3] = {'T', (char)('1' + song.sel % NTRK), 0};
        fx_dots(3, (uint32_t)k, k == 0 ? "FX" : PAGES[ui.page].title, tn);
        if (k == 0) fxg_sends(t, 8, 12, 224, 70);
        else if (k == 1) fxg_dist(t, 8, 12, 224, 70);
        else fxg_pan(t, 8, 12, 224, 70);
        fxg_dist(t, 8, 92, 224, 40);
    } else if (lv == FXL_BUSES) {
        uint32_t m;
        for (m = 0; m < FX_NMOD; m++) {
            int32_t x = 4 + (int32_t)(m % 3u) * 78, y = (int32_t)(m / 3u) * 72;
            int sel = m == ui.fxmod, ok = fx_mod_ok(m);
            uint16_t bg = lz_tile_bg();                 /* (0.8.9, the skins: COLORS a rounded card, the one picked framed
                                                         * THEME; BOTTOM, HUD bare, its name underlined) */
            lz_tile(x, y, 76, 68, sel, T_THEME);
            cv_bg = bg;
            GFX_HOOK_ALIGN(0, y + 3, 0, y + 12, AL_V | AL_PASS, "FX module name");
            cv_text_on(x + 4, y + 3, &AF_X, FX_MOD[m], sel ? T_THEME : UX_SKIN ? T_MID : T_TEXT, bg);
            if (sel)
                lz_uline(x + 4, y + 12, &AF_X, FX_MOD[m], T_THEME);
            if (ok)
                fxg_mod(m, x + 4, y + 14, 68, 50);
            else
                cv_text_on(x + 4, y + 33, &AF_X, "ECO", T_DIM, bg);
            cv_bg = T_SURF;
        }
    } else {
        uint32_t m = fx_mod_of(ui.page), k = fx_mod_k(ui.page);
        fx_dots(fx_mod_n(m), k, FX_MOD[m], pg->title);
        fxg_mod(m, 8, 12, 224, 120);
    }
}
