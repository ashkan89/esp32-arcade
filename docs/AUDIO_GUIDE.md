# Audio guide

The documented circuit is a passive piezo on GPIO27 with a small series resistor. The implementation emits one square-wave pitch at a time through LEDC channel 0 / timer 0, using the installed Arduino-ESP32 2.0.17 tone API. Duty is the tone API’s approximately 50% setting. There is no calibrated volume control or simultaneous polyphony. An active buzzer substituted on the actual board will not reproduce these pitched tracks.

## Controls and lifecycle

System menu → Settings has master Sound, Music, SFX, Atari Classic/Enhanced, and Sound Preview. Settings scroll to show all thirteen rows. In Preview, L/R selects the 52 game tracks or menu theme, A plays a pickup cue, and MENU/BACK returns. The stored master flag still governs music and effects. Save format v3 migrates v1/v2 without erasing favorites or historical records.

Music plays in the launcher/settings and active games. Pause (including controls opened from pause) freezes the gameplay transport and clears gameplay effects; resume continues at the same musical position. Attract is muted. Results have victory/defeat/high-score cues and no looping gameplay music. Restart, exit, track changes and update actions clear stale events. Only Hardware.cpp writes LEDC; Display’s cooperative callback services the same owner between strips, without another task or ISR.

## Composition and arbitration

Original phrases in AudioSequencer.cpp consist of `Note { hz, units, gatePercent }`. Hz 0 is a rest; one unit is a quarter of the descriptor beat duration. A track supplies flash notes, count, variant, beat milliseconds and name. Genre themes have descriptor-specific tempo, register, accents and duration variations. Keystone and River Raid have dedicated phrases. Every phrase has rests and articulation gaps.

Transport is absolute elapsed musical time, independent of the SFX voice. A late update finds the current note in a bounded phrase scan instead of stretching every missed deadline. SFX return to the appropriate current note/rest. Note transitions are still limited by real service calls; hardware jitter is not yet measured.

Priorities: move 1; shot 2; confirm/back 3; pickup/start 4; hit/explosion 5; warning 6; wave 7; defeat/victory/high-score 8. Four queued events are coalesced, stale events expire at 500 ms, and low-priority overflow is dropped. Shot repeats are limited to 120 ms; navigation to 70 ms; other repeats to 180 ms. A 50 ms music recovery interval between ordinary effects prevents gunfire from monopolizing the voice. Critical cues may interrupt it.

## Track map

All names and note data are original to this implementation. Descriptors are catalog indexed and their inferred array length is compile-time checked against GameCount + the menu theme.

| ID | Game | Track | Phrase / variant | Beat ms |
| --- | --- | --- | --- | --- |
| 0 | SkyInvaders | CITY WATCH | space / 0 | 420 |
| 1 | SkyFighters | SKY SIGNAL | space / 1 | 400 |
| 2 | StarAssault | SWARM DANCE | space / 2 | 380 |
| 3 | MeteorPatrol | ROCK PULSE | rocks / 0 | 440 |
| 4 | NeonBreaker | NEON RALLY | bounce / 0 | 440 |
| 5 | SnakeRush | COIL WALK | chase / 0 | 460 |
| 6 | TurboDodge | OPEN LANE | road / 0 | 360 |
| 7 | FlapEscape | WINGBEAT | flight / 1 | 480 |
| 8 | PongCircuit | CIRCUIT SERVE | bounce / 1 | 460 |
| 9 | BlockCascade | FALLING GLASS | puzzle / 0 | 520 |
| 10 | MineGrid | QUIET GRID | puzzle / 1 | 560 |
| 11 | MazeRunner | MAZE FOOTSTEPS | chase / 1 | 480 |
| 12 | RiverLeap | RIVER STEPS | bounce / 2 | 480 |
| 13 | OrbitGuard | ORBIT CLOCK | space / 3 | 420 |
| 14 | LunarLander | SOFT LANDING | flight / 2 | 520 |
| 15 | BlastGrid | FUSE DANCE | action / 0 | 440 |
| 16 | TankArena | STEEL PATROL | action / 1 | 440 |
| 17 | BubbleBurst | BUBBLE HOP | bounce / 3 | 460 |
| 18 | GridMerge | MERGE BLOOM | puzzle / 2 | 540 |
| 19 | MemoryMatch | PAIR WALTZ | puzzle / 3 | 560 |
| 20 | RobotWhack | ROBOT TAPS | bounce / 4 | 420 |
| 21 | StackTower | TOWER TICKS | puzzle / 4 | 480 |
| 22 | JumpQuest | CLOUD CLIMB | flight / 3 | 440 |
| 23 | CargoShift | CARGO STEPS | puzzle / 5 | 580 |
| 24 | ColorCascade | COLOR FALL | puzzle / 6 | 520 |
| 25 | VectorDrift | VECTOR THRUM | rocks / 1 | 400 |
| 26 | BugBlaster | BUG MARCH | space / 4 | 400 |
| 27 | CityShield | CITY BEACON | space / 5 | 440 |
| 28 | PrismRider | PRISM DRIVE | road / 1 | 380 |
| 29 | NeonClaim | CLAIM LINES | puzzle / 7 | 500 |
| 30 | DotChase | DOT PATROL | chase / 2 | 420 |
| 31 | TunnelDigger | TUNNEL SHUFFLE | chase / 3 | 460 |
| 32 | BarrelClimb | BARREL STEPS | action / 2 | 460 |
| 33 | SkyJoust | SKY SPURS | flight / 4 | 420 |
| 34 | BurgerWorks | BURGER BEAT | chase / 4 | 460 |
| 35 | BubbleQuest | BUBBLE TRAIL | bounce / 5 | 440 |
| 36 | SnowSiege | SNOW MARCH | action / 3 | 440 |
| 37 | CanyonStrike | CANYON WIND | flight / 5 | 400 |
| 38 | SolarLance | SOLAR SPARK | space / 6 | 380 |
| 39 | FortressRun | FORTRESS PULSE | action / 4 | 440 |
| 40 | GhoulCastle | CASTLE SHADOW | chase / 5 | 480 |
| 41 | IronFront | IRON CADENCE | action / 5 | 420 |
| 42 | BladeDuel | BLADE ANSWER | action / 6 | 480 |
| 43 | ElevatorHeist | LIFT CIPHER | chase / 6 | 440 |
| 44 | AlpineRush | ALPINE ARC | road / 2 | 380 |
| 45 | WildRails | RAIL RHYTHM | road / 3 | 400 |
| 46 | CrystalCourier | CRYSTAL WALK | chase / 7 | 440 |
| 47 | NightRacer | NIGHT STRIDE | road / 4 | 360 |
| 48 | PinballNova | NOVA TABLE | bounce / 6 | 420 |
| 49 | RescueWing | RESCUE CALL | flight / 6 | 440 |
| 50 | KeystoneKapers | MALL PURSUIT | mall / 0 | 400 |
| 51 | RiverRaid | RIVER ENGINE | river / 0 | 380 |
| 52 | Menu | ARCADE LIGHTS | menu / 0 | 480 |

## Adding or tuning music

Add a compact constexpr Note array with nonzero durations, modest pitches, explicit rests and bounded articulation. Add/update the matching descriptor; keep stable game IDs. Do not call random gameplay state from music. Update host mapping tests if phrase constraints change. Preview the track on the actual piezo and test it with shooting, warnings and pause. Do not increase pitch/occupancy to mask display stalls; profile service gaps first.

Debug reports show maximum audio service gap, LEDC service cost, coalesced and dropped events. Listen and measure GPIO frequency/duty under maximum rendering load before selecting a tighter jitter target or a new scheduling context. Cooperative service was chosen to preserve one owner and avoid unmeasured task/core assumptions.
