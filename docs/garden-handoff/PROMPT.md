# Kickoff prompt

Paste the block below into Claude Code, opened at the root of the `JIANT` repository on a machine that can
build the firmware (Linux x86-64, or WSL2 / Docker; see INSTRUCTIONS.md §9 Phase 0).

---

You are implementing the GARDEN UI for the FM-1 firmware in this repository (branch `stems-10-fixed`).

1. Read `docs/garden-handoff/INSTRUCTIONS.md` in full. It is the spec, and its §3 lists the owner's decisions,
   which are final.
2. Look at the reference images in `docs/garden-handoff/images/` (pages, creatures, menus; `_a_music`,
   `_b_music` and `_c_silent` show the same screen at different moments and music levels). Open the mockups in
   `docs/garden-handoff/mockups/` when you need the exact drawing and motion: their code is the source of truth
   for the look. The owner's own references are in `docs/garden-handoff/refs/`.
3. Read `docs/GUI-REVAMP-GARDEN.md` §4 for the scene of every page, and `docs/STEMS-10-BUILD.md` for the flash,
   RAM and CPU limits and the glitch test.
4. Work through the phases in INSTRUCTIONS.md §9 in order. Start with Phase 0: set up the toolchain, build the
   tree as it is, run `tests/run_tests.sh`, and record the baseline (size, free bytes, test results).
5. At the end of each phase: build, report the app size against the slot, run the host tests, commit with a clear
   message, and give me a short report (what changed, size before and after, tests, anything you need decided).
   Then continue with the next phase unless something needs my decision.
6. Never exceed the flash slot, never move hot audio code out of RAM, keep every control's behaviour, and keep
   saved settings, palette ids and editor ids append-only. If a phase would break one of these, stop and ask.
7. Do not push or open a pull request unless I ask.
