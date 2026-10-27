/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments
 * Copyright (C) 2026 Juan Ignacio Antonucci (Juan Jiant)
 * JIANT 0.5: the beings, in lines. A sound page without a graph of its own (the engine's EDIT pages) shows a creature
 * as a thermal camera's isotherms: four nested outlines of one body, each as hot as the being is there (heat_col:
 * the outer membrane cold, the core hot), the nuclei hottest. Colour is intensity: the output's level warms every
 * line (and swells the body); in silence it cools and rests. Antialiased lines only (ui_organic.c og_line), ~400 a
 * frame. Each knob of the page shapes it by what it is (its label), else by its place:
 *   CUT        the spikes: a round cell closed, opening into spikes as the filter lets the harmonics through
 *   RES        the membrane rings: a fine ripple round the outline
 *   DTN SPRD   two nuclei, apart (the beating of two oscillators)
 *   NOIS RAND BRTH CRSH   the outline frays
 *   WAVE       the body's lobes
 *   (else)     KNOB 1 more spikes, 2 longer, 3 lobes wobbling, 4 more nuclei
 * (DRUM-X's colony retired with GARDEN: its pages draw the kit's pieces, ui_engvis.c vx_kit_page.) */
typedef struct {
    uint8_t k0, kk;                                     /* spikes: k0 + KNOB 1 x kk */
    uint8_t ss;                                         /* their length at KNOB 2 full (16 = the radius x 1.6) */
    uint8_t lob;                                        /* lobes of the membrane */
    uint8_t ax;                                         /* stretched: x * ax / 16 */
    uint8_t tw;                                         /* the spikes twist */
    uint8_t sh;                                         /* their sharpness (a power of the cosine) */
} genome_t;
static const genome_t GENOME[9] = {
    {6, 18, 16, 3, 22, 0, 6},                           /* a radiolarian: a sun of thin rays */
    {3, 5, 14, 2, 26, 3, 2},                            /* an amoeba: its arms swirl */
    {4, 6, 10, 4, 18, 0, 1},                            /* a diatom: blunt */
    {10, 30, 8, 5, 20, 1, 10},                          /* a ciliate: a fringe of hairs */
    {5, 9, 12, 3, 19, 2, 4},                            /* a spore */
    {2, 4, 16, 2, 30, 1, 3},                            /* a worm: long, two horns */
    {8, 12, 12, 6, 17, 0, 8},                           /* a pollen grain */
    {3, 7, 15, 3, 21, 4, 5},                            /* a hydra: curled tentacles */
    {14, 8, 7, 6, 26, 1, 9},                            /* (0.8.3) a larva: long, many short legs */
};
/* (JIANT 0.5) a species per engine: ANALOG the radiolarian, PHASE the swirling amoeba, LOFI the blunt diatom, VOICE the
 * hydra (a mouth of tentacles), WHEEL the ciliate (a fringe of pipes), NOISE the spore, FM6 the pollen grain (its
 * operators' geometry), the retired slots the worm; (0.8.3) ANIMAL the larva */
static const uint8_t EGEN[NENGINES] = {0, 5, 1, 2, 5, 7, 5, 3, 5, 5, 0, 4, 6, 5, 8, 6};   /* SERUM (15, 0.9.10): a pollen grain */
static const genome_t LANE_G_HOME = {12, 0, 10, 3, 18, 0, 6};   /* HOME: a DRUM-X track's being (spiky, a beat) */
typedef struct {
    int32_t spikes, len, wob, lob, nuc, sep, fray, ripple;   /* len wob sep fray ripple 0..256 */
} bfeat_t;
/* a line's colour: heat v; GREY and MONO its grey (their lines blend grey on grey: og_plot) */
static uint16_t bg_col(int32_t v)
{
    uint16_t c = heat_col(v);
    return c;
}
static int32_t bg_sin(uint32_t ang) { return sine_i(ang << 16); }   /* Q15 */
/* (GARDEN, the owner: at MASTER 9 o'clock the scope barely moved) the scope's samples carry the MASTER knob; this
 * gain (Q8) takes it out again, so the pictures follow the music, not the volume (MASTER at 1/32 and under: x 32) */
static int32_t scope_k(void)
{
    int32_t m = (int32_t)song.master_q12;
    return MASTER_FULL * 256 / (m < MASTER_FULL / 32 ? MASTER_FULL / 32 : m);
}
static int32_t scope_s(int32_t v, int32_t k) { return clamp(v * k >> 8, -32767, 32767); }
/* the output's level now, 0..256, slewed (fast up, slow down) */
static int32_t bg_energy(void)
{
    static int32_t e;
    static uint32_t fr = 0xFFFFFFFFu;
    uint32_t w = scope_w, i;
    int32_t pk = 0, v, k = scope_k();
    if (fr == ui.frame)                                 /* (once a frame: a redraw in the same frame draws the same) */
        return e;
    fr = ui.frame;
    for (i = 1; i <= 256u; i += 2u) {
        v = scope_s(scope_buf[(w - i) & (SCOPE_N - 1u)], k);
        if (v < 0) v = -v;
        if (v > pk) pk = v;
    }
    v = pk * 256 / 6000;                                /* (GARDEN: twice the gain, the owner on the device: quiet
                                                         * music barely moved it) */
    if (v > 256) v = 256;
    e += v > e ? (v - e) * 3 / 4 : -((e - v + 5) / 6);   /* (falling: rounded up, so it reaches 0: it stuck at 1..5 in
                                                         * silence, and the FX pictures moved on as if it sounded) */
    return e;
}
#define BG_N 96                                         /* vertices of an outline */
static uint8_t bg_nv_in __attribute__((section(".pool")));   /* (0.9.7) an inner isotherm's at most (0: as the load says) */
/* one outline: the body at (cx, cy), radius r (Q4), x stretched by ax/16; outer: with the spikes, the ripple, the
 * fray. Colour c */
static void bg_outline(const genome_t *g, const bfeat_t *f, int32_t cx, int32_t cy, int32_t r, int outer, uint32_t t,
                       uint16_t c)
{
    /* (0.6.57) LOAD LITE / ECO: fewer vertices, the inner isotherms (no spikes to draw) the coarsest: LITE about half
     * the lines a being, ECO a third; FULL as drawn */
    int32_t i, px = 0, py = 0, nv = !sys_load ? BG_N : outer ? (sys_load == LOAD_ECO ? 48 : 64) : (sys_load == LOAD_ECO ? 24 : 32);
    if (!outer && bg_nv_in && nv > bg_nv_in)
        nv = bg_nv_in;
    for (i = 0; i <= nv; i++) {
        uint32_t th = (uint32_t)(i % nv) * 65536u / (uint32_t)nv;
        int32_t rr = r + ((((r * f->wob) >> 8) * ((9175 * bg_sin((uint32_t)f->lob * th + t) +
                                                    3932 * bg_sin((uint32_t)(f->lob + 2) * th - 2u * t)) >> 15)) >> 15);
        int32_t x, y;
        if (outer) {
            int32_t c2 = bg_sin(((uint32_t)f->spikes * th >> 1) + (uint32_t)g->tw * 3000u + 0x4000u), p = c2 > 0 ? c2 : 0, n;
            for (n = 1; n < g->sh; n++)
                p = (p * c2) >> 15;
            if (p < 0) p = 0;
            rr += (((r * f->len) >> 8) * p >> 15) * 26 >> 4;
            rr += (((r * f->ripple) >> 8) * bg_sin(th * 18u + 3u * t) >> 15) / 14;
            if (f->fray)
                rr += ((int32_t)((((uint32_t)i * 2654435761u) + t * 40503u) >> 24) - 128) * ((r * f->fray) >> 8) / 1800;
        }
        x = cx + ((rr * bg_sin(th + 0x4000u) >> 15) * g->ax >> 4);
        y = cy + (rr * bg_sin(th) >> 15);
        if (i)
            og_line(px, py, x, y, c);
        px = x;
        py = y;
    }
}
/* a small ring (a nucleus), Q4 */
static void bg_ring(int32_t cx, int32_t cy, int32_t r, uint16_t c)
{
    int32_t i, px = cx + r, py = cy;
    for (i = 1; i <= 12; i++) {
        uint32_t th = (uint32_t)i * 65536u / 12u;
        int32_t x = cx + (r * bg_sin(th + 0x4000u) >> 15), y = cy + (r * bg_sin(th) >> 15);
        og_line(px, py, x, y, c);
        px = x;
        py = y;
    }
}
/* the being: its isotherms (outer cold .. core hot, all warmer with en 0..256), then its nuclei. cx cy r in Q4 */
static void being_lines(const genome_t *g, const bfeat_t *f, int32_t cx, int32_t cy, int32_t r, int32_t en, uint32_t t)
{
    static const int16_t SC[4] = {256, 180, 112, 52}, HT[4] = {28, 90, 150, 206};
    int32_t i, n;
    r += r * en * 30 / 65536;                            /* (it swells with the sound) */
    for (i = 0; i < 4; i++)
        bg_outline(g, f, cx, cy, r * SC[i] >> 8, !i, t, bg_col(HT[i] + en * 40 / 256));
    for (n = 0; n < f->nuc; n++) {                       /* the nuclei: orbiting, DTN apart */
        uint32_t a = t + (uint32_t)n * 65536u / (uint32_t)f->nuc;
        int32_t d = f->nuc > 1 ? r * (24 + f->sep * 40 / 256) / 100 : 0;
        int32_t x = cx + ((d * bg_sin(a + 0x4000u) >> 15) * g->ax >> 4), y = cy + (d * bg_sin(a) >> 15) * 7 / 10;
        bg_ring(x, y, r * 15 / 100, bg_col(214 + en * 42 / 256));
        bg_ring(x, y, r * 6 / 100, bg_col(240));
    }
}
/* the page's knob c as 0..256 (none: -1) and its label */
static int32_t bg_knob(uint32_t c, const char **lab)
{
    int16_t *vp;
    const param_desc_t *d = page_desc(cur_page(), c, &vp);
    int32_t r = d && d->label && d->label[0] != '-' ? RATIO(d, enum_rank(d, *vp)) : -1;
    *lab = d && d->label ? d->label : "";
    return r < 0 ? -1 : r * 256 / 1000;
}
/* a being's features from four of its track's parameters (ids), each by what it is (its label), else by its place */
static void bg_feat(const track_t *t, const genome_t *g, const uint8_t *ids, bfeat_t *f)
{
    int32_t k[4];
    const char *lab[4];
    uint32_t c;
    for (c = 0; c < 4u; c++) {
        const param_desc_t *d = ids[c] < P_COUNT ? track_desc(t, ids[c]) : 0;
        int32_t r = d && d->label && d->label[0] != '-' ? RATIO(d, enum_rank(d, t->p[ids[c]])) : -1;
        lab[c] = d && d->label ? d->label : "";
        k[c] = r < 0 ? 128 : r * 256 / 1000;
    }
    f->spikes = g->k0 + k[0] * g->kk / 256;
    f->len = 48 + k[1] * 208 / 256;
    f->wob = 40 + k[2] * 216 / 256;
    f->lob = g->lob;
    f->nuc = 1 + k[3] * 3 / 257;
    f->sep = f->fray = f->ripple = 0;
    for (c = 0; c < 4u; c++) {                           /* a knob by what it is */
        const char *l = lab[c];
        if (str_eq(l, "CUT")) { f->spikes = 10 + k[c] * 22 / 256; f->len = k[c] * k[c] / 256; }
        else if (str_eq(l, "RES") || str_eq(l, "RESO")) f->ripple = k[c];
        else if (str_eq(l, "DETUNE") || str_eq(l, "SPRD") || str_eq(l, "DTUNE")) { f->sep = k[c]; if (f->nuc < 2) f->nuc = 2; }
        else if (str_eq(l, "NOISE") || str_eq(l, "RAND") || str_eq(l, "BREATH") || str_eq(l, "CRUSH")) f->fray = k[c];
        else if (str_eq(l, "WAVE")) f->lob = 2 + k[c] * 5 / 257;
    }
}
/* (JIANT 0.5) the sound exaggerated on a being: at energy en (0..256) it bulges (its lobes), its spikes shoot out, its
 * membrane shivers and frays; a hit is seen at once */
static void bg_excite(bfeat_t *f, int32_t en, int32_t k)
{
    f->wob = clamp(f->wob + en * k / 4, 0, 256);
    f->len = clamp(f->len + en * k / 3, 0, 420);
    f->ripple = clamp(f->ripple + en * k / 3, 0, 256);
    f->fray = clamp(f->fray + en * k / 6, 0, 256);
}
/* the page's being: its engine's genome, its four knobs by what they are */
static void graph_being_at(const track_t *t, int32_t cx, int32_t cy, int32_t r);
static void graph_being(const track_t *t) { graph_being_at(t, 120 * 16, 61 * 16, 36 * 16); }
/* .. at (cx, cy), radius r (Q4: the EDIT lens's SOUND draws it on the left) */
static void graph_being_at(const track_t *t, int32_t cx, int32_t cy, int32_t r)
{
    static uint32_t tm, f0;                              /* its time runs with the sound: in silence it rests */
    const genome_t *g = &GENOME[EGEN[trk_eng(t)]];
    const page_t *pg = cur_page();
    int32_t en = bg_energy();
    bfeat_t f;
    bg_feat(t, g, pg->id, &f);
    f.lob += (int32_t)(ui.page & 1u);                    /* (the engine's second page: a lobe more, the same species) */
    bg_excite(&f, en, 2);
    if (en > 2) tm += (ui.frame - f0) * (uint32_t)(160 + en * 3);
    f0 = ui.frame;
    being_lines(g, &f, cx, cy, r * (256 + en / 3) / 256, en, tm);
}

/* (JIANT 0.5) HOME, the system as an ecosystem: ui_lzhome.c (graph_eco, eco_step) */

