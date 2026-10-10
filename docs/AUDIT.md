# Original repository audit

Read all 11 files in `GameConsole/` and `pinout.txt`. No `AGENTS.md`, PlatformIO manifest,
dependency lock, board setup, display setup, or repository license was present. Git was clean
before implementation, at `5da4e3f`; original files remain untouched.

## Findings

* `Config.h` declares 176×220, and `Game::begin()` selects rotation 0. TFT pins are only in
  `pinout.txt`: MOSI23/SCK18/CS5/DC2/RESET4. These agree with ILI9225 portrait dimensions.
  Physical controller identity/orientation cannot be read back from this wiring because no
  TFT MISO connection is documented. This is source-verified configuration, awaiting device
  confirmation, not an identified physical panel.
* TFT_eSPI was included without a local setup or version. Its installed pinned **2.5.43**
  package has ILI9225 definitions, initialization, rotation, address-window support, and
  synchronous SPI writes. An early audit suspicion that the library lacked this driver was
  disproved by inspecting source. `invertDisplay()` has placeholder command definitions
  for this driver, so the new console uses software inversion.
* SPI frequency and transfer method could not be established for the original build because
  external Arduino library setup was missing. The game fills and pushes a 77,440-byte RGB565
  sprite every frame; no transfer profiling or allocation check existed. CRT draws dark lines
  across every second row, obscuring small projectiles. Rendering and simulation were tied
  to a nominal 16 ms gate, so transfer overhead slows gameplay.
* Controls are left32/right33/fire25/weapon14. Source defines back26 but the wiring note calls
  GPIO26 DATA-AH21. The new baseline leaves it alone and substitutes a simultaneous L/R
  chord. Optional Xbox Series BLE input, D-pad, A/B/X, battery reporting, and vibration
  existed; the controller MAC was hard-coded, and its disconnect method was empty.
* Existing features: 24-enemy formation, three weapon types, divers, bosses with three attack
  patterns, moving UFO and beam, three destructible shields, five power-ups, particles,
  stars, music/effects, settings, top-five leaderboard, pause, wave-clear, and attract mode.
  The OTA screen only displays text; networking/update service is absent. SD writes are
  commented out, so SD is not a working persistence dependency.
* GPIO16 signal LED conflicts with WROVER PSRAM. GPIO12 ambient LED is a flash-voltage
  strap. Both firmware outputs were disabled for reliability. There is no confirmed free
  equivalent LED output once optional expanded controls and the documented sensor/SD pins
  are reserved; they remain preserved in the archived sketch rather than arbitrarily remapped.
* GPIO27 already drives buzzer tones. Two overlapping engines existed; `SoundEngine` is
  unused by `Game`. Both use Arduino core-3 `ledcAttach`/pin APIs, which cannot compile
  unchanged on PlatformIO's selected core 2.0.17. The migration converts the live engine to
  channel-0 `ledcSetup`/`ledcAttachPin`/`ledcWriteTone`; the console shares one prioritized engine.
* NVS namespace `invadersv4` saves settings, one high score, and five leaders. There is no
  schema/range validation; `dificaulty` is misspelled and not applied in game rules. No other
  active asset storage exists. Sprites are flash constants. Console imports useful legacy NVS
  values without changing or deleting that namespace.
* State/input problems: assignment `if (state = GS_SETTINGS)`, no debounce, held back-button
  repeating pause/resume, stale timers between sessions, absolute expiry comparisons that
  fail on timer rollover, and UART startup waiting with no bound. Pause does not preserve
  power-up time, because old absolute deadlines continue advancing.
* Collision/rule problems: point-only projectile checks can tunnel; a shot consumed by a
  diver can also hit a formation enemy; shield geometry reports collisions in already-empty
  cells; double-shot is set but never affects firing; damage has no grace period; invasion
  game-over bypasses leaderboard persistence. Boss damage reads the current weapon rather
  than the weapon that emitted the bullet.
* No radio disable policy, version pins, chip/flash/PSRAM diagnostics, allocation fallback,
  host tests, or hardware timing evidence existed.

## Implementation sequence and outcomes

1. Preserved the original files and generated a structural graph of the original C++ sources.
   Established `migration` with core-2 LEDC, driver setup, `.cpp` entry point, unsafe LED
   outputs disabled, source state-assignment fix, L/R pause, and allocation failure reporting.
   Compiled successfully before the first console release build. Migration intentionally
   remains a historical compatibility stage; use the console for the full reliability fixes.
2. Added fixed-step timing, pools, swept collisions, debounced inputs, clipped drawing,
   validated/versioned records, lifecycle/menus, and improved Sly plus Sky implementation.
3. Completed the additional six games, sharing shooter infrastructure where rules overlap.
   Each entry has a complete playable implementation rather than a placeholder.
4. Added internal/PSRAM allocation tiers, dirty tiles/strips, boot transfer calibration,
   diagnostics, prioritized buzzer audio/music, and opt-in expanded/Xbox inputs.
5. Built all configurations and ran rule/lifecycle/render tests. Results are recorded in
   `VERIFICATION.md`; physical capacity, FPS, latency, and audio remain device checks.

## Original code graph

`graphify-out/graph.json` and `graph.html` describe the preserved **original** game, not the
new engine. Structural extraction found 283 nodes, 515 edges, and 10 communities. The tool
reported partial extraction of `Config.h` and `Sprites.h`, and skipped `.ino`; these files were
read directly during the audit. The graph is supporting navigation evidence, not a substitute
for reading source or proof that wiring works.
