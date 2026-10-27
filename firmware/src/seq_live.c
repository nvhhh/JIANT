/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments
 * Copyright (C) 2026 Juan Ignacio Antonucci (Juan Jiant) */
/* LIVE (0.9.11): the sequencer's live modifiers, GLO > LIVE (the approved mock-up secuencias.html: MANO, its V3, and
 * RIELES, its V1; the pages are ui_live.c). Per track, never destructive: they change how the sequencer walks the steps,
 * not the steps, and are never saved (performance state, as the punch-in effects); COMMIT writes them into the track's
 * own parameters (UNDO: GLO + G4).
 *   RATE   /4 /3 /2 2/3 x1 3/2 x2 x3 x4: the samples the playhead moves by a block scaled
 * (0.9.18) The FX layer's 1/2 TEMPO and RANDOM (pfx.c: PFX_HALF, PFX_RND on the track while held or played from the
 * PUNCH lane) are these: RATE /2 (or slower if LIVE has it slower) and RND over the track's modifiers, in and out at
 * once (they come on the transport's 1/16 already); let go, back on the ghost as any modifier taken back
 *   CORTE  the first N steps only; LOOP (MANO's keys and K2): N steps from where the playhead is (a window of LEN)
 *   DIR    REV, PING (MANO's keys), RND (MANO's F#5) inside the window
 *   GIRO   the playhead a step on / back (MANO's C#5 D#5): a rotation, COMMIT puts it into ROTATE (P_SOFS)
 *   THIN   a share of the steps silent: a roll of its own (deterministic: the track, the step, the loop), as CHANCE
 *   RATCH  every step played x2 .. x4 (seq_ratchet's parts), never shorter than LV_MINPART samples (~1.4 ms)
 *   OCT    the sequencer's notes an octave up / down (not a kit's hits)
 * The ghost: while a track is modified a second playhead (lv.gidx / gpos) walks its steps as if nothing were, silently
 * (it ties a recorded note held into the steps it enters: seq_tick's rec_hold). A modifier taken back (RETURN, a knob
 * back to its centre, a key let go) lands the playhead on the ghost: the track is back in phase. Live recording writes
 * at the ghost's place (rec_note: where the step is in the written pattern), so recording over a modified track never
 * scrambles it; the automation and the locks follow the step that sounds.
 * Timing: the main loop (ui_live.c live_publish) writes what each track should have (lv.want); here (live_q, each block
 * before the sequencer) it goes in on the transport's next 1/16, RATE on the next beat (MANO's keys: the 1/16); RETURN
 * on the next beat; OCT- held (BYPASS) at once (a COMMIT: ui_live.c, at once, the interrupts held off). Stopped: at once.
 * Cost: idle one test a block (lv.busy | lv.act) and one a track (seq_tick: LV_ON); modified, the ghost's step and a
 * multiply / divide by 2..4 per track and block (live_adv). Nothing in the audio render. Included by seq.c. */
static void rec_hold(track_t *t, uint32_t idx, uint32_t len);
enum { LV_X1 = 4, LV_NRATE = 9, LV_NCUT = 11, LV_MINPART = 64 };
static const uint8_t LV_RN[LV_NRATE] = {1, 1, 1, 2, 1, 3, 2, 3, 4};   /* rate - LV_X1 .. : its multiplier as num / den */
static const uint8_t LV_RD[LV_NRATE] = {4, 3, 2, 3, 1, 2, 1, 1, 1};
static const uint8_t LV_CUT[LV_NCUT] = {0, 32, 24, 16, 12, 8, 6, 4, 3, 2, 1};   /* CORTE / LOOP: the steps (0 OFF) */
enum { LD_FWD, LD_REV, LD_PING, LD_RND };
enum { LQ_NONE, LQ_BEAT, LQ_NOW };                     /* a request (lv.rk): RETURN, BYPASS */
#define LF_HERE 1u                                      /* the window from where the playhead is (LOOP), not step 1 */
#define LF_DIR(f) (((f) >> 1) & 3u)                     /* LD_* */
#define LF_UP 8u
#define LF_DN 16u
typedef struct {
    int8_t rate;                                        /* -4 .. 4 (LV_RN / LV_RD [rate + LV_X1]), 0 x1 */
    uint8_t cut, fl, drop, rat;                         /* the window's steps (0 all); LF_*; THIN: % silent; RATCH 2..4 */
    int8_t rot;                                         /* GIRO: steps */
    uint8_t ws, pad;                                    /* a LOOP's first step (ui_live.c: where the playhead was) */
} lv_mod_t;                                             /* all 0: nothing */
static struct {
    /* main loop (ui_live.c): the latched layer per track, the knobs of both pages and MANO's black keys */
    int8_t rate[NTRK], dn[NTRK], rot[NTRK];             /* dn: DENS -9 .. 3 (THIN 90 % .. 10 %, RATCH x2 .. x4) */
    uint8_t cut[NTRK];                                  /* LV_CUT index */
    uint8_t here, rnd;                                  /* a bit per track: the window is a LOOP; DIR RND */
    uint8_t tk[16], lt[16];                             /* MANO's white keys: the tracks each acts on, held / latched */
    uint8_t latch, tgt, byp, rkey, ob;                  /* LATCH; RIELES' K1 (0 ALL, 1..8 T1..T8, 9 SYN, 10 DRM); OCT-
                                                         * held; the tracks a rate key acted on last frame; OCT- / + */
    uint32_t kprev, oms;                                /* MANO's keys last frame; OCT- down since (0: not) */
    /* main loop -> ISR */
    lv_mod_t want[NTRK];
    uint8_t r16, busy;                                  /* tracks whose RATE goes in on the 1/16; anything to do */
    uint8_t rq[NTRK], rk[NTRK];                         /* a request: rq moved on, of kind rk (LQ_*) */
    /* ISR */
    uint32_t kb;                                        /* keyboard_block: the keys held on MANO */
    lv_mod_t cur[NTRK];
    uint8_t act, rs[NTRK], pd[NTRK], rem[NTRK];         /* modified (a bit a track); rq seen; PING going down; RATE's
                                                         * remainder */
    uint8_t pfo[NTRK], pfa;                             /* (0.9.18) the FX layer's 1/2 TEMPO / RANDOM on it last block
                                                         * (PFX_HALF | PFX_RND); a bit a track: some */
    int8_t rj[NTRK];                                    /* GIRO moved: steps the next one is on / back */
    uint16_t gidx[NTRK];                                /* the ghost: its step, samples into it */
    uint32_t gpos[NTRK];
} lv __attribute__((section(".pool")));                 /* (zeroed at boot: nothing on) */
#define LV_ON(ti) ((lv.act >> (ti)) & 1u)

static int lv_neutral(const lv_mod_t *m) { return !(m->rate | m->cut | m->fl | m->drop | m->rot) && m->rat < 2u; }
static int lv_phase0(const lv_mod_t *m) { return !(m->rate | m->cut | (m->fl & 6u) | m->rot); }   /* walks as written */

/* (live_adv) the step a modified track goes to after the one playing: the window, its direction (seq_tick takes it as
 * a JUMP component's: no TIMING early, a component's own JUMP first) */
static __attribute__((noinline)) uint32_t live_next(const track_t *t, uint32_t len)
{
    uint32_t ti = trk_index(t), n = lv.cur[ti].cut, lo = 0, d = LF_DIR(lv.cur[ti].fl);
    int32_t r = (int32_t)(t->seq_idx % len) + lv.rj[ti], w;
    lv.rj[ti] = 0;
    while (r < 0)                                       /* (GIRO: the playhead a step on / back) */
        r += (int32_t)len;
    while (r >= (int32_t)len)
        r -= (int32_t)len;
    if (!n || n >= len)
        n = len;
    else if (!(lv.cur[ti].fl & LF_HERE) || (lo = lv.cur[ti].ws) + n > len)
        lo = lo ? len - n : 0u;
    w = (int32_t)n;
    r -= (int32_t)lo;
    if (r < 0 || r >= w)                                /* (outside the window: it enters at its start / end) */
        r = d == LD_REV ? w : -1;
    if (d == LD_REV)
        r = r <= 0 ? w - 1 : r - 1;
    else if (d == LD_PING) {
        if (w < 2)
            r = 0;
        else if ((r += lv.pd[ti] ? -1 : 1) >= w) {
            r = w - 2;
            lv.pd[ti] = 1;
        } else if (r < 0) {
            r = 1;
            lv.pd[ti] = 0;
        }
    } else if (d == LD_RND)
        r = (int32_t)(rng() % n);
    else
        r = r + 1 >= w ? 0 : r + 1;
    return lo + (uint32_t)r;
}

/* (seq_tick, a modified track) the ghost on by n samples; the samples the playhead moves (RATE) */
static __attribute__((noinline)) uint32_t live_adv(track_t *t, uint32_t n, uint32_t period, uint32_t len)
{
    uint32_t ti = trk_index(t), gp = lv.gpos[ti], gi = lv.gidx[ti], sl, x;
    int32_t r = lv.cur[ti].rate;
    if (!len)
        len = 1;
    if (gp >= 0x7FFFFFFFu) {                            /* the transport's start (as seq_tick: step 0 now) */
        gi = 0;
        gp = chain.carry;
    } else {
        gp += n;
        while ((sl = step_samples(t, period, gi)) != 0u && gp >= sl) {
            gp -= sl;
            gi = gi + 1u >= len ? 0u : gi + 1u;
            rec_hold(t, gi, len);                       /* (a recorded note held ties where it is written) */
        }
    }
    lv.gidx[ti] = (uint16_t)(gi < len ? gi : gi % len);
    lv.gpos[ti] = gp;
    if (!t->jump_to && t->seq_pos < 0x7FFFFFFFu)        /* the step after this one: the window's, its direction's */
        t->jump_to = (uint8_t)(live_next(t, len) + 1u);
    if (!r)
        return n;
    x = n * LV_RN[r + LV_X1] + lv.rem[ti];
    sl = LV_RD[r + LV_X1];
    if (sl > 1u) {
        lv.rem[ti] = (uint8_t)(x % sl);
        x /= sl;
    }
    return x;
}

/* (step_rolls) THIN: the step playing kept silent this time; the same steps on the same loop every PLAY */
static int live_drop(const track_t *t)
{
    uint32_t ti = trk_index(t), h;
    if (!LV_ON(ti) || !lv.cur[ti].drop)
        return 0;
    h = ((ti << 6 | t->seq_idx) + (uint32_t)t->loops * 65539u) * 0x9E3779B1u;
    h = (h ^ h >> 15) * 0x9E3779B1u;
    return ((h >> 16) * 100u) >> 16 < lv.cur[ti].drop;
}
static uint32_t live_rat(const track_t *t) { uint32_t ti = trk_index(t); return LV_ON(ti) ? lv.cur[ti].rat : 0u; }
static int32_t live_up(const track_t *t)               /* (seq_step) OCT: semitones on the sequencer's notes */
{
    uint32_t ti = trk_index(t), f = lv.cur[ti].fl;
    return !LV_ON(ti) ? 0 : (f & LF_UP) ? 12 : (f & LF_DN) ? -12 : 0;
}
/* (rec_note, rec_release) a modified track's playhead and ghost swapped: recording at the ghost's place */
static int live_swap(track_t *t)
{
    uint32_t ti = trk_index(t), p;
    uint16_t x;
    if (!LV_ON(ti))
        return 0;
    x = t->seq_idx; t->seq_idx = lv.gidx[ti]; lv.gidx[ti] = x;
    p = t->seq_pos; t->seq_pos = lv.gpos[ti]; lv.gpos[ti] = p;
    return 1;
}

/* each block, before the sequencer (events_block): what the main loop asked for goes in, on the transport's 1/16 (RATE
 * on the beat), RETURN on the beat, BYPASS and COMMIT at once; back in phase, the playhead onto the ghost */
static __attribute__((noinline)) void live_q(uint32_t n)
{
    uint32_t s16, i, st, b16, bb, pa = 0;
    for (i = 0; i < NTRK; i++)                          /* (0.9.18) 1/2 TEMPO, RANDOM held (pfx.c) */
        pa |= (trk[i].pfx & (PFX_HALF | PFX_RND) ? 1u : 0u) << i;
    if (!(lv.busy | lv.act | pa | lv.pfa))
        return;
    lv.pfa = (uint8_t)pa;
    s16 = div_samples(2);
    st = clk_pos == CLK_START || !song.playing;
    b16 = st || clk_pos + n >= s16;
    bb = b16 && (st || !((clk_step + 1u) & 3u));
    for (i = 0; i < NTRK; i++) {
        track_t *t = &trk[i];
        lv_mod_t *c = &lv.cur[i], w = lv.want[i];
        uint32_t bit = 1u << i, rq = lv.rq[i] != lv.rs[i] ? lv.rk[i] : LQ_NONE, po = t->pfx & (PFX_HALF | PFX_RND);
        if (po & PFX_HALF)                              /* (0.9.18) the FX layer's: on the modifiers, at once */
            w.rate = (int8_t)(w.rate < -2 ? w.rate : -2);
        if (po & PFX_RND)
            w.fl = (uint8_t)((w.fl & ~6u) | LD_RND << 1);
        if (po != lv.pfo[i]) {
            lv.pfo[i] = (uint8_t)po;
            rq = LQ_NOW;
        }
        if (rq != LQ_NOW && !bb && (!b16 || rq == LQ_BEAT))
            continue;
        if (rq != LQ_NOW && !bb && !((lv.r16 >> i) & 1u))
            w.rate = c->rate;                           /* (RATE waits for the beat; MANO's keys: the 1/16) */
        lv.rs[i] = lv.rq[i];
        if (!(lv.act & bit)) {
            if (!song.playing || lv_neutral(&w)) {
                *c = w;
                continue;
            }
            lv.gidx[i] = t->seq_idx;                    /* (the ghost from here) */
            lv.gpos[i] = t->seq_pos;
            lv.rem[i] = lv.pd[i] = 0;
            lv.rj[i] = c->rot;                          /* (the ghost's place: no GIRO in it yet) */
            lv.act |= (uint8_t)bit;
        }
        lv.rj[i] = (int8_t)(lv.rj[i] + w.rot - c->rot);   /* (GIRO: live_next) */
        *c = w;
        t->jump_to = 0;                                 /* (live_adv: the next step again, by the new window) */
        if (lv_phase0(c)) {                             /* in phase again: onto the ghost */
            t->seq_idx = lv.gidx[i];
            t->seq_pos = lv.gpos[i];
            t->ext = 0;
            lv.rj[i] = 0;
            if (lv_neutral(c))
                lv.act &= (uint8_t)~bit;
        }
    }
}
