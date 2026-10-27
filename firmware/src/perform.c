/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments
 * Portions Copyright (C) 2026 Juan Ignacio Antonucci (Juan Jiant) */
/* PERFORM: the effects of the FX hold layer, on the master. A key pressed while FX is held belongs to the
 * layer (seq.c keyboard_block): it plays no note, sends no MIDI, records nothing, and holds its effect until
 * it is let go (ui_input.c opens the layer and shows the map). The 5 white keys from the left (F3 .. C4):
 *   REPEAT 1/8 1/16 1/32, (0.9.14) GLITCH, TAPE STOP; then (JIANT) the 10 punch-in MIDI effects D4 .. F5 (pfx.c); black
 *   keys 1..6 (F#3 .. F#4): six tracks muted while held (0.9.20: T1..T6, or T3..T8 with T7 or T8
 *   selected: seq.c key_trk) (not P_MUTE, never saved); (0.9.17) G#4 FREEZE; A#4 the MIDI
 *   effects' tracks; (0.9.17) C#5 SHUFFLE (the FILTER sweep's key until 0.9.16); (0.9.14) D#5 F#5 the SWEEPS CRUSH, THROW.
 * TONIC keeps only the audio stutters (REPEAT): REVERSE, FREEZE and OCT UP / DN (the harmonizer) were removed; the
 * punch-in effects of TONIC act on the notes instead (FELUCCA-TONIC-SPEC.md). (0.9.17: a granular FREEZE came back.)
 * The buffer effects (REPEAT, the GLITCHER: GLITCH, TAPE STOP, SHUFFLE, FREEZE) start on the next 1/16 (at once while
 * stopped) and end when let go, with a 2.9 ms ramp; of them the last pressed plays, and letting it go returns to the one
 * held before. (0.9.17) While a glitcher's key is down (held, or pressed with FX LATCH) the four knobs are its own
 * (perf_gp, ui_layer.c GX_K; never saved): let it go and they are the macros again.
 *   GLITCH (DENS PITCH SIZE VARI): the master cut on the transport's 1/16s, each one (a hash of its place in a 2-bar
 *     cycle and VARI: the same every time, so it grooves; the beats glitched by DENS squared) played live, or: the last
 *     L repeated (L from SIZE: LONG 1/16 .. TINY 1/256), each repeat PITCH semitones above the one before (a riser, or
 *     falling); the last 1/16 reversed, an octave up or down; gated at 1/32 .. 1/128; SKIP (a CD skipping: 1/128 ..
 *     1/512 fragments from anywhere in the last 1/16, two at a place, with 0.7 ms edges: a tick, shaped); CRUSH (live,
 *     6 .. 9 bits off and held 1 .. 6 samples). Every slice windowed (SL_RAMP), live in and out by the buffer's share
 *     (no click). Below ~82 BPM only its gates, crushes and live slices (the loop holds 371 ms).
 *   TAPE STOP (SCRATCH TIME CURVE): the master read from the loop at a speed falling from 1 to 0 over TIME (1/64 .. a
 *     beat, at most what the loop holds), the curve (1 - x)(1 + CURVE x): - drops at once, + holds then drops; the pitch
 *     with it, faded over its last quarter. Stopped, the loop is kept and KNOB 1 is the hand on the record: each
 *     detent moves it 1/48 beat, the platter follows through two 5.8 ms lags (its speed the pitch, its level with the
 *     speed up to a quarter: silent at rest).
 *   SHUFFLE (AMOUNT SIZE SPEED VARI): the last bar's 1/16s (1/8s, 1/4s: SIZE) played back in another order: each chunk
 *     of the transport (a hash of its place in two bars and VARI; AMOUNT of them, the beats AMOUNT squared, the bar's
 *     first AMOUNT cubed) live or one of the last bar's at the same place in the beat or anywhere; SPEED of the moved
 *     ones at half speed, at double (twice) or their first half twice. The bar is kept in IMA ADPCM, 4 bits, mono at
 *     22.05 kHz in the same 32 KB (2.95 s: a bar and its 1/16 down to 87 BPM, less below), the coder's state noted at
 *     every 1/16 (AD_TBL) so a chunk is read from its start; every change cross-faded over SL_RAMP.
 *   FREEZE (DENS PITCH SPRAY SIZE): the eighth after the press is kept (live meanwhile), then played as grains: SIZE
 *     1/128 .. 1/8 long, 1 .. 4 overlapping (DENS), PITCH semitones, scanning the eighth at 1/8 of its speed and back
 *     (a stretch); SPRAY scatters where they read and moves some an octave, a fifth or a fourth (deterministic).
 * The SWEEPS (0.9.14; until 0.9.13 the knob macros FILTER CRUSH THROW and the keys LPF HPF): held, their mix rises
 * from dry to wet over 2 bars at the tempo, on an S curve (slow out of dry, slow into wet); let go, back to dry over a
 * beat (1/4 bar) on the same curve. CRUSH: the bits and the rate reduced (4 .. 11 bits off, a sample held 1 .. 8), its
 * mix with them; THROW: the dry mix (and the SLICER's bus) into the delay and reverb sends, up to all of it. (0.9.17:
 * the FILTER sweep is gone: the FILTER macro is the layer's one filter.)
 * The knobs with FX (0.9.14): the PERFORMANCE MACROS on the whole project, built up turning right: STEPS (every
 * sequence's window 1/2 1/4 1/8 1 step, from where it plays: seq_live.c LOOP), TRANSPOSE (+2ND +3RD +4TH +5TH +6TH
 * along the scale, +OCT, -OCT: seq.c strn_note; not the kits), MUTATE (project.c perf_mut_poll: the patterns mutated
 * around an anchor, every bar), FILTER (the DJ FILTER's low-pass closing). Any knob turned back (two detents) puts all
 * four off at once: the music as it was. Never saved, never recorded.
 * MENU > FX LATCH ON (#40, a hand that cannot hold FX, a key and a knob together): a key pressed with FX turns its
 * effect on, the next press turns it off (perf_latched; letting the key or FX go does nothing), FX + OCT- turns
 * everything off (ui_layer.c), so does the menu or a dialog.
 * Chain: the buffer effect -> CRUSH, after the master level and before master_out (the limiter); THROW and the mutes
 * act before the buses (fx.c mix_block / mix_part), the FILTER macro in the DJ FILTER.
 * The buffer: the SLICER's recordings (sl_buf, 32 KB) borrowed as one stereo loop of 8192 frames at 22.05 kHz
 * (371 ms; SHUFFLE: the ADPCM ring above). While borrowed, STUT tracks play live; their recordings are dropped
 * afterwards. A REPEAT 1/8 longer than the loop at the tempo (below 81 BPM) does nothing (the map shows it dimmed).
 * Idle (no key, no ramp left) every stage is skipped: the output is bit-identical. */
enum { PF_R8, PF_R16, PF_R32, PF_GLT, PF_TAPE,
       PF_OCTD, PF_OCTU, PF_HALF, PF_DSHT, PF_DLNG, PF_S8, PF_S16, PF_ATK, PF_ARP, PF_RND,   /* the MIDI ones: pfx.c */
       PF_SHUF,                                            /* (0.9.17) C#5 SHUFFLE: the lane's 11 (the FILTER sweep's) */
       PF_SCRU, PF_STHR,                                   /* (0.9.14) the sweeps: D#5 F#5 */
       PF_FRZ,                                             /* (0.9.17) G#4 FREEZE */
       PF_M1, PF_N = PF_M1 + NTRK, PF_TGT = PF_N + 1,     /* PF_TGT: A#4, the MIDI effects' tracks (pfx_tgt) */
       PF_CLR = PF_N + 2 };                                /* PF_CLR: G5, erases the punch-in lane (pfx.c) */
#define PF_NW 15u                                              /* the white keys' effects, F3 .. F5 */
#define PF_BIT(e) (1u << (e))
#define PF_REPEAT 0x7u                                         /* the three REPEAT rates */
#define PF_BUF (0x1Fu | PF_BIT(PF_SHUF) | PF_BIT(PF_FRZ))      /* the buffer effects: REPEAT, GLITCH, TAPE, SHUFFLE, FREEZE */
#define PF_GX (PF_BIT(PF_GLT) | PF_BIT(PF_TAPE) | PF_BIT(PF_SHUF) | PF_BIT(PF_FRZ))   /* (0.9.17) the glitcher: its knobs */
#define PF_Q PF_BUF                                            /* start on the 1/16 */
#define PF_SWEEP (3u << PF_SCRU)
#define PF_MUTE (((1u << NTRK) - 1u) << PF_M1)
#define PF_MIDI (((1u << (PF_RND + 1)) - 1u) & ~((1u << PF_OCTD) - 1u))   /* PF_OCTD .. PF_RND: on the notes (pfx.c) */
#define PF_LANE (PF_MIDI | PF_SWEEP | PF_BIT(PF_GLT) | PF_BIT(PF_TAPE) | PF_BIT(PF_SHUF))   /* what the punch-in lane records */
#define PB_FRAMES SL_FR                       /* stereo frames in sl_buf (a power of 2: the mask), 371 ms at 22.05 kHz */
_Static_assert(PB_FRAMES <= SL_FR && !(PB_FRAMES & (PB_FRAMES - 1u)), "the REPEAT loop in sl_buf");
#define PB_MAX (2u * PB_FRAMES)               /* the longest loop, 44.1 kHz samples */
#define GL_SPAN 7680u                         /* GLITCH: the longest slice it reads back (the loop holds two and a margin) */
#define TP_LAG 15000u                         /* TAPE STOP: how far it may fall behind (the loop's 16384 samples, a margin) */
#define AD_BYTES (sizeof sl_buf - 256u)       /* (0.9.17) SHUFFLE's ADPCM ring: 32512 B, 65024 nibbles, 2.95 s */
#define AD_N (2u * AD_BYTES)
#define AD_TBL (&sl_buf[SL_FR - 64u][0])      /* .. the coder at each 1/16's start: 32 x (place, value, step index) */
_Static_assert(AD_N <= 65536u && 3u * 32u <= 128u, "SHUFFLE's ring places in 16 bits, its table in the last 64 frames");
enum { BM_NONE, BM_LOOP, BM_GLT, BM_TAPE, BM_SHUF, BM_FRZ };
enum { GA_PASS, GA_RPT, GA_REV, GA_UP, GA_DN, GA_GATE, GA_SKIP, GA_CRU };
enum { SK_NORM, SK_HALF, SK_DBL, SK_STUT };
enum { GX_GLT, GX_TAPE, GX_SHUF, GX_FRZ };   /* (0.9.17) perf_gp's rows */

static volatile uint32_t perf_mask;   /* main: the FX button's bit while its layer may own keys, 0 = none */
static volatile uint32_t kb_mask;     /* main: the button bits whose hold makes keys a layer's (ui_layer.c), 0 = none */
static volatile uint8_t perf_solo;    /* main: tracks soloed in the GLO layer (the others muted as by a black key) */
static volatile uint32_t scene_pfx;  /* (JIANT) the punch-in MIDI effects a song row's scene holds (song_play.c), PF bits */
static volatile uint8_t perf_kill;    /* main: every effect off (the menu, a dialog, UBOOT) */
static volatile uint8_t perf_m[4];    /* main (0.9.14): the PERFORMANCE MACROS, 0 OFF: STEPS 0..12 (three detents a
                                       * stage), TRANSPOSE 0..21 (three an interval), MUTATE, FILTER 0..100 (PM_MAX) */
static const uint8_t PM_MAX[4] = {12, 21, 100, 100};
/* (0.9.17) main: the glitcher's knobs (ui_layer.c GX_K: their names, ranges and these defaults), read by the ISR.
 * GLITCH DENS % PITCH st SIZE (LONG MID SHORT TINY) VARI; TAPE (SCRATCH) TIME (1/64 .. 1/4) CURVE; SHUFFLE AMOUNT %
 * SIZE (1/4 1/8 1/16) SPEED % VARI; FREEZE DENS % PITCH st SPRAY % SIZE (1/128 .. 1/8) */
static volatile int8_t perf_gp[4][4] = {{50, 0, 0, 1}, {0, 4, 0, 0}, {60, 2, 20, 1}, {50, 0, 30, 2}};
static volatile uint32_t perf_scr;    /* main (0.9.17): TAPE's KNOB 1 detents, a running count (the ISR keeps its own) */
static volatile int8_t perf_djf;      /* ISR: the FX layer's DJ FILTER (fx.c djf_v): the FILTER macro's low-pass */
static volatile uint32_t kb_layer;    /* ISR: the keys held that are the layer's, a bit per key */
static volatile uint32_t perf_held;   /* ISR: the effects held (PF_*) */
static volatile uint32_t perf_down;   /* ISR (0.9.17): the effects whose key is down (FX LATCH too): a glitcher's knobs */
static volatile uint8_t perf_latch_on;   /* main: MENU > FX LATCH ON (#40): a key with FX toggles its effect */
static volatile uint32_t perf_latched;   /* ISR: the effects latched (FX LATCH; main clears it: OCT-, the menu) */
static volatile uint32_t perf_act;    /* ISR: .. running (a 1/16 one from its start on) */
static uint32_t pfx_lfx;              /* ISR (0.9.14): the audio effects the punch-in lane plays this block (pfx.c) */
static uint16_t perf_ord[PF_N], perf_seq;   /* press order: the last pressed buffer effect plays (0.9.17: 16 bits) */

typedef struct {                       /* (0.9.17) SHUFFLE: a reader of the ADPCM ring, or live */
    uint32_t fr, rate;                 /* between a and b (Q16), ring samples an output sample (Q16: 32768 the speed) */
    uint16_t pos;                      /* the next nibble */
    int16_t a, b;                      /* the last two decoded (b: the decoder's value) */
    uint8_t idx, live;                 /* the step index; 1 = the live input */
} shr_t;
typedef struct { uint32_t pos, rate, x, dx; } grain_t;   /* (0.9.17) FREEZE: read place (Q16), rate, window 0..65536 */
static struct {
    uint32_t ph, P, split;             /* samples since play; the 1/16; this block: where a 1/16 starts */
    uint32_t act, act0;                /* running after / before split */
    uint8_t busy, sync, mute;          /* sync: a 1/16 starts with the next block; mute: tracks to ramp */
    /* the buffer */
    uint8_t mode, src, next;           /* BM_*, the effect it plays (PF_N none), one waiting for the fade */
    uint8_t state, odd;                /* LOOP: 0 recording, 1 playing; TAPE: 1 stopped; FREEZE: 1 frozen; a pair's half */
    uint32_t len, wr, rp;              /* loop length (samples), frames written, read (samples into the loop) */
    uint32_t xrp, xlen, xn;            /* the loop it switched from, faded over xn samples */
    int32_t w, tw, hl, hr;             /* the buffer's share and its target, Q15; the first of a pair */
    int32_t yl, yr;                    /* (0.9.17) what the glitcher plays this sample */
    uint32_t gs, gph;                  /* GLITCH, SHUFFLE: the transport's 1/16 (a count), samples into it */
    union {                            /* (0.9.17) the one buffer effect playing */
        struct {                       /* GLITCH: the action, the crush's count, hold and shift; segment length, samples
                                        * into it, segments past; the loop's sample at the slice's start, the span read;
                                        * the read place at the segment's start (Q16) and its rate (Q16, signed) */
            uint8_t a, n, h, sh;
            uint32_t l, u, k, g0, sp, src;
            int32_t rate, cl, cr;
        } g;
        struct { uint32_t x, dx, p, p1, q, sc; int32_t c, gn; } t;   /* TAPE: the stop's progress (Q24) and step a sample,
                                        * the read place (Q16), the platter's lag and the hand (Q16), KNOB 1's count seen,
                                        * CURVE (Q15), the scratch's level */
        struct { uint32_t c, fs, iv, scan, rnd, ph; int32_t sd, gain, nl, nr; grain_t v[4]; } f;   /* FREEZE: the capture
                                        * (samples), samples since the last grain, between grains, the scan, the dice, odd
                                        * or even, the scan's way, the level, the next even sample's sum */
        struct { uint32_t ew, nrec, cu, mid; int32_t ep; uint8_t ei, kind, j0, xn; shr_t r[2]; } s;   /* SHUFFLE: the coder
                                        * (nibble, value, step index), 1/16s noted, samples into the chunk, its middle;
                                        * the chunk's kind and slot, the cross-fade; the reader and the one fading out */
    } u;
    /* the sweeps (Q24 progress: 0 dry), CRUSH, THROW, mutes */
    uint32_t sw[2];
    int32_t ss[2];                     /* .. on the S curve, Q15 */
    int32_t cw, chl, chr;              /* CRUSH share, held sample */
    uint32_t cn;
    int32_t td;                        /* THROW: the share of the dry mix sent, Q15 */
    int32_t mg[NTRK];                  /* mute gains, Q15 (32768 = open) */
} pf = {.src = PF_N, .next = PF_N, .mg = {[0 ... NTRK - 1] = 32768}};   /* (every track open: 0.6, six) */

/* the loop length of a REPEAT at the tempo, 44.1 kHz samples; 0 = not one */
static const uint8_t PF_DEN[PF_R32 + 1] = {2, 4, 8};
static uint32_t perf_len(uint32_t e) { return e <= PF_R32 ? beat_samples() / PF_DEN[e] : 0u; }
/* the effects that can run at the tempo: a REPEAT longer than the loop cannot */
static uint32_t perf_avail(void)
{
    uint32_t e, m = ~0u, b = beat_samples();
    for (e = PF_R8; e <= PF_R32; e++)
        if (b > PB_MAX * PF_DEN[e])
            m &= ~PF_BIT(e);
    return m;
}

/* seq.c keyboard_block: a layer key down / up (e: PF_*, PF_N = no effect) */
static volatile uint8_t pfx_tgt;      /* main / keys: the MIDI effects' tracks, 0 ALL, 1 SYN, 2 DRM (pfx.c) */
static volatile uint8_t pfx_clr;      /* keys: G5 held (pfx.c: the lane erased where it passes, or all of it) */
static uint32_t pfx_lph;              /* samples into the punch-in lane (pfx.c): from the transport's start, a section's */
static uint8_t pfx_lane[32], pfx_ltgt;   /* the section's punch-in lane (pfx.c), its tracks (as pfx_tgt) */
static void perf_press(uint32_t e, int down)
{
    if (e == PF_TGT && down)
        pfx_tgt = (uint8_t)((pfx_tgt + 1u) % 3u);
    if (e == PF_CLR)
        pfx_clr = (uint8_t)(down ? 1u : 0u);
    if (e >= PF_N)
        return;
    if (down) {
        perf_down |= PF_BIT(e);
        perf_ord[e] = ++perf_seq;
    } else {
        perf_down &= ~PF_BIT(e);
    }
    if (perf_latch_on) {                  /* FX LATCH: a press turns it on or off, letting go does nothing */
        if (down)
            perf_latched ^= PF_BIT(e);
        return;
    }
    if (down)
        perf_held |= PF_BIT(e);
    else
        perf_held &= ~PF_BIT(e);
}

static void perf_start(void) { pf.ph = 0; pf.sync = 1; pfx_lph = 0; }   /* seq_start: a 1/16 starts with the transport */

/* the SLICER's recordings dropped (when the buffer is taken and given back) */
static void perf_drop_slicer(void) { slicer_drop(); }

/* the buffer effect to play: the last pressed of those running */
static uint32_t perf_pick(uint32_t a)
{
    uint32_t e, best = PF_N;
    a &= PF_BUF;
    for (e = 0; e < PF_M1; e++)
        if (((a >> e) & 1u) && (best == PF_N || (uint16_t)(perf_ord[e] - perf_ord[best]) < 0x8000u))
            best = e;
    return best;
}
/* (0.9.17) the glitcher whose knobs the FX layer's are: the last pressed of those whose key is down (PF_N: none) */
static uint32_t perf_gfocus(void) { return perf_kill ? PF_N : perf_pick(perf_down & PF_GX); }
static uint32_t perf_gx(uint32_t e) { return e == PF_GLT ? GX_GLT : e == PF_TAPE ? GX_TAPE : e == PF_SHUF ? GX_SHUF : GX_FRZ; }

/* (0.9.14) a slice's place in its 2-bar cycle (and its variation) to 32 random bits */
static uint32_t pf_hash(uint32_t h)
{
    h *= 0x9E3779B1u;
    h = (h ^ (h >> 15)) * 0x2C1B3C6Du;
    return h ^ (h >> 13);
}
static uint32_t semi_q16(int32_t st);              /* fx.c: 2^(st / 12), Q16 */

static void perf_glt_step(void);
static void perf_shuf_step(void);
static __attribute__((noinline)) void perf_frz_grain(void);
static void perf_buf_start(uint32_t e)
{
    uint32_t b = beat_samples();
    if (!sl_lent) {
        sl_lent = 1;
        perf_drop_slicer();
    }
    pf.src = (uint8_t)e;
    pf.next = PF_N;
    pf.wr = pf.rp = 0;
    pf.odd = 0;
    pf.xn = 0;
    pf.mode = e == PF_GLT ? BM_GLT : e == PF_TAPE ? BM_TAPE : e == PF_SHUF ? BM_SHUF : e == PF_FRZ ? BM_FRZ : BM_LOOP;
    pf.state = 0;
    pf.len = perf_len(e);
    memset(&pf.u, 0, sizeof pf.u);
    memset(sl_buf[PB_FRAMES - 2u], 0, 2u * sizeof sl_buf[0]);   /* (what GLITCH / TAPE read before the start: quiet) */
    if (e == PF_GLT || e == PF_SHUF) {    /* its first 1/16: the transport's (this block's split starts the next one) */
        pf.gs = song.playing && clk_pos != CLK_START ? clk_n + (pf.split ? 1u : 0u) : 0u;
        if (e == PF_GLT) {
            perf_glt_step();
        } else {
            pf.u.s.r[0].live = 1;         /* (live until a chunk moves) */
            perf_shuf_step();
        }
    }
    if (e == PF_TAPE) {                   /* two frames behind (quiet), at full speed falling to 0 over TIME */
        int32_t c = perf_gp[GX_TAPE][2];
        uint32_t t = b >> (4u - (uint32_t)clamp(perf_gp[GX_TAPE][1], 0, 4)), m = TP_LAG * 600u / (uint32_t)(300 - c);
        pf.u.t.p = (PB_MAX - 4u) << 16;   /* (it falls behind by t (1/2 - CURVE / 6): at most TP_LAG) */
        pf.u.t.c = c * 327;
        pf.u.t.dx = (1u << 24) / (t < m ? (t > 64u ? t : 64u) : m);
        pf.u.t.sc = perf_scr;
    }
    if (e == PF_FRZ) {                    /* the eighth after the press, then the grains */
        pf.u.f.c = b / 2u < PB_MAX - 1024u ? (b / 2u > 4096u ? b / 2u : 4096u) : PB_MAX - 1024u;
        pf.u.f.rnd = 0x2545F491u;
        pf.u.f.sd = 1;
        pf.u.f.v[0].x = pf.u.f.v[1].x = pf.u.f.v[2].x = pf.u.f.v[3].x = 65536u;
    }
}

/* the buffer effect to play becomes e (at a 1/16): a shorter REPEAT of what is
 * recorded switches with a cross-fade, anything else fades the old one out first */
static void perf_buf_select(uint32_t e)
{
    if (e == pf.src && pf.next == PF_N)
        return;
    if (e != PF_N && pf.mode == BM_LOOP && pf.state && e <= PF_R32 && pf.src != PF_N &&
        perf_len(e) <= 2u * pf.wr) {
        pf.xrp = pf.rp;
        pf.xlen = pf.len;
        pf.xn = SL_RAMP;
        pf.src = (uint8_t)e;
        pf.next = PF_N;
        pf.len = perf_len(e);
        pf.rp = 0;
        return;
    }
    if (pf.mode == BM_NONE || !pf.w) {
        if (e == PF_N) {
            pf.src = PF_N;
            pf.next = PF_N;
        } else {
            perf_buf_start(e);
        }
        return;
    }
    pf.src = PF_N;                                  /* fade out, then the next one (perf_seg) */
    pf.next = (uint8_t)e;
}

/* each block, after the keys (seq.c) and before the parts: what runs, the 1/16, the sweeps, the mutes.
 * Returns 1 when a stage has something to do */
/* (0.6.19) a track let go from its mute, silent (its voices ended at 0, perf_hush_end): nothing to ramp back, open at
 * once; else mix_part never reached perf_mute again (no signal) and the FX layer stayed busy every block */
static __attribute__((noinline)) void perf_unhush(void)   /* (out of perf_begin: target_budget.py) */
{
    uint32_t k, i, on;
    for (k = 0; k < NTRK; k++)
        if (!pf.mg[k] && !((pf.act >> (PF_M1 + k)) & 1u)) {
            for (i = on = 0; i < NVOICE; i++)
                on |= trk[k].v[i].active;
            if (!on)
                pf.mg[k] = 32768;
        }
}
/* (0.9.14) the sweeps (CRUSH, THROW): held, 2 bars from dry to wet; let go, a beat back; their S curve (3 p^2 - 2 p^3) */
static __attribute__((noinline)) void perf_sweeps(uint32_t n)
{
    uint32_t k, b = beat_samples(), up = (n << 24) / (8u * (b ? b : 1u)), dn = (n << 24) / (b ? b : 1u);
    for (k = 0; k < 2u; k++) {
        uint32_t x = pf.sw[k], p;
        if ((pf.act >> (PF_SCRU + k)) & 1u)
            x = x + up < (1u << 24) ? x + up : 1u << 24;
        else
            x = x > dn ? x - dn : 0u;
        pf.sw[k] = x;
        p = x >> 9;                                 /* (Q15) */
        pf.ss[k] = (int32_t)(((p * p) >> 15) * (3u * 32768u - 2u * p) >> 15);
    }
}
static __attribute__((noinline)) int perf_begin(uint32_t n)
{
    uint32_t held = perf_kill ? 0u : (perf_held | perf_latched | pfx_lfx | (perf_solo ? (~(uint32_t)perf_solo & ((1u << NTRK) - 1u)) << PF_M1 : 0u)) & perf_avail() & ~PF_MIDI;
    uint32_t q, k, ph0, bnd;
    int32_t lp = -(int32_t)(perf_m[3] * 64u / 100u);
    if (!held && !pf.busy) {                        /* idle: the clock only */
        pf.ph += n;
        pf.sync = 0;
        perf_act = 0;
        trk_hush = 0;
        perf_djf = (int8_t)lp;
        return 0;
    }
    pf.P = beat_samples() >> 2;
    if (pf.sync) {
        pf.ph = 0;
        pf.sync = 0;
    }
    if (song.playing && clk_pos != CLK_START)       /* (JIANT) playing: the transport's 1/16 (seq.c click_tick, through
                                                     * tempo changes and an external clock), so a REPEAT starts on the grid */
        pf.P = div_samples(2);
    ph0 = (song.playing && clk_pos != CLK_START ? clk_pos : pf.ph) % pf.P;   /* the 1/16 in this block (n: none) */
    bnd = ph0 ? (pf.P - ph0 < n ? pf.P - ph0 : n) : 0u;
    pf.ph += n;
    pf.act &= held;                                 /* let go: at once */
    pf.act |= held & (~PF_Q | pfx_lfx);             /* (the lane's: on its own step, already the 1/16's) */
    pf.act0 = pf.act;
    q = held & PF_Q & ~pf.act;
    pf.split = n;
    if (q && (!song.playing || bnd < n)) {
        pf.split = song.playing ? bnd : 0u;
        pf.act |= q;
    }
    perf_act = pf.act;
    perf_sweeps(n);
    perf_djf = (int8_t)lp;                          /* (the FILTER macro's low-pass; 0.9.17: no sweep over it) */
    pf.mute = 0;
    perf_unhush();
    for (k = 0; k < NTRK; k++)
        if (((held >> (PF_M1 + k)) & 1u) || pf.mg[k] != 32768)
            pf.mute |= (uint8_t)(1u << k);
    trk_hush = (uint8_t)((pf.act >> PF_M1) & ((1u << NTRK) - 1u));   /* (0.6.15) muted: no new notes (voice.c) */
    pf.busy = 1;
    return 1;
}

/* a track muted by its black key: its signal ramps to 0 and back (fx.c mix_part, after the SLICER) */
static __attribute__((noinline)) void perf_hush_end(uint32_t k)   /* (out of perf_mute's loop: target_budget.py) */
{
    uint32_t i;
    for (i = 0; i < NVOICE; i++) {
        voice_t *v = &trk[k].v[i];
        v->active = v->gate = 0;
        v->stage = 0;
        v->env = v->env_out = 0;
    }
}
static __attribute__((noinline)) void perf_mute(uint32_t k, int32_t *b, uint32_t n)
{
    uint32_t i;
    int32_t g = pf.mg[k], t = (pf.act >> (PF_M1 + k)) & 1u ? 0 : 32768;
    for (i = 0; i < n; i++) {
        g += clamp(t - g, -SL_SLOPE, SL_SLOPE);
        b[i] = mulq16(b[i], (uint32_t)g << 1);
    }
    pf.mg[k] = g;
    if (!g && !t)                                       /* (0.6.15) silent: its voices end, the track costs nothing
                                                         * while muted (a MIDI mute: no new notes, trk_hush) */
        perf_hush_end(k);
}

/* before the buses: THROW (its sweep) adds the dry mix (and the SLICER's bus, bl / br) to the delay and reverb sends */
static __attribute__((noinline)) void perf_pre(const int32_t *ml, const int32_t *mr, const int32_t *bl, const int32_t *br,
                                               int32_t *sd, int32_t *sr, uint32_t n)
{
    uint32_t i;
    int32_t m = pf.ss[1];
    for (i = 0; i < n; i++) {
        pf.td += clamp(m - pf.td, -SL_SLOPE, SL_SLOPE);
        if (pf.td) {
            int32_t x = mulq16((ml[i] + mr[i] + bl[i] + br[i]) >> 1, (uint32_t)pf.td << 1);
            sd[i] += x;
            sr[i] += x;
        }
    }
}

/* x towards y by w (Q15, 32768 = y) */
static inline int32_t pf_mix(int32_t x, int32_t y, int32_t w)
{
    return w == 32768 ? y : x + mulq16(y - x, (uint32_t)w << 1);
}

/* the loop: frame f of it, both channels (Q15) */
static inline void pb_put(uint32_t f, int32_t l, int32_t r)
{
    int16_t *p = &sl_buf[0][0] + 2u * (f & (PB_FRAMES - 1u));
    p[0] = (int16_t)clamp(l, -32768, 32767);
    p[1] = (int16_t)clamp(r, -32768, 32767);
}
/* sample s of the frames recorded from 0 (n of them): between two frames on an odd one */
static inline void pb_at(uint32_t s, uint32_t n, int32_t *l, int32_t *r)
{
    uint32_t f = s >> 1 < n ? s >> 1 : n - 1u;    /* (an odd loop's last sample: the frame before) */
    const int16_t *p = &sl_buf[0][0] + 2u * (f & (PB_FRAMES - 1u));
    const int16_t *q;                               /* a frame is the mean of samples 2f, 2f + 1: sample s */
    if (s & 1u)                                     /* lies a quarter frame from it, towards f + 1 or f - 1 */
        q = f + 1u < n ? p + 2 : p;
    else
        q = f ? p - 2 : p;
    *l = (3 * p[0] + q[0]) >> 1;
    *r = (3 * p[1] + q[1]) >> 1;
}
/* (0.9.17) the loop as a ring (GLITCH, TAPE, FREEZE: frames recorded on and on) at the place s (Q16 of a 44.1 kHz
 * sample), between its two frames (linear): any speed, backwards too */
static __attribute__((noinline)) void pb_ring_f(uint32_t s, int32_t *l, int32_t *r)
{
    uint32_t f = s >> 17;
    int32_t fr = (int32_t)((s >> 2) & 0x7FFFu);
    const int16_t *p = &sl_buf[f & (PB_FRAMES - 1u)][0], *q = &sl_buf[(f + 1u) & (PB_FRAMES - 1u)][0];
    *l = 2 * (p[0] + (((q[0] - p[0]) * fr) >> 15));
    *r = 2 * (p[1] + (((q[1] - p[1]) * fr) >> 15));
}
/* REPEAT at sample rp of a loop of len, windowed at both ends */
static inline void pb_loop(uint32_t rp, uint32_t len, uint32_t n, int32_t *l, int32_t *r)
{
    uint32_t e = len - 1u - rp;
    pb_at(rp, n, l, r);
    if (rp < e)
        e = rp;
    if (e < (uint32_t)SL_RAMP) {
        *l = (*l * (int32_t)e) >> SL_RAMP_LOG2;
        *r = (*r * (int32_t)e) >> SL_RAMP_LOG2;
    }
}

/* record the pair into frame f (the mean, >> 1: headroom) */
static inline uint32_t pb_rec(uint32_t f, int32_t l, int32_t r)
{
    if (!pf.odd) {
        pf.hl = l;
        pf.hr = r;
        pf.odd = 1;
        return f;
    }
    pf.odd = 0;
    pb_put(f, (pf.hl + l) >> 2, (pf.hr + r) >> 2);
    return f + 1u;
}

/* ------------------------------------------------------------------ GLITCH --- */
/* (0.9.17) a glitched 1/16's action, by 4 bits of its hash: repeats most, then the rest */
static const uint8_t GA_OF[16] = {GA_RPT, GA_RPT, GA_RPT, GA_RPT, GA_RPT, GA_RPT, GA_REV, GA_REV, GA_UP, GA_DN,
                                  GA_GATE, GA_GATE, GA_SKIP, GA_SKIP, GA_CRU, GA_CRU};
/* a segment's start: where it reads (Q16) and how fast (a REPEAT's k-th: PITCH k times; SKIP: a place in the 1/16,
 * two segments at each) */
static __attribute__((noinline)) void perf_glt_seg(void)
{
    uint32_t a = pf.u.g.a, L = pf.u.g.l, g0 = pf.u.g.g0, s = g0 - L;
    pf.u.g.rate = 65536;
    if (a == GA_RPT) {
        pf.u.g.rate = (int32_t)semi_q16(clamp((int32_t)(pf.u.g.k < 24u ? pf.u.g.k : 24u) * perf_gp[GX_GLT][1], -24, 24));
    } else if (a == GA_REV) {
        s = g0 - 1u;
        pf.u.g.rate = -65536;
    } else if (a == GA_UP) {
        s = g0 - 2u * L;
        pf.u.g.rate = 131072;
    } else if (a == GA_DN) {
        pf.u.g.rate = 32768;
    } else if (a == GA_SKIP) {
        s = g0 - pf.u.g.sp + pf_hash(pf.gs * 64u + (pf.u.g.k >> 1)) % (pf.u.g.sp - L + 1u);
    }
    pf.u.g.src = s << 16;
}
/* (0.9.14) GLITCH: the next 1/16's slice, by the hash of its place in two bars (0.9.17: and VARI; DENS of them glitched,
 * the beats DENS squared: at 50 %, one beat in four) */
static __attribute__((noinline)) void perf_glt_step(void)
{
    uint32_t h = pf_hash((pf.gs & 31u) + 0x51u + ((uint32_t)(perf_gp[GX_GLT][3] - 1) << 5)), s16 = pf.P ? pf.P : 1u,
             sp = s16 < GL_SPAN ? s16 : GL_SPAN, d = (uint32_t)perf_gp[GX_GLT][0], z = (uint32_t)perf_gp[GX_GLT][2],
             j = (h >> 12) % 3u, a, L;
    a = ((h >> 24) * 100u >> 8) < (pf.gs & 3u ? d : d * d / 100u) ? GA_OF[(h >> 8) & 15u] : GA_PASS;
    pf.u.g.g0 = 2u * pf.wr + pf.odd;                /* (the loop's sample now) */
    if ((s16 + sp > PB_MAX - 512u || pf.u.g.g0 < sp) && a != GA_GATE && a != GA_CRU)
        a = GA_PASS;                                /* (too slow: what it would read is gone; or not recorded yet) */
    L = a == GA_RPT ? sp >> (z + j) : a == GA_GATE ? (s16 / 2u) >> (z / 2u + j % 2u) : a == GA_SKIP ? sp >> (3u + z / 2u + j % 2u)
      : a == GA_UP ? sp / 2u : a == GA_CRU ? s16 : sp;
    pf.u.g.a = (uint8_t)a;
    pf.u.g.l = L < 2u * SL_RAMP ? 2u * SL_RAMP : L;
    pf.u.g.sp = sp;
    pf.u.g.h = (uint8_t)(1u + (h >> 14) % 6u);      /* CRUSH: a sample held 1 .. 6, 6 .. 9 bits off (of Q15) */
    pf.u.g.sh = (uint8_t)(6u + j + (h >> 20) % 2u);
    pf.u.g.n = 255u;
    pf.u.g.u = pf.u.g.k = pf.gph = 0;
    perf_glt_seg();
}
/* GLITCH on one sample: 1 = it plays (pf.yl yr), 0 = live */
static __attribute__((noinline)) int perf_glt(int32_t l, int32_t r)
{
    uint32_t u = pf.u.g.u, L = pf.u.g.l, a = pf.u.g.a, e = L - 1u - u < u ? L - 1u - u : u, g = 32768u;
    pf.wr = pb_rec(pf.wr, l, r);
    if (a != GA_PASS) {
        if (a == GA_SKIP) {                         /* (each segment windowed; SKIP's edges 0.7 ms: a tick, shaped) */
            if (e < SL_RAMP / 4u)
                g = e << (17 - SL_RAMP_LOG2);
        } else if (e < (uint32_t)SL_RAMP) {
            g = e << (15 - SL_RAMP_LOG2);
        }
        if (a == GA_GATE) {
            g = pf.u.g.k & 1u ? 0u : g;
            pf.yl = mulq16(l, g << 1);
            pf.yr = mulq16(r, g << 1);
        } else if (a == GA_CRU) {                   /* (live, crushed; windowed as the others) */
            if (++pf.u.g.n >= pf.u.g.h) {
                int32_t b = 1 << (pf.u.g.sh - 1u);
                pf.u.g.n = 0;
                pf.u.g.cl = ((l + b) >> pf.u.g.sh) << pf.u.g.sh;   /* (to the nearest: silence stays 0, no DC) */
                pf.u.g.cr = ((r + b) >> pf.u.g.sh) << pf.u.g.sh;
            }
            pf.yl = mulq16(pf.u.g.cl, g << 1);
            pf.yr = mulq16(pf.u.g.cr, g << 1);
        } else {
            pb_ring_f(pf.u.g.src + (uint32_t)((int32_t)u * (pf.u.g.rate >> 2)) * 4u, &pf.yl, &pf.yr);
            pf.yl = mulq16(pf.yl, g << 1);
            pf.yr = mulq16(pf.yr, g << 1);
        }
    }
    if (++pf.u.g.u >= L) {
        pf.u.g.u = 0;
        pf.u.g.k++;
        perf_glt_seg();
    }
    if (++pf.gph >= pf.P) {
        pf.gs++;
        perf_glt_step();
    }
    return a != GA_PASS;
}

/* --------------------------------------------------------------- TAPE STOP --- */
/* TAPE on one sample: stopping (the speed (1 - x)(1 + CURVE x), x 0 .. 1 over TIME, the level 4x the speed up to 1),
 * then stopped: the scratch (KNOB 1 the hand, perf_scratch; the platter two one-poles of 256 samples behind it) */
static __attribute__((noinline)) int perf_tape(int32_t l, int32_t r)
{
    uint32_t g;
    if (!pf.state) {
        uint32_t xq = pf.u.t.x >> 9, v = (32768u - xq) * (uint32_t)(32768 + ((pf.u.t.c * (int32_t)xq) >> 15)) >> 15;
        pf.wr = pb_rec(pf.wr, l, r);
        g = v < 8192u ? v << 2 : 32768u;
        pb_ring_f(pf.u.t.p, &pf.yl, &pf.yr);
        pf.u.t.p += v << 1;
        if ((pf.u.t.x += pf.u.t.dx) >= 1u << 24) { /* stopped: the loop kept (no more recording), the hand on it */
            pf.state = 1;
            pf.u.t.q = pf.u.t.p1 = pf.u.t.p;
        }
    } else {
        uint32_t p0 = pf.u.t.p;
        int32_t d;
        pf.u.t.p1 += (uint32_t)((int32_t)(pf.u.t.q - pf.u.t.p1) >> 8);
        pf.u.t.p += (uint32_t)((int32_t)(pf.u.t.p1 - pf.u.t.p) >> 8);
        d = (int32_t)(pf.u.t.p - p0);
        d = d < 0 ? -d : d;                         /* (its speed, Q16: the level up to a quarter of it; below 1/64 none) */
        pf.u.t.gn += ((d >= 16384 ? 32768 : d > 1024 ? (d - 1024) << 1 : 0) - pf.u.t.gn) >> 5;
        g = (uint32_t)pf.u.t.gn;
        pb_ring_f(pf.u.t.p, &pf.yl, &pf.yr);
    }
    pf.yl = mulq16(pf.yl, g << 1);
    pf.yr = mulq16(pf.yr, g << 1);
    return 1;
}
/* (0.9.17) each block, stopped: KNOB 1's detents since the last (perf_scr) move the hand 1/48 beat each, within
 * what the loop holds */
static __attribute__((noinline)) void perf_scratch(void)
{
    uint32_t b = beat_samples() / 48u;
    int32_t d = clamp((int32_t)(perf_scr - pf.u.t.sc), -64, 64), r0, r1;
    r0 = (int32_t)(((pf.u.t.q >> 16) - 2u * pf.wr) & (PB_MAX - 1u)) - (int32_t)PB_MAX;   /* (behind the last written) */
    pf.u.t.sc = perf_scr;
    r1 = clamp(r0 + d * (int32_t)(b < 512u ? b : 512u), -(int32_t)(PB_MAX - 1024u), -4);
    pf.u.t.q += (uint32_t)(r1 - r0) << 16;          /* (on the platter's own lap of the ring) */
}

/* ----------------------------------------------------------------- FREEZE --- */
/* (0.9.17) a new grain (into the voice done, or the oldest): SIZE long at PITCH (SPRAY: some an octave either way, a
 * fifth up, a fourth down), from the scan (SPRAY scatters around it), within the capture; the scan on by 1/8 of the
 * time between grains, back and forth; DENS: 1 .. 3 at once (x100: ov), the level so they sum to about 1 */
static const int8_t FZ_JIT[4] = {12, -12, 7, -5};
static uint32_t frz_ov(void) { return 100u + 2u * (uint32_t)perf_gp[GX_FRZ][0]; }
static __attribute__((noinline)) void perf_frz_grain(void)
{
    grain_t *v = &pf.u.f.v[0];
    uint32_t i, r = pf.u.f.rnd = pf.u.f.rnd * 1664525u + 1013904223u, ov = frz_ov(),
             sp = (uint32_t)perf_gp[GX_FRZ][2], len = beat_samples() >> (5u - (uint32_t)perf_gp[GX_FRZ][3]), span, room, j;
    int32_t n = perf_gp[GX_FRZ][1], rate;
    for (i = 1; i < 4u; i++)
        if (pf.u.f.v[i].x > v->x)
            v = &pf.u.f.v[i];
    if (((r >> 24) * 100u >> 8) < sp / 2u)
        n += FZ_JIT[(r >> 22) & 3u];
    rate = (int32_t)semi_q16(n);
    if (len < 256u)
        len = 256u;
    pf.u.f.iv = len * 100u / ov;
    pf.u.f.gain = ov > 150u ? (int32_t)(4915200u / ov) : 32768;   /* (1.5 / the overlap: their windows' mean is 2/3) */
    if ((len >> 4) * ((uint32_t)rate >> 12) + 64u > pf.u.f.c)   /* (what it reads: within the capture) */
        len = ((pf.u.f.c - 64u) << 4) / ((uint32_t)rate >> 12);
    v->dx = 65536u / len;
    len = 65535u / v->dx + 1u;                      /* (as the window counts it) */
    span = (len >> 4) * ((uint32_t)rate >> 12) + 2u;
    room = span + 48u < pf.u.f.c ? pf.u.f.c - span - 48u : 0u;
    j = room * sp / 100u;
    if (pf.u.f.scan > room)
        pf.u.f.scan = room;
    i = pf.u.f.scan + (((r >> 6) & 0xFFFFu) * j >> 16);
    i = i > j / 2u ? i - j / 2u : 0u;
    v->pos = (16u + (i < room ? i : room)) << 16;
    v->rate = (uint32_t)rate;
    v->x = 0;
    j = pf.u.f.iv / 8u + 1u;                         /* (the scan: 1/8 of the speed, back and forth) */
    if (pf.u.f.sd > 0)
        pf.u.f.scan += j;
    else
        pf.u.f.scan = pf.u.f.scan > j ? pf.u.f.scan - j : 0u;
    if (pf.u.f.scan >= room || !pf.u.f.scan)
        pf.u.f.sd = pf.u.f.scan ? -1 : 1;
}
/* the grains' sum at the next even sample (S(t + 1), into nl nr), each on by two samples: at 22.05 kHz, the loop's rate */
static __attribute__((noinline)) void perf_frz_eval(void)
{
    uint32_t i;
    int32_t al = 0, ar = 0;
    if ((pf.u.f.fs += 2u) >= pf.u.f.iv) {
        pf.u.f.fs = 0;
        perf_frz_grain();
    }
    for (i = 0; i < 4u; i++) {
        grain_t *v = &pf.u.f.v[i];
        if (v->x < 65536u) {
            int32_t gl, gr, w = (int32_t)((v->x * (65536u - v->x)) >> 15);
            pb_ring_f(v->pos, &gl, &gr);
            al += (gl * w) >> 15;
            ar += (gr * w) >> 15;
            v->pos += 2u * v->rate;
            v->x += 2u * v->dx;
        }
    }
    pf.u.f.nl = mulq16(al, (uint32_t)pf.u.f.gain << 1);
    pf.u.f.nr = mulq16(ar, (uint32_t)pf.u.f.gain << 1);
}
/* FREEZE on one sample: recording the capture (live), then the grains: their sum on the even samples, the mean of the
 * two around it on the odd ones (half the work) */
static __attribute__((noinline)) int perf_frz(int32_t l, int32_t r)
{
    if (!pf.state) {
        uint32_t i, ov;
        pf.wr = pb_rec(pf.wr, l, r);
        if (2u * pf.wr < pf.u.f.c)
            return 0;
        ov = frz_ov() / 100u;
        pf.state = 1;                               /* frozen: DENS grains at once, staggered (v[0] .. in turn) */
        for (i = 0; i < ov; i++)
            perf_frz_grain();
        for (i = 0; i < ov; i++)
            pf.u.f.v[i].x = i * (6553600u / frz_ov());
        pf.u.f.fs = 0;
        pf.u.f.ph = 1;
        perf_frz_eval();
    }
    if ((pf.u.f.ph ^= 1u) != 0u) {                  /* (odd: S(t + 1), and the mean) */
        int32_t pl = pf.u.f.nl, pr = pf.u.f.nr;
        perf_frz_eval();
        pf.yl = (pl + pf.u.f.nl) >> 1;
        pf.yr = (pr + pf.u.f.nr) >> 1;
    } else {
        pf.yl = pf.u.f.nl;
        pf.yr = pf.u.f.nr;
    }
    return 1;
}

/* ---------------------------------------------------------------- SHUFFLE --- */
/* (0.9.17) ADPCM, 4 bits, IMA's way (its index moves, its codes) with its steps 2^(1/8) apart (IMA's: 1.1), 8 .. 29696:
 * from 8 numbers, not 89 */
static const uint8_t AD_M[8] = {16, 17, 19, 21, 23, 25, 27, 29};
static const int8_t AD_IDX[8] = {-1, -1, -1, -1, 2, 4, 6, 8};
static int32_t ad_stp(uint32_t ix) { return (int32_t)((uint32_t)AD_M[ix & 7u] << (ix >> 3)) >> 1; }
/* the value after code c from p (the coder and the reader alike), the step index on; (unlike IMA) codes 0 and 8 move
 * nothing, so silence stays 0 (else +-1 forever: the master would never rest, fx.c mix_hush) */
static int32_t ad_next(uint32_t c, int32_t p, uint8_t *ix)
{
    int32_t s = ad_stp(*ix), d = c & 7u ? s >> 3 : 0;
    if (c & 4u) d += s;
    if (c & 2u) d += s >> 1;
    if (c & 1u) d += s >> 2;
    *ix = (uint8_t)clamp((int32_t)*ix + AD_IDX[c & 7u], 0, 95);
    return clamp(c & 8u ? p - d : p + d, -32768, 32767);
}
/* the coder: x (16 bits) into the ring's next nibble */
static void ad_put(int32_t x)
{
    uint32_t w = pf.u.s.ew, c = 0;
    int32_t d = x - pf.u.s.ep, s = ad_stp(pf.u.s.ei);
    uint8_t *b = (uint8_t *)sl_buf + (w >> 1);
    if (d < 0) {
        c = 8u;
        d = -d;
    }
    if (d >= s) { c |= 4u; d -= s; }
    if (d >= s >> 1) { c |= 2u; d -= s >> 1; }
    if (d >= s >> 2) c |= 1u;
    pf.u.s.ep = ad_next(c, pf.u.s.ep, &pf.u.s.ei);
    *b = (uint8_t)(w & 1u ? (*b & 0x0Fu) | c << 4 : (*b & 0xF0u) | c);
    pf.u.s.ew = w + 1u < AD_N ? w + 1u : 0u;
}
/* a reader's sample (the level: x 2), then on by its rate */
static int32_t shr_out(shr_t *q)
{
    int32_t y = q->a + (((q->b - q->a) * (int32_t)(q->fr >> 1)) >> 15);
    if ((q->fr += q->rate) >= 65536u) {
        uint32_t w = q->pos;
        q->fr -= 65536u;
        q->a = q->b;
        q->b = (int16_t)ad_next((uint32_t)((const uint8_t *)sl_buf)[w >> 1] >> (w & 1u ? 4 : 0) & 15u, q->b, &q->idx);
        q->pos = (uint16_t)(w + 1u < AD_N ? w + 1u : 0u);
    }
    return 2 * y;
}
/* the reader from the start of the 1/16 noted in slot j (j > 31: live), the one before fading out */
static __attribute__((noinline)) void perf_shuf_go(uint32_t j, uint32_t rate)
{
    shr_t *c = &pf.u.s.r[0];
    const int16_t *t = AD_TBL + 3u * (j & 31u);
    if (j > 31u && c->live)
        return;
    pf.u.s.r[1] = *c;
    pf.u.s.xn = SL_RAMP;
    c->live = j > 31u;
    c->pos = (uint16_t)t[0];
    c->a = c->b = t[1];
    c->idx = (uint8_t)t[2];
    c->fr = 0;
    c->rate = rate;
}
/* each 1/16: the coder noted (its slot); a chunk's start: live, or one of the last bar's (the same place in the beat
 * half the time), at its speed (SPEED: half, double, its first half twice) */
static __attribute__((noinline)) void perf_shuf_step(void)
{
    uint32_t gs = pf.gs, ch = 4u >> perf_gp[GX_SHUF][1], s16 = pf.P / 2u ? pf.P / 2u : 1u, A = (uint32_t)perf_gp[GX_SHUF][0],
             h, thr, dm, d, k;
    int16_t *t = AD_TBL + 3u * (gs & 31u);
    t[0] = (int16_t)pf.u.s.ew;
    t[1] = (int16_t)pf.u.s.ep;
    t[2] = (int16_t)pf.u.s.ei;
    pf.u.s.nrec++;
    pf.gph = 0;
    if (gs % ch)
        return;
    pf.u.s.cu = 0;
    h = pf_hash((gs & 31u) + 0x3C7u + ((uint32_t)(perf_gp[GX_SHUF][3] - 1) << 5));
    thr = !(gs & 15u) ? A * A / 100u * A / 100u : !(gs & 3u) ? A * A / 100u : A;
    dm = (AD_N - 2048u) / s16;                      /* (how far back it may read: the bar, what the ring holds) */
    dm = dm > ch + 16u ? 16u : dm > ch ? dm - ch : 0u;
    dm = dm < pf.u.s.nrec - 1u ? dm : pf.u.s.nrec - 1u;
    pf.u.s.kind = SK_NORM;
    if (((h >> 24) * 100u >> 8) >= thr || dm < ch) {
        perf_shuf_go(32u, 0);
        return;
    }
    d = ch < 4u && dm >= 4u && (h & 0x100u) ? 4u * (1u + (h >> 9) % (dm / 4u)) : ch * (1u + (h >> 9) % (dm / ch));
    k = (((h >> 16) & 0xFFu) * 100u >> 8) < (uint32_t)perf_gp[GX_SHUF][2] ? 1u + (h >> 13) % 3u : SK_NORM;
    pf.u.s.kind = (uint8_t)k;
    pf.u.s.j0 = (uint8_t)((gs - d) & 31u);
    pf.u.s.mid = ch * pf.P / 2u;
    perf_shuf_go(pf.u.s.j0, k == SK_HALF ? 16384u : k == SK_DBL ? 65536u : 32768u);
}
/* SHUFFLE on one sample: the pair coded (mono, 22.05 kHz), the reader (or live), the one before fading out */
static __attribute__((noinline)) int perf_shuf(int32_t l, int32_t r)
{
    shr_t *c = &pf.u.s.r[0];
    if (pf.odd) {
        ad_put(clamp((pf.hl + l + r) >> 3, -32768, 32767));
        pf.odd = 0;
    } else {
        pf.hl = l + r;
        pf.odd = 1;
    }
    if (c->live) {
        pf.yl = l;
        pf.yr = r;
    } else {
        pf.yl = pf.yr = shr_out(c);
    }
    if (pf.u.s.xn) {
        shr_t *o = &pf.u.s.r[1];
        int32_t g = (int32_t)pf.u.s.xn--, ol = l, orr = r;
        if (!o->live)
            ol = orr = shr_out(o);
        pf.yl += ((ol - pf.yl) * g) >> SL_RAMP_LOG2;
        pf.yr += ((orr - pf.yr) * g) >> SL_RAMP_LOG2;
    }
    if (++pf.u.s.cu == pf.u.s.mid && pf.u.s.kind >= SK_DBL)   /* (DOUBLE, STUTTER: again from the chunk's start) */
        perf_shuf_go(pf.u.s.j0, c->rate);
    if (++pf.gph >= pf.P) {
        pf.gs++;
        perf_shuf_step();
    }
    return 1;
}

/* the glitcher on one sample (BM_GLT ..): 1 = it plays (pf.yl yr), 0 = live. (A table: each stays out of perf_buf on
 * any compiler, as on the target, where they are noinline) */
static int (*const PERF_RING[4])(int32_t, int32_t) = {perf_glt, perf_tape, perf_shuf, perf_frz};

/* the buffer effect faded out: the next one, or the buffer back. Once per effect, out of the per-sample loop
 * (noinline: its calls stay out of perf_block's loop code) */
static __attribute__((noinline)) void perf_buf_done(void)
{
    if (pf.next != PF_N) {
        perf_buf_start(pf.next);
    } else {
        pf.mode = BM_NONE;
        if (sl_lent) {
            sl_lent = 0;
            perf_drop_slicer();
        }
    }
}

/* the buffer effect on one sample: the share pf.w of what it plays, the rest live */
static __attribute__((noinline)) void perf_buf(int32_t *pl, int32_t *pr)   /* (0.9.14: out of perf_block's loop) */
{
    int32_t l = *pl, r = *pr, yl = 0, yr = 0, on = 0;
    if (pf.mode == BM_LOOP) {
        if (!pf.state) {
            pf.wr = pb_rec(pf.wr, l, r);
            if (++pf.rp >= pf.len) {                /* (rp: samples recorded) */
                pf.state = 1;
                pf.rp = 0;
            }
        } else {
            pb_loop(pf.rp, pf.len, pf.wr, &yl, &yr);
            if (pf.xn) {                            /* the loop it switched from, fading out */
                int32_t ol, orr, g = (int32_t)pf.xn;
                pb_loop(pf.xrp, pf.xlen, pf.wr, &ol, &orr);
                yl += (ol * g) >> SL_RAMP_LOG2;
                yr += (orr * g) >> SL_RAMP_LOG2;
                pf.xn--;
                if (++pf.xrp >= pf.xlen)
                    pf.xrp = 0;
            }
            if (++pf.rp >= pf.len)
                pf.rp = 0;
            on = 1;
        }
    } else {
        if ((on = PERF_RING[(pf.mode - BM_GLT) & 3u](l, r)) != 0) {   /* (0.9.14) GLITCH, TAPE; (0.9.17) SHUFFLE, FREEZE */
            yl = pf.yl;
            yr = pf.yr;
        }
    }
    pf.tw = on && pf.src != PF_N ? 32768 : 0;
    pf.w += clamp(pf.tw - pf.w, -SL_SLOPE, SL_SLOPE);
    if (pf.w) {
        *pl = pf_mix(l, yl, pf.w);
        *pr = pf_mix(r, yr, pf.w);
    } else if (pf.src == PF_N) {                    /* faded out: the next one, or the buffer back */
        perf_buf_done();
    }
}

static __attribute__((noinline)) void perf_switch(uint32_t a) { perf_buf_select(perf_pick(a)); }

/* after the master level: the buffer effect, CRUSH on l / r (stereo, Q15) */
static __attribute__((noinline)) void perf_block(int32_t *bl, int32_t *br, uint32_t n)
{
    uint32_t i, a = pf.act0, sh, hold;
    int32_t ct = pf.ss[0];
    sh = 4u + (uint32_t)(ct * 7) / 32768u;
    hold = sh - 3u;
    if (pf.split)
        perf_switch(a);
    if (pf.mode == BM_TAPE && pf.state)             /* (0.9.17) stopped: the hand (KNOB 1) */
        perf_scratch();
    for (i = 0; i < n; i++) {
        int32_t l = bl[i], r = br[i];
        if (i == pf.split) {
            a = pf.act;
            perf_switch(a);
        }
        if (pf.mode != BM_NONE)
            perf_buf(&l, &r);
        {   /* CRUSH: fewer bits, a held sample (its sweep: both, and its mix) */
            pf.cw += clamp(ct - pf.cw, -SL_SLOPE, SL_SLOPE);
            if (pf.cw) {
                if (++pf.cn >= hold) {
                    pf.cn = 0;
                    pf.chl = ((l >> sh) << sh) + (1 << (sh - 1u));
                    pf.chr = ((r >> sh) << sh) + (1 << (sh - 1u));
                }
                l = pf_mix(l, pf.chl, pf.cw);
                r = pf_mix(r, pf.chr, pf.cw);
            }
        }
        bl[i] = l;
        br[i] = r;
    }
    pf.busy = pf.act || pf.mode != BM_NONE || pf.w || pf.cw || pf.td || pf.mute || pf.sw[0] || pf.sw[1];
}
