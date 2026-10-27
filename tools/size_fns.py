#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments
# Portions Copyright (C) 2026 Juan Ignacio Antonucci (Juan Jiant)
"""Build the main-loop code for size (build.py): the functions defined in SIZE_FILES get LLVM's
minsize (about -Oz) while everything else (the sound, render and ISR code, the flash and OTA code,
the main loop) keeps -Os. The JieLi clang 4 has no '#pragma clang attribute' and its minsize
attribute is an error on variables, so build.py compiles felucca.c to LLVM IR without the
optimizer, this script adds 'minsize' to those definitions, and the IR is then compiled at -Os
(without the edit, that round trip is byte-identical to compiling felucca.c directly).
  size_fns.py IN.ll OUT.ll"""
import re
import sys
from pathlib import Path

_ROOT = Path(__file__).resolve().parents[1]
SRC = _ROOT / "firmware" / "src" if (_ROOT / "firmware" / "src").is_dir() else _ROOT / "src"   # either layout
# the UI, the stores, the editor and the console: main loop only, never called by sound code; and (1.1) every
# other file without DSP: USB, MIDI, the sequencer, the drawing, the LCD, OTA, ... Of those, what an interrupt
# reaches (ISR_ROOTS and everything they call or name, from the IR) keeps -Os (isr_reach)
SIZE_FILES = ["ui.c", "favorites.c", "menu_items.c", "cpu_safe.c", "icons.c", "ui_graph.c", "ui_draw.c", "ui_menu.c", "ui_input.c",
              "storage.c", "upreset.c", "project.c", "settings_persist.c",
              "editor.c", "editor_preferences.c", "editor_backup.c", "editor_menu.c", "console.c",
              "usb.c", "midi_uart.c", "midi_control.c", "midi_clock.c", "seq.c", "song_chain.c", "motion.c",
              "panel.c", "lcd.c", "gfx.c", "ota.c", "ota_hw.c", "up_fm6.c", "params.c", "editor_fm6.c",
              "ui_layer.c", "ui_name.c", "ui_events.c", "song_main.c", "main.c", "libc.c", "storage_hw.c", "engines.c", "fm4_convert.c",
              # (0.9.0) the files the ones above include, missed until now (-16.7 KB): GEN, DICE, the lenses, the line
              # art, the beings, the tech graphs, the tools, the FM6 bank and VOICE's words (its sound side, reached
              # from the audio ISR through ENG[]'s hooks, keeps -Os: isr_reach follows globals)
              "gen.c", "gen_tables.c", "song_dice.c", "ui_lens.c", "ui_organic.c", "ui_fm6op.c", "ui_being.c", "ui_garden.c",
              "ui_engvis.c", "ui_tech.c", "ui_lzsave.c", "ui_lzfx.c", "ui_lzhome.c", "ui_lzedit.c", "ui_lzpage.c", "ui_dx.c",
              "ui_tools.c", "fm6_vbank.c", "sing.c", "seq_live.c", "ui_live.c",
              # (0.9.7) the main-loop functions of the sound files (what the interrupts reach stays -Os, as above): the
              # MASTER pot's boot and poll, FM6's patch store, the kit's names and grids, MOD's names, the PUNCH lane
              "pfx.c", "master.c", "audio.c", "mod.c", "fx.c", "drumx_voice.c", "eng_drum.c", "eng_fm6.c",
              # (0.9.15) for their event code below (what runs every sample or block there stays -Os)
              "perform.c", "voice.c", "song_play.c", "eng_wheel.c", "eng_formant.c", "eng_animal.c", "slicer.c",
              "eng_analog.c", "eng_lofi.c", "eng_noise.c", "eng_phase.c", "fugue.c", "../hal/fm1_input.h"]
# (0.9.14) of what an interrupt reaches, the code that runs once an event (a USB control transfer, a MIDI message, a
# step, a note, a recorded key), not once a sample or a block: built for size as the main loop is (1.9 KB, the room
# PROFILE needed in the slot). By prefix; EVENT_KEEP the ones among them that do run every block
EVENT_PFX = ("ep0_", "e0_", "ep1_", "ep3_", "sysex_", "midi_", "um_", "uart_midi_", "ml_", "mn_", "mo_", "seq_step",
             "seq_start", "seq_stop", "seq_note", "seq_release", "seq_ratchet", "arp_", "chord_", "cp_", "rec_",
             "motion_", "cin_", "latch_", "key_", "kb_", "keys_", "fm6_note_on", "drum_note_on", "dxo_on", "dxo_off",
             "dxo_rec_hold", "pfx_fire", "seq_lane_rat", "sy_word", "sy_gen_word", "sy_enter", "live_",
             "mod_key", "mod_note", "sing_note", "ly_", "step_parts", "step_rolls", "tie_span", "every_on", "scale_snap",
             # (0.9.15) a note (its voice found, started, ended), a key, a section or song row in, the FX layer's buffer
             # effect started or switched (a 1/16 at most), a fault: not a sample's or a block's work
             "trk_note_on", "trk_note_off", "voice_start", "voice_victim", "voice_room", "mono_play", "input_on",
             "input_off", "wheel_note_on", "formant_note_on", "animal_init", "drum_oct_note", "sec_apply", "chain_apply",
             "srec_finish", "scene_capture", "fx_trip", "rev_state_clear", "sl_enter", "perf_buf_start", "perf_buf_select",
             "perf_buf_done", "perf_glt_step", "perf_switch", "perf_press", "perf_hush_end", "pfx_resync", "pfx_note",
             "pfx_hush", "pfx_lane_", "strn_note", "scale_steps", "cg_near", "dxo_part", "trk_nvoice", "lofi_note_on",
             "noise_note_on", "animal_note_on", "analog_note_on", "phase_note_on", "fm6_note_of", "glide_set", "glide_from",
             "mono_pick", "mono_remove", "lowest_held", "srec_", "trk_fade_out", "voice_kill", "trk_all_off", "fg_reset",
             "fg_release", "step_set_lmode", "st_micro_set", "lofi_desc", "noise_desc", "animal_desc", "formant_desc",
             "param_desc_of", "param_fit", "uac_rate_set", "uac_ep4_reset", "fl_reset", "rev_line_clear", "dly_line_clear",
             "rev_clear", "dly_lo_reset", "slicer_start", "slicer_drop", "perf_drop_slicer", "dx_mute_set", "mc_lost",
             "uin_ep_config", "fm1__anim_frame", "sy_tok", "trk_chance", "drum_oct_of", "perf_key",
             "perf_len", "mod_midi", "sy_norm", "fl_ok", "get_desc", "drum_muted", "drum_lane", "trk_kit", "drum_kit_of",
             "name_of",
             # (0.9.17) the glitcher's once a slice, segment, chunk, grain or block: GLITCH's segments, SHUFFLE's 1/16s
             # and chunks, FREEZE's grains, the scratch's hand, the knobs' focus
             "perf_glt_seg", "perf_shuf_step", "perf_shuf_go", "perf_frz_grain", "perf_scratch", "perf_gfocus",
             "perf_gx", "perf_pick", "pf_hash")
EVENT_KEEP = {"mo_block", "ep1_tx", "key_latch", "midi_clock_advance", "cin_block", "kb_lat", "live_q", "live_adv"}
ISR_ROOTS = ("fm1_alnk0_irq", "fm1_timer5_irq",      # hal/fm1_isr.S: the audio ISR, the key / LED scan (+ USB poll),
             "fm1_timer5_top")                       # (0.9.14) the scan's body, timed
DEF = re.compile(r"^(?:static|void|int|uint\w*|int\w*|const)\b[^;=(]*?\b([A-Za-z_]\w*)\s*\(", re.M)
IR_DEF = re.compile(r"^(define [^\n]*?@\"?([\w.]+)\"?\([^\n]*\)(?: unnamed_addr| local_unnamed_addr)?)( #\d+[^\n]*\{)$",
                    re.M)


def definitions(text):
    """names of the functions text defines (a signature at column 0 whose ')' is followed by '{')"""
    names = []
    text = re.sub(r"/\*.*?\*/|//[^\n]*", " ", text, flags=re.S)        # comments (none hold code)
    text = re.sub(r"\b__attribute__\s*\(\((?:[^()]|\([^()]*\))*\)\)", " ", text)   # (0.9.0) before the name too
    for m in DEF.finditer(text):
        i, depth = m.end() - 1, 0
        while i < len(text):
            depth += {"(": 1, ")": -1}.get(text[i], 0)
            i += 1
            if depth == 0:
                break
        rest = re.sub(r"^(__attribute__\s*\(\(.*?\)\)\s*)+", "", text[i:i + 200].lstrip())
        if rest.startswith("{"):
            names.append(m.group(1))
    return names


def size_names():
    names = set()
    for f in SIZE_FILES:
        names.update(definitions((SRC / f).read_text()))
    return names


IR_BODY = re.compile(r"^define [^\n]*?@\"?([\w.]+)\"?\((.*?)^\}", re.M | re.S)
IR_REF = re.compile(r"@\"?([\w.]+)\"?")
IR_GLOBAL = re.compile(r"^@\"?([\w.]+)\"? = [^\n]*", re.M)


def isr_reach(ir, with_globals=False):
    """the functions an interrupt can run: ISR_ROOTS and, transitively, every function their bodies call or
    name (a pointer taken there counts as called), from the unoptimised IR (before inlining: a superset).
    (0.9.0) A global they name counts as its initialiser: a function in a table they read (an engine's hooks:
    ENG[].block -> formant_block -> sing_block) is reached too"""
    body = {m.group(1): m.group(2) for m in IR_BODY.finditer(ir)}
    fns = set(body)
    for m in IR_GLOBAL.finditer(ir):
        body.setdefault(m.group(1), m.group(0))
    seen, todo = set(), [r for r in ISR_ROOTS if r in body]
    while todo:
        f = todo.pop()
        if f in seen:
            continue
        seen.add(f)
        todo += [g for g in IR_REF.findall(body[f]) if g in body and g not in seen]
    return seen if with_globals else {f for f in seen if f in fns}


def main(src, dst):
    ir = Path(src).read_text()
    isr = isr_reach(ir)
    names, hit = {n for n in size_names() if n not in isr or (n.startswith(EVENT_PFX) and n not in EVENT_KEEP)}, set()
    # (0.9.15) an event function of some size that an interrupt's -Os code calls stays a call (noinline): inlined there
    # it would be built -Os in its caller and its minsize lost (ep0_service into TIMER5, chord_glide, the arp's notes)
    body = {m.group(1): m.group(2) for m in IR_BODY.finditer(ir)}
    fast = {f for f in isr if f not in names}
    called = {g for f in fast for g in IR_REF.findall(body.get(f, ""))}
    out = {n for n in names & isr & called if body.get(n, "").count("\n") >= 0}

    def mark(m):
        if m.group(2) not in names:
            return m.group(0)
        hit.add(m.group(2))
        return m.group(1) + (" minsize noinline" if m.group(2) in out else " minsize") + m.group(3)
    Path(dst).write_text(IR_DEF.sub(mark, ir))
    print(f"size: {len(hit)} main-loop and event functions built for size ({len(names) - len(hit)} not in this build; "
          f"{len((size_names() & isr) - names)} reached from an interrupt kept -Os)")
    return 0


if __name__ == "__main__":
    sys.exit(main(*sys.argv[1:3]))
