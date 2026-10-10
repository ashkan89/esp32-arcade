# On-device acceptance checklist

Status: **pending**. No serial device was available for this implementation. Record the
module marking, carrier revision, panel marking, controller/transducer part, environment,
firmware hash, power source, serial log, and test duration with each result.

## Identity and first boot

- [ ] Use `flash_id` before upload; detected flash is 16 MB, or amend configuration first.
- [ ] Cold boot, EN reset, USB reconnect, and power cycle all reach the skippable splash.
- [ ] Chip reports classic ESP32, correct revision, and reset reason; no S3 behavior is assumed.
- [ ] Physical PSRAM detection and mapped heap are logged separately. Explain an 8 MB chip
      with approximately 4 MB or less available mapped heap rather than labeling it a failure.
- [ ] GPIO16 has no conflicting external LED load. Display/control wiring matches the table.
- [ ] Panel orientation is portrait 176×220. Verify red/green/blue colors, corners, clipped
      sprites, bottom row, and readable text. Change RGB order only after a color test.
- [ ] Original GPIO26 device is untouched. Existing SD card, if connected, stays deselected.
- [ ] Flash/PSRAM failure messages are readable; `no_psram` runs all games from internal RAM.

## Controls and screens

- [ ] Exercise splash skip, every launcher selection, instructions, utilities, settings, all
      three score pages, and system information.
- [ ] Test short presses, release, long holds, repeat, simultaneous L/R, and quick alternating
      buttons. Fire held while entering a game must not auto-confirm another screen.
- [ ] Pause menu Resume/Restart/Controls/Exit work from every game. Power-ups and world state
      freeze in pause; resumed controls require release. A held pause chord must not oscillate.
- [ ] `one_button`: tap navigation, hold confirmation/pause, utilities after game eight,
      reverse movement in shooters/Breaker, snake turns, lane cycle, flap, replay and exit.
- [ ] `expanded`: GPIO21/22 directions and GPIO34 externally pulled up; no floating pauses.
- [ ] `xbox`: pair in menu, D-pad/A/B/X/Start/Xbox, held reports, 500 ms stale-input release,
      disconnect while firing, reconnect only after pause/exit, and menu rumble.

## Every game's complete lifecycle

For **each** of Sly, Sky, Star, Meteor, Breaker, Snake, Turbo, and Flap:

- [ ] Read instructions, start, use each supported action, score, and reach progression.
- [ ] Pause, wait 30 seconds, resume, restart, lose, replay, exit, and start another game.
- [ ] Verify best score updates once per completed/abandoned session; repeated exits must
      not duplicate a single Sly session in the leaderboard.
- [ ] Difficulty affects pressure, movement speed, or timing without FPS-dependent rules.

Specific checks:

- [ ] Sly: all guns, double shot, rapid, extra life, shield, eroded cover, divers, UFO warning
      then beam, invulnerability after damage, fifth-wave boss and three boss patterns.
- [ ] Sky: three flight patterns, finite squadrons, river scroll, drops, bomb recharge per
      wave, multiple boss attacks, fourth-wave boss, expanded vertical movement.
- [ ] Star: dives return, curved motion, combos expire correctly, clean-stage bonus, boss.
- [ ] Meteor: three rock sizes, splitting near full pool, single-bullet consumption, survival
      pressure and combo multiplier; screen-bottom despawn does not cause unreachable state.
- [ ] Breaker: paddle edges/center, corner bricks, armored bricks, wall contacts, dropped ball,
      wide/slow/life powers, automatic serve and reverse-direction timing in one-button mode.
- [ ] Snake: all three layouts, forbidden reversal, one turn buffered per grid step, legal
      vacated-tail movement, wall/body collisions, speed progression, near-full board.
- [ ] Turbo: each row's safe lane, two-lane movement before hazards, coins, damage grace,
      collision while moving between lanes, increasing pressure.
- [ ] Flap: release between flaps, floor/ceiling and gate edge collision, fair adjacent gaps,
      exactly one point per gate, immediate replay.

## Persistence and failure cases

- [ ] Change every setting, wait six seconds in a menu for the save message, then power cycle.
- [ ] Per-game records, last game, and Sly top five survive. Original records import once.
- [ ] Corrupt or old-format saved blob recovers to defaults without boot looping.
- [ ] Audio compiled out and muted both work; an absent transducer does not block startup.
- [ ] Failed/absent optional SD and GPIO26 device do not affect baseline play.
- [ ] Display allocation failure shows an error; snapshot allocation failure uses strips;
      NVS failure clearly reports volatile records. Do not run destructive PSRAM memory tests
      after allocations have started.

## Performance and endurance

- [ ] Capture boot full-frame/tile timing and compare calibrated tile/strip crossover.
- [ ] Measure requested 30/60 render FPS using serial frame buckets in each game's heavy
      scene. No FPS guarantee applies until these measurements pass; record sustained misses.
- [ ] Confirm simulation speed matches at 30 and 60 rendering; dropped catch-up time is low.
- [ ] Log update/render maxima, input polling gap, debounced edge-to-update maximum, heap
      minimum, and PSRAM free. Use a logic analyzer/high-speed camera for button-to-photon.
- [ ] Check effects/audio priority, mute mid-tone, pause/switch stopping sound, pitch on a
      passive transducer, and absence of speaker/buzzer driver overheating.
- [ ] Switch games at least 100 times and run a mixed session for two hours. Static game
      storage and startup buffers should not leak; optional BLE heap may vary on connection.
- [ ] Exercise repeated reset/power cycles and slow/unstable power conditions appropriate to
      the carrier. Record brownout/watchdog resets rather than concealing them.

Hardware acceptance results should be appended to `VERIFICATION.md` with evidence. Host
tests validate rules and memory guards; they do not validate a panel, wire, transducer, radio,
physical PSRAM, current consumption, or real-time performance.
