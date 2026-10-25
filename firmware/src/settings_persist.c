/* SPDX-License-Identifier: GPL-3.0-only
 * Portions Copyright (C) 2026 Juan Ignacio Antonucci (Juan Jiant) */
/* Shared PER4 layout: each feature updates only its own fields, preserving
 * the other feature's saved preferences when either is built independently.
 * PER1/PER2 are upstream; PER3 added bold; PER4 added favorites.
 * palette: UI_PAL_TAG + index; an old id (below 20, earlier firmware) is migrated on import.
 * bold: no longer used (one font weight); kept as it was saved, unless it holds the HOLD setting (panel.c).
 * zoom: no longer used (the large readout); kept as it was saved, unless it holds the LEDS setting (panel.c).
 * favorites.factory[15][23]: JIANT_PAL_SEEN (JIANT 0.6.81). [15][14]: ECO STYLE (0 = OG, 1 = GOOBERS; 0 in older settings).[15][20]: SKIN (JIANT 0.8.9; 0 = COLORS; 0.9.7: 3 AIR, new settings'; 0.9.17: 1, the retired BOTTOM, = AIR). [15][19]: SCOPE (0.9.7; 0 = OFF). favorites.factory[15][28..31]: CLICK / CLICK LEVEL / COUNT-IN (1.1), STYLE, MENU's flags, [31] the layers seen (0.9.5: unused, kept as saved) (bytes no
 * engine uses; 0 in older settings = every one's default); [15][27]: SCREEN OFF (1.1.5; 0 = 30 MIN, the default).
 * [15][0..12] (0.9.5): how many times each teaching hint has been given (ui.c hint_given; 0 in older settings = not yet).
 * MENU's flags (ui.c PREF_*) are append-only bits whose 0 is the default: BPM LOCK (16) is clear in every older setting
 * = unlocked, LARGE (32) = OFF, RESTORE LAST OFF (128, 1.2) = ON, read as saved (nothing to migrate, so importing twice changes nothing). */
typedef struct {
    uint32_t magic, palette, lowcut, zoom;
    panel_t panel;
    uint32_t bold;
    struct { uint8_t factory[16][32]; uint32_t user, filter; } favorites;
} persist_t;
#define PERSIST_MAGIC 0x50455234u
#define JIANT_PAL_SEEN 0x4Au                    /* favorites.factory[15][23]: the palette set to JIANT once */

/* Normalize in place; 1 = current, 2 = migrated, 0 = invalid. */
static int settings_import(persist_t *p, int n)
{
    int current = n == (int)sizeof *p && p->magic == PERSIST_MAGIC;
    int old3 = n == (int)(sizeof *p - sizeof p->favorites) && p->magic == 0x50455233u;
    int old2 = n == (int)(sizeof *p - sizeof p->favorites - sizeof p->bold) && p->magic == 0x50455232u;
    int old1 = n == (int)(8u + sizeof(panel_t)) && p->magic == 0x50455231u;
    if (!(current || old3 || old2 || old1)) return 0;
    if (old1) {
        panel_t old;
        memcpy(&old, (uint8_t *)p + 8, sizeof old);
        p->panel = old;
        p->lowcut = p->zoom = 0;
    }
    if (old1 || old2) p->bold = 0;
    if (!current) memset(&p->favorites, 0, sizeof p->favorites);
    if (p->favorites.factory[15][20] > 3u)         /* SKIN (JIANT 0.8.9, ui.c ui_skin; 0.9.7: 3 AIR): unknown = COLORS (0, older
                                                     * settings too: they keep the skin they had) */
        p->favorites.factory[15][20] = 0;
    else if (p->favorites.factory[15][20] == 1u)   /* (0.9.17) BOTTOM, retired: AIR (its word tabs, the knobs' row on top) */
        p->favorites.factory[15][20] = 3u;
    if (p->favorites.factory[15][19] > 3u)         /* SCOPE (0.9.7, ui.c ui_scope; 0.9.18: 2 BUBBLES, 3 ORBIT): unknown = OFF (0,
                                                     * older settings too: 0 and 1 keep their meaning) */
        p->favorites.factory[15][19] = 0;
    if (p->favorites.factory[15][14] > 1u)         /* ECO STYLE (ui.c ui_eco_style): 0 OG, 1 GOOBERS; unknown = OG */
        p->favorites.factory[15][14] = 0;
    if (p->favorites.factory[15][29] > 1u)        /* STYLE (ui.c ui_style): 2, the retired PIXEL = LINE; unknown = FLAT */
        p->favorites.factory[15][29] = p->favorites.factory[15][29] == 2u ? 1u : 0u;
    {   /* CLICK, CLICK LEVEL, COUNT-IN (ui.c ui_rec_prefs): two bits each, 3 is no value: the default (0) */
        uint32_t b = p->favorites.factory[15][28], f;
        for (f = 0; f < 3u; f++)
            if (((b >> (2u * f)) & 3u) == 3u)
                b &= ~(3u << (2u * f));
        p->favorites.factory[15][28] = (uint8_t)b;
    }
    if (p->favorites.factory[15][21] > 3u)         /* USB STEMS / IN+OUT / STEMS+IN (0.7.1, 0.7.5, 0.8.0, ui.c usb_out): unknown = stereo */
        p->favorites.factory[15][21] = 0;
    if (p->favorites.factory[15][22] > 4u)         /* USB LEVEL past FIXED (0.7.0, ui.c usb_lvl): unknown = FULL */
        p->favorites.factory[15][22] = 0;
    if (p->favorites.factory[15][26] > 2u)         /* LOAD (JIANT 0.6.30, ui.c ui_load): unknown = FULL */
        p->favorites.factory[15][26] = 0;
    if (p->favorites.factory[15][24] > 3u)         /* MIDI OUT (JIANT 0.6.72, ui.c midi_out_mode): unknown = KEYS */
        p->favorites.factory[15][24] = 0;
    if (p->favorites.factory[15][17] > 127u)       /* (0.9.12) MENU > SYSTEM > CPU, CPU CLOCK (cpu_safe.c cpu_cfg): unknown =
                                                     * 2 CORES, 360 MHZ (0, older settings too) */
        p->favorites.factory[15][17] = 0;
    if (p->favorites.factory[15][31] != 0x5Au)     /* TRACKS 8 (0.9.19, ui.c trk8_pref, TRK8_ON): any other = 0 (4 or 6 as PREF_SIX says) */
        p->favorites.factory[15][31] = 0;
    if (p->favorites.factory[15][27] > 7u)         /* SCREEN OFF (1.1.5, ui.c SCR_CODE): unknown = 0 (NEVER since 1.1.5.1) */
        p->favorites.factory[15][27] = 0;
    if (p->favorites.factory[15][18] >= 10u)       /* PRESETS' LIST type (JIANT 0.9.12, ui.c list_type; core.h PT_N): unknown
                                                     * = none */
        p->favorites.factory[15][18] = 0;
    p->magic = PERSIST_MAGIC;
    p->palette = palette_to_stored(palette_from_stored(p->palette));
    settings.magic = SETTINGS_MAGIC;
    settings.palette = palette_from_stored(p->palette);
    settings.lowcut = p->lowcut;
    settings.zoom = p->zoom;
    settings_hold = (uint8_t)hold_from_stored(p->bold);
    settings_leds = (uint8_t)leds_from_stored(p->zoom);
#ifdef FELUCCA_FAVORITES
    memcpy(&favorites, &p->favorites, sizeof favorites);
    favorites.filter = favorites.filter == 1u;
#if defined(FM4_NPRESETS) && !FELUCCA_FM4
    {   /* DIGITAL's starred presets (engine 1, retired) -> the FM6 presets that cover them (fm4_convert.c) */
        uint32_t k;
        for (k = 0; k < FM4_NPRESETS; k++)
            if ((favorites.factory[ENGI_DIGITAL][0] >> k) & 1u)
                favorites.factory[ENGI_FM6][FM4_TO_FM6[k] / 8u] |= (uint8_t)(1u << (FM4_TO_FM6[k] % 8u));
        favorites.factory[ENGI_DIGITAL][0] = 0;   /* (0.9.12: the row's bytes 2..31 MIDI LEARN's, midi_learn.c) */
    }
#endif
#ifdef SMP_SET_PERC
    if ((favorites.factory[ENGI_SAMPLE][0] >> SMP_SET_PERC) & 1u) {   /* SAMPLE's PERC (preset 4, retired): DRUM's */
        favorites.factory[ENGI_DRUM][0] |= 1u;                         /* kit (core.h drum_from_perc) */
        favorites.factory[ENGI_SAMPLE][0] &= (uint8_t)~(1u << SMP_SET_PERC);
    }
#endif
#endif
    if (p->panel.magic == PANEL_MAGIC) panel = p->panel;
    return current ? 1 : 2;
}

/* (JIANT 0.6.81) at power-on (persist_boot, not a backup's restore): settings saved by Felucca or an earlier JIANT keep
 * their palette, often GREY or MONO (a black and white screen after installing): set to JIANT once, marked (the mark
 * saved with the next settings: a palette picked after this is kept) */
static void settings_jiant_once(persist_t *p)
{
    if (p->favorites.factory[15][23] == JIANT_PAL_SEEN)
        return;
    p->favorites.factory[15][23] = JIANT_PAL_SEEN;
    p->palette = palette_to_stored(UI_JIANT_INDEX);
    settings.palette = UI_JIANT_INDEX;
#ifdef FELUCCA_FAVORITES
    favorites.factory[15][23] = JIANT_PAL_SEEN;
#endif
}

/* (0.9.7) at power-on with no settings saved (a new FM-1, an erased store): the defaults of new settings, SKIN AIR (settings
 * saved before keep what they hold: 0 COLORS ..) */
static void settings_fresh(void)
{
#ifdef FELUCCA_FAVORITES
    favorites.factory[15][20] = SKIN_AIR;
#endif
}

/* Start with the last imported/saved object, including fields owned by a
 * feature absent from this build. No save-on-boot or extra flash writes. */
static void settings_export(persist_t *p)
{
    p->magic = PERSIST_MAGIC;
    p->palette = palette_to_stored(settings.palette);
    p->lowcut = settings.lowcut;
    p->zoom = settings.zoom = leds_to_stored(settings.zoom, settings_leds);
    p->panel = panel;
    p->bold = hold_to_stored(p->bold, settings_hold);
#ifdef FELUCCA_FAVORITES
    memcpy(&p->favorites, &favorites, sizeof favorites);
#endif
}
