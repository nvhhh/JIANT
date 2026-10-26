/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Juan Ignacio Antonucci (Juan Jiant)
 * (0.9.17) the dice measured (not a test: numbers): consecutive presses of every DICE path from a fresh power-on and
 * mid-session, as the owner presses them. GEN (HOME + SELECT, OCT+ again: a new seed) per genre and EXPLORE; SEQ +
 * SELECT (the selected track's sequence); EDIT + SELECT (its sound). Per role: the share of a press's part the same as
 * the press before (exact, near: 90 % of its steps, transposition-invariant) and as one of the 8 before; the distinct
 * presets; the lead's melody (in the key, steps, leaps past a fifth and their resolution, range, rests, syncopation,
 * bar-to-bar reuse); the envelopes (static full-sustain, no release), the filter (cutoff, resonance, envelope, drive),
 * LFO and vibrato, the kit's sends.
 *   cc -O1 -w -Ibuild/gen -Ifirmware/src -Itests -o build/host/dice_metrics tests/dice_metrics.c -lm
 *   build/host/dice_metrics [presses]   (default 40) */
#define UI_TEST_NO_MAIN 1
#include "ui_test.c"

#define DM_MAXP 200u
#define DM_NR 6u
#define DM_HIST 8u
typedef struct { uint16_t len; int16_t v[NSTEP]; } dm_seq_t;
static dm_seq_t dm_last[DM_NR][DM_HIST];
static uint32_t dm_nh[DM_NR];
static uint32_t dm_lcg = 12345u;
static uint32_t dm_rnd(void) { dm_lcg = dm_lcg * 1103515245u + 12345u; return dm_lcg >> 8; }

static void dm_take(const track_t *t, int drum, int32_t ref, dm_seq_t *q)
{
    uint32_t i, len = dlen(t);
    q->len = (uint16_t)len;
    for (i = 0; i < len; i++) {
        const step_t *s = &t->step[i];
        q->v[i] = drum ? (int16_t)(s->time == ST_NOTE ? s->hit : 0) :
                  (int16_t)(s->time == ST_NOTE && s->n ? 200 + s->note[0] - ref : s->time == ST_TIE ? 1 : 0);
    }
}
static int dm_same(const dm_seq_t *a, const dm_seq_t *b, uint32_t pct)
{
    uint32_t i, eq = 0, n = a->len > b->len ? a->len : b->len, any = 0;
    if (a->len != b->len) return 0;
    for (i = 0; i < n; i++) { eq += a->v[i] == b->v[i]; any |= a->v[i] | b->v[i]; }
    return any && eq * 100u >= n * pct;
}
typedef struct { uint32_t n, ex1, near1, ex8, near8; } dm_dup_t;
static void dm_note_dup(uint32_t r, const dm_seq_t *q, dm_dup_t *d)
{
    uint32_t k, e8 = 0, n8 = 0, h = dm_nh[r] < DM_HIST ? dm_nh[r] : DM_HIST;
    int any = 0;
    for (k = 0; k < q->len; k++) any |= q->v[k];
    if (!any) return;
    d->n++;
    if (h) {
        const dm_seq_t *p = &dm_last[r][(dm_nh[r] - 1u) % DM_HIST];
        d->ex1 += dm_same(q, p, 100); d->near1 += dm_same(q, p, 90);
    }
    for (k = 0; k < h; k++) { e8 |= dm_same(q, &dm_last[r][k], 100); n8 |= dm_same(q, &dm_last[r][k], 90); }
    d->ex8 += e8; d->near8 += n8;
    dm_last[r][dm_nh[r] % DM_HIST] = *q;
    dm_nh[r]++;
}
/* the melody's numbers */
typedef struct { uint32_t notes, out, iv, step, rep, leap5, leap5r, range, ranges, steps, rest, onsets, sync, bars, reuse, ct, strong; } dm_mel_t;
static void dm_melody(const track_t *t, dm_mel_t *m, int harm)
{
    uint32_t i, len = dlen(t), pend = 0;
    int32_t prev = -1, piv = 0, lo = 127, hi = 0;
    uint16_t bar0 = 0;
    for (i = 0; i < len; i++) {
        const step_t *s = &t->step[i];
        m->steps++;
        if (s->time == ST_REST) m->rest++;
        if (s->time != ST_NOTE || !s->n) continue;
        {
            int32_t n = s->note[0], iv, a;
            uint32_t mask = scale_mask(t);
            m->notes++; m->onsets++;
            m->sync += (i & 1u);
            m->out += !((mask >> (uint32_t)((n - t->p[P_ROOT] + 120) % 12)) & 1u);
            if (n < lo) lo = n;
            if (n > hi) hi = n;
            if (i < 16u) bar0 |= (uint16_t)(1u << i);
            if (harm && i % 4u == 0u) { m->strong++; m->ct += (gchord_pcs(i) >> (uint32_t)(n % 12)) & 1u; }
            if (prev >= 0) {
                iv = n - prev; a = iv < 0 ? -iv : iv;
                m->iv++; m->rep += !a; m->step += a && a <= 2;
                if (pend) { m->leap5r += a && a <= 2 && (iv > 0) != (piv > 0); pend = 0; }
                if (a > 7) { m->leap5++; pend = 1; piv = iv; }
            }
            prev = n;
        }
    }
    if (hi >= lo) { m->range += (uint32_t)(hi - lo); m->ranges++; }
    for (i = 16; i + 16u <= len; i += 16u) {
        uint16_t bb = 0;
        uint32_t q, d = 0;
        for (q = 0; q < 16u; q++) if (t->step[i + q].time == ST_NOTE && t->step[i + q].n) bb |= (uint16_t)(1u << q);
        if (!bb || !bar0) continue;
        for (q = 0; q < 16u; q++) d += ((bb ^ bar0) >> q) & 1u;
        m->bars++; m->reuse += d <= 3u;
    }
}
static void dm_mel_print(const char *what, const dm_mel_t *m)
{
    printf("  %-12s in key %5.1f%%  steps %4.1f%%  repeats %4.1f%%  leaps>5th %4.1f%% (resolved %3.0f%%)  range %4.1f st  rests %4.1f%%  "
           "offbeat onsets %4.1f%%  bars reusing bar 1 %4.1f%%  strong on chord tones %4.1f%%\n", what,
           m->notes ? 100.0 * (m->notes - m->out) / m->notes : 0, m->iv ? 100.0 * m->step / m->iv : 0, m->iv ? 100.0 * m->rep / m->iv : 0,
           m->iv ? 100.0 * m->leap5 / m->iv : 0, m->leap5 ? 100.0 * m->leap5r / m->leap5 : 0, m->ranges ? (double)m->range / m->ranges : 0,
           m->steps ? 100.0 * m->rest / m->steps : 0, m->onsets ? 100.0 * m->sync / m->onsets : 0, m->bars ? 100.0 * m->reuse / m->bars : 0,
           m->strong ? 100.0 * m->ct / m->strong : 0);
}
/* the sound's numbers, a role */
typedef struct { uint32_t n, flat, held, susv, relv, swell, pluck, cut, cutn, res, envf, drive, lfof, vib, low; uint16_t pre[DM_MAXP]; uint32_t npre, pre_rep1; } dm_snd_t;
static void dm_sound(const track_t *t, dm_snd_t *s)
{
    uint32_t e = t->eng_req, id = e << 8 | t->preset, k;
    int32_t cut = e == EG_A ? t->p[P_E4] : t->p[P_FCUT];
    s->n++;
    s->flat += t->p[P_SUS] >= 120 && t->p[P_REL] <= 12;
    s->held += t->p[P_SUS] >= 100; s->susv += (uint32_t)t->p[P_SUS]; s->relv += (uint32_t)t->p[P_REL];
    s->swell += t->p[P_ATK] >= 40;
    s->pluck += t->p[P_SUS] <= 65 && t->p[P_DEC] <= 78;
    s->cut += (uint32_t)cut; s->cutn++; s->low += cut < 80;
    s->res += (uint32_t)(e == EG_A ? t->p[P_E5] : t->p[P_FRES]);
    s->envf += (uint32_t)(t->p[P_ED_FLT] > 0 ? t->p[P_ED_FLT] : 0);
    s->drive += t->p[P_DIST] > 0;
    s->lfof += t->p[P_LD_FLT] != 0;
    s->vib += t->p[P_LD_PIT] != 0;
    for (k = 0; k < s->npre && k < DM_MAXP; k++) if (s->pre[k] == id) break;
    if (k == s->npre && s->npre < DM_MAXP) s->pre[s->npre++] = (uint16_t)id;
}
static void dm_snd_print(const char *what, const dm_snd_t *s)
{
    if (!s->n) return;
    printf("  %-6s n %3u  presets %3u  static (SUS>=95%%, REL~0) %4.1f%%  SUS>=79%% %4.1f%%  SUS %4.1f REL %4.1f  pluck %4.1f%%  swell %4.1f%%  cutoff %5.1f (under 80: %4.1f%%)  "
           "reso %4.1f  env>flt %4.1f  drive %4.1f%%  LFO>flt %4.1f%%  vibrato %4.1f%%\n", what, s->n, s->npre,
           100.0 * s->flat / s->n, 100.0 * s->held / s->n, (double)s->susv / s->n, (double)s->relv / s->n, 100.0 * s->pluck / s->n, 100.0 * s->swell / s->n, s->cutn ? (double)s->cut / s->cutn : 0,
           s->cutn ? 100.0 * s->low / s->cutn : 0, (double)s->res / s->n, (double)s->envf / s->n, 100.0 * s->drive / s->n,
           100.0 * s->lfof / s->n, 100.0 * s->vib / s->n);
}
static const char *const RLN[DM_NR] = {"DRUMS", "BASS", "CHORDS", "LEAD", "ARP", "TEX"};
static void dm_reset(void)
{
    ui_power_on();
    ui_rec_prefs |= PREF_SIX >> 8; rp_apply();
    settings.palette = 0u; palette_set(0u);   /* (GARDEN, the one palette) */
    memset(dm_nh, 0, sizeof dm_nh);
}
static void dm_wait(void) { uint32_t d = 600000u + dm_rnd() % 2400000u; host_ticks += d * 2u; fm1_ms += d / 1000u; }

int main(int argc, char **argv)
{
    uint32_t N = argc > 1 ? (uint32_t)atoi(argv[1]) : 40u, g, p, k, r;
    dm_dup_t tot[DM_NR] = {{0}};
    dm_mel_t mel_lead = {0}, mel_arp = {0}, mel_bass = {0};
    dm_snd_t snd_all[DM_NR] = {{0}};
    uint32_t seeds_same = 0, kit_send = 0, kit_lanes = 0, kit_n = 0;
    uint32_t cost_max = 0;
    double cost_sum = 0;
    if (N > DM_MAXP) N = DM_MAXP;
    if (argc > 2) {                                     /* (prints: every genre's fingerprint at three seeds) */
        for (g = 0; g <= NGENRE + 1u; g++)
            for (p = 0; p < 3u; p++) {
                dm_reset();
                gp.genre = (uint8_t)g; gp.seed = (uint16_t)(p * 1234u + 7u); gp.energy = 50; gp.form = (uint8_t)(p % 3u);
                gp.complex = 50; gp.swing = GA_AUTO; gp.human = 30;
                for (k = 0; k < GP_N; k++) gp.amt[k] = GA_AUTO;
                gen_go();
                printf("%u %u %08x\n", g, p, dice_print());
            }
        return 0;
    }
    printf("== GEN: %u presses of OCT+ a genre (the same settings: a new seed each), mid-session\n", N);
    for (g = 0; g <= NGENRE; g++) {             /* (NGENRE: EXPLORE) */
        dm_dup_t d[DM_NR] = {{0}};
        dm_snd_t sn[DM_NR] = {{0}};
        uint32_t lastp[DM_NR], prep[DM_NR] = {0};
        uint16_t lastseed = 0xFFFF;
        dm_reset();
        for (k = 0; k < DM_NR; k++) lastp[k] = 0xFFFFu;
        gp.genre = (uint8_t)(g < NGENRE ? g + 1u : GEN_EXPLORE);
        gp.energy = 50; gp.form = 0; gp.complex = 50; gp.swing = GA_AUTO; gp.human = 30;
        for (k = 0; k < GP_N; k++) gp.amt[k] = GA_AUTO;
        gp.seed = (uint16_t)(dm_rnd() % 10000u);
        for (p = 0; p < N; p++) {
            struct timespec t0, t1;
            uint32_t us;
            dm_wait();
            clock_gettime(CLOCK_MONOTONIC, &t0);
            gen_press();
            clock_gettime(CLOCK_MONOTONIC, &t1);
            us = (uint32_t)((t1.tv_sec - t0.tv_sec) * 1000000 + (t1.tv_nsec - t0.tv_nsec) / 1000);
            cost_sum += us; if (us > cost_max) cost_max = us;
            seeds_same += gp.seed == lastseed; lastseed = gp.seed;
            for (k = 0; k < NTRK; k++) {
                const track_t *t = &trk[k];
                dm_seq_t q;
                r = gs.role[k];
                if (!gs.act[r] || (r == RL_TEX && k != gs.trk_of[RL_TEX])) continue;
                dm_take(t, r == RL_DRUM, gs.root, &q);
                dm_note_dup(r, &q, &d[r]);
                if (r != RL_DRUM) {
                    uint32_t id = t->eng_req << 8 | t->preset;
                    dm_sound(t, &sn[r]); dm_sound(t, &snd_all[r]);
                    prep[r] += id == lastp[r]; lastp[r] = id;
                }
                if (r == RL_LEAD) dm_melody(t, &mel_lead, 1);
                if (r == RL_ARP) dm_melody(t, &mel_arp, 1);
                if (r == RL_BASS) dm_melody(t, &mel_bass, 1);
                if (r == RL_DRUM) {
                    uint32_t l;
                    kit_n++;
                    for (l = 0; l < 8u; l++) { kit_lanes++; kit_send += (dx_snd[l] & 63u) != 0; }
                }
            }
        }
        printf("%-13s", g < NGENRE ? GENRE[g].name : "EXPLORE");
        for (r = 0; r < DM_NR; r++) {
            if (!d[r].n) { printf(" %s -", RLN[r]); continue; }
            printf(" %s %2u/%2u/%2u", RLN[r], d[r].ex1 * 100u / d[r].n, d[r].near1 * 100u / d[r].n, d[r].near8 * 100u / d[r].n);
            if (r) printf("(p%u,r%u)", sn[r].npre, prep[r]);
            tot[r].n += d[r].n; tot[r].ex1 += d[r].ex1; tot[r].near1 += d[r].near1; tot[r].ex8 += d[r].ex8; tot[r].near8 += d[r].near8;
        }
        printf("\n");
    }
    printf("  (a role: %% the same as the press before exact / near / near one of the 8 before; p distinct presets, r the same preset as before)\n");
    printf("GEN all genres:");
    for (r = 0; r < DM_NR; r++) if (tot[r].n) printf(" %s %.1f/%.1f/%.1f%%", RLN[r], 100.0 * tot[r].ex1 / tot[r].n, 100.0 * tot[r].near1 / tot[r].n, 100.0 * tot[r].near8 / tot[r].n);
    printf("\n  seeds repeated back to back: %u; a press %.2f ms on the host (the most %.2f ms)\n", seeds_same, cost_sum / ((NGENRE + 1u) * N) / 1000.0, cost_max / 1000.0);
    dm_mel_print("GEN lead", &mel_lead); dm_mel_print("GEN arp", &mel_arp); dm_mel_print("GEN bass", &mel_bass);
    for (r = 1; r < DM_NR; r++) dm_snd_print(RLN[r], &snd_all[r]);
    printf("  kit: sounds with a send %.1f%%\n", kit_lanes ? 100.0 * kit_send / kit_lanes : 0);

    /* ---- power-on: the first GEN press after five power-ons (another time each) */
    {
        uint32_t h[5], same = 0, i, j;
        for (i = 0; i < 5u; i++) {
            dm_reset();
            rng_state = 0x1234567u;
            host_ticks = 1000000u + i * 7777777u;
            session_dice(host_ticks ^ (i * 3u) << 16);   /* (main.c: the ticks and the battery's ADC) */
            gp.genre = GEN_EXPLORE; gp.sig = 0;   /* (gp as at power-on: its SEED session_dice's) */
            gen_press();
            h[i] = dice_print();
            for (j = 0; j < i; j++) same += h[j] == h[i];
        }
        printf("== power-on: the first GEN press after 5 power-ons: %u pairs the same piece of 10\n", same);
    }
    /* ---- SEQ + SELECT: the selected track's sequence diced, N times, on a GEN piece's lead, bass, drums */
    {
        uint32_t roles[3] = {RL_LEAD, RL_BASS, RL_DRUM}, ri;
        printf("== SEQ + SELECT (dice_seq): %u presses on a HOUSE piece's track\n", N);
        for (ri = 0; ri < 3u; ri++) {
            dm_dup_t d = {0};
            dm_mel_t m = {0};
            uint32_t tk;
            dm_reset();
            gp.genre = 1; gp.seed = 77; gp.energy = 50; gp.form = 0; gp.complex = 50; gp.swing = GA_AUTO; gp.human = 30;
            for (k = 0; k < GP_N; k++) gp.amt[k] = GA_AUTO;
            gen_go();
            tk = gs.trk_of[roles[ri]];
            if (tk >= NTRK) continue;
            song.sel = (uint8_t)tk;
            for (p = 0; p < N; p++) {
                dm_seq_t q;
                dm_wait();
                dice_seq(&trk[tk]);
                dm_take(&trk[tk], roles[ri] == RL_DRUM, trk[tk].p[P_ROOT], &q);
                dm_note_dup(roles[ri], &q, &d);
                if (roles[ri] != RL_DRUM) dm_melody(&trk[tk], &m, 1);
            }
            printf("  %-6s the same as the press before %u%% exact %u%% near, near one of the 8 before %u%%\n", RLN[roles[ri]],
                   d.n ? d.ex1 * 100u / d.n : 0, d.n ? d.near1 * 100u / d.n : 0, d.n ? d.near8 * 100u / d.n : 0);
            if (roles[ri] != RL_DRUM) dm_mel_print(RLN[roles[ri]], &m);
        }
    }
    /* ---- EDIT + SELECT: the sound diced, N times a synth engine */
    {
        static const uint8_t EN[5] = {EG_A, EG_FM, EG_PH, EG_LO, EG_VO};
        uint32_t ei;
        printf("== EDIT + SELECT (sound_dice_of): %u presses an engine\n", N);
        for (ei = 0; ei < 5u; ei++) {
            dm_snd_t s = {0};
            uint32_t rep = 0, last = 0xFFFF, in8 = 0, h[8], nh = 0, j;
            dm_reset();
            set_engine_of(&trk[1], EN[ei]);
            for (p = 0; p < N; p++) {
                uint32_t hit = 0;
                dm_wait();
                sound_dice_of(&trk[1], 1);
                dm_sound(&trk[1], &s);
                rep += trk[1].preset == last; last = trk[1].preset;
                for (j = 0; j < (nh < 8u ? nh : 8u); j++) hit |= h[j] == trk[1].preset;
                in8 += hit; h[nh++ % 8u] = trk[1].preset;
            }
            printf("  %-7s of %2u presets: %2u chosen, the same as before %u, one of the 8 before %u;", ENGINES[EN[ei]]->name,
                   ENGINES[EN[ei]]->npresets, s.npre, rep, in8);
            printf(" static env %.0f%%\n", 100.0 * s.flat / s.n);
        }
    }
    return 0;
}
