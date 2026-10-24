/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Juan Ignacio Antonucci (Juan Jiant)
 * JIANT (0.6.91) the song generator, second version: HOME + SELECT opens GEN, a screen of three pages (GENRE SEED
 * ENERGY FORM; DRUMS BASS CHORDS ARP; LEAD COMPLEX SWING HUMAN), OCT+ makes the piece, HOME (or OCT-) is done. Included by
 * ui_input.c after song_dice.c (whose helpers it shares: the roles, dhold, euclid, dname_has, dlevel ..).
 *
 * The piece is deterministic: a seed and the knobs make the same piece every time (the same settings and OCT+ again
 * throw a new seed). Each part (drums, bass, chords, arp, lead, sounds, form) draws from its own stream, so turning
 * one part's knob leaves the others as they were. What the tables know (gen_tables.c): the genre's tempo, modes and
 * progressions, its drums as a map of priorities (a hit where the level clears the density: ghosts go first, anchors
 * last), its bass as one of ~50 archetypes written against the kick, its sounds as recipes a role, its kit, HUMAN by
 * how human the genre is, the snare laid back where the genre does. FORM: a loop, parts (A B A C in ARRANGE, as
 * 0.6.81), or a club song (intro, build with a filter opening and a roll, drop, break). UNDO (GLO + G4) brings back
 * what was there.
 * (0.8.3) Composed, not thrown: the key a mode of seven written to the SCALE page (QUANT SNAP) and the chord track's
 * CHORD page set to the piece's chords, so the keys play along in key; the harmony a plan of chords over the loop
 * (gplan: the genre's progression as degrees of the mode, a mode that would turn a chord diminished taking another;
 * the minor key's dominant raised at its cadence, jazz's turnaround, secondary dominants and mixture where the genre
 * colours, each a chord's alterations off the mode); every part reads the plan: the bass the chord's root at each change
 * and its own third, fifth, seventh, approaches that resolve; the chords voiced voice by voice (gvoice: common tones
 * held, the least motion, close, spread or rootless by the genre, no mud below G3); the arp the voiced chord; the lead
 * composed (gen_lead: a motif developed by a phrase template, chord tones on the beats, steps, a leap turned back, a
 * cadence on a stable degree, the borrowed chords' tones followed). The sections relate: B on the predominant, the pop
 * genres' last section a whole step up (a real modulation, its SCALE page stored with it). Every pitched note is in the
 * key but the rules' chromaticism (tests/gen_test.c measures it).
 * (0.8.6) GENRE's first choice, and where the screen always opens: EXPLORE, no genre, every rule broken (gx_genre). */

#include "gen_tables.c"

/* ------------------------------------------------------------------------------------------- the settings --- */
static struct {
    uint8_t genre;                                      /* 0 RANDOM, 1 + the genre, (0.8.6) GEN_EXPLORE */
    uint16_t seed;                                      /* 0..9999 */
    uint8_t energy, form;                               /* 0..100; 0 LOOP, 1 PARTS, 2 SONG */
    uint8_t amt[GP_N];                                  /* 0 OFF .. 100, GA_AUTO the genre's */
    uint8_t complex, swing, human;                      /* 0..100 (swing: GA_AUTO the genre's) */
    uint8_t page, hot;                                  /* the GEN screen's page, its last knob */
    uint32_t sig;                                       /* the settings of the last piece made (OCT+ again: a new seed) */
    char last[20];                                      /* the last piece: "HOUSE 0421" */
} gp = {GEN_EXPLORE, 1, 50, 1, {GA_AUTO, GA_AUTO, GA_AUTO, GA_AUTO, GA_AUTO}, 50, GA_AUTO, 30, 0, 0, 0, ""};

/* the generator's own random numbers: a stream a part, from the seed */
static uint32_t gq = 1;
static uint32_t gr(void) { gq ^= gq << 13; gq ^= gq >> 17; gq ^= gq << 5; return gq; }
static uint32_t gseed_mix(uint32_t part);
static void gseed(uint32_t part)
{
    gq = ((uint32_t)gp.seed + 1u) * 2654435761u ^ (part + 1u) * 0x9E3779B9u ^ 0x6D2B79F5u ^
         gseed_mix(part);                               /* (0.8.3: a genre's own motifs, figures and drums) */
    if (!gq) gq = 1;
    gr(); gr(); gr();
}
static int32_t grr(int32_t lo, int32_t hi) { return hi <= lo ? lo : lo + (int32_t)(gr() % (uint32_t)(hi - lo + 1)); }
static int gch(uint32_t pct) { return gr() % 100u < pct; }
static uint32_t gpickw(const uint8_t *w, uint32_t n)    /* an index by its weight */
{
    uint32_t i, s = 0, r;
    for (i = 0; i < n; i++) s += w[i];
    if (!s) return 0;
    r = gr() % s;
    for (i = 0; i < n; i++) { if (r < w[i]) return i; r -= w[i]; }
    return n - 1u;
}
enum { GS_PICK, GS_DRUMS, GS_BASS, GS_CHORDS, GS_ARP, GS_LEAD, GS_SOUND, GS_FORM, GS_MIX, GS_TEX, GS_PLAN };

/* ------------------------------------------------------------------------------------------------ the piece --- */
/* (0.8.3) a chord of the plan: its first step, its steps, its root's degree in the mode, its quality (QSTACK) and how
 * its root, third, fifth and seventh move off the mode (bit pairs 0 2 4 6: 1 a semitone up, 2 down; 0 the mode's) */
typedef struct { uint8_t at, len, deg, q, alt; } gch_t;
#define GCH_N 8u
/* (0.9.19) the generator's parts: T1..T6, as before the eighth track. T7 and T8 are never written by a generated piece
 * (no roles, no RNG draws for them), so the pieces and every golden render stay as they were */
#define GEN_NT 6u
static struct {
    const ggenre_t *g;
    uint32_t gi;
    int16_t root, scale;                                /* the key, as the SCALE page has it (always a mode of seven) */
    uint8_t sc[7];                                      /* (0.8.3) the mode: its degrees' semitones over the tonic */
    uint8_t pal, hx;                                    /* the lead's degrees (bit d: degree d of the mode), the harmony */
    uint8_t pdeg[4], pq[4], pn;                         /* the progression as chosen: degrees, qualities with their colour */
    gch_t ch[GCH_N]; uint8_t nch;                       /* the plan: the chords over the loop */
    uint32_t len, cst;                                  /* the loop, a chord's steps */
    uint32_t amt[GP_N], energy, complex, human, swing;
    uint64_t kick, snare;                               /* the kick's steps, the snare's (bit i; the bass reads them) */
    int32_t vo[4]; uint32_t nvo;                        /* the chord track's last voicing */
    int32_t avo[4]; uint32_t navo;                      /* the arp's */
    uint32_t fm6, ba, vm;
    uint8_t role[GEN_NT], act[RL_N], trk_of[RL_N];
    uint32_t drum_fill;                                 /* the fill forced (a BUILD section) */
    uint8_t mutant;                                     /* (0.6.95) RANDOM: a hybrid of two or three genres */
    uint16_t snd[GEN_NT]; uint8_t nsnd;                   /* (0.9.17) the piece's sounds so far (engine << 8 | preset) */
} gs;
#define GKICK(i) ((uint32_t)(gs.kick >> ((i) & 63u)) & 1u)
static uint32_t gseed_mix(uint32_t part) { return part == GS_PICK ? 0u : (gs.gi + 1u) * 0x85EBCA6Bu; }
static ggenre_t gmut;                                   /* (0.6.95) the genre as this throw plays it (its neighbours' parts) */
static char gmut_name[20];

static void gclear(step_t *s) { memset(s, 0, sizeof *s); s->time = ST_REST; }
/* (0.8.6) EXPLORE: on (this piece is EXPLORE's), dirty (an EXPLORE piece set DIV and PLAYHEADS, which a genre's piece
 * puts back), the engine gen_sound takes (-1: the recipe's), ANIMAL and VOICE tracks so far (two each at most, as FM6);
 * (0.9.13) w: how wild (the COMPLEX knob: 100 every rule broken, as 0.8.6 .. 0.9.12), nmax: a chord's notes at most
 * (the CPU budget), np: the tracks so far in a loop or DIV of their own */
static struct { uint8_t on, dirty, nan, nvo; int8_t eng; uint8_t w, nmax, np; } gx = {0, 0, 0, 0, -1, 100, 4, 0};
static int gxw(uint32_t pct) { return gch(pct * gx.w / 100u); }   /* (0.9.13) EXPLORE: a chance, as wild as COMPLEX */

static void gnote(step_t *s, const int32_t *n, uint32_t cnt, int32_t vel, uint32_t flags)
{
    uint32_t j;
    int32_t top = 0;
    if (gx.on && cnt > gx.nmax) cnt = gx.nmax;          /* (0.9.13) EXPLORE's CPU budget: the top notes left out */
    s->n = (uint8_t)cnt;
    for (j = 0; j < 4u; j++) {
        s->note[j] = (uint8_t)(j < cnt ? clamp(n[j], 0, 127) : 0);
        if (j < cnt && n[j] > top) top = n[j];
    }
    s->time = ST_NOTE;
    s->flags = (uint8_t)(flags | (s->flags & SF_MFINE));   /* (0.9.1: MICRO's 64ths stay with its sixteenth) */
    if (top > 74) vel -= (top - 74) * 2 < 30 ? (top - 74) * 2 : 30;   /* (high notes measured: softer) */
    s->vel = (uint8_t)clamp(vel, 30, 127);
}

/* ---------------------------------------------------------------------------------- (0.8.3) the harmony --- */
/* the mode's degree d (any integer: 7 the octave) from the tonic at or above lo; its pitch class */
static int32_t gnd(int32_t lo, int32_t d)
{
    int32_t o = d >= 0 ? d / 7 : -((6 - d) / 7);
    return lo + ((gs.root - lo) % 12 + 12) % 12 + 12 * o + gs.sc[d - 7 * o];
}
static uint32_t gpc(int32_t d) { return (uint32_t)(gs.root + gs.sc[((d % 7) + 7) % 7]) % 12u; }
static int gminor(void) { return gs.sc[2] == 3u; }      /* the mode's third minor */
static int gin_mode(int32_t n)
{
    uint32_t pc = (uint32_t)(n - gs.root + 120) % 12u, d;
    for (d = 0; d < 7u; d++) if (gs.sc[d] == pc) return 1;
    return 0;
}
/* a chord's stack step s (0 its root, 2 its third, 4 its fifth ..) as a note from the tonic at or above lo, coloured */
static int32_t galt(const gch_t *c, int32_t s)
{
    uint32_t k = (uint32_t)(((s % 7) + 7) % 7), a;
    if (k & 1u) return 0;                               /* (its 2nd, 4th, 6th: the mode's) */
    a = (c->alt >> k) & 3u;
    return a == 1u ? 1 : a == 2u ? -1 : 0;
}
static int32_t gtone(const gch_t *c, int32_t s, int32_t lo) { return gnd(lo, c->deg + s) + galt(c, s); }
static int32_t giv(const gch_t *c, int32_t s) { return gtone(c, s, 12) - gtone(c, 0, 12); }   /* over its root */
/* its pitch classes as a line takes them: the stack, the root and the fifth, the third (not a suspension's) */
static uint32_t gpcm(const gch_t *c)
{
    uint32_t m = 0, j, q = c->q;
    for (j = 0; j < 4u && QSTACK[q][j] != 127; j++) m |= 1u << ((uint32_t)gtone(c, QSTACK[q][j], 12) % 12u);
    m |= 1u << ((uint32_t)gtone(c, 0, 12) % 12u) | 1u << ((uint32_t)gtone(c, 4, 12) % 12u);
    if (q != Q_SUS2 && q != Q_SUS4) m |= 1u << ((uint32_t)gtone(c, 2, 12) % 12u);
    return m;
}
static uint32_t gci(uint32_t step)                      /* the plan's chord at a step of any track's loop */
{
    uint32_t j = gs.nch ? gs.nch - 1u : 0u, s = gs.len ? step % gs.len : step;
    while (j && gs.ch[j].at > s) j--;
    return j;
}
static const gch_t *gchord(uint32_t step) { return &gs.ch[gci(step)]; }
static const gch_t *gchord_next(uint32_t step) { return &gs.ch[gs.nch ? (gci(step) + 1u) % gs.nch : 0u]; }
static int gchord_starts(uint32_t step) { return gs.len && gchord(step)->at == step % gs.len; }
static uint32_t gchord_pcs(uint32_t step) { return gpcm(gchord(step)); }
static uint32_t gchord_root_pc(uint32_t step) { return (uint32_t)gtone(gchord(step), 0, 12) % 12u; }
/* a chord's colour (F_MAJ F_DOM F_PAR F_FLAT) as moves of its tones off the mode; 0: not a chord this mode can take */
static int gcolour(gch_t *c, uint32_t f)
{
    static const uint8_t AEOL[7] = {0, 2, 3, 5, 7, 8, 10}, ION[7] = {0, 2, 4, 5, 7, 9, 11};
    int32_t have[4], want[4];
    uint32_t k;
    const uint8_t *P = 0;
    for (k = 0; k < 4u; k++) { uint32_t d = c->deg + 2u * k; have[k] = want[k] = gs.sc[d % 7u] + 12 * (int32_t)(d / 7u); }
    if (f == F_MAJ || f == F_DOM) {
        want[1] = have[0] + 4; want[2] = have[0] + 7;
        if (f == F_DOM) want[3] = have[0] + 10;
    } else if (f == F_PAR) P = gminor() ? ION : AEOL;   /* (the parallel mode: major's minor, minor's major) */
    else if (f == F_FLAT && have[0] != AEOL[c->deg]) P = AEOL;   /* (the flat side's: the mode's own when it has it) */
    for (k = 0; P && k < 4u; k++) { uint32_t d = c->deg + 2u * k; want[k] = P[d % 7u] + 12 * (int32_t)(d / 7u); }
    if (P && f == F_FLAT) { want[1] = want[0] + 4; want[2] = want[0] + 7; }
    c->alt = 0;
    for (k = 0; k < 4u; k++) {
        int32_t d = want[k] - have[k];
        if (d > 1 || d < -1) return 0;
        c->alt |= (uint8_t)((d > 0 ? 1u : d < 0 ? 2u : 0u) << (2u * k));
    }
    return 1;
}
static int gtriad_ok(const gch_t *c)                    /* major or minor (diminished: ii and vii in jazz and film, m7b5) */
{
    int32_t t3 = giv(c, 2), t5 = giv(c, 4);
    if (t5 == 7 && (t3 == 3 || t3 == 4)) return 1;
    return t5 == 6 && t3 == 3 && (HXF(gs.hx) == HF_JAZZ || HXF(gs.hx) == HF_CINE) && (c->deg == 1u || c->deg == 6u);
}
/* the quality as this degree of the mode takes it: a natural ninth only (never b9), no 11 over a major third, a 6
 * major, a fourth perfect, a fifth perfect; a coloured chord a triad or a seventh */
static uint32_t gq_fix(const gch_t *c, uint32_t q)
{
    if (c->alt) return q == Q_TRI || q == Q_SUS2 || q == Q_SUS4 || q == Q_POWER || q == Q_ADD9 ? (q == Q_POWER ? q : Q_TRI) : Q_7;
    if ((q == Q_7 || q == Q_9) && giv(c, 2) == 3 && giv(c, 6) == 11 && HXF(gs.hx) != HF_JAZZ && HXF(gs.hx) != HF_CINE)
        q = giv(c, 8) == 14 ? Q_ADD9 : Q_TRI;          /* (a minor-major seventh: the spy's chord, jazz's and film's only) */
    switch (q) {
    case Q_9: case Q_R9: if (giv(c, 8) != 14) q = Q_7; break;
    case Q_ADD9: if (giv(c, 8) != 14) q = Q_TRI; break;
    case Q_11: q = giv(c, 2) == 4 || giv(c, 8) != 14 ? (giv(c, 8) == 14 ? Q_9 : Q_7) : q; break;
    case Q_6: if (giv(c, 5) != 9) q = Q_7; break;
    case Q_SUS4: if (giv(c, 3) != 5) q = giv(c, 1) == 2 ? Q_SUS2 : Q_TRI; break;
    case Q_SUS2: if (giv(c, 1) != 2) q = giv(c, 3) == 5 ? Q_SUS4 : Q_TRI; break;
    case Q_QUART: if (giv(c, 3) != 5 || giv(c, 6) != 10) q = giv(c, 3) == 5 ? Q_SUS4 : Q_TRI; break;
    case Q_POWER: if (giv(c, 4) != 7) q = Q_TRI; break;
    default: break;
    }
    return q;
}
/* a progression the mode and the genre take: every chord major or minor (ii and vii diminished in jazz and film), a
 * secondary dominant only where the genre has them, a borrowed chord only where it borrows */
static int gprog_ok(const gprog_t *p)
{
    uint32_t k;
    for (k = 0; k < p->n; k++) {
        gch_t c = {0, 0, (uint8_t)p->deg[k], 0, 0};
        uint32_t f = QF(p->q[k]);
        if (!gcolour(&c, f) || !gtriad_ok(&c)) return 0;
        if (c.alt && (f == F_DOM ? !(gs.hx & HX_SEC) : f == F_MAJ ? !(gs.hx & HX_RAISE) : !(gs.hx & HX_BOR))) return 0;
        if (HXF(gs.hx) == HF_JAZZ && c.deg == 4u && !c.alt && !gminor() && giv(&c, 6) != 10) return 0;   /* (jazz's V a
                                                         * dominant: lydian's maj7 V is no ii-V) */
    }
    return 1;
}
/* chord j's second half another chord (the plan has room, the chord a bar at least, the new one a chord of the mode) */
static int gsplit(uint32_t j, uint32_t deg, uint32_t q, uint32_t f)
{
    gch_t c;
    uint32_t k, h;
    if (gs.nch >= GCH_N || j >= gs.nch || gs.ch[j].len < 16u) return 0;
    c.deg = (uint8_t)(deg % 7u);
    if (!gcolour(&c, f) || !gtriad_ok(&c)) return 0;
    c.q = (uint8_t)gq_fix(&c, q);
    h = gs.ch[j].len / 2u;
    c.at = (uint8_t)(gs.ch[j].at + h); c.len = (uint8_t)h;
    gs.ch[j].len = (uint8_t)h;
    for (k = gs.nch; k > j + 1u; k--) gs.ch[k] = gs.ch[k - 1u];
    gs.ch[j + 1u] = c;
    gs.nch++;
    return 1;
}
/* the plan: the progression's chords over the loop from its chord rot, each gs.cst steps; then the cadences and the
 * colours the genre takes (their own stream: the same plan every time for the same settings): the minor key's
 * dominant raised before its tonic (the leading tone), jazz's turnaround (ii V in the last chord), a secondary dominant
 * on the second half of the chord before its target, a major key's IV going minor before I */
static void gplan(uint32_t rot)
{
    uint32_t j, n = gs.pn ? gs.pn : 1u, st = gs.cst ? gs.cst : 16u, at, fam = HXF(gs.hx), keep = gq;
    gseed(GS_PLAN);
    while (st > 8u && st * n > gs.len) st /= 2u;        /* (a section's two chords where A had one: half each) */
    gs.nch = 0;
    for (at = 0; at < gs.len && gs.nch < GCH_N; at += st) {
        gch_t *c = &gs.ch[gs.nch];
        uint32_t k = (gs.nch + rot) % n;
        c->at = (uint8_t)at; c->len = (uint8_t)(at + st <= gs.len ? st : gs.len - at);
        c->deg = gs.pdeg[k];
        if (!gcolour(c, QF(gs.pq[k]))) c->alt = 0;
        c->q = (uint8_t)gq_fix(c, QQ(gs.pq[k]));
        gs.nch++;
    }
    if ((gs.hx & HX_RAISE || fam == HF_JAZZ) && gminor() && gs.sc[6] == 10u && gs.nch > 1u)
        for (j = 0; j < gs.nch; j++) {
            gch_t *c = &gs.ch[j];
            if (c->deg != 4u || c->alt || gs.ch[(j + 1u) % gs.nch].deg != 0u) continue;
            if (gcolour(c, F_MAJ)) c->q = (uint8_t)gq_fix(c, c->q == Q_TRI || c->q == Q_POWER ? c->q : Q_7);
            else c->alt = 0;
        }
    if (fam == HF_JAZZ && gs.complex > 30u && gs.nch && gs.ch[gs.nch - 1u].deg == 0u && gs.ch[gs.nch - 1u].len >= 16u) {
        gch_t *c = &gs.ch[gs.nch - 1u], keep = *c;
        c->deg = 1; c->alt = 0;
        if (gtriad_ok(c)) {
            c->q = (uint8_t)gq_fix(c, Q_7);
            if (!gsplit(gs.nch - 1u, 4, Q_7, gminor() ? F_MAJ : 0u)) *c = keep;
        } else *c = keep;
    }
    if ((gs.hx & HX_SEC) && gs.complex > 45u && gch(fam == HF_JAZZ ? 70u : 45u))
        for (j = 0; j + 1u < gs.nch; j++) {
            const gch_t *x = &gs.ch[j + 1u];
            uint32_t d = x->deg;
            if (x->alt || !(d == 1u || d == 2u || d == 3u || d == 5u) || gs.ch[j].deg == (d + 4u) % 7u) continue;
            if (gsplit(j, d + 4u, Q_7, F_DOM)) break;
        }
    if ((gs.hx & HX_BOR) && !gminor() && gs.complex > 45u && gch(40))
        for (j = 0; j + 1u < gs.nch; j++)
            if (gs.ch[j].deg == 3u && !gs.ch[j].alt && gs.ch[j + 1u].deg == 0u && gsplit(j, 3, Q_TRI, F_PAR)) break;
    gq = keep;                                          /* (the caller's stream as it was) */
}

/* ---------------------------------------------------------------------------------------------- the voicing --- */
enum { VO_CLOSE, VO_OPEN, VO_PLANE };
/* (0.8.3) a chord voiced inside [lo, hi]: of its inversions and spreads, the one nearest the last voicing (the least
 * motion, each voice to its nearest old one, the common tones held, the top voice steady); no second below G3, no close
 * third at the very bottom; CLOSE keeps the hand inside a tenth, OPEN (washes, film) spreads it (drop 2: the second
 * voice from the top an octave down; a triad's middle an octave up); PLANE the same shape moved, the root at the bottom
 * (techno, impressionism). Sorted low to high, no unisons */
static uint32_t gvoice(const gch_t *c, uint32_t q, int32_t lo, int32_t hi, int32_t *prev, uint32_t *np, uint32_t mode, int32_t *out)
{
    int32_t pc[4], best[4], cd[4], bcost = 0x7FFFFFFF;
    uint32_t n = 0, j, r, v, k;
    for (j = 0; j < 4u && QSTACK[q][j] != 127; j++) pc[n++] = gtone(c, QSTACK[q][j], 0);
    if (!n) return 0;
    for (r = 0; r < (mode == VO_PLANE ? 1u : n); r++) {
        int32_t base[4], o;
        for (j = 0; j < n; j++) base[j] = j + r < n ? pc[j + r] : pc[j + r - n] + 12;
        for (o = -1; o <= 10; o++) {
            int32_t sh = lo + (((base[0] - lo) % 12) + 12) % 12 + 12 * o - base[0];
            if (base[0] + sh < lo || base[0] + sh >= lo + 24) continue;
            for (v = 0; v < (mode == VO_PLANE ? 1u : 2u); v++) {
                int32_t cost = 0, span;
                for (j = 0; j < n; j++) cd[j] = base[j] + sh;
                if (v) {                                /* (the spread) */
                    if (n == 4u) cd[2] -= 12; else if (n == 3u) cd[1] += 12; else continue;
                    for (j = 1; j < n; j++) for (k = j; k > 0 && cd[k] < cd[k - 1]; k--) { int32_t x = cd[k]; cd[k] = cd[k - 1]; cd[k - 1] = x; }
                }
                if (cd[0] < lo || cd[n - 1u] > hi) continue;
                for (j = 1; j < n; j++) {
                    if (cd[j] <= cd[j - 1]) break;
                    if (cd[j - 1] < 55 && cd[j] - cd[j - 1] <= 2) break;   /* (no seconds below G3) */
                }
                if (j < n) continue;
                if (cd[0] < 52 && n > 1u && cd[1] - cd[0] < 5) cost += 6;   /* (a close third at the bottom: mud) */
                span = cd[n - 1u] - cd[0];
                if (*np && mode == VO_PLANE) cost += cd[0] > prev[0] ? cd[0] - prev[0] : prev[0] - cd[0];
                else if (*np) {                         /* (voice by voice, low to high; each to its nearest
                                                         * old one when their numbers differ; a common tone held) */
                    for (j = 0; j < n; j++) {
                        int32_t d = 127, e, com = 0;
                        for (k = 0; k < *np; k++) { e = cd[j] - prev[k]; if (e < 0) e = -e; if (e < d) d = e; com |= !e; }
                        if (n == *np) { d = cd[j] - prev[j]; if (d < 0) d = -d; }
                        cost += 2 * d - (com ? 3 : 0);
                    }
                } else {                                /* (the first: in the middle of the range) */
                    int32_t m = (cd[0] + cd[n - 1u]) / 2 - (lo + hi) / 2 + 2;
                    cost += m < 0 ? -m : m;
                }
                if (mode == VO_OPEN) cost += span < 12 ? 6 : 0;
                else if (span > 16) cost += 10 + (span - 16) * 2;
                if (cost < bcost) { bcost = cost; for (j = 0; j < n; j++) best[j] = cd[j]; }
            }
        }
    }
    if (bcost == 0x7FFFFFFF)                            /* (nothing fits: the root position from lo) */
        for (j = 0; j < n; j++) best[j] = pc[j] + lo + (((pc[0] - lo) % 12) + 12) % 12 - pc[0];
    for (j = 0; j < n; j++) { out[j] = best[j]; prev[j] = best[j]; }
    *np = n;
    return n;
}
/* the voicing family of the genre's chords */
static uint32_t gvo_mode(void)
{
    uint32_t f = HXF(gs.hx);
    return gs.vm == VM_PLANE ? VO_PLANE : f == HF_COLOR || f == HF_CINE ? VO_OPEN : VO_CLOSE;
}

/* -------------------------------------------------------------------------------------------------- drums --- */
/* lane k's level at step p (gen_drum_maps.h GM_LANE: a nibble a step; the text's '.' 0, '1'..'9' 26 a step, 'X' 255) */
static uint32_t gm_level(uint32_t k, uint32_t p)
{
    uint32_t v = (uint32_t)(GM_LANE[k][(p >> 1) & 7u] >> ((p & 1u) ? 0 : 4)) & 15u;
    return v == 10u ? 255u : v * 26u;
}
static void gdrum_clear_region(track_t *t, uint32_t from, uint32_t to, uint32_t lanes)
{
    uint32_t i;
    for (i = from; i < to; i++) {
        step_t *s = &t->step[i];
        s->hit &= (uint8_t)~lanes; s->acc &= s->hit;
        if (!s->hit) gclear(s);
    }
}
/* (0.9.18) a roll on lane l of step s alone (its mode x2 / x4: n 2, 3..4), not the step's RATCH: the step's other
 * sounds (a kick under the hat) and a hit recorded live into it never roll with it */
static void glane_rat(step_t *s, uint32_t l, uint32_t n)
{
    step_set_lmode(s, l, n > 2u ? LM_X4 : LM_X2);
}
static void gdrum_hit(track_t *t, uint32_t i, uint32_t lane, int32_t vel, int acc)
{
    step_t *s = &t->step[i];
    if (s->time != ST_NOTE || !s->hit) { gclear(s); s->time = ST_NOTE; s->vel = (uint8_t)clamp(vel, 30, 127); }
    else if (!acc) s->vel = (uint8_t)clamp((s->vel + vel) / 2, 30, 127);
    s->hit |= (uint8_t)(1u << lane);
    if (acc) s->acc |= (uint8_t)(1u << lane); else s->acc &= (uint8_t)~(1u << lane);
}
/* (0.8.6) EXPLORE's drums: a sound of a step its own way (the DRUM-X octaves: x2 x4 x8, SHORT GHOST FLAM; x8 a hat's
 * or the rim's only), hits off the grid (rushed, laid back), a hit's chance (never the kick's) */
static void gx_drums(track_t *t, uint32_t len)
{
    uint32_t i, l;
    for (i = 0; i < len; i++) {
        step_t *s = &t->step[i];
        if (s->time != ST_NOTE || !s->hit) continue;
        if (gch(14u + gs.complex / 8u)) {
            uint32_t hl[8], nh = 0, m;
            for (l = 0; l < 8u; l++) if ((s->hit >> l) & 1u) hl[nh++] = l;
            l = hl[gr() % nh];
            m = gpickw(GX_LM_W, LM_N);
            if (m == LM_X8 && l != 3u && l != 6u) m = LM_X4;
            step_set_lmode(s, l, m);
        }
        if (!s->acc && gch(12)) ST_MICRO_SET(s, grr(-4, 4) * 4);   /* (sixteenths: 0.9.1's MICRO is in 64ths) */
        if (!(s->hit & 1u) && gch(8)) step_set_chance(s, (uint32_t)grr(40, 85));
    }
}
static void gen_drums_idm(track_t *t, uint32_t len)
{
    uint32_t i, kb = (uint32_t)grr(3, 6), ks = (uint32_t)grr(2, 4), kh = (uint32_t)grr(7, 13), rot = (uint32_t)grr(0, 7);
    for (i = 0; i < len; i++) {
        step_t *s = &t->step[i];
        uint32_t h = 0, b = i / 16u;
        if (euclid(i, kb, 16, rot + (b == 3u ? 2u : 0u))) h |= 1u;
        if (euclid(i, ks, 16, rot + 4u) && !(h & 1u)) h |= 1u << (gch(50) ? 1 : 6);
        if (euclid(i, kh + (b & 1u) * 2u - (b == 2u ? 3u : 0u), 16, b) && gch(40 + gs.amt[GP_DRUMS] / 2u)) h |= 1u << (gch(20) ? 4 : 3);
        if (gch(gs.complex / 10u)) h |= 1u << (5u + gr() % 3u);
        if (!h) continue;
        s->time = ST_NOTE; s->hit = (uint8_t)h; s->acc = (uint8_t)(i % 4u == 0u ? h & 1u : 0u);
        s->vel = (uint8_t)(h & 1u ? 104 : grr(70, 100));
        if ((h & 8u) && gch(25)) glane_rat(s, 3u, (uint32_t)grr(2, 4));
        if (!(h & 1u) && gch(30)) step_set_chance(s, (uint32_t)grr(45, 85));
    }
}
static void gen_drums(track_t *t, uint32_t len)
{
    const ggenre_t *g = gs.g;
    uint8_t map[8];                                     /* (0.9.7) lane numbers (gen_drum_maps.h; 0: none) */
    static const int8_t OFF[8] = {40, 20, 20, 0, 0, -20, -20, -20};
    uint32_t a = gs.amt[GP_DRUMS], dens = (uint32_t)clamp(40 + (int32_t)a * 2 + ((int32_t)gs.energy - 50) / 2, 20, 255), i, l, bars = len / 16u ? len / 16u : 1u;
    /* (0.6.93) ENERGY: the density and the velocities with it, the hats sixteenths above 66, the percussion thinned under 34 */
    uint32_t chaos = gs.complex * 40u / 100u, ng = (g->ghosts * a + 50u) / 100u + (gs.complex > 60u);
    const uint8_t *cell = HAT_CELL[g->hats];
    gseed(GS_DRUMS);
    gs.kick = gs.snare = 0;
    for (l = 0; l < 8u; l++) map[l] = DRUM_MAP_I[g->drum][l];
    if (g->drum != DM_IDM && g->drum != DM_NONE && g->drum != DM_SPARSE) {   /* (0.6.97) a throw's own top end: the hats,
                                                         * the open hats and the percussion from the pools now and then;
                                                         * the snare and the clap swapped now and then */
        if (gch(50)) map[3] = HAT_POOL_I[gr() % 8u];
        if (gch(40)) map[4] = OH_POOL_I[gr() % 6u];
        if (gch(35)) map[6] = RS_POOL_I[gr() % 6u];
        if (gch(18)) map[7] = CB_POOL_I[gr() % 4u];
        if (gch(30)) map[5] = TM_POOL_I[gr() % 4u];
        if (map[1] && map[2] && gch(30)) { uint8_t x = map[1]; map[1] = map[2]; map[2] = x; }
    }
    if (g->drum == DM_IDM) {
        gen_drums_idm(t, len);
        for (i = 0; i < len; i++) { gs.kick |= (uint64_t)(t->step[i].hit & 1u) << i; gs.snare |= (uint64_t)((t->step[i].hit >> 1) & 1u) << i; }
        if (gx.on) gx_drums(t, len);
        return;
    }
    for (i = 0; i < len; i++) {
        step_t *s = &t->step[i];
        uint32_t b = i / 16u, p = i % 16u, h = 0, acc = 0, hat_only = 1, lv_sum = 0, lv_n = 0, ghost_lv = 0;
        for (l = 0; l < 8u; l++) {
            uint32_t lv = map[l] ? gm_level(map[l], p) : 0u, lim;
            int32_t d;
            if (!lv) continue;
            if (lv < 230u && b) {                       /* (Grids: a perturbation a bar, the first bar pure) */
                uint32_t k = chaos * (b % 4u == 3u ? 2u : b % 2u ? 1u : 0u) / 2u;
                if (k) lv += gr() % (k + 1u);
            }
            d = (int32_t)dens + OFF[l];
            lim = (uint32_t)clamp(d, 0, 255);
            if (lv <= 255u - lim) continue;
            if (l >= 3u && lv < 230u && a < 100u && gr() % 100u >= 55u + a * 45u / 100u) continue;   /* (0.6.95) the hats
                                                         * and the percussion thinned by the amount, a hit at a time */
            h |= 1u << l;
            if (lv >= 230u) acc |= 1u << l;
            else { lv_sum += lv; lv_n++; }
            if (l < 3u || l > 4u) hat_only = 0;
            if (lv < 90u && l < 3u) ghost_lv = lv;
        }
        if (!h) continue;
        s->time = ST_NOTE; s->hit = (uint8_t)h; s->acc = (uint8_t)(acc & h);
        {
            int32_t vel = lv_n ? 56 + (int32_t)(lv_sum / lv_n) * 58 / 255 : 100;
            if (ghost_lv) vel = 36 + (int32_t)ghost_lv * 20 / 90;
            if (hat_only) vel = vel * cell[p % 4u] / 100;
            if (!(acc & h)) vel = vel * (88 + (int32_t)gs.energy / 4) / 100;
            if (gs.human) vel += grr(-(int32_t)gs.human / 12, (int32_t)gs.human / 12);
            s->vel = (uint8_t)clamp(vel, 30, 127);
        }
    }
    if (gs.energy > 66u && map[3]) {                 /* the hats driven: the empty sixteenth after a closed hat, softly */
        uint8_t ch[NSTEP];
        for (i = 0; i < len; i++) ch[i] = (uint8_t)(t->step[i].time == ST_NOTE && (t->step[i].hit & 8u));
        for (i = 1; i < len; i++) {
            step_t *s = &t->step[i];
            if ((s->time == ST_NOTE && (s->hit & 0x18u)) || !ch[i - 1u] || !gch(55 + gs.energy / 4u)) continue;
            gdrum_hit(t, i, 3, 44 + (int32_t)gs.energy / 5, 0);
        }
    } else if (gs.energy < 34u) {                       /* the room: half the open hats and the percussion gone */
        for (i = 0; i < len; i++) {
            step_t *s = &t->step[i];
            if (s->time != ST_NOTE || !(s->hit & 0xD0u) || !gch(50)) continue;
            s->hit &= (uint8_t)~0xD0u; s->acc &= s->hit;
            if (!s->hit) gclear(s);
        }
    }
    for (i = 0; i < len && ng; i++) {                   /* the snare's ghosts: the best positions first, a bar at a time */
        uint32_t b = i / 16u, k, placed = 0;
        if (i % 16u) continue;
        for (k = 0; k < 12u && placed < ng; k++) {
            uint32_t p = GHOST_RANK[k], j = b * 16u + p;
            step_t *s = &t->step[j];
            if (j >= len || (s->time == ST_NOTE && (s->hit & 6u)) || !gch(55 + gs.complex / 3u)) continue;
            if (p && t->step[j - 1u].time == ST_NOTE && (t->step[j - 1u].hit & 2u) && (t->step[j - 1u].acc & 2u)) continue;
            gdrum_hit(t, j, 1, grr(34, 50), 0);
            placed++;
        }
    }
    if (gs.complex > 20u) {                             /* a variation on the second and fourth bars: a kick added on a
                                                         * weak step, a snare dragged, a perc moved */
        for (i = 16; i < len; i++) {
            uint32_t b = i / 16u, p = i % 16u;
            step_t *s = &t->step[i];
            if (!(b & 1u) || p % 2u == 0u) continue;
            if (s->time != ST_NOTE && gch(8 + gs.complex / 8u) && (p == 7u || p == 10u || p == 11u || p == 14u) && g->kick_mode != KM_ANSWER) {
                gdrum_hit(t, i, 0, 80, 0);
            } else if (s->time == ST_NOTE && (s->hit & 0xC0u) && gch(gs.complex / 4u)) {
                s->hit ^= (uint8_t)(s->hit & 0xC0u) ^ (uint8_t)(((s->hit & 0x40u) << 1) | ((s->hit & 0x80u) >> 1));
                s->acc &= s->hit;
            }
        }
    }
    {   /* the fill: the phrase's last bar (and half of one at the fourth bar of eight); the genre's kind */
        uint32_t fl = gs.drum_fill ? gs.drum_fill : g->fills, last = bars - 1u, base = last * 16u, k;
        int32_t e = (int32_t)gs.energy;
        if (bars >= 2u && fl != FL_NONE && (bars >= 4u || gs.drum_fill || gch(50))) {
            uint32_t lane = map[1] ? 1u : map[2] ? 2u : 6u;
            switch (fl) {
            case FL_ROLL:
                gdrum_clear_region(t, base + 12u, base + 16u, 0x10u);
                for (k = 0; k < 4u; k++) {
                    gdrum_hit(t, base + 12u + k, lane, 70 + (int32_t)k * 15, k == 3u);
                    if (e > 55 && k >= 2u) glane_rat(&t->step[base + 12u + k], lane, 2);
                }
                if (bars >= 8u) { gdrum_clear_region(t, base + 8u, base + 12u, 0x10u); for (k = 0; k < 4u; k++) gdrum_hit(t, base + 8u + k, lane, 50 + (int32_t)k * 5, 0); }
                break;
            case FL_TOM: {
                static const uint8_t TM[8] = {1, 0, 1, 1, 0, 1, 0, 1};
                gdrum_clear_region(t, base + 8u, base + 16u, 0x18u);
                for (k = 0; k < 8u; k++) if (TM[k]) gdrum_hit(t, base + 8u + k, 5, 88 + (int32_t)k * 3, k == 0u || k == 5u);
                if (bars >= 4u && len >= 16u) gdrum_hit(t, 0, 4, 110, 1);   /* (a crash on the one) */
                break;
            }
            case FL_DROP:
                gdrum_clear_region(t, base + 8u, base + 16u, 0x01u);
                gdrum_hit(t, base + 15u, 4, 100, 1);
                break;
            case FL_HAT:
                for (k = 12; k < 16u; k++) { gdrum_hit(t, base + k, 3, 70 + (int32_t)(k - 12u) * 12, 0); glane_rat(&t->step[base + k], 3u, k < 14u ? 2u : 3u); }
                break;
            case FL_LINEAR: {
                static const uint8_t BD[8] = {1, 0, 1, 0, 1, 0, 0, 1}, SD[8] = {0, 1, 0, 1, 0, 1, 1, 0};
                gdrum_clear_region(t, base + 8u, base + 16u, 0x1Fu);
                for (k = 0; k < 8u; k++) {
                    if (BD[k]) gdrum_hit(t, base + 8u + k, 0, 100, k == 0u);
                    if (SD[k]) gdrum_hit(t, base + 8u + k, 1, 84 + (int32_t)k * 4, k == 6u);
                }
                break;
            }
            case FL_KICK16:
                for (k = 12; k < 16u; k++) gdrum_hit(t, base + k, 0, 96, 1);
                break;
            case FL_DUB:
                gdrum_clear_region(t, base + 12u, base + 16u, 0x46u);
                gdrum_hit(t, base + 12u, 6, 110, 1);
                break;
            default:                                    /* FL_BREAK: the last beat silent, then the one */
                gdrum_clear_region(t, base + 12u, base + 16u, 0xFFu);
                if (len >= 16u) gdrum_hit(t, 0, 4, 110, 1);
                break;
            }
        }
        if (bars >= 8u && fl != FL_NONE && g->kick_mode != KM_DOUBLE && gch(30 + gs.complex / 3u))
            gdrum_clear_region(t, (bars - 2u) * 16u + 12u, (bars - 2u) * 16u + 16u, 0x01u);   /* (a breath before the fill) */
    }
    if (g->drum == DM_TRAP || g->drum == DM_DUBSTEP || g->drum == DM_UKG) {   /* (0.6.93) the hats' rolls: a step before the
                                                         * snare and the bar's end, sixteenth or triplet rolls */
        for (i = 0; i < len; i++) {
            step_t *s = &t->step[i];
            uint32_t p = i % 16u;
            if (s->time != ST_NOTE || !(s->hit & 8u) || (s->hit & 3u)) continue;
            if ((p == 6u || p == 14u || p == 10u) && gch(35 + gs.complex / 2u)) glane_rat(s, 3u, (uint32_t)grr(2, p == 14u ? 4 : 3));
        }
    }
    for (i = 0; i < len; i++) {                         /* the snare laid back where the genre plays late; the kick's steps */
        step_t *s = &t->step[i];
        if (s->time != ST_NOTE) continue;
        gs.kick |= (uint64_t)(s->hit & 1u) << i;
        gs.snare |= (uint64_t)((s->hit >> 1) & 1u) << i;
        if (g->sd_micro && gs.human && (s->hit & 2u) && !(s->hit & 1u)) ST_MICRO_SET(s, g->sd_micro * 4);   /* (16ths) */
        if (s->acc & ~s->hit) s->acc &= s->hit;
    }
    if (gx.on) gx_drums(t, len);
}

/* --------------------------------------------------------------------------------------------------- bass --- */
static int32_t gbass_root(int32_t lo, const gch_t *c)   /* a chord's root as the bass plays it: in [lo, lo + 12) */
{
    int32_t n = gtone(c, 0, lo);
    while (n >= lo + 12) n -= 12;
    while (n < lo) n += 12;
    return n;
}
/* (0.8.3) an archetype's degree over the chord as the plan has it, from its root as played: '3' its third (a
 * suspension's second or fourth), '5' its fifth, '7' its seventh, '2' '6' '9' the mode's steps over the root (a coloured
 * chord: its root, fifth, octave: never a step against its own altered tones) */
static int32_t gbass_tone(const gch_t *c, int32_t root, char ch)
{
    int32_t s;
    switch (ch) {
    case '3': s = c->q == Q_SUS2 ? 1 : c->q == Q_SUS4 ? 3 : 2; break;
    case '5': s = 4; break;
    case '7': s = 6; break;
    case '2': s = c->alt ? 0 : 1; break;
    case '6': s = c->alt ? 4 : 5; break;
    case '9': s = c->alt ? 7 : 8; break;
    default: s = 0; break;
    }
    return root + giv(c, s);
}
static void gen_acid(track_t *t, uint32_t len, int32_t lo)
{
    static const uint8_t W[6] = {55, 15, 10, 10, 5, 5};   /* R 8 b7 5 b3 2 */
    static const char D[6] = {'R', '8', '7', '5', '3', '2'};
    uint8_t pt[16], on[16];
    uint32_t i, b, bars = len / 16u ? len / 16u : 1u;
    (void)t;
    for (i = 0; i < 16u; i++) {
        on[i] = (uint8_t)(i % 4u == 0u || gch(45 + gs.complex / 3u));
        pt[i] = (uint8_t)gpickw(W, 6);
    }
    for (b = 0; b < bars; b++) {
        uint32_t na = (uint32_t)grr(3, 5), ns = (uint32_t)grr(2, 3), k, tries;
        uint8_t acc[16] = {0}, sl[16] = {0};
        for (tries = 0, k = 0; k < na && tries < 32u; tries++) { uint32_t p = gr() % 16u; if (on[p] && !acc[p]) { acc[p] = 1; k++; } }
        for (tries = 0, k = 0; k < ns && tries < 32u; tries++) {
            uint32_t p = gr() % 15u;
            if (on[p] && on[p + 1u] && !sl[p]) { sl[p] = 1; k++; if (pt[p + 1u] == 0u && gch(50)) pt[p + 1u] = 1; }
        }
        for (i = 0; i < 16u; i++) {
            uint32_t s = b * 16u + i;
            const gch_t *c;
            int32_t n[1], r;
            if (s >= len || !on[i]) continue;
            c = gchord(s);
            r = gbass_root(lo, c);
            n[0] = gchord_starts(s) ? r : pt[i] == 1u ? r + 12 : gbass_tone(c, r, D[pt[i]]);   /* (the root at a change) */
            gnote(&t->step[s], n, 1, acc[i] ? 112 : 90, (acc[i] ? SF_ACCENT : 0u) | (sl[i] ? SF_SLIDE : 0u));
        }
    }
}
static int32_t gbass_near(int32_t lo, const gch_t *c, int32_t ref)   /* (a walk's root: the octave nearest the line) */
{
    int32_t n = gbass_root(lo, c);
    if (ref < 0) return n;
    if (n + 12 - ref < ref - n && n + 12 <= lo + 24) n += 12;
    if (ref - (n - 12) < n - ref && n - 12 >= lo - 3) n -= 12;
    return n;
}
/* (0.9.7) an archetype's note and flag at step i (gen_bass.h: the text's characters) */
#define BA_NOTE(c) (".=R23456789ALNSx"[(c) & 15u])
#define BA_FLAG(c) (".asg"[((c) >> 4) & 3u])
/* (0.9.17) the archetype as this throw plays it: one to three of its steps moved (a ghost or a soft root after a note
 * on an off sixteenth, a weak root up the octave or to the fifth, a weak note left out, a note a sixteenth late or
 * early), the same in every bar (a riff, its own): until 0.9.16 a genre's throws played one of its three archetypes as
 * written, the same bass in a third of them (MICRO HOUSE: as one of the 8 before in 60 %) */
static void gba_vary(uint8_t *bc, uint32_t alen)
{
    uint32_t n = (uint32_t)grr(1, 2 + (int32_t)gs.complex / 40), j, tries;
    for (j = 0, tries = 0; j < n && tries < 24u; tries++) {
        uint32_t p = 1u + gr() % (alen - 1u), q = (p + 1u) % alen, c = bc[p] & 15u, pc = bc[p - 1u] & 15u, op = gr() % 4u;
        if (!(p % 4u) || (bc[q] & 15u) == 1u) continue;  /* (the beats kept; a note held on kept whole) */
        if (op == 0u && !c && (p & 1u) && pc > 1u) bc[p] = gch(50) ? 15u : 2u | 3u << 4;
        else if (op == 1u && c == 2u) bc[p] = (uint8_t)((bc[p] & 0xF0u) | (gch(60) ? 9u : 6u));
        else if (op == 2u && (c == 2u || c == 6u || c == 9u) && pc != 0u) bc[p] = 0;
        else if (op == 3u && q % 4u && (!c) != !(bc[q] & 15u) && c != 1u) { uint8_t x = bc[p]; bc[p] = bc[q]; bc[q] = x; }
        else continue;
        j++;
    }
}
static void gen_bass(track_t *t, uint32_t len)
{
    const ggenre_t *g = gs.g;
    const barch_t *a = &BASS_ARCH[gs.ba];
    int32_t lo = g->bass_lo, prev = -1, root0 = -1;
    uint32_t i, alen = 16u * a->bars, held = 0, walk = gs.ba == BA_WALK || gs.ba == BA_WALK2 || gs.ba == BA_TWOFEEL;
    uint8_t bc[64];
    gseed(GS_BASS);
    if (gs.ba == BA_ACID) { gen_acid(t, len, lo); return; }
    memcpy(bc, &BA_STEP[a->at], alen);
    if (!walk) gba_vary(bc, alen);
    for (i = 0; i < len; i++) {
        step_t *s = &t->step[i];
        char c = BA_NOTE(bc[i % alen]), f = BA_FLAG(bc[i % alen]);
        const gch_t *cc = gchord(i), *nc = gchord_next(i);
        int32_t root, n[1], vel = 96, same = nc == cc || (nc->deg == cc->deg && nc->alt == cc->alt);
        uint32_t fl = 0, chg = gchord_starts(i);
        if (chg && i % 16u && (c == '.' || c == '=') && !(GKICK(i) && g->kick_mode == KM_ANSWER)) c = 'R';   /* (a chord
                                                         * that comes in mid-bar: its root there) */
        if (c == '.') { held = 0; continue; }
        if (c == '=') { if (held) dstep_tie(s); continue; }
        if (GKICK(i) && i % 16u && (g->kick_mode == KM_ANSWER || g->kick_mode == KM_INTERLOCK) && c != 'x' && c != 'r' && !chg) {
            held = 0; continue;                         /* (the kick's step is the kick's) */
        }
        root = walk ? gbass_near(lo, cc, chg ? prev : root0) : gbass_root(lo, cc);   /* (a walk: each chord's root in the
                                                         * octave nearest the line, kept through its chord) */
        if (chg || root0 < 0) root0 = root;
        switch (c) {
        case '7':                                       /* (a seventh alone over a major one with a major seventh: its
                                                         * sixth, the reggae's and the funk's note; in a run 6 7 8 it passes) */
            n[0] = gbass_tone(cc, root, giv(cc, 6) == 11 && giv(cc, 2) == 4 && BA_NOTE(bc[(i + alen - 1u) % alen]) != '6' &&
                                        BA_NOTE(bc[(i + 1u) % alen]) != '8' ? '6' : '7');
            break;
        case '2': case '3': case '5': case '6': case '9': n[0] = gbass_tone(cc, root, c); break;
        case 'L': n[0] = gbass_tone(cc, root, '5') - 12; break;
        case '8': n[0] = root + 12; break;
        case 'A': case 'B': {                           /* (into the next root as it will sound: the loop's first note
                                                         * when the loop closes) */
            int32_t r2 = nc == &gs.ch[0] && t->step[0].time == ST_NOTE ? t->step[0].note[0] : walk ? gbass_near(lo, nc, prev) : gbass_root(lo, nc);
            n[0] = same ? gbass_tone(cc, root, '5') : r2 + (c == 'A' ? -1 : 1);
            break;
        }
        case 'N': n[0] = gbass_root(lo, nc); break;
        case 'S': {                                     /* a step of the mode towards the next root */
            int32_t r2 = gbass_root(lo, nc);
            if (same) n[0] = gbass_tone(cc, root, '2');
            else n[0] = r2 + (r2 > root ? gnd(0, nc->deg - 1) - gnd(0, nc->deg) : gnd(0, nc->deg + 1) - gnd(0, nc->deg));
            break;
        }
        default: n[0] = root; break;                    /* R r x */
        }
        if (chg && c != 'x') n[0] = c == '8' ? root + 12 : root;   /* (0.8.3: the root at every change) */
        if (n[0] > lo + 26) n[0] -= 12;                /* (two octaves from the root's: the octave jumps live) */
        if (n[0] < lo - 7) n[0] += 12;
        prev = n[0];
        if (c == 'r') vel = 70;
        if (c == 'x') vel = 42;
        if (f == 'g') vel = 50;
        if (f == 'a') { vel = 110; fl |= SF_ACCENT; }
        if (f == 's') fl |= SF_SLIDE;
        if (gs.human) vel += grr(-(int32_t)gs.human / 10, (int32_t)gs.human / 10);
        gnote(s, n, 1, vel, fl);
        held = 1;
    }
    if (g->kick_mode == KM_DOUBLE || g->kick_mode == KM_INTERLOCK) {   /* the kick doubled (DOUBLE: on its steps when the
                                                         * bass rests), interlocked (the step after it) */
        for (i = 0; i < len; i++) {
            uint32_t j = g->kick_mode == KM_DOUBLE ? i : i + 1u;
            int32_t n[1];
            if (!GKICK(i) || j >= len || t->step[j].time != ST_REST || dsounding(t, j) >= 0) continue;
            if (!gch(g->kick_mode == KM_DOUBLE ? 30 + gs.amt[GP_BASS] / 4u : 35)) continue;
            n[0] = gbass_root(lo, gchord(j));
            gnote(&t->step[j], n, 1, g->kick_mode == KM_DOUBLE ? 92 : 72, 0);
        }
    }
    if (len >= 64u && gs.complex > 30u && a->bars < 4u) {   /* the fourth bar's pickup: an approach into the next root */
        uint32_t j = 62;
        const gch_t *cc = gchord(63), *nc = gchord(0);
        int32_t n[1];
        if (t->step[j].time == ST_REST && t->step[j + 1u].time == ST_REST && gch(60)) {
            int32_t r = gbass_root(lo, cc);
            n[0] = nc->deg == cc->deg && nc->alt == cc->alt ? gbass_tone(cc, r, '5') : gbass_root(lo, nc) - 1;
            if (n[0] > lo + 26) n[0] -= 12;
            gnote(&t->step[j], n, 1, 84, 0);
        }
    }
    if (gs.amt[GP_BASS] < 60u) {                        /* a thinner bass: the weak notes thinned (never the note an
                                                         * approach resolves into) */
        int32_t last = -1;
        for (i = 0; i < len; i++) {
            step_t *s = &t->step[i];
            int keep;
            if (s->time != ST_NOTE) continue;
            keep = last >= 0 && !gin_mode(last);
            last = s->note[0];
            if (!keep && i % 4u && !(s->flags & SF_ACCENT) && gin_mode(s->note[0]) && gr() % 100u < (60u - gs.amt[GP_BASS])) {
                gclear(s);
                if (i + 1u < len && t->step[i + 1u].time == ST_TIE) gclear(&t->step[i + 1u]);
            }
        }
    }
    for (i = 0; i < len; i++) {                         /* (an approach left with nothing to go to: its chord's root) */
        step_t *s = &t->step[i];
        const step_t *x;
        uint32_t j;
        if (s->time != ST_NOTE || gin_mode(s->note[0])) continue;
        for (j = 1; j < len && t->step[(i + j) % len].time != ST_NOTE; j++) {}
        x = &t->step[(i + j) % len];
        if (j < len && (x->note[0] - s->note[0] == 1 || s->note[0] - x->note[0] == 1)) continue;
        if ((gpcm(gchord(i)) >> (s->note[0] % 12u)) & 1u) continue;
        s->note[0] = (uint8_t)gbass_root(lo, gchord(i));
    }
}

/* -------------------------------------------------------------------------------------------------- chords --- */
/* the chord at step i voiced for a track (its own last voicing): the pads' rootless where jazz has the bass on the root */
static uint32_t gchord_at(uint32_t i, int32_t lo, int32_t hi, int32_t *n, int32_t *prev, uint32_t *np, uint32_t mode, int rootless)
{
    const gch_t *c = gchord(i);
    uint32_t q = c->q;
    if (rootless && (q == Q_7 || q == Q_9) && !c->alt && giv(c, 8) == 14) q = Q_R9;
    return gvoice(c, q, lo, hi, prev, np, mode, n);
}
static void gen_pads(track_t *t, uint32_t len)
{
    const ggenre_t *g = gs.g;
    uint32_t i, a = gs.amt[GP_CHORDS], last = 0xFFFFu, cnt = 0, style = g->pads, mode = gvo_mode(), fam = HXF(gs.hx);
    int32_t n[4];
    int rootless = (fam == HF_JAZZ || (fam == HF_JAZZY && gs.vm == VM_RICH)) && gs.act[RL_BASS];
    uint8_t dub[16] = {0}, comp[16] = {0}, brk[16] = {0};
    uint32_t off, sh;
    gseed(GS_CHORDS);
    gs.nvo = 0;
    {   /* the bar figures kept the loop through */
        static const uint8_t AT[6] = {6, 10, 14, 7, 11, 13}, CP[6] = {2, 6, 7, 11, 14, 15}, BR[6] = {0, 3, 6, 10, 11, 14};
        uint32_t m = (uint32_t)grr(1, 2) + (gs.energy > 66u), j;
        dub[2] = 1;
        for (j = 0; j < m; j++) dub[AT[gr() % 6u]] = 1;
        for (j = 0, m = (uint32_t)grr(3, 5); j < m; j++) comp[CP[(gr() + j * 2u) % 6u]] = 1;
        comp[gch(50) ? 6u : 14u] = 1;                   /* (the anticipation of 3 or of the bar) */
        for (j = 0, m = (uint32_t)grr(3, 4); j < m; j++) brk[BR[gr() % 6u]] = 1;
        {   /* (0.9.17) a held chord's own place: on the change, or an eighth, a beat, a beat and a half late; a sparse
             * one held to its end or for a dotted quarter (the held styles came out the same piece after piece) */
            static const uint8_t OFS[6] = {0, 0, 0, 2, 4, 6};
            off = OFS[gr() % 6u];
            sh = style == CS_SPARSE && gch(40) ? 6u : 0u;
        }
    }
    for (i = 0; i < len; i++) {
        step_t *s = &t->step[i];
        const gch_t *c = gchord(i);
        uint32_t pp = i % gs.len - c->at, p = i % 16u, b = i / 16u, cl = c->len;
        int on = 0, vel = 80, hold = 0;
        switch (style) {
        case CS_HOLD: on = pp == (cl > off ? off : 0u); hold = (int)cl - 1; break;
        case CS_SWELL: on = pp == (cl > off ? off : 0u); hold = (int)cl - 1; vel = 72; break;
        case CS_STAB: on = (p % 4u == 2u && gch(40 + a / 2u)) || (pp == 0u && gch(50)); vel = p % 4u == 2u ? 84 : 92; break;
        case CS_PULSE: on = i % (gs.energy > 66u ? 2u : 4u) == 0u && !(p == 12u && gch(40)); hold = gch(40); vel = p % 4u ? 76 : 86; break;
        case CS_DUB: on = dub[p]; vel = p == 2u ? 92 : 78; break;
        case CS_SKANK: on = p % 4u == 2u; vel = 84; break;
        case CS_COMP: on = (comp[p] && gch(70 + a / 4u)) || (p == 0u && gch(40)) || (!comp[p] && (p == 6u || p == 10u || p == 13u) && gch(a / 4u));
            hold = p == 14u || p == 7u; vel = p == 0u ? 84 : 80; break;   /* (0.6.93: the bar's figure, varied a bar) */
        case CS_POWER: on = p == 0u || (p == 8u && (b & 1u) && gch(60)); hold = p == 8u ? 5 : 7; vel = 96; break;   /* (0.6.93: a push every bar) */
        case CS_DRONE: on = i == 0u; hold = (int)len - 1; vel = 70; break;
        case CS_SPARSE: on = pp == (cl > off ? off : 0u) && (i == off || gch(40 + a / 3u)); hold = sh ? (int)sh : (int)cl - 1; vel = 76; break;
        case CS_BROKEN: on = brk[p]; vel = p ? 80 : 92; break;
        default: on = ((0xDB6Du >> p) & 1u) && gch(70 + a / 4u); vel = p % 4u ? 78 : 96; break;   /* CS_TRANCE */
        }
        if (!on || !gch(style == CS_HOLD || style == CS_SWELL || style == CS_DRONE || style == CS_SPARSE ? 100 : 70 + a * 3u / 10u)) continue;
        if (gci(i) != last || !cnt || style == CS_HOLD || style == CS_SWELL || style == CS_SPARSE) {
            cnt = gchord_at(i, 48, 79, n, gs.vo, &gs.nvo, mode, rootless);
            last = gci(i);
        } else {
            uint32_t j;
            for (j = 0; j < gs.nvo; j++) n[j] = gs.vo[j];
            cnt = gs.nvo;
        }
        if (hold > 0 && pp + (uint32_t)hold >= cl && style != CS_DRONE) hold = (int)(cl - pp) - 1;   /* (held to its chord's end) */
        if (gs.human) vel += grr(-(int32_t)gs.human / 12, (int32_t)gs.human / 12);
        gnote(s, n, cnt, vel, 0);
        if (hold > 0) dhold(t, i, (uint32_t)hold, len);
    }
}

/* ----------------------------------------------------------------------------------------------------- arp --- */
static void gen_arp(track_t *t, uint32_t len)
{
    const garp_t *ap;
    uint32_t i, k = 0, idx = 0, a = gs.amt[GP_ARP], no = 0, last = 0xFFFFu, cnt = 0, rate, hole = 0, gh = 0, tie = 0;
    int32_t c[4], n[1];
    gseed(GS_ARP);
    ap = &ARP[gch(25) ? AP_PEDAL + gr() % 3u : gs.g->arp[gch(70) ? 0 : 1]];   /* (0.9.17) now and then a pedal, broken
                                                         * thirds, a zigzag */
    rate = ap->rate;
    if (rate == 1u && (a < 35u || (gs.energy < 34u && gch(50)))) rate = 2;   /* (0.6.93) a small amount: eighths */
    gs.navo = 0;
    for (no = 0; no < 8u && ap->ord[no] >= 0; no++) {}
    for (i = 0; i < 16u; i++) {                         /* (0.9.17) the holes (space), the ghosts and the held notes a
                                                         * figure of the bar, kept bar after bar (the fourth bar a little
                                                         * its own): until 0.9.16 thrown every step, a bar like no other */
        hole |= (uint32_t)(gr() % 100u >= 20u + a * 3u / 4u && i % 4u) << i;
        gh |= (uint32_t)(gch(18) && i % 4u) << i;
        tie |= (uint32_t)gch(10) << i;
    }
    for (i = 0; i < len; i++) {
        uint32_t j, p = (i / rate) % 16u;
        int32_t o;
        if (i % rate) continue;
        idx = i / rate;
        if (!((ap->mask >> (idx % 16u)) & 1u)) { k++; continue; }
        if (((hole >> p) & 1u) != (i / 16u % 4u == 3u && p % 4u == 2u && gch(40))) { k++; continue; }
        if (gci(i) != last || !cnt) {
            cnt = gchord_at(i, 50, 74, c, gs.avo, &gs.navo, gvo_mode() == VO_PLANE ? VO_PLANE : VO_CLOSE, 0);
            last = gci(i);
            if (gchord_starts(i)) k = 0;                /* (0.8.3: each chord's figure from its start) */
        } else {
            for (j = 0; j < gs.navo; j++) c[j] = gs.avo[j];
            cnt = gs.navo;
        }
        o = ap->ord[k % no];
        j = (uint32_t)(o & 3) % cnt;
        n[0] = c[j] + (o >= 4 ? 12 : 0);
        while (n[0] > 79) n[0] -= 12;
        while (n[0] < 48) n[0] += 12;
        gnote(&t->step[i], n, 1, (gh >> p) & 1u ? 54 : idx % 4u == 0u || k % no == 0u ? 92 : 78, 0);
        if ((tie >> p) & 1u) dhold(t, i, rate, len);
        if (gs.complex > 50u && gch(6)) step_set_ratchet(&t->step[i], 2);
        k++;
    }
    t->p[P_SGATE] = (int16_t)clamp((int32_t)ap->gate + grr(-8, 8), 10, 127);
}

/* ---------------------------------------------------------------------------------------------------- lead --- */
/* (0.8.3) the lead composed: a line of notes as degrees of the mode (from the lead's tonic), written once polished.
 * The rules every style keeps: a chord tone on the strong steps (the beats), the lead's degrees (the genre's
 * pentatonic where it sings one), inside a tenth or so, a leap past a fourth followed by a step back, never more than
 * an octave, the phrase ending on a stable degree where it cadences, the tones a borrowed or secondary chord moves
 * followed (b7 to the leading tone over the minor key's V). MOTIF and HOOK: a one-bar motif (a rhythm cell and a
 * contour of steps), developed over the phrase by a template (a period: antecedent ending open on the 2nd or 5th,
 * consequent closing on the tonic; a sentence: the motif, its sequence, a continuation climbing to the climax two thirds
 * in, the cadence; A A B A; call and response; the techno hook A A A A') and one operator a varied bar */
#define GL_MAX 48u
typedef struct { uint8_t at, len, vel, fl, lock; int8_t d, chrom; } gln_t;
typedef struct { gln_t n[GL_MAX]; uint32_t k; int32_t tn, dlo, dhi; } gline_t;
static int gl_ct(int32_t d, uint32_t at) { return (int)((gchord_pcs(at) >> gpc(d)) & 1u); }
static int gl_pal(int32_t d) { return (int)((gs.pal >> (uint32_t)(((d % 7) + 7) % 7)) & 1u); }
static int32_t gl_pitch(const gline_t *L, int32_t d) { return gnd(L->tn, d); }
/* the chord tone at step at nearest degree d (the lead's degrees first, dir's side first on a tie), inside the window */
static int32_t gl_near(const gline_t *L, int32_t d, uint32_t at, int dir)
{
    int32_t k, best = d, bs = 99;
    for (k = 0; k <= 4; k++) {
        int32_t sgn, e;
        for (sgn = 0; sgn < 2; sgn++) {
            e = d + ((sgn == 0) == (dir >= 0) ? k : -k);
            if (e < L->dlo || e > L->dhi || !gl_ct(e, at)) continue;
            if (k * 2 + !gl_pal(e) < bs) { bs = k * 2 + !gl_pal(e); best = e; }
        }
    }
    return best;
}
static int32_t gl_fold(const gline_t *L, int32_t d)     /* into the window by octaves */
{
    while (d > L->dhi) d -= 7;
    while (d < L->dlo) d += 7;
    return d;
}
static void gl_add(gline_t *L, uint32_t at, int32_t d, uint32_t vel, uint32_t lock)
{
    gln_t *x;
    if (L->k >= GL_MAX) return;
    if (L->k && L->n[L->k - 1u].at >= at) return;       /* (in time order, one a step) */
    x = &L->n[L->k++];
    x->at = (uint8_t)at; x->d = (int8_t)gl_fold(L, d); x->vel = (uint8_t)vel; x->lock = (uint8_t)lock;
    x->fl = 0; x->len = 0; x->chrom = 0;
}
/* the cadence: the last note of a phrase on the tonic (a full close: else the third, the fifth, the nearest chord tone)
 * or open (a half close: the 2nd or the 5th, else the third), the octave nearest the note before */
static int32_t gl_cad(const gline_t *L, int32_t d, uint32_t at, int full)
{
    static const int8_t FULL[3] = {0, 2, 4}, HALF[3] = {1, 4, 2};
    const int8_t *T = full ? FULL : HALF;
    int32_t k, best = 0x7FFF, bo = 99, ref = L->k ? L->n[L->k - 1u].d : d;   /* (reached from the line's last note:
                                                         * the stable degree nearest it, the tonic first on a near tie) */
    for (k = 0; k < 3; k++) {
        int32_t e;
        for (e = T[k] - 7; e <= T[k] + 14; e += 7) {
            int32_t dd = (e - ref < 0 ? ref - e : e - ref) * 3 + (e - d < 0 ? d - e : e - d) + k * 2;
            if (e >= L->dlo && e <= L->dhi && gl_ct(e, at) && dd < best) { best = dd; bo = e; }
        }
    }
    return bo < 99 ? bo : gl_near(L, d, at, 0);
}
/* the line polished and written: the octave nearest the last note past a 9th; a leap past a fourth followed by a step
 * back (a third where the strong step wants a chord tone); a borrowed or secondary chord's moved tones followed; each
 * note held to the next (a breath before it, more of one at low energy); the slides now and then */
static int gl_clash(int32_t d, uint32_t at)              /* a minor ninth or a tritone over the chord's root (the bass's) */
{
    uint32_t iv = (gpc(d) + 12u - gchord_root_pc(at)) % 12u;
    return (iv == 1u || iv == 6u) && !gl_ct(d, at);
}
static void gl_write(track_t *t, gline_t *L, uint32_t len, uint32_t legato)
{
    uint32_t i;
    for (i = 0; i < L->k; i++) {                        /* no minor ninth, no tritone over the root: a step aside, a
                                                         * chord tone where one is a step away */
        gln_t *x = &L->n[i];
        int32_t dn = x->d - 1, up = x->d + 1;
        int okd, oku;
        if (!gl_clash(x->d, x->at)) continue;
        okd = !gl_clash(dn, x->at) && dn >= L->dlo; oku = !gl_clash(up, x->at) && up <= L->dhi;
        if (okd && (!oku || gl_ct(dn, x->at) || !gl_ct(up, x->at))) x->d = (int8_t)dn;
        else if (oku) x->d = (int8_t)up;
    }
    for (i = 1; i + 1u < L->k; i++) {                   /* the weak notes between two others: passing and neighbour
                                                         * tones (the smoothest of the note, a step up, a step down) */
        gln_t *x = &L->n[i];
        int32_t pv = gl_pitch(L, L->n[i - 1u].d), nv = gl_pitch(L, L->n[i + 1u].d), k, best = x->d, bc = 0x7FFF;
        if (x->at % 8u == 0u || x->lock) continue;
        {   /* (only where the line skips: a step in and a step out are left as they are) */
            int32_t p0 = gl_pitch(L, x->d), a1 = p0 - pv, a2 = nv - p0;
            if (a1 < 0) a1 = -a1;
            if (a2 < 0) a2 = -a2;
            if (a1 <= 2 && a2 <= 2 && (a1 || a2)) continue;
        }
        for (k = -1; k <= 1; k++) {
            int32_t d = x->d + k, p = gl_pitch(L, d), c;
            if (d < L->dlo || d > L->dhi || gl_clash(d, x->at)) continue;
            c = (p > pv ? p - pv : pv - p) + (nv > p ? nv - p : p - nv) + (k ? 2 : 0) + (gl_pal(d) ? 0 : 4) +
                (d == L->n[i - 1u].d ? 5 : 0) + (gl_ct(d, x->at) ? 0 : x->at % 4u ? 1 : 3);   /* (beats 2 and 4: a
                                                         * passing note only where it makes the line move by steps) */
            if (c < bc) { bc = c; best = d; }
        }
        x->d = (int8_t)best;
    }
    for (i = 0; i < L->k; i++) {                        /* no avoid note held a beat or more (the 4th over a major
                                                         * third, b6 over the fifth, b9 over the root) */
        gln_t *x = &L->n[i];
        uint32_t nxt = i + 1u < L->k ? L->n[i + 1u].at : len, dur = x->len ? x->len : nxt - x->at;
        const gch_t *c = gchord(x->at);
        int sus3 = (c->q == Q_SUS2 || c->q == Q_SUS4) && gpc(x->d) == (uint32_t)gtone(c, 2, 12) % 12u;   /* (a suspension's
                                                         * third: against its 4th) */
        if (dur >= 4u && !x->lock && !gl_ct(x->d, x->at) && (sus3 || ((gchord_pcs(x->at) >> ((gpc(x->d) + 11u) % 12u)) & 1u)))
            x->d = (int8_t)gl_near(L, x->d, x->at, i ? (L->n[i - 1u].d > x->d ? 1 : -1) : 0);   /* (a semitone over a chord
                                                         * tone, held: down onto it, or the nearest) */
    }
    for (i = 1; i < L->k; i++) {                        /* never past an octave */
        gln_t *x = &L->n[i], *p = &L->n[i - 1u];
        int32_t iv = gl_pitch(L, x->d) - gl_pitch(L, p->d);
        if (iv > 12 && x->d - 7 >= L->dlo - 2) x->d = (int8_t)(x->d - 7);
        else if (iv < -12 && x->d + 7 <= L->dhi + 2) x->d = (int8_t)(x->d + 7);
    }
    for (i = 1; i + 1u < L->k; i++) {                   /* a leap past a fourth: the next note a step back (a third
                                                         * where the strong step wants a chord tone) */
        gln_t *x = &L->n[i], *p = &L->n[i - 1u], *nx = &L->n[i + 1u];
        int32_t iv = gl_pitch(L, x->d) - gl_pitch(L, p->d), sg = iv > 0 ? 1 : -1, a = iv < 0 ? -iv : iv, nv, b;
        if (a <= 5 || nx->lock) continue;
        nv = gl_pitch(L, nx->d) - gl_pitch(L, x->d);
        b = nv < 0 ? -nv : nv;
        if (b && b <= 2 && (nv > 0) != (iv > 0)) continue;   /* (already back by a step) */
        if ((nx->at % 16u || gl_ct(x->d - sg, nx->at)) && !gl_clash(x->d - sg, nx->at)) nx->d = (int8_t)(x->d - sg);   /* (on
                                                         * beat 3 an appoggiatura too; on the bar's one a chord tone) */
        else if (gl_ct(x->d - 2 * sg, nx->at)) nx->d = (int8_t)(x->d - 2 * sg);
    }
    for (i = 0; i < L->k; i++) {                        /* a coloured chord's moved tones: the line follows them */
        gln_t *x = &L->n[i];
        const gch_t *c = gchord(x->at);
        uint32_t s;
        if (!c->alt) continue;
        for (s = 0; s <= 6u; s += 2u) {
            int32_t al = galt(c, (int32_t)s);
            uint32_t ap = (uint32_t)gtone(c, (int32_t)s, 12) % 12u;
            if (al && (gpc(x->d) + 12u + (uint32_t)al) % 12u == ap && ((gpcm(c) >> ap) & 1u)) { x->chrom = (int8_t)al; break; }
        }
    }
    for (i = 0; i < L->k; i++) {
        gln_t *x = &L->n[i];
        uint32_t nxt = i + 1u < L->k ? L->n[i + 1u].at : len + L->n[0].at, gap, h;
        int32_t n[1];
        if (x->at >= len) break;
        n[0] = gl_pitch(L, x->d) + x->chrom;
        gnote(&t->step[x->at], n, 1, x->vel, (x->fl & 1u) ? SF_SLIDE : 0u);
        gap = nxt - x->at;
        h = x->len ? x->len : gap > legato ? gap - legato : 0u;
        if (h + x->at >= len) h = len - x->at - 1u;
        if (h) dhold(t, x->at, h, len);
    }
}
/* the motif's rhythm cells: figures that hold together (a long note and two short, a tresillo, a pickup into three, a
 * dotted push ..) */
static const char *const GL_CELL[16] = {"x..x..x...x.....", "x.x.x...x.x.....", "x...x.x.x.......", "..x...x...x...x.",
                                        "x..x.x..x..x.x..", "x.......x.x.x...", "x...x...x...x...", "x.x...x.x...x...",
                                        "..x.x...x.x.....", "x..x..x..x..x...", "x.....x...x.x...", "x...x.x.......x.",
                                        "x..x...x..x.....", "..x..x..x.x.....", "x.x..x...x..x...", "x..x.x...x.x..x."};   /* (0.9.17:
                                                         * the last four syncopated: off sixteenths, pushes) */
enum { LB_M, LB_V, LB_R, LB_C, LB_REST };               /* a bar of the phrase: the motif, varied, the answer, the
                                                         * continuation, a rest */
static void gen_lead(track_t *t, uint32_t len)
{
    const ggenre_t *g = gs.g;
    uint32_t i, j, a = gs.amt[GP_LEAD], bars = (len + 15u) / 16u, style = g->lead, fam = HXF(gs.hx), legato = gs.energy < 34u ? 4u : 2u;
    gline_t L;
    gseed(GS_LEAD);
    if (style == LD_NONE) return;
    L.k = 0;
    L.tn = 55 + ((gs.root - 55) % 12 + 12) % 12;        /* the lead's tonic (degree 0): G3 .. F#4 */
    {   /* its window, the same in every key: Bb3 .. E5 (a motif, a hook), lower for an ostinato and long notes, higher
         * for Part's line */
        int32_t lo = style == LD_OSTINATO ? 55 : style == LD_SPARSE ? 56 : style == LD_TINTI ? 60 : 58, hi = lo + 17;
        if (style == LD_OSTINATO) hi = 72;
        for (L.dlo = -7; gnd(L.tn, L.dlo) < lo; L.dlo++) {}
        for (L.dhi = L.dlo; gnd(L.tn, L.dhi + 1) <= hi; L.dhi++) {}
    }
    if (style == LD_OSTINATO) {                         /* a cell of two to four notes (repeats welcome) on an euclid, the
                                                         * chord's tones on the beats, its last note lifted on the third bar */
        int8_t cl[4];
        uint32_t k = (uint32_t)grr(3, 4) + (gs.energy > 66u), rot = (uint32_t)grr(0, 3), nc = (uint32_t)grr(2, 4), q = 0, bre = 0, keep = 0xFFFFu;
        int32_t d = gl_near(&L, grr(0, 4), 0, 1);
        for (j = 0; j < nc; j++) {
            static const uint8_t W[5] = {25, 45, 20, 0, 10};   /* (a repeat, a step, a third, -, a fourth) */
            uint32_t m = gpickw(W, 5);
            if (j + 1u == nc && !m && cl[0] == d && (j < 2u || cl[1] == d)) m = 1;   /* (two notes at least) */
            cl[j] = (int8_t)d;
            d += (gch(50) ? 1 : -1) * (int32_t)m;
            if (d < L.dlo + 1 || d > L.dhi - 2) d -= 2 * (d > L.dhi - 2 ? (int32_t)m : -(int32_t)m);   /* (inside, off the
                                                         * window's edges: room for the chords' moves) */
        }
        for (j = 0; j < 16u; j++) if (j % 4u && gr() % 100u >= 50u + a / 2u) keep &= ~(1u << j);   /* (the same holes a bar) */
        for (i = 0; i < len; i++) {
            uint32_t b = i / 16u;
            int32_t dd, sh = (int32_t)gchord(i)->deg - (int32_t)gchord(0)->deg;
            if (i % 16u == 0u) bre = b % 4u == 3u && gch(50);   /* (the fourth bar: a breath, its second half) */
            if (!euclid(i % 16u, k, 16, rot)) continue;
            if (bre && i % 16u >= 8u) continue;
            if (!((keep >> (i % 16u)) & 1u)) continue;
            while (sh > 3) sh -= 7;
            while (sh < -3) sh += 7;
            dd = cl[q % nc] + sh + (b % 4u == 2u && q % nc == nc - 1u ? 1 : 0);
            if (i % 4u == 0u || !gl_pal(dd)) dd = gl_near(&L, dd, i, 0);
            gl_add(&L, i, dd, q % nc == 0u ? 98 : 82, 0);
            if (gch(12) && L.k) L.n[L.k - 1u].fl = 1;
            q++;
        }
        gl_write(t, &L, len, 1);
        return;
    }
    if (style == LD_MECH) {                             /* a machine's figure on an euclid, moved with the chord */
        static const int8_t MT[4][8] = {{0, 1, 2, 1, 0, 2, 4, 2}, {4, 4, 2, 1, 2, 2, 0, -1}, {0, 0, 4, 3, 2, 1, 2, 0}, {7, 6, 4, 4, 2, 4, 5, 4}};
        const int8_t *mt = MT[gr() % 4u];
        uint32_t k = (uint32_t)grr(4, 6) + (gs.energy > 66u), rot = (uint32_t)grr(0, 3), idx, q;
        int32_t oo = 0, o, best = 99;
        for (o = -7; o <= 7; o += 7) {                  /* (the figure's octave: the most of it inside the window) */
            int32_t c = 0;
            for (q = 0; q < 8u; q++) c += mt[q] + o < L.dlo || mt[q] + o > L.dhi;
            if (c < best) { best = c; oo = o; }
        }
        for (i = 0; i < len; i++) {
            int32_t dd, sh;
            if (!euclid(i % 16u, k, 16, rot)) continue;
            for (q = 0, idx = 0; q < i % 16u; q++) idx += euclid(q, k, 16, rot);
            sh = (int32_t)gchord(i - i % 16u)->deg - (int32_t)gchord(0)->deg;
            while (sh > 3) sh -= 7;
            while (sh < -3) sh += 7;
            dd = mt[idx % 8u] + sh + oo;
            if (i % 4u == 0u || !gl_pal(dd)) dd = gl_near(&L, dd, i, 0);
            gl_add(&L, i, dd, 92, 0);
        }
        gl_write(t, &L, len, 1);
        return;
    }
    if (style == LD_SPARSE || style == LD_TINTI) {      /* long notes (SPARSE: a guide-tone line, each the chord tone
                                                         * nearest the last, now and then a short figure stepping into it;
                                                         * TINTI: Part's line, stepwise, a chord tone on each long note) */
        int32_t d = gl_near(&L, style == LD_TINTI ? grr(2, 5) : 2, 0, 1), dir = gch(50) ? -1 : 1;
        uint32_t fig = style == LD_SPARSE && gch(55), off = style == LD_SPARSE && gch(40) ? 4u : 0u, nlong = 0;
        for (i = 0; i < len; i += 8u) {
            uint32_t b = i / 16u, at = i + off, hold, last = i + 8u >= len;
            if (at >= len) break;
            if (style == LD_SPARSE && ((i % 16u && gr() % 100u >= a) || (b % 2u == 1u && gch(40))) && !last) continue;
            if (style == LD_SPARSE && nlong) {
                int32_t e, k2;                          /* (moving on: the next chord tone that way, a step or a
                                                         * third; the same note only now and then) */
                if (d + 2 * dir < L.dlo || d + 2 * dir > L.dhi) dir = -dir;
                for (k2 = 1, e = d; k2 <= 3; k2++) if (gl_ct(d + dir * k2, at)) { e = d + dir * k2; break; }
                if (e == d || (gl_ct(d, at) && gch(15))) e = gl_near(&L, d, at, dir);
                d = e;
                if (gch(35)) dir = -dir;
            } else if (style == LD_SPARSE) {
                d = gl_near(&L, d, at, 1);              /* (the first: a chord tone near the third) */
            } else if (i % 16u == 0u) {                 /* (TINTI: the bar's long note a chord tone a step or a third
                                                         * on; its second a step on, passing) */
                int32_t e = d + dir;
                if (e < L.dlo + 1 || e > L.dhi - 1) { dir = -dir; e = d + dir; }
                d = gl_ct(e, at) ? e : gl_ct(e + dir, at) ? e + dir : gl_near(&L, e, at, dir);
            } else {
                at = i + 2u;                            /* (off the beat: a passing note) */
                d += dir;
                if (d < L.dlo || d > L.dhi) { dir = -dir; d += 2 * dir; }
            }
            if (last) d = gl_cad(&L, d, at, 1);         /* (the loop closes on the tonic) */
            hold = style == LD_TINTI ? 6u : 12u - (gs.energy > 66u ? 4u : 0u) - off;
            if (fig && gch(60) && nlong) {              /* the figure: one or two short notes stepping into the long one */
                uint32_t q, nf = (uint32_t)grr(1, 2);
                int32_t s = gch(50) ? 1 : -1;
                for (q = 0; q < nf && at + q * 2u + 2u < len; q++) gl_add(&L, at + q * 2u, d + s * (int32_t)(nf - q), 76, 0);
                at += nf * 2u;
                hold = hold > nf * 2u ? hold - nf * 2u : 2u;
            }
            if (at >= len) break;
            gl_add(&L, at, d, 84, last);
            if (L.k) L.n[L.k - 1u].len = (uint8_t)hold;
            nlong++;
        }
        gl_write(t, &L, len, 2);
        return;
    }
    {   /* MOTIF, HOOK */
        static const uint8_t TPL[6][4] = {{LB_M, LB_V, LB_M, LB_V},    /* a period: open, closed */
                                          {LB_M, LB_M, LB_C, LB_V},    /* a sentence */
                                          {LB_M, LB_M, LB_R, LB_V},    /* A A B A' */
                                          {LB_M, LB_M, LB_M, LB_V},    /* the hook: A A A A' */
                                          {LB_M, LB_R, LB_M, LB_R},    /* call and response */
                                          {LB_M, LB_REST, LB_M, LB_V}};/* the dub's: space */
        static const int8_t ARCH[6][4] = {{0, 0, 1, 0}, {0, 0, 2, 0}, {0, 0, 1, 0}, {0, 0, 0, 0}, {0, 0, 1, 0}, {0, 0, 0, 0}};
        uint8_t at[8], used[16] = {0};
        int8_t mo[8];
        uint32_t k = 3u + gs.complex / 40u + (a > 60u), nc = 0, tpl, op = gr() % 6u, ct = gr() % 4u;   /* (op: the varied
                                                         * bar's: 0 TRUNC, 1 RETRO, 2 END moved, 3 ORN, 4 AUG, 5 (0.9.17)
                                                         * LATE: its notes an eighth late, syncopated) */
        int32_t anchor, d = 0, pv = 0, base0;
        const char *cell = GL_CELL[gr() % 16u];
        if (a < 35u && k > 3u) k--;
        if (k > 6u) k = 6u;
        tpl = style == LD_HOOK ? (gch(65) ? 4u : 2u) :
              fam == HF_LOOP ? (gch(60) ? 3u : 5u) :
              fam == HF_POP ? (gr() % 3u == 0u ? 1u : gch(50) ? 0u : 4u) :
              fam == HF_JAZZ || fam == HF_JAZZY ? (gch(40) ? 1u : gch(50) ? 2u : 0u) : (gch(50) ? 0u : 1u);
        for (i = 0; i < 16u; i++) if (cell[i] == 'x') { used[i] = 1; nc++; }
        while (nc > k) {                                /* (thinned: a weak onset dropped, never the first) */
            uint32_t p = 1u + gr() % 15u;
            if (used[p] && (p % 4u || gch(30))) { used[p] = 0; nc--; }
        }
        k = nc;
        for (j = 0, i = 0; i < 16u; i++) if (used[i]) at[j++] = (uint8_t)i;
        for (j = 0; j < k; j++) {                       /* the contour: steps mostly, a leap turned back by a step, the
                                                         * shape (an arch, falling, rising, a wave) leaning each move */
            if (j) {
                static const uint8_t W[4] = {8, 64, 22, 6};   /* (a repeat, a step, a third, a fourth or fifth) */
                int32_t m = (int32_t)gpickw(W, 4), dir;
                if (!m && !pv) m = 1;                   /* (never two repeats running) */
                if (m == 3) m = gch(60) ? 3 : 4;
                dir = ct == 0u ? (j <= k / 2u ? 1 : -1) : ct == 1u ? (gch(75) ? -1 : 1) : ct == 2u ? (gch(75) ? 1 : -1) : (j & 1u ? 1 : -1);
                if (pv >= 3 || pv <= -3) { m = 1; dir = pv > 0 ? -1 : 1; }   /* (the leap filled) */
                if (d + dir * m > 4 || d + dir * m < -3) dir = -dir;
                pv = dir * m;
                d += pv;
            }
            mo[j] = (int8_t)d;
        }
        {   /* the anchor: the first note a chord tone of the first chord, the motif centred in the window */
            int32_t lo = 0, hi = 0;
            for (j = 0; j < k; j++) { if (mo[j] < lo) lo = mo[j]; if (mo[j] > hi) hi = mo[j]; }
            anchor = gl_near(&L, (L.dlo + L.dhi) / 2 - (lo + hi) / 2 - 1, at[0], 1);
            if (anchor + hi > L.dhi) anchor -= 7;
            if (anchor + lo < L.dlo) anchor += 7;
            base0 = (int32_t)gchord(0)->deg;
        }
        for (i = 0; i < bars; i++) {
            uint32_t bt = bars == 1u ? LB_M : bars == 2u ? (i ? (tpl == 4u ? LB_R : LB_V) : LB_M) : TPL[tpl][i % 4u];
            uint32_t b0 = i * 16u, last = i + 1u == bars, cad = last ? 2u : (bars >= 4u && i % 4u == 1u && (tpl == 0u || tpl == 4u)) ? 1u : 0u;
            int32_t sh = (int32_t)gchord(b0)->deg - base0, arch = bars >= 4u ? ARCH[tpl][i % 4u] : 0, inv = bt == LB_R;
            uint32_t kk = k, vop = bt == LB_V ? op : 99u;
            const gch_t *bc0 = gchord(b0);
            int lit = gch(25);
            while (sh > 3) sh -= 7;
            while (sh < -3) sh += 7;
            if (bt == LB_REST) continue;
            if (i && (bc0->deg == (uint32_t)base0 ? bt != LB_C : (!lit || bt == LB_C))) {   /* (0.8.3) the motif moved to this bar's
                                                         * chord the voice-led way: its first note the chord tone a step
                                                         * (or a common tone) from the line's last, a step higher into the
                                                         * climax; on the first chord again: as it was */
                int32_t first = anchor + mo[0] + arch, pd = L.k ? L.n[L.k - 1u].d : first, c = (L.dlo + L.dhi) / 2;
                int32_t want = arch > 0 ? 2 : pd > c ? -1 : 1;
                sh = gl_near(&L, pd + want, b0, want) - first;
            } else if (i && bc0->deg == (uint32_t)base0) sh = 0;
            if (bt == LB_C && !sh) sh = 1;              /* (the continuation: a sequence up) */
            if (vop == 0u && kk > 2u) kk--;             /* TRUNC: its last note left out */
            if (vop == 4u) {                            /* (AUG: the notes of its first half, two at least) */
                uint32_t q;
                for (q = 0; q < kk && at[q] < 8u; q++) {}
                if (q >= 2u) kk = q; else vop = 99u;
            }
            {   /* the bar's octave: inside the window, its first note nearest the line's last (never a note folded alone) */
                int32_t o, bo = 0, bc = 0x7FFF, pd = L.k ? L.n[L.k - 1u].d : anchor;
                for (o = -7; o <= 7; o += 7) {
                    int32_t cst = 0, e;
                    for (j = 0; j < kk; j++) {
                        e = anchor + (inv ? -mo[j] + 2 * mo[0] : mo[j]) + sh + arch + o;
                        cst += (e < L.dlo || e > L.dhi) * 5;
                    }
                    e = anchor + mo[0] + sh + arch + o - pd;
                    cst += 2 * (e < 0 ? -e : e);
                    if (cst < bc) { bc = cst; bo = o; }
                }
                sh += bo;
            }
            for (j = 0; j < kk; j++) {
                uint32_t pos = b0 + (vop == 4u ? at[j] * 2u : vop == 5u && at[j] + 2u < 16u ? at[j] + 2u : at[j]);   /* AUG: its
                                                         * first half's rhythm doubled; LATE */
                int32_t mj = vop == 1u ? mo[kk - 1u - j] - mo[kk - 1u] + mo[0] : mo[j];   /* RETRO: its notes backwards
                                                         * from its first (the rhythm kept) */
                int32_t dd = anchor + (inv ? -mj + 2 * mo[0] : mj) + sh + arch;
                if (vop == 4u && at[j] >= 8u) break;
                int strong = at[j] % 8u == 0u || j == 0u, dir = j ? (mo[j] > mo[j - 1] ? 1 : -1) : 0;   /* (beats 1 and 3:
                                                         * a chord tone; 2 and 4 one too, or a passing note, below) */
                if (bt == LB_C && at[j] >= 8u) continue;
                if (pos >= len) break;
                if (vop == 2u && j + 1u == kk) dd += gch(50) ? 1 : -1;   /* END moved */
                if (j && (mo[j] - mo[j - 1] >= 3 || mo[j - 1] - mo[j] >= 3)) dir = -dir;   /* (after a leap: the chord tone
                                                         * on its near side, the leap never widened) */
                if (strong && !gl_ct(dd, pos)) dd = gl_near(&L, dd, pos, inv ? -dir : dir);
                else if (!gl_pal(dd)) dd = gl_near(&L, dd, pos, dir);
                if (cad && j + 1u == kk) dd = gl_cad(&L, dd, pos, cad == 2u);
                if (vop == 3u && j + 1u == kk && at[j] >= 2u && (!L.k || L.n[L.k - 1u].at + 1u < pos - 1u))
                    gl_add(&L, pos - 1u, dd + (gch(50) ? 1 : -1), 70, 0);   /* ORN: a neighbour into the last note */
                gl_add(&L, pos, dd, (strong ? 96u : 82u) + (arch > 0 ? 6u : 0u), cad && j + 1u == kk);
                if (gch(8) && L.k) L.n[L.k - 1u].fl = 1;
                if (cad == 2u && j + 1u == kk && L.k) {  /* (the close held, a breath after it) */
                    uint32_t end = b0 + 14u < len ? b0 + 14u : len - 1u;
                    L.n[L.k - 1u].len = (uint8_t)(end > pos ? end - pos : 0u);
                }
            }
            if (bt == LB_C) {                           /* the continuation: the motif's first half twice, the second a
                                                         * step higher (the climax) */
                for (j = 0; j < kk; j++) {
                    uint32_t pos = b0 + 8u + at[j];
                    int32_t dd = anchor + mo[j] + sh + arch + 1;
                    if (at[j] >= 8u) break;
                    if (pos >= len) break;
                    if ((at[j] % 4u == 0u || j == 0u) && !gl_ct(dd, pos)) dd = gl_near(&L, dd, pos, 1);
                    gl_add(&L, pos, dd, at[j] % 4u ? 84u : 98u, 0);
                }
            }
        }
        gl_write(t, &L, len, legato);
    }
}

/* ------------------------------------------------------------------------------------------------- texture --- */
static void gen_texture(track_t *t, uint32_t len)
{
    uint32_t i, ri = gs.g->tex;
    int32_t n[1], lo = ri == SR_DRONE || ri == SR_SUB || ri == SR_NOISE || ri == SR_TEXTURE ? 43 : 55;   /* (an organ, a
                                                         * choir, strings: above the bass) */
    gseed(GS_TEX);
    uint32_t at = gr() % 3u * 4u, hold = gs.g->form == FM_ARC ? 63u : 15u - at;   /* (0.9.17) on the bar, or a beat or two
                                                         * into it; the chord's root, fifth, third or octave */
    for (i = 0; i + at < len; i += 16u) {
        static const int8_t DG[6] = {0, 0, 4, 4, 2, 7};
        const gch_t *c = gchord(i + at);
        if (i && gch(45)) continue;
        n[0] = gtone(c, DG[gr() % 6u], lo);
        if (n[0] > lo + 19) n[0] -= 12;
        gnote(&t->step[i + at], n, 1, 72, 0);
        dhold(t, i + at, hold, len);
    }
}

/* -------------------------------------------------------------------------------------------------- sounds --- */
static int gnever(const char *s)
{
    uint32_t i;
    for (i = 0; i < NELEM(GEN_NEVER); i++) if (dname_has(s, GEN_NEVER[i])) return 1;
    return 0;
}
/* (0.9.13) a role's release (the drums', bass, chords, lead, arp, texture: bass short but never a click, the chords and
 * the texture long, plucks and arps between) and the filter envelope it adds to its recipe's */
static const uint8_t GREL[RL_N][2] = {{0, 127}, {40, 64}, {78, 104}, {55, 88}, {50, 80}, {70, 110}};
static const uint8_t GENVF[RL_N][2] = {{0, 0}, {6, 20}, {4, 14}, {6, 18}, {8, 24}, {2, 10}};
/* (0.9.17) a lead's and an arp's envelope shapes, ATK DEC SUS REL ranges: plucked, swelling, held with a tail, the
 * recipe's (no range); their weights */
static const uint8_t GENV[3][8] = {{0, 6, 55, 76, 12, 58, 55, 72}, {38, 62, 70, 88, 55, 90, 66, 90}, {2, 14, 62, 84, 60, 88, 64, 86}};
static const uint8_t GENV_LW[4] = {45, 15, 25, 15}, GENV_AW[4] = {60, 8, 17, 15};
static void gen_sound(track_t *t, uint32_t role, uint32_t ri)
{
    const grecipe_t *r = &RECIPE[ri];
    static const uint8_t EW[3] = {55, 30, 15};
    uint32_t e = r->eng[gpickw(EW, 3)], i, k, n, nm = 0, na = 0;
    uint8_t m[32], w[32], any[32];
    const engine_t *en;
    if (gx.eng >= 0) e = (uint32_t)gx.eng;              /* (0.8.6) EXPLORE: any engine (gx_engine) */
    if (e == ENGI_FM6 && gs.fm6 >= 2u) e = r->eng[0] == ENGI_FM6 ? EG_A : r->eng[0];
    gs.fm6 += e == ENGI_FM6;
    set_engine_of(t, e);
    en = trk_engp(t);
    n = en->npresets;
    if (!n) return;
    for (i = 0; i < n; i++) {
        const char *pn = en->presets[i].name;
        if (gnever(pn)) continue;
        if (role != RL_BASS && PR_TAG(&en->presets[i]) == PT_BASS) continue;   /* (0.9.12: by its type, was its name:
                                                         * BASS GROWL ACID; a REESE now too) */
        if (na < sizeof any) any[na++] = (uint8_t)i;
    }
    for (i = 0; i < na; i++) {                          /* (0.9.17) every preset a word of the recipe names, the first
                                                         * word's likeliest (3 2 1), one another track of the piece has
                                                         * already seldom: until 0.9.16 only the first word that fitted,
                                                         * often one preset (MICRO HOUSE's lead: the press before's
                                                         * sound in 45 % of the presses) */
        const char *pn = en->presets[any[i]].name;
        for (k = 0; k < 3u && r->w[k] && !dname_has(pn, r->w[k]); k++) {}
        if (k < 3u && r->w[k]) {
            uint32_t id = e << 8 | any[i], j;
            for (j = 0; j < gs.nsnd && gs.snd[j] != id; j++) {}
            m[nm] = any[i]; w[nm++] = (uint8_t)(j < gs.nsnd ? 1u : 6u - 2u * k);
        }
    }
    k = nm ? m[gpickw(w, nm)] : na ? any[gr() % na] : gr() % n;
    if (gs.nsnd < GEN_NT) gs.snd[gs.nsnd++] = (uint16_t)(e << 8 | k);
    apply_preset_to(t, k);
    for (i = 0; i < 8u; i++) {                          /* (an eighth of each range: still that sound, a bit its own) */
        const param_desc_t *d = track_desc(t, P_E0 + i);
        int32_t span = (d->max - d->min) / 8;
        if (!d->label || d->label[0] == '-' || d->max <= d->min || d->fmt == F_ENUM || !span) continue;
        t->p[P_E0 + i] = (int16_t)clamp(t->p[P_E0 + i] + grr(-span, span), d->min, d->max);
    }
    t->p[P_ATK] = (int16_t)grr(r->atk[0], r->atk[1]);
    t->p[P_DEC] = (int16_t)grr(r->dec[0], r->dec[1]);
    t->p[P_SUS] = (int16_t)grr(r->sus[0], r->sus[1]);
    t->p[P_REL] = (int16_t)grr(r->rel[0], r->rel[1]);
    if ((role == RL_LEAD || role == RL_ARP) && r->sus[1] < 120u) {   /* (0.9.17, "too many leads with full sustain and
                                                         * no release") a lead's or an arp's envelope a shape of its own:
                                                         * plucked (a real decay to a sustain well under the top, a short
                                                         * release), swelling (a slow attack), held with a tail; the
                                                         * recipe's as it was the rest of the time; an organ's (its
                                                         * recipe's sustain at the top) left full */
        uint32_t sh = gpickw(role == RL_LEAD ? GENV_LW : GENV_AW, 4);
        const uint8_t *e6 = GENV[sh];
        if (sh < 3u) {
            t->p[P_ATK] = (int16_t)grr(e6[0], e6[1]); t->p[P_DEC] = (int16_t)grr(e6[2], e6[3]);
            t->p[P_SUS] = (int16_t)grr(e6[4], e6[5]); t->p[P_REL] = (int16_t)grr(e6[6], e6[7]);
        }
    }
    {   /* (0.9.17) the release inside the tempo: a plucked or short part's tail three sixteenths at most at the genre's
         * tempo (a pad's, a texture's: as long as they are), never under the role's floor */
        uint32_t ms10 = 3u * 150000u / ((gs.g->bpm_lo + gs.g->bpm_hi) / 2u + 1u), v = (uint32_t)t->p[P_REL];
        if (role != RL_PAD && role != RL_TEX && t->p[P_SUS] < 100) while (v > GREL[role][0] && TIME_MS_X10[v] > ms10) v--;
        t->p[P_REL] = (int16_t)clamp((int32_t)v, GREL[role][0], GREL[role][1]);   /* (0.9.13: never dry) */
    }
    if (e == ENGI_FM6) {                                /* (0.9.14) FM6: its carriers' R4 left at 40 at least (~4 s at the
                                                         * most; eng_fm6.c fm6_rel_r4), not 1 (minutes: every chord rang on) */
        uint32_t r4 = fm6_rel_r4(t), mx = r4 > 40u ? (r4 - 40u) * 127u / 90u : 0u;
        if ((uint32_t)t->p[P_REL] > mx) t->p[P_REL] = (int16_t)mx;
    }
    t->p[P_LD_PIT] = t->p[P_LD_FLT] = t->p[P_LD_SHP] = t->p[P_LD_AMP] = t->p[P_LFADE] = 0;   /* (0.9.13) the modulation
                                                         * from nothing: no preset's LFO, no track's before, left over */
    if (e == EG_FM) t->p[P_E2] = (int16_t)clamp(t->p[P_E2] - grr(4, 20), -64, 63);   /* (warmer) */
    if (r->ftype != 255u) {
        int32_t cut = grr(r->cut[0], r->cut[1]), res = grr(r->res[0], r->res[1]), ef = 0;
        if (role == RL_BASS && e != EG_A) cut += 20;    /* (a bass: only its fizz taken) */
        if (gs.g->human >= 40u && role != RL_BASS) cut += 10;   /* (a band: air) */
        if (gs.energy > 66u) cut += 6;
        if (role != RL_TEX && gs.g->wet < 85u && gch(70)) {   /* (0.9.17, "more low cutoff and envelope to the filter,
                                                         * resonance and drive") most parts darker, the envelope opening
                                                         * each note, more resonance the lower the cutoff (never a peak:
                                                         * 30 more at most); the machines' loops (house, techno ..) more of
                                                         * it, the wet rooms (ambient, dub techno, film) as they were */
            int32_t lo = HXF(gs.hx) == HF_LOOP && gs.g->human < 20u ? 8 : 0, d = grr(4, 14) + lo;
            cut -= d; ef = d / 2 + grr(2, 8);
            res += clamp((84 - cut) / 3, 0, 18) + lo / 2;
            if (res > r->res[1] + 30) res = r->res[1] + 30;
        }
        t->p[P_FTYPE] = (int16_t)r->ftype;
        if (e == EG_A) {                                /* (0.6.93: the recipe's cutoff, not the duller of the two) */
            t->p[P_E4] = (int16_t)clamp(cut, 0, 127);
            t->p[P_E5] = (int16_t)clamp(res, 0, 90);
            t->p[P_FTYPE] = 0;
        } else {
            t->p[P_FCUT] = (int16_t)clamp(cut, 0, 127);
            t->p[P_FRES] = (int16_t)res;
        }
        t->p[P_ED_FLT] = (int16_t)clamp(grr(r->envf[0], r->envf[1]) + grr(GENVF[role][0], GENVF[role][1]) + ef, -64, 63);   /* (0.9.13:
                                                         * the filter's envelope on every sound, a role's more) */
    }
    if (r->lfo[1]) {
        t->p[P_LWAVE] = (int16_t)(gch(70) ? 0 : 1);
        t->p[P_LRATE] = (int16_t)grr(r->lfo_rate[0], r->lfo_rate[1]);
        t->p[P_LD_FLT] = (int16_t)grr(r->lfo[0], r->lfo[1]);
    } else if (role != RL_DRUM && gch(role == RL_BASS ? 30u : role == RL_LEAD ? 45u : 65u)) {   /* (0.9.13) the filter
                                                         * moving slowly, a cycle over one, two or four bars at the genre's
                                                         * tempo; 0.9.17 ("LFO to the filters, soft") the lead's and the
                                                         * bass's too, softly */
        uint32_t hz = (uint32_t)(gs.g->bpm_lo + gs.g->bpm_hi) * 50u / (240u << (gr() % 3u)), v = 0;
        while (v < 127u && LFO_HZ_X100[v] < hz) v++;
        t->p[P_LWAVE] = 0; t->p[P_LRATE] = (int16_t)v; t->p[P_LD_FLT] = (int16_t)(role == RL_BASS || role == RL_LEAD ? grr(5, 12) : grr(8, 20));
    }
    if ((role == RL_LEAD || (role == RL_PAD && !t->p[P_LD_FLT])) && t->p[P_SUS] >= 40 && gch(role == RL_LEAD ? 50u : 20u)) {   /* (0.6.97)
                                                         * a vibrato that comes in late, now and then a tremolo; 0.9.13:
                                                         * soft (a fifth of a semitone, a third now and then: LD_PIT 1..2
                                                         * x 3/16) on a held lead only; 0.9.17 ("or a soft vibrato with a
                                                         * high rate") 5 .. 8 Hz, a pad's too, the filter's LFO then off */
        t->p[P_LWAVE] = 0; t->p[P_LRATE] = (int16_t)grr(88, 96); t->p[P_LFADE] = (int16_t)grr(70, 100);
        t->p[P_LD_PIT] = (int16_t)(gch(75) ? 1 : 2); t->p[P_LD_FLT] = 0;
        if (gch(30)) t->p[P_LD_AMP] = (int16_t)grr(6, 16);
    } else if (role == RL_PAD && gch(35)) {             /* (a pad breathing: a slow tremolo or a slow pan) */
        if (gch(50)) { t->p[P_LWAVE] = 0; t->p[P_LRATE] = (int16_t)grr(6, 18); t->p[P_LD_AMP] = (int16_t)grr(12, 30); }
        else { t->p[P_APDEP] = (int16_t)grr(25, 55); t->p[P_APSYNC] = 1; t->p[P_APRATE] = (int16_t)grr(1, 3); }
    } else if (role == RL_ARP && gch(40)) {             /* (an arp moving across the stereo on the beat) */
        t->p[P_APDEP] = (int16_t)grr(30, 70); t->p[P_APSYNC] = 1; t->p[P_APRATE] = (int16_t)grr(3, 5);
    }
    if (r->dist[1] && gch(70)) { t->p[P_DIST] = (int16_t)grr(r->dist[0], r->dist[1]); t->p[P_DTYPE] = (int16_t)(gch(70) ? 0 : 2); }
    else t->p[P_DIST] = 0;
    t->p[P_CHOR] = (int16_t)(r->chor[1] ? grr(r->chor[0], r->chor[1]) : 0);
    t->p[P_GLIDE] = (int16_t)(r->glide ? grr(r->glide - 10, r->glide + 10) : 0);
    t->p[P_VOICE] = (int16_t)r->voice;
    t->p[P_DETUNE] = (int16_t)(ri == SR_REESE || ri == SR_WALL || ri == SR_HOOVER ? grr(30, 60) : t->p[P_DETUNE]);
    if (ri == SR_GTR || ri == SR_DRONE) {               /* (a tremolo: AUTO PAN on the beat) */
        t->p[P_APDEP] = (int16_t)grr(30, 60); t->p[P_APSYNC] = 1; t->p[P_APRATE] = (int16_t)grr(4, 6); t->p[P_APWAVE] = (int16_t)(ri == SR_GTR ? 3 : 0);
    }
}
static void gen_kit(track_t *t)
{
    const gkit_t *k = &KIT[gs.g->kit];
    uint32_t l, i;
    for (l = 0; l < 8u; l++) {
        const dx_lane_t *D = &DX_KIT_DEF[l];
        dx_lane_t *L = &dx_kit[l];
        L->mode = D->mode;
        for (i = 0; i < DXP_N; i++) {
            int32_t j = grr(-6, 6) * (int32_t)gs.complex / 100;
            L->a[i] = (uint8_t)clamp(D->a[i] + k->d[l][i] + j, 0, 127);
            L->b[i] = (uint8_t)clamp(D->b[i] + k->d[l][i] + j, 0, 127);
        }
    }
    memcpy(dx_lay, DX_LAY_DEF, sizeof dx_lay);
    {   /* (0.6.94) the snare a snare (it came out tonal): mostly noise, shorter, less of the pitch sweep, the noise's
         * band a little brighter, and its own FEEDBK on top of the LAYER */
        dx_lane_t *S = &dx_kit[DV_SNARE];
        int32_t nz = grr(26, 40), dc = grr(10, 18), pm = grr(8, 14), cl = grr(4, 10);
        S->a[DXP_NOISE] = (uint8_t)clamp(S->a[DXP_NOISE] + nz, 0, 127); S->b[DXP_NOISE] = (uint8_t)clamp(S->b[DXP_NOISE] + nz, 0, 127);
        S->a[DXP_DECAY] = (uint8_t)clamp(S->a[DXP_DECAY] - dc, 0, 127); S->b[DXP_DECAY] = (uint8_t)clamp(S->b[DXP_DECAY] - dc, 0, 127);
        S->a[DXP_PMOD] = (uint8_t)clamp(S->a[DXP_PMOD] - pm, 0, 127); S->b[DXP_PMOD] = (uint8_t)clamp(S->b[DXP_PMOD] - pm, 0, 127);
        S->a[DXP_COLOR] = (uint8_t)clamp(S->a[DXP_COLOR] + cl, 0, 127); S->b[DXP_COLOR] = (uint8_t)clamp(S->b[DXP_COLOR] + cl, 0, 127);
        dx_lay[DV_SNARE] = (uint8_t)(DX_LAY_L(DX_LAY_DEF[DV_SNARE]) | ((uint32_t)grr(3, 6) << 4));
    }
    {   /* (0.6.97) the snare and the clap into the reverb (the kit's space), the rim into the delay where the genre is
         * wet (dub, dub techno ..): the kit's own SENDs (eng_drum.c dx_snd) */
        int32_t wet = (int32_t)gs.g->wet, rv = clamp((18 + grr(0, 22)) * (40 + wet) / 100, 6, 63);
        dx_snd[DV_SNARE] = (uint8_t)((3u << 6) | (uint32_t)rv);
        dx_snd[DV_CLAP] = (uint8_t)((3u << 6) | (uint32_t)clamp(rv + grr(-6, 10), 6, 63));
        dx_snd[DV_RIM] = (uint8_t)(wet >= 60 ? (2u << 6) | (uint32_t)grr(20, 45) : 0u);
        dx_snd[DV_HATO] = (uint8_t)(wet >= 70 && gch(50) ? (3u << 6) | (uint32_t)grr(8, 20) : 0u);
        {   /* (0.9.17, "percs with sends") the percussion now and then into the delay or the reverb, a little, as wet
             * as the genre (the kick never: lane 0; the snare and the clap: above) */
            uint32_t l;
            dx_snd[DV_KICK] = dx_snd[DV_HATC] = dx_snd[DV_TOM] = dx_snd[DV_BELL] = 0;
            for (l = DV_HATC; l < 8u; l++)
                if (!dx_snd[l] && gch(l == DV_HATC ? 30u : 40u))
                    dx_snd[l] = (uint8_t)((gch(55) ? 2u : 3u) << 6 | (uint32_t)clamp(grr(6, l == DV_HATC ? 14 : 28) * (40 + wet) / 100, 3, 40));
        }
    }
    for (l = DV_HATC; l <= DV_HATO; l++) {               /* (0.6.94) the hats: a band-pass high up, more noise in them */
        dx_lane_t *H = &dx_kit[l];
        int32_t nz = grr(18, 28), cl = grr(10, 18);
        H->mode = (uint8_t)((H->mode & ~0x0Cu) | (DXF_BP << 2));
        H->a[DXP_NOISE] = (uint8_t)clamp(H->a[DXP_NOISE] + nz, 0, 127); H->b[DXP_NOISE] = (uint8_t)clamp(H->b[DXP_NOISE] + nz, 0, 127);
        H->a[DXP_COLOR] = (uint8_t)clamp(H->a[DXP_COLOR] + cl, 0, 127); H->b[DXP_COLOR] = (uint8_t)clamp(H->b[DXP_COLOR] + cl, 0, 127);
    }
    if (gx.on)                                          /* (0.8.6) EXPLORE: the sounds far from the kit's, each its own */
        for (l = 0; l < 8u; l++) {
            dx_lane_t *L = &dx_kit[l];
            for (i = 0; i < DXP_N; i++) {
                int32_t j = gch(40) ? grr(-20, 20) : 0;
                L->a[i] = (uint8_t)clamp(L->a[i] + j, 0, 127);
                L->b[i] = (uint8_t)clamp(L->b[i] + j + grr(-6, 6), 0, 127);
            }
        }
    t->p[P_E0] = (int16_t)grr(40, 88);                  /* MORPH */
    t->p[P_E2] = (int16_t)clamp(70 + k->tone, 0, 127);
    t->p[P_E3] = (int16_t)clamp(64 + k->decy, 0, 127);
    t->p[P_E4] = (int16_t)clamp(64 + k->nois, 0, 127);
    t->p[P_E7] = (int16_t)clamp(k->drv, 0, 127);
    t->p[P_FTYPE] = 0;
    t->p[P_FCUT] = (int16_t)(gs.g->human >= 40u ? grr(118, 127) : grr(104, 120));
    t->p[P_FRES] = (int16_t)grr(0, 14);
    for (l = 0; l < 8u; l++) t->p[P_LN0 + l] = 127;
    t->p[P_LN3] = (int16_t)(gs.g->hats == HC_MACHINE ? 112 : 118);   /* (the hats a little under) */
    t->p[P_LN4] = 118;
    t->p[P_LN6] = t->p[P_LN7] = 110;
}
/* the mix and the humanity of a role */
static void gen_mix(track_t *t, uint32_t r)
{
    const ggenre_t *g = gs.g;
    int32_t wet = (int32_t)g->wet + (gs.energy < 34 ? 20 : gs.energy > 66 ? -15 : 0), hum = (int32_t)(gs.human);
    static const uint8_t HV[RL_N] = {20, 25, 45, 60, 35, 40}, HT[RL_N] = {0, 0, 30, 25, 10, 60}, HG[RL_N] = {0, 10, 10, 40, 30, 0};
    wet = clamp(wet, 0, 100);
    t->p[P_LEVEL] = (int16_t)(r == RL_DRUM ? 106 : r == RL_BASS ? 102 : r == RL_LEAD ? 88 : r == RL_TEX ? (g->tex == SR_NOISE ? 44 : 54) : r == RL_PAD ? 80 : 82);
    t->p[P_PAN] = (int16_t)(r == RL_ARP ? grr(-30, -10) : r == RL_TEX ? grr(10, 30) : r == RL_PAD ? grr(-8, 8) : r == RL_LEAD ? grr(-6, 12) : 0);
    t->p[P_REV] = (int16_t)clamp((r == RL_PAD || r == RL_TEX ? grr(45, 95) : r == RL_LEAD ? grr(30, 70) : r == RL_ARP ? grr(20, 60) :
                                  r == RL_DRUM ? grr(4, 20) : grr(0, 6)) * (40 + wet) / 100, 0, 127);
    t->p[P_DLY] = (int16_t)clamp((r == RL_LEAD || r == RL_ARP ? grr(25, 70) : r == RL_PAD && (g->pads == CS_DUB || g->pads == CS_SKANK) ? grr(55, 95) :
                                  r == RL_PAD ? grr(0, 30) : r == RL_DRUM ? grr(0, 10) : 0) * (40 + wet) / 100, 0, 127);
    if (r == RL_LEAD && t->p[P_REV] < 25 && t->p[P_DLY] < 20) t->p[P_DLY] = (int16_t)grr(25, 55);
    t->p[P_SSWING] = 0;
    t->p[P_HVEL] = (int16_t)(127 - clamp((HV[r] + HT[r] + HG[r]) * hum / 150, 0, 127));   /* (0.6.92: one amount) */
    t->p[P_HTIM] = t->p[P_HGAT] = 127;
}

/* ---------------------------------------------------------------------------------------- the form: parts --- */
static void gen_role(track_t *t, uint32_t k, uint32_t len)   /* a track's part as its role plays it */
{
    uint32_t r = gs.role[k];
    if (!gs.act[r] || (r == RL_TEX && k != gs.trk_of[RL_TEX])) return;
    switch (r) {
    case RL_DRUM: gen_drums(t, len); break;
    case RL_BASS: gen_bass(t, len); break;
    case RL_PAD: gen_pads(t, len); break;
    case RL_ARP: gen_arp(t, len); break;
    case RL_LEAD: gen_lead(t, len); break;
    default: gen_texture(t, len); break;
    }
}
static void gen_post(void);
/* (0.8.3) a section's harmony against A's: the loop started on its predominant (IV or VI, the same chords turned: B,
 * the break; D, the club's break and the arc's apex), a one-chord loop going to its subdominant and back; the chords
 * of the section's plan as rot says */
static uint32_t gsec_rot(void)
{
    uint32_t k;
    if (gs.pn == 1u) {
        gch_t c = {0, 0, (uint8_t)(gs.sc[3] == 5u ? 3u : 1u), 0, 0};   /* (IV or iv; lydian: II) */
        if (!gtriad_ok(&c)) return 0;
        gs.pdeg[1] = gs.pdeg[0]; gs.pq[1] = gs.pq[0];
        gs.pdeg[0] = c.deg; gs.pq[0] = QQ(gs.pq[1]);
        gs.pn = 2;
        return 0;
    }
    for (k = 1; k < gs.pn; k++) if ((gs.pdeg[k] == 3u || gs.pdeg[k] == 5u) && !QF(gs.pq[k])) return k;
    return 1;
}
/* the pitched parts again over the section's plan (each from its own stream: the same motif, the same figures, on
 * these chords), the polish after; lift: the whole of it moved up that many semitones, the SCALE page's ROOT with it
 * (a real modulation, stored in the section: the song plays it in its key) */
static void gen_pitched(uint32_t rot, int32_t lift)
{
    uint32_t k, i, j, dr = gs.trk_of[RL_DRUM];
    gs.kick = gs.snare = 0;                             /* (the bass reads this section's kick) */
    if (dr < GEN_NT && gs.act[RL_DRUM])
        for (i = 0; i < dlen(&trk[dr]); i++)
            if (trk[dr].step[i].time == ST_NOTE) { gs.kick |= (uint64_t)(trk[dr].step[i].hit & 1u) << i; gs.snare |= (uint64_t)((trk[dr].step[i].hit >> 1) & 1u) << i; }
    gplan(rot);
    for (k = 0; k < GEN_NT; k++) {
        track_t *t = &trk[k];
        if (k == dr) continue;
        load_begin(t, LOAD_PAT);
        for (j = 0; j < NSTEP; j++) gclear(&t->step[j]);
        motion_clear(t);
        gen_role(t, k, dlen(t));
        load_end(t);
    }
    gseed(GS_FORM);
    gen_post();
    for (k = 0; k < GEN_NT; k++) {
        track_t *t = &trk[k];
        t->p[P_ROOT] = (int16_t)((gs.root + lift + 12) % 12);
        if (k == dr || !lift) continue;
        for (i = 0; i < NSTEP; i++)
            for (j = 0; j < t->step[i].n && t->step[i].time == ST_NOTE; j++) t->step[i].note[j] = (uint8_t)clamp(t->step[i].note[j] + lift, 0, 127);
        pat_sig[k] = steps_sig(t);
    }
}
static step_t gen_keep[NSTEP];                          /* (the drums of the section before; 0.8.3: the chords are made
                                                         * again, not kept) */
/* (0.9.14) the FORM's order: one of several orders a form takes, and each row's length (in halves of the form's
 * bars: 1 half, 2 as set, 4 double), picked from the SEED, GENRE and FORM alone (not the piece's random streams, so
 * the GEN screen shows it before MAKE). Before, PARTS was always A B A C and SONG B C A D C A (or B A C D), whatever
 * the seed. A: the whole piece, B a break, C a drop / a turn, D the apex (gen_parts) */
#define GF_MAXROWS 8u
static uint32_t gform_hash(void)
{
    uint32_t h = 2166136261u;
    h = (h ^ gp.seed) * 16777619u; h = (h ^ gp.genre) * 16777619u; h = (h ^ gp.form) * 16777619u;
    h ^= h >> 13; h *= 2246822519u; h ^= h >> 16;
    return h;
}
static uint32_t gform_order(uint32_t form, uint8_t *ord, uint8_t *mul)
{
    static const uint8_t PARTS[8][7] = {{4, 0, 1, 0, 2}, {4, 0, 0, 1, 2}, {4, 0, 1, 2, 0}, {4, 0, 2, 0, 1},
                                        {4, 1, 0, 2, 0}, {5, 0, 1, 0, 2, 2}, {4, 0, 2, 1, 0}, {6, 0, 1, 0, 2, 1, 0}};
    static const uint8_t CLUB[8][8] = {{6, 1, 2, 0, 3, 2, 0}, {5, 0, 1, 2, 3, 0}, {6, 1, 0, 2, 3, 2, 0}, {6, 1, 2, 3, 0, 2, 0},
                                       {6, 0, 2, 1, 3, 3, 0}, {7, 1, 1, 0, 2, 3, 2, 0}, {6, 0, 1, 0, 2, 3, 2}, {5, 2, 0, 1, 3, 0}};
    static const uint8_t ARC[6][6] = {{4, 1, 0, 2, 3}, {4, 0, 1, 2, 3}, {5, 1, 0, 3, 2, 0}, {4, 1, 2, 0, 3}, {4, 0, 1, 3, 2},
                                      {5, 1, 0, 2, 3, 0}};
    uint32_t h = gform_hash(), k, n;
    const uint8_t *t = form == FM_CLUB ? CLUB[h % 8u] : form == FM_ARC ? ARC[h % 6u] : PARTS[h % 8u];
    n = t[0];
    for (k = 0; k < n; k++) {
        uint32_t r = (h >> (8u + 3u * k)) % 8u;          /* (a row's length: mostly as set, now and then double or half) */
        ord[k] = t[1 + k];
        mul[k] = (uint8_t)(r < 5u ? 2u : r < 7u ? 4u : 1u);
        if (form == FM_CLUB) mul[k] = ord[k] == 0u ? 4u : r < 6u ? 2u : 1u;   /* (a club's drop, A, the longest) */
    }
    return n;
}
static void section_store(uint32_t s);                  /* (song_main.c) */
static int section_load(uint32_t s);
static void gen_parts(void)
{
    const ggenre_t *g = gs.g;
    uint32_t form = gp.form == 0u ? FM_LOOP : gp.form == 2u ? (g->form == FM_ARC ? FM_ARC : FM_CLUB) : g->form == FM_LOOP ? FM_PARTS : g->form;
    uint32_t dr = gs.trk_of[RL_DRUM], bs = gs.trk_of[RL_BASS], pd = gs.trk_of[RL_PAD], ld = gs.trk_of[RL_LEAD], ar = gs.trk_of[RL_ARP];
    uint32_t k, n = 0, rows = 0, bars = gp.form == 2u ? 16u : 8u, i, fam = HXF(gs.hx);
    static const uint8_t FILLS[6] = {FL_ROLL, FL_TOM, FL_HAT, FL_DROP, FL_LINEAR, FL_KICK16};   /* (0.6.97) a fill a section */
    uint8_t pdeg[4], pq[4], pn = gs.pn;
    uint8_t ord[GF_MAXROWS], mul[GF_MAXROWS];
    uint32_t nord;
    if (form == FM_LOOP) return;
    gseed(GS_FORM);
    for (k = 0; k < 4u; k++)
        if (project_used(k) && !((dice_secs >> k) & 1u)) return;   /* (a section of yours: kept) */
    live.cur = -1;
    section_store(0);                                   /* A: the whole piece */
    if (live.cur != 0) return;
    dice_secs |= 1u;
    memcpy(pdeg, gs.pdeg, sizeof pdeg); memcpy(pq, gs.pq, sizeof pq);
    if (dr < GEN_NT) memcpy(gen_keep, trk[dr].step, sizeof gen_keep);
    n = form == FM_CLUB ? 4u : form == FM_ARC ? 4u : 3u;
    for (k = 1; k < n; k++) {
        uint32_t rot = 0;
        int32_t lift = 0;
        /* back to the whole piece, then this section's cut */
        for (i = 0; i < GEN_NT; i++) trk[i].p[P_MUTE] = 0;
        memcpy(gs.pdeg, pdeg, sizeof pdeg); memcpy(gs.pq, pq, sizeof pq); gs.pn = pn;
        if (dr < GEN_NT) { load_begin(&trk[dr], LOAD_PAT); memcpy(trk[dr].step, gen_keep, sizeof gen_keep); motion_clear(&trk[dr]); load_end(&trk[dr]); }
        if (dr < GEN_NT && gs.act[RL_DRUM]) {             /* (0.6.97) each section its own fill (the BUILD: a roll; the
                                                         * drop and the apex denser); the phrase's last bar differs */
            track_t *t = &trk[dr];
            uint32_t f0 = g->fills == FL_NONE ? FL_ROLL : g->fills, a0 = gs.amt[GP_DRUMS], c0 = gs.complex;
            int dense = (form == FM_ARC && k == 3u) || (form == FM_PARTS && k == 2u);
            load_begin(t, LOAD_PAT);
            for (i = 0; i < NSTEP; i++) gclear(&t->step[i]);
            gs.drum_fill = form == FM_CLUB && k == 2u ? FL_ROLL : dense ? 0u : k == 1u ? f0 : FILLS[(gs.gi + k) % 6u];
            if (dense) {
                gs.amt[GP_DRUMS] = a0 + (form == FM_ARC ? 30u : 25u) > 100u ? 100u : a0 + (form == FM_ARC ? 30u : 25u);
                if (form == FM_PARTS) gs.complex = c0 + 20u > 100u ? 100u : c0 + 20u;
            }
            gen_drums(t, dlen(t));
            gs.drum_fill = 0; gs.amt[GP_DRUMS] = a0; gs.complex = c0;
            load_end(t);
            if (!(form == FM_CLUB && k == 2u) && !dense) memcpy(gen_keep, trk[dr].step, sizeof gen_keep);
        }
        /* (0.8.3) the section's harmony: the break and the apex on the predominant (not the modal loops: theirs keep
         * the loop); PARTS' last section a whole step up where the genre lifts (pop's last chorus) */
        if (fam != HF_LOOP && ((form == FM_PARTS && k == 1u) || (form != FM_PARTS && k == 3u))) rot = gsec_rot();
        if (form == FM_PARTS && k == 2u && (gs.hx & HX_LIFT)) {
            static const int8_t XL[7] = {1, 3, 5, 6, -1, -3, -5};   /* (0.8.6) EXPLORE: anywhere, not a whole step */
            lift = gx.on ? XL[gr() % 7u] : 2;
        }
        gen_pitched(rot, lift);
        if (form == FM_CLUB) {
            if (k == 1u) {                              /* B INTRO: the drums and the bass */
                if (pd < GEN_NT) trk[pd].p[P_MUTE] = 1;
                if (ld < GEN_NT) trk[ld].p[P_MUTE] = 1;
                if (ar < GEN_NT) trk[ar].p[P_MUTE] = 1;
            } else if (k == 2u) {                       /* C BUILD: the chords in, their filter opening over the loop, a
                                                         * roll at its end, the delay thrown on the last step */
                if (ld < GEN_NT) trk[ld].p[P_MUTE] = 1;
                memset(pfx_lane, 0, sizeof pfx_lane);   /* (0.6.97) the punch-in lane: a stutter into the drop (1/8 on
                                                         * the last beat, 1/16 on its last two sixteenths), every track */
                pfx_ltgt = 0;
                for (i = 60; i < 64u; i++) pfx_lane_put(i, i < 62u ? 6u : 7u);
                if (pd < GEN_NT && gs.act[RL_PAD]) {
                    track_t *t = &trk[pd];
                    uint32_t id = t->eng_req == EG_A ? P_E4 : P_FCUT, base = (uint32_t)clamp(t->p[id], 0, 127), len = dlen(t), j;
                    for (j = 0; j < 4u && j * len / 4u < len; j++)
                        motion_set_event(t, j * len / 4u, id, (int16_t)clamp((int32_t)base - 30 + (int32_t)j * 14, 0, 127));
                    motion_set_event(t, len - 1u, P_DLY, (int16_t)clamp(t->p[P_DLY] + 50, 0, 127));
                    motion_set_event(t, 0, P_DLY, t->p[P_DLY]);
                }
            } else {                                    /* D BREAK: no drums, no bass; the chords wetter; (0.6.97) the
                                                         * synths an octave up on the break's last bar */
                memset(pfx_lane, 0, sizeof pfx_lane);
                pfx_ltgt = 1;
                for (i = 48; i < 64u; i++) pfx_lane_put(i, 2u);
                if (dr < GEN_NT) trk[dr].p[P_MUTE] = 1;
                if (bs < GEN_NT) trk[bs].p[P_MUTE] = 1;
                if (pd < GEN_NT) motion_set_event(&trk[pd], 0, P_REV, (int16_t)clamp(trk[pd].p[P_REV] + 30, 0, 127));
            }
        } else if (form == FM_ARC) {
            if (k == 1u) {                              /* B: thin (the chords and the bass) */
                if (dr < GEN_NT) trk[dr].p[P_MUTE] = 1;
                if (ld < GEN_NT) trk[ld].p[P_MUTE] = 1;
                if (ar < GEN_NT) trk[ar].p[P_MUTE] = 1;
            } else if (k == 2u) {                       /* C: the drums in, the lead out */
                if (ld < GEN_NT) trk[ld].p[P_MUTE] = 1;
            }                                           /* D: the apex, the drums denser (above) */
        } else if (k == 1u) {                           /* FM_PARTS: B a break (drums and bass out), C a drop (denser
                                                         * drums, above) */
            if (dr < GEN_NT) trk[dr].p[P_MUTE] = 1;
            if (bs < GEN_NT && (ld < GEN_NT || ar < GEN_NT || pd < GEN_NT)) trk[bs].p[P_MUTE] = 1;
        }
        section_store(k);
        if (live.cur != (int8_t)k) { n = k; break; }
        dice_secs |= (uint8_t)(1u << k);
    }
    memcpy(gs.pdeg, pdeg, sizeof pdeg); memcpy(gs.pq, pq, sizeof pq); gs.pn = pn;
    gplan(0);                                           /* (A's plan: the music live is A) */
    section_load(0);                                    /* (the whole piece back, as A holds it) */
    live.cur = 0;
    if (n < 2u) return;
    chain_defaults(&chain_config);
    nord = gform_order(form, ord, mul);                 /* (0.9.14) the order and lengths from the seed (gform_order) */
    for (k = 0; k < nord && rows < CHAIN_ROWS; k++) {
        if (ord[k] >= n) continue;
        chain_config.row[rows].slot = ord[k];
        chain_config.row[rows].bars = (uint8_t)(bars * mul[k] / 2u);
        rows++;
    }
    chain_config.count = (uint8_t)rows;
    SONG_END(&chain_config) = SONG_FOREVER;
    ui.song_row = 0;
    live.mode = 1;
}

/* ------------------------------------------------------------------------------------------- (0.8.6) EXPLORE --- */
/* EXPLORE (GENRE's first choice, the dice's default): no genre's template. Its genre is thrown, field by field, from
 * everything the tables hold: any tempo from 56 to 200 BPM, any drum map (the procedural IDM's too) and kit with its
 * sounds pulled far from the kit's, any three bass archetypes against any kick rule, any chord play and voicing, any
 * arp, any lead, the recipes a role may take beyond its usual ones (a plucked bass, a bell lead), any space; its key a
 * mode of seven of the SCALE page (Phrygian, Lydian, harmonic and melodic minor, Locrian likeliest), the lead's degrees
 * a pentatonic, a hexatonic or a gapped set over it; its harmony any family with any of the colours (secondary
 * dominants, mixture, the raised dominant, chromatic approaches). Then what no genre does: a progression of one to four
 * chords on any degrees in any order (non-functional: a chord a triad the mode takes, any quality, now and then a
 * borrowed, secondary or flat-side chord: the out-of-key moves), its chords 8 to 48 steps long and the loop 8 to 64
 * (odd metres: 3/4, 5/4, 7/8 ..); each track its own loop and DIV (1/8, 1/32, 8T, 16T: polyrhythm), any engine on any
 * role (ANIMAL, VOICE singing words, FM6 twice at most as the genres, ANIMAL and VOICE twice at most), DRUM-X's octaves
 * per sound on the steps (x2 x4 x8, SHORT GHOST FLAM), hits off the grid, step components scattered (RAND OCT BEND
 * ECHO PULSE WALK CLIMB ARP TONAL JUMP REV, some every other loop), PLAYHEADS, MUTATE, the delay's pitch and spray,
 * any reverb. What it keeps: every note in the key the SCALE page shows but the colours' (the CHORD page as the chords
 * are), two melodic voices at least, the genres' levels, the voice and engine budgets. A seed is a piece (determinism
 * as every genre's); the genres' pieces are untouched by it (their streams, their tables) */
/* its fields thrown: each grr(lo, hi), 0 one time in z % (a part left out) */
typedef struct { uint8_t off, lo, hi, z; } gxf_t;
#define GXF(f, lo, hi, z) {(uint8_t)__builtin_offsetof(ggenre_t, f), (uint8_t)(lo), (uint8_t)(hi), z}
static const gxf_t GX_FIELD[] = {
    GXF(swing, 8, 60, 60), GXF(drum, 0, DM_NONE - 1, 0), GXF(kit, 0, KIT_N - 1, 0), GXF(ghosts, 0, 4, 0),
    GXF(hats, 0, HC_MACHINE, 0), GXF(fills, 0, FL_BREAK, 0), GXF(bass[0], 0, BA_N - 1, 0), GXF(bass[1], 0, BA_N - 1, 0),
    GXF(bass[2], 0, BA_N - 1, 0), GXF(bass_lo, 29, 40, 0), GXF(kick_mode, 0, KM_INTERLOCK, 0), GXF(bass_gate, 30, 115, 0),
    GXF(cbars, 1, 2, 0), GXF(pads, 0, CS_TRANCE, 0), GXF(voicing, 0, VM_PLANE, 0), GXF(arp[0], 0, AP_N - 1, 0),
    GXF(arp[1], 0, AP_N - 1, 0), GXF(lead, 0, LD_NONE - 1, 0),   /* (never NONE: LEAD OFF is the knob's) */
    GXF(amt[GP_DRUMS], 40, 100, 12), GXF(amt[GP_BASS], 40, 100, 0), GXF(amt[GP_CHORDS], 30, 100, 15),
    GXF(amt[GP_ARP], 20, 100, 25), GXF(amt[GP_LEAD], 25, 90, 15), GXF(rtype, 0, 4, 0), GXF(dtime, 0, 7, 0),
    GXF(wet, 10, 95, 0), GXF(dtype, 0, 3, 0), GXF(form, 0, FM_ARC, 0), GXF(human, 0, 90, 0),
};
static const uint8_t *const GX_POOL[5] = {0, GX_BASS, GX_PADS, GX_ARPS, GX_LEADS};   /* (a role's recipes, GP_ order) */
static const uint8_t GX_NPOOL[5] = {0, NELEM(GX_BASS), NELEM(GX_PADS), NELEM(GX_ARPS), NELEM(GX_LEADS)};
static void gx_genre(ggenre_t *m)
{
    uint32_t hw = 16u + 48u * gx.w / 100u, lo = (uint32_t)grr(120 - (int32_t)hw, 120 + (int32_t)hw), k;   /* (0.9.13: the
                                                         * range as wide as COMPLEX: 104 .. 136 at 0, 80 .. 160 at 50) */
    memset(m, 0, sizeof *m);
    m->name = "EXPLORE";
    m->bpm_lo = (uint8_t)lo;
    m->bpm_hi = (uint8_t)(lo + (uint32_t)grr(0, 16));
    for (k = 0; k < NELEM(GX_FIELD); k++) {
        const gxf_t *f = &GX_FIELD[k];
        ((uint8_t *)m)[f->off] = (uint8_t)(gch(f->z) ? 0 : grr(f->lo, f->hi));
    }
    if (m->cbars == 2u && gch(40)) m->cbars = 4;        /* (a chord a bar, two, four) */
    for (k = 0; k < 4u; k++) m->scale[k] = GX_SCALE[gch(gx.w) ? gpickw(GX_SCALE_W, NELEM(GX_SCALE)) : gr() % 4u];   /* (0.9.13:
                                                         * less wild, the commoner modes: major, minor, dorian, mixolydian) */
    m->drum_pct = 100;
    m->sd_micro = (int8_t)grr(-4, 4);
    m->snd[GP_DRUMS] = m->kit;
    for (k = 1; k < GP_N; k++) m->snd[k] = GX_POOL[k][gr() % GX_NPOOL[k]];
    m->tex = GX_TEX[gr() % NELEM(GX_TEX)];
    m->hx = (uint8_t)((gr() % (HF_JAZZ + 1u)) | (gch(50) ? HX_RAISE : 0u) | (gxw(45) ? HX_SEC : 0u) | (gxw(45) ? HX_BOR : 0u) |
                      (gxw(35) ? HX_LIFT : 0u) | (gxw(50) ? HX_APPR : 0u));
}
/* the progression: one to four chords on any degrees (a move every chord), each a major or minor triad of the mode
 * (or coloured: the raised, a secondary dominant, the parallel mode's, the flat side's), any quality but the rootless */
static void gx_prog(void)
{
    static const uint8_t QW[Q_N] = {20, 14, 8, 8, 8, 8, 6, 6, 6, 4, 6, 0}, FL[5] = {0, F_MAJ, F_DOM, F_PAR, F_FLAT}, FW[5] = {58, 10, 12, 10, 10};
    uint32_t n = gch(12) ? 1u : (uint32_t)grr(2, 4), k, tries, d;   /* (0.9.13: one chord now and then, was one in four) */
    gs.pn = 0;
    for (k = 0; k < n; k++)
        for (tries = 0; tries < 12u; tries++) {
            gch_t c = {0, 0, (uint8_t)(gr() % 7u), 0, 0};
            uint32_t f = gch(gx.w) ? FL[gpickw(FW, 5)] : 0u;   /* (0.9.13: less wild, more of the mode's own) */
            if (gs.pn && c.deg == gs.pdeg[gs.pn - 1u] && tries < 10u) continue;
            if (f == F_DOM ? !(gs.hx & HX_SEC) : f == F_MAJ ? !(gs.hx & HX_RAISE) : f && !(gs.hx & HX_BOR)) f = 0;   /* (the
                                                         * colours its harmony took: gx_genre) */
            if (f && (!gcolour(&c, f) || !gtriad_ok(&c))) f = 0;
            if (!f) { c.alt = 0; if (!gtriad_ok(&c)) continue; }
            gs.pdeg[gs.pn] = c.deg;
            gs.pq[gs.pn] = (uint8_t)(gpickw(QW, Q_N) | f);
            gs.pn++;
            break;
        }
    for (d = 0; !gs.pn && d < 7u; d++) {                /* (none came: the mode's first plain triad) */
        gch_t c = {0, 0, (uint8_t)d, 0, 0};
        if (gtriad_ok(&c)) { gs.pdeg[0] = (uint8_t)d; gs.pq[0] = Q_TRI; gs.pn = 1; }
    }
    if (!gs.pn) { gs.pdeg[0] = 0; gs.pq[0] = Q_TRI; gs.pn = 1; }
}
/* a role's engine (the budgets: FM6, ANIMAL, VOICE twice each at most) */
static int8_t gx_engine(uint32_t r)
{
    uint32_t tries, e = EG_A;
    for (tries = 0; tries < 8u; tries++) {
        e = GX_ENG[gpickw(GX_ENG_W[r % RL_N], GX_NENG)];
        uint32_t two = 1u + (gx.w > 60u);                /* (0.9.13: ANIMAL, VOICE once each under COMPLEX 61) */
        if (!(e == ENGI_FM6 && gs.fm6 >= 2u) && !(e == ENGI_ANIMAL && gx.nan >= two) && !(e == EG_VO && gx.nvo >= two)) break;
        e = EG_A;
    }
    gx.nan += e == ENGI_ANIMAL;
    gx.nvo += e == EG_VO;
    return (int8_t)e;
}
/* the sound past its recipe: a VOICE singing words (any language, a dice's phrase or scat), a distortion's type, a
 * wide glide, a vibrato, detune */
static void gx_sound(track_t *t, uint32_t r)
{
    if (t->eng_req == EG_VO && gch(70)) { t->p[P_VLANG] = (int16_t)grr(1, LG_N - 1); t->p[P_VLYR] = (int16_t)grr(1, LYR_N - 1); }
    if (t->p[P_DIST] && gch(50)) t->p[P_DTYPE] = (int16_t)grr(0, 5);
    if ((r == RL_LEAD || r == RL_BASS) && gch(30)) t->p[P_GLIDE] = (int16_t)grr(20, 70);
    if ((r == RL_LEAD || r == RL_PAD) && t->p[P_SUS] >= 60 && !t->p[P_LD_PIT] && gch(10)) {   /* (0.9.13: a soft vibrato,
                                                         * now and then; was a siren a quarter of the time) */
        t->p[P_LWAVE] = 0; t->p[P_LRATE] = (int16_t)grr(84, 92); t->p[P_LFADE] = (int16_t)grr(70, 100); t->p[P_LD_PIT] = 1; t->p[P_LD_FLT] = 0;
    }
    if (gch(25)) t->p[P_DETUNE] = (int16_t)grr(10, 50);
}
/* the step components scattered over the melodic tracks (a bass's never an arpeggio, a jump or a climb) */
static void gx_comps(void)
{
    static const uint8_t EV[7] = {EV_1_2, EV_2_2, EV_1_3, EV_1_4, EV_3_4, EV_FIRST, EV_NOT1};
    uint32_t k;
    for (k = 0; k < GEN_NT; k++) {
        uint32_t r = gs.role[k], len = dlen(&trk[k]), n, j, tries;
        if (!gs.act[r] || r == RL_DRUM || (r == RL_TEX && k != gs.trk_of[RL_TEX]) || !gxw(r == RL_TEX ? 30 : 75)) continue;
        n = (uint32_t)grr(1, 2 + (int32_t)gs.complex / 30);
        for (j = 0, tries = 0; j < n && tries < 24u; tries++) {
            step_t *s = &trk[k].step[gr() % len];
            uint32_t c = GX_COMP[gpickw(GX_COMP_W, NELEM(GX_COMP))], a;
            if (s->time != ST_NOTE || !s->n || s->comp) continue;
            if (r == RL_BASS && (cp_isarp(c) || c == CP_JUMP || c == CP_REV)) c = CP_CLIMB;   /* (0.9.18: OCT) */
            if (c == CP_ARPN && s->n < 2u) c = CP_ARPQ;
            switch (c) {
            case CP_RAND: a = (uint32_t)grr(1, 4); break;
            case CP_BEND: a = (uint32_t)(12 + (gch(50) ? 1 : -1) * grr(1, 12)); break;
            case CP_ECHO: a = gr() % 5u | (gch(25) ? CP_ECHO_EVEN : 0u); break;
            case CP_PULSE: a = gr() % 4u; break;
            case CP_WALK: a = CP_WARG(gr() % CP_NWRATE, gr() % 4u, gr() % 4u); break;
            case CP_CLIMB: a = CP_WARG(r == RL_BASS ? gr() % 4u : gr() % CP_NWRATE, gr() % (r == RL_BASS ? 2u : 4u), gr() % 2u); break;
            case CP_ARPQ: case CP_ARPN: a = CP_ARG(gr() % 4u, ARP_RCODE[1u + gr() % 3u], gr() % 2u); break;   /* (0.9.18:
                                                         * 1/8 1/16 1/32) */
            case CP_TONAL: a = (uint32_t)(7 + grr(-4, 4)); break;
            case CP_JUMP: a = gr() % len | (gr() % 4u) << 6; break;
            default: a = gr() % 4u; break;              /* REV: its TIMES */
            }
            s->comp = (uint8_t)c; s->carg = (uint8_t)a;
            if (gch(35)) ST_EVERY_SET(s, EV[gr() % 7u]);
            j++;
        }
    }
}
/* PLAYHEADS: a head (or two, on two tracks) reading a melodic track at its own rate, way and interval in the scale */
static void gx_heads(void)
{
    static const int8_t IV[10] = {-7, -5, -4, -3, -2, 2, 3, 4, 5, 7};
    uint32_t n = gxw(55) ? (gxw(40) ? 2u : 1u) : 0u, tries;
    for (tries = 0; tries < 16u && n; tries++) {
        uint32_t ti = gr() % GEN_NT, r = gs.role[ti];
        fg_set_t *f = &fg_set[ti][0];
        if (!gs.act[r] || r == RL_DRUM || r == RL_TEX || f->rate || (r == RL_BASS && gch(70))) continue;
        f->rate = (uint8_t)(1u + gr() % (trk[ti].p[P_SDIV] == 3 ? 5u : gch(15) ? 8u : 7u));   /* (1/4 .. x2; x3 now and
                                                         * then; a 1/32 track's never past x1) */
        f->dir = (uint8_t)(gr() % 4u);
        f->ivl = IV[gr() % 10u];
        n--;
    }
}
/* MUTATE on a track or two (SOUND or PATTERN every 2 .. 16 bars, ORBIT mostly: round the piece) */
static void gx_mutate(void)
{
    uint32_t n = gxw(45) ? (gxw(30) ? 2u : 1u) : 0u, tries;
    for (tries = 0; tries < 16u && n; tries++) {
        uint32_t ti = gr() % GEN_NT;
        track_t *t = &trk[ti];
        if (!gs.act[gs.role[ti]] || t->p[P_MSND] || t->p[P_MPAT]) continue;
        if (gs.role[ti] == RL_DRUM || gch(50)) t->p[P_MPAT] = (int16_t)grr(2, 5); else t->p[P_MSND] = (int16_t)grr(2, 5);
        t->p[P_MAMT] = (int16_t)grr(20, 60);
        t->p[P_MMODE] = (int16_t)(gch(75) ? 0 : 1);
        n--;
    }
}

/* (0.9.13) EXPLORE's CPU budget. The piece's load estimated from what it plays, in thousandths of the mean of the
 * CPU TEST song (SAVE > TOOLS 6 CPU TEST, TRACKS 6; README: ~63 % of a half with two cores, ~77 % with one): a base
 * (the sequencer, the master, the effect buses: 295 with two cores, the buses on the second; 380 with one), each part
 * by its engine (GC_ENG) and its voices (GC_VOX a voice, as tests/cpu_vg.c's per-voice costs rank the engines), the
 * reverb by its TYPE and the delay's grain (pitch, spray) as cpu_vg measures them (fx/rev.*, fx/dly.pit against
 * fx/none), each PLAYHEAD. A part's voices: its notes a step (a chord's), times the share of the loop they sound
 * (the steps, their gate, the TIEs, half the release's time as tails at the piece's tempo and DIV), at most the
 * engine's; the drums two at most, by how busy. The numbers fitted over 1200 EXPLORE pieces rendered on the host
 * (one core and two; the error 0.06 of CPU TEST, its 95th percentile 0.10). The budget: 65 % of a half with two cores
 * (the load governor's line, audio.c TIGHT_ON), 55 % with one (TIGHT_ON_1C), less that error. Over it the piece gets
 * lighter, a step at a time, until it fits: the heaviest engine swapped for LOFI or ANALOG, CLOUD / SHIMMER as ROOM /
 * SPRING, the texture left out, then the arp, then the lead (gx_fit_sounds, before the parts); the delay's grain off,
 * a PLAYHEAD less, a note less in every chord (two at least), the long releases shorter, the texture cleared
 * (gx_fit_piece, after them). The genres and RANDOM are left as they were */
static const uint8_t GC_ENG[NENGINES] = {27, 79, 45, 27, 27, 64, 27, 44, 27, 27, 70, 60, 79, 27, 22, 27};   /* SHRUB: ANALOG's, not generated (GX_ENG) */   /* (ANALOG ..
                                                         * ANIMAL by number; the retired ones as ANALOG, DIGITAL as FM6) */
static const uint8_t GC_VOX[NENGINES] = {19, 26, 15, 17, 19, 23, 19, 22, 19, 19, 11, 16, 26, 19, 11, 19};
static const uint8_t GC_REV[5] = {0, 25, 32, 10, 47};   /* ROOM SPRING SHIMMER RESO CLOUD */
static const uint8_t GC_DIVQ[6] = {1, 2, 4, 8, 3, 6};   /* a DIV's steps a beat, x 4 / 4 (1/4 .. 1/32, 8T, 16T) */
#define GC_GRAIN 25u
#define GC_HEAD 6u
/* (0.9.14) the FM-1 against the model: the model is the host's instruction counts, and the device is heavier than they
 * say (its code runs from the flash through a small cache the two cores share: the more engines and effects a piece
 * plays, the more of it misses). Measured: SAVE > TOOLS 6 CPU TEST played (ui_menu.c cpt_res: its load, the host
 * holding it at 63 % with two cores, 77 % with one); not measured, GC_DEV_X100. On 0.9.13 an owner's FM-1 played EXPLORE
 * at ~90 % where the model said ~62 %: x1.45; x1.6 to be safe. Never under x1.0 (0.9.13's budget), x3 at most. To
 * recalibrate from a PROFILE photo: GC_DEV_X100 = the device's WHOLE HALF (AVG, playing) over the host's for the song */
#define GC_DEV_X100 160u
static uint32_t gc_dev(void)
{
    uint32_t d = cpt_res ? (uint32_t)cpt_res * 100u / (cpt_cores == 1u ? 77u : 63u) : GC_DEV_X100;
    return d < 100u ? 100u : d > 300u ? 300u : d;
}
static uint32_t gc_budget(void)                          /* (65 / 63, 55 / 77, over the device; less 0.10; one core 0.12:
                                                         * its pieces sparse, a held chord more of them) */
{
    return mc_two() ? 1030u * 100u / gc_dev() - 100u : 714u * 100u / gc_dev() - 120u;
}
static uint32_t gc_cost(int steps)                      /* steps 0: before the parts (a chord track's triads) */
{
    const ggenre_t *g = gs.g;
    uint32_t k, i, h, bpm = steps ? (uint32_t)song.g[G_BPM] : (g->bpm_lo + g->bpm_hi) / 2u;
    uint32_t c = (mc_two() ? 295u : 380u) + GC_REV[(uint32_t)(steps ? song.g[G_RTYPE] : g->rtype) % 5u];
    if (steps && (song.g[G_DPIT] || song.g[G_DSPRY])) c += GC_GRAIN;
    for (k = 0; k < GEN_NT && k < trk_live; k++) {
        const track_t *t = &trk[k];
        uint32_t r = gs.role[k], e = t->eng_req % NENGINES, len = dlen(t), on = 0, ns = 0, tie = 0, v, f, cap;
        if (!gs.act[r] || (r == RL_TEX && k != gs.trk_of[RL_TEX])) continue;
        for (h = 0; h < FG_HEADS; h++) c += fg_set[k][h].rate ? GC_HEAD : 0u;
        for (i = 0; steps && i < len; i++) {
            const step_t *s = &t->step[i];
            if (s->time == ST_NOTE && (s->n || s->hit)) { on++; ns += s->n; }
            tie += s->time == ST_TIE;
        }
        if (r == RL_DRUM) v = steps ? (on * 300u / (len ? len : 1u) < 200u ? on * 300u / (len ? len : 1u) : 200u) : 200u;
        else if (!steps) v = r == RL_PAD ? 300u : 100u;
        else if (!on) v = 0;
        else {
            uint32_t g100 = t->p[P_SGATE] < 100 ? (uint32_t)t->p[P_SGATE] : 100u;
            uint32_t tail = 5u * TIME_MS_X10[e == ENGI_FM6 ? 112u : t->p[P_REL] & 127] / 600u * bpm *   /* (0.9.14: FM6's own
                                                         * release, its patch's R4: ~2..4 s) */
                            GC_DIVQ[(uint32_t)t->p[P_SDIV] % 6u] / 100u;
            f = (on * (g100 + tail) + tie * 100u) / len;
            cap = t->p[P_VOICE] == V_MONO || t->p[P_VOICE] == V_LEGATO ? 100u : e == ENGI_FM6 || e == EG_VO ? 400u : 800u;
            v = cap == 100u ? f : ns * f / on;
            if (v > cap) v = cap;
        }
        c += GC_ENG[e] + GC_VOX[e] * v / 100u;
    }
    return c;
}
static uint32_t gsnd(uint32_t r)                        /* a role's recipe */
{
    const ggenre_t *g = gs.g;
    return r == RL_BASS ? g->snd[GP_BASS] : r == RL_PAD ? g->snd[GP_CHORDS] : r == RL_LEAD ? g->snd[GP_LEAD] :
           r == RL_ARP ? g->snd[GP_ARP] : g->tex;
}
static void gx_fit_sounds(void)
{
    uint32_t n, k;
    for (n = 0; n < GEN_NT && gc_cost(0) > gc_budget(); n++) {
        uint32_t best = GEN_NT, r, e;
        for (k = 0; k < GEN_NT && k < trk_live; k++)
            if (gs.role[k] != RL_DRUM && gs.act[gs.role[k]] && GC_ENG[trk[k].eng_req % NENGINES] > GC_ENG[EG_A] &&
                (best == GEN_NT || GC_ENG[trk[k].eng_req % NENGINES] > GC_ENG[trk[best].eng_req % NENGINES])) best = k;
        if (best == GEN_NT) break;
        r = gs.role[best]; e = trk[best].eng_req;
        gs.fm6 -= e == ENGI_FM6; gx.nan -= e == ENGI_ANIMAL; gx.nvo -= e == EG_VO;
        gx.eng = (int8_t)(gch(50) ? EG_LO : EG_A);
        gen_sound(&trk[best], r, gsnd(r));
        gx.eng = -1;
        gx_sound(&trk[best], r);
    }
    if (gc_cost(0) > gc_budget() && (gmut.rtype == 4u || gmut.rtype == 2u)) gmut.rtype = gmut.rtype == 4u ? 0u : 1u;
    if (gc_cost(0) > gc_budget()) gs.act[RL_TEX] = 0;
    if (gc_cost(0) > gc_budget() && gs.act[RL_ARP] && gs.act[RL_BASS] + gs.act[RL_PAD] + gs.act[RL_LEAD] >= 2u) gs.act[RL_ARP] = 0;
    if (gc_cost(0) > gc_budget() && gs.act[RL_BASS] && gs.act[RL_PAD]) gs.act[RL_LEAD] = 0;   /* (one core: the drums,
                                                         * the bass and the chords) */
}
static void gx_fit_piece(void)
{
    uint32_t k, h, i;
    if (gc_cost(1) <= gc_budget()) return;
    song.g[G_DPIT] = song.g[G_DSPRY] = 0;
    for (h = FG_HEADS; h--;)
        for (k = 0; k < GEN_NT; k++)
            if (fg_set[k][h].rate && gc_cost(1) > gc_budget()) fg_set[k][h].rate = 0;
    while (gc_cost(1) > gc_budget() && gx.nmax > 2u) {
        gx.nmax--;
        for (k = 0; k < GEN_NT; k++) {
            track_t *t = &trk[k];
            load_begin(t, LOAD_PAT);
            for (i = 0; i < NSTEP; i++) if (t->step[i].n > gx.nmax) { t->step[i].note[gx.nmax] = 0; t->step[i].n = gx.nmax; }
            load_end(t);
            pat_sig[k] = steps_sig(t);
        }
    }
    for (k = 0; k < GEN_NT; k++)
        if (gc_cost(1) > gc_budget() && gs.role[k] != RL_DRUM && trk[k].p[P_REL] >= 90) trk[k].p[P_REL] = 84;
    k = gs.trk_of[RL_TEX];
    if (gc_cost(1) > gc_budget() && k < GEN_NT && gs.act[RL_TEX]) {
        gs.act[RL_TEX] = 0;
        load_begin(&trk[k], LOAD_PAT);
        for (i = 0; i < NSTEP; i++) gclear(&trk[k].step[i]);
        load_end(&trk[k]);
        pat_sig[k] = steps_sig(&trk[k]);
    }
}

/* --------------------------------------------------------------------------------------------- the piece --- */
static void gen_post(void)
{
    uint32_t dr = gs.trk_of[RL_DRUM], bs = gs.trk_of[RL_BASS], i, k;
    if (dr < GEN_NT && bs < GEN_NT && gs.act[RL_DRUM] && gs.act[RL_BASS] && gs.g->kick_mode == KM_SUSTAIN) {
        track_t *b = &trk[bs];                          /* the bass under the kick: softer there */
        for (i = 0; i < dlen(b); i++)
            if (GKICK(i) && dstarts(b, i) && i % 16u) b->step[i].vel = (uint8_t)clamp((int32_t)b->step[i].vel - 18, 40, 127);
    }
    /* (0.8.3: the lead's clashes with the bass are avoided as it is written, against the chord's root: so turning
     * BASS leaves the lead as it was) */
    for (k = 0; k < GEN_NT; k++) {                        /* the breath: each fourth bar's last two steps softer */
        track_t *t = &trk[k];
        uint32_t len = dlen(t);
        if (len < 64u || k == dr) continue;
        for (i = 0; i < len; i++)
            if ((i / 16u) % 4u == 3u && i % 16u >= 14u && t->step[i].time == ST_NOTE)
                t->step[i].vel = (uint8_t)clamp((int32_t)t->step[i].vel * 60 / 100, 30, 127);
    }
    if (gs.complex > 30u) {                             /* step components (0.6.97: more of them, some every other loop):
                                                         * the lead echoed, an octave up or bent, the arp jumping its
                                                         * octave, ratcheted or pulsed, a chord arpeggiated or echoed */
        for (k = 0; k < GEN_NT; k++) {
            uint32_t r = gs.role[k], tries, want = 1u + (gs.complex > 60u) + (gs.complex > 85u), got = 0;
            if (!gs.act[r] || r == RL_DRUM || r == RL_TEX || r == RL_BASS || !gch(40 + gs.complex / 3u)) continue;
            for (tries = 0; tries < 12u && got < want; tries++) {
                uint32_t at = 1u + gr() % (dlen(&trk[k]) - 1u > 0u ? dlen(&trk[k]) - 1u : 1u);
                step_t *s = &trk[k].step[at];
                if (s->time != ST_NOTE || !s->n || s->comp) continue;
                if (r == RL_PAD) {
                    if (s->n < 2u) continue;
                    if (gch(60)) { s->comp = CP_ARPN; s->carg = CP_ARG(gr() % 3u, gch(50) ? 5u : 6u, 0u); }   /* (0.9.18: 1/16, 1/8) */
                    else { s->comp = CP_ECHO; s->carg = (uint8_t)(gr() % 2u); }
                } else if (r == RL_LEAD) {              /* (0.8.3: never a random walk or an octave thrown into the
                                                         * line: its notes are composed) */
                    uint32_t c = gr() % 3u;
                    if (s->time != ST_NOTE || !gin_mode(s->note[0])) continue;
                    s->comp = (uint8_t)(c == 0u ? CP_ECHO : c == 1u ? CP_BEND : CP_PULSE);
                    s->carg = (uint8_t)(s->comp == CP_ECHO ? gr() % 2u : s->comp == CP_BEND ? (gch(50) ? 2u : 10u) : 1u);
                } else {                                /* (the arp: its chord's tones; 0.8.3: no RAND, no WALK) */
                    uint32_t c = gr() % 3u;
                    s->comp = (uint8_t)(c == 1u ? CP_ECHO : CP_CLIMB);   /* (0.9.18: OCT is CLIMB, a lap at a time) */
                    s->carg = (uint8_t)(c == 0u ? CP_WARG(0u, 0u, 0u) : c == 2u ? CP_WARG(2u + gr() % 6u, gch(50) ? 2u : 0u, gr() % 2u) : 0u);
                    if (c == 1u && gs.energy > 60u && gch(50)) { s->comp = 0; step_set_ratchet(s, 2); }
                }
                if (s->comp && gch(40)) ST_EVERY_SET(s, gch(50) ? EV_1_2 : EV_2_2);   /* (every other loop) */
                got++;
            }
        }
    }
    if (gx.on) gx_comps();                              /* (0.8.6) EXPLORE: wilder ones, on the bass too */
}
/* (0.8.3) the piece's key on the SCALE page, every track (the keys, CHORD, ALL TR and the RESO reverb with it), QUANT
 * SNAP unless one is chosen (the keys play in the key); the CHORD page on the chord track as its chords are (TRIAD, 7TH,
 * ADD9, SUS2, SUS4: the plan's commonest), VOICING LEAD, a strum where a guitar plays them; off on the other tracks */
static uint32_t gchord_page(void)
{
    static const uint8_t CD[5] = {CD_TRI, CD_SEV, CD_ADD9, CD_SUS2, CD_SUS4};
    uint8_t n[5] = {0};
    uint32_t k, best = 0;
    for (k = 0; k < gs.nch; k++) {
        uint32_t q = gs.ch[k].q;
        n[q == Q_7 || q == Q_9 || q == Q_11 || q == Q_6 || q == Q_R9 ? 1 : q == Q_ADD9 ? 2 : q == Q_SUS2 ? 3 :
          q == Q_SUS4 || q == Q_QUART || q == Q_7NO5 ? 4 : 0]++;
    }
    for (k = 1; k < 5u; k++) if (n[k] > n[best]) best = k;
    return CD[best];
}
static void gen_scale_page(void)
{
    uint32_t k, pd = gs.trk_of[RL_PAD];
    int16_t qn = QN_OFF;
    for (k = 0; k < GEN_NT; k++) if (!drum_track(&trk[k])) { qn = trk[k].p[P_QUANT]; break; }
    if (qn == QN_OFF) qn = QN_SNAP;
    for (k = 0; k < GEN_NT; k++) {
        track_t *t = &trk[k];
        t->p[P_ROOT] = gs.root; t->p[P_SCALE] = gs.scale; t->p[P_QUANT] = qn;
        t->p[P_CHRD] = 0; t->p[P_CVOI] = CV_CLOSE; t->p[P_CSTR] = 0;
        if (k == pd && gs.act[RL_PAD]) {
            uint32_t ri = gs.g->snd[GP_CHORDS];
            t->p[P_CHRD] = (int16_t)gchord_page();
            t->p[P_CVOI] = CV_LEAD;
            t->p[P_CSTR] = (int16_t)(ri == SR_GTR ? 24 : ri == SR_EP && HXF(gs.hx) == HF_JAZZ ? 6 : 0);
        }
    }
    sl_root = gs.root; sl_scale = gs.scale; sl_quant = qn;
}
static int8_t dice_force_style = -1, dice_force_energy = -1;   /* (a film's script, the renders: this genre, this energy) */
/* (0.9.17) the piece's choices after its genre (gen_make's, from the GS_PICK stream: the same calls in the same order):
 * the key, the energy and the amounts, the progression, the loop, the plan, the bass's archetype. SEQ + SELECT's too
 * (dice_seq: a part where no piece of the generator plays) */
static void gen_pick(void)
{
    const ggenre_t *g = gs.g;
    uint32_t k, pr, steps;
    gs.root = (int16_t)grr(0, 11);
    {   /* (0.8.3) the key: always a mode of seven, so the CHORD page, the chords, the arp and the lead stand on it; a
         * genre's pentatonic or blues is its lead's palette over the mode that holds it (dorian where the genre is
         * jazzy, aeolian else; major for the major pentatonic) */
        uint32_t req = g->scale[gr() % 4u], m, d, c, cnt = 0;
        gs.hx = g->hx; gs.pal = 0x7Fu;
        if (req == SC_MPEN || req == SC_BLUE) { gs.pal = 0x5Du; req = HXF(g->hx) == HF_JAZZY ? SC_DOR : SC_MIN; }
        else if (req == SC_PEN) { gs.pal = 0x37u; req = SC_MAJ; }
        if (req < sizeof SCALE_MASK / sizeof SCALE_MASK[0]) for (c = 0; c < 12u; c++) cnt += (SCALE_MASK[req] >> c) & 1u;
        if (cnt != 7u || (req == 10u && !gx.on)) req = SC_MIN;   /* (LOC, the symmetric scales: never a key of the
                                                         * generator; 0.8.6: LOC EXPLORE's) */
        gs.scale = (int16_t)req;
        m = SCALE_MASK[req];
        for (d = 0, c = 0; c < 12u && d < 7u; c++) if ((m >> c) & 1u) gs.sc[d++] = (uint8_t)c;
        if (gx.on) gs.pal = GX_PAL[gr() % NELEM(GX_PAL)];
    }
    gs.energy = dice_force_energy >= 0 ? (uint32_t)dice_force_energy * 50u : gp.energy;
    gs.complex = gp.complex;
    if (gs.mutant) gs.complex = gs.complex + 20u > 100u ? 100u : gs.complex + 20u;   /* (a mutant: more going on) */
    if (gx.on) gs.complex = (uint32_t)clamp((int32_t)gs.complex + grr(10, 40) * (int32_t)gx.w / 100, 0, 100);   /* (EXPLORE:
                                                         * more still; 0.9.13: as much more as COMPLEX) */
    gs.human = (uint32_t)gp.human * (g->human + 30u) / 100u;
    gs.swing = gp.swing == GA_AUTO ? g->swing : gp.swing;
    for (k = 0; k < GP_N; k++) {
        uint32_t a = gp.amt[k] == GA_AUTO ? g->amt[k] : gp.amt[k];
        if (gp.amt[k] == GA_AUTO && a) a = (uint32_t)clamp((int32_t)a + ((int32_t)gs.energy - 50) / 2, 10, 100);
        gs.amt[k] = a;
    }
    pr = gr() % 3u;
    gs.vm = g->voicing;
    if (gs.complex > 70u && gs.vm == VM_PLAIN && gch(40)) gs.vm = VM_RICH;
    if (gx.on) gx_prog();                               /* (0.8.6) EXPLORE: no genre's progression */
    else {   /* (0.8.3) the progression: the genre's one in three, its next one where this mode would make a chord of it
         * diminished or borrow where the genre does not (gprog_ok), else the mode's own two-chord loop */
        const gprog_t *p = 0;
        uint32_t t;
        for (t = 0; t < 3u && !p; t++) if (gprog_ok(&PROG[g->prog[(pr + t) % 3u]])) p = &PROG[g->prog[(pr + t) % 3u]];
        if (p && p->n < 2u && gch(85)) {                /* (0.9.13) a progression far more often than a pedal: the genre's
                                                         * next one of two chords or more, else the mode's own loop below */
            const gprog_t *q = 0;
            for (t = 1; t < 3u && !q; t++) if (PROG[g->prog[(pr + t) % 3u]].n >= 2u && gprog_ok(&PROG[g->prog[(pr + t) % 3u]])) q = &PROG[g->prog[(pr + t) % 3u]];
            p = q;
        }
        if (!p) {
            p = &PROG[gminor() ? (gs.sc[1] == 1u ? PR_PHRY : gs.sc[6] == 11u ? PR_I_IV : PR_I_VII) : gs.sc[3] == 6u ? PR_I_II : PR_I_IV];
            if (!gprog_ok(p)) p = &PROG[PR_ONE];
        }
        gs.pn = p->n;
        for (k = 0; k < gs.pn; k++) {                   /* the qualities as the voicing mode wants them (a coloured chord
                                                         * keeps its own) */
            uint32_t qq = QQ(p->q[k]), f = QF(p->q[k]);
            switch (f ? (gs.vm == VM_POWER ? VM_POWER : VM_PAD) : gs.vm) {
            case VM_PLAIN: qq = Q_TRI; break;
            case VM_RICH: if (qq == Q_TRI) qq = gch(50) ? Q_7 : gch(50) ? Q_ADD9 : Q_9; break;
            case VM_SUS: qq = gch(50) ? Q_SUS4 : gch(50) ? Q_SUS2 : Q_7NO5; break;
            case VM_QUART: qq = gch(70) ? Q_QUART : Q_SUS4; break;
            case VM_POWER: qq = Q_POWER; break;
            default: break;                             /* PAD, PLANE: as written */
            }
            gs.pdeg[k] = (uint8_t)p->deg[k];
            gs.pq[k] = (uint8_t)(qq | f);
        }
    }
    gs.cst = 16u * g->cbars;
    if (gx.on) {                                        /* (0.8.6) EXPLORE: a chord 8 .. 48 steps, odd metres */
        static const uint8_t CST[8] = {8, 12, 16, 16, 20, 24, 32, 48};
        gs.cst = CST[gch(gx.w) ? gr() % 8u : gch(70) ? 2u : 6u];   /* (0.9.13: less wild, a bar or two) */
    }
    if (gs.cst * gs.pn > NSTEP) gs.cst = NSTEP / gs.pn;
    steps = gs.cst * gs.pn;
    if (gx.on) {                                        /* (the loop 8 .. 64: the chords', or beats of its own) */
        if (gxw(35)) steps = (uint32_t)grr(2, 16) * 4u;
        if (steps < 8u) steps = 8u;
    } else {
        if (steps < 16u) steps = 16u;
        if (gs.complex > 50u && steps < 64u && gch(50)) steps = 64;   /* (four bars: fills, variations, a fourth-bar
                                                         * pickup) */
    }
    gs.len = steps;
    gplan(0);
    gs.ba = g->bass[gr() % 3u];
    if (gs.amt[GP_BASS] > 70u && gch(30)) gs.ba = g->bass[0];
}
static void gen_make(void)
{
    const ggenre_t *g;
    uint32_t k, dr = GEN_NT, nlive = trk_live < GEN_NT ? trk_live : GEN_NT, nmel, drums, next, wet;
    /* the choices from the seed: the genre (RANDOM), the root, the scale, the progression, the bass */
    gseed(GS_PICK);
    gx.on = (uint8_t)(dice_force_style < 0 && gp.genre == GEN_EXPLORE);
    gx.nan = gx.nvo = gx.np = 0; gx.eng = -1; gx.w = gp.complex; gx.nmax = 4;
    if (!gx.on && gx.dirty) {                           /* (0.8.6) after an EXPLORE piece: DIV and PLAYHEADS as a genre's
                                                         * piece has them (1/16, none) */
        for (k = 0; k < GEN_NT; k++) trk[k].p[P_SDIV] = 2;
        memset(fg_set, 0, sizeof fg_set);
        gx.dirty = 0;
    }
    if (gx.on) {                                        /* (0.8.6) EXPLORE: a genre of its own a seed */
        gs.gi = NGENRE;
        gs.mutant = 0;
        gx_genre(&gmut);
        gx.dirty = 1;
        for (k = 0; k < GEN_NT; k++) trk[k].p[P_SDIV] = 2;
        memset(fg_set, 0, sizeof fg_set);
        g = gs.g = &gmut;
    } else {
        gs.gi = dice_force_style >= 0 ? (uint32_t)dice_force_style % NGENRE : gp.genre ? (gp.genre - 1u) % NGENRE : gr() % NGENRE;
        g = gs.g = &GENRE[gs.gi];
        gmut = *g;
        gs.mutant = 0;
        if (!gp.genre && dice_force_style < 0 && gch(45)) {   /* (0.6.95) RANDOM: a mutant, the drums of one genre, the bass
                                                             * of another, the rest from either; a name of its own: "EBM+JAZZ" */
            const ggenre_t *b = &GENRE[gr() % NGENRE], *c = &GENRE[gr() % NGENRE];
            uint32_t k2, n2 = 0;
            gmut.drum = b->drum; gmut.kit = b->kit; gmut.ghosts = b->ghosts; gmut.hats = b->hats; gmut.fills = b->fills;
            gmut.sd_micro = b->sd_micro; gmut.snd[GP_DRUMS] = b->snd[GP_DRUMS];
            gmut.drum_pct = (uint8_t)(b->drum_pct > g->drum_pct ? b->drum_pct : g->drum_pct);
            for (k2 = 0; k2 < 3u; k2++) gmut.bass[k2] = c->bass[k2];
            gmut.bass_lo = c->bass_lo; gmut.kick_mode = c->kick_mode; gmut.bass_gate = c->bass_gate; gmut.snd[GP_BASS] = c->snd[GP_BASS];
            if (gch(50)) {
                for (k2 = 0; k2 < 3u; k2++) gmut.prog[k2] = b->prog[k2];
                gmut.cbars = b->cbars; gmut.pads = b->pads; gmut.voicing = b->voicing; gmut.snd[GP_CHORDS] = b->snd[GP_CHORDS];
                gmut.hx = b->hx;                            /* (0.8.3: the harmony with its progressions) */
            }
            if (gch(50)) { gmut.arp[0] = c->arp[0]; gmut.arp[1] = c->arp[1]; gmut.snd[GP_ARP] = c->snd[GP_ARP]; }
            if (gch(50)) { gmut.lead = b->lead; gmut.snd[GP_LEAD] = b->snd[GP_LEAD]; }
            if (gch(50)) { gmut.rtype = b->rtype; gmut.dtime = b->dtime; gmut.wet = b->wet; gmut.dtype = b->dtype; }
            gmut.scale[1] = b->scale[0]; gmut.scale[3] = c->scale[0];
            gmut.tex = c->tex;
            gmut.bpm_lo = (uint8_t)(((uint32_t)g->bpm_lo + b->bpm_lo) / 2u);
            gmut.bpm_hi = (uint8_t)(((uint32_t)g->bpm_hi + b->bpm_hi) / 2u);
            if (gmut.bpm_hi < gmut.bpm_lo + 4u) gmut.bpm_hi = (uint8_t)(gmut.bpm_lo + 4u);
            gmut.swing = (uint8_t)(((uint32_t)g->swing + b->swing) / 2u);
            for (k2 = 0; g->name[k2] && g->name[k2] != ' ' && k2 < 7u; k2++) gmut_name[n2++] = g->name[k2];   /* (the first words) */
            gmut_name[n2++] = '+';
            for (k2 = 0; b->name[k2] && b->name[k2] != ' ' && k2 < 6u; k2++) gmut_name[n2++] = b->name[k2];
            gmut_name[n2] = 0;
            gmut.name = gmut_name;
            gs.mutant = 1;
        }
        /* (0.6.95) a throw its own: the drums from the map's family, the chords' play and a role's recipe from the neighbours */
        if (gmut.drum < DM_NONE && gmut.drum != DM_IDM && gch(35)) gmut.drum = DM_ALT[gmut.drum][gr() % 2u];
        if (gmut.pads <= CS_TRANCE && gch(35)) gmut.pads = CS_ALT[gmut.pads][gr() % 2u];
        for (k = 1; k < GP_N; k++) if (gmut.snd[k] < SR_N && gch(30)) gmut.snd[k] = SR_ALT[gmut.snd[k]][gr() % 2u];
        g = gs.g = &gmut;
    }
    gen_pick();
    gs.fm6 = 0;
    /* the orchestration: the drums (unless the genre or the knob leaves them out), then the melodic roles with an
     * amount, the most important first; the drums on the DRUM track (none: the fourth, or the last live) */
    {
        static const uint8_t PRI[GP_N] = {RL_DRUM, RL_BASS, RL_PAD, RL_ARP, RL_LEAD};
        uint8_t pri[5];
        uint32_t np = 0;
        drums = gs.amt[GP_DRUMS] && g->drum != DM_NONE && (gp.amt[GP_DRUMS] != GA_AUTO || gch(g->drum_pct));
        for (k = 0; k < RL_N; k++) gs.act[k] = 0;
        gs.act[RL_DRUM] = (uint8_t)drums;
        for (k = 1; k < GP_N; k++) if (gs.amt[k]) { pri[np++] = PRI[k]; gs.act[PRI[k]] = 1; }
        nmel = np;
        if (nmel < 2u) {                                /* (never fewer than two voices: the bass, the chords) */
            if (!gs.act[RL_BASS]) { gs.act[RL_BASS] = 1; gs.amt[GP_BASS] = 60; pri[np++] = RL_BASS; }
            if (np < 2u && !gs.act[RL_PAD]) { gs.act[RL_PAD] = 1; gs.amt[GP_CHORDS] = 60; pri[np++] = RL_PAD; }
        }
        for (k = 0; k < GP_N - 1u; k++) if (!gs.act[PRI[k + 1u]]) pri[np++] = PRI[k + 1u];   /* (the silent ones after) */
        for (k = 0; k < GEN_NT && dr == GEN_NT; k++) if (drum_track(&trk[k])) dr = k;
        if (dr == GEN_NT) { dr = nlive > 3u ? 3u : nlive - 1u; set_engine_of(&trk[dr], ENGI_DRUM); }
        for (k = 0; k < RL_N; k++) gs.trk_of[k] = GEN_NT;
        next = 0;
        for (k = 0; k < GEN_NT; k++) {
            if (k == dr) { gs.role[k] = RL_DRUM; gs.trk_of[RL_DRUM] = (uint8_t)k; continue; }
            gs.role[k] = next < 4u ? pri[next] : RL_TEX;
            if (gs.trk_of[gs.role[k]] == GEN_NT) gs.trk_of[gs.role[k]] = (uint8_t)k;
            next++;
        }
        gs.act[RL_TEX] = (uint8_t)(nlive > 5u && gs.trk_of[RL_TEX] < nlive && gs.amt[GP_CHORDS] > 0u);
    }
    gen_scale_page();
    wet = (uint32_t)clamp((int32_t)g->wet + (gs.energy < 34 ? 20 : gs.energy > 66 ? -15 : 0), 0, 100);
    /* the sounds (their own stream), then the parts in the pipeline's order: drums, bass (reads the kick), chords,
     * arp (reads the voicing), lead (reads the bass) */
    gseed(GS_SOUND);
    gs.nsnd = 0;
    for (k = 0; k < GEN_NT; k++) {
        track_t *t = &trk[k];
        uint32_t r = gs.role[k];
        if (r == RL_DRUM) gen_kit(t);
        else {
            if (gx.on) gx.eng = gx_engine(r);           /* (0.8.6) EXPLORE: any engine */
            gen_sound(t, r, gsnd(r));
            gx.eng = -1;
            if (gx.on) gx_sound(t, r);
        }
    }
    if (gx.on) gx_fit_sounds();                         /* (0.9.13) EXPLORE: inside the CPU budget */
    gen_scale_page();
    gseed(GS_MIX);
    for (k = 0; k < GEN_NT; k++) gen_mix(&trk[k], gs.role[k]);
    for (k = 0; k < GEN_NT && gx.on; k++) {               /* (0.8.6) EXPLORE: wider, a track's own swing */
        uint32_t r = gs.role[k];
        if (r != RL_DRUM && r != RL_BASS) trk[k].p[P_PAN] = (int16_t)grr(-45, 45);
        if (gch(25)) trk[k].p[P_SSWING] = (int16_t)grr(10, 60);
    }
    {
        static const uint8_t ORDER[5] = {RL_DRUM, RL_BASS, RL_PAD, RL_ARP, RL_LEAD};
        uint32_t o;
        for (o = 0; o < 6u; o++) {
            uint32_t r = o < 5u ? ORDER[o] : RL_TEX;
            for (k = 0; k < GEN_NT; k++) {
                track_t *t = &trk[k];
                uint32_t len = gs.len;
                if (gs.role[k] != r) continue;
                if (gx.on) {                            /* (0.8.6) EXPLORE: a track's own loop and DIV (polyrhythm;
                                                         * 0.9.13: as often as COMPLEX says, on 1 + COMPLEX / 20 tracks
                                                         * at most) */
                    uint32_t own = gxw(100u - GX_KEEP[r]), dv = gpickw(GX_DIV_W[r], 6);
                    if (!gch(gx.w)) dv = 2;
                    if (gx.np >= 1u + gx.w / 20u) { own = 0; dv = 2; }
                    gx.np += own || dv != 2u;
                    if (own) len = GX_LEN[gr() % NELEM(GX_LEN)];
                    t->p[P_SDIV] = (int16_t)dv;
                } else {
                    if (r == RL_TEX && gs.complex > 60u && gch(40)) len = (uint32_t)grr(5, 7) * 2u + (gch(50) ? 1u : 0u);
                    if (gs.mutant && (r == RL_ARP || r == RL_LEAD) && gch(50)) {   /* (0.6.95) a mutant: a part in
                                                         * another metre */
                        static const uint8_t ODD[4] = {12, 20, 24, 28};
                        len = ODD[gr() % 4u];
                    } else if (r == RL_LEAD && len < NSTEP && NSTEP % len == 0u && gch(80)) {
                        len = NSTEP;                    /* (0.9.17, "longer melodies, call and response") the lead's
                                                         * phrase over four bars where the loop is shorter (its chords
                                                         * again under it): its template's call and answer, its cadences */
                    }
                }
                load_begin(t, LOAD_PAT);
                track_defaults_steps(t);
                { uint32_t j; for (j = 0; j < NSTEP; j++) gclear(&t->step[j]); }   /* (no note left from before) */
                motion_clear(t);
                gen_role(t, k, len);
                t->p[P_SLEN] = (int16_t)len;
                if (r == RL_BASS) t->p[P_SGATE] = (int16_t)clamp(g->bass_gate, 1, 127);
                else if (r == RL_PAD) t->p[P_SGATE] = (int16_t)(g->pads == CS_STAB || g->pads == CS_DUB || g->pads == CS_SKANK || g->pads == CS_BROKEN ? grr(25, 45) :
                                                              g->pads == CS_TRANCE ? 35 : g->pads == CS_COMP ? 60 : 120);
                else if (r == RL_LEAD) t->p[P_SGATE] = (int16_t)(g->lead == LD_MECH ? 50 : 85);
                else if (r != RL_ARP) t->p[P_SGATE] = 120;
                if (r != RL_DRUM) dlevel(t, len);
                load_end(t);
                pat_sig[k] = steps_sig(t);
            }
        }
    }
    gseed(GS_FORM);
    gen_post();
    if (dr < GEN_NT && gs.act[RL_DRUM] && gch(45 + gs.complex / 4u)) {   /* (0.6.97) MORPH moving over the loop: the kit's
                                                         * character drifting a little and back (the drum track's
                                                         * automation) */
        track_t *t = &trk[dr];
        uint32_t len = dlen(t), j, n = len >= 32u ? 4u : 2u;
        int32_t base = t->p[P_E0], sw = grr(14, 34) * (gch(50) ? 1 : -1);
        for (j = 0; j < n; j++)
            motion_set_event(t, j * len / n, P_E0, (int16_t)clamp(base + (j == 0u ? 0 : j == n / 2u ? sw : sw / 2), 0, 127));
    }
    for (k = 0; k < GEN_NT; k++) pat_sig[k] = steps_sig(&trk[k]);
    /* the tempo (the genre's range, the energy in it; always moved), the swing, the space, the master */
    if (!(ui_prefs & PREF_BPM_LOCK)) {                  /* the genre's range, the energy's place in it (from the seed:
                                                         * the same piece, the same tempo) */
        int32_t lo = g->bpm_lo, hi = g->bpm_hi, v = lo + (hi - lo) * (int32_t)gs.energy / 100 + grr(-3, 3);
        song.g[G_BPM] = (int16_t)clamp(v, lo, hi);
    }
    song.g[G_SWING] = (int16_t)clamp((int32_t)gs.swing, 0, 100);
    song.g[G_DTIME] = g->dtime;
    song.g[G_DFDBK] = (int16_t)clamp(wet >= 70u ? grr(58, 80) : grr(30, 55) + (int32_t)wet / 3, 0, 95);   /* (0.6.97: the wet
                                                         * genres' long tails: dub, dub techno, shoegaze, ambient ..) */
    song.g[G_DMIX] = (int16_t)grr(70, 110);
    song.g[G_DCOLOR] = (int16_t)(g->dtype & 1u ? grr(50, 70) : grr(30, 52));
    song.g[G_DTYPE] = (int16_t)(g->dtype & 1u);
    song.g[G_DPING] = (int16_t)((g->dtype >> 1) & 1u);
    song.g[G_DMOD] = (int16_t)(wet >= 70u ? grr(20, 50) : g->human >= 40u && gch(50) ? grr(10, 36) : 0);   /* (0.6.97: the
                                                         * tape's wobble on the long tails) */
    song.g[G_DPIT] = 0; song.g[G_DSPRY] = 0;
    song.g[G_RTYPE] = g->rtype;
    song.g[G_RSIZE] = (int16_t)clamp(grr(40, 80) + (int32_t)wet / 2, 0, 127);
    song.g[G_RRATE] = (int16_t)grr(10, 60);
    song.g[G_RWIDE] = (int16_t)grr(40, 100);
    song.g[G_RDAMP] = (int16_t)grr(55, 100);
    song.g[G_RFILT] = (int16_t)grr(-40, -8);
    song.g[G_RMOD] = (int16_t)grr(0, 60);
    song.g[G_RPRE] = (int16_t)(g->rtype == 3 ? grr(0, 4) : g->rtype == 2 || g->rtype == 4 ? grr(-12, 0) : grr(0, 30));
    song.g[G_CRATE] = (int16_t)grr(10, 60);
    song.g[G_CDEPTH] = (int16_t)grr(20, 80);
    song.g[G_STRN] = 0;
    song.g[G_SLPQ] = 1;
    song.g[G_DUCK] = (int16_t)(gs.act[RL_DRUM] ? (g->kick_mode == KM_SUSTAIN ? grr(30, 50) : grr(14, 30)) : 0);
    song.g[G_DREL] = (int16_t)(g->drum == DM_TECHNO || g->drum == DM_HARD ? grr(15, 30) : grr(25, 45));
    song.g[G_CLIP] = (int16_t)(gs.act[RL_DRUM] ? grr(25, 55) : grr(10, 30));
    song.g[G_PUNCH] = (int16_t)(gs.act[RL_DRUM] ? grr(45, 85) : 0);
    if (gx.on) {                                        /* (0.8.6) EXPLORE: the grain delay's pitch and spray, a ping-pong,
                                                         * the feedback short of self-oscillation */
        static const int8_t DP[6] = {-12, -7, -5, 5, 7, 12};
        song.g[G_DPIT] = (int16_t)(gxw(30) ? DP[gr() % 6u] : 0);
        song.g[G_DSPRY] = (int16_t)(gxw(25) ? grr(10, 80) : 0);
        song.g[G_DPING] = (int16_t)(gr() % 2u);
        song.g[G_DFDBK] = (int16_t)clamp(song.g[G_DFDBK], 0, 85);
    }
    {   /* the macros' routes from the piece's stream (the same piece, the same routes), then the session's random
         * numbers as they were with these in them (0.9.17: they were the piece's from here on: every dice after a GEN
         * piece, MUTATE, CHANCE the seed's, the same after the same seed in every session) */
        uint32_t keep = rng_state;
        rng_state = gq | 1u;
        macro_dice(3);
        rng_state ^= keep * 2654435761u;
        if (!rng_state) rng_state = 0x1234567u;
    }
    for (k = 0; k < GEN_NT; k++) trk[k].p[P_MSND] = trk[k].p[P_MPAT] = 0;   /* (0.6.96) a new piece: MUTATE off on every
                                                         * track, its anchor the piece (a reroll kept the lanes and
                                                         * orbited the old piece's sounds) */
    if (gx.on) { gx_mutate(); gx_heads(); gx_fit_piece(); }   /* (0.8.6) EXPLORE: MUTATE, PLAYHEADS (before the sections:
                                                         * each holds them); 0.9.13: inside the CPU budget */
    mut_keep();
    gen_parts();
}
/* the piece made from the settings (UNDO brings back what was there); the message */
static void gen_go(void)
{
    uint32_t k;
    char b[8];
    undo_mark();
    gen_make();
    str_cpy(gp.last, gs.g->name, sizeof gp.last);
    k = str_len(gp.last);
    b[0] = ' '; b[1] = (char)('0' + gp.seed / 1000u % 10u); b[2] = (char)('0' + gp.seed / 100u % 10u);
    b[3] = (char)('0' + gp.seed / 10u % 10u); b[4] = (char)('0' + gp.seed % 10u); b[5] = 0;
    str_cpy(gp.last + k, b, sizeof gp.last - k);
    ui_say("GEN: ", gp.last);
    str_cpy(ui.msg2, live.mode ? "PARTS: PLAY THE SONG" : "UNDO: GLO + G4", sizeof ui.msg2);
    ui.force = 1;
}
static uint32_t gen_sig(void)                           /* the settings that make the piece */
{
    uint32_t h = 2166136261u, k;
    h = (h ^ gp.genre) * 16777619u; h = (h ^ gp.seed) * 16777619u; h = (h ^ gp.energy) * 16777619u; h = (h ^ gp.form) * 16777619u;
    for (k = 0; k < GP_N; k++) h = (h ^ gp.amt[k]) * 16777619u;
    h = (h ^ gp.complex) * 16777619u; h = (h ^ gp.swing) * 16777619u; h = (h ^ gp.human) * 16777619u;
    return h ^ (trk_live << 28);
}
static uint16_t gseed_last[8];                          /* (0.9.17) the last seeds thrown */
static void gen_new_seed(void)
{
    uint32_t k, tries;
    rng_state ^= fm1_ticks() * 2654435761u;
    if (!rng_state) rng_state = 0x1234567u;
    for (tries = 0; tries < 8u; tries++) {              /* (never one of the last eight again) */
        gp.seed = (uint16_t)(rng() % 10000u);
        for (k = 0; k < 8u && gseed_last[k] != gp.seed + 1u; k++) {}
        if (k == 8u) break;
    }
    for (k = 7; k; k--) gseed_last[k] = gseed_last[k - 1u];
    gseed_last[0] = (uint16_t)(gp.seed + 1u);
}
/* OCT+ on the GEN screen: the piece; the same settings again: a new seed first */
static void gen_press(void)
{
    if (gp.sig == gen_sig()) gen_new_seed();
    gen_go();
    gp.sig = gen_sig();
}
/* (the old name: tests, the films, the renders) a new throw: any genre (RANDOM, as it always was: the screen's dice is
 * EXPLORE), a new seed (the screen's settings otherwise) */
static void dice_all(void)
{
    uint8_t g = gp.genre;
    gp.genre = 0;
    gen_new_seed();
    gen_go();
    gp.sig = gen_sig();
    gp.genre = g;
}
static uint32_t dice_print(void)                        /* a fingerprint of the piece (the tests) */
{
    uint32_t k, h = 2166136261u ^ (uint32_t)song.g[G_BPM];
    for (k = 0; k < GEN_NT; k++) h = (h ^ steps_sig(&trk[k]) ^ trk[k].eng_req) * 16777619u;
    return h;
}

/* (0.6.67; 0.9.17 the generator's) SEQ + SELECT: the selected track's sequence diced as GEN writes its part, a new
 * stream each press (the random numbers, the ticks in them). Where a piece of the generator plays in the track's key,
 * on its harmony (its plan, its genre, the track's role in it): the part fits the others. Else a piece's choices made
 * up for it (the last genre, or any; the track's own key where it is a mode of seven): the drums on a DRUM track, else
 * the role its steps sound like (low: the bass, chords: the chords, else a lead or an arp). Until 0.9.16 song_dice.c's
 * own parts: a random genre's progression against the song's (its lead's strong steps on the chords' tones 58 % of the
 * time, GEN's 90 %). The sound, the mix and the automation stay. Its message */
static uint32_t dsq_last[GEN_NT];                         /* (each track's last diced sequence) */
static const char *dice_seq(track_t *t)
{
    static const char *const RN[RL_N] = {"SEQ DICED: DRUMS", "SEQ DICED: BASS", "SEQ DICED: PAD", "SEQ DICED: LEAD",
                                         "SEQ DICED: ARP", "SEQ DICED: TEXTURE"};
    uint32_t k = trk_index(t) % GEN_NT, r, i, len, lo = 0, cnt = 0, chord = 0;
    uint16_t s0 = gp.seed;
    for (i = 0; i < NSTEP; i++)                         /* (what it plays now: its register, its chords) */
        if (t->step[i].time == ST_NOTE && t->step[i].n) { lo += t->step[i].note[0]; cnt++; chord += t->step[i].n > 1u; }
    r = drum_track(t) ? RL_DRUM : gs.g && gs.role[k] != RL_DRUM ? gs.role[k] : cnt && lo / cnt < 48u ? RL_BASS :
        chord * 3u > cnt && cnt ? RL_PAD : rng() % 2u ? RL_LEAD : RL_ARP;
    gp.seed = (uint16_t)(rng() % 10000u);
    if (!gs.g || gs.root != t->p[P_ROOT] || gs.scale != t->p[P_SCALE]) {   /* (no piece here: its choices made up) */
        uint32_t m = SCALE_MASK[(uint32_t)t->p[P_SCALE] % (sizeof SCALE_MASK / sizeof SCALE_MASK[0])], c, d;
        gs.gi = gs.g && gs.gi < NGENRE ? gs.gi : rng() % NGENRE;
        gmut = GENRE[gs.gi]; gs.g = &gmut; gs.mutant = 0; gx.on = 0;
        gseed(GS_PICK);
        gen_pick();
        for (d = 0, c = 0; c < 12u; c++) d += (m >> c) & 1u;
        gs.root = t->p[P_ROOT];
        if (d == 7u && t->p[P_SCALE] != 10) {           /* (its own mode, LOC aside) */
            gs.scale = t->p[P_SCALE];
            for (d = 0, c = 0; c < 12u && d < 7u; c++) if ((m >> c) & 1u) gs.sc[d++] = (uint8_t)c;
            gplan(0);
        }
        gs.kick = 0;
        for (c = 0; c < GEN_NT; c++)                      /* (the kick a bass answers: the DRUM track's) */
            if (drum_track(&trk[c]) && &trk[c] != t)
                for (i = 0; i < dlen(&trk[c]); i++) gs.kick |= (uint64_t)(trk[c].step[i].time == ST_NOTE && (trk[c].step[i].hit & 1u)) << i;
    }
    gs.ba = gs.g->bass[rng() % 3u];
    if (gmut.lead == LD_NONE) gmut.lead = LD_HOOK;      /* (asked for: a lead) */
    for (i = 0; i < GP_N; i++) if (!gs.amt[i]) gs.amt[i] = 60;
    gs.role[k] = (uint8_t)r; gs.act[r] = 1; gs.trk_of[r] = (uint8_t)k;
    len = (uint32_t)t->p[P_SLEN];
    if (len < 8u || len > NSTEP) len = gs.len < 16u ? 16u : gs.len;
    load_begin(t, LOAD_PAT);
    latch_drop_req |= (uint8_t)(1u << k);
    for (cnt = 0; cnt < 4u; cnt++) {                    /* (never the press before's again: another stream, three times
                                                         * at most) */
        for (i = 0; i < NSTEP; i++) step_clear(&t->step[i]);
        gen_role(t, k, len);
        if (steps_sig(t) != dsq_last[k]) break;
        gp.seed = (uint16_t)(rng() % 10000u);
        gs.ba = gs.g->bass[rng() % 3u];
    }
    dsq_last[k] = steps_sig(t);
    t->p[P_SLEN] = (int16_t)len;
    if (r != RL_DRUM) dlevel(t, len);
    load_end(t);
    gp.seed = s0;
    return RN[r];
}

/* ------------------------------------------------------------------------------------------- the GEN screen --- */
static const char *const GEN_LBL[3][4] = {{"GENRE", "SEED", "ENERGY", "FORM"}, {"DRUMS", "BASS", "CHORDS", "ARP"},
                                          {"LEAD", "COMPLEX", "SWING", "HUMAN"}};
static const char *const GEN_FORM[3] = {"LOOP", "PARTS", "SONG"};
static uint8_t gen_fresh;                               /* (0.6.96) the screen's first frame: the body cleared once */
static void gen_open(void)
{
    gp.hot = 0xFF;
    gen_fresh = 1;
    gp.genre = GEN_EXPLORE;                             /* (0.6.93) opens on RANDOM: a genre is this visit's choice;
                                                         * (0.8.6) on EXPLORE: the dice always starts there */
}
static void gen_amt_turn(uint8_t *v, int32_t s)         /* OFF, 10 .. 100, AUTO (above 100) */
{
    int32_t x = *v == GA_AUTO ? 110 : (int32_t)*v;
    x = clamp(x + s * 10, 0, 110);
    *v = (uint8_t)(x > 100 ? GA_AUTO : x);
}
static void gen_knob(uint32_t k, int32_t s)
{
    uint32_t p = gp.page;
    gp.hot = (uint8_t)k;
    if (p == 0u) {
        if (k == 0u) {                                  /* (0.8.6) EXPLORE first, then RANDOM, then the genres */
            uint32_t pos = gp.genre == GEN_EXPLORE ? 0u : gp.genre + 1u;
            pos = (pos + (s > 0 ? 1u : GEN_NPOS - 1u)) % GEN_NPOS;
            gp.genre = (uint8_t)(pos ? pos - 1u : GEN_EXPLORE);
        }
        else if (k == 1u) gp.seed = (uint16_t)((gp.seed + 10000u + (uint32_t)clamp(accel(EN_K2, s, 9999), -9999, 9999)) % 10000u);
        else if (k == 2u) gp.energy = (uint8_t)clamp((int32_t)gp.energy + s * 5, 0, 100);
        else gp.form = (uint8_t)clamp((int32_t)gp.form + (s > 0 ? 1 : -1), 0, 2);
    } else if (p == 1u) {
        gen_amt_turn(&gp.amt[k], s);
    } else {
        if (k == 0u) gen_amt_turn(&gp.amt[GP_LEAD], s);
        else if (k == 1u) gp.complex = (uint8_t)clamp((int32_t)gp.complex + s * 5, 0, 100);
        else if (k == 2u) { int32_t x = gp.swing == GA_AUTO ? 105 : (int32_t)gp.swing; x = clamp(x + s * 5, 0, 105); gp.swing = (uint8_t)(x > 100 ? GA_AUTO : x); }
        else gp.human = (uint8_t)clamp((int32_t)gp.human + s * 5, 0, 100);
    }
    ui.force = 1;
}
static uint32_t gen_page(void) { return gp.page % 3u; }
static void gen_page_put(uint32_t p)                     /* (0.9.2) GEN 1..3: HOME's pages (ui_lens.c nav_go) */
{
    if (gp.page != p % 3u)
        gp.hot = 0xFF;
    gp.page = (uint8_t)(p % 3u);
    ui.force = 1;
}
static void gen_value(uint32_t p, uint32_t k, char *b)
{
    uint32_t v;
    b[0] = 0;
    if (p == 0u) {
        if (k == 0u) { str_cpy(b, gp.genre == GEN_EXPLORE ? "EXPLORE" : gp.genre ? GENRE[(gp.genre - 1u) % NGENRE].name : "RANDOM", 16); return; }
        if (k == 1u) { b[0] = (char)('0' + gp.seed / 1000u % 10u); b[1] = (char)('0' + gp.seed / 100u % 10u); b[2] = (char)('0' + gp.seed / 10u % 10u); b[3] = (char)('0' + gp.seed % 10u); b[4] = 0; return; }
        if (k == 3u) { str_cpy(b, GEN_FORM[gp.form % 3u], 16); return; }
        v = gp.energy;
    } else if (p == 1u) v = gp.amt[k];
    else if (k == 0u) v = gp.amt[GP_LEAD];
    else if (k == 1u) v = gp.complex;
    else if (k == 2u) v = gp.swing;
    else v = gp.human;
    if (v == GA_AUTO) { str_cpy(b, "AUTO", 16); return; }
    if (!v && (p == 1u || (p == 2u && k == 0u))) { str_cpy(b, "OFF", 16); return; }
    b[0] = (char)('0' + v / 100u); b[1] = (char)('0' + v / 10u % 10u); b[2] = (char)('0' + v % 10u); b[3] = '%'; b[4] = 0;
    if (b[0] == '0') { b[0] = b[1]; b[1] = b[2]; b[2] = b[3]; b[3] = 0; if (b[0] == '0' && v < 10u) { b[0] = b[1]; b[1] = b[2]; b[2] = 0; } }
}
/* (0.8.5) GEN in HOME's lens (ui_lens.c, the mock-up's GEN level): its four settings on the cards, the panel the five
 * parts as rows (the ones the page's knobs weigh lit), each a preview of its density (its amount) and heat (ENERGY)
 * from the seed, the FORM's sections as chips, the last piece and OCT+ MAKE */
static void gen_cards(void)
{
    uint32_t k;
    char v[16];
    for (k = 0; k < 4u; k++) {
        uint32_t p = gp.page % 3u, a;
        int32_t r;
        gen_value(p, k, v);
        a = p == 1u ? gp.amt[k] : p == 2u && k == 0u ? gp.amt[GP_LEAD] : p == 2u && k == 2u ? gp.swing : 0u;
        r = p == 0u ? (k == 0u ? (int32_t)(gp.genre == GEN_EXPLORE ? 0u : gp.genre + 1u) * 1000 / (int32_t)(GEN_NPOS - 1u) :   /* (0.8.7:
                                                         * KNOB 1's order, EXPLORE at the left) */ k == 1u ? gp.seed / 10 : k == 2u ? gp.energy * 10 :
                       gp.form * 500) :
            (p == 1u || (p == 2u && (k == 0u || k == 2u))) ? (a == GA_AUTO ? 1000 : (int32_t)a * 10) :
            (int32_t)(k == 1u ? gp.complex : gp.human) * 10;
        draw_column(k, GEN_LBL[p][k], v, "", VAL(k), r, ICON_NONE);
    }
}
static uint32_t gen_draw_sig(void)
{
    return gen_sig() * 31u + gp.page * 7u + str_hash(gp.hot * 977u, gp.last) + ux.gen;
}
static uint32_t gen_hash(uint32_t x) { x *= 2654435761u; x ^= x >> 15; x *= 2246822519u; x ^= x >> 13; return x % 1000u; }
static void gen_panel(void)
{
    static const char *const ROLE[5] = {"DRUMS", "BASS", "CHORDS", "ARP", "LEAD"};
    uint8_t ord[GF_MAXROWS], mul[GF_MAXROWS];
    uint32_t k, i, p = gp.page % 3u, n;
    int32_t bw;
    char v[24];
    for (k = 0; k < 5u; k++) {
        int32_t y = 4 + (int32_t)k * 19, dn;
        uint32_t a = gp.amt[k];
        int on = (p == 1u && k < 4u) || (p == 2u && k == 4u);
        uint16_t bg = on ? T_TINT : T_RAISE;               /* (the mock-up's selection: a tint) */
        cv_rect(4, y, 232, 16, bg);
        GFX_HOOK_ALIGN(0, y, 0, y + 16, AL_V, "GEN part name centred up/down");
        cv_text_on(8, y + CAP_IN(X, 16), &AF_X, ROLE[k], on ? T_TEXT : T_MID, bg);
        str_cpy(v, a == GA_AUTO ? "AUTO" : a ? "" : "OFF", sizeof v);
        if (a && a != GA_AUTO) { fmt_int(v, (int32_t)a); str_cpy(v + str_len(v), "%", 2); }
        GFX_HOOK_ALIGN(0, y, 0, y + 16, AL_V, "GEN part name centred up/down");
        cv_text_r(232, y + CAP_IN(X, 16), &AF_X, v, T_DIM, bg);
        dn = a == GA_AUTO ? 550 : (int32_t)a * 10;
        for (i = 0; i < 32u; i++) {
            uint32_t h = gen_hash(gp.seed * 13u + k * 101u + i + (i > 15u ? gp.complex * 3u / 100u : 0u)), vv = gen_hash(i * 7u + k);
            if ((int32_t)h < dn * (k ? 8 : 10) / 10) {
                int32_t x = 52 + (int32_t)i * 49 / 10, yy = y + 3 + (k == 1u || k == 4u ? (int32_t)(1000u - vv) * 8 / 1000 : 0);
                cv_rect(x, yy, k == 2u ? 9 : 3, k == 2u ? 10 : 3, heat_col(64 + (int32_t)gp.energy * 154 / 100 + (int32_t)vv * 38 / 1000));
            }
        }
    }
    if (gp.form % 3u == 0u) { ord[0] = 0; n = 1; }      /* the FORM's sections, in the order this seed makes */
    else n = gform_order(gp.form == 2u ? (gp.genre && GENRE[(gp.genre - 1u) % NGENRE].form == FM_ARC ? FM_ARC : FM_CLUB) : FM_PARTS,
                         ord, mul);
    bw = 228 / (int32_t)n;
    for (i = 0; i < n; i++) {
        uint32_t sc = ord[i];
        char l[2] = {(char)('A' + ord[i]), 0};
        int32_t x = 6 + (int32_t)i * bw;
        cv_rect(x, 102, bw - 2, 14, sec_col(sc));
        GFX_HOOK_ALIGN(x, 102, x + bw - 2, 116, AL_HV, "GEN section letter centred");
        cv_text_in(x, 102 + CAP_IN(X, 14), bw - 2, &AF_X, l, T_INK, sec_col(sc));
    }
    str_cpy(v, "LAST: ", sizeof v);
    str_cpy(v + 6, gp.last[0] ? gp.last : "-", sizeof v - 6);
    cv_clip_text(6, 126, &AF_X, v, T_MID, T_SURF, 150);
    cv_text_r(234, 126, &AF_X, "OCT+ MAKE", T_ACCENT, T_SURF);
}
