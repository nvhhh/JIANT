/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Juan Ignacio Antonucci (Juan Jiant)
 * The generator audited (firmware/src/gen.c): a piece as text, to read its harmony as a musician reads a lead sheet:
 * the key and the SCALE / CHORD pages as GEN wrote them, the chords bar by bar (a symbol and its roman numeral), every
 * part a line a bar (the notes by name, '=' held, '.' rest, a chord's notes joined by '/'), each section of a FORM
 * (its key). Optionally the piece heard: a WAV of it playing (its song, with PARTS or SONG), SECONDS long.
 *   cc -O1 -w -Ibuild/gen -Ifirmware/src -Itests -o build/host/gen_dump tests/gen_dump.c -lm
 *   build/host/gen_dump GENRE SEED [ENERGY [FORM [OUT.wav SECONDS]]]      (GENRE 1..43, 0 every genre; FORM 0 1 2) */
#define UI_TEST_NO_MAIN 1
#include "ui_test.c"
static const char *NS[12] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
static const char *NF[12] = {"C", "Db", "D", "Eb", "E", "F", "Gb", "G", "Ab", "A", "Bb", "B"};
static const char **NN = NS;
static void spell(void)                                 /* flats in the flat keys (as the relative major's signature) */
{
    uint32_t rel = (uint32_t)(gs.root + (gminor() ? 3 : 0)) % 12u;
    NN = rel == 5u || rel == 10u || rel == 3u || rel == 8u || rel == 1u || rel == 6u ? NF : NS;
}
static const char *RLN[6] = {"DRUM", "BASS", "PAD", "LEAD", "ARP", "TEX"};
static const char *FAM[6] = {"LOOP", "JAZZY", "POP", "COLOR", "CINE", "JAZZ"};
static const char *CDN[12] = {"OFF", "TRIAD", "7TH", "ADD9", "SUS2", "SUS4", "MAJ", "MIN", "7", "MAJ7", "MIN7", "POWER"};
static const char *QN[4] = {"OFF", "SNAP", "WHITE", "SEQ"};
static void nname(char *b, int n) { sprintf(b, "%s%d", NN[(n % 12 + 12) % 12], n / 12 - 1); }
/* a chord's symbol (Cm7, Ebmaj7, G7, Fsus4, Dm7b5, C(add9), Am9, C5) and its roman numeral in the key */
static void chord_name(const gch_t *c, char *sym, char *rom)
{
    static const char *R[7] = {"I", "II", "III", "IV", "V", "VI", "VII"};
    int32_t t3 = giv(c, 2), t5 = giv(c, 4), t7 = giv(c, 6), root = (int32_t)gtone(c, 0, 12) % 12;
    int32_t rel = ((root - gs.root) % 12 + 12) % 12, dia = gs.sc[c->deg];
    uint32_t q = c->q, minor = t3 == 3, i;
    const char *tri = t5 == 6 && minor ? "dim" : t5 == 8 ? "+" : minor ? "m" : "", *ext = "";
    char r[8];
    if (q == Q_7 || q == Q_7NO5) ext = t7 == 11 ? (minor ? "(maj7)" : "maj7") : t7 == 10 ? (t5 == 6 && minor ? "7b5" : "7") : "6";
    else if (q == Q_9 || q == Q_R9) ext = t7 == 11 && !minor ? "maj9" : "9";
    else if (q == Q_ADD9) ext = "(add9)";
    else if (q == Q_6) ext = "6";
    else if (q == Q_11) ext = "11";
    if (q == Q_7 && t5 == 6 && minor && t7 == 10) tri = "m";
    if (q == Q_SUS2) { tri = "sus2"; ext = ""; }
    if (q == Q_SUS4) { tri = "sus4"; ext = ""; }
    if (q == Q_QUART) { tri = "sus4"; ext = "(7)"; }
    if (q == Q_POWER) { tri = "5"; ext = ""; }
    sprintf(sym, "%s%s%s", NN[root], tri, ext);
    sprintf(r, "%s%s", rel < dia ? "b" : rel > dia ? "#" : "", R[c->deg]);
    for (i = 0; minor && r[i]; i++) if (r[i] == 'I' || r[i] == 'V') r[i] = (char)(r[i] - 'A' + 'a');
    sprintf(rom, "%s%s%s%s", r, t5 == 6 && minor ? "o" : "", q == Q_7 || q == Q_9 || q == Q_R9 ? "7" : "", c->alt ? "*" : "");
}
static int32_t dump_lift;                               /* (a section in another key: its notes flagged in its key) */
static void dump_track(const track_t *t, uint32_t r, uint32_t bars)
{
    uint32_t i, j, len = dlen(t);
    char b[16];
    if (r == RL_DRUM) {
        static const char *LN[8] = {"BD", "SD", "CP", "CH", "OH", "TM", "RS", "CB"};
        uint32_t l;
        for (l = 0; l < 8u; l++) {
            int any = 0;
            for (i = 0; i < len; i++) any |= t->step[i].time == ST_NOTE && (t->step[i].hit >> l) & 1u;
            if (!any) continue;
            printf("  %-4s ", LN[l]);
            for (i = 0; i < bars * 16u && i < len; i++) {
                const step_t *s = &t->step[i];
                int on = s->time == ST_NOTE && (s->hit >> l) & 1u;
                if (i && i % 16u == 0u) putchar('|');
                putchar(!on ? '.' : (s->acc >> l) & 1u ? 'X' : s->vel >= 90 ? 'x' : s->vel >= 60 ? 'o' : ',');
            }
            putchar('\n');
        }
        return;
    }
    for (i = 0; i < len; i++) {
        const step_t *s = &t->step[i];
        if (i % 16u == 0u) printf(i ? "\n       | " : "       | ");
        if (s->time == ST_NOTE && s->n) {
            for (j = 0; j < s->n; j++) { nname(b, s->note[j]); printf("%s%s", j ? "/" : "", b); }
            if (!gin_mode(s->note[0] - dump_lift)) printf("!");
            printf(" ");
        } else printf(s->time == ST_TIE ? "= " : ". ");
    }
    printf("\n");
}
static void dump_piece(void)
{
    uint32_t k, i, bars = (gs.len + 15u) / 16u;
    char sym[24], rom[16];
    printf("harmony: %s%s%s%s%s%s  palette %02x  progression:", FAM[HXF(gs.hx) % 6u], gs.hx & HX_RAISE ? " RAISE" : "",
           gs.hx & HX_SEC ? " SEC" : "", gs.hx & HX_BOR ? " BORROW" : "", gs.hx & HX_LIFT ? " LIFT" : "", gs.hx & HX_APPR ? " APPR" : "", gs.pal);
    for (k = 0; k < gs.pn; k++) printf(" %u%s", gs.pdeg[k] + 1u, QF(gs.pq[k]) ? "*" : "");
    printf("   (* coloured: borrowed, secondary, raised)\nchords:");
    for (k = 0; k < gs.nch; k++) {
        chord_name(&gs.ch[k], sym, rom);
        printf(" %s[%s] @%u+%u", sym, rom, gs.ch[k].at, gs.ch[k].len);
    }
    printf("\n");
    for (i = 0; i < bars; i++) {
        printf("bar %u:", i + 1u);
        for (k = 0; k < gs.nch; k++)
            if (gs.ch[k].at / 16u == i) { chord_name(&gs.ch[k], sym, rom); printf("  %s (%s)", sym, rom); }
        printf("\n");
    }
    for (k = 0; k < NTRK; k++) {
        const track_t *t = &trk[k];
        const engine_t *e = ENGINES[t->eng_req % NENGINES];
        uint32_t r = gs.role[k];
        if (!gs.act[r]) { printf("T%u %-4s off\n", k + 1u, RLN[r]); continue; }
        printf("T%u %-4s %s %s  LEN %d%s\n", k + 1u, RLN[r], e->name, t->preset < e->npresets ? e->presets[t->preset].name : "-",
               t->p[P_SLEN], t->p[P_MUTE] ? "  MUTED" : "");
        dump_track(t, r, 4);
    }
}
int main(int argc, char **argv)
{
    uint32_t genre = argc > 1 ? (uint32_t)atoi(argv[1]) : 0, seed = argc > 2 ? (uint32_t)atoi(argv[2]) : 1;
    uint32_t energy = argc > 3 ? (uint32_t)atoi(argv[3]) : 50, form = argc > 4 ? (uint32_t)atoi(argv[4]) : 0;
    const char *wav_path = argc > 5 ? argv[5] : 0;
    double secs = argc > 6 ? atof(argv[6]) : 30.0;
    uint32_t from = genre ? genre : 1, to = genre ? genre : NGENRE, g, k, s;
    ui_power_on(); go_home(); frames(4);
    settings.palette = 0u; palette_set(0u);   /* (GARDEN, the one palette) */
    ui_rec_prefs |= PREF_SIX >> 8; rp_apply();
    song.master_q12 = 4096;
    for (g = from; g <= to; g++) {
        uint32_t pd;
        gp.genre = (uint8_t)g; gp.seed = (uint16_t)seed; gp.energy = (uint8_t)energy; gp.form = (uint8_t)form;
        for (k = 0; k < GP_N; k++) gp.amt[k] = GA_AUTO;
        gp.complex = 50; gp.swing = GA_AUTO; gp.human = 30;
        gen_go();
        pd = gs.trk_of[RL_PAD];
        spell();
        printf("\n=== %s  seed %u  energy %u  form %s  %d BPM  swing %d\n", gs.g->name, seed, energy, form == 2u ? "SONG" : form ? "PARTS" : "LOOP",
               song.g[G_BPM], song.g[G_SWING]);
        printf("SCALE page: ROOT %s  SCALE %s  QUANT %s   CHORD page (T%u): %s, VOICING %s, STRUM %d\n", NN[trk[0].p[P_ROOT] % 12],
               N_SCALE[trk[0].p[P_SCALE]], QN[trk[0].p[P_QUANT] & 3], pd + 1u, pd < NTRK ? CDN[trk[pd].p[P_CHRD] % 12] : "-",
               pd < NTRK && trk[pd].p[P_CVOI] == 1 ? "LEAD" : "CLOSE", pd < NTRK ? trk[pd].p[P_CSTR] : 0);
        dump_piece();
        if (live.mode && chain_config.count) {         /* the sections GEN stored (A is the piece above) */
            static project_t p;
            printf("song:");
            for (k = 0; k < chain_config.count; k++) printf(" %c x%u", 'A' + chain_config.row[k].slot, chain_config.row[k].bars);
            printf("\n");
            for (s = 1; s < 4u; s++) {
                uint32_t m = 0, mel = 0;
                if (!((dice_secs >> s) & 1u) || !proj_import(&p, &proj_slot[s], sizeof(project_store_t))) continue;
                for (k = 0; k < NTRK; k++) if (gs.role[k] != RL_DRUM) { mel = k; break; }
                printf("--- section %c: ROOT %s SCALE %s; muted:", 'A' + s, NN[p.t[mel].p[P_ROOT] % 12], N_SCALE[p.t[mel].p[P_SCALE]]);
                dump_lift = ((p.t[mel].p[P_ROOT] - gs.root) % 12 + 12) % 12;
                for (k = 0; k < NTRK; k++) if (p.t[k].p[P_MUTE]) { printf(" T%u", k + 1u); m++; }
                printf(m ? "\n" : " none\n");
                for (k = 0; k < NTRK; k++) {
                    track_t tt;
                    uint32_t r = gs.role[k];
                    if (r == RL_DRUM || !gs.act[r] || p.t[k].p[P_MUTE] || (r == RL_TEX && k != gs.trk_of[RL_TEX])) continue;
                    memset(&tt, 0, sizeof tt);
                    memcpy(tt.step, p.t[k].step, sizeof tt.step);
                    proj_steps(tt.step, p.t[k].engine);
                    tt.p[P_SLEN] = p.t[k].p[P_SLEN];
                    printf("T%u %-4s\n", k + 1u, RLN[r]);
                    dump_track(&tt, r, 4);
                }
            }
        }
        dump_lift = 0;
        if (wav_path && genre) {                        /* the piece heard: its song (PARTS, SONG) or its loop */
            FILE *w = fopen(wav_path, "wb");
            uint32_t nsamp = 0, want = (uint32_t)(secs * FS), f = 0, i;
            if (!w) { fprintf(stderr, "cannot write %s\n", wav_path); return 1; }
            wav_hdr(w, 0);
            if (live.mode) chain_play_ui(); else transport_req = 1;
            while (nsamp < want) {
                uint32_t upto = (uint32_t)((uint64_t)(++f) * FS / 30u);
                ui_input();
                song_poll();
                while (nsamp < upto && nsamp < want) {
                    int32_t o[2 * CTL];
                    int16_t sm[2 * CTL];
                    mix_block(o, CTL);
                    for (i = 0; i < 2u * CTL; i++) sm[i] = (int16_t)clamp(o[i], -32768, 32767);
                    fwrite(sm, 2, 2u * CTL, w);
                    nsamp += CTL;
                }
                host_ticks += 1000000u / 30u; fm1_ms += 1000u / 30u;
            }
            transport_req = 2; ui_input();
            fclose(w);
            if ((w = fopen(wav_path, "r+b"))) { wav_hdr(w, nsamp); fclose(w); }
            printf("wav: %s (%.0f s)\n", wav_path, secs);
        }
    }
    return 0;
}
