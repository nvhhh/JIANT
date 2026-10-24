/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments
 * Portions Copyright (C) 2026 Juan Ignacio Antonucci (Juan Jiant) */
/* Eight tracks (0.9.19, core.h NPART 8): T7 and T8 are new parts with no steps and the power-on sound of TRK_OLD / TRK_DEF
 * (ANALOG preset 0). Built on tests/ui_test.c's harness (the UI sources, stub panel and display), run by tests/run_tests.sh:
 *   (a) a project of six tracks (FUNC, as 0.9.x wrote it) loads: T1..T6 as they were, T7 and T8 with no steps and the
 *       power-on sound (the same as a fresh power-on's), the projects' other parts as they were;
 *   (b) an eight-track project (FUND) saves and reloads identically, byte for byte when saved again;
 *   (c) MENU > TRACKS 4 / 6 / 8 (trk_live 4, 6, 8; the GLO > SYSTEM knob and the menu's names), and the mixer's window
 *       follows the selected track: T5..T8 with T8 selected, T3..T6 with T5 / T6, T1..T4 below that. */
#define PROJ_FUNC_WRITER 1                      /* (project.c proj_pack_func: a FUNC of six tracks, as before 0.9.19) */
#define UI_TEST_NO_MAIN 1                       /* (ui_test.c: its harness, without its main) */
#include "ui_test.c"

/* the device's power-on (main.c felucca_init: ui_power_on_099 of ui_test.c, which that file's guard leaves out here) */
static void t8_power_on(void)
{
    ui_power_on();
    load_depth++;
    fm6_init();
    power_on_parts();
    load_depth--;
    song.sel = 0;
}

/* a few steps, a sound, a mute, a motion event on track k: the same content whatever the project's track count */
static void t8_fill(project_t *p, uint32_t k)
{
    uint32_t i;
    for (i = 0; i < NSTEP; i += 3u) {
        step_t *s = &p->t[k].step[i];
        s->time = ST_NOTE; s->n = 1; s->vel = (uint8_t)(70u + k); s->note[0] = (uint8_t)(40u + i + k);
        s->hit = 1; s->acc = 0;
    }
    p->t[k].p[P_LEVEL] = (int16_t)(60 + k);
    p->t[k].p[P_PAN] = (int16_t)(k * 3u) - 20;
    p->t[k].p[P_MUTE] = k == 1u;
}

/* the tracks of two projects equal: every parameter, every step, the engine (the preset is a sound's name, not kept) */
static int t8_same_track(const proj_trk_t *a, const proj_trk_t *b)
{
    return !memcmp(a->p, b->p, sizeof a->p) && a->engine == b->engine && !memcmp(a->step, b->step, sizeof a->step);
}

static int t8_project_loads(void)
{
    static project_store_t st6, st8, st8b;
    static project_t six, eight, back, expect, got;
    static uint8_t fm_x[FM6_PACKED];
    static track_t before[NTRK];
    uint32_t k;
    int bad = 0, ok = 1;
    int ok_a = 1, ok_b = 1;

    /* a six-track project of 0.9.x: the tracks 1..6 with sounds and steps, FUNC (six tracks, PROJ_MAGIC) */
    memset(&six, 0, sizeof six);
    six.magic = PROJ_MAGIC; six.size = sizeof six; six.parts = 6; six.phys = PROJ_PHYS;
    chain_defaults(&six.chain);
    for (k = 0; k < PROJ_LT_C; k++) { t8_fill(&six, k); six.t[k].engine = (uint8_t)(k == 1u ? ENGI_FM6 : ENGI_ANALOG); }
    proj_new_tracks(&six, PROJ_LT_C);                   /* (T7, T8: their power-on, no steps: as the 0.9.x project had) */
    for (k = 0; k < NTRK; k++) memcpy(six.fm6[k], FM6_INIT, FM6_PACKED);
    for (k = 0; k < G_COUNT; k++) six.g[k] = GP[k].def;
    memcpy(six.dx, DX_KIT_DEF, sizeof six.dx);
    memcpy(six.name, "SIX", 3);
    six.sum = proj_sum(&six);
    ok_a &= proj_pack_func(&st6, &six) && ((uint32_t *)st6.raw)[0] == PROJ_MAGIC && st6.raw[67] == PROJ_LT_C;

    /* what the loaded tracks must be: the six as the project had them, T7, T8 as a fresh power-on */
    t8_power_on();
    for (k = 0; k < NTRK; k++) before[k] = trk[k];
    for (k = PROJ_LT_C; k < NTRK; k++) ok_a &= seq_is_empty(&before[k]) && !motion_count(&before[k]);

    ok_a &= proj_import(&back, &st6, sizeof st6) != 0;
    ok_a &= back.parts == NPART && back.t[6].preset == PROJ_DEF_SOUND && back.t[7].preset == PROJ_DEF_SOUND;
    ok_a &= project_restore_runtime(&back) == 0;
    for (k = 0; k < PROJ_LT_C; k++)
        ok_a &= trk[k].p[P_LEVEL] == six.t[k].p[P_LEVEL] && trk[k].p[P_PAN] == six.t[k].p[P_PAN] &&
                !!trk[k].p[P_MUTE] == (k == 1u) && seq_is_empty(&trk[k]) == 0;
    for (k = PROJ_LT_C; k < NTRK; k++) {
        ok_a &= seq_is_empty(&trk[k]) && !motion_count(&trk[k]) && trk[k].eng_req == TRK_DEF[k][0];
        ok_a &= trk[k].eng_req == ENGI_ANALOG && trk[k].preset == TRK_OLD[k][1] && trk[k].preset == 0u;   /* ANALOG preset 0 */
        ok_a &= trk[k].p[P_LEVEL] == before[k].p[P_LEVEL] && trk[k].p[P_PAN] == before[k].p[P_PAN];
        ok_a &= !memcmp(trk[k].p, before[k].p, sizeof trk[k].p);
    }
    bad += check("(a) a six-track project (FUNC) loads: T1..T6 as they were, T7 and T8 empty with the power-on sound", ok_a);

    /* the six tracks as the project holds them: the runtime after a load, captured; a FUNC of them, loaded again: the
     * same tracks 1..6 (steps, parameters, engines), T7 and T8 empty again */
    project_capture(&expect);
    ok &= proj_pack_func(&st6, &expect) && proj_import(&back, &st6, sizeof st6) != 0 && project_restore_runtime(&back) == 0;
    project_capture(&got);
    for (k = 0; k < PROJ_LT_C; k++) ok &= t8_same_track(&got.t[k], &expect.t[k]);
    for (k = PROJ_LT_C; k < NTRK; k++) ok &= seq_is_empty(&trk[k]) && !motion_count(&trk[k]);
    bad += check("(a) .. and again after a load of that FUNC: tracks 1..6 the same (steps, parameters, engines), 7 and 8 empty", ok);
    ok_a &= ok;

    /* (b) an eight-track project: T7 and T8 with content too; saved as FUND, loaded, saved again: the same bytes */
    memset(&eight, 0, sizeof eight);
    eight.magic = PROJ_MAGIC_8; eight.size = sizeof eight; eight.parts = NPART; eight.phys = PROJ_PHYS;
    chain_defaults(&eight.chain);
    for (k = 0; k < NTRK; k++) { t8_fill(&eight, k); eight.t[k].engine = (uint8_t)(k < 6u ? ENGI_ANALOG : ENGI_FM6); }
    eight.t[7].p[P_REV] = 40;
    for (k = 0; k < NTRK; k++) memcpy(eight.fm6[k], FM6_INIT, FM6_PACKED);
    memcpy(fm_x, FM6_FACTORY[3], FM6_PACKED);
    memcpy(eight.fm6[7], fm_x, FM6_PACKED);            /* T8: an FM6 patch of its own */
    eight.t[7].engine = ENGI_FM6;
    for (k = 0; k < G_COUNT; k++) eight.g[k] = GP[k].def;
    memcpy(eight.dx, DX_KIT_DEF, sizeof eight.dx);
    memcpy(eight.name, "EIGHT", 5);
    eight.sum = proj_sum(&eight);
    ok_b &= proj_pack(&st8, &eight) && ((uint32_t *)st8.raw)[0] == PROJ_MAGIC_8 && st8.raw[67] == NTRK;
    ok_b &= proj_import(&back, &st8, sizeof st8) != 0;
    ok_b &= back.parts == NPART && !memcmp(back.t, eight.t, sizeof eight.t) && !memcmp(back.fm6, eight.fm6, sizeof eight.fm6);
    ok_b &= !memcmp(back.name, "EIGHT", 5) && !memcmp(back.g, eight.g, sizeof eight.g);
    ok_b &= proj_pack(&st8b, &back) && !memcmp(st8.raw, st8b.raw, sizeof st8.raw);
    bad += check("(b) an eight-track project saves (FUND) and reloads identically; saved again, the same bytes", ok_b);
    return bad;
}

/* (c) the MENU's TRACKS: 4 (the default), 6, 8; GLO > SYSTEM's TRACKS knob reads the same; the mixer's window */
static int t8_menu(void)
{
    int ok = 1, bad = 0;
    uint32_t sel0 = song.sel;
    ui_power_on();
    ok &= menu_n(MI_TRACKS) == 3u && !strcmp(menu_vname(MI_TRACKS, 0), "4") && !strcmp(menu_vname(MI_TRACKS, 1), "6") &&
          !strcmp(menu_vname(MI_TRACKS, 2), "8");
    trks_put(0); ok &= trk_live == 4u && menu_get(MI_TRACKS) == 0u && trks_ui == 0;
    trks_put(1); ok &= trk_live == 6u && menu_get(MI_TRACKS) == 1u && trks_ui == 1;
    trks_put(2); ok &= trk_live == 8u && menu_get(MI_TRACKS) == 2u && trks_ui == 2 && (ui_rec_prefs & (PREF_SIX >> 8));
    menu_put(MI_TRACKS, 1u); ok &= trk_live == 6u && menu_get(MI_TRACKS) == 1u;     /* (the menu's own setter) */
    menu_put(MI_TRACKS, 2u); ok &= trk_live == 8u && menu_get(MI_TRACKS) == 2u;
    menu_put(MI_TRACKS, 0u); ok &= trk_live == 4u && menu_get(MI_TRACKS) == 0u;
    ok &= !strcmp(N_TRKS[2], "8");
    bad += check("(c) MENU > TRACKS 4 / 6 / 8: trk_live 4, 6, 8 (menu, GLO > SYSTEM alike) and the names", ok);

    /* the mixer's window: four tracks on screen, following the selected one */
    trks_put(2);
    song.sel = 7; ok = trk_base4() == 4u && trk_base6() == 2u;
    song.sel = 6; ok &= trk_base4() == 4u;
    song.sel = 5; ok &= trk_base4() == 2u && trk_base6() == 0u;
    song.sel = 4; ok &= trk_base4() == 2u;
    song.sel = 0; ok &= trk_base4() == 0u && trk_base6() == 0u;
    trks_put(1); song.sel = 5; ok &= trk_base4() == 2u && trk_base6() == 0u;      /* (TRACKS 6: as before, T3..T6) */
    trks_put(0); song.sel = 0; ok &= trk_base4() == 0u;
    song.sel = (uint8_t)sel0;
    bad += check("(c) the mixer: T5..T8 with T8 selected (T3..T6 with T5 / T6, T1..T4 below)", ok);
    return bad;
}

int main(void)
{
    int bad = 0;
    setvbuf(stdout, NULL, _IONBF, 0);
    bad += t8_project_loads();
    bad += t8_menu();
    if (bad) printf("TRACKS 8 TEST FAILED\n");
    else printf("TRACKS 8 TEST: all passed\n");
    return bad != 0;
}
