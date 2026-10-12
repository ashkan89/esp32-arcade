# Build and test

Implementation date: 2026-10-11. Baseline commit: c36888f. Toolchain: PlatformIO Core with espressif32@6.10.0, Arduino-ESP32 2.0.17, ESP-IDF 4.4.7, TFT_eSPI 2.5.43, ArduinoJson 6.21.5. No display pin, catalog ID or partition was changed.

## Repeatable commands

```powershell
pio run -e release -e debug -e wroom-4mb -e wroom-16mb
python scripts/test_host.py
python scripts/verify.py
python scripts/preview.py --atari-states
```

verify.py builds all four environments, runs real C++ host rules, exports actual renderer previews, packages local WROVER/WROOM firmware, checks slot limits and writes SHA-256 summaries. It does not upload firmware or publish a release. Exact outputs are under test-results/, previews under artifacts/, packages under dist/.

On a supported GCC/Clang sanitizer runtime, run `python scripts/test_host.py --sanitize`. The existing Ubuntu release workflow runs ASan/UBSan. This Windows run uses installed MinGW; sanitizer CI has not been invoked and no sanitizer pass is claimed locally.

## Baseline

Unchanged-source four-environment build and host tests passed before implementation. Host: 2,223,249 assertions and 2,080,000 lifecycle ticks. Binary sizes: release 1,288,416; debug 1,447,824; WROOM 4 MB 1,261,072; WROOM 16 MB 1,261,088 bytes. Static RAM: 61,536 / 61,552 / 61,448 / 61,448 bytes respectively. TFT_eSPI’s unavailable-touch warning is expected for this button console.

## Final compiled and simulated results

| Environment | Static RAM bytes | Binary bytes | OTA slot bytes | Status |
| --- | --- | --- | --- | --- |
| release | 63,744 | 1,313,968 | 3,145,728 | PASS |
| debug | 63,760 | 1,482,704 | 3,145,728 | PASS |
| wroom-4mb | 63,648 | 1,286,544 | 2,031,616 | PASS |
| wroom-16mb | 63,648 | 1,286,544 | 3,145,728 | PASS |

```text
Focused campaign checks: 677249 assertions plus legacy endings and solvable warehouses
Lifecycle soak: 2080000 simulation ticks, 15327 game-over/restart cycles
BUDGET maxGame=6516/8192 Settings=2344 Save=2356 AudioSequencer=152 bytes
PASS audio arbitration/timeline, save migration, bounded AI, 1036800 effects-invariance ticks (Classic + Enhanced)
PASS 2227363 assertions across core, inputs, saves, menus, game rules and lifecycle soak
```


New checks cover: single-voice priority/preemption, queue overflow/expiry, coalescing/starvation, independent mute/music/SFX, musical position after interruptions/stalls/rollover/pause; transactional v1/v2/v3 save behavior; zero seed normalization; bounded path detour and committed aim; 100 seeds × 20 seven-piece bags; all-stage Tunnel Digger indices; moving-log edge/double support; bumper-center separation; target travel budgets; jump buffering/coyote; camera-relative projectiles; bomb ray blocking; rim commitment and simultaneous grace; 128 seeds × all 48 Enhanced river difficulty states; depot positions, seams, bridge passage and retry determinism; 16 seeds × four player difficulties × both speed extremes through ten consecutive river sections; tank/guard cover detours and tank separation; legal Keystone commitments, timed escalators, lift descent/interpolation and capture inside the cabin.

Effects-invariance coverage runs 1,036,800 actual update ticks across all 52 titles, eight seeds, four difficulties and both Atari modes, comparing score, RNG, stage, health and game-over every tick. Existing lifecycle soak adds 2,080,000 ticks and repeated restart/switch/pause checks. Failures print the game/seed/tick where applicable. These are rule/invariant tests, not evidence of human enjoyment or physical board timing.

## Memory, scheduling and reproduction

The shared 8 KiB arena still enforces each game size at compile time. Save bytes increased from 304 to 2,356; category records add 2 KiB internal state. Decode avoids nested migration copies. Music and descriptors are const flash data; sequencer/event state is fixed and small. No gameplay hot-loop allocation was introduced. WROOM still uses RGB332/internal staging without PSRAM; WROVER uses ordinary mapped PSRAM. Frame history is now removed, and write suppression adds a 2,356-byte RAM snapshot on all targets; earlier build sizes above precede this change.

`RUN game=... difficulty=... enhanced=... seed=...` reports the effective game seed. Reproduce in host tests by setting GameOptions.seed/difficulty/enhanced. A firmware build may define ARCADE_FIXED_SEED to reproduce the boot/run seed sequence; this is not a new production menu. Input frames and game ID are still needed to replay an entire session.

60 Hz fixed simulation and bounded catch-up remain; rendering uses the existing 30/60 cap and automatic 30 Hz fallback. Debug serial records maximum update including AI, drawing, SPI transfer, audio service cost/gap, frame buckets, input gap and internal heap. AI is included in update cost, not separately timed. No ESP32 runtime samples were collected here; host CPU times must not be substituted for board timings. Additional task/ISR scheduling and richer WROVER effects await measured need.

## Physical checks still pending

- Test WROVER N16R8, WROOM 4 MB and WROOM 16 MB with their correct images. Verify boot health, expected flash/PSRAM detection, no-PSRAM operation and existing failure behavior.
- Play every title on all difficulty presets; prioritize late-wave shooters, cover detours, maze escape, platform reachability, duel recovery and racing response windows. Check all eight buttons, pause/resume/restart and reduced effects.
- In both Atari modes check fuel collection versus shooting, slow/fast bridge approaches, retries, two-player alternation, mall lift/escalator/roof transitions, jumps/ducking and later-tier capture/hazard timing.
- Listen to all 53 preview tracks on the fitted passive piezo. Check master/music/SFX persistence, shot storms, warning clarity, pause silence, resume position, restart, attract mute and results cues.
- Measure GPIO27 frequency/duty and worst service gap during full-frame rendering, network menus and NVS writes. Compare debug update/draw/transfer/audio maxima and input latency with sustained gameplay. Establish an actual jitter limit before changing scheduler design.
- Reboot after changing settings/records; inspect Classic/Enhanced/difficulty category separation and B’s labeled legacy aggregate view. Test long sessions, low-memory/PSRAM failure and existing OTA rollback health checks.

No physical upload, playtest, listening session or oscilloscope measurement was performed. Host previews verify renderer layout only, not LCD color fidelity or the piezo’s perceived sound.
