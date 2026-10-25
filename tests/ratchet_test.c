/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments
 * Copyright (C) 2026 Juan Ignacio Antonucci (Juan Jiant) */
/* RATCH, a step's ratchet (core.h SF_RATCH, seq.c seq_step / seq_ratchet), on the real sequencer, project, user
 * preset and UI sources (with the stubs of tests/ui_test.c):
 *   PLAY       x1..x4: that many note-ons in one pass of the step, in equal parts of it, each retriggered after its
 *              gate; chords and the DRUM grid's hits repeat whole; x1 slides and ties out as before, x2..x4 never;
 *              the chance is rolled once per pass for all its parts; swing; STOP and an edit to fewer end them.
 *   PROJECTS   FUN8 round trip of x1..x4 (bit 7 of the velocity and chance bytes), an x1 project packs those bytes
 *              as before, FUN7 / FUN6 images load x1.
 *   PRESETS    a user preset's note pattern keeps it (flags 8 | 16, UP_PUT too); a drum grid record has no room: x1.
 *   UI         SEQ > CHANCE KNOB 3 (RATCH x1..x4, clamped, the cursor step; dimmed on a REST, a TIE, an empty
 *              step); the piano roll and the DRUM grid draw a ratcheted step in its parts, the roll no slide line
 *              out of one.
 * The editor protocol (STEP_SET / TRACK_STEP, INFO 52 01 04) is in tests/editor_test.c. Built and run by
 * tests/run_tests.sh. */
#define UI_TEST_NO_MAIN 1
#define PROJ_FUNB_WRITER 1                               /* (JIANT 0.6) the byte checks below read a FUNB */
#include "ui_test.c"

/* track 1 alone, a one-step loop: step 0 NOTE 60 (flags), RATCH hits, GATE 64, no swing; the transport started */
static track_t *one_step(uint32_t hits, uint32_t flags)
{
    track_t *t;
    ui_power_on();
    t = &trk[0];
    track_defaults_steps(t);
    t->p[P_SLEN] = 1;
    t->p[P_SGATE] = 64;
    t->p[P_SSWING] = 0;
    song.g[G_SWING] = 0;
    t->step[0] = (step_t){{60}, 1, ST_NOTE, (uint8_t)flags, 100};
    step_set_ratchet(&t->step[0], hits);
    seq_start();
    return t;
}
static uint32_t step_len(const track_t *t, uint32_t idx) { return step_samples(t, div_samples((uint32_t)t->p[P_SDIV]), idx); }

/* note-ons (voice starts) over `blocks` CTL blocks; at[k]: the sample of the k-th; rel: releases seen between */
static uint32_t run(track_t *t, uint32_t blocks, uint32_t *at, uint32_t nat, uint32_t *rel)
{
    uint32_t b, n = 0, v0, was = 0;
    for (b = 0; b < blocks; b++) {
        v0 = vage;
        seq_tick(t, CTL);
        for (; v0 < vage; v0++, n++)
            if (at && n < nat)
                at[n] = b * CTL;
        if (rel && was && !t->seq_n)
            (*rel)++;
        was = t->seq_n;
    }
    return n;
}
/* blocks in `passes` passes of a one-step loop, the last block before the next pass left out */
static uint32_t passes(const track_t *t, uint32_t n) { return n * step_len(t, 0) / CTL - 1u; }

/* (0.9.18) an 8-step loop on track 1: step 0 NOTE 60 at RATCH hits (flags), the rest empty; the transport started */
static track_t *spill_trk(uint32_t hits, uint32_t flags)
{
    track_t *t = one_step(hits, flags);
    uint32_t i;
    t->p[P_SLEN] = 8;
    for (i = 1; i < 8u; i++)
        t->step[i] = (step_t){{0}, 0, ST_REST};
    seq_start();
    return t;
}
/* note-ons over `steps` steps of t (its note and velocity each: nt, vl) */
static uint32_t run_nv(track_t *t, uint32_t steps, uint32_t *at, uint8_t *nt, uint8_t *vl, uint32_t max)
{
    uint32_t b, n = 0, k, blocks = steps * step_len(t, 0) / CTL - 1u;
    for (b = 0; b < blocks; b++) {
        uint32_t v0 = vage;
        seq_tick(t, CTL);
        for (; v0 < vage; v0++, n++)
            for (k = 0; k < NVOICE && n < max; k++)
                if (t->v[k].age == v0 + 1u) {
                    if (at) at[n] = b * CTL;
                    if (nt) nt[n] = t->v[k].note;
                    if (vl) vl[n] = t->v[k].vel;
                }
    }
    return n;
}
static int play(void)
{
    int bad = 0, ok = 1;
    uint32_t h, n, at[40], len, k;
    uint8_t nt[40], vl[40];
    track_t *t;
    for (h = 1; h <= 4u; h++) {                       /* a synth: h hits, 4 steps / h apart, spilling */
        t = spill_trk(h, 0);
        len = step_len(t, 0);
        n = run_nv(t, 8, at, 0, 0, 40);
        ok &= n == h;
        for (k = 0; k < n && k < 8u; k++) {
            int32_t d = (int32_t)at[k] - (int32_t)(k * (len * 4u / h));
            ok &= d > -(int32_t)CTL && d <= (int32_t)CTL;
        }
        if (n != h) printf("ui:   RATCH x%u: %u note-ons in 8 steps\n", h, n);
    }
    bad += check("RATCH x1..x4 (0.9.18): that many note-ons, 4 steps / n apart from the step's start (spilling)", ok);
    t = spill_trk(4, 0);
    bad += check("  x4 over 8 passes of 8 steps: 32 note-ons", run_nv(t, 64, 0, 0, 0, 0) == 32u);
    t = spill_trk(3, 0);
    t->p[P_VOICE] = V_POLY;
    t->step[0].n = 3; t->step[0].note[1] = 64; t->step[0].note[2] = 67;
    bad += check("  a chord of 3 at x3: 9 note-ons", run_nv(t, 8, 0, 0, 0, 0) == 9u);
    t = spill_trk(1, SF_SLIDE);
    run(t, 4, 0, 0, 0);
    ok = t->seq_hold == 1u;
    t = spill_trk(2, SF_SLIDE);
    run(t, step_len(t, 0) / CTL - 2u, 0, 0, 0);
    bad += check("  x1 slides out as before; x2 never", ok && !t->seq_hold);
    t = spill_trk(4, 0);
    t->step[1] = (step_t){{0}, 0, ST_TIE};
    seq_start();
    run(t, step_len(t, 0) / CTL + step_len(t, 1) / CTL / 2u, 0, 0, 0);
    bad += check("  nor ties out: a TIE after a ratcheted step holds nothing of the step's own", !t->seq_n);

    t = spill_trk(4, 0);                              /* the rule: another step's own note */
    t->p[P_VOICE] = V_POLY;
    t->step[2] = (step_t){{60}, 1, ST_NOTE, 0, 100};
    n = run_nv(t, 8, 0, 0, 0, 40);
    ok = n == 4u;                                     /* (0, 1, step 2's own (the spilled one left out), 3) */
    t = spill_trk(4, 0);
    t->p[P_VOICE] = V_POLY;
    t->step[2] = (step_t){{64}, 1, ST_NOTE, 0, 100};
    ok &= run_nv(t, 8, 0, 0, 0, 40) == 5u;            /* (another note: both) */
    t = spill_trk(4, 0);
    t->p[P_VOICE] = V_MONO;
    t->step[2] = (step_t){{64}, 1, ST_NOTE, 0, 100};
    n = run_nv(t, 8, 0, 0, 0, 40);
    ok &= n == 3u;                                    /* (MONO: step 2's note cuts the rest) */
    if (!ok) printf("  rule: mono %u\n", n);
    bad += check("  a spilled hit on a step with the same note: left out; another note on POLY: both; MONO: cut", ok);

    {   /* (0.9.18) a POLY note recorded into a ratcheted synth step: once, live and on the passes after, not rolled
         * with the step's own (lm slot LM_ONCE); into a plain step: no mark */
        uint32_t n60 = 0, n64 = 0;
        t = spill_trk(2, 0);
        t->p[P_VOICE] = V_POLY;
        run(t, 2, 0, 0, 0);                           /* (early in step 1) */
        rec_note(t, 64, 100);
        rec_release(t, 64);
        ok = t->step[0].n == 2u && t->step[0].note[1] == 64u && step_lm_raw(&t->step[0], 1u) == LM_ONCE &&
             step_lm_raw(&t->step[0], 0u) == LM_NONE;
        seq_start();
        n = run_nv(t, 8, 0, nt, 0, 40);
        for (k = 0; k < n; k++) { n60 += nt[k] == 60u; n64 += nt[k] == 64u; }
        ok &= n60 == 2u && n64 == 1u;
        t = spill_trk(1, 0);
        t->p[P_VOICE] = V_POLY;
        run(t, 2, 0, 0, 0);
        rec_note(t, 64, 100);
        rec_release(t, 64);
        ok &= t->step[0].n == 2u && !step_lm_any(&t->step[0]);
        bad += check("  a POLY note recorded into an x2 step: once (60 x2, 64 x1); into an x1 step: no mark", ok);
    }

    t = spill_trk(3, 0);                              /* the DRUM grid: hits of BD (accented) and CH */
    set_engine_of(t, ENGI_DRUM);
    t->engine = t->eng_req;
    t->step[0] = (step_t){{0}, 0, ST_NOTE, 0, 100, 1u << DV_KICK | 1u << DV_HATC, 1u << DV_KICK};
    step_set_ratchet(&t->step[0], 3);
    seq_start();
    len = step_len(t, 0);
    n = run_nv(t, 8, at, nt, vl, 40);
    ok = n == 6u && at[5] >= len * 4u / 3u - CTL && at[5] < len * 2u;   /* (x3 over 2 steps: 0, 2/3, 4/3) */
    for (k = 0; k < n; k++)
        ok &= nt[k] != DRUM_LANE_NOTE[DV_KICK] || vl[k] == 127u;
    bad += check("  DRUM grid: BD + CH at x3, 6 hits over 2 steps (a kit's x 2), BD accented", ok);
    t->step[1] = (step_t){{0}, 0, ST_NOTE, 0, 100, 1u << DV_KICK};
    seq_start();
    bad += check("  .. a step after it with its own BD: the BD spilled there left out", run_nv(t, 8, 0, 0, 0, 40) == 6u);
    t->step[1] = (step_t){{0}, 0, ST_REST};
    step_set_chance(&t->step[0], 0);
    seq_start();
    bad += check("  chance 0: no hit plays", run_nv(t, 16, 0, 0, 0, 40) == 0u);

    t = spill_trk(4, 0);                              /* chance: one roll for all four hits */
    step_set_chance(&t->step[0], 50);
    {
        uint32_t heard = 0, odd = 0, p;
        for (p = 0; p < 200u; p++) {
            n = run_nv(t, 8, 0, 0, 0, 0);
            odd += n != 0u && n != 4u;
            heard += n;
        }
        bad += check("  chance 50 % at x4: each pass all 4 hits or none, about half", !odd && heard > 4u * 70u &&
                     heard < 4u * 130u);
    }
    {   /* FADE, RISE, the pitch steps */
        static const uint8_t SH[3] = {0, CP_ECHO_RISE, CP_ECHO_EVEN};
        for (k = 0; k < 3u; k++) {
            t = spill_trk(1, 0);
            t->step[0].comp = CP_ECHO; t->step[0].carg = (uint8_t)(2u | SH[k]);   /* x4 */
            seq_start();
            n = run_nv(t, 8, 0, 0, vl, 40);
            ok = n == 4u && (k == 0u ? vl[0] > vl[1] && vl[1] > vl[2] && vl[2] > vl[3] :
                             k == 1u ? vl[0] < vl[1] && vl[1] < vl[2] && vl[2] < vl[3] : vl[0] == vl[3]);
            bad += check(k == 0u ? "  RATCH x4 FADE: each hit softer" : k == 1u ? "  RATCH x4 RISE: each louder" :
                         "  RATCH x4 FLAT: all at the step's level", ok);
        }
        t = spill_trk(1, 0);
        t->p[P_ROOT] = 0; t->p[P_SCALE] = 1;          /* (C major) */
        t->step[0].comp = CP_ECHO; t->step[0].carg = (uint8_t)(2u | CP_ECHO_EVEN | RP_UP << 4);
        seq_start();
        n = run_nv(t, 8, 0, nt, 0, 40);
        ok = n == 4u && nt[0] == 60u && nt[1] == 62u && nt[2] == 64u && nt[3] == 65u;
        t = spill_trk(1, 0);
        t->step[0].comp = CP_ECHO; t->step[0].carg = (uint8_t)(2u | CP_ECHO_EVEN | RP_OCT << 4);
        seq_start();
        n = run_nv(t, 8, 0, nt, 0, 40);
        ok &= n == 4u && nt[0] == 60u && nt[1] == 72u && nt[2] == 60u && nt[3] == 72u;
        bad += check("  RATCH's pitch step: +1 DEG 60 62 64 65; +OCT 60 72 60 72", ok);
    }
    t = spill_trk(4, 0);
    run(t, 2, 0, 0, 0);
    ok = t->sp_left == 3u;
    seq_stop();
    bad += check("  STOP: no hit left", ok && !t->sp_left && !t->sp_pm && !t->seq_n);
    seq_stop();
    return bad;
}

/* (0.9.18) SEQ > COMP marks on the roll and the grid (ui_graph.c seq_overlay): the step the component goes on framed
 * THEME with a tab at its foot, following KNOB 1 at once; the steps with the same component a 3 px ACCENT bar on top
 * (a RATCH of the flags the same as an ECHO), another component a 1 px MID one; a JUMP's step framed ACCENT, following
 * KNOB 3 */
static uint16_t gpx(int32_t x, int32_t y) { return swap16(host_screen[(uint32_t)(y + (int32_t)graph_y()) * 240u + (uint32_t)x]); }
static int comp_marks(void)
{
    int bad = 0, ok, dr;
    uint32_t i;
    track_t *t;
    for (dr = 0; dr < 2; dr++) {
        int32_t x, w, top, bot, y;
        ui_power_on();
        song.sel = dr ? 3u : 0u;
        t = TSEL;
        if (dr) { set_engine_of(t, ENGI_DRUM); t->engine = t->eng_req; }
        track_defaults_steps(t);
        t->p[P_SLEN] = 16;
        for (i = 0; i < 16u; i++)
            t->step[i] = dr ? (step_t){{0}, 0, ST_NOTE, 0, 100, 1u << DV_HATC} : (step_t){{(uint8_t)(52u + i)}, 1, ST_NOTE, 0, 100};
        t->step[2].comp = CP_ECHO; t->step[2].carg = 1;
        step_set_ratchet(&t->step[7], 3);
        t->step[12].comp = CP_ECHO;
        t->step[4].comp = CP_RAND; t->step[4].carg = 2;
        go_page(GR_COMP);
        cursor_set(12);
        ui.force = 1; frame();
        top = dr ? 19 : PR_Y0; bot = dr ? 19 + 8 * 15 - 1 : PR_Y0 + PR_ROWS * PR_RH;
        y = (top + bot) / 2;
        seq_col((uint32_t)dr, 12, &x, &w);
        ok = gpx(x - 1, y) == T_THEME && gpx(x + w + 1, y) == T_THEME && gpx(x + w / 2, bot - 2) == T_THEME;
        seq_col((uint32_t)dr, 2, &x, &w);
        ok &= gpx(x + w / 2, top - 2) == T_ACCENT && gpx(x - 1, y) != T_THEME;
        seq_col((uint32_t)dr, 7, &x, &w);
        ok &= gpx(x + w / 2, top - 2) == T_ACCENT;
        seq_col((uint32_t)dr, 4, &x, &w);
        ok &= gpx(x + w / 2, top - 2) == T_MID && gpx(x + w / 2, top - 3) != T_ACCENT;
        turn(EN_K1, 1); frame();                          /* KNOB 1: the frame on step 14 at once */
        seq_col((uint32_t)dr, 13, &x, &w);
        ok &= ui.cursor == 13u && gpx(x - 1, y) == T_THEME;
        seq_col((uint32_t)dr, 12, &x, &w);
        ok &= gpx(x - 1, y) != T_THEME;
        t->step[13].comp = CP_JUMP; t->step[13].carg = 5;   /* a JUMP to step 6, then KNOB 3 to step 7 */
        ui.force = 1; frame();
        seq_col((uint32_t)dr, 5, &x, &w);
        ok &= gpx(x + 1, y) == T_ACCENT;
        turn(EN_K3, 1); frame();
        ok &= CP_JTO(t->step[13].carg) == 6u && gpx(x + 1, y) != T_ACCENT;
        seq_col((uint32_t)dr, 6, &x, &w);
        ok &= gpx(x + 1, y) == T_ACCENT;
        bad += check(dr ? "  .. the same on a kit's grid" :
                     "SEQ > COMP marks: the step framed THEME (live with KNOB 1), the same component ACCENT, another MID, JUMP's step ACCENT", ok);
    }
    return bad;
}

static int projects(void)
{
    int bad = 0, ok = 1;
    project_t before, after;
    project_store_t packed;
    uint32_t i, k, pos;
    track_t *t;
    ui_power_on();
    t = &trk[0];
    for (i = 0; i < 8u; i++) {
        t->step[i] = (step_t){{(uint8_t)(60 + i)}, 1, ST_NOTE, i & 1u ? SF_ACCENT : 0u, i & 2u ? 127u : 90u};
        step_set_ratchet(&t->step[i], 1u + i % 4u);
        step_set_chance(&t->step[i], i < 4u ? 100u : i == 4u ? 0u : 30u);
    }
    trk[3].step[5].hit = 0x81; trk[3].step[5].acc = 0x80; trk[3].step[5].time = ST_NOTE;
    step_set_ratchet(&trk[3].step[5], 4);
    project_capture(&before);
    ok = proj_pack(&packed, &before) && proj_import(&after, &packed, sizeof packed) && !memcmp(&before, &after, sizeof before);
    for (i = 0; ok && i < 8u; i++)
        ok &= step_ratchet(&after.t[0].step[i]) == 1u + i % 4u && step_chance(&after.t[0].step[i]) == step_chance(&before.t[0].step[i]) &&
              after.t[0].step[i].vel == before.t[0].step[i].vel;
    bad += check("FUN8: x1..x4 round trip with their velocity, accent, chance (0 and 30 %) and lane hits", ok &&
                 step_ratchet(&after.t[3].step[5]) == 4u && after.t[3].step[5].hit == 0x81);
    proj_pack_vb(&packed, &before);                  /* (FUNB: fixed places) */
    pos = 68u + P_COUNT + 2u + 3u * 9u;              /* step 3 of track 1: x4 -> bit 7 of its velocity and chance */
    bad += check("  x4 is bit 7 of the velocity and the chance bytes", (packed.raw[pos + 5] & 128u) && (packed.raw[pos + 8] & 128u) &&
                 (packed.raw[pos + 5] & 127u) == before.t[0].step[3].vel);
    for (k = 0; k < NTRK; k++)
        for (i = 0; i < NSTEP; i++)
            trk[k].step[i].flags &= (uint8_t)~SF_RATCH;
    project_capture(&before);
    proj_pack_vb(&packed, &before);
    ok = 1;
    for (k = 0, pos = 68u; k < PROJ_LT; k++, pos += P_COUNT + 2u + NSTEP * 9u)
        for (i = 0; i < NSTEP; i++)
            ok &= packed.raw[pos + P_COUNT + 2u + i * 9u + 5u] < 128u && packed.raw[pos + P_COUNT + 2u + i * 9u + 8u] < 128u;
    bad += check("  a project without ratchets: every velocity and chance byte below 128, as firmware before wrote", ok);
    {   /* a FUN7 image (its tracks without patches) as firmware before RATCH wrote it: x1 */
        project_store_t old;
        uint32_t sum, magic = PROJ_MAGIC_V7, size = PROJ_STORE_V7;
        project_t fun7;
        memcpy(&old, &packed, sizeof old);
        memcpy(old.raw, &magic, 4); memcpy(old.raw + 4, &size, 4);
        memset(old.raw + PROJ_STORE_V7 - 4u - PROJ_NAME_LEN, 0, PROJ_NAME_LEN);
        sum = proj_hash(old.raw, PROJ_STORE_V7 - 4u); memcpy(old.raw + PROJ_STORE_V7 - 4u, &sum, 4);
        ok = proj_import(&fun7, &old, PROJ_STORE_V7);
        for (k = 0; ok && k < PROJ_LT; k++)
            for (i = 0; i < NSTEP; i++)
                ok &= step_ratchet(&fun7.t[k].step[i]) == 1u;
        bad += check("  FUN7 of before: every step x1", ok && fun7.t[0].step[1].flags == SF_ACCENT);
    }
    {   /* FUN6 (10-byte steps): only accent and slide come over, x1 */
        project_v6_t v6;
        memset(&v6, 0, sizeof v6);
        v6.magic = PROJ_MAGIC_V6; v6.size = sizeof v6; v6.parts = NPART; v6.phys = PROJ_PHYS;
        memcpy(v6.g, before.g, sizeof v6.g);
        chain_defaults(&v6.chain);
        for (k = 0; k < PROJ_LT; k++) {
            for (i = 0; i < 69u; i++) v6.t[k].p[i] = TP[i < 61u ? i : i - 61u + P_E0].def;
            v6.t[k].step[0] = (step10_t){{60}, 1, ST_NOTE, 0xFBu, 100, 0, 0};   /* (stray high bits) */
        }
        v6.sum = proj_hash(&v6, sizeof v6 - 4u);
        bad += check("  FUN6: x1, accent and slide kept", proj_import(&after, &v6, sizeof v6) &&
                     step_ratchet(&after.t[0].step[0]) == 1u && after.t[0].step[0].flags == (SF_ACCENT | SF_SLIDE));
    }
    ui_power_on();                                   /* a real save and load */
    step_set_ratchet(&trk[1].step[7], 3);
    trk[1].step[7].time = ST_NOTE; trk[1].step[7].n = 1; trk[1].step[7].note[0] = 50;
    project_save(2);
    step_set_ratchet(&trk[1].step[7], 1);
    project_load(2);
    bad += check("  project save and load keep the ratchet", step_ratchet(&trk[1].step[7]) == 3u);
    return bad;
}

static int presets(void)
{
    int bad = 0;
    up_rec_t r;
    uint8_t a[400];
    uint32_t slot, i, k, n;
    track_t *t;
    ui_power_on();
    t = &trk[0];
    track_defaults_steps(t);
    for (i = 0; i < 16u; i++)
        t->step[i] = (step_t){{(uint8_t)(48 + i)}, 1, ST_NOTE, 0, 100};
    step_set_ratchet(&t->step[2], 3);
    t->step[5].flags = SF_ACCENT;
    step_set_ratchet(&t->step[5], 4);
    memset(&r, 0, sizeof r);
    up_pat_from(&r, t->step);
    track_defaults_steps(t);
    load_pat16(t, r.note, r.flags);
    bad += check("user preset note pattern: x3 and an accented x4 stored in its flags and loaded back",
                 step_ratchet(&t->step[2]) == 3u && step_ratchet(&t->step[5]) == 4u && t->step[5].flags & SF_ACCENT &&
                 step_ratchet(&t->step[0]) == 1u && r.flags[2] == 2u << SF_RATCH_SH);
    k = 0;                                           /* UP_PUT: slot, engine, name, values, 16 x (note, flags) */
    a[k++] = 3; a[k++] = (uint8_t)t->eng_req;
    memcpy(a + k, "RAT", 4); k += 4;
    for (i = 0; i < P_COUNT; i++) { a[k++] = (uint8_t)((t->p[i] + 8192) & 127); a[k++] = (uint8_t)((t->p[i] + 8192) >> 7); }
    for (i = 0; i < 16u; i++) { a[k++] = r.note[i]; a[k++] = r.flags[i]; }
    n = k;
    memset(&r, 0, sizeof r);
    bad += check("  UP_PUT keeps a pattern's ratchet", !up_parse(a, n, &r, &slot) && r.flags[2] == 2u << SF_RATCH_SH &&
                 r.flags[5] == (SF_ACCENT | 3u << SF_RATCH_SH));
    set_engine_of(t, ENGI_DRUM);                     /* a drum grid record: no room, x1 */
    t->engine = t->eng_req;
    track_defaults_steps(t);
    t->step[0] = (step_t){{0}, 0, ST_NOTE, 0, 100, 1u << DV_KICK};
    step_set_ratchet(&t->step[0], 2);
    up_store(4, "GRID");
    up_pat_load(t, 4);
    bad += check("  a DRUM grid record loads its hits x1", up_grid(up_rec(4)) && t->step[0].hit == 1u << DV_KICK &&
                 step_ratchet(&t->step[0]) == 1u);
    return bad;
}

/* pixels of colour c in card slot k (the cards' row) */
static uint32_t card_px(uint32_t k, uint16_t c)
{
    uint32_t n = 0;
    int32_t x, y;
    for (y = (int32_t)card_y(); y < (int32_t)(card_y() + card_h()); y++)   /* (0.8.5: the page's cards, a lens page's too) */
        for (x = CARD_X(k); x < CARD_X(k) + CARD_W; x++)
            n += swap16(host_screen[y * 240 + x]) == c;
    return n;
}
/* (0.8.5) a slide: the bar of step column i reaches into the next column's first px (its row: note n) */
static int pr_slide_reach(uint32_t i, int32_t n)
{
    return pr_is_bar((uint32_t)PR_CX(i + 1u), graph_y() + (uint32_t)pr_row_y(n) + 1u);
}

static int screens(void)
{
    int bad = 0, ok;
    track_t *t;
    uint32_t x, y;
    ui_power_on();
    t = TSEL;
    go_page(GR_CHANCE);
    ui.cursor = 0;
    turn(EN_K3, 2);
    ok = step_ratchet(&t->step[0]) == 3u && step_ratchet(&t->step[1]) == 1u;
    turn(EN_K3, 100);
    ok &= step_ratchet(&t->step[0]) == 4u;
    turn(EN_K3, -100);
    bad += check("SEQ > CHANCE KNOB 3: RATCH of the cursor step, x1..x4", ok && step_ratchet(&t->step[0]) == 1u &&
                 step_chance(&t->step[0]) == 100u);
    {                                                /* RATCH dimmed where it does nothing: REST, TIE, empty */
        uint32_t dim[4], thm[4], k;
        static const step_t kind[4] = {
            {{60}, 1, ST_NOTE, 0, 100}, {{0}, 0, ST_REST, 0, 0}, {{0}, 0, ST_TIE, 0, 0}, {{0}, 0, ST_NOTE, 0, 0}};
        for (k = 0; k < 4u; k++) {
            t->step[0] = kind[k];
            step_set_ratchet(&t->step[0], 2);
            memset(host_screen, 0, sizeof host_screen); ui.force = 1; ui_draw();
            dim[k] = card_px(2, T_DIM); thm[k] = card_px(2, T_K(2));   /* (0.8.5: a value TEXT; GARDEN, AIR: its knob's colour) */
        }
        t->step[0] = kind[3]; t->step[0].hit = 1u;   /* a NOTE step of lane hits only: it plays */
        step_set_ratchet(&t->step[0], 2);
        memset(host_screen, 0, sizeof host_screen); ui.force = 1; ui_draw();
        ok = card_px(2, T_DIM) == dim[0] && card_px(2, T_K(2)) == thm[0];
        for (k = 1; k < 4u; k++)
            ok &= dim[k] > dim[0] && thm[k] < thm[0];
        bad += check("  RATCH shows dimmed on a REST, a TIE or an empty step (a note or a hit: not)", ok);
        t->step[0] = (step_t){{0}, 0, ST_REST, 0, 0};
    }
    ui_power_on();                                   /* the piano roll: x2 in two bars, the middle column empty */
    t = TSEL;
    track_defaults_steps(t);
    t->p[P_SLEN] = 16;
    t->step[1] = (step_t){{60}, 1, ST_NOTE, 0, 100};
    t->step[3] = t->step[1];
    step_set_ratchet(&t->step[1], 2);
    go_page(GR_ROLL); ui.cursor = 8; ui.bank = 0;
    memset(host_screen, 0, sizeof host_screen); ui.force = 1; ui_draw();
    y = graph_y() + (uint32_t)pr_row_y(60) + 1u;
    x = (uint32_t)PR_CX(1);                          /* (0.8.5: columns 14 / 15 px, the bar a px in: x2 at x + 1 .. 6, 8 .. 13) */
    ok = pr_is_bar(x + 2u, y) && pr_is_bar(x + 10u, y) && !pr_is_bar(x + 7u, y);
    x = (uint32_t)PR_CX(3);
    bad += check("  the piano roll draws an x2 step in two parts (x1 whole)", ok && pr_is_bar(x + 7u, y));
    t->step[4] = (step_t){{64}, 1, ST_NOTE, 0, 100};   /* SLIDE into a note: the line, but not from x2 */
    t->step[3].flags = SF_SLIDE;
    t->step[1].flags |= SF_SLIDE;                     /* (x2 kept) */
    t->step[2] = t->step[4];
    memset(host_screen, 0, sizeof host_screen); ui.force = 1; ui_draw();
    bad += check("  an x1 SLIDE step's bar reaches into the next column, an x2 one's not", pr_slide_reach(3, 60) &&
                 !pr_slide_reach(1, 60));
    t->step[1].flags &= (uint8_t)~SF_SLIDE; t->step[3].flags = 0;
    t->step[2] = t->step[4] = (step_t){{0}, 0, ST_REST, 0, 0};
    set_engine_of(t, ENGI_DRUM);                     /* the grid: x3 in three bars */
    t->engine = t->eng_req;
    t->step[1] = (step_t){{0}, 0, ST_NOTE, 0, 100, 1u};
    step_set_ratchet(&t->step[1], 3);
    ui.lane = 1;
    memset(host_screen, 0, sizeof host_screen); ui.force = 1; ui_draw();
    y = graph_y() + 19u + 2u + 5u;                   /* lane 0's hits: y 21 .. 30 of the panel (0.8.5: under LENTE's bars) */
    x = 40u + 12u;
    bad += check("  the DRUM grid draws an x3 hit in three parts", pr_is_bar(x, y) && !pr_is_bar(x + 2u, y) &&
                 pr_is_bar(x + 3u, y) && !pr_is_bar(x + 5u, y) && pr_is_bar(x + 6u, y));
    return bad;
}

/* (0.6.39) the step components (core.h CP_*, seq.c cp_arp_note / cp_trig_ok, SEQ > COMP): the notes the voices start
 * over `blocks` blocks, in order */
static uint32_t run_notes(track_t *t, uint32_t blocks, uint8_t *nt, uint32_t max)
{
    uint32_t b, n = 0, a, k;
    for (b = 0; b < blocks; b++) {
        uint32_t v0 = vage;
        seq_tick(t, CTL);
        for (a = v0 + 1u; a <= vage; a++)
            for (k = 0; k < NVOICE; k++)
                if (t->v[k].age == a && n < max) nt[n++] = t->v[k].note;
    }
    return n;
}
static int components(void)
{
    int bad = 0, ok;
    uint8_t nt[32];
    uint32_t n;
    track_t *t;
    project_store_t st;
    /* (0.9.18) ARP: the step's notes one at a time at its RATE (a note value), in its ORDER, over RANGE octaves (one
     * note: itself and RANGE octaves up), the step and its TIEs at least once through, spilling over the steps after */
    {
        uint32_t at[16];
        uint8_t vl[16];
        t = spill_trk(1, 0);                              /* (DIV 1/16: a step a sixteenth) */
        t->p[P_VOICE] = V_POLY;
        t->step[0] = (step_t){{67, 60, 64}, 3, ST_NOTE, 0, 100};
        t->step[0].comp = CP_ARPN; t->step[0].carg = CP_ARG(0u, 5u, 0u);   /* UP 1/16 */
        seq_start();
        n = run_nv(t, 8, at, nt, vl, 16);
        ok = n == 3u && nt[0] == 60u && nt[1] == 64u && nt[2] == 67u && at[1] >= step_len(t, 0) - CTL &&
             at[1] <= step_len(t, 0) + CTL && at[2] >= 2u * step_len(t, 0) - CTL;
        bad += check("ARP UP 1/16 over a chord: 60 64 67 a step each, spilling over the empty steps", ok);
        t = spill_trk(1, 0);
        t->p[P_SLEN] = 16;
        t->p[P_VOICE] = V_POLY;
        for (n = 8; n < 16u; n++) t->step[n] = (step_t){{0}, 0, ST_REST};
        t->step[0] = (step_t){{60, 64, 67}, 3, ST_NOTE, 0, 100};
        t->step[0].comp = CP_ARPN; t->step[0].carg = CP_ARG(1u, 6u, 1u);   /* DOWN 1/8, two octaves */
        seq_start();
        n = run_nv(t, 16, at, nt, 0, 16);
        ok = n == 6u && nt[0] == 79u && nt[1] == 76u && nt[2] == 72u && nt[5] == 60u &&
             at[1] >= 2u * step_len(t, 0) - CTL && at[1] <= 2u * step_len(t, 0) + CTL;
        bad += check("ARP DOWN 1/8 over two octaves: 79 76 72 67 64 60, two steps apart", ok);
        t = spill_trk(1, 0);
        t->step[0].comp = CP_ARPN; t->step[0].carg = CP_ARG(0u, 5u, 1u);   /* one note, UP 2 OCT */
        seq_start();
        n = run_nv(t, 8, 0, nt, 0, 16);
        bad += check("ARP of one note, 2 OCT: itself and two octaves up (60 72 84)", n == 3u && nt[0] == 60u &&
                     nt[1] == 72u && nt[2] == 84u);
        t = spill_trk(1, 0);
        t->p[P_VOICE] = V_POLY;
        t->step[0] = (step_t){{60, 64, 67}, 3, ST_NOTE, 0, 100};
        t->step[0].comp = CP_ARPN; t->step[0].carg = CP_ARG(0u, 5u, 0u);
        for (n = 1; n < 5u; n++) t->step[n] = (step_t){{0}, 0, ST_TIE};
        seq_start();
        n = run_nv(t, 8, 0, nt, 0, 16);
        bad += check("  over 4 TIEs: on through the step and its TIEs (60 64 67 60 64)", n == 5u && nt[3] == 60u &&
                     nt[4] == 64u);
        t = spill_trk(1, 0);
        t->p[P_SLEN] = 4;
        t->p[P_VOICE] = V_POLY;
        t->step[0] = (step_t){{60, 64, 67}, 3, ST_NOTE, 0, 100};
        t->step[0].comp = CP_ARPN; t->step[0].carg = CP_ARG(0u, 5u, 0u);
        ST_EVERY_SET(&t->step[0], EV_1_2);
        seq_start();
        n = run_nv(t, 8, at, nt, 0, 16);                  /* (lap 1 the arpeggio, lap 2 the chord at once) */
        ok = n == 6u && at[1] > at[0] + CTL && at[3] == at[4] && at[4] == at[5];
        bad += check("  ARP 1:2: the arpeggio on the odd laps, the chord as written on the others", ok);
        t = spill_trk(1, 0);
        t->p[P_VOICE] = V_MONO;
        t->step[0] = (step_t){{60, 64, 67}, 3, ST_NOTE, 0, 100};
        t->step[0].comp = CP_ARPN; t->step[0].carg = CP_ARG(0u, 6u, 0u);   /* 1/8: 60 64 67 on 0 2 4 */
        t->step[3] = (step_t){{50}, 1, ST_NOTE, 0, 100};
        seq_start();
        n = run_nv(t, 8, 0, nt, 0, 16);
        bad += check("  MONO: a note after it cuts the rest (60 64, then 50)", n == 3u && nt[0] == 60u && nt[1] == 64u &&
                     nt[2] == 50u);
    }
    /* SEQ: the pattern's melody in its order (67 60 64; NOTES would sort it), a note of it a lap */
    t = one_step(1, 0);
    t->p[P_SLEN] = 3;
    t->step[0] = (step_t){{67}, 1, ST_NOTE, 0, 100};
    t->step[0].comp = CP_ARPQ; t->step[0].carg = CP_ARG(0u, 2u, 0u);
    t->step[1] = (step_t){{60}, 1, ST_NOTE, 0, 100};
    t->step[2] = (step_t){{64}, 1, ST_NOTE, 0, 100};
    seq_start();
    n = run_notes(t, 4u * 3u * step_len(t, 0) / CTL - 1u, nt, sizeof nt);
    bad += check("ARP SEQ: the melody a lap at a time: 67 60 64 | 60 60 64 | 64 60 64 | 67 60 64", n == 12u &&
                 nt[0] == 67u && nt[1] == 60u && nt[2] == 64u && nt[3] == 60u && nt[6] == 64u && nt[9] == 67u &&
                 nt[10] == 60u && nt[11] == 64u);
    /* TRIG: the loops it plays on (a one-step loop: a loop a pass) */
    {
        static const uint8_t COND[4] = {0, 1, 10, 11}, WANT[4] = {2, 2, 1, 3};   /* 1:2 2:2 FIRST NOT FIRST, 4 loops */
        uint32_t k;
        ok = 1;
        for (k = 0; k < 4u; k++) {
            t = one_step(1, 0);
            t->step[0].comp = CP_TRIG; t->step[0].carg = COND[k];
            n = run_notes(t, passes(t, 4), nt, sizeof nt);
            ok &= n == WANT[k];
        }
        bad += check("TRIG over 4 loops: 1:2 twice, 2:2 twice, FIRST once, NOT FIRST three times", ok);
    }
    /* a project keeps them (FUNC: x bit 16, two bytes), an old step none */
    ui_power_on();
    trk[0].step[3] = (step_t){{60, 64}, 2, ST_NOTE, 0, 100};
    trk[0].step[3].comp = CP_ARPN; trk[0].step[3].carg = CP_ARG(2u, 6u, 2u);
    trk[1].step[0] = (step_t){{50}, 1, ST_NOTE, 0, 100};
    trk[1].step[0].comp = CP_TRIG; trk[1].step[0].carg = 7;
    project_capture(&proj_scratch);
    ok = proj_pack(&st, &proj_scratch);
    trk[0].step[3].comp = trk[1].step[0].comp = 0;
    ok &= proj_import(&proj_scratch, &st, sizeof st) && !project_restore_runtime(&proj_scratch);
    ok &= trk[0].step[3].comp == CP_ARPN && trk[0].step[3].carg == CP_ARG(2u, 6u, 2u) && trk[1].step[0].comp == CP_TRIG &&
          trk[1].step[0].carg == 7u && !trk[0].step[2].comp;
    bad += check("a project keeps the components (FUNC round trip)", ok);
    /* SEQ > COMP: K2 the TYPE, K3 / K4 its values; OFF / CHANCE / RATCH take a component away */
    ui_power_on();
    t = TSEL;
    t->step[0] = (step_t){{60, 64, 67}, 3, ST_NOTE, 0, 100};
    go_page(GR_COMP); frame();
    ok = str_eq(cur_page()->title, "COMP") && comp_view() == COMP_OFF;
    turn(EN_K2, 3);                                       /* OFF CHANCE RATCH ARP */
    ok &= t->step[0].comp == CP_ARPN && t->step[0].carg == CP_ARG(0u, 5u, 0u);
    turn(EN_K3, 4); turn(EN_K3, 1); turn(EN_K4, 1);       /* (0.6.92) KNOB 3 ORDER x RANGE: DOWN over two octaves; (0.9.18)
                                                           * KNOB 4 RATE x the laps: 1/16 -> 1/32 */
    frame();
    ok &= CP_ORD(t->step[0].carg) == 1u && CP_RATE(t->step[0].carg) == 0u && CP_RNG(t->step[0].carg) == 1u &&
          !strncmp(ui.col[3], "RATE|1/32", 9);         /* (the label as it plays: 1/32) */
    turn(EN_K4, 1); turn(EN_K4, 1); frame();           /* (1/32 1/16 1/8: a detent a value) */
    ok &= CP_RATE(t->step[0].carg) == 6u && !strncmp(ui.col[3], "RATE|1/8", 8);
    turn(EN_K4, -1); turn(EN_K4, -1);
    turn(EN_K4, 4);                                       /* (.. 1/4 1:2: the step's EVERY) */
    ok &= CP_RATE(t->step[0].carg) == 7u && ST_EVERY(&t->step[0]) == EV_1_2;
    turn(EN_K4, -4);
    ok &= ST_EVERY(&t->step[0]) == EV_ALWAYS;
    turn(EN_K2, 1);                                       /* SEQ: the values kept */
    ok &= t->step[0].comp == CP_ARPQ && CP_ORD(t->step[0].carg) == 1u;
    turn(EN_K2, 4); turn(EN_K2, 4); turn(EN_K2, 1); turn(EN_K3, 4); turn(EN_K3, 1);   /* TRIG 1:2 .. 2:4 */
    ok &= t->step[0].comp == CP_TRIG && t->step[0].carg == 5u;
    turn(EN_K2, -4); turn(EN_K2, -4); turn(EN_K2, -4); turn(EN_K3, -40);   /* CHANCE 60 %: the TRIG gone */
    ok &= !t->step[0].comp && step_chance(&t->step[0]) == 60u && comp_view() == COMP_CHANCE;
    turn(EN_K1, 1); frame();
    ok &= ui.cursor == 1u && comp_view() == COMP_OFF;
    bad += check("SEQ > COMP: TYPE, ORDER x RANGE, RATE x laps, TRIG's condition, CHANCE takes the component away", ok);
    return bad;
}

/* (0.6.42) the second set: a pattern of LEN len with the notes nts (0: a REST), run for `steps` steps */
static track_t *pat(uint32_t len, const uint8_t *nts)
{
    track_t *t = one_step(1, 0);
    uint32_t i;
    t->p[P_SLEN] = (int16_t)len;
    t->p[P_ROOT] = 0; t->p[P_SCALE] = 1;                  /* C major */
    for (i = 0; i < len; i++) {
        t->step[i] = nts[i] ? (step_t){{nts[i]}, 1, ST_NOTE, 0, 100} : (step_t){{0}, 0, ST_REST, 0, 0};
    }
    seq_start();
    return t;
}
static uint32_t run_steps(track_t *t, uint32_t steps, uint8_t *nt, uint32_t max)
{
    return run_notes(t, steps * step_len(t, 0) / CTL - 1u, nt, max);
}
static int components2(void)
{
    int bad = 0, ok;
    uint8_t nt[64];
    uint32_t n, i, k;
    track_t *t;
    {   /* RAND 1 degree: 59 60 62 only, more than one of them */
        static const uint8_t P1[1] = {60};
        uint32_t seen = 0;
        t = pat(1, P1); t->step[0].comp = CP_RAND; t->step[0].carg = 1;
        n = run_steps(t, 40, nt, sizeof nt);
        ok = n == 40u;
        for (i = 0; i < n; i++) { ok &= nt[i] == 59u || nt[i] == 60u || nt[i] == 62u; seen |= 1u << (nt[i] - 59u); }
        bad += check("RAND 1: the note a degree up / down at random (C major: 59 60 62)", ok && seen != 1u && seen != 2u && seen != 8u);
    }
    {   /* (0.9.18) OCT (CP_CLIMB): an octave a lap, up to its range, then round: UP 2 OCT 60 72 84 60 ..; DOWN 1 */
        static const uint8_t P1[1] = {60};
        t = pat(1, P1); t->step[0].comp = CP_CLIMB; t->step[0].carg = CP_WARG(0u, 0u, 1u);
        n = run_steps(t, 4, nt, sizeof nt);
        ok = n == 4u && nt[0] == 60u && nt[1] == 72u && nt[2] == 84u && nt[3] == 60u;
        t = pat(1, P1); t->step[0].comp = CP_CLIMB; t->step[0].carg = CP_WARG(0u, 1u, 0u);
        n = run_steps(t, 4, nt, sizeof nt);
        ok &= n == 4u && nt[0] == 60u && nt[1] == 48u && nt[2] == 60u && nt[3] == 48u;
        bad += check("OCT: an octave a lap up to its range and round (60 72 84 60); DOWN 1 (60 48 60 48)", ok);
    }
    {   /* (0.6.98) WALK UP 3 degrees every loop: 60 62 64 65 60 .. (C major); DOWN; UP-DN 3 every 2 loops: 60 60 62 62
         * 64 64 65 65 64 64 62 62 60 ..; CLIMB UP 1 octave every loop: 60 72 60 72; RND: inside the range, the same
         * each time round; WALK and CLIMB stacked (COMP, COMP 2) add up; a kit ignores them */
        static const uint8_t P1[1] = {60};
        t = pat(1, P1); t->step[0].comp = CP_WALK; t->step[0].carg = CP_WARG(0u, 0u, 0u);
        n = run_steps(t, 9, nt, sizeof nt);
        ok = n == 9u && nt[0] == 60u && nt[1] == 62u && nt[2] == 64u && nt[3] == 65u && nt[4] == 60u && nt[8] == 60u;
        t = pat(1, P1); t->step[0].comp = CP_WALK; t->step[0].carg = CP_WARG(0u, 1u, 0u);
        n = run_steps(t, 4, nt, sizeof nt);
        ok &= n == 4u && nt[0] == 60u && nt[1] == 59u && nt[2] == 57u && nt[3] == 55u;
        t = pat(1, P1); t->step[0].comp = CP_WALK; t->step[0].carg = CP_WARG(1u, 2u, 0u);
        n = run_steps(t, 14, nt, sizeof nt);
        ok &= n == 14u && nt[0] == 60u && nt[1] == 60u && nt[2] == 62u && nt[3] == 62u && nt[6] == 65u && nt[7] == 65u && nt[8] == 64u &&
              nt[10] == 62u && nt[12] == 60u;
        if (!ok) printf("  walk: %u %u %u %u %u %u %u %u %u %u %u %u %u %u\n", nt[0], nt[1], nt[2], nt[3], nt[4], nt[5], nt[6], nt[7], nt[8], nt[9], nt[10], nt[11], nt[12], nt[13]);
        bad += check("WALK: UP 3 degrees a loop and round, DOWN, UP-DN 3 every 2 loops (there and back)", ok);
        t = pat(1, P1); t->step[0].comp = CP_CLIMB; t->step[0].carg = CP_WARG(0u, 0u, 0u);
        n = run_steps(t, 4, nt, sizeof nt);
        ok = n == 4u && nt[0] == 60u && nt[1] == 72u && nt[2] == 60u && nt[3] == 72u;
        t = pat(1, P1); t->step[0].comp = CP_WALK; t->step[0].carg = CP_WARG(0u, 3u, 1u);
        n = run_steps(t, 12, nt, sizeof nt);
        ok &= n == 12u;
        for (i = 0; i < 12u; i++) ok &= nt[i] >= 60u && nt[i] <= 69u;
        { uint32_t same = 1; for (i = 1; i < 12u; i++) same &= nt[i] == nt[0]; ok &= !same; }
        if (!ok) printf("  climb / rnd: %u %u %u %u %u %u\n", nt[0], nt[1], nt[2], nt[3], nt[4], nt[5]);
        t = pat(1, P1); t->step[0].comp = CP_WALK; t->step[0].carg = CP_WARG(0u, 0u, 0u);
        t->step[0].comp2 = CP_CLIMB; t->step[0].carg2 = CP_WARG(1u, 0u, 0u);
        n = run_steps(t, 4, nt, sizeof nt);
        ok &= n == 4u && nt[0] == 60u && nt[1] == 62u && nt[2] == 76u && nt[3] == 77u;
        if (!ok) printf("  stacked: %u %u %u %u\n", nt[0], nt[1], nt[2], nt[3]);
        bad += check("OCT (CLIMB): an octave a loop and back; WALK RND inside its range, moving; WALK + OCT stacked add up", ok);
    }
    {   /* (0.6.52) FUGUE: head 2 at x2, +2 degrees: two notes a step, a third up the scale; head 2 REV x1: the pattern
         * backwards beside it; OFF: as before */
        static const uint8_t P4[4] = {60, 62, 64, 65};
        uint32_t c67 = 0, c69 = 0;
        t = pat(4, P4);
        t->p[P_VOICE] = V_POLY;                           /* (the heads beside the sequencer: voices of their own) */
        fg_set[0][0] = (fg_set_t){7, 0, 2};
        seq_start();
        n = run_steps(t, 4, nt, sizeof nt);
        for (i = 0; i < n; i++) { c67 += nt[i] == 67u; c69 += nt[i] == 69u; }
        ok = n == 11u && c67 == 2u && c69 == 2u;       /* (12 notes: on step 3 both play 64, one voice) */
        t = pat(4, P4);
        t->p[P_VOICE] = V_POLY;
        fg_set[0][0] = (fg_set_t){5, 1, 0};
        seq_start();
        n = run_steps(t, 4, nt, sizeof nt);
        ok &= n == 8u && nt[0] == 60u && nt[1] == 65u && nt[2] == 62u && nt[3] == 64u && nt[4] == 64u && nt[5] == 62u &&
              nt[6] == 65u && nt[7] == 60u;
        memset(fg_set, 0, sizeof fg_set);
        t = pat(4, P4);
        n = run_steps(t, 4, nt, sizeof nt);
        ok &= n == 4u;
        seq_stop();
        bad += check("FUGUE: a head x2 +2 degrees plays twice as many a third up; REV the pattern backwards; OFF as before", ok);
    }
    {   /* TONAL +2 on step 1: it and the rest of the loop two degrees up, the next loop from its start again */
        static const uint8_t P2[3] = {60, 64, 67};
        t = pat(3, P2); t->step[1].comp = CP_TONAL; t->step[1].carg = 9;
        n = run_steps(t, 6, nt, sizeof nt);
        bad += check("TONAL +2 on step 2: 60 67 71 | 60 67 71", n == 6u && nt[0] == 60u && nt[1] == 67u && nt[2] == 71u &&
                     nt[3] == 60u && nt[4] == 67u && nt[5] == 71u);
    }
    {   /* BEND +12: the voice from 60 towards 72 over the step */
        static const uint8_t P1[1] = {60};
        t = pat(1, P1); t->step[0].comp = CP_BEND; t->step[0].carg = 24;
        run_notes(t, 2, nt, sizeof nt);
        ok = 0;
        for (k = 0; k < NVOICE; k++)
            if (t->v[k].active && t->v[k].note == 60u)
                ok = t->v[k].pitch16 == 72 * 16 && t->v[k].pitch_cur < 72 * 16 && t->v[k].gstep > 0;
        bad += check("BEND +12: the note glides to 72 over the step", ok);
    }
    {   /* ECHO x3 (0.9.18: RATCH x3 FADE): three note-ons, each softer, spilling over the empty steps after it */
        static const uint8_t P4[4] = {60, 0, 0, 0};
        uint32_t b, v0, vel[8], nv = 0;
        t = pat(4, P4); t->step[0].comp = CP_ECHO; t->step[0].carg = 1;
        for (b = 0; b + 1u < 4u * step_len(t, 0) / CTL; b++) {
            v0 = vage;
            seq_tick(t, CTL);
            for (k = 0; k < NVOICE; k++)
                if (t->v[k].age > v0 && nv < 8u) vel[nv++] = t->v[k].vel;
        }
        bad += check("ECHO x3: three, each softer", nv == 3u && vel[0] > vel[1] && vel[1] > vel[2]);
    }
    {   /* PULSE: on every empty step after it; every 2 */
        static const uint8_t P4[4] = {60, 0, 0, 0};
        t = pat(4, P4); t->step[0].comp = CP_PULSE; t->step[0].carg = 0;
        n = run_steps(t, 4, nt, sizeof nt);
        ok = n == 4u && nt[0] == 60u && nt[3] == 60u;
        t = pat(4, P4); t->step[0].comp = CP_PULSE; t->step[0].carg = 1;
        n = run_steps(t, 4, nt, sizeof nt);
        bad += check("PULSE: the note again on each empty step after it; EVERY 2: every other", ok && n == 2u);
    }
    {   /* JUMP: step 2 then step 1 again; REV on step 3: 1 2 3 2 | 1 2 3 2 */
        static const uint8_t P4[4] = {60, 62, 64, 65};
        t = pat(4, P4); t->step[1].comp = CP_JUMP; t->step[1].carg = 0;
        n = run_steps(t, 4, nt, sizeof nt);
        ok = n == 4u && nt[0] == 60u && nt[1] == 62u && nt[2] == 60u && nt[3] == 62u;
        t = pat(4, P4); t->step[2].comp = CP_REV;
        n = run_steps(t, 8, nt, sizeof nt);
        ok &= n == 8u && nt[0] == 60u && nt[1] == 62u && nt[2] == 64u && nt[3] == 62u && nt[4] == 60u && nt[6] == 64u;
        bad += check("JUMP to step 1 from step 2: 60 62 60 62; REV on step 3: 60 62 64 62 60 62 64 ..", ok);
    }
    {   /* (0.6.62) TIMES: JUMP x1 on step 2 to step 1: 60 62 60 62 64 65 | 60 62 60 62 64 65; x2: two turns; 1:2: once,
         * every other pass; REV x1 on step 3: 60 62 64 62 60 62 64 65 | (the pass over) */
        static const uint8_t P4[4] = {60, 62, 64, 65};
        static const uint8_t X1[12] = {60, 62, 60, 62, 64, 65, 60, 62, 60, 62, 64, 65};
        static const uint8_t X2[8] = {60, 62, 60, 62, 60, 62, 64, 65};
        static const uint8_t AL[10] = {60, 62, 60, 62, 64, 65, 60, 62, 64, 65};
        static const uint8_t RV[9] = {60, 62, 64, 62, 60, 62, 64, 65, 60};
        uint32_t i;
        t = pat(4, P4); t->step[1].comp = CP_JUMP; t->step[1].carg = 0 | 1u << 6;
        n = run_steps(t, 12, nt, sizeof nt);
        ok = n == 12u;
        for (i = 0; i < 12u; i++) ok &= nt[i] == X1[i];
        t = pat(4, P4); t->step[1].comp = CP_JUMP; t->step[1].carg = 0 | 2u << 6;
        n = run_steps(t, 8, nt, sizeof nt);
        ok &= n == 8u;
        for (i = 0; i < 8u; i++) ok &= nt[i] == X2[i];
        t = pat(4, P4); t->step[1].comp = CP_JUMP; t->step[1].carg = 0 | 3u << 6;
        n = run_steps(t, 10, nt, sizeof nt);
        ok &= n == 10u;
        for (i = 0; i < 10u; i++) ok &= nt[i] == AL[i];
        t = pat(4, P4); t->step[2].comp = CP_REV; t->step[2].carg = 1;
        n = run_steps(t, 9, nt, sizeof nt);
        ok &= n == 9u;
        for (i = 0; i < 9u; i++) ok &= nt[i] == RV[i];
        bad += check("TIMES: JUMP x1 / x2 turn the track once / twice a pass, 1:2 once every other pass; REV x1 once", ok);
    }
    {   /* FILL: silent unless FX is held */
        static const uint8_t P1[1] = {60};
        t = pat(1, P1); t->step[0].comp = CP_FILL;
        seq_fill = 0;
        n = run_steps(t, 2, nt, sizeof nt);
        ok = n == 0u;
        seq_fill = 1;
        n = run_steps(t, 2, nt, sizeof nt);
        seq_fill = 0;
        bad += check("FILL: only while FX is held", ok && n == 2u);
    }
    {   /* KILL: the long note before it gone, its own on */
        static const uint8_t P2[2] = {48, 67};
        uint32_t old = 0, mine = 0;
        int32_t o[2u * CTL];
        t = pat(2, P2); t->p[P_REL] = 127; t->p[P_SGATE] = 30; t->step[1].comp = CP_KILL;
        for (k = 1; k < NTRK; k++) trk[k].p[P_MUTE] = 1;
        for (i = 0; i < step_len(t, 0) / CTL + 30u; i++) mix_block(o, CTL);   /* (step 2 and its fade) */
        for (k = 0; k < NVOICE; k++) { old |= t->v[k].active && t->v[k].note == 48u; mine |= t->v[k].active && t->v[k].note == 67u; }
        bad += check("KILL: what sounded (a long release) cut, its own note on", !old && mine);
    }
    {   /* SEQ > COMP: KNOB 2 to the new TYPES, KNOB 3 their value (JUMP: any step) */
        ui_power_on();
        t = TSEL;
        t->step[0] = (step_t){{60}, 1, ST_NOTE, 0, 100};
        go_page(GR_COMP); frame();
        turn(EN_K2, 4); turn(EN_K2, 4);                   /* .. RAND (0.9.18: the list by family) */
        ok = t->step[0].comp == CP_RAND && t->step[0].carg == 2u;
        turn(EN_K2, 3);                                   /* .. JUMP */
        turn(EN_K3, 9);
        ok &= t->step[0].comp == CP_JUMP && t->step[0].carg == 9u;
        turn(EN_K4, 1); frame();                          /* (0.6.92) EVERY 1:2, the step kept */
        ok &= ST_EVERY(&t->step[0]) == EV_1_2 && CP_JTO(t->step[0].carg) == 9u && strstr(ui.col[3], "1:2") != 0;
        turn(EN_K3, -2);
        ok &= ST_EVERY(&t->step[0]) == EV_1_2 && CP_JTO(t->step[0].carg) == 7u;
        turn(EN_K2, 4);                                   /* KILL */
        ok &= t->step[0].comp == CP_KILL && comp_view() == COMP_KILL;
        bad += check("SEQ > COMP: RAND .. KILL on KNOB 2, JUMP's step on KNOB 3, EVERY on KNOB 4", ok);
    }
    return bad;
}


/* (0.6.82) two components a step, WHEN, DIV, LEN */
static uint32_t onsets(track_t *t, uint32_t *at, uint32_t max)   /* one step: the blocks its note-ons start on */
{
    uint32_t b, n = 0, k;
    for (b = 0; b + 1u < step_len(t, 0) / CTL; b++) {
        uint32_t v0 = vage;
        seq_tick(t, CTL);
        for (k = 0; k < NVOICE; k++)
            if (t->v[k].age > v0 && n < max) { at[n++] = b; break; }
    }
    return n;
}
static int components3(void)
{
    int bad = 0, ok;
    uint8_t nt[64];
    uint32_t n, at[8], nb;
    track_t *t;
    project_store_t st;
    static const uint8_t P1[1] = {60};
    /* WHEN: TONAL +7 (an octave of C major) on odd loops (1:2): 72 60 72 60; on even (2:2): 60 72 ..; SKIP: never */
    t = pat(1, P1); t->step[0].comp = CP_TONAL; t->step[0].carg = 14; ST_EVERY_SET(&t->step[0], EV_1_2);
    n = run_steps(t, 4, nt, sizeof nt);
    ok = n == 4u && nt[0] == 72u && nt[1] == 60u && nt[2] == 72u && nt[3] == 60u;
    t = pat(1, P1); t->step[0].comp = CP_TONAL; t->step[0].carg = 14; ST_EVERY_SET(&t->step[0], EV_2_2);
    n = run_steps(t, 4, nt, sizeof nt);
    ok &= n == 4u && nt[0] == 60u && nt[1] == 72u && nt[2] == 60u && nt[3] == 72u;
    t = pat(1, P1); t->step[0].comp = CP_TONAL; t->step[0].carg = 14; ST_EVERY_SET(&t->step[0], EV_SKIP);
    n = run_steps(t, 3, nt, sizeof nt);
    ok &= n == 3u && nt[0] == 60u && nt[1] == 60u && nt[2] == 60u;
    bad += check("EVERY: TONAL on 1:2 loops, on 2:2 loops, SKIP never", ok);
    /* two components: ECHO x2 and TONAL +7: both, two hits an octave up; EVERY the step's: both held back */
    {
        static const uint8_t P4[4] = {60, 0, 0, 0};
        t = pat(4, P4); t->step[0].comp = CP_ECHO; t->step[0].carg = 0; t->step[0].comp2 = CP_TONAL; t->step[0].carg2 = 14;
        n = run_steps(t, 4, nt, sizeof nt);
        ok = n == 2u && nt[0] == 72u && nt[1] == 72u;
        t = pat(4, P4); t->step[0].comp = CP_ECHO; t->step[0].carg = 0; t->step[0].comp2 = CP_TONAL; t->step[0].carg2 = 14;
        ST_EVERY_SET(&t->step[0], EV_2_2);                /* (0.6.92: EVERY is the step's: both on the even loops) */
        n = run_steps(t, 8, nt, sizeof nt);
        ok &= n == 3u && nt[0] == 60u && nt[1] == 72u && nt[2] == 72u;
        bad += check("two components a step: ECHO x2 + TONAL together; EVERY 2:2 holds both back on the odd loops", ok);
    }
    /* the one that silences wins: TONAL and TRIG FIRST: the octave on the first loop, then nothing */
    t = pat(1, P1); t->step[0].comp = CP_TONAL; t->step[0].carg = 14; t->step[0].comp2 = CP_TRIG; t->step[0].carg2 = 10;
    n = run_steps(t, 3, nt, sizeof nt);
    ok = n == 1u && nt[0] == 72u;
    ST_EVERY_SET(&t->step[0], EV_SKIP);                   /* (both skipped: it plays, as written) */
    seq_start();
    n = run_steps(t, 3, nt, sizeof nt);
    ok &= n == 3u && nt[0] == 60u;
    bad += check("a TRIG beside a TONAL: the TRIG keeps it silent (FIRST: one loop only); EVERY SKIP: every loop, as written", ok);
    /* DIV (0.9.18, spilling): ECHO x3 at 1/4 a step x 4 apart; shared out: 4/3 of a step; RATCH x4 at 1/2 two steps
     * apart; ECHO x8 half a step apart */
    nb = step_len(pat(1, P1), 0);
    {
        uint32_t a2[16];
        t = spill_trk(1, 0); t->step[0].comp = CP_ECHO; t->step[0].carg = 1; t->step[0].ctime = 3;
        n = run_nv(t, 8, a2, 0, 0, 16);
        ok = n == 3u && a2[1] + CTL >= nb && a2[1] <= nb + CTL && a2[2] + CTL >= 2u * nb && a2[2] <= 2u * nb + CTL;
        t = spill_trk(1, 0); t->step[0].comp = CP_ECHO; t->step[0].carg = 1;
        n = run_nv(t, 8, a2, 0, 0, 16);
        ok &= n == 3u && a2[1] + CTL >= nb * 4u / 3u && a2[1] <= nb * 4u / 3u + CTL;
        t = spill_trk(4, 0); t->step[0].ctime = 1;
        n = run_nv(t, 8, a2, 0, 0, 16);
        ok &= n == 4u && a2[3] + CTL >= 6u * nb && a2[3] <= 6u * nb + CTL;
        t = spill_trk(1, 0); t->step[0].comp = CP_ECHO; t->step[0].carg = 6;   /* (x8) */
        n = run_nv(t, 8, a2, 0, 0, 16);
        ok &= n == 8u && a2[7] + CTL >= nb * 7u / 2u && a2[7] <= nb * 7u / 2u + CTL;
        bad += check("DIV: ECHO x3 at 1/4 a step apart (x 4); shared out 4/3 of a step; RATCH x4 at 1/2 two steps; ECHO x8", ok);
    }
    nb = step_len(pat(1, P1), 0) / CTL;
    /* LEN 4/16: the note off by half the step (the track's GATE holds it longer) */
    t = pat(1, P1); t->p[P_SGATE] = 120; t->step[0].ctime = 4u << 3;
    run_notes(t, nb / 2u, nt, sizeof nt);
    ok = t->seq_n == 0u;
    t = pat(1, P1); t->p[P_SGATE] = 120;
    run_notes(t, nb / 2u, nt, sizeof nt);
    ok &= t->seq_n == 1u;
    bad += check("LEN 4/16: the note's gate a quarter step (GATE 120 holds it on)", ok);
    /* a project keeps them (FUNC) */
    t = pat(1, P1); t->step[0].comp2 = CP_REV; t->step[0].carg2 = 2; t->step[0].cwhen = 0xBu; t->step[0].ctime = 0x5Cu;
    project_capture(&proj_scratch);
    ok = proj_pack(&st, &proj_scratch);
    t->step[0].comp2 = t->step[0].carg2 = t->step[0].cwhen = t->step[0].ctime = 0;
    ok &= proj_import(&proj_scratch, &st, sizeof st) && !project_restore_runtime(&proj_scratch);
    ok &= t->step[0].comp2 == CP_REV && t->step[0].carg2 == 2u && t->step[0].cwhen == 0xBu && t->step[0].ctime == 0x5Cu;
    bad += check("a project keeps the second component, WHEN, DIV and LEN (FUNC round trip)", ok);
    /* on a drum kit: OCT tunes the hit an octave up a lap; ARP over BD + SD plays them in turn; BEND bends it */
    {
        uint32_t kick = DRUM_LANE_NOTE[DV_KICK], snr = DRUM_LANE_NOTE[DV_SNARE];
        int32_t base = DRUM_GM[drum_gm_ix(kick)][1];
        t = one_step(1, 0);
        set_engine_of(t, ENGI_DRUM);
        t->engine = t->eng_req;
        t->step[0] = (step_t){{0}, 0, ST_NOTE, 0, 100, 1u << DV_KICK, 0};
        t->step[0].comp = CP_CLIMB; t->step[0].carg = CP_WARG(0u, 0u, 0u);
        seq_start();
        n = run_notes(t, passes(t, 1) + 3u, nt, sizeof nt);   /* (the second lap) */
        ok = drum_kit[0][DV_KICK].st == base + 12 && t->dpit == 0;
        if (!ok) printf("  kit oct: n %u loops %u st %d len %d sl %u pos %u idx %u\n", n, t->loops, drum_kit[0][DV_KICK].st, t->p[P_SLEN], step_len(t, 0), t->seq_pos, t->seq_idx);
        t->step[0].comp = CP_BEND; t->step[0].carg = 0;       /* (-12) */
        seq_start();
        run_notes(t, 2, nt, sizeof nt);
        ok &= drum_kit[0][DV_KICK].st == base && drum_kit[0][DV_KICK].bd == -12 * 16 && drum_kit[0][DV_KICK].bs > 0;
        t->p[P_SLEN] = 4;
        t->step[0] = (step_t){{0}, 0, ST_NOTE, 0, 100, 1u << DV_KICK | 1u << DV_SNARE, 0};
        t->step[0].comp = CP_ARPN; t->step[0].carg = CP_ARG(0u, 5u, 0u);   /* (1/16: a sound a step) */
        for (n = 1; n < 4u; n++) t->step[n] = (step_t){{0}, 0, ST_REST};
        seq_start();
        n = run_notes(t, 8u * step_len(t, 0) / CTL - 1u, nt, sizeof nt);
        ok &= n == 4u && nt[0] == kick && nt[1] == snr && nt[2] == kick && nt[3] == snr;
        if (!ok) printf("  kit: n %u %u %u %u %u st %d base %d\n", n, nt[0], nt[1], nt[2], nt[3], drum_kit[0][DV_KICK].st, base);
        bad += check("on a drum kit: OCT tunes the hit, BEND bends it, ARP NOTES plays its sounds in turn", ok);
    }
    /* SEQ > COMP 2: its knobs; (0.6.92) KNOB 4 EVERY there, RATE (DIV) on COMP for an ECHO */
    ui_power_on();
    t = TSEL;
    t->step[0] = (step_t){{60, 64, 67}, 3, ST_NOTE, 0, 100};
    go_page(GR_COMP); frame();
    ui.page++; page_entered(); frame();
    ok = str_eq(cur_page()->title, "COMP 2") && comp_slot() == 1u && comp_view() == COMP_OFF;
    turn(EN_K2, 1);                                       /* OFF -> RATCH (no CHANCE here: an ECHO, x2 FLAT) */
    ok &= t->step[0].comp2 == CP_ECHO && t->step[0].carg2 == CP_ECHO_EVEN && !t->step[0].comp;
    turn(EN_K2, 1);                                       /* .. ARP */
    ok &= t->step[0].comp2 == CP_ARPN;
    turn(EN_K2, 4); turn(EN_K2, 1);                       /* .. RAND */
    ok &= t->step[0].comp2 == CP_RAND;
    turn(EN_K4, 1); turn(EN_K4, 1); turn(EN_K4, 1);       /* EVERY: 1:2 2:2 1:3 */
    ok &= ST_EVERY(&t->step[0]) == EV_1_3 && strstr(ui.col[3], "1:3") != 0;
    ui.page--; page_entered(); frame();
    ok &= str_eq(cur_page()->title, "COMP") && comp_slot() == 0u;
    turn(EN_K2, 2); turn(EN_K3, 4);                       /* OFF CHANCE RATCH: x5 FLAT, an ECHO on the first */
    ok &= t->step[0].comp == CP_ECHO && t->step[0].carg == (CP_ECHO_EVEN | 3u);
    turn(EN_K4, 1); turn(EN_K4, 1); turn(EN_K4, 1);       /* RATE: a step/4 (DIV) */
    ok &= CT_DIV(&t->step[0]) == 3u && strstr(ui.col[3], "1/16") != 0;   /* (4 over a synth's 4 steps: one apart) */
    bad += check("SEQ > COMP 2: a second component (RATCH, ARP, RAND), EVERY 1:3 on KNOB 4; COMP: RATCH x5, its RATE 1/16 (as it plays: a step apart)", ok);
    return bad;
}

/* (0.6.87) SEQ > HUMAN: at 0 % (stored 127, every project before it) each note as written, on the step; VEL varies each
 * note's velocity (+-38 at 100 %), TIME starts it up to 30 ms late (a third of the step at most), never early, GATE
 * varies its length; a chord stays together */
static int human(void)
{
    int bad = 0;
    uint32_t pass, k;
    for (pass = 0; pass < 3u; pass++) {
        track_t *t = one_step(1, 0);
        uint32_t len = step_len(t, 0), b, n = 0, lo = 999999, hi = 0, vlo = 127, vhi = 0, cmax = len / 3u, early = 0;
        uint32_t cap = FS * 40u / 1000u < cmax ? FS * 40u / 1000u : cmax, apart = 0, lo_b = 0;   /* (0.6.92: 40 ms) */
        if (pass == 2u) { t->p[P_VOICE] = V_POLY; t->step[0].n = 3; t->step[0].note[1] = 64; t->step[0].note[2] = 67; }
        t->p[P_HVEL] = pass ? 0 : 127;                  /* (100 %, 0 %) */
        t->p[P_HTIM] = pass ? 0 : 127;
        uint32_t st = 0;
        for (b = 0; b < 64u * len / CTL; b++) {
            uint32_t v0 = vage, first = 1;
            seq_tick(t, CTL);
            if (t->seq_pos < CTL) st = b;              /* (a step began in this block) */
            for (; v0 < vage; v0++) {
                uint32_t off = (b - st) * CTL, vel = 0;
                for (k = 0; k < NVOICE; k++)
                    if (t->v[k].active && t->v[k].age == v0 + 1u) vel = t->v[k].vel;
                if (off > len / 2u) early++;           /* (a note just before a step: early) */
                if (off < lo) lo = off;
                if (off > hi) hi = off;
                if (vel < vlo) vlo = vel;
                if (vel > vhi) vhi = vel;
                if (!first) apart += off != lo_b;
                lo_b = off;
                first = 0;
                n++;
            }
        }
        if (pass == 0u)
            bad += check("HUMAN 0 %: every note on its step, as written (velocity 100)", n >= 60u && hi == 0u && vlo == 100u && vhi == 100u);
        else if (pass == 1u) {
            printf("  human: start %u .. %u samples late (cap %u), velocity %u .. %u\n", lo, hi, cap, vlo, vhi);
            bad += check("HUMAN 100 %: notes up to 40 ms late (a third of a step), never early; velocities spread +-50",
                         n >= 60u && !early && hi > cap / 2u && hi <= cap + CTL && lo < cap / 3u && vlo >= 50u && vhi <= 127u &&
                         vhi - vlo > 40u);
        } else
            printf("  chord: n %u apart %u early %u\n", n, apart, early), bad += check("HUMAN 100 % on a chord: its notes together (one start), each its own velocity", n >= 180u && !apart && !early);
    }
    return bad;
}

/* (0.6.88) TIMING: a step's own place off the grid. Late (+n/64; 0.9.1, sixteenths before): its notes n 64ths of the step
 * after its place; early (-n/64): before it (the step before ends sooner, the one after keeps its place: the loop's
 * length as it was);
 * recorded live with REC Q under 100 %: the step keeps where the note was played (sooner than the middle of a step:
 * the next step, early); 100 %: on the grid; stored with the project */
static int timing(void)
{
    int bad = 0, ok;
    track_t *t;
    uint32_t b, k, len, at[64], n, st = 0;
    int32_t off[64];
    for (k = 0; k < 2u; k++) {                          /* (k 0: steps 1 and 3 late +4 / early -4; k 1: all on the grid) */
        ui_power_on();
        t = &trk[0];
        track_defaults_steps(t);
        t->p[P_SLEN] = 4; t->p[P_SGATE] = 32; t->p[P_SSWING] = 0; song.g[G_SWING] = 0;
        for (b = 0; b < 4u; b++) t->step[b] = (step_t){{(uint8_t)(60 + b)}, 1, ST_NOTE, 0, 100};
        if (!k) { ST_MICRO_SET(&t->step[1], 16); ST_MICRO_SET(&t->step[3], -16); }   /* (0.9.1: 64ths, +4/16 -4/16) */
        seq_start();
        len = step_len(t, 0);
        n = 0;
        for (b = 0; b < 40u * len / CTL && n < 64u; b++) {
            uint32_t v0 = vage;
            seq_tick(t, CTL);
            for (; v0 < vage && n < 64u; v0++, n++) at[n] = b * CTL;
        }
        for (b = 0; b < n; b++) off[b] = (int32_t)at[b] - (int32_t)(b * len);   /* (note b on the grid: step b) */
        if (k) {
            ok = n >= 32u;
            for (b = 0; b < n; b++) ok &= off[b] > -(int32_t)CTL && off[b] < (int32_t)CTL;
            bad += check("TIMING 0: every step on the grid", ok);
        } else {
            int32_t q = (int32_t)len / 4;
            ok = n >= 32u;
            for (b = 0; b < n; b++) {
                int32_t want = b % 4u == 1u ? q : b % 4u == 3u ? -q : 0;
                ok &= off[b] > want - 2 * (int32_t)CTL && off[b] < want + 2 * (int32_t)CTL;
            }
            if (!ok) for (b = 0; b < 8u; b++) printf("  timing: note %u at %d (step %u)\n", b, off[b], len);
            bad += check("TIMING +16/64 and -16/64: the steps a quarter late / early, the rest on the grid, the loop's length kept", ok);
        }
    }
    {   /* live recording: a note played 5/16 into step 2 and one 3/16 before step 4, REC Q 0 % and 100 % */
        uint32_t pass;
        for (pass = 0; pass < 2u; pass++) {
            ui_power_on();
            t = &trk[0];
            track_defaults_steps(t);
            for (b = 0; b < NSTEP; b++) step_clear(&t->step[b]);
            t->p[P_SLEN] = 8; t->p[P_SSWING] = 0; song.g[G_SWING] = 0;
            t->p[P_HRQ] = pass ? 127 : 0;
            seq_start();
            song.rec = 1;
            len = step_len(t, 0);
            for (b = 0; b < 8u * len / CTL; b++) {
                uint32_t p0 = b * CTL, p1 = p0 + CTL;
                seq_tick(t, CTL);
                if (p0 <= len + 5u * len / 16u && p1 > len + 5u * len / 16u) { input_on(t, 64, 100); trk_note_off(t, 64); }
                if (p0 <= 3u * len - 3u * len / 16u && p1 > 3u * len - 3u * len / 16u) { input_on(t, 67, 100); trk_note_off(t, 67); }
            }
            song.rec = 0;
            if (pass)
                ok = t->step[1].note[0] == 64 && ST_MICRO(&t->step[1]) == 0 && t->step[3].note[0] == 67 && ST_MICRO(&t->step[3]) == 0;
            else {
                printf("  timing: recorded %d/64 (step 2, %u) and %d/64 (step 4, %u)\n", ST_MICRO(&t->step[1]), t->step[1].note[0],
                       ST_MICRO(&t->step[3]), t->step[3].note[0]);
                ok = t->step[1].note[0] == 64 && ST_MICRO(&t->step[1]) >= 19 && ST_MICRO(&t->step[1]) <= 21 &&   /* (0.9.1: 64ths,
                                                         * within one of 20 and -12 now, the block's sample apart) */
                     t->step[3].note[0] == 67 && ST_MICRO(&t->step[3]) >= -13 && ST_MICRO(&t->step[3]) <= -11;
            }
            bad += check(pass ? "  REC Q 100 %: recorded on the grid" : "  REC Q 0 %: recorded where played (+20/64 late, -12/64 early)", ok);
        }
    }
    {   /* stored with the project */
        project_t a, c;
        project_store_t p;
        ui_power_on();
        trk[0].step[2] = (step_t){{62}, 1, ST_NOTE, 0, 100};
        ST_MICRO_SET(&trk[0].step[2], -27);             /* (0.9.1: 64ths, -7/16 and a 64th more) */
        trk[0].p[P_HRQ] = 40;
        project_capture(&a);
        ok = proj_pack(&p, &a) && proj_import(&c, &p, sizeof p) && ST_MICRO(&c.t[0].step[2]) == -27 && c.t[0].p[P_HRQ] == 40;
        bad += check("  TIMING and REC Q kept with the project", ok);
    }
    (void)st;
    return bad;
}

int main(void)
{
    int bad = play() + comp_marks() + projects() + presets() + screens() + components() + components2() + components3() + human() + timing();
    printf("%s\n", bad ? "RATCHET TEST FAILED" : "ratchet tests passed");
    return bad != 0;
}
