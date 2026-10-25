/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments
 * Copyright (C) 2026 Juan Ignacio Antonucci (Juan Jiant) */
/* LIVE (0.9.11, GLO > LIVE: MANO and RIELES; firmware/src/seq_live.c, ui_live.c) on the real sequencer and UI sources
 * (with the stubs of tests/ui_test.c):
 *   TIMING     RATE goes in on the next beat (MANO's rate keys: the next 1/16), CORTE / LOOP / RATCH / THIN on the next
 *              1/16; CORTE plays steps 1..N, a LOOP the N steps the playhead was in; RATCH xN that many parts a step, never
 *              shorter than ~1.4 ms; THIN the same steps on the same loop every PLAY, about its share; REV backwards, PING
 *              to and fro.
 *   GHOST      while modified the ghost walks as an unmodified track does (block for block); RETURN lands the playhead on it
 *              on the next beat (and the track is plain again); BYPASS (OCT- held) the original at once, the modifiers back.
 *   COMMIT     RATE into DIV (x2 of 1/16: 1/32), else STRETCH (x3: LEN / 3), else live (2/3 of 1/16); CORTE into LEN, a
 *              LOOP's steps to the start; GIRO into ROTATE; THIN into CHANCE; the dialog (OCT+, OCT+) and GLO + G4's UNDO.
 *   RECORD     a note played while modified lands at the ghost's step (where it is in the written pattern).
 *   PAGES      RIELES' KNOB 1: ALL, T1 .. T6 one by one (then SYN, DRM); MANO's keys are its board only on MANO (black keys
 *              pick tracks, several at once; white keys hold modifiers; LATCH; G5 RETURN; KNOB 4 the DJ FILTER) and play
 *              notes again off it; OCT- tapped RETURN, held BYPASS, the octave untouched; the GLO chain (LIVE first).
 * Built and run by tests/run_tests.sh. */
#define UI_TEST_NO_MAIN 1
#include "ui_test.c"

typedef struct { uint32_t at, idx; } lev_t;            /* a note-on of track 1: the sample, the step it played */
static uint32_t lv_clock;                               /* samples since the transport started (lsetup) */

/* six tracks of 16 steps of 1/16, no swing; track 1 a note on every step (48 + step), the others silent; playing */
static track_t *lsetup(void)
{
    uint32_t i, k;
    ui_power_on();
    for (i = 0; i < NTRK; i++) {
        track_t *t = &trk[i];
        track_defaults_steps(t);
        t->p[P_SLEN] = 16;
        t->p[P_SDIV] = 2;
        t->p[P_SSWING] = 0;
        t->p[P_SGATE] = 24;
        for (k = 0; k < 16u && !i; k++)
            t->step[k] = (step_t){{(uint8_t)(48 + k)}, 1, ST_NOTE, 0, 100};
    }
    song.g[G_SWING] = 0;
    memset(fg_set, 0, sizeof fg_set);
    seq_start();
    lv_clock = 0;
    return &trk[0];
}
static uint32_t lper(void) { return step_samples(&trk[0], div_samples(2), 0); }
/* one block of the ISR (events_block: live_q, the sequencer, the click); a note-on of track 1 into ev */
static uint32_t lblk(lev_t *ev, uint32_t n, uint32_t max)
{
    uint32_t v0 = vage;
    events_block(CTL);
    lv_clock += CTL;
    for (; v0 < vage; v0++)
        if (ev && n < max) {
            ev[n].at = lv_clock - CTL;
            ev[n++].idx = trk[0].seq_idx;
        } else
            n++;
    return n;
}
static uint32_t lrun(uint32_t samples, lev_t *ev, uint32_t n, uint32_t max)
{
    uint32_t end = lv_clock + samples;
    while (lv_clock < end)
        n = lblk(ev, n, max);
    return n;
}
static int crumb_is(const char *s, const char *cnt)   /* the footer's breadcrumb and n/N (ui_draw.c) */
{
    char b[48], c[8];
    foot_crumbs(b, sizeof b);
    return str_eq(b, s) && crumb_count(c) && str_eq(c, cnt);
}
static uint32_t tabs_lit(uint32_t *lit)                 /* the tabs: how many, the one lit */
{
    const char *nm[8], *tab[8];
    uint8_t cnt[8];
    uint32_t at;
    return tabs_now(nm, tab, cnt, lit, &at);
}
static int near(uint32_t a, uint32_t b) { return a + 2u * CTL >= b && a <= b + 2u * CTL; }

static int timing(void)
{
    int bad = 0, ok;
    uint32_t p, n, k, i;
    lev_t ev[64];
    track_t *t = lsetup();
    p = lper();
    lrun(3u * p / 2u, 0, 0, 0);                         /* half way into step 2 */
    lv.rate[0] = 2;                                     /* x2, track 1 */
    live_publish();
    n = lrun(4u * p, ev, 0, 64);
    ok = n >= 5u && near(ev[0].at, 2u * p) && near(ev[1].at, 3u * p) && near(ev[2].at, 4u * p) &&
         near(ev[3].at, 4u * p + p / 2u) && near(ev[4].at, 5u * p);
    if (!ok)
        for (k = 0; k < 6u && k < n; k++) printf("  x2: note-on %u at %u (step %u, p %u)\n", k, ev[k].at, ev[k].idx, p);
    bad += check("RATE x2 asked half way into step 2: steps 2 and 3 as they were, from the beat (step 5) twice as fast", ok);

    t = lsetup();
    lrun(3u * p / 2u, 0, 0, 0);
    lv.tk[1] = 1;                                       /* MANO's G3 (x2) held on track 1 */
    live_publish();
    n = lrun(2u * p, ev, 0, 64);
    ok = n >= 3u && near(ev[0].at, 2u * p) && near(ev[1].at, 2u * p + p / 2u) && near(ev[2].at, 3u * p);
    bad += check("  MANO's x2 key: on the next 1/16 (step 3 already twice as fast)", ok);
    lv.tk[1] = 0;
    live_publish();
    lrun(2u * p, 0, 0, 0);
    bad += check("  let go: back in phase with the transport (the ghost's place)", t->seq_idx == lv.gidx[0] ||
                 !LV_ON(0));

    t = lsetup();                                       /* CORTE 4 from step 6 */
    lrun(11u * p / 2u, 0, 0, 0);
    lv.cut[0] = 7;
    live_publish();
    n = lrun(9u * p, ev, 0, 64);
    ok = n >= 8u && ev[0].idx == 0u;
    for (k = 0; k < 8u && k < n; k++)
        ok &= ev[k].idx == k % 4u && near(ev[k].at, (6u + k) * p);
    bad += check("CORTE 4 asked in step 6: from the next 1/16 steps 1 2 3 4 1 2 3 4", ok);

    t = lsetup();                                       /* LOOP 2 (MANO's E4) in step 10 */
    lrun(19u * p / 2u, 0, 0, 0);
    lv.tk[6] = 1;
    live_publish();
    n = lrun(6u * p, ev, 0, 64);
    ok = n >= 6u && lv.want[0].ws == 8u;
    for (k = 0; k < 6u && k < n; k++)
        ok &= ev[k].idx == 8u + k % 2u;
    bad += check("LOOP 2 held in step 10: steps 9 10 9 10 (the two the playhead was in)", ok);

    t = lsetup();                                       /* RATCH x3 */
    lrun(p / 2u, 0, 0, 0);
    lv.dn[0] = 2;
    live_publish();
    lrun(p, 0, 0, 0);
    n = lrun(4u * p, 0, 0, 0);
    bad += check("DENS RATCH x3: three note-ons a step (12 in 4 steps)", n == 12u);
    t = lsetup();
    song.g[G_BPM] = GP[G_BPM].max;                      /* the fastest: 1/32 at x4, RATCH x4 asked */
    t->p[P_SDIV] = 3;
    lv.rate[0] = 4;
    lv.dn[0] = 3;
    live_publish();
    k = div_samples(3) / 4u;                            /* the step at x4 */
    ok = lv.want[0].rat >= 2u && lv.want[0].rat < 4u && k * 3u / 5u / lv.want[0].rat >= LV_MINPART;
    bad += check("  RATCH capped: no part shorter than 64 samples (~1.4 ms) at the shortest swing", ok);
    song.g[G_BPM] = GP[G_BPM].def;

    {   /* THIN 50 %: the same steps every PLAY, about half */
        uint64_t m[2] = {0, 0};
        uint32_t r, heard = 0;
        for (r = 0; r < 2u; r++) {
            t = lsetup();
            lv.dn[0] = -5;
            live_publish();
            n = lrun(64u * p - CTL, ev, 0, 64);
            for (k = 0; k < n && k < 64u; k++)
                m[r] |= 1ull << ((ev[k].at + CTL) / p % 64u);
            if (!r) heard = n;
        }
        bad += check("THIN 50 %: the same steps on the same loops every PLAY, 20 .. 44 of 64 heard", m[0] == m[1] &&
                     heard >= 20u && heard <= 44u);
    }

    t = lsetup();                                       /* REV (G4 latched) */
    lrun(p / 2u, 0, 0, 0);
    lv.lt[8] = 1;
    live_publish();
    n = lrun(8u * p, ev, 0, 64);
    ok = n >= 7u;
    for (k = 1; k < 7u && k < n; k++)
        ok &= ev[k].idx == (ev[k - 1u].idx + 15u) % 16u;
    bad += check("REV: backwards round the loop", ok);
    t = lsetup();                                       /* PING (A4 latched) */
    lrun(p / 2u, 0, 0, 0);
    lv.lt[9] = 1;
    live_publish();
    n = lrun(40u * p, ev, 0, 64);
    ok = n >= 38u;
    for (k = 1, i = 0; k < n && k < 64u; k++) {
        int32_t d = (int32_t)ev[k].idx - (int32_t)ev[k - 1u].idx;
        ok &= d == 1 || d == -1;
        i += k > 1u && d != (int32_t)ev[k - 1u].idx - (int32_t)ev[k - 2u].idx;
        ok &= !(k > 1u && d != (int32_t)ev[k - 1u].idx - (int32_t)ev[k - 2u].idx) || ev[k - 1u].idx == 0u ||
              ev[k - 1u].idx == 15u;
    }
    bad += check("PING: to and fro, turning at steps 1 and 16 only", ok && i >= 2u);
    return bad;
}

static int ghost(void)
{
    int bad = 0, ok = 1, differ = 0;
    uint32_t p, b, beat;
    track_t *t = lsetup(), *r = &trk[1];
    p = lper();
    beat = 4u * p;
    lv.rate[0] = 2;
    lv.lt[8] = 1;                                       /* x2 and REV */
    live_publish();
    for (b = 0; b < 3u * beat / CTL; b++) {
        lblk(0, 0, 0);
        if (LV_ON(0))
            ok &= lv.gidx[0] == r->seq_idx && lv.gpos[0] == r->seq_pos;
        differ |= t->seq_idx != r->seq_idx;
    }
    bad += check("the ghost walks as the unmodified track 2, block for block, while track 1 plays x2 REV", ok && differ &&
                 LV_ON(0));
    lrun((lv_clock / beat + 1u) * beat + beat / 4u - lv_clock, 0, 0, 0);   /* a quarter into a beat */
    live_return(1);
    ok = 1;
    differ = 0;
    while (lv_clock % beat >= CTL) {
        lblk(0, 0, 0);
        differ |= t->seq_idx != r->seq_idx;
    }
    for (b = 0; b < 2u * beat / CTL; b++) {
        lblk(0, 0, 0);
        ok &= t->seq_idx == r->seq_idx && t->seq_pos == r->seq_pos;
    }
    bad += check("RETURN: modified to the next beat, then on the ghost's step and sample (plain again)", ok && differ &&
                 !LV_ON(0) && lv_neutral(&lv.cur[0]));

    lv.rate[0] = 2;                                     /* BYPASS */
    live_publish();
    lrun(beat, 0, 0, 0);
    lv.byp = 1;
    lv_req(LV_ALL, LQ_NOW);
    live_publish();
    lblk(0, 0, 0);
    ok = t->seq_idx == r->seq_idx && t->seq_pos == r->seq_pos;
    for (b = 0; b < beat / CTL; b++) {
        lblk(0, 0, 0);
        ok &= t->seq_idx == r->seq_idx && t->seq_pos == r->seq_pos;
    }
    lv.byp = 0;
    lv_req(LV_ALL, LQ_NOW);
    live_publish();
    lblk(0, 0, 0);
    ok &= LV_ON(0) && lv.cur[0].rate == 2;
    bad += check("BYPASS: the original at once, block for block; let go, the modifiers on again", ok);
    return bad;
}

static int commit(void)
{
    int bad = 0, ok;
    uint32_t p;
    track_t *t = lsetup();
    p = lper();
    lv.rate[0] = 2;
    live_publish();
    lrun(2u * p, 0, 0, 0);
    undo_mark();
    live_commit(1);
    ok = t->p[P_SDIV] == 3 && !lv.rate[0] && lv_neutral(&lv.want[0]) && !LV_ON(0);
    lrun(p, 0, 0, 0);
    ok &= !undo_do() && t->p[P_SDIV] == 2;
    bad += check("COMMIT x2 of 1/16: DIV 1/32, nothing live after; UNDO: 1/16 again", ok);

    t = lsetup();
    lv.rate[0] = 3;                                     /* x3: no DIV, STRETCH /3 */
    live_publish();
    lrun(p, 0, 0, 0);
    live_commit(1);
    ok = t->p[P_SDIV] == 2 && t->p[P_SLEN] == 6 && !lv.rate[0];
    t = lsetup();
    lv.rate[0] = -1;                                    /* 2/3: none, live */
    live_publish();
    live_commit(1);
    ok &= t->p[P_SDIV] == 2 && t->p[P_SLEN] == 16 && lv.rate[0] == -1 && lv.want[0].rate == -1;
    bad += check("  x3: STRETCH (LEN 16 -> 6); 2/3 of 1/16 has no DIV: it stays live", ok);

    t = lsetup();
    lv.cut[0] = 7;                                      /* CORTE 4 */
    live_publish();
    lrun(p, 0, 0, 0);
    live_commit(1);
    ok = t->p[P_SLEN] == 4 && t->step[0].note[0] == 48;
    t = lsetup();
    stop_transport();
    t->seq_idx = 9;
    lv.cut[0] = 7;                                      /* LOOP 4 from step 10: steps 9 .. 12 */
    lv.here = 1;
    live_publish();
    live_commit(1);
    ok &= t->p[P_SLEN] == 4 && t->step[0].note[0] == 56 && t->step[3].note[0] == 59;
    bad += check("  CORTE 4: LEN 4; a LOOP of 4 from step 10: its steps to the start, LEN 4", ok);

    t = lsetup();
    lv.rot[0] = 2;
    lv.dn[0] = -5;
    live_publish();
    lrun(p, 0, 0, 0);
    live_commit(1);
    ok = t->p[P_SOFS] == -2 && t->p[P_TCHAN] == 64 && lv_neutral(&lv.want[0]);
    bad += check("  GIRO +2: ROTATE -2; THIN 50 %: the track's CHANCE 50 %", ok);

    t = lsetup();                                       /* the dialog, on RIELES */
    nav_put(SEC_GLO, "RIELES");
    frame();
    lv.rate[0] = 2;
    frame();
    press(B_OCTUP);
    ok = ui.confirm == CF_LIVE_COMMIT && t->p[P_SDIV] == 2;
    press(B_OCTUP);
    frame();
    ok &= !ui.confirm && t->p[P_SDIV] == 3 && undo_ok;
    ly_white(B_GLO, GLO_UNDO);
    ok &= t->p[P_SDIV] == 2;
    bad += check("  OCT+ asks, OCT+ writes (DIV 1/32); GLO + G4 takes it back", ok);
    return bad;
}

static int record(void)
{
    int bad = 0, ok = 0;
    uint32_t p, b, gi, s;
    track_t *t = lsetup();
    p = lper();
    for (b = 0; b < 16u; b++) t->step[b].time = ST_REST, t->step[b].n = 0;
    song.rec = 1;
    lv.rate[0] = 2;
    live_publish();
    for (b = 0; b < 4u * p / CTL; b++) lblk(0, 0, 0);
    while (!(LV_ON(0) && t->seq_idx % 16u != lv.gidx[0] && lv.gpos[0] < p / 3u)) lblk(0, 0, 0);
    gi = lv.gidx[0];
    s = t->seq_idx % 16u;
    rec_note(t, 99, 100);
    rec_release(t, 99);
    ok = t->step[seq_src(t, gi)].time == ST_NOTE && t->step[seq_src(t, gi)].note[0] == 99 &&
         t->step[seq_src(t, s)].time != ST_NOTE && t->seq_idx % 16u == s;
    song.rec = 0;
    bad += check("REC while x2: the note lands at the ghost's step (the written pattern's), not the playhead's", ok);
    return bad;
}

static int pages(void)
{
    int bad = 0, ok = 1;
    uint32_t k, v0, kx2, kb1, kb3, kd2, ksyn = 0, kdrm = 0;
    static const uint8_t TG[9] = {0xFF, 1, 2, 4, 8, 16, 32, 64, 128};   /* (0.9.19: ALL = the eight tracks) */
    lsetup();
    stop_transport();
    for (k = 0; k < NTRK; k++)
        *(drum_track(&trk[k]) ? &kdrm : &ksyn) |= 1u << k;
    nav_put(SEC_GLO, "RIELES");
    frame();
    ok = lv_tracks() == 0xFFu;
    for (k = 1; k < 11u; k++) {
        turn(EN_K1, 1);
        ok &= lv.tgt == k && lv_tracks() == (k < 9u ? TG[k] : k == 9u ? ksyn : kdrm);
    }
    for (k = 0; k < 11u; k++) turn(EN_K1, -1);
    ok &= lv.tgt == 0 && lv_tracks() == 0xFFu && song.grid == 0;
    bad += check("RIELES KNOB 1 PISTAS: ALL, then T1 .. T8 one by one, then SYN and DRM; the keys play notes here", ok);
    turn(EN_K1, 3);                                     /* T3: RATE x2, CORTE 8, DENS 70 % */
    turn(EN_K2, 2);
    turn(EN_K3, 5);
    turn(EN_K4, -3);
    frame();
    ok = lv.rate[2] == 2 && !lv.rate[0] && lv.want[2].cut == 8 && !(lv.want[2].fl & LF_HERE) && lv.want[2].drop == 30;
    if (!ok) printf("  tgt %u rate %d %d cut %u fl %u drop %u\n", lv.tgt, lv.rate[2], lv.rate[0], lv.want[2].cut, lv.want[2].fl, lv.want[2].drop);
    k = song.octave;
    press(B_OCTDN);
    frames(64);
    ok &= !lv.rate[2] && lv_neutral(&lv.want[2]) && song.octave == (int8_t)k;
    bad += check("  T3: RATE x2 CORTE 8 DENS 70 %; OCT- tapped RETURN (the octave untouched)", ok);
    turn(EN_K2, 2);
    fm1_in.buttons |= 1u << panel.btn[B_OCTDN];
    host_pressed |= 1u << panel.btn[B_OCTDN];
    frames(480);
    ok = lv.byp && lv_neutral(&lv.want[2]);
    fm1_in.buttons &= ~(1u << panel.btn[B_OCTDN]);
    frame();
    frame();
    ok &= !lv.byp && lv.want[2].rate == 2;
    bad += check("  OCT- held: BYPASS (nothing live while held); let go, the modifiers again", ok);

    lsetup();
    stop_transport();
    nav_put(SEC_GLO, "MANO");
    frame();
    kx2 = key_at(0, 1);
    kb1 = key_at(1, 0);
    kb3 = key_at(1, 2);
    kd2 = key_at(0, 0);
    ok = song.grid == 3;
    v0 = vage;
    fm1_in.notes |= 1u << kx2; host_notes |= 1u << kx2;
    keyboard_block(); frame();
    for (k = 0; k < NTRK; k++) ok &= lv.want[k].rate == 2;
    ok &= vage == v0 && ((lv.kb >> kx2) & 1u);
    fm1_in.notes &= ~(1u << kx2);
    keyboard_block(); frame();
    for (k = 0; k < NTRK; k++) ok &= lv_neutral(&lv.want[k]);
    bad += check("MANO: the keys are its board (G3 held: x2 on every track, no note); let go, nothing", ok);
    fm1_in.notes |= 1u << kb1 | 1u << kb3; keyboard_block(); frame();
    fm1_in.notes |= 1u << kd2; keyboard_block(); frame();
    ok = lv.want[0].rate == -2 && lv.want[2].rate == -2 && !lv.want[1].rate && !lv.want[3].rate && lv_tracks() == 5u;
    fm1_in.notes = 0; keyboard_block(); frame();
    bad += check("  black keys 1 and 3 held, F3 (/2): on tracks 1 and 3 only", ok);
    fm1_in.notes |= 1u << key_at(1, 6); keyboard_block(); frame();
    fm1_in.notes = 0; keyboard_block(); frame();
    fm1_in.notes |= 1u << key_at(0, 5); keyboard_block(); frame();
    fm1_in.notes = 0; keyboard_block(); frame();
    ok = lv.latch && lv.lt[5] == 0xFF && lv.want[4].cut == 4 && (lv.want[4].fl & LF_HERE);
    fm1_in.notes |= 1u << key_at(0, 5); keyboard_block(); frame();
    fm1_in.notes = 0; keyboard_block(); frame();
    ok &= !lv.lt[5] && lv_neutral(&lv.want[4]);
    fm1_in.notes |= 1u << key_at(0, 5); keyboard_block(); frame();
    fm1_in.notes = 0; keyboard_block(); frame();
    fm1_in.notes |= 1u << key_at(0, 15); keyboard_block(); frame();
    fm1_in.notes = 0; keyboard_block(); frame();
    ok &= !lv.lt[5] && lv_neutral(&lv.want[4]);
    bad += check("  G#4 LATCH: C4 (LOOP 4) on, again off; G5 RETURN clears what is latched", ok);
    k = (uint32_t)song.g[G_DJF];
    turn(EN_K4, 3);
    turn(EN_K1, 1);
    frame();
    ok = song.g[G_DJF] != (int16_t)k && lv.rate[0] == 1 && lv.rate[5] == 1;
    if (!ok) printf("  djf %d was %d rate %d %d kb %x page %s\n", song.g[G_DJF], (int)k, lv.rate[0], lv.rate[5], lv.kb, cur_page()->title);
    bad += check("  KNOB 4 the DJ FILTER (as MIXER's), KNOB 1 RATE on every track (none held)", ok);
    nav_put(SEC_GLO, "MIXER");
    frame();
    v0 = vage;
    fm1_in.notes |= 1u << kx2; host_notes |= 1u << kx2;
    keyboard_block(); frame();
    ok = song.grid == 0 && vage != v0 && !((lv.kb >> kx2) & 1u);
    fm1_in.notes = 0; keyboard_block(); frame();
    bad += check("  off MANO (MIXER) the keys play notes again", ok);

    ui_power_on();
    frame();
    press(B_GLO); frame();
    ok = str_eq(cur_page()->title, "MIXER");
    turn(EN_SELECT, -1); frame();
    ok &= str_eq(cur_page()->title, "RIELES") && crumb_is("GLO \xB7 LIVE \xB7 RIELES", "2/6");
    turn(EN_SELECT, -1); frame();
    turn(EN_SELECT, -1); frame();
    ok &= str_eq(cur_page()->title, "MANO") && crumb_is("GLO \xB7 LIVE \xB7 MANO", "1/6") && tabs_lit(&k) == 4u && !k &&
          song.grid == 3;
    bad += check("GLO: MIXER at power-on; SELECT back RIELES, MANO (the chain's first, LIVE · four tabs)", ok);
    ui_power_on();
    return bad;
}

/* (0.9.14) the FX layer's PERFORMANCE MACROS (perform.c perf_m; ui_layer.c pm_knob; ui_live.c, seq.c, project.c) */
static uint32_t gated_note(const track_t *t)             /* the note track t struck last (0: none) */
{
    uint32_t i, a = 0, n = 0;
    for (i = 0; i < NVOICE; i++)
        if (t->v[i].age > a) {
            a = t->v[i].age;
            n = t->v[i].note;
        }
    return n;
}
static int macros(void)
{
    int bad = 0, ok;
    uint32_t p, n, k;
    lev_t ev[64];
    track_t *t = lsetup();
    p = lper();
    lrun(19u * p / 2u, 0, 0, 0);                        /* in step 10 */
    btn_down(B_FX); frame();
    turn(EN_K1, 1);                                     /* STEPS 1/2: the 8 steps the playhead is in */
    n = lrun(10u * p, ev, 0, 64);
    ok = n >= 9u && lv.want[0].cut == 8u && lv.want[0].ws == 8u && near(ev[0].at, 10u * p);
    for (k = 0; k < 9u && k < n; k++)
        ok &= ev[k].idx == 8u + (2u + k) % 8u;
    bad += check("STEPS 1/2 turned in step 10: from the next 1/16 steps 11 .. 16 9 10 11 (the half it is in)", ok);
    turn(EN_K1, 11);                                    /* all the way: 1 STEP */
    n = lrun(4u * p, ev, 0, 64);
    ok = lv.want[0].cut == 1u && n >= 3u && ev[1].idx == ev[2].idx && ev[2].idx == ev[3 < n ? 3 : 2].idx;
    bad += check("  STEPS at the right end: a 1-step loop (every track)", ok && lv.want[3].cut == 1u);
    turn(EN_K1, -2);                                    /* the cut: back in phase on the next 1/16 */
    lrun(2u * p, 0, 0, 0);
    ok = !perf_m[0] && !LV_ON(0) && t->seq_idx == (lv_clock / p) % 16u;
    bad += check("  turned back: all off, the track where it would have been (its ghost)", ok);
    btn_up(B_FX); frame();

    /* TRANSPOSE: along the scale; the kit untouched */
    t = lsetup();
    t->p[P_ROOT] = 0;
    t->p[P_SCALE] = 1;                                  /* C MAJ */
    for (k = 0; k < 16u; k++)
        t->step[k].note[0] = 60;
    btn_down(B_FX); frame();
    turn(EN_K2, 4);                                     /* +3RD (three detents an interval) */
    ok = pm_trn() == 2u && strn_note(t, 60) == 64u && strn_note(t, 62) == 65u;
    lrun(2u * p + p / 4u, 0, 0, 0);
    ok &= gated_note(t) == 64u;
    turn(EN_K2, 30);                                    /* -OCT at the right end */
    ok &= pm_trn() == 7u && strn_note(t, 60) == 48u;
    lrun(p, 0, 0, 0);
    ok &= gated_note(t) == 48u;
    turn(EN_K2, -2);
    lrun(p, 0, 0, 0);
    ok &= gated_note(t) == 60u && !perf_m[1];
    bad += check("TRANSPOSE: +3RD in C major (C -> E, D -> F) on the next step, -OCT at the end; back: as written", ok);
    btn_up(B_FX); frame();
    /* MUTATE: the patterns move on the beat, every bar; off: exactly as they were */
    t = lsetup();
    for (k = 0; k < 16u; k++) t->step[k].note[0] = (uint8_t)(48 + (k * 5u) % 12u);
    {
        static step_t before[NTRK][NSTEP];
        uint32_t i, moved = 0;
        for (i = 0; i < NTRK; i++) memcpy(before[i], trk[i].step, sizeof before[i]);
        btn_down(B_FX); frame();
        turn(EN_K3, 100);
        for (i = 0; i < 8u * 4u; i++) { lrun(p, 0, 0, 0); frame(); mutate_poll(); }
        moved = memcmp(before[0], trk[0].step, sizeof before[0]) != 0;
        turn(EN_K3, -2); frame(); mutate_poll();
        ok = moved && !perf_m[2];
        for (i = 0; i < NTRK; i++) ok &= !memcmp(before[i], trk[i].step, sizeof before[i]);
        btn_up(B_FX); frame();
        bad += check("MUTATE at 100: the patterns mutate while playing; turned back: every step as it was", ok);
    }

    /* FILTER: the DJ FILTER's low-pass, closing to the right; off: centred */
    btn_down(B_FX); frame();
    turn(EN_K4, 50);
    perf_begin(CTL);
    ok = perf_djf == -32 && djf_v() == -32;
    turn(EN_K4, 50);
    perf_begin(CTL);
    ok &= perf_djf == -64;
    turn(EN_K4, -3);
    perf_begin(CTL);
    ok &= !perf_djf && djf_v() == song.g[G_DJF];
    btn_up(B_FX); frame();
    bad += check("FILTER: the DJ FILTER's low-pass, -32 at 50 %, -64 at the end; back: the knob's own", ok);
    stop_transport();
    return bad;
}

int main(void)
{
    int bad = timing() + ghost() + commit() + record() + pages() + macros();
    printf("%s\n", bad ? "LIVE TEST FAILED" : "live tests passed");
    return bad != 0;
}
