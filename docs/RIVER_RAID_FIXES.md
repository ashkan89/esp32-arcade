# River Raid visual and gameplay fixes

The October 12, 2026 investigation reproduced the one-sided Enhanced layout in source:
depots were fixed at x=52, hostile objects at x=108, and enemy movement rejected any
leftward position below x=96 plus the sprite radius. The route validator only considered
the left branch. The white-line report was not reproduced on a physical LCD during this work.

## Changes

- Enhanced depots alternate between both sides, with seeded lateral variation. Short islands
  leave open water for crossing; enemy spawns alternate sides rather than depending on chance
  for balance. All generation remains seeded, bounded to four attempts, and reproducible on
  checkpoint restart. The fallback has open terrain and retains both refuelling sides.
- Reachability retains two disjoint island branches, rather than incorrectly connecting them
  through land. It includes the jet's body/nose, lateral movement at maximum forward speed,
  depot contact and the bridge approach. An explicit reconstructed route is stored only in
  host builds for the regression pilot, not in deployed games.
- Patrols can traverse open water in either direction. Activation phases are staggered per
  object; Enhanced speed grows within a small cap. Bounds include the full sprite height,
  so hulls and wings stay in water. Classic initial placements also use those bounds:
  the diagnostic found 1,682 overlapping placements across the sampled 6,144 Classic sections.
  Island branches still require going around land.
- Tankers have a cabin, funnel and low hull. Jets have separate wings, tail and canopy rather
  than the original arrow silhouette. Mirrored details stay attached. Houses use a quieter
  roof color and follow the narrowest shoreline under their full footprint.
- Missile target boxes match each sprite's height, including the tall fuel station. A shot
  is clipped at the first riverbank/island contact before targets are resolved, preventing
  hits through land. Aircraft body collision now accounts for its wings; hostile contact
  uses the target's height. Refuelling and player collisions still run when a shot is blocked.
- Rendering still crops objects, missiles and bursts behind the HUD and side borders. A
  replay checks that sparse/merged dirty transfers reconstruct the full image exactly across
  moving sprites, changed bridges, fractional scrolling and section seams.

Classic retains its historical seed, feedback sequence, difficulty progression, score values,
checkpoint behavior and two-player rules. Common sprite, shoreline and collision fixes also
apply to Classic. These are native gameplay improvements, not a claim of cartridge equivalence.

## Verification

The focused River suite passes 1,662,179 checks, including 128 seeds × 48 progression tiers,
both-sided fuel/enemy placement, deterministic retries, 10-section pilots at both speed
extremes and all four console difficulties, patrol crossing/bounds, wall-shot protection,
sprite mirroring, full sprite footprints in Classic, two-player checkpoints and 180
dirty-transfer replay frames. The pilot
disables enemies to isolate terrain, steering and refuelling; actual encounters need playtesting.
The full host suite retains its game lifecycle and effects-invariance coverage.
The final full run passed 2,282,313 main assertions and 1,688,641 focused game checks,
including 2,080,000 lifecycle ticks and 1,036,800 effects-invariance ticks. All four firmware
targets passed (`test-results/river-builds-final.log`); release application/factory packages
were refreshed in `dist/`. No hardware was flashed or release published.

Updated actual-renderer scenes are in `artifacts/atari-states.png`, including Enhanced patrol
and seam examples. Build/test logs are local ignored files under `test-results/`.
Device verification should check both modes, RGB565/RGB332 boards, steering through refuelling
side changes, high-tier patrols, bridge damage, restart and inversion. Intermittent physical
white lines or tearing remain unverified; host pixel equality cannot test SPI electrical
behavior, LCD scanout or panel timing.
