#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 Juan Ignacio Antonucci (Juan Jiant)
"""(JIANT 0.9.2) The navigation map: every section (its button), its groups (the tabs) and its pages in chain order,
with the six rules, the held layers and the combos, read from the firmware's own chain tables (firmware/src/ui_lens.c
CE_* / CG_* / CHAINS / NAV_DEF), so the map cannot drift from the firmware.

  python3 tools/gen_nav_map.py            docs/navigation-map.svg, .es.svg and their 2x PNGs
  python3 tools/gen_nav_map.py --svg      the SVGs only (no Chromium needed)
  python3 tools/gen_nav_map.py --check    exit 1 if the committed SVGs differ from what the firmware's chains give

Every page of every chain the firmware builds is checked onto the map (the DIGITAL engine's chain, CE_DIG, only when
core.h builds FELUCCA_FM4). The SVG carries its own dark card and fonts (Inter Tight from assets/fonts, instanced and
subset into the file; the 5 x 7 pixel face of tools/gen_px_font.py drawn as paths), so it reads the same on GitHub's
light and dark themes. The PNGs (2x) come from the SVG through Playwright's Chromium (as tools/render_cheatsheets.mjs).
Needs Pillow and fontTools.
"""
import base64
import hashlib
import io
import os
import re
import subprocess
import unicodedata
import sys
from pathlib import Path
from xml.sax.saxutils import escape

from PIL import ImageFont

R = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(R / 'tools'))
from gen_px_font import G as PXG  # noqa: E402  (the UI's 5 x 7 pixel face)

LENS = R / 'firmware/src/ui_lens.c'
FONT = R / 'assets/fonts/InterTight[wght].ttf'
VERSION = re.search(r'FELUCCA_VERSION "(v[^"]+)"', (R / 'firmware/src/felucca.c').read_text()).group(1)

# ------------------------------------------------------------------------------------------- the chains ---
def parse():
    s = LENS.read_text()
    sec = re.findall(r'"(\w+)"', re.search(r'SEC_NAME\[SEC_N\] = \{(.*?)\};', s, re.S).group(1))
    lit = lambda v: None if v == '0' else v.strip('"')
    tabs = {}
    for name, body in re.findall(r'static const nv_t (CE_\w+)\[\] = \{(.*?)\};', s, re.S):
        tabs[name] = [dict(t=lit(t), nm=lit(nm), g=int(g), lv=lv)
                      for t, nm, g, lv in re.findall(r'NV\((0|"[^"]*"),\s*(0|"[^"]*"),\s*(\d+),\s*(\w+)\)', body)]
    groups = {}
    for name, body in re.findall(r'static const ng_t (CG_\w+)\[\] = \{(.*?)\};', s, re.S):
        groups[name] = [dict(nm=nm, tab=lit(tab)) for nm, tab in re.findall(r'\{"([^"]+)",\s*(0|"[^"]*")\}', body)]
    chains = re.findall(r'CH\((CE_\w+),\s*(CG_\w+)\)', re.search(r'CHAINS\[NAV_KEYS\] = \{(.*?)\};', s, re.S).group(1))
    navdef = [int(x) for x in re.findall(r'\d+', re.search(r'NAV_DEF\[NAV_KEYS\] = \{(.*?)\};', s, re.S).group(1))]
    fm4 = re.search(r'#define FELUCCA_FM4 (\d)', (R / 'firmware/src/core.h').read_text()).group(1) != '0'
    keys = []                                          # key k: the section's chain (EDIT's synths), then FM6 DRUM DIGITAL
    for k, (ce, cg) in enumerate(chains):
        if not fm4 and ce == 'CE_DIG':
            continue
        pages = [dict(e, name=e['nm'] or e['t'], i=i) for i, e in enumerate(tabs[ce])]
        keys.append(dict(key=k, ce=ce, sec=sec[k] if k < len(sec) else 'EDIT', groups=groups[cg], pages=pages,
                         default=navdef[k]))
    assert len(navdef) == len(chains), 'NAV_DEF and CHAINS disagree'
    return sec, keys


# ------------------------------------------------------------------------------------------- the words ---
K = ['#8A6CFF', '#FFD23C', '#5874FF', '#FF4B4B']       # COLORS skin, JIANT palette: KNOB 1..4 (gen_ui_palettes KNOBS)
ORANGE, VIOLET = '#FF7A00', '#B870FF'
TXT, MID, DIM, FAINT = '#F4F1EA', '#A39D99', '#6A6470', '#2B2833'
CARD, RAISE, BG = '#0A090E', '#131218', '#000000'

L = {
 'en': dict(
  title='Navigation map', sub=f'firmware {VERSION}  ·  six rules, one chain of pages per section',
  lead='Ten buttons, ten sections. SELECT walks a section\'s pages as one chain; its button jumps group to group.',
  how=[('tab', 'a tab = a group (the button jumps to the next one, round)'), ('chain', 'the line = SELECT\'s order, stops at both ends'),
       ('on', 'orange = the page at power-on'), ('dash', 'dashed = only on the engines (or LOAD) that have it')],
  hold='hold', sel='+ SELECT', nolayer='no layer',
  synth='SYNTHS', synth_sub='ANALOG PHASE LOFI WHEEL VOICE ANIMAL …',
  rules_t='The six rules', layers_t='Held layers (hold the button)', combos_t='Combos',
  rules=[('SELECT', 'SELECT walks the pages', 'One chain per section, a page a detent; it stops at the first and the last. No levels, no zoom.'),
         ('BUTTON', 'The button: in, or on', 'From elsewhere, the page you left there. Inside, the first page of the next group, round.'),
         ('PRESETS', 'PRESETS loads presets', 'The track\'s sounds everywhere, patterns in SEQ, the genre in GEN. Never a page, a track or a module.'),
         ('KNOB 1', 'KNOB 1 picks', 'The step, the bar, the slot, the stage, the operator, the kit\'s sound. The track is always ALGORITHM\'s.'),
         ('HOLD', 'Hold = the layer', 'A button held puts its shortcuts on the keys and knobs; ALGORITHM while held: MAP · KNOBS · STATE.'),
         ('SCREEN', 'The screen says where', 'Tabs on top: the groups, a dot a page. At the foot: SECTION · GROUP · page and n/N.')],
  hints={'HOME': ('MENU', 'GEN'), 'EDIT': ('INIT · RECALL', 'sound dice'), 'SEQ': ('SEQ TOOLS', 'sequence dice'),
         'FX': ('punch-in FX', 'FX dice'), 'GLO': ('mutes · solos · UNDO', 'tempo'), 'ENV': (None, 'walk this chain'),
         'LFO': ('MACROS M1–M4', 'LFO dice'), 'SCL': ('ROOT on the keys', 'walk this chain'), 'ARP': (None, 'walk this chain'),
         'SAVE': ('sections live · NAME', 'walk this chain')},
  notes={'HOME': 'GEN 1–3: PRESETS the genre, OCT+ makes the piece, OCT− back to ECO.',
         'SEQ': 'PRESETS: the patterns (PHRASES). KNOB 1: all 64 steps on STEP (a fast turn several).',
         'EDIT': 'TONE pages only where the engine has them: MORPH (ANALOG PHASE LOFI), RETRIG (LOFI), ROTOR (WHEEL), WORDS (VOICE). DRUM-X: KNOB 1 picks the kit\'s sound on SOUND.',
         'FX': 'CHORUS and SLICER are hidden with LOAD ECO; the chain skips them.',
         'SAVE': 'From outside, USER and SECTIONS come with SAVE armed: SAVE · OCT+ · OCT+.',
         'ARP': 'One group: ARP inside goes back to ARP.', 'SCL': 'KNOB 1 sets ROOT.',
         'ENV': 'ALGORITHM picks the track on TRACKS.', 'GLO': 'LIVE: MANO plays the modifiers on the keys. MIXER KNOB 4: the DJ FILTER.',
         'LFO': 'KNOB 1: the slot on MOD, the step on MSEQ.'},
  layers=[('HOME', 'the MENU: SCREEN · AUDIO · MIDI · USB · SYSTEM (SELECT the pages, ALGORITHM the tabs, KNOB 1–4 the settings)'),
          ('EDIT', 'F3 INIT · G3 RECALL · DRUM: black keys 1–8 mute each sound'),
          ('SEQ', 'F3 CLEAR · G3 REVERSE · A3/B3 SHIFT · C4 RANDOM · D4 COOK · E4 BEAT · F4 CLR AUTO · D5 REVERT · **G5 CLR STEP** · REC: CLEAR ALL'),
          ('FX', 'punch-in: F3–A3 REPEAT · B3 GLITCH · C4 TAPE STOP · D4/E4 OCT · F4 ½ TEMPO · G4/A4 DEC · B4/C5 STUTTER · D5–F5 ATK+ ARP RANDOM · G#4 FREEZE · C#5 SHUFFLE · D#5 F#5 sweeps · knobs: macros (a glitcher held: its own)'),
          ('GLO', 'black 1–6 MUTE · F3–D4 SOLO · E4 unmute all · F4 TAP · A4 FREEZE · B4 REVERT ALL · **G4 UNDO** · PLAY restart · knobs: LEVEL'),
          ('LFO', 'KNOB 1–4 = M1–M4, latched · F3 new routes · G3 clear'),
          ('SCL', 'any key sets the ROOT'),
          ('SAVE', 'F3–B3 play A–D · C4–F4 store A–D · **C5 NAME** · D5 LOOP/SONG · E5 SONG REC · G5 ARRANGE'),
          ('REC', 'F3 CLEAR · G3 CLICK')],
  combos=[(['HOME', 'SELECT'], 'GEN, the song generator'),
          (['EDIT·SEQ·LFO·FX', 'SELECT'], 'their dice: BEFORE / NOW, keep or cancel'),
          (['GLO', 'SELECT'], 'the tempo, from any page'),
          (['ENV·SCL·ARP·SAVE', 'SELECT'], 'walk that chain from anywhere'),
          (['ALGORITHM'], 'the track, always'),
          (['SEQ', 'G5'], 'CLR STEP: the step and its locks'),
          (['STEP HELD', 'SELECT'], 'the EDIT page its knobs lock (SEQ › STEP)'),
          (['SAVE', 'C5'], 'NAME: rename the slot, section, project'),
          (['GLO', 'G4'], 'UNDO (again: redo)'),
          (['OCT−', 'OCT+'], 'back / do it (NO / YES)')]),
 'es': dict(
  title='Mapa de navegación', sub=f'firmware {VERSION}  ·  seis reglas, una cadena de páginas por sección',
  lead='Diez botones, diez secciones. SELECT recorre las páginas de una sección como una cadena; su botón salta de grupo en grupo.',
  how=[('tab', 'pestaña = un grupo (el botón salta al siguiente, en ronda)'), ('chain', 'la línea = el orden de SELECT, frena en las puntas'),
       ('on', 'naranja = la página al encender'), ('dash', 'punteado = solo en los motores (o el LOAD) que la tienen')],
  hold='mantener', sel='+ SELECT', nolayer='sin capa',
  synth='SINTES', synth_sub='ANALOG PHASE LOFI WHEEL VOICE ANIMAL …',
  rules_t='Las seis reglas', layers_t='Capas (mantené el botón)', combos_t='Combinaciones',
  rules=[('SELECT', 'SELECT recorre las páginas', 'Una cadena por sección, una página por paso; frena en la primera y en la última. Sin niveles, sin zoom.'),
         ('BOTÓN', 'El botón: entrar o saltar', 'Desde afuera, la página donde la dejaste. Adentro, la primera página del grupo siguiente, en ronda.'),
         ('PRESETS', 'PRESETS carga presets', 'Los sonidos de la pista en todos lados, los patrones en SEQ, el género en GEN. Nunca una página, pista ni módulo.'),
         ('KNOB 1', 'KNOB 1 elige', 'El paso, el compás, el slot, la etapa, el operador, el sonido del kit. La pista, siempre ALGORITHM.'),
         ('MANTENER', 'Mantener = la capa', 'Un botón mantenido pone sus atajos en teclas y perillas; ALGORITHM mientras lo mantenés: MAP · KNOBS · STATE.'),
         ('PANTALLA', 'La pantalla dice dónde', 'Arriba las pestañas: los grupos, un punto por página. Abajo: SECCIÓN · GRUPO · página y n/N.')],
  hints={'HOME': ('MENU', 'GEN'), 'EDIT': ('INIT · RECALL', 'dados de sonido'), 'SEQ': ('SEQ TOOLS', 'dados de secuencia'),
         'FX': ('FX punch-in', 'dados de FX'), 'GLO': ('mutes · solos · UNDO', 'tempo'), 'ENV': (None, 'recorre esta cadena'),
         'LFO': ('MACROS M1–M4', 'dados de LFO'), 'SCL': ('ROOT en las teclas', 'recorre esta cadena'),
         'ARP': (None, 'recorre esta cadena'), 'SAVE': ('secciones en vivo · NAME', 'recorre esta cadena')},
  notes={'HOME': 'GEN 1–3: PRESETS el género, OCT+ arma la pieza, OCT− vuelve a ECO.',
         'SEQ': 'PRESETS: los patrones (PHRASES). KNOB 1: los 64 pasos en STEP (girado rápido, varios).',
         'EDIT': 'Las páginas de TONE solo donde el motor las tiene: MORPH (ANALOG PHASE LOFI), RETRIG (LOFI), ROTOR (WHEEL), WORDS (VOICE). DRUM-X: KNOB 1 elige el sonido del kit en SOUND.',
         'FX': 'CHORUS y SLICER se ocultan con LOAD ECO; la cadena los saltea.',
         'SAVE': 'Desde afuera, USER y SECTIONS llegan con SAVE elegido: SAVE · OCT+ · OCT+.',
         'ARP': 'Un grupo: ARP adentro vuelve a ARP.', 'SCL': 'KNOB 1 fija ROOT.',
         'ENV': 'ALGORITHM elige la pista en TRACKS.', 'GLO': 'LIVE: en MANO las teclas tocan los modificadores. MIXER KNOB 4: el DJ FILTER.',
         'LFO': 'KNOB 1: el slot en MOD, el paso en MSEQ.'},
  layers=[('HOME', 'el MENU: SCREEN · AUDIO · MIDI · USB · SYSTEM (SELECT las páginas, ALGORITHM las pestañas, KNOB 1–4 los ajustes)'),
          ('EDIT', 'F3 INIT · G3 RECALL · DRUM: negras 1–8 silencian cada sonido'),
          ('SEQ', 'F3 CLEAR · G3 REVERSE · A3/B3 SHIFT · C4 RANDOM · D4 COOK · E4 BEAT · F4 CLR AUTO · D5 REVERT · **G5 CLR STEP** · REC: CLEAR ALL'),
          ('FX', 'punch-in: F3–A3 REPEAT · B3 GLITCH · C4 TAPE STOP · D4/E4 OCT · F4 ½ TEMPO · G4/A4 DEC · B4/C5 STUTTER · D5–F5 ATK+ ARP RANDOM · G#4 FREEZE · C#5 SHUFFLE · D#5 F#5 barridos · perillas: macros (con un glitcher, las suyas)'),
          ('GLO', 'negras 1–6 MUTE · F3–D4 SOLO · E4 desmutea todo · F4 TAP · A4 FREEZE · B4 REVERT ALL · **G4 UNDO** · PLAY reinicia · perillas: LEVEL'),
          ('LFO', 'KNOB 1–4 = M1–M4, enganchadas · F3 rutas nuevas · G3 las borra'),
          ('SCL', 'cualquier tecla fija la ROOT'),
          ('SAVE', 'F3–B3 tocar A–D · C4–F4 guardar A–D · **C5 NAME** · D5 LOOP/SONG · E5 SONG REC · G5 ARRANGE'),
          ('REC', 'F3 CLEAR · G3 CLICK')],
  combos=[(['HOME', 'SELECT'], 'GEN, el generador de temas'),
          (['EDIT·SEQ·LFO·FX', 'SELECT'], 'sus dados: BEFORE / NOW, quedarse o cancelar'),
          (['GLO', 'SELECT'], 'el tempo, desde cualquier página'),
          (['ENV·SCL·ARP·SAVE', 'SELECT'], 'recorre esa cadena desde cualquier lado'),
          (['ALGORITHM'], 'la pista, siempre'),
          (['SEQ', 'G5'], 'CLR STEP: el paso y sus locks'),
          (['PASO', 'SELECT'], 'la página EDIT que bloquean sus perillas (SEQ › STEP)'),
          (['SAVE', 'C5'], 'NAME: renombra slot, sección, proyecto'),
          (['GLO', 'G4'], 'UNDO (otra vez: redo)'),
          (['OCT−', 'OCT+'], 'atrás / hacerlo (NO / YES)')]),
}

# pages shown only where they apply (the manual's "Some pages only appear when they apply"); drawn dashed
COND = {('EDIT', 'MORPH'), ('EDIT', 'RETRIG'), ('EDIT', 'ROTOR'), ('EDIT', 'WORDS'), ('FX', 'CHORUS'), ('FX', 'SLICER'),
        ('FX', 'SL BUS'), ('FX', 'SL BUS 2'), ('FX', 'SL BUS 3'), ('FX', 'SL PITCH')}

# the map's rows: the sections in each (EDIT: its three chains side by side) and their accent (KNOB 1..4 colours)
ROWS = [['HOME', 'SEQ', 'SCL'], ['EDIT'], ['FX', 'GLO'], ['ENV', 'LFO', 'ARP', 'SAVE']]
ACCENT = {'HOME': K[0], 'SEQ': K[2], 'SCL': K[3], 'EDIT': K[1], 'FX': K[3], 'GLO': K[0], 'ENV': K[1], 'LFO': K[2],
          'ARP': K[0], 'SAVE': K[1]}

# ------------------------------------------------------------------------------------------- drawing ---
_pil = {}
def pil(size, w):
    k = (size, w)
    if k not in _pil:
        f = ImageFont.truetype(str(FONT), size)
        f.set_variation_by_axes([w])
        _pil[k] = f
    return _pil[k]
def tw(s, size, w=500): return pil(round(size * 4), w).getlength(s) / 4


def mix(a, b, t):
    a, b = [int(a[i:i + 2], 16) for i in (1, 3, 5)], [int(b[i:i + 2], 16) for i in (1, 3, 5)]
    return '#%02X%02X%02X' % tuple(round(a[i] + (b[i] - a[i]) * t) for i in range(3))


class Svg:
    def __init__(s): s.o, s.chars, s.glyphs = [], set(), set()
    def add(s, x): s.o.append(x)

    def text(s, x, y, t, size, fill=TXT, w=500, anchor='start', op=None):
        s.chars |= set(t)
        a = '' if anchor == 'start' else f' text-anchor="{anchor}"'
        o = f' opacity="{op}"' if op else ''
        s.add(f'<text x="{x:.1f}" y="{y:.1f}" font-size="{size}" font-weight="{w}" fill="{fill}"{a}{o}>{escape(t)}</text>')

    def rich(s, x, y, parts, size, fill=TXT, bfill=None):
        """parts: [(text, bold)] on one line"""
        out = []
        for t, b in parts:
            s.chars |= set(t)
            out.append(f'<tspan font-weight="{700 if b else 500}"{f" fill=" + chr(34) + bfill + chr(34) if b and bfill else ""}>{escape(t)}</tspan>')
        s.add(f'<text x="{x:.1f}" y="{y:.1f}" font-size="{size}" fill="{fill}" xml:space="preserve">{"".join(out)}</text>')

    def px(s, x, y, t, sc, fill=TXT, anchor='start'):
        """the 5 x 7 pixel face, as one path; y = the capitals' top"""
        w = px_w(t, sc)
        if anchor == 'middle': x -= w / 2
        elif anchor == 'end': x -= w
        u = []
        for i, ch in enumerate(t):
            ch = {'−': '-', '–': '-'}.get(ch, ch)
            if ch not in PXG and ch != '·':                 # (no accents in the face: É -> E)
                ch = unicodedata.normalize('NFD', ch)[0]
            if ch == ' ': continue
            s.glyphs.add(ch)
            u.append(f'<use href="#p{ord(ch):X}"' + (f' x="{i * 6}"' if i else '') + '/>')
        if u: s.add(f'<g transform="translate({x:.2f} {y:.2f}) scale({sc:g})" fill="{fill}">{"".join(u)}</g>')

    def rrect(s, x, y, w, h, r, fill='none', stroke=None, sw=1.5, dash=None, op=None):
        st = f' stroke="{stroke}" stroke-width="{sw}"' if stroke else ''
        if dash: st += f' stroke-dasharray="{dash}"'
        if op: st += f' opacity="{op}"'
        s.add(f'<rect x="{x:.1f}" y="{y:.1f}" width="{w:.1f}" height="{h:.1f}" rx="{r}" fill="{fill}"{st}/>')

    def circle(s, x, y, r, fill='none', stroke=None, sw=2):
        st = f' stroke="{stroke}" stroke-width="{sw}"' if stroke else ''
        s.add(f'<circle cx="{x:.1f}" cy="{y:.1f}" r="{r}" fill="{fill}"{st}/>')

    def path(s, d, stroke, sw=3, fill='none', op=None):
        o = f' opacity="{op}"' if op else ''
        s.add(f'<path d="{d}" fill="{fill}" stroke="{stroke}" stroke-width="{sw}" stroke-linecap="round" stroke-linejoin="round"{o}/>')


def px_w(t, sc): return (len(t) * 6 - 1) * sc


def glyph_defs(glyphs):
    """the pixel face's glyphs used, once each, in 1 px cells"""
    out = []
    for ch in sorted(glyphs):
        rows = PXG.get(ch) or (['.....', '.....', '.....', '..#..'] if ch == '·' else [])
        d = []
        for r, row in enumerate(rows):
            for m in re.finditer(r'#+', row):
                d.append(f'M{m.start()} {r}h{len(m.group())}v1h-{len(m.group())}z')
        out.append(f'<path id="p{ord(ch):X}" d="{"".join(d)}"/>')
    return '<defs>' + ''.join(out) + '</defs>'



def rounded(pts, r=12):
    """a polyline with its corners rounded"""
    d = f'M{pts[0][0]:.1f} {pts[0][1]:.1f}'
    for i in range(1, len(pts) - 1):
        (x0, y0), (x1, y1), (x2, y2) = pts[i - 1], pts[i], pts[i + 1]
        l1, l2 = max(abs(x1 - x0), abs(y1 - y0)), max(abs(x2 - x1), abs(y2 - y1))
        rr = min(r, l1 / 2, l2 / 2)
        ax, ay = x1 - (x1 - x0) / (l1 or 1) * rr, y1 - (y1 - y0) / (l1 or 1) * rr
        bx, by = x1 + (x2 - x1) / (l2 or 1) * rr, y1 + (y2 - y1) / (l2 or 1) * rr
        d += f'L{ax:.1f} {ay:.1f}Q{x1:.1f} {y1:.1f} {bx:.1f} {by:.1f}'
    return d + f'L{pts[-1][0]:.1f} {pts[-1][1]:.1f}'


def wrap(text, size, width):
    """'**bold**' marked text -> lines of [(word, bold)]"""
    toks, bold = [], False
    for part in re.split(r'(\*\*)', text):
        if part == '**':
            bold = not bold
            continue
        for wd in part.split(' '):
            if wd: toks.append((wd, bold))
    lines, cur, cw = [], [], 0
    sp = tw(' ', size)
    for wd, b in toks:
        w = tw(wd, size, 700 if b else 500)
        if cur and cw + sp + w > width:
            lines.append(cur); cur, cw = [], 0
        cw += (sp if cur else 0) + w
        cur.append((wd, b))
    if cur: lines.append(cur)
    out = []
    for ln in lines:                                   # merge words of one weight: [(text, bold)]
        m = []
        for i, (wd, b) in enumerate(ln):
            t = (' ' if i else '') + wd
            if m and m[-1][1] == b: m[-1] = (m[-1][0] + t, b)
            else: m.append((t, b))
        out.append(m)
    return out


# ------------------------------------------------------------------------------------------- layout ---
W, M, GAP, PAD = 1500, 30, 18, 16
COLG = 16                                               # between two columns (the chain rises in it)
PILL_H, STEP = 34, 44
PAGE_FS, TAB_SC, KEY_SC = 19, 2.5, 3.5


def signature(sv, ox, oy, sc):
    src = (R / 'firmware/src/ui_signature.h').read_text()
    nums = [int(x) for x in re.findall(r'\b\d+\b', src.split('{', 1)[1])]
    pts = [(nums[i], nums[i + 1]) for i in range(0, len(nums) - 1, 2)]
    heat = ['#00D2FF', '#5A3CFF', '#B432E6', '#F02832', '#FF7A00', '#FFD700']
    n = len(pts)
    for i in range(n - 1):
        if pts[i + 1][1] & 128: continue
        a = (pts[i][0], pts[i][1] & 127); b = (pts[i + 1][0], pts[i + 1][1] & 127)
        t = i / n * (len(heat) - 1); k = min(len(heat) - 2, int(t))
        c = mix(heat[k], heat[k + 1], t - k)
        sv.add(f'<line x1="{ox + a[0] * sc:.1f}" y1="{oy + a[1] * sc:.1f}" x2="{ox + b[0] * sc:.1f}" y2="{oy + b[1] * sc:.1f}" '
               f'stroke="{c}" stroke-width="2.6" stroke-linecap="round"/>')


def block_cols(ch):
    """a chain's columns: [(group, [pages])] in chain order"""
    cols = []
    for p in ch['pages']:
        if not cols or cols[-1][0] != p['g']: cols.append((p['g'], []))
        cols[-1][1].append(p)
    return cols


def draw_block(sv, ch, sec, x, y, colw, accent, drawn, label=None):
    """one chain: tabs (the groups) and the pages under them, the chain line through them; returns its bottom"""
    cols = block_cols(ch)
    n = len(ch['pages'])
    if label:
        sv.px(x, y + 2, label[0], 2.5, accent)
        if label[1]: sv.text(x + px_w(label[0], 2.5) + 12, y + 18, label[1], 14, DIM)
        y += 46
    ty = y
    pos = {}
    for c, (g, pages) in enumerate(cols):               # the tabs: full name, else the device's short one
        gx = x + c * (colw + COLG)
        grp = ch['groups'][g]
        nm = grp['nm'] if px_w(grp['nm'], TAB_SC) <= colw - 20 else (grp['tab'] or grp['nm'])
        sv.rrect(gx, ty, colw, 30, 15, fill=RAISE, stroke=mix(accent, BG, .45), sw=1.5)
        sv.px(gx + colw / 2, ty + 15 - 3.5 * TAB_SC, nm, TAB_SC, accent, 'middle')
        dots = len(pages)
        for k in range(dots):                           # a dot a page, as the screen's tabs
            sv.circle(gx + colw / 2 + (k - (dots - 1) / 2) * 10, ty + 40, 2.6, fill=mix(accent, BG, .35))
        if c < len(cols) - 1:                           # the button's jump to the next group
            sv.px(gx + colw + COLG / 2, ty + 15 - 3.5 * 2, '>', 2, DIM, 'middle')
    if len(cols) > 1:                                   # .. and round
        lx, rx = x + colw / 2, x + (len(cols) - 1) * (colw + COLG) + colw / 2
        sv.path(f'M{rx:.1f} {ty - 2:.1f}C{rx:.1f} {ty - 16:.1f} {rx - 18:.1f} {ty - 14:.1f} {(lx + rx) / 2:.1f} {ty - 14:.1f}'
                f'S{lx:.1f} {ty - 16:.1f} {lx:.1f} {ty - 4:.1f}', mix(DIM, BG, .2), 1.6, op=None)
        sv.add(f'<path d="M{lx - 4.5:.1f} {ty - 10:.1f}L{lx:.1f} {ty - 3:.1f}L{lx + 4.5:.1f} {ty - 10:.1f}" fill="none" '
               f'stroke="{mix(DIM, BG, .2)}" stroke-width="1.6" stroke-linecap="round" stroke-linejoin="round"/>')
    py0 = ty + 62
    for c, (g, pages) in enumerate(cols):
        gx = x + c * (colw + COLG)
        for r, p in enumerate(pages):
            pos[p['i']] = (gx, py0 + r * STEP + PILL_H / 2)
    col = lambda i: mix(accent, ORANGE, i / max(1, n - 1) * .85)
    seq = [p['i'] for p in ch['pages']]
    for a, b in zip(seq, seq[1:]):                      # the chain: SELECT's order
        (x0, y0), (x1, y1) = pos[a], pos[b]
        if x0 == x1:
            sv.path(f'M{x0:.1f} {y0:.1f}L{x1:.1f} {y1:.1f}', col(a), 3)
        else:                                           # down, along the column's foot, up its right gap, in from the top
            gxm = x1 - COLG / 2
            sv.path(rounded([(x0, y0), (x0, y0 + PILL_H / 2 + 8), (gxm, y0 + PILL_H / 2 + 8), (gxm, y1 - PILL_H / 2 - 9),
                             (x1, y1 - PILL_H / 2 - 9), (x1, y1)], 10), col(a), 3)
    first, last = pos[seq[0]], pos[seq[-1]]             # the ends: SELECT stops there (no wrap)
    sv.circle(first[0], first[1], 13.5, stroke=col(seq[0]), sw=1.6)
    sv.add(f'<line x1="{last[0] - 9:.1f}" y1="{last[1] + PILL_H / 2 + 7:.1f}" x2="{last[0] + 9:.1f}" '
           f'y2="{last[1] + PILL_H / 2 + 7:.1f}" stroke="{col(seq[-1])}" stroke-width="3" stroke-linecap="round"/>')
    sv.path(f'M{last[0]:.1f} {last[1]:.1f}L{last[0]:.1f} {last[1] + PILL_H / 2 + 7:.1f}', col(seq[-1]), 3)
    bottom = py0
    for p in ch['pages']:
        gx, cy = pos[p['i']]
        on = p['i'] == ch['default']
        cond = (sec, p['name']) in COND
        sv.rrect(gx, cy - PILL_H / 2, colw, PILL_H, 10, fill=RAISE, stroke=ORANGE if on else mix(accent, BG, .62),
                 sw=2 if on else 1.4, dash='5 4' if cond else None)
        nx = gx + 16
        room = gx + colw - 8 - nx
        fs = PAGE_FS
        while tw(p['name'], fs, 600) > room and fs > 13: fs -= .5
        if tw(p['name'], fs, 600) > room:
            raise SystemExit(f'gen_nav_map: "{p["name"]}" does not fit its pill ({room:.0f} px)')
        sv.text(nx, cy + fs * .36, p['name'], fs, TXT, 600)
        c = ORANGE if on else col(p['i'])
        sv.circle(gx, cy, 9.5, fill=c, stroke=RAISE, sw=2.5)
        idx = str(seq.index(p['i']) + 1)
        isc = 1.3 if len(idx) < 2 else 1.05
        sv.px(gx + .5, cy - 3.5 * isc, idx, isc, '#000000', 'middle')
        drawn.add((ch['key'], p['i']))
        bottom = max(bottom, cy + PILL_H / 2)
    return bottom + 12


def key_cap(sv, x, y, name, accent):
    w = px_w(name, KEY_SC) + 30
    sv.rrect(x, y, w, 40, 12, fill='#1C1A22', stroke=accent, sw=2)
    sv.rrect(x + 4, y + 4, w - 8, 32, 9, fill='none', stroke=mix(accent, BG, .7), sw=1)
    sv.px(x + w / 2, y + 20 - 3.5 * KEY_SC, name, KEY_SC, TXT, 'middle')
    return w


def card_head(sv, x, y, w, sec, lang, accent):
    T = L[lang]
    kw = key_cap(sv, x, y, sec, accent)
    hold, sel = T['hints'][sec]
    a = (T['hold'] + ': ' + hold) if hold else T['nolayer']
    b = f"{T['sel']}: {sel}"
    hx, fs = x + kw + 12, 15
    if max(tw(a, fs), tw(b, fs)) <= x + w - hx:          # beside the button, else under it (a narrow card: ARP)
        sv.text(hx, y + 16, a, fs, MID if hold else DIM, 500)
        sv.text(hx, y + 35, b, fs, MID, 500)
        return y + 62
    for i, t in enumerate(wrap(a, fs, w)[:1] + wrap(b, fs, w)):
        sv.rich(x, y + 62 + i * 19, t, fs, MID if (hold or i) else DIM)
    return y + 62 + (1 + len(wrap(b, fs, w))) * 19 + 4


def note(sv, x, y, w, text, accent):
    lines = wrap(text, 15, w)
    for i, ln in enumerate(lines):
        sv.rich(x, y + 15 + i * 20, ln, 15, DIM)
    return y + len(lines) * 20 + 6


def build(lang, keys):
    T = L[lang]
    sv = Svg()
    bykey = {}
    for ch in keys: bykey.setdefault(ch['sec'], []).append(ch)
    drawn = set()
    y = M + 8
    # ---- the title
    signature(sv, M + 8, y - 4, 0.62)
    sv.text(M + 170, y + 46, 'JIANT FM', 52, TXT, 700)
    sv.text(M + 170 + tw('JIANT FM', 52, 700) + 22, y + 46, T['title'], 52, VIOLET, 500)
    sv.text(M + 172, y + 82, T['sub'], 19, MID, 500)
    # how to read it, at the right
    hx, hy = W - M - 500, y + 4
    for i, ln in enumerate(wrap(T['lead'], 17, hx - 24 - (M + 172))):
        sv.rich(M + 172, y + 108 + i * 22, ln, 17, DIM)
    for i, (kind, t) in enumerate(T['how']):
        yy = hy + i * 27
        if kind == 'tab':
            sv.rrect(hx, yy, 40, 20, 10, fill=RAISE, stroke=mix(K[0], BG, .45)); sv.px(hx + 20, yy + 5, 'TAB', 1.4, K[0], 'middle')
        elif kind == 'chain':
            sv.path(f'M{hx + 2} {yy + 10}L{hx + 38} {yy + 10}', mix(K[0], ORANGE, .5), 3); sv.circle(hx + 6, yy + 10, 4.5, fill=K[0])
            sv.circle(hx + 34, yy + 10, 4.5, fill=mix(K[0], ORANGE, .8))
        elif kind == 'on':
            sv.rrect(hx, yy, 40, 20, 7, fill=RAISE, stroke=ORANGE, sw=2)
        else:
            sv.rrect(hx, yy, 40, 20, 7, fill=RAISE, stroke=MID, sw=1.4, dash='5 4')
        sv.text(hx + 52, yy + 15, t, 15, MID)
    y += 150
    hue = [K[0], K[1], K[2], K[3], ORANGE, '#FFD700', '#F02832']
    for i in range(60):                                 # the heat rule (gen_readme_hero.py's)
        t = i / 59 * 4; k = min(3, int(t))
        sv.rrect(M + i * (W - 2 * M) / 60, y, (W - 2 * M) / 60 + .5, 3, 0, fill=mix(['#5A3CFF', '#B432E6', '#F02832', '#FF7A00', '#FFD700'][k], ['#5A3CFF', '#B432E6', '#F02832', '#FF7A00', '#FFD700'][k + 1], t - k))
    y += 26
    # ---- the sections, row by row
    for row in ROWS:
        blocks = []                                     # (sec, chain, label)
        for sec in row:
            chs = bykey[sec]
            for ch in chs:
                lab = None
                if sec == 'EDIT':
                    lab = {'CE_SYN': (T['synth'], T['synth_sub']), 'CE_FM6': ('FM6', None), 'CE_DRM': ('DRUM-X', None),
                           'CE_DIG': ('DIGITAL', None)}[ch['ce']]
                blocks.append((sec, ch, lab))
        ncols = sum(len(block_cols(ch)) for _, ch, _ in blocks)
        nb = len(blocks)
        cards = len(row)
        inner = W - 2 * M - (cards - 1) * GAP - cards * 2 * PAD - (ncols - nb) * COLG - (nb - cards) * 34
        colw = inner / ncols
        x = M
        tops, bots = [], []
        # card extents first (one row of cards shares a height)
        layout = []
        for sec in row:
            mine = [b for b in blocks if b[0] == sec]
            nc = sum(len(block_cols(ch)) for _, ch, _ in mine)
            cw = nc * colw + (nc - len(mine)) * COLG + (len(mine) - 1) * 34 + 2 * PAD
            layout.append((sec, mine, x, cw))
            x += cw + GAP
        sub = Svg()
        bottom = y
        for sec, mine, cx, cw in layout:
            acc = ACCENT[sec]
            hy = card_head(sub, cx + PAD, y + PAD, cw - 2 * PAD, sec, lang, acc)
            bx = cx + PAD
            b = hy
            for i, (_, ch, lab) in enumerate(mine):
                if i:
                    sub.add(f'<line x1="{bx - 17:.1f}" y1="{hy:.1f}" x2="{bx - 17:.1f}" y2="{hy + 300:.1f}" stroke="{FAINT}" stroke-width="1" stroke-dasharray="2 5"/>')
                bb = draw_block(sub, ch, sec, bx, hy + 18, colw, acc, drawn, lab)
                b = max(b, bb)
                bx += len(block_cols(ch)) * (colw + COLG) - COLG + 34
            b = note(sub, cx + PAD, b, cw - 2 * PAD, T['notes'][sec], acc)
            bottom = max(bottom, b + PAD - 6)
        for sec, mine, cx, cw in layout:
            sv.rrect(cx, y, cw, bottom - y, 18, fill=CARD, stroke='#1E1C25', sw=1.2)
        sv.o += sub.o; sv.chars |= sub.chars; sv.glyphs |= sub.glyphs
        y = bottom + GAP
    # ---- the six rules
    sv.px(M, y + 6, T['rules_t'].upper() if lang == 'en' else 'LAS SEIS REGLAS', 3, TXT)
    y += 40
    rc = [K[0], K[1], K[2], K[3], ORANGE, '#F4F1EA']
    tw3 = (W - 2 * M - 2 * GAP) / 3
    rh = 128
    for i, (ctl, head, body) in enumerate(T['rules']):
        rx, ry = M + (i % 3) * (tw3 + GAP), y + (i // 3) * (rh + 14)
        sv.rrect(rx, ry, tw3, rh, 16, fill=CARD, stroke='#1E1C25', sw=1.2)
        sv.rrect(rx, ry + 18, 5, rh - 36, 2.5, fill=rc[i])
        sv.text(rx + 26, ry + 52, str(i + 1), 46, rc[i], 700)
        hx = rx + 70
        sv.px(hx, ry + 18, ctl, 2, rc[i])
        sv.text(hx, ry + 54, head, 21, TXT, 700)
        for j, ln in enumerate(wrap(body, 15.5, tw3 - 96)):
            sv.rich(hx, ry + 80 + j * 21, ln, 15.5, MID)
    y += 2 * rh + 14 + GAP + 8
    # ---- held layers and combos
    lw = (W - 2 * M - GAP) * .56
    cw2 = W - 2 * M - GAP - lw
    top = y
    sv.px(M, y + 6, T['layers_t'].upper(), 3, TXT)
    sv.px(M + lw + GAP, y + 6, T['combos_t'].upper(), 3, TXT)
    y += 40
    ly = y + 18
    sub = Svg()
    for btn, txt in T['layers']:
        acc = ACCENT.get(btn, ORANGE)
        w = px_w(btn, 2.2) + 20
        sub.rrect(M + 18, ly, 92, 26, 9, fill='#1C1A22', stroke=acc, sw=1.6)
        sub.px(M + 18 + 46, ly + 13 - 3.5 * 2.2, btn, 2.2, TXT, 'middle')
        lines = wrap(txt, 15, lw - 150)
        for j, ln in enumerate(lines):
            sub.rich(M + 126, ly + 18 + j * 21, ln, 15, MID, bfill=TXT)
        ly += max(34, len(lines) * 21 + 13)
    cy = y + 18
    cx0 = M + lw + GAP + 18
    kws = lambda ks: sum(px_w(k, 2) + 24 for k in ks) + 18 * (len(ks) - 1)
    rx0 = cx0 + max(kws(k) for k, _ in T['combos']) + 8
    for keys_, res in T['combos']:
        kx = cx0
        for j, k in enumerate(keys_):
            if j:
                sub.text(kx + 2, cy + 18, '+', 17, DIM, 700); kx += 18
            w = px_w(k, 2) + 18
            hot = k in ('SELECT', 'G5', 'C5', 'G4')
            sub.rrect(kx, cy, w, 26, 9, fill='#1C1A22', stroke=ORANGE if hot else '#3A3644', sw=1.5)
            sub.px(kx + w / 2, cy + 6, k, 2, TXT, 'middle')
            kx += w + 6
        lines = wrap(res, 15, M + lw + GAP + cw2 - 16 - rx0)
        for j, ln in enumerate(lines):
            sub.rich(rx0, cy + 18 + j * 21, ln, 15, MID)
        cy += max(34, len(lines) * 21 + 13)
    bottom = max(ly, cy) + 8
    sv.rrect(M, y, lw, bottom - y, 16, fill=CARD, stroke='#1E1C25', sw=1.2)
    sv.rrect(M + lw + GAP, y, cw2, bottom - y, 16, fill=CARD, stroke='#1E1C25', sw=1.2)
    sv.o += sub.o; sv.chars |= sub.chars; sv.glyphs |= sub.glyphs
    y = bottom + 26
    sv.text(M, y + 4, 'JIANT FM  ·  GPL-3.0  ·  ' + ('generated by' if lang == 'en' else 'generado por') +
            ' tools/gen_nav_map.py ' + ('from' if lang == 'en' else 'desde') + ' firmware/src/ui_lens.c', 14, DIM)
    sv.text(W - M, y + 4, 'docs/MANUAL.md § 3' if lang == 'en' else 'docs/MANUAL.es.md § 3', 14, DIM, anchor='end')
    H = y + M
    return sv, H, drawn


def fonts_css(chars):
    """Inter Tight at 500 600 700, instanced and subset to the map's letters, as WOFF data"""
    from fontTools.ttLib import TTFont
    from fontTools.varLib import instancer
    from fontTools import subset
    css = []
    text = ''.join(sorted(chars | set('0123456789')))
    for w in (500, 600, 700):
        f = TTFont(str(FONT))
        f = instancer.instantiateVariableFont(f, {'wght': w})
        opt = subset.Options(); opt.flavor = 'woff'; opt.layout_features = ['kern', 'liga']; opt.name_IDs = []
        opt.notdef_outline = True
        sub = subset.Subsetter(opt); sub.populate(text=text); sub.subset(f)
        b = io.BytesIO(); f.flavor = 'woff'; f.recalcTimestamp = False; f.save(b)
        css.append(f"@font-face{{font-family:'JIANT Inter Tight';font-weight:{w};"
                   f"src:url(data:font/woff;base64,{base64.b64encode(b.getvalue()).decode()}) format('woff')}}")
    return ''.join(css)


def svg_doc(lang, keys):
    sv, H, drawn = build(lang, keys)
    sig = hashlib.sha1(repr([(c['ce'], [(p['name'], p['g']) for p in c['pages']], [g['nm'] for g in c['groups']], c['default'])
                             for c in keys]).encode()).hexdigest()[:12]
    head = (f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 {W} {H:.0f}" width="{W}" height="{H:.0f}" '
            f'role="img" aria-label="JIANT FM {escape(L[lang]["title"])} {VERSION}">\n'
            f'<!-- generated by tools/gen_nav_map.py from firmware/src/ui_lens.c (chains {sig}); do not edit -->\n'
            f'<title>JIANT FM · {escape(L[lang]["title"])} · {VERSION}</title>\n'
            f'<style>{fonts_css(sv.chars)}text{{font-family:\'JIANT Inter Tight\',\'Inter Tight\',Inter,system-ui,sans-serif}}</style>\n'
            f'{glyph_defs(sv.glyphs)}\n'
            f'<rect width="{W}" height="{H:.0f}" rx="28" fill="{BG}"/>\n'
            f'<rect x="1" y="1" width="{W - 2}" height="{H - 2:.0f}" rx="27" fill="none" stroke="#24212c" stroke-width="2"/>\n')
    return head + '\n'.join(sv.o) + '\n</svg>\n', H, drawn


def verify(keys, drawn):
    want = {(c['key'], p['i']) for c in keys for p in c['pages']}
    miss = want - drawn
    if miss:
        raise SystemExit(f'gen_nav_map: pages of the firmware missing from the map: {sorted(miss)}')
    secs = {c['sec'] for c in keys}
    rows = {s for r in ROWS for s in r}
    if secs != rows:
        raise SystemExit(f'gen_nav_map: sections {sorted(secs ^ rows)} are not on the map\'s rows')


RENDER_JS = r'''
const { chromium } = require("playwright");
const [svg, png, w, h] = process.argv.slice(1);
(async () => {
  const exe = process.env.CHROMIUM || "/opt/pw-browsers/chromium-1194/chrome-linux/chrome";
  const b = await chromium.launch({ executablePath: exe }).catch(() => chromium.launch());
  const p = await b.newPage({ viewport: { width: +w, height: +h }, deviceScaleFactor: 2 });
  await p.goto("file://" + svg);
  await p.waitForTimeout(300);
  await p.screenshot({ path: png, omitBackground: true, clip: { x: 0, y: 0, width: +w, height: +h } });
  await b.close();
})();
'''


def main():
    sec, keys = parse()
    check = '--check' in sys.argv
    for lang, name in (('en', 'navigation-map'), ('es', 'navigation-map.es')):
        doc, H, drawn = svg_doc(lang, keys)
        verify(keys, drawn)
        out = R / 'docs' / f'{name}.svg'
        if check:
            if not out.exists() or out.read_text() != doc:
                raise SystemExit(f'gen_nav_map: {out.relative_to(R)} is out of date (run tools/gen_nav_map.py)')
            continue
        out.write_text(doc)
        print(f'{out.relative_to(R)}: {len(keys)} chains, {len(drawn)} pages, {len(doc) // 1024} KB')
        if '--svg' not in sys.argv:
            png = out.with_suffix('.png')
            env = dict(os.environ)
            if 'NODE_PATH' not in env:
                env['NODE_PATH'] = subprocess.run(['npm', 'root', '-g'], capture_output=True, text=True).stdout.strip()
            subprocess.run(['node', '-e', RENDER_JS, str(out), str(png), str(W), str(int(round(H)))], check=True, env=env)
            from PIL import Image                      # (256 colours: a third of the bytes, the same to the eye)
            Image.open(png).quantize(256, method=Image.Quantize.FASTOCTREE, dither=Image.Dither.NONE).save(png, optimize=True)
            print(f'{png.relative_to(R)}: 2x, {png.stat().st_size // 1024} KB')
    if check:
        print('gen_nav_map: the maps match the firmware\'s chains')


if __name__ == '__main__':
    main()
