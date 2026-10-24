/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Juan Ignacio Antonucci (Juan Jiant) */
/* (JIANT 0.6.83) CPU cost on any host with valgrind: the instruction counts of the audio path, counted by callgrind
 * (the same on every run: exact, unlike the time), section by section, for tests/cpu_vg.py to compare with
 * tests/cpu_baseline_vg.txt. The proc_pid_rusage counts of regress / reverb_test / mod_test exist on the Mac only;
 * this is the alarm everywhere else (Linux, CI).
 *   valgrind --tool=callgrind --instr-atstart=no build/host/cpu_vg    (tests/cpu_vg.py runs it)
 * Each section: the state set, a warm-up, then CALLGRIND_ZERO_STATS, NB blocks of mix_block (the whole audio block:
 * the sequencer, the parts, the buses, the master) and CALLGRIND_DUMP_STATS_AT(its name). Sections:
 *   eng/<ENGINE>   one part, 8 notes held (VOICE: 4), the engine's heaviest preset (hostsim.c's list)
 *   eng/ANIMAL.<MODE>  (0.8.3) ANIMAL's five sources at their heaviest: 8 notes, no gaps (GROUP HOLD), every option on
 *   rev/<TYPE>     a pad soaked in the reverb, each REVERB TYPE (ROOM SPRING SHIMR RESO CLOUD)
 *   dly/<RATE>     (0.8.4) a pad into the delay, its line at the full rate, half, a quarter (fx.c dly_lo)
 *   mix/full       six parts: four ANALOG / FM6 chords, drums, every FX on
 *   mix/v12        (0.9.12) the second core's 12 voices: six ANALOG / FM6 chords (24 notes held), every FX on, 12 sounding,
 *                  CORES 2 (mc_host): both cores' instructions; mix/v12.c0 core 0's alone (core 1's jobs not counted:
 *                  what the audio ISR waits on); mix/v12.one the same scene on one core (8 sounding)
 *   seq/plain      the sequencer: six tracks of chords / hits on every step at 200 BPM
 *   seq/heavy      .. with two step components on each (ARP SEQ / ECHO x8, RAND / OCT), DIV, LEN, chord glides
 *   seqtick/plain  (0.9.11) the sequencer alone (events_block, no render): seq/plain's six tracks
 *   seqtick/live   .. every track under GLO > LIVE's modifiers (seq_live.c: x2, a LOOP of 4, REV, THIN 30 %, GIRO +1)
 *   song/cputest   SAVE > TOOLS 6 CPU TEST's song (song_dice.c cpu_test_song), playing: what the device is measured on
 *   song/cputest.djf  (0.9.1) .. with the master DJ FILTER on (GLO > MIXER KNOB 4 at -24, a low-pass): its cost on top
 *   fx/<EFFECT>    (0.9.17) the CPU TEST song with an FX layer buffer effect held (REPEAT 1/16, GLITCH, TAPE STOP, its
 *                  SCRATCH, SHUFFLE, FREEZE; .max at their heaviest knobs): the mean of its rounds (cpu_vg.py)
 *   idle/stopped   (0.9.16) that song stopped and every tail out: the audio at rest (both cores' work)
 *   peak/VOICE.EN6 (0.9.0, last) six VOICE EN tracks starting a word in one block: that block's instructions, the worst
 *                  of 24 rounds (cpu_vg.py: not per sample) */
#include <valgrind/callgrind.h>
static int vg_c0;                                   /* (0.9.12) mix/v12.c0: core 1's jobs not counted (fx.c mc_run) */
#define MC_HOST_JOB() do { if (vg_c0) CALLGRIND_TOGGLE_COLLECT; } while (0)
#define UI_TEST_NO_MAIN
#include "ui_test.c"

#define NB 256u                                     /* blocks a section (8192 samples) */
static int32_t vo[2 * CTL];

static uint32_t vg_preset(uint32_t e, const char *n)
{
    uint32_t k;
    for (k = 0; k < ENGINES[e]->npresets; k++)
        if (str_eq(ENGINES[e]->presets[k].name, n))
            return k;
    return 0;
}
static void vg_section(const char *name)
{
    uint32_t b;
    for (b = 0; b < 64u; b++)
        mix_block(vo, CTL);                         /* (warm: the attacks, the glides, the FX lines filling) */
    CALLGRIND_ZERO_STATS;
    CALLGRIND_START_INSTRUMENTATION;
    for (b = 0; b < NB; b++)
        mix_block(vo, CTL);
    CALLGRIND_DUMP_STATS_AT(name);
    CALLGRIND_STOP_INSTRUMENTATION;
}
static void vg_quiet(void)                          /* every part muted but what the section plays */
{
    uint32_t k;
    ui_power_on();
    for (k = 0; k < NTRK; k++)
        trk[k].p[P_MUTE] = 1;
}
static void vg_part(uint32_t k, uint32_t e, uint32_t pr, uint32_t notes)
{
    static const uint8_t N[8] = {48, 52, 55, 59, 62, 65, 69, 72};
    track_t *t = &trk[k];
    uint32_t i;
    set_engine_of(t, e);
    t->engine = t->eng_req;
    apply_preset_to(t, pr);
    t->p[P_MUTE] = 0;
    t->p[P_VOICE] = V_POLY;
    t->p[P_SUS] = 127;
    for (i = 0; i < 512u && (i < 64u || t->xf_on || t->eng_req != t->engine); i++)
        mix_block(vo, CTL);                         /* (the sound load and the engine switch over first: they let held notes go) */
    for (i = 0; i < notes && i < 8u; i++)
        trk_note_on(t, e == ENGI_DRUM ? DRUM_LANE_NOTE[i] : N[i], 100);
}
static void vg_seq(int heavy)
{
    uint32_t k, i;
    ui_power_on();
    for (k = 0; k < NTRK; k++) {
        track_t *t = &trk[k];
        if (k == 5u) set_engine_of(t, ENGI_DRUM);
        else { set_engine_of(t, k == 1u ? ENGI_FM6 : ENGI_ANALOG); apply_preset_to(t, 0); }
        t->engine = t->eng_req;
        t->p[P_VOICE] = V_POLY; t->p[P_SLEN] = 64; t->p[P_GLIDE] = heavy ? 100 : 0; t->p[P_SGATE] = 120;
        t->p[P_MUTE] = 0;
        for (i = 0; i < 64u; i++) {
            step_t *s = &t->step[i];
            memset(s, 0, sizeof *s);
            s->time = ST_NOTE; s->vel = 100;
            if (k == 5u) s->hit = 0xFF;
            else {
                s->n = 4; s->note[0] = (uint8_t)(48 + i % 12); s->note[1] = (uint8_t)(52 + i % 12);
                s->note[2] = (uint8_t)(55 + i % 12); s->note[3] = (uint8_t)(59 + i % 12);
            }
            if (heavy) {
                s->comp = i % 3u == 0u ? CP_ARPQ : CP_ECHO;
                s->carg = s->comp == CP_ARPQ ? CP_ARG(3u, 4u, 3u) : 6u;
                s->comp2 = i % 2u ? CP_RAND : CP_OCT; s->carg2 = i % 2u ? 7u : 5u;
                s->ctime = 5u | (6u << 3);
            }
        }
    }
    song.g[G_BPM] = 200;
    transport_req = 1; ui_input();
}
int main(void)
{
    static const struct { const char *eng, *pre; uint32_t notes; } E[] = {
        {"ANALOG", "FAT BASS", 8}, {"PHASE", "STRING", 8}, {"LOFI", "BYTE BENT", 8}, {"VOICE", "WOW BASS", 4},
        {"WHEEL", "ROCK DRIVE", 8}, {"DRUM", "", 8}, {"NOISE", "METAL", 8}, {"FM6", "BRASS", 8}, {"ANIMAL", "ROAR", 8}};
    static const int16_t AM[AN_NMODE][8] = {           /* (0.8.3) ANIMAL's modes, every option on, no gaps */
        {AN_CMOS, 127, 127, 64, 63, 127, 0, 40}, {AN_PULSE, 127, 127, 26, 63, 127, 0, 22},
        {AN_WHSTL, 127, 127, 40, 63, 127, 0, 22}, {AN_WING, 127, 127, 64, 63, 127, 0, 22},
        {AN_GLOTT, 127, 127, 64, 63, 127, 0, 40}};
    static const char *const RT[5] = {"ROOM", "SPRING", "SHIMR", "RESO", "CLOUD"};
    char name[40];
    uint32_t i, e;
    for (i = 0; i < sizeof E / sizeof E[0]; i++) {
        for (e = 0; e < NENGINES && !str_eq(ENGINES[e]->name, E[i].eng); e++)
            ;
        if (e == NENGINES) continue;
        vg_quiet();
        vg_part(0, e, vg_preset(e, E[i].pre), E[i].notes);
        snprintf(name, sizeof name, "eng/%s", E[i].eng);
        vg_section(name);
        if (!i) {                                   /* (a self-check: the part does sound) */
            uint32_t k, n = 0;
            for (k = 0; k < NVOICE; k++) n += trk[0].v[k].active;
            if (n < 4u) { printf("cpu_vg: the part does not sound (%u voices)\n", n); return 1; }
        }
    }
    for (i = 0; i < AN_NMODE; i++) {                    /* (0.8.3) ANIMAL's five sources */
        uint32_t k;
        vg_quiet();
        set_engine_of(&trk[0], ENGI_ANIMAL);
        trk[0].engine = trk[0].eng_req;
        for (k = 0; k < 8u; k++)
            trk[0].p[P_E0 + k] = AM[i][k];
        trk[0].p[P_MUTE] = 0; trk[0].p[P_VOICE] = V_POLY; trk[0].p[P_SUS] = 127;
        for (k = 0; k < 64u; k++)
            mix_block(vo, CTL);
        for (k = 0; k < 8u; k++)
            trk_note_on(&trk[0], 48 + 4 * k, 100);
        snprintf(name, sizeof name, "eng/ANIMAL.%s", N_ANIMAL_MODE[i]);
        vg_section(name);
    }
    for (i = 0; i < 2u; i++) {                          /* (0.8.5) VOICE singing words (sing.c): 4 voices, a lyric */
        for (e = 0; e < NENGINES && ENGINES[e] != &ENG_FORMANT; e++)
            ;
        vg_quiet();
        lyr_set(0, i ? "strength through the starlight" : "construir estrellas");
        vg_part(0, e, vg_preset(e, i ? "SINGER EN" : "CANTOR ES"), 4);
        trk[0].p[P_VLYR] = 0;
        snprintf(name, sizeof name, "eng/VOICE.%s", i ? "EN" : "ES");
        vg_section(name);
    }
    for (i = 0; i < 5u; i++) {
        vg_quiet();
        song.g[G_RTYPE] = (int16_t)i;
        vg_part(0, ENGI_ANALOG, 0, 4);
        trk[0].p[P_REV] = 127;
        snprintf(name, sizeof name, "rev/%s", RT[i]);
        vg_section(name);
    }
    for (i = 0; i < 3u; i++) {                          /* (0.8.4) the delay's line at each rate (fx.c dly_lo): 634 ms
                                                         * full, 1.03 s half, 2.13 s a quarter (DLY_LEN 32768) */
        static const char *const DN[3] = {"dly/full", "dly/half", "dly/quarter"};
        static const uint8_t DT[3] = {DT_FREE + 40, DT_FREE + 44, DT_FREE + 50};
        vg_quiet();
        song.g[G_DTIME] = DT[i]; song.g[G_DFDBK] = 70; song.g[G_DMOD] = 20;
        vg_part(0, ENGI_ANALOG, 0, 4);
        trk[0].p[P_DLY] = 127;
        vg_section(DN[i]);
    }
    vg_quiet();
    for (i = 0; i < 4u; i++) {
        vg_part(i, i == 1u ? ENGI_FM6 : ENGI_ANALOG, 0, 4);
        trk[i].p[P_DLY] = 60; trk[i].p[P_REV] = 60; trk[i].p[P_CHOR] = 60; trk[i].p[P_DIST] = 40;
    }
    vg_part(5, ENGI_DRUM, 0, 4);
    song.g[G_RTYPE] = 4;
    vg_section("mix/full");
    for (e = 0; e < 3u; e++) {                          /* (0.9.12) twelve voices on two cores; the scene on one */
        static const char *const VN[3] = {"mix/v12.one", "mix/v12", "mix/v12.c0"};
        uint32_t k, nv = 0, live = trk_live;
        vg_quiet();
        trk_live = 6u;
        mc_host = e != 0u;
        for (i = 0; i < NPART; i++) {
            vg_part(i, i & 1u ? ENGI_FM6 : ENGI_ANALOG, 0, 4);
            trk[i].p[P_DLY] = 60; trk[i].p[P_REV] = 60; trk[i].p[P_CHOR] = 60; trk[i].p[P_DIST] = 40;
        }
        song.g[G_RTYPE] = 4;
        vg_c0 = e == 2u;
        vg_section(VN[e]);
        vg_c0 = 0;
        for (i = 0; i < NPART; i++)
            for (k = 0; k < NVOICE; k++)
                nv += trk[i].v[k].active && trk[i].v[k].stage != 4u;
        mc_host = 0;
        trk_live = live;
        if (nv != (e ? 12u : 8u)) { printf("cpu_vg: %s: %u voices sounding\n", VN[e], nv); return 1; }
    }
    vg_seq(0);
    vg_section("seq/plain");
    vg_seq(1);
    vg_section("seq/heavy");
    {   /* (0.9.11) the sequencer alone, plain and under LIVE's modifiers on every track */
        uint32_t b, k, r;
        for (r = 0; r < 2u; r++) {
            vg_seq(0);
            for (k = 0; k < NTRK && r; k++) {
                lv.rate[k] = 2; lv.cut[k] = 7; lv.dn[k] = -3; lv.rot[k] = 1;
                lv.here |= (uint8_t)(1u << k); lv.lt[8] |= (uint8_t)(1u << k);
            }
            live_publish();
            for (b = 0; b < 64u; b++)
                events_block(CTL);
            CALLGRIND_ZERO_STATS;
            CALLGRIND_START_INSTRUMENTATION;
            for (b = 0; b < NB; b++)
                events_block(CTL);
            CALLGRIND_DUMP_STATS_AT(r ? "seqtick/live" : "seqtick/plain");
            CALLGRIND_STOP_INSTRUMENTATION;
        }
    }
    ui_power_on();
    cpu_test_song();
    transport_req = 1; ui_input();
    vg_section("song/cputest");
    song.g[G_DJF] = -24;                                /* (0.9.1) the same song through the DJ FILTER (a low-pass, ~1.4 kHz) */
    vg_section("song/cputest.djf");
    song.g[G_DJF] = 0;
    {   /* (0.9.17) fx/<EFFECT>: the same song with an FX layer buffer effect held, rounds of NB blocks (cpu_vg.py: their
         * mean): REPEAT 1/16 (for scale), the glitcher at its defaults and at its heaviest (.max) */
        static const struct { const char *n; uint8_t e, rounds, k, v; uint16_t pre; } FXS[] = {
            {"fx/R16", PF_R16, 4, 0, 0, 64}, {"fx/GLITCH", PF_GLT, 8, 0, 0, 64},
            {"fx/GLITCH.max", PF_GLT, 8, 0, 100, 64}, {"fx/TAPE", PF_TAPE, 1, 0, 0, 8},
            {"fx/SCRATCH", PF_TAPE, 4, 1, 0, 1200}, {"fx/SHUFFLE", PF_SHUF, 8, 0, 0, 900},
            {"fx/SHUFFLE.max", PF_SHUF, 8, 0, 100, 900}, {"fx/FREEZE", PF_FRZ, 4, 0, 0, 400},
            {"fx/FREEZE.max", PF_FRZ, 4, 0, 100, 400}};
        uint32_t f, r, b;
        for (f = 0; f < sizeof FXS / sizeof FXS[0]; f++) {
            if (FXS[f].v) {                             /* (.max: GLITCH DENS 100 PITCH +12 TINY; SHUFFLE AMOUNT and SPEED
                                                         * 100; FREEZE DENS 100 (4 grains) SPRAY 100 PITCH +12) */
                perf_gp[GX_GLT][0] = 100; perf_gp[GX_GLT][1] = 12; perf_gp[GX_GLT][2] = 3;
                perf_gp[GX_SHUF][0] = 100; perf_gp[GX_SHUF][2] = 100;
                perf_gp[GX_FRZ][0] = 100; perf_gp[GX_FRZ][1] = 12; perf_gp[GX_FRZ][2] = 100;
            }
            perf_press(FXS[f].e, 1);
            for (b = 0; b < FXS[f].pre; b++)
                mix_block(vo, CTL);
            for (r = 0; r < FXS[f].rounds; r++) {
                CALLGRIND_ZERO_STATS;
                CALLGRIND_START_INSTRUMENTATION;
                for (b = 0; b < NB; b++) {
                    if (FXS[f].k && b % 4u == 0u)   /* (SCRATCH: KNOB 1 a detent every 4 blocks, on and back) */
                        perf_scr += (b / 64u) & 1u ? (uint32_t)-1 : 1u;
                    mix_block(vo, CTL);
                }
                snprintf(name, sizeof name, "%s.r%02u", FXS[f].n, r);
                CALLGRIND_DUMP_STATS_AT(name);
                CALLGRIND_STOP_INSTRUMENTATION;
            }
            perf_press(FXS[f].e, 0);
            for (b = 0; b < 64u; b++)
                mix_block(vo, CTL);
            if (pf.mode != BM_NONE) { printf("cpu_vg: %s: the effect still on after it was let go\n", FXS[f].n); return 1; }
            perf_gp[GX_GLT][0] = 50; perf_gp[GX_GLT][1] = 0; perf_gp[GX_GLT][2] = 0;
            perf_gp[GX_SHUF][0] = 60; perf_gp[GX_SHUF][2] = 20;
            perf_gp[GX_FRZ][0] = 50; perf_gp[GX_FRZ][1] = 0; perf_gp[GX_FRZ][2] = 30;
        }
    }
    {   /* (0.9.16) idle/stopped: the CPU TEST song stopped, 20 s on (every tail out): the audio at rest, both cores'
         * work (the silence path, fx.c mix_hush: the events, the LFOs, the engines' block work; 0.9.14: ~500) */
        uint32_t b;
        song.master_q12 = 4096;
        transport_req = 2; ui_input();
        mc_host = 1;
        for (b = 0; b < 28000u; b++) {
            if (b % 32u == 0u) { ui_input(); host_ticks += 20000u; fm1_ms += 20u; }
            mix_block(vo, CTL);
        }
        vg_section("idle/stopped");
        mc_host = 0;
    }
    {   /* (0.9.0) peak/VOICE.EN6: six VOICE EN tracks starting a word in the same block (TEXT of one-syllable words on
         * three, W1 W9 S3 on the others), 24 times: the instructions of that one block, the worst of them */
        static const char *const TX[3] = {"love me now we go home", "strength through the night", "stay and fly so high"};
        static const uint8_t LY[3] = {1, 9, LYR_NW + 3u};
        uint32_t r, k, b, live = trk_live;
        for (e = 0; e < NENGINES && ENGINES[e] != &ENG_FORMANT; e++)
            ;
        vg_quiet();
        trk_live = 6u;                            /* (TRACKS 6: all six parts play) */
        for (k = 0; k < 6u; k++) {
            if (k < 3u)
                lyr_set(k, TX[k]);
            vg_part(k, e, vg_preset(e, "SINGER EN"), 0);
            trk[k].p[P_VLANG] = LG_EN;
            trk[k].p[P_VLYR] = (int16_t)(k < 3u ? 0 : LY[k - 3u]);
        }
        for (r = 0; r < 24u; r++) {
            for (k = 0; k < 6u; k++)
                trk_note_on(&trk[k], 48 + 2 * k + r % 5u, 100);
            CALLGRIND_ZERO_STATS;
            CALLGRIND_START_INSTRUMENTATION;
            mix_block(vo, CTL);                     /* (the block that starts the words: sing_block's sy_next) */
            snprintf(name, sizeof name, "peak/VOICE.EN6.r%02u", r);
            CALLGRIND_DUMP_STATS_AT(name);
            CALLGRIND_STOP_INSTRUMENTATION;
            for (b = 0; b < 40u; b++)
                mix_block(vo, CTL);
            for (k = 0; k < 6u; k++)
                trk_note_off(&trk[k], 48 + 2 * k + r % 5u);
            for (b = 0; b < 12u; b++)
                mix_block(vo, CTL);
        }
        for (k = 0; k < 6u; k++)                    /* (a self-check: each track sang its 24 syllables) */
            if (sing[k].nsy < 24u) { printf("cpu_vg: peak/VOICE.EN6: track %u sang %u syllables\n", k, sing[k].nsy); return 1; }
        trk_live = live;                            /* (the sections after as they were) */
    }
    return 0;
}
