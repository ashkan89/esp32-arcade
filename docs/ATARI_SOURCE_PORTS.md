# River Raid source port and Keystone cartridge runtime (4.0.2)

This revision removes the previous two native recreations and their shared
`AtariPixels.h` artwork. Catalog IDs 50 and 51 remain stable, preserving favorites,
recents and historical score records. Old records are retained but are not evidence
of scores earned under the new implementations.

## River Raid

Source: https://github.com/athaless/RiverRaid, commit
`d6dcebe7e62e41d0a0566cb57694edd34406b7d8`.
This is a Python/Pygame remake, not the Atari cartridge source.

The replacement C++ game uses the repository's actual bank and island tables,
palette and sprite maps, imported to `include/assets/RiverSource.h`. The five-object
enemy stream, directional movement, fuel depots, single missile, acceleration and
braking, 40/80/120/30-point targets and 250-point bridge checkpoints follow its
mechanisms. Source-space coordinates remain 800×480; the playable view is scaled
to 160×180, with a separate fuel/score panel. Source rows are clipped to the field.

The uncapped Pygame event loop is replaced with the console's fixed 60 Hz update.
Horizontal movement is tuned for the physical buttons. Pixel-color collision reads
are replaced with geometry shared by rendering and collisions. Enemy placement has
a bounded search, missiles use swept collision tests, and checkpoint retries restore
the random stream. Intro interaction is A/direction to launch; B does not select a
second player. The old 48-state generator and Classic/Enhanced rules are removed.
These adaptations mean it is a source-based port, not a bit-exact Python execution
or a cycle-accurate recreation of the original River Raid cartridge.

Rebuild the imported tables (normal firmware builds do not need this checkout):

```powershell
git clone https://github.com/athaless/RiverRaid .cache/RiverRaid-source
git -C .cache/RiverRaid-source checkout d6dcebe7e62e41d0a0566cb57694edd34406b7d8
python scripts/import_atari_sources.py --river-source .cache/RiverRaid-source
```

No license file is present in that upstream revision. Imported data remains attributed
to its source and is not relicensed as original project artwork.

## Keystone Kapers

Input: the user's `roms/Keystone Kapers (USA).a26`, 4096 bytes.
SHA-256: `10fc464822fcc94d5573a772a9a4fdaf17c4d08a2e73015ab010f08ed124b02d`.
It is a fixed 4K cartridge, with reset/IRQ vectors at F000 and no bank switching.

The cartridge executes in a new bounded Atari runtime. The cycle-stepped 6502 core
is Andre Weissflog's `floooh/chips` m6502.h (zlib license retained in the header),
revision `ee88c35ad6427341aa6999c3b07233e1f8bd2396`.
CPU header SHA-256: `b3fc32a01f1f7cdc62f919ccba06cc20c2a333e7ca30c38189e31aa65868d46e`.
The importer generates a clearly marked 6507 adaptation with a 32-bit pin bus and
unused 6510 port output disabled. The upstream header and license remain intact.
WROOM puts the hot CPU dispatcher in instruction RAM; WROVER retains flash placement
because its PSRAM workaround reserves more instruction RAM.
The local adapter implements the 6507 address mirrors, 128-byte RIOT RAM,
joystick/fire/reset inputs, RIOT interval timer, WSYNC/VSYNC/VBLANK, playfield,
players, missiles, ball, copies, reflection, vertical latches, motion and collisions.
Each CPU cycle advances three TIA clocks. A frame is bounded to 24,000 CPU cycles
so invalid execution cannot trap the console indefinitely.
The raster expands constant-register spans between bus writes. WSYNC stalls and
the cartridge's two known INTIM polling loops advance hardware clocks in batches;
the terminating CPU cycles execute normally. The adapted runtime matched the original
64-bit, unaccelerated core over 5,000 frames, including CPU registers, RAM, TIA state,
beam/timer clocks and sampled pixels. Fixed reference fixtures remain in host tests.

Disassembly identified packed-BCD score RAM at 9A:9B:9C (adders FDB6/FDB8), the timer at
9D (initialization FA37), reserve officers at 96, and active-play bit 7 at 88.
The adapter reads those values for console scores and game-over integration;
it does not replace the cartridge's chase rules with a native approximation.

To reproduce the local analysis:

```powershell
python -m pip install py65==1.2.0
python scripts/import_atari_sources.py --disassemble
```

This writes reachable-code disassembly and analysis JSON to ignored `test-results/`.
Computed/indirect targets and data are not exhaustively disassembled. `py65` is
only an analysis dependency; the firmware does not use Python.

The runtime keeps a 33,600-byte indexed frame on the heap and frees it on cleanup.
The cartridge display is centered at 160×210. NTSC color conversion and TIA timing
are approximations, particularly unusual mid-scanline motion/reset behavior. This
is not a general-purpose, fully compatible Atari emulator. Cartridge audio channels
are reduced to buzzer cues; exact two-channel TIA sound is not implemented.
The console's Music/SFX controls still apply. The cartridge's own rules and initial
officer count apply; the four console difficulty tiers do not rewrite cartridge code.

**Timing limitation on COM9:** even after optimization, the sampled cartridge update
cost is about 28 ms, above the original 16.7 ms frame interval. This board therefore
plays the ROM at reduced speed. Keystone limits overdue updates to one per main loop
to avoid a catch-up backlog delaying buttons and menus. Other games retain the normal
four-update catch-up limit. This is not a claim of real-time 60 Hz cartridge emulation.
WROVER timing has not been measured on hardware.

The ROM and generated `include/generated/KeystoneRom.h` remain ignored. Builds
require the local ROM and verify its hash; a checkout without it fails with a missing
input error. CI therefore needs that private input provisioned separately. Built
firmware embeds the cartridge; no ROM-containing release is published by this task.

The old Atari mode setting is retained as **ATARI RECORDS**, choosing the historical
Classic/Enhanced score bucket. Both choices now execute the same new implementation.
Difficulty also remains a score-record category for these two games; it does not
change their source rules.

## Verification

`python scripts/test_host.py` covers cartridge boot, RAM timer initialization,
movement/jump frame captures, pause/resume, restart, game arena bounds, lifecycle,
render clipping and effects invariance. Source-port checks cover speed selection,
target score values, safe refuelling, bridge destruction and checkpoint restoration.
`artifacts/atari-preview.png` and `atari-states.png` are generated from the actual C++
renderers, including ROM execution.

`pio run -e atari-check` is a private on-device diagnostic for catalog IDs 50/51,
using normal simulation, drawing and SPI transfer. It also checks framebuffer format
transitions. Replace it with the normal `wroom-4mb` build after testing. A scripted
SPI exercise does not verify physical button feel, panel appearance, sound quality
or completion of every cartridge level.

## Installed result (2026-11-03)

Normal **4.0.2 WROOM 4 MB firmware is installed on COM9**, with whole-image
verification and boot-health confirmation. The application is 1,623,744 bytes;
SHA-256 `1fe52a1137182a1973482962d567fbedacbdc9fbda648f5d5c776053f15bc88b`.
Normal startup contains no diagnostic run and loads the existing saved settings.
Writes stayed inside app0. A subsequent read-back of the first 65,536 flash bytes
matched the pre-install backup exactly: bootloader/header, partition table, NVS
(including settings, scores, favorites and credentials) and OTA selection are unchanged.
The complete private 4 MB backup remains at `.cache/com9-before-atari-20261102.bin`.

All four final board configurations built successfully. The final host run passed
2,282,590 regular assertions plus 43,617 focused checks, including 2,080,000 lifecycle
ticks, pause/restart, the 500-frame reference fixtures and effects invariance.
The 5,000-frame CPU adaptation comparison also passed against the original
unaccelerated 64-bit core, including CPU register state and hardware clock phases.

The on-device diagnostic exercised **two games**, IDs 50 and 51, for 180 simulation
updates and 60 actual draw/SPI presentations each. Its `games=52` banner is the
catalog size, not a claim of 52-game device coverage for this revision.

| Game | Maximum update | Maximum draw | Maximum transfer | Heap after cleanup |
| --- | ---: | ---: | ---: | ---: |
| Keystone Kapers | 27,969 µs | 7,380 µs | 43,040 µs | 150,660 bytes |
| River Raid | 339 µs | 4,234 µs | 42,846 µs | 150,660 bytes |

Ten RGB565/RGB332 allocation transitions passed. Minimum observed heap was 111,828
bytes and minimum stack headroom was 4,728 bytes. These are sampled diagnostics,
not a sustained FPS guarantee. The measured Keystone timing limitation above remains.
Physical button operation, LCD legibility, listening and complete playthroughs still
require manual acceptance.

Evidence: `artifacts/com9-atari-cpu32.log`, `com9-atari-normal.log` and its flash log;
`test-results/atari-final-builds.log`, `atari-final-host.log`,
`atari-cpu32-equivalence.log`, `atari-final-images.json` and
`atari-protected-verification.json`. Raw backups and cartridge data remain private,
outside version control. No public release was created.
