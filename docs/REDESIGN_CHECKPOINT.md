> Current 3.7.0 artwork and COM9 installation evidence: [MODERN_GRAPHICS.md](MODERN_GRAPHICS.md). Earlier revision measurements below remain historical.

# Redesign audit — October 28, 2026

The starting repository is release 3.5.1 at `5625d67`, with a clean working tree.
It already contains 52 implemented native games, a shared bounded runtime, pixel artwork,
audio sequencing, persistence, Atari recreations and extensive host regressions. This
iteration preserves that work and repairs unmet interface and documentation requirements.
It does not claim that simulated gameplay establishes production quality on the device.

## Hardware compatibility and baseline

`include/BoardConfig.h` and `platformio.ini` are the active configuration. The wiring table
in HARDWARE.md agrees with them. References to rewiring describe a historical migration,
not an instruction to alter the present console. No pin, polarity, SPI frequency, display
initialization, partition, watchdog, brownout setting or boot capacity check was changed.

- Chip: classic dual-core ESP32; build profiles WROVER N16R8, WROOM N4 and WROOM N16.
  Exact currently attached module cannot be verified: serial enumeration found no ports.
  Historical COM5 logs report 16 MB flash and 8 MB physical PSRAM; they are earlier evidence.
- Display: ILI9225, 176×220 portrait, rotation 0, SPI MOSI23/SCLK18/CS13/DC26/RST14,
  no MISO/touch/backlight GPIO. Transfer is RGB565 at 20 MHz; WROOM stores RGB332.
  Full RGB565 payload is 77,440 bytes, requiring at least 30.976 ms on the bus.
  Earlier measured full presentation was about 74.6 ms; no current device FPS is claimed.
- Input: eight active-low ground switches; GPIO25/21/32/33 directions, 22 action,
  4 secondary, 5 pause, 15 back. Internal pull-ups, 20 ms debounce, 350 ms initial
  repeat, 120 ms repeat interval, 700 ms long press. GPIO5/15 are boot straps.
  Gameplay Menu/Back pauses; pause has Resume/Restart/Controls/Exit. Transitions
  suppress held buttons until release.
- Audio: one LEDC passive-piezo voice on GPIO27, nonblocking sequencer with priority
  effects, master/music/SFX mute. No amplifier, stereo or calibrated volume is evidenced.
  No brightness slider is added because backlight control is not configured.
- Storage: Preferences/NVS; no SD or filesystem. 16 MB layout has two 3 MiB application
  slots; 4 MB layout has two 1,984 KiB slots. Both retain NVS, OTA metadata and coredump.
  No new networking is added; existing on-demand Wi-Fi and verified OTA are retained.
- Rendering: WROVER has two 77,440-byte mapped-PSRAM buffers; WROOM has one 38,720-byte
  internal framebuffer. Shared game arena is 8,192 bytes. No game hot-loop allocation,
  new tasks, banked screenshot history or frame copies were introduced.
- Builds use espressif32 6.10.0, Arduino-ESP32 2.0.17 / IDF 4.4.7, TFT_eSPI 2.5.43,
  ArduinoJson 6.21.5. PlatformIO dependencies are pinned in platformio.ini.
- Unchanged release baseline passed: static RAM **66,128 bytes**, linked flash
  **1,312,989 bytes**. Evidence: `artifacts/baseline-20261028.log` (local ignored log).
  An initial concurrent host build overlapped source edits and had mixed-object linker
  errors; it is not counted as a baseline pass. Final verification runs against fixed sources.
- Runtime heap, stack high-water marks, power stability, achieved FPS, physical controls
  and sound cannot be remeasured without a device. Existing boot diagnostics and serial
  profiling are retained. Historical himem startup and TLS-watchdog fixes are documented
  in VERIFICATION.md; protection was not disabled to work around them.

## Migration stages

1. Audit registry, configuration, history and original Atari manuals; build unchanged firmware.
2. Add a fixed-capacity library view: stable IDs, categories, favorites, recent and sorting.
3. Extend saves to v4: preserve v1/v2/v3 data; append eight validated recent IDs under CRC.
4. Integrate library selection, empty states and deliberate reset confirmation in the real UI.
5. Run all game rules, simulation, migration, UI and rendering checks; build four targets;
   regenerate previews and local release packages. Physical acceptance remains separate.

The game identity is **Keystone Kapers**, verified by the source class, registry title,
and [Activision's original manual](https://www.atariage.com/manual_html_page.php?SoftwareLabelID=261).
[River Raid's manual](https://www.atariage.com/manual_html_page.php?SoftwareLabelID=409)
confirms its controls, fuel, scoring and reserves. Existing research distinguishes confirmed
rules from reconstructed terrain, sprites, mall coordinates and hazard schedules. No ROMs
or extracted commercial assets were added. See ATARI_RESEARCH.md and THIRD_PARTY.md.

## New interface behavior and save compatibility

Menu → Library selects All, Favorites, Recent, Combat, Action, Puzzle or Atari. Sorting is
catalog order, title A–Z or comparable mode score; Recent always uses launch order. Selection
always resolves to an original catalog ID, including scores and instructions. Empty lists
cannot launch an unrelated game. Filters and sort are session preferences, not flash writes.

Recent stores eight unique IDs, newest first. Only an actual run launch changes history;
preview, browsing and attract do not. Replaying the newest game leaves the list unchanged.
The existing two-second quiet window and one-minute write-attempt limit apply unchanged.

Save v4 is 2,364 bytes, eight bytes larger than v3. The decoder accepts exact historical
length/version pairs, validates CRC, flags and packed unique recent IDs, then commits
transactionally. Existing aggregate/mode scores and favorites migrate without reassignment.
An older firmware's decoder cannot read v4. Rolling back to that firmware can show default
or historical records and, if it saves, overwrite the newer state. Firmware rollback alone
is not backward save compatibility; retain an NVS backup before a deliberate downgrade.

Settings → Reset Settings / Scores has separate actions, a second confirmation screen,
and Cancel selected initially. Preferences reset preserves records; score reset preserves
preferences. Both preserve favorites, recents, last selection and Wi-Fi credentials. Resets
use the ordinary deferred persistence policy: power-off before saving can restore old data.
No reset is performed automatically by this implementation task.

## Completion boundary

All original 52 games are retained; none are replaced by a title screen or generic duplicate.
No optional games were added. GAMES.md covers controls/objectives; GAMEPLAY_AUDIT.md records
per-game source findings; ATARI_RESEARCH.md records fidelity deviations. Host-tested and
build-verified statuses must be read separately from physical acceptance. Hardware tuning,
manual playthroughs of all 52 games, audio listening and faithful Atari screen comparison
remain required before calling this a production-ready product.

## Current host verification

`python scripts/test_host.py` passed with **2,282,492 assertions**, **2,080,000**
four-difficulty lifecycle ticks, **15,323** game-over/restart cycles and **1,036,800**
Classic/Enhanced effects-invariance ticks. Focused campaign checks reported 1,744,638
assertions. These are separate counters with overlapping coverage; do not sum them.
The native build emitted no warnings. Logs are `test-results/host-build.log` and
`test-results/host-tests.log`.

New regressions cover recent eviction/promotion/no-change replay; v3-to-v4 retention of
all score arrays and favorites; CRC corruption at each saved byte; duplicate, invalid
and sparse recent lists; category partitioning; title/mode-best ordering; empty views;
launches after filter changes; reset cancellation/confirmation; and guard bytes around
the added RGB332 menu renders. Existing game-specific rules remain in the same suite.

`artifacts/menu-preview.png`, `console-preview.png`, `atari-preview.png` and
`atari-states.png` were regenerated from real C++ output. Menu review found the new
library, favorites, recent, reset and confirmation screens legible within the logical
176×220 canvas. These are host previews, not photographs or physical color/timing evidence.
`python scripts/catalog_report.py` generates the numbered controls inventory and
`test-results/game-verification.csv`, whose cells distinguish exercised host paths from
pending physical acceptance.

## Final firmware and local artifacts

All four 3.6.0 targets passed. Build log: `test-results/firmware-builds.log`.
Machine-readable sizes and SHA-256 values: `test-results/firmware-summary.json`.

| Target | Static RAM bytes | Linked flash bytes | Application binary bytes | Application slot bytes |
| --- | --- | --- | --- | --- |
| release / WROVER N16R8 | 66,208 | 1,316,149 | 1,322,720 | 3,145,728 |
| debug / WROVER N16R8 | 66,224 | 1,476,769 | 1,483,344 | 3,145,728 |
| WROOM 4 MB | 66,152 | 1,291,773 | 1,298,352 | 2,031,616 |
| WROOM 16 MB | 66,152 | 1,291,773 | 1,298,352 | 3,145,728 |

Release static RAM increased by 80 bytes and linked flash by 3,160 bytes versus the
unchanged baseline. Static RAM excludes runtime framebuffers, network heap and stacks.
Native budget checks report a largest game of 6,516 / 8,192 bytes, Settings 2,352 bytes,
serialized save 2,364 bytes and host AudioSequencer 160 bytes. Host object sizes are not
substitutes for ESP32 runtime measurements.

`dist/` contains application images, first-install factory images, board-specific manifests
and SHA256SUMS.txt for WROVER N16R8, WROOM N4 and WROOM N16. All three packaging checks
passed: image header, board/version markers, A/B/OTA partition layout, application size
and SHA-256. The debug image remains in `.pio/build/debug/firmware.bin`.
Factory images include blank NVS space and are for deliberate first installation; they
must not be used as an ordinary settings-preserving upgrade. No upload, Git tag, GitHub
release or other publication was performed.

Reproduce the complete local verification with PlatformIO Core 6.2.0:

```powershell
python scripts/verify.py
```

This run executed the same build, host, catalog, preview and packaging stages separately
to allow independent builds and host compilation to overlap. The host compiler was the
PlatformIO MinGW-W64 GCC 5.1.0 fallback. No sanitizer pass is claimed. `git diff --check`
and Python syntax checks passed. This iteration changes shared console/persistence behavior
and retains the existing per-game gameplay/art implementations. It is not a new rewrite
of all 52 games or proof that they meet the requested final production quality bar.

Resume at physical acceptance in HARDWARE_CHECKLIST.md, using the correct board image.
Prioritize all 52 actual control/play/pause/restart/end/exit paths, late-stage balance,
Atari fidelity comparison, piezo listening, repeated-switch minimum heap/stack measurements,
save migration/reboot/reset and power-loss recovery. These checks remain open.
