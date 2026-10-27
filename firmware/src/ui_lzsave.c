/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments
 * Copyright (C) 2026 Juan Ignacio Antonucci (Juan Jiant) */
/* (0.8.5) SAVE's lens (ui.c save_*): the panels of its three levels, drawn as the approved HTML mock-up (SAVE, the
 * proposal "LENTE"), in the lens's panel (240 x 140 at y 84; the mock-up's coordinates less 84). Included by
 * ui_graph.c.
 *   LIBRARY  PROJECT: the eight projects as tiles (4 x 2, 56 x 66): the number, the name (its section A's), its four
 *            sections as chips A..D (stored: the section's colour; empty: dark), the project shown TINT with a THEME
 *            frame, its section picked framed TEXT. TOOLS: the six tools as rows (the one picked TINT).
 *   PROJECT  the project's four sections large (stored: their colour, the one picked full with a TEXT frame, the
 *            others half; the music's: NOW), ARRANGE and the song's rows as chips ("A16"; a scene: a dot; ARRANGE's
 *            row picked full and framed), the song as a strip (its rows by their bars, the one picked full; playing:
 *            a TEXT line where it is) and its length and END under it.
 *   SOUND    the track's being (its engine's species, its HOME knobs, moving with its sound) with the track and its
 *            sound under it; USER: the 32 slots (4 x 8; a stored one its engine's being small and its name, the one
 *            picked TINT with a THEME frame); PRESETS: the sounds around the one loaded (TINT), their engine at the
 *            right, a favourite's star.
 * The other projects' sections and names come from their flash headers (project.c lib_scan), one project a frame,
 * cached here until a project is written, loaded or renamed (lib_gen). */
/* a section's chip colour: stored, its colour (half: mixed with BG); empty: BG */
static uint16_t lz_sec(uint32_t s, int used, int half)
{
    uint16_t c = used ? sec_col(s) : half < 0 ? ux_mix(T_BG, T_RAISE, 50) : T_BG;
    return used && half ? ux_mix(T_BG, c, 50) : c;
}
#define LZ_TX(c) (4 + (int32_t)(c) * 117 / 2)          /* four columns of 56 px, 58 or 59 apart (the mock-up's 58.5) */
static void graph_lib(void)
{
    uint32_t i, j;
    for (i = 0; i < NSONG; i++) {
        int32_t x = LZ_TX(i % 4u), y = (int32_t)(i / 4u) * 70;
        int cur = i == song_cur;
        uint16_t bg = cur ? T_TINT : T_RAISE;
        char b[4], nm[13];
        cv_rect(x, y, 56, 66, bg);
        if (cur)
            cv_frame(x, y, 56, 66, T_THEME);
        fmt_int(b, (int32_t)i + 1);
        GFX_HOOK_ALIGN(0, y + 3, 0, y + 12, AL_V | AL_PASS, "library tile number");
        cv_text_on(x + 4, y + 3, &AF_X, b, cur ? T_THEME : T_DIM, bg);
        if (cur)
            str_cpy(nm, graph_project_used(0) ? graph_project_name(0) : "", sizeof nm);
        else
            str_cpy(nm, lib.name[i], sizeof nm);
        if (!nm[0] && (cur ? graph_project_used(0) : (lib.mask[i] & 1u)))
            str_cpy(nm, "USED", sizeof nm);
        cv_clip_text(x + 4, y + 14, &AF_X, nm[0] ? nm : "--", nm[0] ? T_TEXT : T_DIM, bg, 50);
        for (j = 0; j < 4u; j++) {
            int32_t sx = x + 4 + (int32_t)(j % 2u) * 25, sy = y + 28 + (int32_t)(j / 2u) * 18;
            int used = cur ? graph_project_used(j) : (int)((lib.mask[i] >> j) & 1u);
            uint16_t c = lz_sec(j, used, 0);
            char l[2] = {(char)('A' + j), 0};
            cv_rect(sx, sy, 23, 15, c);
            GFX_HOOK_ALIGN(sx, sy, sx + 23, sy + 15, AL_HV, "library section chip letter centred");
            cv_text_in(sx, sy + CAP_IN(X, 15), 23, &AF_X, l, used ? T_INK : T_DIM, c);
            if (cur && (int32_t)j + 1 == song.g[G_SLOT])
                cv_frame(sx - 1, sy - 1, 25, 17, T_TEXT);
        }
    }
}
static void graph_lz_tools(void)
{
    static const char *const WHAT[6] = {"CLEAR SEQ", "INIT SOUND", "DELETE ROW", "CLEAR SONG", "NEW PROJECT", "CPU TEST"};
    static const char *const ON[6] = {"STEPS + AUTO", "ENGINE DEFAULT", "", "ALL ROWS", "EVERYTHING", "HEAVY SONG"};
    uint32_t c;
    char b[12];
    for (c = 0; c < 6u; c++) {
        int32_t y = 1 + (int32_t)c * 23;
        int sel = ui.act == c + 1u;
        uint16_t bg = sel ? T_TINT : T_RAISE;
        cv_rect(4, y, 232, 21, bg);
        GFX_HOOK_ALIGN(0, y, 0, y + 21, AL_V, "tools row text centred up/down");
        cv_text_on(12, y + CAP_IN(M, 21), &AF_M, WHAT[c], sel ? T_TEXT : T_MID, bg);
        if (c == 2u) {                                   /* (the row ARRANGE has picked) */
            str_cpy(b, "ROW ", sizeof b);
            fmt_int(b + 4, (int32_t)ui.song_row + 1);
        }
        GFX_HOOK_ALIGN(0, y, 0, y + 21, AL_V, "tools row text centred up/down");
        cv_text_r(230, y + CAP_IN(X, 21), &AF_X, c == 2u ? b : ON[c], sel ? T_MID : T_DIM, bg);
    }
}
/* PROJECT: the sections, the arrangement's chips, the song's strip (ar: ARRANGE, its rows picked) */
static void graph_proj(int ar)
{
    uint32_t i, total = 0, played = 0, sel = (uint32_t)song.g[G_SLOT] - 1u, n = chain_config.count, end;
    char b[24];
    for (i = 0; i < 4u; i++) {
        int32_t x = LZ_TX(i);
        int used = graph_project_used(i), pick = !ar && i == (sel & 3u);
        uint16_t c = lz_sec(i, used, used ? !pick : -1), ink = used ? T_INK : T_DIM;
        const char *nm = graph_project_name(i);
        b[0] = (char)('A' + i); b[1] = 0;
        cv_rect(x, 2, 56, 49, c);
        GFX_HOOK_ALIGN(0, 8, 0, 19, AL_V | AL_PASS, "project section letter");
        cv_text_on(x + 6, 4, &AF_M, b, ink, c);
        cv_free_text(x + 6, 25, &AF_X, !used ? "EMPTY" : nm[0] ? nm : "USED", ink, c, 47);
        if (live.cur == (int8_t)i && used)
            cv_text_r(x + 52, 38, &AF_X, "NOW", ink, c);
        if (pick) {                                      /* (2 px, round it) */
            cv_frame(x - 2, 0, 60, 53, T_TEXT);
            cv_frame(x - 1, 1, 58, 51, T_TEXT);
        }
    }
    GFX_HOOK_ALIGN(0, 58, 0, 67, AL_V | AL_PASS, "project arrange title");
    cv_text(6, 58, &AF_X, "ARRANGE", ar ? T_THEME : T_DIM);
    for (i = 0; i < n && i < CHAIN_ROWS; i++) {
        const chain_row_t *r = &chain_config.row[i];
        int32_t x = 4 + (int32_t)(i % 8u) * 293 / 10, y = 72 + (int32_t)(i / 8u) * 22;
        int pick = ar && i == ui.song_row, used = graph_project_used(r->slot);
        uint16_t c = lz_sec(r->slot, 1, !pick);
        b[0] = (char)('A' + (r->slot & 3u));
        fmt_int(b + 1, r->bars);
        cv_rect(x, y, 27, 20, c);
        GFX_HOOK_ALIGN(x, y, x + 27, y + 20, AL_HV, "arrange chip centred");
        cv_text_in(x, y + CAP_IN(X, 20), 27, &AF_X, b, used ? T_INK : T_DIM, c);
        if (pick)
            cv_frame(x - 1, y - 1, 29, 22, T_TEXT);
        if (song_idx.scene[song_cur][i].on)              /* a scene: a dot */
            cv_rect(x + 22, y + 3, 2, 2, T_TEXT);
        total += r->bars;
        if (chain.running && i < chain.row) played += r->bars;
    }
    if (chain.running && chain.row < n)
        played += chain_config.row[chain.row].bars - chain.remaining;
    if (total) {                                         /* the song: its rows by their bars */
        int32_t acc = 0;
        for (i = 0; i < n; i++) {
            int32_t x0 = 4 + acc * 232 / (int32_t)total, x1;
            acc += chain_config.row[i].bars;
            x1 = 4 + acc * 232 / (int32_t)total;
            cv_rect(x0, 118, x1 - x0 > 1 ? x1 - x0 - 1 : 1, 6, lz_sec(chain_config.row[i].slot, 1, !(ar && i == ui.song_row)));
            if (song_idx.scene[song_cur][i].on)
                cv_rect(x0 + 2, 120, 2, 2, T_TEXT);
        }
        if (chain.running)
            cv_rect(4 + (int32_t)(played * 232u / total), 116, 1, 10, T_TEXT);
    }
    end = SONG_END(&chain_config);
    fmt_int(b, (int32_t)total);
    str_cpy(b + str_len(b), " BARS \xB7 ", 9);
    if (end == SONG_FOREVER) str_cpy(b + str_len(b), "LOOP", 6);
    else if (end >= 2u) { uint32_t k = str_len(b); b[k] = 'x'; fmt_int(b + k + 1u, (int32_t)end); }
    else str_cpy(b + str_len(b), "STOP", 6);
    GFX_HOOK_ALIGN(0, 128, 0, 137, AL_V | AL_PASS, "project song length");
    cv_text_r(236, 128, &AF_X, n ? b : "NO ROWS: OCT+ ADDS ONE", ar && ui.song_row >= song_end_row() ? T_THEME : T_DIM,
              T_SURF);
}
/* a being small at (cx, cy) radius r (px): engine e's species, its outline in heat c; 24 vertices (bg_outline's form,
 * its spikes and lobes, at a slot's size: a quarter of the lines) */
static void lz_mini_being(uint32_t e, int32_t cx, int32_t cy, int32_t r, uint16_t c)
{
    const genome_t *g = &GENOME[EGEN[e % NENGINES]];
    int32_t i, px = 0, py = 0, R = r * 16, k = g->k0 + g->kk / 2;
    for (i = 0; i <= 24; i++) {
        uint32_t th = (uint32_t)(i % 24) * 65536u / 24u;
        int32_t rr = R + ((R * 90 >> 8) * (9175 * bg_sin((uint32_t)g->lob * th) >> 15) >> 15), x, y, c2, p, n;
        c2 = bg_sin(((uint32_t)k * th >> 1) + (uint32_t)g->tw * 3000u + 0x4000u);
        p = c2 > 0 ? c2 : 0;
        for (n = 1; n < g->sh; n++)
            p = (p * c2) >> 15;
        if (p < 0) p = 0;
        rr += ((R * 110 >> 8) * p >> 15) * 26 >> 4;
        x = cx * 16 + ((rr * bg_sin(th + 0x4000u) >> 15) * g->ax >> 4);
        y = cy * 16 + (rr * bg_sin(th) >> 15);
        if (i)
            og_line(px, py, x, y, c);
        px = x;
        py = y;
    }
}
/* SOUND: the track's being (GARDEN, the owner: the engine's creature, as BEING's and EDIT's, alive with its sound), the
 * track and its sound */
static void lz_track_being(void)
{
    const track_t *t = TSEL;
    char b[20];
    vx_draw(t, 4, 4, 78, 76, -1);
    b[0] = 'T'; b[1] = (char)('1' + song.sel % NTRK); b[2] = 0;
    GFX_HOOK_ALIGN(4, 0, 82, 0, AL_H | AL_PASS, "sound track centred");
    cv_text_in(4, 85, 78, &AF_X, b, T_THEME, T_SURF);
    sound_name(t, b);
    cv_free_text(42 - HALF_UP(text_w(&AF_X, b) < 76 ? text_w(&AF_X, b) : 76), 99, &AF_X, b, T_TEXT, T_SURF, 76);
}
static void graph_sound(void)
{
    uint32_t k;
    lz_track_being();
    for (k = 0; k < UP_SLOTS; k++) {
        int32_t x = 88 + (int32_t)(k % 4u) * 37, y = 2 + (int32_t)(k / 4u) * 17;
        int sel = k == ui.uslot, used = up_used(k);
        uint16_t bg = sel ? T_TINT : T_RAISE;
        char b[13];
        cv_rect(x, y, 35, 15, bg);
        if (used) {
            lz_mini_being(up_engine(k), x + 5, y + 7, 3, bg_col(150));
            up_name(k, b);
            cv_clip_text(x + 10, y + CAP_IN(X, 15), &AF_X, b, T_TEXT, bg, 24);
        } else {
            up_slot_label(b, k);
            GFX_HOOK_ALIGN(x, y, x + 35, y + 15, AL_HV, "sound slot label centred");
            cv_text_in(x, y + CAP_IN(X, 15), 35, &AF_X, b, T_DIM, bg);
        }
        if (sel)
            cv_frame(x, y, 35, 15, T_THEME);
    }
}
static void graph_lz_presets(void)
{
    uint32_t total, cur = preset_pos(&total), e, k, i, first;
    lz_track_being();
    if (!total) {
        cv_text_in(88, 63, 148, &AF_X, "NO FAVORITES", T_MID, T_SURF);
        return;
    }
    first = cur < 4u ? 0u : cur - 4u;
    if (total > 9u && first + 9u > total)
        first = total - 9u;
    for (i = first; i < total && i < first + 9u; i++) {
        int32_t y = 2 + (int32_t)(i - first) * 15;
        int sel = i == cur;
        uint16_t bg = sel ? T_TINT : T_SURF;
        char nm[13];
        const char *en;
        e = preset_at(i, &k);
        if (e == NENGINES) {
            up_name(k, nm);
            en = "USER";
        } else {
            str_cpy(nm, ENGINES[e]->presets[k].name, sizeof nm);
            en = ENGINES[e]->name;
        }
        if (sel)
            cv_rect(88, y, 148, 14, bg);
        GFX_HOOK_ALIGN(0, y, 0, y + 14, AL_V, "sound preset row centred up/down");
        cv_free_text(94, y + CAP_IN(X, 14), &AF_X, nm, sel ? T_TEXT : T_MID, bg, 84);
        if (favorite_has(e, k)) {
            GFX_HOOK_ALIGN(0, y, 0, y + 14, AL_V, "sound preset star centred up/down");
            cv_text_on(182, y + CAP_IN(S, 14), &AF_S, "*", T_ACCENT, bg);   /* (a favourite; 0.8.5 no icons) */
        }
        GFX_HOOK_ALIGN(0, y, 0, y + 14, AL_V, "sound preset row centred up/down");
        cv_text_r(232, y + CAP_IN(X, 14), &AF_X, eng_abbr(en), T_DIM, bg);
    }
}
/* the lens's panel for the SAVE page shown */
static void graph_save_lz(const page_t *pg)
{
    uint32_t lv = save_level();
    if (pg->graph == GR_TOOLS) graph_lz_tools();
    else if (pg->graph == GR_SONG) graph_proj(1);
    else if (pg->graph == GR_SLOTS) { if (lv == SV_PROJ) graph_proj(0); else graph_lib(); }
    else if (pg->graph == GR_USER) graph_sound();
    else graph_lz_presets();
}
