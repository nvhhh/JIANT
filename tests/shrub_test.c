/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments */
/* SHRUB test (src/eng_shrub.c, src/shrub_tables.c) on the host, through hostsim.c as voice_test.c.
 *   gcc -O2 -w -Ibuild/gen -Ifirmware/src -o build/host/shrub_test tests/shrub_test.c -lm
 *   build/host/shrub_test
 * 1. every preset renders finite samples, none at or above full scale, and sustains a held note (1 s);
 * 2. WT 0 against WT 127 gives different spectra (the first difference's energy against the signal's);
 * 3. RES 127 with each FILT, CUT 127 and DRIVE 127, over 10 s at a low and a high note: no NaN (ints: no overflow),
 *    no runaway (peak under the limit at every second), still sounding at the end;
 * 4. SUB 127 adds energy an octave below the note (Goertzel at f / 2), SUB 0 does not.
 * Float math here is the host test's only; the firmware code is integer. */
#define main hostsim_main
#include "hostsim.c"
#undef main
#include "../firmware/src/shrub_tables.c"
#include "../firmware/src/eng_shrub.c"

#define SR 44100u
#define NOTE_LO 36
#define NOTE_HI 84
#define FULL 32767                  /* a sample's full scale in the voice's units (before the output stage) */
/* the bound on a voice's output: the filters' soft knee caps the signal at 16000 + 16384 (softclip), and the
 * voice's output is voice_amp(y << 1) << 1 = 2 * 0.73 * 2 * y: at most about 95000 < 3 * FULL. ANALOG's RES 127
 * reaches 78680 on the same bound; SHRUB's presets sit at 8000..33000 (the firmware's level convention). */
#define LIMIT (3 * FULL)

static int bad;
static void check(const char *what, int ok)
{
    printf("shrub: %-96s %s\n", what, ok ? "ok" : "FAIL");
    bad += !ok;
}

static track_t TR;
static voice_t VO;
static vmod_t MO;
static int32_t BUF[10 * SR];

/* a fresh voice playing `note` with the 8 parameters e (P_E0..P_E7 order) */
static void setup(const int8_t *e, int note)
{
    uint32_t i;
    memset(&TR, 0, sizeof TR);
    for (i = 0; i < 8; i++)
        TR.p[P_E0 + i] = e[i];
    memset(&VO, 0, sizeof VO);
    VO.note = (uint8_t)note;
    VO.vel = 100;
    VO.pitch16 = (int32_t)note * 16;
    memset(&MO, 0, sizeof MO);
    MO.inc = pitch_inc((uint32_t)VO.pitch16);
    MO.pitch16 = VO.pitch16;
    MO.amp0 = MO.amp1 = 32767;
    MO.shape = 64 << 8;                       /* no SHAPE offset: the matrix's neutral */
    ENG_SHRUB.note_on(&TR, &VO);
}

/* n samples (a multiple of CTL) of the voice into out, one control block at a time */
static void render(int32_t *out, uint32_t n)
{
    uint32_t b;
    memset(out, 0, n * sizeof *out);
    for (b = 0; b < n / CTL; b++)
        ENG_SHRUB.render(&TR, &VO, out + b * CTL, CTL, &MO);
}

static int32_t peak(const int32_t *x, uint32_t n)
{
    int32_t p = 0, i;
    for (i = 0; i < (int32_t)n; i++) {
        int32_t a = x[i] < 0 ? -x[i] : x[i];
        if (a > p)
            p = a;
    }
    return p;
}

static double rms(const int32_t *x, uint32_t n)
{
    double s = 0;
    uint32_t i;
    for (i = 0; i < n; i++)
        s += (double)x[i] * (double)x[i];
    return n ? sqrt(s / n) : 0.0;
}

/* sum of squared first differences over the sum of squares: high-frequency share */
static double hf_share(const int32_t *x, uint32_t n)
{
    double d = 0, s = 0;
    uint32_t i;
    for (i = 1; i < n; i++) {
        double a = (double)x[i] - x[i - 1];
        d += a * a;
        s += (double)x[i] * x[i];
    }
    return s > 0 ? d / s : 0.0;
}

/* amplitude of the frequency hz in x (Goertzel) */
static double tone(const int32_t *x, uint32_t n, double hz)
{
    double w = 2.0 * M_PI * hz / SR, c = 2.0 * cos(w), s0 = 0, s1 = 0, s2 = 0, re, im;
    uint32_t i;
    for (i = 0; i < n; i++) {
        s0 = x[i] + c * s1 - s2;
        s2 = s1;
        s1 = s0;
    }
    re = s1 - s2 * cos(w);
    im = s2 * sin(w);
    return 2.0 * sqrt(re * re + im * im) / n;
}

static void p_param(int8_t *e, const preset_t *pr)
{
    memcpy(e, pr->e, 8);
}

int main(void)
{
    uint32_t k, f, sr_n = SR;
    int8_t e[8];
    static const char *const names[3] = {"LADDER", "ACID", "SVF"};

    /* 1. presets */
    for (k = 0; k < ENG_SHRUB.npresets; k++) {
        const preset_t *pr = &ENG_SHRUB.presets[k];
        char what[128];
        int32_t pk;
        double tail;
        p_param(e, pr);
        setup(e, NOTE_LO + (int)(k % 3) * 12);
        render(BUF, sr_n);
        pk = peak(BUF, sr_n);
        tail = rms(BUF + sr_n - SR / 5, SR / 5);
        snprintf(what, sizeof what, "preset %-12s peak %5d (limit %d), tail rms %8.1f", pr->name, pk, LIMIT, tail);
        check(what, pk < LIMIT && tail > 1.0 && pk > 0);
    }

    /* 2. WT 0 against WT 127: the high-frequency share of the spectrum */
    {
        int8_t a[8] = {0, 0, 0, 0, 127, 0, 0, 0}, b[8] = {127, 0, 0, 0, 127, 0, 0, 0};
        double ha, hb;
        setup(a, 48);
        render(BUF, SR / 2);
        ha = hf_share(BUF, SR / 2);
        setup(b, 48);
        render(BUF, SR / 2);
        hb = hf_share(BUF, SR / 2);
        printf("shrub: (WT 0 hf share %.5f, WT 127 %.5f)\n", ha, hb);
        check("WT 0 against WT 127: the spectra differ (WT 127 at least 3x the high share)", hb > 3.0 * ha && ha > 0.0);
    }

    /* 3. RES 127 with each FILT, bounded over 10 s */
    for (f = 0; f < 3; f++) {
        uint32_t nt, ok = 1;
        for (nt = 0; nt < 2; nt++) {
            int note = nt ? NOTE_HI : NOTE_LO;
            int8_t c[8] = {64, 40, 10, 0, 127, 127, 127, (int8_t)f};
            int32_t worst = 0;
            uint32_t sec;
            setup(c, note);
            render(BUF, 10 * SR);
            for (sec = 0; sec < 10; sec++) {
                int32_t pk = peak(BUF + sec * SR, SR);
                if (pk > worst)
                    worst = pk;
            }
            if (worst >= LIMIT || rms(BUF + 9 * SR, SR) <= 1.0)
                ok = 0;
            printf("shrub:   %-6s RES 127 CUT 127 DRIVE 127 note %d: peak %d over 10 s\n", names[f], note, worst);
        }
        {
            char what[96];
            snprintf(what, sizeof what, "%-6s RES 127: bounded and still sounding after 10 s (low and high notes)", names[f]);
            check(what, ok);
        }
    }

    /* 4. SUB 127 adds energy an octave below the note */
    {
        int8_t off[8] = {0, 0, 0, 0, 127, 0, 0, 0}, on[8] = {0, 0, 0, 127, 127, 0, 0, 0};
        double a0, a1, f0 = 261.63 / 2.0;
        setup(off, 60);
        render(BUF, SR / 2);
        a0 = tone(BUF + SR / 10, SR / 2 - SR / 10, f0);
        setup(on, 60);
        render(BUF, SR / 2);
        a1 = tone(BUF + SR / 10, SR / 2 - SR / 10, f0);
        printf("shrub:   (at %.1f Hz: SUB 0 %.1f, SUB 127 %.1f)\n", f0, a0, a1);
        check("SUB 127 adds energy an octave below (at least 10x SUB 0 there, and audible)", a1 > 10.0 * a0 && a1 > 100.0);
    }

    printf("shrub: %s (%d failed)\n", bad ? "FAIL" : "all ok", bad);
    return bad ? 1 : 0;
}
