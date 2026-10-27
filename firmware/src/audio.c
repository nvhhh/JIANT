/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments
 * Portions Copyright (C) 2026 Juan Ignacio Antonucci (Juan Jiant) */
/* I2S output (ALNK0 -> external codec) and the
 * audio ISR: per half buffer, blocks of CTL samples: mix_block (fx.c: events ->
 * each synth part -> dist -> level / pan -> sends -> buses -> master) -> USB audio (uac_tap) -> the metronome's
 * click (click.c, the DAC only) -> 24-bit stereo. */
/* registers: hal/fm1_audio.h */
#define HALF_WORDS (HALF_FRAMES * 2u)
#define DAC_TICKS 544u            /* TIMER4 ticks per I2S frame (24 MHz / 44,117.6 Hz) */
#define CPU_AVG (4096u / HALF_FRAMES)   /* halves the load meter averages: ~93 ms whatever the half */
#define OUT_SHIFT 7               /* Q15 -> 24-bit, -6 dBFS ceiling (M0f ran clean at -18 dBFS) */

static int32_t abuf[2u * HALF_WORDS] __attribute__((aligned(4)));

/* diagnostics, kept across resets and UBOOT entry: read with `fm1t memr` */
#define DBG_MAGIC 0x44424731u                       /* "DBG1" */
struct felucca_dbg {
    uint32_t magic, halves, max_us, nested, in_audio, late, timer_irqs, ui_frames;
    uint32_t last_us, cpu_q8, boots;
    uint32_t stage, page, home;           /* where the main loop is (breadcrumbs) */
    uint32_t prev_stage, prev_page, prev_home, prev_rst, prev_frames;   /* as found at boot */
} felucca_dbg __attribute__((section(".noinit")));
static volatile uint32_t audio_halves, audio_max_us;
static volatile uint32_t t5_nested_ticks;              /* TIMER4 ticks TIMER5 spent nested in this ISR (main.c) */
static uint32_t audio_cpu_rem;
static uint32_t audio_last_half = 2u, audio_lost;    /* (0.8.9) halves never rendered: a render past two halves (console) */                         /* keep the fractional IIR step: no low-load bias */
#define SCOPE_N 512u
static int16_t scope_buf[SCOPE_N];
static uint32_t scope_w;

static __attribute__((noinline)) void prf_block(void)   /* core 0, a block's end (audio.c audio_block) */
{
    uint32_t k, m = mc_mask_last;
    if (prf_on) {
        for (k = 0; k < PRF_NB; k++) {
            prf_cur.sum[k] += prf_blk[k];
            if (prf_blk[k] > prf_cur.max[k])
                prf_cur.max[k] = prf_blk[k];
        }
        for (k = 0; k < NPART; k++)
            prf_cur.c1[k] += (m >> k) & 1u;
        prf_cur.blocks++;
    }
    for (k = 0; k < PRF_NB; k++)
        prf_blk[k] = 0;
}

/* core 0, a half's end (fm1_alnk0_irq): all its ticks, TIMER5's in it, when it began; once a second the window kept */
static __attribute__((noinline)) void prf_half(uint32_t all, uint32_t nested, uint32_t t0)
{
    uint32_t d = t0 - prf.last - HALF_FRAMES * 544u, r = all - nested, w;
    prf.last = t0;
    if (!prf_on)
        return;
    if (d > prf_cur.lat && d < 2400000u)               /* (started past its period: IRQs off, a flash write) */
        prf_cur.lat = d;
    prf_cur.sum[PRF_IRQ] += nested;
    prf_cur.sum[PRF_HALF] += r;
    if (nested > prf_cur.max[PRF_IRQ]) prf_cur.max[PRF_IRQ] = nested;
    if (r > prf_cur.max[PRF_HALF]) prf_cur.max[PRF_HALF] = r;
    if (++prf.halves < PRF_HALVES)
        return;
    prf.halves = 0;
    w = song.playing ? 1u : 0u;
    prf_win[w] = prf_cur;
    prf_have |= (uint8_t)(1u << w);
    memset(&prf_cur, 0, sizeof prf_cur);
    prf_seq++;
}

static void audio_block(int32_t *out, uint32_t n)       /* mix (fx.c), then Q15 -> 24 bit */
{
    uint32_t i, t0 = PRF_T(), t1;
#if FELUCCA_UAC && defined(UAC_UIN)
    if (USB_DUPLEX)                                     /* (0.7.5) MENU > USB IN+OUT: the host's audio, for the mix */
        uac_in_fill(n);
    else
        uin_on = 0;
#endif
    t1 = PRF_T();
    mix_block(out, n);
    t0 += PRF_T() - t1;                                 /* (0.9.14) PROFILE: USB and the output, the mix left out */
#if FELUCCA_UAC
#ifdef UAC_TAP8
    if (fx_stems)                                       /* (0.7.1) MENU > USB STEMS: 8 channels (fx.c stem_buf) */
        uac_tap8(n);
    else
#endif
        uac_tap(out, n);                                /* the USB audio input: the same master output */
#endif
    if (fx_usb_fixed)                                   /* USB LEVEL FIXED: USB took the full level, the DAC */
        usb_fixed_dac(out, n);                          /* (speaker, headphones) gets MASTER's (fx.c) */
    for (i = 1; i < n; i += 2u)                         /* the HOME scope: the music, before the click */
        scope_buf[scope_w++ & (SCOPE_N - 1u)] = (int16_t)out[2u * i];
    click_render(out, n);                               /* the metronome (click.c): the DAC only, not USB */
    for (i = 0; i < n; i++) {
        out[2u * i] *= 1 << OUT_SHIFT;
        out[2u * i + 1u] *= 1 << OUT_SHIFT;
    }
    prf_blk[PRF_IO] = PRF_T() - t0;
    prf_block();
}

/* Overload: two halves in a row above 80 % (0.6.15: was 85, too late: a click first, the UI starved) (the render plus TIMER5 nested in it), fade one voice over the
 * next block, and one more each half while it lasts. Keep each part's
 * lowest POLY note and MONO / LEGATO / UNISON lead, as the shared budget does. */
static volatile uint8_t shed_req;
static uint8_t shed_over;                              /* bit k: the half k halves ago was over 80 % */
#define SHED_TICKS ((HALF_FRAMES * 1000000u / FS) * 80u / 100u * FM1_TICKS_PER_US)   /* 80 % of a half */
static uint32_t shed_count;

/* end of a half: all = its ticks, TIMER5 nested in it included -> the render alone in us. The deadline sees
 * both; shed after 2 overloaded halves in a row (out of line: keeps the ISR's render loops as they were) */
#define TIGHT_ON ((HALF_FRAMES * 1000000u / FS) * 65u / 100u * FM1_TICKS_PER_US)    /* (0.6.16) the load budget (0.6.17: 65 %, was 70) */
#define TIGHT_OFF ((HALF_FRAMES * 1000000u / FS) * 60u / 100u * FM1_TICKS_PER_US)
#define TIGHT_ON_ECO ((HALF_FRAMES * 1000000u / FS) * 50u / 100u * FM1_TICKS_PER_US)   /* (0.6.70) LOAD ECO: the budget */
#define TIGHT_OFF_ECO ((HALF_FRAMES * 1000000u / FS) * 45u / 100u * FM1_TICKS_PER_US)  /* sooner: room kept for the peaks */
#define TIGHT_ON_1C ((HALF_FRAMES * 1000000u / FS) * 55u / 100u * FM1_TICKS_PER_US)    /* (0.9.12) one core: the budget */
#define TIGHT_OFF_1C ((HALF_FRAMES * 1000000u / FS) * 50u / 100u * FM1_TICKS_PER_US)   /* sooner (no core 1 for the peaks) */
#if FELUCCA_DUAL_CORE
#define TIGHT_1C() (!fm1_mc_on)                        /* (MENU > SYSTEM > CPU 1 CORE, REC at power-on, core 1 given up) */
#else
#define TIGHT_1C() 0                                   /* (the host: as before) */
#endif
static uint8_t tight_calm;                             /* halves in a row under 60 % */
/* (0.9.13) the load governor (voice.c gov_cap, gov_lite), a half at a time: over the budget 3 halves in a row (0.9.14:
 * was 6), a voice less for all parts (4 at least: the song thins out, its oldest released voices first, voice_room);
 * (0.9.14) past GOV_HOT (75 %, ahead of the guard's 80 %) a voice less every half, and at 4 still past it 8 halves, LOAD
 * LITE of its own; a shed (the guard at 80 %) two less; under the budget 172 halves (0.5 s), one back; two sheds in 344
 * halves (1 s), LOAD LITE of its own (CLOUD / SHIMMER as ROOM, RESO as SPRING, the GRAIN delay plain, FM6 two voices)
 * until 5 s under the budget */
#define GOV_HOT ((HALF_FRAMES * 1000000u / FS) * 75u / 100u * FM1_TICKS_PER_US)
#define GOV_HOT_ECO ((HALF_FRAMES * 1000000u / FS) * 62u / 100u * FM1_TICKS_PER_US)
#define GOV_HOT_1C ((HALF_FRAMES * 1000000u / FS) * 70u / 100u * FM1_TICKS_PER_US)
static uint16_t gov_hot, gov_calm, gov_win, gov_lcalm;
static uint8_t gov_sheds, gov_floor;
static void gov_tick(int over, int hot, int under, int shed)
{
    uint32_t c = gov_cap > NVOICE + 4u ? NVOICE + 4u : gov_cap;
    if (shed) {
        c = c > 6u ? c - 2u : 4u;
        if (!gov_win) gov_sheds = 0;
        gov_win = 344u;
        if (++gov_sheds >= 2u) { gov_lite = LOAD_LITE; gov_lcalm = 0; }
    }
    if (gov_win) gov_win--;
    gov_floor = hot ? gov_floor : 0;
    if (over || hot) {
        gov_calm = 0;
        if (hot || ++gov_hot >= 3u) {
            gov_hot = 0;
            if (c > 4u) c--;
            else if (hot && ++gov_floor >= 8u) { gov_lite = LOAD_LITE; gov_lcalm = 0; }
        }
    } else {
        gov_hot = 0;
        if (under && ++gov_calm >= 172u) { gov_calm = 0; if (c < NVOICE + 4u) c++; }
    }
    if (gov_lite && (under ? ++gov_lcalm >= 1720u : (gov_lcalm = 0, 0))) gov_lite = 0;
    gov_cap = (uint8_t)c;
}
static __attribute__((noinline)) uint32_t shed_check(uint32_t all)
{
    uint32_t r = all - t5_nested_ticks;                /* (0.6.19) the budget on the render alone: a key scan or
                                                         * USB burst nested in it is not the sound's */
    uint32_t eco = sys_load == LOAD_ECO, one = TIGHT_1C();
    if (r > (eco ? TIGHT_ON_ECO : one ? TIGHT_ON_1C : TIGHT_ON)) {   /* (voice.c load_tight: no voice added while set) */
        load_tight = 1;
        tight_calm = 0;
    } else if (load_tight && r < (eco ? TIGHT_OFF_ECO : one ? TIGHT_OFF_1C : TIGHT_OFF) && ++tight_calm >= 16u) {
        load_tight = 0;
    }
    shed_over = (uint8_t)(shed_over << 1 | (all > SHED_TICKS));
    if ((shed_over & 3u) == 3u)                        /* (0.6.19: two in a row again, as before 0.6.17: one
                                                         * spike shed a voice every 2.9 ms, tails cut, clicks) */
        shed_req = 1;
    gov_tick(r > (eco ? TIGHT_ON_ECO : one ? TIGHT_ON_1C : TIGHT_ON), r > (eco ? GOV_HOT_ECO : one ? GOV_HOT_1C : GOV_HOT),
             r < (eco ? TIGHT_OFF_ECO : one ? TIGHT_OFF_1C : TIGHT_OFF), shed_req);
    return (all - t5_nested_ticks) / FM1_TICKS_PER_US;
}

static void shed_voice(void)
{
    uint32_t p, i;
    voice_t *best = 0;
    for (p = 0; p < NPART; p++)
        for (i = 0; i < NVOICE; i++) {
            voice_t *v = &trk[p].v[i];
            if (v->active && !v->gate && v->stage != 4u && (!best || v->env < best->env))
                best = v;
        }
    if (best) {
        voice_kill(best);
        shed_count++;
        return;
    }
    for (p = 0; p < NPART; p++) {
        const track_t *t = &trk[p];
        uint32_t mode = trk_vmode(t), low = mode == V_POLY ? lowest_held(t) : 0u;
        for (i = 0; i < NVOICE; i++) {
            voice_t *v = &trk[p].v[i];
            if (v->active && v->gate && v->stage != 4u &&
                ((mode == V_POLY && i != low) || (mode != V_POLY && i > 0u))) {
                if (!best || v->age < best->age)
                    best = v;
            }
        }
    }
    if (best) {
        voice_kill(best);
        shed_count++;
    }
}

void fm1_alnk0_irq(void)                       /* via isr_alnk0 (hal/fm1_isr.S) */
{
    uint8_t p;
    uint32_t t0;
    felucca_dbg.in_audio = 1;                   /* first: TIMER5 nests from here on (main.c) */
    p = fm1_audio_pending();
    t0 = fm1_ticks();
    t5_nested_ticks = 0;
    fm1_audio_ack_aux(p);
    if (p & FM1_AUDIO_HALF) {
        uint32_t half = fm1_audio_free_half(), b, us;
        int32_t *o = &abuf[half * HALF_WORDS];
        fm1_audio_ack_half();                           /* (0.8.9) first: a flip during a late render stays pending */
        if (half == audio_last_half)                    /* and this ISR runs again at once for the other half (until */
            audio_lost++;                               /* 0.8.8 the ack after the render cleared it: every late half */
        audio_last_half = half;                         /* lost the next one, its stale audio played again and USB short) */
        if (shed_req) {
            shed_req = 0;
            shed_voice();
        }
#if FELUCCA_UAC
        uac_render_start();
#endif
        for (b = 0; b < HALF_FRAMES; b += CTL) {
#ifdef FM1_INPUT_LAT
            kb_out_tick = t0 + (HALF_FRAMES + b) * DAC_TICKS;   /* when this block plays (seq.c kb_lat) */
#endif
            audio_block(o + 2u * b, CTL);
        }
#if FELUCCA_UAC
        uac_render_end();                               /* (0.8.9) the half's 48 kHz resampling: to core 1, off the render */
#endif
        audio_halves++;
        b = fm1_ticks() - t0;
        prf_half(b, t5_nested_ticks, t0);              /* (0.9.14) PROFILE */
        us = shed_check(b);
        if (us > audio_max_us)
            audio_max_us = us;
        {
            uint32_t load = song.cpu_q8 * (CPU_AVG - 1u) + (us * 256u) / (HALF_FRAMES * 1000000u / FS) + audio_cpu_rem;
            song.cpu_q8 = load / CPU_AVG;
            audio_cpu_rem = load % CPU_AVG;
        }
        if (fm1_audio_free_half() != half)
            felucca_dbg.late++, prf_cur.late++;         /* the DMA moved on while we rendered (0.8.9: caught up at
                                                         * once, its half pending; a click only past ~1.7 halves) */
        {   /* (0.6.17) MENU > ABOUT's numbers (voice.c load_diag) */
            uint32_t pc = us * 100u / (HALF_FRAMES * 1000000u / FS);
            if (pc > load_diag.peak) load_diag.peak = pc;
            load_diag.late = felucca_dbg.late;
            load_diag.shed = shed_count;
        }
        felucca_dbg.halves++;
        felucca_dbg.last_us = us;
        if (us > felucca_dbg.max_us)
            felucca_dbg.max_us = us;
        felucca_dbg.cpu_q8 = song.cpu_q8;
    }
    felucca_dbg.in_audio = 0;
}
extern void isr_alnk0(void);

static void audio_init(void)                   /* codec and ALNK0 bring-up (fm1_audio.h) */
{
    uint32_t i;
    for (i = 0; i < 2u * HALF_WORDS; i++)
        abuf[i] = 0;
    fm1_audio_init(abuf, HALF_WORDS, isr_alnk0, 3);
}
