# Original Sly Invaders code graph

Structural audit snapshot of `GameConsole/` at revision `5da4e3f`.

283 nodes, 515 edges, 10 detected communities. `graph.json` and `graph.html` describe the
original sources, which are retained. The new console architecture is documented separately
in `docs/ARCHITECTURE.md`.

The extractor reported syntax issues/partial extraction in `Config.h` and `Sprites.h` and
did not classify `GameConsole.ino`. All were read directly. This graph therefore is incomplete
as a representation of the original repository. No semantic LLM extraction was used; input
and output semantic token costs are zero. Do not infer physical wiring validity from edges.

## Architectural hubs

* `Game` owns display, sprite, preferences, audio, controller, entity arrays and global state.
* `Game::updateInput` dispatches screen transitions and mixes GPIO/controller reads.
* `Game::updateGame` / `checkCollisions` coordinate the majority of simulation behavior.
* `AudioEngine` and the unused `SoundEngine` represent overlapping tone implementations.

## Cross-module findings

* Display dimensions and pools are in `Config.h`, but TFT pin/frequency setup is absent from
  that header and was external to the repository.
* Controller reads happen in input, game update, and rendering. The new console publishes a
  shared input snapshot and moves optional connection operations to menus.
* Persistence is called from collision/game-state paths. New rules remain pure C++, with
  deferred writes owned by the hardware layer.

These findings are source-review conclusions; they are not invented graph relationships.

## Useful navigation queries

```sh
graphify query "Game updateInput checkCollisions audio" --budget 2000
graphify explain "Game"
graphify path "Game" "AudioEngine"
```

For current source relationships, rebuild a separate graph of `src/` and `include/` rather
than confusing this original audit snapshot with the new implementation.
