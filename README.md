# Game Arcade — ESP32-WROVER N16R8

50 native arcade games on a 176×220 ILI9225 display, with eight buttons, passive-buzzer audio,
favorites, saved scores, difficulty settings, pause/restart menus, and a PSRAM-backed frame history.
Wi-Fi setup runs on demand; firmware updates come from
[ashkan89/esp32-arcade releases](https://github.com/ashkan89/esp32-arcade/releases).
Only `release` and `debug` environments exist, with the same complete hardware and features.

![Actual C++ game and menu renderer](artifacts/console-preview.png)

## Build and first upload

```sh
python -m pip install platformio==6.2.0
pio run -e release
pio run -e debug
pio run -e release -t upload --upload-port YOUR_PORT
pio device monitor --port YOUR_PORT --baud 115200
```

Rewire to [the new pin map](docs/HARDWARE.md) before running this firmware. The first upload
must use USB/serial so the rollback-enabled bootloader and A/B partition table are installed.
The build targets classic ESP32, 16 MB flash, and 8 MB PSRAM. Startup checks detected capacities;
wrong memory hardware stops startup and rejects a pending OTA boot.

## Controls

| Button | Menus | Games |
| --- | --- | --- |
| Up / Down | Select game or menu row | Movement / game-specific action |
| Left / Right | Launcher page; menu navigation | Movement / aiming |
| A | Confirm / play | Main action |
| B | Toggle launcher favorite; back elsewhere | Secondary action |
| Menu | System menu; pause/resume | Pause menu |
| Back | System menu / back | Pause menu with exit option |

Each game has its own controls screen. Scores and favorites save when outside gameplay.
Attract mode starts after 20 seconds idle and never records scores.
The last-run history holds up to 40 sampled frames (one every 250 ms), using banked PSRAM.
It is a visual history, not a replay of game inputs.

## Games

| Shooters / combat | Action / reflex | Puzzles / strategy |
| --- | --- | --- |
| Sky Invaders | Neon Breaker | Block Cascade |
| Sky Fighters | Snake Rush | Mine Grid |
| Star Assault | Turbo Dodge | Grid Merge |
| Meteor Patrol | Flap Escape | Memory Match |
| Orbit Guard | Pong Circuit | Cargo Shift |
| Tank Arena | River Leap | Color Cascade |
| Blast Grid | Lunar Lander | |
| Bubble Burst | Robot Whack | |
| | Stack Tower | |
| | Jump Quest | |
| | Maze Runner | |

The 25 additions are Vector Drift, Bug Blaster, City Shield, Prism Rider, Neon Claim,
Dot Chase, Tunnel Digger, Barrel Climb, Sky Joust, Burger Works, Bubble Quest, Snow Siege,
Canyon Strike, Solar Lance, Fortress Run, Ghoul Castle, Iron Front, Blade Duel,
Elevator Heist, Alpine Rush, Wild Rails, Crystal Courier, Night Racer, Pinball Nova,
and Rescue Wing. Each lives in its own source file with its own game rules.

All 50 games have twelve stages and a **Campaign Complete** ending with a score bonus.
Combat games include bosses; puzzle, territory and racing campaigns finish their own
objectives. Read the [complete game guide](docs/GAMES.md) for stage objectives and boss
schedules, and [web research](docs/GAME_REFERENCES.md) for the 25 classic inspirations.
The additions use original code and procedural artwork, with their own titles and rules.

## Wi-Fi and firmware updates

Open **Menu → Wi-Fi / Firmware Update → Wi-Fi / Repository Setup**. Join `GameArcade-Setup`
using the random key shown on the display, then open `http://192.168.4.1/`.
Enter the key exactly as shown, using uppercase letters. If your phone opens a Wi-Fi sign-in
page, use that page; otherwise open the address manually and stay connected when warned
that the setup network has no internet. Keep the console on the update menu during setup.
Enter your Wi-Fi name/password and `ashkan89/esp32-arcade` (or another public repository).
Setup supports WPA/WPA2 personal credentials, with an 8–63 character password. Credentials
stay on the console in NVS; they are never added to source or printed on serial.

Choose **Check GitHub Release**, then **Install Verified Update** if a newer stable version
is available. Wi-Fi runs during setup/check/install and shuts off afterwards. The application
downloads to the inactive 3 MB slot; HTTPS, SHA-256, size, board identity, version and ESP image
validation must pass before boot selection changes. A pending firmware must pass startup
checks and eight seconds of functioning UI before acceptance. Failure/reset before acceptance
returns to the previous healthy slot. The update menu also offers explicit previous-firmware boot.

See [OTA details and release instructions](docs/OTA.md),
[adding a game](docs/ARCHITECTURE.md), and [verification](docs/VERIFICATION.md).

```sh
python scripts/verify.py
python scripts/package_release.py
```

For the next firmware release, increase `VERSION`, commit, create the matching `vX.Y.Z` tag,
and push it. The GitHub workflow builds both environments, runs host tests, and publishes the
application binary, manifest, initial-flash image and checksums.

Boot was verified on a physical ESP32 via COM5: 16 MB flash, 8 MB PSRAM, 40 banked-history
slots and successful boot-health confirmation, with no serial errors during a 35-second capture.
Buttons, sound, Wi-Fi provisioning, history capture/playback and power-loss/rollback still need the
[hardware acceptance checks](docs/HARDWARE_CHECKLIST.md).
