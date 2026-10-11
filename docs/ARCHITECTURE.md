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
| `src/games/<Game>.cpp` | One concrete game, local state/rules/rendering and metadata |
| `ShooterBase` | Shared projectile pools, collisions, power-ups, boss and sprite utilities |
| `Registration` / `Catalog.def` | Type-erased construction/destruction and stable catalog IDs |
| `Game` / `Campaign` | Twelve-stage victory, transitions, grace periods and bounded visual effects |
| `Hardware` | Display transfers, piezo sound, NVS persistence and measurements |
| `FrameHistory` | 8 MB PSRAM bank API; bounded sampled-frame ring |
| `Network` / `ReleasePolicy` | Setup portal, GitHub release discovery, HTTPS, inactive-slot OTA |
| `main.cpp` | Main-loop ownership and startup health acceptance |

`gameFor()` destroys the previous instance and placement-constructs the next one into a single
aligned 8 KB internal RAM arena. Returned references are invalid after the next `gameFor()` call.
Never cache inactive game pointers. Menus call cleanup before replacing/leaving a game.
There are no allocations in game update/render paths. Shared shooter utilities are not a
collection of games in one file: each shooter's enemy AI lives in its own named source file.

The main task owns buttons, console, audio, display, NVS, and banked frame history. A temporary
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
3. Append `ARCADE_GAME(MyId, MyGame, 50)` to `include/games/Catalog.def`. Use the next unused ID.
   Never reorder/renumber released IDs; scores/favorites use stable numeric slots. Current save
   capacity is 64 slots. The launcher/page count and both build environments update automatically.
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
Cargo Shift constructs later warehouses by reversing legal pushes from solved boards;
host tests replay those constructive solution witnesses. Reset recreates the same warehouse.

The firmware identity is `SKY_ARCADE_N16R8|<version>`. Opaque historical marker data is
also retained in the image so installed v2/v3 consoles can update. The OTA board ID and
firmware asset names remain stable. Game Arcade remains the console and release brand.

Settings schema 2 is a 304-byte CRC-protected NVS blob containing 64 high scores, favorites,
options and the Sky top five. Schema 1 console saves migrate in memory and are rewritten later.
Difficulty values 0–3 represent Easy, Medium (formerly Normal), Hard and Nightmare.
Existing schema 1/2 saves retain their scores, favorites and difficulty indexes. Nightmare
fits the existing options byte; no blob-layout migration is needed. Older firmware that
only accepts 0–2 cannot load a save while its difficulty byte is set to Nightmare.
Settings use the `skyarcade` namespace. If its state is absent or invalid, a valid historical
namespace state is imported and saved later; the historical state remains untouched for rollback.
Writes are deferred outside gameplay and retried after failure. Invalid data never partially
overwrites settings. Network credentials use a separate atomic JSON string in their own namespace.

Display presentation compares exact 16×8 tiles against a PSRAM snapshot. Sparse frames transfer
tiles; dense frames use 176×8 internal strips. The crossover and requested FPS cap are measured
at startup. Transfer is synchronous, with no claimed DMA overlap. Software RGB inversion is
used because the library's ILI9225 hardware-invert operation is a placeholder.
