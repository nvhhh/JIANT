/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments
 * Copyright (C) 2026 Juan Ignacio Antonucci (Juan Jiant) */
/* Native emulator boot: includes main.c's real felucca_init, not just the storage loader. */
/* (JIANT 0.6.84) by @workeffortwaste (pull request #134, the ARRANGE power-off bug): it fails without main.c's
 * 0.6.81 fix (felucca_init clearing the arrangement persist_boot had read) and passes with it. */
#include "../web/emu/felucca_web.c"

static int startup_case(int saved, int restore, int noprefs)
{
    song_index_t want, got;
    persist_t prefs = {0};
    uint32_t i;
    int ok;
    web_nor_erase();
    memset(&want, 0, sizeof want);
    want.magic = SONG_MAGIC;
    want.version = 2;
    want.cur = saved ? 2u : 0u;
    for (i = 0; i < NSONG; i++) chain_defaults(&want.rows[i]);
    if (saved) {
        want.rows[2].count = 2;
        want.rows[2].row[0] = (chain_row_t){0, 3};
        want.rows[2].row[1] = (chain_row_t){1, 7};
        SONG_END(&want.rows[2]) = 3;
        want.scene[2][0] = (scene_t){1, 2, 4, {11, 22, 33, 44}};
        want.rows[0].count = 1;
        want.rows[0].row[0] = (chain_row_t){3, 9};
        if (st_save(OBJ_SONGIDX, &want, sizeof want)) return 1;
    }
    prefs.magic = PERSIST_MAGIC;
    prefs.panel = PANEL_DEFAULT;
    prefs.favorites.factory[15][30] = restore ? PREF_RESTORE_ON : 0;
    if (!noprefs && st_save(OBJ_SETTINGS, &prefs, sizeof prefs)) return 1;

    web_boot();
    if (noprefs) {                                     /* (0.9.7) no settings saved: new settings, SKIN AIR, SCOPE OFF */
        int okf = UX_SKIN == SKIN_AIR, i2;              /* (GARDEN: AIR's layout, always) */
        for (i2 = 0; i2 < 2000; i2++) web_render(128);   /* (past the splash) */
        okf &= ui_scope == 0u;
        printf("startup: empty flash, no settings: SKIN AIR, SCOPE OFF (new settings) %s\n", okf ? "ok" : "FAIL");
        return !okf;
    }
    if (ui_scope) {                                    /* (0.9.7) settings saved before AIR: SCOPE OFF (GARDEN: no SKIN, AIR's layout) */
        printf("startup: saved settings: SCOPE OFF: FAIL\n");
        return 1;
    }
    ok = song_cur == want.cur && !memcmp(&chain_config, &want.rows[want.cur], sizeof chain_config);
    ok &= !!(ui_prefs & PREF_RESTORE_ON) == restore;
    ok &= !memcmp(song_idx.scene, want.scene, sizeof want.scene);
    /* Advance beyond the splash and the arrangement's two-second automatic save. */
    for (i = 0; i < 2000u; i++) web_render(128);
    ok &= !memcmp(&chain_config, &want.rows[want.cur], sizeof chain_config);
    if (saved)
        ok &= st_load(OBJ_SONGIDX, &got, sizeof got) == (int)sizeof got && !memcmp(&got, &want, sizeof got);
    else
        ok &= st_load(OBJ_SONGIDX, &got, sizeof got) < 0;
    printf("startup: %s, RESTORE LAST %s: rows, bars, END, scenes and flash %s\n",
           saved ? "saved project 3" : "empty flash", restore ? "ON" : "OFF", ok ? "ok" : "FAIL");
    return !ok;
}

int main(void)
{
    int mode, status, bad = 0;
    /* Each child starts with fresh static RAM, as a new device instance does. */
    for (mode = 0; mode < 4; mode++) {                 /* (0.9.7: 3, no settings saved at all) */
        pid_t child = fork();
        if (child < 0) return 2;
        if (!child) exit(startup_case(mode == 1 || mode == 2, mode == 2, mode == 3));
        if (waitpid(child, &status, 0) != child || !WIFEXITED(status) || WEXITSTATUS(status)) bad++;
    }
    puts(bad ? "startup test FAILED" : "startup test passed");
    return !!bad;
}
