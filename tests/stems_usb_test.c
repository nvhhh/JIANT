/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments
 * Copyright (C) 2026 Juan Ignacio Antonucci (Juan Jiant) */
/* (0.9.8) MENU > USB STEMS / STEMS+IN over minutes: src/usb.c's 8- and 10-channel stream (uac_render_start, uac_tap8,
 * uac_packet) in the audio ISR's order against a simulated FM-1 and a host that reads it as the JIANT Stems app does
 * (android/stems: usbstream.c's isochronous URBs, each packet's buffer the endpoint's wMaxPacketSize, then
 * StemSplitter's frames), in ns:
 *   the FM-1     halves of HALF_FRAMES at 44,117.6 Hz (its clock is fast: the ring's fill climbs, packets of 46 come
 *                often), renders of a heavy song (~70 % of a half, ~4 % late, up to 131 %), TIMER5 every tick
 *                (UAC_POLL_HZ, 0.9.10; its period off by +-1667 ppm or none, 0..20 us late) nested in them queueing the
 *                next packet (uac_service) once TxPktRdy has cleared, its 20 ms stall count as ep4_tx's.
 *   the host     an IN token each 1 ms of its own clock (+-500 ppm), 30..40 us into the frame; the packet then on the
 *                wire (TxPktRdy still set) for its (bytes + 9) x 8 x 1.04 / 12 MHz (0.9.10: 920 bytes ~0.65 ms). A
 *                packet longer than wMaxPacketSize is babble, as an xHCI controller has it: the packet cut to
 *                wMaxPacketSize and flagged, the endpoint halted and restarted 1..3 ms later (no tokens meanwhile).
 * Each channel of each frame holds the frame's number (channel c: (f + 4099 c) & 0x7FFF), so the host sees exactly
 * which frame came, in which channel: every frame the FM-1 rendered must reach the app's lanes once, in order, every
 * channel in its own lane (no slip, no repeat, no channel moved), and no packet may be babble. 0.9.7's descriptor
 * (720 bytes for 8 channels, while the stream sends 46-frame packets of 736) is run too, to show what it did: blocks
 * dropped by the hundred, as in a field take (TOMA_03: T1..T6 and FX broken at ~120 block edges a second, the same
 * instants in every channel). Then starvation (renders of 150..190 % for 50 ms): the underrun repeats are made up
 * with the frame spread over a packet (uac_squeeze), not skipped: every channel continuous (a 441..1323 Hz sine each).
 * (0.9.10) 0.9.9's poll every 5th tick (2 kHz) is run too: the next packet could only be queued in the ~0.35 ms
 * between one leaving the wire and the next token, and whenever the poll's phase drifted into it tokens found none,
 * the ring overran and blocks were dropped in bursts in every channel (a STEMS+IN take's rhythmic glitch). */
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef HALF_FRAMES
#error "-DHALF_FRAMES=n (src/core.h; run_tests.sh passes it)"
#endif
#define RING_PUBLISH() __asm__ volatile("" ::: "memory")
#define FELUCCA_OTA 0
#define FELUCCA_CDC 1
#define FELUCCA_UAC 1
static void fm1_delay_ms(uint32_t ms) { (void)ms; }
#pragma GCC diagnostic ignored "-Wint-to-pointer-cast"
static uint8_t t_stems;                                  /* fx.c's fx_stems: 1 the 8 channels, 2 the 10 */
#define UAC_STEMS() t_stems
#define UAC_STEMS_SRC 1
#ifndef NPART
#define NPART 8                  /* (core.h: the tracks; usb.c uses it, this test does not include core.h) */
#endif
static int32_t stem_buf[NPART + 1][32], stem_fx[2][32];   /* [0..7] the tracks, [NPART]: STEMS+IN's host audio as mixed (usb.c) */
static int32_t uin_l[64], uin_r[64];
static volatile uint8_t uin_on;
#define UIN_SRC 1
#include "../firmware/src/usb.c"

static int fails;
static void check(const char *what, int ok)
{
    printf("%-112s %s\n", what, ok ? "ok" : "FAIL");
    fails += !ok;
}
static uint32_t lcg = 0x13579BDu;
static double rnd(void) { lcg = lcg * 1664525u + 1013904223u; return (lcg >> 8) / 16777216.0; }

static struct {
    double fs, ppm, load, p_late, late_max;
    double t5_ppm, t5_phase, poll_hz;                    /* TIMER5's period error, its first tick (ns), uac_service's rate */
    uint32_t maxp; int starve, sine;
} M;
static struct {
    uint64_t rendered, sent, pkts, babble, stall_ns, missed, tokens;
    uint64_t got, slips, lost, repeats, moved, partial, jumps, maxjump_x100;
} R;
static uint32_t nch;                                     /* channels: 8 or 10 */
static uint64_t f_render;                                /* the next frame the render makes */
static uint32_t pkt[UA_MAXF * UA_W], pkt_n;
static int queued;

/* ---- the render: a block of 32 frames into fx.c's stem buffers, tapped as the audio ISR does */
static double sine_ph[10];
static void render_block(void)
{
    uint32_t i, c;
    for (i = 0; i < 32u; i++, f_render++)
        for (c = 0; c < nch; c++) {
            int32_t v;
            if (M.sine) {                                /* (channel c: (c + 1) x 441 Hz / 3 .. ) */
                v = (int32_t)lrint(12000.0 * sin(sine_ph[c]));
                sine_ph[c] += 2 * M_PI * 441.0 * (1.0 + c / 4.0) / 44100.0;
            } else {
                v = (int32_t)((f_render + 4099u * c) & 0x7FFFu);
            }
            if (c < 6u) stem_buf[c][i] = v;
            else if (c < 8u) stem_fx[c - 6u][i] = v;
            else if (c == 8u) stem_buf[8][i] = v;          /* STEMS+IN's ninth channel: the host's audio as mixed (NPART) */
            else uin_l[i] = uin_r[i] = v;
        }
    uin_on = nch == 10u;
    uac_tap8(32);
    R.rendered += 32;
}

/* ---- the app: usbstream.c copies each packet's actual_length (whole frames: 0.9.8), StemSplitter splits the frames */
static int64_t last_f = -1;
static double last_v[10], last_d[10];
static int have_v;
static void app_frames(const uint8_t *b, uint32_t bytes)
{
    uint32_t fb = nch * 2u, n = bytes / fb, i, c;
    if (bytes % fb)
        R.partial++;                                     /* (a frame cut: dropped whole, the lanes stay aligned) */
    for (i = 0; i < n; i++) {
        int16_t v[10];
        for (c = 0; c < nch; c++)
            v[c] = (int16_t)(b[i * fb + 2u * c] | b[i * fb + 2u * c + 1u] << 8);
        R.got++;
        if (M.sine) {                                    /* continuity: no step past 1.3 x the sine's largest one */
            for (c = 0; c < nch; c++) {
                double d = v[c] - last_v[c], lim = 12000.0 * 2 * M_PI * 441.0 * (1.0 + c / 4.0) / 44100.0;
                if (have_v && fabs(d - last_d[c]) > 0.3 * lim + 3.0) {
                    R.jumps++;
                    if (fabs(d - last_d[c]) / lim * 100 > R.maxjump_x100) R.maxjump_x100 = (uint64_t)(fabs(d - last_d[c]) / lim * 100);
                }
                last_d[c] = have_v ? d : 0;
                last_v[c] = v[c];
            }
            have_v = 1;
            continue;
        }
        {
            int64_t f0 = (uint16_t)v[0], f;
            int bad = 0, z = 1;
            for (c = 0; c < nch; c++)
                z &= v[c] == 0;
            if (z && last_f < 0)
                continue;                                /* (the primed silence ahead of the first frame) */
            for (c = 1; c < nch; c++)
                bad |= (((uint32_t)f0 + 4099u * c) & 0x7FFFu) != (uint16_t)v[c];
            if (bad) { R.moved++; continue; }            /* (a channel not in its lane, or not a frame of the render) */
            if (last_f < 0) {
                last_f = f0;
                continue;
            }
            f = last_f + (((f0 - last_f) % 32768 + 32768) % 32768);   /* (the 15-bit count unwrapped) */
            if (f == last_f) R.repeats++;
            else if (f != last_f + 1) { R.slips++; R.lost += (uint64_t)(f - last_f - 1); }
            last_f = f;
        }
    }
}

/* ---- the host: an IN token; the packet on the wire until tx_end; babble (> wMaxPacketSize): cut, flagged, the
 * endpoint halted 1..3 ms */
static double stall_until, tx_end;
static void host_token(double t)
{
    uint32_t bytes;
    R.tokens++;
    if (t < stall_until)
        return;
    if (!queued) {
        R.missed++;
        return;
    }
    queued = 0;
    R.pkts++;
    R.sent += pkt_n;
    bytes = pkt_n * UA_FW * 4u;
    if (bytes > M.maxp) {
        double s = 1e6 + 2e6 * rnd();
        R.babble++;
        R.stall_ns += (uint64_t)s;
        stall_until = t + s;
        bytes = M.maxp;
    }
    tx_end = t + (bytes + 9u) * 8.0 * 1.04 / 12e6 * 1e9;   /* (sync, PID, CRC, EOP; ~4 % bit stuffing) */
    app_frames((const uint8_t *)pkt, bytes);
}

static void poll(double t)                             /* TIMER5's uac_service: TxPktRdy set while queued or on the wire */
{
    if (queued || t < tx_end) {
        if (uac.wait < 255u && ++uac.wait == (uint32_t)M.poll_hz / 50u && uac.flowing) {   /* (ep4_tx: 20 ms) */
            uac.flowing = 0;
            uac.stalls++;
        }
        return;
    }
    if (R.pkts)
        uac.flowing = 1;
    uac.wait = 0;
    pkt_n = uac_packet(pkt);
    queued = 1;
}

static void sim(double secs, double *now)
{
    double th = HALF_FRAMES * 1e9 / M.fs, tf = 1e6 * (1.0 + M.ppm * 1e-6), end = *now + secs * 1e9;
    static double next_flip, next_poll, next_sof, next_tok, isr_free, dl, blk_t[HALF_FRAMES / 32u];
    static uint32_t blocks, pending, starve_left;
    static double next_starve;
    static uint64_t tick;
    double tp = 1e9 / M.poll_hz * (1.0 + M.t5_ppm * 1e-6);
    if (*now == 0) {
        next_flip = th; next_sof = 0; next_tok = 30000; isr_free = 0; blocks = pending = 0;
        tick = 0;
        next_poll = M.t5_phase;
        next_starve = 3e9;
    }
    while (*now < end) {
        double tn = next_flip;
        if (blocks && blk_t[0] < tn) tn = blk_t[0];
        if (!blocks && pending && isr_free < tn) tn = isr_free > *now ? isr_free : *now;
        if (next_poll < tn) tn = next_poll;
        if (next_tok < tn) tn = next_tok;
        if (tn < *now) tn = *now;
        *now = tn;
        if (tn == next_flip) {
            pending = 1;
            next_flip += th;
            continue;
        }
        if (blocks && tn == blk_t[0]) {
            uint32_t i;
            render_block();
            for (i = 1; i < blocks; i++) blk_t[i - 1] = blk_t[i];
            if (!--blocks) {
                uac_render_end();
                isr_free = tn;
                (void)dl;
            }
            continue;
        }
        if (!blocks && pending && tn >= isr_free) {
            double d, u = rnd();
            uint32_t i;
            pending = 0;
            if (M.starve && *now >= next_starve) {
                starve_left = (uint32_t)(0.05e9 / th);
                next_starve += 5e9;
            }
            if (starve_left) {
                starve_left--;
                d = th * (1.5 + 0.4 * rnd());
            } else if (u < M.p_late) {
                d = th * (1.0 + (M.late_max - 1.0) * rnd());
            } else {
                d = th * (M.load * (0.75 + 0.5 * rnd()));
                if (d > th * 0.98) d = th * 0.98;
            }
            dl = next_flip;
            uac_render_start();
            blocks = HALF_FRAMES / 32u;
            for (i = 0; i < blocks; i++) blk_t[i] = tn + d * (i + 1) / blocks;
            continue;
        }
        if (tn == next_poll) {
            next_poll = M.t5_phase + (double)++tick * tp + rnd() * 20000;   /* (a hardware period; the ISR 0..20 us late) */
            poll(tn);
            continue;
        }
        if (tn == next_tok) {
            host_token(tn);
            next_sof += tf;
            next_tok = next_sof + 30000 + rnd() * 10000;
            continue;
        }
    }
}

static uint32_t desc_maxp(void)                         /* EP4 IN's wMaxPacketSize in the configuration the FM-1 presents */
{
    const uint8_t *d; uint16_t l; uint32_t off;
    get_desc(0x0200, &d, &l);
    for (off = 0; off + 1u < l; off += d[off])
        if (d[off + 1] == 5 && d[off + 2] == 0x84)
            return d[off + 4] | (uint32_t)d[off + 5] << 8;
    return 0;
}

static void reset(uint32_t ch, double ppm, uint32_t maxp)
{
    memset(&uac, 0, sizeof uac);
    memset(&R, 0, sizeof R);
    ua_r = ua_w = 0;
    nch = ch;
    t_stems = ch == 10u ? 2u : 1u;
    usb_stems = 1;
    usb_duplex = ch == 10u;
    usb.config = 1;
    uac_rate_set(44100);
    uac_stream(1);
    uac.flowing = 1;
    queued = 0;
    last_f = -1;
    have_v = 0;
    f_render = 0;
    stall_until = tx_end = 0;
    M.poll_hz = UAC_POLL_HZ;
    M.t5_ppm = 0;
    M.t5_phase = 123456;
    memset(sine_ph, 0, sizeof sine_ph);
    M.ppm = ppm;
    M.maxp = maxp ? maxp : desc_maxp();
}

/* the poll's drift against the host's frames: TIMER5 off by -+1667 ppm (PRD 600 against 601) sweeps its phase through
 * the frame every ~0.3 s; none, it can sit in the gap after a packet left the wire (stuck: the worst case at 2 kHz) */
static const struct { double host, t5; int gap; } CASE[3] = {{-500.0, 1667.0, 0}, {500.0, -1667.0, 0}, {0.0, 0.0, 1}};
static void reset_case(uint32_t ch, uint32_t k)
{
    reset(ch, CASE[k].host, 0);
    M.t5_ppm = CASE[k].t5;
    if (CASE[k].gap)                                     /* (a 2 kHz poll just after the token: the next one too early) */
        M.t5_phase = 35000 + ((M.maxp + 9u) * 8.0 * 1.04 / 12e6 * 1e9 - 500000.0) / 2;
}

int main(void)
{
    static const uint32_t CH[2] = {8, 10};
    char s[240];
    uint32_t k, m;
    printf("-- USB STEMS / STEMS+IN at 44.1 kHz: minutes of the stream into the app's lanes (uac_service at %u Hz)\n",
           UAC_POLL_HZ);
    M.fs = 44117.6;
    M.load = 0.70;
    M.p_late = 0.043;
    M.late_max = 1.31;
    for (m = 0; m < 2u; m++) {
        uint32_t want = (CH[m] == 8u ? 736u : 920u);
        reset(CH[m], 0, 0);
        snprintf(s, sizeof s, "%u ch: EP4 IN wMaxPacketSize %u = 46 frames of %u bytes (44.1 kHz's 45 + 1)", CH[m], M.maxp, CH[m] * 2u);
        check(s, M.maxp == want && M.maxp >= 46u * CH[m] * 2u);
        for (k = 0; k < 3u; k++) {
            double now = 0;
            uint64_t sizes46 = 0;
            reset_case(CH[m], k);
            sim(1.0, &now);                              /* (settled: the first token comes before any packet) */
            R.missed = R.babble = 0;
            sim(120.0, &now);
            sizes46 = uac.adj_up;
            snprintf(s, sizeof s, "%u ch, host %+.0f ppm, TIMER5 %+.0f ppm%s, 2 min: %llu packets (%llu one frame longer), %llu babble, "
                     "%llu tokens with none", CH[m], CASE[k].host, CASE[k].t5, CASE[k].gap ? " (in the gap)" : "",
                     (unsigned long long)R.pkts, (unsigned long long)sizes46, (unsigned long long)R.babble, (unsigned long long)R.missed);
            check(s, R.babble == 0 && R.missed == 0 && sizes46 > 0 && !uac.stalls);
            snprintf(s, sizeof s, "  every frame in the lanes once, in order, each channel its own: %llu frames, %llu slips (%llu lost), "
                     "%llu repeated, %llu moved", (unsigned long long)R.got, (unsigned long long)R.slips, (unsigned long long)R.lost,
                     (unsigned long long)R.repeats, (unsigned long long)R.moved);
            check(s, R.slips == 0 && R.repeats == 0 && R.moved == 0 && R.partial == 0 && R.got + 2u * UA_HI > R.rendered &&
                     !uac.overruns && !uac.underruns);
        }
    }
    for (m = 0; m < 2u; m++)                             /* what 0.9.9 did: uac_service every 5th tick */
        for (k = 0; k < 3u; k++) {
            double now = 0;
            reset_case(CH[m], k);
            M.poll_hz = 2000;
            sim(1.0, &now);
            R.missed = 0;
            sim(30.0, &now);
            snprintf(s, sizeof s, "0.9.9's 2 kHz poll, %u ch, TIMER5 %+.0f ppm%s, 30 s: %.1f %% of the tokens with none, %u overruns: "
                     "%.1f slips/s, %.2f %% of the frames lost", CH[m], CASE[k].t5, CASE[k].gap ? " (in the gap)" : "",
                     100.0 * R.missed / R.tokens, uac.overruns, R.slips / 30.0, 100.0 * R.lost / (R.lost + R.got));
            check(s, R.missed > 30u && (CH[m] == 8u || R.slips > 30u));   /* (10 ch: the take's bursts; 0.9.10 above: none) */
        }
    {   /* what 0.9.7 did: 720 bytes for 8 channels */
        double now = 0;
        reset(8, 0, 720u);
        sim(30.0, &now);
        snprintf(s, sizeof s, "0.9.7's 720 bytes (8 ch, 30 s): %.0f babble/s (%.1f %% of the time halted), %u overruns: "
                 "%.0f slips/s, %.1f %% of the frames lost",
                 R.babble / 30.0, R.stall_ns / 3e8, uac.overruns, R.slips / 30.0, 100.0 * R.lost / (R.lost + R.got));
        check(s, R.babble > 30u * 10u && R.slips > 30u * 20u);   /* (it shows the field take's damage; 0.9.8 above: none) */
    }
    for (m = 0; m < 2u; m++) {   /* starvation: the underruns' repeats made up, every channel continuous */
        double now = 0;
        reset(CH[m], 0, 0);
        M.sine = 1;
        M.starve = 1;
        sim(36.0, &now);
        M.starve = 0;
        sim(6.0, &now);                                  /* (the last burst's repeats made up) */
        /* (a hold has two corners, where it starts and ends; a frame skipped would add two more each, the frame spread
         * over a packet none: the corners are the holds' alone) */
        snprintf(s, sizeof s, "%u ch, 50 ms starved every 5 s: %u underruns (%u frames repeated, made up %u, owed %u), %llu corners "
                 "(largest %.2f x the sine's step): the holds' only", CH[m], uac.underruns, uac.repaid + uac.owe, uac.repaid, uac.owe,
                 (unsigned long long)R.jumps, R.maxjump_x100 / 100.0);
        check(s, uac.underruns > 0 && uac.repaid > 8u && uac.owe == 0 && R.babble == 0 && R.maxjump_x100 < 120u &&
                 R.jumps <= 2u * nch * uac.underruns);
        M.sine = 0;
        M.starve = 0;
    }
    printf(fails ? "STEMS USB TEST FAILED (%d)\n" : "stems usb: all ok\n", fails);
    return fails != 0;
}
