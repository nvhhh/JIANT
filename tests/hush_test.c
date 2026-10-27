/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Juan Ignacio Antonucci (Juan Jiant) */
/* (0.9.16) the silence path (fx.c master_rest, mix_hush, mix_silent): built twice by run_tests.sh, as it is and with
 * -DFX_HUSH_OFF (the mix as 0.9.14 rendered it), the same session rendered: the power-on parts played and let ring,
 * a long silence, notes again; an EXPLORE piece played, stopped, 15 s of silence, played again, stopped; LOWCUT; a
 * CLOUD tail; MASTER. It prints a hash of every output sample (and of the LFOs' and the voices' state at the end): the
 * two builds must print the same line. It also prints how many blocks the silence path took (none with FX_HUSH_OFF) */
#include <stdint.h>
#include <stdio.h>
static unsigned hushed;
#define HUSH_COUNT() (hushed++)
#define UI_TEST_NO_MAIN
#include "ui_test.c"

static uint32_t h = 2166136261u, nblk;
static FILE *dump;
static int32_t o[2 * CTL];
static void blocks(uint32_t nb)
{
    uint32_t b, i;
    for (b = 0; b < nb; b++) {
        if (b % 32u == 0u) { ui_input(); song_poll(); host_ticks += 20000u; fm1_ms += 20u; }
        mix_block(o, CTL);
        for (i = 0; i < 2u * CTL; i++) h = (h ^ (uint32_t)o[i]) * 16777619u;
        if (dump) fprintf(dump, "%u %08x %d %d\n", nblk, h, o[0], o[1]);
        nblk++;
    }
}
static void notes(track_t *t, uint32_t base)
{
    uint32_t i;
    for (i = 0; i < 3u; i++) trk_note_on(t, base + 4u * i, 100);
    blocks(600);
    for (i = 0; i < 3u; i++) trk_note_off(t, base + 4u * i);
}
int main(void)
{
    uint32_t k, seed;
    if (getenv("HUSH_DUMP")) dump = fopen(getenv("HUSH_DUMP"), "w");
    ui_power_on();
    load_depth++; fm6_init(); power_on_parts(); load_depth--;
    ui_rec_prefs |= PREF_SIX >> 8; rp_apply();
    song.master_q12 = 4096;
    mc_host = 1;
    for (k = 0; k < NTRK; k++) trk[k].p[P_REV] = 40;
    notes(&trk[4], 48);
    blocks(30000);                                       /* (~22 s: every tail out, the master at rest) */
    notes(&trk[5], 60);
    trk[4].p[P_LD_FLT] = 40;                             /* (its LFO on the filter: the LFO turns on through the silence) */
    notes(&trk[4], 55);
    blocks(20000);
    fx_lowcut = 1;                                       /* SPEAKER EQ LOWCUT */
    notes(&trk[1], 57);
    blocks(20000);
    notes(&trk[2], 52);
    fx_lowcut = 0;
    for (seed = 3; seed < 6u; seed++) {                  /* EXPLORE pieces: PLAY, STOP, silence, PLAY, STOP */
        gp.genre = GEN_EXPLORE; gp.seed = (uint16_t)seed; gp.energy = 60; gp.form = 0;
        for (k = 0; k < GP_N; k++) gp.amt[k] = GA_AUTO;
        gp.complex = 50; gp.swing = GA_AUTO;
        gen_go();
        song.master_q12 = 4096;
        transport_req = 1; ui_input();
        blocks(3000);
        transport_req = 2; ui_input();
        blocks(21000);
        transport_req = 1; ui_input();
        blocks(2000);
        transport_req = 2; ui_input();
        blocks(3000);
    }
    for (k = 0; k < NTRK; k++) {
        uint32_t i;
        h = (h ^ trk[k].lfo_ph ^ (uint32_t)trk[k].lfo_val) * 16777619u;
        for (i = 0; i < NVOICE; i++) h = (h ^ trk[k].v[i].active ^ (uint32_t)trk[k].v[i].env) * 16777619u;
    }
    h = (h ^ (uint32_t)lev_g ^ (uint32_t)lim_env ^ (uint32_t)dc_l) * 16777619u;
    printf("hush: the session's output and state %08x\n", h);
    printf("hush: %u blocks of %u through the silence path\n", hushed, nblk);
    return 0;
}
