/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments
 * Portions Copyright (C) 2026 Juan Ignacio Antonucci (Juan Jiant) */
/* Host test of the FX hold layer's effects (firmware/src/perform.c), same sources as the firmware (through
 * hostsim.c).   build/host/perform_test [DEMO_DIR]          (run_tests.sh: build/perform_demo)
 * 1. timing: REPEAT starts on the next 1/16 of the transport (sample exact) and ends when let go (the live
 *    signal back, bit for bit, after the 2.9 ms ramp); the sweeps start at once.
 * 2. stereo: REPEAT keeps left and right apart (a silent right channel stays silent; each side follows its
 *    own input).
 * 3. a REPEAT too long for the loop (1/8 below 81 BPM) does nothing at all.
 * 4. the SLICER: its recordings are not used meanwhile and are dropped afterwards.
 * 5. the keys: a layer key plays nothing and sends no MIDI; a key held before FX stays a note; the white
 *    keys past the first 5 are the layer's too but do nothing.
 * 6. idle: nothing held, nothing ramping, the mix is bit-identical (and the goldens of regress.c too).
 * 7. no clicks, no overflow; the sweeps (FILTER CRUSH THROW) and the mutes do what they say.
 * 8. cost: instructions per sample of the song, idle and with every effect at once (proc_pid_rusage).
 * 9. the punch-in MIDI effects (pfx.c): OCT+ doubles a held note's frequency and lets it back; A#4 DRM leaves a synth
 *    alone; DEC- shortens a kick's tail (DECY back after the block); STUTTER 1/16 strikes a kick 4 times a beat;
 *    1/2 TEMPO slows the sequencer and, let go, puts it where it would have been; RANDOM moves some notes.
 * 10. (0.9.14) the sweeps: 2 bars dry to wet on an S curve, a beat back; GLITCH: the same slices at the same places
 *    of the transport (two bars), every kind of slice, no click (a 110 Hz sine), live back bit for bit; TAPE STOP:
 *    the pitch falling, silent after a beat, no click; the lane records and plays them.
 * Demos (WAV) into DEMO_DIR. TONIC removed REVERSE, FREEZE and OCT UP / DN (perform.c). */
static unsigned pt_hushed;                         /* (0.9.17) blocks the silence path took (fx.c mix_silent) */
#define HUSH_COUNT() (pt_hushed++)
#define main hostsim_main
#include "hostsim.c"
#undef main
#ifdef __APPLE__
#include <libproc.h>
#include <sys/resource.h>
#endif

static int check(const char *what, int ok)
{
    printf("perform: %-73s %s\n", what, ok ? "ok" : "FAIL");
    return ok ? 0 : 1;
}

/* ------------------------------------------------------ a test signal --- */
#define NT (8u * 44100u)
static int32_t in_l[NT], in_r[NT], out_l[NT], out_r[NT];
static void test_signal(double fl, double al, double fr, double ar)
{
    uint32_t t;
    for (t = 0; t < NT; t++) {
        in_l[t] = (int32_t)lrint(al * sin(2 * M_PI * fl * t / FS));
        in_r[t] = (int32_t)lrint(ar * sin(2 * M_PI * fr * t / FS + 1.0));
    }
}
static void perf_reset(void)
{
    memset(&pf, 0, sizeof pf);
    pf.src = pf.next = PF_N;
    for (uint32_t m = 0; m < NTRK; m++) pf.mg[m] = 32768;
    perf_held = perf_act = 0;
    perf_kill = 0;
    memset((void *)perf_m, 0, sizeof perf_m);
    pfx_lfx = 0;
    perf_latched = 0;
    pfx_tgt = 0;
    pfx_any = 0;
    sl_lent = 0;
    slicer_reset();
}
/* the stage alone on the test signal: samples [t0, t1); ev: press (+e + 1) / let go (-(e + 1)) at block bt */
typedef struct { uint32_t t; int e; } ev_t;
static int busy_seen;
static uint8_t blk_cru[NT / CTL + 1];              /* (0.9.17) a block in a GLITCH CRUSH slice (its steps are the crusher's) */
static void run(uint32_t bpm, int playing, const ev_t *ev, uint32_t nev, uint32_t t1)
{
    uint32_t t, k = 0;
    host_tracks_init();
    perf_reset();
    song.g[G_BPM] = (int16_t)bpm;
    song.playing = (uint8_t)playing;
    perf_start();
    busy_seen = 0;
    for (t = 0; t < t1; t += CTL) {
        while (k < nev && ev[k].t <= t) {
            perf_press((uint32_t)(ev[k].e > 0 ? ev[k].e - 1 : -ev[k].e - 1), ev[k].e > 0);
            k++;
        }
        memcpy(out_l + t, in_l + t, CTL * 4);
        memcpy(out_r + t, in_r + t, CTL * 4);
        clk_pos = t % div_samples(2);              /* (the transport's 1/16 at the block's start, as events_block leaves it) */
        clk_n = t / div_samples(2);
        blk_cru[t / CTL] = pf.mode == BM_GLT && pf.u.g.a == GA_CRU;
        if (perf_begin(CTL)) {
            busy_seen = 1;
            perf_block(out_l + t, out_r + t, CTL);
        }
        blk_cru[t / CTL] |= pf.mode == BM_GLT && pf.u.g.a == GA_CRU;
    }
}
#define P16 5512u                                  /* a 1/16 at 120 BPM */
static int same(uint32_t a, uint32_t b)            /* out == in on [a, b), both sides */
{
    uint32_t t;
    for (t = a; t < b; t++)
        if (out_l[t] != in_l[t] || out_r[t] != in_r[t])
            return 0;
    return 1;
}
static int32_t err_vs(uint32_t a, uint32_t b, int32_t (*src)(const int32_t *, uint32_t), int side)
{
    uint32_t t;
    int32_t e = 0;
    for (t = a; t < b; t++) {
        int32_t d = abs((side ? out_r : out_l)[t] - src(side ? in_r : in_l, t));
        e = d > e ? d : e;
    }
    return e;
}
static uint32_t shift_d;                           /* the loop: in[t - shift_d] */
static int32_t delayed(const int32_t *x, uint32_t t) { return x[t - shift_d]; }
static int32_t peak(const int32_t *x, uint32_t a, uint32_t b)
{
    int32_t m = 0;
    for (; a < b; a++)
        m = abs(x[a]) > m ? abs(x[a]) : m;
    return m;
}

/* ------------------------------------------------------------ 1. timing --- */
static int test_timing(void)
{
    int bad = 0;
    char what[160];
    test_signal(440, 12000, 277, 9000);
    {   /* REPEAT 1/16: pressed mid-1/16, records the next 1/16, then loops it */
        ev_t ev[] = {{992, PF_R16 + 1}, {20000, -(PF_R16 + 1)}};
        int32_t el, er, el2;
        run(120, 1, ev, 2, 40000);
        shift_d = P16;
        el = err_vs(2 * P16 + 160, 3 * P16 - 160, delayed, 0);
        er = err_vs(2 * P16 + 160, 3 * P16 - 160, delayed, 1);
        shift_d = 2 * P16;
        el2 = err_vs(3 * P16 + 160, 20000 - 160, delayed, 0);
        snprintf(what, sizeof what, "REPEAT 1/16: live until the 1/16 after the press + one 1/16 (exact)");
        bad += check(what, same(0, 2 * P16));
        snprintf(what, sizeof what, "  then the 1/16 from that 1/16, looped (error L %d R %d, 2nd pass %d of 12000)", el, er, el2);
        bad += check(what, el < 100 && er < 100 && el2 < 100);
        bad += check("  let go: the live signal again after the ramp, bit for bit", same(20000 + 160, 40000));
        bad += check("  the buffer given back, nothing left running", !sl_lent && pf.mode == BM_NONE && !pf.busy);
    }
    {   /* stopped: at once */
        ev_t ev[] = {{992, PF_R16 + 1}};
        run(120, 0, ev, 1, 12000);
        bad += check("stopped: a 1/16 effect starts at once", (perf_act & PF_BIT(PF_R16)) && !same(992, 12000));
    }
    {   /* the immediate ones: a sweep moves at once */
        ev_t ev[] = {{992, PF_SCRU + 1}};
        run(120, 1, ev, 1, 3000);
        bad += check("the CRUSH sweep starts at once (not on the 1/16)", same(0, 992) && !same(992, 3000));
    }
    return bad;
}

/* ------------------------------------------------------------ 2. stereo --- */
static int test_stereo(void)
{
    static const struct { const char *name; int e; } C[] = {{"REPEAT 1/8", PF_R8}};
    uint32_t c;
    int bad = 0;
    char what[160];
    for (c = 0; c < sizeof C / sizeof C[0]; c++) {
        ev_t ev[] = {{992, C[c].e + 1}, {60000, -(C[c].e + 1)}};
        int32_t pl, pr;
        double cl = 0, cr = 0, nl = 0, nr = 0;
        uint32_t t;
        test_signal(440, 12000, 0, 0);                    /* the right side silent */
        run(120, 1, ev, 2, 70000);
        pl = peak(out_l, 30000, 60000);
        pr = peak(out_r, 0, 70000);
        snprintf(what, sizeof what, "%s: a silent right side stays silent (L %d, R %d)", C[c].name, pl, pr);
        bad += check(what, pr == 0 && pl > 3000);
        test_signal(440, 12000, 3 * 440, 12000);          /* each side its own pitch */
        run(120, 1, ev, 2, 70000);
        for (t = 30000u; t < 50000u; t++) {
            cl += (double)out_l[t] * out_l[t - 1];
            nl += (double)out_l[t] * out_l[t];
            cr += (double)out_r[t] * out_r[t - 1];
            nr += (double)out_r[t] * out_r[t];
        }
        cl = nl > 0 ? cl / nl : 1;
        cr = nr > 0 ? cr / nr : 1;
        snprintf(what, sizeof what, "  each side keeps its own pitch (lag-1 correlation L %.3f, R %.3f)", cl, cr);
        bad += check(what, cl > 0.99 && cr < 0.99 && cr > 0.9 && cl - cr > 0.008);
    }
    return bad;
}

/* -------------------------------------------------------- 3. too long --- */
static int test_too_long(void)
{
    int bad = 0;
    ev_t ev[] = {{992, PF_R8 + 1}, {40000, -(PF_R8 + 1)}};
    test_signal(440, 12000, 277, 9000);
    run(72, 1, ev, 2, 50000);
    bad += check("REPEAT 1/8 at 72 BPM (417 ms, the loop holds 371): nothing happens, bit for bit",
                 same(0, 50000) && !busy_seen && !sl_lent && !(perf_avail() & PF_BIT(PF_R8)));
    run(120, 1, ev, 2, 50000);
    bad += check("REPEAT 1/8 at 120 BPM (250 ms): it plays", !same(0, 40000) && (perf_avail() & PF_BIT(PF_R8)));
    song.g[G_BPM] = 81;
    bad += check("  REPEAT 1/8 fits from 81 BPM, 1/16 from 41",
                 (perf_avail() & PF_BIT(PF_R8)) &&
                 (song.g[G_BPM] = 80, !(perf_avail() & PF_BIT(PF_R8))) &&
                 (song.g[G_BPM] = 41, perf_avail() & PF_BIT(PF_R16)) &&
                 (song.g[G_BPM] = 40, !(perf_avail() & PF_BIT(PF_R16))));
    {   /* a REPEAT held while the tempo slows past its fit stops cleanly */
        uint32_t t;
        int32_t mx = 0;
        host_tracks_init();
        perf_reset();
        song.g[G_BPM] = 120;
        song.playing = 1;
        perf_start();
        test_signal(110, 12000, 110, 12000);
        perf_press(PF_R8, 1);
        for (t = 0; t < 120000u; t += CTL) {
            uint32_t i;
            if (t == 60000u)
                song.g[G_BPM] = 72;
            memcpy(out_l + t % NT, in_l + t % NT, CTL * 4);
            memcpy(out_r + t % NT, in_r + t % NT, CTL * 4);
            if (perf_begin(CTL))
                perf_block(out_l + t % NT, out_r + t % NT, CTL);
            for (i = 1; i < CTL && t > 1000u; i++) {
                int32_t d = abs(out_l[t % NT + i] - out_l[t % NT + i - 1]);
                mx = d > mx ? d : mx;
            }
        }
        perf_press(PF_R8, 0);
        bad += check("  a REPEAT 1/8 held while the tempo drops below its fit: it ends, no click",
                     pf.mode == BM_NONE && !sl_lent && mx < 4 * 190);
    }
    return bad;
}

/* ------------------------------------------------------------ the song --- */
static void song_setup(void)
{
    static const uint8_t ACID[16] = {45, 45, 57, 45, 0, 48, 45, 55, 45, 0, 57, 52, 45, 48, 0, 50};
    static const uint8_t AM[4] = {57, 60, 64, 67};
    track_t *t1 = &trk[0], *t2 = &trk[1], *td = &trk[3];
    uint32_t i;
    memset(trk, 0, sizeof trk);
    host_tracks_init();
    perf_reset();
    song.g[G_BPM] = 120;
    host_preset(t1, 0, 4);
    host_preset(t2, 1, 5);
    host_drums(td);                                /* DRUM KIT (SAMPLE PERC until 1.0.2) */
    for (i = 0; i < 16u; i++) {
        uint8_t n = ACID[i], d[4], k = 0;
        put_step(t1, i, n ? 1u : 0u, &n, n ? ST_NOTE : ST_REST, 0);
        put_step(t2, i, i % 8u == 0u ? 4u : 0u, AM, i % 8u == 0u ? ST_NOTE : ST_TIE, 0);
        if (i % 4u == 0u) d[k++] = 36;
        if (i % 2u == 0u) d[k++] = 42;
        if (i == 4u || i == 12u) d[k++] = 38;
        put_step(td, i, k, d, k ? ST_NOTE : ST_REST, 0);
    }
    trk[0].p[P_PAN] = -40;                         /* the mix itself stereo */
    trk[1].p[P_PAN] = 40;
}
static int32_t song_l[10u * 44100u + CTL], song_r[10u * 44100u + CTL];   /* (room for a 10 s render), and
                                                         * the last block may run past the frames asked */
static void song_render(uint32_t frames, void (*at)(uint32_t t))
{
    uint32_t t, i;
    int32_t o[2 * CTL];
    transport_req = 1;
    for (t = 0; t < frames; t += CTL) {
        if (at)
            at(t);
        mix_block(o, CTL);
        for (i = 0; i < CTL; i++) {
            song_l[t + i] = o[2 * i];
            song_r[t + i] = o[2 * i + 1];
        }
    }
    transport_req = 2;
    mix_block(o, CTL);
}

/* ------------------------------------------------------------ 4. SLICER --- */
static int slicer_ok;
static void stut_at(uint32_t t)
{
    uint32_t k;
    if (t == 44032u)
        perf_press(PF_R16, 1);
    (void)k;
    if (t > 50000u && t < 80000u && (sl.rec_on || sl.loop))
        slicer_ok = 0;
    if (t > 50000u && t < 80000u && !sl_lent)
        slicer_ok = 0;
    if (t == 80000u)
        perf_press(PF_R16, 0);
    if (t == 80000u + 320u && (sl_lent || sl.rec || sl.loop))
        slicer_ok = 0;
}
static int test_slicer(void)
{
    uint32_t k;
    song_setup();
    (void)k;
    song.g[G_SLMODE] = SL_STUT;                  /* (JIANT 0.6.3: the bus, everything in) */
    song.g[G_SLPAT] = 1;
    song.g[G_SLRATE] = 1;
    song.g[G_SLDEP] = 127;
    slicer_ok = 1;
    song_render(120000, stut_at);
    return check("SLICER STUT: live (no recording, no repeat) while REPEAT has the buffer, dropped after", slicer_ok);
}

/* -------------------------------------------------------------- 5. keys --- */
static int test_keys(void)
{
    int bad = 0;
    uint32_t mo0, fx = 1u << 3, k = 6;              /* (a button bit for FX; key 6: a white key, B3: LPF) */
    song_setup();
    usb.config = 1;
    mo_r = mo_w = 0;
    kb_mask = perf_mask = fx;
    fm1_in.buttons = fx;
    fm1_in.notes = 1u << k;
    keyboard_block();
    mo0 = mo_w;
    events_block(CTL);
    bad += check("a key pressed while FX is held: no voice, no MIDI, the layer's, its effect held",
                 !busy_now() && mo0 == 0u && (kb_layer >> k) & 1u && (perf_held & PF_BIT(perf_key(k))));
    fm1_in.buttons = 0;                             /* FX let go first: the effect stays with the key */
    keyboard_block();
    bad += check("  FX let go first: the key still holds its effect", (perf_held & PF_BIT(perf_key(k))) != 0u);
    fm1_in.notes = 0;
    keyboard_block();
    bad += check("  let go: no note-off, the effect off", mo_w == 0u && !kb_layer && !perf_held);
    fm1_in.notes = 1u << 7;                         /* a key, then FX: a note, and its note-off later */
    keyboard_block();
    fm1_in.buttons = fx;
    keyboard_block();
    fm1_in.notes = 0;
    keyboard_block();
    bad += check("a key held before FX stays a note (note-on and note-off sent)", mo_w == 2u && !kb_layer);
    kb_mask = perf_mask = 0;                                /* no layer now (a menu): keys are notes */
    fm1_in.notes = 1u << 7;
    keyboard_block();
    bad += check("no layer (kb_mask 0): FX held, keys are notes", mo_w == 3u && !kb_layer);
    fm1_in.notes = 0;
    fm1_in.buttons = 0;
    keyboard_block();
    {   /* the white keys past the first 5 (D4 on), and black keys past the mutes: the layer's, nothing */
        uint32_t q, ok = 1, assigned = 0;
        for (q = 0; q < 27u; q++)
            if (perf_key(q) < PF_N)
                assigned |= 1u << q;
        kb_mask = perf_mask = fx;
        mo0 = mo_w;
        for (q = 0; q < 27u; q++) {
            if ((assigned >> q) & 1u)
                continue;
            fm1_in.buttons = fx;
            fm1_in.notes = 1u << q;
            keyboard_block();
            ok &= (kb_layer >> q) & 1u && kb_note[q] == KB_SILENT && !perf_held && mo_w == mo0;
            fm1_in.notes = 0;
            keyboard_block();
            ok &= !kb_layer && !perf_held && mo_w == mo0;
        }
        fm1_in.buttons = 0;
        bad += check("the keys without an effect (G5, the black keys past the mutes): silent, nothing held",
                     ok && (uint32_t)__builtin_popcount(assigned) == PF_N - (NTRK > 6u ? NTRK - 6u : 0u));   /* (15 white F3 .. F5,
                                                     * 10 black: six mutes, FREEZE, SHUFFLE and the sweeps; stems-10's
                                                     * T7 and T8 have mutes in the layer but no key left for them) */
        pfx_tgt = 0;                                /* (A#4 stepped it) */
        kb_mask = perf_mask = 0;
    }
    usb.config = 0;
    return bad;
}

/* -------------------------------------------------------------- 6. idle --- */
static int test_idle(void)
{
    static int32_t a_l[NT], a_r[NT];
    uint32_t t, diff = 0;
    int bad = 0, fd[2];
    pid_t pid;
    song_setup();
    if (pipe(fd) || (pid = fork()) < 0)
        return check("idle: fork", 0);
    if (!pid) {                                     /* the same state, FX held the whole time, no key */
        kb_mask = perf_mask = 1u << 3;
        fm1_in.buttons = 1u << 3;
        song_render(3u * FS, 0);
        close(fd[0]);
        if (write(fd[1], song_l, 3u * FS * 4u) < 0 || write(fd[1], song_r, 3u * FS * 4u) < 0)
            _exit(1);
        _exit(0);
    }
    close(fd[1]);
    song_render(3u * FS, 0);
    {
        FILE *f = fdopen(fd[0], "rb");
        size_t got = fread(a_l, 4, 3u * FS, f) + fread(a_r, 4, 3u * FS, f);
        fclose(f);
        waitpid(pid, 0, 0);
        diff = got != 6u * FS;
    }
    for (t = 0; t < 3u * FS; t++)
        diff += a_l[t] != song_l[t] || a_r[t] != song_r[t];
    bad += check("idle (FX held, no key): the song bit for bit as without the layer", !diff);
    {   /* after an effect: once its ramps are over, the stage is skipped again */
        uint32_t f;
        int32_t o[2 * CTL];
        perf_press(PF_R16, 1);
        for (f = 0; f < FS; f += CTL)
            mix_block(o, CTL);
        perf_press(PF_R16, 0);
        for (f = 0; f < 512u; f += CTL)
            mix_block(o, CTL);
        bad += check("  after an effect is let go and its ramp is over: skipped again", !pf.busy && !perf_begin(CTL));
    }
    return bad;
}

/* ----------------------------------------------- 7. clicks, the rest --- */
static int test_misc(void)
{
    int bad = 0;
    char what[160];
    static const struct { const char *name; int e; uint32_t off; } C[] = {
        {"REPEAT 1/8", PF_R8, 30000}, {"REPEAT 1/16", PF_R16, 30000}, {"REPEAT 1/32", PF_R32, 30000},
        {"GLITCH", PF_GLT, 170000}, {"TAPE STOP", PF_TAPE, 30000}, {"SHUFFLE", PF_SHUF, 220000},
        {"FREEZE", PF_FRZ, 60000}};
    uint32_t c, t;
    int32_t own;
    test_signal(110, 16000, 110, 16000);
    for (own = 0, t = 1; t < NT; t++)
        own = abs(in_l[t] - in_l[t - 1]) > own ? abs(in_l[t] - in_l[t - 1]) : own;
    for (c = 0; c < sizeof C / sizeof C[0]; c++) {
        ev_t ev[] = {{992, C[c].e + 1}, {C[c].off, -(C[c].e + 1)}};
        int32_t mx = 0;
        run(133, 1, ev, 2, C[c].off + 9000);
        for (t = 1; t < C[c].off + 9000; t++) {
            int32_t d = abs(out_l[t] - out_l[t - 1]);
            if (!blk_cru[t / CTL] && !blk_cru[(t - 1u) / CTL])
                mx = d > mx ? d : mx;
        }
        snprintf(what, sizeof what, "no clicks: %s on a 110 Hz sine, in and out: largest step %d (sine %d)%s", C[c].name, mx, own,
                 C[c].e == PF_GLT ? " (not in CRUSH)" : "");
        bad += check(what, mx <= 4 * own);
        bad += check("  back to the live signal, bit for bit", same(C[c].off + 3000, C[c].off + 9000));
    }
    {   /* full scale in, no overflow */
        for (t = 0; t < NT; t++) {                 /* full scale square waves, past Q15 */
            in_l[t] = (t / 37u) & 1u ? 120000 : -120000;
            in_r[t] = (t / 53u) & 1u ? 120000 : -120000;
        }
        {
            ev_t e2[] = {{0, PF_SCRU + 1}, {0, PF_GLT + 1}, {90000, PF_R16 + 1}, {150000, PF_TAPE + 1}, {170000, PF_FRZ + 1},
                         {190000, PF_SHUF + 1}};
            perf_gp[GX_GLT][1] = 12;                /* (the repeats rising: up to 4x) */
            run(120, 1, e2, 6, 200000);
            perf_gp[GX_GLT][1] = 0;
            snprintf(what, sizeof what, "4x full scale through the CRUSH sweep + GLITCH, REPEAT, TAPE, FREEZE, SHUFFLE: peak %d (no wrap)",
                     peak(out_l, 0, 200000) > peak(out_r, 0, 200000) ? peak(out_l, 0, 200000) : peak(out_r, 0, 200000));
            bad += check(what, peak(out_l, 0, 200000) < 300000 && peak(out_r, 0, 200000) < 300000);
        }
    }
    {   /* THROW (its sweep): the dry mix into the delay and reverb sends after 2 bars */
        static int32_t ml[CTL], mr[CTL], sd[CTL], sr[CTL], z[CTL];
        uint32_t i;
        host_tracks_init();
        perf_reset();
        song.g[G_BPM] = 120;
        song.playing = 1;
        perf_press(PF_STHR, 1);
        for (t = 0; t < 8u * 22050u + 1024u; t += CTL) {
            for (i = 0; i < CTL; i++)
                ml[i] = mr[i] = 8000, sd[i] = sr[i] = 0;
            if (perf_begin(CTL))
                perf_pre(ml, mr, z, z, sd, sr, CTL);
        }
        perf_press(PF_STHR, 0);
        bad += check("THROW held 2 bars: the dry mix into both sends, full, the dry mix untouched",
                     sd[CTL - 1] > 7900 && sr[CTL - 1] > 7900 && ml[CTL - 1] == 8000 && mr[CTL - 1] == 8000);
    }
    {
        int32_t o[2 * CTL];
        uint32_t f;
        int32_t before = 0, during = 0;
        song_setup();
        trk[1].p[P_SLEN] = 1; trk[3].p[P_SLEN] = 1;
        trk[1].step[0].n = 0; trk[3].step[0].n = 0;          /* only track 1 plays, dry */
        trk[0].p[P_CHOR] = trk[0].p[P_DLY] = trk[0].p[P_REV] = 0;
        transport_req = 1;
        for (f = 0; f < 2u * FS; f += CTL) {
            uint32_t i;
            if (f == 44096u)
                perf_press(PF_M1, 1);
            mix_block(o, CTL);
            for (i = 0; i < CTL; i++) {
                if (f > FS / 2u && f < 44096u) before = abs(o[2 * i]) > before ? abs(o[2 * i]) : before;
                if (f > FS + 512u) during = abs(o[2 * i]) > during ? abs(o[2 * i]) : during;
            }
        }
        perf_press(PF_M1, 0);
        transport_req = 2;
        mix_block(o, CTL);
        snprintf(what, sizeof what, "black key 1: track 1 muted while held (peak %d -> %d), P_MUTE untouched", before, during);
        bad += check(what, before > 1000 && during < 200 && !trk[0].p[P_MUTE]);
    }
    return bad;
}

/* -------------------------------------------------------------- 8. cost --- */
static uint64_t instr_now(void)
{
#ifdef __APPLE__
    struct rusage_info_v4 ri;
    if (!proc_pid_rusage(getpid(), RUSAGE_INFO_V4, (rusage_info_t *)&ri))
        return ri.ri_instructions;
#endif
    return 0;
}
static double cost_run(int on)
{
    static const int ALL[] = {PF_GLT, PF_SCRU, PF_STHR, PF_M1 + 2};
    uint32_t f, k;
    uint64_t i0;
    int32_t o[2 * CTL];
    song_setup();
    transport_req = 1;
    for (f = 0; f < FS; f += CTL)
        mix_block(o, CTL);
    if (on == 1) {
        for (k = 0; k < sizeof ALL / sizeof ALL[0]; k++)
            perf_press((uint32_t)ALL[k], 1);
    } else if (on == 3) {                           /* REPEAT 1/16 alone: the layer's own cost, for scale */
        perf_press(PF_R16, 1);
    }
    i0 = instr_now();
    for (f = 0; f < 2u * FS; f += CTL)
        mix_block(o, CTL);
    for (k = 0; k < PF_N; k++)
        perf_press(k, 0);
    transport_req = 2;
    return i0 ? (double)(instr_now() - i0) / (2.0 * FS) : 0;
}
static int test_cost(void)
{
    double idle = cost_run(0), on = cost_run(1), rep = cost_run(3);
    char what[200];
    int bad;
    if (!idle) {
        printf("perform: cost: no instruction counter on this host\n");
        return 0;
    }
    snprintf(what, sizeof what, "cost: the song %.0f instructions / sample idle, %.0f with GLITCH+CRUSH+THROW+MUTE: +%.0f",
             idle, on, on - idle);
    bad = check(what, on - idle < 400);
    printf("perform: cost: over the idle song: REPEAT 1/16 alone +%.0f (the layer's own stage)\n", rep - idle);
    return bad;
}

/* ------------------------------------------------------------ demos --- */
static void demo_at(uint32_t t)
{
    static const struct { uint32_t t; int e; } D[] = {     /* press +(e + 1), let go -(e + 1) */
        {88192, PF_R8 + 1}, {110080, -(PF_R8 + 1)}, {110080, PF_R16 + 1}, {121088, PF_R32 + 1},
        {126592, -(PF_R32 + 1)}, {132096, -(PF_R16 + 1)}, {132096, PF_GLT + 1}, {220288, -(PF_GLT + 1)},
        {220288, PF_SHUF + 1}, {308480, -(PF_SHUF + 1)}, {330496, PF_TAPE + 1}, {352512, -(PF_TAPE + 1)},
        {374528, PF_SCRU + 1}, {418560, -(PF_SCRU + 1)}, {440896, PF_M1 + 1}, {462848, -(PF_M1 + 1)},
        {418560, PF_STHR + 1}, {440576, -(PF_STHR + 1)}};
    uint32_t i;
    for (i = 0; i < sizeof D / sizeof D[0]; i++)
        if (D[i].t == t)
            perf_press((uint32_t)abs(D[i].e) - 1u, D[i].e > 0);
}
static void demos(const char *dir)
{
    char path[512];
    FILE *f;
    uint32_t t;
    song_setup();
    song_render(NT - 4096u, demo_at);
    snprintf(path, sizeof path, "%s/perform_tour.wav", dir);
    if (!(f = fopen(path, "wb")))
        return;
    wav_hdr(f, NT - 4096u);
    for (t = 0; t < NT - 4096u; t++)
        wav_put(f, song_l[t], song_r[t]);
    fclose(f);
    printf("perform: demo %s (REPEAT 1/8, a 1/16 -> 1/32 roll, GLITCH, SHUFFLE, TAPE STOP, CRUSH, THROW, MUTE 1)\n", path);
}

/* ------------------------------------------------- 9. punch-in MIDI --- */
static uint32_t zc(const int32_t *x, uint32_t a, uint32_t b)   /* zero crossings, low-passed (~350 Hz, 2 poles) */
{
    uint32_t i, c = 0;
    int32_t y1 = 0, y2 = 0, p = 0;
    for (i = a; i < b; i++) {
        y1 += (x[i] - y1) >> 5;
        y2 += (y1 - y2) >> 5;
        if (i > a + 400u)
            c += p < 0 && y2 >= 0;
        p = y2;
    }
    return c;
}
static void pfx_render(uint32_t frames, uint32_t press_at, uint32_t free_at, uint32_t e)
{
    uint32_t t, i;
    int32_t o[2 * CTL];
    for (t = 0; t < frames; t += CTL) {
        if (t == press_at)
            perf_press(e, 1);
        if (t == free_at)
            perf_press(e, 0);
        mix_block(o, CTL);
        for (i = 0; i < CTL; i++)
            song_l[t + i] = o[2 * i];
    }
}
static int test_pfx(void)
{
    int bad = 0;
    uint32_t c0, c1, c2, i, hits = 0;
    track_t *t1 = &trk[0], *td = &trk[3];
    int32_t o[2 * CTL];
    /* OCT+ on a held note; then with DRM: the synth untouched */
    song_setup();
    for (i = 0; i < NTRK; i++) trk[i].p[P_DIST] = trk[i].p[P_CHOR] = trk[i].p[P_DLY] = trk[i].p[P_REV] = 0;
    host_preset(t1, 0, 1);
    trk_note_on(t1, 45, 100);
    pfx_render(4u * 44032u, 88064u, 132096u, PF_OCTU);   /* (its first second: the sound settles) */
    c0 = zc(song_l, 66048u, 88064u);
    c1 = zc(song_l, 110080u, 132096u);
    c2 = zc(song_l, 154112u, 176128u);
    bad += check("OCT+: a held note an octave up while held, back after", c1 > c0 * 1.7 && c1 < c0 * 2.3 &&
                 c2 > c0 * 0.8 && c2 < c0 * 1.15 && !t1->pfx_pit);   /* (0.9.14: 0.8, was 0.85: the buses start
                                                         * silent since they rest (fx.c fx_rest), the delay's echoes of the
                                                         * attack in c0's window plain: 65 / 55; with the last test's residue
                                                         * under them, 54 / 56) */
    pfx_tgt = 2;
    pfx_render(2u * 44032u, 22016u, 88064u, PF_OCTU);
    c1 = zc(song_l, 44032u, 66048u);
    c0 = c2;
    bad += check("  A#4 DRM: OCT+ leaves the synth alone", c1 > c0 * 0.85 && c1 < c0 * 1.15 && !t1->pfx_pit);
    pfx_tgt = 0;
    trk_note_off(t1, 45);
    /* DEC- on a kick: the tail shorter; DECY as it was outside the block */
    {
        double r0 = 0, r1 = 0;
        int16_t d0 = td->p[P_E3];
        uint32_t m;
        for (m = 0; m < 2u; m++) {
            for (i = 0; i < 44100u; i += CTL) mix_block(o, CTL);
            if (m) perf_press(PF_DSHT, 1);
            mix_block(o, CTL);                          /* (the effect in) */
            trk_note_on(td, 36, 100);
            pfx_render(11008u, 0xFFFFFFFFu, 0xFFFFFFFFu, 0);
            for (i = 2205u; i < 8820u; i++) *(m ? &r1 : &r0) += (double)song_l[i] * song_l[i];
            if (m) perf_press(PF_DSHT, 0);
        }
        mix_block(o, CTL);
        bad += check("DEC-: a kick's tail shorter; DECY as stored outside the ISR", r1 < r0 * 0.5 && td->p[P_E3] == d0);
    }
    /* STUTTER 1/16 on the drums: the last kick 4 times a beat (120 BPM: 22050 samples) */
    song_setup();
    trk_note_on(td, 36, 100);
    mix_block(o, CTL);
    perf_press(PF_S8, 1);
    for (i = 0; i < 22016u; i += CTL) {
        mix_block(o, CTL);
        hits += drum_kit[3][DV_KICK].age == 1u;
    }
    perf_press(PF_S8, 0);
    mix_block(o, CTL);
    bad += check("STUTTER 1/8 (JIANT 0.4, was 1/16): the last kick twice in a beat; let go: none more", hits == 2u && !(td->pfx & PFX_REP));
    /* (JIANT 0.4) ATK+ on the drums: a hit's first ms softer (faded in); DEC- on a synth: its sustain 0 (a pluck), put
     * back after; ARP on the drums: a fill, each press another */
    {
        double e0 = 0, e1 = 0;
        uint32_t m;
        for (m = 0; m < 2u; m++) {
            song_setup();
            for (i = 0; i < 22016u; i += CTL) mix_block(o, CTL);
            if (m) perf_press(PF_ATK, 1);
            mix_block(o, CTL);
            mix_block(o, CTL);
            trk_note_on(td, 38, 110);
            pfx_render(4416u, 0xFFFFFFFFu, 0xFFFFFFFFu, 0);
            for (i = 0; i < 441u; i++) *(m ? &e1 : &e0) += (double)song_l[i] * song_l[i];
            if (m) perf_press(PF_ATK, 0);
        }
        mix_block(o, CTL);
        bad += check("ATK+ on the drums: a snare's first 10 ms softer (faded in)", e1 < e0 * 0.5 && e0 > 0);
    }
    {
        int16_t s0;
        song_setup();
        s0 = t1->p[P_SUS] = 100;
        perf_press(PF_DSHT, 1);
        for (i = 0; i < 4u * CTL; i += CTL) mix_block(o, CTL);
        {
            int16_t during;
            uint32_t v;
            int32_t env = 0;
            trk_note_on(t1, 60, 100);
            for (i = 0; i < 22016u; i += CTL) mix_block(o, CTL);
            for (v = 0; v < NVOICE; v++)
                if (t1->v[v].active && t1->v[v].env > env) env = t1->v[v].env;
            during = t1->p[P_SUS];
            perf_press(PF_DSHT, 0);
            mix_block(o, CTL);
            bad += check("DEC- on a synth: its sustain 0 (a held note decays away), back after", env < (1 << 24) / 20 &&
                         during == s0 && t1->p[P_SUS] == s0);
            trk_note_off(t1, 60);
        }
    }
    {
        uint8_t f[2][16];
        uint32_t m, n, same = 0, notes[2] = {0, 0};
        for (m = 0; m < 2u; m++) {
            song_setup();
            perf_press(PF_ARP, 1);
            for (n = 0; n < 16u; n++) {
                f[m][n] = 0;
                for (i = 0; i < 5504u; i += CTL) {
                    mix_block(o, CTL);
                    if (td->pfx_snn && !f[m][n]) { f[m][n] = td->pfx_sn[0]; notes[m]++; }
                }
            }
            perf_press(PF_ARP, 0);
            mix_block(o, CTL);
        }
        for (n = 0; n < 16u; n++) same += f[0][n] == f[1][n];
        bad += check("ARP on the drums: a fill without a note struck first, another at the next press", notes[0] >= 8u &&
                     notes[1] >= 8u && same < 12u);
    }
    /* 1/2 TEMPO: the sequencer at half speed while held; let go, where it would have been */
    {
        uint16_t i0, ih;
        uint32_t p0;
        song_setup();
        song_render(4u * 22016u, 0);
        i0 = trk[0].seq_idx; p0 = trk[0].seq_pos;
        song_setup();
        transport_req = 1;
        for (i = 0; i < 4u * 22016u; i += CTL) {
            if (i == 22016u) perf_press(PF_HALF, 1);
            if (i == 3u * 22016u) { ih = trk[0].seq_idx; perf_press(PF_HALF, 0); }
            mix_block(o, CTL);
        }
        transport_req = 2;
        mix_block(o, CTL);
        bad += check("1/2 TEMPO: half the steps while held; let go, the sequencer where it would have been",
                     (uint16_t)(ih - 4u) <= 5u && trk[0].seq_idx == i0 && trk[0].seq_pos == p0);
    }
    /* RANDOM: some notes moved (an octave, a fifth, a fourth), none without it */
    {
        uint32_t moved = 0, far = 0, m;
        song_setup();
        host_preset(t1, 0, 1);
        for (m = 0; m < 2u; m++) {
            if (m) perf_press(PF_RND, 1);
            mix_block(o, CTL);
            for (i = 0; i < 24u; i++) {
                uint32_t v;
                trk_note_on(t1, 57, 100);
                for (v = 0; v < NVOICE; v++)
                    if (t1->v[v].active && t1->v[v].note == 57 && t1->v[v].gate) {
                        int32_t d = t1->v[v].pitch16 - 57 * 16;
                        moved += m ? d != 0 : (uint32_t)(d != 0) * 100u;
                        far += d > 12 * 16 || d < -12 * 16;
                    }
                trk_note_off(t1, 57);
                mix_block(o, CTL);
            }
            if (m) perf_press(PF_RND, 0);
        }
        bad += check("RANDOM: some notes moved (octave, fifth, fourth), none past an octave; none without it",
                     moved > 3u && moved < 100u && !far);
    }
    /* ARP TRNS: a key (F4: +5 from C4) transposes the track's sequence, plays nothing itself */
    {
        uint32_t seen = 0, wrong = 0, v;
        song_setup();
        t1->p[P_AMODE] = AM_TRNS;
        input_on(t1, 65, 100);
        input_off(t1, 65);
        transport_req = 1;
        for (i = 0; i < 4u * 22016u; i += CTL) {
            mix_block(o, CTL);
            for (v = 0; v < NVOICE; v++)
                if (t1->v[v].active && t1->v[v].gate) {
                    seen |= t1->v[v].note == 50u;           /* (step 1: A2 45, +5) */
                    wrong |= t1->v[v].note == 45u || t1->v[v].note == 65u;
                }
        }
        transport_req = 2;
        mix_block(o, CTL);
        bad += check("ARP TRNS: a key transposes the sequence by its interval from C4 (kept when let go), no note itself",
                     t1->trn == 5 && seen && !wrong);
    }
    /* the punch-in lane: REC armed, OCT+ held over steps 4..7 of the 64: recorded there (its tracks: DRM); the next
     * pass plays it on the drums (DRUM's pitch offset at step 5, none at step 10), not on the synths; stopped,
     * FX + G5 clears it */
    {
        uint32_t s16, st, ok = 1, pit5 = 0, pit10 = 1, syn = 0;
        song_setup();
        memset(pfx_lane, 0, sizeof pfx_lane);
        s16 = beat_samples() / 4u;
        song.rec = 1;
        pfx_tgt = 2;
        transport_req = 1;
        for (i = 0; i < 64u * s16; i += CTL) {
            st = (i / s16) & 63u;
            if (st == 4u && !(perf_held & PF_BIT(PF_OCTU))) perf_press(PF_OCTU, 1);
            if (st == 8u && (perf_held & PF_BIT(PF_OCTU))) perf_press(PF_OCTU, 0);
            mix_block(o, CTL);
        }
        song.rec = 0;
        pfx_tgt = 0;
        for (st = 0; st < 64u; st++)
            ok &= pfx_lane_at(st) == (st >= 4u && st < 8u ? (uint32_t)(PF_OCTU - PF_OCTD + 1) : 0u);
        ok &= pfx_ltgt == 2u;
        for (i = 64u * s16; i < 128u * s16; i += CTL) {
            st = ((i - 64u * s16) / s16) & 63u;
            mix_block(o, CTL);
            if (st == 5u) pit5 = td->pfx_pit == 192, syn |= t1->pfx_pit != 0;
            if (st == 10u) pit10 = td->pfx_pit;
        }
        transport_req = 2;
        mix_block(o, CTL);
        bad += check("punch-in lane: REC + OCT+ over steps 4..7 recorded (its tracks DRM); played back on the drums only",
                     ok && pit5 && !pit10 && !syn);
        perf_press(PF_CLR, 1);
        mix_block(o, CTL);
        perf_press(PF_CLR, 0);
        for (ok = 1, st = 0; st < 64u; st++) ok &= !pfx_lane_at(st);
        bad += check("  stopped, FX + G5 clears the lane", ok);
    }
    /* (0.6.99) FX > PUNCH: LEN 16 of 1/8: the lane 16 steps of two sixteenths; a press over sixteenths 12..15 lands on
     * steps 6 and 7, and the second pass (sixteenths 32..) plays it there again (the lane wraps at 16 of 1/8 = 32
     * sixteenths); stopped, CLEAR empties it and the defaults come back */
    {
        uint32_t s16, st, ok = 1, pit6 = 0, pit0 = 1;
        song_setup();
        memset(pfx_lane, 0, sizeof pfx_lane);
        song.g[G_PFXLEN] = 16; song.g[G_PFXRES] = 0;
        s16 = beat_samples() / 4u;
        song.rec = 1;
        pfx_tgt = 0;
        transport_req = 1;
        for (i = 0; i < 32u * s16; i += CTL) {
            st = (i / s16) & 63u;
            if (st == 12u && !(perf_held & PF_BIT(PF_OCTU))) perf_press(PF_OCTU, 1);
            if (st == 16u && (perf_held & PF_BIT(PF_OCTU))) perf_press(PF_OCTU, 0);
            mix_block(o, CTL);
        }
        song.rec = 0;
        ok &= pfx_lane_at(6) == (uint32_t)(PF_OCTU - PF_OCTD + 1) && pfx_lane_at(7) == (uint32_t)(PF_OCTU - PF_OCTD + 1) &&
              !pfx_lane_at(5) && !pfx_lane_at(8) && !pfx_lane_at(16);
        for (i = 32u * s16; i < 64u * s16; i += CTL) {  /* the second pass of the 16 */
            st = ((i - 32u * s16) / s16) & 63u;
            mix_block(o, CTL);
            if (st == 13u) pit6 = t1->pfx_pit == 192;
            if (st == 2u) pit0 = t1->pfx_pit;
        }
        transport_req = 2;
        mix_block(o, CTL);
        if (!(ok && pit6 && !pit0)) printf("  punch LEN 16 of 1/8: lane %u %u %u, pit %u %u\n", pfx_lane_at(5), pfx_lane_at(6), pfx_lane_at(7), pit6, pit0);
        bad += check("PUNCH LEN 16 RES 1/8: a press over sixteenths 12..15 on steps 6 7, played again on the lane's second pass", ok && pit6 && !pit0);
        song.g[G_PFXLEN] = 64; song.g[G_PFXRES] = 1;
    }
    return bad;
}

/* ------------------------------------- 10. sweeps, GLITCH, TAPE STOP --- */
static void blocks(uint32_t t0, uint32_t t1)       /* perf_begin + perf_block on the test signal, as run() */
{
    uint32_t t;
    for (t = t0; t < t1; t += CTL) {
        memcpy(out_l + t % NT, in_l + t % NT, CTL * 4);
        memcpy(out_r + t % NT, in_r + t % NT, CTL * 4);
        clk_pos = t % div_samples(2);
        clk_n = t / div_samples(2);
        if (perf_begin(CTL))
            perf_block(out_l + t % NT, out_r + t % NT, CTL);
    }
}
static void fresh(uint32_t bpm)
{
    host_tracks_init();
    perf_reset();
    song.g[G_BPM] = (int16_t)bpm;
    song.playing = 1;
    perf_start();
}
static int test_sweeps(void)
{
    int bad = 0, ok;
    char what[200];
    uint32_t bar = 4u * 22050u, t, k;
    int32_t q, h, e, f;
    test_signal(440, 12000, 277, 9000);
    fresh(120);                                     /* (0.9.17: no FILTER sweep) the FILTER macro: the DJ FILTER's low-pass */
    perf_m[3] = 50;
    blocks(0, CTL);
    ok = perf_djf == -32;
    perf_m[3] = 100;
    blocks(CTL, 2u * CTL);
    ok &= perf_djf == -64;
    perf_m[3] = 0;
    blocks(2u * CTL, 3u * CTL);
    ok &= !perf_djf;
    bad += check("FILTER macro (the layer's one filter, 0.9.17): 50 % -> DJ FILTER -32, 100 % -> -64, off -> 0", ok);
    (void)q; (void)h; (void)e; (void)f;
    {   /* CRUSH: the bits and the rate fall as it rises; let go, a beat and the ramp later the input bit for bit */
        ev_t ev[] = {{0, PF_SCRU + 1}, {2u * 88200u, -(PF_SCRU + 1)}};
        int32_t e1, e2;
        run(120, 1, ev, 2, 2u * 88200u + 30000u);
        shift_d = 0;
        e1 = err_vs(40000, 44000, delayed, 0);
        e2 = err_vs(170000, 176000, delayed, 0);
        snprintf(what, sizeof what, "CRUSH sweep: the error grows (%d half a bar in, %d after 2 bars); let go: dry after a beat", e1, e2);
        bad += check(what, e1 < e2 && e2 > 1500 && same(2u * 88200u + 22050u + 2u * CTL, 2u * 88200u + 30000u) && !pf.busy);
    }
    /* GLITCH: the same slice at the same place of the transport whenever it is pressed; every kind; no click */
    {
        static uint8_t ga[2][64], seen[2][64];
        static int32_t o1[NT];
        uint32_t r, n[8] = {0}, agree = 1, both = 0;
        for (r = 0; r < 2u; r++) {
            fresh(120);
            test_signal(440, 12000, 277, 9000);
            for (t = 0; t < 48u * P16; t += CTL) {
                if (t == (r ? 1234u * CTL : 992u))                 /* (steps 2 and 8) */
                    perf_press(PF_GLT, 1);
                blocks(t, t + CTL);
                if (pf.mode == BM_GLT && pf.gs < 48u && pf.gs != (r ? 8u : 1u)) {   /* (its first: live, nothing before) */
                    ga[r][pf.gs] = pf.u.g.a;
                    seen[r][pf.gs] = 1;
                }
            }
            if (!r)
                memcpy(o1, out_l, sizeof o1);
            perf_press(PF_GLT, 0);
        }
        for (k = 0; k < 48u; k++)
            if (seen[0][k] && seen[1][k]) {
                both++;
                agree &= ga[0][k] == ga[1][k];
            }
        for (k = 2; k < 34u; k++) {
            n[ga[0][k] % 8u]++;
            agree &= ga[0][k] == ga[0][k + 32u] || k + 32u >= 48u;
        }
        snprintf(what, sizeof what, "GLITCH: pressed in step 1 or 8, the same slices at the same steps (%u compared), "
                 "a 2-bar cycle; in 2 bars: live %u repeat %u reverse %u up %u down %u gate %u skip %u crush %u", both,
                 n[0], n[1], n[2], n[3], n[4], n[5], n[6], n[7]);
        bad += check(what, agree && both >= 36u && n[0] >= 8u && n[0] <= 22u && n[1] && n[1] + n[2] + n[3] + n[4] >= 6u &&
                     n[5] + n[6] + n[7] >= 3u);
        fresh(120);
        test_signal(440, 12000, 277, 9000);
        perf_press(PF_GLT, 1);
        blocks(0, 992u);
        perf_press(PF_GLT, 0);
        perf_reset();
        perf_start();
        for (t = 0; t < 48u * P16; t += CTL) {
            if (t == 992u)
                perf_press(PF_GLT, 1);
            blocks(t, t + CTL);
        }
        perf_press(PF_GLT, 0);
        bad += check("  the same press twice: the same output, sample for sample", !memcmp(o1, out_l, 48u * P16 * 4u));
        {   /* the downbeats mostly live; the slices' audio from the step before (a reverse is the last 1/16 backwards) */
            uint32_t live4 = 0;
            for (k = 4; k < 48u; k += 4u) live4 += ga[0][k] == GA_PASS;
            bad += check("  the beats mostly left live (the groove)", live4 >= 7u);
        }
    }
    {   /* TAPE STOP: the pitch falls over a beat, then silence; let go: live */
        uint32_t c0, c1;
        int32_t tail;
        fresh(120);
        test_signal(880, 12000, 880, 12000);
        perf_press(PF_TAPE, 1);
        blocks(0, 40000u);
        c0 = zc(out_l, 5512u + 100u, 5512u + 5512u);
        c1 = zc(out_l, 5512u + 11025u, 5512u + 16537u);
        tail = peak(out_l, 5512u + 22050u + 256u, 40000u);
        perf_press(PF_TAPE, 0);
        blocks(40000u, 50000u);
        snprintf(what, sizeof what, "TAPE STOP: the first eighth %u crossings, the third %u (slowing), silent after a beat (%d)",
                 c0, c1, tail);
        bad += check(what, c1 * 10u < c0 * 7u && c1 > 0u && tail < 300 && same(40000u + 256u, 50000u));
    }
    {   /* the lane: CRUSH held over steps 4..7 recorded, played back the next pass; GLITCH and TAPE too */
        uint32_t s16, st, i, ok2 = 1, on5 = 0, on10 = 1;
        int32_t o[2 * CTL];
        song_setup();
        memset(pfx_lane, 0, sizeof pfx_lane);
        s16 = beat_samples() / 4u;
        song.rec = 1;
        transport_req = 1;
        for (i = 0; i < 64u * s16; i += CTL) {
            st = (i / s16) & 63u;
            if (st == 4u && !(perf_held & PF_BIT(PF_SCRU))) perf_press(PF_SCRU, 1);
            if (st == 8u && (perf_held & PF_BIT(PF_SCRU))) perf_press(PF_SCRU, 0);
            if (st == 20u && !(perf_held & PF_BIT(PF_GLT))) perf_press(PF_GLT, 1);
            if (st == 22u && (perf_held & PF_BIT(PF_GLT))) perf_press(PF_GLT, 0);
            if (st == 30u && !(perf_held & PF_BIT(PF_TAPE))) perf_press(PF_TAPE, 1);
            if (st == 31u && (perf_held & PF_BIT(PF_TAPE))) perf_press(PF_TAPE, 0);
            mix_block(o, CTL);
        }
        song.rec = 0;
        for (st = 0; st < 64u; st++)
            ok2 &= pfx_lane_at(st) == (st >= 4u && st < 8u ? 12u : st >= 20u && st < 22u ? 14u : st == 30u ? 15u : 0u);
        for (i = 64u * s16; i < 128u * s16; i += CTL) {
            st = ((i - 64u * s16) / s16) & 63u;
            mix_block(o, CTL);
            if (st == 5u) on5 = (perf_act & PF_BIT(PF_SCRU)) && pf.sw[0];
            if (st == 10u) on10 = (perf_act & PF_BIT(PF_SCRU)) != 0u;
            if (st == 20u && i % s16 > 2u * CTL) ok2 &= pf.mode == BM_GLT;   /* (on the lane's own step) */
            if (st == 30u && i % s16 > s16 / 2u) ok2 &= pf.mode == BM_TAPE;
        }
        transport_req = 2;
        mix_block(o, CTL);
        memset(pfx_lane, 0, sizeof pfx_lane);
        bad += check("punch-in lane (0.9.14): a sweep (CRUSH), GLITCH, TAPE STOP recorded (12, 14, 15), played back", ok2 && on5 && !on10);
    }
    return bad;
}

/* ----------------------------------------------- 11. the glitcher (0.9.17) --- */
static void gp_reset(void)
{
    static const int8_t D[4][4] = {{50, 0, 0, 1}, {0, 4, 0, 0}, {60, 2, 20, 1}, {50, 0, 30, 2}};
    memcpy((void *)perf_gp, D, sizeof D);
}
static uint32_t xings(const int32_t *x, uint32_t a, uint32_t b)   /* rising zero crossings, with a +-400 hysteresis */
{
    uint32_t c = 0, up = x[a] > 0;
    for (; a < b; a++)
        if (!up && x[a] > 400) { up = 1; c++; }
        else if (up && x[a] < -400) up = 0;
    return c;
}
static void glide_signal(void)                     /* (SHUFFLE) a tone a 1/16 at 120 BPM, each another, phase on */
{
    double ph = 0;
    uint32_t t;
    for (t = 0; t < NT; t++) {
        ph += 2 * M_PI * 220.0 * pow(2.0, (double)((t / P16) % 7u) * 2.0 / 12.0) / FS;
        in_l[t] = (int32_t)lrint(11000 * sin(ph));
        in_r[t] = (int32_t)lrint(9000 * sin(ph + 0.3));
    }
}
static int test_glitcher(void)
{
    int bad = 0, ok;
    char what[240];
    uint32_t t, k, v;
    /* ---- GLITCH */
    gp_reset();
    test_signal(440, 12000, 277, 9000);
    perf_gp[GX_GLT][0] = 0;
    fresh(120);
    perf_press(PF_GLT, 1);
    blocks(0, 40u * P16);
    perf_press(PF_GLT, 0);
    blocks(40u * P16, 41u * P16);
    bad += check("GLITCH DENS 0: no 1/16 glitched, the input bit for bit", same(0, 41u * P16));
    {
        uint32_t pass = 0, n = 0;
        perf_gp[GX_GLT][0] = 100;
        fresh(120);
        perf_press(PF_GLT, 1);
        for (t = 0; t < 40u * P16; t += CTL) {
            blocks(t, t + CTL);
            if (pf.mode == BM_GLT && pf.gs >= 3u && pf.gph < CTL) {
                n++;
                pass += pf.u.g.a == GA_PASS;
            }
        }
        perf_press(PF_GLT, 0);
        snprintf(what, sizeof what, "GLITCH DENS 100: every 1/16 glitched once there is a 1/16 to read (%u of %u live)", pass, n);
        bad += check(what, !pass && n >= 30u);
    }
    {
        static uint8_t A[3][64];
        static const int8_t VR[3] = {1, 2, 1};
        memset(A, 0xFF, sizeof A);
        for (v = 0; v < 3u; v++) {
            gp_reset();
            perf_gp[GX_GLT][3] = VR[v];
            fresh(120);
            perf_press(PF_GLT, 1);
            for (t = 0; t < 40u * P16; t += CTL) {
                blocks(t, t + CTL);
                if (pf.mode == BM_GLT && pf.gs >= 3u && pf.gs < 64u && pf.gph < CTL)
                    A[v][pf.gs] = pf.u.g.a;
            }
            perf_press(PF_GLT, 0);
        }
        bad += check("GLITCH VARI: VARI 2 another pattern, VARI 1 again the same one", !memcmp(A[0], A[2], 64) && memcmp(A[0], A[1], 64));
    }
    {   /* PITCH: a repeat's k-th segment k * PITCH semitones up (or down), two octaves at most */
        int32_t r[2][4];
        uint32_t i;
        for (i = 0; i < 2u; i++) {
            perf_gp[GX_GLT][1] = (int8_t)(i ? -12 : 12);
            pf.u.g.a = GA_RPT;
            pf.u.g.l = 1000;
            pf.u.g.g0 = 20000;
            for (k = 0; k < 4u; k++) {
                pf.u.g.k = k;
                perf_glt_seg();
                r[i][k] = pf.u.g.rate;
            }
        }
        snprintf(what, sizeof what, "GLITCH PITCH +12: the repeats at x%.2f x%.2f x%.2f x%.2f; -12: x%.2f x%.2f x%.2f x%.2f",
                 r[0][0] / 65536.0, r[0][1] / 65536.0, r[0][2] / 65536.0, r[0][3] / 65536.0, r[1][0] / 65536.0,
                 r[1][1] / 65536.0, r[1][2] / 65536.0, r[1][3] / 65536.0);
        bad += check(what, r[0][0] == 65536 && r[0][1] == 131072 && r[0][2] == 262144 && r[0][3] == 262144 &&
                     r[1][0] == 65536 && r[1][1] == 32768 && r[1][2] == 16384 && r[1][3] == 16384);
        gp_reset();
    }
    {   /* every SIZE and PITCH on a full-scale input: bounded */
        int32_t pk = 0;
        for (t = 0; t < NT; t++) {
            in_l[t] = (t / 37u) & 1u ? 120000 : -120000;
            in_r[t] = (t / 53u) & 1u ? 120000 : -120000;
        }
        for (v = 0; v < 8u; v++) {
            gp_reset();
            perf_gp[GX_GLT][0] = 100;
            perf_gp[GX_GLT][1] = (int8_t)(v & 1u ? 12 : -12);
            perf_gp[GX_GLT][2] = (int8_t)(v / 2u);
            fresh(120);
            perf_press(PF_GLT, 1);
            blocks(0, 24u * P16);
            perf_press(PF_GLT, 0);
            pk = peak(out_l, 0, 24u * P16) > pk ? peak(out_l, 0, 24u * P16) : pk;
            pk = peak(out_r, 0, 24u * P16) > pk ? peak(out_r, 0, 24u * P16) : pk;
        }
        snprintf(what, sizeof what, "GLITCH every SIZE, PITCH +-12, DENS 100, 4x full scale in: peak %d (the input's 120000)", pk);
        bad += check(what, pk <= 140000);
        gp_reset();
    }
    /* ---- SHUFFLE */
    test_signal(440, 12000, 277, 9000);
    perf_gp[GX_SHUF][0] = 0;
    fresh(120);
    perf_press(PF_SHUF, 1);
    blocks(0, 40u * P16);
    perf_press(PF_SHUF, 0);
    blocks(40u * P16, 41u * P16);
    bad += check("SHUFFLE AMOUNT 0: nothing moved, the input bit for bit", same(0, 41u * P16));
    {   /* AMOUNT 100, SPEED 0: each moved 1/16 is one of the bar's, as it was (ADPCM, mono): its SNR */
        static uint8_t kd[2][64], kk[2][64];
        static int32_t o1[NT];
        double se = 0, sr = 0, wse = 0;
        uint32_t r, moved = 0, beat = 0, far = 0, agree = 1, cmp = 0;
        for (r = 0; r < 2u; r++) {
            gp_reset();
            perf_gp[GX_SHUF][0] = 100;
            perf_gp[GX_SHUF][2] = 0;
            glide_signal();
            fresh(120);
            memset(kd[r], 0xFF, 64);
            for (t = 0; t < 52u * P16; t += CTL) {
                if (t == (r ? (7u * P16 / CTL + 10u) * CTL : 992u))
                    perf_press(PF_SHUF, 1);
                blocks(t, t + CTL);
                if (pf.mode == BM_SHUF && pf.gph < CTL && pf.gs < 64u) {
                    kd[r][pf.gs] = pf.u.s.r[0].live ? 0xFEu : (uint8_t)((pf.gs - pf.u.s.j0) & 31u);
                    kk[r][pf.gs] = pf.u.s.kind;
                }
            }
            perf_press(PF_SHUF, 0);
            if (!r)
                memcpy(o1, out_l, sizeof o1);
        }
        for (k = 26u; k < 50u; k++) {               /* (both with a bar behind them) */
            cmp++;
            agree &= kd[0][k] == kd[1][k] && kk[0][k] == kk[1][k];
        }
        for (k = 18u; k < 50u; k++) {
            uint32_t d = kd[0][k], a = k * P16 + 256u, b = (k + 1u) * P16 - 192u, lag;
            if (d == 0xFEu || d == 0xFFu)
                continue;
            moved++;
            beat += d % 4u == 0u;
            far += d > 8u;
            {   /* (the best of a few samples' lag: the coder's half rate and its reader's interpolation, ~4) */
                double be = 0, bq = 1;
                for (lag = 0; lag < 8u; lag++) {
                    double e = 0, q = 0;
                    for (t = a; t < b; t++) {
                        double ref = (in_l[t - d * P16 - lag] + in_r[t - d * P16 - lag]) / 2.0, x = o1[t] - ref;
                        e += x * x;
                        q += ref * ref;
                    }
                    if (!lag || e / q < be / bq) {
                        be = e;
                        bq = q;
                    }
                }
                se += be;
                sr += bq;
                wse = be / bq > wse ? be / bq : wse;   /* (the worst 1/16) */
            }
        }
        snprintf(what, sizeof what, "SHUFFLE AMOUNT 100: %u of 32 1/16s moved (%u by whole beats, %u from past half a bar), "
                 "each the bar's own 1/16 (SNR %.1f dB, the worst %.1f: mono 4-bit)", moved, beat, far, 10 * log10(sr / (se ? se : 1)),
                 -10 * log10(wse ? wse : 1e-9));
        bad += check(what, moved >= 20u && beat && far && 10 * log10(sr / (se ? se : 1)) > 28.0 && -10 * log10(wse ? wse : 1e-9) > 24.0);
        snprintf(what, sizeof what, "  pressed on step 2 or 8: the same chunks once both have a bar (%u compared)", cmp);
        bad += check(what, agree && cmp == 24u);
    }
    {   /* SPEED 100, every SIZE, 4x full scale: bounded; at 70 BPM too (less of the bar fits) */
        int32_t pk = 0;
        uint32_t kinds = 0;
        for (t = 0; t < NT; t++) {
            in_l[t] = (t / 37u) & 1u ? 120000 : -120000;
            in_r[t] = (t / 53u) & 1u ? 120000 : -120000;
        }
        for (v = 0; v < 4u; v++) {
            gp_reset();
            perf_gp[GX_SHUF][0] = 100;
            perf_gp[GX_SHUF][2] = 100;
            perf_gp[GX_SHUF][1] = (int8_t)(v % 3u);
            fresh(v == 3u ? 70 : 120);
            perf_press(PF_SHUF, 1);
            for (t = 0; t < 48u * P16; t += CTL) {
                blocks(t, t + CTL);
                kinds |= 1u << pf.u.s.kind;
            }
            perf_press(PF_SHUF, 0);
            pk = peak(out_l, 0, 48u * P16) > pk ? peak(out_l, 0, 48u * P16) : pk;
            pk = peak(out_r, 0, 48u * P16) > pk ? peak(out_r, 0, 48u * P16) : pk;
        }
        snprintf(what, sizeof what, "SHUFFLE SPEED 100 (half, double, stutter: %s), SIZE 1/4 1/8 1/16, 70 BPM: 4x full scale, peak %d",
                 kinds == 15u ? "all" : "NOT ALL", pk);
        bad += check(what, kinds == 15u && pk <= 140000);
        gp_reset();
    }
    /* ---- FREEZE */
    {
        uint32_t f0, c0, c1;
        int32_t pk, tail;
        for (v = 0; v < 2u; v++) {
            gp_reset();
            perf_gp[GX_FRZ][2] = 0;                 /* (SPRAY 0: every grain at PITCH) */
            perf_gp[GX_FRZ][1] = (int8_t)(v ? 12 : 0);
            test_signal(440, 12000, 440, 12000);
            fresh(120);
            perf_press(PF_FRZ, 1);
            for (t = 0; !pf.state && t < 40000u; t += CTL)
                blocks(t, t + CTL);
            f0 = t;
            for (k = f0; k < NT; k++)
                in_l[k] = in_r[k] = 0;              /* (the input silent once frozen) */
            blocks(f0, f0 + 2u * 44100u);
            if (!v)
                c0 = xings(out_l, f0 + 4410u, f0 + 4410u + 44100u);
            else
                c1 = xings(out_l, f0 + 4410u, f0 + 4410u + 44100u);
            pk = peak(out_l, f0 + 4410u, f0 + 2u * 44100u);
            perf_press(PF_FRZ, 0);
            blocks(f0 + 2u * 44100u, f0 + 2u * 44100u + 4096u);
            tail = peak(out_l, f0 + 2u * 44100u + 512u, f0 + 2u * 44100u + 4096u);
            if (!v) {
                snprintf(what, sizeof what, "FREEZE: an eighth kept (%u samples), held as grains with the input silent: "
                         "peak %d, %u crossings a second (440 Hz)", f0, pk, c0);
                bad += check(what, f0 > 11025u && f0 < 11025u + 6000u && pk > 6000 && pk < 16000 && c0 > 400u && c0 < 480u);
                bad += check("  let go: the (silent) input again", !tail && !pf.busy);
            }
        }
        snprintf(what, sizeof what, "FREEZE PITCH +12: %u crossings a second (880 Hz)", c1);
        bad += check(what, c1 > 820u && c1 < 940u);
        pk = 0;                                     /* 4x full scale, DENS SPRAY 100, every SIZE, PITCH +12 */
        for (t = 0; t < NT; t++) {
            in_l[t] = (t / 37u) & 1u ? 120000 : -120000;
            in_r[t] = (t / 53u) & 1u ? 120000 : -120000;
        }
        for (v = 0; v < 5u; v++) {
            gp_reset();
            perf_gp[GX_FRZ][0] = 100;
            perf_gp[GX_FRZ][1] = 12;
            perf_gp[GX_FRZ][2] = 100;
            perf_gp[GX_FRZ][3] = (int8_t)v;
            fresh(120);
            perf_press(PF_FRZ, 1);
            blocks(0, 60000u);
            perf_press(PF_FRZ, 0);
            pk = peak(out_l, 0, 60000u) > pk ? peak(out_l, 0, 60000u) : pk;
        }
        snprintf(what, sizeof what, "FREEZE every SIZE, DENS and SPRAY 100, PITCH +12, 4x full scale: peak %d", pk);
        bad += check(what, pk <= 150000);
        gp_reset();
    }
    /* ---- TAPE STOP: TIME, CURVE; the scratch */
    {
        int32_t pk[3], tl[3], mv, rest, mx = 0;
        static const int8_t CV[3] = {0, -100, 100};
        test_signal(880, 12000, 880, 12000);
        for (v = 0; v < 3u; v++) {
            gp_reset();
            perf_gp[GX_TAPE][1] = 2;                /* (TIME 1/16: 5512 samples) */
            perf_gp[GX_TAPE][2] = CV[v];
            fresh(120);
            perf_press(PF_TAPE, 1);
            blocks(0, 2u * P16 + 512u);
            pk[v] = peak(out_l, 0, P16 / 2u);
            tl[v] = peak(out_l, P16 + 64u, 2u * P16 + 512u);
            perf_press(PF_TAPE, 0);
        }
        snprintf(what, sizeof what, "TAPE STOP TIME 1/16: stopped within it, CURVE 0 / -100 / +100 (silent after: %d %d %d)",
                 tl[0], tl[1], tl[2]);
        bad += check(what, pk[0] > 8000 && !tl[0] && !tl[1] && !tl[2] && pf.state);
        gp_reset();
        fresh(120);                                 /* the scratch: KNOB 1 a detent every 8 blocks, on, back, then at rest */
        test_signal(440, 12000, 440, 12000);
        perf_press(PF_TAPE, 1);
        blocks(0, 30000u);
        ok = pf.mode == BM_TAPE && pf.state;
        for (t = 30000u; t < 30000u + 512u * CTL; t += CTL) {
            uint32_t b = (t - 30000u) / CTL;
            if (b % 8u == 0u && b < 384u)
                perf_scr += b < 192u ? 1u : (uint32_t)-1;
            blocks(t, t + CTL);
        }
        mv = peak(out_l, 30000u, 30000u + 384u * CTL);
        rest = peak(out_l, 30000u + 448u * CTL, 30000u + 512u * CTL);
        for (t = 30001u; t < 30000u + 512u * CTL; t++)
            mx = abs(out_l[t] - out_l[t - 1]) > mx ? abs(out_l[t] - out_l[t - 1]) : mx;
        perf_press(PF_TAPE, 0);
        snprintf(what, sizeof what, "TAPE stopped, KNOB 1 the hand: the record moves (peak %d), at rest silent (%d), "
                 "no click (largest step %d)", mv, rest, mx);
        bad += check(what, ok && mv > 3000 && rest < 64 && mx < 1500);
        gp_reset();
    }
    /* ---- the knobs follow the glitcher whose key is down */
    perf_reset();
    perf_press(PF_GLT, 1);
    ok = perf_gfocus() == PF_GLT;
    perf_press(PF_FRZ, 1);
    ok &= perf_gfocus() == PF_FRZ;
    perf_press(PF_FRZ, 0);
    ok &= perf_gfocus() == PF_GLT;
    perf_press(PF_GLT, 0);
    ok &= perf_gfocus() == PF_N;
    perf_latch_on = 1;
    perf_press(PF_SHUF, 1);
    ok &= perf_gfocus() == PF_SHUF && perf_latched == PF_BIT(PF_SHUF);
    perf_press(PF_SHUF, 0);
    ok &= perf_gfocus() == PF_N && perf_latched == PF_BIT(PF_SHUF);
    perf_press(PF_R16, 1);
    ok &= perf_gfocus() == PF_N;
    perf_press(PF_R16, 0);
    perf_latch_on = 0;
    perf_reset();
    bad += check("the knobs: the glitcher whose key is down (the last pressed), FX LATCH too; REPEAT none", ok);
    /* ---- the lane: SHUFFLE recorded (11, the FILTER sweep's), played back; FREEZE not */
    {
        uint32_t s16, st, i, ok2 = 1, on = 0;
        int32_t o[2 * CTL];
        song_setup();
        memset(pfx_lane, 0, sizeof pfx_lane);
        s16 = beat_samples() / 4u;
        song.rec = 1;
        transport_req = 1;
        for (i = 0; i < 64u * s16; i += CTL) {
            st = (i / s16) & 63u;
            if (st == 40u && !(perf_held & PF_BIT(PF_SHUF))) perf_press(PF_SHUF, 1);
            if (st == 44u && (perf_held & PF_BIT(PF_SHUF))) perf_press(PF_SHUF, 0);
            if (st == 50u && !(perf_held & PF_BIT(PF_FRZ))) perf_press(PF_FRZ, 1);
            if (st == 52u && (perf_held & PF_BIT(PF_FRZ))) perf_press(PF_FRZ, 0);
            mix_block(o, CTL);
        }
        song.rec = 0;
        for (st = 0; st < 64u; st++)
            ok2 &= pfx_lane_at(st) == (st >= 40u && st < 44u ? 11u : 0u);
        for (i = 64u * s16; i < 128u * s16; i += CTL) {
            st = ((i - 64u * s16) / s16) & 63u;
            mix_block(o, CTL);
            if (st == 42u) on |= pf.mode == BM_SHUF;
            if (st == 51u) ok2 &= pf.mode != BM_FRZ;
        }
        transport_req = 2;
        mix_block(o, CTL);
        memset(pfx_lane, 0, sizeof pfx_lane);
        bad += check("punch-in lane (0.9.17): SHUFFLE recorded (11), played back on its steps; FREEZE not recorded", ok2 && on);
    }
    /* ---- at rest: once a glitcher is let go and stopped, the silence path again (0.9.16) */
    {
        int32_t o[2 * CTL];
        uint32_t i, h0;
        song_setup();
        transport_req = 2;
        for (i = 0, h0 = pt_hushed; i < 20u * FS && pt_hushed - h0 < 64u; i += CTL)   /* (every tail out first) */
            mix_block(o, CTL);
        ok = pt_hushed - h0 >= 64u;
        for (v = 0; v < 4u; v++) {
            static const uint8_t G[4] = {PF_GLT, PF_TAPE, PF_SHUF, PF_FRZ};
            perf_press(G[v], 1);
            for (i = 0; i < FS / 2u; i += CTL)
                mix_block(o, CTL);
            ok &= pf.mode != BM_NONE;
            perf_press(G[v], 0);
            for (i = 0; i < FS / 4u; i += CTL)
                mix_block(o, CTL);
            h0 = pt_hushed;
            for (i = 0; i < FS / 10u; i += CTL)
                mix_block(o, CTL);
            ok &= pf.mode == BM_NONE && !pf.busy && !sl_lent && !perf_begin(CTL) && pt_hushed - h0 >= FS / 10u / CTL;
        }
        bad += check("at rest: each glitcher let go (stopped): the layer idle, the buffer back, the silence path every block", ok);
    }
    gp_reset();
    return bad;
}

int main(int argc, char **argv)
{
    int bad = 0;
    bad += test_timing();
    bad += test_stereo();
    bad += test_too_long();
    bad += test_slicer();
    bad += test_keys();
    bad += test_idle();
    bad += test_misc();
    bad += test_pfx();
    bad += test_sweeps();
    bad += test_glitcher();
    bad += test_cost();
    if (argc > 1)
        demos(argv[1]);
    printf("%s\n", bad ? "PERFORM TEST FAILED" : "perform test passed");
    return bad != 0;
}
