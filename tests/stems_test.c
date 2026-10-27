/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments
 * Copyright (C) 2026 Juan Ignacio Antonucci (Juan Jiant) */
/* (0.9.8) MENU > USB STEMS, the render side (fx.c stem_buf / stem_fx, what usb.c uac_tap8 sends), on the host build of
 * the UI tests: dense DICE (EXPLORE) songs of eight tracks played for seconds, the stems tapped each block as the audio
 * ISR does, against each track rendered alone in the same take (the others played but left out of the mix: mix_skip, as
 * the emulator's offline stems), from the same state (fork). The USB STEMS are EIGHT track stems (tracks 1..8, mono) and
 * the FX return (L R): 10 channels (stems-10: was six tracks, 8 channels). All eight are exported and in the mix:
 *   null       every stem bit for bit its track's solo render, in LOAD FULL, LITE and ECO, one core and CORES 2 (no bleed,
 *              no sound of a voice in another track's stem, no block given to the wrong part, the PAN out, LEVEL, DIST,
 *              DUCK, mutes in);
 *   sum        the eight stems and the FX return add up to the mix as the buses hear it, each block (before the master: the
 *              DJ FILTER, CLIP and the leveler off), within the stems' halving (L + R >> 1);
 *   silence    a track with nothing to play is a stem of exact zeros;
 *   SLICER     a track through it: its stem is the SLICER's output, bit for bit its solo render, the sum still the mix
 *              (until 0.9.7 its dry sound was in its stem and again, sliced, in the FX return); FX ON: in the FX return;
 *   ECO        its tail trimming (the load budget, LOAD tight) and the shared voice cap act on the take itself: the stems
 *              keep exactly what the mix plays (they null against the solo render of the same take), however many voices
 *              the budget gave up (counted). */
#define UI_TEST_NO_MAIN 1
#include "ui_test.c"
#include <sys/mman.h>
#include <sys/wait.h>

#define NSTEM 8                                          /* the track stems: T1..T8 (USB STEMS, all exported) */
#define NCH (NSTEM + 2)                                  /* the stems, then the FX return L R: the 10 channels */
#define OUT_MASK 0u                                      /* (no track left out of the mix or the stems) */
static int st_fail;
static int st_check(const char *what, int ok)
{
    printf("stems: %-104s %s\n", what, ok ? "ok" : "FAIL");
    st_fail += !ok;
    return ok;
}

static void song_from(uint32_t seed, uint32_t load)
{
    uint32_t k;
    ui_power_on();
    trk8_pref = TRK8_ON;                                 /* MENU > TRACKS 8: all eight play (all eight stems) */
    ui_load = (uint8_t)load;
    rp_apply();                                          /* LOAD */
    gp.genre = GEN_EXPLORE; gp.seed = (uint16_t)seed; gp.energy = 95; gp.form = 0;
    for (k = 0; k < GP_N; k++) gp.amt[k] = GA_AUTO;
    gp.complex = 70; gp.swing = GA_AUTO;
    mc_host_v12 = 1;                                     /* (0.9.13: made inside two cores' CPU budget, the FM-1's default;
                                                         * one core's would leave out parts: these are dense on purpose) */
    cpt_res = 63; cpt_cores = 2;                         /* (0.9.14: the device as the model holds it, gen.c gc_dev x1.00) */
    gen_go();
    cpt_res = 0;
    mc_host_v12 = 0;
    song.g[G_DJF] = 0;                                   /* (the sum is checked before the master: these off) */
    song.g[G_CLIP] = 0;
}

/* nb blocks of the song (the transport started here), the stems tapped into d[frame][NCH]; skip: the parts left out of
 * the mix. Returns the blocks whose sum matched the mix (sum_ok) of those checked (sum_n), the voices given up */
static uint32_t sum_ok, sum_n, gave_up, worst;
static void play(int32_t (*d)[NCH], uint32_t nb, uint32_t skip, uint32_t cores, int tight)
{
    uint32_t b, i, k;
    song.master_q12 = 4096;
    mix_skip = (uint8_t)(skip | OUT_MASK);
    mc_host = (uint8_t)cores;
    transport_req = 1;
    sum_ok = sum_n = worst = 0;
    gave_up = voice_kills;
    for (b = 0; b < nb; b++) {
        int32_t o[2 * CTL];
        if (b % 32u == 0u) { ui_input(); song_poll(); host_ticks += 20000u; fm1_ms += 20u; }
        fx_stems = 1;                                    /* (as MENU > USB STEMS: ui_input follows the menu's choice) */
        if (tight)
            load_tight = 1;                              /* (the load budget held: a render over 50 % on ECO) */
        mix_block(o, CTL);
        for (i = 0; i < CTL; i++) {
            for (k = 0; k < NSTEM; k++)
                d[b * CTL + i][k] = stem_buf[k][i];
            d[b * CTL + i][NSTEM] = stem_fx[0][i];
            d[b * CTL + i][NSTEM + 1] = stem_fx[1][i];
        }
        if (!clip_g && !djf.mode && !(skip & ((1u << NSTEM) - 1u))) {   /* the mix as the buses hear it: mix_l + wet (master_out's
                                                         * input), when no track is left out (a solo render: no) */
            uint32_t bad = 0;
            for (i = 0; i < CTL; i++) {
                int32_t s = 0, m = mix_l[i] + mix_r[i] + 2 * wet[i], e;
                for (k = 0; k < NSTEM; k++)
                    s += 2 * stem_buf[k][i];
                s += stem_fx[0][i] + stem_fx[1][i];
                e = s > m ? s - m : m - s;
                if (e > (int32_t)worst) worst = (uint32_t)e;
                bad |= e > (slicer_on() ? 6 : 2) * NSTEM;  /* (each stem's >> 1 floors: up to 1 off, doubled; through
                                                         * the SLICER its gain's too) */
            }
            sum_n++;
            sum_ok += !bad;
        }
    }
    gave_up = voice_kills - gave_up;
    transport_req = 2;
    { int32_t o[2 * CTL]; ui_input(); mix_block(o, CTL); }
}

/* the take in full, then each track alone (its solo render), every one from the same state: returns the stems that
 * are not bit for bit their solo render; *energy: tracks that sounded */
static int32_t null_tol;                                 /* (the SLICER's gain on each part apart: its rounding) */
static uint32_t take(uint32_t nb, uint32_t cores, int tight, uint32_t *sounding, uint32_t *gave, uint32_t *sums)
{
    size_t sz = sizeof(int32_t) * NCH * CTL * nb;
    int32_t (*full)[NCH] = mmap(0, sz, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    int32_t (*solo)[NCH] = mmap(0, sz, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    uint32_t *res = mmap(0, 4 * sizeof(uint32_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    uint32_t k, i, bad = 0, snd = 0;
    pid_t p;
    fflush(stdout);
    if (!(p = fork())) {
        play(full, nb, 0, cores, tight);
        res[0] = gave_up; res[1] = sum_ok; res[2] = sum_n; res[3] = worst;
        _exit(0);
    }
    waitpid(p, 0, 0);
    *gave = res[0];
    *sums = res[2] && res[1] == res[2];
    for (k = 0; k < NSTEM; k++) {
        uint32_t nd = 0;
        double e = 0;
        if (!(p = fork())) {
            play(solo, nb, ((1u << NPART) - 1u) & ~(1u << k), cores, tight);   /* (the other seven left out of the mix) */
            _exit(0);
        }
        waitpid(p, 0, 0);
        for (i = 0; i < CTL * nb; i++) {
            nd += full[i][k] - solo[i][k] > null_tol || solo[i][k] - full[i][k] > null_tol;
            e += (double)full[i][k] * full[i][k];
        }
        snd += e > 0;
        bad += nd != 0;
        if (nd)
            printf("  T%u: %u samples differ from its solo render\n", k + 1, nd);
    }
    if (!*sums)
        printf("  the sum: %u of %u blocks match the mix (worst %u)\n", res[1], res[2], res[3]);
    *sounding = snd;
    munmap(full, sz); munmap(solo, sz); munmap(res, 4 * sizeof(uint32_t));
    return bad;
}

int main(void)
{
    static const char *const LOADS[3] = {"FULL", "LITE", "ECO"};
    static const uint32_t SEEDS[3] = {42, 3, 123};
    char s[200];
    uint32_t si, l, c, nb = (uint32_t)(4.0 * FS / CTL);   /* 4 s a take */
    printf("-- the stems (fx.c) against each track's solo render\n");
    for (si = 0; si < 3u; si++)
        for (l = 0; l < 3u; l++)
            for (c = 0; c < 2u; c++) {
                uint32_t snd, gave, sums, bad;
                if (c && (l == 1u || si == 2u))
                    continue;                            /* (CORES 2 on FULL and ECO, two songs: enough) */
                song_from(SEEDS[si], l);
                bad = take(nb, c, 0, &snd, &gave, &sums);
                snprintf(s, sizeof s, "DICE %u, %s, %s: %u tracks sounding, %u voices given up: 8 stems = solo renders, sum = mix",
                         SEEDS[si], LOADS[l], c ? "CORES 2" : "one core", snd, gave);
                st_check(s, !bad && sums && snd >= 4u);
            }
    {   /* ECO with the load budget held all along: its quiet tails trimmed, in the mix and the stems alike */
        uint32_t snd, gave, sums, bad, t0 = voice_trims;
        song_from(42, LOAD_ECO);
        bad = take(nb, 1, 1, &snd, &gave, &sums);
        snprintf(s, sizeof s, "ECO, the load budget tight: %u voices given up (tails under -50 dB), the stems still the solo renders, sum = mix",
                 gave);
        st_check(s, !bad && sums && gave > 0);
        (void)t0;
    }
    {   /* an empty track: exact zeros */
        size_t sz = sizeof(int32_t) * NCH * CTL * 2000u;
        int32_t (*d)[NCH] = mmap(0, sz, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
        uint32_t i, nz = 0, e = 0;
        song_from(42, LOAD_FULL);
        for (i = 0; i < NSTEP; i++) trk[7].step[i].time = ST_REST;   /* (T8: stem 8 of the eight) */
        for (i = 0; i < NVOICE; i++) trk[7].v[i].active = 0;
        play(d, 2000u, 0, 1, 0);
        for (i = 0; i < CTL * 2000u; i++) { nz += d[i][7] != 0; e += d[i][0] != 0; }
        st_check("a track with nothing to play: its stem exact zeros (T8 empty, T1 playing)", nz == 0 && e > 1000u);
        munmap(d, sz);
    }
    {   /* the SLICER: T2 through it (GATE, then STUT), T2 and T5, then everything with its FX ON (its defaults) */
        static const struct { int mode, fx; uint8_t mask; const char *what; } V[4] = {
            {1, 0, 0x02, "GATE on T2"}, {2, 0, 0x02, "STUT on T2"}, {1, 0, 0x12, "GATE on T2 and T5"},
            {2, 1, 0xFF, "STUT on all eight, FX ON (its defaults)"}};
        uint32_t v, i;
        for (v = 0; v < 4u; v++) {
            uint32_t snd, gave, sums, bad;
            song_from(42, LOAD_FULL);
            song.g[G_SLMODE] = V[v].mode;
            song.g[G_SLFX] = V[v].fx;
            for (i = 0; i < NPART; i++)
                song.g[sl_trk_g(i)] = (V[v].mask >> i) & 1u;   /* (0.9.20: T7 T8 their own switches) */
            null_tol = 2;
            bad = take(nb, 1, 0, &snd, &gave, &sums);
            null_tol = 0;
            /* (one part through it: its stem is it alone through it; several, or the returns too: one bus, so its
             * repeats are in one place (the first part's stem, or the FX return) and only the sum must hold) */
            snprintf(s, sizeof s, "SLICER %s: %s; the sum = the mix (until 0.9.7 the bus twice: dry in the stems, sliced in FX)",
                     V[v].what, V[v].mask == 0x02 ? "T2's stem T2 alone through it (within 2 LSB)" : "each part at the gain it got");
            st_check(s, sums && (V[v].mask != 0x02 || !bad));
        }
    }
    printf(st_fail ? "STEMS TEST FAILED (%d)\n" : "stems: all ok\n", st_fail);
    return st_fail != 0;
}
