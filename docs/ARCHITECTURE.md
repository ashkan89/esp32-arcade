# Architecture and expansion

The core runs gameplay at 60 fixed simulation steps/second, independently of 30/60 display
requests. Catch-up is bounded to four steps and drops excessive backlog. Hardware input is
polled every loop, debounced for 20 ms, and edges are consumed once; held buttons are suppressed
until released after screen transitions. All rendering is clipped. Menus own game lifecycle;
each game owns its rules, pools and presentation.

| Module | Responsibility |
| --- | --- |
| `Core` / `Canvas` | Portable timing, collision, pools, input, CRC saves and graphics |
| `Console` | Paged launcher, favorites, controls, pause/restart, scores/settings, update actions |
| `Library` | Fixed-capacity catalog-ID views, category/favorite/recent filters and stable sorting |
| `src/games/<Game>.cpp` | One concrete game, local state/rules/rendering and metadata |
| `ShooterBase` | Shared projectile pools, collisions, power-ups, boss and sprite utilities |
| `Registration` / `Catalog.def` | Type-erased construction/destruction and stable catalog IDs |
| `Game` / `Campaign` | Twelve-stage victory, transitions, grace periods and bounded visual effects |
| `Hardware` | Display transfers, piezo sound, NVS persistence and measurements |
| `Persistence` | Unchanged-save suppression, edit batching and write-attempt cooldown |
| `Network` / `ReleasePolicy` | Setup portal, GitHub release discovery, HTTPS, inactive-slot OTA |
| `main.cpp` | Main-loop ownership and startup health acceptance |

`gameFor()` destroys the previous instance and placement-constructs the next one into a single
aligned 8 KB internal RAM arena. Returned references are invalid after the next `gameFor()` call.
Never cache inactive game pointers. Menus call cleanup before replacing/leaving a game.
There are no allocations in game update/render paths. Shared shooter utilities are not a
collection of games in one file: each shooter's enemy AI lives in its own named source file.

The main task owns buttons, console, audio, display and NVS. A temporary
network task runs HTTPS/OTA on core 0 while the update screen remains responsive; game/menu
actions are frozen for that job. It never draws or changes console state. Status uses a short
critical section and completion/progress use atomics. The task deletes itself after the job.
No display/audio worker tasks or BLE services exist.

## Add a game

1. Create `src/games/MyGame.cpp`, include `games/Registration.h`, and implement `Game` or `TickGame`.
   Implement `start`, `simulate` (or `update`), `render`, `activeEntities`, and cleanup of owned
   resources. Start must reset every state field and call `baseStart(options)`.
2. Export `GameEntry describeMyGame()` returning `gameEntry<MyGame>({...})` with title, description,
   six short instruction lines (at most 27 characters), accent, icon ID, and required-button mask.
3. Append `ARCADE_GAME(MyId, MyGame, 52)` to `include/games/Catalog.def`. Use the next unused ID.
   Never reorder/renumber released IDs; scores/favorites use stable numeric slots. Current save
   capacity is 64 slots. The launcher/page count and all build environments update automatically.
   Assign the new ID a category in `Library.cpp` and deliberately update the 52-game audit
   guard in `scripts/catalog_report.py` when expanding the shipped catalog.
4. Add meaningful rule tests and run `python scripts/verify.py`. Raise the arena size deliberately
   if the compile-time `sizeof` guard rejects your game. Test pause, resume, restart, boundaries,
   pool capacity, deterministic seeds and game-over behavior.

For a staged action game, derive `CampaignGame` in `games/Campaign.h`, implement `setupStage`,
`play`, `render` and `activeEntities`; call `clearStage()` when its objective is complete.
It waits 90 ticks, then advances and resets stage time and grace. Boss rules stay in the game.
`bossHealth()` and `bossEncounter()` expose read-only state. Puzzle/territory/racing games
complete their normal objectives instead of an unrelated boss minigame.

`advanceStage()` grants the completion bonus once and stops simulation after stage 12.
`campaignComplete()` distinguishes victory from failure; score-based games use `progressTo()`.
Callers must stop immediately on `ended`, avoiding board generation after victory.
Host-only named friend checks exercise private rules and compile out of firmware.
Keystone Kapers and River Raid instead derive from `TickGame`, expose `stageLimit() == 0`
and `ownsHud() == true`, and implement continued score play directly. The console
preserves their full-frame HUDs; their fixed four starting lives override campaign tuning.
The new IDs are 50 and 51. River Raid maintains two small player records for alternating
play, restoring a player's checkpoint seed rather than retaining whole inactive simulations.
Cargo Shift constructs later warehouses by reversing legal pushes from solved boards;
host tests replay those constructive solution witnesses. Reset recreates the same warehouse.

The WROVER firmware identity is `SKY_ARCADE_N16R8|<version>`. Opaque historical marker data is
also retained in the image so installed v2/v3 consoles can update. The OTA board ID and
firmware asset names remain stable. Game Arcade remains the console and release brand.
WROOM targets use separate `SKY_ARCADE_WROOM_N4|` / `SKY_ARCADE_WROOM_N16|` identities,
manifests and assets; see [OTA.md](OTA.md).

Settings schema 4 is a 2,364-byte CRC-protected NVS blob containing 64 aggregate high scores,
Classic/Enhanced records by difficulty, favorites, options, eight recent IDs and the Sky top five. Schema 1/2/3
console saves migrate in memory and are rewritten later. An older decoder cannot load v4;
see REDESIGN_CHECKPOINT.md before a deliberate downgrade.
Difficulty values 0–3 represent Easy, Medium (formerly Normal), Hard and Nightmare.
Existing schema 1/2 saves retain their scores, favorites and difficulty indexes. Nightmare
fits the existing options byte. Older firmware that
only accepts 0–2 cannot load a save while its difficulty byte is set to Nightmare.
Settings use the `skyarcade` namespace. If its state is absent or invalid, a valid historical
namespace state is imported and saved later; the historical state remains untouched for rollback.
Writes are deferred outside gameplay/attract, batched after two quiet seconds and limited to one
attempt per minute, including retries. A RAM snapshot suppresses unchanged or reverted saves;
booting with defaults does not save the state blob until a real change. Invalid data never partially
overwrites settings. Network credentials use a separate atomic JSON string in their own namespace,
with identical-value suppression and a one-minute attempt limit. See FLASH_WRITES.md for power-loss
tradeoffs and remaining SDK/OTA writes.

Display presentation compares eight-row bands against a PSRAM snapshot. Dirty tile runs merge
horizontally, with a measured window-overhead/wire-cost estimate choosing runs or one trimmed
band. Dense bands stay local instead of forcing a full-screen transfer. Snapshot presentation
does not calculate CRCs. WROOM hashes native RGB332 bands before expanding changed bands through
a lookup table; its CRC remains save-format compatible. Filled rectangles use bulk row writes.
Transfer stays synchronous at 20 MHz, with no claimed DMA overlap. Software RGB inversion is
used because the library's ILI9225 hardware-invert operation is a placeholder.
Simulation and presentation use shared-phase fractional timing for exactly 60 Hz simulation
and 30/60 Hz render deadlines. Catch-up remains bounded, and missed render deadlines skip
obsolete slots. Sustained workload can lower the current screen's 60 FPS request to 30;
changing screens retries the requested rate. Frame history and its capture work have been removed.
Diagnostics display measured
presentation FPS and report missed deadlines in profiling builds. See RENDERING.md for limits.
WROOM keeps RGB332 in internal RAM without a display snapshot.
The updater streams filtered JSON to avoid full-body allocation
and dangling zero-copy strings. Its buffered HTTP JSON adapter has host regression tests.
