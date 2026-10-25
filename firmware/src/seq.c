/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments
 * Copyright (C) 2026 Juan Ignacio Antonucci (Juan Jiant) */
/* Keyboard, scale, arpeggiator, sequencer and transport.
 * Everything here runs in the audio ISR, once per CTL-sample block, and
 * ends in trk_note_on / trk_note_off: engines never see where a note came from.
 * Four tracks, one transport: every track's pattern loops on its own LEN / DIV /
 * SWING / GATE (polymeter). The keys play the selected track; MIDI IN by GLO > SYSTEM ROUT (G_ROUTE):
 * CH1-4 (0) channels 1..4 play parts 1..4, channels 5..16 are ignored (midi_control.c midi_event); SEL (1)
 * every channel the selected track. Their CC1 (mod wheel), CC11 (expression) and channel aftertouch go to
 * the same track's modulation matrix (mod.c; the selected track: the one selected then).
 * A note into an armed track (song.rec) while
 * the transport runs is recorded into its pattern, quantised to its (swung) steps, with its
 * held length as TIE steps (rec_note, rec_hold, rec_release). With the ARP on, the notes the arp
 * plays are recorded, not the keys held (what plays back is what was heard). A key or MIDI note plays a
 * chord when the track's CHRD is on (chord.c): its notes go through the same input as so many keys. */
static const uint16_t SCALE_MASK[] = {
    0xFFF,                                   /* CHR */
    (1 << 0) | (1 << 2) | (1 << 4) | (1 << 5) | (1 << 7) | (1 << 9) | (1 << 11),   /* MAJ */
    (1 << 0) | (1 << 2) | (1 << 3) | (1 << 5) | (1 << 7) | (1 << 8) | (1 << 10),   /* MIN */
    (1 << 0) | (1 << 2) | (1 << 3) | (1 << 5) | (1 << 7) | (1 << 9) | (1 << 10),   /* DOR */
    (1 << 0) | (1 << 2) | (1 << 4) | (1 << 5) | (1 << 7) | (1 << 9) | (1 << 10),   /* MIX */
    (1 << 0) | (1 << 2) | (1 << 4) | (1 << 7) | (1 << 9),                          /* PEN */
    (1 << 0) | (1 << 3) | (1 << 5) | (1 << 7) | (1 << 10),                         /* MPEN */
    (1 << 0) | (1 << 2) | (1 << 3) | (1 << 5) | (1 << 7) | (1 << 8) | (1 << 11),   /* HARM */
    (1 << 0) | (1 << 1) | (1 << 3) | (1 << 5) | (1 << 7) | (1 << 8) | (1 << 10),   /* PHRY */
    (1 << 0) | (1 << 2) | (1 << 4) | (1 << 6) | (1 << 7) | (1 << 9) | (1 << 11),   /* LYD */
    (1 << 0) | (1 << 1) | (1 << 3) | (1 << 5) | (1 << 6) | (1 << 8) | (1 << 10),   /* LOC */
    (1 << 0) | (1 << 2) | (1 << 3) | (1 << 5) | (1 << 7) | (1 << 9) | (1 << 11),   /* MEL (ascending) */
    (1 << 0) | (1 << 3) | (1 << 5) | (1 << 6) | (1 << 7) | (1 << 10),              /* BLUES (minor) */
    (1 << 0) | (1 << 2) | (1 << 4) | (1 << 6) | (1 << 8) | (1 << 10),              /* WHOLE */
    (1 << 0) | (1 << 1) | (1 << 3) | (1 << 4) | (1 << 6) | (1 << 7) | (1 << 9) | (1 << 10), /* DIMHW */
    (1 << 0) | (1 << 2) | (1 << 3) | (1 << 5) | (1 << 6) | (1 << 8) | (1 << 9) | (1 << 11), /* DIMWH */
};

#define KB_SILENT 255u
static uint32_t kb_prev;
static volatile uint8_t kb_boot_hold;    /* #137: set from power-on to the end of the splash (main.c): the keys sound nothing,
                                          * and one held across its end stays silent until it is let go and pressed again */
static uint8_t kb_note[27], kb_trk[27];  /* per key: the note it started and on which track */
static int8_t kb_oct[27];                /* (0.8.5) .. on a DRUM track, the kit's octave it plays (OCT then: seq.c dxo_on) */
static volatile uint8_t kb_asleep;       /* MENU > SCREEN OFF (ui.c): the screen dark or waking, a key pressed now only
                                          * wakes it (silent, no layer, its release nothing) */
static uint8_t last_note = 60;
static volatile uint8_t transport_req;   /* 1 start, 2 stop, 3 restart (from the UI) */
static volatile uint8_t panic_req;       /* bit per track: release every sounding note (preset / engine change) */
static volatile uint8_t midi_hint;       /* MIDI IN played track n - 1, which is not the selected one (the UI says so) */

static uint32_t trk_index(const track_t *t) { return (uint32_t)(t - trk); }

static uint32_t trk_midi_ch(uint32_t i) { return i % NTRK; }   /* MIDI channel 0..15 of track i (keys -> MIDI out) */

static int drum_track(const track_t *t) { return ENGINES[eng_idx(t->eng_req)] == &ENG_DRUM; }

/* (JIANT 0.6.72) MENU > CONTROL > MIDI OUT (ui.c midi_out_mode, over USB): KEYS (as ever: the keys' notes), CLOCK (and
 * the clock, 24 a beat, with START / STOP while the tempo is the FM-1's own), ALL (and every note the FM-1 plays: the
 * sequencer, the arp, FUGUE, the punch-in repeats, OCT and RANDOM on them, each track on its channel; the keys then
 * only where no arp plays them), OFF. The notes sent are remembered (mo_out): each gets its note-off, all of them at
 * STOP, a panic or a change of setting */
enum { MO_KEYS, MO_CLOCK, MO_ALL, MO_OFF };
static volatile uint8_t mo_lvl;                    /* the setting (ui.c sets it every frame) */
static uint8_t mo_last;                            /* .. as the ISR last saw it */
static uint8_t mo_out[NTRK][128];                  /* a played note's note sent on, + 1 (0: none) */
static uint32_t mo_pulse_acc;                      /* (samples towards the next clock) */
static void mo_send(uint32_t st, uint32_t d1, uint32_t d2)
{
    midi_out_event((st >> 4) | st << 8 | d1 << 16 | d2 << 24);
}
static int mo_plays(const track_t *t, uint32_t note)   /* trk_note_on would sound it (voice.c: the same tests) */
{
    return trk_index(t) < trk_live && !t->p[P_MUTE] && !((trk_hush >> trk_index(t)) & 1u) &&
           !(t->engine == ENGI_DRUM && drum_muted(note));
}
/* a note the FM-1 plays (not a key's): sounded, and with ALL sent as it sounds: the punch-ins' OCT- / OCT+ and RANDOM
 * on it (pfx.c pfx_pit, pfx_rpit: drawn by trk_note_on) */
static void seq_note_on(track_t *t, uint32_t note, uint32_t vel)
{
    uint32_t i = trk_index(t) % NTRK, send = mo_lvl == MO_ALL && note < 128u && mo_plays(t, note) && !pfx_holds(t);
    trk_note_on(t, note, vel);
    if (send) {
        uint32_t x = (uint32_t)clamp((int32_t)note + (t->pfx_pit + t->pfx_rpit) / 16, 0, 127);
        if (mo_out[i][note])
            mo_send(0x80u | trk_midi_ch(i), mo_out[i][note] - 1u, 0);   /* (struck again: its last one ends first) */
        mo_out[i][note] = (uint8_t)(x + 1u);
        mo_send(0x90u | trk_midi_ch(i), x, vel ? vel : 1u);
    }
}
static void seq_note_off(track_t *t, uint32_t note)
{
    uint32_t i = trk_index(t) % NTRK;
    if (note < 128u && mo_out[i][note]) {
        mo_send(0x80u | trk_midi_ch(i), mo_out[i][note] - 1u, 0);
        mo_out[i][note] = 0;
    }
    trk_note_off(t, note);
}
static __attribute__((noinline)) void mo_flush_of(uint32_t mask)   /* the notes sent on, of these tracks: off */
{
    uint32_t i, n;
    for (i = 0; i < NTRK; i++)
        if ((mask >> i) & 1u)
            for (n = 0; n < 128u; n++)
                if (mo_out[i][n]) {
                    mo_send(0x80u | trk_midi_ch(i), mo_out[i][n] - 1u, 0);
                    mo_out[i][n] = 0;
                }
}
#define mo_flush() mo_flush_of(0xFFu)
static void mo_key(const track_t *t, uint32_t pkt)   /* a key's note: not with OFF; with ALL, an arp sends its own */
{
    if (mo_lvl != MO_OFF && !(mo_lvl == MO_ALL && t->p[P_AMODE]))
        midi_out_event(pkt);
}
static int mo_clock_on(void) { return (mo_lvl == MO_CLOCK || mo_lvl == MO_ALL) && !song.g[G_CLOCK]; }
static __attribute__((noinline)) void mo_block(uint32_t n)   /* events_block: the setting changed; the clock */
{
    if (mo_lvl != mo_last) {
        if (mo_last == MO_ALL)
            mo_flush();
        if ((mo_last == MO_CLOCK || mo_last == MO_ALL) && !(mo_lvl == MO_CLOCK || mo_lvl == MO_ALL) && song.playing &&
            !song.g[G_CLOCK])
            mo_send(0xFCu, 0, 0);                    /* (the clock leaves: whatever follows it stops) */
        mo_last = mo_lvl;
    }
    if (mo_clock_on()) {                             /* 24 a beat, also while stopped (a follower holds the tempo) */
        uint32_t len = beat_samples() / 24u;
        if (len < 64u)
            len = 64u;
        mo_pulse_acc += n;
        while (mo_pulse_acc >= len) {
            mo_pulse_acc -= len;
            midi_out_event(0x0Fu | 0xF8u << 8);
        }
    }
}

/* the 27 keys from F: black or white, and the key's place among the keys of its colour (white 0..15, black
 * 0..10). The DRUM grid: white keys are steps, black keys 1..8 lanes, 9 ACC, 10 / 11 the page */
static int key_black(uint32_t k) { return (int)((0x54Au >> ((k + 5u) % 12u)) & 1u); }
static uint32_t key_place(uint32_t k)
{
    uint32_t i, n = 0;
    for (i = 0; i < k; i++)
        n += (uint32_t)(key_black(i) == key_black(k));
    return n;
}
enum { GK_ACC = NLANE, GK_PGDN, GK_PGUP };       /* black keys 9..11 on the grid */

static uint32_t scale_mask(const track_t *t)
{
    return SCALE_MASK[clamp(t->p[P_SCALE], 0, sizeof SCALE_MASK / sizeof SCALE_MASK[0] - 1)];
}

/* note n moved down onto the track's scale (ROOT, SCALE): QNT SNAP's keys and QNT SEQ's sequenced notes */
static int32_t scale_snap(const track_t *t, int32_t n)
{
    uint32_t mask = scale_mask(t), guard = 12;
    while (guard-- && !((mask >> (uint32_t)((n - t->p[P_ROOT] + 120) % 12)) & 1u))
        n--;
    return n;
}

enum { QN_OFF, QN_SNAP, QN_WHITE, QN_SEQ };      /* P_QUANT */

/* (JIANT 0.4) G_STRN: note n onto the track's scale (down), then d steps of the scale up (d > 0) or down */
static int32_t scale_steps(const track_t *t, int32_t n, int32_t d)
{
    uint32_t mask = scale_mask(t);
    int32_t r = t->p[P_ROOT];
    n = scale_snap(t, n);
    if (!(mask & 0xFFFu))
        return n;
    for (; d > 0; d--)
        do n++; while (!((mask >> (uint32_t)((n - r + 120) % 12)) & 1u));
    for (; d < 0; d++)
        do n--; while (!((mask >> (uint32_t)((n - r + 120) % 12)) & 1u));
    return n;
}

/* (0.9.14) a sequence's note with ALL TR (G_STRN) and the FX layer's TRANSPOSE macro (perf_m[1]: OFF, +2ND +3RD +4TH +5TH
 * +6TH along the scale, +OCT, -OCT) */
static const int8_t PM_TRN[8] = {0, 1, 2, 3, 4, 5, 0, 0};    /* (scale steps; +OCT -OCT: PM_OCT's semitones) */
static const int8_t PM_OCT[8] = {0, 0, 0, 0, 0, 0, 12, -12};
static uint32_t pm_trn(void) { return (perf_m[1] + 2u) / 3u; }
static uint32_t strn_note(const track_t *t, uint32_t x)
{
    uint32_t i = pm_trn();
    int32_t s = song.g[G_STRN] + PM_TRN[i], n = (int32_t)x;
    if (s)
        n = scale_steps(t, n, s);
    return (uint32_t)clamp(n + PM_OCT[i], 0, 127);
}

static uint32_t kb_map(const track_t *t, uint32_t k)
{
    static const int8_t DEGREE[12] = {0, -1, 1, -1, 2, 3, -1, 4, -1, 5, -1, 6};
    const engine_t *e = ENGINES[eng_idx(t->eng_req)];   /* (the engine it switches to) */
    int32_t n;
    if (e->keys && (n = e->keys(t, k)) >= 0)           /* the engine's own key map (DRUM's GM map, slices) */
        return (uint32_t)n;
    n = 53 + (int32_t)k;
    if (t->p[P_QUANT] == QN_SNAP || t->p[P_QUANT] == QN_SEQ)   /* SNAP (and SEQ): every key, rounded down */
        return (uint32_t)clamp(scale_snap(t, n + 12 * song.octave), 0, 127);   /* (0.6.81: TRANS as it sounds, voice.c) */
    if (t->p[P_QUANT] == QN_WHITE) {                    /* WHITE: white keys walk the scale, black keys are silent */
        uint32_t mask = scale_mask(t), i;
        int32_t count = 0, degree = DEGREE[n % 12], oct;
        if (degree < 0)
            return KB_SILENT;
        /* C4 is the root. Walk scale degrees on successive white keys, including
         * below C4; scales with 5, 6, 8 or 12 notes still have no duplicated degrees. */
        degree += (n / 12 - 5) * 7;
        for (i = 0; i < 12u; i++)
            count += (mask >> i) & 1u;
        oct = degree / count;
        degree %= count;
        if (degree < 0) {
            degree += count;
            oct--;
        }
        for (i = 0; i < 12u; i++)
            if ((mask >> i) & 1u) {
                if (!degree)
                    break;
                degree--;
            }
        n = 60 + t->p[P_ROOT] + 12 * oct + (int32_t)i;
    }
    return (uint32_t)clamp(n + 12 * song.octave, 0, 127);
}

/* the notes key k plays (kb_sn: how many, its chord; kb_on: how many of them started, the rest strummed in later) */
#define KB_CH 4u
static uint8_t kb_snd[27][KB_CH], kb_sn[27], kb_on[27];
static uint16_t kb_sdue[27];                    /* (0.6.81) samples to the strum's next note */

/* (JIANT 0.6.81) CHORD (SCL > CHORD; after the Omnichord and the Orchid, light): a key plays a chord on the track.
 * TRIAD 7TH ADD9 SUS2 SUS4 are built in the track's scale on the key (every tone in key; a key outside it takes the
 * scale note below; CHR: the major scale of ROOT; 0.8.3: a scale of five, six or eight notes takes each tone as the
 * interval the chord asks for where the scale has it, else the nearest it offers: sus chords and sixths in the
 * pentatonics, augmented in whole tone, diminished in the diminished scales), MAJ MIN 7 MAJ7 MIN7 POWER that shape on
 * any key. VOICING CLOSE
 * (root position), LEAD (each chord the inversion nearest the last one the track played: voice leading), OPEN (the
 * second tone an octave up), +BASS (the root an octave down added). STRUM: the notes one after the other, low to
 * high. The root alone on a kit and on MONO / LEGATO / UNISON. The keys only (MIDI notes play as they come); the
 * notes go to the sequencer's recording and the ARP as the key's own would */
enum { CD_OFF, CD_TRI, CD_SEV, CD_ADD9, CD_SUS2, CD_SUS4, CD_MAJ, CD_MIN, CD_DOM7, CD_MAJ7, CD_MIN7, CD_POW };
enum { CV_CLOSE, CV_LEAD, CV_OPEN, CV_BASS };
static uint8_t chord_last[NTRK][KB_CH], chord_nlast[NTRK];   /* the last chord each track played (LEAD) */
/* (0.8.3) the first of the intervals p (n of them) the scale (mask) has over a root rel semitones above ROOT; -1 none */
static int32_t chord_pick(uint32_t mask, int32_t rel, const int8_t *p, uint32_t n)
{
    uint32_t i;
    for (i = 0; i < n; i++)
        if ((mask >> (uint32_t)((rel + p[i]) % 12)) & 1u)
            return p[i];
    return -1;
}
static uint32_t chord_build(const track_t *t, uint32_t root, uint8_t *out)
{
    static const int8_t DIA[5][4] = {{0, 2, 4, -1}, {0, 2, 4, 6}, {0, 2, 4, 8}, {0, 1, 4, -1}, {0, 3, 4, -1}};
    static const int8_t SHP[6][4] = {{0, 4, 7, -1}, {0, 3, 7, -1}, {0, 4, 7, 10}, {0, 4, 7, 11}, {0, 3, 7, 10},
                                     {0, 7, 12, -1}};
    const engine_t *e = ENGINES[eng_idx(t->eng_req)];
    uint32_t mode = (uint32_t)t->p[P_CHRD], vc = (uint32_t)t->p[P_CVOI], n = 0, i, j, ti = trk_index(t) % NTRK;
    int32_t nt[KB_CH + 1], r = (int32_t)root;
    if (!mode || mode > CD_POW || e->oneshot || (e->keys && e->keys(t, 0) >= 0) || t->p[P_VOICE] != V_POLY) {
        out[0] = (uint8_t)root;
        return 1;
    }
    if (mode >= CD_MAJ) {
        for (i = 0; i < 4u && SHP[mode - CD_MAJ][i] >= 0; i++)
            nt[n++] = r + SHP[mode - CD_MAJ][i];
    } else {                                    /* every other note of the scale from the key's */
        uint32_t mask = scale_mask(t), deg[12], c = 0, at = 0, guard = 12;
        int32_t rt = t->p[P_ROOT];
        if ((mask & 0xFFFu) == 0xFFFu || !(mask & 0xFFFu))
            mask = SCALE_MASK[1];               /* (CHR: the major scale of ROOT) */
        while (guard-- && !((mask >> (uint32_t)((r - rt + 120) % 12)) & 1u))
            r--;
        for (i = 0; i < 12u; i++)
            if ((mask >> i) & 1u) {
                if (i == (uint32_t)((r - rt + 120) % 12)) at = c;
                deg[c++] = i;
            }
        if (c == 7u) {                          /* a mode of seven: its own chord on the degree (diatonic) */
            for (i = 0; i < 4u && DIA[mode - CD_TRI][i] >= 0; i++) {
                uint32_t k = at + (uint32_t)DIA[mode - CD_TRI][i];
                nt[n++] = r + (int32_t)deg[k % c] + 12 * (int32_t)(k / c) - (int32_t)deg[at];
            }
        } else {                                /* (0.8.3) five, six or eight notes (PEN MPEN BLUES WHOLE DIMHW DIMWH):
                                                 * every other note of such a scale is a quartal or odd stack, so each
                                                 * tone is the interval the chord asks for where the scale has it, else
                                                 * the nearest it offers: a third major or minor (none: a suspension),
                                                 * the fifth perfect (else diminished over a minor third, augmented over
                                                 * a major), the seventh minor, major or sixth; pentatonic keys give
                                                 * triads, sus chords and sixths, whole tone augmented, diminished
                                                 * diminished and dominant sevenths: every tone in the scale */
            static const int8_t T3[4] = {4, 3, 5, 2}, F5[3] = {7, 8, 6}, F5M[3] = {7, 6, 8}, S7[3] = {10, 11, 9}, S7D[2] = {9, 10},
                                S9[2] = {14, 15}, S2[2] = {2, 1}, S4[2] = {5, 6};
            int32_t rel = (r - rt + 120) % 12, th, fi, x;
#define CPK(P) chord_pick(mask, rel, P, sizeof P)
            th = mode == CD_SUS2 ? CPK(S2) : mode == CD_SUS4 ? CPK(S4) : -1;
            if (th < 0) th = CPK(T3);
            fi = th == 3 ? CPK(F5M) : CPK(F5);
            nt[n++] = r;
            if (th >= 0) nt[n++] = r + th;
            if (fi >= 0) nt[n++] = r + fi;
            if (mode == CD_SEV && (x = th == 3 && fi == 6 ? CPK(S7D) : CPK(S7)) >= 0) nt[n++] = r + x;
            if (mode == CD_ADD9 && (x = CPK(S9)) >= 0) nt[n++] = r + x;
#undef CPK
        }
    }
    if (vc == CV_OPEN && n > 2u)
        nt[1] += 12;
    if (vc == CV_LEAD && chord_nlast[ti]) {     /* each tone the octave nearest the last chord's middle */
        int32_t mid = 0;
        for (i = 0; i < chord_nlast[ti]; i++) mid += chord_last[ti][i];
        mid /= (int32_t)chord_nlast[ti];
        for (i = 0; i < n; i++) {
            while (nt[i] - mid > 6 && nt[i] - 12 >= 24) nt[i] -= 12;
            while (mid - nt[i] > 6 && nt[i] + 12 <= 108) nt[i] += 12;
        }
    }
    if (vc == CV_BASS) {                        /* the root an octave down (a four-note chord leaves its fifth out) */
        if (n == 4u) { nt[2] = nt[3]; n = 3; }
        nt[n++] = r - 12;
    }
    for (i = 1; i < n; i++)                     /* low to high, no unisons */
        for (j = i; j > 0 && nt[j] < nt[j - 1]; j--) { int32_t x = nt[j]; nt[j] = nt[j - 1]; nt[j - 1] = x; }
    for (i = 1; i < n; i++)
        if (nt[i] <= nt[i - 1]) nt[i] = nt[i - 1] + 12;
    for (i = 0, j = 0; i < n && j < KB_CH; i++)
        if (nt[i] >= 0 && nt[i] <= 127) out[j++] = (uint8_t)nt[i];
    if (!j) out[j++] = (uint8_t)root;
    for (i = 0; i < j; i++) chord_last[ti][i] = out[i];
    chord_nlast[ti] = (uint8_t)j;
    return j;
}
static uint32_t chord_gap(const track_t *t) { return (uint32_t)clamp(t->p[P_CSTR], 0, 127) * (FS / 2000u); }   /* (~0.5 ms a step) */

/* ------------------------------------------------------------- arp --- */
static void arp_add(track_t *t, uint32_t note)
{
    uint32_t i;
    if (t->p[P_AHOLD] && t->arp_phys == 0u)
        t->nheld = 0;                               /* new chord replaces the latched one */
    t->arp_phys++;                                  /* every key-down: arp_remove counts every key-up */
    for (i = 0; i < t->nheld; i++)
        if (t->held[i] == note)
            return;                                 /* repeated note-on: not a new note */
    if (t->nheld < 16u)
        t->held[t->nheld++] = (uint8_t)note;
    if (t->nheld == 1u) {
        t->arp_pos = 0xFFFFFFF;                     /* fire on this block */
        t->arp_idx = 0xFFFFFFFFu;
    }
}

static void arp_remove(track_t *t, uint32_t note)
{
    uint32_t i, k = 0;
    if (t->arp_phys)
        t->arp_phys--;
    if (t->p[P_AHOLD])
        return;
    for (i = 0; i < t->nheld; i++)
        if (t->held[i] != note)
            t->held[k++] = t->held[i];
    t->nheld = (uint8_t)k;
}

static void rec_note(track_t *t, uint32_t note, uint32_t vel);
static void rec_release(track_t *t, uint32_t note);
static int rec_on(const track_t *t) { return ((song.rec >> trk_index(t)) & 1u) && song.playing && !chain.running; }
static int pfx_rec_ok(void) { return song.rec && song.playing && !chain.running; }   /* (pfx.c: the punch-in lane) */

#include "motion.c"

/* the ARP's modes (P_AMODE). Append-only: a stored value keeps its meaning (ORD plays as UP, the ORD switch of
 * ARP 2 orders the notes). 1.2 (Discussion #95), over the note list (the held notes low to high, or as played,
 * across OCT octaves), C E G held: DNUP G E C E; UP+8 C E G C' (the first note again an octave above the top);
 * CONV C G E (outside in); DIVG E G C (inside out); PINKY C G E G (the top note between the others); THUMB
 * C E C G (the bottom one); WALK a random step up or down the list (bouncing off its ends); CHORD all the held
 * notes together (the first 4), an octave higher each step through OCT */
enum { AM_OFF, AM_UP, AM_DN, AM_UPDN, AM_RND, AM_ORD, AM_REPEAT, AM_DNUP, AM_UP8, AM_CONV, AM_DIVG, AM_PINKY,
       AM_THUMB, AM_WALK, AM_CHORD,
       AM_TRNS };   /* (JIANT) TRNS: a key no note but the track's sequence transposed by its interval from C4 (t->trn, kept
                     * when let go: the next key moves it), MIDI in too; the steps' notes, never the lane hits */

static void arp_forget(track_t *t)               /* (its notes ended otherwise: trk_all_off) */
{
    t->arp_note = t->arp_ch[0] = t->arp_ch[1] = t->arp_ch[2] = 0;
}

/* CHORD's notes. Not inlined: they run once an arp step, and kept out of the audio ISR's loop over the tracks
 * (tests/target_budget.py) */
static __attribute__((noinline)) void arp_chord_off(track_t *t, int rec)   /* the other notes end */
{
    uint32_t i;
    for (i = 0; i < 3u; i++)
        if (t->arp_ch[i]) {
            if (rec)
                rec_release(t, t->arp_ch[i]);
            seq_note_off(t, t->arp_ch[i]);
            t->arp_ch[i] = 0;
        }
}

static __attribute__((noinline)) void arp_chord(track_t *t, const uint32_t *list, uint32_t n)
{
    uint32_t i, nt, top = 0, o = t->arp_idx % (uint32_t)(t->p[P_AOCT] > 0 ? t->p[P_AOCT] : 1);
    if (n > 4u)
        n = 4u;
    for (i = 0; i < n; i++) {                       /* the held notes (sorted or as played), o octaves up */
        nt = (uint32_t)clamp((int32_t)list[i] + 12 * (int32_t)o, 0, 127);
        if (nt == 127u && top++)
            continue;                               /* (notes clamped onto the top one: once) */
        if (!t->arp_note)
            t->arp_note = (uint8_t)nt;
        else
            t->arp_ch[i - 1u] = (uint8_t)nt;
        if (rec_on(t))
            rec_note(t, nt, 100);
        seq_note_on(t, nt, 100);
    }
}

static void arp_silence(track_t *t)              /* the sounding arp note(s) end */
{
    if (t->arp_ch[0] | t->arp_ch[1] | t->arp_ch[2])
        arp_chord_off(t, 0);
    seq_note_off(t, t->arp_note);
    t->arp_note = 0;
}

static void arp_off(track_t *t)                  /* the sounding arp note(s) end (a recorded one too) */
{
    if (t->arp_ch[0] | t->arp_ch[1] | t->arp_ch[2])
        arp_chord_off(t, rec_on(t));
    seq_note_off(t, t->arp_note);
    if (rec_on(t))
        rec_release(t, t->arp_note);
    t->arp_note = 0;
}

/* the place in the note list (len notes) the step arp_idx plays (not inlined, as arp_chord); UP+8 adds its note */
static __attribute__((noinline)) uint32_t arp_pick(track_t *t, uint32_t *list, uint32_t len)
{
    uint32_t i = t->arp_idx, m = (uint32_t)t->p[P_AMODE], cyc, k, w;
    if (m == AM_UP8 && len < 64u)                   /* UP+8: the first note again over the top */
        list[len++] = (uint32_t)clamp((int32_t)list[0] + 12 * t->p[P_AOCT], 0, 127);
    cyc = len > 1u ? 2u * len - 2u : 1u;
    k = i % cyc;
    switch (m) {
    case AM_DN:
        return len - 1u - i % len;
    case AM_UPDN:
        return k < len ? k : cyc - k;
    case AM_DNUP:
        return k < len ? len - 1u - k : k - (len - 1u);
    case AM_RND:
        return rng() % len;
    case AM_CONV:
    case AM_DIVG:
        k = i % len;
        if (m == AM_DIVG)
            k = len - 1u - k;
        return (k & 1u) ? len - 1u - k / 2u : k / 2u;
    case AM_PINKY:
        return len < 2u ? 0u : (k & 1u) ? len - 1u : k / 2u;
    case AM_THUMB:
        return len < 2u ? 0u : (k & 1u) ? 1u + k / 2u : 0u;
    case AM_WALK:
        w = t->arp_walk;
        if (!i)
            w = 0;                                  /* (from the bottom) */
        else if (w >= len)
            w = len - 1u;                           /* (notes let go) */
        else if (len > 1u)
            w = !w ? 1u : w == len - 1u ? w - 1u : (rng() >> 7) & 1u ? w + 1u : w - 1u;
        t->arp_walk = (uint8_t)w;
        return w;
    default:
        return i % len;
    }
}

/* n: samples the arp advances by (the external clock's while it runs, 0 stopped); gate_n: samples its
 * gate counts down by (real time when the external transport stops: a tapped or latched note still ends) */
static volatile uint8_t fx_lock_on;              /* (0.6.44) ui_layer.c: the FX layer locked open, no other layer armed:
                                                 * the keys are its effects without FX held */
static volatile uint32_t beat_pos, beat_n;      /* samples into the beat, the beat of the bar (0..3) */
static volatile uint32_t bar_n;                 /* (0.6.43) the bars since PLAY (MUTATE: project.c mutate_poll) */

/* the metronome (click.c) and the count-in (1.1, Discussion #131). The click follows the song tempo's quarter notes
 * (BPM, or the external clock's 24 pulses), 4 to the bar, the first accented, whatever a track's DIV or SWING: a beat is
 * four 1/16 steps as the sequencer counts them (click_beat_len: 4 x div_samples(1/16), which may be a few samples
 * shorter than beat_samples, so the click never drifts from the 1/16 tracks it is recorded against). clk_pos and
 * clk_step count the 1/16 steps as seq_tick counts a 1/16 track's (and midi_clock_pulse rescales clk_pos with them when
 * an external clock's tempo moves): CLK_START at seq_start clicks the first beat in the block step 0 starts in, and
 * beat k in the block a 1/16 track's step 4k starts in (every 4th step; step 0 of 16, the bar's first, accented).
 * The count-in: with MENU > COUNT-IN on, PLAY from stop with a track armed (REC while stopped starts PLAY too) on
 * the internal clock counts cin_total beats first (1 or 2 bars, clicked whatever CLICK is), then
 * starts the sequencer at step 1 exactly that many beats later (song.playing stays 0 until then: nothing plays or
 * records; the keys sound). A note into an armed track in the count-in's last eighth (the second half of its last
 * beat) is recorded onto step 1 when it starts (cin_keep, cin_flush), held on or let go as it was. An external clock
 * never counts in (its Start starts). STOP (seq_stop) cancels it. */
/* (clk_pos, clk_step, clk_n: mod.c, read by the SLICER and the FX layer before this file) */
static volatile uint8_t cin_left, cin_total;    /* the count-in's beats still to come (0: none), of how many */
static volatile uint32_t cin_pos;               /* samples into its beat playing */
static uint8_t cin_flush;                       /* the count-in ended in this block: record what was kept */
static uint8_t cin_note[NTRK][4], cin_vel[NTRK][4], cin_n[NTRK], cin_up[NTRK];   /* kept notes; cin_up bit k: let go */
static uint8_t cin_oct[NTRK][4];                /* (0.8.5) .. a kit's octave: its hits a step (bits 0-3), TOUCH (4-5) */
static int seq_counting(void) { return cin_left != 0u; }
static uint32_t click_beat_len(void) { return 4u * div_samples(2); }   /* DIV 2: 1/16 */
static int click_wanted(void)                  /* CLICK REC: while a track is armed (not in a song: no recording) */
{
    return click_mode == CLICK_ON || (click_mode == CLICK_REC && song.rec && !chain.running);
}
static void arp_step(track_t *t, uint32_t n, uint32_t gate_n)
{
    uint32_t period, cnt, list[64], len = 0, i, j, o;
    int32_t sw;
    if (t->arp_note) {
        if (t->arp_off <= gate_n)
            arp_off(t);
        else
            t->arp_off -= gate_n;
    }
    if (!t->p[P_AMODE] || !t->nheld || t->p[P_AMODE] == AM_TRNS) {   /* (TRNS: no arpeggio) */
        if ((!t->nheld || t->p[P_AMODE] == AM_TRNS) && t->arp_note)
            arp_off(t);
        return;
    }
    period = div_samples((uint32_t)t->p[P_ARATE]);
    sw = t->p[P_ASWING] * (int32_t)period / 250;
    t->arp_pos += n;
    if (t->arp_pos < period + (uint32_t)((t->arp_idx & 1u) ? sw : -sw) && t->arp_pos != 0xFFFFFFF + n)
        return;
    t->arp_pos = 0;
    /* build the note list: held notes (sorted or as played) over OCT octaves */
    for (i = 0; i < t->nheld; i++)
        list[i] = t->held[i];
    cnt = t->nheld;
    if (!t->p[P_AORDER])
        for (i = 1; i < cnt; i++)
            for (j = i; j > 0 && list[j - 1] > list[j]; j--) {
                uint32_t x = list[j];
                list[j] = list[j - 1];
                list[j - 1] = x;
            }
    for (o = 0; o < (uint32_t)t->p[P_AOCT]; o++)
        for (i = 0; i < cnt && len < 64u; i++)
            list[len++] = clamp((int32_t)list[i] + 12 * (int32_t)o, 0, 127);
    if (t->p[P_AMODE] == AM_REPEAT) { list[0] = t->held[t->nheld - 1u]; len = 1; } /* REPEAT: last played note, no octave traversal */
    t->arp_idx++;
    j = t->p[P_AMODE] == AM_CHORD ? 0u : arp_pick(t, list, len);
    if (t->arp_note)
        arp_off(t);
    if ((uint32_t)(rng() & 127u) <= (uint32_t)t->p[P_APROB]) {
        t->arp_off = period * (uint32_t)t->p[P_AGATE] / 128u;
        if (t->p[P_AMODE] == AM_CHORD) {            /* CHORD: the first 4 notes, an octave up each step */
            arp_chord(t, list, cnt);
            return;
        }
        t->arp_note = (uint8_t)list[j];
        if (rec_on(t))
            rec_note(t, t->arp_note, 100);           /* recording: what the arp plays */
        seq_note_on(t, t->arp_note, 100);
    }
}

static void arp_tick(track_t *t, uint32_t n) { arp_step(t, n, n); }

/* -------------------------------------------------------- note input --- */
/* the length of step idx in samples: SWING (the track's + the global, at most 100) makes the even steps longer
 * and the odd ones shorter, so every odd step starts late */
static uint32_t step_samples(const track_t *t, uint32_t period, uint32_t idx)
{
    return swing_step_len(t, period, idx);   /* core.h: own + global, at most 100 */
}

/* (JIANT) SHIFT OFS: the step whose notes play at place idx, the sequence OFS steps later (left with OFS < 0,
 * wrapping inside LEN). Live recording writes there too, so a note recorded plays where it was heard */
static uint32_t seq_src(const track_t *t, uint32_t idx)
{
    int32_t len = t->p[P_SLEN] > 0 ? t->p[P_SLEN] : 1, o = t->p[P_SOFS] % len;
    return (uint32_t)(((int32_t)(idx % (uint32_t)len) - o + len) % len);
}

#include "seq_live.c"                              /* (0.9.11) GLO > LIVE: the live modifiers, the ghost playhead */

/* (0.8.5) a recorded hit's octave (dx_in_rat / dx_in_tch: the key's or MIDI note's, eng_drum.c) as its lane's mode on
 * the step (core.h LM_*): x2 / x4 / x8 above the kit's octave, SHORT / GHOST / FLAM below, none in it; that lane only
 * (its hit and its notes), the rest of the step as it was. The last hit recorded on a lane sets it */
static uint32_t rec_lmode(void)
{
    if (dx_in_tch)
        return LM_SHORT + (dx_in_tch - 1u) % 3u;
    /* (0.9.18) a lane's xN spills (N hits, RAT_WKIT / N of a step apart): n hits a step live are x 2n */
    return dx_in_rat >= 4u ? LM_X8 : dx_in_rat >= 2u ? LM_X4 : LM_NONE;
}
/* a hit (a GM note) into step s of a DRUM track (rec_note, a held ratchet's next step: dxo_rec_hold) */
static void rec_drum(track_t *t, step_t *s, uint32_t note, uint32_t vel, int32_t mic)
{
    uint32_t l = drum_lane(note), k, m = rec_lmode(), was;
    if (s->time != ST_NOTE || (!s->n && !s->hit)) {
        s->n = s->hit = s->acc = 0;
        s->flags = 0;
        s->vel = 0;
        ST_MICRO_SET(s, mic);                           /* (0.6.88) its place off the grid (REC Q) */
        s->lm[0] = s->lm[1] = s->lm[2] = 0;
    }
    s->time = ST_NOTE;
    if (note == DRUM_LANE_NOTE[l]) {                    /* a lane's note: its hit */
        was = (s->hit >> l) & 1u;                       /* (0.9.16) the sound in the step already (it rolls with it) */
        s->hit |= (uint8_t)(1u << l);
        if (vel > 110)
            s->acc |= (uint8_t)(1u << l);
    } else {                                            /* another GM drum (a low tom, a crash): a note */
        for (k = 0; k < s->n && s->note[k] != note; k++)
            ;
        if (!(was = k < s->n)) {
            if (s->n < 4u)
                s->n++;
            s->note[s->n - 1u] = (uint8_t)note;
        }
        if (vel > 110)
            s->flags |= SF_ACCENT;
    }
    if (vel > s->vel)
        s->vel = (uint8_t)vel;
    /* (0.9.16) a plain hit on a lane new to a step with a RATCH or an ECHO: once, as played (it rolled x2..x4 at once,
     * the step's next parts replaying it, and on every pass after: rolls finger drumming over a DICE / GEN beat); played
     * there again, still once */
    if (!m && (was ? step_lm_raw(s, l) == LM_ONCE : step_ratchet(s) > 1u || s->comp == CP_ECHO || s->comp2 == CP_ECHO))
        m = LM_ONCE;
    step_set_lmode(s, l, m);
    t->seq_active = 1;
}
static uint8_t rec_at, rec_late;                        /* (0.8.5) the step (play index + 1) the last hit went into;
                                                         * 1: the next one (it was played late in the one before) */
/* live recording: the note goes into the nearest step, as swung (the one playing, or
 * the next one when it is past the middle of the playing one). Overdub: a step that
 * holds notes gets this one added (a chord of up to 4; when full, the last note is
 * replaced); MONO / LEGATO / UNISON parts keep one note per step, as step entry does.
 * Held on (synth parts): each further step the sequencer enters while the note is
 * held becomes a TIE (rec_hold), up to the pattern length; a release before the middle
 * of the last one puts that step back (rec_release), so a short note stays one step.
 * A note recorded into another step ends the hold before (the step model ties the
 * notes of one step only). */
static void rec_note_at(track_t *t, uint32_t note, uint32_t vel)
{
    uint32_t len = t->p[P_SLEN] > 0 ? (uint32_t)t->p[P_SLEN] : 1u, idx = t->seq_idx % len, k;
    uint32_t period = div_samples((uint32_t)t->p[P_SDIV]), sl = step_samples(t, period, t->seq_idx);
    int32_t gp = (int32_t)t->seq_pos - (int32_t)t->ext;   /* (0.6.88: from the step's place; an early one began before) */
    uint32_t next = t->seq_pos < 0x7FFFFFFFu && gp > (int32_t)(sl / 2u);
    int32_t mic = 0;
    step_t *s;
    if (next)
        idx = (idx + 1u) % len;
    s = &t->step[seq_src(t, idx)];
    if (t->seq_pos < 0x7FFFFFFFu && t->p[P_HRQ] < 127) {   /* (0.6.88) REC Q under 100 %: where it was played, kept */
        int32_t off = next ? gp - (int32_t)sl : gp, l = (int32_t)(next ? step_samples(t, period, idx) : sl);
        int32_t q = 127 - clamp(t->p[P_HRQ], 0, 127);
        mic = (off * 64 + (off < 0 ? -l / 2 : l / 2)) / (l ? l : 1);   /* (0.9.1: 64ths of the step, rounded; was 16ths) */
        mic = clamp((mic * q + (mic < 0 ? -63 : 63)) / 127, MICRO_MIN, MICRO_MAX);   /* (REC Q's share of it, rounded) */
    }
    if (drum_track(t)) {                            /* DRUM: into the grid, no holds (hits) */
        rec_drum(t, s, note, vel, mic);
        rec_at = (uint8_t)(idx + 1u);
        rec_late = (uint8_t)next;
        if (next) {
            if (t->rskip_idx != idx)
                t->rskip_n = 0;
            t->rskip_idx = (uint8_t)idx;
            if (t->rskip_n < NLANE)
                t->rskip[t->rskip_n++] = (uint8_t)note;
        }
        return;
    }
    if (s->time != ST_NOTE || !s->n || trk_vmode(t) != V_POLY) {
        s->n = 0;                                   /* a fresh step */
        s->flags = 0;
        s->vel = 0;
        s->lm[0] = s->lm[1] = s->lm[2] = 0;         /* (0.9.18: no note of it once) */
        ST_MICRO_SET(s, mic);                       /* (0.6.88) its place off the grid (REC Q) */
    }
    for (k = 0; k < s->n && s->note[k] != note; k++)
        ;
    if (k == s->n) {
        /* (0.9.18) a note added to a POLY step that repeats (RATCH, ECHO): once, as played, not rolled with the step's
         * own notes (lm_once; 0.9.16's LM_ONCE for a kit's lanes) */
        uint32_t rep = s->n && (step_ratchet(s) > 1u || s->comp == CP_ECHO || s->comp2 == CP_ECHO);
        if (s->n < 4u)
            s->n++;
        s->note[s->n - 1u] = (uint8_t)note;
        step_set_lmode(s, s->n - 1u, rep ? LM_ONCE : LM_NONE);
    }
    s->time = ST_NOTE;
    if (vel > 110)
        s->flags |= SF_ACCENT;
    if (vel > s->vel)
        s->vel = (uint8_t)vel;
    t->seq_active = 1;
    if (next) {                                     /* it sounds now: the step must not trigger it again */
        if (t->rskip_idx != idx)
            t->rskip_n = 0;
        t->rskip_idx = (uint8_t)idx;
        if (t->rskip_n < 4u)
            t->rskip[t->rskip_n++] = (uint8_t)note;
    }
    if (!t->rh_n || t->rh_start != idx) {           /* a new hold (one in another step ends) */
        t->rh_n = 0;
        t->rh_start = (uint8_t)idx;
        t->rh_ties = 0;
    }
    for (k = 0; k < t->rh_n && t->rh_note[k] != note; k++)
        ;
    if (k == t->rh_n && t->rh_n < 4u)
        t->rh_note[t->rh_n++] = (uint8_t)note;      /* a chord: held until its last key is up */
}

/* the sequencer enters step idx (before playing it): a recorded note still held ties into it */
static void rec_hold(track_t *t, uint32_t idx, uint32_t len)
{
    step_t *s;
    uint32_t k;
    if (!t->rh_n)
        return;
    if (!((song.rec >> trk_index(t)) & 1u) || t->rh_ties + 1u >= len) {
        t->rh_n = 0;                                /* disarmed, or the whole pattern is this note */
        return;
    }
    if (idx == t->rh_start)
        return;                                     /* (recorded ahead into the step now starting) */
    s = &t->step[seq_src(t, idx)];
    t->rh_bak = *s;
    t->rh_last = (uint8_t)seq_src(t, idx);
    t->rh_ties++;
    for (k = 0; k < 4u; k++)
        s->note[k] = 0;
    s->n = 0;
    s->time = ST_TIE;
    s->flags &= SF_MFINE;                           /* (0.9.1: MICRO kept whole, as its sixteenth in cwhen) */
    s->vel = 0;
    s->hit = s->acc = 0;
    s->lm[0] = s->lm[1] = s->lm[2] = 0;
}

/* (0.9.11) LIVE: a modified track records at its ghost's place (seq_live.c), where the step is in the written pattern */
static void rec_note(track_t *t, uint32_t note, uint32_t vel)
{
    uint16_t e = t->ext;
    int s = live_swap(t);
    if (s)
        t->ext = 0;
    rec_note_at(t, note, vel);
    if (s)
        live_swap(t);
    t->ext = e;
}
/* a key of a recorded note is up: the hold ends with the last one */
static void rec_release_at(track_t *t, uint32_t note)
{
    uint32_t i, k = 0;
    for (i = 0; i < t->rh_n; i++)
        if (t->rh_note[i] != note)
            t->rh_note[k++] = t->rh_note[i];
    if (k == t->rh_n || (t->rh_n = (uint8_t)k))
        return;                                     /* not one of them, or others still held */
    if (t->rh_ties && seq_src(t, t->seq_idx) == t->rh_last &&
        t->seq_pos < step_samples(t, div_samples((uint32_t)t->p[P_SDIV]), t->seq_idx) / 2u)
        t->step[t->rh_last] = t->rh_bak;            /* released early in it: not held into this step */
}
static void rec_release(track_t *t, uint32_t note)
{
    int s = live_swap(t);
    rec_release_at(t, note);
    if (s)
        live_swap(t);
}

/* a note into armed track t in the count-in's last eighth: kept for step 1 (cin_flush); once each, 4 per track */
static __attribute__((noinline)) void cin_keep(track_t *t, uint32_t note, uint32_t vel)
{
    uint32_t i = trk_index(t), k;
    if (cin_left != 1u || cin_pos < click_beat_len() / 2u || !((song.rec >> i) & 1u) || t->p[P_AMODE])
        return;
    for (k = 0; k < cin_n[i] && cin_note[i][k] != note; k++)
        ;
    if (k == cin_n[i]) {
        if (k == 4u)
            return;
        cin_n[i]++;
    }
    cin_note[i][k] = (uint8_t)note;
    cin_vel[i][k] = (uint8_t)vel;
    cin_oct[i][k] = (uint8_t)((dx_in_rat & 15u) | (dx_in_tch & 3u) << 4);
    cin_up[i] &= (uint8_t)~(1u << k);
}

static uint8_t in_norec;                            /* (0.6.84) input_on plays without recording (key_next) */
static void input_on(track_t *t, uint32_t note, uint32_t vel)
{
    last_note = (uint8_t)note;
    if (cin_left)
        cin_keep(t, note, vel);
    if (t->p[P_AMODE] == AM_TRNS) {                 /* ARP TRNS: the key transposes the sequence */
        t->trn = (int8_t)clamp((int32_t)note - 60, -36, 36);
        return;
    }
    if (rec_on(t) && !t->p[P_AMODE] && !in_norec)  /* (ARP on: arp_tick records its notes; 0.6.84: a key's chord was
                                                     * recorded whole when the key went down) */
        rec_note(t, note, vel);
    if (t->p[P_AMODE])
        arp_add(t, note);
    else
        trk_note_on(t, note, vel);
}

static int midi_note_held(const track_t *t, uint32_t note);
static int midi_local_held(const track_t *t, uint32_t note);
static void input_off(track_t *t, uint32_t note)
{
    if (midi_note_held(t, note))
        return;
    if (cin_left) {                                 /* a kept note let go before step 1: recorded, then let go */
        uint32_t i = trk_index(t), k;
        for (k = 0; k < cin_n[i]; k++)
            if (cin_note[i][k] == note)
                cin_up[i] |= (uint8_t)(1u << k);
    }
    rec_release(t, note);
    arp_remove(t, note);                            /* both: the note may have started in the */
    trk_note_off(t, note);                          /* other mode (ARP switched while held) */
}

/* ------------------------------------------- (0.8.5) the kit's octaves --- */
/* A DRUM track's note in another octave than the kit's (eng_drum.c DXO_*): a key at OCT +-n (kb_oct), a MIDI note of a
 * band around the GM map (drum_oct_of). Below: the hit with TOUCH n (dx_in_tch on its note-on: SHORT GHOST FLAM).
 * Above: the hit, then again every other step / once / x2 a step while held (dxo_tick; 0.9.13, was x2 x4 x8; 0.9.17,
 * was once x2 x4): playing, on the grid the sequencer's lane
 * ratchets use (a part every step / n from each step's start, swung as the steps are: seq_lane_rat), stopped, every
 * step / n from the press. Recording, playing (rec_on), the step it goes into gets the hit and its lane's mode
 * (rec_lmode: that lane only); every step entered while it is held too, before it plays (dxo_rec_hold: the sequencer
 * plays those, the held note is silent there), so the pass after plays what was heard; let go before the middle of
 * such a step, that step is put back (dxo_bak), as a held synth note's TIE. The ARP on: the notes go to it plain. Each
 * held one: its key or MIDI note (id), up to DXO_N a track (one more replaces the last) */
#define DXO_N 4u
#define DXO_KEY 0x100u                                  /* id: a key (+ k), a MIDI note (+ ch << 7 + note) */
#define DXO_MIDI 0x1000u
typedef struct {
    uint16_t id, idx;                                   /* who holds it; the step (play index) part counts in */
    uint8_t note, vel, r, part;                         /* the hit, its hits a step, the part struck last (255 none) */
    uint8_t quiet, rec;                                 /* a step (+ 1) it is silent in (the sequencer plays it); the
                                                         * step (+ 1) its press was recorded into */
    uint8_t half, ph;                                   /* (0.9.17) OCT +1: every other step; ph the steps since the
                                                         * press, odd: none */
    uint32_t cnt;                                       /* stopped: samples since its last hit */
} dxo_t;
static dxo_t dxo[NPART][DXO_N];
static uint8_t dxo_n[NPART];
static step_t dxo_bak[NPART];                           /* the step a held one was last recorded into, as it was */
static uint8_t dxo_bak_at[NPART];                       /* .. which (its source step + 1; 0 none), entered at play idx */
static uint16_t dxo_bak_idx[NPART];
static uint32_t dxo_part(const track_t *t, uint32_t r)  /* the part of the playing step it is in */
{
    uint32_t sl = step_samples(t, div_samples((uint32_t)t->p[P_SDIV]), t->seq_idx) / (r ? r : 1u), p;
    p = t->seq_pos / (sl ? sl : 1u);
    return p < r ? p : r - 1u;
}
static void dxo_on(track_t *t, uint32_t note, int32_t o, uint32_t vel, uint32_t id)
{
    uint32_t ti = trk_index(t), k;
    dxo_t *e;
    if (!o || t->p[P_AMODE] || ti >= NPART || !drum_track(t)) {
        input_on(t, note, vel);
        return;
    }
    if (o < 0) {
        dx_in_tch = (uint8_t)(o < -3 ? 3 : -o);
        input_on(t, note, vel);
        dx_in_tch = 0;
        return;
    }
    dx_in_rat = DXO_RAT[o > 3 ? 3 : o];
    rec_at = 0;
    input_on(t, note, vel);                             /* (its first hit; recorded with the RATCH) */
    for (k = 0; k < dxo_n[ti] && dxo[ti][k].id != id; k++)
        ;
    if (k == dxo_n[ti] && dxo_n[ti] < DXO_N)
        dxo_n[ti]++;
    e = &dxo[ti][k < DXO_N ? k : DXO_N - 1u];
    e->id = (uint16_t)id;
    e->note = (uint8_t)note;
    e->vel = (uint8_t)vel;
    e->r = dx_in_rat;
    e->half = o == 1;
    e->ph = 0;
    e->cnt = 0;
    e->idx = t->seq_idx;
    e->part = song.playing && t->seq_pos < 0x7FFFFFFFu ? (uint8_t)dxo_part(t, e->r) : 255u;
    e->quiet = e->rec = 0;
    if (cin_left)                                       /* (the count-in keeps it for step 1) */
        e->rec = 1;
    else if (rec_at && rec_on(t)) {
        e->rec = rec_at;
        if (rec_late)                                   /* into the next step: silent to its end, as it plays back */
            e->quiet = (uint8_t)(t->seq_idx + 1u);
    }
    dx_in_rat = 0;
}
static void dxo_off(track_t *t, uint32_t id)
{
    uint32_t ti = trk_index(t), k, j = 0;
    if (ti >= NPART || !dxo_n[ti])
        return;
    for (k = 0; k < dxo_n[ti]; k++)
        if (dxo[ti][k].id != id)
            dxo[ti][j++] = dxo[ti][k];
    if (j == dxo_n[ti])
        return;
    dxo_n[ti] = (uint8_t)j;
    if (!j && dxo_bak_at[ti]) {                         /* let go early in a step it was held into: that step back */
        if (rec_on(t) && dxo_bak_idx[ti] == t->seq_idx && seq_src(t, t->seq_idx) == dxo_bak_at[ti] - 1u &&
            t->seq_pos < step_samples(t, div_samples((uint32_t)t->p[P_SDIV]), t->seq_idx) / 2u)
            t->step[dxo_bak_at[ti] - 1u] = dxo_bak[ti];
        dxo_bak_at[ti] = 0;
    }
}
/* the sequencer enters step idx of track t (before it plays it): recording, the held ones go into it */
static __attribute__((noinline)) void dxo_rec_hold(track_t *t, uint32_t idx)
{
    uint32_t ti = trk_index(t), k, src = seq_src(t, idx), bak = 0;
    for (k = 0; k < dxo_n[ti]; k++) {
        dxo_t *e = &dxo[ti][k];
        if (e->rec == idx + 1u) {                       /* (its press went into this one: it plays it) */
            e->quiet = (uint8_t)(idx + 1u);
            continue;
        }
        if (!rec_on(t) || !drum_track(t) || (e->half && !e->ph))   /* (0.9.17: OCT +1, every other step) */
            continue;
        if (!bak) {
            dxo_bak[ti] = t->step[src];
            dxo_bak_at[ti] = (uint8_t)(src + 1u);
            dxo_bak_idx[ti] = (uint16_t)idx;
            bak = 1;
        }
        dx_in_rat = e->r;
        rec_drum(t, &t->step[src], e->note, e->vel, 0);
        dx_in_rat = 0;
        e->quiet = (uint8_t)(idx + 1u);
    }
}
static void dxo_hit(track_t *t, const dxo_t *e) { trk_note_on(t, e->note, e->vel); }
/* a block on (events_block, after the sequencer): the held ones' next hits */
static __attribute__((noinline)) void dxo_tick(track_t *t, uint32_t n)
{
    uint32_t ti = trk_index(t), k;
    if (!drum_track(t) || t->p[P_AMODE]) {
        dxo_n[ti] = 0;
        return;
    }
    for (k = 0; k < dxo_n[ti]; k++) {
        dxo_t *e = &dxo[ti][k];
        uint32_t r = e->r ? e->r : 1u, part;
        if (!song.playing || t->seq_pos >= 0x7FFFFFFFu) {   /* stopped: from the press */
            uint32_t per = div_samples((uint32_t)t->p[P_SDIV]) / r << e->half;
            e->part = 255u;
            e->idx = 0xFFFFu;
            e->cnt += n;
            if (per && e->cnt >= per) {
                e->cnt -= per;
                if (e->cnt >= per)
                    e->cnt = 0;
                dxo_hit(t, e);
            }
            continue;
        }
        e->cnt = 0;
        part = dxo_part(t, r);
        if (t->seq_idx != e->idx) {
            e->idx = t->seq_idx;
            e->part = 255u;
            e->ph ^= 1u;
        }
        if (e->quiet == t->seq_idx + 1u) {
            e->part = (uint8_t)part;
            continue;
        }
        if (part != e->part) {
            e->part = (uint8_t)part;
            if (!(e->half & e->ph))
                dxo_hit(t, e);
        }
    }
}

/* the effect of key k in the FX layer (PF_N none): the first 15 white keys (F3 .. F5) in order, G5 CLEAR; black keys
 * 1..6 (F#3 .. F#4) the track mutes, (0.9.17) G#4 FREEZE, A#4 the tracks, C#5 SHUFFLE, (0.9.14) D#5 F#5 the sweeps */
/* (0.9.19: the black keys 1..6 are the six track keys of the perform layer, as they were; T7 and T8 have none) */
static uint32_t perf_key(uint32_t k)
{
    uint32_t p = key_place(k);
    return !key_black(k) ? (p < PF_NW ? p : p == PF_NW ? PF_CLR : PF_N)
                         : p < 6u ? PF_M1 + p : p == 6u ? PF_FRZ : p == 7u ? PF_TGT : p >= 8u ? PF_SHUF + p - 8u : PF_N;
}

/* (0.6.14) LATCH: ARP OFF and ARP 2's HOLD ON, the keys' notes keep sounding when let go; the first key of the next
 * chord (no key down) lets the latched ones go. In the arp's note list (held / nheld / arp_phys, unused with ARP OFF);
 * HOLD turned off lets them go (events_block) */
static int key_latch(const track_t *t) { return t->p[P_AHOLD] && !t->p[P_AMODE]; }
static void latch_release(track_t *t)
{
    uint32_t i;
    for (i = 0; i < t->nheld; i++) {
        trk_note_off(t, t->held[i]);
        mo_key(t, 0x08u | (0x80u | trk_midi_ch(trk_index(t))) << 8 | (uint32_t)t->held[i] << 16);
    }
    t->nheld = 0;
}

/* (0.6.19) the UI (main loop) lets a track's held / latched notes go through here, in events_block: SEQ TOOLS CLEAR,
 * REC CLEAR ALL (they zeroed the list from the main loop: the latched voices kept sounding, nothing left to end them) */
static volatile uint8_t latch_drop_req;
static uint8_t latch_was;                           /* bit i: track i was in LATCH last block */
static void latch_drop(track_t *t)
{
    latch_release(t);                               /* (their voices and MIDI OUT's notes end) */
    pfx_hush(t);                                    /* (0.6.28) the punch-in repeats end and forget the notes struck */
    t->pfx_lnn = 0;                                 /* (STUTTER / ARP replayed the cleared steps' last notes) */
    t->arp_phys = 0;
    if (t->arp_note)
        arp_silence(t);
}

/* key k plays kb_note[k] on track t. A note another key holds already sounds: it is not started again (nor sent
 * to MIDI OUT); the key keeps its note */
/* (0.6.81) the next note of key k's chord in (a note another key holds already sounds: not started again) */
static void key_next(uint32_t k, track_t *t)
{
    uint32_t x = kb_snd[k][kb_on[k]], mc = trk_midi_ch(trk_index(t));
    if (!midi_local_held(t, x)) {
        in_norec = kb_sn[k] > 1u;                   /* (0.6.84: the chord's notes recorded at once, key_on) */
        dxo_on(t, x, kb_oct[k], 100, DXO_KEY + k);  /* (0.8.5: a kit's octave; else input_on) */
        in_norec = 0;
        mo_key(t, 0x09u | (0x90u | mc) << 8 | drum_oct_note(x, kb_oct[k]) << 16 | 100u << 24);
    }
    kb_on[k]++;
}
static void key_on(uint32_t k, track_t *t)
{
    uint32_t n = chord_build(t, kb_note[k], kb_snd[k]), gap = n > 1u ? chord_gap(t) : 0u, i;
    if (key_latch(t)) {                             /* LATCH: a new chord lets the last one go */
        uint32_t j;
        if (!t->arp_phys)
            latch_release(t);
        t->arp_phys++;
        for (j = 0; j < n; j++) {
            for (i = 0; i < t->nheld && t->held[i] != kb_snd[k][j]; i++)
                ;
            if (i == t->nheld && t->nheld < 16u)
                t->held[t->nheld++] = kb_snd[k][j];
        }
    }
    kb_on[k] = 0;
    kb_sn[k] = (uint8_t)n;
    if (n > 1u && rec_on(t) && !t->p[P_AMODE])      /* (0.6.84) recording: the whole chord into one step, where the
                                                     * key went down (the strum's later notes would fall into the next
                                                     * step); the sequencer strums it back as it plays (seq_step) */
        for (i = 0; i < n; i++)
            rec_note(t, kb_snd[k][i], 100);
    for (i = 0; i < n && (!gap || !i); i++)        /* (STRUM: the first now, the others by keys_strum) */
        key_next(k, t);
    kb_sdue[k] = (uint16_t)(gap < 65535u ? gap : 65535u);
    last_note = kb_note[k];                         /* (step entry, the SAMPLE zone: the key's note) */
}
/* (0.6.81) STRUM: the keys' chords still coming in, a block on */
static void keys_strum(uint32_t n)
{
    uint32_t k;
    for (k = 0; k < 27u; k++) {
        if (kb_on[k] >= kb_sn[k] || !kb_on[k])
            continue;
        if (kb_sdue[k] > n) {
            kb_sdue[k] = (uint16_t)(kb_sdue[k] - n);
            continue;
        }
        key_next(k, &trk[kb_trk[k] % NTRK]);
        kb_sdue[k] = (uint16_t)clamp((int32_t)chord_gap(&trk[kb_trk[k] % NTRK]), 1, 65535);
    }
}

/* key k is up: the note it started ends, unless another key still holds it */
static void key_off(uint32_t k, track_t *t)
{
    uint32_t n = kb_on[k], i, mc = trk_midi_ch(trk_index(t));
    if (!n)
        return;
    if (kb_oct[k] > 0)
        dxo_off(t, DXO_KEY + k);                    /* (0.8.5) its ratchet ends (LATCH too) */
    kb_on[k] = 0;                                   /* (the strum's notes not in yet: never) */
    kb_sn[k] = 0;
    if (key_latch(t)) {                             /* LATCH: it keeps sounding (recorded as let go) */
        if (t->arp_phys)
            t->arp_phys--;
        for (i = 0; i < n; i++)
            rec_release(t, kb_snd[k][i]);
        return;
    }
    for (i = 0; i < n; i++) {
        uint32_t x = kb_snd[k][i];
        if (midi_local_held(t, x))
            continue;
        input_off(t, x);
        mo_key(t, 0x08u | (0x80u | mc) << 8 | drum_oct_note(x, kb_oct[k]) << 16);
    }
}

#ifdef FM1_INPUT_LAT
/* the press latency stats (hal/fm1_input.h fm1_in_stat, console `inp`): from the first scan that saw
 * key k closed to its note-on here, and to the note's first sample leaving the DMA (kb_out_tick:
 * audio.c, the TIMER4 tick at which the block being rendered starts to play) */
static uint32_t kb_out_tick;
static __attribute__((noinline)) void kb_lat(uint32_t k)
{
    uint32_t t0 = fm1_in.note_t0[k], d = fm1_ticks() - t0, o = kb_out_tick - t0;
    fm1_in_stat.kb_n++;
    fm1_in_stat.kb_sum += d;
    fm1_in_stat.dac_sum += o;
    if (d > fm1_in_stat.kb_max)
        fm1_in_stat.kb_max = d;
    if (o > fm1_in_stat.dac_max)
        fm1_in_stat.dac_max = o;
}
#endif

static void keyboard_block(void)
{
    uint32_t cur = fm1_in.notes, ch, k;
    uint32_t lay, fx;
    keys_strum(CTL);                              /* (0.6.81) the chords' strums */
    ch = cur ^ kb_prev;                           /* keys also sound while entering steps */
    if (!ch)
        return;
    if (kb_boot_hold) {                           /* #137: under the splash; their release is silent too */
        for (k = 0; k < 27u; k++)
            if ((ch & cur) >> k & 1u)
                kb_note[k] = KB_SILENT;
        kb_prev = cur;
        return;
    }
    lay = (fm1_in.buttons & kb_mask) != 0u || fx_lock_on;
    fx = (fm1_in.buttons & perf_mask) != 0u || fx_lock_on;   /* (0.6.44: the FX layer locked open) */
    for (k = 0; k < 27u; k++) {
        if (!((ch >> k) & 1u))
            continue;
        if ((cur >> k) & 1u) {                    /* the selected track; the key-up goes to the same one */
            kb_trk[k] = song.sel;
            if (kb_asleep) {                      /* (SCREEN OFF: the wake press is swallowed) */
                kb_note[k] = KB_SILENT;
                continue;
            }
            if (lay) {                            /* a layer's button held (or locked): the key is the layer's */
                kb_note[k] = KB_SILENT;           /* (ui_layer.c), with FX an effect (perform.c) */
                kb_layer |= 1u << k;
                if (fx)
                    perf_press(perf_key(k), 1);
                continue;
            }
            if (song.grid >= 2u) {                /* NAME (ui_name.c): every key types, none sounds; (0.9.11) 3: GLO >
                                                   * LIVE MANO, the keys its board (seq_live.c, ui_live.c) */
                kb_note[k] = KB_SILENT;
                if (song.grid == 3u)
                    lv.kb |= 1u << k;
            }
            else if (song.grid)                   /* the DRUM grid: a lane key plays its lane, the rest are the UI's */
                kb_note[k] = key_black(k) && key_place(k) < NLANE ? DRUM_LANE_NOTE[key_place(k)] : KB_SILENT;
            else
                kb_note[k] = (uint8_t)kb_map(&trk[kb_trk[k]], k);
            kb_oct[k] = (int8_t)(!song.grid && drum_track(&trk[kb_trk[k]]) ? song.octave : 0);   /* (0.8.5) */
            if (kb_note[k] == KB_SILENT)
                continue;
            key_on(k, &trk[kb_trk[k]]);
#ifdef FM1_INPUT_LAT
            kb_lat(k);
#endif
        } else {
            if ((kb_layer >> k) & 1u) {
                kb_layer &= ~(1u << k);
                perf_press(perf_key(k), 0);
                continue;
            }
            if (kb_note[k] == KB_SILENT) {
                lv.kb &= ~(1u << k);
                continue;
            }
            key_off(k, &trk[kb_trk[k] % NTRK]);
        }
    }
    kb_prev = cur;
}

/* (0.6.62) a JUMP's or REV's TIMES: how often it has turned the track this pass (a step each; a pass ends where the
 * pattern wraps by itself, not by a jump) and the passes since PLAY (1:2) */
static uint8_t fl_cnt[NTRK][NSTEP], fl_pass[NTRK];
static void fl_reset(uint32_t ti) { memset(fl_cnt[ti % NTRK], 0, NSTEP); }
static int fl_ok(const track_t *t, uint32_t c, uint32_t a)   /* it turns the track now (and counts it) */
{
    uint32_t ti = (uint32_t)(t - trk) % NTRK, k = t->seq_idx % NSTEP, m = CP_TIMES(c, a);
    if (m == 3u) {                                  /* 1:2: once, every other pass */
        if (fl_pass[ti] & 1u)
            return 0;
        m = 1u;
    }
    if (m && fl_cnt[ti][k] >= m)
        return 0;
    if (m)
        fl_cnt[ti][k]++;
    return 1;
}
/* -------------------------------------------------------- sequencer --- */
static void fg_reset(void);                         /* (fugue.c) */
static void spill_stop(track_t *t);                 /* (0.9.18) RATCH's spill (below) */
static void seq_start(void)
{
    uint32_t i;
    ms_clock = 0;                                  /* (JIANT) the modulation sequences from their step 0 */
    if (mo_clock_on()) {                           /* (0.6.72) MIDI OUT CLOCK / ALL: START, the clock from here */
        mo_send(0xFAu, 0, 0);
        mo_pulse_acc = 0;
    }
    motion_begin();
    chain_start();
    for (i = 0; i < NTRK; i++) {                   /* every track from its step 0, together */
        track_t *t = &trk[i];
        t->seq_idx = (uint16_t)(t->p[P_SLEN] - 1);
        t->seq_pos = 0x7FFFFFFF;                   /* step 0 fires on the first block */
        t->ext = 0;
        t->rskip_n = 0;
        t->rh_n = 0;
        spill_stop(t);
        t->loops = 0;                              /* (0.6.39) the components: the first loop */
        t->tonal = 0; t->pulse_from = t->jump_to = t->rev = 0;   /* (0.6.42) no TONAL, PULSE, JUMP, REV */
        fl_reset(i), fl_pass[i] = 0;               /* (0.6.62) TIMES from the first pass */
    }
    fg_reset();                                    /* (0.6.52) FUGUE's heads from their first step */
    lv.act = 0;                                    /* (0.9.11) LIVE: the ghosts from step 0 too (live_q) */
    song.tick = 0;
    beat_pos = 0; beat_n = 0;                      /* the ARP LED's beat from the top too */
    bar_n = 0;
    clk_pos = CLK_START;                           /* the metronome's first beat with step 0 */
    song.playing = 1;
    slicer_start();                                /* slicer.c: its step 0 with the sequencer's */
    perf_start();                                  /* perform.c: its 1/16 grid too */
}

/* (0.6.37) a step's chance as the track plays it: its own, scaled by SHIFT > CHANCE (P_TCHAN 0: as it is) */
static uint32_t trk_chance(const track_t *t, const step_t *s)
{
    return step_chance(s) * (127u - (uint32_t)clamp(t->p[P_TCHAN], 0, 127)) / 127u;
}
/* (0.6.92) the step's EVERY: its components play this loop of the pattern (ALWAYS; a of b loops; the first; not the
 * first; never) */
static int every_on(const track_t *t, const step_t *s)
{
    const uint8_t *e = EV_AB[ST_EVERY(s) % EV_N];
    if (!e[0])
        return e[1] == 0u || (e[1] == 1u ? t->loops == 0u : e[1] == 2u ? t->loops != 0u : 0);
    return t->loops % e[1] == e[0] - 1u;
}
/* (0.6.82) the step's component of a type lo..hi that plays this loop: the first one's, else the second's; 0 none
 * (*a: its arguments) */
static uint32_t cp_get(const track_t *t, const step_t *s, uint32_t lo, uint32_t hi, uint8_t *a)
{
    if (!every_on(t, s))
        return 0;
    if (s->comp >= lo && s->comp <= hi) {
        if (a) *a = s->carg;
        return s->comp;
    }
    if (s->comp2 >= lo && s->comp2 <= hi) {
        if (a) *a = s->carg2;
        return s->comp2;
    }
    return 0;
}
#define CP_HAS(t, s, c, a) cp_get(t, s, c, c, a)
/* (0.6.39) a TRIG condition: this loop of the pattern is one it plays on */
static int cp_trig_cond(const track_t *t, uint32_t a)
{
    const uint8_t *c = CP_TRIGS[a % CP_NTRIG];
    if (!c[0])
        return c[1] == 1u ? t->loops == 0u : t->loops != 0u;   /* FIRST, NOT FIRST */
    return t->loops % c[1] == c[0] - 1u;
}
/* (0.6.98) WALK / CLIMB: the move this loop, in degrees (WALK) or octaves (CLIMB): the loops counted in RATEs, the
 * count round the RANGE: UP 0 1 .. R 0 ..; DOWN 0 -1 .. -R; UP-DN 0 1 .. R .. 1 0 ..; RND a place of 0..R a move, by
 * a hash of the move and the step (the same every time round, the pattern's own) */
static int32_t cp_walk_off(const track_t *t, const step_t *s, uint32_t a, int climb)
{
    uint32_t k = t->loops / CP_WRATES[CP_WRATE(a) % CP_NWRATE], m = CP_WMODE(a), g = CP_WRNG(a);
    int32_t r = climb ? (int32_t)g + 1 : (int32_t)CP_WDEGS[g];
    if (m == 0u) return (int32_t)(k % ((uint32_t)r + 1u));
    if (m == 1u) return -(int32_t)(k % ((uint32_t)r + 1u));
    if (m == 2u) { int32_t p = (int32_t)(k % (2u * (uint32_t)r)); return p <= r ? p : 2 * r - p; }
    { uint32_t h = (k * 2654435761u) ^ ((uint32_t)(s - seq_steps(t)) * 40503u); h ^= h >> 13; h *= 0x5BD1E995u; h ^= h >> 15; return (int32_t)(h % ((uint32_t)r + 1u)); }
}
/* every TRIG of the step that plays this loop lets it play (any other step: yes) */
static int cp_trig_ok(const track_t *t, const step_t *s)
{
    if (!every_on(t, s))
        return 1;
    if (s->comp == CP_TRIG && !cp_trig_cond(t, s->carg))
        return 0;
    return !(s->comp2 == CP_TRIG && !cp_trig_cond(t, s->carg2));
}
/* (0.6.42) FILL: the FX button held (ui_input.c sets it each pass) */
static volatile uint8_t seq_fill;
/* the step's one roll: its TRIGs (FILL: FX held), then its chance (the track's scaling it). (0.6.82) These come
 * first: a component that keeps the note silent this loop wins over the step's other one */
static int step_rolls(const track_t *t, const step_t *s)
{
    uint32_t c = trk_chance(t, s);
    if (!seq_fill && CP_HAS(t, s, CP_FILL, 0))
        return 0;
    if (live_drop(t))                               /* (0.9.11) LIVE THIN */
        return 0;
    return cp_trig_ok(t, s) && (c >= 100u || rng() % 100u < c);
}
/* (0.6.39) the step whose ARP component plays at s: s itself (a NOTE step with one), or the one a TIE continues
 * (arp_from); 0: none (a drum kit, the slices: never) */
static int trk_kit(const track_t *t)
{
    const engine_t *e = ENGINES[eng_idx(t->eng_req)];
    return e->oneshot || (e->keys && e->keys(t, 0) >= 0);
}
/* (0.6.88) TIMING early: how much before its place the step after the playing one starts (its ST_MICRO < 0), so the
 * playing one ends that much sooner (seq_tick); 0 when the next step is not known yet (a JUMP or REV coming, RANDOM
 * held, a song's section about to change) */
static uint32_t seq_early(const track_t *t, uint32_t period, uint32_t len)
{
    uint32_t ni = (t->seq_idx + 1u) % (len ? len : 1u);
    const step_t *s;
    int32_t m;
    if (t->jump_to || t->rev || (!ni && (chain.running || chain.armed)))
        return 0;
    s = &seq_steps(t)[seq_src(t, ni)];
    if (s->time != ST_NOTE || (!s->n && !s->hit))       /* (nothing to play there: never early) */
        return 0;
    m = ST_MICRO(s);
    return m < 0 ? (uint32_t)-m * step_samples(t, period, ni) / 64u : 0u;   /* (0.9.1: 64ths) */
}
/* (0.9.17) an ARP component walks its notes a lap at a time: the step plays one, the next the next time the pattern
 * comes round (seq_step: t->loops, its ORDER), held over the TIEs after it as a plain note (no RATE: was its notes
 * in the step, over the TIEs) */
static const step_t *arp_owner(const track_t *t, const step_t *s)
{
    const step_t *o = s;
    return o->time == ST_NOTE && (o->n || (o->hit && trk_kit(t))) && cp_get(t, o, CP_ARPN, CP_ARPQ, 0) ? o : 0;   /* (0.6.82:
                                                         * a kit's hits too) */
}
/* (0.9.18) a step as it comes in (a project, a section, an UNDO: project.c proj_steps): the components COMP no longer
 * lists, as what replaces them. ARP SCALE (since 0.9.17 the scale up from the step's lowest note, a degree a lap) is a
 * WALK of that note, a move a lap, its ORDER the MODE, its range the WALK's nearest (3 5 8 15 degrees: 3 5 7 12); TOUCH
 * (SHORT GHOST FLAM on all the step's hits) is the mode of each of its lanes that has none; OCT's old modes (its
 * octave every time, every other loop, by turns, at random) are OCT now (CLIMB: an octave a lap) or the notes moved
 * (kit: the track a kit, DRUM) */
static void step_norm(step_t *s, uint32_t kit)
{
    uint32_t k, l;
    for (k = 0; k < 2u; k++) {
        uint8_t *c = k ? &s->comp2 : &s->comp, *a = k ? &s->carg2 : &s->carg;
        if (*c == CP_ARPS) {
            if (s->n > 1u && !ST_EVERY(s)) {            /* (it played the lowest note alone) */
                uint32_t lo = s->note[0];
                for (l = 1; l < s->n && l < 4u; l++) if (s->note[l] < lo) lo = s->note[l];
                s->note[0] = (uint8_t)lo;
                for (l = 1; l < 4u; l++) s->note[l] = 0;
                s->n = 1;
            }
            *c = CP_WALK;
            *a = CP_WARG(0u, CP_ORD(*a), CP_RNG(*a));
        } else if (*c == CP_OCT) {                      /* (0.9.18) OCT is a move a lap (CP_CLIMB); the old modes: */
            uint32_t md = *a % 6u, sh = md == 0u ? 12u : md == 1u ? (uint32_t)-12 : md >= 4u ? (uint32_t)-12 : 0u;
            for (l = 0; l < s->n && l < 4u && !kit; l++)   /* (a kit's: its sounds kept; its +1 / -1 always gone) */       /* +1 / -1 always: the notes moved; ALT (+1 / -1 by turns): */
                s->note[l] = (uint8_t)clamp((int32_t)s->note[l] + (int32_t)sh, 0, 127);   /* an octave down, up a lap */
            *c = md <= 1u ? CP_NONE : CP_CLIMB;        /* and back; RND (-1 0 +1): an octave down, a place of 2 */
            *a = md <= 1u ? 0u : CP_WARG(0u, md == 3u ? 1u : md == 5u ? 3u : 0u, md == 5u ? 1u : 0u);
        } else if (*c == CP_TOUCH) {
            uint32_t ln = step_lanes(s), m = LM_SHORT + *a % 3u;
            for (l = 0; l < NLANE; l++)
                if (((ln >> l) & 1u) && step_lmode(s, l) == LM_NONE)
                    step_set_lmode(s, l, m);
            *c = *a = 0;
        }
    }
}
/* (0.9.18) RATCH: the hits a step plays (1..8: its flags' x1..x4, an ECHO component's x2..x8, at least LIVE RATCH's)
 * and r->q, the samples between them: the step's length sl shared out over RAT_W steps (a kit's 2, a synth's 4), or a
 * DIV's 1/n of a step x RAT_W; the hits after the first spill over the steps after it (seq_spill). Until 0.9.17 they
 * fell inside the step (a DIV's as many as fit). An ARP plays one note a lap (0.9.17): no hits */
typedef struct { uint32_t h, q; uint8_t shape, pit; } rat_t;
static uint32_t step_rat(const track_t *t, const step_t *s, uint32_t sl, rat_t *r)
{
    uint32_t h, d, l;
    uint8_t a = 0;
    r->shape = RS_FLAT;
    r->pit = RP_NONE;
    if (arp_owner(t, s)) {
        r->q = sl;
        return r->h = 1u;
    }
    if (s->time == ST_NOTE && CP_HAS(t, s, CP_ECHO, &a)) {   /* (0.6.42) ECHO: x2..x8, FADE (0.8.5: EVEN, FLAT) */
        h = 2u + (a & 7u) % 7u;
        r->shape = a & CP_ECHO_EVEN ? RS_FLAT : a & CP_ECHO_RISE ? RS_RISE : RS_FADE;
        r->pit = (uint8_t)CP_ECHO_PIT(a);
    } else
        h = step_ratchet(s);
    d = CT_DIVS[CT_DIV(s) % CT_NDIV];
    if ((l = live_rat(t)) > h) {                    /* (0.9.11) LIVE RATCH: every step at least that many hits, */
        r->h = l;                                   /* (0.9.18) inside the step (it plays on every one: no spill) */
        r->q = sl / l;
        return l;
    }
    r->h = h;
    r->q = h < 2u ? sl : sl * (trk_kit(t) ? RAT_WKIT : RAT_WMEL) / (d ? d : h);
    return h;
}
/* (0.9.18) a hit k of h's velocity in its shape: FLAT all at v, FADE each softer (ECHO's), RISE each louder */
static uint32_t rat_vel(uint32_t v, uint32_t shape, uint32_t k, uint32_t h)
{
    if (shape == RS_FADE) v = v * (h - k) / h;
    else if (shape == RS_RISE) v = v * (k + 1u) / h;
    return v < 8u ? 8u : v;
}
/* .. its pitch step: degrees (RP_UP / RP_DOWN, along the scale) or semitones (RP_OCT, every other hit an octave up) */
static int32_t rat_pit(const track_t *t, int32_t x, uint32_t pit, uint32_t k)
{
    if (pit == RP_OCT) return x + (k & 1u ? 12 : 0);
    if (pit == RP_NONE || !k) return x;
    return scale_steps(t, x, pit == RP_UP ? (int32_t)k : -(int32_t)k);
}
/* an ARP's k-th of n in its ORDER: UP, DOWN, UP-DN, RND */
static uint32_t cp_order(uint32_t o, uint32_t k, uint32_t n)
{
    if (!n) return 0;
    if (o == 1u) return n - 1u - k % n;                 /* DOWN */
    if (o == 2u && n > 1u) { k %= 2u * n - 2u; return k >= n ? 2u * n - 2u - k : k; }   /* UP-DN */
    if (o == 3u) return rng() % n;                      /* RND */
    return k % n;                                       /* UP */
}
/* (0.6.82) SEQ on a drum kit, its k-th note: the pattern's sounds in the order it plays them, into nn (0.9.18: the ARP's
 * the step's sounds one after another, arp_start; SCALE, the hits a degree further up, is WALK on a kit) */
static uint32_t cp_kit_arp(const track_t *t, const step_t *s, uint32_t c, uint32_t a, uint32_t k, uint8_t *nn)
{
    uint8_t l[16];
    uint32_t n = 0, i, m, cnt = t->p[P_SLEN] ? (uint32_t)t->p[P_SLEN] : 1u;
    const step_t *src = seq_steps(t);
    (void)s; (void)c;
    for (m = 0; m < cnt && m < NSTEP; m++) {
        if (src[m].time != ST_NOTE) continue;
        for (i = 0; i < src[m].n && i < 4u; i++)
            if (n < sizeof l && (!n || l[n - 1u] != src[m].note[i])) l[n++] = src[m].note[i];
        for (i = 0; i < NLANE; i++)
            if (((src[m].hit >> i) & 1u) && n < sizeof l && (!n || l[n - 1u] != DRUM_LANE_NOTE[i]))
                l[n++] = DRUM_LANE_NOTE[i];
    }
    if (!n) return 0;
    nn[0] = l[cp_order(CP_ORD(a), k, n)];
    return 1;
}
/* (0.6.39) SEQ's note this lap (0.9.17): the pattern's melody in the order it plays it (0.6.82), over RANGE octaves,
 * the k-th in its ORDER (0.9.18: the ARP's notes, arp_start; SCALE is WALK: step_norm) */
static uint32_t cp_arp_note(const track_t *t, const step_t *s, uint32_t c, uint32_t a, uint32_t k)
{
    uint8_t base[16];
    uint32_t nb = 0, n, i, m, o = CP_ORD(a), len = t->p[P_SLEN] ? (uint32_t)t->p[P_SLEN] : 1u;
    const step_t *src = seq_steps(t);
    (void)c;
    for (m = 0; m < len; m++)
        for (i = 0; i < src[m].n && i < 4u && src[m].time == ST_NOTE; i++) {
            uint8_t x = src[m].note[i];
            if (nb < sizeof base && (!nb || base[nb - 1u] != x))   /* (a note again at once: one) */
                base[nb++] = x;
        }
    if (!nb) return s->n ? s->note[0] : 60u;
    n = nb * (CP_RNG(a) + 1u);
    k = cp_order(o, k, n);
    return (uint32_t)clamp(base[k % nb] + 12 * (int32_t)(k / nb), 0, 127);
}
static void seq_release(track_t *t)
{
    uint32_t i;
    for (i = 0; i < t->seq_n; i++)
        seq_note_off(t, t->seq_notes[i]);
    t->seq_n = 0;
    t->seq_hold = 0;
    t->slide_glide = 0;                             /* live MONO / LEG keys must not glide after it */
    t->ss_n = t->ss_i = 0;                          /* (0.6.84) a strum still coming in: never */
}

static void seq_stop(void)
{
    uint32_t i;
    if (song.playing && mo_clock_on())             /* (0.6.72) MIDI OUT CLOCK / ALL: STOP */
        mo_send(0xFCu, 0, 0);
    song.playing = 0;
    cin_left = 0;                                  /* (a count-in: cancelled, nothing recorded) */
    cin_flush = 0;
    memset(cin_n, 0, sizeof cin_n);
    if (song.g[G_CLOCK])                           /* external clock: the arp stops with the transport, */
        for (i = 0; i < NPART; i++)                /* its sounding note too (HOLD keeps the latched chord) */
            if (trk[i].arp_note)
                arp_off(&trk[i]);
    for (i = 0; i < NTRK; i++) {
        seq_release(&trk[i]);
        trk[i].rh_n = 0;                           /* a recorded note held over the stop: as far as it got */
        spill_stop(&trk[i]);
        trk[i].pulse_from = trk[i].jump_to = trk[i].rev = 0;
    }
    fg_reset();                                    /* (0.6.52) FUGUE's heads silent */
    mo_flush();                                    /* (0.6.72) MIDI OUT ALL: nothing left hanging */
    chain_stop();
    motion_end();
}

/* play one step: TIE extends, REST releases, NOTE (re)triggers its notes, then its lane hits (their GM
 * notes, DRUM_LANE_NOTE; an accented hit at 127); a SLIDE on the previous step makes this one legato with a
 * glide (acid style). skip: bit k = note k, bit 8 + l = the hit of lane l already sounds from live
 * recording (not triggered, not released here).
 * RATCH x2..x8 (step_rat): a NOTE step plays its notes that many times, its spacing (0.9.18) a share of RAT_W steps,
 * the hits after the first spilling over the steps after it (seq_spill), each retriggered with GATE of its spacing;
 * the chance is rolled once for the whole step (a failed roll: no hit), a slide into it glides into the first hit
 * only, and it never slides or ties out. x1, what every older pattern holds, is the step exactly as before. */
#define SEQ_ROLLED (1u << 17)   /* seq_tick rolled the chance already (before the step's parameter locks): it passed */
#define SEQ_MISS (1u << 18)     /* .. it failed: nothing plays */
/* (0.9.18) RATCH's spill: a ratcheted step's hits after the first (seq_step played it), each t->sp_q samples on from
 * its start, as the track's clock runs (LIVE's RATE included), over the steps after it. A hit that lands on another
 * step that has its own note there (a synth: the same note sounding from it; a kit: a hit on the same lane) is left
 * out; a synth's hit takes the step's notes and velocity (its shape, its pitch step), a kit's its hits and TOUCH. On a
 * MONO / LEGATO track a new note cuts what is still to come; a new ratcheted step replaces it. The lanes' x2 x4 x8
 * (LM_*) spill the same way, each lane its own. Nothing to do: one test a block (seq_tick) */
/* (0.9.18) an entry of a step not in its RATCH's repeats: on a kit lane l recorded once (LM_ONCE, 0.9.16) or with its
 * own x2 x4 x8 (the lane's mode wins: no repeats of repeats); on a synth note l (0..3) overdubbed once (its lm slot l
 * LM_ONCE, seq.c rec_note_at: a POLY note added live to a step that repeats) */
static int lm_once(const step_t *s, uint32_t l, uint32_t kit)
{
    uint32_t r = step_lm_raw(s, l % NLANE);
    return r == LM_ONCE || (kit && lm_hits(r) > 1u);
}
static void spill_off(track_t *t)                   /* the notes the last spilled hit sounded: off */
{
    uint32_t i;
    for (i = 0; i < t->sp_pm; i++)
        seq_note_off(t, t->sp_pn[i]);
    t->sp_pm = 0;
}
static void spill_cut(track_t *t)
{
    t->sp_left = 0;
    spill_off(t);
}
static void spill_stop(track_t *t)                  /* STOP, PLAY, a section in: none */
{
    spill_cut(t);
    t->ls_on = 0;
}
static __attribute__((noinline)) void spill_start(track_t *t, const step_t *s, const uint8_t *nn, const uint8_t *vv,
                                                   uint32_t m, uint32_t sk, uint32_t kit, const rat_t *r, uint32_t gate)
{
    uint32_t i, n = 0;
    spill_off(t);
    for (i = 0; i < m && n < sizeof t->sp_nn; i++) {
        uint32_t l = drum_lane(nn[i]), lt = kit && l < NLANE ? lm_touch(step_lmode(s, l)) : 0u;
        if ((sk >> i) & 1u)                         /* (a note live recording sounds: not repeated) */
            continue;
        t->sp_nn[n] = nn[i];
        t->sp_vv[n] = vv[i];
        t->sp_tc[n++] = (uint8_t)(lt ? lt : t->dtch);
    }
    t->sp_m = (uint8_t)n;
    t->sp_arp = 0;
    t->sp_left = (uint8_t)(n ? r->h - 1u : 0u);
    t->sp_k = 1;
    t->sp_inst = t->step_inst;
    t->sp_shape = r->shape;
    t->sp_pit = r->pit;
    t->sp_kit = (uint8_t)kit;
    t->sp_dpit = t->dpit;
    t->sp_t = t->seq_pos < 0x7FFFFFFFu ? t->seq_pos : 0u;
    t->sp_q = r->q ? r->q : 1u;
    t->sp_gate = gate ? gate : 1u;
    t->sp_fresh |= 1u;
}
/* (0.9.18) ARP: the step's notes (all it would play: its chord, or its hits on a kit, after every other component),
 * one at a time at the ARP's RATE (a note value of the transport: 1/4 .. 1/32), in its ORDER, over RANGE octaves (a
 * single note: itself and RANGE octaves above), for the step and its TIEs and at least once through, spilling over the
 * steps after it as RATCH's hits do. nn: the notes, sorted here; returns 1, nn[0] the first */
static __attribute__((noinline)) uint32_t arp_start(track_t *t, uint8_t *nn, uint8_t *vv, uint32_t m, uint32_t kit,
                                                    uint32_t a, uint32_t gate, uint32_t span)
{
    uint32_t i, j, n = 0, q = div_samples(ARP_DIVI[CP_RATE(a)]), o = CP_ORD(a), tot, pl;
    spill_off(t);
    for (i = 0; i < m; i++) {                       /* (low to high, each once) */
        uint8_t x = nn[i];
        for (j = 0; j < n && t->sp_nn[j] < x; j++)
            ;
        if (j < n && t->sp_nn[j] == x) continue;
        for (uint32_t k = n; k > j; k--) { t->sp_nn[k] = t->sp_nn[k - 1u]; t->sp_vv[k] = t->sp_vv[k - 1u]; }
        t->sp_nn[j] = x;
        t->sp_vv[j] = vv[i];
        n++;
    }
    t->sp_m = (uint8_t)n;
    t->sp_oc = (uint8_t)(kit ? 1u : n == 1u ? CP_RNG(a) + 2u : CP_RNG(a) + 1u);
    tot = n * t->sp_oc;
    pl = o == 2u && tot > 1u ? 2u * tot - 2u : tot;   /* (UP-DN: there and back) */
    q = q ? q : 1u;
    if ((span + q - 1u) / q > pl) pl = (span + q - 1u) / q;
    t->sp_arp = 1;
    t->sp_ord = (uint8_t)o;
    t->sp_left = (uint8_t)((pl > 64u ? 64u : pl) - 1u);
    t->sp_k = 1;
    t->sp_inst = t->step_inst;
    t->sp_shape = RS_FLAT;
    t->sp_pit = RP_NONE;
    t->sp_kit = (uint8_t)kit;
    t->sp_dpit = t->dpit;
    t->sp_t = t->seq_pos < 0x7FFFFFFFu ? t->seq_pos : 0u;
    t->sp_q = q;
    t->sp_gate = gate ? gate : 1u;
    t->sp_fresh |= 1u;
    i = cp_order(o, 0u, tot);
    nn[0] = (uint8_t)clamp(t->sp_nn[i % n] + 12 * (int32_t)(i / n), 0, 127);
    vv[0] = t->sp_vv[i % n];
    return 1;
}
/* (seq_step, a kit) the lanes in a ratcheting mode on step s: each its hits from here, sl * RAT_WKIT / n apart */
static __attribute__((noinline)) void lane_start(track_t *t, const step_t *s, uint32_t vel, uint32_t sl)
{
    uint32_t l, k, h;
    for (l = 0; l < NLANE; l++) {
        uint32_t note = 0, v = vel, q;
        if ((h = lm_hits(step_lmode(s, l))) < 2u)
            continue;
        if ((s->hit >> l) & 1u) {
            note = DRUM_LANE_NOTE[l];
            if ((s->acc >> l) & 1u) v = 127u;
        } else
            for (k = 0; k < s->n && k < 4u && !note; k++)
                if (drum_lane(s->note[k]) == l) note = s->note[k];
        if (!note)
            continue;
        q = sl * RAT_WKIT / h;
        t->ls_on |= (uint8_t)(1u << l);
        t->ls_left[l] = (uint8_t)(h - 1u);
        t->ls_note[l] = (uint8_t)note;
        t->ls_vel[l] = (uint8_t)v;
        t->ls_inst[l] = t->step_inst;
        t->ls_q[l] = q ? q : 1u;
        t->ls_due[l] = t->seq_pos < 0x7FFFFFFFu && q > t->seq_pos ? q - t->seq_pos : 1u;
        t->ls_new |= (uint8_t)(1u << l);
    }
}
/* the step playing has its own note x (a synth: sounding from it; a kit: a hit on x's lane) */
static int spill_owned(const track_t *t, uint32_t x, uint32_t kit)
{
    const step_t *s = &seq_steps(t)[seq_src(t, t->seq_idx)];
    uint32_t i;
    if (kit)
        return s->time == ST_NOTE && ((step_lanes(s) >> (drum_lane(x) % NLANE)) & 1u);
    for (i = 0; t->st_inst == t->step_inst && i < t->st_n; i++)
        if (t->st_nn[i] == x)
            return 1;
    return 0;
}
static __attribute__((noinline)) void seq_spill(track_t *t, uint32_t n)
{
    uint32_t i, l, f = t->sp_fresh, ln = t->ls_new;
    t->sp_fresh = t->ls_new = 0;
    if (t->sp_pm) {                                 /* a synth's spilled hit: its gate */
        if (t->sp_off <= n) spill_off(t);
        else t->sp_off -= n;
    }
    if (t->sp_left && (t->sp_t += f & 1u ? 0u : n) >= (uint32_t)t->sp_k * t->sp_q) {
        uint32_t k = t->sp_k, h = k + t->sp_left, other = t->sp_inst != t->step_inst;
        int8_t dp = t->dpit;
        uint8_t dt = t->dtch;
        spill_off(t);
        for (i = 0; i < (t->sp_arp ? 1u : t->sp_m); i++) {
            uint32_t x = t->sp_nn[i], v = rat_vel(t->sp_vv[i], t->sp_shape, k, h);
            if (t->sp_arp) {                        /* (an ARP: its k-th note) */
                uint32_t tot = t->sp_m * t->sp_oc, p = cp_order(t->sp_ord, k, tot);
                x = (uint32_t)clamp(t->sp_nn[p % t->sp_m] + 12 * (int32_t)(p / t->sp_m), 0, 127);
                v = t->sp_vv[p % t->sp_m];
            }
            if (t->sp_kit) {
                t->dpit = (int8_t)clamp(t->sp_dpit + rat_pit(t, 60, t->sp_pit, k) - 60, -48, 48);
                t->dtch = t->sp_tc[i];
            } else
                x = (uint32_t)clamp(rat_pit(t, (int32_t)x, t->sp_pit, k), 0, 127);
            if (other && spill_owned(t, x, t->sp_kit))
                continue;
            seq_note_on(t, x, v);
            if (!t->sp_kit)
                t->sp_pn[t->sp_pm++] = (uint8_t)x;
        }
        t->dpit = dp;
        t->dtch = dt;
        t->sp_off = t->sp_gate;
        t->sp_k++;
        t->sp_left--;
    }
    for (l = 0; t->ls_on && l < NLANE; l++) {
        if (!((t->ls_on >> l) & 1u))
            continue;
        if (t->ls_due[l] > ((ln >> l) & 1u ? 0u : n)) {
            t->ls_due[l] -= (ln >> l) & 1u ? 0u : n;
            continue;
        }
        t->ls_due[l] = t->ls_q[l];
        if (t->ls_inst[l] == t->step_inst || !spill_owned(t, t->ls_note[l], 1u))
            seq_note_on(t, t->ls_note[l], t->ls_vel[l]);
        if (!--t->ls_left[l])
            t->ls_on &= (uint8_t)~(1u << l);
    }
}
/* (JIANT 0.6.81) a chord's slide on a POLY track (SLD on a chord step): its notes glide to the next chord's, low to
 * high, none struck again; the next chord with more notes: each extra one rises from the nearest of the old (its
 * octave nearest), fading in; with fewer: each old one left glides to the nearest new one, fading out as it goes (its
 * release). The glide over three quarters of the step */
static int32_t cg_near(int32_t x, const uint8_t *set, uint32_t n, int other)   /* the note of set, in the octave
                                                         * nearest x (other: never x itself, 0.6.82) */
{
    uint32_t i, o;
    int32_t best = x, bd = 1000;
    for (i = 0; i < n; i++)
        for (o = 0; o < 2u; o++) {                      /* (its octave nearest, and the next nearest) */
            int32_t y = set[i] + 12 * ((x - set[i] + 6 + 1200) / 12 - 100), d;
            if (o) y += y > x ? -12 : 12;
            d = y > x ? y - x : x - y;
            if (other && !d) continue;
            if (d < bd) { bd = d; best = y; }
        }
    return clamp(best, 0, 127);
}
static int32_t glide_q8(int32_t d, uint32_t ticks)   /* (0.6.82) a glide's step over ticks: gstep, Q8 */
{
    int32_t g = ((d < 0 ? -d : d) * 256 + (int32_t)ticks - 1) / (int32_t)(ticks ? ticks : 1u);   /* (rounded up: there in time) */
    return g ? g : 1;
}
/* (0.6.82) the samples from this step to the next that is not a TIE: a chord's slide lasts as long as the chord */
static uint32_t tie_span(const track_t *t, uint32_t period, uint32_t len)
{
    uint32_t n = step_samples(t, period, t->seq_idx), k;
    for (k = 1; k < len; k++) {
        uint32_t i = (t->seq_idx + k) % len;
        if (seq_steps(t)[seq_src(t, i)].time != ST_TIE)
            break;
        n += step_samples(t, period, i);
    }
    return n;
}
static void chord_glide(track_t *t, const uint8_t *nn, const uint8_t *vv, uint32_t m, uint32_t sk, uint32_t ssamp)
{
    uint8_t o[8], n[8], nv[8];
    uint32_t a = 0, b = 0, i, j, k, ti = trk_index(t) % NTRK;
    uint32_t ticks = (uint32_t)((t->p[P_GLIDE] > 0 ? (uint64_t)ssamp * (uint32_t)t->p[P_GLIDE] / 127u :
                                 (uint64_t)ssamp * 3u / 4u) / CTL);   /* (GLIDE: that much of the chord, its TIEs
                                                         * too (0.6.82); SLD alone: three quarters) */
    int32_t tr = trk_trans(t);
    if (!ticks) ticks = 1;
    for (i = 0; i < t->seq_n && a < 8u; i++)            /* (0.6.81) the old chord: its notes still sounding (a voice
                                                         * taken by another note is not one) */
        for (k = 0; k < NVOICE; k++)
            if (t->v[k].active && t->v[k].gate && t->v[k].note == t->seq_notes[i]) { o[a++] = t->seq_notes[i]; break; }
    for (i = 0; i < m && b < 8u; i++)
        if (!((sk >> i) & 1u)) { n[b] = nn[i]; nv[b++] = vv[i]; }
    for (i = 1; i < a; i++) for (j = i; j > 0 && o[j] < o[j - 1]; j--) { uint8_t x = o[j]; o[j] = o[j - 1]; o[j - 1] = x; }
    for (i = 1; i < b; i++) for (j = i; j > 0 && n[j] < n[j - 1]; j--) {
        uint8_t x = n[j]; n[j] = n[j - 1]; n[j - 1] = x; x = nv[j]; nv[j] = nv[j - 1]; nv[j - 1] = x; }
    for (i = 0; i < a || i < b; i++) {
        voice_t *v = 0;
        if (i < a)
            for (k = 0; k < NVOICE; k++)
                if (t->v[k].active && t->v[k].gate && t->v[k].note == o[i]) { v = &t->v[k]; break; }
        if (i < a && i < b && v) {                      /* a pair: the old note glides to the new */
            int32_t d;
            if (mo_out[ti][o[i]]) { mo_send(0x80u | trk_midi_ch(ti), mo_out[ti][o[i]] - 1u, 0); mo_out[ti][o[i]] = 0; }
            v->note = n[i];
            glide_from_trk(t, v);                       /* (0.9.8) VOICE's S-curve from where it sounds */
            v->pitch16 = ((int32_t)n[i] + tr) * 16 + t->pfx_rpit;
            d = v->pitch16 - v->pitch_cur;
            v->gstep = glide_q8(d, ticks);
            if (mo_lvl == MO_ALL && mo_plays(t, n[i])) {
                mo_out[ti][n[i]] = (uint8_t)(n[i] + 1u);
                mo_send(0x90u | trk_midi_ch(ti), n[i], nv[i] ? nv[i] : 1u);
            }
        } else if (i < b) {                             /* a new one: from the nearest old note, fading in */
            uint32_t a0 = vage;
            seq_note_on(t, n[i], nv[i]);
            if (vage != a0 && a)
                for (k = 0; k < NVOICE; k++)
                    if (t->v[k].age == vage && t->v[k].active) {
                        voice_t *w = &t->v[k];
                        int32_t d;
                        w->pitch_cur = (cg_near(n[i], o, a, 1) + tr) * 16 + t->pfx_rpit;
                        w->gfrom = (int16_t)w->pitch_cur;   /* (0.9.8: VOICE's glide from there) */
                        d = w->pitch16 - w->pitch_cur;
                        w->gstep = glide_q8(d, ticks);
                        w->gacc = 0;
                        w->fin_d = (uint16_t)(d < 0 ? -d : d);   /* (in as it glides; 0.6.82) */
                    }
        } else if (v) {                                 /* an old one left: to the nearest new note, fading out */
            int32_t d;
            glide_from_trk(t, v);
            v->pitch16 = (cg_near(o[i], n, b, 0) + tr) * 16 + t->pfx_rpit;
            d = v->pitch16 - v->pitch_cur;
            v->gstep = glide_q8(d, ticks);
            seq_note_off(t, o[i]);
        }
    }
    t->slide_glide = 0;
}

/* (0.6.81) GLIDE on a POLY track: every chord glides into the next by itself (no SLD needed): its notes held to the next
 * step when that one plays notes */
static int chord_auto(const track_t *t, uint32_t len)
{
    const step_t *nx;
    if (t->p[P_VOICE] != V_POLY || t->p[P_GLIDE] <= 0)
        return 0;
    nx = &seq_steps(t)[seq_src(t, (t->seq_idx + 1u) % len)];
    return nx->time == ST_NOTE && nx->n;
}

/* (0.6.87) HUMAN: SEQ > TRACK's HUMAN amount (0..127; stored 127 - it in P_HVEL: 127 none; 0.6.92: one knob, it moves
 * each note's velocity, start, gate, decay and release); its own random numbers (never libc's rng: the UI's dice keep
 * theirs) */
static uint32_t hum_rs = 0x2545F491u;
static uint32_t hum_rnd(void) { hum_rs ^= hum_rs << 13; hum_rs ^= hum_rs >> 17; hum_rs ^= hum_rs << 5; return hum_rs; }
static uint32_t hum_amt(const track_t *t, uint32_t id)
{
    if (FELUCCA_FM4 && t->eng_req % NENGINES == ENGI_DIGITAL) return 0;
    return 127u - (uint32_t)clamp(t->p[id], 0, 127);
}
static int32_t hum_pm(uint32_t r) { return (int32_t)(hum_rnd() % (2u * r + 1u)) - (int32_t)r; }   /* -r .. r */
#define HUM_TMAX (FS * 40u / 1000u)                 /* at 100 %: up to 40 ms late (0.6.92: was 30) */
static __attribute__((noinline)) void seq_step(track_t *t, const step_t *s, uint32_t period, uint32_t skip)
{
    uint32_t i, j, gate = period * (uint32_t)t->p[P_SGATE] / 128u;
    uint32_t vel = (s->flags & SF_ACCENT) ? 127u : (s->vel ? s->vel : 96u);
    uint32_t slide_in = t->seq_hold && t->seq_n;
    uint32_t len = t->p[P_SLEN] ? (uint32_t)t->p[P_SLEN] : 1u;
    uint32_t next_tie = seq_steps(t)[seq_src(t, (t->seq_idx + 1u) % len)].time == ST_TIE;
    rat_t rt;
    uint32_t hits = step_rat(t, s, step_samples(t, period, t->seq_idx), &rt);
    const step_t *arp = arp_owner(t, s);            /* (0.6.39) an ARP component: one of its notes a part */
    uint8_t ca = 0, aa = 0, ra = 0, ba = 0;
    uint32_t ac = 0, rnd = 0, bend = 0, arpn, arpq, many;
    int32_t walk = 0, climb = 0;                       /* (0.6.98) WALK / OCT (0.9.18: CLIMB): this loop's move */
    int cont = 0;                                   /* .. a PULSE's note again */
    uint8_t nn[4 + NLANE], vv[4 + NLANE];           /* the notes it plays: its notes, then its hits */
    uint32_t m = 0, sk = 0, sk0, hdel = 0, once = 0;
    /* QNT SEQ: the step's notes (not the lane hits) snap to the scale as they play; never on a drum kit, the slices
     * or another engine that maps the keys itself. seq_notes keeps the notes that sound: their note-offs match */
    const engine_t *e = ENGINES[eng_idx(t->eng_req)];
    uint32_t kit = e->oneshot || (e->keys && e->keys(t, 0) >= 0);
    uint32_t qseq = t->p[P_QUANT] == QN_SEQ && !kit;
    int32_t up = (kit ? 0 : t->p[P_POFS] + live_up(t)) +   /* SHIFT PIT: the notes up (a kit keeps its lanes); LIVE OCT */
                 (t->p[P_AMODE] == AM_TRNS ? t->trn : 0);   /* ARP TRNS: the keys' interval */
    if ((skip & SEQ_MISS) || (!(skip & SEQ_ROLLED) && !step_rolls(t, s))) {
        seq_release(t);
        return;
    }
    if (s->time == ST_TIE) {
        if (t->seq_n) {
            t->seq_off = gate + period / 2u;
            t->seq_hold = (s->flags & SF_SLIDE) != 0 || next_tie || chord_auto(t, len);   /* chains hold at any GATE /
                                                         * swing; (0.6.81) GLIDE on POLY: to the next chord */
        }
        return;
    }
    if (!cont && CP_HAS(t, s, CP_KILL, 0))
        trk_fade_out(t);                            /* (0.6.42) KILL: what sounds goes (~3 ms), its notes after */
    if (!cont && (s->time == ST_REST || (!s->n && !s->hit)) && t->pulse_from) {
        const step_t *o = &seq_steps(t)[(t->pulse_from - 1u) % NSTEP];   /* (0.6.42) PULSE: again on an empty step */
        if (CP_HAS(t, o, CP_PULSE, &ca) && o->time == ST_NOTE && (o->n || o->hit) && ++t->pulse_k % (ca % 4u + 1u) == 0u) {
            s = o;
            cont = 2;
            vel = (s->flags & SF_ACCENT) ? 127u : (s->vel ? s->vel : 96u);
        }
    }
    if (!cont && s->time == ST_NOTE && (s->n || s->hit)) {
        uint32_t pi = (uint32_t)(s - seq_steps(t));    /* (0.6.42) PULSE from here; another note ends one */
        uint32_t pul = CP_HAS(t, s, CP_PULSE, 0);
        t->pulse_from = (uint8_t)(pul && pi < NSTEP ? pi + 1u : 0u);
        t->pulse_k = 0;
        if (CP_HAS(t, s, CP_TONAL, &ca))
            t->tonal = (int8_t)((int32_t)(ca % 15u) - 7);
    }
    if (s->time == ST_REST || (!s->n && !s->hit)) {
        seq_release(t);
        return;
    }
    if (arp)                                        /* (0.6.82) the components on these notes, one a type: the */
        ac = cp_get(t, arp, CP_ARPN, CP_ARPQ, &aa); /* step's first, else its second */
    arpn = ac == CP_ARPN;                           /* (0.9.18) ARP: the notes one at a time (arp_start); SEQ: a lap's */
    arpq = ac == CP_ARPQ;
    rnd = CP_HAS(t, s, CP_RAND, &ra);
    bend = CP_HAS(t, s, CP_BEND, &ba);
    {   /* (0.6.98) WALK and CLIMB: this loop's move, from the step's two components (both may walk / climb) */
        uint8_t wa = 0, c2 = 0;
        walk = CP_HAS(t, s, CP_WALK, &wa) ? cp_walk_off(t, s, wa, 0) : 0;
        climb = CP_HAS(t, s, CP_CLIMB, &c2) ? cp_walk_off(t, s, c2, 1) * 12 : 0;
    }
    if (CT_LEN(s))                                  /* (0.6.82) LEN: the step's own gate, n/16 */
        gate = period * CT_LEN(s) / 16u;
    if (hits > 1u)                                  /* RATCH: the gate of a hit (a share of its spacing), no slide or
                                                     * tie out; (0.9.18) the hits after it spill (seq_spill) */
        gate = rt.q * (CT_LEN(s) ? CT_LEN(s) * 8u : (uint32_t)t->p[P_SGATE]) / 128u;
    if (arpn)                                       /* (0.9.18) an ARP: the gate of a note at its RATE */
        gate = div_samples(ARP_DIVI[CP_RATE(aa)]) * (CT_LEN(s) ? CT_LEN(s) * 8u : (uint32_t)t->p[P_SGATE]) / 128u;
    many = hits > 1u || arpn;                       /* (no slide or tie out, no HUMAN timing) */
    for (i = 0; i < (arpq ? 1u : s->n) && i < 4u && !(kit && arpq); i++) {
        uint32_t x = arpq ? cp_arp_note(t, arp, ac, aa, t->loops) : s->note[i];   /* (0.9.17: SEQ, this lap's) */
        x = (uint32_t)clamp((int32_t)x + up, 0, 127);
        if (!kit) {                                 /* (0.6.42) TONAL, RAND; (0.6.98) WALK, OCT (CLIMB) */
            if (walk)
                x = (uint32_t)clamp(scale_steps(t, (int32_t)x, walk), 0, 127);
            if (climb)
                x = (uint32_t)clamp((int32_t)x + climb, 0, 127);
            if (t->tonal)
                x = (uint32_t)clamp(scale_steps(t, (int32_t)x, t->tonal), 0, 127);
            if (rnd) {
                int32_t r = ra % 8u ? (int32_t)(ra % 8u) : 1;
                x = (uint32_t)clamp(scale_steps(t, (int32_t)x, (int32_t)(rng() % (uint32_t)(2 * r + 1)) - r), 0, 127);
            }
        }
        if (!kit)                                   /* (JIANT 0.4) STRN: up / down the scale, every sequence */
            x = strn_note(t, x);
        if (qseq) {                                 /* QNT SEQ: onto the scale now; two notes snapping */
            x = (uint32_t)clamp(scale_snap(t, (int32_t)x), 0, 127);   /* together play once */
            for (j = 0; j < m && nn[j] != x; j++)
                ;
            if (j < m)
                continue;
        }
        nn[m] = (uint8_t)x;
        vv[m] = (uint8_t)vel;
        sk |= ((skip >> i) & 1u) << m;
        once |= (uint32_t)(arpq ? 0 : lm_once(s, kit ? drum_lane(s->note[i]) : i, kit)) << m;
        m++;
    }
    for (i = 0; i < NLANE && !arpq; i++)
        if ((s->hit >> i) & 1u) {
            nn[m] = DRUM_LANE_NOTE[i];
            vv[m] = (uint8_t)((s->acc >> i) & 1u ? 127u : vel);
            sk |= ((skip >> (8u + i)) & 1u) << m;
            once |= (uint32_t)lm_once(s, i, 1u) << m;
            m++;
        }
    if (kit) {                                      /* (0.6.82) on a drum kit: SEQ plays the pattern's sounds (cp_kit_arp); */
        int32_t dp = climb;                         /* OCT RAND TONAL (0.9.18: WALK) tune the hits, BEND bends them */
        if (arpq) {
            uint8_t kn[4 + NLANE];
            uint32_t c2 = cp_kit_arp(t, arp, ac, aa, t->loops, kn);
            for (i = 0; i < c2 && m < 4u + NLANE; i++) {
                nn[m] = kn[i];
                vv[m++] = (uint8_t)vel;
            }
        }
        if (t->tonal) dp += scale_steps(t, 60, t->tonal) - 60;
        if (walk) dp += scale_steps(t, 60, walk) - 60;
        if (rnd) {
            int32_t r = ra % 8u ? (int32_t)(ra % 8u) : 1;
            dp += scale_steps(t, 60, (int32_t)(rng() % (uint32_t)(2 * r + 1)) - r) - 60;
        }
        t->dpit = (int8_t)clamp(dp, -48, 48);
        t->dtch = 0;                                /* (0.9.18: TOUCH is each lane's mode, below) */
        t->dbend = (int16_t)(bend ? ((int32_t)(ba % 25u) - 12) * 16 : 0);
        t->dbt = (uint16_t)clamp((int32_t)(step_samples(t, period, t->seq_idx) / CTL), 1, 65535);
    }
    if (slide_in && !kit && !arp && !many && m && t->p[P_VOICE] == V_POLY) {   /* (0.6.81) a POLY chord sliding into
                                                         * this one: its notes glide to these */
        chord_glide(t, nn, vv, m, sk, tie_span(t, period, len));
        t->seq_n = 0;
        for (i = 0; i < m; i++)
            if (!((sk >> i) & 1u))
                t->seq_notes[t->seq_n++] = nn[i];
        t->seq_off = gate;
        t->seq_hold = (s->flags & SF_SLIDE) != 0 || next_tie || chord_auto(t, len);
        return;
    }
    t->slide_glide = (uint8_t)slide_in;
    if (!slide_in)
        seq_release(t);
    t->ss_n = t->ss_i = 0;
    {   /* (0.6.87) HUMAN (0.6.92: one amount): each note's velocity up to +-50 at 100 %, the gate +-40 %, the step up to
         * 40 ms late, its DEC and REL up to +-20 steps of their knobs (voice_start takes hum_env) */
        uint32_t hv = hum_amt(t, P_HVEL), hg = hv, ht = hv;
        t->hum_env = (int8_t)(hv ? hum_pm(hv * 20u / 127u) : 0);
        if (hv)
            for (i = 0; i < m; i++)
                vv[i] = (uint8_t)clamp((int32_t)vv[i] + hum_pm(hv * 50u / 127u), 1, 127);
        if (hg && !many)
            gate = (uint32_t)clamp((int32_t)gate + (int32_t)gate / 100 * hum_pm(hg * 40u / 127u), 16, 0x7FFFFFFF);
        if (ht && !many && !bend && !slide_in) {   /* (late, never early: the step can't play before it starts) */
            uint32_t d = hum_rnd() % (HUM_TMAX * ht / 127u + 1u), cap = period / 3u;
            hdel = d < cap ? d : cap;
        }
        if (t->mic > 0 && !many && !bend && !slide_in)   /* (0.6.88) TIMING late: on top */
            hdel += (uint32_t)t->mic * step_samples(t, period, t->seq_idx) / 64u;   /* (0.9.1: 64ths) */
    }
    sk0 = sk;                                       /* (the notes the step sounds, strummed or not: seq_notes) */
    if (!kit && !arp && m > 1u && t->p[P_CSTR] > 0 && t->p[P_VOICE] == V_POLY) {   /* (0.6.84) STRUM: the chord in
                                                         * one note after another, low to high, as the keys strum it
                                                         * (the first now, the others by seq_tick); within half the gate */
        uint32_t g = chord_gap(t), cap = gate / (2u * (m - 1u)), q, w = 0;
        uint8_t ord[4 + NLANE];
        for (q = 0; q < m; q++)
            if (!((sk >> q) & 1u)) {
                for (j = w; j > 0 && nn[ord[j - 1u]] > nn[q]; j--) ord[j] = ord[j - 1u];
                ord[j] = (uint8_t)q; w++;
            }
        for (q = 1; q < w && t->ss_n < 4u; q++) {       /* (the rest, queued; marked done here) */
            t->ss_nn[t->ss_n] = nn[ord[q]];
            t->ss_vv[t->ss_n++] = vv[ord[q]];
            sk |= 1u << ord[q];
        }
        t->ss_due = t->ss_gap = (uint32_t)clamp((int32_t)(g < cap ? g : cap), 1, 65535);
    }
    if (hdel) {                                     /* (0.6.87) HUMAN TIME: every note of the step queued, hdel late
                                                     * (a strum's after it, its gaps kept; else all at once) */
        uint32_t q, gap = t->ss_n ? t->ss_gap : 0u;
        for (q = 0; q < m; q++)                     /* (the strum's first, the ones not queued yet, to the front) */
            if (!((sk >> q) & 1u) && t->ss_n < sizeof t->ss_nn) {
                for (j = t->ss_n; j > 0u; j--) { t->ss_nn[j] = t->ss_nn[j - 1u]; t->ss_vv[j] = t->ss_vv[j - 1u]; }
                t->ss_nn[0] = nn[q]; t->ss_vv[0] = vv[q];
                t->ss_n++;
                sk |= 1u << q;
            }
        t->ss_gap = gap;
        t->ss_due = hdel ? hdel : 1u;
        t->ss_tch = t->dtch;                        /* (0.8.5: the queued hits keep their TOUCH) */
        gate += hdel;
    }
    if (!kit && m && t->p[P_VOICE] != V_POLY)      /* (0.9.18) a new note on a mono track cuts a pending spill */
        spill_cut(t);
    t->st_inst = t->step_inst;                      /* (0.9.18) what this step strikes: a spilled hit leaves it out */
    t->st_n = (uint8_t)m;
    memcpy(t->st_nn, nn, m < sizeof t->st_nn ? m : sizeof t->st_nn);
    if (arpn && m) {                                /* (0.9.18) an ARP: its first note now, the others spilled */
        m = arp_start(t, nn, vv, m, kit, aa, gate, tie_span(t, period, len));
        sk = 0;
    } else if (hits > 1u && m)                      /* (0.9.18) RATCH: its other hits, from here (seq_spill) */
        spill_start(t, s, nn, vv, m, sk | once, kit, &rt, gate);   /* (LM_ONCE, a lane's own xN: not repeated) */
    if (kit)                                        /* (0.8.5) a kit: each lane's own mode on its hits (TOUCH now; 0.9.18:
                                                     * x2 x4 x8 spill as RATCH does, seq_spill) */
        lane_start(t, s, vel, step_samples(t, period, t->seq_idx));
    if (hits > 1u && rt.shape == RS_RISE)           /* (RISE: the first hit the softest) */
        for (i = 0; i < m; i++)
            vv[i] = (uint8_t)rat_vel(vv[i], RS_RISE, 0u, hits);
    for (i = 0; i < m; i++)
        if (!((sk >> i) & 1u)) {
            uint32_t a0 = vage, dt0 = t->dtch, lt = kit ? lm_touch(step_lmode(s, drum_lane(nn[i]))) : 0u;
            if (lt)
                t->dtch = (uint8_t)lt;
            seq_note_on(t, nn[i], vv[i]);
            t->dtch = (uint8_t)dt0;
            if (bend && !kit && vage != a0) {   /* (0.6.42) BEND: its voice glides over the step */
                uint32_t k2, ticks = step_samples(t, period, t->seq_idx) / CTL;
                int32_t d = ((int32_t)(ba % 25u) - 12) * 16;
                for (k2 = 0; k2 < NVOICE; k2++) {
                    voice_t *v = &t->v[k2];
                    if (v->age == vage && v->active) {
                        v->pitch_cur = v->pitch16;
                        v->pitch16 = clamp(v->pitch16 + d, 0, 2047);
                        v->gstep = glide_q8(d, ticks);
                    }
                }
            }
        }
    t->dpit = 0;                                    /* (0.6.82: the keys' hits as they are) */
    t->dbend = 0;
    t->dtch = 0;
    if (slide_in)                                   /* release what is not held over */
        for (i = 0; i < t->seq_n; i++) {
            for (j = 0; j < m && nn[j] != t->seq_notes[i]; j++)
                ;
            if (j == m)
                seq_note_off(t, t->seq_notes[i]);
        }
    t->seq_n = 0;
    for (i = 0; i < m; i++)
        if (!((sk0 >> i) & 1u))
            t->seq_notes[t->seq_n++] = nn[i];
    t->seq_off = gate;
    t->seq_hold = !many && ((s->flags & SF_SLIDE) != 0 || next_tie ||   /* next step a TIE: keep the notes to it */
                                 (!kit && !arp && chord_auto(t, len)));   /* (0.6.81) GLIDE on POLY: to the next chord */
}

#include "fugue.c"                                 /* (JIANT 0.6.52) SEQ > FUGUE: three more playheads a track */

static void seq_tick(track_t *t, uint32_t n)
{
    uint32_t period, len, lon;
    if (t->ss_i < t->ss_n) {                         /* (0.6.84) STRUM: the step's chord coming in */
        if (t->ss_due > n) {
            t->ss_due -= n;
        } else {
            do {                                    /* (0.6.87: HUMAN TIME without a strum: the chord together) */
                t->dtch = t->ss_tch;
                if (trk_kit(t)) {                       /* (0.8.5) its lane's TOUCH, from the step playing */
                    const step_t *ps = &seq_steps(t)[seq_src(t, t->seq_idx)];
                    uint32_t lt = lm_touch(step_lmode(ps, drum_lane(t->ss_nn[t->ss_i])));
                    if (lt)
                        t->dtch = (uint8_t)lt;
                }
                seq_note_on(t, t->ss_nn[t->ss_i], t->ss_vv[t->ss_i]);
                t->dtch = 0;
                t->ss_i++;
            } while (!t->ss_gap && t->ss_i < t->ss_n);
            t->ss_due = t->ss_gap;
        }
    }
    if (t->seq_n && !t->seq_hold) {
        if (t->seq_off <= n)
            seq_release(t);
        else
            t->seq_off -= n;
    }
    if (!song.playing)
        return;
    period = div_samples((uint32_t)t->p[P_SDIV]);
    len = (uint32_t)t->p[P_SLEN];
    if ((lon = LV_ON(trk_index(t))) != 0u)
        n = live_adv(t, n, period, len);            /* (0.9.11) LIVE: the ghost on, the playhead at its RATE, its next
                                                     * step (the window, the direction) as a JUMP's */
    t->seq_pos += n;
    for (;;) {
        uint32_t cur_len = step_samples(t, period, t->seq_idx) + t->ext, start, e = seq_early(t, period, len);
        if (e >= cur_len) e = cur_len - 1u;             /* (0.6.88) TIMING: the next step early, this one shorter */
        if (t->seq_pos < cur_len - e && t->seq_pos != 0x7FFFFFFFu + n)
            break;
        start = t->seq_pos >= 0x7FFFFFFFu;
        t->seq_pos = start ? chain.carry : t->seq_pos - (cur_len - e);   /* (a start: 0; a section in: its carry) */
        t->ext = (uint16_t)(start ? 0u : e);            /* (the next one longer by as much: the grid kept) */
        if (t->jump_to && !start) {                     /* (0.6.42) JUMP: the step it names */
            t->seq_idx = (uint16_t)((t->jump_to - 1u) % (len ? len : 1u));
            t->rev = 0;
        } else if (t->rev && !start && t->seq_idx > 1u) {   /* REV: back towards step 1 */
            t->seq_idx--;
        } else if (t->rev && !start) {
            t->seq_idx = 0;
            t->rev = 0;
        } else {
            t->seq_idx = (uint16_t)((t->seq_idx + 1u) % (len ? len : 1u));
            if (!t->seq_idx && !start) {                /* (0.6.62) the pattern wrapped by itself: a new pass */
                fl_reset((uint32_t)(t - trk));
                fl_pass[(t - trk) % NTRK]++;
            }
        }
        t->jump_to = 0;
        t->step_inst++;                                 /* (0.9.18) one more step started (seq_spill) */
        if (start) t->rev = 0;
        if (!t->seq_idx) t->tonal = 0;                  /* (0.6.42) TONAL lasts to the loop's end */
        if (!t->seq_idx && !start && t->loops < 0xFFFFu)
            t->loops++;                                 /* (0.6.39) one more loop of the pattern (TRIG) */
        if (!lon)
            rec_hold(t, t->seq_idx, len ? len : 1u);   /* (LIVE: the ghost's steps, live_adv) */
        if (trk_index(t) < NPART && dxo_n[trk_index(t)])
            dxo_rec_hold(t, t->seq_idx);                /* (0.8.5) a held ratchet records into it */
        {
            const step_t *s = &seq_steps(t)[seq_src(t, t->seq_idx)];   /* (0.9.18: RANDOM held is LIVE's RND) */
            uint32_t skip = SEQ_ROLLED, i, k;
            uint8_t ja = 0;
            t->mic = (int8_t)ST_MICRO(s);               /* (0.6.88) its own TIMING (late: seq_step) */
            /* the chance first (the one roll, as seq_step made it): a step that does not play applies no lock; the
             * automation and the locks before the notes, so a note-on reads them (eng_drum's KIT, ..) */
            if (!step_rolls(t, s))
                skip = SEQ_MISS;
            motion_step(t, t->seq_idx, &motion, skip == SEQ_ROLLED);   /* (a song: its section's motion is the motion) */
            if (t->rskip_n && t->rskip_idx == t->seq_idx) {
                for (k = 0; k < t->rskip_n; k++) {
                    for (i = 0; i < s->n; i++)
                        if (s->note[i] == t->rskip[k])
                            skip |= 1u << i;
                    for (i = 0; i < NLANE; i++)
                        if (((s->hit >> i) & 1u) && DRUM_LANE_NOTE[i] == t->rskip[k])
                            skip |= 1u << (8u + i);
                }
                t->rskip_n = 0;
            }
            seq_step(t, s, period, skip);
            if (skip != SEQ_MISS && CP_HAS(t, s, CP_JUMP, &ja) && fl_ok(t, CP_JUMP, ja))
                t->jump_to = (uint8_t)(CP_JTO(ja) % NSTEP + 1u);   /* (0.6.42) then that step */
            else if (skip != SEQ_MISS && CP_HAS(t, s, CP_REV, &ja) && t->seq_idx > 0u && fl_ok(t, CP_REV, ja))
                t->rev = 1;                             /* (0.6.42) then backwards to step 1 */
        }
    }
    if (t->sp_left | t->sp_pm | t->ls_on)
        seq_spill(t, n);                            /* (0.9.18) RATCH's hits to come */
    fugue_tick(t, n, period, len);                      /* (0.6.52) */
}

/* MIDI in: the track a channel plays (0..15): G_ROUTE CH1-4 (0) channels 1..4 their parts (5..16 never get
 * here: midi_event drops them), SEL (1) every channel the selected track */
static uint8_t midi_route;                 /* the G_ROUTE events_block last saw (a change to CH1-4: midi_route_ch14) */
static track_t *midi_track(uint32_t ch) { return ch < NROUTE_CH && !song.g[G_ROUTE] ? &trk[ch] : TSEL; }

#include "midi_control.c"
#include "midi_clock.c"

/* the count-in: its first beat now (accented) */
static __attribute__((noinline)) void cin_begin(void)
{
    cin_total = cin_left = (uint8_t)(4u * cin_bars);
    cin_pos = 0;
    memset(cin_n, 0, sizeof cin_n);
    click_req = 2;
}
/* n samples on: a beat ends; the last one starts the sequencer (in this block, before seq_tick: step 0 with it) */
static __attribute__((noinline)) void cin_block(uint32_t n)
{
    uint32_t b = click_beat_len();
    cin_pos += n;
    if (cin_pos < b)
        return;
    cin_pos -= b;
    cin_pos = cin_pos < b ? cin_pos : 0u;
    if (--cin_left) {
        click_req = (uint8_t)((cin_total - cin_left) % 4u ? 1u : 2u);
        return;
    }
    seq_start();
    cin_flush = 1;
}
/* the count-in's kept notes onto step 1, which has just started (events_block, after seq_tick): as if played now */
static __attribute__((noinline)) void cin_record(void)
{
    uint32_t i, k;
    cin_flush = 0;
    for (i = 0; i < NTRK; i++) {
        track_t *t = &trk[i];
        for (k = 0; k < cin_n[i]; k++)
            if (rec_on(t)) {
                dx_in_rat = (uint8_t)(cin_oct[i][k] & 15u);
                dx_in_tch = (uint8_t)(cin_oct[i][k] >> 4);
                rec_note(t, cin_note[i][k], cin_vel[i][k]);
                dx_in_rat = dx_in_tch = 0;
                if ((cin_up[i] >> k) & 1u)
                    rec_release(t, cin_note[i][k]);
            }
        cin_n[i] = 0;
    }
}
/* the metronome's beat: adv samples on while the transport runs (click.c sounds it in this block) */
static void click_tick(uint32_t adv)
{
    uint32_t p = div_samples(2), beat = 0;
    if (clk_pos == CLK_START) {
        clk_pos = 0;
        clk_step = 0;
        clk_n = 0;
        beat = 2;
    } else {
        clk_pos += adv;
        while (clk_pos >= p) {                      /* (as seq_tick: the remainder kept) */
            clk_pos -= p;
            clk_step = (clk_step + 1u) & 15u;
            clk_n++;
            if (!(clk_step & 3u))
                beat = clk_step ? 1u : 2u;
        }
    }
    if (beat && click_wanted())
        click_req = (uint8_t)beat;
}
/* the beat of the bar the click is on (0..3: 0 the accented first), -1 stopped: the count-in's, else the playing
 * transport's (clk_step, internal or external clock). It moves in the block a beat clicks in (the header's metronome,
 * ui_draw.c, swings on it; CLICK OFF it moves all the same) */
static int32_t click_beat_now(void)
{
    if (cin_left)
        return (int32_t)((uint32_t)(cin_total - cin_left) % 4u);
    if (!song.playing)
        return -1;
    return clk_pos == CLK_START ? 0 : (int32_t)(clk_step >> 2);
}

/* everything that happens between two rendered blocks */
static void events_block(uint32_t n)
{
    uint32_t i, pr, fo, seq_n = n;
    uint32_t clock_mode = (uint32_t)song.g[G_CLOCK];
    if (midi_clock.mode != clock_mode) {
        seq_stop();
        memset(&midi_clock, 0, sizeof midi_clock);
        midi_clock.mode = (uint8_t)clock_mode;
        midi_beat_samples = 0;
    }
    mo_block(n);                                    /* (0.6.72) MIDI OUT: its setting, the clock */
    if (cin_left && !clock_mode)
        cin_block(n);
    if (transport_req == 1u) {
        if (clock_mode)
            midi_clock_transport(0xFAu, fm1_ms);
        else if (cin_left)
            ;                                       /* counting in already (the UI's PLAY stops it: seq_counting) */
        else if (cin_bars && song.rec && !song.playing && !chain.armed)
            cin_begin();                            /* recording from stop: the count-in first */
        else
            seq_start();
        transport_req = 0;
    } else if (transport_req == 2u) {
        seq_stop();
        transport_req = 0;
    } else if (transport_req == 3u) {                 /* GLO + PLAY: from the top without stopping (INT clock) */
        if (!clock_mode) {
            seq_stop();
            seq_start();
        }
        transport_req = 0;
    }
    if (midi_route != (uint8_t)song.g[G_ROUTE]) {     /* ROUT changed: CH1-4 lets go of channels 5..16 */
        midi_route = (uint8_t)song.g[G_ROUTE];
        if (!midi_route)
            midi_route_ch14();
    }
    pr = panic_req;
    panic_req = 0;
    if (pr)
        mo_flush_of(pr);                               /* (0.6.72) their MIDI OUT notes too */
    fo = pr;                                           /* (0.6.13) sound loads: what sounds fades on the old sound */
    if (midi_in_overflow) {                            /* a lost note-off must never leave a held note */
        mi_r = mi_w;
        memset(midi_sel_on, 0, sizeof midi_sel_on);
        memset(midi_ch, 0, sizeof midi_ch);
        memset(midi_owners, 0, sizeof midi_owners);
        midi_hint = 0;
        for (i = 0; i < NTRK; i++) {
            trk[i].rh_n = trk[i].rskip_n = 0;
            trk[i].seq_n = trk[i].seq_hold = trk[i].slide_glide = trk[i].sp_left = trk[i].sp_pm = trk[i].ls_on = 0;
        }
        pr |= (1u << NTRK) - 1u;
        RING_PUBLISH();
        midi_in_overflow = 0;
    }
    for (i = 0; i < NTRK; i++) {
        track_t *t = &trk[i];
        if (((latch_drop_req >> i) & 1u) || (((latch_was >> i) & 1u) && !key_latch(t))) {
            latch_drop_req &= (uint8_t)~(1u << i);  /* (0.6.19) a clear, or LATCH left (HOLD off, ARP on, whatever
                                                     * the keys: a key down kept it all sounding, and stacking) */
            latch_drop(t);
        }
        latch_was = (uint8_t)(key_latch(t) ? latch_was | 1u << i : latch_was & ~(1u << i));
        if ((pr >> i) & 1u) {
            if (i < NPART)
                dxo_n[i] = 0;                       /* (0.8.5) the held ratchets stop */
            if ((latch_was >> i) & 1u)
                latch_release(t);                   /* (0.6.19: MIDI OUT's latched notes end too) */
            midi_forget_track(i);
            {   /* (0.6.41) another engine: what is held sounds on in it (kept past the release, which forgets) */
                uint8_t hn[4], hv[4];
                uint32_t hc = t->eng_req != t->engine ? xf_held(t, hn, hv) : 0u, h;
                trk_all_off(t);
                for (h = 0; h < hc; h++) { t->xp_note[h] = hn[h]; t->xp_vel[h] = hv[h]; }
                t->xp_n = (uint8_t)hc;
            }
            if ((fo >> i) & 1u)
                trk_fade_out(t);                        /* (0.6.13: a sound load: on the old values, ~3 ms; voice.c) */
            t->nheld = 0;
            t->arp_phys = 0;
            arp_forget(t);
        }
        engine_block(t);                              /* engine switch: fade, then switch (voice.c) */
        /* ARP turned off, or HOLD released with no key down: drop the latched chord */
        if ((t->armp && !t->p[P_AMODE]) || (t->aholdp && !t->p[P_AHOLD] && !t->arp_phys)) {
            if (!t->armp && t->aholdp && !t->p[P_AHOLD])
                latch_release(t);                       /* (0.6.14) LATCH off: its notes go */
            t->nheld = 0;
            if (!t->p[P_AMODE])
                t->arp_phys = 0;
            if (t->arp_note)
                arp_silence(t);
        }
        t->armp = t->p[P_AMODE];
        t->aholdp = t->p[P_AHOLD];
    }
    keyboard_block();
    while (mi_r != mi_w) {                            /* USB-MIDI (and TRS) in */
        uint32_t at = mi_r % MQ, pkt = midi_in_q[at], status = (pkt >> 8) & 0xFFu;
        uint32_t st = status & 0xF0u, ch = status & 0x0Fu;
        uint32_t d1 = (pkt >> 16) & 0x7Fu, d2 = (pkt >> 24) & 0x7Fu;
        uint32_t source = midi_in_source[at] ? midi_in_source[at] : 1u, ms = midi_in_ms[at];
        mi_r++;
        if (status >= 0xF8u) {
            if (clock_mode == source) {
                if (status == 0xF8u)
                    midi_clock_pulse(ms);
                else
                    midi_clock_transport(status, ms);
            }
        } else
            midi_event(st, ch, d1, d2);
    }
    if (clock_mode) {
        seq_n = 0;
        if (song.playing) {
            uint32_t last = midi_clock.have_pulse ? midi_clock.last_ms : midi_clock.start_ms;
            if (fm1_ms - last > 500u) {
                seq_stop();                         /* cable loss must not leave a running held note */
                midi_clock.tempo_valid = 0;
            } else
                seq_n = midi_clock_advance(fm1_ms);
        }
    }
    chain_tick(seq_n);
    live_q(seq_n);                                  /* (0.9.11) LIVE: what goes in now (seq_live.c; idle: a test) */
    for (i = 0; i < NTRK; i++)
        seq_tick(&trk[i], seq_n);
    for (i = 0; i < NPART; i++)                       /* (0.8.5) the kit's octaves held: their ratchets */
        if (dxo_n[i])
            dxo_tick(&trk[i], clock_mode ? seq_n : n);
    if (cin_flush)
        cin_record();
    if (song.playing)
        click_tick(seq_n);
    for (i = 0; i < NPART; i++)
        arp_step(&trk[i], clock_mode ? seq_n : n, clock_mode && song.playing ? seq_n : n);
    beat_pos += clock_mode ? seq_n : n;               /* the beat the ARP LED flashes on (ui_leds) */
    if (beat_pos >= beat_samples()) {
        beat_pos -= beat_samples();
        beat_pos = beat_pos < beat_samples() ? beat_pos : 0u;
        beat_n = (beat_n + 1u) & 3u;
        if (!beat_n && song.playing)
            bar_n++;
    }
    if (song.playing)
        song.tick++;
}

#include "song_play.c"                           /* the song: the ISR's part (song_chain.c) */
