#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Watch the FM-1's audio load and dropouts over the USB serial console (MENU > USB > USB AUDIO: CONSOLE).
# Each second it sends `status` and prints the load, the slowest render, and what went wrong since the last
# second: halves rendered late, halves lost (a click each), voices shed by the load budget. Ctrl-C prints the totals.
#   pip3 install pyserial
#   python3 tools/glitch_watch.py /dev/cu.usbmodemXXXX [SECONDS]   (macOS; /dev/ttyACM0 on Linux, COM3 on Windows;
#                                                                  SECONDS: stop by itself after that long)
import sys
import time

try:
    import serial
except ImportError:
    sys.exit("glitch_watch: needs pyserial (pip3 install pyserial)")

HALF_US = 128 * 1_000_000 / 44_100          # one I2S half buffer (core.h HALF_FRAMES at 44.1 kHz): the render's budget
COUNTERS = ("audio_late", "audio_lost", "voices_shed", "voices_trimmed")


def status(port):
    port.reset_input_buffer()
    port.write(b"status\r")
    kv, end = {}, time.time() + 0.5
    while time.time() < end:
        ln = port.readline().decode("ascii", "replace").strip()
        if not ln:
            continue
        k, _, v = ln.partition(" ")
        try:
            kv[k] = int(v)
        except ValueError:
            pass
        if k == "fx_trips":                 # (con_status: the last counter before the track and engine lines)
            break
    return kv


def main():
    if len(sys.argv) not in (2, 3):
        sys.exit("usage: glitch_watch.py SERIAL_PORT [SECONDS]")
    secs = float(sys.argv[2]) if len(sys.argv) == 3 else None
    port = serial.Serial(sys.argv[1], 115200, timeout=0.2)
    first = status(port)
    if "audio_late" not in first:
        sys.exit("glitch_watch: no status from the FM-1 (is MENU > USB > USB AUDIO set to CONSOLE?)")
    last, worst_us, peak_cpu, t0 = first, 0, 0, time.time()
    print("  time   cpu%  slowest so far       late  lost  shed  trimmed   (late .. trimmed: in the last second)")
    try:
        while secs is None or time.time() - t0 < secs:
            time.sleep(1)
            now = status(port)
            if "audio_late" not in now:
                continue
            d = {k: now.get(k, 0) - last.get(k, 0) for k in COUNTERS}
            us = now.get("audio_max_us", 0)
            worst_us, peak_cpu = max(worst_us, us), max(peak_cpu, now.get("cpu_pct", 0))
            flag = "  <-- CLICK" if d["audio_lost"] else "  <-- late" if d["audio_late"] else ""
            print(f"{time.time() - t0:6.0f}s  {now.get('cpu_pct', 0):4d}  {us:5d} us ({us * 100 / HALF_US:3.0f} %)"
                  f"  {d['audio_late']:5d} {d['audio_lost']:5d} {d['voices_shed']:5d} {d['voices_trimmed']:8d}{flag}")
            last = now
    except KeyboardInterrupt:
        pass
    tot = {k: last.get(k, 0) - first.get(k, 0) for k in COUNTERS}
    print(f"\n{time.time() - t0:.0f} s: peak cpu {peak_cpu} %, slowest render {worst_us} us "
          f"({worst_us * 100 / HALF_US:.0f} % of the {HALF_US:.0f} us budget)")
    print(f"late halves {tot['audio_late']}, lost halves (clicks) {tot['audio_lost']}, "
          f"voices shed {tot['voices_shed']}, trimmed {tot['voices_trimmed']}")


if __name__ == "__main__":
    main()
