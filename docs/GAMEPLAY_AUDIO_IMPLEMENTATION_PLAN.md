# Gameplay and audio implementation plan

Prepared 2026-10-11 from the supplied `ESP32_Arcade_Gameplay_Audio_Codex_Prompt.md` and the current repository.

## Implementation status

The user subsequently authorized full implementation. The original planning text below records the pre-change assessment. Implemented outcomes and verification are now documented in [GAMEPLAY_AUDIT.md](GAMEPLAY_AUDIT.md), [RESEARCH_NOTES.md](RESEARCH_NOTES.md), [AUDIO_GUIDE.md](AUDIO_GUIDE.md), and [BUILD_AND_TEST.md](BUILD_AND_TEST.md). Physical validation remains explicitly pending.

## Scope and evidence

The user's request is to review the document and plan implementation. The document's embedded instructions to implement immediately are requirements for a later implementation task, not authorization to change firmware during this review. This file is a plan; proposed work is not implemented or physically verified.

Preserve all 52 registered games, stable catalog IDs, controls, display wiring, saved records, and supported board configurations. Improve gameplay first, original buzzer audio second, and measured polish third. Keep changes in independently buildable batches rather than replacing the engine.

Initial inspection found no `AGENTS.md` in the project, `C:\Projects`, or `C:\`. Git was clean before this review. The existing graph was queried for orientation and then source files were checked directly; its truncated traversal is not a complete gameplay audit.

## Confirmed starting point

| Area | Repository evidence | Implementation consequence |
| --- | --- | --- |
| Catalog | `include/games/Catalog.def`: IDs 0–51 | 50 campaign games plus Keystone Kapers (50) and River Raid (51); audit all 52 individually. |
| Framework | `platformio.ini` and installed SDK version headers: Espressif32 6.10.0, Arduino-ESP32 2.0.17, IDF 4.4.7 | Keep the pinned toolchain and Arduino 2.x LEDC API. A 3.x migration is separate work. |
| Display | `include/BoardConfig.h`: ILI9225, portrait 176×220, rotation 0, SPI 20 MHz | Preserve MOSI 23, clock 18, CS 13, DC 26, reset 14. |
| Input | `include/BoardConfig.h`, `Core.cpp`: eight buttons, debounce/repeat/suppression | Preserve mapping and consume button edges once across catch-up steps. |
| Buzzer | `docs/HARDWARE.md`, `Hardware.cpp`: documented passive piezo, GPIO27, resistor, LEDC channel 0 | Single pitched voice, fixed duty; verify actual part/circuit on hardware before claiming listening quality. |
| Simulation | `Core.h::StepClock`, `Game.h`, `main.cpp` | Already approximately 60 Hz fixed simulation with bounded catch-up; retain it and test pause/stalls rather than replacing it. |
| Rendering | `Hardware.cpp::Display`, `main.cpp` | Synchronous RGB565 transfers, internal staging; WROVER RGB565 buffers, WROOM RGB332. Keep rendering separate from simulation. |
| Memory | `Registry.cpp`, `docs/WROOM.md`, `Persistence.h` | Shared 8 KiB game arena, bounded game pools, saved-state RAM snapshot; frame history removed. New state must fit; flash capacity does not add RAM. |
| Builds | `platformio.ini` | WROVER `release` and profiling `debug`; WROOM `wroom-4mb` and `wroom-16mb`. |
| Flash slots | Partition CSVs and existing WROOM documentation | 4 MB board: 1,984 KiB per OTA application; 16 MB boards: 3 MiB. Preserve A/B OTA and identities. |
| Difficulty | `Game.h`, `Core.h`, `Console.cpp` | Four presets already exist. Tune by genre; retain classic exceptions and start-of-run application. |
| Audio | `Hardware.h::Audio`, `Hardware.cpp::Audio`, `main.cpp` | Existing nonblocking short effects and eight-note menu phrase; needs musical timeline, track mapping, coalescing, explicit priorities, and independent settings. |
| Testing | `tests/rules.cpp`, `scripts/test_host.py`, `scripts/verify.py` | Extend real C++ host tests and existing lifecycle/renderer coverage. Hardware audio currently has no pure host-tested sequencer. |

## Concrete gaps found in the initial inspection

1. `ShooterBase.cpp::explode` uses `rng` for particle velocity/lifetime, while enemy selection and power drops use that same stream. Effects on/off can change subsequent gameplay. Add separate gameplay and cosmetic streams before new random behaviors; audio gets its own stream if needed.
2. `main.cpp` seeds from `esp_random()`, but `Console.cpp` derives some new-run seeds from elapsed time. Centralize run seed creation and expose the effective seed for reproductions. Radios-off hardware RNG output is not automatically a continuous true-entropy source.
3. `Audio::next` bases note deadlines on the current service call, and `Audio::update` advances at most one expired note. Late service can stretch phrases. Implement an absolute musical timeline with bounded overdue-event handling.
4. Effect priority is currently the `Sound` enum ordinal, and equal-priority effects can restart a cue. Define musical/critical priorities explicitly; rate-limit repeated shots and navigation effects.
5. Music is enabled only on Boot/Launcher in `main.cpp`; there is no per-game soundtrack mapping or independent persisted music/SFX setting. `Settings` has only a single sound flag.
6. `PongCircuit::simulate` follows the current ball height every tick with a movement cap but no reaction interval or prediction error. Add sampled observations and bounded, imperfect interception as a first AI example.
7. `KeystoneKapers::thief` selects direction every tick and changes floors at path edges. Review transitions against the verified mall topology and add commitment windows where appropriate; do not label the current behavior impossible without a failing case.
8. River Raid already has deterministic sections, fuel, checkpoints, two-player state, bridges, and 48 difficulty states. Existing geometry clamps are not a full route-and-fuel feasibility proof. Build validation before changing generation, and retain classic sequences by default.
9. `Hardware.cpp::Audio::next` calls `ledcWriteTone(0, hz)` followed by `ledcWrite(0, 128)`, described as 50% duty. The installed Arduino 2.0.17 `cores/esp32/esp32-hal-ledc.c::ledcWriteTone` reconfigures the timer to 10-bit resolution and writes `0x1FF`; overriding it with 128 gives approximately 12.5%, not 50%. Preserve the tone API's intended duty or explicitly use the verified resolution. Validate frequency/duty with hardware measurements; do not infer perceived loudness from duty.

Other catalog rows below are proposed audit targets, not proven defects. A full source audit remains part of implementation.

## Batch 0 — baseline and complete audit

Record commit, working-tree state, installed SDK configuration, build sizes, partition checks and existing host results. Build all four environments without changing source. Separate fresh results from old `test-results` and documented historical results.

Create `GAMEPLAY_AUDIT.md` with one row per stable ID and concrete file/function/line references. For each game inspect update, input, rendering, collision, score/progression, generation/random use, audio, allocations, pause/restart, and failure behavior. Record severity, proposed fix, tests and final disposition, including justified unchanged mechanics. Every game must get an individual review even if it shares an engine.

Inspect game arena maximum sizes and entity counts; display transfer and service intervals; allocation/PSRAM failures; timer/channel ownership; boot health and NVS persistence. Existing `delay(1)` is a loop yield and `delay(100)` is the halted startup path: neither should be reported as delay-based music without evidence.

Acceptance: reproducible baseline commands/results; 52 audit entries; hardware unknowns distinguished from repository facts.

## Batch 1 — determinism and gameplay foundations

- Add independently derived nonzero gameplay/cosmetic/audio seeds, preserve the effective run seed and provide test/debug seed injection. Replay equality concerns game state, not particle state.
- Keep River Raid's classic feedback-register generator separate from Enhanced generation and ordinary game RNG.
- Verify the installed RNG implementation and documented radios-off behavior. A startup-only entropy sampling window may be used if necessary after checking conflicts; do not enable radios during gameplay just for variation.
- Retain the fixed clock; add focused tests for long stalls, wraparound, transitions, held inputs and pause/resume without accumulated catch-up.
- Reuse existing swept/segment collision helpers where fast movement requires them; test actual identified tunneling cases rather than replacing all collision code.
- Add compact behavior helpers only when multiple games need them: state dwell/cooldown, bounded lead aim, weighted recent-action selection and attack slots. Keep genre-specific behavior in game files.
- Preserve old records while separating new comparable score categories by difficulty and Classic/Enhanced rules where rules differ. Define migration/storage size before expanding settings.

Acceptance: gameplay is identical for the same seed and inputs with effects/music toggled; no hot-loop allocations; game arena stays within its checked capacity; all baseline tests and builds pass.

## Batch 2 — high-impact gameplay

Start with Sky Invaders, Sky Fighters, Star Assault, Meteor Patrol and Pong Circuit. Give each a distinct encounter rhythm and test its own rules. Use reaction delays, maximum lead, aim error, attack commitment and coordinated pressure limits. Do not infer hidden future inputs. Telegraph strong attacks and verify an escape corridor against speeds, hitboxes and the 176×220 geometry.

Boss work should provide explicit phases, repeat penalties, cues and recovery windows only in games with existing suitable bosses. Difficulty modifies visible pressure and decision windows between encounters; optional adaptation is deferred unless playtesting demonstrates a benefit. Never silently change hitboxes/health mid-encounter.

Acceptance: legal state transitions, bounded attack concurrency, seeded variation, clear response windows, distinct behavior across the three shared-engine shooters, and no scoring/pause regressions.

## Batch 3 — shared audio service and composition

Extract a pure C++ `AudioSequencer` (proposed `include/arcade/AudioSequencer.h`, `src/arcade/AudioSequencer.cpp`) from the Arduino output adapter in `Hardware.cpp`. Add flash-resident original note/pattern data and a catalog-indexed track descriptor table with compile-time coverage of all 52 IDs. Update the host source list to compile the sequencer.

Specify note pitch/rest, tick duration, articulation, tempo and looping. Maintain the music transport independently of the currently audible voice: an interrupting SFX resumes the note/rest appropriate to the musical position, not a restarted phrase. Use explicit priorities, a small fixed queue, coalescing, expiration and starvation limits; no heap per note or event.

Keep one owner for LEDC channel/timer configuration and use the installed 2.x APIs. Begin with cooperative service before/after expensive work; measure worst service gaps during rendering/network menus. If cooperative scheduling cannot meet the chosen jitter budget, evaluate an independent service task or carefully bounded timer notification, with a single LEDC writer and synchronized bounded commands. Do not call flash-backed pattern code from an unsafe ISR or introduce core pinning without measurements.

Add persisted master mute, music enable and SFX enable, plus a sound-preview menu. Version the save format and test migration of existing v1/v2 records/favorites/settings. Specify lifecycle policy: pause freezes gameplay transport and silences gameplay SFX; resume restores transport; exit/restart/error clears stale cues; attract mode remains muted by default. Menu and results cues have explicit ownership.

Compose an original menu theme; distinct River Raid propulsion/fuel motifs and Keystone chase/capture motifs; genre themes with meaningful per-game phrase/rhythm variants. Deliver an explicit 52-game track mapping. Include navigation, confirm/back, start, pickup, shot, hit, explosion, warning, victory, defeat and high-score cues as relevant. Use rests and restrained registers. No copied soundtrack, simultaneous-polyphony claims or uncalibrated volume slider.

Acceptance: host tests for priority, resume position, repeated effects, starvation, overflow, mute, tempo, pause, delayed calls and rollover; all games mapped; hardware listening checklist and measured service jitter still required.

## Batch 4 — Atari preservation and optional enhancements

River Raid: retain classic generator/checkpoint/score semantics. In Enhanced mode validate continuous banks, connected safe corridors, bridge alignment, object occupancy, reachable refueling and fuel expenditure over bounded lookahead. Retry generation a fixed number of times, then use deterministic safe fallback sections. Test both speed extremes, island sides, fuel collection versus shooting and section boundaries. Add differentiated existing enemy behaviors and stronger fuel/bridge cues without obscuring terrain. Retest both players, retries and all 48 classic difficulty states.

Keystone Kapers: formalize the existing floor graph, lift stops/travel and upward escalators. Verify thief/officer transitions and reachable interception windows. Add bounded route/reaction variety with commitment periods in Enhanced mode; review hazard combinations and wraparound entry cues. Preserve classic timer penalties, roof escape, jumping/ducking controls and capture scoring by default. Improved cues and defect fixes can apply to both modes when they preserve rules.

Check original manuals before claiming fidelity. Existing research is a useful starting point but does not prove equivalence to cartridge behavior. Original artwork and melodies only.

Acceptance: separate Classic/Enhanced tests and score policy; feasible Enhanced sections; legal mall transitions; no unexplained visible jumps or unavoidable spawn collisions; existing Atari regressions retained.

## Batch 5 — full-catalog implementation checklist

Each row requires individual source inspection, justified changes or an unchanged disposition, genre-appropriate music/SFX mapping and focused regression coverage. Source paths below are `src/games/<Name>.cpp`; the first shared shooters also require `ShooterBase.cpp` and their headers. The proposed review order groups related changes into batches of roughly 4–8 games.

| ID | Game/source name | Gameplay review and candidate improvements |
| --- | --- | --- |
| 0 | SkyInvaders | Formation pressure, bottom-row fire selection, barriers, readable boss recovery. |
| 1 | SkyFighters | Distinct entry routes/roles, attack slots, bounded predictive fire. |
| 2 | StarAssault | Dive commitment, return paths, formation variety, telegraphed boss phases. |
| 3 | MeteorPatrol | Rock split clearance, spawn reaction time, shot sweeps, resource pressure. |
| 4 | NeonBreaker | Multi-impact ball collision, paddle control, reachable drops and recovery. |
| 5 | SnakeRush | Reachable food, growth/collision order, consistent turns and speed escalation. |
| 6 | TurboDodge | Lane reachability, obstacle previews, safe rows and opponent trajectories. |
| 7 | FlapEscape | Gap-to-gap reachability, responsive flap edges, collision readability. |
| 8 | PongCircuit | Sampled reaction, imperfect reflected-path prediction, capped movement. |
| 9 | BlockCascade | Rotation/lock/input timing, legal generation, drop feedback and progression. |
| 10 | MineGrid | First-open safety, explicit rules; distinguish valid boards from deduction-solvable generation. |
| 11 | MazeRunner | Maze connectivity, bounded pathfinding, pursuit/escape balance. |
| 12 | RiverLeap | Moving support consistency, reachable crossing windows and preview. |
| 13 | OrbitGuard | Attack sectors, angular response windows, wave recovery. |
| 14 | LunarLander | Stable landing/contact rules, achievable fuel/terrain budgets and cues. |
| 15 | BlastGrid | Bomb timing, safe escape paths, legal opponents and chain reactions. |
| 16 | TankArena | Terrain-aware steering, aim delay, separation and readable shots. |
| 17 | BubbleBurst | Splitting safety, collision/order robustness, varied encounter pacing. |
| 18 | GridMerge | Legal move detection, deterministic tile stream, undo and score rules. |
| 19 | MemoryMatch | Paired generation, reveal timing, fair deadlines and input feedback. |
| 20 | RobotWhack | Target dwell/response time, anti-repeat pacing, fair simultaneous prompts. |
| 21 | StackTower | Placement tolerance, consistent moving platform timing and recovery cues. |
| 22 | JumpQuest | Ground/contact rules, jump buffering/coyote time where appropriate, reachable hazards. |
| 23 | CargoShift | Replay generated solutions, undo/reset invariants and move-budget feasibility. |
| 24 | ColorCascade | Match/cascade resolution, valid moves, readable combos and deadlines. |
| 25 | VectorDrift | Momentum/turn consistency, spawn clearance, split and projectile collision. |
| 26 | BugBlaster | Enemy roles, obstacle navigation, attack budgets and boss cues. |
| 27 | CityShield | Interception feasibility, threat spacing, explosion chains and wave breathing room. |
| 28 | PrismRider | Lane transition timing, cue clarity and safe attack sequences. |
| 29 | NeonClaim | Territory closure rules, bounded enemies, fair risk/reward. |
| 30 | DotChase | Distinct pursuit roles, bounded routes, frightened behavior and escape windows. |
| 31 | TunnelDigger | Terrain-aware pursuit, bounded digging interactions and trapping/counterplay. |
| 32 | BarrelClimb | Ladders/ground reliability, jump windows, fair barrel combinations. |
| 33 | SkyJoust | Steering commitment, altitude collision, recovery and boundary awareness. |
| 34 | BurgerWorks | Floor navigation, ingredient interactions, fair pursuit and resource use. |
| 35 | BubbleQuest | Capture/release timing, platforms and reachable enemy interactions. |
| 36 | SnowSiege | Attack commitments, projectile safety, readable boss phases. |
| 37 | CanyonStrike | Terrain corridors, encounter variety, approach previews and fire fairness. |
| 38 | SolarLance | Formation roles, directional threats, attack slots and recovery. |
| 39 | FortressRun | Jump/ground rules, enemy placement, resource and boss counterplay. |
| 40 | GhoulCastle | Patrol tells, hazard combinations, reachability and recovery. |
| 41 | IronFront | Enemy roles, cover/terrain awareness, bounded aim and encounter pacing. |
| 42 | BladeDuel | Opponent windup/commit/recovery, recent-action penalties, readable counterplay. |
| 43 | ElevatorHeist | Floor/lift legality, entry safety, pursuit timing and reachable objectives. |
| 44 | AlpineRush | Obstacle preview, speed-dependent reachable gaps and course variation. |
| 45 | WildRails | Track/hazard reaction time, legal encounter combinations and pacing. |
| 46 | CrystalCourier | Route/objective reachability, pursuit windows and resource feedback. |
| 47 | NightRacer | Opponent lane intent, braking/interception fairness, safe traffic generation. |
| 48 | PinballNova | Fast ball collision, flipper response, stable contact and scoring. |
| 49 | RescueWing | Rescue approach feasibility, differentiated threats, fuel/mission pacing. |
| 50 | KeystoneKapers | Classic preservation, topology/timing validation, Enhanced chase variation. |
| 51 | RiverRaid | Classic preservation, Enhanced route/fuel validation, checkpoint and bridge robustness. |

## Batch 6 — profiling, polish and final verification

Measure simulation, AI, drawing, SPI transfer, audio service gap/cost, input latency, internal free/largest/minimum heap and stacks under maximum entity load. Existing aggregate diagnostics need attribution to AI/audio and drawing versus transfer. Establish sustainable render caps per board from real measurements; do not equate 60 Hz simulation with 60 FPS display.

Preserve internal staging buffers, allocation failure behavior and mapped/banked PSRAM distinctions. Scale particles/buffer sizes before rules; core fairness/scoring must agree across boards. Keep OTA/NVS/flash-write cache constraints in mind when choosing audio execution context. New music tables belong in flash, small transport state in internal RAM.

Add readable cues, restrained particles, score feedback and clearer instructions after gameplay changes. Recheck reduced effects, RGB332 legibility, all eight controls, pause/restart, transitions and new settings on the real renderer.

Run repeatable seeded simulations with idle, directional, alternating, firing and adversarial edge-input patterns. Record game ID, difficulty/mode, seed, tick and inputs for failures. Add tests for procedural reachability, legal AI transitions, collision edges, pause and queue overflow; use sanitizer CI where supported. Bot survival is evidence for invariants, not proof of fun.

Stress maximum entities/projectiles, many game switches, long sessions, timer rollover, allocation failures and no-PSRAM targets. WROVER missing PSRAM should retain its documented safe failure policy; WROOM should operate normally without it. Physically test all three memory configurations for sustained play and audio when hardware is available.

## Commands and delivery gates

Fresh host baseline on 2026-10-11: `python scripts/test_host.py` passed 2,223,249 assertions, including 2,080,000 lifecycle simulation ticks and 15,555 game-over/restart cycles. This verifies existing automated coverage, not human play quality or physical buzzer output. The catalog checklist was checked against stable IDs 0–51 and all listed source files exist.

Fresh unchanged-source firmware baseline on 2026-10-11: `pio run -e release -e debug -e wroom-4mb -e wroom-16mb` completed successfully for all four environments. TFT_eSPI emitted the expected unavailable-touch warning; the console uses buttons. No device upload or physical play/listening test was performed during this review.

```powershell
pio run -e release -e debug -e wroom-4mb -e wroom-16mb
python scripts/test_host.py
python scripts/verify.py
```

Use the first two for baseline and relevant batches. Use `scripts/verify.py` at final integration: it builds, runs host tests, generates previews and packages local release artifacts. Run sanitizer coverage in the existing Ubuntu workflow or a supported local runtime. No flashing or publishing is part of this planning task.

Final implementation deliverables:

- Working integrated firmware and original melody data with all 52 catalog entries preserved.
- `GAMEPLAY_AUDIT.md`: every game, source evidence, fixes/disposition and verification.
- `RESEARCH_NOTES.md`: source URL, access date, version/edition, supported fact, design choice and code informed.
- `AUDIO_GUIDE.md`: circuit assumptions/limits, ownership, settings, all-game track map and authoring guide.
- `BUILD_AND_TEST.md`: exact commands, fresh binary/slot/static RAM results, measured runtime data, seed failures and hardware checklist.

Each batch exits only after affected focused tests and the four firmware builds pass. Final acceptance requires the full catalog audit and all planned integration/tests; completion of one shared engine does not complete the catalog. Physical listening/playtesting must be labeled pending unless actually performed.

## Research checked for this plan

Access date: 2026-10-11. The following were opened during planning; original manuals and individual AI implementation references still need deeper verification during the relevant batch.

- [ESP-IDF 4.4.7 LEDC](https://docs.espressif.com/projects/esp-idf/en/v4.4.7/esp32/api-reference/peripherals/ledc.html): channel/timer configuration and single-owner concerns support the output adapter. Absolute musical transport is our design choice.
- [ESP-IDF 4.4.7 RNG](https://docs.espressif.com/projects/esp-idf/en/v4.4.7/esp32/api-reference/system/random.html): radios-off output requires entropy prerequisites for true randomness; temporary entropy source must be disabled before conflicting peripherals. Independent reproducible streams are our design choice.
- [ESP-IDF 4.4.7 external RAM](https://docs.espressif.com/projects/esp-idf/en/v4.4.7/esp32/api-guides/external-ram.html): mapped-address limits, cache restrictions and internal DMA memory support retaining existing memory architecture.
- [Game AI Pro: What is Game AI?](https://www.gameaipro.com/papers/what-is-game-ai.html): background on game AI; this overview alone does not validate specific algorithms or tuning.

Read `docs/ATARI_RESEARCH.md` and `docs/GAME_REFERENCES.md` as prior research, then re-open their primary sources before implementing fidelity-sensitive changes. Do not treat archived screenshots, secondary summaries or the existing document as proof of original mechanics.
