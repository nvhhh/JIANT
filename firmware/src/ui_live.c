/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments
 * Copyright (C) 2026 Juan Ignacio Antonucci (Juan Jiant) */
/* GLO > LIVE (0.9.11): the six sequences side by side and their live modifiers (seq_live.c), two pages, the approved
 * mock-up secuencias.html (V3 MANO, V1 RIELES), first in GLO in a group of their own (ui_lens.c CE_GLO).
 *   MANO   the keys are a board: the black keys 1..6 (F#3 .. F#4) pick the tracks while held (several at once; none, or
 *          A#4: ALL), the white keys are the modifiers while held: F3 /2, G3 x2, A3 TR (3/2), B3 DT (2/3), C4 .. F4 LOOP
 *          8 4 2 1 (from where the playhead is; the shortest held wins), G4 REV, A4 PING, B4 C5 RATCH x2 x3, D5 THIN 50 %,
 *          E5 F5 OCT+ OCT-, G5 RETURN. G#4 LATCH: a white key then turns its modifier on / off (for the tracks held then).
 *          C#5 D#5 GIRO -1 / +1 (a step back / on), F#5 RND (the steps in any order) on / off. The keys play no note here
 *          (seq.c keyboard_block, song.grid 3); off the page they play again (a key still held keeps its modifier until
 *          it is let go). KNOB 1 RATE, 2 LOOP, 3 THIN on the tracks held (none: all), 4 the DJ FILTER (MIXER's KNOB 4).
 *          The panel: the six rails small, the board (held: ACCENT, latched: THEME).
 *   RIELES six rails, each its loop across the screen, its playhead, its ghost, its window, under it LEN . DIV and what
 *          is done to it. KNOB 1 PISTAS: ALL, T1 .. T6, SYN (the synths), DRM (the DRUM tracks); 2 RATE /4 .. x4; 3 CORTE
 *          (the first N steps); 4 DENS (left THIN 90 .. 10 %, the middle all, right RATCH x2 .. x4).
 * How they combine: one state per track. The knobs of both pages are its LATCHED layer (RATE is the same value on both
 * pages; MANO's LOOP and RIELES' CORTE are the one window, LOOP from the playhead, CORTE from step 1, the last turned
 * wins; THIN and DENS are the one density); MANO's keys sit on top while held or latched: a rate key replaces the
 * rate, a LOOP key shortens the window (the shortest wins), THIN takes the thinner, RATCH the larger, REV / PING / OCT
 * are the keys' alone. On both pages OCT- tapped is RETURN (the tracks picked: everything back, in phase, on the next
 * beat), OCT- held BYPASS (the original while held, every track), OCT+ COMMIT (asks; OCT+ again writes RATE into DIV,
 * else STRETCH; CORTE / LOOP into LEN, a LOOP's steps to the start; GIRO into ROTATE; THIN into CHANCE; REV, PING, RND,
 * RATCH, OCT and a rate with no DIV or STRETCH stay live; UNDO: GLO + G4). Never saved; STOP keeps them. Included by
 * ui_layer.c. */
static uint32_t tl_len(const track_t *t);
static void tl_take(const track_t *t);
static uint32_t tl_stretch(track_t *t, int32_t k);
static uint16_t pk_fill(uint32_t st, int blk, int n, uint16_t *ink);
static void tracks_edit(uint32_t slot, int32_t steps);
#define LV_ALL ((1u << NTRK) - 1u)
static const char LV_WK[16][3] = {"/2", "x2", "TR", "DT", "L8", "L4", "L2", "L1", "RV", "PP", "R2", "R3", "TH", "O+", "O-",
                                  "RT"};
static const char *const LV_RNAME[LV_NRATE] = {"/4", "/3", "/2", "2/3", "x1", "3/2", "x2", "x3", "x4"};
static const char LV_TG[NTRK + 3][4] = {"ALL", "T1", "T2", "T3", "T4", "T5", "T6", "T7", "T8", "SYN", "DRM"};   /* RIELES' PISTAS (0.9.19: eight tracks) */
static const char LV_BK[11] = {'1', '2', '3', '4', '5', '6', 'L', 'A', '<', '>', '?'};
static const int8_t LV_KRATE[4] = {-2, 2, 1, -1};       /* F3 .. B3: /2 x2 3/2 2/3 */

static int live_page(void) { return !ui.home && !ui.menu && cur_page()->graph == GR_LIVE; }
static int lv_isman(void) { return cur_page()->title[0] == 'M'; }
static int live_mano(void) { return live_page() && !ui.confirm && lv_isman(); }
/* the tracks the knobs, RETURN and COMMIT act on: MANO the black keys held (none, or A#4: all); RIELES its KNOB 1 */
static uint32_t lv_tracks(void)
{
    uint32_t m = 0, k, i;
    if (lv_isman()) {
        for (k = 0; k < 27u; k++)
            if (((lv.kb >> k) & 1u) && key_black(k) && (i = key_place(k)) <= 7u)
                m |= i == 7u ? LV_ALL : i < NTRK ? 1u << i : 0u;
        return m ? m : LV_ALL;
    }
    if (lv.tgt - 1u < NTRK)
        return 1u << (lv.tgt - 1u);
    for (i = 0; i < NTRK && lv.tgt; i++)
        m |= (uint32_t)(drum_track(&trk[i]) == (lv.tgt == NTRK + 2u)) << i;   /* (DRM last) */
    return lv.tgt ? m : LV_ALL;
}
static void lv_req(uint32_t m, uint32_t kind)           /* a request to the ISR for tracks m (seq_live.c live_q) */
{
    uint32_t i;
    for (i = 0; i < NTRK; i++)
        if ((m >> i) & 1u) {
            lv.rk[i] = (uint8_t)kind;
            lv.rq[i]++;
        }
}
static void lv_keys_off(uint32_t m, uint32_t from)      /* MANO's white keys from `from` on: not on tracks m (PING kept) */
{
    for (; from < 15u; from++)
        if (from != 9u) {
            lv.tk[from] &= (uint8_t)~m;
            lv.lt[from] &= (uint8_t)~m;
        }
}
/* what each track should have now (the latched layer, the keys over it) for the ISR */
static void live_publish(void)
{
    uint32_t i, k, busy = 0, rk = 0;
    for (i = 0; i < NTRK; i++) {
        lv_mod_t m = {0};
        uint32_t bit = 1u << i, d;
        int32_t dn = lv.dn[i];
        if (!lv.byp) {
            m.rate = lv.rate[i];
            m.cut = LV_CUT[lv.cut[i] % LV_NCUT];
            m.fl = (uint8_t)(((lv.here >> i) & 1u) * LF_HERE | ((lv.rnd >> i) & 1u) * (LD_RND << 1));
            m.drop = (uint8_t)(dn < 0 ? -10 * dn : 0);
            m.rat = (uint8_t)(dn > 0 ? 1 + dn : 0);
            m.rot = lv.rot[i];
            for (k = 0; k < 15u; k++) {                 /* the keys over it (a rate: the last on the map) */
                if (!(((lv.tk[k] | lv.lt[k]) >> i) & 1u))
                    continue;
                if (k < 4u) {
                    m.rate = LV_KRATE[k];
                    rk |= bit;
                } else if (k < 8u) {                    /* LOOP 8 4 2 1: the shortest */
                    d = 8u >> (k - 4u);
                    m.cut = (uint8_t)(m.cut && (m.fl & LF_HERE) && m.cut < d ? m.cut : d);
                    m.fl |= LF_HERE;
                } else if (k < 10u) {
                    m.fl = (uint8_t)((m.fl & ~6u) | (k - 7u) << 1);   /* REV, PING */
                } else if (k < 12u) {
                    if (m.rat < k - 8u)
                        m.rat = (uint8_t)(k - 8u);      /* RATCH x2, x3 */
                } else if (k == 12u) {
                    if (m.drop < 50u)
                        m.drop = 50;
                } else {
                    m.fl |= k == 13u ? LF_UP : LF_DN;
                }
            }
            if (perf_m[0]) {                            /* (0.9.14) the FX layer's STEPS: 1/2 1/4 1/8 of it, 1 step */
                k = (perf_m[0] + 2u) / 3u;
                d = k < 4u ? (uint32_t)clamp(trk[i].p[P_SLEN], 1, NSTEP) >> k : 1u;
                m.cut = (uint8_t)(d ? d : 1u);          /* (over LIVE's own window) */
                m.fl |= LF_HERE;
            }
            if (m.cut && (m.fl & LF_HERE)) {            /* a LOOP: the N steps the playhead is in when it is taken */
                d = (uint32_t)clamp(trk[i].p[P_SLEN], 1, NSTEP);
                m.ws = lv.want[i].ws;
                if (m.cut != lv.want[i].cut || !(lv.want[i].fl & LF_HERE)) {
                    k = trk[i].seq_idx % d;
                    k -= k % m.cut;
                    m.ws = (uint8_t)(m.cut >= d ? 0u : k + m.cut > d ? d - m.cut : k);
                }
            }
            if (m.rat > 1u) {                           /* RATCH: no part shorter than LV_MINPART (~1.4 ms; the step's
                                                         * shortest swing: 0.6) */
                d = div_samples((uint32_t)trk[i].p[P_SDIV]) * LV_RD[m.rate + LV_X1] / LV_RN[m.rate + LV_X1] * 3u /
                    (5u * LV_MINPART);
                if (m.rat > d)
                    m.rat = (uint8_t)(d > 1u ? d : 0u);
            }
        }
        lv.want[i] = m;
        busy |= !lv_neutral(&m) || !lv_neutral(&lv.cur[i]) || lv.rq[i] != lv.rs[i];
    }
    lv.r16 = (uint8_t)(rk | lv.rkey);                    /* (a rate key pressed or let go: on the 1/16) */
    lv.rkey = (uint8_t)rk;
    lv.busy = (uint8_t)busy;
}
/* RETURN: tracks m back to nothing (both pages' knobs, the keys on them), on the next beat */
static void live_return(uint32_t m)
{
    uint32_t i;
    for (i = 0; i < NTRK; i++)
        if ((m >> i) & 1u)
            lv.rate[i] = lv.dn[i] = lv.rot[i] = (int8_t)(lv.cut[i] = 0);
    lv.here &= (uint8_t)~m;
    lv.rnd &= (uint8_t)~m;
    lv_keys_off(m, 0);
    lv.tk[9] &= (uint8_t)~m;
    lv.lt[9] &= (uint8_t)~m;
    live_publish();
    lv_req(m, LQ_BEAT);
}
/* COMMIT (the dialog's OCT+, UNDO marked): what tracks m play now into their own parameters, at once; then, the
 * interrupts held off, the playhead where the written pattern plays what sounded (a LOOP's steps moved to the start,
 * ROTATE moved, a step's samples at the new DIV), its ghost there too */
static __attribute__((noinline)) void live_commit(uint32_t m)
{
    static const uint16_t DL[10] = {48, 24, 12, 6, 16, 8, 96, 192, 384, 768};   /* N_DIV's steps, 48ths of a beat */
    uint32_t i, k, f;
    for (i = 0; i < NTRK; i++) {
        track_t *t = &trk[i];
        lv_mod_t w = lv.want[i];
        uint32_t len = tl_len(t), idx = t->seq_idx, pos = t->seq_pos, ws = 0, rn = LV_RN[w.rate + LV_X1];
        uint32_t rd = LV_RD[w.rate + LV_X1];
        if (!((m >> i) & 1u))
            continue;
        if (w.rot) {
            t->p[P_SOFS] = (int16_t)clamp((t->p[P_SOFS] - w.rot) % (int32_t)len, -32, 32);
            idx = (uint32_t)((int32_t)(idx % len) - w.rot % (int32_t)len + 2 * (int32_t)len);   /* (mod len below) */
        }
        if (w.cut && w.cut < len) {
            ws = (w.fl & LF_HERE) ? w.ws : 0u;
            if (ws + w.cut > len)
                ws = len - w.cut;
            for (k = 0; k < w.cut; k++)                 /* (the LOOP's steps to the start) */
                t->step[k] = t->step[k + ws];
            t->p[P_SLEN] = (int16_t)w.cut;
        }
        if (w.rate) {                                   /* DIV, else STRETCH, else it stays live */
            uint32_t to = DL[t->p[P_SDIV] % 10] * rd;
            for (k = 0; k < 10u && (to % rn || DL[k] != to / rn); k++) {}
            if (k < 10u)
                t->p[P_SDIV] = (int16_t)k;
            else if (rn == 1u || rd == 1u) {
                tl_take(t);
                tl_stretch(t, rn == 1u ? (int32_t)rd : -(int32_t)rn);
            }
            if (k < 10u || rn == 1u || rd == 1u) {
                pos = pos * rd / rn;
                lv.rate[i] = 0;
                lv_keys_off(1u << i, 0);
            }
        }
        if (w.drop)
            t->p[P_TCHAN] = (int16_t)(127 - (127 - clamp(t->p[P_TCHAN], 0, 127)) * (100 - w.drop) / 100);
        lv.cut[i] = lv.dn[i] = lv.rot[i] = 0;
        lv.here &= (uint8_t)~(1u << i);
        lv_keys_off(1u << i, 4);
        live_publish();
        f = motion_guard();
        if (LV_ON(i) && song.playing && t->seq_pos < 0x7FFFFFFFu) {
            len = tl_len(t);
            lv.gidx[i] = t->seq_idx = (uint16_t)((idx - (idx >= ws ? ws : 0u)) % len);
            lv.gpos[i] = t->seq_pos = pos;
            t->jump_to = 0;
        }
        lv.cur[i] = lv.want[i];
        lv.rj[i] = 0;
        if (lv_neutral(&lv.cur[i]))
            lv.act &= (uint8_t)~(1u << i);
        motion_unguard(f);
    }
    ui_message("WRITTEN: GLO+G4 UNDO");
}
/* each frame: MANO's keys (seq.c keyboard_block holds them in lv.kb), OCT- / OCT+ on a LIVE page (OCT- held BYPASS,
 * tapped RETURN; OCT+ the COMMIT dialog), then what goes to the ISR */
static __attribute__((noinline)) void live_frame(void)
{
    uint32_t kb = lv.kb, ch = kb ^ lv.kprev, tg = lv_tracks(), k, i, any = 0;
    uint32_t ob = ((fm1_in.buttons >> panel.btn[B_OCTDN]) & 1u) | ((fm1_in.buttons >> panel.btn[B_OCTUP]) & 1u) << 1;
    lv.kprev = kb;
    for (k = 0; k < 27u; k++) {
        uint32_t p = key_place(k), dn = (kb >> k) & 1u;
        if (!((ch >> k) & 1u))
            continue;
        if (key_black(k)) {
            if (!dn)
                continue;
            if (p == 6u)
                lv.latch ^= 1u;
            else if (p == 8u || p == 9u) {
                for (i = 0; i < NTRK; i++)
                    if ((tg >> i) & 1u)
                        lv.rot[i] = (int8_t)clamp(lv.rot[i] + (p == 8u ? -1 : 1), -16, 16);
            } else if (p == 10u)
                lv.rnd ^= (uint8_t)tg;
        } else if (p == 15u) {
            if (dn)
                live_return(tg);
        } else if (!dn) {
            lv.tk[p] = 0;
        } else if (lv.latch) {
            lv.lt[p] = (uint8_t)(lv.lt[p] ? 0u : tg);
        } else {
            lv.tk[p] = (uint8_t)tg;
        }
    }
    if (live_page() && !ui.confirm && ob != 3u) {
        if ((ob & ~lv.ob) & 1u)
            lv.oms = fm1_ms | 1u;
        if ((ob & ~lv.ob) & 2u) {
            for (i = 0; i < NTRK; i++)
                any |= ((tg >> i) & 1u) && !lv_neutral(&lv.want[i]);
            if (!any || chain_busy())
                ui_message(any ? "STOP TO EDIT" : "NOTHING TO COMMIT");
            else
                confirm_open(CF_LIVE_COMMIT, tg);
        }
    }
    lv.ob = (uint8_t)ob;
    if (lv.oms && (ob & 1u) && !lv.byp && (int32_t)(fm1_ms - lv.oms) > 350) {
        lv.byp = 1;
        lv_req(LV_ALL, LQ_NOW);
        ui_message("BYPASS: THE ORIGINAL");
    } else if (lv.oms && !(ob & 1u)) {
        if (lv.byp)
            lv_req(LV_ALL, LQ_NOW);
        else if (live_page())
            live_return(tg);
        lv.byp = 0;
        lv.oms = 0;
    }
    live_publish();
}
/* KNOB slot turned on a LIVE page (edit_param) */
static __attribute__((noinline)) void live_knob(uint32_t slot, int32_t steps)
{
    int mano = lv_isman();
    uint32_t tg = lv_tracks(), f = (uint32_t)__builtin_ctz(tg), i;
    int32_t d = clamp(steps, -12, 12), v;
    if (mano && slot == 3u) {                           /* the DJ FILTER, as MIXER's KNOB 4 */
        tracks_edit(3, steps);
        return;
    }
    if (!mano && !slot--) {                             /* PISTAS: ALL, T1 .. T6, SYN, DRM */
        lv.tgt = (uint8_t)clamp((int32_t)lv.tgt + d, 0, NTRK + 2);
        return;
    }
    v = slot == 0u ? clamp(lv.rate[f] + d, -4, 4) : slot == 1u ? clamp((int32_t)lv.cut[f] + d, 0, LV_NCUT - 1) :
        mano ? clamp((lv.dn[f] < 0 ? lv.dn[f] : 0) - d, -9, 0) : clamp(lv.dn[f] + d, -9, 3);   /* (MANO THIN, RIELES DENS) */
    for (i = 0; i < NTRK; i++) {                        /* the first track picked's value, to every one picked */
        if (!((tg >> i) & 1u))
            continue;
        if (slot == 0u) {
            lv.rate[i] = (int8_t)v;
        } else if (slot == 1u) {
            lv.cut[i] = (uint8_t)v;
            lv.here = (uint8_t)(mano ? lv.here | 1u << i : lv.here & ~(1u << i));
        } else {
            lv.dn[i] = (int8_t)v;
        }
    }
}
static void lv_card(uint32_t c, const char *l, const char *v, const char *u, int on, int32_t r)
{
    draw_column(c, l, v, u, on ? VAL(c) : T_DIM, r, ICON_NONE);
}
/* the cards: MANO RATE LOOP THIN DJ FILT, RIELES PISTAS RATE CORTE DENS (the values of the first track picked) */
static __attribute__((noinline)) void live_cards(void)
{
    int mano = lv_isman();
    uint32_t tg = lv_tracks(), f = (uint32_t)__builtin_ctz(tg), c = 0, ci = lv.cut[f] % LV_NCUT;
    int32_t r = lv.rate[f], dn = lv.dn[f];
    char val[8];
    const char *u = "";
    if (!mano)
        lv_card(c++, "PISTAS", LV_TG[lv.tgt % (NTRK + 3u)], "", 1, (int32_t)lv.tgt * 1000 / (NTRK + 2));
    lv_card(c, "RATE", LV_RNAME[r + LV_X1], "", r, ratio_of(-4, 4, r));
    fmt_int(val, LV_CUT[ci]);
    c++;
    lv_card(c, mano ? "LOOP" : "CORTE", ci ? val : "OFF", ci ? "ST" : "", ci, (int32_t)ci * 100);
    if (mano) {
        fmt_int(val, dn < 0 ? 100 + 10 * dn : 100);
        lv_card(2, "THIN", val, "%", dn < 0, dn < 0 ? -dn * 111 : 0);
        djf_format(song.g[G_DJF], val, &u);
        card_mot_next = (uint8_t)motion_djf_any();
        card_curve_next = (int16_t)(song.g[G_DJF] + 256);
        draw_column(3, "DJ FILT", val, u, VAL(3u), RATIO(&GP[G_DJF], song.g[G_DJF]), ICON_AUTO);
        return;
    }
    val[0] = 'x';
    fmt_int(val + (dn > 0), dn > 0 ? 1 + dn : 100 + 10 * dn);
    lv_card(3, "DENS", val, dn > 0 ? "RAT" : "%", dn, ratio_of(-9, 3, dn));
}
#define LV_RX 16                                        /* the rails: x 16 .. 236 */
#define LV_RW 220
/* a playhead's x on a rail: step idx of len, pos samples into it */
static int32_t lv_x(const track_t *t, uint32_t idx, uint32_t pos, uint32_t len)
{
    uint32_t sl = step_samples(t, div_samples((uint32_t)t->p[P_SDIV]), idx);
    int32_t x = (int32_t)((idx * 256u + (sl && pos < sl ? pos * 256u / sl : 0u)) * LV_RW / (len * 256u));
    return LV_RX + (x < LV_RW ? x : LV_RW - 1);
}
/* track i's loop as a rail at y, h px high: its steps (a synth's notes as dashes over the pattern's range, a kit's hits
 * as bars), the window (the rest dim), the step sounding white, the playhead and, modified, the ghost (dotted) */
static void lv_rail(uint32_t i, int32_t y, int32_t h)
{
    const track_t *t = &trk[i];
    const lv_mod_t *m = song.playing && LV_ON(i) ? &lv.cur[i] : &lv.want[i];
    uint32_t len = (uint32_t)clamp(t->p[P_SLEN], 1, NSTEP), n = m->cut && m->cut < len ? m->cut : 0u, k, ix = 99;
    uint32_t ws = n && (m->fl & LF_HERE) ? m->ws : 0u;
    int32_t lo, hi, a, e, cw = (LV_RW << 8) / (int32_t)len, sy;
    int drum = drum_track(t);
    pat_range(t, &lo, &hi);
    sy = ((h - 2) << 8) / (hi - lo);
    if (ws + n > len)
        ws = len - n;
    if (song.playing && t->seq_pos < 0x7FFFFFFFu)
        ix = t->seq_idx % len;
    cv_rect(LV_RX, y + h, LV_RW, 1, T_RAISE);
    for (k = 0; k < 2u && n; k++)                       /* the window's ends */
        cv_rect(LV_RX + ((int32_t)(ws + k * n) * cw >> 8) - (int32_t)k, y - 1, 1, h + 2, T_THEME);
    for (k = 0; k < len; k++) {
        const step_t *st = &t->step[seq_src(t, k)];
        int32_t x0 = LV_RX + ((int32_t)k * cw >> 8), x1 = LV_RX + ((int32_t)(k + 1u) * cw >> 8);
        uint16_t c;
        if (!step_on(st))
            continue;
        c = k - ws >= n && n ? T_DIM : k == ix ? T_TEXT : lz_note(st, (st->flags & SF_ACCENT) || st->acc, T_THEME);
        x1 -= x1 - x0 > 3;
        e = drum ? 0 : h - 2 - ((st->note[0] - lo) * sy >> 8);
        cv_rect(x0, y + e, x1 - x0, drum ? h : 2, c);
    }
    if (ix > len)
        return;
    cv_rect(lv_x(t, ix, t->seq_pos, len), y - 1, 1, h + 2, T_ACCENT);
    if (!LV_ON(i) || lv.gpos[i] >= 0x7FFFFFFFu || (lv.gidx[i] % len == ix && lv_phase0(m)))
        return;
    a = lv_x(t, lv.gidx[i] % len, lv.gpos[i], len);
    for (e = -1; e <= h; e += 2)
        cv_rect(a, y + e, 1, 1, T_THEME);
}
/* MANO's board, F3 .. G5: the black keys in a row over the white ones, each its word, lit held (ACCENT) or latched /
 * on (THEME) for the tracks picked */
static void lv_board(int32_t y)
{
    uint32_t tg = lv_tracks(), k;
    char nm[3] = {0, 0, 0};
    for (k = 0; k < 27u; k++) {
        int blk = key_black(k);
        uint32_t p = key_place(k), st = (lv.kb >> k) & 1u ? LS_HELD : LS_OFF;
        int32_t x0 = blk ? 4 + PK_BLK[p] * 232 / 16 - 5 : 4 + (int32_t)p * 232 / 16;
        int32_t w = blk ? 10 : 4 + ((int32_t)p + 1) * 232 / 16 - x0 - 1, yy = blk ? y : y + 15, hh = blk ? 13 : 26;
        uint16_t ink, fill;
        if (blk) {
            nm[0] = LV_BK[p];
            nm[1] = 0;
            if (st == LS_OFF && ((p == 6u && lv.latch) || (p == 10u && (lv.rnd & tg))))
                st = LS_SEL;
        } else {
            str_cpy(nm, LV_WK[p], 3);
            if (st == LS_OFF && p < 15u && (lv.lt[p] & tg))
                st = LS_SEL;
        }
        fill = pk_fill(st, blk, 1, &ink);
        cv_rect(x0, yy, w, hh, fill);
        cv_text_in(x0, yy + hh - 9 - AF_X_CAP_Y, w, &AF_X, nm, ink, fill);
    }
}
/* the panel: RIELES six rails across it; MANO six rails small over the board */
static __attribute__((noinline)) void graph_live(void)
{
    int mano = lv_isman();
    int32_t h = (int32_t)graph_h(), by = h - 44, rh = (mano ? by - 3 : h) / NTRK, lh = rh - 4;
    uint32_t tg = lv_tracks(), i;
    for (i = 0; i < NTRK; i++) {
        int32_t y = (int32_t)i * rh + 2, by0 = y + lh - 8;
        char d[2] = {(char)('1' + i), 0};
        uint16_t bg = i == song.sel ? T_ACCENT : cv_bg;
        lv_rail(i, y, lh);
        if (i == song.sel)                              /* ALGORITHM's track: on its badge; the tracks picked THEME */
            cv_rect(3, by0, 9, 9, T_ACCENT);
        cv_text_in(3, by0 + CAP_IN(X, 9), 9, &AF_X, d, i == song.sel ? T_INK : (tg >> i) & 1u ? T_THEME : T_DIM, bg);
        if ((tg >> i) & 1u)
            cv_rect(13, by0, 1, 9, T_THEME);
    }
    if (mano)
        lv_board(by);
}
/* the panel's signature: playing, every frame (the playheads move); stopped, all of LIVE's state (a page entered, an
 * UNDO: ui.force) */
static __attribute__((noinline)) uint32_t live_sig(uint32_t h)
{
    uint32_t i;
    for (i = 0; i < sizeof lv; i++)
        h = (h ^ ((const uint8_t *)&lv)[i]) * 16777619u;
    return h ^ (song.playing ? ui.frame * 2654435761u ^ song.sel : song.sel);
}
/* MANO's key LEDs: the keys held, the white keys latched, LATCH on */
static __attribute__((noinline)) uint32_t live_leds(void)
{
    uint32_t m = lv.kb, k;
    for (k = 0; k < 27u; k++)
        if (key_black(k) ? key_place(k) == 6u && lv.latch : key_place(k) < 15u && lv.lt[key_place(k)])
            m |= 1u << k;
    return m;
}
