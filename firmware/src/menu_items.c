/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments
 * Portions Copyright (C) 2026 Juan Ignacio Antonucci (Juan Jiant) */
/* MENU's rows and their settings: what ui_menu.c draws and steps, and what the editor reads and sets (editor_menu.c
 * MENU_DESC / MENU_SET). One apply path for both: menu_put. A row is a setting (its MI_* is its place in the menu's
 * tables, not stored: the editor's ids are editor_menu.c ED_MENU's). (0.9.18) On nine pages of up to four knobs in five
 * tabs (MPAGE): SCREEN (LARGE; SCOPE ANIMATION SCREEN SLEEP; LEDS: LED GLOW SCALE LEDS), AUDIO
 * (SPEAKER EQ TRACKS LOAD BPM LOCK; CLICK CLICK LEVEL COUNT-IN), MIDI (MIDI OUT LEARN LEARN CLEAR), USB (USB AUDIO USB
 * LEVEL), SYSTEM (CONTROLS: HOLD TIME KNOB SPEEDUP FX LATCH CALIBRATION; SYSTEM: RESTORE LAST CPU CORES CPU CLOCK, what
 * a power-on does, and ABOUT). Until 0.9.17 one list in the
 * tabs DISPLAY CONTROL AUDIO MIDI SYSTEM, SELECT a row, any knob its value; the stored values and their meanings are
 * the same. LEARN (OFF / ON) is not a setting: it arms MIDI LEARN (midi_learn.c ml_set) and is never saved; LEARN
 * CLEAR is an action: OCT+ clears the map (ml_clear_all), its card shows how many CCs are learned. Neither is offered
 * to the editor (editor_menu.c ED_MENU); the map travels with the settings */
#include "cpu_safe.c"        /* (0.9.12) MENU > SYSTEM > CPU CORES, CPU CLOCK; the last crash kept in the settings */
enum { MI_SCOPE, MI_LARGE, MI_ANIM, MI_LEDS, MI_SCROFF, MI_HOLD, MI_ACCEL, MI_LATCH, MI_BPMLOCK, MI_SCLLED,
       MI_LOWCUT, MI_USB,
       MI_CLICK, MI_CLKLVL, MI_COUNTIN, MI_TRACKS, MI_LOAD, MI_MIDIOUT, MI_LEARN, MI_LCLEAR, MI_SERIAL, MI_RESTORE, MI_CPU,
       MI_CLOCK, MI_MOTION, MI_PANEL, MI_ABOUT, MI_COUNT };   /* (GARDEN: MOTION, ui.c ui_motion; PANEL and ABOUT take the next numbers)
                      * (1.1: the metronome's rows; 1.2: RESTORE LAST, SCALE LEDS; 0.9.12: MIDI
                      * OUT, LEARN, LEARN CLEAR; CPU, CPU CLOCK, cpu_safe.c; 0.9.18: INIT SYSTEM gone, SAVE > TOOLS > NEW
                      * PROJECT is the same) */
#define MI_VALUES MI_PANEL
#define MI_NONE 0xFFu
static int menu_valued(uint32_t row) { return row < MI_VALUES && row != MI_LCLEAR; }   /* (a row with a value) */
/* (0.9.18) the full names (the panel's rows, the editor's MENU_DESC; plain words, at most 12 characters) */
static const char *const MI_NAME[MI_COUNT] = {"SCOPE", "LARGE", "ANIMATION", "LED GLOW", "SCREEN SLEEP",
                                              "HOLD TIME", "KNOB SPEEDUP", "FX LATCH", "BPM LOCK", "SCALE LEDS",
                                              "SPEAKER EQ", "USB LEVEL", "CLICK", "CLICK LEVEL", "COUNT-IN", "TRACKS", "LOAD",
                                              "MIDI OUT", "LEARN", "LEARN CLEAR", "USB AUDIO",
                                              "RESTORE LAST", "CPU CORES", "CPU CLOCK", "MOTION", "CALIBRATION", "ABOUT"};
/* (0.9.18) the MENU as the rest of the system (rule 1..6): pages of four knobs, KNOB n the page's setting n, SELECT walks
 * the pages, ALGORITHM jumps to the next / previous tab; the tabs the groups (a dot a page), the footer MENU . TAB . page
 * n/N (a page named as its tab, or its tab and a number: MENU . page). Each row on one page (tests/ui_test.c checks it); the editor gets a row's tab after its MENU_DESC reply (tab
 * indexes are not ids: 0.9.18 regrouped them). An action (LEARN CLEAR, CALIBRATION, ABOUT) is a card armed by its knob,
 * OCT+ does it */
enum { MTAB_SCREEN, MTAB_AUDIO, MTAB_MIDI, MTAB_USB, MTAB_SYSTEM, MTAB_COUNT };
static const char *const MTAB_NAME[MTAB_COUNT] = {"SCREEN", "AUDIO", "MIDI", "USB", "SYSTEM"};
#define MP_COUNT 10u
typedef struct { char name[9]; uint8_t tab, row[4]; } menu_page_t;
static const menu_page_t MPAGE[MP_COUNT] = {   /* (a page opens on its first setting: a value, never an action) */
    {"SCREEN", MTAB_SCREEN, {MI_LARGE, MI_NONE, MI_NONE, MI_NONE}},   /* (GARDEN: COLOR, SKIN and STYLE are gone) */
    {"SCREEN 2", MTAB_SCREEN, {MI_SCOPE, MI_ANIM, MI_SCROFF, MI_NONE}}   /* (GARDEN: ECO STYLE retired) */,
    {"LEDS", MTAB_SCREEN, {MI_LEDS, MI_SCLLED, MI_NONE, MI_NONE}},
    {"MOTION", MTAB_SCREEN, {MI_MOTION, MI_NONE, MI_NONE, MI_NONE}},   /* (GARDEN) how fast the animations run */
    {"AUDIO", MTAB_AUDIO, {MI_LOWCUT, MI_TRACKS, MI_LOAD, MI_BPMLOCK}},
    {"CLICK", MTAB_AUDIO, {MI_CLICK, MI_CLKLVL, MI_COUNTIN, MI_NONE}},
    {"MIDI", MTAB_MIDI, {MI_MIDIOUT, MI_LEARN, MI_LCLEAR, MI_NONE}},
    {"USB", MTAB_USB, {MI_SERIAL, MI_USB, MI_NONE, MI_NONE}},
    {"CONTROLS", MTAB_SYSTEM, {MI_HOLD, MI_ACCEL, MI_LATCH, MI_PANEL}},
    {"SYSTEM", MTAB_SYSTEM, {MI_RESTORE, MI_CPU, MI_CLOCK, MI_ABOUT}},   /* (RESTORE LAST, CPU CORES, CPU CLOCK: at power-on) */
};
static uint32_t menu_page_of(uint32_t row)             /* the page a row is on */
{
    uint32_t p, c;
    for (p = 0; p < MP_COUNT; p++)
        for (c = 0; c < 4u; c++)
            if (MPAGE[p].row[c] == row)
                return p;
    return 0;
}
static uint32_t mi_tab(uint32_t row) { return MPAGE[menu_page_of(row)].tab; }
static uint32_t mtab_page(uint32_t t)                  /* a tab's first page */
{
    uint32_t p = 0;
    while (p < MP_COUNT && MPAGE[p].tab != t)
        p++;
    return p;
}
static uint32_t mtab_pages(uint32_t t)                 /* .. and how many it has */
{
    uint32_t p, n = 0;
    for (p = 0; p < MP_COUNT; p++)
        n += MPAGE[p].tab == t;
    return n;
}
static const char *const SCOPEOPT_N[4] = {"OFF", "ON", "BUBBLES", "ORBIT"};   /* (0.9.7) ui.c ui_scope; (0.9.18) 2, 3 */
static const char *const MOTION_N[4] = {"100%", "50%", "150%", "200%"};   /* (GARDEN) ui.c ui_motion, MOTION_Q8 */
/* the two-valued rows: a bit of ui_prefs (PREF_REC: of ui_rec_prefs, ui.c PREF_SCALE_LEDS) and its names (bit clear =
 * the default, bit set). A step up (a knob right,
 * OCT+) = ON on the ON / OFF rows (the switch's knob to the right; ANIM, USB SERIAL: their bit clears), else the
 * second name (STYLE LINE, USB LEVEL FIXED); a step down the other (menu_step) */
typedef struct { uint8_t row; uint16_t bit; const char *name[2]; } menu_flag_t;
static const menu_flag_t MENU_FLAGS[] = {
    {MI_LARGE, PREF_LARGE, {"OFF", "ON"}},            /* #15 / Discussion #80: ON, big knob labels and values (ui.c large_kind) */
    {MI_ANIM, PREF_ANIM_OFF, {"ON", "OFF"}},          /* #46: OFF, values snap (ui_draw.c roll_note, ui_graph.c pr_follow) */
    {MI_ACCEL, PREF_ACCEL_OFF, {"ON", "OFF"}},        /* #52: ui_input.c accel (JIANT: ON by default) */
    {MI_LATCH, PREF_LATCH, {"OFF", "ON"}},
    {MI_BPMLOCK, PREF_BPM_LOCK, {"OFF", "ON"}},        /* #58: ON, DICE ALL keeps the tempo (0.6.67: SELECT alone never sets it) */
    {MI_RESTORE, PREF_RESTORE_ON, {"OFF", "ON"}},      /* 1.2, #130; JIANT 0.6.40: OFF (the default), no autosave, power-on as
                                                        * INIT SYSTEM; ON, the last session back (project.c) */
    {MI_SCLLED, PREF_SCALE_LEDS, {"OFF", "ON"}},      /* 1.2, Discussion #127: ON, the keys show the scale (ui_input.c) */
};
/* SPEAKER EQ (settings.lowcut, fx.c fx_lowcut): an EQ on the master for the small speaker, not a speaker switch
 * (#42: "OFF" read as the speaker off). FLAT is the old OFF (0, stored as before). The built-in speaker cannot be
 * turned off from the firmware (no amp enable or mute line is known); the EQ reaches
 * the headphone / line out and USB audio too (one DAC, uac_tap reads the master) */
static const char *const SPK_EQ[3] = {"FLAT", "LOWCUT", "BASS+"};
static const char *const TRKS_N[3] = {"4", "6", "8"};    /* (0.9.19) MENU > TRACKS: the tracks that play */
/* (0.7.0) USB LEVEL: MASTER (USB follows the knob, the default), FIXED (full level, the knob after: #42), then fixed
 * and attenuated (-6 .. -24 dB: a phone recording over USB clipped at the full level). ui.c usb_lvl, fx.c fx_usb_gain */
static const char *const USBLVL_N[6] = {"MASTER", "FIXED", "-6 dB", "-12 dB", "-18 dB", "-24 dB"};
/* (0.7.1) MENU > SYSTEM > USB (was USB SERIAL ON / OFF, #67): CONSOLE, the serial console with the stereo audio (the
 * default, PREF_SERIAL_OFF clear: the descriptors as before); STEREO, audio + MIDI only (class 0: the old OFF); STEMS,
 * audio as 10 channels (tracks 1..8 mono, the FX return L R; stems-10: was 8, tracks 1..6; fx.c, usb.c), no console; (0.7.5) IN+OUT, the stereo
 * audio and the host's audio output too (usb.c usb_duplex: the computer or the phone plays into the last track), no
 * console; (0.8.0) STEMS+IN, the stems with the host's audio output too (10 channels in: tracks 1..8, then the host's
 * audio as the last track mixed it and as it came; the phone app records a voice through the FM-1 and moves it by
 * the latency). ui.c usb_out */
static const char *const USBOUT_N[5] = {"CONSOLE", "STEREO", "STEMS", "IN+OUT", "STEMS+IN"};
static const char *const HOLD_N[4] = {"0.3 s", "0.4 s", "0.5 s", "0.6 s"};   /* (HOLD TIME: the menu's and the editor's names) */
/* 1.1 (Discussion #131): the metronome (click.c) and the count-in (seq.c); ui.c ui_rec_prefs holds them */
static const char *const CLICK_N[3] = {"OFF", "REC", "ON"};          /* REC: while a track is armed and playing */
static const char *const CLKLVL_N[3] = {"LOW", "MID", "HIGH"};
static const char *const COUNTIN_N[3] = {"OFF", "1 BAR", "2 BARS"};
/* 1.1.5: SCREEN OFF (ui.c scr_*: the screen dark after this long without panel input; NEVER the default since 1.1.5.1) */
static const char *const SCROFF_N[5] = {"NEVER", "5 MIN", "15 MIN", "30 MIN", "60 MIN"};
/* (JIANT 0.6.72) MIDI OUT over USB (ui.c midi_out_mode, seq.c MO_*): the keys; and the clock + START / STOP; and every
 * note the FM-1 plays; nothing */
static const char *const MIDIOUT_N[4] = {"KEYS", "CLOCK", "ALL", "OFF"};

/* MENU > USB SERIAL (#67). The serial console is a developer tool (README: FELUCCA_CDC). ON (the default) presents it,
 * the descriptors byte for byte as before; OFF re-enumerates as audio + MIDI only, device class 0 (the bytes of a
 * FELUCCA_CDC=0 build): macOS 13-15 then attach their USB audio driver (with the console Apple's CDC composite driver
 * takes the device and the audio input never appears). USB-MIDI stays, and with it the editor, the update installer
 * and the soft key (SysEx on EP1). Applied at boot before USB starts (main.c: enumerated with it from the start, never
 * on then off) and while the menu is closed (ui_input: one re-enumeration when it closes, not one per KNOB 1 step).
 * A change from the editor (MENU_SET) waits 200 ms (menu_serial_at) so its reply leaves before the device drops off
 * the bus: the whole device re-enumerates, USB-MIDI and audio too */
static uint32_t menu_serial_at;                        /* fm1_ms | 1 of the editor's change; 0 none */
static void usb_serial_apply(void)
{
    if (menu_serial_at && fm1_ms - menu_serial_at < 200u)
        return;
    menu_serial_at = 0;
#if FELUCCA_CDC
    usb_cdc_switch(FELUCCA_CDC_DEFAULT && !(ui_prefs & PREF_SERIAL_OFF) && !usb_out);   /* (a FELUCCA_CDC_DEFAULT=0 build: off) */
#endif
#if FELUCCA_UAC
    usb_stems_switch(usb_out == 1u || usb_out == 3u);  /* (0.7.1) MENU > USB STEMS: the 8-channel device */
    usb_duplex_switch(usb_out == 2u || usb_out == 3u); /* (0.7.5) MENU > USB IN+OUT: the audio output too */
#endif                                                 /* ((0.8.0) STEMS+IN: both, the 10-channel device) */
    fx_stems = (uint8_t)(usb_out == 1u ? 1u : usb_out == 3u ? 2u : 0u);   /* (the parts write their stems from the next block) */
}
static const menu_flag_t *menu_flag(uint32_t row)
{
    uint32_t i;
    for (i = 0; i < NELEM(MENU_FLAGS); i++)
        if (MENU_FLAGS[i].row == row)
            return &MENU_FLAGS[i];
    return 0;
}

/* a row's value as 0..menu_n(row) - 1 in the order the menu steps it (LEDS: OFF DIM LO DIM HI INV, LEDS_MENU) */
static uint32_t menu_n(uint32_t row)
{
    return row == MI_SCOPE ? 4u : row == MI_LOWCUT || (row >= MI_CLICK && row <= MI_COUNTIN) || row == MI_LOAD || row == MI_TRACKS ? 3u :
           row == MI_HOLD || row == MI_MIDIOUT ? 4u : row == MI_USB ? 6u : row == MI_SERIAL ? 5u : row == MI_LEDS ? LEDS_COUNT : row == MI_SCROFF ? NELEM(SCROFF_N) : row == MI_MOTION ? NELEM(MOTION_N) : 2u;
}
static uint32_t menu_get(uint32_t row)
{
    const menu_flag_t *f = menu_flag(row);
    uint32_t i = 0;
    if (f)
        return ((ui_prefs | (uint32_t)ui_rec_prefs << 8) & f->bit) != 0;
    switch (row) {
    case MI_SCOPE: return ui_scope % 4u;
    case MI_LOWCUT: return settings.lowcut % 3u;
    case MI_USB: return (ui_prefs & PREF_USB_FIXED) ? 1u + usb_lvl % 5u : 0u;
    case MI_SERIAL: return usb_out == 3u ? 4u : usb_out == 2u ? 3u : usb_out ? 2u : (ui_prefs & PREF_SERIAL_OFF) ? 1u : 0u;
    case MI_HOLD: return settings_hold % 4u;
    case MI_SCROFF: return scr_get();
    case MI_MOTION: return ui_motion % NELEM(MOTION_N);
    case MI_CLICK: case MI_CLKLVL: case MI_COUNTIN: return rp_get(row - MI_CLICK);
    case MI_LOAD: return load_get();
    case MI_TRACKS: return trks_pos();                  /* (0.9.19: 4, 6, 8) */
    case MI_MIDIOUT: return midi_out_mode & 3u;
    case MI_CPU: case MI_CLOCK: return cpu_get(row == MI_CLOCK);
    case MI_LEARN: return ml.on;
    case MI_LEDS:
        while (i + 1u < LEDS_COUNT && LEDS_MENU[i] != settings_leds)
            i++;
        return i;
    }
    return 0;
}
static const char *menu_vname(uint32_t row, uint32_t v)
{
    const menu_flag_t *f = menu_flag(row);
    if (f)
        return f->name[v & 1u];
    switch (row) {
    case MI_SCOPE: return SCOPEOPT_N[v & 3u];
    case MI_LEARN: return SCOPEOPT_N[v & 1u];
    case MI_LOWCUT: return SPK_EQ[v % 3u];
    case MI_USB: return USBLVL_N[v % 6u];
    case MI_SERIAL: return USBOUT_N[v % 5u];
    case MI_HOLD: return HOLD_N[v & 3u];
    case MI_CLICK: return CLICK_N[v % 3u];
    case MI_CLKLVL: return CLKLVL_N[v % 3u];
    case MI_COUNTIN: return COUNTIN_N[v % 3u];
    case MI_LOAD: return N_LOAD[v % 3u];
    case MI_TRACKS: return TRKS_N[v % 3u];
    case MI_MIDIOUT: return MIDIOUT_N[v & 3u];
    case MI_CPU: case MI_CLOCK: return CPU_N[row == MI_CLOCK][v & 1u];
    case MI_SCROFF: return SCROFF_N[v % NELEM(SCROFF_N)];
    case MI_MOTION: return MOTION_N[v % NELEM(MOTION_N)];
    case MI_LEDS: return LEDS_NAME[LEDS_MENU[v % LEDS_COUNT]];
    }
    return "";
}
/* set a row (v below menu_n(row)) and do what the menu does on a change: the EQ and USB LEVEL at once;
 * LARGE, ANIM, LEDS, HOLD, the flags are read where they are used, every frame;
 * USB SERIAL when the menu has closed (usb_serial_apply). Saved by the caller (menu_close, the editor) */
static void menu_put(uint32_t row, uint32_t v)
{
    const menu_flag_t *f = menu_flag(row);
    if (f) {
        if (f->bit & PREF_REC)
            ui_rec_prefs = (uint8_t)(v ? ui_rec_prefs | f->bit >> 8 : ui_rec_prefs & ~(f->bit >> 8));
        else
            ui_prefs = (uint8_t)(v ? ui_prefs | f->bit : ui_prefs & ~f->bit);
        fx_usb_fixed = (ui_prefs & PREF_USB_FIXED) != 0u;   /* (at once; every frame too: ui_input) */
        return;
    }
    switch (row) {
    case MI_SCOPE: ui_scope = (uint8_t)(v & 3u); ui.force = 1; break;   /* (0.9.7) the EDIT pages so from the next frame */
    case MI_LOWCUT: settings.lowcut = v; fx_lowcut = (uint8_t)v; break;
    case MI_USB:                                   /* (at once; every frame too: ui_input) */
        ui_prefs = (uint8_t)(v ? ui_prefs | PREF_USB_FIXED : ui_prefs & ~PREF_USB_FIXED);
        usb_lvl = (uint8_t)(v ? (v - 1u) % 5u : 0u);
        fx_usb_fixed = v != 0u;
        fx_usb_gain = USB_GAIN[usb_lvl];
        break;
    case MI_SERIAL:                                /* (0.7.1) applied when the menu closes (usb_serial_apply) */
        ui_prefs = (uint8_t)(v ? ui_prefs | PREF_SERIAL_OFF : ui_prefs & ~PREF_SERIAL_OFF);
        usb_out = (uint8_t)(v == 2u ? 1u : v == 3u ? 2u : v == 4u ? 3u : 0u);   /* (0.7.5: 2 IN+OUT; 0.8.0: 3 STEMS+IN) */
        break;
    case MI_HOLD: settings_hold = (uint8_t)v; break;
    case MI_SCROFF: scr_put(v); break;                 /* (read every frame: ui.c scr_frame) */
    case MI_MOTION: ui_motion = (uint8_t)(v % NELEM(MOTION_N)); break;   /* (GARDEN: read every frame, ui_draw.c) */
    case MI_LEDS: settings_leds = LEDS_MENU[v]; break;
    case MI_CLICK: case MI_CLKLVL: case MI_COUNTIN: rp_put(row - MI_CLICK, v); break;   /* (at once: click.c, seq.c) */
    case MI_LOAD: load_put(v % 3u); break;
    case MI_TRACKS: trks_put(v % 3u); break;            /* (0.9.19: 4, 6, 8; ui.c) */
    case MI_CPU: case MI_CLOCK: cpu_put(row == MI_CLOCK, v & 1u); break;   /* (0.9.12, cpu_safe.c) */
    case MI_MIDIOUT: midi_out_mode = (uint8_t)(v & 3u); break;
    case MI_LEARN: ml_set(v); break;                   /* (0.9.12: armed, or off; not saved) */   /* (seq.c reads it every frame: ui_input) */         /* (at once: voice.c sys_load) */
    }
}
/* the menu's step, any of KNOB 1..4 or OCT+ (s > 0) / OCT- (s < 0): the next / previous value, stopping at the ends;
 * On an ON / OFF row up is ON (MENU_FLAGS) */
static uint32_t menu_step(uint32_t row, int32_t s)
{
    const menu_flag_t *f = menu_flag(row);
    uint32_t v = menu_get(row), n = menu_n(row);
    if (f && str_eq(f->name[0], "ON"))                 /* (ANIM, USB SERIAL, RESTORE LAST: ON is their value 0) */
        s = -s;
    if (s > 0)
        return v + 1u < n ? v + 1u : v;
    if (s < 0)
        return v ? v - 1u : 0u;
    return v;
}
