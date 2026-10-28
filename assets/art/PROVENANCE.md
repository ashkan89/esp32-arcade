# Artwork source and conversion

Four original atlas PNGs were generated for this repository with the built-in OpenAI image
generation tool on October 28, 2026, then inspected and copied into this directory. They are
production inputs, not screenshots of games or downloaded commercial sprite sheets.

- `environment-atlas.png`: 8×7 cells. Catalog backgrounds 0–51, followed by four UI materials.
- `sprite-atlas.png`: 8×8 cells. Ships, aliens, people, vehicles, wildlife, materials and objects.
- `supplement-atlas.png`: 8×4 cells. Snake, lander, sports paddles, food, railway, duel and maze art.
- `finishing-atlas.png`: 4×2 cells. Rear-view race cars, crouched actors, jump, home, bubble and slab.

`scripts/pack_art.py` (Pillow 12.3.0) crops the inspected atlas cells, fits sprites to a 24×24
logical canvas, preserves an alpha-derived transparent index, quantizes to fifteen opaque
colors plus transparency, and records tight bounds. Backgrounds use sixteen colors at
88×100 and scale to the display. Sprite bounds prevent letterboxing from shifting the visible
edge of a platform away from its collision surface. Runtime uses clipped nearest-neighbor
sampling of packed four-bit flash data, not PNG/JPEG decoding or per-frame allocation.

The snake-head input points downward; the renderer rotates it to the game's actual heading.
Rear-view car assets are used for perspective racing; top-view assets remain in top-view games.
The full source and its SHA-256 manifest are retained so conversion is reproducible.
Embedded output is `src/arcade/ArtData.cpp` and includes palettes and sprite bounds.
The environment generator produced six square rows and a double-height last row rather
than uniform-height cells. The measured boundaries are retained in the packer and manifest;
blindly dividing its height by seven would mix neighboring environments.

New project artwork is supplied under `LICENSE-new-code.txt` alongside the new code. This is
not a licensed commercial port or a distribution of cartridge artwork. Names and visual
references to existing genres/games do not confer third-party trademark or character rights.
The historical user-supplied sprites remain documented separately in THIRD_PARTY.md.
